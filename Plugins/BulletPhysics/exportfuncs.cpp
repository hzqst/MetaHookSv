#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <cl_entity.h>
#include <com_model.h>
#include <triangleapi.h>
#include <event_api.h>
#include <cvardef.h>
#include <entity_types.h>
#include <pm_defs.h>

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

cvar_t *chase_active = NULL;
cvar_t* sv_cheats = NULL;

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;
bool g_bIsDayOfDefeat = false;

int g_iRagdollRenderEntIndex = 0;
int g_iRagdollRenderFlags = 0;
bool g_bIsUpdatingRefdef = false;

int g_iPlayerFlags = 0;

ref_params_t r_params = { 0 };

cl_entity_t* r_worldentity = NULL;
model_t** cl_worldmodel = NULL;

int* cl_max_edicts = NULL;
cl_entity_t** cl_entities = NULL;

int* cl_numvisedicts = NULL;
cl_entity_t** cl_visedicts = NULL;

float* r_origin = NULL;

model_t* cl_sprite_white = nullptr;

model_t* CounterStrike_RedirectPlayerModel(model_t* original_model, int PlayerNumber, int* modelindex);

bool IsDebugDrawEnabled()
{
	return bv_debug_draw->value >= 1;
}

bool IsDebugDrawWallHackEnabled()
{
	return bv_debug_draw_wallhack->value >= 1;
}

bool ShouldForceUpdateBones()
{
	return bv_force_updatebones->value >= 1;
}

int GetSyncronizeViewLevel()
{
	return (int)bv_syncview->value;
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

int ClientGetPlayerFlags()
{
	return g_iPlayerFlags;
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
	if (!(entindex >= 0 && entindex <= MAX_CLIENTS))
	{
		Sys_Error("R_GetPlayerState: Invalid index %d !", entindex);
		return nullptr;
	}

	//gamedata cl_frames is the frame_t ring base; the per-client entity states
	//live in frame_t::playerstate, so the member offset must be added explicitly.
	return ((entity_state_t *)((char *)cl_frames + size_of_frame * ((*cl_parsecount) & 63) + offsetof(frame_t, playerstate) + sizeof(entity_state_t) * entindex));
}

bool CL_IsFirstPersonMode(cl_entity_t *player)
{
	if (!gExportfuncs.CL_IsThirdPerson() && (*cl_viewentity) == player->index && !chase_active->value)
		return true;

	return false;
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

TEMPENTITY* EngineGetTempTentsBase(void)
{
	return gTempEnts;
}

TEMPENTITY* EngineGetTempTentByIndex(int index)
{
	return &gTempEnts[index];
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

	if (1)
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

	if (1)
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

void ClientStudio_FillAddress(PVOID clientBase)
{
	//The client studio renderer is optional: clients without these virtuals keep the engine-side hooks only.
	gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioDrawModel", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION, false);
	gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioDrawPlayer", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION, false);
	gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioSetupBones", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION, false);
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

void EngineStudio_InstallHooks()
{
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

}

void EngineStudio_UninstallHooks()
{
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

	cl_sprite_white = IEngineStudio.Mod_ForName("sprites/white.spr", 1);

	pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();
	plighttransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();

	EngineStudio_InstallHooks();

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	ClientStudio_FillAddress(g_ClientDLLInfo.ImageBase);
	ClientStudio_InstallHooks();

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

	if (g_pViewPort) {
		g_pViewPort->Init();
	}
}

void R_NewMap(void)
{
	gPrivateFuncs.R_NewMap();
	ClientPhysicManager()->NewMap();
	ClientEntityManager()->NewMap();

	if (g_pViewPort) {
		g_pViewPort->NewMap();
	}
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
	if (g_pViewPort) {
		g_pViewPort->UpdateInspectStuffs();
	}

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
		if (GetSyncronizeViewLevel() >= 1)
		{
			auto pSpectatingPlayer = pLocalPlayer;

			if (g_iUser1 && g_iUser2 && (*g_iUser1))
			{
				pSpectatingPlayer = gEngfuncs.GetEntityByIndex((*g_iUser2));
			}

			auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(pSpectatingPlayer->index);

			if (pPhysicObject)
			{
				if (pPhysicObject->CalcRefDef(pparams, !CL_IsFirstPersonMode(pSpectatingPlayer) ? true : false, GetSyncronizeViewLevel(), gExportfuncs.V_CalcRefdef))
				{
					return;
				}
			}
		}
	}
skip:
	gExportfuncs.V_CalcRefdef(pparams);
}

void HUD_PostRunCmd(struct local_state_s* from, struct local_state_s* to, struct usercmd_s* cmd, int runfuncs, double time, unsigned int random_seed)
{
	gExportfuncs.HUD_PostRunCmd(from, to, cmd, runfuncs, time, random_seed);

	g_iPlayerFlags = to->client.flags;
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