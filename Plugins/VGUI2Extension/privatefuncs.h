#pragma once

#include "plugins.h"
#include <usercmd.h>
#include <com_model.h>
#include <studio.h>
#include <pm_defs.h>
#include <tier0/basetypes.h>
#include <cvardef.h>
#include "enginedef.h"

class vgui1_TextImage;
class KeyValues;

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
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
	void* (__cdecl* vgui_App_getInstance)();
	void (__fastcall *vgui_TextImage_paint)(vgui1_TextImage *pthis, int, void *panel);
	void** vftable_vgui1_TextImage;
	void** vftable_vgui1_Color;

	//VGUI2;
	char* (*V_strncpy)(char* a1, const char* a2, size_t a3);

	//Engine init
	PVOID (*VGUIClient001_CreateInterface)(HINTERFACEMODULE hModule);

	//ClientVGUI
	void(__fastcall* ClientVGUI_Panel_Init)(void* pthis, int dummy, int x, int y, int w, int h);
	void(__fastcall* ClientVGUI_Panel_SetSize)(void* pthis, int dummy, int width, int height);
	void(__fastcall* ClientVGUI_LoadControlSettings)(void* pthis, int dummy, const char* controlResourceName, const char* pathID);
	void** ClientVGUI_KeyValues_vftable;
	bool(__fastcall* ClientVGUI_KeyValues_LoadFromFile)(void* pthis, int dummy, IFileSystem* pFileSystem, const char* resourceName, const char* pathId);

	void(__fastcall* ClientVGUI_RichText_SetTextW)(void* pthis, int dummy, const wchar_t* text);
	void(__fastcall* ClientVGUI_RichText_SetTextA)(void* pthis, int dummy, const char* text);

	//void** ClientVGUI_BuildGroup_vftable;
	//void(__fastcall* ClientVGUI_BuildGroup_ApplySettings)(void* pthis, int dummy, void* resourceData);
	//void(__fastcall* ClientVGUI_BuildGroup_LoadControlSettings)(void* pthis, int dummy, const char* controlResourceName, const char* pathID);

	int ClientVGUI_Frame_Activate_vftable_index;
	//void* (__fastcall* CCSBackGroundPanel_ctor)(void* pthis, int, void* parent);
	void (__fastcall* CCSBackGroundPanel_Activate)(void* pthis, int dummy);
	void** CCSBackGroundPanel_vftable;
	int CCSBackGroundPanel_XOffsetBase;

	int CWorldMap_PaintBackground_vftable_index;
	void(__fastcall* CWorldMap_PaintBackground)(void* pthis, int dummy);
	void** CWorldMap_vftable;

	int CWorldMapMissionSelect_PaintBackground_vftable_index;
	void(__fastcall* CWorldMapMissionSelect_PaintBackground)(void* pthis, int dummy);
	void** CWorldMapMissionSelect_vftable;

	//void* (__fastcall* CClientMOTD_ctor)(void* pthis, int, void* parent);
	//void (__fastcall* CClientMOTD_PerformLayout)(void* pthis, int dummy);
	//void (__fastcall* CClientMOTD_ApplySettings)(void* pthis, int dummy, void* inResourceData);
	//void** CClientMOTD_vftable;

	void* (__fastcall* CSBuyMenu_ctor)(void* pthis, int, void* parent);
	void (__fastcall* CSBuyMenu_Activate)(void* pthis, int);
	void** CSBuyMenu_vftable;

	//void* (__fastcall* CBuySubMenu_ctor)(void* pthis, int, void* parent);
	//void(__fastcall* CBuySubMenu_OnDisplay)(void* pthis, int);
	//void** CBuySubMenu_vftable;

	//ServerBrowser
	void(__fastcall* ServerBrowser_Panel_Init)(void* pthis, int dummy, int x, int y, int w, int h);
	void(__fastcall* ServerBrowser_LoadControlSettings)(void* pthis, int dummy, const char* controlResourceName, const char* pathID);
	//void(__fastcall* ServerBrowser_LoadControlSettingsAndUserConfig)(void* pthis, int dummy, const char* dialogResourceName, int dialogID);
	//void* (__fastcall* ServerBrowser_KeyValues_ctor)(void* pthis, int dummy, const char* name);
	//void (__fastcall* CBaseGamesPage_OnButtonToggled)(void* pthis, int dummy, void* a2, int state);
	void(__fastcall* ServerBrowser_Panel_SetSize)(void* pthis, int dummy, int width, int height);
	void(__fastcall* ServerBrowser_Panel_SetMinimumSize)(void* pthis, int dummy, int width, int height);
	void ** ServerBrowser_KeyValues_vftable;
	bool(__fastcall* ServerBrowser_KeyValues_LoadFromFile)(void* pthis, int dummy, IFileSystem* pFileSystem, const char* resourceName, const char* pathId);
	//void* (__fastcall* CServerBrowserDialog_ctor)(void* pthis, int dummy, void* parent);
	//GameUI
	void(__fastcall* GameUI_Panel_Init)(void* pthis, int dummy, int x, int y, int w, int h);
	void(__fastcall* GameUI_LoadControlSettings)(void* pthis, int dummy, const char* controlResourceName, const char* pathID);
	void(__fastcall* GameUI_LoadControlSettingsAndUserConfig)(void* pthis, int dummy, const char* dialogResourceName, int dialogID);
	void*(__fastcall* GameUI_KeyValues_ctor)(void* pthis, int dummy, const char* name);
	void** GameUI_KeyValues_vftable;
	bool(__fastcall* GameUI_KeyValues_LoadFromFile)(void* pthis, int dummy, IFileSystem* pFileSystem, const char* resourceName, const char* pathId);
	void(__fastcall* GameUI_Panel_SetSize)(void* pthis, int dummy, int width, int height);
	void** GameUI_Menu_vftable;
	void(__fastcall* GameUI_Menu_MakeItemsVisibleInScrollRange)(void* pthis, int dummy);
	void *(__fastcall* GameUI_Sheet_ctor)(void* pthis, int dummy, void* parent, const char *panelName);
	int offset_ScrollBar;
	void** GameUI_Sheet_vftable;
	int offset_propertySheet;
	void *(__fastcall*MessageBox_ctor)(void* pthis, int dummy, const char *title, const char *text, void *parent);
	void (__fastcall*MessageBox_ApplySchemeSettings)(void* pthis, int dummy, void *pScheme);
	void** MessageBox_vftable;
	void *(__fastcall*CCreateMultiplayerGameDialog_ctor)(void* pthis, int dummy, void* parent);
	void *(__fastcall*CGameConsoleDialog_ctor)(void* pthis, int dummy);
	void *(__fastcall*COptionsDialog_ctor)(void *pthis, int dummy, void *parent);
	void *(__fastcall*COptionsSubVideo_ctor)(void *pthis, int dummy, void *parent);
	void *(__fastcall*COptionsSubAudio_ctor)(void *pthis, int dummy, void *parent);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings)(void *pthis, int dummy, bool bForceRestart);
	void(__fastcall *COptionsSubVideo_ApplyVidSettings_HL25)(void *pthis, int dummy);

	void** CTaskBar_vftable;
	void*(__fastcall*CTaskBar_ctor)(void* pthis, int dummy, void* parent, const char* panelName);
	void(__fastcall* CTaskBar_OnCommand)(void* pthis, int dummy, const char* command);
	void(__fastcall* CTaskBar_CreateGameMenu)(void* pthis, int dummy);

	void* (__fastcall* CBasePanel_ctor)(void* pthis, int dummy);
	void(__fastcall* CBasePanel_ApplySchemeSettings)(void* pthis, int dummy, void* pScheme);
	void** CBasePanel_vftable;

	void(__fastcall* GameUI_PropertySheet_PerformLayout)(void* pthis, int dummy);
	void* (__fastcall *GameUI_PropertySheet_HasHotkey)(void* pthis, int dummy, wchar_t key);
	void* (__fastcall *GameUI_FocusNavGroup_GetCurrentFocus)(void* pthis, int dummy);

	void(__fastcall* GameUI_RichText_Print)(void* pthis, int dummy, const char* msg);
	void (__fastcall* GameUI_RichText_InsertStringA)(void* pthis, int dummy, const char* msg);
	void(__fastcall* GameUI_RichText_InsertStringW)(void* pthis, int dummy, const wchar_t* msg);
	void(__fastcall* GameUI_RichText_InsertChar)(void* pthis, int dummy, wchar_t ch);
	void (__fastcall* GameUI_RichText_OnThink)(void* pthis, int dummy);//virtual 0x158

	void (__fastcall* GameUI_TextEntry_LayoutVerticalScrollBarSlider)(void* pthis, int dummy);//virtual 0x2C0
	void (__fastcall* GameUI_TextEntry_OnKeyCodeTyped)(void* pthis, int dummy, int code);//virtual 0x194
	int  (__fastcall* GameUI_TextEntry_GetStartDrawIndex)(void* pthis, int dummy, int& lineBreakIndexIndex);//virtual 0x2F8
	//void (__fastcall* TextEntry_InsertChar)(void* pthis, int dummy, wchar_t ch);//virtual 0x250

	void* (__fastcall* CCareerProfileFrame_ctor)(void* pthis, int dummy, void* parent);
	void* (__fastcall* CCareerMapFrame_ctor)(void* pthis, int dummy, void* parent);
	void* (__fastcall* CCareerBotFrame_ctor)(void* pthis, int dummy, void* parent);

	//SDL2
	void (*SDL_GetWindowPosition)(void* window, int* x, int* y);
	void (*SDL_GetWindowSize)(void* window, int* w, int* h);
	int (*SDL_GetDisplayDPI)(int displayIndex, float* ddpi, float* hdpi, float* vdpi);
	void*(*SDL_GetWindowFromID)(int id);
	int (*SDL_GetWindowWMInfo)(void* window, void* info);
	void* (*SDL_GL_GetCurrentWindow)(void);
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

extern char m_szCurrentGameLanguage[128];

//VGUI1 engineSurfaceWrapper
extern void* staticEngineSurface;

extern private_funcs_t gPrivateFuncs;

const char* GetCurrentGameLanguage();

extern HMODULE g_hGameUI;
extern HMODULE g_hServerBrowser;
extern bool g_bIsServerBrowserHooked;

PVOID VGUIClient001_CreateInterface(HINTERFACEMODULE hModule);

bool SCR_IsLoadingVisible(void);

PVOID VGUI2_FindPanelInit(PVOID TextBase, ULONG TextSize);
PVOID *VGUI2_FindKeyValueVFTable(PVOID TextBase, ULONG TextSize, PVOID RdataBase, ULONG RdataSize, PVOID DataBase, ULONG DataSize);
PVOID* VGUI2_FindMenuVFTable(PVOID TextBase, ULONG TextSize, PVOID RdataBase, ULONG RdataSize, PVOID DataBase, ULONG DataSize);

void Client_FillAddress(void);
void Client_InstallHooks(void);
void Client_UninstallHooks(void);
void SDL2_FillAddress(void);
void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_InstallHooks(void);
void Engine_UninstallHooks(void);
void BaseUI_InstallHooks(void);
void BaseUI_UninstallHooks(void);
void GameUI_FillAddress(void);
void GameUI_InstallHooks(void);
void GameUI_UninstallHooks(void);
void ServerBrowser_FillAddress(void);
void ServerBrowser_InstallHooks(void);
void ServerBrowser_UninstallHooks(void);
void ClientVGUI_InstallHooks(cl_exportfuncs_t* pExportFunc); 
void NativeClientUI_UninstallHooks(void);
void VGUI1_InstallHooks(void);
void VGUI1_PostInstallHooks(void);
void VGUI1_Shutdown(void);
void Surface_InstallHooks(void);
void Surface_UninstallHooks(void);
void Scheme_InstallHooks(void);
void Scheme_UninstallHooks(void);
void KeyValuesSystem_InstallHooks(void);
void KeyValuesSystem_UninstallHooks(void);
void InputWin32_FillAddress(void);

void DllLoadNotification(mh_load_dll_notification_context_t* ctx);
