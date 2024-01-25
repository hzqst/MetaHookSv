#include <metahook.h>
#include "exportfuncs.h"
#include "privatehook.h"

cl_exportfuncs_t gExportfuncs = {0};
mh_interface_t *g_pInterface = NULL;
metahook_api_t *g_pMetaHookAPI = NULL;
mh_enginesave_t *g_pMetaSave = NULL;
IFileSystem *g_pFileSystem = NULL;
IFileSystem_HL25 *g_pFileSystem_HL25 = NULL;

HINSTANCE g_hInstance = 0;
HMODULE g_hThisModule = 0;
HMODULE g_hEngineModule = 0;
PVOID g_dwEngineBase = 0;
DWORD g_dwEngineSize = 0;
PVOID g_dwEngineTextBase = 0;
DWORD g_dwEngineTextSize = 0;
PVOID g_dwEngineDataBase = 0;
DWORD g_dwEngineDataSize = 0;
PVOID g_dwEngineRdataBase = 0;
DWORD g_dwEngineRdataSize = 0;
DWORD g_dwEngineBuildnum = 0;
int g_iEngineType = 0;
PVOID g_dwClientBase = 0;
DWORD g_dwClientSize = 0;

HANDLE g_MainThreadId = 0;

void IPluginsV4::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
	g_hInstance = GetModuleHandle(NULL);
}

void IPluginsV4::Shutdown(void)
{
	g_pMetaHookAPI->UnregisterLoadDllNotificationCallback(DllLoadNotification);
}

void IPluginsV4::LoadEngine(cl_enginefunc_t *pEngfuncs)
{
	if (g_pInterface->MetaHookAPIVersion < METAHOOK_API_VERSION)
	{
		g_pMetaHookAPI->SysError("MetaHookAPIVersion too low! expect %d, got %d !", METAHOOK_API_VERSION, g_pInterface->MetaHookAPIVersion);
		return;
	}

	g_pFileSystem = g_pInterface->FileSystem;
	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_hEngineModule = g_pMetaHookAPI->GetEngineModule();
	g_dwEngineBase = g_pMetaHookAPI->GetEngineBase();
	g_dwEngineSize = g_pMetaHookAPI->GetEngineSize();
	g_dwEngineTextBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".text\x0\x0\x0", &g_dwEngineTextSize);
	g_dwEngineDataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".data\x0\x0\x0", &g_dwEngineDataSize);
	g_dwEngineRdataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".rdata\x0\x0", &g_dwEngineRdataSize);

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));

	g_MainThreadId = (HANDLE)GetCurrentThreadId();

	g_pMetaHookAPI->RegisterLoadDllNotificationCallback(DllLoadNotification);
}

void IPluginsV4::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	g_dwClientBase = g_pMetaHookAPI->GetClientBase();
	g_dwClientSize = g_pMetaHookAPI->GetClientSize();

	EngineCommand_InstallHook();
}

void IPluginsV4::ExitGame(int iResult)
{
	Engine_WaitForShutdown(g_pMetaHookAPI->GetEngineModule(), g_pMetaHookAPI->GetBlobEngineModule());
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