#include <metahook.h>
#include <VGUI\IScheme.h>
#include <VGUI\ILocalize.h>
#include <VGUI\ISurface.h>
#include <VGUI\IInput.h>
#include <IEngineSurface.h>
#include <IKeyValuesSystem.h>
#include <IGameUIFuncs.h>
#include <IVGUI2Extension.h>
#include "plugins.h"

IGameUIFuncs* gameuifuncs = NULL;

extern vgui::ISurface* g_pSurface;
extern vgui::ISurface_HL25* g_pSurface_HL25;
extern vgui::ISchemeManager* g_pScheme;
extern vgui::ISchemeManager_HL25* g_pScheme_HL25;
extern IKeyValuesSystem* g_pKeyValuesSystem;
extern IEngineSurface* staticSurface;
extern IEngineSurface_HL25* staticSurface_HL25;

namespace vgui
{
	bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

class CVGUI2Extension_BaseUICallbacks : public IVGUI2Extension_BaseUICallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		HINTERFACEMODULE hVGUI2 = (HINTERFACEMODULE)GetModuleHandleA("vgui2.dll");

		if (hVGUI2)
		{
			CreateInterfaceFn fnVGUI2CreateInterface = Sys_GetFactory(hVGUI2);

			if (g_iEngineType == ENGINE_GOLDSRC_HL25)
				g_pScheme_HL25 = (vgui::ISchemeManager_HL25*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL);
			else
				g_pScheme = (vgui::ISchemeManager*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL);

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
	}

	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion) override
	{

	}

	void Shutdown(void)
	{

	}

	void Key_Event(int& down, int& keynum, const char*& pszCurrentBinding, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void CallEngineSurfaceProc(void*& pevent, void*& userData, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void Paint(int& x, int& y, int& right, int& bottom, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ActivateGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideConsole(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ShowConsole(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}
};

static CVGUI2Extension_BaseUICallbacks s_BaseUICallbacks;

void BaseUI_InstallHooks(void)
{
	CreateInterfaceFn fnCreateInterface = g_pMetaHookAPI->GetEngineFactory();

	if (!fnCreateInterface)
	{
		g_pMetaHookAPI->SysError("Failed to get engine factory.");
		return;
	}

	gameuifuncs = (IGameUIFuncs *)fnCreateInterface(VENGINE_GAMEUIFUNCS_VERSION, NULL);
	
	if (!fnCreateInterface)
	{
		g_pMetaHookAPI->SysError("Failed to get interface \"" VENGINE_GAMEUIFUNCS_VERSION "\" from engine.");
		return;
	}

	VGUI2Extension()->RegisterBaseUICallbacks(&s_BaseUICallbacks);
}

void BaseUI_UninstallHooks(void)
{
	VGUI2Extension()->UnregisterBaseUICallbacks(&s_BaseUICallbacks);
}