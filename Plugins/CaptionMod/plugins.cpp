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

void IPluginsV3::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
}

void IPluginsV3::Shutdown(void)
{
}

void IPluginsV3::LoadEngine(cl_enginefunc_t *pEngfuncs)
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

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));

	gCapFuncs.GetProcAddress = (decltype(gCapFuncs.GetProcAddress))GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");

	gCapFuncs.pfnTextMessageGet = pEngfuncs->pfnTextMessageGet;

	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gEngfuncs.GetClientTime, 0x20, "\xDD\x05", Sig_Length("\xDD\x05"));
	Sig_AddrNotFound("cl_time");
	gCapFuncs.pcl_time = (double *)*(DWORD *)(addr + 2);
	gCapFuncs.pcl_oldtime = gCapFuncs.pcl_time + 1;

	Steam_Init();
	Engine_FillAddress();
	Engine_InstallHook();
	BaseUI_InstallHook();

	g_pFullFileSystem = g_pFileSystem;
}

void IPluginsV3::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	//Get video settings again since width and height might have been changed during initialization.
	g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, NULL, NULL);

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_VidInit = HUD_VidInit;
	pExportFunc->HUD_Frame = HUD_Frame;

	g_hClientDll = GetModuleHandle("client.dll");
	g_dwClientSize = g_pMetaHookAPI->GetModuleSize(g_hClientDll);

	auto pfnClientCreateInterface = Sys_GetFactory((HINTERFACEMODULE)g_hClientDll);

	//Fix SvClient Portal Rendering Confliction
	if (pfnClientCreateInterface && pfnClientCreateInterface("SCClientDLL001", 0))
	{
#define SV_FINDSOUND_SIG_SVENGINE "\x51\x55\x8B\x6C\x24\x0C\x89\x4C\x24\x04\x85\xED\x0F\x84\x2A\x2A\x2A\x2A\x80\x7D\x00\x00"

		gCapFuncs.SvClient_FindSoundEx = (decltype(gCapFuncs.SvClient_FindSoundEx))
			g_pMetaHookAPI->SearchPattern((void *)g_hClientDll, g_dwClientSize, SV_FINDSOUND_SIG_SVENGINE, Sig_Length(SV_FINDSOUND_SIG_SVENGINE));

		Sig_FuncNotFound(SvClient_FindSoundEx);

		Install_InlineHook(SvClient_FindSoundEx);
	}

	Install_InlineHook(pfnTextMessageGet);

	//Try installing hook to interface VClientVGUI001
	ClientVGUI_InstallHook();

	VGUI1_InstallHook();

	//hook textmsg
	MSG_Init();
}

void IPluginsV3::ExitGame(int iResult)
{
	if (gCapFuncs.hk_GetProcAddress)
		g_pMetaHookAPI->UnHook(gCapFuncs.hk_GetProcAddress);

	ClientVGUI_Shutdown();
}

EXPOSE_SINGLE_INTERFACE(IPluginsV3, IPluginsV3, METAHOOK_PLUGIN_API_VERSION_V3);