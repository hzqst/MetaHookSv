#include <metahook.h>
#include "plugins.h"
#include "UtilHTTPClient.h"

static HINTERFACEMODULE g_hUtilHTTPClient;
static IUtilHTTPClient* g_pUtilHTTPClient;

IUtilHTTPClient* UtilHTTPClient()
{
	return g_pUtilHTTPClient;
}

void UtilHTTPClient_Init()
{
	g_hUtilHTTPClient = Sys_LoadModule("UtilHTTPClient_SteamAPI.dll");

	if (!g_hUtilHTTPClient)
	{
		g_pMetaHookAPI->SysError("Could not load UtilHTTPClient_SteamAPI.dll");
		return;
	}

	auto factory = Sys_GetFactory(g_hUtilHTTPClient);

	if (!factory)
	{
		g_pMetaHookAPI->SysError("Could not get factory from UtilHTTPClient_SteamAPI.dll");
		return;
	}

	g_pUtilHTTPClient = (decltype(g_pUtilHTTPClient))factory(UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION, NULL);

	if (!g_pUtilHTTPClient)
	{
		g_pMetaHookAPI->SysError("Could not get UtilHTTPClient from UtilHTTPClient_SteamAPI.dll");
		return;
	}

	if (g_pUtilHTTPClient)
	{
		g_pUtilHTTPClient->Init();
	}
}

void UtilHTTPClient_RunFrame()
{
	if (g_pUtilHTTPClient)
	{
		g_pUtilHTTPClient->RunFrame();
	}
}

void UtilHTTPClient_Shutdown()
{
	if (g_pUtilHTTPClient)
	{
		g_pUtilHTTPClient->Shutdown();
		g_pUtilHTTPClient->Destroy();
		g_pUtilHTTPClient = nullptr;
	}

	if (g_hUtilHTTPClient)
	{
		Sys_FreeModule(g_hUtilHTTPClient);
		g_hUtilHTTPClient = NULL;
	}
}
