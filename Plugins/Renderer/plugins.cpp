#include <metahook.h>
#include "plugins.h"
#include "gl_local.h"
#include "exportfuncs.h"
#include "VGUI2ExtensionImport.h"
#include "UtilThreadTask.h"

#include <FreeImage.h>

cl_exportfuncs_t gExportfuncs = {0};
mh_interface_t *g_pInterface = NULL;
metahook_api_t *g_pMetaHookAPI = NULL;
mh_enginesave_t *g_pMetaSave = NULL;
IFileSystem *g_pFileSystem = NULL;
IFileSystem_HL25* g_pFileSystem_HL25 = NULL;

int g_iEngineType = 0;
DWORD g_dwEngineBuildnum = 0;
mh_dll_info_t g_EngineDLLInfo = { 0 };
mh_dll_info_t g_MirrorEngineDLLInfo = {0};
mh_dll_info_t g_ClientDLLInfo = { 0 };
mh_dll_info_t g_MirrorClientDLLInfo = { 0 };

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
		Sys_Error("FreeImage.dll version mismatch, expect \"%d.%d.%d\", got \"%d.%d.%d\" ! Are you using an older version of FreeImage.dll ?", 
			FREEIMAGE_MAJOR_VERSION, FREEIMAGE_MINOR_VERSION, FREEIMAGE_RELEASE_SERIAL, 
			FreeImage_MajorVersion, FreeImage_MinorVersion, FreeImage_ReleaseSerial);
	}

	g_pFileSystem = g_pInterface->FileSystem;
	if (!g_pFileSystem)//backward compatibility
		g_pFileSystem_HL25 = g_pInterface->FileSystem_HL25;

	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	
	g_EngineDLLInfo.ImageBase = g_pMetaHookAPI->GetEngineBase();
	g_EngineDLLInfo.ImageSize = g_pMetaHookAPI->GetEngineSize();
	g_EngineDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_EngineDLLInfo.ImageBase, ".text\x0\x0\x0", &g_EngineDLLInfo.TextSize);
	g_EngineDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_EngineDLLInfo.ImageBase, ".data\x0\x0\x0", &g_EngineDLLInfo.DataSize);
	g_EngineDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_EngineDLLInfo.ImageBase, ".rdata\x0\x0", &g_EngineDLLInfo.RdataSize);

	g_MirrorEngineDLLInfo.ImageBase = g_pMetaHookAPI->GetMirrorEngineBase();
	g_MirrorEngineDLLInfo.ImageSize = g_pMetaHookAPI->GetMirrorEngineSize();

	if (g_MirrorEngineDLLInfo.ImageBase)
	{
		g_MirrorEngineDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_MirrorEngineDLLInfo.ImageBase, ".text\x0\x0\x0", &g_MirrorEngineDLLInfo.TextSize);
		g_MirrorEngineDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorEngineDLLInfo.ImageBase, ".data\x0\x0\x0", &g_MirrorEngineDLLInfo.DataSize);
		g_MirrorEngineDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorEngineDLLInfo.ImageBase, ".rdata\x0\x0", &g_MirrorEngineDLLInfo.RdataSize);
	}

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));

	Engine_FillAddress(g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);
	Engine_InstallHooks();
	EngineSurface_InstallHooks();

	R_PatchResetLatched(g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);

	R_RedirectEngineLegacyOpenGLCall(g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);

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
	pExportFunc->HUD_Frame = HUD_Frame;
	pExportFunc->HUD_CreateEntities = HUD_CreateEntities;
	pExportFunc->HUD_PlayerMoveInit = HUD_PlayerMoveInit;
	pExportFunc->V_CalcRefdef = V_CalcRefdef;

	g_ClientDLLInfo.ImageBase = g_pMetaHookAPI->GetClientBase();
	g_ClientDLLInfo.ImageSize = g_pMetaHookAPI->GetClientSize();
	g_ClientDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_ClientDLLInfo.ImageBase, ".text\0\0\0", &g_ClientDLLInfo.TextSize);
	g_ClientDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_ClientDLLInfo.ImageBase, ".data\0\0\0", &g_ClientDLLInfo.DataSize);
	g_ClientDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_ClientDLLInfo.ImageBase, ".rdata\0\0", &g_ClientDLLInfo.RdataSize);

	g_MirrorClientDLLInfo.ImageBase = g_pMetaHookAPI->GetMirrorClientBase();
	g_MirrorClientDLLInfo.ImageSize = g_pMetaHookAPI->GetMirrorClientSize();

	if (g_MirrorClientDLLInfo.ImageBase)
	{
		g_MirrorClientDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_MirrorClientDLLInfo.ImageBase, ".text\0\0\0", &g_MirrorClientDLLInfo.TextSize);
		g_MirrorClientDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorClientDLLInfo.ImageBase, ".data\0\0\0", &g_MirrorClientDLLInfo.DataSize);
		g_MirrorClientDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorClientDLLInfo.ImageBase, ".rdata\0\0", &g_MirrorClientDLLInfo.RdataSize);
	}

	Client_FillAddress(g_MirrorClientDLLInfo.ImageBase ? g_MirrorClientDLLInfo : g_ClientDLLInfo, g_ClientDLLInfo);
	Client_InstallHooks();
	UtilThreadTask_Init();

	R_RedirectClientLegacyOpenGLCall(g_MirrorClientDLLInfo.ImageBase ? g_MirrorClientDLLInfo : g_ClientDLLInfo, g_ClientDLLInfo);
}

void IPluginsV4::ExitGame(int iResult)
{
	BaseUI_UninstallHooks();
	GameUI_UninstallHooks();
	VGUI2Extension_Shutdown();
	Engine_UninstallHooks();
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