#include <metahook.h>
#include "exportfuncs.h"
#include "engfuncs.h"

//Steam API
#include "steam_api.h"

//Viewport
#include <VGUI/VGUI.h>
#include "Viewport.h"

#include <intrin.h>

cl_enginefunc_t gEngfuncs;

cvar_t *al_enable = NULL;
cvar_t *cap_show = NULL;
cvar_t *cap_netmessage = NULL;

static CDictionary *m_SentenceDictionary = NULL;
static qboolean m_bSentenceSound = false;
static float m_flSentenceDuration = 0;

void *NewClientFactory(void)
{
	return Sys_GetFactoryThis();
}

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(&gEngfuncs, pEnginefuncs, sizeof(gEngfuncs));

	pEnginefuncs->pfnTextMessageGet = pfnTextMessageGet;

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}

int HUD_VidInit(void)
{
	int result = gExportfuncs.HUD_VidInit();

	g_pViewPort->VidInit();

	return result;
}

void Cap_Version_f(void)
{
	gEngfuncs.Con_Printf("%s\n", CAPTION_MOD_VERSION);
}

void SvClient_StartWave(const char *name, float duration)
{
	if (m_bSentenceSound && m_SentenceDictionary)
	{
		if (m_SentenceDictionary->m_flDuration <= 0)
		{
			float flDuration = duration;
			if (flDuration > 0)
			{
				m_flSentenceDuration += flDuration;
			}
		}
	}

	CDictionary *Dict = g_pViewPort->FindDictionary(name, DICT_SOUND);

	if (cap_show && cap_show->value)
	{
		gEngfuncs.Con_Printf((Dict) ? "CaptionMod: Sound [%s] found.\n" : "CaptionMod: Sound [%s] not found.\n", name);
	}

	if (!Dict)
		return;

	//Get duration for zero-duration
	if (Dict->m_flDuration <= 0)
	{
		float flDuration = duration;
		if (flDuration > 0)
		{
			Dict->m_flDuration = flDuration;
		}
	}

	if (Dict->m_flDuration > 0)
	{
		m_flSentenceDuration += Dict->m_flDuration;
	}

	g_pViewPort->StartSubtitle(Dict);
}

int __fastcall SvClient_FindSoundEx(int pthis, int, const char *sound)
{
	auto result = gCapFuncs.SvClient_FindSoundEx(pthis, 0, sound);

	if (result)
	{
		int duration = 0;
		gCapFuncs.FMOD_Sound_getLength(result, &duration, 1);
		SvClient_StartWave(sound, (float)duration / 1000.0f);
	}

	return result;
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	g_pViewPort->Init();

	al_enable = gEngfuncs.pfnGetCvarPointer("al_enable");
	cap_show = gEngfuncs.pfnRegisterVariable("cap_show", "0", FCVAR_CLIENTDLL);
	cap_netmessage = gEngfuncs.pfnRegisterVariable("cap_netmessage", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	gEngfuncs.pfnAddCommand("cap_version", Cap_Version_f);

	if (g_EngineType == ENGINE_SVENGINE)
	{
		gCapFuncs.fmodex = GetModuleHandleA("fmodex.dll");
		Sig_FuncNotFound(fmodex);

		gCapFuncs.FMOD_Sound_getLength = (decltype(gCapFuncs.FMOD_Sound_getLength))GetProcAddress(gCapFuncs.fmodex, "?getLength@Sound@FMOD@@QAG?AW4FMOD_RESULT@@PAII@Z");
		Sig_FuncNotFound(FMOD_Sound_getLength);
	}
}

float S_GetDuration(sfx_t *sfx)
{
	float flDuration = 0;

	if(sfx->name[0] != '*' && sfx->name[0] != '?')
	{
		//MetaAudio is active? use MetaAudio's structs
		if(al_enable && al_enable->value)
		{
			aud_sfxcache_t *asc = (aud_sfxcache_t *)gCapFuncs.S_LoadSound(sfx, NULL);
			//not a voice sound
			if(asc && asc->length != 0x40000000)
			{
				flDuration = (float)asc->length / asc->speed;
			}
		}
		else
		{
			sfxcache_t *sc = gCapFuncs.S_LoadSound(sfx, NULL);
			//not a voice sound
			if(sc && sc->length != 0x40000000)
			{
				flDuration = (float)sc->length / sc->speed;
			}
		}
	}
	return flDuration;
}

//2015-11-26 added, support added up the duration of sound for zero-duration sentences
void S_StartWave(sfx_t *sfx)
{
	const char *name = sfx->name;

	if(!Q_strnicmp(name, "sound/", 6))
		name += 6;
	else if(!Q_strnicmp(name + 1, "sound/", 6))
		name += 7;

	if(m_bSentenceSound && m_SentenceDictionary)
	{
		if(m_SentenceDictionary->m_flDuration <= 0)
		{
			float flDuration = S_GetDuration(sfx);
			if(flDuration > 0)
			{
				m_flSentenceDuration += flDuration;
			}
		}
	}

	CDictionary *Dict = g_pViewPort->FindDictionary(name, DICT_SOUND);

	if(cap_show && cap_show->value)
	{
		gEngfuncs.Con_Printf((Dict) ? "CaptionMod: Sound [%s] found.\n" : "CaptionMod: Sound [%s] not found.\n", name);
	}

	if(!Dict)
		return;

	//Get duration for zero-duration
	if(Dict->m_flDuration <= 0)
	{
		float flDuration = S_GetDuration(sfx);
		if(flDuration > 0)
		{
			Dict->m_flDuration = flDuration;
		}
	}

	if(Dict->m_flDuration > 0)
	{
		m_flSentenceDuration += Dict->m_flDuration;
	}

	g_pViewPort->StartSubtitle(Dict);
}

void S_StartSentence(const char *name)
{
	CDictionary *Dict = g_pViewPort->FindDictionary(name, DICT_SENTENCE);	

	if(!Dict && (name[0] == '!' || name[0] == '#'))
	{
		//skip ! and # then search again
		Dict = g_pViewPort->FindDictionary(name + 1);
	}

	if(cap_show && cap_show->value)
	{
		gEngfuncs.Con_Printf((Dict) ? "CaptionMod: SENTENCE [%s] found.\n" : "CaptionMod: SENTENCE [%s] not found.\n", name);
	}

	m_SentenceDictionary = Dict;
}

//2015-11-26 fixed, to support !SENTENCE and #SENTENCE
void S_EndSentence(void)
{
	if(!m_SentenceDictionary)
		return;

	//use the total duration we added up before
	if(m_SentenceDictionary->m_flDuration <= 0 && m_flSentenceDuration > 0)
	{
		m_SentenceDictionary->m_flDuration = m_flSentenceDuration;
	}

	g_pViewPort->StartSubtitle(m_SentenceDictionary);
}

//2015-11-26 fixed, to support !SENTENCE and #SENTENCE
//2015-11-26 added, support added up the duration of sound for zero-duration sentences
void S_StartDynamicSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch)
{
	if(sfx)
	{
		if(sfx->name[0] == '!' || sfx->name[0] == '#')
		{
			m_bSentenceSound = true;
			m_flSentenceDuration = 0;
			S_StartSentence(sfx->name);
		}
		else
		{
			S_StartWave(sfx);
		}
	}

	gCapFuncs.S_StartDynamicSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);

	if(m_bSentenceSound)
	{
		S_EndSentence();
		m_bSentenceSound = false;
	}
}

//2015-11-26 fixed, to support !SENTENCE and #SENTENCE
//2015-11-26 added, support added up the duration of sound for zero-duration sentences
void S_StartStaticSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch)
{
	if(sfx)
	{
		if(sfx->name[0] == '!' || sfx->name[0] == '#')
		{
			m_bSentenceSound = true;
			m_flSentenceDuration = 0;
			S_StartSentence(sfx->name);
		}
		else
		{
			S_StartWave(sfx);
		}
	}

	gCapFuncs.S_StartStaticSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);

	if(m_bSentenceSound)
	{
		S_EndSentence();
		m_bSentenceSound = false;
	}
}

sfx_t *S_FindName(char *name, int *pfInCache)
{
	sfx_t *sfx = gCapFuncs.S_FindName(name, pfInCache);;

	//we should add
	if(m_bSentenceSound && sfx)
	{
		S_StartWave(sfx);
	}
	return sfx;
}

IBaseInterface *NewCreateInterface(const char *pName, int *pReturnCode)
{
	//MessageBoxA(NULL, pName, "NewCreateInterface", 0);

	auto fnCreateInterface = (decltype(NewCreateInterface) *)Sys_GetFactoryThis();
	auto fn = fnCreateInterface(pName, pReturnCode);
	if (fn)
		return fn;

	fnCreateInterface = (decltype(NewCreateInterface) *)gCapFuncs.GetProcAddress(g_hClientDll, CREATEINTERFACE_PROCNAME);
	fn = fnCreateInterface(pName, pReturnCode);
	if (fn)
		return fn;

	return NULL;
}

FARPROC WINAPI NewGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	if(hModule == g_hClientDll && (DWORD)lpProcName > 0xFFFF && !strcmp(lpProcName, CREATEINTERFACE_PROCNAME))
	{
		//MessageBoxA(NULL, lpProcName, "NewGetProcAddress", 0);
		return (FARPROC)NewCreateInterface;
	}
	return gCapFuncs.GetProcAddress(hModule, lpProcName);
}

void Steam_Init(void)
{
	gCapFuncs.szLanguage[0] = '\0';

	if(SteamAPI_Init())
	{
		g_bIsRunningSteam = SteamAPI_IsSteamRunning();
		
		if (g_bIsRunningSteam)
		{
			const char *pszLanguage = SteamApps()->GetCurrentGameLanguage();

			if (pszLanguage)
				Q_strncpy(gCapFuncs.szLanguage, pszLanguage, sizeof(gCapFuncs.szLanguage));
		}
	}

	if(!gCapFuncs.szLanguage[0])
	{
		Sys_GetRegKeyValueUnderRoot("Software\\Valve\\Steam", "Language", gCapFuncs.szLanguage, sizeof(gCapFuncs.szLanguage), "english");
	}
}