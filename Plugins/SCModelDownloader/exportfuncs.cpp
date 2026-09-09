#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <cl_entity.h>
#include <com_model.h>
#include <cvardef.h>
#include <entity_types.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "plugins.h"
#include "SCModelDatabase.h"
#include "UtilHTTPClient.h"
#include "UtilAssetsIntegrity.h"

static cvar_t* g_pDeveloper = NULL;

cvar_t *scmodel_autodownload = NULL;
cvar_t *scmodel_downloadlatest = NULL;
cvar_t* scmodel_cdn = NULL;
cvar_t* scmodel_max_retry = NULL;

cl_enginefunc_t gEngfuncs = {0};
engine_studio_api_t IEngineStudio = { 0 };
r_studio_interface_t **gpStudioInterface = NULL;

bool SCModel_AutoDownload()
{
	return scmodel_autodownload->value >= 1 ? true : false;
}

bool SCModel_ShouldDownloadLatest()
{
	return scmodel_downloadlatest->value >= 1 ? true : false;
}

int SCModel_CDN()
{
	return (int)scmodel_cdn->value;
}

int SCModel_MaxRetry()
{
	return (int)scmodel_max_retry->value;
}

/*
	Purpose: Reload model for players that are using the specified model
*/

void SCModel_ReloadModel(const char *name)
{
	//Reload models for those players
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!stricmp((*DM_PlayerState)[i].name, name))
		{
			(*DM_PlayerState)[i].name[0] = 0;
			(*DM_PlayerState)[i].model = nullptr;
		}
	}
}

/*
	Purpose: Reload models for all players
*/

void SCModel_ReloadAllModels()
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		(*DM_PlayerState)[i].name[0] = 0;
		(*DM_PlayerState)[i].model = nullptr;
	}
}

/*
	Purpose: Rebuild the engine's model-change trigger predicate for one player.

	Both callers gate the same block on ( developer.value || !Host_IsSinglePlayerGame() )
	&& cl.players[i].model[0], then compare the player's model name, or the entity model
	when the named model is not in use. Evaluated at the caller entry, before the engine
	mutates DM_PlayerState, so it reproduces the engine's own test.
*/
static bool SCModel_IsModelChangeTriggered(int playerindex, cl_entity_t* currentEntity)
{
	const char* playerModelName = (g_iEngineType == ENGINE_SVENGINE)
		? cl_players_sc[playerindex].model
		: cl_players[playerindex].model;

	const bool usesNamedModel =
		(g_pDeveloper->value || !gPrivateFuncs.Host_IsSinglePlayerGame())
		&& playerModelName[0];

	if (usesNamedModel)
		return strcmp((*DM_PlayerState)[playerindex].name, playerModelName) != 0;

	return (*DM_PlayerState)[playerindex].model != currentEntity->model;
}

/*
	Purpose: Query the database with the state the original caller just wrote.
*/
static void SCModel_OnPlayerModelChanged(int playerindex, cl_entity_t* currentEntity)
{
	player_model_t* state = &(*DM_PlayerState)[playerindex];

	if (state->model == currentEntity->model || !state->model)
	{
		if (state->name[0])
		{
			if (SCModel_AutoDownload())
			{
				SCModelDatabase()->QueryModel(state->name);
			}
		}
	}
}

int R_StudioDrawPlayer(int flags, entity_state_t* pplayer)
{
	cl_entity_t* currentEntity = IEngineStudio.GetCurrentEntity();
	const int playerindex = pplayer->number - 1;

	bool triggered = false;

	if (playerindex >= 0 && playerindex < gEngfuncs.GetMaxClients())
	{
		triggered = SCModel_IsModelChangeTriggered(playerindex, currentEntity);
	}

	const int result = gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);

	if (triggered)
	{
		SCModel_OnPlayerModelChanged(playerindex, currentEntity);
	}

	return result;
}

model_t* studioapi_SetupPlayerModel(int playerindex)
{
	cl_entity_t* currentEntity = IEngineStudio.GetCurrentEntity();

	const bool triggered = SCModel_IsModelChangeTriggered(playerindex, currentEntity);

	model_t* result = gPrivateFuncs.studioapi_SetupPlayerModel(playerindex);

	if (triggered)
	{
		SCModel_OnPlayerModelChanged(playerindex, currentEntity);
	}

	return result;
}

void SCModel_Reload_f(void)
{
	SCModel_ReloadAllModels();
}

void HUD_Frame(double frame)
{
	gExportfuncs.HUD_Frame(frame);

	SCModelDatabase()->RunFrame();
	UtilHTTPClient()->RunFrame();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();
	
	scmodel_autodownload = gEngfuncs.pfnRegisterVariable("scmodel_autodownload", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	scmodel_downloadlatest = gEngfuncs.pfnRegisterVariable("scmodel_downloadlatest", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	scmodel_cdn = gEngfuncs.pfnRegisterVariable("scmodel_cdn", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	scmodel_max_retry = gEngfuncs.pfnRegisterVariable("scmodel_max_retry", "3", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	gEngfuncs.pfnAddCommand("scmodel_reload", SCModel_Reload_f);

	SCModelDatabase()->Init();
}

void HUD_Shutdown(void)
{
	SCModelDatabase()->Shutdown();

	gExportfuncs.HUD_Shutdown();

	UtilAssetsIntegrity_Shutdown();
	UtilHTTPClient_Shutdown();
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	g_pDeveloper = IEngineStudio.GetCvar("developer");

	if (!g_pDeveloper)
	{
		Sys_Error("%s", "Failed to resolve the \"developer\" cvar");
		return 0;
	}

	Engine_InstallHook();

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	return result;
}
