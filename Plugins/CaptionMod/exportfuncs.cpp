#include <metahook.h>
#include "exportfuncs.h"
#include "privatefuncs.h"

//Steam API
#include "steam_api.h"

//Viewport
#include <VGUI/VGUI.h>
#include "Viewport.h"

cl_enginefunc_t gEngfuncs;

cvar_t *al_enable = NULL;
cvar_t *cap_debug = NULL;
cvar_t *cap_enabled = NULL;
cvar_t *cap_max_distance = NULL;
cvar_t *cap_netmessage = NULL;
cvar_t *cap_hudmessage = NULL;
cvar_t *cap_newchat = NULL;
cvar_t *hud_saytext = NULL;
cvar_t *hud_saytext_time = NULL;

static CDictionary *m_SentenceDictionary = NULL;
static qboolean m_bSentenceSound = false;
static float m_flSentenceDuration = 0;
int m_iIntermission = 0;

void *GameViewport = NULL;
int *g_iVisibleMouse = NULL;
void *gHud = NULL;

HWND g_MainWnd = NULL;
WNDPROC g_MainWndProc = NULL;

void *NewClientFactory(void)
{
	return Sys_GetFactoryThis();
}

int HUD_VidInit(void)
{
	int result = gExportfuncs.HUD_VidInit();

	if(g_pViewPort)
		g_pViewPort->VidInit();

	return result;
}

int HUD_Redraw(float time, int intermission)
{
	m_iIntermission = intermission;

	return gExportfuncs.HUD_Redraw(time, intermission);
}

void HUD_Shutdown(void)
{
	Client_UninstallHooks();

	gExportfuncs.HUD_Shutdown();
}

void HUD_Frame(double time)
{
	//Update resolution?
	g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, NULL, NULL);

	if(g_pViewPort)
		g_pViewPort->Think();

	gExportfuncs.HUD_Frame(time);
}

void IN_MouseEvent(int mstate)
{
	if (g_iVisibleMouse && vgui::surface()->IsCursorVisible())
	{
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;

		gExportfuncs.IN_MouseEvent(mstate);

		*g_iVisibleMouse = iVisibleMouse;
	}
	else
	{
		gExportfuncs.IN_MouseEvent(mstate);
	}
}

void IN_Accumulate(void)
{
	if (g_iVisibleMouse && vgui::surface()->IsCursorVisible())
	{
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;

		gExportfuncs.IN_Accumulate();

		*g_iVisibleMouse = iVisibleMouse;
	}
	else
	{
		gExportfuncs.IN_Accumulate();
	}
}

void CL_CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	if (g_iVisibleMouse && vgui::surface()->IsCursorVisible())
	{
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;

		gExportfuncs.CL_CreateMove(frametime, cmd, active);

		*g_iVisibleMouse = iVisibleMouse;
	}
	else
	{
		gExportfuncs.CL_CreateMove(frametime, cmd, active);
	}
}

void Cap_Reload_f(void)
{
	if (g_pViewPort)
	{
		g_pViewPort->LoadBaseDictionary();

		auto levelname = gEngfuncs.pfnGetLevelName();
		if (levelname[0])
		{
			std::string name = levelname;
			name = name.substr(0, name.length() - 4);
			name += "_dictionary.csv";

			g_pViewPort->LoadCustomDictionary(name.c_str());

			if (0 != strcmp(m_szCurrentLanguage, "english"))
			{
				name = levelname;
				name = name.substr(0, name.length() - 4);
				name += "_dictionary_";
				name += m_szCurrentLanguage;
				name += ".csv";

				g_pViewPort->LoadCustomDictionary(name.c_str());
			}

			g_pViewPort->LinkDictionary();
		}
	}
}

void SvClient_StartWave(const char *name, float duration)
{
	if (!g_pViewPort)
		return;

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

	if (cap_debug && cap_debug->value)
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

int __fastcall ScClient_FindSoundEx(void *pthis, int, const char *sound)
{
	auto result = gPrivateFuncs.ScClient_FindSoundEx(pthis, 0, sound);

	if (result)
	{
		int duration = 0;
		gPrivateFuncs.FMOD_Sound_getLength(result, &duration, 1);
		SvClient_StartWave(sound, (float)duration / 1000.0f);
	}

	return result;
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	if(g_pViewPort)
		g_pViewPort->Init();

	al_enable = gEngfuncs.pfnGetCvarPointer("al_enable");

	hud_saytext_time = gEngfuncs.pfnGetCvarPointer("hud_saytext_time");
	if(!hud_saytext_time)
		hud_saytext_time = gEngfuncs.pfnRegisterVariable("hud_saytext_time", "10.0f", FCVAR_CLIENTDLL);

	hud_saytext = gEngfuncs.pfnGetCvarPointer("hud_saytext");
	if (!hud_saytext)
		hud_saytext = gEngfuncs.pfnRegisterVariable("hud_saytext", "1", FCVAR_CLIENTDLL);

	cap_debug = gEngfuncs.pfnRegisterVariable("cap_debug", "0", FCVAR_CLIENTDLL);
	cap_enabled = gEngfuncs.pfnRegisterVariable("cap_enabled", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_max_distance = gEngfuncs.pfnRegisterVariable("cap_max_distance", "1500", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_netmessage = gEngfuncs.pfnRegisterVariable("cap_netmessage", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_hudmessage = gEngfuncs.pfnRegisterVariable("cap_hudmessage", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_newchat = gEngfuncs.pfnRegisterVariable("cap_newchat", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	
	gEngfuncs.pfnAddCommand("cap_version", Cap_Version_f);
	gEngfuncs.pfnAddCommand("cap_reload", Cap_Reload_f);

	gPrivateFuncs.MessageMode_f = g_pMetaHookAPI->HookCmd("messagemode", MessageMode_f);
	gPrivateFuncs.MessageMode2_f = g_pMetaHookAPI->HookCmd("messagemode2", MessageMode2_f);

	//Fix SvClient Portal Rendering Confliction
	if (g_bIsSvenCoop)
	{
		gPrivateFuncs.fmodex = GetModuleHandleA("fmodex.dll");
		Sig_FuncNotFound(fmodex);

		gPrivateFuncs.FMOD_Sound_getLength = (decltype(gPrivateFuncs.FMOD_Sound_getLength))GetProcAddress(gPrivateFuncs.fmodex, "?getLength@Sound@FMOD@@QAG?AW4FMOD_RESULT@@PAII@Z");
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
			aud_sfxcache_t *asc = (aud_sfxcache_t *)gPrivateFuncs.S_LoadSound(sfx, NULL);
			//not a voice sound
			if(asc && asc->length != 0x40000000)
			{
				flDuration = (float)asc->length / asc->speed;
			}
		}
		else
		{
			sfxcache_t *sc = gPrivateFuncs.S_LoadSound(sfx, NULL);
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
	if (!g_pViewPort)
		return;

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

	if(cap_debug && cap_debug->value)
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
	if (!g_pViewPort)
		return;

	CDictionary *Dict = g_pViewPort->FindDictionary(name, DICT_SENTENCE);	

	if(!Dict && (name[0] == '!' || name[0] == '#'))
	{
		//skip ! and # then search again
		Dict = g_pViewPort->FindDictionary(name + 1);
	}

	if(cap_debug && cap_debug->value)
	{
		gEngfuncs.Con_Printf((Dict) ? "CaptionMod: SENTENCE [%s] found.\n" : "CaptionMod: SENTENCE [%s] not found.\n", name);
	}

	m_SentenceDictionary = Dict;
}

//2015-11-26 fixed, to support !SENTENCE and #SENTENCE
void S_EndSentence(void)
{
	if (!g_pViewPort)
		return;

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
		bool bIgnore = false;

		auto level = gEngfuncs.pfnGetLevelName();
		if (cap_max_distance && cap_max_distance->value && origin && !(origin[0] == 0 && origin[1] == 0 && origin[2] == 0) && level[0])
		{
			auto local = gEngfuncs.GetLocalPlayer();

			float dir[3];
			VectorSubtract(origin, local->origin, dir);

			auto distance = VectorLength(dir);

			if (distance > cap_max_distance->value)
				bIgnore = true;
		}

		if (!bIgnore)
		{
			if (sfx->name[0] == '!' || sfx->name[0] == '#')
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
	}

	gPrivateFuncs.S_StartDynamicSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);

	if(m_bSentenceSound)
	{
		S_EndSentence();
		m_flSentenceDuration = 0;
		m_bSentenceSound = false;
	}
}

//2015-11-26 fixed, to support !SENTENCE and #SENTENCE
//2015-11-26 added, support added up the duration of sound for zero-duration sentences
void S_StartStaticSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch)
{
	if(sfx)
	{
		bool bIgnore = false;

		auto level = gEngfuncs.pfnGetLevelName();
		if (cap_max_distance && cap_max_distance->value && origin && !(origin[0] == 0 && origin[1] == 0 && origin[2] == 0) && level[0])
		{
			auto local = gEngfuncs.GetLocalPlayer();

			float dir[3];
			VectorSubtract(origin, local->origin, dir);

			auto distance = VectorLength(dir);

			if (distance > cap_max_distance->value)
				bIgnore = true;
		}

		if (!bIgnore)
		{
			if (sfx->name[0] == '!' || sfx->name[0] == '#')
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
	}

	gPrivateFuncs.S_StartStaticSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);

	if(m_bSentenceSound)
	{
		S_EndSentence();
		m_flSentenceDuration = 0;
		m_bSentenceSound = false;
	}
}

sfx_t *S_FindName(char *name, int *pfInCache)
{
	sfx_t *sfx = gPrivateFuncs.S_FindName(name, pfInCache);;

	//we should add
	if(m_bSentenceSound && sfx)
	{
		S_StartWave(sfx);
	}
	return sfx;
}

IBaseInterface *NewCreateInterface(const char *pName, int *pReturnCode)
{
	auto fnCreateInterface = (decltype(NewCreateInterface) *)Sys_GetFactoryThis();
	auto fn = fnCreateInterface(pName, pReturnCode);
	if (fn)
		return fn;

	fnCreateInterface = (decltype(NewCreateInterface) *)GetProcAddress(g_hClientDll, CREATEINTERFACE_PROCNAME);
	fn = fnCreateInterface(pName, pReturnCode);
	if (fn)
		return fn;

	return NULL;
}

#if defined(_WIN32)
void Sys_GetRegKeyValueUnderRoot(HKEY rootKey, const char *pszSubKey, const char *pszElement, char *pszReturnString, int nReturnLength, const char *pszDefaultValue)
{
	LONG lResult;           // Registry function result code
	HKEY hKey;              // Handle of opened/created key
	char szBuff[128];       // Temp. buffer
	DWORD dwDisposition;    // Type of key opening event
	DWORD dwType;           // Type of key
	DWORD dwSize;           // Size of element data

	// Assume the worst
	Q_snprintf(pszReturnString, nReturnLength, pszDefaultValue);

	// Create it if it doesn't exist.  (Create opens the key otherwise)
	lResult = RegCreateKeyEx(
		rootKey,	// handle of open key 
		pszSubKey,			// address of name of subkey to open 
		0,					// DWORD ulOptions,	  // reserved 
		"String",			// Type of value
		REG_OPTION_NON_VOLATILE, // Store permanently in reg.
		KEY_ALL_ACCESS,		// REGSAM samDesired, // security access mask 
		NULL,
		&hKey,				// Key we are creating
		&dwDisposition);    // Type of creation

	if (lResult != ERROR_SUCCESS)  // Failure
		return;

	// First time, just set to Valve default
	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		// Just Set the Values according to the defaults
		lResult = RegSetValueEx(hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, Q_strlen(pszDefaultValue) + 1);
	}
	else
	{
		// We opened the existing key. Now go ahead and find out how big the key is.
		dwSize = nReturnLength;
		lResult = RegQueryValueEx(hKey, pszElement, 0, &dwType, (unsigned char *)szBuff, &dwSize);

		// Success?
		if (lResult == ERROR_SUCCESS)
		{
			// Only copy strings, and only copy as much data as requested.
			if (dwType == REG_SZ)
			{
				Q_strncpy(pszReturnString, szBuff, nReturnLength);
				pszReturnString[nReturnLength - 1] = '\0';
			}
		}
		else
			// Didn't find it, so write out new value
		{
			// Just Set the Values according to the defaults
			lResult = RegSetValueEx(hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, Q_strlen(pszDefaultValue) + 1);
		}
	};

	// Always close this key before exiting.
	RegCloseKey(hKey);
}

void Sys_GetRegKeyValue(char *pszSubKey, char *pszElement, char *pszReturnString, int nReturnLength, char *pszDefaultValue)
{
	Sys_GetRegKeyValueUnderRoot(HKEY_CURRENT_USER, pszSubKey, pszElement, pszReturnString, nReturnLength, pszDefaultValue);
}

#endif

char * NewV_strncpy(char *a1, const char *a2, size_t a3)
{
	char language[128] = { 0 };
	const char *lang = NULL;
	auto gamedir = gEngfuncs.pfnGetGameDirectory();
	if (CommandLine()->CheckParm("-forcelang", &lang) && lang && lang[0])
	{
		a2 = lang;
	}
	else if ((gamedir && !strcmp(gamedir, "svencoop")) || CommandLine()->CheckParm("-steamlang"))
	{
		Sys_GetRegKeyValue("Software\\Valve\\Steam", "Language", language, sizeof(language), "");
		if ((Q_strlen(language) > 0) && (Q_stricmp(language, "english")))
		{
			a2 = language;
		}
	}

	gPrivateFuncs.V_strncpy(m_szCurrentLanguage, a2, sizeof(m_szCurrentLanguage) - 1);
	m_szCurrentLanguage[sizeof(m_szCurrentLanguage) - 1] = 0;

	return gPrivateFuncs.V_strncpy(a1, a2, a3);
}

void MessageMode_f(void)
{
	if (!m_iIntermission && gEngfuncs.Cmd_Argc() == 1 && cap_newchat->value)
		return g_pViewPort->StartMessageMode();

	return gPrivateFuncs.MessageMode_f();
}

void MessageMode2_f(void)
{
	if (!m_iIntermission && gEngfuncs.Cmd_Argc() == 1 && cap_newchat->value)
		return g_pViewPort->StartMessageMode2();

	return gPrivateFuncs.MessageMode2_f();
}

bool g_bIMEComposing = false;
double g_flImeComposingTime = 0;

double GetAbsoluteTime()
{
	return gEngfuncs.GetAbsoluteTime();
}

LRESULT WINAPI VID_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND s_hLastHWnd;
	if (hWnd != s_hLastHWnd)
	{
		s_hLastHWnd = hWnd;
		vgui::input()->SetIMEWindow(hWnd);
	}

	switch (uMsg)
	{
	case WM_SYSCHAR:
	case WM_CHAR:
	{
		if (g_bIMEComposing)
			return 1;

		break;
	}
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		if (wParam == VK_BACK)
		{
			if (g_bIMEComposing)
				return 1;
		}

		break;
	}
	case WM_INPUTLANGCHANGE:
	{
		vgui::input()->OnInputLanguageChanged();
		//break;
		return 1;
	}

	case WM_IME_STARTCOMPOSITION:
	{
		g_bIMEComposing = true;
		g_flImeComposingTime = GetAbsoluteTime();
		vgui::input()->OnIMEStartComposition();
		return 1;
	}

	case WM_IME_COMPOSITION:
	{
		int flags = (int)lParam;
		vgui::input()->OnIMEComposition(flags);
		return 1;
	}

	case WM_IME_ENDCOMPOSITION:
	{
		g_bIMEComposing = false;
		g_flImeComposingTime = GetAbsoluteTime();
		vgui::input()->OnIMEEndComposition();
		return 1;
	}

	case WM_IME_NOTIFY:
	{
		switch (wParam)
		{
		case IMN_OPENCANDIDATE:
		{
			vgui::input()->OnIMEShowCandidates();
			return 1;
		}

		case IMN_CHANGECANDIDATE:
		{
			vgui::input()->OnIMEChangeCandidates();
			return 1;
		}

		case IMN_CLOSECANDIDATE:
		{
			vgui::input()->OnIMECloseCandidates();
			//break;
			return 1;
		}

		case IMN_SETCONVERSIONMODE:
		case IMN_SETSENTENCEMODE:
		case IMN_SETOPENSTATUS:
		{
			vgui::input()->OnIMERecomputeModes();
			break;
		}

		case IMN_CLOSESTATUSWINDOW:
		case IMN_GUIDELINE:
		case IMN_OPENSTATUSWINDOW:
		case IMN_SETCANDIDATEPOS:
		case IMN_SETCOMPOSITIONFONT:
		case IMN_SETCOMPOSITIONWINDOW:
		case IMN_SETSTATUSWINDOWPOS:
		{
			break;
		}
		}

		break;
	}

	case WM_IME_SETCONTEXT:
	{
		lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
		lParam &= ~ISC_SHOWUIGUIDELINE;
		lParam &= ~ISC_SHOWUIALLCANDIDATEWINDOW;
		break;
	}

	case WM_IME_CHAR:
	{
		return 0;
	}
	}

	return CallWindowProc(g_MainWndProc, hWnd, uMsg, wParam, lParam);
}