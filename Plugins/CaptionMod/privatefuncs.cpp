#include <metahook.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "privatefuncs.h"

double *cl_time = NULL;
double *cl_oldtime = NULL;

int* cl_viewentity = NULL;

char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
int *cszrawsentences = NULL;

vec3_t *listener_origin = NULL;

qboolean *scr_drawloading = NULL;

CreateInterfaceFn *g_pClientFactory = NULL;

void* gViewport = NULL;

private_funcs_t gPrivateFuncs = { 0 };

static hook_t *g_phook_S_StartDynamicSound = NULL;
static hook_t *g_phook_S_StartStaticSound = NULL;
static hook_t *g_phook_pfnTextMessageGet = NULL;
static hook_t* g_phook_pfnServerCmdUnreliable = NULL;
static hook_t *g_phook_TextMessageParse = NULL;
static hook_t* g_phook_COM_ExplainDisconnection = NULL;
static hook_t *g_phook_WeaponsResource_SelectSlot = NULL;
static hook_t* g_phook_SCClient_SoundEngine_LoadSoundList = NULL;
static hook_t *g_phook_SCClient_SoundEngine_PlayFMODSound = NULL;
static hook_t *g_phook_FMOD_System_playSound = NULL;

static HMODULE g_hFMODEx = NULL;

void FMOD_InstallHooks(HMODULE fmodex)
{
	gPrivateFuncs.FMOD_Sound_getLength = (decltype(gPrivateFuncs.FMOD_Sound_getLength))GetProcAddress(fmodex, "?getLength@Sound@FMOD@@QAG?AW4FMOD_RESULT@@PAII@Z");
	gPrivateFuncs.FMOD_System_playSound = (decltype(gPrivateFuncs.FMOD_System_playSound))GetProcAddress(fmodex, "?playSound@System@FMOD@@QAG?AW4FMOD_RESULT@@W4FMOD_CHANNELINDEX@@PAVSound@2@_NPAPAVChannel@2@@Z");

	if (gPrivateFuncs.FMOD_System_playSound)
	{
	//	Install_InlineHook(FMOD_System_playSound);
	}
}

void FMOD_UninstallHooks(HMODULE fmodex)
{
	//Uninstall_Hook(FMOD_System_playSound);
}

bool SCR_IsLoadingVisible(void)
{
	return scr_drawloading && (*scr_drawloading) == 1 ? true : false;
}

void Engine_FillAddress_GetClientTime(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	cl_time = (double*)GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "cl_time", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_oldtime = (double*)GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "cl_oldtime", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_S_FindName(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "S_FindName", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_S_StartDynamicSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "S_StartDynamicSound", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_S_StartStaticSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "S_StartStaticSound", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_S_LoadSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "S_LoadSound", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_TextMessageParse(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.TextMessageParse = (decltype(gPrivateFuncs.TextMessageParse))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "TextMessageParse", MH_GAMESYMBOL_KIND_FUNCTION);
}
// End of Selection


void Engine_FillAddress_COM_ExplainDisconnection(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.COM_ExplainDisconnection = (decltype(gPrivateFuncs.COM_ExplainDisconnection))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "COM_ExplainDisconnection", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.COM_ExtendedExplainDisconnection = (decltype(gPrivateFuncs.COM_ExtendedExplainDisconnection))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "COM_ExtendedExplainDisconnection", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_SequenceGetSentenceByIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "SequenceGetSentenceByIndex", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_SCR_BeginLoadingPlaque(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	scr_drawloading = (decltype(scr_drawloading))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "scr_drawloading", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_viewentity = NULL;
	*/

	cl_viewentity = (decltype(cl_viewentity))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "cl_viewentity", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_ListenerOrigin(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	listener_origin = (decltype(listener_origin))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "listener_origin", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_VOX_LookupString(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*

		char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
		int *cszrawsentences = NULL;
	*/

	cszrawsentences = (decltype(cszrawsentences))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "cszrawsentences", MH_GAMESYMBOL_KIND_GLOBAL);
	rgpszrawsentence = (decltype(rgpszrawsentence))GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "rgpszrawsentence", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Engine_FillAddress_GetClientTime(DllInfo, RealDllInfo);
	Engine_FillAddress_S_FindName(DllInfo, RealDllInfo);
	Engine_FillAddress_S_StartDynamicSound(DllInfo, RealDllInfo);
	Engine_FillAddress_S_StartStaticSound(DllInfo, RealDllInfo);
	Engine_FillAddress_S_LoadSound(DllInfo, RealDllInfo);
	Engine_FillAddress_TextMessageParse(DllInfo, RealDllInfo);
	Engine_FillAddress_COM_ExplainDisconnection(DllInfo, RealDllInfo);
	Engine_FillAddress_SequenceGetSentenceByIndex(DllInfo, RealDllInfo);
	Engine_FillAddress_SCR_BeginLoadingPlaque(DllInfo, RealDllInfo);
	Engine_FillAddress_CL_ViewEntityVars(DllInfo, RealDllInfo);
	Engine_FillAddress_ListenerOrigin(DllInfo, RealDllInfo);
	Engine_FillAddress_VOX_LookupString(DllInfo, RealDllInfo);
}

void Client_FillAddress_SCClient_SoundEngine(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//The catalog publishes the backing singleton pointer, not the engine's
	//lazy-construction accessor; consumers dereference it through
	//SCClient_SoundEngine_GetInstance and tolerate a null value.
	gPrivateFuncs.SCClient_soundengine = (decltype(gPrivateFuncs.SCClient_soundengine))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "CClient_SoundEngine_m_pSoundEngine", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Client_FillAddress_SCClient_SoundEngine_maxsentences(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//`CClient_SoundEngine.m_iSentenceCount` is the sentence-handle count at
	//object offset; the old locator re-derived exactly this offset by
	//disassembling the `Sentence length too long` write-back in LoadSoundList.
	gPrivateFuncs.SCClient_soundengine_maxsentences = GamedataQueryStructMember(RealDllInfo.ImageBase, "client", "CClient_SoundEngine.m_iSentenceCount");
}

void Client_FillAddress_SCClient_SoundEngine_LoadSoundList(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.SCClient_SoundEngine_LoadSoundList = (decltype(gPrivateFuncs.SCClient_SoundEngine_LoadSoundList))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "CClient_SoundEngine_LoadSoundList", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_SoundEngine_PlayFMODSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.SCClient_SoundEngine_PlayFMODSound = (decltype(gPrivateFuncs.SCClient_SoundEngine_PlayFMODSound))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "CClient_SoundEngine_PlayFMODSound", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_SoundEngine_LookupSoundBySentenceIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySentenceIndex = (decltype(gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySentenceIndex))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "CClient_SoundEngine_LookupSoundBySentenceIndex", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_SoundEngine_LookupSoundBySample(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySample = (decltype(gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySample))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "CClient_SoundEngine_LookupSoundBySample", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_GetClientColor(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "GetClientColor", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_GameViewport_AllowedToPrintText(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into client dll vars.
	*/

	gViewport = (decltype(gViewport))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "gViewPort", MH_GAMESYMBOL_KIND_GLOBAL);
	gPrivateFuncs.GameViewport_AllowedToPrintText = (decltype(gPrivateFuncs.GameViewport_AllowedToPrintText))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "TeamFortressViewport_AllowedToPrintText", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_GameViewport_IsScoreBoardVisible(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GameViewport_IsScoreBoardVisible = (decltype(gPrivateFuncs.GameViewport_IsScoreBoardVisible))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "TeamFortressViewport_IsScoreBoardVisible", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_WeaponsResource_SelectSlot(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.WeaponsResource_SelectSlot = (decltype(gPrivateFuncs.WeaponsResource_SelectSlot))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "WeaponsResource_SelectSlot", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_CHud_GetBorderSize(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gHud = (decltype(gHud))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "gHUD", MH_GAMESYMBOL_KIND_GLOBAL);
	gPrivateFuncs.CHud_GetBorderSize = (decltype(gPrivateFuncs.CHud_GetBorderSize))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "CHud_GetBorderSize", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		g_bIsSvenCoop = true;

		Client_FillAddress_SCClient_SoundEngine(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_maxsentences(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_LoadSoundList(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_PlayFMODSound(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_LookupSoundBySentenceIndex(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_LookupSoundBySample(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_GetClientColor(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_GameViewport_AllowedToPrintText(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_GameViewport_IsScoreBoardVisible(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_WeaponsResource_SelectSlot(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_CHud_GetBorderSize(DllInfo, RealDllInfo);
	}
}

void Client_FillAddress_CounterStrike_GetTextColor(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//`GetTextColor` is published for five cstrike builds and czero-8684, but not
	//for cstrike-10210, czero-10210 or the czeror family - the same builds on
	//which the old signature locator found nothing. `GetClientColor` is published
	//for every cstrike / czero / czeror client.
	gPrivateFuncs.GetTextColor = (decltype(gPrivateFuncs.GetTextColor))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "client", "GetTextColor", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "client", "GetClientColor", MH_GAMESYMBOL_KIND_FUNCTION);

	if (0 != strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		if (!gPrivateFuncs.GetTextColor)
		{
			//The location colour fallback is catalog-covered under its real name
			//`g_LocationColor`, which is what `mov eax, offset g_LocationColor` in the
			//old pattern `33 C0 EB ?? B8 <imm32> EB ??` was loading. Only
			//cstrike-10210 and czero-10210 reach this branch - the two non-czeror
			//clients that publish no `GetTextColor` - and both publish it, so the
			//resolve stays required. `czeror` publishes neither symbol, which is why
			//the game-directory guard above is still needed.
			gPrivateFuncs.LocationColor = (decltype(gPrivateFuncs.LocationColor))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "g_LocationColor", MH_GAMESYMBOL_KIND_GLOBAL);
		}
	}
}
// End of Selection

void Client_FillAddress_CounterStrike(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		Client_FillAddress_CounterStrike_GetTextColor(DllInfo, RealDllInfo);
	}
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Client_FillAddress_SCClient(DllInfo, RealDllInfo);

	Client_FillAddress_CounterStrike(DllInfo, RealDllInfo);
}

void Engine_InstallHooks(void)
{
	Install_InlineHook(S_StartDynamicSound);
	Install_InlineHook(S_StartStaticSound);
	Install_InlineHook(pfnTextMessageGet);
	Install_InlineHook(pfnServerCmdUnreliable);
	Install_InlineHook(TextMessageParse);
	Install_InlineHook(COM_ExplainDisconnection);
}

void Engine_UninstallHooks(void)
{
	Uninstall_Hook(S_StartDynamicSound);
	Uninstall_Hook(S_StartStaticSound);
	Uninstall_Hook(pfnTextMessageGet);
	Uninstall_Hook(pfnServerCmdUnreliable);
	Uninstall_Hook(TextMessageParse);
	Uninstall_Hook(COM_ExplainDisconnection);
}

void Client_InstallHooks(void)
{
	if (gPrivateFuncs.SCClient_SoundEngine_PlayFMODSound)
	{
		Install_InlineHook(SCClient_SoundEngine_PlayFMODSound);
	}

	if (gPrivateFuncs.SCClient_SoundEngine_LoadSoundList)
	{
		Install_InlineHook(SCClient_SoundEngine_LoadSoundList);
	}

	if (gPrivateFuncs.WeaponsResource_SelectSlot)
	{
		Install_InlineHook(WeaponsResource_SelectSlot);
	}
}

void Client_UninstallHooks(void)
{
	Uninstall_Hook(SCClient_SoundEngine_PlayFMODSound);
	Uninstall_Hook(SCClient_SoundEngine_LoadSoundList);
	Uninstall_Hook(WeaponsResource_SelectSlot);
}

void DllLoadNotification(mh_load_dll_notification_context_t* ctx)
{
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)
	{
		if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_CLIENT)
		{
			
		}
		else if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"fmodex.dll"))
		{
			g_hFMODEx = ctx->hModule;
			FMOD_InstallHooks(ctx->hModule);
		}
	}
	else if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)
	{
		if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_CLIENT)
		{
			
		}
		else if (ctx->hModule == g_hFMODEx)
		{
			FMOD_UninstallHooks(ctx->hModule);
			g_hFMODEx = NULL;
		}
	}
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