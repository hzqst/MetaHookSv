#include <metahook.h>
#include "gl_local.h"
#include "exportfuncs.h"

cl_exportfuncs_t gExportfuncs = {0};
mh_interface_t *g_pInterface = NULL;
metahook_api_t *g_pMetaHookAPI = NULL;
mh_enginesave_t *g_pMetaSave = NULL;
IFileSystem *g_pFileSystem = NULL;
IFileSystem_HL25* g_pFileSystem_HL25 = NULL;

HINSTANCE g_hInstance, g_hThisModule, g_hEngineModule;
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
PVOID g_dwClientBase;
DWORD g_dwClientSize;

void IPluginsV4::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
	g_hInstance = GetModuleHandle(NULL);
}

void IPluginsV4::Shutdown(void)
{
	
}

void IPluginsV4::LoadEngine(cl_enginefunc_t *pEngfuncs)
{
	int bbp = 0;
	int iVideoMode = g_pMetaHookAPI->GetVideoMode(NULL, NULL, &bbp, NULL);

	if (iVideoMode == 2)
	{
		g_pMetaHookAPI->SysError("D3D mode is not supported.");
	}
	if (iVideoMode == 0)
	{
		g_pMetaHookAPI->SysError("Software mode is not supported.");
	}
	if (bbp == 16)
	{
		g_pMetaHookAPI->SysError("16bit mode is not supported.");
	}

	g_pFileSystem = g_pInterface->FileSystem;
	if (!g_pFileSystem)//backward compatibility
		g_pFileSystem_HL25 = g_pInterface->FileSystem_HL25;

	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_hEngineModule = g_pMetaHookAPI->GetEngineModule();
	g_dwEngineBase = g_pMetaHookAPI->GetEngineBase();
	g_dwEngineSize = g_pMetaHookAPI->GetEngineSize();
	g_dwEngineTextBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".text\x0\x0\x0", &g_dwEngineTextSize);
	g_dwEngineDataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".data\x0\x0\x0", &g_dwEngineDataSize);
	g_dwEngineRdataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".rdata\x0\x0", &g_dwEngineRdataSize);

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));

	if(g_iEngineType != ENGINE_SVENGINE && g_iEngineType != ENGINE_GOLDSRC_BLOB && g_iEngineType != ENGINE_GOLDSRC && g_iEngineType != ENGINE_GOLDSRC_HL25)
	{
		g_pMetaHookAPI->SysError("Unsupported engine: %s, buildnum %d", g_pMetaHookAPI->GetEngineTypeName(), g_dwEngineBuildnum);
	}

	R_FillAddress();
	R_InstallHooks();
}

void IPluginsV4::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	int bbp = 0;
	int iVideoMode = g_pMetaHookAPI->GetVideoMode(&glwidth, &glheight, &bbp, NULL);

	if (iVideoMode == 2)
	{
		g_pMetaHookAPI->SysError("D3D mode is not supported");
	}
	if (iVideoMode == 0)
	{
		g_pMetaHookAPI->SysError("Software mode is not supported");
	}
	if (bbp == 16)
	{
		g_pMetaHookAPI->SysError("16bbp mode is not supported");
	}

	g_dwClientBase = g_pMetaHookAPI->GetClientBase();
	g_dwClientSize = g_pMetaHookAPI->GetClientSize();

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	GL_Init();

	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;
	pExportFunc->HUD_Redraw = HUD_Redraw;
	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_Shutdown = HUD_Shutdown;
	pExportFunc->HUD_AddEntity = HUD_AddEntity;
	pExportFunc->HUD_Frame = HUD_Frame;
	pExportFunc->HUD_PlayerMoveInit = HUD_PlayerMoveInit;
	pExportFunc->V_CalcRefdef = V_CalcRefdef;

	Uninstall_Hook(DLL_SetModKey);
}

void IPluginsV4::ExitGame(int iResult)
{
	R_UninstallHooksForEngineDLL();
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

void R_Version_f(void)
{
	gEngfuncs.Con_Printf("Renderer version : %s\n", completeVersion);
}

EXPOSE_SINGLE_INTERFACE(IPluginsV4, IPluginsV4, METAHOOK_PLUGIN_API_VERSION_V4);