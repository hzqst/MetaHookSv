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
bool g_bIsHL1MMOD = false;

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
	//Engine Studio global slots resolved from gamedata
	currententity = (decltype(currententity))GamedataResolvePtr(RealDllInfo.ImageBase, "currententity", MH_GAMESYMBOL_KIND_GLOBAL);
	r_model = (decltype(r_model))GamedataResolvePtr(RealDllInfo.ImageBase, "r_model", MH_GAMESYMBOL_KIND_GLOBAL);
	pstudiohdr = (decltype(pstudiohdr))GamedataResolvePtr(RealDllInfo.ImageBase, "pstudiohdr", MH_GAMESYMBOL_KIND_GLOBAL);
	r_origin = (decltype(r_origin))GamedataResolvePtr(RealDllInfo.ImageBase, "r_origin", MH_GAMESYMBOL_KIND_GLOBAL);
	g_ChromeOrigin = (decltype(g_ChromeOrigin))GamedataResolvePtr(RealDllInfo.ImageBase, "g_ChromeOrigin", MH_GAMESYMBOL_KIND_GLOBAL);

	EngineStudio_FillAddress_GetTimes(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_SetForceFaceFlags(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetRemapColors(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetRenderamt(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_SetupRenderer(pstudio, DllInfo, RealDllInfo);
	EngineStudio_FillAddress_StudioSetupModel(pstudio, DllInfo, RealDllInfo);
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

void ClientStudio_FillAddress(struct r_studio_interface_s** ppinterface)
{
	auto clientBase = g_ClientDLLInfo.ImageBase;
	auto engineBase = g_EngineDLLInfo.ImageBase;

	//Client CGameStudioRenderer singleton, vtable and virtual functions resolved from gamedata
	g_pGameStudioRenderer = GamedataResolvePtr(clientBase, "g_pGameStudioRenderer", MH_GAMESYMBOL_KIND_GLOBAL);

	gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioDrawModel", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioDrawPlayer", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

	gPrivateFuncs.GameStudioRenderer_StudioRenderModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderModel))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioRenderModel", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioRenderFinal = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderFinal))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioRenderFinal", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioSetupBones", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioSaveBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSaveBones))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioSaveBones", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioMergeBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioMergeBones))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioMergeBones", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

	//Engine Studio render pipeline resolved from gamedata
	gPrivateFuncs.R_StudioDrawModel = (decltype(gPrivateFuncs.R_StudioDrawModel))
		GamedataResolvePtr(engineBase, "R_StudioDrawModel", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioDrawPlayer = (decltype(gPrivateFuncs.R_StudioDrawPlayer))
		GamedataResolvePtr(engineBase, "R_StudioDrawPlayer", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioRenderModel = (decltype(gPrivateFuncs.R_StudioRenderModel))
		GamedataResolvePtr(engineBase, "R_StudioRenderModel", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioRenderFinal = (decltype(gPrivateFuncs.R_StudioRenderFinal))
		GamedataResolvePtr(engineBase, "R_StudioRenderFinal", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioSetupBones = (decltype(gPrivateFuncs.R_StudioSetupBones))
		GamedataResolvePtr(engineBase, "R_StudioSetupBones", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioMergeBones = (decltype(gPrivateFuncs.R_StudioMergeBones))
		GamedataResolvePtr(engineBase, "R_StudioMergeBones", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioSaveBones = (decltype(gPrivateFuncs.R_StudioSaveBones))
		GamedataResolvePtr(engineBase, "R_StudioSaveBones", MH_GAMESYMBOL_KIND_FUNCTION);
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

	UtilThreadTask_Shutdown();
}

void HUD_OnClientDisconnect(void)
{
	//The engine have done Mod_Clear before...
	
	//TODO: free bsp VBO?
	//R_FreeUnreferencedStudioRenderData();
}
