#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"

vec3_t* v_origin = NULL;
vec3_t* g_vVecViewangles = NULL;

int* g_iUser1 = NULL;
int* g_iUser2 = NULL;

struct event_api_s** g_pClientDLLEventAPI = NULL;

private_funcs_t gPrivateFuncs = { 0 };

static hook_t* g_phook_V_CalcNormalRefdef = NULL;

void Client_FillAddress_CL_IsThirdPerson(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//The old locator disassembled the client's exported CL_IsThirdPerson and
	//picked the last two adjacent .data references out of it; the catalog
	//publishes those two globals directly for both SvEngine clients.
	g_iUser1 = (decltype(g_iUser1))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "g_iUser1", MH_GAMESYMBOL_KIND_GLOBAL);
	g_iUser2 = (decltype(g_iUser2))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "g_iUser2", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		//The old locator disassembled the client's `mov eax,[gEngfuncs]; mov
		//eax,[eax+EventAPI]; jmp eax` sequence and took the loaded .data operand
		//as the EventAPI slot; the catalog publishes the `gEngfuncs` global, so
		//the slot is its `pEventAPI` member.
		auto pEngfuncs = (cl_enginefunc_t*)GamedataResolvePtr(RealDllInfo.ImageBase, "client", "gEngfuncs", MH_GAMESYMBOL_KIND_GLOBAL);
		g_pClientDLLEventAPI = &pEngfuncs->pEventAPI;
	}
	else
	{
		Sys_Error("This plugin is for Sven Co-op!");
		return;
	}

	v_origin = (decltype(v_origin))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "v_origin", MH_GAMESYMBOL_KIND_GLOBAL);

	gPrivateFuncs.V_CalcNormalRefdef = (decltype(gPrivateFuncs.V_CalcNormalRefdef))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "V_CalcNormalRefdef", MH_GAMESYMBOL_KIND_FUNCTION);
	g_vVecViewangles = (decltype(g_vVecViewangles))GamedataResolvePtr(RealDllInfo.ImageBase, "client", "g_vVecViewangles", MH_GAMESYMBOL_KIND_GLOBAL);

	Client_FillAddress_CL_IsThirdPerson(DllInfo, RealDllInfo);
}

void Client_InstallHooks(void)
{
	Install_InlineHook(V_CalcNormalRefdef);
}

void Client_UninstallHooks(void)
{
	Uninstall_Hook(V_CalcNormalRefdef);
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
