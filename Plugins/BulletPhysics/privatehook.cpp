#include <metahook.h>
#include "privatehook.h"
#include "corpse.h"
#include "physics.h"

privte_funcs_t gPrivateFuncs;

void R_NewMap(void)
{
	gPrivateFuncs.R_NewMap();
	gPhysicsManager.NewMap();
	gCorpseManager.NewMap();
}
