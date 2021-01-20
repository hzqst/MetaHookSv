#include <metahook.h>
#include <cvardef.h>
#include "plugins.h"

#define V_CALCGUNANGLE_SIG "\xFF\x15\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84\x2A\x2A\x2A\x2A\x8B\x4C\x24\x04\xD9\x81\x94\x00\x00\x00\xD8\x41\x10\xD9\x98\x58\x0B\x00\x00\xD9\x81\x90\x00\x00\x00\xDC\x0D"

extern void (*g_pfnV_CalcGunAngle)(ref_params_s *pparams);
void V_CalcGunAngle(ref_params_s *pparams);

cl_enginefunc_t gEngfuncs;
cvar_t *cl_vgunlag;

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(&gEngfuncs, pEnginefuncs, sizeof(gEngfuncs));

	g_pfnV_CalcGunAngle = (void (*)(ref_params_s *pparams))g_pMetaHookAPI->SearchPattern((void *)g_pMetaSave->pExportFuncs->HUD_DrawNormalTriangles, 0x100000, V_CALCGUNANGLE_SIG, sizeof(V_CALCGUNANGLE_SIG) - 1);

	if (g_pfnV_CalcGunAngle)
	{
		g_pMetaHookAPI->InlineHook(g_pfnV_CalcGunAngle, V_CalcGunAngle, (void *&)g_pfnV_CalcGunAngle);
	}

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}

void HUD_Init(void)
{
	cl_vgunlag = gEngfuncs.pfnRegisterVariable("cl_vgunlag", "0", FCVAR_ARCHIVE);

	return gExportfuncs.HUD_Init();
}

int HUD_Redraw(float time, int intermission)
{
	return gExportfuncs.HUD_Redraw(time, intermission);
}