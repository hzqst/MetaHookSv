#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "privatehook.h"

private_funcs_t gPrivateFuncs = {0};

void Engine_FillAddreess()
{
	if (1)
	{
		const char sigs1[] = "#GameUI_PrecachingResources";
		auto CL_PrecacheResources_String = Search_Pattern_Data(sigs1);
		if (!CL_PrecacheResources_String)
			CL_PrecacheResources_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(CL_PrecacheResources_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)CL_PrecacheResources_String;

		auto CL_PrecacheResources_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(CL_PrecacheResources_PushString);

		g_pMetaHookAPI->DisasmRanges(CL_PrecacheResources_PushString, 0x250, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(ULONG_PTR)pinst->detail->x86.operands[1].imm >= (ULONG_PTR)g_dwEngineDataBase &&
					(ULONG_PTR)pinst->detail->x86.operands[1].imm <= (ULONG_PTR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//A1 84 5C 32 02 mov     eax, mod_numknown
					ULONG_PTR imm = (ULONG_PTR)pinst->detail->x86.operands[1].imm;

					cl_resourcesonhand = (decltype(cl_resourcesonhand))imm;
				}

				if (cl_resourcesonhand)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
		}, 0, NULL);
	}
}