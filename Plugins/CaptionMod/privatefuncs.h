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

	//Engine Surface
	void(__fastcall *CWin32Font_GetCharRGBA)(void *pthis, int, int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);

	//SC ClientDLL

	int(__fastcall *ScClient_FindSoundEx)(void *pthis, int, const char *soundName);

	//FMOD

	HMODULE fmodex;
	int(__stdcall *FMOD_Sound_getLength)(int a1, void* a2, int a3);//?getLength@Sound@FMOD@@QAG?AW4FMOD_RESULT@@PAII@Z

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
	void(__fastcall* GameUI_LoadControlSettings)(void* pthis, int dummy, const char* controlResourceName, const char* pathID);
	void* (__fastcall* CCreateMultiplayerGameDialog_ctor)(void* pthis, int dummy, void* parent);
	void *(__fastcall*COptionsDialog_ctor)(void *pthis, int dummy, void *parent);
	void *(__fastcall*COptionsSubVideo_ctor)(void *pthis, int dummy, void *parent);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings)(void *pthis, int dummy, bool bForceRestart);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings_HL25)(void *pthis, int dummy);
	void *(__fastcall*COptionsSubAudio_ctor)(void *pthis, int dummy, void *parent);
	void *(__fastcall *COptionsDialog_AddPage)(void *pthis, int dummy, void *panel, const char *name);

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
extern int *cl_viewentity;

extern vec3_t *listener_origin;

extern char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX];
extern int *cszrawsentences;

extern char(*s_pBaseDir)[512];

extern HWND g_MainWnd;
extern WNDPROC g_MainWndProc;

extern char m_szCurrentLanguage[128];

extern private_funcs_t gPrivateFuncs;

cl_entity_t *EngineGetViewEntity(void);

bool SCR_IsLoadingVisible(void);

PVOID VGUI2_FindPanelInit(PVOID TextBase, ULONG TextSize);