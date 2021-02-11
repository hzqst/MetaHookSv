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

DWORD g_dwEngineBase, g_dwEngineSize;
DWORD g_dwEngineBuildnum;
DWORD g_iVideoMode;
int g_EngineType;
int g_iVideoWidth, g_iVideoHeight, g_iBPP;
bool g_bWindowed;
bool g_bIsUseSteam;
bool g_bIsRunningSteam;

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

	CommandLine()->AppendParm("-nomaster", NULL);
	CommandLine()->AppendParm("-insecure", NULL);
}

void IPlugins::Shutdown(void)
{
}

void IPlugins::LoadEngine(void)
{
	g_pFileSystem = g_pInterface->FileSystem;
	g_iVideoMode = g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, &g_iBPP, &g_bWindowed);
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_EngineType = g_pMetaHookAPI->GetEngineType();

	g_dwEngineBase = g_pMetaHookAPI->GetEngineBase();
	g_dwEngineSize = g_pMetaHookAPI->GetEngineSize();

	Steam_Init();
	Engine_FillAddress();
	BaseUI_InstallHook();

	g_pFullFileSystem = g_pFileSystem;
}

void IPlugins::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	//Get video settings again since W&H might change during initialization.
	g_iVideoMode = g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, &g_iBPP, &g_bWindowed);

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->Initialize = Initialize;
	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_VidInit = HUD_VidInit;

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