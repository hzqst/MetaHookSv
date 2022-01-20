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
cvar_t *bv_enable = NULL;
cvar_t *bv_force_ragdoll_sequence = NULL;

studiohdr_t **pstudiohdr = NULL;
model_t **r_model = NULL;
model_t *r_worldmodel = NULL;
void *g_pGameStudioRenderer = NULL;
int *r_visframecount = NULL;
int *cl_parsecount = NULL;
void *mod_known = NULL;
int *mod_numknown = NULL;

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

bool IsEntityGargantua(cl_entity_t* ent);
bool IsEntityBarnacle(cl_entity_t* ent);

int GetSequenceActivityType(model_t *mod, entity_state_t* entstate);

int EngineGetMaxKnownModel(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return 16384;

	return 1024;
}

int EngineGetModelIndex(model_t *mod)
{
	int index = (mod - (model_t *)(mod_known));

	if (index >= 0 && index < *mod_numknown)
		return index;

	return -1;
}

model_t *EngineGetModelByIndex(int index)
{
	auto pmod_known = (model_t *)(mod_known);
	
	if (index >= 0 && index < *mod_numknown)
		return &pmod_known[index];

	return NULL;
}

void RagdollDestroyCallback(int entindex)
{
	gCorpseManager.FreePlayerForBarnacle(entindex);
}

//EngineSetupBones

void R_StudioSetupBones(void)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	if (currententity->curstate.iuser4 == 114514)
	{
		if (gPhysicsManager.SetupBones((*pstudiohdr), currententity->index))
			return;
	}
	else if (currententity->curstate.iuser4 == 1919810)
	{
		if (gPhysicsManager.SetupBones((*pstudiohdr), currententity->curstate.number))
			return;
	}

	gPrivateFuncs.R_StudioSetupBones();

	if (IsEntityBarnacle(currententity))
	{
		auto player = gCorpseManager.FindPlayerForBarnacle(currententity->index);
		if (player)
		{
			gPhysicsManager.MergeBarnacleBones((*pstudiohdr), player->index);
		}
	}

	if (currententity->curstate.iuser4 == 114515)
	{
		if (gPhysicsManager.SetupJiggleBones((*pstudiohdr), currententity->index))
			return;
	}
	else if (currententity->curstate.iuser4 == 1919811)
	{
		if (gPhysicsManager.SetupJiggleBones((*pstudiohdr), currententity->curstate.number))
			return;
	}
}

//ClientSetupBones

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

	if (currententity->curstate.iuser4 == 114515)
	{
		if (gPhysicsManager.SetupJiggleBones((*pstudiohdr), currententity->index))
			return;
	}
	else if (currententity->curstate.iuser4 == 1919811)
	{
		if (gPhysicsManager.SetupJiggleBones((*pstudiohdr), currententity->curstate.number))
			return;
	}
}

//EngineDrawModel

int R_StudioDrawModel(int flags)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	if ((flags & STUDIO_RENDER) &&
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
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.R_StudioDrawModel(0);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, entindex, (*pstudiohdr), iActivityType, false);

				goto has_ragdoll;
			}
		}
		else
		{
			/*if (!iActivityType)
			{
				gPhysicsManager.RemoveRagdoll(entindex);
				return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
			}*/

		has_ragdoll:

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, &currententity->curstate))
			{
				//Monster don't have barnacle animation
				/*if (ragdoll->m_iActivityType == 2)
				{
					cl_entity_t *barnacleEntity = gCorpseManager.FindBarnacleForPlayer(&currententity->curstate);

					gPhysicsManager.ApplyBarnacle(ragdoll, barnacleEntity);
				}*/
			}

			if (ragdoll->m_iActivityType > 0)
			{
				int iuser4 = currententity->curstate.iuser4;
				currententity->curstate.iuser4 = 114514;

				vec3_t saved_origin;
				VectorCopy(currententity->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

				int result = gPrivateFuncs.R_StudioDrawModel(flags);

				VectorCopy(saved_origin, currententity->origin);

				currententity->curstate.iuser4 = iuser4;

				return result;
			}
			else
			{
				int iuser4 = currententity->curstate.iuser4;
				currententity->curstate.iuser4 = 114515;

				int result = gPrivateFuncs.R_StudioDrawModel(flags);

				currententity->curstate.iuser4 = iuser4;

				return result;
			}
		}
	}

	return gPrivateFuncs.R_StudioDrawModel(flags);
}

//ClientDrawModel

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
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, 0);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, entindex, (*pstudiohdr), iActivityType, false);

				goto has_ragdoll;
			}
		}
		else
		{
			/*if (!iActivityType)
			{
				gPhysicsManager.RemoveRagdoll(entindex);
				return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
			}*/

		has_ragdoll:

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, &currententity->curstate))
			{
				//Monster don't have barnacle animation
				/*if (ragdoll->m_iActivityType == 2)
				{
					cl_entity_t *barnacleEntity = gCorpseManager.FindBarnacleForPlayer(&currententity->curstate);

					gPhysicsManager.ApplyBarnacle(ragdoll, barnacleEntity);
				}*/
			}

			if (ragdoll->m_iActivityType > 0)
			{
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
			else
			{
				int iuser4 = currententity->curstate.iuser4;
				currententity->curstate.iuser4 = 114515;

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);

				currententity->curstate.iuser4 = iuser4;

				return result;
			}
		}
	}

	return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
}

//EngineDrawPlayer

int __fastcall R_StudioDrawPlayer(int flags, struct entity_state_s *pplayer)
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
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.R_StudioDrawPlayer(0, pplayer);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, playerindex, (*pstudiohdr), iActivityType, true);

				goto has_ragdoll;
			}
		}
		else
		{
			//model changed ?
			if (ragdoll->m_studiohdr != IEngineStudio.Mod_Extradata(model))
			{
				gPhysicsManager.RemoveRagdoll(playerindex);
				return gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);
			}

		has_ragdoll:

			int oldActivityType = ragdoll->m_iActivityType;

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, pplayer))
			{
				//Transform from whatever to barnacle
				if (ragdoll->m_iActivityType == 2)
				{
					cl_entity_t *barnacleEntity = gCorpseManager.FindBarnacleForPlayer(&currententity->curstate);

					if (barnacleEntity)
					{
						gPhysicsManager.ApplyBarnacle(ragdoll, barnacleEntity);
					}
					else
					{
						cl_entity_t *gargantuaEntity = gCorpseManager.FindGargantuaForPlayer(&currententity->curstate);
						if (gargantuaEntity)
						{
							gPhysicsManager.ApplyGargantua(ragdoll, gargantuaEntity);
						}
					}
				}

				//Transform from Death or barnacle to idle
				else if (oldActivityType > 0 && ragdoll->m_iActivityType == 0)
				{
					gPrivateFuncs.R_StudioDrawPlayer(0, pplayer);

					gPhysicsManager.ResetPose(ragdoll, pplayer);
				}

			}

			//Teleport ?
			else if (oldActivityType == 0 && ragdoll->m_iActivityType == 0 &&
				VectorDistance(currententity->curstate.origin, currententity->latched.prevorigin) > 500)
			{
				gPrivateFuncs.R_StudioDrawPlayer(0, pplayer);

				gPhysicsManager.ResetPose(ragdoll, pplayer);
			}

			if (ragdoll->m_iActivityType > 0)
			{
				int number = currententity->curstate.number;
				int iuser4 = currententity->curstate.iuser4;
				currententity->curstate.number = pplayer->number;
				currententity->curstate.iuser4 = 1919810;

				vec3_t saved_origin;
				VectorCopy(currententity->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

				int result = gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);

				VectorCopy(saved_origin, currententity->origin);

				currententity->curstate.number = number;
				currententity->curstate.iuser4 = iuser4;
				return result;
			}
			else
			{
				int number = currententity->curstate.number;
				int iuser4 = currententity->curstate.iuser4;
				currententity->curstate.number = pplayer->number;
				currententity->curstate.iuser4 = 1919811;

				int result = gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);

				currententity->curstate.number = number;
				currententity->curstate.iuser4 = iuser4;
				return result;
			}
		}
	}

	return gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);
}

//ClientDrawPlayer

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
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, 0, pplayer);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, playerindex, (*pstudiohdr), iActivityType, true);

				goto has_ragdoll;
			}
		}
		else
		{
			//model changed ?
			if (ragdoll->m_studiohdr != IEngineStudio.Mod_Extradata(model))
			{
				gPhysicsManager.RemoveRagdoll(playerindex);
				return gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
			}

		has_ragdoll:

			int oldActivityType = ragdoll->m_iActivityType;

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, pplayer))
			{
				//Transform from whatever to barnacle
				if (ragdoll->m_iActivityType == 2)
				{
					cl_entity_t *barnacleEntity = gCorpseManager.FindBarnacleForPlayer(&currententity->curstate);
					
					if (barnacleEntity)
					{
						gPhysicsManager.ApplyBarnacle(ragdoll, barnacleEntity);
					}
					else
					{
						cl_entity_t *gargantuaEntity = gCorpseManager.FindGargantuaForPlayer(&currententity->curstate);
						if (gargantuaEntity)
						{
							gPhysicsManager.ApplyGargantua(ragdoll, gargantuaEntity);
						}
					}
				}

				//Transform from Death or barnacle to idle
				else if (oldActivityType > 0 && ragdoll->m_iActivityType == 0)
				{
					gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, 0, pplayer);

					gPhysicsManager.ResetPose(ragdoll, pplayer);
				}

			}

			//Teleport ?
			else if (oldActivityType == 0 && ragdoll->m_iActivityType == 0 &&
				VectorDistance(currententity->curstate.origin, currententity->latched.prevorigin) > 500)
			{
				gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, 0, pplayer);

				gPhysicsManager.ResetPose(ragdoll, pplayer);
			}

			if (ragdoll->m_iActivityType > 0)
			{
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
			else
			{
				int number = currententity->curstate.number;
				int iuser4 = currententity->curstate.iuser4;
				currententity->curstate.number = pplayer->number;
				currententity->curstate.iuser4 = 1919811;

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);

				currententity->curstate.number = number;
				currententity->curstate.iuser4 = iuser4;
				return result;
			}
		}
	}

	return gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

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
		g_pMetaHookAPI->DisasmRanges((void *)(*ppinterface)->StudioDrawPlayer, 0x80, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
				pinst->detail->x86.operands[0].type == X86_OP_MEM  &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp >= 8 && pinst->detail->x86.operands[0].mem.disp <= 0x200)
			{
				gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
			}

			if (g_pGameStudioRenderer && gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(g_pGameStudioRenderer);

		//Sig_FuncNotFound(GameStudioRenderer_StudioDrawPlayer_vftable_index);
		if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index == 0)
			gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = 3;

		g_pMetaHookAPI->DisasmRanges((void*)(*ppinterface)->StudioDrawModel, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
				auto pinst = (cs_insn*)inst;

				if (pinst->id == X86_INS_CALL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					pinst->detail->x86.operands[0].mem.disp >= 8 && pinst->detail->x86.operands[0].mem.disp <= 0x200)
				{
					gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = pinst->detail->x86.operands[0].mem.disp / 4;
				}

				if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
		}, 0, NULL);

		//Sig_FuncNotFound(GameStudioRenderer_StudioDrawModel_vftable_index);

		if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index == 0)
			gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = 2;

		DWORD *vftable = *(DWORD **)g_pGameStudioRenderer;

		gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))vftable[7];

		Install_InlineHook(GameStudioRenderer_StudioSetupBones);
		Install_InlineHook(GameStudioRenderer_StudioDrawPlayer);
		Install_InlineHook(GameStudioRenderer_StudioDrawModel);
	}
	else if((void *)(*ppinterface)->StudioDrawPlayer > g_dwEngineBase && (void *)(*ppinterface)->StudioDrawPlayer < (PUCHAR)g_dwEngineBase + g_dwEngineSize)
	{
		const char sigs1[] = "Bip01 Spine\0";
		auto Bip01_String = Search_Pattern_Data(sigs1);
		if (!Bip01_String)
			Bip01_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Bip01_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
		*(DWORD *)(pattern + 1) = (DWORD)Bip01_String;
		auto Bip01_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(Bip01_PushString);

		gPrivateFuncs.R_StudioSetupBones = (decltype(gPrivateFuncs.R_StudioSetupBones))g_pMetaHookAPI->ReverseSearchFunctionBegin(Bip01_PushString, 0x600);
		Sig_FuncNotFound(R_StudioSetupBones);

		gPrivateFuncs.R_StudioDrawModel = (decltype(gPrivateFuncs.R_StudioDrawModel))(*ppinterface)->StudioDrawModel;
		gPrivateFuncs.R_StudioDrawPlayer = (decltype(gPrivateFuncs.R_StudioDrawPlayer))(*ppinterface)->StudioDrawPlayer;

		Install_InlineHook(R_StudioSetupBones);
		Install_InlineHook(R_StudioDrawPlayer);
		Install_InlineHook(R_StudioDrawModel);
	}
	else
	{
		gEngfuncs.Con_Printf("Warning : failed to locate g_pGameStudioRenderer or EngineStudioRenderer!\n");
	}

	return result;
}

void BV_Reload_f(void)
{
	gPhysicsManager.ReloadConfig();
	gPhysicsManager.RemoveAllRagdolls();
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
	bv_scale = gEngfuncs.pfnRegisterVariable("bv_scale", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_enable = gEngfuncs.pfnRegisterVariable("bv_enable", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_force_ragdoll_sequence = gEngfuncs.pfnRegisterVariable("bv_force_ragdoll_sequence", "0", FCVAR_CLIENTDLL);

	gEngfuncs.pfnAddCommand("bv_reload", BV_Reload_f);

	gPrivateFuncs.ThreadPerson_f = g_pMetaHookAPI->HookCmd("thirdperson", BV_ThreadPerson_f);
	gPrivateFuncs.FirstPerson_f = g_pMetaHookAPI->HookCmd("firstperson", BV_FirstPerson_f);
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
		else if (IsEntityGargantua(ent))
		{
			gPhysicsManager.CreateGargantua(ent);
			gCorpseManager.AddGargantua(ent->index, 0);
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

void HUD_DrawNormalTriangles(void)
{
	gExportfuncs.HUD_DrawNormalTriangles();

	gPhysicsManager.DebugDraw();
}