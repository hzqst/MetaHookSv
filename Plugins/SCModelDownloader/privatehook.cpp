#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"
#include "exportfuncs.h"

static_assert(METAHOOK_API_VERSION >= 109, "SCModelDownloader resolves the engine player-model symbols from gamedata and requires MetaHook API 109 (ResolveGameSymbol)");

private_funcs_t gPrivateFuncs = {0};

player_model_t(*DM_PlayerState)[MAX_CLIENTS];

player_info_t* cl_players = nullptr;
player_info_sc_t* cl_players_sc = nullptr;

static hook_t* g_phook_R_StudioDrawPlayer = NULL;
static hook_t* g_phook_studioapi_SetupPlayerModel = NULL;

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

// On gamedata failure, print diagnostics (symbol / buildnum / CRC64 / status string) and abort via Sys_Error.
static void ReportSymbolFailure(const char* symbolName, mh_gamesymbol_status_t status)
{
	uint64_t crc64 = 0;
	mh_gamesymbol_status_t crcSt = g_pMetaHookAPI->GetModuleCRC64(g_EngineDLLInfo.ImageBase, &crc64);

	if (crcSt == MH_GAMESYMBOL_OK)
	{
		Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nCRC64: %016llx\nReason: %s",
			symbolName, g_dwEngineBuildnum, (unsigned long long)crc64, g_pMetaHookAPI->GetGameSymbolStatusString(status));
	}
	else
	{
		Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nReason: %s",
			symbolName, g_dwEngineBuildnum, g_pMetaHookAPI->GetGameSymbolStatusString(status));
	}
}

// The return value is the real-image VA of the gamedata record.
static PVOID ResolveGameSymbolOrError(const char* symbolName, mh_gamesymbol_kind_t expectedKind)
{
	PVOID va = NULL;
	mh_gamesymbol_status_t st = g_pMetaHookAPI->ResolveGameSymbol(g_EngineDLLInfo.ImageBase, symbolName, expectedKind, &va);

	if (st == MH_GAMESYMBOL_OK)
		return va;

	ReportSymbolFailure(symbolName, st);
	return NULL;
}

void Engine_FillAddress(void)
{
	gPrivateFuncs.R_StudioDrawPlayer = (decltype(gPrivateFuncs.R_StudioDrawPlayer))ResolveGameSymbolOrError("R_StudioDrawPlayer", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.studioapi_SetupPlayerModel = (decltype(gPrivateFuncs.studioapi_SetupPlayerModel))ResolveGameSymbolOrError("studioapi_SetupPlayerModel", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))ResolveGameSymbolOrError("Host_IsSinglePlayerGame", MH_GAMESYMBOL_KIND_FUNCTION);

	DM_PlayerState = (decltype(DM_PlayerState))ResolveGameSymbolOrError("DM_PlayerState", MH_GAMESYMBOL_KIND_GLOBAL);

	auto* clPlayersModel = (unsigned char*)ResolveGameSymbolOrError("cl_players_model", MH_GAMESYMBOL_KIND_GLOBAL);

	if (!clPlayersModel)
		return;

	// cl_players_model is the address of the cl.players[0].model member itself,
	// not a pointer slot: subtract the member offset to recover the array head.
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		cl_players_sc = reinterpret_cast<player_info_sc_t*>(clPlayersModel - offsetof(player_info_t, model));
	}
	else
	{
		cl_players = reinterpret_cast<player_info_t*>(clPlayersModel - offsetof(player_info_t, model));
	}
}

void Engine_InstallHook(void)
{
	Install_InlineHook(R_StudioDrawPlayer);
	Install_InlineHook(studioapi_SetupPlayerModel);

	if (!g_phook_R_StudioDrawPlayer || !g_phook_studioapi_SetupPlayerModel)
	{
		Uninstall_Hook(R_StudioDrawPlayer);
		Uninstall_Hook(studioapi_SetupPlayerModel);
		Sys_Error("%s", "Failed to install the player-model hooks");
	}
}

void Engine_UninstallHook(void)
{
	Uninstall_Hook(R_StudioDrawPlayer);
	Uninstall_Hook(studioapi_SetupPlayerModel);
}
