#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <capstone.h>
#include <cl_entity.h>
#include <com_model.h>
#include <triangleapi.h>
#include <event_api.h>
#include <cvardef.h>
#include <entity_types.h>
#include <pm_defs.h>

#include <set>
#include <vector>

#include "mathlib2.h"
#include "util.h"
#include "plugins.h"
#include "enginedef.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "message.h"
#include "ClientEntityManager.h"
#include "ClientPhysicManager.h"
#include "Viewport.h"

static hook_t *g_phook_GameStudioRenderer_StudioSetupBones = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioDrawPlayer = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioDrawModel = NULL;
static hook_t *g_phook_R_StudioSetupBones = NULL;
static hook_t *g_phook_R_StudioDrawPlayer = NULL;
static hook_t *g_phook_R_StudioDrawModel = NULL;
static hook_t *g_phook_studioapi_StudioCheckBBox = NULL;
static hook_t *g_phook_efxapi_R_TempModel = NULL;

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

cvar_t *bv_debug_draw = NULL;
cvar_t* bv_debug_draw_wallhack = NULL;
cvar_t* bv_debug_draw_level_ragdoll = NULL;
cvar_t* bv_debug_draw_level_static = NULL;
cvar_t* bv_debug_draw_level_dynamic = NULL;
cvar_t* bv_debug_draw_level_rigidbody = NULL;
cvar_t* bv_debug_draw_level_constraint = NULL;
cvar_t* bv_debug_draw_level_behavior = NULL;
cvar_t* bv_debug_draw_constraint_color = NULL;
cvar_t* bv_debug_draw_behavior_color = NULL;
cvar_t* bv_debug_draw_inspected_color = NULL;
cvar_t* bv_debug_draw_selected_color = NULL;

cvar_t *bv_simrate = NULL;
cvar_t *bv_syncview = NULL;
cvar_t* bv_force_updatebones = NULL;
//cvar_t *bv_ragdoll_sleepaftertime = NULL;
//cvar_t *bv_ragdoll_sleeplinearvel = NULL;
//cvar_t *bv_ragdoll_sleepangularvel = NULL;

cvar_t *chase_active = NULL;
cvar_t* sv_cheats = NULL;

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;
bool g_bIsDayOfDefeat = false;

int g_iRagdollRenderEntIndex = 0;
int g_iRagdollRenderFlags = 0;
bool g_bIsUpdatingRefdef = false;

ref_params_t r_params = { 0 };

model_t* r_worldmodel = NULL;
cl_entity_t* r_worldentity = NULL;

int* cl_max_edicts = NULL;
cl_entity_t** cl_entities = NULL;

int* cl_numvisedicts = NULL;
cl_entity_t** cl_visedicts = NULL;

float* g_ChromeOrigin = NULL;
float* r_origin = NULL;

model_t* CounterStrike_RedirectPlayerModel(model_t* original_model, int PlayerNumber, int* modelindex);

bool IsDebugDrawEnabled()
{
	return bv_debug_draw->value >= 1;
}

bool IsDebugDrawWallHackEnabled()
{
	return bv_debug_draw_wallhack->value >= 1;
}

bool ShouldSyncronizeView()
{
	return bv_syncview->value >= 1;
}

bool ShouldForceUpdateBones()
{
	return bv_force_updatebones->value >= 1;
}

float GetSimulationTickRate()
{
	return bv_simrate->value;
}

bool R_IsRenderingPortals()
{
	return g_bRenderingPortals_SCClient && (*g_bRenderingPortals_SCClient);
}

float* EngineGetRendererViewOrigin()
{
	return r_origin;
}

bool AllowCheats()
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		return (*allow_cheats) != 0;
	}

	return (sv_cheats->value != 0) ? true : false;
}

entity_state_t *R_GetPlayerState(int entindex)
{
	return ((entity_state_t *)((char *)cl_frames + size_of_frame * ((*cl_parsecount) & 63) + sizeof(entity_state_t) * entindex));
}

bool CL_IsFirstPersonMode(cl_entity_t *player)
{
	if (!gExportfuncs.CL_IsThirdPerson() && (*cl_viewentity) == player->index && !chase_active->value)
		return true;

	return false;
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

int EngineGetMaxClientEdicts(void)
{
	return (*cl_max_edicts);
}

cl_entity_t* EngineGetClientEntitiesBase(void)
{
	return (*cl_entities);
}

int EngineGetMaxTempEnts(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return MAX_TEMP_ENTITIES_SVENGINE;

	return MAX_TEMP_ENTITIES;
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
	auto start = (model_t*)(mod_known);
	auto end = (model_t*)(mod_known);
	end  += (*mod_numknown);

	if (mod >= start && mod < end)
	{
		int index = (mod - (model_t*)(mod_known));

		return index;
	}

	return -1;
}

model_t *EngineGetModelByIndex(int index)
{
	auto pmod_known = (model_t *)(mod_known);
	
	if (index >= 0 && index < EngineGetNumKnownModel())
		return &pmod_known[index];

	return NULL;
}

model_t* EngineFindWorldModelBySubModel(model_t* psubmodel)
{
	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_brush && mod->name[0] && mod->name[0] != '*')
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				if (mod->vertexes == psubmodel->vertexes)
					return mod;
			}
		}
	}
	return nullptr;
}

/*
	Purpose : StudioSetupBones hook handler
*/

template<typename CallType>
__forceinline void StudioSetupBones_Template(CallType pfnSetupBones, void* pthis = nullptr, int dummy = 0)
{
	CRagdollObjectSetupBoneContext Context;

	Context.m_studiohdr = (*pstudiohdr);
	Context.m_entindex = g_iRagdollRenderEntIndex;
	Context.m_flags = g_iRagdollRenderFlags;

	if (g_iRagdollRenderEntIndex > 0 && ClientPhysicManager()->SetupBones(&Context))
		return;

	pfnSetupBones(pthis, dummy);

	if (g_iRagdollRenderEntIndex > 0 && ClientPhysicManager()->SetupJiggleBones(&Context))
		return;
}

/*
	Purpose : wrapper to call engine StudioSetupBones
*/

__forceinline void R_StudioSetupBones_originalcall_wrapper(void* pthis, int dummy)
{
	return gPrivateFuncs.R_StudioSetupBones();
}

/*
	Purpose : Engine StudioSetupBones hook handler
*/

void R_StudioSetupBones(void)
{
	return StudioSetupBones_Template(R_StudioSetupBones_originalcall_wrapper);
}

/*
	Purpose : ClientDLL StudioSetupBones hook handler
*/

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
	if (ClientEntityManager()->IsEntityDeadPlayer((*currententity)))
	{
		return pfnDrawModel(pthis, 0, flags);
	}

	if (flags & STUDIO_RAGDOLL_SETUP_BONES)
	{
		return pfnDrawModel(pthis, 0, 0);
	}

	if (flags & STUDIO_RAGDOLL_UPDATE_BONES)
	{
		int entindex = ClientEntityManager()->GetEntityIndex((*currententity));

		g_iRagdollRenderEntIndex = entindex;
		g_iRagdollRenderFlags = flags;

		int result = pfnDrawModel(pthis, 0, 0);

		g_iRagdollRenderEntIndex = 0;
		g_iRagdollRenderFlags = 0;

		return result;
	}

	if (flags & STUDIO_RENDER)
	{
		int entindex = ClientEntityManager()->GetEntityIndex((*currententity));

		auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);
		
		if (pPhysicObject && pPhysicObject->IsRagdollObject())
		{
			auto pRagdollObject = (IRagdollObject*)pPhysicObject;

			if (pRagdollObject->GetActivityType() == StudioAnimActivityType_Death || pRagdollObject->GetActivityType() == StudioAnimActivityType_CaughtByBarnacle)
			{
				g_iRagdollRenderEntIndex = entindex;
				g_iRagdollRenderFlags = flags;

				vec3_t vecSavedOrigin;
				VectorCopy((*currententity)->origin, vecSavedOrigin);

				pRagdollObject->GetGoldSrcOriginAngles((*currententity)->origin, nullptr);

				int result = pfnDrawModel(pthis, 0, flags);

				VectorCopy(vecSavedOrigin, (*currententity)->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderFlags = 0;

				return result;
			}
			else
			{
				g_iRagdollRenderEntIndex = entindex;
				g_iRagdollRenderFlags = flags;

				int result = pfnDrawModel(pthis, 0, flags);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderFlags = 0;

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
	int entindex = ClientEntityManager()->GetEntityIndex((*currententity));
	int playerindex = pplayer->number;

	if (flags & STUDIO_RAGDOLL_SETUP_BONES)
	{
		return pfnDrawPlayer(pthis, 0, 0, pplayer);
	}

	if (flags & STUDIO_RAGDOLL_UPDATE_BONES)
	{
		g_iRagdollRenderEntIndex = entindex;
		g_iRagdollRenderFlags = flags;

		int result = pfnDrawPlayer(pthis, 0, 0, pplayer);

		g_iRagdollRenderEntIndex = 0;
		g_iRagdollRenderFlags = 0;

		return result;
	}

	if (flags & STUDIO_RENDER)
	{
		auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

		if (g_bIsCounterStrike)
		{
			//Counter-Strike redirects playermodel in a pretty tricky way
			int modelindex = 0;
			model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
		}

		auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);

		if (pPhysicObject && pPhysicObject->IsRagdollObject())
		{
			auto pRagdollObject = (IRagdollObject*)pPhysicObject;

			if (pRagdollObject->GetActivityType() == StudioAnimActivityType_Death || pRagdollObject->GetActivityType() == StudioAnimActivityType_CaughtByBarnacle)
			{
				g_iRagdollRenderEntIndex = entindex;
				g_iRagdollRenderFlags = flags;

				vec3_t vecSavedOrigin;
				VectorCopy((*currententity)->origin, vecSavedOrigin);

				pRagdollObject->GetGoldSrcOriginAngles((*currententity)->origin, nullptr);

				int iSavedWeaponModel = pplayer->weaponmodel;

				pplayer->weaponmodel = 0;

				int result = pfnDrawPlayer(pthis, 0, flags, pplayer);

				pplayer->weaponmodel = iSavedWeaponModel;

				VectorCopy(vecSavedOrigin, (*currententity)->origin);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderFlags = 0;

				return result;
			}
			else
			{
				g_iRagdollRenderEntIndex = entindex;
				g_iRagdollRenderFlags = flags;

				int result = pfnDrawPlayer(pthis, 0, flags, pplayer);

				g_iRagdollRenderEntIndex = 0;
				g_iRagdollRenderFlags = 0;

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

int studioapi_StudioCheckBBox()
{
	int nVisible = 0;
	int entindex = ClientEntityManager()->GetEntityIndex((*currententity));

	if (ClientPhysicManager()->StudioCheckBBox((*pstudiohdr), entindex, &nVisible))
	{
		return nVisible;
	}

	return gPrivateFuncs.studioapi_StudioCheckBBox();
}

qboolean R_CullBox(vec3_t mins, vec3_t maxs)
{
	return gPrivateFuncs.R_CullBox(mins, maxs);
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

	if (1)
	{
		typedef struct
		{
			int r_origin_candidate_count;
			PVOID r_origin_candidate[3];
			int g_ChromeOrigin_candidate_count;
			PVOID g_ChromeOrigin_candidate[3];
		}SetChromeOrigin_ctx;

		SetChromeOrigin_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(pstudio->SetChromeOrigin, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (SetChromeOrigin_ctx*)context;

				if (pinst->id == X86_INS_FLD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//D9 05 00 40 F5 03 fld     r_origin
					if (ctx->r_origin_candidate_count < 3)
					{
						ctx->r_origin_candidate[ctx->r_origin_candidate_count] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
						ctx->r_origin_candidate_count++;
					}
				}
				else if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					if (ctx->r_origin_candidate_count < 3)
					{
						ctx->r_origin_candidate[ctx->r_origin_candidate_count] = (PVOID)pinst->detail->x86.operands[1].mem.disp;
						ctx->r_origin_candidate_count++;
					}
				}
				else if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					if (ctx->r_origin_candidate_count < 3)
					{
						ctx->r_origin_candidate[ctx->r_origin_candidate_count] = (PVOID)pinst->detail->x86.operands[1].imm;
						ctx->r_origin_candidate_count++;
					}
				}
				else if (pinst->id == X86_INS_MOVQ &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					if (ctx->r_origin_candidate_count < 3)
					{
						ctx->r_origin_candidate[ctx->r_origin_candidate_count] = (PVOID)pinst->detail->x86.operands[1].mem.disp;
						ctx->r_origin_candidate_count++;
					}
				}

				else if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{//A3 40 88 35 02 mov     g_ChromeOrigin, eax
					if (ctx->g_ChromeOrigin_candidate_count < 3)
					{
						ctx->g_ChromeOrigin_candidate[ctx->g_ChromeOrigin_candidate_count] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
						ctx->g_ChromeOrigin_candidate_count++;
					}
				}
				else if (pinst->id == X86_INS_MOVQ &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{//A3 40 88 35 02 mov     g_ChromeOrigin, eax
					if (ctx->g_ChromeOrigin_candidate_count < 3)
					{
						ctx->g_ChromeOrigin_candidate[ctx->g_ChromeOrigin_candidate_count] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
						ctx->g_ChromeOrigin_candidate_count++;
					}
				}
				else if (pinst->id == X86_INS_FSTP &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//D9 1D A0 39 DB 08 fstp    g_ChromeOrigin
					if (ctx->g_ChromeOrigin_candidate_count < 3)
					{
						ctx->g_ChromeOrigin_candidate[ctx->g_ChromeOrigin_candidate_count] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
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
			std::qsort(ctx.r_origin_candidate, ctx.r_origin_candidate_count, sizeof(ctx.r_origin_candidate[0]), [](const void* a, const void* b) {
				return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
				});
			r_origin = (decltype(r_origin))ctx.r_origin_candidate[0];
		}

		if (ctx.g_ChromeOrigin_candidate_count >= 2)
		{
			std::qsort(ctx.g_ChromeOrigin_candidate, ctx.g_ChromeOrigin_candidate_count, sizeof(ctx.g_ChromeOrigin_candidate[0]), [](const void* a, const void* b) {
				return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
				});
			g_ChromeOrigin = (decltype(g_ChromeOrigin))ctx.g_ChromeOrigin_candidate[0];
		}

		Sig_VarNotFound(r_origin);
		Sig_VarNotFound(g_ChromeOrigin);
	}

	auto StudioDrawPlayerThunk = (PUCHAR)(*ppinterface)->StudioDrawPlayer;
	if (StudioDrawPlayerThunk > g_dwClientTextBase && StudioDrawPlayerThunk < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		if (StudioDrawPlayerThunk[0] == 0xE9)
		{
			StudioDrawPlayerThunk = (PUCHAR)GetCallAddress(StudioDrawPlayerThunk);
		}

		g_pMetaHookAPI->DisasmRanges(StudioDrawPlayerThunk, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

	}

	auto StudioDrawModelThunk = (PUCHAR)(*ppinterface)->StudioDrawModel;
	if (StudioDrawModelThunk > g_dwClientTextBase && StudioDrawModelThunk < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		if (StudioDrawModelThunk[0] == 0xE9)
		{
			StudioDrawModelThunk = (PUCHAR)GetCallAddress(StudioDrawModelThunk);
		}

		g_pMetaHookAPI->DisasmRanges(StudioDrawModelThunk, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
				bool bFoundD4h;
				bool bFoundD8h;
			}StudioCalcAttachments_SearchContext;

			StudioCalcAttachments_SearchContext ctx;

			ctx.base = (void*)vftable[i];
			ctx.index = i;
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
						(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwClientBase &&
						(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwClientBase + g_dwClientSize)
					{
						const char* pPushedString = (const char*)pinst->detail->x86.operands[0].imm;
						if (0 == memcmp(pPushedString, "Too many attachments on %s\n", sizeof("Too many attachments on %s\n") - 1))
						{
							gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index = ctx->index;
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
		}

		if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index)
			gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawModel_vftable_index];

		if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index)
			gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer_vftable_index];

		if(gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index)
			gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer))vftable[gPrivateFuncs.GameStudioRenderer__StudioDrawPlayer_vftable_index];

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
				
				if (gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index)
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

		Sig_FuncNotFound(GameStudioRenderer_StudioRenderModel_vftable_index);

		gPrivateFuncs.GameStudioRenderer_StudioRenderModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderModel))vftable[gPrivateFuncs.GameStudioRenderer_StudioRenderModel_vftable_index];

		g_pMetaHookAPI->DisasmRanges((void*)gPrivateFuncs.GameStudioRenderer_StudioRenderModel, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
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

		gPrivateFuncs.GameStudioRenderer_StudioRenderFinal = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderFinal))vftable[gPrivateFuncs.GameStudioRenderer_StudioRenderFinal_vftable_index];

		gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index - 1;
		gPrivateFuncs.GameStudioRenderer_StudioSaveBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 1;
		gPrivateFuncs.GameStudioRenderer_StudioMergeBones_vftable_index = gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments_vftable_index + 2;

		gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))vftable[gPrivateFuncs.GameStudioRenderer_StudioSetupBones_vftable_index];
		gPrivateFuncs.GameStudioRenderer_StudioMergeBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioMergeBones))vftable[gPrivateFuncs.GameStudioRenderer_StudioMergeBones_vftable_index];

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

		gPrivateFuncs.R_StudioSetupBones = (decltype(gPrivateFuncs.R_StudioSetupBones))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bip01_PushString, 0x1000, [](PUCHAR Candidate) {
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

	}
	else
	{
		Sys_Error("Failed to locate g_pGameStudioRenderer or EngineStudioRenderer!\n");
	}
}

void ClientStudio_InstallHooks()
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
}

void EngineStudio_InstallHooks()
{
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

	/*
		Engine studio interface
	*/

	if (gPrivateFuncs.studioapi_StudioCheckBBox)
	{
		Install_InlineHook(studioapi_StudioCheckBBox);
	}
}

void ClientStudio_UninstallHooks()
{
	/*
		Client studio implementation
	*/

	if (gPrivateFuncs.GameStudioRenderer_StudioSetupBones)
	{
		Uninstall_Hook(GameStudioRenderer_StudioSetupBones);
	}

	if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer)
	{
		Uninstall_Hook(GameStudioRenderer_StudioDrawPlayer);
	}

	if (gPrivateFuncs.GameStudioRenderer_StudioDrawModel)
	{
		Uninstall_Hook(GameStudioRenderer_StudioDrawModel);
	}
}

void EngineStudio_UninstallHooks()
{
	/*
		Engine studio implementation
	*/

	if (gPrivateFuncs.R_StudioSetupBones)
	{
		Uninstall_Hook(R_StudioSetupBones);
	}

	if (gPrivateFuncs.R_StudioDrawPlayer)
	{
		Uninstall_Hook(R_StudioDrawPlayer);
	}

	if (gPrivateFuncs.R_StudioDrawModel)
	{
		Uninstall_Hook(R_StudioDrawModel);
	}

	/*
		Engine studio interface
	*/

	if (gPrivateFuncs.studioapi_StudioCheckBBox)
	{
		Uninstall_Hook(studioapi_StudioCheckBBox);
	}
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	gPrivateFuncs.studioapi_StudioCheckBBox = pstudio->StudioCheckBBox;

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	EngineStudio_FillAddress(version, ppinterface, pstudio);
	ClientStudio_InstallHooks();
	EngineStudio_InstallHooks();

	return result;
}

void BV_OpenDebugUI_f(void)
{
	if (AllowCheats())
	{
		g_pViewPort->OpenPhysicDebugGUI();
	}
}

void BV_ReloadObjects_f(void)
{
	if (AllowCheats())
	{
		ClientPhysicManager()->RemoveAllPhysicObjects(PhysicObjectFlag_AnyObject, PhysicObjectFlag_FromBSP);
	}
}

void BV_ReloadConfigs_f(void)
{
	if (AllowCheats())
	{
		ClientPhysicManager()->FreeAllIndexArrays(PhysicIndexArrayFlag_FromExternal, PhysicIndexArrayFlag_FromBSP);
		ClientPhysicManager()->RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromConfig, 0);
		ClientPhysicManager()->LoadPhysicObjectConfigs();
	}
}

void BV_ReloadAll_f(void)
{
	if (AllowCheats())
	{
		BV_ReloadConfigs_f();
		BV_ReloadObjects_f();
	}
}

void BV_SaveConfigs_f(void)
{
	if (AllowCheats())
	{
		ClientPhysicManager()->SavePhysicObjectConfigs();
	}
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	ClientPhysicManager()->Init();

	bv_debug_draw = gEngfuncs.pfnRegisterVariable("bv_debug_draw", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_wallhack = gEngfuncs.pfnRegisterVariable("bv_debug_draw_wallhack", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_level_static = gEngfuncs.pfnRegisterVariable("bv_debug_draw_level_static", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_level_dynamic = gEngfuncs.pfnRegisterVariable("bv_debug_draw_level_dynamic", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_level_ragdoll = gEngfuncs.pfnRegisterVariable("bv_debug_draw_level_ragdoll", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_level_rigidbody = gEngfuncs.pfnRegisterVariable("bv_debug_draw_level_rigidbody", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_level_constraint = gEngfuncs.pfnRegisterVariable("bv_debug_draw_level_constraint", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_level_behavior = gEngfuncs.pfnRegisterVariable("bv_debug_draw_level_behavior", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_constraint_color = gEngfuncs.pfnRegisterVariable("bv_debug_draw_constraint_color", "54 136 255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_behavior_color = gEngfuncs.pfnRegisterVariable("bv_debug_draw_behavior_color", "95 255 117", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_inspected_color = gEngfuncs.pfnRegisterVariable("bv_debug_draw_inspected_color", "0 255 255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_debug_draw_selected_color = gEngfuncs.pfnRegisterVariable("bv_debug_draw_selected_color", "255 255 0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	bv_simrate = gEngfuncs.pfnRegisterVariable("bv_simrate", "64", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_syncview = gEngfuncs.pfnRegisterVariable("bv_syncview", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_force_updatebones = gEngfuncs.pfnRegisterVariable("bv_force_updatebones", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	//bv_ragdoll_sleepaftertime = gEngfuncs.pfnRegisterVariable("bv_ragdoll_sleepaftertime", "3", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	//bv_ragdoll_sleeplinearvel = gEngfuncs.pfnRegisterVariable("bv_ragdoll_sleeplinearvel", "5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	//bv_ragdoll_sleepangularvel = gEngfuncs.pfnRegisterVariable("bv_ragdoll_sleepangularvel", "3", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	sv_cheats = gEngfuncs.pfnGetCvarPointer("sv_cheats");
	chase_active = gEngfuncs.pfnGetCvarPointer("chase_active");
	cl_minmodels = gEngfuncs.pfnGetCvarPointer("cl_minmodels");
	cl_min_ct = gEngfuncs.pfnGetCvarPointer("cl_min_ct");
	cl_min_t = gEngfuncs.pfnGetCvarPointer("cl_min_t");

	gEngfuncs.pfnAddCommand("bv_open_debug_ui", BV_OpenDebugUI_f);

	gEngfuncs.pfnAddCommand("bv_reload_all", BV_ReloadAll_f);
	gEngfuncs.pfnAddCommand("bv_reload_objects", BV_ReloadObjects_f);
	gEngfuncs.pfnAddCommand("bv_reload_configs", BV_ReloadConfigs_f);
	gEngfuncs.pfnAddCommand("bv_save_configs", BV_SaveConfigs_f);

	//For ClCorpse hook

	m_pfnClCorpse = HOOK_MESSAGE(ClCorpse);

	if (m_pfnClCorpse)
	{
		gPrivateFuncs.efxapi_R_TempModel = gEngfuncs.pEfxAPI->R_TempModel;
		Install_InlineHook(efxapi_R_TempModel);
	}

	g_pViewPort->Init();
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

void HUD_Frame(double frametime)
{
	gExportfuncs.HUD_Frame(frametime);

	ClientEntityManager()->ClearEntityEmitStates();
}

int HUD_AddEntity(int type, cl_entity_t* ent, const char* model)
{
	return gExportfuncs.HUD_AddEntity(type, ent, model);
}

void HUD_CreateEntities(void)
{
	g_pViewPort->UpdateInspectStuffs();

	gExportfuncs.HUD_CreateEntities();

	auto localplayer = gEngfuncs.GetLocalPlayer();

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		auto state = R_GetPlayerState(i);

		if (state->messagenum != (*cl_parsecount))
			continue;

		if (!state->modelindex || (state->effects & EF_NODRAW))
			continue;

		auto entindex = state->number;
		auto ent = gEngfuncs.GetEntityByIndex(entindex);

		ClientEntityManager()->SetEntityEmitted(ent);

		ClientPhysicManager()->CreatePhysicObjectForEntity(ent, state, gEngfuncs.hudGetModelByIndex(state->modelindex));
	}

	for (int entindex = 0; entindex < EngineGetMaxClientEdicts(); ++entindex)
	{
		auto ent = gEngfuncs.GetEntityByIndex(entindex);

		if (ent->curstate.number >= 1 && ent->curstate.number <= gEngfuncs.GetMaxClients())
			continue;

		if (!ent->curstate.modelindex || (ent->curstate.effects & EF_NODRAW))
			continue;

		if (!ent->model)
			continue;

		if (!(ent->curstate.entityType & ENTITY_NORMAL))
			continue;

		if (ent->curstate.messagenum != (*cl_parsecount))
			continue;

		//CL_ProcessEntityUpdate not called yet..?
		if (ent->curstate.number != entindex)
			continue;

		ClientEntityManager()->SetEntityEmitted(ent);

		ClientPhysicManager()->CreatePhysicObjectForEntity(ent, &ent->curstate, ent->model);
	}
}

//double g_frametime{};
//double g_client_time{};

void HUD_TempEntUpdate(
	double frametime,   // Simulation time
	double client_time, // Absolute time on client
	double cl_gravity,  // True gravity on client
	TEMPENTITY** ppTempEntFree,   // List of freed temporary ents
	TEMPENTITY** ppTempEntActive, // List 
	int(*Callback_AddVisibleEntity)(cl_entity_t* pEntity),
	void(*Callback_TempEntPlaySound)(TEMPENTITY* pTemp, float damp))
{
	gExportfuncs.HUD_TempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);

	auto pTemp = (*ppTempEntActive);

	while (pTemp)
	{
		auto ent = &pTemp->entity;

		if (ent->model)
		{
			ClientEntityManager()->SetEntityEmitted(ent);

			ClientPhysicManager()->CreatePhysicObjectForEntity(ent, &ent->curstate, ent->model);
		}
		pTemp = pTemp->next;
	}

	ClientPhysicManager()->SetGravity(cl_gravity);
	//g_frametime = frametime;
	//g_client_time = client_time;

	//Force Sven Client to update LocalPlayer's pitch for us
	if (g_bIsSvenCoop && gExportfuncs.CL_IsThirdPerson())
	{
		struct pitchdrift_t saved_pitchdrift = (*g_pitchdrift);

		//TODO use ClientDLL_CAM_Think instead?
		g_bIsUpdatingRefdef = true;
		gExportfuncs.CAM_Think();
		V_RenderView();
		g_bIsUpdatingRefdef = false;

		(*g_pitchdrift) = saved_pitchdrift;
	}

	ClientPhysicManager()->UpdateAllPhysicObjects(ppTempEntFree, ppTempEntActive, frametime, client_time, cl_gravity);
	ClientPhysicManager()->StepSimulation(frametime);
}

void V_CalcRefdef(struct ref_params_s* pparams)
{
	memcpy(&r_params, pparams, sizeof(r_params));
	auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

	if (pparams->intermission)
		goto skip;

	if (pparams->paused)
		goto skip;

	if (pparams->nextView)
		goto skip;

	if (R_IsRenderingPortals())
		goto skip;

	if (g_bIsUpdatingRefdef)
		goto skip;

	if (pLocalPlayer && pLocalPlayer->player)
	{
		if (ShouldSyncronizeView())
		{
			auto pSpectatingPlayer = pLocalPlayer;

			if (g_iUser1 && g_iUser2 && (*g_iUser1))
			{
				pSpectatingPlayer = gEngfuncs.GetEntityByIndex((*g_iUser2));
			}

			auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(pSpectatingPlayer->index);

			if (pPhysicObject)
			{
				if (pPhysicObject->CalcRefDef(pparams, !CL_IsFirstPersonMode(pSpectatingPlayer) ? true : false, gExportfuncs.V_CalcRefdef))
				{
					return;
				}
			}
		}
	}
skip:
	gExportfuncs.V_CalcRefdef(pparams);
}

void V_RenderView(void)
{
	gPrivateFuncs.V_RenderView();
}

void R_RenderView()
{
	if (g_bIsUpdatingRefdef)
		return;

	gPrivateFuncs.R_RenderView();
}

void R_RenderView_SvEngine(int viewIdx)
{
	if (g_bIsUpdatingRefdef)
		return;
#if 0
	if (g_bIsSvenCoop && gExportfuncs.CL_IsThirdPerson())
	{
		ClientPhysicManager()->UpdateAllPhysicObjects(0, 0, g_frametime, g_client_time);
		ClientPhysicManager()->StepSimulation(g_frametime);
	}
#endif
	gPrivateFuncs.R_RenderView_SvEngine(viewIdx);
}

void HUD_DrawTransparentTriangles(void)
{
	gExportfuncs.HUD_DrawTransparentTriangles();

	if (AllowCheats() && IsDebugDrawEnabled())
	{
		ClientPhysicManager()->DebugDraw();
	}
}

void HUD_Shutdown(void)
{
	gExportfuncs.HUD_Shutdown();

	ClientPhysicManager()->Shutdown();

	ClientStudio_UninstallHooks();
	EngineStudio_UninstallHooks();

	Uninstall_Hook(efxapi_R_TempModel);
}
