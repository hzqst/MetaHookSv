
#include <metahook.h>
#include <capstone.h>
#include <cstring>
#include "gl_local.h"
#include <utlvector.h>
#include <SDL2/SDL_video.h>



#define R_DRAWPARTICLES_SIG_BLOB "\x83\xEC\x40\xA1\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xC0\x0B\x00\x00"


#define S_EXTRAUPDATE_SVENGINE "\xE8\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x05"
#define S_EXTRAUPDATE_BLOB "\xE8\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x2A\x2A\xE9\x2A\x2A\x2A\x2A\xC3"

#define R_DECALSHOTINTERNAL_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x8B\x54\x24\x2A\x8B\x4C\x24\x2A\x53\x8B\x5C\x24\x2A\x56\x69\xF2\xB8\x0B\x00\x00"


#define R_NEWMAP_SIG_COMMON    "\x55\x8B\xEC\x83\xEC\x2A\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC"

#define GL_SHUTDOWN_SIG_HL25 "\xFF\x35\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xFF\x30\xE8"

#define R_DRAWTENTITIESONLIST_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x0F\x2A\x2A\x2A\x00\x00\x8B\x44\x24\x04"

#define R_DRAWSEQUENTIALPOLY_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x8B\x88\xF8\x02\x00\x00\xBE\x01\x00\x00\x00"

#define R_DECALMPOLY_SIG "\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4C\x24\x10\x8B\x51\x18"
#define R_DECALMPOLY_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4D\x0C\x8B\x51\x18\x52\xE8"

#define R_DRAWDECALS_SIG "\xB8\x0C\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0"
#define R_DRAWDECALS_SIG_NEW "\x55\x8B\xEC\xB8\x10\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84"
#define R_DRAWDECALS_SIG_SVENGINE "\xB8\x2A\x2A\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x84\x2A\x2A\x2A\x2A\x53\x8B\x1D"

#define R_RENDERVIEW_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\x83\xEC\x14\xDF\xE0\xF6\xC4"

#define V_RENDERVIEW_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x00\x00\x00\x2A\x2A\x33\x2A\x33\x2A\x2A\x2A\x89\x35\x2A\x2A\x2A\x2A\x89\x35"

#define R_FORCECVARS_SIG_SVENGINE "\x83\x7C\x24\x2A\x00\x2A\x2A\x2A\x2A\x00\x00\x81\x3D\x2A\x2A\x2A\x2A\xFF\x00\x00\x00"
#define R_FORCECVARS_SIG_NEW "\x55\x8B\xEC\x8B\x45\x08\x85\xC0\x0F\x84\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D"

//Studio Funcs
#define R_GLSTUDIODRAWPOINTS_SIG_BLOB2    "\x83\xEC\x48\x8B\x0D\x2A\x2A\x2A\x2A\x8B\x15\x2A\x2A\x2A\x2A\x53\x55\x8B\x41\x54\x8B\x59\x60"

#define BUILDNORMALINDEXTABLE_SIG_BLOB "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x4A\x50\x85\xC9\x2A\x2A\x83\xC8\xFF"
#define BUILDNORMALINDEXTABLE_SIG_NEW "\x55\x8B\xEC\x51\x8B\x15\x2A\x2A\x2A\x2A\x57\x8B\x4A\x50\x85\xC9\x7E\x0A\x83\xC8\xFF\xBF"
#define BUILDNORMALINDEXTABLE_SIG_HL25 ""
#define BUILDNORMALINDEXTABLE_SIG_SVENGINE ""

#define MOD_LOADSPRITEMODEL_BLOB		"\x53\x55\x56\x57\x8B\x7C\x24\x18\x8B\x47\x04\x50\xFF\x15"

#define R_INITPARTICLETEXTURE_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x2A\x00\x00\x8B\xC8\x40"
#define R_INITPARTICLETEXTURE_COMMON "\x68\x01\x14\x00\x00\x68\x08\x19\x00\x00\x6A\x00\x6A\x08\x6A\x08"

#define DRAWSTARTUPGRAPHIC_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x2C\x53\x56\x57\x8B\xF9\x8B\x87\xA8\x01\x00\x00"


#define DRAW_SPRITEFRAMEHOLES_BLOB "\x68\xC0\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D"

#define DRAW_SPRITEFRAMEADDITIVE_BLOB "\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x6A\x01\x6A\x01\xFF\x15\x2A\x2A\x2A\x2A\x8B\x44\x24\x14"
#define DRAW_SPRITEFRAMEADDITIVE_NEW "\x55\x8B\xEC\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x6A\x01\x6A\x01\xFF\x15"

#define DRAW_SPRITEFRAMEGENERIC_BLOB "\x8B\x44\x24\x20\x8B\x4C\x24\x24\x2A\x2A\x8B\x74\x24\x0C\x2A\x68\xE2\x0B\x00\x00\x8B\x3E"

#define DRAW_FILLEDRGBA_BLOB "\x83\xEC\x08\x8D\x44\x24\x28\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x51\xFF\x15\x2A\x2A\x2A\x2A\x83\xC4\x20\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x00\x04\x46\x68\x00\x22\x00\x00\x68\x00\x23\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x6A\x01"

#define DRAW_FILLEDRGBABLEND_BLOB "\x83\xEC\x08\x8D\x44\x24\x28\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x51\xFF\x15\x2A\x2A\x2A\x2A\x83\xC4\x20\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x00\x04\x46\x68\x00\x22\x00\x00\x68\x00\x23\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x03\x03\x00\x00"


#define D_FILLRECT_BLOB "\x83\xEC\x08\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x00\x04\x46\x68\x00\x22\x00\x00\x68\x00\x23\x00\x00"

#define DRAW_PIC_BLOB "\x51\x56\x8B\x74\x24\x14\x85\xF6\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\xE1\x0D\x00\x00"

static hook_t* g_phook_GL_Init = NULL;
static hook_t* g_phook_GL_SetMode_SvEngine = NULL;
static hook_t* g_phook_GL_SetMode_GoldSrc = NULL;
static hook_t* g_phook_GL_SetModeLegacy = NULL;
static hook_t* g_phook_GL_SelectPixelFormat = NULL;
static hook_t* g_phook_GL_Bind = NULL;
static hook_t* g_phook_GL_Set2D = NULL;
static hook_t* g_phook_GL_Finish2D = NULL;
static hook_t* g_phook_GL_BeginRendering = NULL;
static hook_t* g_phook_GL_EndRendering = NULL;
static hook_t* g_phook_R_RenderView_SvEngine = NULL;
static hook_t* g_phook_R_RenderView = NULL;
static hook_t* g_phook_R_LoadSkyBox_SvEngine = NULL;
static hook_t* g_phook_R_LoadSkys = NULL;
static hook_t* g_phook_R_NewMap = NULL;
static hook_t* g_phook_R_CullBox = NULL;
static hook_t* g_phook_R_ForceCVars = NULL;
static hook_t* g_phook_Mod_PointInLeaf = NULL;
static hook_t* g_phook_R_GLStudioDrawPoints = NULL;
static hook_t* g_phook_GL_UnloadTextures = NULL;
static hook_t* g_phook_GL_LoadFilterTexture = NULL;
static hook_t* g_phook_GL_LoadTexture2 = NULL;
static hook_t* g_phook_GL_BuildLightmaps = NULL;
static hook_t* g_phook_LegacyMultiTextureInit = NULL;
static hook_t* g_phook_Mod_LoadStudioModel = NULL;
static hook_t* g_phook_Mod_LoadBrushModel = NULL;
static hook_t* g_phook_Mod_LoadSpriteModel = NULL;
static hook_t* g_phook_Mod_UnloadSpriteTextures = NULL;
static hook_t* g_phook_triapi_RenderMode = NULL;
static hook_t* g_phook_triapi_Begin = NULL;
static hook_t* g_phook_triapi_End = NULL;
static hook_t* g_phook_triapi_Color4f = NULL;
static hook_t* g_phook_triapi_Color4ub = NULL;
static hook_t* g_phook_triapi_TexCoord2f = NULL;
static hook_t* g_phook_triapi_Vertex3fv = NULL;
static hook_t* g_phook_triapi_Vertex3f = NULL;
static hook_t* g_phook_triapi_Brightness = NULL;
static hook_t* g_phook_triapi_Color4fRendermode = NULL;
static hook_t* g_phook_triapi_Fog = NULL;
static hook_t* g_phook_triapi_GetMatrix = NULL;
static hook_t* g_phook_BuildGammaTable = NULL;
static hook_t* g_phook_DLL_SetModKey = NULL;
static hook_t* g_phook_SDL_GL_SetAttribute = NULL;
static hook_t* g_phook_PVSNode = NULL;
static hook_t* g_phook_Host_ClearMemory = NULL;
static hook_t* g_phook_CVideoMode_Common_DrawStartupGraphic = NULL;
static hook_t* g_phook_CGame_DrawStartupVideo = NULL;
static hook_t* g_phook_Draw_Frame = NULL;
static hook_t* g_phook_Draw_SpriteFrameHoles = NULL;
static hook_t* g_phook_Draw_SpriteFrameHoles_SvEngine = NULL;
static hook_t* g_phook_Draw_SpriteFrameAdditive = NULL;
static hook_t* g_phook_Draw_SpriteFrameAdditive_SvEngine = NULL;
static hook_t* g_phook_Draw_SpriteFrameGeneric = NULL;
static hook_t* g_phook_Draw_SpriteFrameGeneric_SvEngine = NULL;
static hook_t* g_phook_Draw_FillRGBA = NULL;
static hook_t* g_phook_Draw_FillRGBABlend = NULL;
static hook_t* g_phook_Draw_FillRGBABuf = NULL;
static hook_t* g_phook_Draw_Pic = NULL;
static hook_t* g_phook_D_FillRect = NULL;
static hook_t* g_phook_R_GetSpriteFrame = NULL;

static hook_t* g_phook_ClientPortalManager_ResetAll = NULL;
static hook_t* g_phook_ClientPortalManager_DrawPortalSurface = NULL;
static hook_t* g_phook_ClientPortalManager_EnableClipPlane = NULL;
static hook_t* g_phook_ClientPortalManager_RenderPortals = NULL;
static hook_t* g_phook_UpdatePlayerPitch = NULL;

void Engine_FillAddress_HasOfficialGLTexAllocSupport(const mh_dll_info_t& RealDllInfo)
{
	//Legacy engines publish the counter used by the texture allocation redirect.
	const auto status = g_pMetaHookAPI->IsGameSymbolAvailable(RealDllInfo.ImageBase, "texture_extension_number");
	if (status == MH_GAMESYMBOL_OK)
	{
		g_bHasOfficialGLTexAllocSupport = false;
	}
	else if (status == MH_GAMESYMBOL_SYMBOL_NOT_FOUND)
	{
		g_bHasOfficialGLTexAllocSupport = true;
	}
	else
	{
		Sys_Error("Could not query gamedata symbol: texture_extension_number (%s)\nEngine buildnum: %d",
			g_pMetaHookAPI->GetGameSymbolStatusString(status), g_dwEngineBuildnum);
	}
}

void Engine_FillAddress_GL_Init(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_Init = (decltype(gPrivateFuncs.GL_Init))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Init", MH_GAMESYMBOL_KIND_FUNCTION);
	gl_extensions = (decltype(gl_extensions))GamedataResolvePtr(RealDllInfo.ImageBase, "gl_extensions", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_GL_SetMode(const mh_dll_info_t& RealDllInfo)
{
	//SvEngine uses the true 3-arg ABI, SDL GoldSrc/HL25 keep the six-arg ABI and
	//every other identity (legacy GoldSrc, blob builds, CoF) uses the legacy entry.
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_SetMode_SvEngine = (decltype(gPrivateFuncs.GL_SetMode_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetMode", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25 || gPrivateFuncs.SDL_GL_GetProcAddress)
	{
		gPrivateFuncs.GL_SetMode_GoldSrc = (decltype(gPrivateFuncs.GL_SetMode_GoldSrc))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetMode", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.GL_SetModeLegacy = (decltype(gPrivateFuncs.GL_SetModeLegacy))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetModeLegacy", MH_GAMESYMBOL_KIND_FUNCTION);
	}

	//The patch record is the FF 15 call instruction to redirect; SvEngine never uses it.
	if (g_iEngineType != ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_SetMode_call_qwglCreateContext = (decltype(gPrivateFuncs.GL_SetMode_call_qwglCreateContext))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetMode_call_qwglCreateContext", MH_GAMESYMBOL_KIND_PATCH);
	}

	if (gPrivateFuncs.SDL_GL_GetProcAddress &&
		(g_iEngineType == ENGINE_GOLDSRC || g_iEngineType == ENGINE_GOLDSRC_HL25))
	{
		gPrivateFuncs.SDL_InitGL = (decltype(gPrivateFuncs.SDL_InitGL))GamedataResolvePtr(RealDllInfo.ImageBase, "SDL_InitGL", MH_GAMESYMBOL_KIND_FUNCTION);
	}

	if (gPrivateFuncs.GL_SetModeLegacy)
	{
		gPrivateFuncs.GL_SelectPixelFormat = (decltype(gPrivateFuncs.GL_SelectPixelFormat))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SelectPixelFormat", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_GL_Bind(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Bind", MH_GAMESYMBOL_KIND_FUNCTION);

	currenttexture = (decltype(currenttexture))GamedataResolvePtr(RealDllInfo.ImageBase, "currenttexture", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_GL_LoadTexture2(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_LoadTexture2", MH_GAMESYMBOL_KIND_FUNCTION);
	gHostSpawnCount = (decltype(gHostSpawnCount))GamedataResolvePtr(RealDllInfo.ImageBase, "gHostSpawnCount", MH_GAMESYMBOL_KIND_GLOBAL);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gltextures_SvEngine = (decltype(gltextures_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "gltextures", MH_GAMESYMBOL_KIND_GLOBAL);
		numgltextures = (decltype(numgltextures))GamedataResolvePtr(RealDllInfo.ImageBase, "gltextures.m_Size", MH_GAMESYMBOL_KIND_GLOBAL);
		maxgltextures_SvEngine = (decltype(maxgltextures_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "gltextures.m_Memory.m_nAllocationCount", MH_GAMESYMBOL_KIND_GLOBAL);
		peakgltextures_SvEngine = (decltype(peakgltextures_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "peakgltextures", MH_GAMESYMBOL_KIND_GLOBAL);
		gPrivateFuncs.realloc_SvEngine = (decltype(gPrivateFuncs.realloc_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "realloc", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gltextures = (decltype(gltextures))GamedataResolvePtr(RealDllInfo.ImageBase, "gltextures", MH_GAMESYMBOL_KIND_GLOBAL);
		numgltextures = (decltype(numgltextures))GamedataResolvePtr(RealDllInfo.ImageBase, "numgltextures", MH_GAMESYMBOL_KIND_GLOBAL);
	}
}

void Engine_FillAddress_R_CullBox(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))GamedataResolvePtr(RealDllInfo.ImageBase, "R_CullBox", MH_GAMESYMBOL_KIND_FUNCTION);
	frustum = (decltype(frustum))GamedataResolvePtr(RealDllInfo.ImageBase, "frustum", MH_GAMESYMBOL_KIND_GLOBAL);
	vpn = (decltype(vpn))GamedataResolvePtr(RealDllInfo.ImageBase, "vpn", MH_GAMESYMBOL_KIND_GLOBAL);
	vup = (decltype(vup))GamedataResolvePtr(RealDllInfo.ImageBase, "vup", MH_GAMESYMBOL_KIND_GLOBAL);
	vright = (decltype(vright))GamedataResolvePtr(RealDllInfo.ImageBase, "vright", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_SetupGL(const mh_dll_info_t& RealDllInfo)
{
	gWorldToScreen = (decltype(gWorldToScreen))GamedataResolvePtr(RealDllInfo.ImageBase, "gWorldToScreen", MH_GAMESYMBOL_KIND_GLOBAL);
	gScreenToWorld = (decltype(gScreenToWorld))GamedataResolvePtr(RealDllInfo.ImageBase, "gScreenToWorld", MH_GAMESYMBOL_KIND_GLOBAL);
	r_world_matrix = (decltype(r_world_matrix))GamedataResolvePtr(RealDllInfo.ImageBase, "r_world_matrix", MH_GAMESYMBOL_KIND_GLOBAL);
	r_projection_matrix = (decltype(r_projection_matrix))GamedataResolvePtr(RealDllInfo.ImageBase, "gProjectionMatrix", MH_GAMESYMBOL_KIND_GLOBAL);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		vertical_fov_SvEngine = (decltype(vertical_fov_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "gmodinfo_vertical_fov", MH_GAMESYMBOL_KIND_GLOBAL);
	}
}

void Engine_FillAddress_R_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID R_RenderView_VA = (PVOID)GamedataResolvePtr(RealDllInfo.ImageBase, "R_RenderView", MH_GAMESYMBOL_KIND_FUNCTION);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_RenderView_SvEngine = (decltype(gPrivateFuncs.R_RenderView_SvEngine))R_RenderView_VA;
	}
	else
	{
		gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))R_RenderView_VA;
	}


	//SvEngine publishes the alias counter under its own name, c_model_polys.
	c_alias_polys = (decltype(c_alias_polys))GamedataResolvePtr(RealDllInfo.ImageBase,
		(g_iEngineType == ENGINE_SVENGINE) ? "c_model_polys" : "c_alias_polys", MH_GAMESYMBOL_KIND_GLOBAL);
	c_brush_polys = (decltype(c_brush_polys))GamedataResolvePtr(RealDllInfo.ImageBase, "c_brush_polys", MH_GAMESYMBOL_KIND_GLOBAL);

	r_worldentity = (decltype(r_worldentity))GamedataResolvePtr(RealDllInfo.ImageBase, "r_worldentity", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_worldmodel = (decltype(cl_worldmodel))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_worldmodel", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_V_RenderView(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))GamedataResolvePtr(RealDllInfo.ImageBase, "V_RenderView", MH_GAMESYMBOL_KIND_FUNCTION);
	r_playerViewportAngles = (decltype(r_playerViewportAngles))GamedataResolvePtr(RealDllInfo.ImageBase, "r_playerViewportAngles", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_NewMap(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))GamedataResolvePtr(RealDllInfo.ImageBase, "R_NewMap", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.GL_UnloadTextures = (decltype(gPrivateFuncs.GL_UnloadTextures))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_UnloadTextures", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_DrawSequentialPoly(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID R_DrawSequentialPoly_VA = (PVOID)GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawSequentialPoly", MH_GAMESYMBOL_KIND_FUNCTION);

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_DrawSequentialPoly_HL25 = (decltype(gPrivateFuncs.R_DrawSequentialPoly_HL25))R_DrawSequentialPoly_VA;
	}
	else
	{
		gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))R_DrawSequentialPoly_VA;
	}


	/*
		//Global pointers that link into engine vars
		byte *lightmaps = NULL;
		int *gDecalSurfCount = NULL;
	*/
	lightmaps = (decltype(lightmaps))GamedataResolvePtr(RealDllInfo.ImageBase, "lightmaps", MH_GAMESYMBOL_KIND_GLOBAL);
	gDecalSurfCount = (decltype(gDecalSurfCount))GamedataResolvePtr(RealDllInfo.ImageBase, "gDecalSurfCount", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_DrawWorld(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	modelorg = (decltype(modelorg))GamedataResolvePtr(RealDllInfo.ImageBase, "modelorg", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_DrawViewModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	envmap = (decltype(envmap))GamedataResolvePtr(RealDllInfo.ImageBase, "envmap", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_stats = (decltype(cl_stats))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_stats", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_weaponstarttime = (decltype(cl_weaponstarttime))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_weaponstarttime", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_weaponsequence = (decltype(cl_weaponsequence))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_weaponsequence", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_light_level = (decltype(cl_light_level))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_light_level", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_MarkLeaves(const mh_dll_info_t& RealDllInfo)
{
	r_viewleaf = (decltype(r_viewleaf))GamedataResolvePtr(RealDllInfo.ImageBase, "r_viewleaf", MH_GAMESYMBOL_KIND_GLOBAL);
	r_oldviewleaf = (decltype(r_oldviewleaf))GamedataResolvePtr(RealDllInfo.ImageBase, "r_oldviewleaf", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_VID_UpdateWindowVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	window_rect = (decltype(window_rect))GamedataResolvePtr(RealDllInfo.ImageBase, "window_rect", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_BuildGammaTable(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))GamedataResolvePtr(RealDllInfo.ImageBase, "BuildGammaTable", MH_GAMESYMBOL_KIND_FUNCTION);

	texgammatable = (decltype(texgammatable))GamedataResolvePtr(RealDllInfo.ImageBase, "texgammatable", MH_GAMESYMBOL_KIND_GLOBAL);
	lightgammatable = (decltype(lightgammatable))GamedataResolvePtr(RealDllInfo.ImageBase, "lightgammatable", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_DrawParticles(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawParticles", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_FreeDeadParticles = (decltype(gPrivateFuncs.R_FreeDeadParticles))GamedataResolvePtr(RealDllInfo.ImageBase, "R_FreeDeadParticles", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_TracerDraw = (decltype(gPrivateFuncs.R_TracerDraw))GamedataResolvePtr(RealDllInfo.ImageBase, "R_TracerDraw", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_BeamDrawList = (decltype(gPrivateFuncs.R_BeamDrawList))GamedataResolvePtr(RealDllInfo.ImageBase, "R_BeamDrawList", MH_GAMESYMBOL_KIND_FUNCTION);

	active_particles = (decltype(active_particles))GamedataResolvePtr(RealDllInfo.ImageBase, "active_particles", MH_GAMESYMBOL_KIND_GLOBAL);
	particletexture = (decltype(particletexture))GamedataResolvePtr(RealDllInfo.ImageBase, "particletexture", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_StudioLighting(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	r_ambientlight = (decltype(r_ambientlight))GamedataResolvePtr(RealDllInfo.ImageBase, "r_ambientlight", MH_GAMESYMBOL_KIND_GLOBAL);
	r_shadelight = (decltype(r_shadelight))GamedataResolvePtr(RealDllInfo.ImageBase, "r_shadelight", MH_GAMESYMBOL_KIND_GLOBAL);
	r_plightvec = (decltype(r_plightvec))GamedataResolvePtr(RealDllInfo.ImageBase, "r_plightvec", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Cache_Alloc(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Cache_Alloc = (decltype(gPrivateFuncs.Cache_Alloc))GamedataResolvePtr(RealDllInfo.ImageBase, "Cache_Alloc", MH_GAMESYMBOL_KIND_FUNCTION);

	cache_head = (decltype(cache_head))GamedataResolvePtr(RealDllInfo.ImageBase, "cache_head", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Draw_DecalTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_DecalTexture", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GlowBlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//GlowBlend is inlined on SvEngine and HL25 and has no catalog record there.
	if (g_iEngineType == ENGINE_SVENGINE || g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		return;
	}

	gPrivateFuncs.GlowBlend = (decltype(gPrivateFuncs.GlowBlend))GamedataResolvePtr(RealDllInfo.ImageBase, "GlowBlend", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_SCR_BeginLoadingPlaque(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	scr_drawloading = (decltype(scr_drawloading))GamedataResolvePtr(RealDllInfo.ImageBase, "scr_drawloading", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Mod_LoadSpriteFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gSpriteMipMap = (decltype(gSpriteMipMap))GamedataResolvePtr(RealDllInfo.ImageBase, "gSpriteMipMap", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Hunk_AllocName(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Hunk_AllocName = (decltype(gPrivateFuncs.Hunk_AllocName))GamedataResolvePtr(RealDllInfo.ImageBase, "Hunk_AllocName", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_EndRenderingVars(const mh_dll_info_t& RealDllInfo)
{
	const auto status = g_pMetaHookAPI->IsGameSymbolAvailable(RealDllInfo.ImageBase, "s_fXMouseAspectAdjustment");
	if (status == MH_GAMESYMBOL_OK)
	{
		s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))GamedataResolvePtr(RealDllInfo.ImageBase, "s_fXMouseAspectAdjustment", MH_GAMESYMBOL_KIND_GLOBAL);
		s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))GamedataResolvePtr(RealDllInfo.ImageBase, "s_fYMouseAspectAdjustment", MH_GAMESYMBOL_KIND_GLOBAL);
	}
	else if (status == MH_GAMESYMBOL_SYMBOL_NOT_FOUND)
	{
		s_fXMouseAspectAdjustment = &s_fXMouseAspectAdjustment_Storage;
		s_fYMouseAspectAdjustment = &s_fYMouseAspectAdjustment_Storage;
	}
	else
	{
		Sys_Error("Could not query gamedata symbol: s_fXMouseAspectAdjustment (%s)\nEngine buildnum: %d",
			g_pMetaHookAPI->GetGameSymbolStatusString(status), g_dwEngineBuildnum);
	}
}

void Engine_FillAddress_R_AllocTransObjectsVars(const mh_dll_info_t& RealDllInfo)
{
	numTransObjs = (decltype(numTransObjs))GamedataResolvePtr(RealDllInfo.ImageBase, "numTransObjs", MH_GAMESYMBOL_KIND_GLOBAL);
	maxTransObjs = (decltype(maxTransObjs))GamedataResolvePtr(RealDllInfo.ImageBase, "maxTransObjs", MH_GAMESYMBOL_KIND_GLOBAL);
	transObjects = (decltype(transObjects))GamedataResolvePtr(RealDllInfo.ImageBase, "transObjects", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_RenderFinalFog(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	g_bUserFogOn = (decltype(g_bUserFogOn))GamedataResolvePtr(RealDllInfo.ImageBase, "g_bUserFogOn", MH_GAMESYMBOL_KIND_GLOBAL);

	flFinalFogColor = (decltype(flFinalFogColor))GamedataResolvePtr(RealDllInfo.ImageBase, "flFinalFogColor", MH_GAMESYMBOL_KIND_GLOBAL);

	flFogDensity = (decltype(flFogDensity))GamedataResolvePtr(RealDllInfo.ImageBase, "flFogDensity", MH_GAMESYMBOL_KIND_GLOBAL);

	flFogStart = (decltype(flFogStart))GamedataResolvePtr(RealDllInfo.ImageBase, "flFogStart", MH_GAMESYMBOL_KIND_GLOBAL);

	flFogEnd = (decltype(flFogEnd))GamedataResolvePtr(RealDllInfo.ImageBase, "flFogEnd", MH_GAMESYMBOL_KIND_GLOBAL);
}


void Engine_FillAddress_R_DrawTEntitiesOnListVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		float* r_blend = NULL;
		int *cl_parsecount = NULL;
	*/
	cl_parsecount = (decltype(cl_parsecount))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_parsecount", MH_GAMESYMBOL_KIND_GLOBAL);

	r_blend = (decltype(r_blend))GamedataResolvePtr(RealDllInfo.ImageBase, "r_blend", MH_GAMESYMBOL_KIND_GLOBAL);

	r_entorigin = (decltype(r_entorigin))GamedataResolvePtr(RealDllInfo.ImageBase, "r_entorigin", MH_GAMESYMBOL_KIND_GLOBAL);

	gPrivateFuncs.ClientDLL_DrawTransparentTriangles = (decltype(gPrivateFuncs.ClientDLL_DrawTransparentTriangles))GamedataResolvePtr(RealDllInfo.ImageBase, "ClientDLL_DrawTransparentTriangles", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_RecursiveWorldNodeVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars.
		int *r_framecount = NULL;
		int *r_visframecount = NULL;
	*/

	r_framecount = (decltype(r_framecount))GamedataResolvePtr(RealDllInfo.ImageBase, "r_framecount", MH_GAMESYMBOL_KIND_GLOBAL);

	r_visframecount = (decltype(r_visframecount))GamedataResolvePtr(RealDllInfo.ImageBase, "r_visframecount", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_LoadSkybox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_LoadSkyBox_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyBox_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "R_LoadSkyBox_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.R_LoadSkys = (decltype(gPrivateFuncs.R_LoadSkys))GamedataResolvePtr(RealDllInfo.ImageBase, "R_LoadSkys", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_GL_FilterMinMaxVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars.
		int *gl_filter_min = NULL;
		int *gl_filter_max = NULL;
	*/

	gl_filter_min = (decltype(gl_filter_min))GamedataResolvePtr(RealDllInfo.ImageBase, "gl_filter_min", MH_GAMESYMBOL_KIND_GLOBAL);

	gl_filter_max = (decltype(gl_filter_max))GamedataResolvePtr(RealDllInfo.ImageBase, "gl_filter_max", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_ScrFov(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars.
		cvar_t scr_fov;
		scr_fov.value;
	*/

	scr_fov_value = (decltype(scr_fov_value))GamedataResolvePtr(RealDllInfo.ImageBase, "scr_fov_value", MH_GAMESYMBOL_KIND_GLOBAL);
}

//Got CL_IsDevOverviewMode, CL_SetDevOverView and refdef here
void Engine_FillAddress_RenderSceneVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	 // We use our own struct and use pointer to link to actual engine refdef_t
	 // because SvEngine and GoldSrc have different refdef_t definitions.

		 typedef struct refdef_SvEngine_s
		{
			vrect_GoldSrc_t vrect;
			vec3_t vieworg;
			vec3_t viewangles;
			color24 ambientlight;
			byte padding;
			qboolean onlyClientDraws;
			qboolean useCamera;
			vec3_t r_camera_origin;
		}refdef_SvEngine_t;

		typedef struct refdef_GoldSrc_s
		{
			vrect_GoldSrc_t vrect;
			char padding[96];
			vec3_t vieworg;
			vec3_t viewangles;
			color24 ambientlight;
			byte padding2;
			qboolean onlyClientDraws;
		}refdef_GoldSrc_t;

		typedef struct refdef_s
		{
			vrect_GoldSrc_t *vrect;// link to &enginedll_refdef.vrect
			vec3_t *vieworg; // link to &enginedll_refdef.vieworg
			vec3_t *viewangles; // link to &enginedll_refdef.viewangles
			color24 *ambientlight;// link to &enginedll_refdef.ambientlight
			qboolean *onlyClientDraws;// link to &enginedll_refdef.onlyClientDraws
		}refdef_t;

		refdef_t r_refdef;
	*/

	gPrivateFuncs.CL_IsDevOverviewMode = (decltype(gPrivateFuncs.CL_IsDevOverviewMode))GamedataResolvePtr(RealDllInfo.ImageBase, "CL_IsDevOverviewMode", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.CL_SetDevOverView = (decltype(gPrivateFuncs.CL_SetDevOverView))GamedataResolvePtr(RealDllInfo.ImageBase, "CL_SetDevOverView", MH_GAMESYMBOL_KIND_FUNCTION);

	//Both engine families publish the same r_refdef global; only the local view of its
	//layout differs, so link the pointers through whichever struct matches the engine.
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		r_refdef_SvEngine = (decltype(r_refdef_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "r_refdef", MH_GAMESYMBOL_KIND_GLOBAL);

		r_refdef.vrect = &r_refdef_SvEngine->vrect;
		r_refdef.vieworg = &r_refdef_SvEngine->vieworg;
		r_refdef.viewangles = &r_refdef_SvEngine->viewangles;
		r_refdef.ambientlight = &r_refdef_SvEngine->ambientlight;
		r_refdef.onlyClientDraws = &r_refdef_SvEngine->onlyClientDraws;
	}
	else
	{
		r_refdef_GoldSrc = (decltype(r_refdef_GoldSrc))GamedataResolvePtr(RealDllInfo.ImageBase, "r_refdef", MH_GAMESYMBOL_KIND_GLOBAL);

		r_refdef.vrect = &r_refdef_GoldSrc->vrect;
		r_refdef.vieworg = &r_refdef_GoldSrc->vieworg;
		r_refdef.viewangles = &r_refdef_GoldSrc->viewangles;
		r_refdef.ambientlight = &r_refdef_GoldSrc->ambientlight;
		r_refdef.onlyClientDraws = &r_refdef_GoldSrc->onlyClientDraws;
	}
}

//Got ClientDLL_DrawNormalTriangles_VA and gDevOverview here
void Engine_FillAddress_RenderSceneVars2(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	cl_waterlevel = (decltype(cl_waterlevel))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_waterlevel", MH_GAMESYMBOL_KIND_GLOBAL);

	gPrivateFuncs.ClientDLL_DrawNormalTriangles = (decltype(gPrivateFuncs.ClientDLL_DrawNormalTriangles))GamedataResolvePtr(RealDllInfo.ImageBase, "ClientDLL_DrawNormalTriangles", MH_GAMESYMBOL_KIND_FUNCTION);

	gDevOverview = (decltype(gDevOverview))GamedataResolvePtr(RealDllInfo.ImageBase, "gDevOverview", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_CL_IsDevOverviewModeVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		allow_cheats = (decltype(allow_cheats))GamedataResolvePtr(RealDllInfo.ImageBase, "allow_cheats", MH_GAMESYMBOL_KIND_GLOBAL);
	}
	else
	{
		// "int allow_cheats" is not a thing in GoldSrc 
	}
}

void Engine_FillAddress_R_DecalInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gDecalPool = (decltype(gDecalPool))GamedataResolvePtr(RealDllInfo.ImageBase, "gDecalPool", MH_GAMESYMBOL_KIND_GLOBAL);

	gDecalCache = (decltype(gDecalCache))GamedataResolvePtr(RealDllInfo.ImageBase, "gDecalCache", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_LightstyleVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	d_lightstylevalue = (decltype(d_lightstylevalue))GamedataResolvePtr(RealDllInfo.ImageBase, "d_lightstylevalue", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_WaterVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	cshift_water = (decltype(cshift_water))GamedataResolvePtr(RealDllInfo.ImageBase, "cshift_water", MH_GAMESYMBOL_KIND_GLOBAL);

	gWaterColor = (decltype(gWaterColor))GamedataResolvePtr(RealDllInfo.ImageBase, "gWaterColor", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_BasePalette(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	host_basepal = (decltype(host_basepal))GamedataResolvePtr(RealDllInfo.ImageBase, "host_basepal", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_MoveVars(const mh_dll_info_t& RealDllInfo)
{
	pmovevars = (decltype(pmovevars))GamedataResolvePtr(RealDllInfo.ImageBase, "movevars", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_MissingTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		r_missingtexture = (decltype(r_missingtexture))GamedataResolvePtr(RealDllInfo.ImageBase, "r_missingtexture", MH_GAMESYMBOL_KIND_GLOBAL);
	}
}

void Engine_FillAddress_NoTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType != ENGINE_SVENGINE)
	{
		r_notexture_mip = (decltype(r_notexture_mip))GamedataResolvePtr(RealDllInfo.ImageBase, "r_notexture_mip", MH_GAMESYMBOL_KIND_GLOBAL);
	}
}

void Engine_FillAddress_LegacyMultiTextureInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	detTexSupported = (decltype(detTexSupported))GamedataResolvePtr(RealDllInfo.ImageBase, "detTexSupported", MH_GAMESYMBOL_KIND_GLOBAL);

	//We only need to neuter one function per engine: the legacy multitexture probe is the
	//sole caller of DT_Initialize wherever both exist, so hooking the probe covers both.
	//HL/CoF/blob call it CheckMultiTextureExtensions, SvEngine calls it InitMultitexturing.
	//HL25 and SvEngine inline the probe into GL_Init on Windows and publish no record for
	//it, so those fall back to the standalone DT_Initialize that GL_Init still calls.
	static const char* s_LegacyMultiTextureInitNames[] = {
		"CheckMultiTextureExtensions",
		"InitMultitexturing",
		"DT_Initialize",
	};

	for (auto name : s_LegacyMultiTextureInitNames)
	{
		gPrivateFuncs.LegacyMultiTextureInit = (decltype(gPrivateFuncs.LegacyMultiTextureInit))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, name, MH_GAMESYMBOL_KIND_FUNCTION);

		if (gPrivateFuncs.LegacyMultiTextureInit)
			break;
	}

	Sig_FuncNotFound(LegacyMultiTextureInit);
}

void Engine_FillAddress_DrawStartupGraphic(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.CVideoMode_Common_DrawStartupGraphic = (decltype(gPrivateFuncs.CVideoMode_Common_DrawStartupGraphic))GamedataResolvePtr(RealDllInfo.ImageBase, "CVideoMode_Common_DrawStartupGraphic", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.offset_CVideoMode_Common_m_ImageID = GamedataQueryStructMember(RealDllInfo.ImageBase, "CVideoMode_Common.m_ImageID");
	gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResX = GamedataQueryStructMember(RealDllInfo.ImageBase, "CVideoMode_Common.m_iBaseResX");
	gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResY = GamedataQueryStructMember(RealDllInfo.ImageBase, "CVideoMode_Common.m_iBaseResY");
}

void Engine_FillAddress_DrawStartupVideo(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//Only available in HL25
	if (g_iEngineType != ENGINE_GOLDSRC_HL25)
		return;

	gPrivateFuncs.CGame_DrawStartupVideo = (decltype(gPrivateFuncs.CGame_DrawStartupVideo))GamedataResolvePtr(RealDllInfo.ImageBase, "CGame_DrawStartupVideo", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Draw_Frame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Draw_Frame = (decltype(gPrivateFuncs.Draw_Frame))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_Frame", MH_GAMESYMBOL_KIND_FUNCTION);

	giScissorTest = (decltype(giScissorTest))GamedataResolvePtr(RealDllInfo.ImageBase, "giScissorTest", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Draw_SpriteFrameHoles(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.Draw_SpriteFrameHoles_SvEngine = (decltype(gPrivateFuncs.Draw_SpriteFrameHoles_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameHoles_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.Draw_SpriteFrameHoles = (decltype(gPrivateFuncs.Draw_SpriteFrameHoles))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameHoles", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_Draw_SpriteFrameAdditive(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.Draw_SpriteFrameAdditive_SvEngine = (decltype(gPrivateFuncs.Draw_SpriteFrameAdditive_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameAdditive_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.Draw_SpriteFrameAdditive = (decltype(gPrivateFuncs.Draw_SpriteFrameAdditive))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameAdditive", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_Draw_SpriteFrameGeneric(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.Draw_SpriteFrameGeneric_SvEngine = (decltype(gPrivateFuncs.Draw_SpriteFrameGeneric_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameGeneric_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.Draw_SpriteFrameGeneric = (decltype(gPrivateFuncs.Draw_SpriteFrameGeneric))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameGeneric", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

//SvEngine replaced the immediate-mode netgraph rectangle with a buffered one that takes
//eight integers (x, y, w, h, r, g, b, a) and appends four vertices to a 1024-entry vertex
//buffer. The catalog used to publish that body under the wrong name NET_DrawRect; it is
//now published as Draw_FillRGBABuf with no compatibility alias, so the old name no longer
//resolves. No other engine identity carries it.
void Engine_FillAddress_Draw_FillRGBABuf(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType != ENGINE_SVENGINE)
		return;

	gPrivateFuncs.Draw_FillRGBABuf = (decltype(gPrivateFuncs.Draw_FillRGBABuf))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_FillRGBABuf", MH_GAMESYMBOL_KIND_FUNCTION);
}

//Every engine identity publishes Mod_UnloadSpriteTextures as a standalone body that
//SPR_Shutdown still calls out of line, SvEngine and HL25 included, so the inline hook
//on it covers the HUD sprite list too and there is no branch left to keep.
//take it carefully with linux build ! Mod_UnloadSpriteTextures can be inlined into SPR_Shutdown in linux build !
void Engine_FillAddress_Mod_UnloadSpriteTextures(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_UnloadSpriteTextures", MH_GAMESYMBOL_KIND_FUNCTION);
}

//SvEngine keeps the cl_enginefuncs slots 11 and 130, but the entries there only forward to
//the real drawing bodies, so the catalog publishes the bodies for every engine identity and
//the dispatch has no branch left to keep.
void Engine_FillAddress_Draw_FillRGBA(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Draw_FillRGBA = (decltype(gPrivateFuncs.Draw_FillRGBA))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_FillRGBA", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Draw_FillRGBABlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Draw_FillRGBABlend = (decltype(gPrivateFuncs.Draw_FillRGBABlend))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_FillRGBABlend", MH_GAMESYMBOL_KIND_FUNCTION);
}

//SvEngine has no body with the legacy (vrect_t*, color*) interface: its connection message
//fills the rectangle through the eight-integer Draw_FillRGBABlend instead, which is hooked
//on its own, so there is nothing left to resolve or hook here on that engine.
void Engine_FillAddress_D_FillRect(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return;

	gPrivateFuncs.D_FillRect = (decltype(gPrivateFuncs.D_FillRect))GamedataResolvePtr(RealDllInfo.ImageBase, "D_FillRect", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_Shutdown(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_Shutdown = (decltype(gPrivateFuncs.GL_Shutdown))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Shutdown", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.Sys_ShutdownGame_call_GL_Shutdown = (decltype(gPrivateFuncs.Sys_ShutdownGame_call_GL_Shutdown))GamedataResolvePtr(RealDllInfo.ImageBase, "Sys_ShutdownGame_to_GL_Shutdown_callsite_0", MH_GAMESYMBOL_KIND_PATCH);
}

void Engine_FillAddress_V_FadeAlpha(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.V_FadeAlpha = (decltype(gPrivateFuncs.V_FadeAlpha))GamedataResolvePtr(RealDllInfo.ImageBase, "V_FadeAlpha", MH_GAMESYMBOL_KIND_FUNCTION);
	cl_sf = (decltype(cl_sf))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_sf", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_S_ExtraUpdate(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))GamedataResolvePtr(RealDllInfo.ImageBase, "S_ExtraUpdate", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_SelectTexture(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SelectTexture", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_ForceCVars(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_ForceCVars = (decltype(gPrivateFuncs.R_ForceCVars))GamedataResolvePtr(RealDllInfo.ImageBase, "R_ForceCVars", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))GamedataResolvePtr(RealDllInfo.ImageBase, "R_CheckVariables", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))GamedataResolvePtr(RealDllInfo.ImageBase, "R_AnimateLight", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_LoadFilterTexture(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_LoadFilterTexture = (decltype(gPrivateFuncs.GL_LoadFilterTexture))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_LoadFilterTexture", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_BuildLightmaps(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_BuildLightmaps", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_TextureAnimation(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))GamedataResolvePtr(RealDllInfo.ImageBase, "R_TextureAnimation", MH_GAMESYMBOL_KIND_FUNCTION);
	rtable = (decltype(rtable))GamedataResolvePtr(RealDllInfo.ImageBase, "rtable", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_GL_Set2D(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_Set2D = (decltype(gPrivateFuncs.GL_Set2D))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Set2D", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_Finish2D(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_Finish2D = (decltype(gPrivateFuncs.GL_Finish2D))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Finish2D", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_BeginRendering(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_BeginRendering", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GL_EndRendering(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_EndRendering", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Mod_PointInLeaf(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_PointInLeaf", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_DrawTEntitiesOnList(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawTEntitiesOnList", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_CL_DlightVars(const mh_dll_info_t& RealDllInfo)
{
	cl_dlights = (decltype(cl_dlights))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_dlights", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_elights = (decltype(cl_elights))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_elights", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_GLStudioDrawPoints(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))GamedataResolvePtr(RealDllInfo.ImageBase, "R_GLStudioDrawPoints", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Host_ClearMemory(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Host_ClearMemory = (decltype(gPrivateFuncs.Host_ClearMemory))GamedataResolvePtr(RealDllInfo.ImageBase, "Host_ClearMemory", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_GetSpriteFrame(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.R_GetSpriteFrame = (decltype(gPrivateFuncs.R_GetSpriteFrame))GamedataResolvePtr(RealDllInfo.ImageBase, "R_GetSpriteFrame", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Host_IsSinglePlayerGame(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))GamedataResolvePtr(RealDllInfo.ImageBase, "Host_IsSinglePlayerGame", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Mod_LoadSpriteModel(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadSpriteModel", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_VisEdicts(const mh_dll_info_t& RealDllInfo)
{
	cl_numvisedicts = (decltype(cl_numvisedicts))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_numvisedicts", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_visedicts = (decltype(cl_visedicts))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_visedicts", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_CL_SimOrgVars(const mh_dll_info_t& RealDllInfo)
{
	cl_simorg = (decltype(cl_simorg))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_simorg", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& RealDllInfo)
{
	cl_viewentity = (decltype(cl_viewentity))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_viewentity", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_CL_ReallocateDynamicData(const mh_dll_info_t& RealDllInfo)
{
	cl_max_edicts = (decltype(cl_max_edicts))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_max_edicts", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_entities = (decltype(cl_entities))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_entities", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_TempEntsVars(const mh_dll_info_t& RealDllInfo)
{
	gTempEnts = (decltype(gTempEnts))GamedataResolvePtr(RealDllInfo.ImageBase, "gTempEnts", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_ModKnownVars(const mh_dll_info_t& RealDllInfo)
{
	mod_known = (decltype(mod_known))GamedataResolvePtr(RealDllInfo.ImageBase, "mod_known", MH_GAMESYMBOL_KIND_GLOBAL);
	mod_numknown = (decltype(mod_numknown))GamedataResolvePtr(RealDllInfo.ImageBase, "mod_numknown", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Mod_LoadStudioModel(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Mod_LoadStudioModel = (decltype(gPrivateFuncs.Mod_LoadStudioModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadStudioModel", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Mod_LoadBrushModel(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Mod_LoadBrushModel = (decltype(gPrivateFuncs.Mod_LoadBrushModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadBrushModel", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Mod_LoadModel(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Mod_LoadModel = (decltype(gPrivateFuncs.Mod_LoadModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadModel", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_SetFilterMode(const mh_dll_info_t& RealDllInfo)
{
	filterMode = (decltype(filterMode))GamedataResolvePtr(RealDllInfo.ImageBase, "filterMode", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_SetFilterColor(const mh_dll_info_t& RealDllInfo)
{
	filterColorRed = (decltype(filterColorRed))GamedataResolvePtr(RealDllInfo.ImageBase, "filterColorRed", MH_GAMESYMBOL_KIND_GLOBAL);
	filterColorGreen = (decltype(filterColorGreen))GamedataResolvePtr(RealDllInfo.ImageBase, "filterColorGreen", MH_GAMESYMBOL_KIND_GLOBAL);
	filterColorBlue = (decltype(filterColorBlue))GamedataResolvePtr(RealDllInfo.ImageBase, "filterColorBlue", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_SetFilterBrightness(const mh_dll_info_t& RealDllInfo)
{
	filterBrightness = (decltype(filterBrightness))GamedataResolvePtr(RealDllInfo.ImageBase, "filterBrightness", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_PVSNode(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.PVSNode = (decltype(gPrivateFuncs.PVSNode))GamedataResolvePtr(RealDllInfo.ImageBase, "PVSNode", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Draw_Pic(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.Draw_Pic = (decltype(gPrivateFuncs.Draw_Pic))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_Pic", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto hSDL2 = GetModuleHandleA("SDL2.dll");

	if (hSDL2)
	{
		gPrivateFuncs.SDL_GetWindowPosition = (decltype(gPrivateFuncs.SDL_GetWindowPosition))GetProcAddress(hSDL2, "SDL_GetWindowPosition");
		gPrivateFuncs.SDL_GL_SetAttribute = (decltype(gPrivateFuncs.SDL_GL_SetAttribute))GetProcAddress(hSDL2, "SDL_GL_SetAttribute");
		gPrivateFuncs.SDL_GetWindowSize = (decltype(gPrivateFuncs.SDL_GetWindowSize))GetProcAddress(hSDL2, "SDL_GetWindowSize");
		gPrivateFuncs.SDL_GL_SwapWindow = (decltype(gPrivateFuncs.SDL_GL_SwapWindow))GetProcAddress(hSDL2, "SDL_GL_SwapWindow");
		gPrivateFuncs.SDL_GL_GetProcAddress = (decltype(gPrivateFuncs.SDL_GL_GetProcAddress))GetProcAddress(hSDL2, "SDL_GL_GetProcAddress");
		gPrivateFuncs.SDL_CreateWindow = (decltype(gPrivateFuncs.SDL_CreateWindow))GetProcAddress(hSDL2, "SDL_CreateWindow");
		//Fuck Sniber
		gPrivateFuncs.SDL_GL_ExtensionSupported = (decltype(gPrivateFuncs.SDL_GL_ExtensionSupported))GetProcAddress(hSDL2, "SDL_GL_ExtensionSupported");
	}

	auto engineFactory = g_pMetaHookAPI->GetEngineFactory();

	if (engineFactory("SCEngineClient002", nullptr))
	{
		gPrivateFuncs.SvEngine_glewInit = (decltype(gPrivateFuncs.SvEngine_glewInit))GetProcAddress(g_pMetaHookAPI->GetEngineModule(), "_glewInit@0");
	}

	gPrivateFuncs.triapi_RenderMode = gEngfuncs.pTriAPI->RenderMode;
	gPrivateFuncs.triapi_Begin = gEngfuncs.pTriAPI->Begin;
	gPrivateFuncs.triapi_End = gEngfuncs.pTriAPI->End;
	gPrivateFuncs.triapi_Color4f = gEngfuncs.pTriAPI->Color4f;
	gPrivateFuncs.triapi_Color4ub = gEngfuncs.pTriAPI->Color4ub;
	gPrivateFuncs.triapi_TexCoord2f = gEngfuncs.pTriAPI->TexCoord2f;
	gPrivateFuncs.triapi_Vertex3fv = gEngfuncs.pTriAPI->Vertex3fv;
	gPrivateFuncs.triapi_Vertex3f = gEngfuncs.pTriAPI->Vertex3f;
	gPrivateFuncs.triapi_Brightness = gEngfuncs.pTriAPI->Brightness;
	gPrivateFuncs.triapi_Color4fRendermode = gEngfuncs.pTriAPI->Color4fRendermode;
	gPrivateFuncs.triapi_GetMatrix = gEngfuncs.pTriAPI->GetMatrix;
	gPrivateFuncs.triapi_BoxInPVS = gEngfuncs.pTriAPI->BoxInPVS;
	gPrivateFuncs.triapi_Fog = gEngfuncs.pTriAPI->Fog;
	gPrivateFuncs.triapi_FogParams = gEngfuncs.pTriAPI->FogParams;
	gPrivateFuncs.triapi_SpriteTexture = gEngfuncs.pTriAPI->SpriteTexture;

	EngineSurface_FillAddress(DllInfo, RealDllInfo);

	VideoMode_FillAddress(DllInfo, RealDllInfo);

	Engine_FillAddress_HasOfficialGLTexAllocSupport(RealDllInfo);

	Engine_FillAddress_GL_Init(RealDllInfo);

	Engine_FillAddress_GL_SetMode(RealDllInfo);

	Engine_FillAddress_GL_Shutdown(RealDllInfo);

	Engine_FillAddress_V_FadeAlpha(RealDllInfo);

	Engine_FillAddress_S_ExtraUpdate(RealDllInfo);

	Engine_FillAddress_GL_Bind(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_SelectTexture(RealDllInfo);

	Engine_FillAddress_GL_LoadTexture2(RealDllInfo);

	Engine_FillAddress_R_CullBox(RealDllInfo);

	Engine_FillAddress_R_ForceCVars(RealDllInfo);

	Engine_FillAddress_R_SetupGL(RealDllInfo);

	Engine_FillAddress_R_RenderView(DllInfo, RealDllInfo);

	Engine_FillAddress_V_RenderView(RealDllInfo);

	Engine_FillAddress_R_NewMap(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_LoadFilterTexture(RealDllInfo);

	Engine_FillAddress_GL_BuildLightmaps(RealDllInfo);

	Engine_FillAddress_R_DrawSequentialPoly(DllInfo, RealDllInfo);

	Engine_FillAddress_R_TextureAnimation(RealDllInfo);

	Engine_FillAddress_R_DrawWorld(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawViewModel(DllInfo, RealDllInfo);

	Engine_FillAddress_R_MarkLeaves(RealDllInfo);

	Engine_FillAddress_GL_Set2D(RealDllInfo);

	Engine_FillAddress_GL_Finish2D(RealDllInfo);

	Engine_FillAddress_GL_BeginRendering(RealDllInfo);

	Engine_FillAddress_GL_EndRendering(RealDllInfo);

	Engine_FillAddress_VID_UpdateWindowVars(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_PointInLeaf(RealDllInfo);

	Engine_FillAddress_R_DrawTEntitiesOnList(RealDllInfo);

	Engine_FillAddress_BuildGammaTable(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawParticles(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_DlightVars(RealDllInfo);

	Engine_FillAddress_R_GLStudioDrawPoints(RealDllInfo);

	Engine_FillAddress_R_StudioLighting(DllInfo, RealDllInfo);

	Engine_FillAddress_Host_ClearMemory(RealDllInfo);

	Engine_FillAddress_Cache_Alloc(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_DecalTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_R_GetSpriteFrame(RealDllInfo);

	Engine_FillAddress_GlowBlend(DllInfo, RealDllInfo);

	Engine_FillAddress_SCR_BeginLoadingPlaque(DllInfo, RealDllInfo);

	Engine_FillAddress_Host_IsSinglePlayerGame(RealDllInfo);

	Engine_FillAddress_Mod_UnloadSpriteTextures(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_LoadSpriteModel(RealDllInfo);

	Engine_FillAddress_Mod_LoadSpriteFrame(DllInfo, RealDllInfo);

	Engine_FillAddress_Hunk_AllocName(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_EndRenderingVars(RealDllInfo);

	Engine_FillAddress_VisEdicts(RealDllInfo);

	Engine_FillAddress_R_AllocTransObjectsVars(RealDllInfo);

	Engine_FillAddress_R_RenderFinalFog(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawTEntitiesOnListVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RecursiveWorldNodeVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LoadSkybox(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_FilterMinMaxVars(DllInfo, RealDllInfo);

	Engine_FillAddress_ScrFov(DllInfo, RealDllInfo);

	//Got CL_IsDevOverviewMode, CL_SetDevOverView and refdef here
	Engine_FillAddress_RenderSceneVars(DllInfo, RealDllInfo);

	//Got ClientDLL_DrawNormalTriangles_VA, cl_waterlevel and gDevOverview here
	Engine_FillAddress_RenderSceneVars2(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_IsDevOverviewModeVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DecalInit(DllInfo, RealDllInfo);

	Engine_FillAddress_LightstyleVars(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_SimOrgVars(RealDllInfo);

	Engine_FillAddress_CL_ViewEntityVars(RealDllInfo);

	Engine_FillAddress_CL_ReallocateDynamicData(RealDllInfo);

	Engine_FillAddress_TempEntsVars(RealDllInfo);

	Engine_FillAddress_WaterVars(DllInfo, RealDllInfo);

	Engine_FillAddress_ModKnownVars(RealDllInfo);

	Engine_FillAddress_Mod_LoadStudioModel(RealDllInfo);

	Engine_FillAddress_Mod_LoadBrushModel(RealDllInfo);

	Engine_FillAddress_Mod_LoadModel(RealDllInfo);

	Engine_FillAddress_BasePalette(DllInfo, RealDllInfo);

	Engine_FillAddress_SetFilterMode(RealDllInfo);

	Engine_FillAddress_SetFilterColor(RealDllInfo);

	Engine_FillAddress_SetFilterBrightness(RealDllInfo);

	Engine_FillAddress_MoveVars(RealDllInfo);

	Engine_FillAddress_MissingTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_NoTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_LegacyMultiTextureInit(DllInfo, RealDllInfo);

	Engine_FillAddress_PVSNode(RealDllInfo);

	Engine_FillAddress_DrawStartupGraphic(RealDllInfo);

	Engine_FillAddress_DrawStartupVideo(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_Frame(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_SpriteFrameHoles(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_SpriteFrameAdditive(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_SpriteFrameGeneric(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_FillRGBA(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_FillRGBABlend(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_FillRGBABuf(DllInfo, RealDllInfo);

	Engine_FillAddress_D_FillRect(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_Pic(RealDllInfo);
}

void Engine_InstallHooks(void)
{
	Install_InlineHook(GL_Init);

	if (gPrivateFuncs.GL_SetModeLegacy)
	{
		Install_InlineHook(GL_SetModeLegacy);
		Install_InlineHook(GL_SelectPixelFormat);
	}
	else if (gPrivateFuncs.GL_SetMode_GoldSrc)
	{
		Install_InlineHook(GL_SetMode_GoldSrc);
	}
	else
	{
		Install_InlineHook(GL_SetMode_SvEngine);
	}

	g_pMetaHookAPI->InlinePatchRedirectBranch(gPrivateFuncs.Sys_ShutdownGame_call_GL_Shutdown, GL_Shutdown, NULL);

	Install_InlineHook(GL_Bind);
	Install_InlineHook(GL_Set2D);
	Install_InlineHook(GL_Finish2D);
	Install_InlineHook(GL_BeginRendering);
	Install_InlineHook(GL_EndRendering);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Install_InlineHook(R_RenderView_SvEngine);
		Install_InlineHook(R_LoadSkyBox_SvEngine);
	}
	else
	{
		Install_InlineHook(R_RenderView);
		Install_InlineHook(R_LoadSkys);
	}

	//For Sven
	Install_InlineHook(R_ForceCVars);
	Install_InlineHook(R_NewMap);
	Install_InlineHook(Mod_PointInLeaf);
	Install_InlineHook(R_GLStudioDrawPoints);
	Install_InlineHook(GL_UnloadTextures);
	Install_InlineHook(GL_LoadFilterTexture);
	Install_InlineHook(GL_LoadTexture2);
	Install_InlineHook(GL_BuildLightmaps);

	Install_InlineHook(LegacyMultiTextureInit);

	Install_InlineHook(Mod_LoadStudioModel);
	Install_InlineHook(Mod_LoadSpriteModel);
	Install_InlineHook(Mod_UnloadSpriteTextures);

	gEngfuncs.pTriAPI->RenderMode = triapi_RenderMode;
	gEngfuncs.pTriAPI->Begin = triapi_Begin;
	gEngfuncs.pTriAPI->End = triapi_End;
	gEngfuncs.pTriAPI->Color4f = triapi_Color4f;
	gEngfuncs.pTriAPI->Color4ub = triapi_Color4ub;
	gEngfuncs.pTriAPI->TexCoord2f = triapi_TexCoord2f;
	gEngfuncs.pTriAPI->Vertex3fv = triapi_Vertex3fv;
	gEngfuncs.pTriAPI->Vertex3f = triapi_Vertex3f;
	gEngfuncs.pTriAPI->Brightness = triapi_Brightness;
	gEngfuncs.pTriAPI->Color4fRendermode = triapi_Color4fRendermode;

	gEngfuncs.pTriAPI->GetMatrix = triapi_GetMatrix;
	gEngfuncs.pTriAPI->BoxInPVS = triapi_BoxInPVS;
	gEngfuncs.pTriAPI->Fog = triapi_Fog;
	gEngfuncs.pTriAPI->FogParams = triapi_FogParams;
	gEngfuncs.pTriAPI->SpriteTexture = triapi_SpriteTexture;

	Install_InlineHook(BuildGammaTable);
	Install_InlineHook(R_CullBox);
	Install_InlineHook(PVSNode);
	Install_InlineHook(Host_ClearMemory);
	Install_InlineHook(CVideoMode_Common_DrawStartupGraphic);
	Install_InlineHook(CGame_DrawStartupVideo);
	Install_InlineHook(Draw_Frame);
	Install_InlineHook(Draw_SpriteFrameHoles);
	Install_InlineHook(Draw_SpriteFrameHoles_SvEngine);
	Install_InlineHook(Draw_SpriteFrameAdditive);
	Install_InlineHook(Draw_SpriteFrameAdditive_SvEngine);
	Install_InlineHook(Draw_SpriteFrameGeneric);
	Install_InlineHook(Draw_SpriteFrameGeneric_SvEngine);
	Install_InlineHook(Draw_FillRGBA);
	Install_InlineHook(Draw_FillRGBABlend);
	Install_InlineHook(Draw_FillRGBABuf);
	Install_InlineHook(Draw_Pic);
	Install_InlineHook(D_FillRect);
	Install_InlineHook(R_GetSpriteFrame);
}

void Engine_UninstallHooks(void)
{
	//Engine
	Uninstall_Hook(GL_Init);
	if (gPrivateFuncs.GL_SetModeLegacy)
	{
		Uninstall_Hook(GL_SetModeLegacy);
		Uninstall_Hook(GL_SelectPixelFormat);
	}
	else if (gPrivateFuncs.GL_SetMode_GoldSrc)
	{
		Uninstall_Hook(GL_SetMode_GoldSrc);
	}
	else
	{
		Uninstall_Hook(GL_SetMode_SvEngine);
	}
	Uninstall_Hook(GL_Bind);
	Uninstall_Hook(GL_Set2D);
	Uninstall_Hook(GL_Finish2D);
	Uninstall_Hook(GL_BeginRendering);
	Uninstall_Hook(GL_EndRendering);

	if (gPrivateFuncs.R_RenderView_SvEngine)
	{
		Uninstall_Hook(R_RenderView_SvEngine);
		Uninstall_Hook(R_LoadSkyBox_SvEngine);
	}
	else
	{
		Uninstall_Hook(R_RenderView);
		Uninstall_Hook(R_LoadSkys);
	}

	Uninstall_Hook(R_ForceCVars);
	Uninstall_Hook(R_NewMap);
	Uninstall_Hook(Mod_PointInLeaf);
	Uninstall_Hook(R_GLStudioDrawPoints);
	Uninstall_Hook(GL_UnloadTextures);
	Uninstall_Hook(GL_LoadFilterTexture);
	Uninstall_Hook(GL_LoadTexture2);
	Uninstall_Hook(GL_BuildLightmaps);
	Uninstall_Hook(LegacyMultiTextureInit);

	Uninstall_Hook(Mod_LoadStudioModel);
	Uninstall_Hook(Mod_LoadSpriteModel);
	Uninstall_Hook(Mod_UnloadSpriteTextures);
	Uninstall_Hook(BuildGammaTable);
	Uninstall_Hook(R_CullBox);
	Uninstall_Hook(PVSNode);
	Uninstall_Hook(CVideoMode_Common_DrawStartupGraphic);
	Uninstall_Hook(CGame_DrawStartupVideo);
	Uninstall_Hook(Draw_Frame);
	Uninstall_Hook(Draw_SpriteFrameHoles);
	Uninstall_Hook(Draw_SpriteFrameHoles_SvEngine);
	Uninstall_Hook(Draw_SpriteFrameAdditive);
	Uninstall_Hook(Draw_SpriteFrameAdditive_SvEngine);
	Uninstall_Hook(Draw_SpriteFrameGeneric);
	Uninstall_Hook(Draw_SpriteFrameGeneric_SvEngine);
	Uninstall_Hook(Draw_FillRGBA);
	Uninstall_Hook(Draw_FillRGBABlend);
	Uninstall_Hook(Draw_FillRGBABuf);
	Uninstall_Hook(Draw_Pic);
	Uninstall_Hook(D_FillRect);
	Uninstall_Hook(R_GetSpriteFrame);
}

int WINAPI GL_RedirectedGenTexture(void)
{
	return GL_GenTexture();
}

/*
	Purpose: Redirect legacy texture allocation sites to GL_RedirectedGenTexture.
*/
void R_RedirectEngineLegacyOpenGLTextureAllocation(const mh_dll_info_t& RealDllInfo)
{
	if (g_bHasOfficialGLTexAllocSupport)
		return;

	constexpr size_t kCallInstructionLength = 5;
	constexpr size_t kMaxX86InstructionLength = 15;
	constexpr BYTE kCallOpcode = 0xE8;
	constexpr BYTE kNopOpcode = 0x90;

	static constexpr const char* patchNames[] = {
		"texture_extension_number_mov_site_GL_BuildLightmaps",
		"texture_extension_number_mov_site_GL_LoadFilterTexture",
		"texture_extension_number_mov_site_GL_LoadTexture2",
		"texture_extension_number_mov_site_LoadTransPic_bind",
		"texture_extension_number_mov_site_LoadTransPic_increment",
		"texture_extension_number_mov_site_R_InitParticleTexture",
		"texture_extension_number_mov_site_R_Init_playertextures",
	};

	for (const char* patchName : patchNames)
	{
		const auto status = g_pMetaHookAPI->IsGameSymbolAvailable(RealDllInfo.ImageBase, patchName);
		if (status == MH_GAMESYMBOL_SYMBOL_NOT_FOUND)
			continue;
		if (status != MH_GAMESYMBOL_OK)
		{
			Sys_Error("Could not query gamedata symbol: %s (%s)\nEngine buildnum: %d",
				patchName, g_pMetaHookAPI->GetGameSymbolStatusString(status), g_dwEngineBuildnum);
		}

		auto patchSite = (PUCHAR)GamedataResolvePtr(RealDllInfo.ImageBase, patchName, MH_GAMESYMBOL_KIND_PATCH);
		size_t patchLength = 0;
		const int decodedLength = g_pMetaHookAPI->DisasmSingleInstruction(patchSite,
			[](void* inst, PUCHAR, size_t instLen, PVOID context) {
				auto pinst = (cs_insn*)inst;
				const auto& x86 = pinst->detail->x86;
				if (pinst->id == X86_INS_MOV &&
					x86.op_count == 2 &&
					x86.operands[0].type == X86_OP_REG &&
					x86.operands[0].reg == X86_REG_EAX &&
					x86.operands[1].type == X86_OP_MEM &&
					instLen >= kCallInstructionLength)
				{
					*(size_t*)context = instLen;
				}
			}, &patchLength);
		if (!patchLength)
		{
			Sys_Error("Invalid texture allocation patch %s at %p: expected MOV EAX, [mem] of at least 5 bytes (decoded length %d)",
				patchName, patchSite, decodedLength);
			continue;
		}

		BYTE redirectCode[kMaxX86InstructionLength];
		std::memset(redirectCode, kNopOpcode, patchLength);
		redirectCode[0] = kCallOpcode;
		*(int*)(redirectCode + 1) = (PUCHAR)GL_RedirectedGenTexture - (patchSite + kCallInstructionLength);
		g_pMetaHookAPI->WriteMemory(patchSite, redirectCode, (DWORD)patchLength);
	}
}

void R_RedirectEngineLegacyOpenGLCallAPI(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glGetString", CoreProfile_glGetString, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glAlphaFunc", CoreProfile_glAlphaFunc, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glEnable", CoreProfile_glEnable, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glDisable", CoreProfile_glDisable, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glIsEnabled", CoreProfile_glIsEnabled, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glShadeModel", CoreProfile_glShadeModel, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glTexEnvf", CoreProfile_glTexEnvf, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glTexParameterf", CoreProfile_glTexParameterf, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glBegin", CoreProfile_glBegin, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glColor4f", CoreProfile_glColor4f, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glColor4ub", CoreProfile_glColor4ub, NULL);

		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_ExtensionSupported", CoreProfile_SDL_GL_ExtensionSupported, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_SetAttribute", CoreProfile_GL_SetAttribute, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_CreateWindow", CoreProfile_SDL_CreateWindow, NULL);
	}
	else if (gPrivateFuncs.SDL_GL_GetProcAddress)
	{
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_GetProcAddress", CoreProfile_SDL_GL_GetProcAddress, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_SetAttribute", CoreProfile_GL_SetAttribute, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_CreateWindow", CoreProfile_SDL_CreateWindow, NULL);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		g_pMetaHookAPI->BlobIATHook(g_pMetaHookAPI->GetBlobEngineModule(), "kernel32.dll", "GetProcAddress", CoreProfile_GetProcAddress, NULL);
	}
	else
	{
		//non-SDL
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "kernel32.dll", "GetProcAddress", CoreProfile_GetProcAddress, NULL);

	}

	if (gPrivateFuncs.GL_SetMode_call_qwglCreateContext)
	{
		g_pMetaHookAPI->InlinePatchRedirectBranch(gPrivateFuncs.GL_SetMode_call_qwglCreateContext, CoreProfile_qwglCreateContext, NULL);
	}
}

void R_RedirectEngineLegacyOpenGLCall(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	R_RedirectEngineLegacyOpenGLTextureAllocation(RealDllInfo);
	R_RedirectEngineLegacyOpenGLCallAPI(DllInfo, RealDllInfo);
}

void R_RedirectSCClientLegacyOpenGLCall_glTexEnvf(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x68\x01\x85\x00\x00\x68\x00\x85\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 10, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glTexEnvf, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glBegin(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x6A\x06\xFF\x15\x2A\x2A\x2A\x2A\xF6\x87\x88\x00\x00\x00\x80";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 2, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, triapi_glBegin, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glEnd(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xFF\x15\x2A\x2A\x2A\x2A\xF7\x87\x88\x00\x00\x00\x00\x01\x00\x00";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 0, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, triapi_glEnd, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawParticle(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xF3\x0F\x11\x0C\x24\xFF\x15\x2A\x2A\x2A\x2A\x8D\x45\x98";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 5, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glColor4f, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawPortal(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xC7\x44\x24\x08\x00\x00\x80\x3F\xC7\x44\x24\x04\x00\x00\x80\x3F\xC7\x04\x24\x00\x00\x80\x3F\xFF\x15\x2A\x2A\x2A\x2A\x68";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound +
				Sig_Length("\xC7\x44\x24\x08\x00\x00\x80\x3F\xC7\x44\x24\x04\x00\x00\x80\x3F\xC7\x04\x24\x00\x00\x80\x3F")
				, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glColor4f, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glEnable_GenerateInvisibleTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x75\x00\x68\xE1\x0D\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 5, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glEnable, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glEnable_GeneratePortalTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x6A\x01\xFF\x15\x2A\x2A\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x36\x68";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 13, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glEnable, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glDisable_FOG(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	// glDisable(GL_FOG);
	const char pattern[] = "\x68\x60\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xA1";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 5, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glDisable, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

static void* g_glDisable_ClipPlane = nullptr;

void R_RedirectSCClientLegacyOpenGLCall_glDisable_ClipPlane(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	g_glDisable_ClipPlane = CoreProfile_glDisable;

	// glDisable(GL_CLIPPLANE0);
	const char pattern[] = "\x8B\x35\x2A\x2A\x2A\x2A\x33\xD2\xC6\x05";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PUCHAR pRealCall = (PUCHAR)ConvertDllInfoSpace(pFound, DllInfo, RealDllInfo);

			g_pMetaHookAPI->WriteDWORD(pRealCall + 2, (ULONG_PTR)&g_glDisable_ClipPlane);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glCopyTexSubImage2D_RenderPortals(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	// glDisable(GL_FOG);
	const char pattern[] = "\x50\x6A\x00\x6A\x00\x6A\x00\x6A\x00\x68\xE1\x0D\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + Sig_Length(pattern) - 2, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glCopyTexSubImage2D_RenderPortals, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glClear_ClipPlane(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xC7\x44\x24\x0C\x00\x00\x00\x00\xC7\x44\x24\x08\x00\x00\x80\x3F\xC7\x44\x24\x04\x00\x00\x00\x00\xC7\x04\x24\x00\x00\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x40\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + Sig_Length(pattern) - 2, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glClear_RenderPortals, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientRenderPortalAngleVectors(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xC7\x82\xE8\x00\x00\x00\x01\x00\x00\x00\x50\x8D\x42\x18\x50\x51\xE8";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + Sig_Length(pattern) - 1, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, ClientPortalManager_AngleVectors, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectClientLegacyOpenGLCall(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsSvenCoop)
	{
		//Sniber NMSL
		R_RedirectSCClientLegacyOpenGLCall_glTexEnvf(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glBegin(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glEnd(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawParticle(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawPortal(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glEnable_GenerateInvisibleTexture(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glEnable_GeneratePortalTexture(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glDisable_FOG(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glDisable_ClipPlane(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glCopyTexSubImage2D_RenderPortals(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glClear_ClipPlane(DllInfo, RealDllInfo);
		R_RedirectSCClientRenderPortalAngleVectors(DllInfo, RealDllInfo);
	}
}

void R_PatchResetLatched(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		return;

	//CL_LinkPacketEntities calls R_ResetLatched at two consecutive call sites; the
	//interpolation fix must cover both, so every numbered call-site record is
	//redirected and the resolved function itself feeds R_ResetLatched_Patched.
	char symbolName[96];

	for (int index = 0; ; ++index)
	{
		snprintf(symbolName, sizeof(symbolName), "CL_LinkPacketEntities_to_R_ResetLatched_callsite_%d", index);

		PVOID callsite = GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, symbolName, MH_GAMESYMBOL_KIND_PATCH);

		if (!callsite)
			break;

		g_pMetaHookAPI->InlinePatchRedirectBranch(callsite, R_ResetLatched_Patched, NULL);
	}

	gPrivateFuncs.R_ResetLatched = (decltype(gPrivateFuncs.R_ResetLatched))GamedataResolvePtr(RealDllInfo.ImageBase, "R_ResetLatched", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_ClientPortalManager_ResetAll(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.ClientPortalManager_ResetAll)
		return;

	gPrivateFuncs.ClientPortalManager_ResetAll = (decltype(gPrivateFuncs.ClientPortalManager_ResetAll))GamedataResolvePtr(RealDllInfo.ImageBase, "ClientPortalManager_ResetAll", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Client_FillAddress_ClientPortalManager_GetOriginalSurfaceTexture_DrawPortalSurface(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture = (decltype(gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture))
		GamedataResolvePtr(RealDllInfo.ImageBase, "ClientPortalManager_GetOriginalSurfaceTexture", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.ClientPortalManager_DrawPortalSurface = (decltype(gPrivateFuncs.ClientPortalManager_DrawPortalSurface))
		GamedataResolvePtr(RealDllInfo.ImageBase, "ClientPortalManager_DrawPortalSurface", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_ClientPortalManager_EnableClipPlane(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.ClientPortalManager_EnableClipPlane)
		return;

	gPrivateFuncs.ClientPortalManager_EnableClipPlane = (decltype(gPrivateFuncs.ClientPortalManager_EnableClipPlane))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "ClientPortalManager_EnableClipPlane", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Client_FillAddress_ClientPortalManager_RenderPoratals(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.ClientPortalManager_RenderPortals)
		return;

	gPrivateFuncs.ClientPortalManager_RenderPortals = (decltype(gPrivateFuncs.ClientPortalManager_RenderPortals))GamedataResolvePtr(RealDllInfo.ImageBase, "ClientPortalManager_RenderPortals", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Client_FillAddress_UpdatePlayerPitch(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.UpdatePlayerPitch)
		return;

	gPrivateFuncs.UpdatePlayerPitch = (decltype(gPrivateFuncs.UpdatePlayerPitch))GamedataResolvePtr(RealDllInfo.ImageBase, "UpdatePlayerPitch", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_FogParams(const mh_dll_info_t& RealDllInfo)
{
	g_iFogColor_SCClient = (decltype(g_iFogColor_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iFogColor", MH_GAMESYMBOL_KIND_GLOBAL);
	g_iStartDist_SCClient = (decltype(g_iStartDist_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iStartDist", MH_GAMESYMBOL_KIND_GLOBAL);
	g_iEndDist_SCClient = (decltype(g_iEndDist_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_iEndDist", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Client_FillAddress_SCClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory)
	{
		auto SCClient001 = pfnClientFactory("SCClientDLL001", 0);

		if (SCClient001)
		{
			gPrivateFuncs.SCClientDLL_glewInit = (decltype(gPrivateFuncs.SCClientDLL_glewInit))GetProcAddress(g_pMetaHookAPI->GetClientModule(), "_glewInit@0");

			Client_FillAddress_ClientPortalManager_ResetAll(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_GetOriginalSurfaceTexture_DrawPortalSurface(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_EnableClipPlane(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_RenderPoratals(DllInfo, RealDllInfo);
			Client_FillAddress_UpdatePlayerPitch(DllInfo, RealDllInfo);
			Client_FillAddress_FogParams(RealDllInfo);

			g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_bRenderingPortals_SCClient", MH_GAMESYMBOL_KIND_GLOBAL);
			//Only svencoop-10257 publishes this slot; 8948 keeps it null and the
			//studio view-entity save/restore is skipped there.
			g_ViewEntityIndex_SCClient = (decltype(g_ViewEntityIndex_SCClient))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "g_ViewEntityIndex_SCClient", MH_GAMESYMBOL_KIND_GLOBAL);

			g_bIsSvenCoop = true;
		}
	}
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Client_FillAddress_SCClient(DllInfo, RealDllInfo);

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
			g_PlayerExtraInfo_CZDS = (decltype(g_PlayerExtraInfo_CZDS))GamedataResolvePtr(RealDllInfo.ImageBase, "g_PlayerExtraInfo_CZDS", MH_GAMESYMBOL_KIND_GLOBAL);
		else
			g_PlayerExtraInfo = (decltype(g_PlayerExtraInfo))GamedataResolvePtr(RealDllInfo.ImageBase, "g_PlayerExtraInfo", MH_GAMESYMBOL_KIND_GLOBAL);
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "aomdc"))
	{
		g_bIsAoMDC = true;
	}

}

void Client_InstallHooks()
{
	//Install_InlineHook(ClientPortalManager_ResetAll);
	Install_InlineHook(ClientPortalManager_DrawPortalSurface);
	Install_InlineHook(ClientPortalManager_EnableClipPlane);
	Install_InlineHook(ClientPortalManager_RenderPortals);
	Install_InlineHook(UpdatePlayerPitch);

	//Fuck Sniber
	if (gPrivateFuncs.SCClientDLL_glewInit)
		gPrivateFuncs.SCClientDLL_glewInit();
}

void Client_UninstallHooks()
{
	//Uninstall_Hook(ClientPortalManager_ResetAll);
	Uninstall_Hook(ClientPortalManager_DrawPortalSurface);
	Uninstall_Hook(ClientPortalManager_EnableClipPlane);
	Uninstall_Hook(ClientPortalManager_RenderPortals);
	Uninstall_Hook(UpdatePlayerPitch);
}

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
	if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
	{
		auto addr_VA = (ULONG_PTR)addr;
		auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

		return (PVOID)VA_from_RVA(addr, TargetDllInfo);
	}

	return nullptr;
}

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo)
{
	if ((ULONG_PTR)vftable > (ULONG_PTR)RealDllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)RealDllInfo.ImageBase + RealDllInfo.ImageSize)
	{
		ULONG_PTR vftable_VA = (ULONG_PTR)vftable;
		ULONG vftable_RVA = RVA_from_VA(vftable, RealDllInfo);
		auto vftable_DllInfo = (decltype(vftable))VA_from_RVA(vftable, DllInfo);

		auto vf_VA = (ULONG_PTR)vftable_DllInfo[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}
	else if ((ULONG_PTR)vftable > (ULONG_PTR)DllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)DllInfo.ImageBase + DllInfo.ImageSize)
	{
		auto vf_VA = (ULONG_PTR)vftable[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}

	return vftable[index];
}
