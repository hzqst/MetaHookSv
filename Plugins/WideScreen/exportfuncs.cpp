#include <metahook.h>
#include "mempatchs.h"

cl_enginefunc_t gEngfuncs;

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(&gEngfuncs, pEnginefuncs, sizeof(gEngfuncs));

	MemPatch_Start(MEMPATCH_STEP_INITCLIENT);

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}

void HUD_Init(void)
{
	return gExportfuncs.HUD_Init();
}

int HUD_Redraw(float time, int intermission)
{
	return gExportfuncs.HUD_Redraw(time, intermission);
}