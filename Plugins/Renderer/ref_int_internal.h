#pragma once

#include <gl/gl.h>

#include "enginedef.h"

typedef struct FBO_Container_s
{
	GLuint s_hBackBufferFBO;
	GLuint s_hBackBufferCB;
	GLuint s_hBackBufferDB;
	GLuint s_hBackBufferTex;
	GLuint s_hBackBufferTex2;
	GLuint s_hBackBufferDepthTex;
	GLuint s_hBackBufferStencilView;
	int iWidth;
	int iHeight;
	int iTextureColorFormat;
	int iTextureDepthFormat;
}FBO_Container_t;

typedef void(*ExtraShaderStageCallback)(GLuint *objs, int *used);

typedef struct
{
	//void (*Cvar_DirectSet)(cvar_t *var, char *value);
	void (*BuildGammaTable)(float gamma);
	void (*R_ForceCVars)(qboolean mp);
	void (*R_CheckVariables)(void);
	void (*R_AnimateLight)(void);
	void (*V_RenderView)(void);
	void (*R_RenderView)(void);
	void (*R_RenderView_SvEngine)(int a1);
	void (*R_RenderScene)(void);
	void (*R_NewMap)(void);
	void (*R_DrawParticles)(void);
	void (*R_TracerDraw)(void);
	void (*R_BeamDrawList)(void);
	void (*R_FreeDeadParticles)(particle_t **);
	void (*R_DrawTEntitiesOnList)(int onlyClientDraw);
	void (*R_AddTEntity)(cl_entity_t *pEnt);
	void (*R_DrawWorld)(void);
	void (*R_SetupFrame)(void);
	void (*R_SetupGL)(void);
	qboolean (*R_CullBox)(vec3_t mins, vec3_t maxs);
	void (*GL_Bind)(int texnum);
	void (*GL_SelectTexture)(GLenum target);
	void (*GL_DisableMultitexture)(void);
	void (*GL_EnableMultitexture)(void);
	void (*GL_BeginRendering)(int *x, int *y, int *width, int *height);
	void (*GL_EndRendering)(void);
	void (*GL_BuildLightmaps)(void);
	void (*EmitWaterPolys)(msurface_t *fa, int direction);
	void (*R_DrawSequentialPoly)(msurface_t *s, int face);
	void (*R_RecursiveWorldNode)(mnode_t *node);
	texture_t *(*R_TextureAnimation)(msurface_t *fa);
	void (*R_RenderDynamicLightmaps)(msurface_t *fa);
	void(*R_RotateForEntity)(float *origin, cl_entity_t *ent);
	void (*R_DrawDecals)(qboolean bMultitexture);
	void (*Draw_MiptexTexture)(cachewad_t *wad, byte *data);
	texture_t *(*Draw_DecalTexture)(int index);
	void (*R_BuildLightMap)(msurface_t *psurf, byte *dest, int stride);
	void(*R_AddDynamicLights)(msurface_t *psurf);
	int(*GL_LoadTexture)(char *identifier, int textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal);
	int(*GL_LoadTexture2)(char *identifier, int textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter);
	int(*GL_Upload16)(byte *data, int width, int height, int iType, byte *pPal, int a6, int a7, int a8);
	void (*R_DecalMPoly)(float *v, texture_t *ptexture, msurface_t *psurf, int vertCount);
	void (*R_MarkLeaves)(void);
	void (*R_DrawBrushModel)(cl_entity_t *e);
	void (*R_DrawSpriteModel)(cl_entity_t *ent);
	int (*CL_FxBlend)(cl_entity_t *ent);
	float(*R_GlowBlend)(cl_entity_t *ent);
	void (*VID_UpdateWindowVars)(RECT *prc, int x, int y);
	mleaf_t *(*Mod_PointInLeaf)(vec3_t p, model_t *model);
	void *(*realloc_SvEngine)(void *, size_t);
	dlight_t *(*CL_AllocDlight)(int key);
	void(*S_ExtraUpdate)(void);
	void(*R_DrawViewModel)(void);
	void(*R_PolyBlend)(void);
	void(*R_DecalShootInternal)(texture_t *ptexture, int index, int entity, int modelIndex, vec3_t position, int flags, float flScale);
	void(*R_LoadSkys)(void);
	void(*R_LoadSkyboxInt_SvEngine)(const char *name);
	void(*R_LoadSkyBox_SvEngine)(const char *name);
	//void(*R_MarkLights)(dlight_t *light, int bit, mnode_t *node);
	int(*CL_IsDevOverviewMode)(void);
	void(*CL_SetDevOverView)(void *a1);
	void(*Mod_LoadStudioModel)(model_t *mod, void *buffer);
	void(*Mod_LoadBrushModel)(model_t *mod, void *buffer);
	model_t *(*Mod_LoadModel)(model_t *mod, qboolean crash, qboolean trackCRC);
	void(*triapi_RenderMode)(int mode);
	void(*triapi_Color4f) (float r, float g, float b, float a);
	void(__fastcall *enginesurface_drawFlushText)(void *pthis, int);
	void(*DLL_SetModKey)(void *pinfo, char *pkey, char *pvalue);
	void(*SCR_BeginLoadingPlaque)(qboolean reconnect);
	qboolean(*Host_IsSinglePlayerGame)(void);
	void *(*Hunk_AllocName)(int size, const char *name);

	//Sven Client DLL
	void(__fastcall *ClientPortalManager_ResetAll)(void * pthis, int dummy);
	mtexinfo_t *(__fastcall *ClientPortalManager_GetOriginalSurfaceTexture)(void * pthis, int dummy, msurface_t *surf);
	void(__fastcall *ClientPortalManager_DrawPortalSurface)(void * pthis, int dummy, void *ClientPortal, msurface_t *surf, GLuint texture);
	void(__fastcall *ClientPortalManager_EnableClipPlane)(void * pthis, int dummy, int index, vec3_t a1, vec3_t a2, vec3_t a3);

	//Engine Studio
	void (*R_GLStudioDrawPoints)(void);
	void (*R_LightStrength)(int bone, float *vert, float (*light)[4]);
	void (*R_StudioLighting)(float *lv, int bone, int flags, vec3_t normal);
	void (*R_StudioSetupSkin)(studiohdr_t *ptexturehdr, int index);
	void (*R_LightLambert)(float (*light)[4], float *normal, float *src, float *lambert);
	void (*BuildNormalIndexTable)(void);
	void (*BuildGlowShellVerts)(vec3_t *pstudioverts, auxvert_t *pauxverts);
	void (*R_StudioChrome)(int *pchrome, int bone, vec3_t normal);

	//Engine Studio Exported API
	void (*studioapi_StudioDynamicLight)(struct cl_entity_s *ent, struct alight_s *plight);
	//void (*studioapi_RestoreRenderer)(void);
	qboolean (*studioapi_StudioCheckBBox)(void);

	//Client Studio
	void(__fastcall *GameStudioRenderer_StudioSetupBones)(void *pthis, int);
	void(__fastcall *GameStudioRenderer_StudioMergeBones)(void *pthis, int, model_t *pSubModel);
	int(__fastcall *GameStudioRenderer_StudioDrawModel)(void *pthis, int, int flags);
	int(__fastcall *GameStudioRenderer_StudioDrawPlayer)(void *pthis, int, int flags, struct entity_state_s *pplayer);
	int(__fastcall *GameStudioRenderer__StudioDrawPlayer)(void *pthis, int, int flags, struct entity_state_s *pplayer);
	void (__fastcall *GameStudioRenderer_StudioRenderModel)(void *pthis, int);
	void (__fastcall *GameStudioRenderer_StudioRenderFinal)(void *pthis, int);

	int GameStudioRenderer_StudioCalcAttachments_vftable_index;
	int GameStudioRenderer_StudioSetupBones_vftable_index;
	int GameStudioRenderer_StudioSaveBones_vftable_index;
	int GameStudioRenderer_StudioMergeBones_vftable_index;
	int GameStudioRenderer_StudioDrawModel_vftable_index;
	int GameStudioRenderer_StudioDrawPlayer_vftable_index;
	int GameStudioRenderer__StudioDrawPlayer_vftable_index;
	int GameStudioRenderer_StudioRenderModel_vftable_index;
	int GameStudioRenderer_StudioRenderFinal_vftable_index;

	//Engine Studio
	void(*R_StudioRenderModel)(void);
	void(*R_StudioRenderFinal)(void);
	void(*R_StudioSetupBones)(void);
	void(*R_StudioMergeBones)(model_t *pSubModel);

}ref_funcs_t;

extern hook_t *g_phook_GL_BeginRendering;
extern hook_t *g_phook_GL_EndRendering;
extern hook_t *g_phook_R_RenderView_SvEngine;
extern hook_t *g_phook_R_RenderView;
extern hook_t *g_phook_R_LoadSkyBox_SvEngine;
extern hook_t *g_phook_R_LoadSkys;
extern hook_t *g_phook_R_NewMap;
extern hook_t *g_phook_R_CullBox;
extern hook_t *g_phook_Mod_PointInLeaf;
extern hook_t *g_phook_R_BuildLightMap;
extern hook_t *g_phook_R_AddDynamicLights;
extern hook_t *g_phook_R_GLStudioDrawPoints;
extern hook_t *g_phook_GL_LoadTexture2;
extern hook_t *g_phook_enginesurface_drawFlushText;
extern hook_t *g_phook_Mod_LoadStudioModel;
extern hook_t *g_phook_Mod_LoadBrushModel;
extern hook_t *g_phook_triapi_RenderMode;
extern hook_t *g_phook_Draw_MiptexTexture;
extern hook_t *g_phook_BuildGammaTable;
extern hook_t *g_phook_DLL_SetModKey;

//extern hook_t *g_phook_studioapi_RestoreRenderer;
extern hook_t *g_phook_studioapi_StudioDynamicLight;
extern hook_t *g_phook_studioapi_StudioCheckBBox;
extern hook_t *g_phook_CL_FxBlend;

extern hook_t *g_phook_ClientPortalManager_ResetAll;
extern hook_t *g_phook_ClientPortalManager_DrawPortalSurface;
extern hook_t *g_phook_ClientPortalManager_EnableClipPlane;

extern hook_t *g_phook_GameStudioRenderer_StudioRenderModel;
extern hook_t *g_phook_GameStudioRenderer_StudioRenderFinal;
extern hook_t *g_phook_R_StudioRenderModel;
extern hook_t *g_phook_R_StudioRenderFinal;

extern ref_funcs_t gRefFuncs;