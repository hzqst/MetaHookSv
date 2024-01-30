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

	//VGUI1
	void (__fastcall *vgui_TextImage_paint)(vgui1_TextImage *pthis, int, void *panel);

	//VGUI2
	char* (*V_strncpy)(char* a1, const char* a2, size_t a3);

	//Engine init
	PVOID (*VGUIClient001_CreateInterface)(HINTERFACEMODULE hModule);

	//GameUI
	void(__fastcall* GameUI_Panel_Init)(void* pthis, int dummy, int x, int y, int w, int h);
	void(__fastcall* GameUI_LoadControlSettings)(void* pthis, int dummy, const char* controlResourceName, const char* pathID);
	void *(__fastcall* Sheet_ctor)(void* pthis, int dummy, void* parent, const char *panelName);
	int offset_propertySheet;
	//void *(__fastcall* QueryBox_ctor)(void* pthis, int dummy, const char* title, const char* queryText, void* parent);
	void *(__fastcall* CCreateMultiplayerGameDialog_ctor)(void* pthis, int dummy, void* parent);
	void *(__fastcall* CGameConsoleDialog_ctor)(void* pthis, int dummy);
	void *(__fastcall*COptionsDialog_ctor)(void *pthis, int dummy, void *parent);
	void *(__fastcall*COptionsSubVideo_ctor)(void *pthis, int dummy, void *parent);
	void *(__fastcall*COptionsSubAudio_ctor)(void *pthis, int dummy, void *parent);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings)(void *pthis, int dummy, bool bForceRestart);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings_HL25)(void *pthis, int dummy);
	void*(__fastcall* CTaskBar_ctor)(void* pthis, int dummy, void* parent, const char* panelName);
	void (__fastcall* CTaskBar_OnCommand)(void* pthis, int dummy, const char* command);
	void(__fastcall* CTaskBar_CreateGameMenu)(void* pthis, int dummy);
	void* (__fastcall* KeyValues_ctor)(void* pthis, int dummy, const char* name);
	bool (__fastcall* KeyValues_LoadFromFile)(void* pthis, int dummy, IFileSystem *pFileSystem, const char* resourceName, const char *pathId);
	void* (__fastcall *PropertySheet_HasHotkey)(void* pthis, int dummy, wchar_t key);
	void* (__fastcall *FocusNavGroup_GetCurrentFocus)(void* pthis, int dummy);

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
extern char m_szCurrentGameLanguage[128];

extern private_funcs_t gPrivateFuncs;

const char* GetCurrentGameLanguage();

HMODULE GetGameUIModule();

PVOID VGUIClient001_CreateInterface(HINTERFACEMODULE hModule);

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
void GameUI_FillAddress(void);
void GameUI_InstallHooks(void);
void GameUI_UninstallHooks(void);
void ClientVGUI_InstallHooks(cl_exportfuncs_t* pExportFunc);
void VGUI1_InstallHooks(void);
void VGUI1_Shutdown(void);
void Surface_InstallHooks(void);
void Surface_UninstallHooks(void);
void Scheme_InstallHooks(void);
void Scheme_UninstallHooks(void);
void KeyValuesSystem_InstallHooks(void);
void KeyValuesSystem_UninstallHooks(void);
void InputWin32_FillAddress(void);

void DllLoadNotification(mh_load_dll_notification_context_t* ctx);
