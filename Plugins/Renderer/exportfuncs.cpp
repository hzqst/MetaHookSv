#include <metahook.h>
#include <capstone.h>
#include "exportfuncs.h"
#include "gl_local.h"
#include "parsemsg.h"
#include "qgl.h"

#include "UtilThreadTask.h"

#include <set>

//Error when can't find sig

cl_enginefunc_t gEngfuncs = {0};
engine_studio_api_t IEngineStudio = { 0 };
r_studio_interface_t **gpStudioInterface = NULL;
void *g_pGameStudioRenderer = NULL;

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;
bool g_bIsAoMDC = false;

static hook_t *g_phook_GameStudioRenderer_StudioDrawPlayer = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioSetupBones = NULL;
static hook_t* g_phook_GameStudioRenderer_StudioSaveBones = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioMergeBones = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioRenderModel = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioRenderFinal = NULL;

static hook_t* g_phook_R_StudioDrawPlayer = NULL;
static hook_t *g_phook_R_StudioSetupBones = NULL;
static hook_t *g_phook_R_StudioMergeBones = NULL;
static hook_t* g_phook_R_StudioSaveBones = NULL;
static hook_t *g_phook_R_StudioRenderModel = NULL;
static hook_t *g_phook_R_StudioRenderFinal = NULL;

static hook_t* g_phook_studioapi_GL_SetRenderMode = NULL;
static hook_t* g_phook_studioapi_SetupRenderer = NULL;
static hook_t* g_phook_studioapi_RestoreRenderer = NULL;
static hook_t* g_phook_studioapi_StudioDynamicLight = NULL;
static hook_t* g_phook_studioapi_StudioCheckBBox = NULL;

static hook_t* g_phook_CL_FxBlend = NULL;

void EngineStudio_UninstallHooks(void)
{
	Uninstall_Hook(studioapi_GL_SetRenderMode);
	Uninstall_Hook(studioapi_SetupRenderer);
	Uninstall_Hook(studioapi_RestoreRenderer);
	Uninstall_Hook(studioapi_StudioDynamicLight);
	Uninstall_Hook(studioapi_StudioCheckBBox);

	Uninstall_Hook(CL_FxBlend);

	Uninstall_Hook(R_StudioRenderModel);
	Uninstall_Hook(R_StudioRenderFinal);
	Uninstall_Hook(R_StudioSetupBones);
	Uninstall_Hook(R_StudioMergeBones);
	Uninstall_Hook(R_StudioSaveBones);
}

void ClientStudio_UninstallHooks(void)
{
	Uninstall_Hook(GameStudioRenderer_StudioSetupBones);
	Uninstall_Hook(GameStudioRenderer_StudioMergeBones);
	Uninstall_Hook(GameStudioRenderer_StudioSaveBones);
	Uninstall_Hook(GameStudioRenderer_StudioRenderModel);
	Uninstall_Hook(GameStudioRenderer_StudioRenderFinal);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	R_Init();

	gEngfuncs.pfnAddCommand("r_version", R_Version_f);
	gEngfuncs.pfnAddCommand("r_reload", R_Reload_f);
	gEngfuncs.pfnAddCommand("r_dumptextures", R_DumpTextures_f);

#if 0
	gEngfuncs.pfnAddCommand("r_buildcubemaps", R_BuildCubemaps_f);
	gEngfuncs.pfnAddCommand("buildcubemaps", R_BuildCubemaps_f);
#endif

	//gl_texturemode is command in SvEngine, but cvar in GoldSrc
	if (!g_pMetaHookAPI->HookCmd("gl_texturemode", GL_Texturemode_f))
	{
		g_pMetaHookAPI->HookCvarCallback("gl_texturemode", GL_Texturemode_cb);
	}
}

int HUD_VidInit(void)
{
	return gExportfuncs.HUD_VidInit();
}

void V_CalcRefdef(struct ref_params_s *pparams)
{
	if (g_iStartDist_SCClient && g_iEndDist_SCClient)
	{
		/*
			//Elimate Sv Co-op client's fixed-function fog
			if ( g_iWaterLevel <= 2 && g_iStartDist_SCClient >= 0.0 && g_iEndDist_SCClient > 0.0 )
			{
				glFogi(GL_FOG_MODE, GL_LINEAR);
				v157[0] = *(float *)&dword_1063A6D4 / 255.0;
				v157[1] = *(float *)&dword_1063A6D8 / 255.0;
				v157[2] = *(float *)&dword_1063A6DC / 255.0;
				glFogfv(0xB66u, v157);
				glHint(GL_FOG_HINT, GL_DONT_CARE);
				glFogf(GL_FOG_START, g_iStartDist_SCClient);
				glFogf(GL_FOG_END, g_iEndDist_SCClient);
				glEnable(GL_FOG);
			}
		*/

		float Saved_iStartDist_SCClient = (*g_iStartDist_SCClient);
		float Saved_iEndDist_SCClient = (*g_iEndDist_SCClient);

		(*g_iStartDist_SCClient) = 0;
		(*g_iEndDist_SCClient) = 0;

		gExportfuncs.V_CalcRefdef(pparams);

		(*g_iStartDist_SCClient) = Saved_iStartDist_SCClient;
		(*g_iEndDist_SCClient) = Saved_iEndDist_SCClient;
	}
	else
	{
		gExportfuncs.V_CalcRefdef(pparams);
	}

	memcpy(&r_params, pparams, sizeof(struct ref_params_s));
}

int HUD_Redraw(float time, int intermission)
{
	return gExportfuncs.HUD_Redraw(time, intermission);
}

void EngineStudio_FillAddress_GetCurrentEntity(struct engine_studio_api_s* pstudio, const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID GetCurrentEntity = ConvertDllInfoSpace(pstudio->GetCurrentEntity, RealDllInfo, DllInfo);

	if (!GetCurrentEntity)
	{
		Sig_NotFound(GetCurrentEntity);
	}

	/*
	//Global pointers that link into engine vars.
		cl_entity_t **currententity = NULL;
	*/

	typedef struct GetCurrentEntity_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}GetCurrentEntity_SearchContext;

	GetCurrentEntity_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)GetCurrentEntity, 0x10, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (GetCurrentEntity_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				currententity = (decltype(currententity))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (currententity)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

	}, 0, &ctx);

	Sig_VarNotFound(currententity);
}

void EngineStudio_FillAddress_GetTimes(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID GetTimes = ConvertDllInfoSpace(pstudio->GetTimes, RealDllInfo, DllInfo);

	if (!GetTimes)
	{
		Sig_NotFound(GetTimes);
	}

	typedef struct GetTimes_SearchContext_t
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;

		ULONG_PTR candidates[10]{};
		int candidate_count{};
	}GetTimes_SearchContext;

	GetTimes_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)GetTimes, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (GetTimes_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D87E06 8B 0D EC 97 BC 02                                   mov     ecx, r_framecount  
				if (ctx->candidate_count < 10)
				{
					ctx->candidates[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->candidate_count++;
				}
			}

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D87E06 8B 0D EC 97 BC 02                                   mov     ecx, r_framecount  

				if (ctx->candidate_count < 10)
				{
					ctx->candidates[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
					ctx->candidate_count++;
				}
			}

			if (pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				if (!cl_time)
					cl_time = (decltype(cl_time))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				else if (!cl_oldtime)
					cl_oldtime = (decltype(cl_oldtime))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
			if (pinst->id == X86_INS_MOVSD &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{// movsd   xmm0, cl_time	

				if (!cl_time)
					cl_time = (decltype(cl_time))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				else if (!cl_oldtime)
					cl_oldtime = (decltype(cl_oldtime))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	if (ctx.candidate_count >= 1)
	{
		r_framecount = (decltype(r_framecount))ConvertDllInfoSpace((PVOID)ctx.candidates[0], DllInfo, RealDllInfo);
	}

	if (ctx.candidate_count == 5)
	{
		cl_time = (decltype(cl_time))ConvertDllInfoSpace((PVOID)ctx.candidates[1], DllInfo, RealDllInfo);
		cl_oldtime = (decltype(cl_oldtime))ConvertDllInfoSpace((PVOID)ctx.candidates[3], DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(r_framecount);
	Sig_VarNotFound(cl_time);
	Sig_VarNotFound(cl_oldtime);
}

void EngineStudio_FillAddress_SetRenderModel(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID SetRenderModel = ConvertDllInfoSpace(pstudio->SetRenderModel, RealDllInfo, DllInfo);

	if (!SetRenderModel)
	{
		Sig_NotFound(SetRenderModel);
	}

	typedef struct 
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}SetRenderModel_SearchContext;

	SetRenderModel_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)SetRenderModel, 0x10, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (SetRenderModel_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				r_model = (decltype(r_model))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (r_model)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(r_model);
}

void EngineStudio_FillAddress_StudioSetHeader(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID StudioSetHeader = ConvertDllInfoSpace(pstudio->StudioSetHeader, RealDllInfo, DllInfo);

	if (!StudioSetHeader)
	{
		Sig_NotFound(StudioSetHeader);
	}

	{
		typedef struct StudioSetHeader_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}StudioSetHeader_SearchContext;

		StudioSetHeader_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)StudioSetHeader, 0x10, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (StudioSetHeader_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{
					pstudiohdr = (decltype(pstudiohdr))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (pstudiohdr)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);
	}

	Sig_VarNotFound(pstudiohdr);
}

void EngineStudio_FillAddress_SetForceFaceFlags(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID SetForceFaceFlags = ConvertDllInfoSpace(pstudio->SetForceFaceFlags, RealDllInfo, DllInfo);

	if (!SetForceFaceFlags)
	{
		Sig_NotFound(SetForceFaceFlags);
	}

	{
		typedef struct
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}SetForceFaceFlags_SearchContext;

		SetForceFaceFlags_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)SetForceFaceFlags, 0x10, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (SetForceFaceFlags_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{
					g_ForcedFaceFlags = (decltype(g_ForcedFaceFlags))ConvertDllInfoSpace((PVOID) pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (g_ForcedFaceFlags)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

			}, 0, &ctx);

	}

	Sig_VarNotFound(g_ForcedFaceFlags);
}

void EngineStudio_FillAddress_StudioSetRemapColors(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID StudioSetRemapColors = ConvertDllInfoSpace(pstudio->StudioSetRemapColors, RealDllInfo, DllInfo);

	if (!StudioSetRemapColors)
	{
		Sig_NotFound(StudioSetRemapColors);
	}

	{
		typedef struct
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}StudioSetRemapColors_SearchContext;

		StudioSetRemapColors_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)StudioSetRemapColors, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (StudioSetRemapColors_SearchContext*)context;

			if (!r_topcolor && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				r_topcolor = (decltype(r_topcolor))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (r_topcolor && !r_bottomcolor && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				if ((PVOID)r_topcolor != (PVOID)ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo))
				{
					r_bottomcolor = (decltype(r_bottomcolor))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}
			}

			if (r_topcolor && r_bottomcolor)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	}

	Sig_VarNotFound(r_topcolor);
	Sig_VarNotFound(r_bottomcolor);
}

void EngineStudio_FillAddress_StudioSetRenderamt(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID StudioSetRenderamt = ConvertDllInfoSpace(pstudio->StudioSetRenderamt, RealDllInfo, DllInfo);

	if (!StudioSetRenderamt)
	{
		Sig_NotFound(StudioSetRenderamt);
	}

	{
		typedef struct
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}StudioSetRenderamt_SearchContext;

		StudioSetRenderamt_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)StudioSetRenderamt, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (StudioSetRenderamt_SearchContext*)context;

				if (!gPrivateFuncs.CL_FxBlend && 
					address[0] == 0xE8 && instLen == 5)
				{
					gPrivateFuncs.CL_FxBlend = (decltype(gPrivateFuncs.CL_FxBlend))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
				}

				else if (!r_blend && 
					pinst->id == X86_INS_FSTP &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0)
				{
					r_blend = (decltype(r_blend))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (gPrivateFuncs.CL_FxBlend && r_blend)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

	}

	Sig_VarNotFound(r_blend);
	Sig_FuncNotFound(CL_FxBlend);
}

void EngineStudio_FillAddress_SetupRenderer(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID SetupRenderer = ConvertDllInfoSpace(pstudio->SetupRenderer, RealDllInfo, DllInfo);

	if (!SetupRenderer)
	{
		Sig_NotFound(SetupRenderer);
	}
	/*
	//Global pointers that link into engine
	auxvert_t** pauxverts = NULL;
	float** pvlightvalues = NULL;
	*/
	{
		typedef struct
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}SetupRenderer_SearchContext;

		SetupRenderer_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)SetupRenderer, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (SetupRenderer_SearchContext*)context;

				if (address[0] == 0xC7 && address[1] == 0x05 && instLen == 10)//C7 05 C0 7D 73 02 98 14 36 02 mov     pauxverts, offset auxverts
				{
					if (!pauxverts)
					{
						auto pauxverts_VA = *(ULONG_PTR*)(address + 2);
						auto auxverts_VA = *(ULONG_PTR*)(address + 6);

						pauxverts = (decltype(pauxverts))ConvertDllInfoSpace((PVOID)pauxverts_VA, ctx->DllInfo, ctx->RealDllInfo);
						auxverts = (decltype(auxverts))ConvertDllInfoSpace((PVOID)auxverts_VA, ctx->DllInfo, ctx->RealDllInfo);
					}
					else if (!pvlightvalues)
					{
						auto pvlightvalues_VA = *(ULONG_PTR*)(address + 2);
						auto lightvalues_VA = *(ULONG_PTR*)(address + 6);

						pvlightvalues = (decltype(pvlightvalues))ConvertDllInfoSpace((PVOID)pvlightvalues_VA, ctx->DllInfo, ctx->RealDllInfo);
						lightvalues = (decltype(lightvalues))ConvertDllInfoSpace((PVOID)lightvalues_VA, ctx->DllInfo, ctx->RealDllInfo);
					}
				}

				if (pauxverts && auxverts && pvlightvalues && lightvalues)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

	}

	Sig_VarNotFound(pauxverts);
	Sig_VarNotFound(auxverts);
	Sig_VarNotFound(pvlightvalues);
	Sig_VarNotFound(lightvalues);
}

void EngineStudio_FillAddress_StudioSetupModel(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID StudioSetupModel = ConvertDllInfoSpace(pstudio->StudioSetupModel, RealDllInfo, DllInfo);

	if (!StudioSetupModel)
	{
		Sig_NotFound(StudioSetupModel);
	}

	/*
mstudiomodel_t** psubmodel = NULL;
mstudiobodyparts_t** pbodypart = NULL;
	*/

	typedef struct
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}StudioSetupModel_SearchContext;

	StudioSetupModel_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)StudioSetupModel, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (StudioSetupModel_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				pinst->detail->x86.operands[0].mem.disp == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D87E55 C7 01 B8 94 37 02                                   mov     dword ptr [ecx], offset pbodypart
				if (!pbodypart)
				{
					pbodypart = (decltype(pbodypart))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
				}
				else if (!psubmodel)
				{
					psubmodel = (decltype(psubmodel))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
				}
			}

			if (pbodypart && psubmodel)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(pbodypart);
	Sig_VarNotFound(psubmodel);
}

void EngineStudio_FillAddress_SetChromeOrigin(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID SetChromeOrigin = ConvertDllInfoSpace(pstudio->SetChromeOrigin, RealDllInfo, DllInfo);

	if (!SetChromeOrigin)
	{
		Sig_NotFound(SetChromeOrigin);
	}

	typedef struct SetChromeOrigin_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;

		ULONG_PTR r_origin_candidateVA[3]{};
		int r_origin_candidate_count{};
		ULONG_PTR g_ChromeOrigin_candidateVA[3]{};
		int g_ChromeOrigin_candidate_count{};

	}SetChromeOrigin_SearchContext;

	SetChromeOrigin_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)SetChromeOrigin, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (SetChromeOrigin_SearchContext*)context;

			if (pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//D9 05 00 40 F5 03 fld     r_origin
				if (ctx->r_origin_candidate_count < 3)
				{
					ctx->r_origin_candidateVA[ctx->r_origin_candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->r_origin_candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				if (ctx->r_origin_candidate_count < 3)
				{
					ctx->r_origin_candidateVA[ctx->r_origin_candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->r_origin_candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				if (ctx->r_origin_candidate_count < 3)
				{
					ctx->r_origin_candidateVA[ctx->r_origin_candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
					ctx->r_origin_candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_MOVQ &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				if (ctx->r_origin_candidate_count < 3)
				{
					ctx->r_origin_candidateVA[ctx->r_origin_candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->r_origin_candidate_count++;
				}
			}

			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//A3 40 88 35 02 mov     g_ChromeOrigin, eax
				if (ctx->g_ChromeOrigin_candidate_count < 3)
				{
					ctx->g_ChromeOrigin_candidateVA[ctx->g_ChromeOrigin_candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->g_ChromeOrigin_candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_MOVQ &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//A3 40 88 35 02 mov     g_ChromeOrigin, eax
				if (ctx->g_ChromeOrigin_candidate_count < 3)
				{
					ctx->g_ChromeOrigin_candidateVA[ctx->g_ChromeOrigin_candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->g_ChromeOrigin_candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//D9 1D A0 39 DB 08 fstp    g_ChromeOrigin
				if (ctx->g_ChromeOrigin_candidate_count < 3)
				{
					ctx->g_ChromeOrigin_candidateVA[ctx->g_ChromeOrigin_candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->g_ChromeOrigin_candidate_count++;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

	if (ctx.r_origin_candidate_count >= 2)
	{
		std::qsort(ctx.r_origin_candidateVA, ctx.r_origin_candidate_count, sizeof(ctx.r_origin_candidateVA[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
		});
		r_origin = (decltype(r_origin))ConvertDllInfoSpace((PVOID)ctx.r_origin_candidateVA[0], DllInfo, RealDllInfo);
	}

	if (ctx.g_ChromeOrigin_candidate_count >= 2)
	{
		std::qsort(ctx.g_ChromeOrigin_candidateVA, ctx.g_ChromeOrigin_candidate_count, sizeof(ctx.g_ChromeOrigin_candidateVA[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
		});
		g_ChromeOrigin = (decltype(g_ChromeOrigin))ConvertDllInfoSpace((PVOID)ctx.g_ChromeOrigin_candidateVA[0], DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(r_origin);
	Sig_VarNotFound(g_ChromeOrigin);
}

void EngineStudio_FillAddress_StudioSetupLighting(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID StudioSetupLighting = ConvertDllInfoSpace(pstudio->StudioSetupLighting, RealDllInfo, DllInfo);

	if (!StudioSetupLighting)
	{
		Sig_NotFound(StudioSetupLighting);
	}

	typedef struct StudioSetupLighting_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;

		ULONG_PTR candidateVA[10]{};
		int candidate_count{};
		int and_FF00_start{};

	}StudioSetupLighting_SearchContext;

	StudioSetupLighting_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)StudioSetupLighting, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (StudioSetupLighting_SearchContext*)context;

			if (pinst->id == X86_INS_AND &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0xFF00)
			{
				ctx->candidate_count = 0;
				ctx->and_FF00_start = 1;
			}
			else if (ctx->and_FF00_start &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//.text:01D84A49 89 0D 04 AE 75 02                                   mov     r_colormix+4, ecx
				if (ctx->candidate_count < 10)
				{
					ctx->candidateVA[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
				}
			}
			else if (ctx->and_FF00_start &&
				pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D8F6AD D9 1D F0 EA 51 08                                   fstp    r_colormix

				if (ctx->candidate_count < 10)
				{
					ctx->candidateVA[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
				}
			}
			else if (ctx->and_FF00_start &&
				pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D8F6AD D9 1D F0 EA 51 08                                   fstp    r_colormix

				if (ctx->candidate_count < 10)
				{
					ctx->candidateVA[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	if (ctx.candidate_count >= 3)
	{
		std::qsort(ctx.candidateVA, ctx.candidate_count, sizeof(ctx.candidateVA[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
			});

		//other, other, other, r_colormix[0], r_colormix[1], r_colormix[2]
		if (ctx.candidateVA[ctx.candidate_count - 3] + 4 == ctx.candidateVA[ctx.candidate_count - 2] &&
			ctx.candidateVA[ctx.candidate_count - 2] + 4 == ctx.candidateVA[ctx.candidate_count - 1])
		{
			r_colormix = (decltype(r_colormix))ConvertDllInfoSpace((PVOID)ctx.candidateVA[ctx.candidate_count - 3], DllInfo, RealDllInfo);
		}
		//r_colormix[0], r_colormix[1], r_colormix[2], other, other, other
		else if (ctx.candidateVA[0] + 4 == ctx.candidateVA[1] &&
			ctx.candidateVA[1] + 4 == ctx.candidateVA[2])
		{
			r_colormix = (decltype(r_colormix))ConvertDllInfoSpace((PVOID)ctx.candidateVA[0], DllInfo, RealDllInfo);
		}
	}

	Sig_VarNotFound(r_colormix);
}

void EngineStudio_FillAddress(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	EngineStudio_FillAddress_GetCurrentEntity(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_GetTimes(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_SetRenderModel(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetHeader(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_SetForceFaceFlags(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetRemapColors(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetRenderamt(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_SetupRenderer(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetupModel(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_SetChromeOrigin(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetupLighting(pstudio, DllInfo, RealDllInfo);
}

void EngineStudio_InstalHooks()
{
	Install_InlineHook(studioapi_GL_SetRenderMode);
	Install_InlineHook(studioapi_SetupRenderer);
	Install_InlineHook(studioapi_RestoreRenderer);
	Install_InlineHook(studioapi_StudioDynamicLight);
	Install_InlineHook(studioapi_StudioCheckBBox);
	Install_InlineHook(CL_FxBlend);
}

void ClientStudio_FillAddress_StudioDrawPlayer(struct r_studio_interface_s** ppinterface, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto StudioDrawPlayerThunk = (PUCHAR)(*ppinterface)->StudioDrawPlayer;

	if (StudioDrawPlayerThunk)
	{
		StudioDrawPlayerThunk = (decltype(StudioDrawPlayerThunk))ConvertDllInfoSpace(StudioDrawPlayerThunk, RealDllInfo, DllInfo);
	}

	if (StudioDrawPlayerThunk)
	{
		//There is a E9 jmp in Debug build and we need to skip it.
		if (StudioDrawPlayerThunk[0] == 0xE9)
		{
			StudioDrawPlayerThunk = (PUCHAR)GetCallAddress(StudioDrawPlayerThunk);
		}
	}

	if (StudioDrawPlayerThunk)
	{
		typedef struct
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}StudioDrawPlayer_SearchContext;

		StudioDrawPlayer_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)StudioDrawPlayerThunk, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (StudioDrawPlayer_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ECX &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				g_pGameStudioRenderer = (decltype(g_pGameStudioRenderer))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (g_pGameStudioRenderer)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		//g_pGameStudioRenderer is mandatory in this case

		Sig_VarNotFound(g_pGameStudioRenderer);

		g_pMetaHookAPI->DisasmRanges((void*)StudioDrawPlayerThunk, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (StudioDrawPlayer_SearchContext*)context;

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp >= 8 && pinst->detail->x86.operands[0].mem.disp <= 0x200)
			{
				gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
			}

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				PVOID callTarget = (PVOID)pinst->detail->x86.operands[0].imm;

				PVOID* vftable = *(PVOID**)g_pGameStudioRenderer;

				for (int i = 1; i < 4; ++i)
				{
					if (GetVFunctionFromVFTable(vftable, i, ctx->DllInfo, ctx->RealDllInfo, ctx->DllInfo) == callTarget)
					{
						gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = i;
						break;
					}
				}
			}

			if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index == 0)
			gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = 3;

		PVOID* vftable = *(PVOID**)g_pGameStudioRenderer;

		gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))
			GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index, DllInfo, RealDllInfo, RealDllInfo);
		Sig_FuncNotFound(GameStudioRenderer_StudioDrawPlayer);
	}
}

void ClientStudio_FillAddress_StudioDrawModel(struct r_studio_interface_s** ppinterface, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	ULONG_PTR StudioDrawModel_VA = 0;
	ULONG StudioDrawModel_RVA = 0;

	auto StudioDrawModelThunk = (PUCHAR)(*ppinterface)->StudioDrawModel;

	if (StudioDrawModelThunk)
	{
		StudioDrawModelThunk = (decltype(StudioDrawModelThunk))ConvertDllInfoSpace(StudioDrawModelThunk, RealDllInfo, DllInfo);
	}

	if (StudioDrawModelThunk)
	{
		//There is a E9 jmp in Debug build and we need to skip it.
		if (StudioDrawModelThunk[0] == 0xE9)
		{
			StudioDrawModelThunk = (PUCHAR)GetCallAddress(StudioDrawModelThunk);
		}
	}

	if (StudioDrawModelThunk)
	{
		PVOID* vftable = *(PVOID**)g_pGameStudioRenderer;

		{
			typedef struct StudioDrawModel_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
			}StudioDrawModel_SearchContext;

			StudioDrawModel_SearchContext ctx = { DllInfo, RealDllInfo };

			g_pMetaHookAPI->DisasmRanges((void*)StudioDrawModelThunk, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (StudioDrawModel_SearchContext*)context;

				if (pinst->id == X86_INS_CALL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					pinst->detail->x86.operands[0].mem.disp >= 8 && pinst->detail->x86.operands[0].mem.disp <= 0x200)
				{
					gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
				}

				if (pinst->id == X86_INS_CALL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM)
				{
					PVOID callTarget = (PVOID)pinst->detail->x86.operands[0].imm;
					PVOID* vftable = *(PVOID**)g_pGameStudioRenderer;

					for (int i = 1; i < 4; ++i)
					{
						if (GetVFunctionFromVFTable(vftable, i, ctx->DllInfo, ctx->RealDllInfo, ctx->DllInfo) == callTarget)
						{
							gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = i;
							break;
						}
					}
				}

				if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

			if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index == 0)
				gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = 2;
		}

		{
			//Search for GameStudioRenderer_StudioCalcAttachments_vftable_index

			for (int i = 4; i < 9; ++i)
			{
				typedef struct StudioCalcAttachments_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					PVOID base{};
					size_t max_insts{};
					int max_depth{};
					std::set<PVOID> code;
					std::set<PVOID> branches;
					std::vector<walk_context_t> walks;
					int vftable_index{};
					bool bFoundD4h{};
					bool bFoundD8h{};
				}StudioCalcAttachments_SearchContext;

				StudioCalcAttachments_SearchContext ctx = { DllInfo, RealDllInfo };

				ctx.base = (void*)GetVFunctionFromVFTable(vftable, i, DllInfo, RealDllInfo, DllInfo);
				ctx.vftable_index = i;
				ctx.bFoundD4h = false;
				ctx.bFoundD8h = false;

				ctx.max_insts = 1000;
				ctx.max_depth = 16;
				ctx.walks.emplace_back(ctx.base, 0x1000, 0);

				while (ctx.walks.size())
				{
					auto walk = ctx.walks[ctx.walks.size() - 1];
					ctx.walks.pop_back();

					g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx = (StudioCalcAttachments_SearchContext*)context;

						if (gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index)
							return TRUE;

						if (ctx->code.size() > ctx->max_insts)
							return TRUE;

						if (ctx->code.find(address) != ctx->code.end())
							return TRUE;

						ctx->code.emplace(address);

						if (pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_IMM &&
							(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.ImageBase &&
							(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize)
						{
							const char* pPushedString = (const char*)pinst->detail->x86.operands[0].imm;

							if (0 == memcmp(pPushedString, "Too many attachments on %s\n", sizeof("Too many attachments on %s\n") - 1))
							{
								gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = ctx->vftable_index;
							}
						}

						if (address < (PUCHAR)ctx->base + 0x60)
						{
							if (!ctx->bFoundD4h)
							{
								if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_CMP) &&
									pinst->detail->x86.op_count == 2 &&
									pinst->detail->x86.operands[0].type == X86_OP_MEM &&
									pinst->detail->x86.operands[0].mem.base != 0 &&
									pinst->detail->x86.operands[0].mem.disp == 0xD4)
								{
									ctx->bFoundD4h = true;
								}
								else if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_CMP) &&
									pinst->detail->x86.op_count == 2 &&
									pinst->detail->x86.operands[1].type == X86_OP_MEM &&
									pinst->detail->x86.operands[1].mem.base != 0 &&
									pinst->detail->x86.operands[1].mem.disp == 0xD4)
								{
									ctx->bFoundD4h = true;
								}
							}
							if (!ctx->bFoundD8h)
							{
								if (pinst->id == X86_INS_MOV &&
									pinst->detail->x86.op_count == 2 &&
									pinst->detail->x86.operands[0].type == X86_OP_REG &&
									pinst->detail->x86.operands[1].type == X86_OP_MEM &&
									pinst->detail->x86.operands[1].mem.base != 0 &&
									pinst->detail->x86.operands[1].mem.disp == 0xD8)
								{
									ctx->bFoundD8h = true;
								}
							}

							if (ctx->bFoundD4h && ctx->bFoundD8h)
							{
								gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = ctx->vftable_index;
							}
						}

						if (gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index)
							return TRUE;

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

			Sig_FuncNotFound(GameStudioRenderer_StudioCalcAttachments_vftable_index);
		}

		if (g_bIsCounterStrike)
		{
			PVOID GameStudioRenderer_StudioDrawPlayerThunk = ConvertDllInfoSpace(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer, RealDllInfo, DllInfo);

			if (!GameStudioRenderer_StudioDrawPlayerThunk)
			{
				Sig_NotFound(GameStudioRenderer_StudioDrawPlayerThunk);
			}

			/*
				Search for GameStudioRenderer::_StudioDrawPlayer, from GameStudioRenderer::StudioDrawPlayer
			*/
			g_pMetaHookAPI->DisasmRanges(GameStudioRenderer_StudioDrawPlayerThunk, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				if (pinst->id == X86_INS_CALL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.disp >= 0x60 &&
					pinst->detail->x86.operands[0].mem.disp <= 0x70)
				{
					gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
				}

				if (gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index)
					return TRUE;

				return FALSE;

			}, 0, NULL);

			if (gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index == 0)
				gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index = 100 / 4;
		}

		if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index)
			gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))
			GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index, DllInfo, RealDllInfo, RealDllInfo);


		if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index)
			gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))
			GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index, DllInfo, RealDllInfo, RealDllInfo);

		if (gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index)
			gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer))
			GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index, DllInfo, RealDllInfo, RealDllInfo);

		Sig_FuncNotFound(GameStudioRenderer_StudioDrawModel);
		Sig_FuncNotFound(GameStudioRenderer_StudioDrawPlayer);

		{
			typedef struct GameStudioRenderer_StudioDrawPlayer_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
				PVOID base{};
				size_t max_insts{};
				int max_depth{};
				std::set<PVOID> code;
				std::set<PVOID> branches;
				std::vector<walk_context_t> walks;
				int StudioSetRemapColors_instcount{};
			}GameStudioRenderer_StudioDrawPlayer_SearchContext;

			GameStudioRenderer_StudioDrawPlayer_SearchContext ctx = { DllInfo, RealDllInfo };

			ctx.base = ConvertDllInfoSpace(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer, RealDllInfo, DllInfo);

			if (gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer)
			{
				ctx.base = ConvertDllInfoSpace(gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer, RealDllInfo, DllInfo);
			}

			if (!ctx.base)
			{
				Sig_NotFound(GameStudioRenderer_StudioDrawPlayer);
			}

			ctx.max_insts = 1000;
			ctx.max_depth = 16;
			ctx.walks.emplace_back(ctx.base, 0x1000, 0);

			while (ctx.walks.size())
			{
				auto walk = ctx.walks[ctx.walks.size() - 1];
				ctx.walks.pop_back();

				g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (GameStudioRenderer_StudioDrawPlayer_SearchContext*)context;

					if (gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index)
						return TRUE;

					if (ctx->code.size() > ctx->max_insts)
						return TRUE;

					if (ctx->code.find(address) != ctx->code.end())
						return TRUE;

					ctx->code.emplace(address);

					if (pinst->id == X86_INS_CALL &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.disp >= (ULONG_PTR)ctx->DllInfo.ImageBase &&
						pinst->detail->x86.operands[0].mem.disp < (ULONG_PTR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize)
					{
						PVOID calltarget_pfn = (PVOID)pinst->detail->x86.operands[0].mem.disp;
						calltarget_pfn = ConvertDllInfoSpace(calltarget_pfn, ctx->DllInfo, ctx->RealDllInfo);

						if (calltarget_pfn && *(PVOID *)calltarget_pfn == IEngineStudio.StudioSetRemapColors)
						{
							ctx->StudioSetRemapColors_instcount = instCount;
						}
					}

					if (ctx->StudioSetRemapColors_instcount != 0 &&
						instCount > ctx->StudioSetRemapColors_instcount &&
						instCount < ctx->StudioSetRemapColors_instcount + 6 &&
						pinst->id == X86_INS_CALL &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						(pinst->detail->x86.operands[0].mem.base == X86_REG_EAX ||
							pinst->detail->x86.operands[0].mem.base == X86_REG_EBX ||
							pinst->detail->x86.operands[0].mem.base == X86_REG_ECX ||
							pinst->detail->x86.operands[0].mem.base == X86_REG_EDX ||
							pinst->detail->x86.operands[0].mem.base == X86_REG_ESI ||
							pinst->detail->x86.operands[0].mem.base == X86_REG_EDI) &&
						pinst->detail->x86.operands[0].mem.disp > 0x30 &&
						pinst->detail->x86.operands[0].mem.disp < 0x80)
					{
						gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
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

			gPrivateFuncs.GameStudioRenderer_StudioRenderModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderModel))
				GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index, DllInfo, RealDllInfo, RealDllInfo);

			Sig_FuncNotFound(GameStudioRenderer_StudioRenderModel);

		}

		{
			auto GameStudioRenderer_StudioRenderModel = ConvertDllInfoSpace(gPrivateFuncs.GameStudioRenderer_StudioRenderModel, RealDllInfo, DllInfo);

			g_pMetaHookAPI->DisasmRanges((void*)GameStudioRenderer_StudioRenderModel, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;

				if (pinst->id == X86_INS_CALL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					pinst->detail->x86.operands[0].mem.disp > gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index * 4 &&
					pinst->detail->x86.operands[0].mem.disp <= gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index * 4 + 0x20)
				{
					gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
				}

				if (gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, NULL);

			if (!gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index)
				gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index + 1;

			gPrivateFuncs.GameStudioRenderer_StudioRenderFinal = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderFinal))
				GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index, DllInfo, RealDllInfo, RealDllInfo);

			Sig_FuncNotFound(GameStudioRenderer_StudioRenderFinal);
		}

		gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index - 1;
		gPrivateFuncs.GameStudioRenderer_StudioSaveBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 1;
		gPrivateFuncs.GameStudioRenderer_StudioMergeBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 2;

		gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))
			GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index, DllInfo, RealDllInfo, RealDllInfo);
		gPrivateFuncs.GameStudioRenderer_StudioSaveBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSaveBones))
			GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioSaveBones_vftable_index, DllInfo, RealDllInfo, RealDllInfo);
		gPrivateFuncs.GameStudioRenderer_StudioMergeBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioMergeBones))
			GetVFunctionFromVFTable(vftable, gPrivateFuncs.GameStudioRenderer_StudioMergeBones_vftable_index, DllInfo, RealDllInfo, RealDllInfo);

		Sig_FuncNotFound(GameStudioRenderer_StudioSetupBones);
		Sig_FuncNotFound(GameStudioRenderer_StudioSaveBones);
		Sig_FuncNotFound(GameStudioRenderer_StudioMergeBones);
	}
}

void ClientStudio_FillAddress_EngineStudioDrawPlayer(struct r_studio_interface_s** ppinterface, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto EngineStudioDrawPlayerThunk = (PUCHAR)(*ppinterface)->StudioDrawPlayer;
	auto EngineStudioDrawModelThunk = (PUCHAR)(*ppinterface)->StudioDrawModel;

	if (EngineStudioDrawPlayerThunk)
	{
		EngineStudioDrawPlayerThunk = (decltype(EngineStudioDrawPlayerThunk))ConvertDllInfoSpace(EngineStudioDrawPlayerThunk, RealDllInfo, DllInfo);
	}
	if (EngineStudioDrawModelThunk)
	{
		EngineStudioDrawModelThunk = (decltype(EngineStudioDrawModelThunk))ConvertDllInfoSpace(EngineStudioDrawModelThunk, RealDllInfo, DllInfo);
	}

	if (EngineStudioDrawPlayerThunk)
	{
		//There is a E9 jmp in Debug build and we need to skip it.
		if (EngineStudioDrawPlayerThunk[0] == 0xE9)
		{
			EngineStudioDrawPlayerThunk = (PUCHAR)GetCallAddress(EngineStudioDrawPlayerThunk);
		}
	}
	if (EngineStudioDrawModelThunk)
	{
		//There is a E9 jmp in Debug build and we need to skip it.
		if (EngineStudioDrawModelThunk[0] == 0xE9)
		{
			EngineStudioDrawModelThunk = (PUCHAR)GetCallAddress(EngineStudioDrawModelThunk);
		}
	}

	if (EngineStudioDrawPlayerThunk && EngineStudioDrawModelThunk)
	{
		gPrivateFuncs.R_StudioDrawPlayer = (decltype(gPrivateFuncs.R_StudioDrawPlayer))ConvertDllInfoSpace(EngineStudioDrawPlayerThunk, DllInfo, RealDllInfo);
		gPrivateFuncs.R_StudioDrawModel = (decltype(gPrivateFuncs.R_StudioDrawModel))ConvertDllInfoSpace(EngineStudioDrawModelThunk, DllInfo, RealDllInfo);

		{
			/*
.text:01D8A906 50                                                  push    eax
.text:01D8A907 E8 F4 5A 00 00                                      call    sub_1D90400
.text:01D8A90C 83 C4 10                                            add     esp, 10h
.text:01D8A90F E8 7C 20 00 00                                      call    sub_1D8C990
.text:01D8A914 E8 97 00 00 00                                      call    sub_1D8A9B0
.text:01D8A919 8B 3D 50 41 F9 03                                   mov     edi, dword_3F94150
			*/
			const char pattern[] = "\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x10\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B";

			auto addr = Search_Pattern(pattern, DllInfo);

			Sig_AddrNotFound(R_StudioRenderModel);

			PVOID R_StudioRenderModel_VA = GetCallAddress((PUCHAR)addr + 9);
			gPrivateFuncs.R_StudioRenderModel = (decltype(gPrivateFuncs.R_StudioRenderModel))ConvertDllInfoSpace(R_StudioRenderModel_VA, DllInfo, RealDllInfo);

			Sig_FuncNotFound(R_StudioRenderModel);

			typedef struct R_StudioRenderModel_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
			}R_StudioRenderModel_SearchContext;

			R_StudioRenderModel_SearchContext ctx = { DllInfo, RealDllInfo };

			g_pMetaHookAPI->DisasmRanges(R_StudioRenderModel_VA, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (R_StudioRenderModel_SearchContext*)context;

				if (address[0] == 0xE8 && instLen == 5)
				{
					gPrivateFuncs.R_StudioRenderFinal = (decltype(gPrivateFuncs.R_StudioRenderFinal))
						ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (gPrivateFuncs.R_StudioRenderFinal)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
				}, 0, &ctx);

			Sig_FuncNotFound(R_StudioRenderFinal);
		}

		{
			const char sigs[] = "Bip01 Spine\0";
			auto Bip01_String = Search_Pattern_Data(sigs, DllInfo);
			if (!Bip01_String)
				Bip01_String = Search_Pattern_Rdata(sigs, DllInfo);

			Sig_VarNotFound(Bip01_String);

			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
			*(DWORD*)(pattern + 1) = (DWORD)Bip01_String;
			auto Bip01_PushString = Search_Pattern(pattern, DllInfo);
			Sig_VarNotFound(Bip01_PushString);

			PVOID R_StudioSetupBones_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bip01_PushString, 0x1000, [](PUCHAR Candidate) {
				//.text : 01D8DD90 83 EC 48                                            sub     esp, 48h
				//.text : 01D8DD93 A1 E8 F0 ED 01                                      mov     eax, ___security_cookie
				//.text : 01D8DD98 33 C4 xor eax, esp
				if (Candidate[0] == 0x83 &&
					Candidate[1] == 0xEC &&
					Candidate[3] == 0xA1 &&
					Candidate[8] == 0x33 &&
					Candidate[9] == 0xC4)
					return TRUE;

				//.text : 01D82A50 55                                                  push    ebp
				//.text : 01D82A51 8B EC                                               mov     ebp, esp
				//.text : 01D82A53 83 EC 48                                            sub     esp, 48h
				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC &&
					Candidate[3] == 0x83 &&
					Candidate[4] == 0xEC)
					return TRUE;

				return FALSE;
				});

			gPrivateFuncs.R_StudioSetupBones = (decltype(gPrivateFuncs.R_StudioSetupBones))ConvertDllInfoSpace(R_StudioSetupBones_VA, DllInfo, RealDllInfo);

			Sig_FuncNotFound(R_StudioSetupBones);
		}

		{
			char pattern[] = "\x83\xB8\x08\x03\x00\x00\x0C";
			auto addr = Search_Pattern_From_Size(EngineStudioDrawModelThunk, 0x250, pattern);
			Sig_AddrNotFound(R_StudioMergeBones);

			typedef struct R_StudioMergeBones_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
			}R_StudioMergeBones_SearchContext;

			R_StudioMergeBones_SearchContext ctx = { DllInfo, RealDllInfo };

			g_pMetaHookAPI->DisasmRanges(addr, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_StudioMergeBones_SearchContext*)context;

				if (address[0] == 0xE8 && instLen == 5)
				{
					if (!gPrivateFuncs.R_StudioMergeBones)
					{
						gPrivateFuncs.R_StudioMergeBones = (decltype(gPrivateFuncs.R_StudioMergeBones))
							ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
					}
					else if (gPrivateFuncs.R_StudioMergeBones && !gPrivateFuncs.R_StudioSaveBones)
					{
						PVOID candidate = ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);

						if (candidate != gPrivateFuncs.R_StudioSetupBones)
						{
							gPrivateFuncs.R_StudioSaveBones = (decltype(gPrivateFuncs.R_StudioSaveBones))candidate;
						}
					}
				}

				if (gPrivateFuncs.R_StudioMergeBones && gPrivateFuncs.R_StudioSaveBones)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
				}, 0, &ctx);

			Sig_FuncNotFound(R_StudioSaveBones);
			Sig_FuncNotFound(R_StudioMergeBones);
		}
	}
}

void ClientStudio_FillAddress(struct r_studio_interface_s** ppinterface)
{
	ClientStudio_FillAddress_StudioDrawPlayer(ppinterface, g_MirrorClientDLLInfo.ImageBase ? g_MirrorClientDLLInfo : g_ClientDLLInfo, g_ClientDLLInfo);
	ClientStudio_FillAddress_StudioDrawModel(ppinterface, g_MirrorClientDLLInfo.ImageBase ? g_MirrorClientDLLInfo : g_ClientDLLInfo, g_ClientDLLInfo);
	ClientStudio_FillAddress_EngineStudioDrawPlayer(ppinterface, g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);

	if (!g_pGameStudioRenderer && !gPrivateFuncs.R_StudioRenderModel)
	{
		Sys_Error("Failed to locate g_pGameStudioRenderer or EngineStudioRenderer!\n");
	}
}

void ClientStudio_InstallHooks()
{
	if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer)
	{
		Install_InlineHook(GameStudioRenderer_StudioDrawPlayer);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioRenderModel)
	{
		Install_InlineHook(GameStudioRenderer_StudioRenderModel);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioRenderFinal)
	{
		Install_InlineHook(GameStudioRenderer_StudioRenderFinal);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioSetupBones)
	{
		Install_InlineHook(GameStudioRenderer_StudioSetupBones);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioSaveBones)
	{
		Install_InlineHook(GameStudioRenderer_StudioSaveBones);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioMergeBones)
	{
		Install_InlineHook(GameStudioRenderer_StudioMergeBones);
	}

	if (gPrivateFuncs.R_StudioDrawPlayer)
	{
		Install_InlineHook(R_StudioDrawPlayer);
	}
	if (gPrivateFuncs.R_StudioRenderModel)
	{
		Install_InlineHook(R_StudioRenderModel);
	}
	if (gPrivateFuncs.R_StudioRenderFinal)
	{
		Install_InlineHook(R_StudioRenderFinal);
	}
	if (gPrivateFuncs.R_StudioSetupBones)
	{
		Install_InlineHook(R_StudioSetupBones);
	}
	if (gPrivateFuncs.R_StudioSaveBones)
	{
		Install_InlineHook(R_StudioSaveBones);
	}
	if (gPrivateFuncs.R_StudioMergeBones)
	{
		Install_InlineHook(R_StudioMergeBones);
	}
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	gPrivateFuncs.studioapi_GL_SetRenderMode = pstudio->GL_SetRenderMode;
	gPrivateFuncs.studioapi_SetupRenderer = pstudio->SetupRenderer;
	gPrivateFuncs.studioapi_RestoreRenderer = pstudio->RestoreRenderer;
	gPrivateFuncs.studioapi_StudioDynamicLight = pstudio->StudioDynamicLight;
	gPrivateFuncs.studioapi_StudioCheckBBox = pstudio->StudioCheckBBox;

	EngineStudio_FillAddress(pstudio, g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);
	EngineStudio_InstalHooks();

	pbonetransform = (decltype(pbonetransform))pstudio->StudioGetBoneTransform();
	plighttransform = (decltype(plighttransform))pstudio->StudioGetLightTransform();
	rotationmatrix = (decltype(rotationmatrix))pstudio->StudioGetRotationMatrix();

	pstudio->GetModelCounters(&r_smodels_total, &r_amodels_drawn);

	cl_viewent = gEngfuncs.GetViewModel();

	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	cl_sprite_white = IEngineStudio.Mod_ForName("sprites/white.spr", 1);
	cl_sprite_shell = IEngineStudio.Mod_ForName("sprites/shellchrome.spr", 1);

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	ClientStudio_FillAddress(ppinterface);
	ClientStudio_InstallHooks();

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		spec_pip = gEngfuncs.pfnGetCvarPointer("spec_pip_internal");

		if(!spec_pip)
			spec_pip = gEngfuncs.pfnGetCvarPointer("spec_pip");
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "aomdc"))
	{
		g_bIsAoMDC = true;
	}

	return result;
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	return gExportfuncs.HUD_AddEntity(type, ent, model);
}

void HUD_PlayerMoveInit(struct playermove_s* ppmove)
{
	gExportfuncs.HUD_PlayerMoveInit(ppmove);

	if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
	{
		pmove_10152 = (decltype(pmove_10152))ppmove;
	}
	else
	{
		pmove = ppmove;
	}
}

void HUD_Frame(double frametime)
{
	R_GameFrameStart();

	gExportfuncs.HUD_Frame(frametime);

	float time = gEngfuncs.GetAbsoluteTime();

	GameThreadTaskScheduler()->RunTasks(time, 0);
}

void HUD_CreateEntities(void)
{
	R_EmitFlashlights();
	R_CreateLowerBodyModel();

	gExportfuncs.HUD_CreateEntities();

	R_AllocateEntityComponentsForVisEdicts();
}

//Client DLL Shutting down...

void HUD_Shutdown(void)
{
	gExportfuncs.HUD_Shutdown();

	R_SaveProgramStates_f();

	ClientStudio_UninstallHooks();
	EngineStudio_UninstallHooks();

	R_Shutdown();

	GL_Shutdown();

	UtilThreadTask_Shutdown();
}

void HUD_OnClientDisconnect(void)
{
	//The engine have done Mod_Clear before...
	
	//TODO: free bsp VBO?
	//R_FreeUnreferencedStudioRenderData();
}