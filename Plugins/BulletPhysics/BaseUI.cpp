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

extern IKeyValuesSystem* g_pKeyValuesSystem;

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

			g_pKeyValuesSystem = (IKeyValuesSystem*)fnVGUI2CreateInterface(KEYVALUESSYSTEM_INTERFACE_VERSION, NULL);

			if (g_pKeyValuesSystem) {
				g_pKeyValuesSystem->RegisterSizeofKeyValues(sizeof(KeyValues));
			}
		}
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

	void CallEngineSurfaceAppProc(void*& pevent, void*& userData, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void CallEngineSurfaceWndProc(void*& hwnd, unsigned int& msg, unsigned int& wparam, long& lparam, VGUI2Extension_CallbackContext* CallbackContext) override
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
	if (!VGUI2Extension())
	{
		return;
	}

	CreateInterfaceFn fnCreateInterface = g_pMetaHookAPI->GetEngineFactory();

	if (!fnCreateInterface)
	{
		Sys_Error("Failed to get engine factory.");
		return;
	}

	gameuifuncs = (IGameUIFuncs *)fnCreateInterface(VENGINE_GAMEUIFUNCS_VERSION, NULL);
	
	if (!fnCreateInterface)
	{
		Sys_Error("Failed to get interface \"" VENGINE_GAMEUIFUNCS_VERSION "\" from engine.");
		return;
	}

	VGUI2Extension()->RegisterBaseUICallbacks(&s_BaseUICallbacks);
}

void BaseUI_UninstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->UnregisterBaseUICallbacks(&s_BaseUICallbacks);
}