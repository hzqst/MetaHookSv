#pragma once

#include <stddef.h>
#include <com_model.h>

typedef struct
{
	char		name[260];
	char		modelname[260];
	model_t*	model;
} player_model_t;

static_assert(sizeof(player_model_t) == 0x20C, "player_model_t must match the engine DM_PlayerState element size");
static_assert(offsetof(player_info_t, model) == 0x130, "player_info_t::model must match the engine cl.players element layout");
static_assert(sizeof(player_info_sc_t) == 0x250, "player_info_sc_t must match the SvEngine cl.players element stride");

typedef struct
{
	int (*R_StudioDrawPlayer)(int flags, struct entity_state_s* pplayer);
	model_t* (*studioapi_SetupPlayerModel)(int playerindex);
	int (*Host_IsSinglePlayerGame)(void);
}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

extern player_model_t(*DM_PlayerState)[MAX_CLIENTS];

// Recovered from the gamedata cl_players_model member address by
// Engine_FillAddress; only the array matching the current engine is assigned.
extern player_info_t* cl_players;
extern player_info_sc_t* cl_players_sc;

void Engine_FillAddress(void);
void Engine_InstallHook(void);
void Engine_UninstallHook(void);

void DllLoadNotification(mh_load_dll_notification_context_t* ctx);
