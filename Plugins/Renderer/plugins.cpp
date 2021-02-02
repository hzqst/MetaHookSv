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
DWORD g_dwEngineBase, g_dwEngineSize;
DWORD g_dwEngineBuildnum;

int g_iEngineType = ENGINE_GOLDSRC;

void IPlugins::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	BOOL (*IsDebuggerPresent)(void) = (BOOL (*)(void))GetProcAddress(GetModuleHandle("kernel32.dll"), "IsDebuggerPresent");

	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
	g_hInstance = GetModuleHandle(NULL);

	//TODO: D3D Support
	//g_pInterface->CommandLine->RemoveParm("-d3d");
	//g_pInterface->CommandLine->AppendParm("-gl", NULL);
	//g_pInterface->CommandLine->AppendParm("-32bpp", NULL);
}

void IPlugins::Shutdown(void)
{
}

void IPlugins::LoadEngine(void)
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

	Memory_Init();
	R_FillAddress();
	R_InstallHook();
}

void IPlugins::LoadClient(cl_exportfuncs_t *pExportFunc)
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

	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));
	memcpy(&gEngfuncs, g_pMetaSave->pEngineFuncs, sizeof(gEngfuncs));

	Cmd_GetCmdBase = *(cmd_function_t *(**)(void))((DWORD)g_pMetaSave->pEngineFuncs + 0x198);

	if(g_dwEngineBuildnum < 5953)
	{
		g_pMetaHookAPI->InlineHook(gEngfuncs.pfnGetMousePos, hudGetMousePos, (void *&)gEngfuncs.pfnGetMousePos);
		g_pMetaHookAPI->InlineHook(gEngfuncs.GetMousePosition, hudGetMousePosition, (void *&)gEngfuncs.GetMousePosition);
	}

	GL_Init();

	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;
	pExportFunc->HUD_UpdateClientData = HUD_UpdateClientData;
	pExportFunc->HUD_AddEntity = HUD_AddEntity;
	pExportFunc->HUD_Redraw = HUD_Redraw;
	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_VidInit = HUD_VidInit;
	pExportFunc->V_CalcRefdef = V_CalcRefdef;
	pExportFunc->HUD_DrawNormalTriangles = HUD_DrawNormalTriangles;
	pExportFunc->HUD_DrawTransparentTriangles = HUD_DrawTransparentTriangles;
}

void IPlugins::ExitGame(int iResult)
{
	R_Shutdown();
}

EXPOSE_SINGLE_INTERFACE(IPlugins, IPlugins, METAHOOK_PLUGIN_API_VERSION);

//renderer exports

void IRenderer::GetInterface(ref_export_t *pRefExports, const char *version)
{
	if(!strcmp(version, META_RENDERER_VERSION))
	{
		memcpy(pRefExports, &gRefExports, sizeof(ref_export_t));
	}
	else
	{
		Sys_ErrorEx("Meta Renderer interface version (%s) should be (%s)\n", version, META_RENDERER_VERSION);
	}
}

EXPOSE_SINGLE_INTERFACE(IRenderer, IRenderer, RENDERER_API_VERSION);