#include <metahook.h>
#include "privatehook.h"

privte_funcs_t gPrivateFuncs;

player_model_t(*DM_PlayerState)[MAX_CLIENTS];