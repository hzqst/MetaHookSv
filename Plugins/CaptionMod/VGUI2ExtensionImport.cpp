#include <metahook.h>
#include <IScheme2.h>
#include <ISurface2.h>
#include "plugins.h"
#include "VGUI2ExtensionImport.h"

IVGUI2Extension* g_pVGUI2Extension = NULL;
IDpiManager* g_pDpiManager = NULL;
vgui::ISchemeManager2* g_pVGuiSchemeManager = NULL;
vgui::ISurface2* g_pVGuiSurface = NULL;

void VGUI2Extension_Init()
{
	auto hVGUI2Extension = GetModuleHandleA("VGUI2Extension.dll");

	if (!hVGUI2Extension)
	{
		Sys_Error("VGUI2Extension.dll is required!");
		return;
	}

	auto factory = Sys_GetFactory((HINTERFACEMODULE)hVGUI2Extension);

	if (!factory)
	{
		Sys_Error("Could not get factory from VGUI2Extension.dll");
		return;
	}

	g_pVGUI2Extension = (decltype(g_pVGUI2Extension))factory(VGUI2_EXTENSION_INTERFACE_VERSION, NULL);

	if (!g_pVGUI2Extension)
	{
		Sys_Error("Could not get interface \" VGUI2_EXTENSION_INTERFACE_VERSION \" from VGUI2Extension.dll");
		return;
	}

	g_pDpiManager = (decltype(g_pDpiManager))factory(DPI_MANAGER_INTERFACE_VERSION, NULL);

	if (!g_pDpiManager)
	{
		Sys_Error("Could not get interface \" DPI_MANAGER_INTERFACE_VERSION \" from VGUI2Extension.dll");
		return;
	}

	g_pVGuiSurface = (decltype(g_pVGuiSurface))factory(VGUI_SURFACE2_INTERFACE_VERSION, NULL);

	if (!g_pVGuiSurface)
	{
		Sys_Error("Could not get interface \" VGUI_SURFACE2_INTERFACE_VERSION \" from VGUI2Extension.dll");
		return;
	}

	g_pVGuiSchemeManager = (decltype(g_pVGuiSchemeManager))factory(VGUI_SCHEME2_INTERFACE_VERSION, NULL);

	if (!g_pVGuiSchemeManager)
	{
		Sys_Error("Could not get interface \" VGUI_SCHEME2_INTERFACE_VERSION \" from VGUI2Extension.dll");
		return;
	}
}

void VGUI2Extension_Shutdown()
{

}

IVGUI2Extension* VGUI2Extension()
{
	return g_pVGUI2Extension;
}

IDpiManager* DpiManager()
{
	return g_pDpiManager;
}