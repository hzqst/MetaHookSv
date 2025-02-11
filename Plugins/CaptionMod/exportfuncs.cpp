#include <metahook.h>
#include "exportfuncs.h"
#include "privatefuncs.h"

#include <mathlib/mathlib.h>

//Viewport
#include <IVGUI2Extension.h>
#include <VGUI/VGUI.h>
#include "Viewport.h"

#include <functional>

cl_enginefunc_t gEngfuncs = { 0 };

//Legacy MetaAudio
cvar_t *al_enable = NULL;

//LAGonauta's MetaAudio only
cvar_t *al_doppler = NULL;

cvar_t *cap_debug = NULL;
cvar_t *cap_enabled = NULL;
cvar_t *cap_max_distance = NULL;
cvar_t *cap_min_avol = NULL;
cvar_t *cap_netmessage = NULL;
cvar_t *cap_hudmessage = NULL;
cvar_t *cap_newchat = NULL;
cvar_t *hud_saytext = NULL;
cvar_t* hud_saytext_time = NULL;
cvar_t* cap_lang = NULL;

int m_iIntermission = 0;

//client.dll
void *GameViewport = NULL;
int *g_iVisibleMouse = NULL;
void *gHud = NULL;

cvar_t* cap_subtitle_prefix = NULL;
cvar_t* cap_subtitle_waitplay = NULL;
cvar_t* cap_subtitle_antispam = NULL;
cvar_t* cap_subtitle_fadein = NULL;
cvar_t* cap_subtitle_fadeout = NULL;
cvar_t* cap_subtitle_holdtime = NULL;
cvar_t* cap_subtitle_stimescale = NULL;
cvar_t* cap_subtitle_htimescale = NULL;
cvar_t* cap_subtitle_extraholdtime = NULL;

void Cap_RegisterSubtitleCvars()
{
	cap_subtitle_prefix = gEngfuncs.pfnRegisterVariable("cap_subtitle_prefix", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_waitplay = gEngfuncs.pfnRegisterVariable("cap_subtitle_waitplay", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_antispam = gEngfuncs.pfnRegisterVariable("cap_subtitle_antispam", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_fadein = gEngfuncs.pfnRegisterVariable("cap_subtitle_fadein", "0.3", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_fadeout = gEngfuncs.pfnRegisterVariable("cap_subtitle_fadeout", "0.3", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_holdtime = gEngfuncs.pfnRegisterVariable("cap_subtitle_holdtime", "10.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_stimescale = gEngfuncs.pfnRegisterVariable("cap_subtitle_stimescale", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_htimescale = gEngfuncs.pfnRegisterVariable("cap_subtitle_htimescale", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_subtitle_extraholdtime = gEngfuncs.pfnRegisterVariable("cap_subtitle_extraholdtime", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}

cl_entity_t* EngineGetViewEntity(void)
{
	return gEngfuncs.GetEntityByIndex((*cl_viewentity));
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

	if (cap_lang && 0 != strcmp(VGUI2Extension()->GetCurrentLanguage(), cap_lang->string))
	{
		gEngfuncs.Cvar_Set("cap_lang", VGUI2Extension()->GetCurrentLanguage());
	}

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

			if (0 != strcmp(VGUI2Extension()->GetCurrentLanguage(), "english"))
			{
				name = levelname;
				name = name.substr(0, name.length() - 4);
				name += "_dictionary_";
				name += VGUI2Extension()->GetCurrentLanguage();
				name += ".csv";

				g_pViewPort->LoadCustomDictionary(name.c_str());
			}

			g_pViewPort->LinkDictionary();
		}
	}
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	Cap_RegisterSubtitleCvars();

	if(g_pViewPort)
		g_pViewPort->Init();

	al_enable = gEngfuncs.pfnGetCvarPointer("al_enable");
	al_doppler = gEngfuncs.pfnGetCvarPointer("al_doppler");

	hud_saytext_time = gEngfuncs.pfnGetCvarPointer("hud_saytext_time");
	if(!hud_saytext_time)
		hud_saytext_time = gEngfuncs.pfnRegisterVariable("hud_saytext_time", "10.0f", FCVAR_CLIENTDLL);

	hud_saytext = gEngfuncs.pfnGetCvarPointer("hud_saytext");
	if (!hud_saytext)
		hud_saytext = gEngfuncs.pfnRegisterVariable("hud_saytext", "1", FCVAR_CLIENTDLL);

	cap_debug = gEngfuncs.pfnRegisterVariable("cap_debug", "0", FCVAR_CLIENTDLL);
	cap_enabled = gEngfuncs.pfnRegisterVariable("cap_enabled", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_max_distance = gEngfuncs.pfnRegisterVariable("cap_max_distance", "1500", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_min_avol = gEngfuncs.pfnRegisterVariable("cap_min_avol", "0.25", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_netmessage = gEngfuncs.pfnRegisterVariable("cap_netmessage", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_hudmessage = gEngfuncs.pfnRegisterVariable("cap_hudmessage", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cap_newchat = gEngfuncs.pfnRegisterVariable("cap_newchat", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	
	gEngfuncs.pfnAddCommand("cap_version", Cap_Version_f);
	gEngfuncs.pfnAddCommand("cap_reload", Cap_Reload_f);

	gPrivateFuncs.MessageMode_f = g_pMetaHookAPI->HookCmd("messagemode", MessageMode_f);
	gPrivateFuncs.MessageMode2_f = g_pMetaHookAPI->HookCmd("messagemode2", MessageMode2_f);

	cap_lang = gEngfuncs.pfnRegisterVariable("cap_lang", VGUI2Extension()->GetCurrentLanguage(), FCVAR_CLIENTDLL | FCVAR_USERINFO);
}

float S_GetDuration(sfx_t *sfx)
{
	float flDuration = 0;

	if(sfx->name[0] != '*' && sfx->name[0] != '?')
	{
		if(al_enable && al_enable->value)
		{
			//Legacy MetaAudio is active? use MetaAudio's structs
			aud_sfxcache_t *asc = (aud_sfxcache_t *)gPrivateFuncs.S_LoadSound(sfx, NULL);
			//not a voice sound
			if(asc && asc->length != 0x40000000)
			{
				flDuration = (float)asc->length / asc->speed;
			}
		}
		else if (al_doppler && al_doppler->value)
		{
			//LAGonauta's MetaAudio is active? use MetaAudio's structs

			aud_sfxcache_LAGonauta_t *asc = (aud_sfxcache_LAGonauta_t *)gPrivateFuncs.S_LoadSound(sfx, NULL);
			//not a voice sound
			if(asc && asc->length != 0x40000000)
			{
				flDuration = (float)asc->length / asc->samplerate;
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

void S_StartWave(sfx_t *sfx, float distance, float avol)
{
	if (!g_pViewPort)
		return;

	const char *name = sfx->name;

	if(!Q_strnicmp(name, "sound/", 6))
		name += 6;
	else if(!Q_strnicmp(name + 1, "sound/", 6))
		name += 7;

	auto pDict = g_pViewPort->FindDictionary(name, DICT_SOUND);

	if(cap_debug && cap_debug->value)
	{
		if (pDict)
		{
			gEngfuncs.Con_Printf("CaptionMod: Sound [%s] found. dist: %.2f, avol: %.2f\n", name, distance, avol);
		}
		else
		{
			gEngfuncs.Con_Printf("CaptionMod: Sound [%s] not found.\n", name);
		}
	}

	if (!pDict)
	{
		return;
	}

	if (!pDict->m_bIgnoreDistanceLimit && cap_max_distance && cap_max_distance->value > 0 && distance > cap_max_distance->value)
	{
		return;
	}

	if (!pDict->m_bIgnoreVolumeLimit && cap_min_avol && cap_min_avol->value > 0 && avol < cap_min_avol->value)
	{
 		return;
	}

	float duration;

	if(pDict->m_bOverrideDuration)
	{
		duration = pDict->m_flDuration;
	}
	else
	{
		duration = S_GetDuration(sfx);
	}

	CStartSubtitleContext StartSubtitleContext;

	g_pViewPort->StartSubtitle(pDict, duration, &StartSubtitleContext);
}

static char szsentences[] = "sound/sentences.txt";
static char voxperiod[] = "_period";
static char voxcomma[] = "_comma";

void VOX_ParseString(char *psz, char *(*rgpparseword)[CVOXWORDMAX])
{
	int i;
	int fdone = 0;
	char *pszscan = psz;
	char c;

	Q_memset(*rgpparseword, 0, sizeof(char *) * CVOXWORDMAX);

	if (!psz)
		return;

	i = 0;
	(*rgpparseword)[i++] = psz;

	while (!fdone && i < CVOXWORDMAX)
	{
		c = *pszscan;

		while (c && !(c == '.' || c == ' ' || c == ','))
			c = *(++pszscan);

		if (c == '(')
		{
			while (*pszscan != ')')
				pszscan++;

			c = *(++pszscan);

			if (!c)
				fdone = 1;
		}

		if (fdone || !c)
		{
			fdone = 1;
		}
		else
		{
			if ((c == '.' || c == ',') && *(pszscan + 1) != '\n' && *(pszscan + 1) != '\r' && *(pszscan + 1) != 0)
			{
				if (c == '.')
					(*rgpparseword)[i++] = voxperiod;
				else
					(*rgpparseword)[i++] = voxcomma;

				if (i >= CVOXWORDMAX)
					break;
			}

			*pszscan++ = 0;

			c = *pszscan;

			while (c && (c == '.' || c == ' ' || c == ','))
				c = *(++pszscan);

			if (!c)
				fdone = 1;
			else
				(*rgpparseword)[i++] = pszscan;
		}
	}
}

char *VOX_GetDirectory(char *szpath, char *psz)
{
	char c;
	int cb = 0;
	char *pszscan = psz + Q_strlen(psz) - 1;

	c = *pszscan;

	while (pszscan > psz && c != '/')
	{
		c = *(--pszscan);
		cb++;
	}

	if (c != '/')
	{
		Q_strcpy(szpath, "vox/");
		return psz;
	}

	cb = Q_strlen(psz) - cb;
	Q_memcpy(szpath, psz, cb);
	szpath[cb] = 0;
	return pszscan + 1;
}

int VOX_ParseWordParams(char *psz, voxword_t *pvoxword, int fFirst)
{
	char *pszsave = psz;
	char c;
	char ct;
	char sznum[8];
	int i;
	static voxword_t voxwordDefault;

	if (fFirst)
	{
		voxwordDefault.pitch = -1;
		voxwordDefault.volume = 100;
		voxwordDefault.start = 0;
		voxwordDefault.end = 100;
		voxwordDefault.fKeepCached = 0;
		voxwordDefault.timecompress = 0;
	}

	*pvoxword = voxwordDefault;

	c = *(psz + Q_strlen(psz) - 1);

	if (c != ')')
		return 1;

	c = *psz;

	while (!(c == '(' || c == ')'))
		c = *(++psz);

	if (c == ')')
		return 0;

	*psz = 0;
	ct = *(++psz);

	while (1)
	{
		while (ct && !(ct == 'v' || ct == 'p' || ct == 's' || ct == 'e' || ct == 't'))
			ct = *(++psz);

		if (ct == ')')
			break;

		Q_memset(sznum, 0, sizeof(sznum));
		i = 0;

		c = *(++psz);

		if (!isdigit(c))
			break;

		while (isdigit(c) && i < sizeof(sznum) - 1)
		{
			sznum[i++] = c;
			c = *(++psz);
		}

		i = atoi(sznum);

		switch (ct)
		{
		case 'v': pvoxword->volume = i; break;
		case 'p': pvoxword->pitch = i; break;
		case 's': pvoxword->start = i; break;
		case 'e': pvoxword->end = i; break;
		case 't': pvoxword->timecompress = i; break;
		}

		ct = c;
	}

	if (Q_strlen(pszsave) == 0)
	{
		voxwordDefault = *pvoxword;
		return 0;
	}
	else
		return 1;
}

char *VOX_LookupString(const char *pszin, int *psentencenum)
{
	int i;
	char *cptr;
	sentenceEntry_s *sentenceEntry;

	if (pszin[0] == '#')
	{
		const char *indexAsString;

		indexAsString = &pszin[1];
		sentenceEntry = gPrivateFuncs.SequenceGetSentenceByIndex(atoi(indexAsString));

		if (sentenceEntry)
			return sentenceEntry->data;
	}

	for (i = 0; i < (*cszrawsentences); i++)
	{
		if (!Q_strcasecmp(pszin, (*rgpszrawsentence)[i]))
		{
			if (psentencenum)
				*psentencenum = i;

			cptr = &(*rgpszrawsentence)[i][Q_strlen((*rgpszrawsentence)[i]) + 1];

			while (*cptr == ' ' || *cptr == '\t')
				cptr++;

			return cptr;
		}
	}

	return NULL;
}

void S_LoadSentence(const char *pszin, const std::function<void(sfx_t *)> &callback)
{
	char buffer[512];
	int i, j, k, cword;
	char pathbuffer[64];
	char szpath[32];
	sfxcache_t *sc;
	char *psz;
	voxword_t rgvoxword[CVOXWORDMAX];
	char *rgpparseword[CVOXWORDMAX];

	if (!pszin)
		return;

	Q_memset(rgvoxword, 0, sizeof(voxword_t) * CVOXWORDMAX);
	Q_memset(buffer, 0, sizeof(buffer));

	psz = VOX_LookupString(pszin, NULL);

	if (!psz)
	{
		gEngfuncs.Con_DPrintf("S_LoadSentence: no sentence named %s\n", pszin);
		return;
	}

	psz = VOX_GetDirectory(szpath, psz);

	if (Q_strlen(psz) > sizeof(buffer) - 1)
	{
		gEngfuncs.Con_DPrintf("S_LoadSentence: sentence is too long %s\n", psz);
		return;
	}

	Q_strncpy(buffer, psz, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = 0;
	psz = buffer;

	VOX_ParseString(psz, &rgpparseword);

	i = 0;
	cword = 0;

	while (rgpparseword[i])
	{
		if (VOX_ParseWordParams(rgpparseword[i], &rgvoxword[cword], i == 0))
		{
			Q_snprintf(pathbuffer, sizeof(pathbuffer), "%s%s.wav", szpath, rgpparseword[i]);
			pathbuffer[sizeof(pathbuffer) - 1] = 0;

			if (Q_strlen(pathbuffer) >= sizeof(pathbuffer))
				continue;

			rgvoxword[cword].sfx = S_FindName(pathbuffer, &(rgvoxword[cword].fKeepCached));

			if (rgvoxword[cword].sfx)
			{
				callback(rgvoxword[cword].sfx);
			}

			cword++;
		}

		i++;
	}
}

bool S_StartSentence(const char *name, float distance, float avol)
{
	if (!g_pViewPort)
		return false;

	auto pDict = g_pViewPort->FindDictionary(name, DICT_SENTENCE);

	if(!pDict)
	{
		//skip "!" and "#"
		pDict = g_pViewPort->FindDictionary(name + 1);
	}

	if(cap_debug && cap_debug->value)
	{
		if (pDict)
		{
			gEngfuncs.Con_Printf("CaptionMod: SENTENCE [%s] found. dist: %.2f, avol: %.2f\n", name, distance, avol);
		}
		else
		{
			gEngfuncs.Con_Printf("CaptionMod: SENTENCE [%s] not found.\n", name);
		}
	}

	if (!pDict)
	{
		return false;
	}

	if (!pDict->m_bIgnoreDistanceLimit && cap_max_distance && cap_max_distance->value > 0 && distance > cap_max_distance->value)
	{
		return false;
	}

	if (!pDict->m_bIgnoreVolumeLimit && cap_min_avol && cap_min_avol->value > 0 && avol < cap_min_avol->value)
	{
		return false;
	}

	float duration = 0;

	if (pDict->m_bOverrideDuration)
	{
		duration = pDict->m_flDuration;
	}
	else
	{
		S_LoadSentence(name + 1, [&duration](sfx_t* sfx) {

			duration += S_GetDuration(sfx);

		});
	}

	CStartSubtitleContext StartSubtitleContext;

	g_pViewPort->StartSubtitle(pDict, duration, &StartSubtitleContext);

	return true;
}

void S_StartSoundTemplate(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch)
{
	bool ignore = false;
	float distance = 0;
	float avol = 1;

	if (sfx)
	{
		if (flags & (SND_STOP | SND_CHANGE_VOL | SND_CHANGE_PITCH))
		{
			ignore = true;
		}

		if (!ignore)
		{
			auto szLevelName = gEngfuncs.pfnGetLevelName();

			if (szLevelName[0])
			{
				if (origin && !(origin[0] == 0 && origin[1] == 0 && origin[2] == 0) && attenuation > 0 && EngineGetViewEntity())
				{
					float vecLocalOrigin[3];
					VectorCopy(EngineGetViewEntity()->origin, vecLocalOrigin);

					float vecDirection[3];
					VectorSubtract(origin, vecLocalOrigin, vecDirection);

					distance = VectorLength(vecDirection);

					avol = fvol * (1.0f - distance * (attenuation / 1000.0f));
					if (avol < 0)
						avol = 0;
				}
			}
		}

		if (!ignore)
		{
			if (sfx->name[0] == '!' || sfx->name[0] == '#')
			{
				if (!S_StartSentence(sfx->name, distance, avol))
				{
					S_LoadSentence(sfx->name + 1, [distance, avol](sfx_t* sfx_sentence) {

						S_StartWave(sfx_sentence, distance, avol);

					});
				}
				ignore = true;
			}
		}

		if (!ignore)
		{
			S_StartWave(sfx, distance, avol);
		}
	}
}

void S_StartDynamicSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch)
{
	S_StartSoundTemplate(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
	gPrivateFuncs.S_StartDynamicSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
}

void S_StartStaticSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch)
{
	S_StartSoundTemplate(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
	gPrivateFuncs.S_StartStaticSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
}

sfx_t *S_FindName(const char *name, int *pfInCache)
{
	return gPrivateFuncs.S_FindName(name, pfInCache);;
}

//Sven Co-op client.dll

static bool g_bPlayingFMODSound = false;
static bool g_bPlayedFMODSound = false;
static int g_iCurrentPlayingFMODSoundLengthMs = 0;
static void* g_pFMODSystem = NULL;

bool ScClient_StartSentence(const char* name, float distance, float avol)
{
	if (!g_pViewPort)
		return false;

	auto pDict = g_pViewPort->FindDictionary(name, DICT_SENTENCE);

	if (!pDict)
	{
		//skip "!" and "#"
		pDict = g_pViewPort->FindDictionary(name + 1);
	}

	if (cap_debug && cap_debug->value)
	{
		if (pDict)
		{
			gEngfuncs.Con_Printf("CaptionMod: SENTENCE [%s] found. dist: %.2f, avol: %.2f\n", name, distance, avol);
		}
		else
		{
			gEngfuncs.Con_Printf("CaptionMod: SENTENCE [%s] not found.\n", name);
		}
	}

	if (!pDict)
	{
		return false;
	}

	if (!pDict->m_bIgnoreDistanceLimit && cap_max_distance && cap_max_distance->value > 0 && distance > cap_max_distance->value)
	{
		return false;
	}

	if (!pDict->m_bIgnoreVolumeLimit && cap_min_avol && cap_min_avol->value > 0 && avol < cap_min_avol->value)
	{
		return false;
	}

	float duration = 0;

	if (pDict->m_bOverrideDuration)
	{
		duration = pDict->m_flDuration;
	}
	else
	{
#if 0//TODO...
		ScClient_LoadSentence(name + 1, [&duration](sfx_t* sfx) {

			duration += S_GetDuration(sfx);

		});
#endif
	}

	CStartSubtitleContext StartSubtitleContext;

	g_pViewPort->StartSubtitle(pDict, duration, &StartSubtitleContext);

	return true;
}

void ScClient_StartWave(const char* name, float distance, float avol, int ms_duration)
{
	if (!g_pViewPort)
		return;

	if (!Q_strnicmp(name, "sound/", 6))
		name += 6;
	else if (!Q_strnicmp(name + 1, "sound/", 6))
		name += 7;

	auto pDict = g_pViewPort->FindDictionary(name, DICT_SOUND);

	if (cap_debug && cap_debug->value)
	{
		if (pDict)
		{
			gEngfuncs.Con_Printf("CaptionMod: Sound [%s] found. dist: %.2f, avol: %.2f\n", name, distance, avol);
		}
		else
		{
			gEngfuncs.Con_Printf("CaptionMod: Sound [%s] not found.\n", name);
		}
	}

	if (!pDict)
	{
		return;
	}

	if (!pDict->m_bIgnoreDistanceLimit && cap_max_distance && cap_max_distance->value > 0 && distance > cap_max_distance->value)
	{
		return;
	}

	if (!pDict->m_bIgnoreVolumeLimit && cap_min_avol && cap_min_avol->value > 0 && avol < cap_min_avol->value)
	{
		return;
	}

	float duration;

	if (pDict->m_bOverrideDuration)
	{
		duration = pDict->m_flDuration;
	}
	else
	{
		duration = ms_duration / 1000.0f;
	}

	CStartSubtitleContext StartSubtitleContext;

	g_pViewPort->StartSubtitle(pDict, duration, &StartSubtitleContext);
}

void __fastcall ScClient_SoundEngine_PlayFMODSound(void* pSoundEngine, int, int flags, int entindex, float* origin, int channel, const char* name, float fvol, float attenuation, int extraflags, int pitch, int sentenceIndex, float soundLength)
{
	g_bPlayingFMODSound = true;
	g_iCurrentPlayingFMODSoundLengthMs = 0;
	g_pFMODSystem = *(void **)((PUCHAR)pSoundEngine + 0x2004);

	gPrivateFuncs.ScClient_SoundEngine_PlayFMODSound(pSoundEngine, 0, flags, entindex, origin, channel, name, fvol, attenuation, extraflags, pitch, sentenceIndex, soundLength);

	if (g_bPlayedFMODSound)
	{
		bool ignore = false;
		float distance = 0;
		float avol = 1;

		if (flags & (SND_STOP | SND_CHANGE_VOL | SND_CHANGE_PITCH))
		{
			ignore = true;
		}

		if (!ignore)
		{
			auto szLevelName = gEngfuncs.pfnGetLevelName();

			if (szLevelName[0])
			{
				if (origin && !(origin[0] == 0 && origin[1] == 0 && origin[2] == 0) && attenuation > 0 && EngineGetViewEntity())
				{
					float vecLocalOrigin[3];
					VectorCopy(EngineGetViewEntity()->origin, vecLocalOrigin);

					float vecDirection[3];
					VectorSubtract(origin, vecLocalOrigin, vecDirection);

					distance = VectorLength(vecDirection);

					avol = fvol * (1.0f - distance * (attenuation / 1000.0f));
					if (avol < 0)
						avol = 0;
				}
			}
		}

		if (!ignore && !name && sentenceIndex >= 0)
		{
			auto sentenceName = gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySentenceIndex(pSoundEngine, sentenceIndex);

			if (sentenceName)
			{
				ScClient_StartWave(sentenceName, distance, avol, g_iCurrentPlayingFMODSoundLengthMs);

				ignore = true;
			}
		}

		if (!ignore && name)
		{
			ScClient_StartWave(name, distance, avol, g_iCurrentPlayingFMODSoundLengthMs);
		}
	}

	g_bPlayingFMODSound = false;
	g_bPlayedFMODSound = false;
	g_iCurrentPlayingFMODSoundLengthMs = 0;
}

int __stdcall FMOD_System_playSound(void* FMOD_System, int channelid, void* FMOD_Sound, bool paused, void** FMOD_Channel)
{
	if (g_bPlayingFMODSound && g_pFMODSystem == FMOD_System)
	{
		int duration = 0;
		gPrivateFuncs.FMOD_Sound_getLength(FMOD_Sound, &duration, 1);
		g_iCurrentPlayingFMODSoundLengthMs = duration;
		g_bPlayedFMODSound = true;
	}

	return gPrivateFuncs.FMOD_System_playSound(FMOD_System, channelid, FMOD_Sound, paused, FMOD_Channel);
}

void TextMessageParse(byte* pMemFile, int fileSize)
{
	if (fileSize > 2 && pMemFile[0] == 0xFF && pMemFile[1] == 0xFE)
	{
		if (IsTextUnicode(pMemFile + 2, fileSize - 2, NULL))
		{
			auto wszBuf = (const wchar_t*)(pMemFile + 2);
			char* szBuf = (char*)malloc(fileSize);
			if (szBuf)
			{
				memset(szBuf, 0, fileSize);

				int szBufLen = vgui::localize()->ConvertUnicodeToANSI(wszBuf, szBuf, fileSize);

				szBuf[szBufLen] = 0;

				gPrivateFuncs.TextMessageParse((byte *)szBuf, szBufLen);

				free(szBuf);
				return;
			}
		}
	}

	return gPrivateFuncs.TextMessageParse(pMemFile, fileSize);
}


void COM_ExplainDisconnection(qboolean bPrint, const char* fmt, ...)
{
	if (!strcmp(fmt, "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)"))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		// Assuming the first argument is of type 'int'
		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);
		int thirdArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_LoadBrushModel");
	}
	else if (!strcmp(fmt, "Error: Corrupt demo file."))
	{
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_CorruptDemoFile");
	}
	else if (!strcmp(fmt, "Client world model is NULL\n"))
	{
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_InvalidClientWorldModel");
	}
	else if (!strcmp(fmt, "Client world model is invalid. Not a brush.\n"))
	{
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_InvalidClientWorldModel");
	}
	else if (!strcmp(fmt, "Client world model is invalid: Unexpected name.\n"))
	{
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_InvalidClientWorldModel");
	}
	else if (!strcmp(fmt, "Connection to server lost during level change."))
	{
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_ConnectionLostDuringLevelChange");
	}
	else if (!strcmp(fmt, "Invalid server version, unable to connect."))
	{
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_InvalidServerVersion");
	}
	else if (!strcmp(fmt, "Cannot continue without script %s, disconnecting."))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_CannotContinueWithoutScript");
	}
	else if (!strcmp(fmt, "Cannot continue without model %s, disconnecting."))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_CannotContinueWithoutModel");
	}
	else if (!strcmp(fmt, "Cannot continue with altered model %s, disconnecting."))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_CannotContinueWithoutAlteredModel");
	}
	else if (!strcmp(fmt, "Refusing to download map %s, (cl_allowdownload is 0 ) disconnecting.\n"))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_RefuseToDownloadMap");
	}
	else if (!strcmp(fmt, "Couldn't CRC client side dll %s.") || !strcmp(fmt, "Couldn't CRC .dll [%s]."))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_CouldntCRCClientDll");
	}
	else if (!strncmp(fmt, "Your map [%s] differs from the server's.", sizeof("Your map [%s] differs from the server's.") - 1))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_MapDiffersFromServer");
	}
	else if (!strncmp(fmt, "Your .dll [%s] differs from the server's.", sizeof("Your .dll [%s] differs from the server's.") - 1))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		const char* firstArg = va_arg(args, const char*);
		int secondArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%s", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_DllDiffersFromServer");
	}
	else if (!strcmp(fmt, "Too many sounds precached. (Limit is %d.)"))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		int firstArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%d", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_TooManyPrecachedSounds");
	}
	else if (!strcmp(fmt, "Too many models precached. (Limit is %d."))
	{
		va_list args;
		va_start(args, fmt); // Initialize 'args' to point to the first argument after 'dummy'

		int firstArg = va_arg(args, int);

		gPrivateFuncs.COM_ExtendedExplainDisconnection(false, "%d", firstArg);
		return gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", "#GameUI_ExplainDisconnection_TooManyPrecachedModels");
	}

	va_list args;
	va_start(args, fmt);

	// Use vsnprintf to calculate the required length
	int length = vsnprintf(nullptr, 0, fmt, args);
	va_end(args);

	// Check for error
	if (length <= 0) {
		return;
	}

	std::string str;

	str.resize(length);

	// Format the string again with the actual buffer
	va_start(args, fmt);
	vsnprintf((char*)str.c_str(), str.length() + 1, fmt, args);
	va_end(args);

	gPrivateFuncs.COM_ExplainDisconnection(bPrint, "%s", str.c_str());
}

client_textmessage_t* pfnTextMessageGet(const char* pName)
{
	if (g_pViewPort)
	{
		CDictionary* dict = g_pViewPort->FindDictionary(pName);

		if (dict)
		{
			return NULL;
		}
	}

	return gPrivateFuncs.pfnTextMessageGet(pName);
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

double GetAbsoluteTime()
{
	return gEngfuncs.GetAbsoluteTime();
}

static unsigned long g_VoiceBanMask = 0;

unsigned long GetVoiceBanMask()
{
	return g_VoiceBanMask;
}

int pfnServerCmdUnreliable(const char* szCmdString)
{
	if (!strncmp(szCmdString, "vban ",sizeof("vban ") - 1 ))
	{
		unsigned long banMask = 0;
		if (1 == sscanf(szCmdString, "vban %x", &banMask))
		{
			g_VoiceBanMask = banMask;
		}
	}

	return gPrivateFuncs.pfnServerCmdUnreliable(szCmdString);
}
