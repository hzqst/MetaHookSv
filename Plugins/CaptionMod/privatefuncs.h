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

	//Engine VGUI2 wrapper
	//void(*VGuiWrap2_Paint)(void);
	void(__fastcall* EngineVGUI2_Panel_Init)(void* pthis, int dummy, int x, int y, int w, int h);

	//Engine FileSystem
	//int(*FileSystem_SetGameDirectory)(const char *pDefaultDir, const char *pGameDir);

	//Engine Sound
	void (*S_Init)(void);
	sfx_t *(*S_FindName)(char *name, int *pfInCache);//hooked
	void (*S_StartDynamicSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
	void (*S_StartStaticSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
	sfxcache_t *(*S_LoadSound)(sfx_t *s, channel_t *ch);
	sentenceEntry_s*(*SequenceGetSentenceByIndex)(unsigned int);

	//SC ClientDLL

	//int(__fastcall *ScClient_FindSoundEx)(void *pthis, int, const char *soundName);
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
	//int(__fastcall *CHud_DrawHudString)(void *pthis, int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b);
	//int(__fastcall *DrawHudStringReverse)(void *pthis, int xpos, int ypos, int iMinX, char *szString, int r, int g, int b);

	//VGUI1
	void (__fastcall *vgui_TextImage_paint)(vgui1_TextImage *pthis, int, void *panel);

	//VGUI2
	char* (*V_strncpy)(char* a1, const char* a2, size_t a3);

	//EngineFuncs
	client_textmessage_t *(*pfnTextMessageGet)(const char *pName);

	//Commands
	void(*MessageMode_f)(void);
	void(*MessageMode2_f)(void);

	//Engine init
	PVOID (*VGUIClient001_CreateInterface)(HINTERFACEMODULE hModule);

	//GameUI
	void(__fastcall* GameUI_Panel_Init)(void* pthis, int dummy, int x, int y, int w, int h);
	void(__fastcall* GameUI_LoadControlSettings)(void* pthis, int dummy, const char* controlResourceName, const char* pathID);
	void * (__fastcall* QueryBox_ctor)(void* pthis, int dummy, const char* title, const char* queryText, void* parent);
	void* (__fastcall* CCreateMultiplayerGameDialog_ctor)(void* pthis, int dummy, void* parent);
	void* (__fastcall* CGameConsoleDialog_ctor)(void* pthis, int dummy);
	void *(__fastcall*COptionsDialog_ctor)(void *pthis, int dummy, void *parent);
	void *(__fastcall*COptionsSubVideo_ctor)(void *pthis, int dummy, void *parent);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings)(void *pthis, int dummy, bool bForceRestart);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings_HL25)(void *pthis, int dummy);
	void *(__fastcall*COptionsSubAudio_ctor)(void *pthis, int dummy, void *parent);
	//void *(__fastcall *COptionsDialog_AddPage)(void *pthis, int dummy, void *panel, const char *name);

	void(__fastcall* RichText_Print)(void* pthis, int dummy, const char* msg);
	void (__fastcall* RichText_InsertStringA)(void* pthis, int dummy, const char* msg);
	void(__fastcall* RichText_InsertStringW)(void* pthis, int dummy, const wchar_t* msg);
	void(__fastcall* RichText_InsertChar)(void* pthis, int dummy, wchar_t ch);
	void (__fastcall* RichText_OnThink)(void* pthis, int dummy);//virtual 0x158

	void (__fastcall* TextEntry_LayoutVerticalScrollBarSlider)(void* pthis, int dummy);//virtual 0x2C0
	void (__fastcall* TextEntry_OnKeyCodeTyped)(void* pthis, int dummy, int code);//virtual 0x194
	int  (__fastcall* TextEntry_GetStartDrawIndex)(void* pthis, int dummy, int& lineBreakIndexIndex);//virtual 0x2F8
	//void (__fastcall* TextEntry_InsertChar)(void* pthis, int dummy, wchar_t ch);//virtual 0x250

	//SDL2
	void (*SDL_GetWindowPosition)(void* window, int* x, int* y);
	void (*SDL_GetWindowSize)(void* window, int* w, int* h);
	int (*SDL_GetDisplayDPI)(int displayIndex, float* ddpi, float* hdpi, float* vdpi);
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

extern HWND g_MainWnd;
extern WNDPROC g_MainWndProc;

extern char m_szCurrentLanguage[128];

extern private_funcs_t gPrivateFuncs;

cl_entity_t *EngineGetViewEntity(void);

bool SCR_IsLoadingVisible(void);

PVOID VGUI2_FindPanelInit(PVOID TextBase, ULONG TextSize);
void Client_FillAddress(void);
void Client_InstallHooks(void);
void Client_UninstallHooks(void);
void SDL2_FillAddress(void);
void Engine_FillAddress(void);
void Engine_InstallHooks(void);
void Engine_UninstallHooks(void);
void BaseUI_InstallHook(void);
void BaseUI_UninstallHook(void);
void GameUI_InstallHooks(void);
void GameUI_UninstallHooks(void);
void ClientVGUI_InstallHook(cl_exportfuncs_t* pExportFunc);
void ClientVGUI_Shutdown(void);
void VGUI1_InstallHook(void);
void VGUI1_Shutdown(void);
void Surface_InstallHooks(void);
void Surface_UninstallHooks(void);
void Scheme_InstallHooks(void);
void KeyValuesSystem_InstallHook(void);
void FMOD_InstallHooks(HMODULE fmodex);
void FMOD_UninstallHooks(HMODULE fmodex);

void DllLoadNotification(mh_load_dll_notification_context_t* ctx);
