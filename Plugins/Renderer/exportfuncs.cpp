#include <metahook.h>
#include <capstone.h>
#include "exportfuncs.h"
#include "gl_local.h"
#include "parsemsg.h"
#include "qgl.h"

//Error when can't find sig

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;
void *g_pGameStudioRenderer = NULL;

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;

//extern int g_LastPortalTextureId;

hook_t *g_phook_studioapi_RestoreRenderer = NULL;
hook_t *g_phook_studioapi_StudioDynamicLight = NULL;
hook_t *g_phook_studioapi_StudioCheckBBox = NULL;
hook_t *g_phook_CL_FxBlend = NULL;

hook_t *g_phook_ClientPortalManager_ResetAll = NULL;
hook_t *g_phook_ClientPortalManager_DrawPortalSurface = NULL;
hook_t *g_phook_ClientPortalManager_EnableClipPlane = NULL;

hook_t *g_phook_GameStudioRenderer_StudioSetupBones = NULL;
hook_t *g_phook_GameStudioRenderer_StudioMergeBones = NULL;
hook_t *g_phook_GameStudioRenderer_StudioRenderModel = NULL;
hook_t *g_phook_GameStudioRenderer_StudioRenderFinal = NULL;
hook_t *g_phook_R_StudioSetupBones = NULL;
hook_t *g_phook_R_StudioRenderModel = NULL;
hook_t *g_phook_R_StudioRenderFinal = NULL;

void R_UninstallHooksForClientDLL(void)
{
	Uninstall_Hook(studioapi_RestoreRenderer);
	Uninstall_Hook(studioapi_StudioDynamicLight);
	Uninstall_Hook(studioapi_StudioCheckBBox);
	Uninstall_Hook(CL_FxBlend);

	//SCClient

	Uninstall_Hook(ClientPortalManager_ResetAll);
	Uninstall_Hook(ClientPortalManager_DrawPortalSurface);
	Uninstall_Hook(ClientPortalManager_EnableClipPlane);

	Uninstall_Hook(GameStudioRenderer_StudioSetupBones);
	Uninstall_Hook(GameStudioRenderer_StudioMergeBones);
	Uninstall_Hook(GameStudioRenderer_StudioRenderModel);
	Uninstall_Hook(GameStudioRenderer_StudioRenderFinal);
	Uninstall_Hook(R_StudioRenderModel);
	Uninstall_Hook(R_StudioRenderFinal);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	R_Init();

	gEngfuncs.pfnAddCommand("r_version", R_Version_f);
	gEngfuncs.pfnAddCommand("r_reload", R_Reload_f);

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
	gExportfuncs.V_CalcRefdef(pparams);

	memcpy(&r_params, pparams, sizeof(struct ref_params_s));
}

void HUD_DrawTransparentTriangles(void)
{
	//gExportfuncs.HUD_DrawTransparentTriangles();
}

int HUD_Redraw(float time, int intermission)
{
	//TODO
	if(r_water_debug && r_water_debug->value > 0)
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glColor4f(1, 1, 1, 1);

		glEnable(GL_TEXTURE_2D);
		switch ((int)r_water_debug->value)
		{
		case 1:
			if (g_WaterReflectCaches[0].reflectmap)
				R_DrawHUDQuad_Texture(g_WaterReflectCaches[0].reflectmap, glwidth / 2, glheight / 2);
			break;
		case 2:
			if (g_WaterReflectCaches[0].refractmap)
				R_DrawHUDQuad_Texture(g_WaterReflectCaches[0].refractmap, glwidth / 2, glheight / 2);
			break;
		case 3:
			if (g_WaterReflectCaches[1].reflectmap)
				R_DrawHUDQuad_Texture(g_WaterReflectCaches[1].reflectmap, glwidth / 2, glheight / 2);
			break;
		case 4:
			if (g_WaterReflectCaches[1].refractmap)
				R_DrawHUDQuad_Texture(g_WaterReflectCaches[1].refractmap, glwidth / 2, glheight / 2);
			break;
		//case 3:
		//	R_DrawHUDQuad_Texture(g_LastPortalTextureId, glwidth / 2, glheight / 2);
		//	break;
		default:
			break;
		}
	}
	else if(r_shadow_debug && r_shadow_debug->value && current_shadow_texture && current_shadow_texture->depth)
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glColor4f(1,1,1,1);

		glEnable(GL_TEXTURE_2D);

		hud_debug_program_t prog = { 0 };
		R_UseHudDebugProgram(HUD_DEBUG_SHADOW, &prog);

		GL_Bind(current_shadow_texture->depth);

		glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex3f(0,0,0);
		glTexCoord2f(1,1);
		glVertex3f(glwidth/2,0,0);
		glTexCoord2f(1,0);
		glVertex3f(glwidth/2,glheight/2,0);
		glTexCoord2f(0,0);
		glVertex3f(0,glheight/2,0);
		glEnd();

		glEnable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		GL_UseProgram(0);
	}
	else if(r_light_debug && r_light_debug->value)
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glColor4f(1,1,1,1);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_2D_ARRAY);

		hud_debug_program_t prog = {0};
		R_UseHudDebugProgram(HUD_DEBUG_TEXARRAY, &prog);

		glBindTexture(GL_TEXTURE_2D_ARRAY, s_GBufferFBO.s_hBackBufferTex);

		if (prog.layer != -1)
			glUniform1f(prog.layer, r_light_debug->value - 1);

		glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex3f(0,0,0);
		glTexCoord2f(1,1);
		glVertex3f(glwidth/2,0,0);
		glTexCoord2f(1,0);
		glVertex3f(glwidth/2,glheight/2,0);
		glTexCoord2f(0,0);
		glVertex3f(0,glheight/2,0);
		glEnd();

		glEnable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D_ARRAY);

		glEnable(GL_ALPHA_TEST);

		GL_UseProgram(0);
	}
	else if(r_hdr_debug && r_hdr_debug->value)
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glColor4f(1,1,1,1);

		glEnable(GL_TEXTURE_2D);
		FBO_Container_t *pFBO = NULL;
		switch((int)r_hdr_debug->value)
		{
		case 1:
			pFBO = &s_DownSampleFBO[0];break;
		case 2:
			pFBO = &s_DownSampleFBO[1];break;
		case 3:
			pFBO = &s_LuminFBO[0]; break;
		case 4:
			pFBO = &s_LuminFBO[1]; break;
		case 5:
			pFBO = &s_LuminFBO[2]; break;
		case 6:
			pFBO = &s_Lumin1x1FBO[0]; break;
		case 7:
			pFBO = &s_Lumin1x1FBO[1]; break;
		case 8:
			pFBO = &s_Lumin1x1FBO[2]; break;
		case 9:
			pFBO = &s_BrightPassFBO;break;
		case 10:
			pFBO = &s_BlurPassFBO[0][0];break;
		case 11:
			pFBO = &s_BlurPassFBO[0][1];break;
		case 12:
			pFBO = &s_BlurPassFBO[1][0];break;
		case 13:
			pFBO = &s_BlurPassFBO[1][1];break;
		case 14:
			pFBO = &s_BlurPassFBO[2][0];break;
		case 15:
			pFBO = &s_BlurPassFBO[2][1];break;
		case 16:
			pFBO = &s_BrightAccumFBO;break;
		case 17:
			pFBO = &s_ToneMapFBO;break;
		case 18:
			pFBO = &s_BackBufferFBO; break;
		default:
			break;
		}

		if(pFBO)
		{
			glBindTexture(GL_TEXTURE_2D, pFBO->s_hBackBufferTex);
			glBegin(GL_QUADS);
			glTexCoord2f(0,1);
			glVertex3f(0,0,0);
			glTexCoord2f(1,1);
			glVertex3f(glwidth/2, 0,0);
			glTexCoord2f(1,0);
			glVertex3f(glwidth/2, glheight/2,0);
			glTexCoord2f(0,0);
			glVertex3f(0, glheight/2,0);
			glEnd();
		}
	}
	else if (r_ssao_debug && r_ssao_debug->value)
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glColor4f(1, 1, 1, 1);

		glEnable(GL_TEXTURE_2D);
		int texId = 0;
		switch ((int)r_ssao_debug->value)
		{	
		case 1:
			//GL_UseProgram(drawdepth.program);
			texId = s_BackBufferFBO.s_hBackBufferDepthTex; break;
		case 2:
			texId = s_DepthLinearFBO.s_hBackBufferTex; break;
		case 3:
			texId = s_HBAOCalcFBO.s_hBackBufferTex; break;
		case 4:
			texId = s_HBAOCalcFBO.s_hBackBufferTex2; break;
		default:
			break;
		}

		if (texId)
		{
			glBindTexture(GL_TEXTURE_2D, texId);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 1);
			glVertex3f(0, 0, 0);
			glTexCoord2f(1, 1);
			glVertex3f(glwidth / 2, 0, 0);
			glTexCoord2f(1, 0);
			glVertex3f(glwidth / 2, glheight / 2, 0);
			glTexCoord2f(0, 0);
			glVertex3f(0, glheight / 2, 0);
			glEnd();

			GL_UseProgram(0);
		}
	}
	return gExportfuncs.HUD_Redraw(time, intermission);
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	//Save StudioAPI Funcs
	gRefFuncs.studioapi_RestoreRenderer = pstudio->RestoreRenderer;
	gRefFuncs.studioapi_StudioDynamicLight = pstudio->StudioDynamicLight;

	//Vars in Engine Studio API

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->GetCurrentEntity, 0x10, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				currententity = (decltype(currententity))pinst->detail->x86.operands[1].mem.disp;
			}

			if (currententity)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(currententity);
	}

	if (1)
	{
		typedef struct
		{
			DWORD candidate[10];
			int candidate_count;
		}GetTimes_ctx;

		GetTimes_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(pstudio->GetTimes, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (GetTimes_ctx *)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D87E06 8B 0D EC 97 BC 02                                   mov     ecx, r_framecount  
				if (ctx->candidate_count < 10)
				{
					ctx->candidate[ctx->candidate_count] = (DWORD)pinst->detail->x86.operands[1].mem.disp;
					ctx->candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D87E06 8B 0D EC 97 BC 02                                   mov     ecx, r_framecount  
			
				if (ctx->candidate_count < 10)
				{
					ctx->candidate[ctx->candidate_count] = (DWORD)pinst->detail->x86.operands[1].imm;
					ctx->candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D87E06 8B 0D EC 97 BC 02                                   mov     ecx, r_framecount  
				
				if (!cl_time)
					cl_time = (decltype(cl_time))pinst->detail->x86.operands[0].mem.disp;
				else if (!cl_oldtime)
					cl_oldtime = (decltype(cl_oldtime))pinst->detail->x86.operands[0].mem.disp;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		if (ctx.candidate_count >= 1)
		{
			r_framecount = (decltype(r_framecount))ctx.candidate[0];
		}
		if (ctx.candidate_count == 5)
		{
			cl_time = (decltype(cl_time))ctx.candidate[1];
			cl_oldtime = (decltype(cl_time))ctx.candidate[3];
		}

		Sig_VarNotFound(r_framecount);
		Sig_VarNotFound(cl_time);
		Sig_VarNotFound(cl_oldtime);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->SetRenderModel, 0x10, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG )
			{
				r_model = (decltype(r_model))pinst->detail->x86.operands[0].mem.disp;
			}

			if (r_model)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(r_model);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->StudioSetHeader, 0x10, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				pstudiohdr = (decltype(pstudiohdr))pinst->detail->x86.operands[0].mem.disp;
			}

			if (pstudiohdr)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(pstudiohdr);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->SetForceFaceFlags, 0x10, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				g_ForcedFaceFlags = (decltype(g_ForcedFaceFlags))pinst->detail->x86.operands[0].mem.disp;
			}

			if (g_ForcedFaceFlags)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(g_ForcedFaceFlags);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->StudioSetRenderamt, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (address[0] == 0xE8 && instLen == 5)
			{
				gRefFuncs.CL_FxBlend = (decltype(gRefFuncs.CL_FxBlend))pinst->detail->x86.operands[0].imm;
			}
			else if (pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0)
			{
				r_blend = (decltype(r_blend))pinst->detail->x86.operands[0].mem.disp;
			}

			if (gRefFuncs.CL_FxBlend && r_blend)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(r_blend);
		Sig_FuncNotFound(CL_FxBlend);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->SetupRenderer, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (address[0] == 0xC7 && address[1] == 0x05 && instLen == 10)//C7 05 C0 7D 73 02 98 14 36 02 mov     pauxverts, offset auxverts
			{
				if (!pauxverts)
				{
					pauxverts = *(decltype(pauxverts)*)(address + 2);
					auxverts = *(decltype(auxverts)*)(address + 6);
				}
				else if (!pvlightvalues)
				{
					pvlightvalues = *(decltype(pvlightvalues)*)(address + 2);
					lightvalues = *(decltype(lightvalues)*)(address + 6);
				}
			}

			if (pvlightvalues && lightvalues)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(pauxverts);
		Sig_VarNotFound(pvlightvalues);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->StudioSetupModel, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				pinst->detail->x86.operands[0].mem.disp == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize  )
			{//.text:01D87E55 C7 01 B8 94 37 02                                   mov     dword ptr [ecx], offset pbodypart
				if (!pbodypart)
				{
					pbodypart = (decltype(pbodypart))pinst->detail->x86.operands[1].imm;
				}
				else if (!psubmodel)
				{
					psubmodel = (decltype(psubmodel))pinst->detail->x86.operands[1].imm;
				}
			}

			if (pbodypart && psubmodel)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(pbodypart);
		Sig_VarNotFound(psubmodel);
	}

	if(1)
	{
		typedef struct
		{
			DWORD r_origin_candidate;
			DWORD g_ChromeOrigin_candidate;
		}SetChromeOrigin_ctx;

		SetChromeOrigin_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(pstudio->SetChromeOrigin, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (SetChromeOrigin_ctx *)context;

			if (address[0] == 0xD9 && address[1] == 0x05 && instLen == 6)//D9 05 00 40 F5 03 fld     r_origin
			{
				DWORD imm = *(DWORD *)(address + 2);

				if (!ctx->r_origin_candidate || imm < ctx->r_origin_candidate)
					ctx->r_origin_candidate = imm;
			}
			else if (address[0] == 0xD9 && address[1] == 0x1D && instLen == 6)//D9 1D A0 39 DB 08 fstp    g_ChromeOrigin
			{
				DWORD imm = *(DWORD *)(address + 2);

				if (!ctx->g_ChromeOrigin_candidate || imm < ctx->g_ChromeOrigin_candidate)
					ctx->g_ChromeOrigin_candidate = imm;
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//A1 F0 98 BC 02 mov     eax, r_origin
				DWORD imm = pinst->detail->x86.operands[1].mem.disp;

				if (!ctx->r_origin_candidate || imm < ctx->r_origin_candidate)
					ctx->r_origin_candidate = imm;
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//A3 40 88 35 02 mov     g_ChromeOrigin, eax
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				if (!ctx->g_ChromeOrigin_candidate || imm < ctx->g_ChromeOrigin_candidate)
					ctx->g_ChromeOrigin_candidate = imm;
			}
			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		if (ctx.r_origin_candidate)
			r_origin = (decltype(r_origin))ctx.r_origin_candidate;

		if (ctx.g_ChromeOrigin_candidate)
			g_ChromeOrigin = (decltype(g_ChromeOrigin))ctx.g_ChromeOrigin_candidate;

		Sig_VarNotFound(r_origin);
		Sig_VarNotFound(g_ChromeOrigin);
	}

	if (1)
	{
		typedef struct
		{
			int and_FF00_start;
			DWORD candidate[10];
			int candidate_count;
		}StudioSetupLighting_ctx;

		StudioSetupLighting_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(pstudio->StudioSetupLighting, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (StudioSetupLighting_ctx *)context;
			
			if (pinst->id == X86_INS_AND &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0xFF00 )
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
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG )
			{//.text:01D84A49 89 0D 04 AE 75 02                                   mov     r_colormix+4, ecx
				if (ctx->candidate_count < 10)
				{
					ctx->candidate[ctx->candidate_count] = (DWORD)pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
				}
			}
			else if (ctx->and_FF00_start &&
				pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D8F6AD D9 1D F0 EA 51 08                                   fstp    r_colormix

				if (ctx->candidate_count < 10)
				{
					ctx->candidate[ctx->candidate_count] = (DWORD)pinst->detail->x86.operands[0].mem.disp;
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
			std::qsort(ctx.candidate, ctx.candidate_count, sizeof(int), [](const void*a, const void*b) {
				return (int)(*(LONG_PTR *)a - *(LONG_PTR *)b);
			});

			//other, other, other, r_colormix[0], r_colormix[1], r_colormix[2]
			if (ctx.candidate[ctx.candidate_count - 3] + 4 == ctx.candidate[ctx.candidate_count - 2] &&
				ctx.candidate[ctx.candidate_count - 2] + 4 == ctx.candidate[ctx.candidate_count - 1])
			{
				r_colormix = (decltype(r_colormix))ctx.candidate[ctx.candidate_count - 3];
			}
			//r_colormix[0], r_colormix[1], r_colormix[2], other, other, other
			else if (ctx.candidate[0] + 4 == ctx.candidate[1] &&
				ctx.candidate[1] + 4 == ctx.candidate[2])
			{
				r_colormix = (decltype(r_colormix))ctx.candidate[0];
			}
		}

		Sig_VarNotFound(r_colormix);
	}

	pbonetransform = (decltype(pbonetransform))pstudio->StudioGetBoneTransform();
	plighttransform = (decltype(plighttransform))pstudio->StudioGetLightTransform();
	rotationmatrix = (decltype(rotationmatrix))pstudio->StudioGetRotationMatrix();
	pstudio->GetModelCounters(&r_smodels_total, &r_amodels_drawn);

	cl_viewent = gEngfuncs.GetViewModel();

	//Save Studio API
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	//InlineHook StudioAPI
	Install_InlineHook(studioapi_RestoreRenderer);
	Install_InlineHook(studioapi_StudioDynamicLight);
	Install_InlineHook(studioapi_StudioCheckBBox);
	Install_InlineHook(CL_FxBlend);

	cl_sprite_white = IEngineStudio.Mod_ForName("sprites/white.spr", 1);

	cl_shellchrome = IEngineStudio.Mod_ForName("sprites/shellchrome.spr", 1);

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	//Fix SvClient Portal Rendering Confliction
	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		if (1)
		{
#define SCCLIENT_CLIENTPORTALMANAGER_RESETALL_SIG "\xC7\x45\x2A\xFF\xFF\xFF\xFF\xA3\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B\x0D"
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SCCLIENT_CLIENTPORTALMANAGER_RESETALL_SIG, sizeof(SCCLIENT_CLIENTPORTALMANAGER_RESETALL_SIG) - 1);
			
			Sig_AddrNotFound(ClientPortalManager_ResetAll);

			gRefFuncs.ClientPortalManager_ResetAll = (decltype(gRefFuncs.ClientPortalManager_ResetAll))GetCallAddress(addr + 12);
		}
		if (1)
		{
#define SCCLIENT_CLIENTPORTALMANAGER_GETORIGINALSURFACETEXTURE_SIG "\x8B\x2A\x24\x04\x83\xEC\x08\x2A\x2A\x2A\x2A\xC5\x9D\x1C\x81\x2A\x2A\x2A\x2A\x70\x2A\x2A\x6C"
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SCCLIENT_CLIENTPORTALMANAGER_GETORIGINALSURFACETEXTURE_SIG, sizeof(SCCLIENT_CLIENTPORTALMANAGER_GETORIGINALSURFACETEXTURE_SIG) - 1);

			Sig_AddrNotFound(ClientPortalManager_GetOriginalSurfaceTexture);

			gRefFuncs.ClientPortalManager_GetOriginalSurfaceTexture = (decltype(gRefFuncs.ClientPortalManager_GetOriginalSurfaceTexture))addr;
			//Install_InlineHook(ClientPortalManager_GetOriginalSurfaceTexture);
		}

		if (1)
		{
#define SCCLIENT_CLIENTPORTALMANAGER_DRAWPORTALSURFACE_SIG "\x83\xEC\x2A\x2A\x8B\x74\x24\x2A\x2A\x8B\x7C\x24\x2A\x89\x4C\x24\x2A\x83\x2A\x28\x01"
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SCCLIENT_CLIENTPORTALMANAGER_DRAWPORTALSURFACE_SIG, sizeof(SCCLIENT_CLIENTPORTALMANAGER_DRAWPORTALSURFACE_SIG) - 1);

			Sig_AddrNotFound(ClientPortalManager_DrawPortalSurface);

			gRefFuncs.ClientPortalManager_DrawPortalSurface = (decltype(gRefFuncs.ClientPortalManager_DrawPortalSurface))addr;
		}

		if (1)
		{
#define SCCLIENT_CLIENTPORTALMANAGER_ENABLECLIPPLANE_SIG "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x2A\x44\x24\x2A\x2A\x2A\x24\x2A\x2A\x2A\x24\x2A\x2A\x44\x24\x2A\xF3\x0F"
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SCCLIENT_CLIENTPORTALMANAGER_ENABLECLIPPLANE_SIG, sizeof(SCCLIENT_CLIENTPORTALMANAGER_ENABLECLIPPLANE_SIG) - 1);

			Sig_AddrNotFound(ClientPortalManager_EnableClipPlane);

			gRefFuncs.ClientPortalManager_EnableClipPlane = (decltype(gRefFuncs.ClientPortalManager_EnableClipPlane))addr;
		}

		if (1)
		{
#define SCCLIENT_ISRENDERINGPORTALS_SIG "\xFF\x50\x24\xC6\x05\x2A\x2A\x2A\x2A\x01"
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SCCLIENT_ISRENDERINGPORTALS_SIG, sizeof(SCCLIENT_ISRENDERINGPORTALS_SIG) - 1);
			Sig_AddrNotFound(g_bRenderingPortals);
			g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))*(ULONG_PTR *)(addr + 5);
		}		

		Install_InlineHook(ClientPortalManager_ResetAll);
		Install_InlineHook(ClientPortalManager_DrawPortalSurface);
		Install_InlineHook(ClientPortalManager_EnableClipPlane);

		g_bIsSvenCoop = true;
	}

	if ((void *)g_pMetaSave->pExportFuncs->CL_IsThirdPerson > g_dwClientBase && (void *)g_pMetaSave->pExportFuncs->CL_IsThirdPerson < (PUCHAR)g_dwClientBase + g_dwClientSize)
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

		if (ctx.iNumCandidates >= 3 && ctx.Candidates[ctx.iNumCandidates - 1] == ctx.Candidates[ctx.iNumCandidates - 2] + sizeof(int))
		{
			g_iUser1 = (decltype(g_iUser1))ctx.Candidates[ctx.iNumCandidates - 2];
			g_iUser2 = (decltype(g_iUser2))ctx.Candidates[ctx.iNumCandidates - 1];
		}
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		spec_pip = gEngfuncs.pfnGetCvarPointer("spec_pip_internal");
		if(!spec_pip)
			spec_pip = gEngfuncs.pfnGetCvarPointer("spec_pip");
	}

	if ((void *)(*ppinterface)->StudioDrawPlayer > g_dwClientBase && (void *)(*ppinterface)->StudioDrawPlayer < (PUCHAR)g_dwClientBase + g_dwClientSize)
	{
		g_pMetaHookAPI->DisasmRanges((void *)(*ppinterface)->StudioDrawPlayer, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ECX &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwClientBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwClientBase + g_dwClientSize)
			{
				g_pGameStudioRenderer = (decltype(g_pGameStudioRenderer))pinst->detail->x86.operands[1].imm;
			}

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp >= 8 && pinst->detail->x86.operands[0].mem.disp <= 0x200)
			{
				gRefFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
			}

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;

				PVOID *vftable = *(PVOID **)g_pGameStudioRenderer;
				for (int i = 1; i < 4; ++i)
				{
					if (vftable[i] == imm)
					{
						gRefFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = i;
						break;
					}
				}
			}

			if (g_pGameStudioRenderer && gRefFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(g_pGameStudioRenderer);

		if (gRefFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index == 0)
			gRefFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = 3;

		g_pMetaHookAPI->DisasmRanges((void*)(*ppinterface)->StudioDrawModel, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp >= 8 && pinst->detail->x86.operands[0].mem.disp <= 0x200)
			{
				gRefFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
			}

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;

				PVOID *vftable = *(PVOID **)g_pGameStudioRenderer;
				for (int i = 1; i < 4; ++i)
				{
					if (vftable[i] == imm)
					{
						gRefFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = i;
						break;
					}
				}
			}

			if (gRefFuncs.GameStudioRenderer_StudioDrawModel_vftable_index)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		if (gRefFuncs.GameStudioRenderer_StudioDrawModel_vftable_index == 0)
			gRefFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = 2;

		PVOID *vftable = *(PVOID **)g_pGameStudioRenderer;

		for (int i = 4; i < 9; ++i)
		{
			//GameStudioRenderer_StudioCalcAttachments_vftable_index
			typedef struct
			{
				int index;
			}StudioCalcAttachments_Context;

			StudioCalcAttachments_Context ctx;
			ctx.index = i;

			g_pMetaHookAPI->DisasmRanges((void*)vftable[i], 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn*)inst;
					auto ctx = (StudioCalcAttachments_Context *)context;

					if (address[0] == 0x68 &&
						address[5] == 0xFF &&
						address[6] == 0x15)
					{
						auto pPushedString = *(const char **)(address + 1);
						if (0 == memcmp(pPushedString, "Too many attachments on %s\n", sizeof("Too many attachments on %s\n") - 1))
						{
							gRefFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = ctx->index;
						}
					}

					if (gRefFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, &ctx);
		}

		if (gRefFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index == 0)
			gRefFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = 8;

		gRefFuncs.GameStudioRenderer_StudioSetupBones_vftable_index = gRefFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index - 1;
		gRefFuncs.GameStudioRenderer_StudioSaveBones_vftable_index = gRefFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 1;
		gRefFuncs.GameStudioRenderer_StudioMergeBones_vftable_index = gRefFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 2;

		gRefFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gRefFuncs.GameStudioRenderer_StudioDrawModel))vftable[gRefFuncs.GameStudioRenderer_StudioDrawModel_vftable_index];
		gRefFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gRefFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[gRefFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index];
		gRefFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gRefFuncs.GameStudioRenderer_StudioSetupBones))vftable[gRefFuncs.GameStudioRenderer_StudioSetupBones_vftable_index];
		gRefFuncs.GameStudioRenderer_StudioMergeBones = (decltype(gRefFuncs.GameStudioRenderer_StudioMergeBones))vftable[gRefFuncs.GameStudioRenderer_StudioMergeBones_vftable_index];

		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;
			int StudioSetRemapColors_instcount;
		}GameStudioRenderer_StudioDrawPlayer_ctx;

		GameStudioRenderer_StudioDrawPlayer_ctx ctx = { 0 };

		ctx.base = gRefFuncs.GameStudioRenderer_StudioDrawPlayer;
		ctx.max_insts = 1000;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x1000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn *)inst;
				auto ctx = (GameStudioRenderer_StudioDrawPlayer_ctx *)context;

				if (gRefFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index)
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
					pinst->detail->x86.operands[0].mem.disp >= (ULONG_PTR)g_dwClientBase &&
					pinst->detail->x86.operands[0].mem.disp < (ULONG_PTR)g_dwClientBase + g_dwClientSize)
				{
					PVOID pfnCall = *(PVOID *)pinst->detail->x86.operands[0].mem.disp;

					if (pfnCall == IEngineStudio.StudioSetRemapColors)
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
					gRefFuncs.GameStudioRenderer_StudioRenderModel_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
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

		gRefFuncs.GameStudioRenderer_StudioRenderModel = (decltype(gRefFuncs.GameStudioRenderer_StudioRenderModel))vftable[gRefFuncs.GameStudioRenderer_StudioRenderModel_vftable_index];

		g_pMetaHookAPI->DisasmRanges((void*)gRefFuncs.GameStudioRenderer_StudioRenderModel, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp > gRefFuncs.GameStudioRenderer_StudioRenderModel_vftable_index &&
				pinst->detail->x86.operands[0].mem.disp <= gRefFuncs.GameStudioRenderer_StudioRenderModel_vftable_index + 0x20)
			{
				gRefFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
			}

			if (gRefFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		if (!gRefFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index)
			gRefFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index = gRefFuncs.GameStudioRenderer_StudioRenderModel_vftable_index + 1;

		gRefFuncs.GameStudioRenderer_StudioRenderFinal = (decltype(gRefFuncs.GameStudioRenderer_StudioRenderFinal))vftable[gRefFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index];

		Install_InlineHook(GameStudioRenderer_StudioRenderModel);
		Install_InlineHook(GameStudioRenderer_StudioRenderFinal);
		Install_InlineHook(GameStudioRenderer_StudioSetupBones);
		Install_InlineHook(GameStudioRenderer_StudioMergeBones);
	}
	else if ((void *)(*ppinterface)->StudioDrawPlayer > g_dwEngineBase && (void *)(*ppinterface)->StudioDrawPlayer < (PUCHAR)g_dwEngineBase + g_dwEngineSize)
	{
#define R_STUDIORENDERMODEL_SIG "\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x10\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B"
		auto addr = Search_Pattern(R_STUDIORENDERMODEL_SIG);
		Sig_AddrNotFound(R_StudioRenderModel);
		gRefFuncs.R_StudioRenderModel = (decltype(gRefFuncs.R_StudioRenderModel))GetCallAddress((PUCHAR)addr + 9);
		
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_StudioRenderModel, 0x80, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (address[0] == 0xE8)
			{
				gRefFuncs.R_StudioRenderFinal = (decltype(gRefFuncs.R_StudioRenderFinal))pinst->detail->x86.operands[0].imm;
			}

			if (gRefFuncs.R_StudioRenderFinal)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_FuncNotFound(R_StudioRenderFinal);

		Install_InlineHook(R_StudioRenderModel);
		Install_InlineHook(R_StudioRenderFinal);
	}
	else
	{
		g_pMetaHookAPI->SysError("Failed to locate g_pGameStudioRenderer or EngineStudioRenderer!\n");
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_LightStrength, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn *)inst;

			if (!numlight &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[0].type == X86_OP_REG)
			{
				if ((uintptr_t)pinst->detail->x86.operands[1].mem.disp != (uintptr_t)r_smodels_total)
				{
					//.text:01D8BD47 8B 35 B4 39 DB 08                                   mov     esi, numlight
					//.text:01D83D0B A1 98 74 36 02                                      mov     eax, numlight
					numlight = (decltype(numlight))pinst->detail->x86.operands[1].mem.disp;
				}
			}

			if (numlight)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, NULL);

		Sig_VarNotFound(numlight);
	}

	return result;
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	int r = gExportfuncs.HUD_AddEntity(type, ent, model);
	
	if (r &&
		ent->curstate.movetype == MOVETYPE_FOLLOW &&
		ent->curstate.aiment > 0 &&
		ent->curstate.aiment < EngineGetMaxClientEdicts() &&
		ent->model && 
		ent->model->type == mod_studio)
	{
		auto aiment = gEngfuncs.GetEntityByIndex(ent->curstate.aiment);
		auto comp = R_GetEntityComponent(aiment, true);
		if (comp)
		{
			comp->FollowEnts.emplace_back(ent);
		}
		//return 0;
	}

	return r;
}

void HUD_Frame(double time)
{
	gExportfuncs.HUD_Frame(time);
}

//Client DLL Shutting down...

void HUD_Shutdown(void)
{
	gExportfuncs.HUD_Shutdown();

	R_SaveProgramStates_f();

	R_Shutdown();

	GL_Shutdown();

	R_UninstallHooksForClientDLL();
}