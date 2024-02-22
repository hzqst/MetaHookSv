#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <capstone.h>
#include <cl_entity.h>
#include <com_model.h>
#include <triangleapi.h>
#include <cvardef.h>
#include <entity_types.h>
#include <pm_defs.h>
#include <set>

#include "mathlib2.h"
#include "plugins.h"
#include "enginedef.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "message.h"
#include "ClientEntityManager.h"
#include "physics.h"

static hook_t *g_phook_GameStudioRenderer_StudioSetupBones = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioDrawPlayer = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioDrawModel = NULL;
static hook_t *g_phook_R_StudioSetupBones = NULL;
static hook_t *g_phook_R_StudioDrawPlayer = NULL;
static hook_t *g_phook_R_StudioDrawModel = NULL;
static hook_t *g_phook_efxapi_R_TempModel = NULL;

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

cvar_t *bv_debug = NULL;
cvar_t *bv_simrate = NULL;
cvar_t *bv_enable = NULL;
cvar_t *bv_syncview = NULL;
cvar_t *bv_ragdoll_sleepaftertime = NULL;
cvar_t *bv_ragdoll_sleeplinearvel = NULL;
cvar_t *bv_ragdoll_sleepangularvel = NULL;
cvar_t *chase_active = NULL;
cvar_t* sv_cheats = NULL;

const int RagdollRenderState_None = 0;
const int RagdollRenderState_Monster = 1;
const int RagdollRenderState_Player = 2;
const int RagdollRenderState_Jiggle = 4;

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;
int g_iRagdollRenderState = 0;
int g_iRagdollRenderEntIndex = 0;

ref_params_t r_params = { 0 };

model_t* r_worldmodel = NULL;
cl_entity_t* r_worldentity = NULL;

model_t* CounterStrike_RedirectPlayerModel(model_t* original_model, int PlayerNumber, int* modelindex);

bool AllowCheats()
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		return (*allow_cheats) != 0;
	}

	return (sv_cheats->value != 0) ? true : false;
}

typedef enum
{
	ACT_RESET,
	ACT_IDLE,
	ACT_GUARD,
	ACT_WALK,
	ACT_RUN,
	ACT_FLY,
	ACT_SWIM,
	ACT_HOP,
	ACT_LEAP,
	ACT_FALL,
	ACT_LAND,
	ACT_STRAFE_LEFT,
	ACT_STRAFE_RIGHT,
	ACT_ROLL_LEFT,
	ACT_ROLL_RIGHT,
	ACT_TURN_LEFT,
	ACT_TURN_RIGHT,
	ACT_CROUCH,
	ACT_CROUCHIDLE,
	ACT_STAND,
	ACT_USE,
	ACT_SIGNAL1,
	ACT_SIGNAL2,
	ACT_SIGNAL3,
	ACT_TWITCH,
	ACT_COWER,
	ACT_SMALL_FLINCH,
	ACT_BIG_FLINCH,
	ACT_RANGE_ATTACK1,
	ACT_RANGE_ATTACK2,
	ACT_MELEE_ATTACK1,
	ACT_MELEE_ATTACK2,
	ACT_RELOAD,
	ACT_ARM,
	ACT_DISARM,
	ACT_EAT,
	ACT_DIESIMPLE,
	ACT_DIEBACKWARD,
	ACT_DIEFORWARD,
	ACT_DIEVIOLENT,
	ACT_BARNACLE_HIT,
	ACT_BARNACLE_PULL,
	ACT_BARNACLE_CHOMP,
	ACT_BARNACLE_CHEW,
	ACT_SLEEP,
	ACT_INSPECT_FLOOR,
	ACT_INSPECT_WALL,
	ACT_IDLE_ANGRY,
	ACT_WALK_HURT,
	ACT_RUN_HURT,
	ACT_HOVER,
	ACT_GLIDE,
	ACT_FLY_LEFT,
	ACT_FLY_RIGHT,
	ACT_DETECT_SCENT,
	ACT_SNIFF,
	ACT_BITE,
	ACT_THREAT_DISPLAY,
	ACT_FEAR_DISPLAY,
	ACT_EXCITED,
	ACT_SPECIAL_ATTACK1,
	ACT_SPECIAL_ATTACK2,
	ACT_COMBAT_IDLE,
	ACT_WALK_SCARED,
	ACT_RUN_SCARED,
	ACT_VICTORY_DANCE,
	ACT_DIE_HEADSHOT,
	ACT_DIE_CHESTSHOT,
	ACT_DIE_GUTSHOT,
	ACT_DIE_BACKSHOT,
	ACT_FLINCH_HEAD,
	ACT_FLINCH_CHEST,
	ACT_FLINCH_STOMACH,
	ACT_FLINCH_LEFTARM,
	ACT_FLINCH_RIGHTARM,
	ACT_FLINCH_LEFTLEG,
	ACT_FLINCH_RIGHTLEG,
	ACT_FLINCH_SMALL,
	ACT_FLINCH_LARGE,
	ACT_HOLDBOMB
}activity_e;

int StudioGetSequenceActivityType(model_t* mod, entity_state_t* entstate)
{
	if (g_bIsSvenCoop)
	{
		if (entstate->scale != 0 && entstate->scale != 1.0f)
			return 0;
	}

	if (mod->type != mod_studio)
		return 0;

	auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return 0;

	int sequence = entstate->sequence;
	if (sequence >= studiohdr->numseq)
		return 0;

	auto pseqdesc = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex) + sequence;

	if (
		pseqdesc->activity == ACT_DIESIMPLE ||
		pseqdesc->activity == ACT_DIEBACKWARD ||
		pseqdesc->activity == ACT_DIEFORWARD ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIE_HEADSHOT ||
		pseqdesc->activity == ACT_DIE_CHESTSHOT ||
		pseqdesc->activity == ACT_DIE_GUTSHOT ||
		pseqdesc->activity == ACT_DIE_BACKSHOT
		)
	{
		return 1;
	}

	if (
		pseqdesc->activity == ACT_BARNACLE_HIT ||
		pseqdesc->activity == ACT_BARNACLE_PULL ||
		pseqdesc->activity == ACT_BARNACLE_CHOMP ||
		pseqdesc->activity == ACT_BARNACLE_CHEW
		)
	{
		return 2;
	}

	return 0;
}

entity_state_t *R_GetPlayerState(int index)
{
	return ((entity_state_t *)((char *)cl_frames + size_of_frame * ((*cl_parsecount) & 63) + sizeof(entity_state_t) * index));
}

bool CL_IsFirstPersonMode(cl_entity_t *player)
{
	return (!gExportfuncs.CL_IsThirdPerson() && (*cl_viewentity) == player->index && !(chase_active && chase_active->value)) ? true : false;
}

msurface_t* GetWorldSurfaceByIndex(int index)
{
	msurface_t* surf;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		surf = (((msurface_hl25_t*)r_worldmodel->surfaces) + index);
	}
	else
	{
		surf = r_worldmodel->surfaces + index;
	}

	return surf;
}

int GetWorldSurfaceIndex(msurface_t* surf)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		auto surf25 = (msurface_hl25_t*)surf;
		auto surfbase = (msurface_hl25_t*)r_worldmodel->surfaces;

		return surf25 - surfbase;
	}

	return surf - r_worldmodel->surfaces;
}

int EngineGetNumKnownModel()
{
	return (*mod_numknown);
}

int EngineGetMaxKnownModel()
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
	ClientEntityManager()->FreePlayerForBarnacle(entindex);
}

/*
	Purpose : StudioSetupBones hook handler
*/

template<typename CallType>
__forceinline void StudioSetupBones_Template(CallType pfnSetupBones, void* pthis = nullptr, int dummy = 0)
{
	if ((g_iRagdollRenderState & (RagdollRenderState_Monster | RagdollRenderState_Player)) && !(g_iRagdollRenderState & RagdollRenderState_Jiggle))
	{
		if (gPhysicsManager.SetupBones((*pstudiohdr), g_iRagdollRenderEntIndex))
			return;
	}

	pfnSetupBones(pthis, dummy);

	if (ClientEntityManager()->IsEntityBarnacle((*currententity)))
	{
		auto player = ClientEntityManager()->FindPlayerForBarnacle((*currententity)->index);

		if (player)
		{
			gPhysicsManager.MergeBarnacleBones((*pstudiohdr), player->index);
		}
	}

	if ((g_iRagdollRenderState & (RagdollRenderState_Monster | RagdollRenderState_Player)) && (g_iRagdollRenderState & RagdollRenderState_Jiggle))
	{
		if (gPhysicsManager.SetupJiggleBones((*pstudiohdr), g_iRagdollRenderEntIndex))
			return;
	}
}

__forceinline void R_StudioSetupBones_originalcall_wrapper(void* pthis, int dummy)
{
	return gPrivateFuncs.R_StudioSetupBones();
}

void R_StudioSetupBones(void)
{
	return StudioSetupBones_Template(R_StudioSetupBones_originalcall_wrapper);
}

//ClientSetupBones

void __fastcall GameStudioRenderer_StudioSetupBones(void *pthis, int dummy)
{
	return StudioSetupBones_Template(gPrivateFuncs.GameStudioRenderer_StudioSetupBones, pthis, dummy);
}

/*
	Purpose : StudioDrawModel hook handler
*/

template<typename CallType>
__forceinline int StudioDrawModel_Template(CallType pfnDrawModel, int flags, void* pthis = nullptr, int dummy = 0)
{
	if ((flags & STUDIO_RENDER) &&
		!(*currententity)->player &&
		(*currententity)->index &&
		(*currententity)->curstate.messagenum == (*cl_parsecount) &&
		(*currententity)->curstate.renderfx != kRenderFxDeadPlayer &&
		((*currententity)->curstate.scale == 1.0f || (*currententity)->curstate.scale == 0.0f)
		)
	{
		int entindex = (*currententity)->index;
		auto model = (*currententity)->model;

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);
		if (!ragdoll)
		{
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				pfnDrawModel(pthis, 0, 0);

				ragdoll = gPhysicsManager.CreateRagdoll(model, cfg, entindex);

				goto has_ragdoll;
			}
		}
		else
		{
		has_ragdoll:

			int iActivityType = StudioGetSequenceActivityType(model, &(*currententity)->curstate);

			if (iActivityType == 0)
			{
				iActivityType = gPhysicsManager.GetSequenceActivityType(ragdoll, &(*currententity)->curstate);
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, &(*currententity)->curstate))
			{
				//Monster don't have barnacle animation
			}

			if (ClientEntityManager()->IsEntityGargantua((*currententity)))
			{
				g_iRagdollRenderState = RagdollRenderState_Monster | RagdollRenderState_Jiggle;
				g_iRagdollRenderEntIndex = entindex;

				int result = pfnDrawModel(pthis, 0, flags);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
			else if (ragdoll->m_iActivityType > 0)
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy((*currententity)->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, (*currententity)->origin);

				int result = pfnDrawModel(pthis, 0, flags);

				VectorCopy(saved_origin, (*currententity)->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
			else
			{
				g_iRagdollRenderState = RagdollRenderState_Monster | RagdollRenderState_Jiggle;
				g_iRagdollRenderEntIndex = entindex;

				int result = pfnDrawModel(pthis, 0, flags);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
		}
	}

	//ClCorpse temp entity?

	if ((flags & STUDIO_RENDER) &&
		!(*currententity)->player &&
		!(*currententity)->index &&
		(*currententity)->curstate.iuser4 == PhyCorpseFlag &&
		(*currententity)->curstate.iuser3 >= ENTINDEX_TEMPENTITY &&
		(*currententity)->curstate.owner >= 1 && (*currententity)->curstate.owner <= gEngfuncs.GetMaxClients()
		)
	{
		auto model = (*currententity)->model;

		int entindex = (*currententity)->curstate.iuser3;

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);
		if (!ragdoll)
		{
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				pfnDrawModel(pthis, 0, 0);

				ragdoll = gPhysicsManager.CreateRagdoll(model, cfg, entindex);

				goto has_ragdoll_clcorpse;
			}
		}
		else
		{
		has_ragdoll_clcorpse:

			int iActivityType = StudioGetSequenceActivityType(model, &(*currententity)->curstate);

			if (iActivityType == 0)
			{
				iActivityType = gPhysicsManager.GetSequenceActivityType(ragdoll, &(*currententity)->curstate);
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, &(*currententity)->curstate))
			{

			}

			if (ragdoll->m_iActivityType > 0)
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy((*currententity)->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, (*currententity)->origin);

				int result = pfnDrawModel(pthis, 0, flags);

				VectorCopy(saved_origin, (*currententity)->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
			else
			{
				g_iRagdollRenderState = RagdollRenderState_Monster | RagdollRenderState_Jiggle;
				g_iRagdollRenderEntIndex = entindex;

				int result = pfnDrawModel(pthis, 0, flags);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
		}
	}

	return pfnDrawModel(pthis, 0, flags);
}

__forceinline int R_StudioDrawModel_originalcall_wrapper(void* pthis, int dummy, int flags)
{
	return gPrivateFuncs.R_StudioDrawModel(flags);
}

int R_StudioDrawModel(int flags)
{
	return StudioDrawModel_Template(R_StudioDrawModel_originalcall_wrapper, flags);
}

int __fastcall GameStudioRenderer_StudioDrawModel(void *pthis, int dummy, int flags)
{
	return StudioDrawModel_Template(gPrivateFuncs.GameStudioRenderer_StudioDrawModel, flags, pthis, dummy);
}

/*

	Purpose : StudioDrawPlayer hook handler

*/

template<typename CallType>
__forceinline int StudioDrawPlayer_Template(CallType pfnDrawPlayer, int flags, struct entity_state_s*pplayer, void* pthis = nullptr, int dummy = 0)
{
	int playerindex = pplayer->number;

	int entindex = ((*currententity)->curstate.renderfx == kRenderFxDeadPlayer) ? playerindex : (*currententity)->index;

	if (flags & STUDIO_RAGDOLL)
	{
		flags = 0;
		goto start_render;
	}

	if (flags & STUDIO_RENDER)
	{
	start_render:

		auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

		if (g_bIsCounterStrike)
		{
			//Counter-Strike redirects playermodel in a pretty tricky way
			int modelindex = 0;
			model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
		}

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);

		if (!ragdoll)
		{
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				//Remove weapon model for me ?
				int save_weaponmodel = pplayer->weaponmodel;
				int save_sequence = pplayer->sequence;
				int save_gaitsequence = pplayer->gaitsequence;

				pplayer->weaponmodel = 0;
				pplayer->sequence = 0;
				pplayer->gaitsequence = 0;

				pfnDrawPlayer(pthis, 0, 0, pplayer);

				pplayer->weaponmodel = save_weaponmodel;
				pplayer->sequence = save_sequence;
				pplayer->gaitsequence = save_gaitsequence;

				if (!(*pstudiohdr))
					return 0;

				ragdoll = gPhysicsManager.CreateRagdoll(model, cfg, entindex);

				goto has_ragdoll;
			}
		}
		else
		{
			//model changed ?
			if (ragdoll->m_model != model)
			{
				gPhysicsManager.RemoveRagdoll(entindex);
				return pfnDrawPlayer(pthis, 0, flags, pplayer);
			}

		has_ragdoll:

			int oldActivityType = ragdoll->m_iActivityType;

			int iActivityType = StudioGetSequenceActivityType(model, pplayer);

			if (iActivityType == 0)
			{
				iActivityType = gPhysicsManager.GetSequenceActivityType(ragdoll, pplayer);
			}

			if (playerindex == entindex)
			{
				if (iActivityType == 1)
				{
					ClientEntityManager()->SetPlayerDeathState(playerindex, pplayer, model);
				}
				else
				{
					ClientEntityManager()->ClearPlayerDeathState(playerindex);
				}
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, pplayer))
			{
				//Transform from whatever to barnacle
				if (ragdoll->m_iActivityType == 2)
				{
					auto BarnacleEntity = ClientEntityManager()->FindBarnacleForPlayer(pplayer);

					if (BarnacleEntity)
					{
						gPhysicsManager.ApplyBarnacle(ragdoll, BarnacleEntity);
					}
					else
					{
						auto GargantuaEntity = ClientEntityManager()->FindGargantuaForPlayer(pplayer);

						if (GargantuaEntity)
						{
							gPhysicsManager.ApplyGargantua(ragdoll, GargantuaEntity);
						}
					}
				}

				//Transform from death or barnacle to idle state.
				else if (oldActivityType > 0 && ragdoll->m_iActivityType == 0)
				{
					pfnDrawPlayer(pthis, 0, 0, pplayer);

					gPhysicsManager.ResetPose(ragdoll, pplayer);
				}
			}

			//Teleport ?
			else if (oldActivityType == 0 && ragdoll->m_iActivityType == 0 &&
				VectorDistance((*currententity)->curstate.origin, (*currententity)->latched.prevorigin) > 500)
			{
				pfnDrawPlayer(pthis, 0, 0, pplayer);

				gPhysicsManager.ResetPose(ragdoll, pplayer);
			}

			if (ragdoll->m_iActivityType > 0)
			{
				g_iRagdollRenderState = RagdollRenderState_Player;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy((*currententity)->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, (*currententity)->origin);

				//Remove weapon model for me ?
				int saved_weaponmodel = pplayer->weaponmodel;

				pplayer->weaponmodel = 0;

				int result = pfnDrawPlayer(pthis, 0, flags, pplayer);

				pplayer->weaponmodel = saved_weaponmodel;

				VectorCopy(saved_origin, (*currententity)->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
			else
			{
				g_iRagdollRenderState = RagdollRenderState_Player | RagdollRenderState_Jiggle;
				g_iRagdollRenderEntIndex = entindex;

				int result = pfnDrawPlayer(pthis, 0, flags, pplayer);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
		}
	}

	return pfnDrawPlayer(pthis, 0, flags, pplayer);
}

__forceinline int R_StudioDrawPlayer_originalcall_wrapper(void* pthis, int dummy, int flags, struct entity_state_s* pplayer)
{
	return gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);
}

int R_StudioDrawPlayer(int flags, struct entity_state_s*pplayer)
{
	return StudioDrawPlayer_Template(R_StudioDrawPlayer_originalcall_wrapper, flags, pplayer);
}

int __fastcall GameStudioRenderer_StudioDrawPlayer(void *pthis, int dummy, int flags, struct entity_state_s*pplayer)
{
	return StudioDrawPlayer_Template(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer, flags, pplayer, pthis, dummy);
}

void EngineStudio_FillAddress(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();
	plighttransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->GetCurrentEntity, 0x10, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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
		g_pMetaHookAPI->DisasmRanges(pstudio->StudioSetHeader, 0x10, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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
		g_pMetaHookAPI->DisasmRanges(pstudio->SetRenderModel, 0x10, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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

	if ((void*)(*ppinterface)->StudioDrawPlayer > g_dwClientTextBase && (void*)(*ppinterface)->StudioDrawPlayer < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pMetaHookAPI->DisasmRanges((void*)(*ppinterface)->StudioDrawPlayer, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_ECX &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwClientDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwClientDataBase + g_dwClientDataSize)
				{
					g_pGameStudioRenderer = (decltype(g_pGameStudioRenderer))pinst->detail->x86.operands[1].imm;
				}

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
					PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;

					PVOID* vftable = *(PVOID**)g_pGameStudioRenderer;
					for (int i = 1; i < 4; ++i)
					{
						if (vftable[i] == imm)
						{
							gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index = i;
							break;
						}
					}
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

				if (pinst->id == X86_INS_CALL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM)
				{
					PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;

					PVOID* vftable = *(PVOID**)g_pGameStudioRenderer;
					for (int i = 1; i < 4; ++i)
					{
						if (vftable[i] == imm)
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
			}, 0, NULL);

		if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index == 0)
			gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index = 2;

		PVOID* vftable = *(PVOID**)g_pGameStudioRenderer;

		for (int i = 4; i < 9; ++i)
		{
			//GameStudioRenderer_StudioCalcAttachments_vftable_index

			typedef struct
			{
				PVOID base;
				size_t max_insts;
				int max_depth;
				std::set<PVOID> code;
				std::set<PVOID> branches;
				std::vector<walk_context_t> walks;
				int index;
			}StudioCalcAttachments_SearchContext;

			StudioCalcAttachments_SearchContext ctx;

			ctx.base = (void*)vftable[i];
			ctx.index = i;

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
						(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwClientBase &&
						(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwClientBase + g_dwClientSize)
					{
						const char* pPushedString = (const char*)pinst->detail->x86.operands[0].imm;
						if (0 == memcmp(pPushedString, "Too many attachments on %s\n", sizeof("Too many attachments on %s\n") - 1))
						{
							gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = ctx->index;
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

		if (g_bIsCounterStrike)
		{
			g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

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

			gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer))vftable[gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index];

		}

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

		ctx.base = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer;

		if (gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer)
		{
			ctx.base = gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer;
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
				auto ctx = (GameStudioRenderer_StudioDrawPlayer_ctx*)context;

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
					pinst->detail->x86.operands[0].mem.disp >= (ULONG_PTR)g_dwClientBase &&
					pinst->detail->x86.operands[0].mem.disp < (ULONG_PTR)g_dwClientBase + g_dwClientSize)
				{
					PVOID pfnCall = *(PVOID*)pinst->detail->x86.operands[0].mem.disp;

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

		gPrivateFuncs.GameStudioRenderer_StudioRenderModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderModel))vftable[gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index];

		g_pMetaHookAPI->DisasmRanges((void*)gPrivateFuncs.GameStudioRenderer_StudioRenderModel, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

				if (pinst->id == X86_INS_CALL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					pinst->detail->x86.operands[0].mem.disp > gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index &&
					pinst->detail->x86.operands[0].mem.disp <= gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index + 0x20)
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

		gPrivateFuncs.GameStudioRenderer_StudioRenderFinal = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderFinal))vftable[gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index];

		gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index - 1;
		gPrivateFuncs.GameStudioRenderer_StudioSaveBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 1;
		gPrivateFuncs.GameStudioRenderer_StudioMergeBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 2;

		gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))vftable[gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioMergeBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioMergeBones))vftable[gPrivateFuncs.GameStudioRenderer_StudioMergeBones_vftable_index];

		gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))vftable[gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index];

		//Install_InlineHook(GameStudioRenderer_StudioSetupBones);
		//Install_InlineHook(GameStudioRenderer_StudioDrawPlayer);
		//Install_InlineHook(GameStudioRenderer_StudioDrawModel);
	}
	else if ((void*)(*ppinterface)->StudioDrawPlayer > g_dwEngineBase && (void*)(*ppinterface)->StudioDrawPlayer < (PUCHAR)g_dwEngineBase + g_dwEngineSize)
	{
		const char sigs1[] = "Bip01 Spine\0";
		auto Bip01_String = Search_Pattern_Data(sigs1);
		if (!Bip01_String)
			Bip01_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Bip01_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
		*(DWORD*)(pattern + 1) = (DWORD)Bip01_String;
		auto Bip01_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(Bip01_PushString);

		gPrivateFuncs.R_StudioSetupBones = (decltype(gPrivateFuncs.R_StudioSetupBones))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bip01_PushString, 0x600, [](PUCHAR Candidate) {
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
		Sig_FuncNotFound(R_StudioSetupBones);

		gPrivateFuncs.R_StudioDrawModel = (decltype(gPrivateFuncs.R_StudioDrawModel))(*ppinterface)->StudioDrawModel;
		gPrivateFuncs.R_StudioDrawPlayer = (decltype(gPrivateFuncs.R_StudioDrawPlayer))(*ppinterface)->StudioDrawPlayer;

		Install_InlineHook(R_StudioSetupBones);
		Install_InlineHook(R_StudioDrawPlayer);
		Install_InlineHook(R_StudioDrawModel);
	}
	else
	{
		Sys_Error("Failed to locate g_pGameStudioRenderer or EngineStudioRenderer!\n");
	}
}

void EngineStudio_InstallHooks()
{
	/*
		Client studio implementation
	*/

	if (gPrivateFuncs.GameStudioRenderer_StudioSetupBones)
	{
		Install_InlineHook(GameStudioRenderer_StudioSetupBones);
	}
	
	if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer)
	{
		Install_InlineHook(GameStudioRenderer_StudioDrawPlayer);
	}

	if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel)
	{
		Install_InlineHook(GameStudioRenderer_StudioDrawModel);
	}

	/*
		Engine studio implementation
	*/

	if (gPrivateFuncs.R_StudioSetupBones)
	{
		Install_InlineHook(R_StudioSetupBones);
	}

	if (gPrivateFuncs.R_StudioDrawPlayer)
	{
		Install_InlineHook(R_StudioDrawPlayer);
	}

	if (gPrivateFuncs.R_StudioDrawModel)
	{
		Install_InlineHook(R_StudioDrawModel);
	}
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	EngineStudio_FillAddress(version, ppinterface, pstudio);
	EngineStudio_InstallHooks();

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
		if (StudioGetSequenceActivityType(localplayer->model, &localplayer->curstate) == 2)
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
		if (StudioGetSequenceActivityType(localplayer->model, &localplayer->curstate) == 2)
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
	//bv_scale = gEngfuncs.pfnRegisterVariable("bv_scale", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_enable = gEngfuncs.pfnRegisterVariable("bv_enable", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_syncview = gEngfuncs.pfnRegisterVariable("bv_syncview", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_ragdoll_sleepaftertime = gEngfuncs.pfnRegisterVariable("bv_ragdoll_sleepaftertime", "3", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_ragdoll_sleeplinearvel = gEngfuncs.pfnRegisterVariable("bv_ragdoll_sleeplinearvel", "5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_ragdoll_sleepangularvel = gEngfuncs.pfnRegisterVariable("bv_ragdoll_sleepangularvel", "3", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	sv_cheats = gEngfuncs.pfnGetCvarPointer("sv_cheats");
	chase_active = gEngfuncs.pfnGetCvarPointer("chase_active");
	cl_minmodels = gEngfuncs.pfnGetCvarPointer("cl_minmodels");
	cl_min_ct = gEngfuncs.pfnGetCvarPointer("cl_min_ct");
	cl_min_t = gEngfuncs.pfnGetCvarPointer("cl_min_t");

	gEngfuncs.pfnAddCommand("bv_reload", BV_Reload_f);

	//gPrivateFuncs.ThreadPerson_f = g_pMetaHookAPI->HookCmd("thirdperson", BV_ThreadPerson_f);
	//gPrivateFuncs.FirstPerson_f = g_pMetaHookAPI->HookCmd("firstperson", BV_FirstPerson_f);

	//For clcorpse hook
	m_pfnClCorpse = HOOK_MESSAGE(ClCorpse);
	if (m_pfnClCorpse)
	{
		gPrivateFuncs.efxapi_R_TempModel = gEngfuncs.pEfxAPI->R_TempModel;
		Install_InlineHook(efxapi_R_TempModel);
	}
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	if (type == ET_NORMAL && ent->model)
	{
		if (ent->model->type == modtype_t::mod_brush && ent->curstate.solid == SOLID_BSP)
		{
			gPhysicsManager.CreateBrushModel(ent);
		}

		if (ClientEntityManager()->IsEntityDeadPlayer(ent))
		{
			int playerindex = (int)ent->curstate.renderamt;

			gPhysicsManager.ChangeRagdollEntIndex(playerindex, ent->index);
		}
		else if (ClientEntityManager()->IsEntityBarnacle(ent))
		{
			gPhysicsManager.CreateBarnacle(ent);
			ClientEntityManager()->AddBarnacle(ent->index, 0);
		}
		else if (ClientEntityManager()->IsEntityGargantua(ent))
		{
			gPhysicsManager.CreateGargantua(ent);
			ClientEntityManager()->AddGargantua(ent->index, 0);
		}
	}

	if (type == ET_PLAYER && ent->model)
	{
		ClientEntityManager()->SetPlayerEmitted(ent->index);
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
	gExportfuncs.HUD_TempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);

	//Update only if level is present

	auto levelname = gEngfuncs.pfnGetLevelName();

	if (levelname && levelname[0])
	{
		gPhysicsManager.SetGravity(cl_gravity);
		gPhysicsManager.UpdateTempEntity(ppTempEntFree, ppTempEntActive, frametime, client_time);
		gPhysicsManager.StepSimulation(frametime);
	}
}

void HUD_Frame(double frametime)
{
	gExportfuncs.HUD_Frame(frametime);

	ClientEntityManager()->ClearAllPlayerEmitState();
}

void HUD_Shutdown(void)
{
	gExportfuncs.HUD_Shutdown();

	gPhysicsManager.Shutdown();

	Uninstall_Hook(GameStudioRenderer_StudioSetupBones);
	Uninstall_Hook(GameStudioRenderer_StudioDrawPlayer);
	Uninstall_Hook(GameStudioRenderer_StudioDrawModel);
	Uninstall_Hook(R_StudioSetupBones);
	Uninstall_Hook(R_StudioDrawPlayer);
	Uninstall_Hook(R_StudioDrawModel);
	Uninstall_Hook(efxapi_R_TempModel);
}

void V_CalcRefdef(struct ref_params_s *pparams)
{
	memcpy(&r_params, pparams, sizeof(r_params));

	if (pparams->intermission)
		goto skip;

	if (pparams->paused)
		goto skip;

	if (pparams->nextView)
		goto skip;

	if (g_bRenderingPortals_SCClient && (*g_bRenderingPortals_SCClient))
		goto skip;

	auto local = gEngfuncs.GetLocalPlayer();

	if (local && local->player && bv_syncview->value)
	{
		auto spectating_player = local;

		if (g_iUser1 && g_iUser2 && (*g_iUser1))
		{
			spectating_player = gEngfuncs.GetEntityByIndex((*g_iUser2));
		}

		if (!CL_IsFirstPersonMode(spectating_player))
		{
			auto ragdoll = gPhysicsManager.FindRagdoll(spectating_player->index);

			if (ragdoll && ragdoll->m_iActivityType != 0)
			{
				vec3_t save_simorg;
				vec3_t save_origin_spec;
				vec3_t save_origin_aiming;

				VectorCopy(pparams->simorg, save_simorg);
				VectorCopy(spectating_player->origin, save_origin_spec);

				gPhysicsManager.GetRagdollOrigin(ragdoll, spectating_player->origin);
				VectorCopy(spectating_player->origin, pparams->simorg);

				if (spectating_player != local)
				{
					auto ragdoll_spectating = gPhysicsManager.FindRagdoll(spectating_player->index);
					if (ragdoll_spectating)
					{
						gPhysicsManager.GetRagdollOrigin(ragdoll_spectating, spectating_player->origin);
					}
				}

				gExportfuncs.V_CalcRefdef(pparams);

				VectorCopy(save_origin_spec, spectating_player->origin);
				VectorCopy(save_simorg, pparams->simorg);

				return;
			}
		}
		else
		{
			auto ragdoll = gPhysicsManager.FindRagdoll(spectating_player->index);

			if (ragdoll && ragdoll->m_iActivityType != 0)
			{
				if (g_bIsCounterStrike && spectating_player->index == gEngfuncs.GetLocalPlayer()->index)
				{
					if (g_iUser1 && !(*g_iUser1))
						goto skip;
				}

				vec3_t save_simorg;
				vec3_t save_cl_viewangles;
				int save_health = pparams->health;

				VectorCopy(pparams->simorg, save_simorg);
				VectorCopy(pparams->cl_viewangles, save_cl_viewangles);
				
				gPhysicsManager.SyncFirstPersonView(ragdoll, spectating_player, pparams);

				gExportfuncs.V_CalcRefdef(pparams);

				VectorCopy(save_simorg, pparams->simorg);
				VectorCopy(save_cl_viewangles, pparams->cl_viewangles);
				pparams->health = save_health;

				return;
			}
		}
	}
skip:
	gExportfuncs.V_CalcRefdef(pparams);
}

void HUD_DrawNormalTriangles(void)
{
	gExportfuncs.HUD_DrawNormalTriangles();

	if (AllowCheats())
	{
		gPhysicsManager.DebugDraw();
	}
}

void HUD_CreateEntities(void)
{
	gExportfuncs.HUD_CreateEntities();

	if ((*cl_viewentity) >= 1 && (*cl_viewentity) <= gEngfuncs.GetMaxClients())
	{
		auto localplayer = gEngfuncs.GetLocalPlayer();
		auto viewplayer = gEngfuncs.GetEntityByIndex((*cl_viewentity));
		if (viewplayer && viewplayer->player)
		{
			if (ClientEntityManager()->IsEntityPresent(viewplayer) &&
				!ClientEntityManager()->IsPlayerEmitted(viewplayer->index) &&
				CL_IsFirstPersonMode(viewplayer))
			{
				auto playerstate = R_GetPlayerState(viewplayer->index);

				if (localplayer->index == viewplayer->index)
				{
					VectorCopy(playerstate->origin, viewplayer->origin);
					VectorCopy(playerstate->origin, viewplayer->prevstate.origin);
					VectorCopy(playerstate->origin, viewplayer->curstate.origin);
					VectorCopy(viewplayer->curstate.angles, viewplayer->angles);

					VectorCopy(r_params.simorg, viewplayer->origin);
				}

				auto save_currententity = (*currententity);
				(*currententity) = viewplayer;
				(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL, playerstate);
				(*currententity) = save_currententity;

				//gEngfuncs.CL_CreateVisibleEntity(ET_PLAYER, viewplayer);
			}
		}
	}
}