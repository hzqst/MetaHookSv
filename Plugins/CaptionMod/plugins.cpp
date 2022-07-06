#include <metahook.h>
#include <capstone.h>
#include "exportfuncs.h"
#include "privatefuncs.h"

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;
IFileSystem *g_pFileSystem;

bool g_IsClientVGUI2 = false;
HMODULE g_hClientDll = NULL;
PVOID g_dwClientBase = 0;
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

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;

extern IFileSystem *g_pFullFileSystem;

ICommandLine *CommandLine(void)
{
	return g_pInterface->CommandLine;
}

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

	gPrivateFuncs.GetProcAddress = (decltype(gPrivateFuncs.GetProcAddress))GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");

	gPrivateFuncs.pfnTextMessageGet = pEngfuncs->pfnTextMessageGet;

	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gEngfuncs.GetClientTime, 0x20, "\xDD\x05", sizeof("\xDD\x05") - 1);
	Sig_AddrNotFound("cl_time");
	cl_time = (double *)*(DWORD *)(addr + 2);
	cl_oldtime = cl_time + 1;

	Engine_FillAddress();
	Engine_InstallHook();
	BaseUI_InstallHook();
}

void IPluginsV4::LoadClient(cl_exportfuncs_t *pExportFunc)
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

	g_hClientDll = g_pMetaHookAPI->GetClientModule();
	g_dwClientBase = g_pMetaHookAPI->GetClientBase();
	g_dwClientSize = g_pMetaHookAPI->GetClientSize();

	Client_FillAddress();

	Install_InlineHook(pfnTextMessageGet);

	//Try installing hook to interface VClientVGUI001
	ClientVGUI_InstallHook();

	VGUI1_InstallHook();

	EnumWindows([](HWND hwnd, LPARAM lParam
		)
	{
		DWORD pid = 0;
		if (GetWindowThreadProcessId(hwnd, &pid) && pid == GetCurrentProcessId())
		{
			char windowClass[256] = { 0 };
			RealGetWindowClassA(hwnd, windowClass, sizeof(windowClass));
			if (!strcmp(windowClass, "Valve001") || !strcmp(windowClass, "SDL_app"))
			{
				g_MainWnd = hwnd;
				return FALSE;
			}
		}
		return TRUE;
	}, NULL);

	g_MainWndProc = (WNDPROC)GetWindowLong(g_MainWnd, GWL_WNDPROC);
	SetWindowLong(g_MainWnd, GWL_WNDPROC, (LONG)VID_MainWndProc);
}

void IPluginsV4::ExitGame(int iResult)
{
	if (gPrivateFuncs.hk_GetProcAddress)
		g_pMetaHookAPI->UnHook(gPrivateFuncs.hk_GetProcAddress);

	ClientVGUI_Shutdown();
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

void Cap_Version_f(void)
{
	gEngfuncs.Con_Printf("CaptionMod version : %s\n", completeVersion);
}

const char *IPluginsV4::GetVersion(void)
{
	return completeVersion;
}

EXPOSE_SINGLE_INTERFACE(IPluginsV4, IPluginsV4, METAHOOK_PLUGIN_API_VERSION_V4);