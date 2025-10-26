#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "privatehook.h"

private_funcs_t gPrivateFuncs = {0};

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		const char sigs1[] = "#GameUI_PrecachingResources";
		auto CL_PrecacheResources_String = Search_Pattern_Data(sigs1, DllInfo);
		if (!CL_PrecacheResources_String)
			CL_PrecacheResources_String = Search_Pattern_Rdata(sigs1, DllInfo);
		Sig_VarNotFound(CL_PrecacheResources_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)CL_PrecacheResources_String;

		auto CL_PrecacheResources_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(CL_PrecacheResources_PushString);

		typedef struct CL_PrecacheResources_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}CL_PrecacheResources_SearchContext;

		CL_PrecacheResources_SearchContext SearchContext = { DllInfo, RealDllInfo  };

		g_pMetaHookAPI->DisasmRanges(CL_PrecacheResources_PushString, 0x250, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (CL_PrecacheResources_SearchContext*)context;

				if (!cl_resourcesonhand &&
					pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(ULONG_PTR)pinst->detail->x86.operands[1].imm >= (ULONG_PTR)ctx->DllInfo.DataBase &&
					(ULONG_PTR)pinst->detail->x86.operands[1].imm <= (ULONG_PTR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					cl_resourcesonhand = (decltype(cl_resourcesonhand))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (cl_resourcesonhand)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
		}, 0, & SearchContext);
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