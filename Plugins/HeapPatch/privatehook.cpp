#include <metahook.h>
#include <capstone.h>
#include <vector>
#include <set>
#include "plugins.h"
#include "privatehook.h"

#define SYS_INITMEMORY_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x34\x05\x00\x00\xA1"
#define SYS_INITMEMORY_SIG_8308 "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08"
#define SYS_INITMEMORY_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define SYS_INITMEMORY_SIG_BLOB "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"

private_funcs_t gPrivateFuncs = { 0 };

void Engine_InstallHooks()
{
	if (1)
	{
		/*
.text:01D98912 68 F0 52 ED 01                                      push    offset aSLoadsoundCoul ; "S_LoadSound: Couldn't load %s\n"
.text:01D98917 E8 24 70 F9 FF                                      call    sub_1D2F940
.text:01D9891C 83 C4 08                                            add     esp, 8
		*/
		const char sigs[] = "Available memory less than";
		auto Sys_InitMemory_String = Search_Pattern_Data(sigs);
		if (!Sys_InitMemory_String)
			Sys_InitMemory_String = Search_Pattern_Rdata(sigs);
		if (Sys_InitMemory_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 1) = (DWORD)Sys_InitMemory_String;
			auto Sys_InitMemory_PushString = (PUCHAR)Search_Pattern(pattern);
			if (Sys_InitMemory_PushString)
			{
				//Inlined in HL25
				gPrivateFuncs.Sys_InitMemory = (decltype(gPrivateFuncs.Sys_InitMemory))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Sys_InitMemory_PushString, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
					{
						return TRUE;
					}

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
					{
						return TRUE;
					}

					return FALSE;
				});
			}
		}
	}

	Sig_FuncNotFound(Sys_InitMemory);

	if (1)
	{
		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;

			std::set<PVOID> patches;
		}Sys_InitMemory_SearchContext;

		Sys_InitMemory_SearchContext ctx = { 0 };

		ctx.base = gPrivateFuncs.Sys_InitMemory;

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

				if (g_iEngineType == ENGINE_SVENGINE)
				{
					if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_CMP) &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 0x20000000//512MB
						)
					{
						auto patch_addr = (PVOID)(address + pinst->detail->x86.encoding.imm_offset);

						ctx->patches.emplace(patch_addr);
					}
				}
				else
				{
					if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_CMP) &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						((pinst->detail->x86.operands[1].imm == 0x2000000 && g_dwEngineBuildnum < 6153)//32MB
							||
							(pinst->detail->x86.operands[1].imm == 0x2800000)//40MB
							|| (pinst->detail->x86.operands[1].imm == 0x8000000 && g_dwEngineBuildnum >= 6153))//128MB
						)
					{
						auto patch_addr = (PVOID)(address + pinst->detail->x86.encoding.imm_offset);

						ctx->patches.emplace(patch_addr);
					}
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

		for (auto patch : ctx.patches)
		{
			g_pMetaHookAPI->WriteDWORD(patch, HeapLimitOverrideInBytes);
		}
	}
}

void Engine_UninstallHooks()
{

}