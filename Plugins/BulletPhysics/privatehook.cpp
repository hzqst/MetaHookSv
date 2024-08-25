#include <metahook.h>
#include <capstone.h>

#include "enginedef.h"
#include "plugins.h"
#include "privatehook.h"
#include "exportfuncs.h"
#include "message.h"

#include "ClientPhysicManager.h"
#include "ClientEntityManager.h"
#include "Viewport.h"

#define R_NEWMAP_SIG_COMMON    "\x55\x8B\xEC\x83\xEC\x2A\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC"
#define R_NEWMAP_SIG_BLOB      R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_NEW       R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_HL25      R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_SVENGINE "\x55\x8B\xEC\x51\xC7\x45\xFC\x00\x00\x00\x00\xEB\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00"

#define R_RECURSIVEWORLDNODE_SIG_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE"
#define R_RECURSIVEWORLDNODE_SIG_NEW2 R_RECURSIVEWORLDNODE_SIG_BLOB
#define R_RECURSIVEWORLDNODE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x47\x04"
#define R_RECURSIVEWORLDNODE_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x08\x2A\x8B\x5D\x08\x83\x3B\xFE\x0F"
#define R_RECURSIVEWORLDNODE_SIG_SVENGINE "\x83\xEC\x08\x53\x8B\x5C\x24\x10\x83\x3B\xFE"

#define R_DRAWTENTITIESONLIST_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x0F\x2A\x2A\x2A\x00\x00\x8B\x44\x24\x04"
#define R_DRAWTENTITIESONLIST_SIG_NEW2 R_DRAWTENTITIESONLIST_SIG_BLOB
#define R_DRAWTENTITIESONLIST_SIG_NEW "\x55\x8B\xEC\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44\x0F\x8B\x2A\x2A\x2A\x2A\x8B\x45\x08"
#define R_DRAWTENTITIESONLIST_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\xF3\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x0F\x2E"
#define R_DRAWTENTITIESONLIST_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\x2A\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x00\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD9\xEE"

private_funcs_t gPrivateFuncs = {0};

studiohdr_t** pstudiohdr = NULL;
model_t** r_model = NULL;
void* g_pGameStudioRenderer = NULL;
int* r_framecount = NULL;
int* r_visframecount = NULL;
int* cl_parsecount = NULL;
void* cl_frames = NULL;
int size_of_frame = 0;
int* cl_viewentity = NULL;
cl_entity_t** currententity = NULL;
void* mod_known = NULL;
int* mod_numknown = NULL;
TEMPENTITY* gTempEnts = NULL;

//Sven Co-op only
int* allow_cheats = NULL;

int* g_iWaterLevel = NULL;
bool* g_bRenderingPortals_SCClient = NULL;

int* g_iUser1 = NULL;
int* g_iUser2 = NULL;

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

static hook_t* g_phook_R_NewMap = NULL;
static hook_t* g_phook_Mod_LoadStudioModel = NULL;

void Engine_FillAddreess(void)
{
	if (1)
	{
		const char sigs1[] = "Non-sprite set to glow";
		auto NonSprite_String = Search_Pattern_Data(sigs1);
		if (!NonSprite_String)
			NonSprite_String = Search_Pattern_Rdata(sigs1);
		if (NonSprite_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B";
			*(DWORD*)(pattern + 1) = (DWORD)NonSprite_String;
			auto NonSprite_Call = Search_Pattern(pattern);
			if (NonSprite_Call)
			{
				gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(NonSprite_Call, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05 &&
						Candidate[6] == 0xD8)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
				});
			}
		}
	}

	if (!gPrivateFuncs.R_DrawTEntitiesOnList)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW);
			if (!gPrivateFuncs.R_DrawTEntitiesOnList)
				gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW2);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_DrawTEntitiesOnList);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_NEW);
		if (!gPrivateFuncs.R_RecursiveWorldNode)
			gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_BLOB);
	}
	Sig_FuncNotFound(R_RecursiveWorldNode);

	if (1)
	{
		//Seach "CL_Reallocate cl_entities"
		const char sigs1[] = "CL_Reallocate cl_entities\n";
		auto CL_Reallocate_String = Search_Pattern_Data(sigs1);
		if (!CL_Reallocate_String)
			CL_Reallocate_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(CL_Reallocate_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 1) = (DWORD)CL_Reallocate_String;
		auto CL_Reallocate_Call = Search_Pattern(pattern);
		Sig_VarNotFound(CL_Reallocate_Call);

		auto CL_ReallocateDynamicData = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(CL_Reallocate_Call, 0x100, [](PUCHAR Candidate) {

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
			{
				return TRUE;
			}

			if (Candidate[0] == 0x8B &&
				Candidate[1] == 0x44 &&
				Candidate[2] == 0x24)
			{
				return TRUE;
			}

			if (Candidate[0] == 0xFF &&
				Candidate[2] == 0x24)
			{
				return TRUE;
			}

			return FALSE;
			});

		Sig_VarNotFound(CL_ReallocateDynamicData);

		typedef struct
		{
			PVOID CL_Reallocate_Call;
		}CL_ReallocateDynamicData_ctx;

		CL_ReallocateDynamicData_ctx ctx = { 0 };

		ctx.CL_Reallocate_Call = CL_Reallocate_Call;

		g_pMetaHookAPI->DisasmRanges(CL_ReallocateDynamicData, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (CL_ReallocateDynamicData_ctx*)context;

				if (!cl_max_edicts && pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					// mov     eax, cl_max_edicts
					// add     esp, 4

					if (0 == memcmp(address + instLen, "\x83\xC4\x04", 3))
					{
						cl_max_edicts = (decltype(cl_max_edicts))pinst->detail->x86.operands[1].mem.disp;
					}
				}

				if (!cl_max_edicts && pinst->id == X86_INS_IMUL &&
					pinst->detail->x86.op_count == 3 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[2].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					cl_max_edicts = (decltype(cl_max_edicts))pinst->detail->x86.operands[1].mem.disp;
				}

				if (!cl_entities && address > ctx->CL_Reallocate_Call && pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_EAX &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					// mov     eax, cl_max_edicts
					// add     esp, 4
					cl_entities = (decltype(cl_entities))pinst->detail->x86.operands[0].mem.disp;
				}

				if (cl_entities && cl_max_edicts)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Sig_VarNotFound(cl_max_edicts);
		Sig_VarNotFound(cl_entities);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#if 0
#define CL_MAXEDICTS_SIG_SVENGINE "\x69\xC0\xB8\x0B\x00\x00\x50\xE8\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xE8"
		DWORD addr = (DWORD)Search_Pattern(CL_MAXEDICTS_SIG_SVENGINE);
		Sig_AddrNotFound(cl_max_edicts);
		cl_max_edicts = *(decltype(cl_max_edicts)*)(addr + 14);
		cl_entities = *(decltype(cl_entities)*)(addr + 19);
#endif
#define GTEMPENTS_SIG_SVENGINE "\x68\x00\xE0\x5F\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xA3"
		if (1)
		{
			DWORD addr = (DWORD)Search_Pattern(GTEMPENTS_SIG_SVENGINE);
			Sig_AddrNotFound(gTempEnts);
			gTempEnts = *(decltype(gTempEnts)*)(addr + 8);
		}

	}
	else
	{
	
#if 0
#define CL_MAXEDICTS_SIG_NEW "\xC1\xE1\x03\x51\xE8\x2A\x2A\x2A\x2A\x8B\x15\x2A\x2A\x2A\x2A\xA3"
		DWORD addr = (DWORD)Search_Pattern(CL_MAXEDICTS_SIG_NEW);
		Sig_AddrNotFound(cl_max_edicts);
		cl_max_edicts = *(decltype(cl_max_edicts)*)(addr + 11);
		cl_entities = *(decltype(cl_entities)*)(addr + 16);
#endif

#define GTEMPENTS_SIG_NEW "\x68\x30\x68\x17\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8"
		if (1)
		{
			DWORD addr = (DWORD)Search_Pattern(GTEMPENTS_SIG_NEW);
			Sig_AddrNotFound(gTempEnts);
			gTempEnts = *(decltype(gTempEnts)*)(addr + 8);
		}
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		auto CL_Set_ServerExtraInfo = g_pMetaHookAPI->FindCLParseFuncByName("svc_sendextrainfo");

		Sig_VarNotFound(CL_Set_ServerExtraInfo);

		g_pMetaHookAPI->DisasmRanges(CL_Set_ServerExtraInfo, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_EAX)
				{
					allow_cheats = (decltype(allow_cheats))pinst->detail->x86.operands[0].mem.disp;
				}

				if (allow_cheats)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

		}, 0, NULL);

		Sig_VarNotFound(allow_cheats);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_SVENGINE);
		Sig_AddrNotFound(cl_viewentity);
		cl_viewentity = *(decltype(cl_viewentity)*)(addr + 10);
	}
	else
	{
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_GOLDSRC);
		Sig_AddrNotFound(cl_viewentity);

		typedef struct
		{
			bool found_cmp_200;
		}CL_ViewEntity_ctx;

		CL_ViewEntity_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges((PVOID)addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_ViewEntity_ctx*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x200)
			{
				ctx->found_cmp_200 = true;
			}

			if (ctx->found_cmp_200)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		if (ctx.found_cmp_200)
		{
			cl_viewentity = *(decltype(cl_viewentity)*)(addr + 1);
		}

		Sig_VarNotFound(cl_viewentity);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_NEW);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_BLOB);
	}
	Sig_FuncNotFound(R_NewMap);

	if (1)
	{
		typedef struct
		{
			int movexx_offset;
			int movexx_instcount;
			int movexx_register;
			int cmp_register;
			DWORD cmp_candidate;
			int test_cl_instcount;
			int test_cl_flag;
		}R_RecursiveWorldNode_ctx;

		R_RecursiveWorldNode_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_RecursiveWorldNode, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RecursiveWorldNode_ctx*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base != 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(pinst->detail->x86.operands[1].mem.disp == 0 || pinst->detail->x86.operands[1].mem.disp == 4)
				)
			{//.text:01D49235 8B 47 04                                            mov     eax, [edi+4]

				ctx->movexx_offset = pinst->detail->x86.operands[1].mem.disp;
				ctx->movexx_instcount = instCount;
				ctx->movexx_register = pinst->detail->x86.operands[0].reg;
			}
			else if (ctx->movexx_instcount &&
				instCount < ctx->movexx_instcount + 3 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{
				//.text:01D49238 8B 0D D4 98 BC 02                                   mov     ecx, r_visframecount

				ctx->cmp_register = pinst->detail->x86.operands[0].reg;
				ctx->cmp_candidate = (decltype(ctx->cmp_candidate))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (ctx->movexx_instcount &&
				instCount < ctx->movexx_instcount + 3 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{
				//.text:01D5A533 3B 05 7C 3F F5 03                                   cmp     eax, r_visframecount

				if (ctx->movexx_offset == 4 && !r_visframecount)
					r_visframecount = (decltype(r_visframecount))pinst->detail->x86.operands[1].mem.disp;
				else if (ctx->movexx_offset == 0 && !r_framecount)
					r_framecount = (decltype(r_framecount))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (ctx->movexx_instcount &&
				instCount < ctx->movexx_instcount + 3 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				((pinst->detail->x86.operands[0].reg == ctx->cmp_register &&
					pinst->detail->x86.operands[1].reg == ctx->movexx_register) ||
					(pinst->detail->x86.operands[1].reg == ctx->cmp_register &&
						pinst->detail->x86.operands[0].reg == ctx->movexx_register)))
			{
				//.text:01D49235 8B 47 04                                            mov     eax, [edi+4]
				//.text:01D49238 8B 0D D4 98 BC 02                                   mov     ecx, r_visframecount
				//.text:01D4923E 3B C1                                               cmp     eax, ecx

				//.text:01D4932E 8B 0E                                               mov     ecx, [esi]
				//.text:01D49330 A1 EC 97 BC 02                                      mov     eax, r_framecount
				//.text:01D49335 3B C8                                               cmp     ecx, eax
				if (ctx->movexx_offset == 4 && !r_visframecount)
					r_visframecount = (decltype(r_visframecount))ctx->cmp_candidate;
				else if (ctx->movexx_offset == 0 && !r_framecount)
					r_framecount = (decltype(r_framecount))ctx->cmp_candidate;
			}

			if (r_visframecount && r_framecount)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(r_framecount);
		Sig_VarNotFound(r_visframecount);
	}

	if (1)
	{
#define MOD_KNOWN_SIG "\xB8\x9D\x82\x97\x53\x81\xE9"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(MOD_KNOWN_SIG);
		Sig_AddrNotFound(mod_known);
		mod_known = *(void **)(addr + 7);
	}

	if (1)
	{
		const char sigs1[] = "bogus\0";
		auto Bogus_String = Search_Pattern_Data(sigs1);
		if (!Bogus_String)
			Bogus_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Bogus_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)Bogus_String;
		auto Bogus_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Bogus_Call);

		gPrivateFuncs.Mod_LoadStudioModel = (decltype(gPrivateFuncs.Mod_LoadStudioModel))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bogus_Call, 0x50, [](PUCHAR Candidate) {

			//  .text : 01D71630 81 EC 10 01 00 00                                   sub     esp, 110h
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[4] == 0x00 &&
				Candidate[5] == 0x00)
			{
				return TRUE;
			}
			//  .text : 01D61AD0 55                                                  push    ebp
			//  .text : 01D61AD1 8B EC                                               mov     ebp, esp
			//	.text : 01D61AD3 81 EC 0C 01 00 00                                   sub     esp, 10Ch
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x81 &&
				Candidate[4] == 0xEC &&
				Candidate[7] == 0x00 &&
				Candidate[8] == 0x00)
			{
				return TRUE;
			}

			return FALSE;
		});
		Sig_FuncNotFound(Mod_LoadStudioModel);
	}

	if (1)
	{
		const char sigs1[] = "Cached models:\n";
		auto Mod_Print_String = Search_Pattern_Data(sigs1);
		if (!Mod_Print_String)
			Mod_Print_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Mod_Print_String);
		char pattern[] = "\x57\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 2) = (DWORD)Mod_Print_String;
		auto Mod_Print_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Mod_Print_Call);

		g_pMetaHookAPI->DisasmRanges(Mod_Print_Call, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn *)inst;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0)
				{//A1 84 5C 32 02 mov     eax, mod_numknown
					DWORD imm = pinst->detail->x86.operands[1].mem.disp;

					mod_numknown = (decltype(mod_numknown))imm;
				}
				else if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{//39 3D 44 32 90 03 cmp     mod_numknown, edi
					DWORD imm = pinst->detail->x86.operands[0].mem.disp;

					mod_numknown = (decltype(mod_numknown))imm;
				}

				if (mod_numknown)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, NULL);
	}

	if (1)
	{
		typedef struct
		{
			int disableFog_instcount;
			int parsemod_instcount;
			int getskin_instcount;
		}R_DrawTEntitiesOnList_ctx;

		R_DrawTEntitiesOnList_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_DrawTEntitiesOnList, 0x500, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn *)inst;
				auto ctx = (R_DrawTEntitiesOnList_ctx *)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					//.text:01D923D9 A1 DC 72 ED 01                                      mov     eax, cl_parsemod
					//.text:01D88CBB A1 CC AF E3 01                                      mov     eax, cl_parsemod
					DWORD value = *(DWORD *)pinst->detail->x86.operands[1].mem.disp;
					if (value == 63)
					{
						ctx->parsemod_instcount = instCount;
					}
				}
				else if (!cl_parsecount && ctx->parsemod_instcount &&
					instCount < ctx->parsemod_instcount + 3 &&
					(pinst->id == X86_INS_MOV || pinst->id == X86_INS_AND) &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					//.text:01D923DE 23 05 AC D2 30 02                                   and     eax, cl_parsecount
					//.text:01D88CC0 8B 0D 04 AE D8 02                                   mov     ecx, cl_parsecount
					cl_parsecount = (decltype(cl_parsecount))pinst->detail->x86.operands[1].mem.disp;
				}
				else if (!cl_frames && ctx->parsemod_instcount &&
					instCount < ctx->parsemod_instcount + 20 &&
					pinst->id == X86_INS_LEA &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base != 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					//.text:01D923F0 8D 80 F4 D5 30 02                                   lea     eax, cl_frames[eax]
					//.text:01D88CE8 8D 84 CA 4C B1 D8 02                                lea     eax, cl_frames_1[edx+ecx*8]
					cl_frames = (decltype(cl_frames))pinst->detail->x86.operands[1].mem.disp;
				}
				else if (ctx->parsemod_instcount &&
					instCount < ctx->parsemod_instcount + 5 &&
					pinst->id == X86_INS_IMUL &&
					pinst->detail->x86.op_count == 3 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[2].type == X86_OP_IMM &&
					pinst->detail->x86.operands[2].imm > 0x4000 &&
					pinst->detail->x86.operands[2].imm < 0xF000)
				{
					//.text:01D923E4 69 C8 D8 84 00 00                                   imul    ecx, eax, 84D8h
					size_of_frame = pinst->detail->x86.operands[2].imm;
				}
				else if (
					pinst->id == X86_INS_MOVSX &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].size == 4 &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].size == 2 &&
					pinst->detail->x86.operands[1].mem.base != 0 &&
					pinst->detail->x86.operands[1].mem.disp == 0x2E8)
				{
					//.text:01D924D9 0F BF 83 E8 02 00 00                                movsx   eax, word ptr [ebx+2E8h]
					ctx->getskin_instcount = instCount;
				}

				if (cl_parsecount && cl_frames)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Sig_VarNotFound(cl_frames);
		Sig_VarNotFound(cl_parsecount);
	}

	if (1)
	{
		char pattern[] = "\x8B\x0D\x2A\x2A\x2A\x2A\x81\xF9\x00\x2A\x00\x00";
		auto ClientDLL_AddEntity_Pattern = Search_Pattern(pattern);
		Sig_VarNotFound(ClientDLL_AddEntity_Pattern);

		cl_numvisedicts = *(decltype(cl_numvisedicts)*)((PUCHAR)ClientDLL_AddEntity_Pattern + 2);

		g_pMetaHookAPI->DisasmRanges(ClientDLL_AddEntity_Pattern, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

				if (!cl_visedicts &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == X86_REG_ECX &&
					pinst->detail->x86.operands[0].mem.scale == 4 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{
					//.text:01D198C9 89 04 8D 00 3A 6E 02                                mov     cl_visedicts[ecx*4], eax
					//.text:01D0C7C5 89 14 8D C0 F0 D5 02                                mov     cl_visedicts[ecx*4], edx
					DWORD imm = pinst->detail->x86.operands[0].mem.disp;

					cl_visedicts = (decltype(cl_visedicts))imm;
				}

				if (cl_visedicts)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, NULL);

		Sig_VarNotFound(cl_visedicts);
	}
}

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

	if ((void *)g_pMetaSave->pExportFuncs->CL_IsThirdPerson > g_dwClientTextBase && (void *)g_pMetaSave->pExportFuncs->CL_IsThirdPerson < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		typedef struct
		{
			ULONG_PTR Candidates[16];
			int iNumCandidates;

		}CL_IsThirdPerson_ctx;

		CL_IsThirdPerson_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges((void *)g_pMetaSave->pExportFuncs->CL_IsThirdPerson, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (CL_IsThirdPerson_ctx *)context;
			auto pinst = (cs_insn *)inst;

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
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwClientDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwClientDataBase + g_dwClientDataSize)
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwClientDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwClientDataBase + g_dwClientDataSize)
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

	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory)
	{
		auto SCClient001 = pfnClientFactory("SCClientDLL001", 0);
		if (SCClient001)
		{
			if (1)
			{
				const char pattern[] = "\x6A\x00\x6A\x00\x6A\x00\x8B\x2A\xFF\x50\x2A";
				auto addr = Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, pattern);
				Sig_AddrNotFound(g_bRenderingPortals);

				g_pMetaHookAPI->DisasmRanges(addr, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwClientDataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwClientDataBase + g_dwClientDataSize &&
						pinst->detail->x86.operands[1].imm == 1)
					{
						g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))pinst->detail->x86.operands[0].mem.disp;
						return TRUE;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

				}, 0, NULL);

				Sig_VarNotFound(g_bRenderingPortals_SCClient);
			}

			g_bIsSvenCoop = true;
		}
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		//g_PlayerExtraInfo
		//66 85 C0 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ??
		/*
		.text:019A4575 66 85 C0                                            test    ax, ax
		.text:019A4578 66 89 99 20 F4 A2 01                                mov     word_1A2F420[ecx], bx
		.text:019A457F 66 89 A9 22 F4 A2 01                                mov     word_1A2F422[ecx], bp
		.text:019A4586 66 89 91 48 F4 A2 01                                mov     word_1A2F448[ecx], dx
		.text:019A458D 66 89 81 4A F4 A2 01                                mov     word_1A2F44A[ecx], ax
		*/
#define CSTRIKE_PLAYEREXTRAINFO_SIG      "\x66\x85\xC0\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89"
#define CSTRIKE_PLAYEREXTRAINFO_SIG_HL25 "\x66\x89\x90\x2A\x2A\x2A\x2A\x8B\x55\x2A\x66\x89\x98\x2A\x2A\x2A\x2A\x66\x89\x90\x2A\x2A\x2A\x2A\x66\x89\x88"

		auto addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, CSTRIKE_PLAYEREXTRAINFO_SIG);
		if (addr)
		{
			g_PlayerExtraInfo = *(decltype(g_PlayerExtraInfo)*)(addr + 6);
		}
		else
		{
			addr = (ULONG_PTR)Search_Pattern_From_Size(g_dwClientTextBase, g_dwClientTextSize, CSTRIKE_PLAYEREXTRAINFO_SIG_HL25);
			if (addr)
			{
				g_PlayerExtraInfo = *(decltype(g_PlayerExtraInfo)*)(addr + 3);
			}
		}

		Sig_VarNotFound(g_PlayerExtraInfo);

	}
}

void R_NewMap(void)
{
	r_worldentity = gEngfuncs.GetEntityByIndex(0);
	r_worldmodel = r_worldentity->model;
	
	gPrivateFuncs.R_NewMap();
	ClientPhysicManager()->NewMap();
	ClientEntityManager()->NewMap();
	g_pViewPort->NewMap();
}

TEMPENTITY *efxapi_R_TempModel(float *pos, float *dir, float *angles, float life, int modelIndex, int soundtype)
{
	auto r = gPrivateFuncs.efxapi_R_TempModel(pos, dir, angles, life, modelIndex, soundtype);

	if (r && g_bIsCreatingClCorpse && g_iCreatingClCorpsePlayerIndex > 0 && g_iCreatingClCorpsePlayerIndex <= gEngfuncs.GetMaxClients())
	{
		int entindex = r - gTempEnts + ENTINDEX_TEMPENTITY;

		r->entity.curstate.iuser4 = PhyCorpseFlag;
		r->entity.curstate.owner = g_iCreatingClCorpsePlayerIndex;
	}

	return r;
}

void Mod_LoadStudioModel(model_t* mod, void* buffer)
{
	gPrivateFuncs.Mod_LoadStudioModel(mod, buffer);
}

void Engine_InstallHook(void)
{
	Install_InlineHook(R_NewMap);
	Install_InlineHook(Mod_LoadStudioModel);
}

void Engine_UninstallHook(void)
{
	Uninstall_Hook(R_NewMap);
	Uninstall_Hook(Mod_LoadStudioModel);
}