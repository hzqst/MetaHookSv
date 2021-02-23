#pragma once

#include <gl/gl.h>
#include "gl_model.h"

typedef struct vrect_s
{
	int x, y, width, height;
	struct vrect_s *pnext;
}
vrect_t;

typedef struct
{
	int r, g, b;
}mcolor24_t;

typedef struct refdef_s
{
	vec3_t vieworg;
	vec3_t viewangles;
	color24 ambientlight;
	qboolean onlyClientDraws;
	qboolean useCamera;
	vec3_t r_camera_origin;
}refdef_t;

typedef struct skybox_s
{
	float v[2][6];
}skybox_t;

typedef struct msurface_s msurface_t;

typedef struct
{
	void(*Cache_Free)(cache_user_t *c);
	void (*R_Clear)(void);
	void (*R_ForceCVars)(qboolean mp);
	void (*R_RenderView)(void);
	void(*R_RenderView_SvEngine)(int a1);
	void (*R_RenderScene)(void);
	void (*R_NewMap)(void);
	void (*R_DrawEntitiesOnList)(void);
	void (*R_DrawTEntitiesOnList)(int onlyClientDraw);
	void(*R_AddTEntity)(cl_entity_t *pEnt);
	void (*R_DrawWorld)(void);
	void (*R_SetupFrame)(void);
	void (*R_SetFrustum)(void);
	void (*R_SetupGL)(void);
	void (*R_DrawSkyChain)(msurface_t *s);
	void (*R_DrawSkyBox)(void);
	qboolean (*R_CullBox)(vec3_t mins, vec3_t maxs);
	void (*GL_BuildLightmaps)(void);
	void (*GL_Bind)(int texnum);
	void (*GL_SelectTexture)(GLenum target);
	void (*GL_DisableMultitexture)(void);
	void (*GL_EnableMultitexture)(void);
	void (*GL_BeginRendering)(int *x, int *y, int *width, int *height);
	void (*GL_EndRendering)(void);
	void (*EmitWaterPolys)(msurface_t *fa, int direction);
	void (*BuildSurfaceDisplayList)(msurface_t *fa);
	void (*R_DrawSequentialPoly)(msurface_t *s, int face);
	void (*R_RecursiveWorldNode)(mnode_t *node);
	texture_t *(*R_TextureAnimation)(msurface_t *fa);
	void (*R_RenderDynamicLightmaps)(msurface_t *fa);
	void (*R_DrawDecals)(qboolean bMultitexture);
	texture_t *(*Draw_DecalTexture)(int index);
	void (*R_AllocObjects)(int nMax);
	void (*Draw_MiptexTexture)(cachewad_t *wad, byte *data);
	void (*R_BuildLightMap)(msurface_t *psurf, byte *dest, int stride);
	void(*R_AddDynamicLights)(msurface_t *psurf);
	int(*GL_LoadTexture)(char *identifier, int textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal);
	int(*GL_LoadTexture2)(char *identifier, int textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter);
	void (*R_DecalMPoly)(float *v, texture_t *ptexture, msurface_t *psurf, int vertCount);
	void (*R_MarkLeaves)(void);
	void (*R_DrawBrushModel)(cl_entity_t *e);
	float (*GlowBlend)(cl_entity_t *e);
	void (*R_DrawSpriteModel)(cl_entity_t *ent);
	int (*CL_FxBlend)(cl_entity_t *ent);
	mspriteframe_t *(*R_GetSpriteFrame)(msprite_t *spr, int frame);
	void (*R_GetSpriteAxes)(cl_entity_t *entity, int type, float *vforwrad, float *vright, float *vup);
	void (*R_SpriteColor)(mcolor24_t *col, cl_entity_t *entity, int renderamt);
	void (*VID_UpdateWindowVars)(RECT *prc, int x, int y);
	mleaf_t *(*Mod_PointInLeaf)(vec3_t p, model_t *model);
	void *(*realloc_SvEngine)(void *, size_t);
	dlight_t *(*CL_AllocDlight)(int key);

	//Engine Studio
	void (*R_GLStudioDrawPoints)(void);
	studiohdr_t *(*R_LoadTextures)(struct model_s *psubmodel);
	void (*R_LightStrength)(int bone, float *vert, float (*light)[4]);
	void (*R_StudioLighting)(float *lv, int bone, int flags, vec3_t normal);
	void (*R_StudioSetupSkin)(studiohdr_t *ptexturehdr, int index);
	void (*R_LightLambert)(float (*light)[4], float *normal, float *src, float *lambert);
	void (*BuildNormalIndexTable)(void);
	void (*BuildGlowShellVerts)(vec3_t *pstudioverts, auxvert_t *pauxverts);
	void (*R_StudioChrome)(int *pchrome, int bone, vec3_t normal);
	void (*R_StudioRenderFinal)(void);

	//Studio API
	void (*studioapi_StudioDynamicLight)(struct cl_entity_s *ent, struct alight_s *plight);
	void (*studioapi_SetupRenderer)(int rendermode);
	void (*studioapi_RestoreRenderer)(void);
	void (*studioapi_SetupModel)(int bodypart, void **ppbodypart, void **ppsubmodel);
}ref_funcs_t;

typedef struct
{
	GLuint (*R_CompileShader)(const char *vscode, const char *gscode, const char *fscode, const char *vsfile, const char *gsfile, const char *fsfile);
	GLuint (*R_CompileShaderFile)(const char *vsfile, const char *gsfile, const char *fsfile);
	GLuint (*R_CompileShaderFileEx)(const char *vsfile, const char *gsfile, const char *fsfile, const char *vsdefine, const char *gsdefine, const char *fsdefine);
	void (*GL_UseProgram)(GLuint program);
	void (*GL_EndProgram)(void);
	GLuint (*GL_GetUniformLoc)(GLuint program, const char *name);
	GLuint (*GL_GetAttribLoc)(GLuint program, const char *name);
	void (*GL_Uniform1i)(GLuint loc, int v0);
	void (*GL_Uniform2i)(GLuint loc, int v0, int v1);
	void (*GL_Uniform3i)(GLuint loc, int v0, int v1, int v2);
	void (*GL_Uniform4i)(GLuint loc, int v0, int v1, int v2, int v3);
	void (*GL_Uniform1f)(GLuint loc, float v0);
	void (*GL_Uniform2f)(GLuint loc, float v0, float v1);
	void (*GL_Uniform3f)(GLuint loc, float v0, float v1, float v2);
	void (*GL_Uniform4f)(GLuint loc, float v0, int v1, int v2, int v3);
	void (*GL_VertexAttrib3f)(GLuint index, float x, float y, float z);
	void (*GL_VertexAttrib3fv)(GLuint index, float *v);
	void (*GL_MultiTexCoord2f)(GLenum target, float s, float t);
	void (*GL_MultiTexCoord3f)(GLenum target, float s, float t, float r);
}shaderapi_t;

typedef struct
{
	void (*GL_Bind)(int texnum);
	void (*GL_SelectTexture)(GLenum target);
	void (*GL_DisableMultitexture)(void);
	void (*GL_EnableMultitexture)(void);
	void (*R_DrawBrushModel)(cl_entity_t *entity);
	void (*R_DrawSpriteModel)(cl_entity_t *entity);
	void (*R_GetSpriteAxes)(cl_entity_t *entity, int type, float *vforwrad, float *vright, float *vup);
	void (*R_SpriteColor)(mcolor24_t *col, cl_entity_t *entity, int renderamt);
	float (*GlowBlend)(cl_entity_t *entity);
	int (*CL_FxBlend)(cl_entity_t *entity);
	int (*R_CullBox)(vec3_t mins, vec3_t maxs);
}engrefapi_t;

typedef struct
{
	//common
	int (*R_GetDrawPass)(void);
	int (*R_GetSupportExtension)(void);
	//refdef
	void (*R_PushRefDef)(void);
	void (*R_PopRefDef)(void);
	refdef_t *(*R_GetRefDef)(void);
	//framebuffer
	void (*GL_PushFrameBuffer)(void);
	void (*GL_PopFrameBuffer)(void);
	//texture
	GLuint (*GL_GenTextureRGBA8)(int w, int h);
	int (*R_LoadTextureEx)(const char *filepath, const char *name, int *width, int *height, GL_TEXTURETYPE type, qboolean mipmap, qboolean ansio);
	int (*GL_LoadTextureEx)(const char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, qboolean ansio);
	gltexture_t *(*R_GetCurrentGLTexture)(void);
	void (*GL_UploadDXT)(byte *data, int width, int height, qboolean mipmap, qboolean ansio);
	int (*LoadDDS)(const char *filename, byte *buf, int bufSize, int *width, int *height);
	int (*LoadImageGeneric)(const char *filename, byte *buf, int bufSize, int *width, int *height);	
	int (*SaveImageGeneric)(const char *filename, int width, int height, byte *data);
	//capture screen
	byte *(*R_GetSCRCaptureBuffer)(int *bufsize);
	//3dsky
	void (*R_Add3DSkyEntity)(cl_entity_t *ent);
	void (*R_Setup3DSkyModel)(void);
	void (*R_Finish3DSkyModel)(void);
	//2d postprocess
	void (*R_BeginFXAA)(int w, int h);
	//cloak
	void (*R_RenderCloakTexture)(void);
	int (*R_GetCloakTexture)(void);
	void (*R_BeginRenderConc)(float flBlurFactor, float flRefractFactor);
	//shader
	shaderapi_t ShaderAPI;
	engrefapi_t RefAPI;
}ref_export_t;

extern ref_funcs_t gRefFuncs;
extern ref_export_t gRefExports;

#define r_draw_normal 0
#define r_draw_reflect 1
#define r_draw_refract 2
#define r_draw_shadow 3
#define r_draw_3dhud 4
#define r_draw_shadowscene 5
#define r_draw_3dsky 6
#define r_draw_hudinworld 7

#define r_ext_fbo (1<<0)
#define r_ext_msaa (1<<1)
#define r_ext_water (1<<2)
#define r_ext_shader (1<<3)
#define r_ext_shadow (1<<4)

enum
{
	kRenderFxCloak = 22,
	kRenderFxShadow,
	kRenderFxFireLayer,
	kRenderFxInvulnLayer
};

#define META_RENDERER_VERSION "Meta Renderer 3.0"