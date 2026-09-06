#include <metahook.h>
#include <capstone.h>
#include <vector>
#include <set>

#include "plugins.h"
#include "privatehook.h"
#include "util.h"

#include "ResourceReplacer.h"

static_assert(METAHOOK_API_VERSION >= 109, "ResourceReplacer resolves engine private symbols from gamedata and requires MetaHook API 109 (ResolveGameSymbol)");

private_funcs_t gPrivateFuncs = {  };

std::set<PVOID> S_LoadSound_call_FS_Open;
std::set<PVOID> Mod_LoadModel_call_FS_Open;

static hook_t* g_phook_CL_PrecacheResources = NULL;

// gamedata 解析失败时打印诊断（符号名 / buildnum / CRC64 / 状态串）并 Sys_Error 终止。
// 返回值即真实镜像 VA，直接写入 gPrivateFuncs 对应字段。
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

typedef struct FS_Open_SearchContext_s
{
	const mh_dll_info_t& DllInfo;
	const mh_dll_info_t& RealDllInfo;

	PVOID fsOpenRealVA{};
	size_t callWindowBytes{};
	std::set<PVOID>& outCallSites;

	size_t max_insts{};
	int max_depth{};
	std::set<PVOID> code;
	std::set<PVOID> branches;
	std::vector<walk_context_t> walks;

	bool mismatchWarned{};

	PVOID address_rb{};
	int instCount_rb{};

	FS_Open_SearchContext_s(const mh_dll_info_t& dllInfo, const mh_dll_info_t& realDllInfo, std::set<PVOID>& out)
		: DllInfo(dllInfo), RealDllInfo(realDllInfo), outCallSites(out)
	{
	}

}FS_Open_SearchContext;

// 从 rootVA（搜索空间地址）有界走查 push "rb"; call FS_Open 指令序列，收集 call-site（转回真实镜像）。
// gamedata 符号模型表达不了函数内部的 call 指令地址，此走查是 call-site 重定向的功能本体。
static void FindFSOpenCallSites(PVOID rootVA, const mh_dll_info_t& SearchDllInfo,
                                const mh_dll_info_t& RealDllInfo,
                                PVOID fsOpenRealVA, size_t callWindowBytes,
                                std::set<PVOID>& outCallSites)
{
	FS_Open_SearchContext ctx(SearchDllInfo, RealDllInfo, outCallSites);

	ctx.fsOpenRealVA = fsOpenRealVA;
	ctx.callWindowBytes = callWindowBytes;

	ctx.max_insts = 1000;
	ctx.max_depth = 16;
	ctx.walks.emplace_back(rootVA, 0x1000, 0);

	while (ctx.walks.size())
	{
		auto walk = ctx.walks[ctx.walks.size() - 1];
		ctx.walks.pop_back();

		g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (FS_Open_SearchContext*)context;

			if (ctx->code.size() > ctx->max_insts)
				return TRUE;

			if (ctx->code.find(address) != ctx->code.end())
				return TRUE;

			ctx->code.emplace(address);

			if (!ctx->address_rb &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(
					((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize) ||
					((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize)
					))
			{
				auto pString = (PCHAR)pinst->detail->x86.operands[0].imm;
				if (!memcmp(pString, "rb", sizeof("rb") - 1))
				{
					ctx->instCount_rb = instCount;
					ctx->address_rb = address;
				}
			}

			if (address[0] == 0xE8 && instLen == 5 &&
				ctx->address_rb && address > ctx->address_rb && address <= (PUCHAR)ctx->address_rb + ctx->callWindowBytes &&
				instCount > ctx->instCount_rb && instCount <= ctx->instCount_rb + 5)
			{
				// FS_Open 以 gamedata 为权威，disasm 恢复的 call 目标仅作交叉校验。
				auto callTargetRealVA = ConvertDllInfoSpace(GetCallAddress(address), ctx->DllInfo, ctx->RealDllInfo);

				if (!ctx->mismatchWarned && callTargetRealVA != ctx->fsOpenRealVA)
				{
					ctx->mismatchWarned = true;
					gEngfuncs.Con_DPrintf("[ResourceReplacer] Warning: disasm-recovered FS_Open call target 0x%p does not match gamedata FS_Open 0x%p\n",
						callTargetRealVA, ctx->fsOpenRealVA);
				}

				auto realAddress = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

				ctx->outCallSites.emplace(realAddress);

				return TRUE;
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
						ctx->walks.emplace_back(imm, 0x300, depth + 1);
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
}

qboolean CL_PrecacheResources()
{
	if (1)
	{
		std::string name = gEngfuncs.pfnGetLevelName();

		RemoveFileExtension(name);

		name += ".gmr";

		ModelReplacer()->LoadMapReplaceList(name.c_str());
	}

	if (1)
	{
		std::string name = gEngfuncs.pfnGetLevelName();

		RemoveFileExtension(name);

		name += ".gsr";

		SoundReplacer()->LoadMapReplaceList(name.c_str());
	}

	return gPrivateFuncs.CL_PrecacheResources();
}

FileHandle_t Mod_LoadModel_FS_Open(const char* pFileName, const char* pOptions)
{
	if (!strcmp(pOptions, "rb"))
	{
		std::string ReplacedFileName;
		if (ModelReplacer()->ReplaceFileName(pFileName, ReplacedFileName))
		{
			return gPrivateFuncs.FS_Open(ReplacedFileName.c_str(), pOptions);
		}
	}
	return gPrivateFuncs.FS_Open(pFileName, pOptions);
}

FileHandle_t S_LoadSound_FS_Open(const char* pFileName, const char* pOptions)
{
	if (!strcmp(pOptions, "rb"))
	{
		std::string ReplacedFileName;
		if (SoundReplacer()->ReplaceFileName(pFileName, ReplacedFileName))
		{
			return gPrivateFuncs.FS_Open(ReplacedFileName.c_str(), pOptions);
		}
	}

	return gPrivateFuncs.FS_Open(pFileName, pOptions);
}

void Engine_FillAddress_S_LoadSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto S_LoadSound_VA = ResolveGameSymbolOrError("S_LoadSound");

	gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))S_LoadSound_VA;

	// 走查在搜索空间（mirror 存在时为 mirror 副本）进行，入口需要从真实镜像映射过去。
	FindFSOpenCallSites(ConvertDllInfoSpace(S_LoadSound_VA, RealDllInfo, DllInfo), DllInfo, RealDllInfo, gPrivateFuncs.FS_Open, 0x30, S_LoadSound_call_FS_Open);

	if (S_LoadSound_call_FS_Open.empty())
	{
		Sys_Error("S_LoadSound.FS_Open not found");
		return;
	}
}

void Engine_FillAddress_Mod_LoadModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto Mod_LoadModel_VA = ResolveGameSymbolOrError("Mod_LoadModel");

	gPrivateFuncs.Mod_LoadModel = (decltype(gPrivateFuncs.Mod_LoadModel))Mod_LoadModel_VA;

	// 走查在搜索空间（mirror 存在时为 mirror 副本）进行，入口需要从真实镜像映射过去。
	FindFSOpenCallSites(ConvertDllInfoSpace(Mod_LoadModel_VA, RealDllInfo, DllInfo), DllInfo, RealDllInfo, gPrivateFuncs.FS_Open, 0x50, Mod_LoadModel_call_FS_Open);

	if (Mod_LoadModel_call_FS_Open.empty())
	{
		Sys_Error("Mod_LoadModel.FS_Open not found");
		return;
	}
}

void Engine_FillAddress_CL_PrecacheResources(void)
{
	gPrivateFuncs.CL_PrecacheResources = (decltype(gPrivateFuncs.CL_PrecacheResources))ResolveGameSymbolOrError("CL_PrecacheResources");
}

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.FS_Open = (decltype(gPrivateFuncs.FS_Open))ResolveGameSymbolOrError("FS_Open");

	Engine_FillAddress_S_LoadSound(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_LoadModel(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_PrecacheResources();
}

void Engine_InstallHooks()
{
	{
		for (auto addr : S_LoadSound_call_FS_Open)
		{
			g_pMetaHookAPI->InlinePatchRedirectBranch(addr, S_LoadSound_FS_Open, NULL);
		}
	}

	{
		for (auto addr : Mod_LoadModel_call_FS_Open)
		{
			g_pMetaHookAPI->InlinePatchRedirectBranch(addr, Mod_LoadModel_FS_Open, NULL);
		}
	}

	Install_InlineHook(CL_PrecacheResources);
}

void Engine_UninstallHooks()
{
	Uninstall_Hook(CL_PrecacheResources);
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
