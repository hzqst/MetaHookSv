#include <metahook.h>
#include "plugins.h"
#include "gl_local.h"
#include "exportfuncs.h"
#include "VGUI2ExtensionImport.h"

#include <FreeImage.h>

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
	int iVideoMode = g_pMetaHookAPI->GetVideoMode(NULL, NULL, NULL, NULL);

	if (iVideoMode == 0)
	{
		Sys_Error("Software mode is not supported.\nPlease add \"-gl\" in the launch parameters.");
	}

	auto FreeImage_VersionString = FreeImage_GetVersion();
	int FreeImage_MajorVersion = 0, FreeImage_MinorVersion = 0, FreeImage_ReleaseSerial = 0;
	if (3 != sscanf(FreeImage_VersionString, "%d.%d.%d", &FreeImage_MajorVersion, &FreeImage_MinorVersion, &FreeImage_ReleaseSerial) ||
		FreeImage_MajorVersion != FREEIMAGE_MAJOR_VERSION ||
		FreeImage_MinorVersion != FREEIMAGE_MINOR_VERSION ||
		FreeImage_ReleaseSerial != FREEIMAGE_RELEASE_SERIAL)
	{
		Sys_Error("FreeImage.dll version mismatch, expect \"%d.%d.%d\", got \"%d.%d.%d\" !", 
			FREEIMAGE_MAJOR_VERSION, FREEIMAGE_MINOR_VERSION, FREEIMAGE_RELEASE_SERIAL, 
			FreeImage_MajorVersion, FreeImage_MinorVersion, FreeImage_ReleaseSerial);
	}

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

	R_FillAddress();
	R_InstallHooks();
	R_RedirectLegacyOpenGLTextureAllocation();
	R_PatchResetLatched();

	VGUI2Extension_Init();
	BaseUI_InstallHooks();
	GameUI_InstallHooks();
}

void IPluginsV4::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	int bbp = 0;
	int iVideoMode = g_pMetaHookAPI->GetVideoMode(&glwidth, &glheight, &bbp, NULL);

	if (iVideoMode == 0)
	{
		Sys_Error("Software mode is not supported.\nPlease add \"-gl\" in the launch parameters.");
	}
	if (iVideoMode == 2)
	{
		Sys_Error("D3D mode is not supported.\nPlease add \"-gl\" in the launch parameters.");
	}
	if (bbp == 16)
	{
		Sys_Error("16bbp mode is not supported.\nPlease add \"-32bpp\" in the launch parameters.");
	}

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;
	pExportFunc->HUD_Redraw = HUD_Redraw;
	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_Shutdown = HUD_Shutdown;
	pExportFunc->HUD_AddEntity = HUD_AddEntity;
	pExportFunc->HUD_Frame = HUD_Frame;
	pExportFunc->HUD_CreateEntities = HUD_CreateEntities;
	pExportFunc->HUD_PlayerMoveInit = HUD_PlayerMoveInit;
	pExportFunc->V_CalcRefdef = V_CalcRefdef;

	Client_FillAddress();
}

void IPluginsV4::ExitGame(int iResult)
{
	BaseUI_UninstallHooks();
	GameUI_UninstallHooks();
	VGUI2Extension_Shutdown();
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