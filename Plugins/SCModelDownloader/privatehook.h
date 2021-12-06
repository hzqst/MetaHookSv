#pragma once

typedef struct
{
	struct model_s *(*studioapi_SetupPlayerModel)(int index);
	void (*R_StudioChangePlayerModel)(void);
}privte_funcs_t;

extern privte_funcs_t gPrivateFuncs;