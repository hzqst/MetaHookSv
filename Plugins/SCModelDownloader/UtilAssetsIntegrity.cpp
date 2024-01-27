#include <metahook.h>
#include "plugins.h"
#include "UtilAssetsIntegrity.h"

static HINTERFACEMODULE g_hUtilAssetsIntegrity;
static IUtilAssetsIntegrity* g_pUtilAssetsIntegrity;

IUtilAssetsIntegrity* UtilAssetsIntegrity()
{
	return g_pUtilAssetsIntegrity;
}

void UtilAssetsIntegrity_Init()
{
	g_hUtilAssetsIntegrity = Sys_LoadModule("UtilAssetsIntegrity.dll");

	if (!g_hUtilAssetsIntegrity)
	{
		g_pMetaHookAPI->SysError("Could not load UtilAssetsIntegrity.dll");
		return;
	}

	auto factory = Sys_GetFactory(g_hUtilAssetsIntegrity);

	if (!factory)
	{
		g_pMetaHookAPI->SysError("Could not get factory from UtilAssetsIntegrity.dll");
		return;
	}

	g_pUtilAssetsIntegrity = (decltype(g_pUtilAssetsIntegrity))factory(UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION, NULL);

	if (!g_pUtilAssetsIntegrity)
	{
		g_pMetaHookAPI->SysError("Could not get UtilAssetsIntegrity from UtilAssetsIntegrity.dll");
		return;
	}
}

void UtilAssetsIntegrity_Shutdown()
{
	if (g_hUtilAssetsIntegrity)
	{
		Sys_FreeModule(g_hUtilAssetsIntegrity);
		g_hUtilAssetsIntegrity = NULL;
	}
}
