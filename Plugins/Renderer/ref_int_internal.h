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
	void (*Cvar_DirectSet)(cvar_t *var, char *value);
	void (*BuildGammaTable)(float gamma);
	void (*R_ForceCVars)(qboolean mp);
	void (*R_CheckVariables)(void);
	void (*R_AnimateLight)(void);
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
	void (*EmitWaterPolys)(msurface_t *fa, int direction);
	void (*R_DrawSequentialPoly)(msurface_t *s, int face);
	void (*R_RecursiveWorldNode)(mnode_t *node);
	texture_t *(*R_TextureAnimation)(msurface_t *fa);
	void (*R_RenderDynamicLightmaps)(msurface_t *fa);
	void(*R_RotateForEntity)(float *origin, cl_entity_t *ent);
	void (*R_DrawDecals)(qboolean bMultitexture);
	texture_t *(*Draw_DecalTexture)(int index);
	void (*R_BuildLightMap)(msurface_t *psurf, byte *dest, int stride);
	void(*R_AddDynamicLights)(msurface_t *psurf);
	int(*GL_LoadTexture)(char *identifier, int textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal);
	int(*GL_LoadTexture2)(char *identifier, int textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter);
	void (*R_DecalMPoly)(float *v, texture_t *ptexture, msurface_t *psurf, int vertCount);
	void (*R_MarkLeaves)(void);
	void (*R_DrawBrushModel)(cl_entity_t *e);
	void (*R_DrawSpriteModel)(cl_entity_t *ent);
	int (*CL_FxBlend)(cl_entity_t *ent);
	float(*GlowBlend)(cl_entity_t *ent);
	void (*VID_UpdateWindowVars)(RECT *prc, int x, int y);
	mleaf_t *(*Mod_PointInLeaf)(vec3_t p, model_t *model);
	void *(*realloc_SvEngine)(void *, size_t);
	dlight_t *(*CL_AllocDlight)(int key);
	void(*S_ExtraUpdate)(void);
	void(*R_PolyBlend)(void);
	void(*R_DecalShootInternal)(texture_t *ptexture, int index, int entity, int modelIndex, vec3_t position, int flags, float flScale);
	void(*R_LoadSkys)(void);
	void(*R_LoadSkyName_SvEngine)(const char *name);
	void(*R_MarkLights)(dlight_t *light, int bit, mnode_t *node);
	int(*CL_IsDevOverviewMode)(void);
	void(*CL_SetDevOverView)(void *a1);
	void (*Mod_LoadStudioModel)(model_t *mod, void *buffer);
	void(*triapi_RenderMode)(int mode);
	void(__fastcall *enginesurface_drawFlushText)(void *pthis, int);
	void(*DLL_SetModKey)(void *pinfo, char *pkey, char *pvalue);
	//SvClient
	void(__fastcall *PortalManager_ResetAll)(int pthis, int);

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
	void (*studioapi_RestoreRenderer)(void);

	//Client Studio
	void (__fastcall *GameStudioRenderer_StudioRenderModel)(void *pthis, int);
	void (__fastcall *GameStudioRenderer_StudioRenderFinal)(void *pthis, int);
}ref_funcs_t;

extern ref_funcs_t gRefFuncs;

#define r_draw_normal 0
#define r_draw_reflect 1