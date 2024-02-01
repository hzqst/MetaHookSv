#include <metahook.h>
#include <VGUI\IScheme.h>
#include <VGUI\ILocalize.h>
#include <VGUI\ISurface.h>
#include <VGUI\IInput.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <IBaseUI.h>
#include <IGameUI.h>
#include <IGameUIFuncs.h>
#include <IEngineSurface.h>
#include <IKeyValuesSystem.h>
#include "vgui_internal.h"
#include "FontTextureCache.h"
#include "DpiManagerInternal.h"

#include "exportfuncs.h"
#include "privatefuncs.h"

#include "VGUI2ExtensionInternal.h"

static hook_t* g_phook_EngineVGUI2_Panel_Init = NULL;

IBaseUI* baseui = NULL;
IBaseUI_Legacy* baseui_legacy = NULL;
IGameUIFuncs* gameuifuncs = NULL;

extern vgui::ISurface* g_pSurface;
extern vgui::ISurface_HL25* g_pSurface_HL25;
extern vgui::ISchemeManager* g_pSchemeManager;
extern vgui::ISchemeManager_HL25* g_pSchemeManager_HL25;
extern IKeyValuesSystem* g_pKeyValuesSystem;
extern IEngineSurface* staticSurface;
extern IEngineSurface_HL25* staticSurface_HL25;
extern CreateInterfaceFn* g_pClientFactory;

namespace vgui
{
	bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

void(__fastcall *m_pfnCBaseUI_Initialize)(void *pthis, int, CreateInterfaceFn *factories, int count);
void(__fastcall *m_pfnCBaseUI_Start)(void *pthis, int, struct cl_enginefuncs_s *engineFuncs, int interfaceVersion);
void(__fastcall *m_pfnCBaseUI_Shutdown)(void *pthis, int);
int(__fastcall *m_pfnCBaseUI_Key_Event)(void *pthis, int, int down, int keynum, const char *pszCurrentBinding);
void(__fastcall *m_pfnCBaseUI_CallEngineSurfaceAppHandler)(void *pthis, int, void* pevent, void* userData);
void(__fastcall* m_pfnCBaseUI_CallEngineSurfaceWndProc)(void* pthis, int, void* hwnd, unsigned int msg, unsigned int wparam, long lparam);
void(__fastcall *m_pfnCBaseUI_Paint)(void *pthis, int, int x, int y, int right, int bottom);
void(__fastcall *m_pfnCBaseUI_HideGameUI)(void *pthis, int);
void(__fastcall *m_pfnCBaseUI_ActivateGameUI)(void *pthis, int);
void(__fastcall *m_pfnCBaseUI_HideConsole)(void *pthis, int);
void(__fastcall *m_pfnCBaseUI_ShowConsole)(void *pthis, int);

void __fastcall EngineVGUI2_Panel_Init(vgui::Panel* pthis, int dummy, int x, int y, int w, int h)
{
	gPrivateFuncs.EngineVGUI2_Panel_Init(pthis, 0, x, y, w, h);

	if (g_iEngineType != ENGINE_GOLDSRC_HL25 && DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		PVOID* PanelVFTable = *(PVOID**)pthis;
		void(__fastcall * pfnSetProportional)(vgui::Panel * pthis, int dummy, bool state) = (decltype(pfnSetProportional))PanelVFTable[113];
		pfnSetProportional(pthis, 0, true);
	}
}

class CBaseUIProxy : public IBaseUI
{
public:
	void Initialize(CreateInterfaceFn* factories, int count) override;
	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion) override;
	void Shutdown(void) override;
	int Key_Event(int down, int keynum, const char *pszCurrentBinding) override;
	void CallEngineSurfaceAppHandler(void* pevent, void* userData) override;
	void Paint(int x, int y, int right, int bottom) override;
	void HideGameUI(void) override;
	void ActivateGameUI(void) override;
	void HideConsole(void) override;
	void ShowConsole(void) override;
};

static CBaseUIProxy s_BaseUIProxy;

class CBaseUILegacyProxy : public IBaseUI_Legacy
{
public:
	void Initialize(CreateInterfaceFn* factories, int count) override;
	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion) override;
	void Shutdown(void) override;
	int Key_Event(int down, int keynum, const char* pszCurrentBinding) override;
	void CallEngineSurfaceWndProc(void* hwnd, unsigned int msg, unsigned int wparam, long lparam)  override;
	void Paint(int x, int y, int right, int bottom) override;
	void HideGameUI(void) override;
	void ActivateGameUI(void) override;
	void HideConsole(void) override;
	void ShowConsole(void) override;
};

static CBaseUILegacyProxy s_BaseUILegacyProxy;

/*
=========================================================================
SDL2 IBaseUI hook proxy
=========================================================================
*/

void CBaseUIProxy::Initialize(CreateInterfaceFn *factories, int count)
{
	m_pfnCBaseUI_Initialize(this, 0, factories, count);

	HINTERFACEMODULE hVGUI2 = (HINTERFACEMODULE)GetModuleHandle("vgui2.dll");

	if (hVGUI2)
	{
		CreateInterfaceFn fnVGUI2CreateInterface = Sys_GetFactory(hVGUI2);

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			g_pSchemeManager_HL25 = (vgui::ISchemeManager_HL25*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL); 
		else
			g_pSchemeManager = (vgui::ISchemeManager*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL);

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

	KeyValuesSystem_InstallHooks();
	Surface_InstallHooks();
	Scheme_InstallHooks();
	GameUI_FillAddress();
	GameUI_InstallHooks();
	InputWin32_FillAddress();

	VGUI2ExtensionInternal()->BaseUI_Initialize(factories, count);
}

void CBaseUIProxy::Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion)
{
	m_pfnCBaseUI_Start(this, 0, engineFuncs, interfaceVersion);

	VGUI2ExtensionInternal()->BaseUI_Start(engineFuncs, interfaceVersion);
}

void CBaseUIProxy::Shutdown(void)
{
	// Give panels a chance to settle so things
	// Marked for deletion will actually get deleted
	vgui::ivgui()->RunFrame();

	VGUI2ExtensionInternal()->ClientVGUI_Shutdown();

	GameUI_UninstallHooks();

	//GameUI.dll and vgui2.dll will be unloaded by engine!CBaseUI::Shutdown
	m_pfnCBaseUI_Shutdown(this, 0);

	VGUI2ExtensionInternal()->BaseUI_Shutdown();
}

int CBaseUIProxy::Key_Event(int down, int keynum, const char *pszCurrentBinding)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->BaseUI_Key_Event(down, keynum, pszCurrentBinding, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = m_pfnCBaseUI_Key_Event(this, 0, down, keynum, pszCurrentBinding);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->BaseUI_Key_Event(down, keynum, pszCurrentBinding, &CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

void CBaseUIProxy::CallEngineSurfaceAppHandler(void* pevent, void* userData)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_CallEngineSurfaceAppProc(pevent, userData, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_CallEngineSurfaceAppHandler(this, 0, pevent, userData);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_CallEngineSurfaceAppProc(pevent, userData, &CallbackContext);
	}
}

void CBaseUIProxy::Paint(int x, int y, int right, int bottom)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_Paint(x, y, right, bottom, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_Paint(this, 0, x, y, right, bottom);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_Paint(x, y, right, bottom, &CallbackContext);
	}
}

void CBaseUIProxy::HideGameUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_HideGameUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_HideGameUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_HideGameUI(&CallbackContext);
	}
}

void CBaseUIProxy::ActivateGameUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_ActivateGameUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_ActivateGameUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_ActivateGameUI(&CallbackContext);
	}
}

void CBaseUIProxy::HideConsole(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_HideConsole(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_HideConsole(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_HideConsole(&CallbackContext);
	}
}

void CBaseUIProxy::ShowConsole(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_ShowConsole(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_ShowConsole(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_ShowConsole(&CallbackContext);
	}
}

/*
=========================================================================
Legacy IBaseUI hook proxy
=========================================================================
*/

void CBaseUILegacyProxy::Initialize(CreateInterfaceFn* factories, int count)
{
	m_pfnCBaseUI_Initialize(this, 0, factories, count);

	HINTERFACEMODULE hVGUI2 = (HINTERFACEMODULE)GetModuleHandle("vgui2.dll");

	if (hVGUI2)
	{
		CreateInterfaceFn fnVGUI2CreateInterface = Sys_GetFactory(hVGUI2);

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			g_pSchemeManager_HL25 = (vgui::ISchemeManager_HL25*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL);
		else
			g_pSchemeManager = (vgui::ISchemeManager*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL);

		g_pKeyValuesSystem = (IKeyValuesSystem*)fnVGUI2CreateInterface(KEYVALUESSYSTEM_INTERFACE_VERSION, NULL);
	}

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		g_pSurface_HL25 = (vgui::ISurface_HL25*)factories[0](VGUI_SURFACE_INTERFACE_VERSION, NULL);
	else
		g_pSurface = (vgui::ISurface*)factories[0](VGUI_SURFACE_INTERFACE_VERSION, NULL);

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		staticSurface_HL25 = (IEngineSurface_HL25*)factories[0](ENGINE_SURFACE_VERSION, NULL);
	else
		staticSurface = (IEngineSurface*)factories[0](ENGINE_SURFACE_VERSION, NULL);

	KeyValuesSystem_InstallHooks();
	Surface_InstallHooks();
	Scheme_InstallHooks();
	GameUI_FillAddress();
	GameUI_InstallHooks();
	InputWin32_FillAddress();

	VGUI2ExtensionInternal()->BaseUI_Initialize(factories, count);
}

void CBaseUILegacyProxy::Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion)
{
	m_pfnCBaseUI_Start(this, 0, engineFuncs, interfaceVersion);

	VGUI2ExtensionInternal()->BaseUI_Start(engineFuncs, interfaceVersion);
}

void CBaseUILegacyProxy::Shutdown(void)
{
	// Give panels a chance to settle so things
	// Marked for deletion will actually get deleted
	vgui::ivgui()->RunFrame();

	VGUI2ExtensionInternal()->ClientVGUI_Shutdown();

	GameUI_UninstallHooks();

	//GameUI.dll and vgui2.dll will be unloaded by engine!CBaseUI::Shutdown
	m_pfnCBaseUI_Shutdown(this, 0);

	VGUI2ExtensionInternal()->BaseUI_Shutdown();
}

int CBaseUILegacyProxy::Key_Event(int down, int keynum, const char* pszCurrentBinding)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->BaseUI_Key_Event(down, keynum, pszCurrentBinding, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = m_pfnCBaseUI_Key_Event(this, 0, down, keynum, pszCurrentBinding);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->BaseUI_Key_Event(down, keynum, pszCurrentBinding, &CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

void CBaseUILegacyProxy::CallEngineSurfaceWndProc(void* hwnd, unsigned int msg, unsigned int wparam, long lparam)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_CallEngineSurfaceWndProc(hwnd, msg, wparam, lparam, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_CallEngineSurfaceWndProc(this, 0, hwnd, msg, wparam, lparam);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_CallEngineSurfaceWndProc(hwnd, msg, wparam, lparam, &CallbackContext);
	}
}

void CBaseUILegacyProxy::Paint(int x, int y, int right, int bottom)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_Paint(x, y, right, bottom, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_Paint(this, 0, x, y, right, bottom);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_Paint(x, y, right, bottom, &CallbackContext);
	}
}

void CBaseUILegacyProxy::HideGameUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_HideGameUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_HideGameUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_HideGameUI(&CallbackContext);
	}
}

void CBaseUILegacyProxy::ActivateGameUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_ActivateGameUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_ActivateGameUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_ActivateGameUI(&CallbackContext);
	}
}

void CBaseUILegacyProxy::HideConsole(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_HideConsole(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_HideConsole(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_HideConsole(&CallbackContext);
	}
}

void CBaseUILegacyProxy::ShowConsole(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->BaseUI_ShowConsole(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCBaseUI_ShowConsole(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->BaseUI_ShowConsole(&CallbackContext);
	}
}

void BaseUI_InstallHooks(void)
{
	CreateInterfaceFn fnCreateInterface = g_pMetaHookAPI->GetEngineFactory();

	if (!fnCreateInterface)
	{
		Sys_Error("Failed to get engine factory.");
		return;
	}

	if (gPrivateFuncs.SDL_GetWindowPosition)
	{
		baseui = (IBaseUI*)fnCreateInterface(BASEUI_INTERFACE_VERSION, NULL);

		if (!baseui)
		{
			Sys_Error("Failed to get interface \"" BASEUI_INTERFACE_VERSION "\" from engine.");
			return;
		}
	}
	else
	{
		baseui_legacy = (IBaseUI_Legacy*)fnCreateInterface(BASEUI_INTERFACE_VERSION, NULL);

		if (!baseui_legacy)
		{
			Sys_Error("Failed to get interface \"" BASEUI_INTERFACE_VERSION "\" from engine.");
			return;
		}
	}

	gameuifuncs = (IGameUIFuncs *)fnCreateInterface(VENGINE_GAMEUIFUNCS_VERSION, NULL);
	
	if (!gameuifuncs)
	{
		Sys_Error("Failed to get interface \"" VENGINE_GAMEUIFUNCS_VERSION "\" from engine.");
		return;
	}

	if (baseui)
	{
		PVOID* ProxyVFTable = *(PVOID**)&s_BaseUIProxy;

		g_pMetaHookAPI->VFTHook(baseui, 0, 1, ProxyVFTable[1], (void**)&m_pfnCBaseUI_Initialize);
		g_pMetaHookAPI->VFTHook(baseui, 0, 2, ProxyVFTable[2], (void**)&m_pfnCBaseUI_Start);
		g_pMetaHookAPI->VFTHook(baseui, 0, 3, ProxyVFTable[3], (void**)&m_pfnCBaseUI_Shutdown);
		g_pMetaHookAPI->VFTHook(baseui, 0, 4, ProxyVFTable[4], (void**)&m_pfnCBaseUI_Key_Event);
		g_pMetaHookAPI->VFTHook(baseui, 0, 5, ProxyVFTable[5], (void**)&m_pfnCBaseUI_CallEngineSurfaceAppHandler);
		g_pMetaHookAPI->VFTHook(baseui, 0, 6, ProxyVFTable[6], (void**)&m_pfnCBaseUI_Paint);
		g_pMetaHookAPI->VFTHook(baseui, 0, 7, ProxyVFTable[7], (void**)&m_pfnCBaseUI_HideGameUI);
		g_pMetaHookAPI->VFTHook(baseui, 0, 8, ProxyVFTable[8], (void**)&m_pfnCBaseUI_ActivateGameUI);
		g_pMetaHookAPI->VFTHook(baseui, 0, 9, ProxyVFTable[9], (void**)&m_pfnCBaseUI_HideConsole);
		g_pMetaHookAPI->VFTHook(baseui, 0, 10, ProxyVFTable[10], (void**)&m_pfnCBaseUI_ShowConsole);
	}
	else
	{
		PVOID* ProxyVFTable = *(PVOID**)&s_BaseUILegacyProxy;

		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 1, ProxyVFTable[1], (void**)&m_pfnCBaseUI_Initialize);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 2, ProxyVFTable[2], (void**)&m_pfnCBaseUI_Start);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 3, ProxyVFTable[3], (void**)&m_pfnCBaseUI_Shutdown);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 4, ProxyVFTable[4], (void**)&m_pfnCBaseUI_Key_Event);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 5, ProxyVFTable[5], (void**)&m_pfnCBaseUI_CallEngineSurfaceWndProc);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 6, ProxyVFTable[6], (void**)&m_pfnCBaseUI_Paint);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 7, ProxyVFTable[7], (void**)&m_pfnCBaseUI_HideGameUI);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 8, ProxyVFTable[8], (void**)&m_pfnCBaseUI_ActivateGameUI);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 9, ProxyVFTable[9], (void**)&m_pfnCBaseUI_HideConsole);
		g_pMetaHookAPI->VFTHook(baseui_legacy, 0, 10, ProxyVFTable[10], (void**)&m_pfnCBaseUI_ShowConsole);
	}

	Install_InlineHook(EngineVGUI2_Panel_Init);
}

void BaseUI_UninstallHooks(void)
{
	Uninstall_Hook(EngineVGUI2_Panel_Init);
}