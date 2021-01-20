#include <metahook.h>

cl_enginefunc_t gEngfuncs;

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(&gEngfuncs, pEnginefuncs, sizeof(gEngfuncs));
	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}

int HUD_VidInit(void)
{
	gEngfuncs.Con_Printf("HUD_VidInit test\n");

	return gExportfuncs.HUD_VidInit();
}

int HUD_Redraw(float time, int intermission)
{
	gEngfuncs.Con_Printf("HUD_Redraw test\n");

	return gExportfuncs.HUD_Redraw(time, intermission);
}