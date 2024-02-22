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

pitchdrift_t* g_pitchdrift = NULL;

struct event_api_s** g_pClientDLLEventAPI = NULL;

private_funcs_t gPrivateFuncs = { 0 };

void Client_FillAddress(void)
{
	g_dwClientBase = g_pMetaHookAPI->GetClientBase();
	g_dwClientSize = g_pMetaHookAPI->GetClientSize();

	g_dwClientTextBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".text\0\0\0", &g_dwClientTextSize);

	if (!g_dwClientTextBase)
	{
		Sys_Error("Failed to locate section \".text\" in client.dll!");
		return;
	}

	g_dwClientDataBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".data\0\0\0", &g_dwClientDataSize);

	if (!g_dwClientDataBase)
	{
		Sys_Error("Failed to locate section \".text\" in client.dll!");
		return;
	}

	g_dwClientRdataBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".rdata\0\0", &g_dwClientRdataSize);

	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		if (1)
		{
			char pattern[] = "\xA1\x2A\x2A\x2A\x2A\x8B\x40\x3C\xFF\xD0\xA1\x2A\x2A\x2A\x2A\x6A\x01\x6A\x01";
			auto addr = (PUCHAR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
			Sig_VarNotFound("CAM_Think_Pattern");

			g_pClientDLLEventAPI = *(decltype(g_pClientDLLEventAPI)*)(addr + 1);
		}
	}
	else
	{
		Sys_Error("This plugin is for Sven Co-op!");
		return;
	}

	if (1)
	{
		const char pattern[] = "\x66\x0F\xD6\x05\x2A\x2A\x2A\x2A\x2A\x2A\x08\xA3\x2A\x2A\x2A\x2A\xF3\x0F\x2A\x2A\x0C";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
		Sig_AddrNotFound(v_origin);
		v_origin = (decltype(v_origin)) * (ULONG_PTR*)(addr + 4);
	}

	if (1)
	{
		const char pattern[] = "\xA3\x2A\x2A\x2A\x2A\x83\x2A\xE0\x00\x00\x00\x00\x0F\x85\x2A\x2A\x2A\x2A\x80\x3D\x2A\x2A\x2A\x2A\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
		Sig_AddrNotFound(g_bRenderingPortals);
		g_iWaterLevel = (decltype(g_iWaterLevel)) * (ULONG_PTR*)(addr + 1);
		g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient)) * (ULONG_PTR*)(addr + 20);
	}

	if (1)
	{
		const char pattern[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x85\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x85\x2A\x2A\x2A\x2A\xE8";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
		Sig_AddrNotFound(g_iIsSpectator);
		g_iIsSpectator = (decltype(g_iIsSpectator)) * (ULONG_PTR*)(addr + 2);
	}

	if (1)
	{
		const char pattern[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x89\x2A\x2A\x2A\xFF\x15";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
		Sig_AddrNotFound(g_pitchdrift);
		g_pitchdrift = (decltype(g_pitchdrift)) * (ULONG_PTR*)(addr + 2);
	}

	if (1)
	{
		const char pattern[] = "\x2A\x2A\x48\x00\x75\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
		Sig_AddrNotFound(V_CalcNormalRefdef);
		gPrivateFuncs.V_CalcNormalRefdef = (decltype(gPrivateFuncs.V_CalcNormalRefdef))GetCallAddress(addr + 7);
	}

	if (1)
	{
		const char pattern[] = "\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
		Sig_AddrNotFound(g_iFogColor);

		const char pattern2[] = "\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xF3\x0F\x2A\x2A\x10\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xF3\x0F\x2A\x2A\x14\xF3\x0F\x11\x05";
		ULONG_PTR addr2 = (ULONG_PTR)Search_Pattern_From_Size(addr - 0x100, 0x100, pattern2);
		if (!addr2)
		{
			Sig_NotFound(g_vVecViewangles);
			return;
		}

		g_vVecViewangles = (decltype(g_vVecViewangles)) * (ULONG_PTR*)(addr2 + 4);

		typedef struct
		{
			ULONG_PTR Candidates[16];
			int iNumCandidates;
		}V_CalcNormalRefdef_ctx;

		V_CalcNormalRefdef_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges((void*)addr, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (V_CalcNormalRefdef_ctx*)context;
			auto pinst = (cs_insn*)inst;

			if (ctx->iNumCandidates < 16)
			{
				if (pinst->id == X86_INS_MOVSS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_XMM0 &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwClientBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwClientBase + g_dwClientSize)
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
			g_iFogColor_SCClient = (decltype(g_iFogColor_SCClient))ctx.Candidates[0];
			g_iStartDist_SCClient = (decltype(g_iStartDist_SCClient))ctx.Candidates[3];
			g_iEndDist_SCClient = (decltype(g_iEndDist_SCClient))ctx.Candidates[4];
		}
	}
	Sig_VarNotFound(g_iFogColor_SCClient);
	Sig_VarNotFound(g_iStartDist_SCClient);
	Sig_VarNotFound(g_iEndDist_SCClient);

	if ((void*)g_pMetaSave->pExportFuncs->CL_IsThirdPerson > g_dwClientTextBase && (void*)g_pMetaSave->pExportFuncs->CL_IsThirdPerson < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		typedef struct
		{
			ULONG_PTR Candidates[16];
			int iNumCandidates;
		}CL_IsThirdPerson_ctx;

		CL_IsThirdPerson_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges((void*)g_pMetaSave->pExportFuncs->CL_IsThirdPerson, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (CL_IsThirdPerson_ctx*)context;
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
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwClientBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwClientBase + g_dwClientSize)
				{
					ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwClientBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwClientBase + g_dwClientSize)
				{
					ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->iNumCandidates++;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		if (ctx.iNumCandidates >= 3 && ctx.Candidates[ctx.iNumCandidates - 1] == ctx.Candidates[ctx.iNumCandidates - 2] + sizeof(int))
		{
			g_iUser1 = (decltype(g_iUser1))ctx.Candidates[ctx.iNumCandidates - 2];
			g_iUser2 = (decltype(g_iUser2))ctx.Candidates[ctx.iNumCandidates - 1];
		}
	}
}

void Client_InstallHooks(void)
{
	
}

void Client_UninstallHooks(void)
{

}