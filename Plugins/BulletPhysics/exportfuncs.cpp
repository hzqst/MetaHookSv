#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <capstone.h>
#include <cl_entity.h>
#include <com_model.h>
#include <triangleapi.h>
#include <cvardef.h>
#include <entity_types.h>
#include <command.h>
#include <pm_defs.h>
#include <mathlib.h>

#include "plugins.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "corpse.h"
#include "physics.h"

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

cvar_t *bv_debug = NULL;
cvar_t *bv_simrate = NULL;
cvar_t *bv_scale = NULL;
cvar_t *bv_force_ragdoll_sequence = NULL;

studiohdr_t **pstudiohdr = NULL;
model_t **r_model = NULL;
model_t *r_worldmodel = NULL;
void *g_pGameStudioRenderer = NULL;
int *r_visframecount = NULL;
int *cl_parsecount = NULL;

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

bool IsEntityBarnacle(cl_entity_t* ent);

int GetSequenceActivityType(model_t *mod, entity_state_t* entstate);

void Sys_ErrorEx(const char *fmt, ...)
{
	char msg[4096] = { 0 };

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if (gEngfuncs.pfnClientCmd)
		gEngfuncs.pfnClientCmd("escape\n");

	MessageBox(NULL, msg, "Fatal Error", MB_ICONERROR);
	TerminateProcess((HANDLE)(-1), 0);
}

void RagdollDestroyCallback(int entindex)
{
	gCorpseManager.FreePlayerForBarnacle(entindex);
}

void __fastcall GameStudioRenderer_StudioSetupBones(void *pthis, int)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	if (currententity->curstate.iuser4 == 114514)
	{
		if (gPhysicsManager.SetupBones((*pstudiohdr), currententity->index))
			return;
	}
	else if (currententity->curstate.iuser4 == 1919810)
	{
		if(gPhysicsManager.SetupBones((*pstudiohdr), currententity->curstate.number))
			return;
	}

	gPrivateFuncs.GameStudioRenderer_StudioSetupBones(pthis, 0);

	if (IsEntityBarnacle(currententity))
	{
		auto player = gCorpseManager.FindPlayerForBarnacle(currententity->index);
		if (player)
		{
			gPhysicsManager.MergeBarnacleBones((*pstudiohdr), player->index);
		}
	}
}

int __fastcall GameStudioRenderer_StudioDrawModel(void *pthis, int dummy, int flags)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	if((flags & STUDIO_RENDER) &&
		!currententity->player && 
		currententity->index &&
		currententity->curstate.messagenum == (*cl_parsecount) &&
		currententity->curstate.renderfx != kRenderFxDeadPlayer
		)
	{
		int entindex = currententity->index;
		auto model = currententity->model;

		int iActivityType = GetSequenceActivityType(model, &currententity->curstate);

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);
		if (!ragdoll)
		{
			if (iActivityType)
			{
				auto cfg = gPhysicsManager.LoadRagdollConfig(model);

				if (cfg)
				{
					bool bTransformToRagdoll = false;

					if (!bTransformToRagdoll)
					{
						auto itor = cfg->animcontrol.find(currententity->curstate.sequence);

						if ((itor != cfg->animcontrol.end() && currententity->curstate.frame >= itor->second))
						{
							bTransformToRagdoll = true;
						}
					}

					if (bTransformToRagdoll)
					{
						vec3_t velocity = { 0 };						
						if (iActivityType == 1)
						{
							float frametime = currententity->curstate.animtime - currententity->latched.prevanimtime;
							if (frametime > 0)
							{
								VectorSubtract(currententity->curstate.origin, currententity->latched.prevorigin, velocity);
								velocity[0] /= frametime;
								velocity[1] /= frametime;
								velocity[2] /= frametime;
							}
						}

						cl_entity_t *barnacle = NULL;
						if (iActivityType == 2)
						{
							barnacle = gCorpseManager.FindBarnacleForPlayer(&currententity->curstate);
						}

						gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, 0);

						if (gPhysicsManager.CreateRagdoll(
							cfg,
							entindex,
							(*pstudiohdr),
							iActivityType,
							currententity->origin,
							velocity,
							barnacle,
							false))
						{
							return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
						}
					}
				}
			}
		}
		else
		{
			if (!iActivityType)
			{
				gPhysicsManager.RemoveRagdoll(entindex);			
				return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
			}

			int iuser4 = currententity->curstate.iuser4;
			currententity->curstate.iuser4 = 114514;

			vec3_t saved_origin;
			VectorCopy(currententity->origin, saved_origin);
			gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

			int result = gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);

			VectorCopy(saved_origin, currententity->origin);

			currententity->curstate.iuser4 = iuser4;

			return result;
		}
	}

	return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
}

int __fastcall GameStudioRenderer_StudioDrawPlayer(void *pthis, int dummy, int flags, struct entity_state_s *pplayer)
{
	if (flags & STUDIO_RENDER)
	{
		auto currententity = IEngineStudio.GetCurrentEntity();

		int playerindex = pplayer->number;

		auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

		int iActivityType = GetSequenceActivityType(model, pplayer);

		auto ragdoll = gPhysicsManager.FindRagdoll(playerindex);

		if (!ragdoll)
		{
			if (iActivityType)
			{
				auto cfg = gPhysicsManager.LoadRagdollConfig(model);

				if (cfg)
				{
					bool bTransformToRagdoll = false;

					if (bv_force_ragdoll_sequence->value == currententity->curstate.sequence)
						bTransformToRagdoll = true;

					if (!bTransformToRagdoll)
					{
						auto itor = cfg->animcontrol.find(pplayer->sequence);

						if ((itor != cfg->animcontrol.end() && pplayer->frame >= itor->second))
						{
							bTransformToRagdoll = true;
						}
					}

					if(bTransformToRagdoll)
					{
						vec3_t velocity = { 0 };

						if (iActivityType == 1 && currententity->curstate.usehull != 1)
						{
							float frametime = currententity->curstate.animtime - currententity->latched.prevanimtime;
							if (frametime > 0)
							{
								VectorSubtract(currententity->curstate.origin, currententity->latched.prevorigin, velocity);
								velocity[0] /= frametime;
								velocity[1] /= frametime;
								velocity[2] /= frametime;
							}
						}

						cl_entity_t *barnacle = NULL;

						if (iActivityType == 2)
						{
							barnacle = gCorpseManager.FindBarnacleForPlayer(pplayer);
						}

						gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, 0, pplayer);

						if (gPhysicsManager.CreateRagdoll(
							cfg,
							playerindex,
							(*pstudiohdr),
							iActivityType,
							pplayer->origin,
							velocity,
							barnacle, 
							true))
						{
							return gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
						}
					}
				}
			}
		}
		else
		{
			if (!iActivityType)
			{
				gPhysicsManager.RemoveRagdoll(playerindex);
				return gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
			}

			int number = currententity->curstate.number;
			int iuser4 = currententity->curstate.iuser4;
			currententity->curstate.number = pplayer->number;
			currententity->curstate.iuser4 = 1919810;

			vec3_t saved_origin;
			VectorCopy(currententity->origin, saved_origin);
			gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

			int result = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);

			VectorCopy(saved_origin, currententity->origin);

			currententity->curstate.number = number;
			currententity->curstate.iuser4 = iuser4;

			return result;
		}
	}

	return gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	int result = gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio);

	pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();
	plighttransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();

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
				pinst->detail->x86.operands[1].type == X86_OP_REG)
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

			if (g_pGameStudioRenderer)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(g_pGameStudioRenderer);

		DWORD *vftable = *(DWORD **)g_pGameStudioRenderer;

		gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))vftable[2];
		gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[3];
		gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))vftable[7];

		Install_InlineHook(GameStudioRenderer_StudioSetupBones);
		Install_InlineHook(GameStudioRenderer_StudioDrawPlayer);
		Install_InlineHook(GameStudioRenderer_StudioDrawModel);

		Install_InlineHook(GameStudioRenderer_StudioSetupBones);
	}
	else
	{
		Sig_NotFound(g_pGameStudioRenderer);
	}

	return result;
}

void BV_Reload_f(void)
{
	gPhysicsManager.ReloadConfig();
}

void BV_ThreadPerson_f(void)
{
	auto localplayer = gEngfuncs.GetLocalPlayer();

	if (localplayer && localplayer->player)
	{
		if (GetSequenceActivityType(localplayer->model, &localplayer->curstate) == 2)
		{
			gEngfuncs.Con_Printf("Cannot change to thirdperson when playing barnacle animation.\n");
			return;
		}
	}

	gPrivateFuncs.ThreadPerson_f();
}

void BV_FirstPerson_f(void)
{
	auto localplayer = gEngfuncs.GetLocalPlayer();

	if (localplayer && localplayer->player)
	{
		if (GetSequenceActivityType(localplayer->model, &localplayer->curstate) == 2)
		{
			gEngfuncs.Con_Printf("Cannot change to firstperson when playing barnacle animation.\n");
			return;
		}
	}

	gPrivateFuncs.FirstPerson_f();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	gPhysicsManager.Init();

	bv_debug = gEngfuncs.pfnRegisterVariable("bv_debug", "0", FCVAR_CLIENTDLL);
	bv_simrate = gEngfuncs.pfnRegisterVariable("bv_simrate", "64", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_scale = gEngfuncs.pfnRegisterVariable("bv_scale", "0.1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_force_ragdoll_sequence = gEngfuncs.pfnRegisterVariable("bv_force_ragdoll_sequence", "0", FCVAR_CLIENTDLL);

	gEngfuncs.pfnAddCommand("bv_reload", BV_Reload_f);

	gPrivateFuncs.ThreadPerson_f = Cmd_HookCmd("thirdperson", BV_ThreadPerson_f);
	gPrivateFuncs.FirstPerson_f = Cmd_HookCmd("firstperson", BV_FirstPerson_f);
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	if (type == ET_NORMAL && ent->model)
	{
		if (ent->model->type == modtype_t::mod_brush && ent->curstate.solid == SOLID_BSP)
		{
			gPhysicsManager.CreateBrushModel(ent);
		}

		if (IsEntityBarnacle(ent))
		{
			gPhysicsManager.CreateBarnacle(ent);
			gCorpseManager.AddBarnacle(ent->index, 0);
		}
	}

	return gExportfuncs.HUD_AddEntity(type, ent, model);
}

void HUD_TempEntUpdate(
	double frametime,   // Simulation time
	double client_time, // Absolute time on client
	double cl_gravity,  // True gravity on client
	TEMPENTITY **ppTempEntFree,   // List of freed temporary ents
	TEMPENTITY **ppTempEntActive, // List 
	int(*Callback_AddVisibleEntity)(cl_entity_t *pEntity),
	void(*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp))
{
	auto levelname = gEngfuncs.pfnGetLevelName();
	if (levelname && levelname[0] && gPhysicsManager.HasRagdolls())
	{
		gPhysicsManager.SetGravity(cl_gravity);
		gPhysicsManager.UpdateTempEntity(ppTempEntActive, frametime, client_time);
		gPhysicsManager.StepSimulation(frametime);
	}

	return gExportfuncs.HUD_TempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);
}

void V_CalcRefdef(struct ref_params_s *pparams)
{
	if (gExportfuncs.CL_IsThirdPerson())
	{
		auto local = gEngfuncs.GetLocalPlayer();

		if (local && local->player && GetSequenceActivityType(local->model, &local->curstate))
		{
			gPhysicsManager.SyncPlayerView(local, pparams);
		}
	}

	gExportfuncs.V_CalcRefdef(pparams);
}

void HUD_DrawTransparentTriangles(void)
{
	gExportfuncs.HUD_DrawTransparentTriangles();

	gPhysicsManager.DebugDraw();
}