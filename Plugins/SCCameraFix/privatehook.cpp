#include <metahook.h>
#include <capstone.h>
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
	PVOID CL_IsThirdPerson = ConvertDllInfoSpace((void*)g_pMetaSave->pExportFuncs->CL_IsThirdPerson, RealDllInfo, DllInfo);

	if (!CL_IsThirdPerson)
	{
		if (g_pMetaHookAPI->GetClientModule())
		{
			CL_IsThirdPerson = ConvertDllInfoSpace(GetProcAddress(g_pMetaHookAPI->GetClientModule(), "CL_IsThirdPerson"), RealDllInfo, DllInfo);
		}
	}

	if (CL_IsThirdPerson)
	{
		typedef struct CL_IsThirdPerson_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			ULONG_PTR CandidateVA[16]{};
			int iNumCandidates{};
		}CL_IsThirdPerson_SearchContext;

		CL_IsThirdPerson_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(CL_IsThirdPerson, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (CL_IsThirdPerson_SearchContext*)context;
			auto pinst = (cs_insn*)inst;

			if (ctx->iNumCandidates < 16)
			{
				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					(
						pinst->detail->x86.operands[0].reg == X86_REG_EAX ||
						pinst->detail->x86.operands[0].reg == X86_REG_EBX ||
						pinst->detail->x86.operands[0].reg == X86_REG_ECX ||
						pinst->detail->x86.operands[0].reg == X86_REG_EDX ||
						pinst->detail->x86.operands[0].reg == X86_REG_ESI ||
						pinst->detail->x86.operands[0].reg == X86_REG_EDI
						) &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidateVA[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->iNumCandidates++;
				}
			}

			if (ctx->iNumCandidates < 16)
			{
				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidateVA[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->iNumCandidates++;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		if (ctx.iNumCandidates >= 3 && ctx.CandidateVA[ctx.iNumCandidates - 1] == ctx.CandidateVA[ctx.iNumCandidates - 2] + sizeof(int))
		{
			g_iUser1 = (decltype(g_iUser1))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[ctx.iNumCandidates - 2], DllInfo, RealDllInfo);
			g_iUser2 = (decltype(g_iUser2))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[ctx.iNumCandidates - 1], DllInfo, RealDllInfo);
		}
	}
}

void Client_FillAddress_FogParams(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00";

	PVOID addr = Search_Pattern(pattern, DllInfo);

	Sig_AddrNotFound(g_iFogColor);

	typedef struct V_CalcNormalRefdef_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		ULONG_PTR Candidates[16]{};
		int iNumCandidates{};
	}V_CalcNormalRefdef_SearchContext;

	V_CalcNormalRefdef_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(addr, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto ctx = (V_CalcNormalRefdef_SearchContext*)context;
		auto pinst = (cs_insn*)inst;

		if (ctx->iNumCandidates < 16)
		{
			if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_XMM0 &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.ImageBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize)
			{
				ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				ctx->iNumCandidates++;
			}
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	if (ctx.iNumCandidates >= 5 &&
		ctx.Candidates[ctx.iNumCandidates - 1] == ctx.Candidates[ctx.iNumCandidates - 2] + sizeof(int) &&
		ctx.Candidates[ctx.iNumCandidates - 2] == ctx.Candidates[ctx.iNumCandidates - 3] + sizeof(int) &&
		ctx.Candidates[ctx.iNumCandidates - 3] == ctx.Candidates[ctx.iNumCandidates - 4] + sizeof(int))
	{
		g_iFogColor_SCClient = (decltype(g_iFogColor_SCClient))ConvertDllInfoSpace((PVOID)ctx.Candidates[0], DllInfo, RealDllInfo);
		g_iStartDist_SCClient = (decltype(g_iStartDist_SCClient))ConvertDllInfoSpace((PVOID)ctx.Candidates[3], DllInfo, RealDllInfo);
		g_iEndDist_SCClient = (decltype(g_iEndDist_SCClient))ConvertDllInfoSpace((PVOID)ctx.Candidates[4], DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(g_iFogColor_SCClient);
	Sig_VarNotFound(g_iStartDist_SCClient);
	Sig_VarNotFound(g_iEndDist_SCClient);
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
		Sig_AddrNotFound(g_iFogColor);

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