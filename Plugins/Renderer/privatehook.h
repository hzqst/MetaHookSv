#pragma once

#include "qgl.h"

#include <studio.h>

#include "enginedef.h"

typedef void(*ExtraShaderStageCallback)(GLuint* objs, int* used);

typedef struct
{
	void (*BuildGammaTable)(float gamma);
	void (*R_ForceCVars)(qboolean mp);
	void (*R_CheckVariables)(void);
	void (*R_AnimateLight)(void);
	void (*V_RenderView)(void);
	void (*R_RenderView)(void);
	void (*R_RenderView_SvEngine)(int viewIdx);
	void (*R_RenderScene)(void);
	void (*R_RenderFinalFog)(void);
	void (*ClientDLL_DrawNormalTriangles)(void);
	void (*R_NewMap)(void);
	void (*R_ClearParticles)(void);
	void (*R_DecalInit)(void);
	void (*V_InitLevel)(void);
	void (*GL_BuildLightmaps)(void);
	void (*R_DrawParticles)(void);
	void (*R_TracerDraw)(void);
	void (*R_BeamDrawList)(void);
	void (*R_FreeDeadParticles)(particle_t**);
	void (*R_DrawTEntitiesOnList)(int onlyClientDraw);
	void (*ClientDLL_DrawTransparentTriangles)(void);
	ULONG_PTR pfnDrawTransparentTriangles;
	void (*R_AddTEntity)(cl_entity_t* pEnt);
	void (*R_DrawWorld)(void);
	void (*R_SetupFrame)(void);
	void (*R_SetupGL)(void);
	qboolean(*R_CullBox)(vec3_t mins, vec3_t maxs);
	void (*GL_Bind)(int texnum);
	void (*GL_SelectTexture)(GLenum target);
	void (*GL_DisableMultitexture)(void);
	void (*GL_EnableMultitexture)(void);
	void (*GL_Init)(void);
	void (*GL_Set2D)(void);
	void (*GL_Finish2D)(void);
	void (*GL_BeginRendering)(int* x, int* y, int* width, int* height);
	void (*GL_EndRendering)(void);
	void (*EmitWaterPolys)(msurface_t* fa, int direction);
	void (*R_DrawSequentialPoly)(msurface_t* s, int face);
	void (*R_RecursiveWorldNode)(mnode_t* node);
	texture_t* (*R_TextureAnimation)(msurface_t* fa);
	void (*R_RenderDynamicLightmaps)(msurface_t* fa);
	void(*R_RotateForEntity)(float* origin, cl_entity_t* ent);
	void (*R_DrawDecals)(qboolean bMultitexture);
	void (*Draw_MiptexTexture)(cachewad_t* wad, byte* data);
	void (*GL_UnloadTexture)(const char* identifier);
	void (*GL_UnloadTextures)(void);
	texture_t* (*Draw_DecalTexture)(int index);
	void* (*Draw_CustomCacheGet)(cachewad_t* wad, void* raw, int rawsize, int index);
	void* (*Draw_CacheGet)(cachewad_t* wad, int index);
	void (*R_BuildLightMap)(msurface_t* psurf, byte* dest, int stride);
	void(*R_AddDynamicLights)(msurface_t* psurf);
	//int(*GL_LoadTexture)(char *identifier, int textureType, int width, int height, byte *data, qboolean mipmap, int iPalTextureType, byte *pPal);
	int(*GL_LoadTexture2)(char* identifier, int textureType, int width, int height, byte* data, qboolean mipmap, int iPalTextureType, byte* pPal, int filter);
	int(*GL_Upload16)(byte* data, int width, int height, int iType, byte* pPal, int a6, int a7, int a8);
	void (*Mod_UnloadSpriteTextures)(model_t* mod);
	void (*Mod_LoadSpriteModel)(model_t* mod, void* buffer);
	void* (*Mod_LoadSpriteFrame)(void* pin, mspriteframe_t** ppframe, int framenum);
	void (*R_DecalMPoly)(float* v, texture_t* ptexture, msurface_t* psurf, int vertCount);
	void (*R_MarkLeaves)(void);
	void (*R_DrawBrushModel)(cl_entity_t* e);
	void (*R_DrawSpriteModel)(cl_entity_t* ent);
	int (*CL_FxBlend)(cl_entity_t* ent);
	float(*R_GlowBlend)(cl_entity_t* ent);
	void (*VID_UpdateWindowVars)(RECT* prc, int x, int y);
	mleaf_t* (*Mod_PointInLeaf)(vec3_t p, model_t* model);
	void* (*realloc_SvEngine)(void*, size_t);
	dlight_t* (*CL_AllocDlight)(int key);
	dlight_t* (*CL_AllocElight)(int key);
	void(*S_ExtraUpdate)(void);
	void(*R_DrawViewModel)(void);//inlined in SvEngine
	void(*R_PolyBlend)(void);
	void(*R_DecalShootInternal)(texture_t* ptexture, int index, int entity, int modelIndex, vec3_t position, int flags, float flScale);
	void(*R_ResetLatched)(cl_entity_t* ent, qboolean full_reset);
	void(*DT_Initialize)(void);
	mnode_t* (*PVSNode)(mnode_t* node, vec3_t emins, vec3_t emaxs);
	void(*R_LoadSkys)(void);
	void(*R_LoadSkyboxInt_SvEngine)(const char* name);
	void(*R_LoadSkyBox_SvEngine)(const char* name);
	int(*CL_IsDevOverviewMode)(void);
	void(*CL_SetDevOverView)(void* a1);
	void(*Mod_LoadStudioModel)(model_t* mod, void* buffer);
	void(*Mod_LoadBrushModel)(model_t* mod, void* buffer);
	model_t* (*Mod_LoadModel)(model_t* mod, qboolean crash, qboolean trackCRC);
	void(*triapi_RenderMode)(int mode);
	void(*triapi_Begin)(int primitiveCode);
	void(*triapi_End)();
	void(*triapi_Color4f)(float r, float g, float b, float a);
	void(*triapi_Color4ub)(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	void(*triapi_TexCoord2f)(float s, float t);
	void(*triapi_Vertex3fv)(float* v);
	void(*triapi_Vertex3f)(float x, float y, float z);
	void(*triapi_Brightness)(float brightness);
	void(*triapi_Color4fRendermode)(float r, float g, float b, float a, int rendermode);
	void(*triapi_GetMatrix) (const int pname, float* matrix);
	int (*triapi_BoxInPVS)(float* mins, float* maxs);
	void (*triapi_Fog)(float* flFogColor, float flStart, float flEnd, qboolean bOn);
	void (*triapi_FogParams)(float flDensity, qboolean bFogAffectsSkybox);
	void (*Draw_Frame)(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect);
	void (*Draw_SpriteFrameHoles)(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect);
	void (*Draw_SpriteFrameHoles_SvEngine)(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect);
	void (*Draw_SpriteFrameAdditive)(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect);
	void (*Draw_SpriteFrameAdditive_SvEngine)(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect);
	void (*Draw_SpriteFrameGeneric)(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect, int src, int dest, int width, int height);
	void (*Draw_SpriteFrameGeneric_SvEngine)(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect, int src, int dest, int width, int height);
	void (*Draw_FillRGBA)(int x, int y, int w, int h, int r, int g, int b, int a);
	void (*Draw_FillRGBABlend)(int x, int y, int w, int h, int r, int g, int b, int a);
	void (*NET_DrawRect)(int x, int y, int w, int h, int r, int g, int b, int a);
	void (*Draw_Pic)(int x, int y, qpic_t* pic);
	enginesurface_Texture* (*staticGetTextureById)(int id);
	void (__fastcall* enginesurface_pushMakeCurrent)(void* pthis, int, int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace);
	void (__fastcall* enginesurface_popMakeCurrent)(void* pthis, int);
	void(__fastcall* enginesurface_drawFilledRect)(void* pthis, int, int x0, int y0, int x1, int y1);
	void(__fastcall* enginesurface_drawOutlinedRect)(void* pthis, int, int x0, int y0, int x1, int y1);
	void(__fastcall* enginesurface_drawLine)(void* pthis, int, int x0, int y0, int x1, int y1);
	void(__fastcall* enginesurface_drawPolyLine)(void* pthis, int, int* px, int* py, int numPoints);
	void(__fastcall* enginesurface_drawSetTextureRGBA)(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, qboolean hasAlphaChannel);
	void(__fastcall* enginesurface_drawSetTexture)(void* pthis, int, int textureId);
	void(__fastcall* enginesurface_drawTexturedRect)(void* pthis, int, int x0, int y0, int x1, int y1);
	int(__fastcall* enginesurface_createNewTextureID)(void* pthis, int);
	void(__fastcall* enginesurface_drawPrintCharAdd)(void* pthis, int, int x, int y, int wide, int tall, float s0, float t0, float s1, float t1);
	void(__fastcall* enginesurface_drawSetTextureFile)(void* pthis, int, int textureId, const char* filename, qboolean hardwareFilter, bool forceReload);
	void(__fastcall* enginesurface_drawGetTextureSize)(void* pthis, int, int textureId, int& wide, int& tall);
	bool(__fastcall* enginesurface_isTextureIDValid)(void* pthis, int, int);
	void(__fastcall* enginesurface_drawFlushText)(void* pthis, int);
	bool(__fastcall* BaseUISurface_DeleteTextureByID)(void* pthis, int, int textureId);
	void( __fastcall *enginesurface_drawSetTextureBGRA)(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, bool forceUpload);
	void(*SCR_BeginLoadingPlaque)(qboolean reconnect);
	qboolean(*Host_IsSinglePlayerGame)(void);
	void* (*Hunk_AllocName)(int size, const char* name);
	void* (*Cache_Alloc)(cache_user_t* c, int size, const char* name);
	void (*Host_ClearMemory)(qboolean bQuite);
	void(__fastcall* CVideoMode_Common_DrawStartupGraphic)(void* videomode, int dummy, void* window);
	int CVideoMode_Common_m_ImageID_Size_offset;
	int CVideoMode_Common_m_ImageID_offset;
	int CVideoMode_Common_m_iBaseResX_offset;
	int CVideoMode_Common_m_iBaseResY_offset;

	//Sven Co-op Client DLL
	void(__fastcall* ClientPortalManager_ResetAll)(void* pthis, int dummy);
	mtexinfo_t* (__fastcall* ClientPortalManager_GetOriginalSurfaceTexture)(void* pthis, int dummy, msurface_t* surf);
	void(__fastcall* ClientPortalManager_DrawPortalSurface)(void* pthis, int dummy, void* ClientPortal, msurface_t* surf, GLuint texture);
	void(__fastcall* ClientPortalManager_EnableClipPlane)(void* pthis, int dummy, int index, vec3_t a1, vec3_t a2, vec3_t a3);
	void(__cdecl* UpdatePlayerPitch)(cl_entity_t* a1, float a2);

	//Engine Studio
	void (*R_GLStudioDrawPoints)(void);
	void (*R_LightStrength)(int bone, float* vert, float (*light)[4]);
	void (*R_StudioLighting)(float* lv, int bone, int flags, vec3_t normal);
	void (*R_StudioSetupSkin)(studiohdr_t* ptexturehdr, int index);
	skin_t* (*R_StudioGetSkin)(int keynum, int index);
	void (*R_LightLambert)(float (*light)[4], float* normal, float* src, float* lambert);

	void (*BuildGlowShellVerts)(vec3_t* pstudioverts, auxvert_t* pauxverts);
	void (*R_StudioChrome)(int* pchrome, int bone, vec3_t normal);

	//Engine Studio Exported API
	void (*studioapi_StudioDynamicLight)(struct cl_entity_s* ent, struct alight_s* plight);
	qboolean(*studioapi_StudioCheckBBox)(void);
	void(*studioapi_GL_SetRenderMode)(int rendermode);
	void(*studioapi_SetupRenderer)(int rendermode);
	void(*studioapi_RestoreRenderer)(void);

	//Client Studio
	void(__fastcall* GameStudioRenderer_StudioSetupBones)(void* pthis, int);
	void(__fastcall* GameStudioRenderer_StudioMergeBones)(void* pthis, int, model_t* pSubModel);
	void(__fastcall* GameStudioRenderer_StudioSaveBones)(void* pthis, int);
	int(__fastcall* GameStudioRenderer_StudioDrawModel)(void* pthis, int, int flags);
	int(__fastcall* GameStudioRenderer_StudioDrawPlayer)(void* pthis, int, int flags, struct entity_state_s* pplayer);
	int(__fastcall* GameStudioRenderer__StudioDrawPlayer)(void* pthis, int, int flags, struct entity_state_s* pplayer);
	void(__fastcall* GameStudioRenderer_StudioRenderModel)(void* pthis, int);
	void(__fastcall* GameStudioRenderer_StudioRenderFinal)(void* pthis, int);

	int GameStudioRenderer_StudioCalcAttachments_vftable_index;
	int GameStudioRenderer_StudioSetupBones_vftable_index;
	int GameStudioRenderer_StudioSaveBones_vftable_index;
	int GameStudioRenderer_StudioMergeBones_vftable_index;
	int GameStudioRenderer_StudioDrawModel_vftable_index;
	int GameStudioRenderer_StudioDrawPlayer_vftable_index;
	int GameStudioRenderer__StudioDrawPlayer_vftable_index;
	int GameStudioRenderer_StudioRenderModel_vftable_index;
	int GameStudioRenderer_StudioRenderFinal_vftable_index;

	//Client DLL
	int (*CL_IsThirdPerson)(void);

	//Engine Studio
	int (*R_StudioDrawModel)(int flags);
	int (*R_StudioDrawPlayer)(int flags, struct entity_state_s* pplayer);
	void(*R_StudioRenderModel)(void);
	void(*R_StudioRenderFinal)(void);
	void(*R_StudioSetupBones)(void);
	void(*R_StudioMergeBones)(model_t* pSubModel);
	void(*R_StudioSaveBones)(void);

	//SDL2
	void (__cdecl*SDL_GetWindowPosition)(void* window, int* x, int* y);
	int(__cdecl* SDL_GL_SetAttribute)(int attr, int value);
	void (__cdecl*SDL_GetWindowSize)(void* window, int* w, int* h);
	void (__cdecl* SDL_GL_SwapWindow)(void* window);

	bool R_ForceCVars_inlined;
	bool R_SetupFrame_inlined;
	bool R_RenderScene_inlined;
	bool R_LightStrength_inlined;
	bool R_GlowBlend_inlined;

	//Just for debugging
	//void(__stdcall* glDeleteTextures)(GLsizei n, const GLuint* textures);
}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

extern extra_player_info_t(*g_PlayerExtraInfo)[65];
extern extra_player_info_czds_t(*g_PlayerExtraInfo_CZDS)[65];

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_InstallHooks();
void Engine_UninstallHooks();
void ClientStudio_UninstallHooks();
void EngineStudio_UninstallHooks();
void R_RedirectEngineLegacyOpenGLTextureAllocation(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void R_RedirectEngineLegacyOpenGLCall(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void R_RedirectClientLegacyOpenGLCall(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void R_PatchResetLatched(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Client_InstallHooks();
void Client_UninstallHooks();
void DllLoadNotification(mh_load_dll_notification_context_t* ctx);

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo);
PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);