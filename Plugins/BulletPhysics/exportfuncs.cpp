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
#include "enginedef.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "message.h"
#include "corpse.h"
#include "physics.h"

hook_t *g_phook_GameStudioRenderer_StudioSetupBones = NULL;
hook_t *g_phook_GameStudioRenderer_StudioDrawPlayer = NULL;
hook_t *g_phook_GameStudioRenderer_StudioDrawModel = NULL;
hook_t *g_phook_R_StudioSetupBones = NULL;
hook_t *g_phook_R_StudioDrawPlayer = NULL;
hook_t *g_phook_R_StudioDrawModel = NULL;
hook_t *g_phook_efxapi_R_TempModel = NULL;

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

cvar_t *bv_debug = NULL;
cvar_t *bv_simrate = NULL;
cvar_t *bv_scale = NULL;
cvar_t *bv_enable = NULL;
cvar_t *bv_syncview = NULL;
cvar_t *chase_active = NULL;

const int RagdollRenderState_None = 0;
const int RagdollRenderState_Monster = 1;
const int RagdollRenderState_Player = 2;
const int RagdollRenderState_PlayerWithJiggle = 3;

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;
int g_iRagdollRenderState = 0;
int g_iRagdollRenderEntIndex = 0;

studiohdr_t **pstudiohdr = NULL;
model_t **r_model = NULL;
model_t *r_worldmodel = NULL;
cl_entity_t *r_worldentity = NULL;
void *g_pGameStudioRenderer = NULL;
int *r_visframecount = NULL;
int *cl_parsecount = NULL;
void *cl_frames = NULL;
int size_of_frame = 0;
int *cl_viewentity = NULL;
void *mod_known = NULL;
int *mod_numknown = NULL;
TEMPENTITY *gTempEnts = NULL;

int *g_iUser1 = NULL;
int *g_iUser2 = NULL;

ref_params_t r_params = { 0 };

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

bool IsEntityGargantua(cl_entity_t* ent);
bool IsEntityBarnacle(cl_entity_t* ent);
bool IsEntityWater(cl_entity_t* ent);
bool IsEntityDeadPlayer(cl_entity_t* ent);
bool IsEntityPresent(cl_entity_t* ent);

int StudioGetSequenceActivityType(model_t *mod, entity_state_t* entstate);

entity_state_t *R_GetPlayerState(int index)
{
	return ((entity_state_t *)((char *)cl_frames + size_of_frame * ((*cl_parsecount) & 63) + sizeof(entity_state_t) * index));
}

bool CL_IsFirstPersonMode(cl_entity_t *player)
{
	return (!gExportfuncs.CL_IsThirdPerson() && (*cl_viewentity) == player->index && !(chase_active && chase_active->value)) ? true : false;
}

int EngineGetNumKnownModel(void)
{
	return (*mod_numknown);
}

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

	if (g_iRagdollRenderState == RagdollRenderState_Monster || g_iRagdollRenderState == RagdollRenderState_Player)
	{
		if (gPhysicsManager.SetupBones((*pstudiohdr), g_iRagdollRenderEntIndex))
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

	if (g_iRagdollRenderState == RagdollRenderState_Monster || g_iRagdollRenderState == RagdollRenderState_PlayerWithJiggle)
	{
		if (gPhysicsManager.SetupJiggleBones((*pstudiohdr), g_iRagdollRenderEntIndex))
			return;
	}
}

//ClientSetupBones

void __fastcall GameStudioRenderer_StudioSetupBones(void *pthis, int)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	if (g_iRagdollRenderState == RagdollRenderState_Monster || g_iRagdollRenderState == RagdollRenderState_Player)
	{
		if (gPhysicsManager.SetupBones((*pstudiohdr), g_iRagdollRenderEntIndex))
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

	if (g_iRagdollRenderState == RagdollRenderState_Monster || g_iRagdollRenderState == RagdollRenderState_PlayerWithJiggle)
	{
		if (gPhysicsManager.SetupJiggleBones((*pstudiohdr), g_iRagdollRenderEntIndex))
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
		currententity->curstate.renderfx != kRenderFxDeadPlayer &&
		(currententity->curstate.scale == 1.0f || currententity->curstate.scale == 0.0f)
		)
	{
		int entindex = currententity->index;
		auto model = currententity->model;

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);
		if (!ragdoll)
		{
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.R_StudioDrawModel(0);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, entindex, false);

				goto has_ragdoll;
			}
		}
		else
		{
		has_ragdoll:

			int iActivityType = StudioGetSequenceActivityType(model, &currententity->curstate);

			if (iActivityType == 0)
			{
				iActivityType = gPhysicsManager.GetSequenceActivityType(ragdoll, &currententity->curstate);
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, &currententity->curstate))
			{
				//Monster don't have barnacle animation
			}

			if (ragdoll->m_iActivityType > 0)
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy(currententity->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

				int result = gPrivateFuncs.R_StudioDrawModel(flags);

				VectorCopy(saved_origin, currententity->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
			else
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				int result = gPrivateFuncs.R_StudioDrawModel(flags);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

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

	if ((flags & STUDIO_RENDER) &&
		!currententity->player && 
		currententity->index &&
		currententity->curstate.messagenum == (*cl_parsecount) &&
		currententity->curstate.renderfx != kRenderFxDeadPlayer &&
		(currententity->curstate.scale == 1.0f || currententity->curstate.scale == 0.0f)
		)
	{
		int entindex = currententity->index;
		auto model = currententity->model;

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);
		if (!ragdoll)
		{
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, 0);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, entindex, false);

				goto has_ragdoll;
			}
		}
		else
		{
		has_ragdoll:

			int iActivityType = StudioGetSequenceActivityType(model, &currententity->curstate);

			if (iActivityType == 0)
			{
				iActivityType = gPhysicsManager.GetSequenceActivityType(ragdoll, &currententity->curstate);
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, &currententity->curstate))
			{
				//Monster don't have barnacle animation
			}

			if (ragdoll->m_iActivityType > 0)
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy(currententity->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);

				VectorCopy(saved_origin, currententity->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
			else
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
		}
	}

	//ClCorpse temp entity?

	if ((flags & STUDIO_RENDER) &&
		!currententity->player &&
		!currententity->index &&
		currententity->curstate.iuser4 == PhyCorpseFlag &&
		currententity->curstate.iuser3 >= ENTINDEX_TEMPENTITY &&
		currententity->curstate.owner >= 1 && currententity->curstate.owner <= gEngfuncs.GetMaxClients()
		)
	{
		auto model = currententity->model;

		int entindex = currententity->curstate.iuser3;

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);
		if (!ragdoll)
		{
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, 0);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, entindex, true);

				goto has_ragdoll_clcorpse;
			}
		}
		else
		{
		has_ragdoll_clcorpse:

			int iActivityType = StudioGetSequenceActivityType(model, &currententity->curstate);

			if (iActivityType == 0)
			{
				iActivityType = gPhysicsManager.GetSequenceActivityType(ragdoll, &currententity->curstate);
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, &currententity->curstate))
			{

			}

			if (ragdoll->m_iActivityType > 0)
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy(currententity->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);

				VectorCopy(saved_origin, currententity->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
			else
			{
				g_iRagdollRenderState = RagdollRenderState_Monster;
				g_iRagdollRenderEntIndex = entindex;

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;

				return result;
			}
		}
	}

	return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
}

//EngineDrawPlayer

int __fastcall R_StudioDrawPlayer(int flags, struct entity_state_s *pplayer)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	int playerindex = pplayer->number;

	int entindex = (currententity->curstate.renderfx == kRenderFxDeadPlayer) ? currententity->index : playerindex;

	if (flags & STUDIO_RENDER)
	{
		if (currententity->index == (*cl_viewentity) &&
			(*cl_viewentity) >= 1 && (*cl_viewentity) <= gEngfuncs.GetMaxClients())
		{
			auto viewplayer = gEngfuncs.GetEntityByIndex((*cl_viewentity));

			if (IsEntityPresent(viewplayer) &&
				!gCorpseManager.IsPlayerEmitted(viewplayer->index) &&
				CL_IsFirstPersonMode(viewplayer))
			{
				flags &= ~STUDIO_RENDER;
			}
		}

		auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

		auto ragdoll = gPhysicsManager.FindRagdoll(entindex);

		if (!ragdoll)
		{
			auto cfg = gPhysicsManager.LoadRagdollConfig(model);

			if (cfg && cfg->state == 1 && bv_enable->value)
			{
				gPrivateFuncs.R_StudioDrawPlayer(0, pplayer);

				ragdoll = gPhysicsManager.CreateRagdoll(cfg, entindex, true);

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

			int iActivityType = StudioGetSequenceActivityType(model, pplayer);

			if (iActivityType == 0)
			{
				iActivityType = gPhysicsManager.GetSequenceActivityType(ragdoll, pplayer);
			}

			if (playerindex == entindex)
			{
				if (iActivityType == 1)
				{
					gCorpseManager.SetPlayerDying(playerindex, pplayer, model);
				}
				else
				{
					gCorpseManager.ClearPlayerDying(playerindex);
				}
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, pplayer))
			{
				//Transform from whatever to barnacle anim
				if (ragdoll->m_iActivityType == 2)
				{
					cl_entity_t *barnacleEntity = gCorpseManager.FindBarnacleForPlayer(pplayer);

					if (barnacleEntity)
					{
						gPhysicsManager.ApplyBarnacle(ragdoll, barnacleEntity);
					}
					else
					{
						cl_entity_t *gargantuaEntity = gCorpseManager.FindGargantuaForPlayer(pplayer);
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
				//int number = currententity->curstate.number;
				//currententity->curstate.number = pplayer->number;
				g_iRagdollRenderState = RagdollRenderState_Player;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy(currententity->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

				int result = gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);

				VectorCopy(saved_origin, currententity->origin);

				//currententity->curstate.number = number;
				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;
				return result;
			}
			else
			{
				//int number = currententity->curstate.number;
				//currententity->curstate.number = pplayer->number;
				g_iRagdollRenderState = RagdollRenderState_PlayerWithJiggle;
				g_iRagdollRenderEntIndex = currententity->curstate.number;

				int result = gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);

				//currententity->curstate.number = number;
				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;
				return result;
			}
		}
	}

	return gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);
}

//client.dll GameStudioRenderer::StudioDrawPlayer

model_t *CounterStrike_RedirectPlayerModel(model_t *original_model, int PlayerNumber, int *modelindex);

int __fastcall GameStudioRenderer_StudioDrawPlayer(void *pthis, int dummy, int flags, struct entity_state_s *pplayer)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	int playerindex = pplayer->number;

	int entindex = (currententity->curstate.renderfx == kRenderFxDeadPlayer) ? currententity->index : playerindex;

	if (flags & STUDIO_RENDER)
	{
		if (currententity->index == (*cl_viewentity) &&
			(*cl_viewentity) >= 1 && (*cl_viewentity) <= gEngfuncs.GetMaxClients())
		{
			auto viewplayer = gEngfuncs.GetEntityByIndex((*cl_viewentity));

			if (IsEntityPresent(viewplayer) &&
				!gCorpseManager.IsPlayerEmitted(viewplayer->index) &&
				CL_IsFirstPersonMode(viewplayer))
			{
				flags &= ~STUDIO_RENDER;
			}
		}

		auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

		if (g_bIsCounterStrike)
		{
			//Counter-Strike redirect player models in pretty tricky way
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
				int saved_weaponmodel = pplayer->weaponmodel;

				pplayer->weaponmodel = 0;

				gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, 0, pplayer);

				pplayer->weaponmodel = saved_weaponmodel;
				
				ragdoll = gPhysicsManager.CreateRagdoll(cfg, entindex, true);

				goto has_ragdoll;
			}
		}
		else
		{
			//model changed ?
			if (ragdoll->m_studiohdr != IEngineStudio.Mod_Extradata(model))
			{
				gPhysicsManager.RemoveRagdoll(entindex);
				return gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
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
					gCorpseManager.SetPlayerDying(playerindex, pplayer, model);
				}
				else
				{
					gCorpseManager.ClearPlayerDying(playerindex);
				}
			}

			if (gPhysicsManager.UpdateKinematic(ragdoll, iActivityType, pplayer))
			{
				//Transform from whatever to barnacle
				if (ragdoll->m_iActivityType == 2)
				{
					cl_entity_t *barnacleEntity = gCorpseManager.FindBarnacleForPlayer(pplayer);
					
					if (barnacleEntity)
					{
						gPhysicsManager.ApplyBarnacle(ragdoll, barnacleEntity);
					}
					else
					{
						cl_entity_t *gargantuaEntity = gCorpseManager.FindGargantuaForPlayer(pplayer);
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
				//int number = currententity->curstate.number;
				//currententity->curstate.number = pplayer->number;
				g_iRagdollRenderState = RagdollRenderState_Player;
				g_iRagdollRenderEntIndex = entindex;

				vec3_t saved_origin;
				VectorCopy(currententity->origin, saved_origin);
				gPhysicsManager.GetRagdollOrigin(ragdoll, currententity->origin);

				//Remove weapon model for me ?
				int saved_weaponmodel = pplayer->weaponmodel;

				pplayer->weaponmodel = 0;

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);

				pplayer->weaponmodel = saved_weaponmodel;

				VectorCopy(saved_origin, currententity->origin);

				//currententity->curstate.number = number;
				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;
				return result;
			}
			else
			{
				//int number = currententity->curstate.number;
				//currententity->curstate.number = pplayer->number;
				g_iRagdollRenderState = RagdollRenderState_PlayerWithJiggle;
				g_iRagdollRenderEntIndex = entindex;

				int result = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);

				//currententity->curstate.number = number;
				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderState = RagdollRenderState_None;
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

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "svencoop"))
	{
		g_bIsSvenCoop = true;
	}
	else if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
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
#define CSTRIKE_PLAYEREXTRAINFO_SIG "\x66\x85\xC0\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89"

		auto addr = (ULONG_PTR)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, CSTRIKE_PLAYEREXTRAINFO_SIG, sizeof(CSTRIKE_PLAYEREXTRAINFO_SIG) - 1);
		
		Sig_AddrNotFound(g_PlayerExtraInfo);

		g_PlayerExtraInfo = *(decltype(g_PlayerExtraInfo) *)(addr + 6);
	}
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

				PVOID *vftable = *(PVOID **)g_pGameStudioRenderer;
				for (int i = 0; i < 4; ++i)
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
						gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = ctx->index;
					}
				}

				if (gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);
		}

		if (gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index == 0)
			gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = 8;

		gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index - 1;

		gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))vftable[gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index];

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
	bv_scale = gEngfuncs.pfnRegisterVariable("bv_scale", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_enable = gEngfuncs.pfnRegisterVariable("bv_enable", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_syncview = gEngfuncs.pfnRegisterVariable("bv_syncview", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

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

		if (IsEntityDeadPlayer(ent))
		{
			int playerindex = (int)ent->curstate.renderamt;

			gPhysicsManager.ChangeRagdollEntIndex(playerindex, ent->index);
		}
		else if (IsEntityBarnacle(ent))
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

	if (type == ET_PLAYER && ent->model)
	{
		gCorpseManager.SetPlayerEmitted(ent->index);
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

	auto levelname = gEngfuncs.pfnGetLevelName();
	if (levelname && levelname[0])
	{
		gPhysicsManager.SetGravity(cl_gravity);
		gPhysicsManager.UpdateTempEntity(ppTempEntActive, frametime, client_time);
		gPhysicsManager.StepSimulation(frametime);
	}
}

void HUD_Frame(double frametime)
{
	gExportfuncs.HUD_Frame(frametime);

	gCorpseManager.ClearAllPlayerEmitState();
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

	auto local = gEngfuncs.GetLocalPlayer();
	if (local && local->player && bv_syncview->value)
	{
		auto spectating_player = local;

		if (g_iUser1 && g_iUser2 && (*g_iUser1))
		{
			spectating_player = gEngfuncs.GetEntityByIndex(*g_iUser2);
		}

		if (!CL_IsFirstPersonMode(spectating_player))
		{
			auto ragdoll = gPhysicsManager.FindRagdoll(spectating_player->index);

			if (ragdoll && ragdoll->m_iActivityType != 0)
			{
				vec3_t save_simorg;
				vec3_t save_origin;

				VectorCopy(pparams->simorg, save_simorg);
				VectorCopy(spectating_player->origin, save_origin);

				gPhysicsManager.SyncThirdPersonView(ragdoll, spectating_player, pparams);

				gExportfuncs.V_CalcRefdef(pparams);

				VectorCopy(save_origin, spectating_player->origin);
				VectorCopy(save_simorg, pparams->simorg);

				return;
			}
		}
		else
		{
			auto ragdoll = gPhysicsManager.FindRagdoll(spectating_player->index);

			if (ragdoll && ragdoll->m_iActivityType != 0)
			{
				vec3_t save_simorg;
				vec3_t save_cl_viewangles;
				int save_health = pparams->health;
				//vec3_t save_origin;

				VectorCopy(pparams->simorg, save_simorg);
				VectorCopy(pparams->cl_viewangles, save_cl_viewangles);
				//VectorCopy(spectating_player->origin, save_origin);
				
				gPhysicsManager.SyncFirstPersonView(ragdoll, spectating_player, pparams);

				gExportfuncs.V_CalcRefdef(pparams);

				//VectorCopy(save_origin, spectating_player->origin);
				VectorCopy(save_simorg, pparams->simorg);
				VectorCopy(save_cl_viewangles, pparams->cl_viewangles);
				pparams->health = save_health;

				return;
			}
		}
	}

	gExportfuncs.V_CalcRefdef(pparams);
}

void HUD_DrawNormalTriangles(void)
{
	gExportfuncs.HUD_DrawNormalTriangles();

	gPhysicsManager.DebugDraw();
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
			if (IsEntityPresent(viewplayer) &&
				!gCorpseManager.IsPlayerEmitted(viewplayer->index) &&
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

				gEngfuncs.CL_CreateVisibleEntity(ET_PLAYER, viewplayer);
			}
		}
	}
}