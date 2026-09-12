#include <metahook.h>

#include "enginedef.h"
#include "plugins.h"
#include "privatehook.h"
#include "exportfuncs.h"
#include "message.h"

#include "ClientPhysicManager.h"
#include "ClientEntityManager.h"
#include "Viewport.h"

static_assert(METAHOOK_API_VERSION >= 112, "BulletPhysics resolves all game-private symbols from gamedata (FUNCTION/GLOBAL/VIRTUAL_FUNCTION/scalar) and requires MetaHook API 112");

private_funcs_t gPrivateFuncs = {0};

studiohdr_t** pstudiohdr = NULL;
void* g_pGameStudioRenderer = NULL;
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

bool* g_bRenderingPortals_SCClient = NULL;
int* g_ViewEntityIndex_SCClient = NULL;

struct pitchdrift_t* g_pitchdrift = NULL;

int* g_iUser1 = NULL;
int* g_iUser2 = NULL;

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

static hook_t* g_phook_R_NewMap = NULL;
static hook_t* g_phook_R_RenderView_SvEngine = NULL;
static hook_t* g_phook_R_RenderView = NULL;

PVOID GamedataResolvePtr(PVOID moduleBase, const char* symbolName, mh_gamesymbol_kind_t kind, bool required)
{
	PVOID address = nullptr;
	mh_gamesymbol_status_t status = g_pMetaHookAPI->ResolveGameSymbol(moduleBase, symbolName, kind, &address);

	if (status != MH_GAMESYMBOL_OK)
	{
		if (required)
		{
			Sys_Error("Could not resolve gamedata symbol: %s (%s)",
				symbolName, g_pMetaHookAPI->GetGameSymbolStatusString(status));
		}

		return nullptr;
	}

	return address;
}

uint32_t GamedataResolveScalar(PVOID moduleBase, const char* symbolName, bool required)
{
	uint32_t value = 0;
	mh_gamesymbol_status_t status = g_pMetaHookAPI->QueryGameSymbolScalar(moduleBase, symbolName, &value);

	if (status != MH_GAMESYMBOL_OK)
	{
		if (required)
		{
			Sys_Error("Could not resolve gamedata scalar: %s (%s)",
				symbolName, g_pMetaHookAPI->GetGameSymbolStatusString(status));
		}

		return 0;
	}

	return value;
}

void Engine_FillAddress(PVOID engineBase)
{
	//Engine render / view functions
	gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))GamedataResolvePtr(engineBase, "R_NewMap", MH_GAMESYMBOL_KIND_FUNCTION, true);
	gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))GamedataResolvePtr(engineBase, "R_CullBox", MH_GAMESYMBOL_KIND_FUNCTION, true);
	gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))GamedataResolvePtr(engineBase, "V_RenderView", MH_GAMESYMBOL_KIND_FUNCTION, true);

	//SvEngine exposes the same entry as R_RenderView, but with an int viewIdx argument.
	PVOID R_RenderView_VA = GamedataResolvePtr(engineBase, "R_RenderView", MH_GAMESYMBOL_KIND_FUNCTION, true);

	if (g_iEngineType == ENGINE_SVENGINE)
		gPrivateFuncs.R_RenderView_SvEngine = (decltype(gPrivateFuncs.R_RenderView_SvEngine))R_RenderView_VA;
	else
		gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))R_RenderView_VA;

	//Engine Studio functions
	gPrivateFuncs.R_StudioDrawModel = (decltype(gPrivateFuncs.R_StudioDrawModel))GamedataResolvePtr(engineBase, "R_StudioDrawModel", MH_GAMESYMBOL_KIND_FUNCTION, true);
	gPrivateFuncs.R_StudioDrawPlayer = (decltype(gPrivateFuncs.R_StudioDrawPlayer))GamedataResolvePtr(engineBase, "R_StudioDrawPlayer", MH_GAMESYMBOL_KIND_FUNCTION, true);
	gPrivateFuncs.R_StudioSetupBones = (decltype(gPrivateFuncs.R_StudioSetupBones))GamedataResolvePtr(engineBase, "R_StudioSetupBones", MH_GAMESYMBOL_KIND_FUNCTION, true);

	//Engine global slots
	cl_max_edicts = (decltype(cl_max_edicts))GamedataResolvePtr(engineBase, "cl_max_edicts", MH_GAMESYMBOL_KIND_GLOBAL, true);
	cl_entities = (decltype(cl_entities))GamedataResolvePtr(engineBase, "cl_entities", MH_GAMESYMBOL_KIND_GLOBAL, true);
	gTempEnts = (decltype(gTempEnts))GamedataResolvePtr(engineBase, "gTempEnts", MH_GAMESYMBOL_KIND_GLOBAL, true);
	cl_viewentity = (decltype(cl_viewentity))GamedataResolvePtr(engineBase, "cl_viewentity", MH_GAMESYMBOL_KIND_GLOBAL, true);
	mod_known = (decltype(mod_known))GamedataResolvePtr(engineBase, "mod_known", MH_GAMESYMBOL_KIND_GLOBAL, true);
	mod_numknown = (decltype(mod_numknown))GamedataResolvePtr(engineBase, "mod_numknown", MH_GAMESYMBOL_KIND_GLOBAL, true);
	cl_frames = (decltype(cl_frames))GamedataResolvePtr(engineBase, "cl_frames", MH_GAMESYMBOL_KIND_GLOBAL, true);
	cl_parsecount = (decltype(cl_parsecount))GamedataResolvePtr(engineBase, "cl_parsecount", MH_GAMESYMBOL_KIND_GLOBAL, true);
	cl_numvisedicts = (decltype(cl_numvisedicts))GamedataResolvePtr(engineBase, "cl_numvisedicts", MH_GAMESYMBOL_KIND_GLOBAL, true);
	cl_visedicts = (decltype(cl_visedicts))GamedataResolvePtr(engineBase, "cl_visedicts", MH_GAMESYMBOL_KIND_GLOBAL, true);
	r_worldentity = (decltype(r_worldentity))GamedataResolvePtr(engineBase, "r_worldentity", MH_GAMESYMBOL_KIND_GLOBAL, true);
	cl_worldmodel = (decltype(cl_worldmodel))GamedataResolvePtr(engineBase, "cl_worldmodel", MH_GAMESYMBOL_KIND_GLOBAL, true);

	//Engine Studio global slots
	currententity = (decltype(currententity))GamedataResolvePtr(engineBase, "currententity", MH_GAMESYMBOL_KIND_GLOBAL, true);
	pstudiohdr = (decltype(pstudiohdr))GamedataResolvePtr(engineBase, "pstudiohdr", MH_GAMESYMBOL_KIND_GLOBAL, true);
	r_origin = (decltype(r_origin))GamedataResolvePtr(engineBase, "r_origin", MH_GAMESYMBOL_KIND_GLOBAL, true);

	//SvEngine-only engine global
	if (g_iEngineType == ENGINE_SVENGINE)
		allow_cheats = (decltype(allow_cheats))GamedataResolvePtr(engineBase, "allow_cheats", MH_GAMESYMBOL_KIND_GLOBAL, true);
	else
		allow_cheats = nullptr;

	//frame_t stride: a plain uint32 value for the matched engine binary.
	size_of_frame = (int)GamedataResolveScalar(engineBase, "size_of_frame", true);
}

void Client_FillAddress(PVOID clientBase)
{
	//Observer state is required by the release gate for every client-bearing game.
	g_iUser1 = (decltype(g_iUser1))GamedataResolvePtr(clientBase, "g_iUser1", MH_GAMESYMBOL_KIND_GLOBAL, true);
	g_iUser2 = (decltype(g_iUser2))GamedataResolvePtr(clientBase, "g_iUser2", MH_GAMESYMBOL_KIND_GLOBAL, true);

	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		g_bIsSvenCoop = true;

		g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))GamedataResolvePtr(clientBase, "g_bRenderingPortals_SCClient", MH_GAMESYMBOL_KIND_GLOBAL, true);
		g_ViewEntityIndex_SCClient = (decltype(g_ViewEntityIndex_SCClient))GamedataResolvePtr(clientBase, "g_ViewEntityIndex_SCClient", MH_GAMESYMBOL_KIND_GLOBAL, true);
		g_pitchdrift = (decltype(g_pitchdrift))GamedataResolvePtr(clientBase, "g_pitchdrift", MH_GAMESYMBOL_KIND_GLOBAL, true);
	}

	const char* gameDir = gEngfuncs.pfnGetGameDirectory();

	if (!strcmp(gameDir, "dod"))
	{
		g_bIsDayOfDefeat = true;
	}

	if (!strcmp(gameDir, "cstrike") || !strcmp(gameDir, "czero") || !strcmp(gameDir, "czeror"))
	{
		g_bIsCounterStrike = true;

		if (!strcmp(gameDir, "czeror"))
			g_PlayerExtraInfo_CZDS = (decltype(g_PlayerExtraInfo_CZDS))GamedataResolvePtr(clientBase, "g_PlayerExtraInfo_CZDS", MH_GAMESYMBOL_KIND_GLOBAL, true);
		else
			g_PlayerExtraInfo = (decltype(g_PlayerExtraInfo))GamedataResolvePtr(clientBase, "g_PlayerExtraInfo", MH_GAMESYMBOL_KIND_GLOBAL, true);
	}
}

void Client_InstallHooks(void)
{
	//do nothing
}

TEMPENTITY *efxapi_R_TempModel(float *pos, float *dir, float *angles, float life, int modelIndex, int soundtype)
{
	auto r = gPrivateFuncs.efxapi_R_TempModel(pos, dir, angles, life, modelIndex, soundtype);

	if (r && g_bIsCreatingClCorpse && g_iCreatingClCorpsePlayerIndex > 0 && g_iCreatingClCorpsePlayerIndex <= gEngfuncs.GetMaxClients())
	{
		r->entity.curstate.iuser4 = PhyCorpseFlag;
		r->entity.curstate.owner = g_iCreatingClCorpsePlayerIndex;
	}

	return r;
}

void Engine_InstallHook(void)
{
	Install_InlineHook(R_NewMap);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Install_InlineHook(R_RenderView_SvEngine);
	}
	else
	{
		Install_InlineHook(R_RenderView);
	}
}

void Engine_UninstallHook(void)
{
	Uninstall_Hook(R_NewMap);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Uninstall_Hook(R_RenderView_SvEngine);
	}
	else
	{
		Uninstall_Hook(R_RenderView);
	}
}
