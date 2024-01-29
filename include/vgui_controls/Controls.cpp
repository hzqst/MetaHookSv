#include <metahook.h>
#include "Controls.h"
#include <locale.h>
#include <VGUI/IEngineVGui.h>
#include <IEngineSurface.h>
#include "plugins.h"

#include <capstone.h>

vgui::IInput *g_pVGuiInput = NULL;
vgui::ISystem *g_pVGuiSystem = NULL;
vgui::IVGui *g_pVGui = NULL;
vgui::IPanel *g_pVGuiPanel = NULL;
vgui::IPanel2 *g_pVGuiPanel2 = NULL;
vgui::ILocalize *g_pVGuiLocalize = NULL;

vgui::ISurface *g_pSurface = NULL;
vgui::ISurface_HL25 *g_pSurface_HL25 = NULL;
vgui::ISchemeManager *g_pScheme = NULL;
vgui::ISchemeManager_HL25 *g_pScheme_HL25 = NULL;

IFileSystem *g_pFullFileSystem = NULL;
IFileSystem_HL25 *g_pFullFileSystem_HL25 = NULL;
IKeyValuesSystem *g_pKeyValuesSystem = NULL;
vgui::IEngineVGui *g_pEngineVGui = NULL;

IEngineSurface *staticSurface = NULL;
IEngineSurface_HL25 *staticSurface_HL25 = NULL;

namespace vgui
{
	static char g_szControlsModuleName[256] = {0};

const char* GetControlsModuleName(void)
{
	return g_szControlsModuleName;
}

bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories)
{
	strncpy(g_szControlsModuleName, moduleName, sizeof(g_szControlsModuleName));
	g_szControlsModuleName[sizeof(g_szControlsModuleName) - 1] = 0;

	setlocale(LC_CTYPE, "");
	setlocale(LC_TIME, "");
	setlocale(LC_COLLATE, "");
	setlocale(LC_MONETARY, "");

	g_pFullFileSystem = (IFileSystem *)factoryList[2](FILESYSTEM_INTERFACE_VERSION, NULL);

	g_pVGuiInput = (IInput *)factoryList[1](VGUI_INPUT_INTERFACE_VERSION, NULL);
	g_pVGuiSystem = (ISystem *)factoryList[1](VGUI_SYSTEM_INTERFACE_VERSION, NULL);
	g_pVGui = (IVGui *)factoryList[1](VGUI_IVGUI_INTERFACE_VERSION, NULL);
	g_pVGuiPanel = (IPanel *)factoryList[1](VGUI_PANEL_INTERFACE_VERSION, NULL);
	g_pVGuiPanel2 = (IPanel2*)factoryList[1](VGUI_PANEL2_INTERFACE_VERSION, NULL);
	g_pVGuiLocalize = (ILocalize *)factoryList[1](VGUI_LOCALIZE_INTERFACE_VERSION, NULL);

	g_pEngineVGui = (IEngineVGui *)factoryList[0](VENGINE_VGUI_VERSION, NULL);

	if (!g_pFullFileSystem || !g_pKeyValuesSystem || !g_pVGuiInput || !g_pVGuiSystem || !g_pVGui || !g_pVGuiPanel || !g_pVGuiLocalize)
	{
		g_pMetaHookAPI->SysError("vgui_controls is missing a required interface!\n");
		return false;
	}

	return true;
}
}