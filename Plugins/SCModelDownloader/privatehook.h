#pragma once

#include <com_model.h>

typedef struct
{
	char		name[260];
	char		modelname[260];
	model_t*	model;
} player_model_t;

typedef struct
{
	struct model_s *(*studioapi_SetupPlayerModel)(int index);
	void (*R_StudioChangePlayerModel)(void);
}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

extern player_model_t(*DM_PlayerState)[MAX_CLIENTS];