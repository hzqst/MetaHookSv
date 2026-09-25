#include <metahook.h>
#include "exportfuncs.h"
#include "gl_local.h"
#include "parsemsg.h"
#include "qgl.h"

#include "UtilThreadTask.h"

#include <set>

//Error when can't find sig

cl_enginefunc_t gEngfuncs = {0};
engine_studio_api_t IEngineStudio = { 0 };
r_studio_interface_t **gpStudioInterface = NULL;

bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;
bool g_bIsAoMDC = false;

static hook_t *g_phook_GameStudioRenderer_StudioDrawPlayer = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioSetupBones = NULL;
static hook_t* g_phook_GameStudioRenderer_StudioSaveBones = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioMergeBones = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioRenderModel = NULL;
static hook_t *g_phook_GameStudioRenderer_StudioRenderFinal = NULL;

static hook_t* g_phook_R_StudioDrawPlayer = NULL;
static hook_t *g_phook_R_StudioSetupBones = NULL;
static hook_t *g_phook_R_StudioMergeBones = NULL;
static hook_t* g_phook_R_StudioSaveBones = NULL;
static hook_t *g_phook_R_StudioRenderModel = NULL;
static hook_t *g_phook_R_StudioRenderFinal = NULL;

static hook_t* g_phook_studioapi_GL_SetRenderMode = NULL;
static hook_t* g_phook_studioapi_SetupRenderer = NULL;
static hook_t* g_phook_studioapi_RestoreRenderer = NULL;
static hook_t* g_phook_studioapi_StudioDynamicLight = NULL;
static hook_t* g_phook_studioapi_StudioCheckBBox = NULL;

static hook_t* g_phook_CL_FxBlend = NULL;

void EngineStudio_UninstallHooks(void)
{
	Uninstall_Hook(studioapi_GL_SetRenderMode);
	Uninstall_Hook(studioapi_SetupRenderer);
	Uninstall_Hook(studioapi_RestoreRenderer);
	Uninstall_Hook(studioapi_StudioDynamicLight);
	Uninstall_Hook(studioapi_StudioCheckBBox);

	Uninstall_Hook(CL_FxBlend);

	Uninstall_Hook(R_StudioRenderModel);
	Uninstall_Hook(R_StudioRenderFinal);
	Uninstall_Hook(R_StudioSetupBones);
	Uninstall_Hook(R_StudioMergeBones);
	Uninstall_Hook(R_StudioSaveBones);
}

void ClientStudio_UninstallHooks(void)
{
	Uninstall_Hook(GameStudioRenderer_StudioSetupBones);
	Uninstall_Hook(GameStudioRenderer_StudioMergeBones);
	Uninstall_Hook(GameStudioRenderer_StudioSaveBones);
	Uninstall_Hook(GameStudioRenderer_StudioRenderModel);
	Uninstall_Hook(GameStudioRenderer_StudioRenderFinal);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	R_Init();

	gEngfuncs.pfnAddCommand("r_version", R_Version_f);
	gEngfuncs.pfnAddCommand("r_reload", R_Reload_f);
	gEngfuncs.pfnAddCommand("r_dumptextures", R_DumpTextures_f);

#if 0
	gEngfuncs.pfnAddCommand("r_buildcubemaps", R_BuildCubemaps_f);
	gEngfuncs.pfnAddCommand("buildcubemaps", R_BuildCubemaps_f);
#endif

	//gl_texturemode is command in SvEngine, but cvar in GoldSrc
	if (!g_pMetaHookAPI->HookCmd("gl_texturemode", GL_Texturemode_f))
	{
		g_pMetaHookAPI->HookCvarCallback("gl_texturemode", GL_Texturemode_cb);
	}
}

int HUD_VidInit(void)
{
	return gExportfuncs.HUD_VidInit();
}

void V_CalcRefdef(struct ref_params_s *pparams)
{
	if (g_iStartDist_SCClient && g_iEndDist_SCClient)
	{
		/*
			//Elimate Sv Co-op client's fixed-function fog
			if ( g_iWaterLevel <= 2 && g_iStartDist_SCClient >= 0.0 && g_iEndDist_SCClient > 0.0 )
			{
				glFogi(GL_FOG_MODE, GL_LINEAR);
				v157[0] = *(float *)&dword_1063A6D4 / 255.0;
				v157[1] = *(float *)&dword_1063A6D8 / 255.0;
				v157[2] = *(float *)&dword_1063A6DC / 255.0;
				glFogfv(0xB66u, v157);
				glHint(GL_FOG_HINT, GL_DONT_CARE);
				glFogf(GL_FOG_START, g_iStartDist_SCClient);
				glFogf(GL_FOG_END, g_iEndDist_SCClient);
				glEnable(GL_FOG);
			}
		*/

		float Saved_iStartDist_SCClient = (*g_iStartDist_SCClient);
		float Saved_iEndDist_SCClient = (*g_iEndDist_SCClient);

		(*g_iStartDist_SCClient) = 0;
		(*g_iEndDist_SCClient) = 0;

		gExportfuncs.V_CalcRefdef(pparams);

		(*g_iStartDist_SCClient) = Saved_iStartDist_SCClient;
		(*g_iEndDist_SCClient) = Saved_iEndDist_SCClient;
	}
	else
	{
		gExportfuncs.V_CalcRefdef(pparams);
	}

	memcpy(&r_params, pparams, sizeof(struct ref_params_s));
}

int HUD_Redraw(float time, int intermission)
{
	return gExportfuncs.HUD_Redraw(time, intermission);
}

void EngineStudio_InstalHooks()
{
	Install_InlineHook(studioapi_GL_SetRenderMode);
	Install_InlineHook(studioapi_SetupRenderer);
	Install_InlineHook(studioapi_RestoreRenderer);
	Install_InlineHook(studioapi_StudioDynamicLight);
	Install_InlineHook(studioapi_StudioCheckBBox);
	Install_InlineHook(CL_FxBlend);
}

void ClientStudio_FillAddress(struct r_studio_interface_s** ppinterface)
{
	auto clientBase = g_ClientDLLInfo.ImageBase;
	auto engineBase = g_EngineDLLInfo.ImageBase;

	//Client CGameStudioRenderer vtable and virtual functions resolved from gamedata

	gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioDrawModel", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioDrawPlayer", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

	gPrivateFuncs.GameStudioRenderer_StudioRenderModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderModel))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioRenderModel", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioRenderFinal = (decltype(gPrivateFuncs.GameStudioRenderer_StudioRenderFinal))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioRenderFinal", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioSetupBones", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioSaveBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSaveBones))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioSaveBones", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
	gPrivateFuncs.GameStudioRenderer_StudioMergeBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioMergeBones))
		GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioMergeBones", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

	//Engine Studio render pipeline resolved from gamedata
	gPrivateFuncs.R_StudioDrawModel = (decltype(gPrivateFuncs.R_StudioDrawModel))
		GamedataResolvePtr(engineBase, "R_StudioDrawModel", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioDrawPlayer = (decltype(gPrivateFuncs.R_StudioDrawPlayer))
		GamedataResolvePtr(engineBase, "R_StudioDrawPlayer", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioRenderModel = (decltype(gPrivateFuncs.R_StudioRenderModel))
		GamedataResolvePtr(engineBase, "R_StudioRenderModel", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioRenderFinal = (decltype(gPrivateFuncs.R_StudioRenderFinal))
		GamedataResolvePtr(engineBase, "R_StudioRenderFinal", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioSetupBones = (decltype(gPrivateFuncs.R_StudioSetupBones))
		GamedataResolvePtr(engineBase, "R_StudioSetupBones", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioMergeBones = (decltype(gPrivateFuncs.R_StudioMergeBones))
		GamedataResolvePtr(engineBase, "R_StudioMergeBones", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_StudioSaveBones = (decltype(gPrivateFuncs.R_StudioSaveBones))
		GamedataResolvePtr(engineBase, "R_StudioSaveBones", MH_GAMESYMBOL_KIND_FUNCTION);
}

void ClientStudio_InstallHooks()
{
	if (gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer)
	{
		Install_InlineHook(GameStudioRenderer_StudioDrawPlayer);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioRenderModel)
	{
		Install_InlineHook(GameStudioRenderer_StudioRenderModel);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioRenderFinal)
	{
		Install_InlineHook(GameStudioRenderer_StudioRenderFinal);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioSetupBones)
	{
		Install_InlineHook(GameStudioRenderer_StudioSetupBones);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioSaveBones)
	{
		Install_InlineHook(GameStudioRenderer_StudioSaveBones);
	}
	if (gPrivateFuncs.GameStudioRenderer_StudioMergeBones)
	{
		Install_InlineHook(GameStudioRenderer_StudioMergeBones);
	}

	if (gPrivateFuncs.R_StudioDrawPlayer)
	{
		Install_InlineHook(R_StudioDrawPlayer);
	}
	if (gPrivateFuncs.R_StudioRenderModel)
	{
		Install_InlineHook(R_StudioRenderModel);
	}
	if (gPrivateFuncs.R_StudioRenderFinal)
	{
		Install_InlineHook(R_StudioRenderFinal);
	}
	if (gPrivateFuncs.R_StudioSetupBones)
	{
		Install_InlineHook(R_StudioSetupBones);
	}
	if (gPrivateFuncs.R_StudioSaveBones)
	{
		Install_InlineHook(R_StudioSaveBones);
	}
	if (gPrivateFuncs.R_StudioMergeBones)
	{
		Install_InlineHook(R_StudioMergeBones);
	}
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	gPrivateFuncs.studioapi_GL_SetRenderMode = pstudio->GL_SetRenderMode;
	gPrivateFuncs.studioapi_SetupRenderer = pstudio->SetupRenderer;
	gPrivateFuncs.studioapi_RestoreRenderer = pstudio->RestoreRenderer;
	gPrivateFuncs.studioapi_StudioDynamicLight = pstudio->StudioDynamicLight;
	gPrivateFuncs.studioapi_StudioCheckBBox = pstudio->StudioCheckBBox;

	EngineStudio_InstalHooks();

	pbonetransform = (decltype(pbonetransform))pstudio->StudioGetBoneTransform();
	plighttransform = (decltype(plighttransform))pstudio->StudioGetLightTransform();
	rotationmatrix = (decltype(rotationmatrix))pstudio->StudioGetRotationMatrix();

	pstudio->GetModelCounters(&r_smodels_total, &r_amodels_drawn);

	cl_viewent = gEngfuncs.GetViewModel();

	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	cl_sprite_white = IEngineStudio.Mod_ForName("sprites/white.spr", 1);
	cl_sprite_shell = IEngineStudio.Mod_ForName("sprites/shellchrome.spr", 1);

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	ClientStudio_FillAddress(ppinterface);
	ClientStudio_InstallHooks();

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		spec_pip = gEngfuncs.pfnGetCvarPointer("spec_pip_internal");

		if(!spec_pip)
			spec_pip = gEngfuncs.pfnGetCvarPointer("spec_pip");
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "aomdc"))
	{
		g_bIsAoMDC = true;
	}

	return result;
}

void HUD_PlayerMoveInit(struct playermove_s* ppmove)
{
	gExportfuncs.HUD_PlayerMoveInit(ppmove);

	if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
	{
		pmove_10152 = (decltype(pmove_10152))ppmove;
	}
	else
	{
		pmove = ppmove;
	}
}

void HUD_Frame(double frametime)
{
	R_GameFrameStart();

	gExportfuncs.HUD_Frame(frametime);

	float time = gEngfuncs.GetAbsoluteTime();

	GameThreadTaskScheduler()->RunTasks(time, 0);
}

void HUD_CreateEntities(void)
{
	R_EmitFlashlights();
	R_CreateLowerBodyModel();

	gExportfuncs.HUD_CreateEntities();

	R_AllocateEntityComponentsForVisEdicts();
}

//Client DLL Shutting down...

void HUD_Shutdown(void)
{
	gExportfuncs.HUD_Shutdown();

	R_SaveProgramStates_f();

	ClientStudio_UninstallHooks();
	EngineStudio_UninstallHooks();

	R_Shutdown();

	UtilThreadTask_Shutdown();
}

void HUD_OnClientDisconnect(void)
{
	//The engine have done Mod_Clear before...
	
	//TODO: free bsp VBO?
	//R_FreeUnreferencedStudioRenderData();
}
