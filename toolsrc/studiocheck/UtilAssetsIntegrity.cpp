#include <metahook.h>
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
		fprintf(stderr, "[Error] Could not load UtilAssetsIntegrity.dll\n");
		return;
	}

	auto factory = Sys_GetFactory(g_hUtilAssetsIntegrity);

	if (!factory)
	{
		fprintf(stderr, "[Error] Could not get factory from UtilAssetsIntegrity.dll\n");
		return;
	}

	g_pUtilAssetsIntegrity = (decltype(g_pUtilAssetsIntegrity))factory(UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION, NULL);

	if (!g_pUtilAssetsIntegrity)
	{
		fprintf(stderr, "[Error] Could not get UtilAssetsIntegrity from UtilAssetsIntegrity.dll\n");
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
