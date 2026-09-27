#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"

vec3_t* v_origin = NULL;
vec3_t* g_vVecViewangles = NULL;

int* g_iUser1 = NULL;
int* g_iUser2 = NULL;

float* g_iFogColor_SCClient = NULL;
float* g_iStartDist_SCClient = NULL;
float* g_iEndDist_SCClient = NULL;

int* g_iWaterLevel = NULL;
int* g_iIsSpectator = NULL;
bool* g_bRenderingPortals_SCClient = NULL;

//pitchdrift_t* g_pitchdrift = NULL;

struct event_api_s** g_pClientDLLEventAPI = NULL;

private_funcs_t gPrivateFuncs = { 0 };

static hook_t* g_phook_V_CalcNormalRefdef = NULL;

void Client_FillAddress_CL_IsThirdPerson(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//The old locator disassembled the client's exported CL_IsThirdPerson and
	//picked the last two adjacent .data references out of it; the catalog
	//publishes those two globals directly for both SvEngine clients.
	g_iUser1 = (decltype(g_iUser1))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iUser1", MH_GAMESYMBOL_KIND_GLOBAL);
	g_iUser2 = (decltype(g_iUser2))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iUser2", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Client_FillAddress_FogParams(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//The old locator walked the `push 0x2601 / push 0xB65` texture-state block
	//for five adjacent `movss xmm0, [imm]` reads and took the first, fourth and
	//fifth (.data dword 0/3/4); the catalog publishes the same three globals.
	g_iFogColor_SCClient = (decltype(g_iFogColor_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iFogColor", MH_GAMESYMBOL_KIND_GLOBAL);
	g_iStartDist_SCClient = (decltype(g_iStartDist_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iStartDist", MH_GAMESYMBOL_KIND_GLOBAL);
	g_iEndDist_SCClient = (decltype(g_iEndDist_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iEndDist", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		if (1)
		{
			char pattern[] = "\xA1\x2A\x2A\x2A\x2A\x8B\x40\x0C\xFF\xE0";
			auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
			Sig_AddrNotFound(ClientDLLEventAPI);

			PVOID ClientDLLEventAPI_VA = *(decltype(ClientDLLEventAPI_VA)*)(addr + 1);

			g_pClientDLLEventAPI = (decltype(g_pClientDLLEventAPI))ConvertDllInfoSpace(ClientDLLEventAPI_VA, DllInfo, RealDllInfo);
		}
	}
	else
	{
		Sys_Error("This plugin is for Sven Co-op!");
		return;
	}

	{
		const char pattern[] = "\x66\x0F\xD6\x05\x2A\x2A\x2A\x2A\x2A\x2A\x08\xA3\x2A\x2A\x2A\x2A\xF3\x0F\x2A\x2A\x0C";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(v_origin);

		auto v_origin_VA = *(PVOID*)(addr + 4);

		v_origin = (decltype(v_origin))ConvertDllInfoSpace(v_origin_VA, DllInfo, RealDllInfo);
	}

	{
		const char pattern[] = "\xA3\x2A\x2A\x2A\x2A\x83\x2A\xE0\x00\x00\x00\x00\x0F\x85\x2A\x2A\x2A\x2A\x80\x3D\x2A\x2A\x2A\x2A\x00";
		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(g_bRenderingPortals);

		auto g_iWaterLevel_VA = *(PVOID*)(addr + 1);
		g_iWaterLevel = (decltype(g_iWaterLevel))ConvertDllInfoSpace(g_iWaterLevel_VA, DllInfo, RealDllInfo);

		auto g_bRenderingPortals_SCClient_VA = *(PVOID*)(addr + 20);
		g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))ConvertDllInfoSpace(g_bRenderingPortals_SCClient_VA, DllInfo, RealDllInfo);
	}

	if (1)
	{
		const char pattern[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x85\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x85\x2A\x2A\x2A\x2A\xE8";
		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(g_iIsSpectator);

		PVOID g_iIsSpectator_VA = *(PVOID*)(addr + 2);

		g_iIsSpectator = (decltype(g_iIsSpectator))ConvertDllInfoSpace(g_iIsSpectator_VA, DllInfo, RealDllInfo);
	}

	if (1)
	{
		const char pattern[] = "\x2A\x2A\x48\x00\x75\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(V_CalcNormalRefdef);

		PVOID V_CalcNormalRefdef_VA = GetCallAddress(addr + 7);

		gPrivateFuncs.V_CalcNormalRefdef = (decltype(gPrivateFuncs.V_CalcNormalRefdef))ConvertDllInfoSpace(V_CalcNormalRefdef_VA, DllInfo, RealDllInfo);
	}

	if (1)
	{
		const char pattern[] = "\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00";
		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(g_vVecViewangles);

		const char pattern2[] = "\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xF3\x0F\x2A\x2A\x10\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xF3\x0F\x2A\x2A\x14\xF3\x0F\x11\x05";
		auto addr2 = (PUCHAR)Search_Pattern_From_Size(addr - 0x100, 0x100, pattern2);
		if (!addr2)
		{
			Sig_NotFound(g_vVecViewangles);
			return;
		}

		PVOID g_vVecViewangles_VA = *(PVOID*)(addr2 + 4);

		g_vVecViewangles = (decltype(g_vVecViewangles))ConvertDllInfoSpace(g_vVecViewangles_VA, DllInfo, RealDllInfo);
	}

	Client_FillAddress_FogParams(DllInfo, RealDllInfo);

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