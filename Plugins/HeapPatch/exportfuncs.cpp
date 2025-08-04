#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "plugins.h"

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

int HUD_VidInit(void)
{
	return gExportfuncs.HUD_VidInit();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();
}

void HUD_Shutdown(void)
{
	gExportfuncs.HUD_Shutdown();
}