#include <metahook.h>
#include "gl_local.h"
#include "exportfuncs.h"
#include "command.h"
#include <IRenderer.h>

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;
IFileSystem *g_pFileSystem;

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

void IPluginsV3::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
	g_hInstance = GetModuleHandle(NULL);
}

void IPluginsV3::Shutdown(void)
{
	R_Shutdown();
	GL_Shutdown();
}

void IPluginsV3::LoadEngine(cl_enginefunc_t *pEngfuncs)
{
	int iVideoMode = g_pMetaHookAPI->GetVideoMode(NULL, NULL, NULL, NULL);

	if (iVideoMode == 2)
	{
		Sys_ErrorEx("D3D mode is not supported.");
	}
	if (iVideoMode == 0)
	{
		Sys_ErrorEx("Software mode is not supported.");
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

	Cmd_GetCmdBase = *(cmd_function_t *(**)(void))((DWORD)pEngfuncs + 0x198);

	if(g_iEngineType != ENGINE_SVENGINE && g_iEngineType != ENGINE_GOLDSRC)
	{
		Sys_ErrorEx("Unsupported engine: %s, buildnum %d", g_pMetaHookAPI->GetEngineTypeName(), g_dwEngineBuildnum);
	}

	R_FillAddress();
}

void IPluginsV3::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	int iVideoMode = g_pMetaHookAPI->GetVideoMode(&glwidth, &glheight, NULL, NULL);

	if (iVideoMode == 2)
	{
		Sys_ErrorEx("D3D mode is not supported.");
	}
	if (iVideoMode == 0)
	{
		Sys_ErrorEx("Software mode is not supported.");
	}

	g_dwClientBase = (PVOID)GetModuleHandleA("client.dll");
	g_dwClientSize = g_pMetaHookAPI->GetModuleSize((HMODULE)g_dwClientBase);

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	GL_Init();
	R_InstallHook();

	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;
	pExportFunc->HUD_UpdateClientData = HUD_UpdateClientData;
	pExportFunc->HUD_Redraw = HUD_Redraw;
	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->V_CalcRefdef = V_CalcRefdef;
	pExportFunc->HUD_DrawNormalTriangles = HUD_DrawNormalTriangles;
}

void IPluginsV3::ExitGame(int iResult)
{
	
}

EXPOSE_SINGLE_INTERFACE(IPluginsV3, IPluginsV3, METAHOOK_PLUGIN_API_VERSION_V3);

//renderer exports

void IRenderer::GetInterface(ref_export_t *pRefExports, const char *version)
{
	if(!strcmp(version, META_RENDERER_VERSION))
	{
		memcpy(pRefExports, &gRefExports, sizeof(ref_export_t));
	}
	else
	{
		Sys_ErrorEx("Meta Renderer interface version mismatch, expect [%s], got [%s] \n", META_RENDERER_VERSION, version);
	}
}

EXPOSE_SINGLE_INTERFACE(IRenderer, IRenderer, RENDERER_API_VERSION);