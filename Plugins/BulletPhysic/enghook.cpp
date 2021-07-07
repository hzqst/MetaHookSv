#include <metahook.h>
#include "enghook.h"
#include "phycorpse.h"
#include "physics.h"

privte_funcs_t gPrivateFuncs;

void R_NewMap(void)
{
	gPrivateFuncs.R_NewMap();
	gPhysicsManager.NewMap();
}
