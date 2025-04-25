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
DWORD g_dwEngineBuildnum = 0;
DWORD g_dwVideoMode = VIDEOMODE_SOFTWARE;

mh_dll_info_t g_EngineDLLInfo = { 0 };
mh_dll_info_t g_MirrorEngineDLLInfo = { 0 };
mh_dll_info_t g_ClientDLLInfo = { 0 };
mh_dll_info_t g_MirrorClientDLLInfo = { 0 };

static HMODULE g_hVGUI2 = NULL;

//Just in case KeyValuesSystem == nullptr, or sizeof(KeyValues) too small.
void DllLoadNotification(mh_load_dll_notification_context_t* ctx)
{
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)
	{
		if (ctx->BaseDllName && ctx->hModule && !g_hVGUI2 && !_wcsicmp(ctx->BaseDllName, L"vgui2.dll"))
		{
			g_hVGUI2 = ctx->hModule;
		}
		if (ctx->BaseDllName && ctx->hModule && g_hVGUI2 && !_wcsicmp(ctx->BaseDllName, L"GameUI.dll"))
		{
			KeyValuesSystem_Init(g_hVGUI2);
		}
	}
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)
	{
		if (g_hVGUI2 == ctx->hModule)
		{
			g_hVGUI2 = NULL;
		}
	}
}

void IPluginsV4::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
}

void IPluginsV4::Shutdown(void)
{
	g_pMetaHookAPI->UnregisterLoadDllNotificationCallback(DllLoadNotification);
}

void IPluginsV4::LoadEngine(cl_enginefunc_t *pEngfuncs)
{
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

	g_dwVideoMode = g_pMetaHookAPI->GetVideoMode(nullptr, nullptr, nullptr, nullptr);

	g_pMetaHookAPI->RegisterLoadDllNotificationCallback(DllLoadNotification);

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));

	Engine_FillAddress(g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);
	Engine_InstallHook();

	VGUI2Extension_Init();
	BaseUI_InstallHooks();
	GameUI_InstallHooks();
	ClientVGUI_InstallHooks();

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
	pExportFunc->HUD_PostRunCmd = HUD_PostRunCmd;

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
}

void IPluginsV4::ExitGame(int iResult)
{
	g_pClientPhysicManager->Destroy();

	ClientVGUI_UninstallHooks();
	GameUI_UninstallHooks();
	BaseUI_UninstallHooks();

	VGUI2Extension_Shutdown();
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