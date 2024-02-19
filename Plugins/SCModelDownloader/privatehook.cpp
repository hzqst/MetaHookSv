#include <metahook.h>
#include "privatehook.h"

private_funcs_t gPrivateFuncs = {0};

player_model_t(*DM_PlayerState)[MAX_CLIENTS];

static HMODULE g_hServerBrowser = NULL;

void NewSteamAPI_Shutdown()
{

}

void ServerBrowser_InstallHook(HMODULE hModule)
{
	//idk why Sven Co-op Team added SteamAPI_Shutdown inside ServerBrowser's CVGUIModule::Shutdown which causes Steam API to be unavailable even before client's HUD_Shutdown.
	if (g_pMetaHookAPI->ModuleHasImport(hModule, "steam_api.dll") && !g_pMetaHookAPI->ModuleHasImportEx(hModule, "steam_api.dll", "SteamAPI_Shutdown"))
		return;

	g_pMetaHookAPI->IATHook(hModule, "steam_api.dll", "SteamAPI_Shutdown", NewSteamAPI_Shutdown, NULL);
}

void ServerBrowser_UninstallHook(HMODULE hModule)
{

}

void DllLoadNotification(mh_load_dll_notification_context_t* ctx)
{
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)
	{
		if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"serverbrowser.dll"))
		{
			g_hServerBrowser = ctx->hModule;
			ServerBrowser_InstallHook(ctx->hModule);
		}
	}
	else if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)
	{
		if (ctx->hModule == g_hServerBrowser)
		{
			ServerBrowser_UninstallHook(ctx->hModule);
			g_hServerBrowser = NULL;
		}
	}
}