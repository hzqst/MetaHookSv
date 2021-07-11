#include <metahook.h>
#include "exportfuncs.h"
#include "enghook.h"
#include "qgl.h"

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;
IFileSystem *g_pFileSystem;

HINSTANCE g_hInstance, g_hThisModule, g_hEngineModule;
DWORD g_dwEngineBase, g_dwEngineSize;
DWORD g_dwEngineBuildnum;
int g_iEngineType;

extern int *r_visframecount;

void IPlugins::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
	g_hInstance = GetModuleHandle(NULL);
}

void IPlugins::Shutdown(void)
{
}

#define R_NEWMAP_SIG_SVENGINE "\x55\x8B\xEC\x51\xC7\x45\xFC\x00\x00\x00\x00\xEB\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00"
#define R_RECURSIVEWORLDNODE_SIG_SVENGINE "\x83\xEC\x08\x53\x8B\x5C\x24\x10\x83\x3B\xFE"

void IPlugins::LoadEngine(void)
{
	g_pFileSystem = g_pInterface->FileSystem;
	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_hEngineModule = g_pMetaHookAPI->GetEngineModule();
	g_dwEngineBase = g_pMetaHookAPI->GetEngineBase();
	g_dwEngineSize = g_pMetaHookAPI->GetEngineSize();

	gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))g_pMetaHookAPI->SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, R_RECURSIVEWORLDNODE_SIG_SVENGINE, sizeof(R_RECURSIVEWORLDNODE_SIG_SVENGINE) - 1);
	if (!gPrivateFuncs.R_RecursiveWorldNode)
		Sys_ErrorEx("R_RecursiveWorldNode not found");

	//mov     eax, [edi+4]
	//mov     ecx, r_visframecount
#define R_VISFRAMECOUNT_SIG_SVENGINE "\x8B\x43\x04\x3B\x05"
	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_RecursiveWorldNode, 0x100, R_VISFRAMECOUNT_SIG_SVENGINE, sizeof(R_VISFRAMECOUNT_SIG_SVENGINE) - 1);
	if (!addr)
		Sys_ErrorEx("r_visframecount not found");
	r_visframecount = *(int **)(addr + 5);

	gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))g_pMetaHookAPI->SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, R_NEWMAP_SIG_SVENGINE, sizeof(R_NEWMAP_SIG_SVENGINE) - 1);
	if (!gPrivateFuncs.R_NewMap)
		Sys_ErrorEx("R_NewMap not found");
}

void IPlugins::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->Initialize = Initialize;
	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;
	pExportFunc->HUD_TempEntUpdate = HUD_TempEntUpdate;
	pExportFunc->HUD_AddEntity = HUD_AddEntity;
	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_DrawTransparentTriangles = HUD_DrawTransparentTriangles;

	QGL_Init();

	g_pMetaHookAPI->InlineHook(gPrivateFuncs.R_NewMap, R_NewMap, (void *&)gPrivateFuncs.R_NewMap);
}

void IPlugins::ExitGame(int iResult)
{
}

EXPOSE_SINGLE_INTERFACE(IPlugins, IPlugins, METAHOOK_PLUGIN_API_VERSION);