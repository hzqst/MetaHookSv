#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "privatehook.h"

private_funcs_t gPrivateFuncs = { 0 };

static hook_t* g_phook_CClient_SoundEngine_FlushCache = NULL;

extern int g_iRetryState;

void __fastcall CClient_SoundEngine_FlushCache(int pthis, int dummy, qboolean including_local)
{
	if (g_iRetryState == 2)
		return;

	return gPrivateFuncs.CClient_SoundEngine_FlushCache(pthis, dummy, including_local);
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
#define CCLIENT_SOUNDENGINE_FLUSHCACHE_SIG "\x81\xEC\x58\x03\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x54\x03\x00\x00"

	PVOID CClient_SoundEngine_FlushCache_VA = Search_Pattern(CCLIENT_SOUNDENGINE_FLUSHCACHE_SIG, DllInfo);;

	gPrivateFuncs.CClient_SoundEngine_FlushCache = (decltype(gPrivateFuncs.CClient_SoundEngine_FlushCache))ConvertDllInfoSpace(CClient_SoundEngine_FlushCache_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(CClient_SoundEngine_FlushCache);
}

void Client_InstallHooks()
{
	Install_InlineHook(CClient_SoundEngine_FlushCache);
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