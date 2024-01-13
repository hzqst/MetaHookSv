#include <metahook.h>
#include "BaseUI.h"
#include <IGameUI.h>
#include <VGUI\IScheme.h>
#include <VGUI\ILocalize.h>
#include <VGUI\ISurface.h>
#include <VGUI\IInput.h>
#include <IEngineSurface.h>
#include <IKeyValuesSystem.h>
#include "vgui_internal.h"
#include "FontTextureCache.h"
#include "EngineSurfaceHook.h"
#include "ViewPort.h"
#include "DpiManager.h"

#include "exportfuncs.h"
#include "privatefuncs.h"

extern IGameUI *g_pGameUI;

static hook_t* g_phook_EngineVGUI2_Panel_Init = NULL;
static bool s_LoadingBaseUI = false;

namespace vgui
{
	bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

void(__fastcall *m_pfnCBaseUI_Initialize)(void *pthis, int, CreateInterfaceFn *factories, int count);
void(__fastcall *m_pfnCBaseUI_Start)(void *pthis, int, struct cl_enginefuncs_s *engineFuncs, int interfaceVersion);
void(__fastcall *m_pfnCBaseUI_Shutdown)(void *pthis, int);
int(__fastcall *m_pfnCBaseUI_Key_Event)(void *pthis, int, int down, int keynum, const char *pszCurrentBinding);
void(__fastcall *m_pfnCBaseUI_CallEngineSurfaceProc)(void *pthis, int, void *hwnd, unsigned int msg, unsigned int wparam, long lparam);
void(__fastcall *m_pfnCBaseUI_Paint)(void *pthis, int, int x, int y, int right, int bottom);
void(__fastcall *m_pfnCBaseUI_HideGameUI)(void *pthis, int);
void(__fastcall *m_pfnCBaseUI_ActivateGameUI)(void *pthis, int);
bool(__fastcall *m_pfnCBaseUI_IsGameUIVisible)(void *pthis, int);
void(__fastcall *m_pfnCBaseUI_HideConsole)(void *pthis, int);
void(__fastcall *m_pfnCBaseUI_ShowConsole)(void *pthis, int);

class CBaseUIProxy : public IBaseUI
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count);
	virtual void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion);
	virtual void Shutdown(void);
	virtual int Key_Event(int down, int keynum, const char *pszCurrentBinding);
	virtual void CallEngineSurfaceProc(void *hwnd, unsigned int msg, unsigned int wparam, long lparam);
	virtual void Paint(int x, int y, int right, int bottom);
	virtual void HideGameUI(void);
	virtual void ActivateGameUI(void);
	virtual bool IsGameUIVisible(void);
	virtual void HideConsole(void);
	virtual void ShowConsole(void);
};

static CBaseUIProxy s_BaseUIProxy;

IBaseUI *baseuifuncs;
IGameUIFuncs *gameuifuncs;

extern vgui::ISurface *g_pSurface;
extern vgui::ISurface_HL25* g_pSurface_HL25;
extern vgui::ISchemeManager *g_pScheme;
extern vgui::ISchemeManager_HL25 *g_pScheme_HL25;
extern IKeyValuesSystem *g_pKeyValuesSystem;
extern IEngineSurface *staticSurface;
extern IEngineSurface_HL25 *staticSurface_HL25;

extern CreateInterfaceFn* g_pClientFactory;

void __fastcall EngineVGUI2_Panel_Init(vgui::Panel* pthis, int dummy, int x, int y, int w, int h)
{
	gPrivateFuncs.EngineVGUI2_Panel_Init(pthis, 0, x, y, w, h);

	//if (s_LoadingBaseUI)
	{
		if (dpimanager()->IsHighDpiSupportEnabled())
		{
			PVOID* PanelVFTable = *(PVOID**)pthis;
			void(__fastcall * pfnSetProportional)(vgui::Panel * pthis, int dummy, bool state) = (decltype(pfnSetProportional))PanelVFTable[113];
			pfnSetProportional(pthis, 0, true);
		}
	}
}

void CBaseUIProxy::Initialize(CreateInterfaceFn *factories, int count)
{
	m_pfnCBaseUI_Initialize(this, 0, factories, count);

	HINTERFACEMODULE hVGUI2 = (HINTERFACEMODULE)GetModuleHandle("vgui2.dll");
	if (hVGUI2)
	{
		CreateInterfaceFn fnVGUI2CreateInterface = Sys_GetFactory(hVGUI2);

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			g_pScheme_HL25 = (vgui::ISchemeManager_HL25*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL); 
		else
			g_pScheme = (vgui::ISchemeManager*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL);

		g_pKeyValuesSystem = (IKeyValuesSystem *)fnVGUI2CreateInterface(KEYVALUESSYSTEM_INTERFACE_VERSION, NULL);
	}

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		g_pSurface_HL25 = (vgui::ISurface_HL25*)factories[0](VGUI_SURFACE_INTERFACE_VERSION, NULL);
	else
		g_pSurface = (vgui::ISurface *)factories[0](VGUI_SURFACE_INTERFACE_VERSION, NULL);

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		staticSurface_HL25 = (IEngineSurface_HL25*)factories[0](ENGINE_SURFACE_VERSION, NULL); 
	else
		staticSurface = (IEngineSurface*)factories[0](ENGINE_SURFACE_VERSION, NULL);

	EngineSurface_FillAddress();
	EngineSurface_InstallHooks();
	KeyValuesSystem_InstallHook();
	Surface_InstallHooks();
	Scheme_InstallHooks();
	GameUI_InstallHooks();
}

void CBaseUIProxy::Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion)
{
	s_LoadingBaseUI = true;

	m_pfnCBaseUI_Start(this, 0, engineFuncs, interfaceVersion);

	s_LoadingBaseUI = false;
}

void CBaseUIProxy::Shutdown(void)
{
	ClientVGUI_Shutdown();

	//TODO: why???
	//if (g_iEngineType != ENGINE_GOLDSRC_HL25)
	GameUI_UninstallHooks();

	//GameUI.dll and vgui2.dll will be unloaded by engine!CBaseUI::Shutdown
	m_pfnCBaseUI_Shutdown(this, 0);
}

int CBaseUIProxy::Key_Event(int down, int keynum, const char *pszCurrentBinding)
{
	return m_pfnCBaseUI_Key_Event(this, 0, down, keynum, pszCurrentBinding);
}

void CBaseUIProxy::CallEngineSurfaceProc(void *hwnd, unsigned int msg, unsigned int wparam, long lparam)
{
	m_pfnCBaseUI_CallEngineSurfaceProc(this, 0, hwnd, msg, wparam, lparam);
}

void CBaseUIProxy::Paint(int x, int y, int right, int bottom)
{
	m_pfnCBaseUI_Paint(this, 0, x, y, right, bottom);
}

void CBaseUIProxy::HideGameUI(void)
{
	m_pfnCBaseUI_HideGameUI(this, 0);
}

void CBaseUIProxy::ActivateGameUI(void)
{
	m_pfnCBaseUI_ActivateGameUI(this, 0);
}

bool CBaseUIProxy::IsGameUIVisible(void)
{
	return m_pfnCBaseUI_IsGameUIVisible(this, 0);
}

void CBaseUIProxy::HideConsole(void)
{
	m_pfnCBaseUI_HideConsole(this, 0);
}

void CBaseUIProxy::ShowConsole(void)
{
	m_pfnCBaseUI_ShowConsole(this, 0);
}

void BaseUI_InstallHook(void)
{
	CreateInterfaceFn fnCreateInterface = g_pMetaHookAPI->GetEngineFactory();
	baseuifuncs = (IBaseUI *)fnCreateInterface(BASEUI_INTERFACE_VERSION, NULL);
	gameuifuncs = (IGameUIFuncs *)fnCreateInterface(VENGINE_GAMEUIFUNCS_VERSION, NULL);

	//Search CBaseUI::Initialize for ClientFactory
	PVOID* ProxyVFTable = *(PVOID**)&s_BaseUIProxy;

	g_pMetaHookAPI->VFTHook(baseuifuncs, 0, 1, ProxyVFTable[1], (void**)&m_pfnCBaseUI_Initialize);
	//g_pMetaHookAPI->VFTHook(baseuifuncs, 0, 2, ProxyVFTable[2], (void**)&m_pfnCBaseUI_Start);
	g_pMetaHookAPI->VFTHook(baseuifuncs, 0, 3, ProxyVFTable[3], (void**)&m_pfnCBaseUI_Shutdown);
	//g_pMetaHookAPI->VFTHook(baseuifuncs, 0, 4, ProxyVFTable[4], (void **)&m_pfnCBaseUI_Key_Event);
	//g_pMetaHookAPI->VFTHook(baseuifuncs, 0, 6, ProxyVFTable[6], (void**)&m_pfnCBaseUI_Paint);

	Install_InlineHook(EngineVGUI2_Panel_Init);
}

void BaseUI_UninstallHook(void)
{
	Uninstall_Hook(EngineVGUI2_Panel_Init);
}