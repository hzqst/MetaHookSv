#include <metahook.h>
#include "exportfuncs.h"
#include "BaseUI.h"
#include "Encode.h"
#include "UnicodeVoice.h"

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;

DWORD g_dwEngineBuildnum;

void IPlugins::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
}

void IPlugins::Shutdown(void)
{
}

void IPlugins::LoadEngine(void)
{
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();

	BaseUI_InstallHook();
}

void IPlugins::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->Initialize = Initialize;

	g_pMetaHookAPI->InlineHook(pExportFunc->HUD_VoiceStatus, HUD_VoiceStatus, (void *&)g_pfnHUD_VoiceStatus);
}

void IPlugins::ExitGame(int iResult)
{
}

EXPOSE_SINGLE_INTERFACE(IPlugins, IPlugins, METAHOOK_PLUGIN_API_VERSION);