#include <metahook.h>
#include "exportfuncs.h"
#include "engfuncs.h"
#include "command.h"

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;
IFileSystem *g_pFileSystem;

bool g_IsClientVGUI2 = false;
bool g_IsSCClient = false;
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
	g_pFullFileSystem = g_pFileSystem;
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

	Cmd_GetCmdBase = *(cmd_function_t *(**)(void))((DWORD)pEngfuncs + 0x198);

	Steam_Init();
	Engine_FillAddress();
	Engine_InstallHook();
	BaseUI_InstallHook();
}

void IPluginsV3::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	//Get video settings again since width and height might have been changed during initialization.
	g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, NULL, NULL);

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_VidInit = HUD_VidInit;
	pExportFunc->HUD_Frame = HUD_Frame;
	pExportFunc->HUD_Redraw = HUD_Redraw;
	pExportFunc->IN_MouseEvent = IN_MouseEvent;
	pExportFunc->IN_Accumulate = IN_Accumulate;
	pExportFunc->CL_CreateMove = CL_CreateMove;

	g_hClientDll = GetModuleHandle("client.dll");
	g_dwClientSize = g_pMetaHookAPI->GetModuleSize(g_hClientDll);

	auto pfnClientCreateInterface = Sys_GetFactory((HINTERFACEMODULE)g_hClientDll);

	if (pfnClientCreateInterface && pfnClientCreateInterface("SCClientDLL001", 0))
	{
		g_IsSCClient = true;

#define SC_FINDSOUND_SIG "\x51\x55\x8B\x6C\x24\x0C\x89\x4C\x24\x04\x85\xED\x0F\x84\x2A\x2A\x2A\x2A\x80\x7D\x00\x00"
		{
			gCapFuncs.ScClient_FindSoundEx = (decltype(gCapFuncs.ScClient_FindSoundEx))
				g_pMetaHookAPI->SearchPattern((void *)g_hClientDll, g_dwClientSize, SC_FINDSOUND_SIG, Sig_Length(SC_FINDSOUND_SIG));

			Sig_FuncNotFound(ScClient_FindSoundEx);
			Install_InlineHook(ScClient_FindSoundEx);
		}

#define SC_GETCLIENTCOLOR_SIG "\x8B\x4C\x24\x04\x85\xC9\x2A\x2A\x6B\xC1\x58"
		{
			gCapFuncs.GetClientColor = (decltype(gCapFuncs.GetClientColor))
				g_pMetaHookAPI->SearchPattern((void *)g_hClientDll, g_dwClientSize, SC_GETCLIENTCOLOR_SIG, Sig_Length(SC_GETCLIENTCOLOR_SIG));

			Sig_FuncNotFound(GetClientColor);
		}

#define SC_VIEWPORT_SIG "\x8B\x0D\x2A\x2A\x2A\x2A\x85\xC9\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x84\xC0\x0F"
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_hClientDll, g_dwClientSize, SC_VIEWPORT_SIG, Sig_Length(SC_VIEWPORT_SIG));

			Sig_AddrNotFound(GameViewport);

			gCapFuncs.GameViewport = *(decltype(gCapFuncs.GameViewport) *)(addr + 2);
			gCapFuncs.GameViewport_AllowedToPrintText = (decltype(gCapFuncs.GameViewport_AllowedToPrintText))GetCallAddress(addr + 10);
		}

#define SC_UPDATECURSORSTATE_SIG "\x8B\x40\x28\xFF\xD0\x84\xC0\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00"
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_hClientDll, g_dwClientSize, SC_UPDATECURSORSTATE_SIG, Sig_Length(SC_UPDATECURSORSTATE_SIG));
			Sig_AddrNotFound(GameViewport_UpdateCursorState);

			gCapFuncs.g_iVisibleMouse = *(decltype(gCapFuncs.g_iVisibleMouse) *)(addr + 11);
			gCapFuncs.GameViewport_UpdateCursorState = (decltype(gCapFuncs.GameViewport_UpdateCursorState))
				g_pMetaHookAPI->ReverseSearchFunctionBegin((PVOID)addr, 0x80);

			//Install_InlineHook(GameViewport_UpdateCursorState);
		}
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