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
	if (1)
	{
		g_hUtilHTTPClient = Sys_LoadModule("UtilHTTPClient_libcurl.dll");

		if (g_hUtilHTTPClient)
		{
			auto factory = Sys_GetFactory(g_hUtilHTTPClient);

			if (!factory)
			{
				Sys_Error("Could not get factory from UtilHTTPClient_libcurl.dll");
				return;
			}

			g_pUtilHTTPClient = (decltype(g_pUtilHTTPClient))factory(UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION, NULL);

			if (!g_pUtilHTTPClient)
			{
				Sys_Error("Could not get UtilHTTPClient from UtilHTTPClient_libcurl.dll");
				return;
			}

			if (g_pUtilHTTPClient)
			{
				CUtilHTTPClientCreationContext context;
				g_pUtilHTTPClient->Init(&context);
			}
			return;
		}
	}

	if (1)
	{
		g_hUtilHTTPClient = Sys_LoadModule("UtilHTTPClient_SteamAPI.dll");

		if (g_hUtilHTTPClient)
		{
			auto factory = Sys_GetFactory(g_hUtilHTTPClient);

			if (!factory)
			{
				Sys_Error("Could not get factory from UtilHTTPClient_SteamAPI.dll");
				return;
			}

			g_pUtilHTTPClient = (decltype(g_pUtilHTTPClient))factory(UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION, NULL);

			if (!g_pUtilHTTPClient)
			{
				Sys_Error("Could not get UtilHTTPClient from UtilHTTPClient_SteamAPI.dll");
				return;
			}

			if (g_pUtilHTTPClient)
			{
				CUtilHTTPClientCreationContext context;
				g_pUtilHTTPClient->Init(&context);
			}
			return;
		}
	}
}

void UtilHTTPClient_InitSteamAPI()
{
	g_hUtilHTTPClient = Sys_LoadModule("UtilHTTPClient_SteamAPI.dll");

	if (!g_hUtilHTTPClient)
	{
		Sys_Error("Could not load UtilHTTPClient_SteamAPI.dll");
		return;
	}

	auto factory = Sys_GetFactory(g_hUtilHTTPClient);

	if (!factory)
	{
		Sys_Error("Could not get factory from UtilHTTPClient_SteamAPI.dll");
		return;
	}

	g_pUtilHTTPClient = (decltype(g_pUtilHTTPClient))factory(UTIL_HTTPCLIENT_STEAMAPI_INTERFACE_VERSION, NULL);

	if (!g_pUtilHTTPClient)
	{
		Sys_Error("Could not get UtilHTTPClient from UtilHTTPClient_SteamAPI.dll");
		return;
	}

	if (g_pUtilHTTPClient)
	{
		CUtilHTTPClientCreationContext context;
		g_pUtilHTTPClient->Init(&context);
	}
}

void UtilHTTPClient_InitLibCurl()
{
	g_hUtilHTTPClient = Sys_LoadModule("UtilHTTPClient_libcurl.dll");

	if (!g_hUtilHTTPClient)
	{
		Sys_Error("Could not load UtilHTTPClient_libcurl.dll");
		return;
	}

	auto factory = Sys_GetFactory(g_hUtilHTTPClient);

	if (!factory)
	{
		Sys_Error("Could not get factory from UtilHTTPClient_libcurl.dll");
		return;
	}

	g_pUtilHTTPClient = (decltype(g_pUtilHTTPClient))factory(UTIL_HTTPCLIENT_LIBCURL_INTERFACE_VERSION, NULL);

	if (!g_pUtilHTTPClient)
	{
		Sys_Error("Could not get UtilHTTPClient from UtilHTTPClient_libcurl.dll");
		return;
	}

	if (g_pUtilHTTPClient)
	{
		CUtilHTTPClientCreationContext context;
		g_pUtilHTTPClient->Init(&context);
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
