#include <metahook.h>
#include <capstone.h>
#include <vector>
#include <set>
#include "plugins.h"
#include "privatehook.h"

static_assert(METAHOOK_API_VERSION >= 109, "HeapPatch resolves engine private symbols from gamedata and requires MetaHook API 109 (ResolveGameSymbol)");

private_funcs_t gPrivateFuncs = { 0 };

static std::set<PVOID> g_Sys_InitMemory_Patches;

// Heap-limit immediates observed in Sys_InitMemory (GoldSrc_VibeSignatures bin_artifacts):
// SvEngine: 512MB only.
// GoldSrc blob (3248-4554): 32MB + 40MB.
// GoldSrc 6153+ / HL25 / Cry of Fear: 40MB + 128MB.
// cof-5936 (build 5936) ships 128MB despite buildnum < 6153, so 128MB is not gated on buildnum.
enum : long long
{
	kHeapLimitImm32MB = 0x2000000,
	kHeapLimitImm40MB = 0x2800000,
	kHeapLimitImm128MB = 0x8000000,
	kHeapLimitImm512MB = 0x20000000,
};

static bool IsHeapLimitImmediate(long long imm)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return imm == kHeapLimitImm512MB;

	return imm == kHeapLimitImm32MB
		|| imm == kHeapLimitImm40MB
		|| imm == kHeapLimitImm128MB;
}

// On gamedata resolution failure, print diagnostics (symbol / buildnum / CRC64 / status string) and abort via Sys_Error.
// The return value is the real-image VA, written directly into the corresponding gPrivateFuncs field.
static PVOID ResolveGameSymbolOrError(const char* symbolName)
{
	PVOID va = NULL;
	auto st = g_pMetaHookAPI->ResolveGameSymbol(g_EngineDLLInfo.ImageBase, symbolName, MH_GAMESYMBOL_KIND_FUNCTION, &va);

	if (st == MH_GAMESYMBOL_OK)
		return va;

	uint64_t crc64 = 0;
	auto crcSt = g_pMetaHookAPI->GetModuleCRC64(g_EngineDLLInfo.ImageBase, &crc64);

	if (crcSt == MH_GAMESYMBOL_OK)
	{
		Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nCRC64: %016llx\nReason: %s",
			symbolName, g_dwEngineBuildnum, (unsigned long long)crc64, g_pMetaHookAPI->GetGameSymbolStatusString(st));
	}
	else
	{
		Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nReason: %s",
			symbolName, g_dwEngineBuildnum, g_pMetaHookAPI->GetGameSymbolStatusString(st));
	}

	return NULL;
}

void Engine_FillAddress_Sys_InitMemory()
{
	gPrivateFuncs.Sys_InitMemory = (decltype(gPrivateFuncs.Sys_InitMemory))ResolveGameSymbolOrError("Sys_InitMemory");
}

void Engine_FillAddress_Sys_InitMemory_Patches(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	typedef struct Sys_InitMemory_SearchContext_
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		std::set<PVOID>& patches;

		PVOID base{};
		size_t max_insts{};
		int max_depth{};
		std::set<PVOID> code;
		std::set<PVOID> branches;
		std::vector<walk_context_t> walks;
	}Sys_InitMemory_SearchContext;

	Sys_InitMemory_SearchContext ctx = { DllInfo, RealDllInfo, g_Sys_InitMemory_Patches };

	// The walk runs in the search space (the mirror copy when a mirror exists), so the entry point must be mapped from the real image.
	ctx.base = ConvertDllInfoSpace(gPrivateFuncs.Sys_InitMemory, RealDllInfo, DllInfo);

	ctx.max_insts = 1000;
	ctx.max_depth = 16;
	ctx.walks.emplace_back(ctx.base, 0x1000, 0);

	while (ctx.walks.size())
	{
		auto walk = ctx.walks[ctx.walks.size() - 1];
		ctx.walks.pop_back();

		g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (Sys_InitMemory_SearchContext*)context;

			if (ctx->code.size() > ctx->max_insts)
				return TRUE;

			if (ctx->code.find(address) != ctx->code.end())
				return TRUE;

			ctx->code.emplace(address);

			if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_CMP) &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				IsHeapLimitImmediate(pinst->detail->x86.operands[1].imm))
			{
				auto patch_addr_VA = (PVOID)(address + pinst->detail->x86.encoding.imm_offset);

				auto patch_addr = ConvertDllInfoSpace(patch_addr_VA, ctx->DllInfo, ctx->RealDllInfo);

				ctx->patches.emplace(patch_addr);
			}

			if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
				auto foundbranch = ctx->branches.find(imm);
				if (foundbranch == ctx->branches.end())
				{
					ctx->branches.emplace(imm);
					if (depth + 1 < ctx->max_depth)
						ctx->walks.emplace_back(imm, 0x1000, depth + 1);
				}

				if (pinst->id == X86_INS_JMP)
					return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, walk.depth, &ctx);
	}

	if (ctx.patches.size() == 0)
	{
		Sys_Error("Sys_InitMemory imm not found");
		return;
	}
}

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Engine_FillAddress_Sys_InitMemory();
	Engine_FillAddress_Sys_InitMemory_Patches(DllInfo, RealDllInfo);
}

void Engine_InstallHooks()
{
	auto HeapLimitOverride = (g_iEngineType == ENGINE_SVENGINE) ? 256 : 256;
	DWORD HeapLimitOverrideInBytes = (DWORD)HeapLimitOverride * 1024 * 1024;

	const char* pszHeapLimitOverride = NULL;
	if (gEngfuncs.CheckParm("-heaplimit_override", &pszHeapLimitOverride) &&
		pszHeapLimitOverride &&
		pszHeapLimitOverride[0])
	{
		HeapLimitOverride = atoi(pszHeapLimitOverride);
		HeapLimitOverride = max(min(HeapLimitOverride, 1024), 32);

		HeapLimitOverrideInBytes = (DWORD)HeapLimitOverride * 1024 * 1024;
	}
	for (auto patch : g_Sys_InitMemory_Patches)
	{
		g_pMetaHookAPI->WriteDWORD(patch, HeapLimitOverrideInBytes);
	}
}

void Engine_UninstallHooks()
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