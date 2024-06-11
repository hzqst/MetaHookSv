#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <IKeyValuesSystem.h>
#include "plugins.h"

#include "ResourceReplacer.h"

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;
IKeyValuesSystem* g_pKeyValuesSystem = NULL;

int HUD_VidInit(void)
{
	ModelReplacer()->FreeMapEntries();
	SoundReplacer()->FreeMapEntries();

	return gExportfuncs.HUD_VidInit();
}

void HUD_Init(void)
{
	ModelReplacer()->LoadGlobalReplaceList("resreplacer/default_global.gmr");
	SoundReplacer()->LoadGlobalReplaceList("resreplacer/default_global.gsr");

	return gExportfuncs.HUD_Init();
}

void HUD_Shutdown(void)
{
	ModelReplacer()->Shutdown();
	SoundReplacer()->Shutdown();

	return gExportfuncs.HUD_Shutdown();
}