#include <metahook.h>
#include "exportfuncs.h"

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;

void IPlugins::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;

	MessageBoxA(NULL, "IPlugins::Init", "test", MB_OK);
}

void IPlugins::Shutdown(void)
{
}

void IPlugins::LoadEngine(void)
{
	MessageBoxA(NULL, "IPlugins::LoadEngine", "test", MB_OK);
}

void IPlugins::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	MessageBoxA(NULL, "IPlugins::LoadClient", "test", MB_OK);

	pExportFunc->Initialize = Initialize;
	pExportFunc->HUD_VidInit = HUD_VidInit;
	pExportFunc->HUD_Redraw = HUD_Redraw;
}

void IPlugins::ExitGame(int iResult)
{
}

EXPOSE_SINGLE_INTERFACE(IPlugins, IPlugins, METAHOOK_PLUGIN_API_VERSION);