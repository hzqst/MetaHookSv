#include <metahook.h>
#include "enginedef.h"
#include "privatehook.h"
#include "message.h"
#include "corpse.h"
#include "physics.h"

privte_funcs_t gPrivateFuncs;

TEMPENTITY *gTempEnts = NULL;

void R_NewMap(void)
{
	gPrivateFuncs.R_NewMap();
	gPhysicsManager.NewMap();
	gCorpseManager.NewMap();
}

TEMPENTITY *efxapi_R_TempModel(float *pos, float *dir, float *angles, float life, int modelIndex, int soundtype)
{
	auto r = gPrivateFuncs.efxapi_R_TempModel(pos, dir, angles, life, modelIndex, soundtype);
	if (r && g_bIsCreatingClCorpse && g_iCreatingClCorpsePlayerIndex > 0)
	{
		int tentindex = r - gTempEnts;

		gPhysicsManager.ChangeRagdollEntIndex(g_iCreatingClCorpsePlayerIndex, ENTINDEX_TEMPENTITY + tentindex);

		r->entity.curstate.iuser4 = PhyCorpseFlag;
		r->entity.curstate.iuser3 = ENTINDEX_TEMPENTITY + tentindex;
		r->entity.curstate.owner = g_iCreatingClCorpsePlayerIndex;
	}

	return r;
}