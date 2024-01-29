#pragma once

#include "plugins.h"
#include <usercmd.h>
#include <com_model.h>
#include <studio.h>
#include <pm_defs.h>
#include <tier0/basetypes.h>
#include <cvardef.h>
#include "enginedef.h"

struct vgui1_TextImage;
class KeyValues;

typedef struct walk_context_s
{
	walk_context_s(PVOID a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	PVOID address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	//Engine Screen
	void(*SCR_BeginLoadingPlaque)(qboolean reconnect);

	//Engine Sound
	void (*S_Init)(void);
	sfx_t *(*S_FindName)(char *name, int *pfInCache);//hooked
	void (*S_StartDynamicSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
	void (*S_StartStaticSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
	sfxcache_t *(*S_LoadSound)(sfx_t *s, channel_t *ch);
	sentenceEntry_s*(*SequenceGetSentenceByIndex)(unsigned int);

	//SC ClientDLL
	void (__fastcall*ScClient_SoundEngine_PlayFMODSound)(void *pSoundEngine, int, int flags, int entindex, float *origin, int channel, const char *name, float fvol, float attenuation, int extraflags, int pitch, int sentenceIndex, float soundLength);
	const char *(__thiscall* ScClient_SoundEngine_LookupSoundBySentenceIndex)(void* pSoundEngine, int sentenceIndex);

	//FMOD

	int(__stdcall*FMOD_Sound_getLength)(void * FMOD_Sound, void* output, int type);//?getLength@Sound@FMOD@@QAG?AW4FMOD_RESULT@@PAII@Z
	int(__stdcall*FMOD_System_playSound)(void* FMOD_System, int channelid, void* FMOD_Sound, bool paused, void** FMOD_Channel);//?playSound@System@FMOD@@QAG?AW4FMOD_RESULT@@W4FMOD_CHANNELINDEX@@PAVSound@2@_NPAPAVChannel@2@@Z

	//ClientDLL

	float *(*GetClientColor)(int clientIndex);

	//Only available in vgui2 based Counter Strike
	float *(*GetTextColor)(int colorNum, int clientIndex);

	//ClientDLL
	bool (__fastcall *GameViewport_AllowedToPrintText)(void *pthis, int);
	bool (__fastcall *GameViewport_IsScoreBoardVisible)(void *pthis, int);
	void (__fastcall *WeaponsResource_SelectSlot)(void *pthis, int, int iSlot, int fAdvance, int iDirection);
	int (__fastcall *CHud_GetBorderSize)(void *pthis, int);

	//Engine TextMessage
	client_textmessage_t *(*pfnTextMessageGet)(const char *pName);
	void (*TextMessageParse)(byte* pMemFile, int fileSize);

	//Commands
	void(*MessageMode_f)(void);
	void(*MessageMode2_f)(void);

}private_funcs_t;

extern void *GameViewport;
extern int *g_iVisibleMouse;
extern void *gHud;

extern double *cl_time;
extern double *cl_oldtime;
extern double* realtime;

extern int *cl_viewentity;

extern vec3_t *listener_origin;

extern char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX];
extern int *cszrawsentences;

//extern char(*s_pBaseDir)[512];
extern char*(*hostparam_basedir);

extern private_funcs_t gPrivateFuncs;

cl_entity_t *EngineGetViewEntity(void);

bool SCR_IsLoadingVisible(void);

void Client_FillAddress(void);
void Client_InstallHooks(void);
void Client_UninstallHooks(void);
void Engine_FillAddress(void);
void Engine_InstallHooks(void);
void Engine_UninstallHooks(void);
void BaseUI_InstallHooks(void);
void BaseUI_UninstallHooks(void);
void GameUI_FillAddress(HMODULE hModule);
void GameUI_InstallHooks(void);
void GameUI_UninstallHooks(void);
void ClientVGUI_InstallHooks(void);
void ClientVGUI_UninstallHooks(void);
void FMOD_InstallHooks(HMODULE fmodex);
void FMOD_UninstallHooks(HMODULE fmodex);

void DllLoadNotification(mh_load_dll_notification_context_t* ctx);
