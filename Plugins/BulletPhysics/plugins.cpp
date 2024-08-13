#include <metahook.h>
#include <com_model.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "plugins.h"
#include "command.h"
#include "message.h"

#include "VGUI2ExtensionImport.h"

#include "ClientPhysicManager.h"

#include <glew.h>

cl_exportfuncs_t gExportfuncs = {0};
mh_interface_t *g_pInterface = NULL;
metahook_api_t *g_pMetaHookAPI = NULL;
mh_enginesave_t *g_pMetaSave = NULL;
IFileSystem *g_pFileSystem = NULL;
IFileSystem_HL25* g_pFileSystem_HL25 = NULL;

int g_iEngineType = 0;
PVOID g_dwEngineBase = 0;
DWORD g_dwEngineSize = 0;
PVOID g_dwEngineTextBase = 0;
DWORD g_dwEngineTextSize = 0;
PVOID g_dwEngineDataBase = 0;
DWORD g_dwEngineDataSize = 0;
PVOID g_dwEngineRdataBase = 0;
DWORD g_dwEngineRdataSize = 0;
DWORD g_dwEngineBuildnum = 0;

PVOID g_dwClientBase = 0;
DWORD g_dwClientSize = 0;
PVOID g_dwClientTextBase = 0;
DWORD g_dwClientTextSize = 0;
PVOID g_dwClientDataBase = 0;
DWORD g_dwClientDataSize = 0;
PVOID g_dwClientRdataBase = 0;
DWORD g_dwClientRdataSize = 0;

void IPluginsV4::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
}

void IPluginsV4::Shutdown(void)
{

}

void IPluginsV4::LoadEngine(cl_enginefunc_t *pEngfuncs)
{
	g_pFileSystem = g_pInterface->FileSystem;	
	if (!g_pFileSystem)//backward compatibility
		g_pFileSystem_HL25 = g_pInterface->FileSystem_HL25;

	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_dwEngineBase = g_pMetaHookAPI->GetEngineBase();
	g_dwEngineSize = g_pMetaHookAPI->GetEngineSize();
	g_dwEngineTextBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".text\x0\x0\x0", &g_dwEngineTextSize);
	g_dwEngineDataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".data\x0\x0\x0", &g_dwEngineDataSize);
	g_dwEngineRdataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".rdata\x0\x0", &g_dwEngineRdataSize);

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));

	Engine_FillAddreess();
	Engine_InstallHook();

	VGUI2Extension_Init();
	BaseUI_InstallHooks();
	GameUI_InstallHooks();

	g_pClientPhysicManager = BulletPhysicManager_CreateInstance();

	glewInit();
}

void IPluginsV4::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;
	pExportFunc->HUD_CreateEntities = HUD_CreateEntities;
	pExportFunc->HUD_TempEntUpdate = HUD_TempEntUpdate;
	pExportFunc->HUD_AddEntity = HUD_AddEntity;
	pExportFunc->HUD_DrawTransparentTriangles = HUD_DrawTransparentTriangles;
	pExportFunc->HUD_Frame = HUD_Frame;
	pExportFunc->HUD_Shutdown = HUD_Shutdown;
	pExportFunc->V_CalcRefdef = V_CalcRefdef;

	Client_FillAddress();
}

void IPluginsV4::ExitGame(int iResult)
{
	g_pClientPhysicManager->Destroy();

	Engine_UninstallHook();
}

const char completeVersion[] =
{
	BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
	'-',
	BUILD_MONTH_CH0, BUILD_MONTH_CH1,
	'-',
	BUILD_DAY_CH0, BUILD_DAY_CH1,
	'T',
	BUILD_HOUR_CH0, BUILD_HOUR_CH1,
	':',
	BUILD_MIN_CH0, BUILD_MIN_CH1,
	':',
	BUILD_SEC_CH0, BUILD_SEC_CH1,
	'\0'
};

const char *IPluginsV4::GetVersion(void)
{
	return completeVersion;
}

EXPOSE_SINGLE_INTERFACE(IPluginsV4, IPluginsV4, METAHOOK_PLUGIN_API_VERSION_V4);