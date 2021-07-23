#include <metahook.h>
#include "exportfuncs.h"
#include "engfuncs.h"

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;
IFileSystem *g_pFileSystem;
BOOL g_IsClientVGUI2 = false;
HMODULE g_hClientDll = NULL;
DWORD g_dwClientSize = 0;
int g_iVideoWidth = 0;
int g_iVideoHeight = 0;

PVOID g_dwEngineBase;
DWORD g_dwEngineSize;
PVOID g_dwEngineTextBase;
DWORD g_dwEngineTextSize;
PVOID g_dwEngineDataBase;
DWORD g_dwEngineDataSize;
PVOID g_dwEngineRdataBase;
DWORD g_dwEngineRdataSize;
DWORD g_dwEngineBuildnum;
int g_iEngineType;

extern IFileSystem *g_pFullFileSystem;

ICommandLine *CommandLine(void)
{
	return g_pInterface->CommandLine;
}

void IPlugins::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;

	//CommandLine()->AppendParm("-nomaster", NULL);
	//CommandLine()->AppendParm("-insecure", NULL);
}

void IPlugins::Shutdown(void)
{
}

void IPlugins::LoadEngine(void)
{
	g_pFileSystem = g_pInterface->FileSystem;
	g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, NULL, NULL);

	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_dwEngineBase = g_pMetaHookAPI->GetEngineBase();
	g_dwEngineSize = g_pMetaHookAPI->GetEngineSize();
	g_dwEngineTextBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".text\x0\x0\x0", &g_dwEngineTextSize);
	g_dwEngineDataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".data\x0\x0\x0", &g_dwEngineDataSize);
	g_dwEngineRdataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".rdata\x0\x0", &g_dwEngineRdataSize);

	Steam_Init();
	Engine_FillAddress();
	BaseUI_InstallHook();

	g_pFullFileSystem = g_pFileSystem;
}

void IPlugins::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	//Get video settings again since width and height might have been changed during initialization.
	g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, NULL, NULL);

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->Initialize = Initialize;
	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_VidInit = HUD_VidInit;
	pExportFunc->HUD_Frame = HUD_Frame;

	g_hClientDll = GetModuleHandle("client.dll");

	if (!g_hClientDll)
	{
		Sys_ErrorEx("CaptionMod: client.dll not found.");
	}

	g_dwClientSize = g_pMetaHookAPI->GetModuleSize(g_hClientDll);

	gCapFuncs.GetProcAddress = GetProcAddress;

	//Try installing hook to interface VClientVGUI001
	ClientVGUI_InstallHook();

	VGUI1_InstallHook();

	//hook textmsg
	MSG_Init();

	//hook engine audio
	Engine_InstallHook();
}

void IPlugins::ExitGame(int iResult)
{
	if (gCapFuncs.hk_GetProcAddress)
		g_pMetaHookAPI->UnHook(gCapFuncs.hk_GetProcAddress);

	ClientVGUI_Shutdown();
}

EXPOSE_SINGLE_INTERFACE(IPlugins, IPlugins, METAHOOK_PLUGIN_API_VERSION);