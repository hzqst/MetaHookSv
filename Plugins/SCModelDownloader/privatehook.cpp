#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"

#define R_STUDIOCHANGEPLAYERMODEL_SIG_SVENGINE "\x2A\x33\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x0B"

private_funcs_t gPrivateFuncs = {0};

player_model_t(*DM_PlayerState)[MAX_CLIENTS];

static HMODULE g_hServerBrowser = NULL;
static hook_t * g_phook_R_StudioChangePlayerModel = NULL;

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

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		auto R_StudioChangePlayerModel_VA = Search_Pattern(R_STUDIOCHANGEPLAYERMODEL_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.R_StudioChangePlayerModel = (decltype(gPrivateFuncs.R_StudioChangePlayerModel))ConvertDllInfoSpace(R_StudioChangePlayerModel_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_StudioChangePlayerModel);
}

void Engine_InstallHook(void)
{
/*
* Cannot install inlinehook because:
* only 4 bytes available in the prologue
.text:01D90DE0                                     ; void R_StudioChangePlayerModel()
.text:01D90DE0                                     R_StudioChangePlayerModel proc near     ; CODE XREF: StudioDrawModel+9D↑p
.text:01D90DE0                                                                             ; StudioDrawModel:loc_1D8A540↑p ...
.text:01D90DE0 56                                                  push    esi
.text:01D90DE1 33 D2                                               xor     edx, edx
.text:01D90DE3 57                                                  push    edi
*/

//	Install_InlineHook(R_StudioChangePlayerModel);
}

void Engine_UninstallHook(void)
{
//	Uninstall_Hook(R_StudioChangePlayerModel);
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{

}

void Client_InstallHooks(void)
{

}

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
	if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
	{
		auto addr_VA = (ULONG_PTR)addr;
		auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

		return (PVOID)VA_from_RVA(addr, TargetDllInfo);
	}

	return nullptr;
}

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo)
{
	if ((ULONG_PTR)vftable > (ULONG_PTR)RealDllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)RealDllInfo.ImageBase + RealDllInfo.ImageSize)
	{
		ULONG_PTR vftable_VA = (ULONG_PTR)vftable;
		ULONG vftable_RVA = RVA_from_VA(vftable, RealDllInfo);
		auto vftable_DllInfo = (decltype(vftable))VA_from_RVA(vftable, DllInfo);

		auto vf_VA = (ULONG_PTR)vftable_DllInfo[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}
	else if ((ULONG_PTR)vftable > (ULONG_PTR)DllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)DllInfo.ImageBase + DllInfo.ImageSize)
	{
		auto vf_VA = (ULONG_PTR)vftable[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}

	return vftable[index];
}