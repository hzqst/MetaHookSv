#pragma once

#include <metahook.h>
#include <math.h>
#include <assert.h>
#include <archtypes.h>
#include <const.h>
#include <custom.h>
#include <com_model.h>
#include <ref_params.h>
#include <cvardef.h>
#include <studio.h>
#include <r_studioint.h>
#include <pm_movevars.h>
#include <pm_shared.h>
#include <particledef.h>
#include <triangleapi.h>
#include <entity_types.h>

#include <set>
#include <map>
#include <atomic>

#include "qgl.h"
#include "mathlib2.h"
#include "plugins.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "util.h"

#include "zone.h"

#include "gl_common.h"
#include "gl_shader.h"
#include "gl_model.h"
#include "gl_water.h"
#include "gl_sprite.h"
#include "gl_studio.h"
#include "gl_hud.h"
#include "gl_shadow.h"
#include "gl_light.h"
#include "gl_wsurf.h"
#include "gl_draw.h"
#include "gl_cvar.h"
#include "gl_portal.h"
#include "gl_entity.h"

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct refdef_s
{
	vrect_GoldSrc_t *vrect;
	vec3_t *vieworg;
	vec3_t *viewangles;
	color24 *ambientlight;
	qboolean *onlyClientDraws;
}refdef_t;

extern refdef_t r_refdef;
extern refdef_GoldSrc_t *r_refdef_GoldSrc;
extern refdef_SvEngine_t *r_refdef_SvEngine;
extern ref_params_t r_params;
extern float *scrfov;
extern float r_xfov;
extern float r_yfov;
extern float r_xfov_viewmodel;
extern float r_yfov_viewmodel;
extern float r_xfov_currentpass;
extern float r_yfov_currentpass;
extern float r_screenaspect;

extern cl_entity_t* r_worldentity;
extern model_t** cl_worldmodel;

extern int *cl_numvisedicts;
extern cl_entity_t **cl_visedicts;
extern cl_entity_t **currententity;
extern int *maxTransObjs;
extern int *numTransObjs;
extern transObjRef **transObjects;
extern mleaf_t **r_viewleaf;
extern mleaf_t **r_oldviewleaf;
extern int *r_loading_skybox;

extern RECT *window_rect;

extern float * s_fXMouseAspectAdjustment;
extern float * s_fYMouseAspectAdjustment;

extern float s_fXMouseAspectAdjustment_Storage;
extern float s_fYMouseAspectAdjustment_Storage;

extern vec_t *vup;
extern vec_t *vpn;
extern vec_t *vright;
extern vec_t *r_origin;
extern vec_t *modelorg;
extern vec_t *r_entorigin;
extern float *r_world_matrix;
extern float *r_projection_matrix;
extern float *gWorldToScreen;
extern float *gScreenToWorld;
extern overviewInfo_t *gDevOverview;
extern mplane_t *frustum;

extern qboolean* vertical_fov_SvEngine;

extern vec_t* cl_simorg;

extern int *r_framecount;
extern int *r_visframecount;

extern int *cl_max_edicts;
extern cl_entity_t **cl_entities;

extern TEMPENTITY *gTempEnts;

extern struct playermove_s* pmove;
extern struct playermove_10152_s* pmove_10152;

extern int *cl_viewentity;
extern void *cl_frames;
extern int size_of_frame;
extern int *cl_parsecount;
extern int *cl_waterlevel;
extern int *envmap;
extern int *cl_stats;
extern double *cl_time;
extern double *cl_oldtime;
extern float *cl_weaponstarttime;
extern int *cl_weaponsequence;
extern int *cl_light_level;
extern int *c_alias_polys;
extern int *c_brush_polys;
extern int(*rtable)[20][20];
extern void *tmp_palette;

extern int* gSpriteMipMap;

//fog
extern int *g_bUserFogOn;
extern float *g_UserFogColor;
extern float *g_UserFogDensity;
extern float *g_UserFogStart;
extern float *g_UserFogEnd;

extern model_t *mod_known;
extern int *mod_numknown;

extern char(*loadname)[64];
extern model_t **loadmodel;

//client dll

extern int *g_iUser1;
extern int *g_iUser2;

extern float* g_iFogColor_SCClient;
extern float* g_iStartDist_SCClient;
extern float* g_iEndDist_SCClient;

extern int* g_iWaterLevel;
extern bool* g_bRenderingPortals_SCClient;
extern int* g_ViewEntityIndex_SCClient;

extern bool g_bPortalClipPlaneEnabled[6];

extern vec4_t g_PortalClipPlane[6];

extern bool g_bHasLowerBody;

//gl extension

extern int gl_max_texture_size;
extern float gl_max_ansio;
extern int *gl_msaa_fbo;
extern int *gl_backbuffer_fbo;
extern int *gl_mtexable;

extern qboolean *mtexenabled;

extern vec_t* r_soundOrigin;
extern vec_t* r_playerViewportAngles;

extern cactive_t *cls_state;
extern int *cls_signon;
extern qboolean *scr_drawloading;

extern movevars_t* pmovevars;

extern int *filterMode;
extern float *filterColorRed;
extern float *filterColorGreen;
extern float *filterColorBlue;
extern float *filterBrightness;

extern bool* detTexSupported;

extern cache_system_t(*cache_head);

extern texture_t** r_notexture_mip;

//Sven Co-op only
extern texture_t** r_missingtexture;

extern int* allow_cheats;

extern int* allocated_textures;

extern int *gRenderMode;

extern int glx;
extern int gly;
extern int glwidth;
extern int glheight;

extern bool g_bNoStretchAspect;
extern bool g_bUseOITBlend;
//extern bool bVerticalFov;//unused
extern bool g_bUseLegacyTextureLoader;
extern bool g_bHasOfficialFBOSupport;
extern bool g_bHasOfficialGLTexAllocSupport;

extern FBO_Container_t s_FinalBufferFBO;
extern FBO_Container_t s_BackBufferFBO;
extern FBO_Container_t s_BackBufferFBO2;
extern FBO_Container_t s_BackBufferFBO3;
extern FBO_Container_t s_GBufferFBO;
extern FBO_Container_t s_BlendBufferFBO;
extern FBO_Container_t s_DownSampleFBO[DOWNSAMPLE_BUFFERS];
extern FBO_Container_t s_LuminFBO[LUMIN_BUFFERS];
extern FBO_Container_t s_Lumin1x1FBO[LUMIN1x1_BUFFERS];
extern FBO_Container_t s_BrightPassFBO;
extern FBO_Container_t s_BlurPassFBO[BLUR_BUFFERS][2];
extern FBO_Container_t s_BrightAccumFBO;
extern FBO_Container_t s_ToneMapFBO;
extern FBO_Container_t s_DepthLinearFBO;
extern FBO_Container_t s_HBAOCalcFBO;
extern FBO_Container_t s_ShadowFBO;
extern FBO_Container_t s_WaterSurfaceFBO;

extern FBO_Container_t* g_CurrentSceneFBO;
extern FBO_Container_t *g_CurrentRenderingFBO;

extern msurface_t **skychain;
extern msurface_t **waterchain;

extern int *gSkyTexNumber;

extern cvar_t *r_bmodelinterp;
extern cvar_t *r_bmodelhighfrac;
extern cvar_t *r_norefresh;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawviewmodel;
extern cvar_t *r_speeds;
extern cvar_t *r_fullbright;
extern cvar_t *r_decals;
extern cvar_t *r_lightmap;
extern cvar_t *r_shadows;
extern cvar_t *r_mirroralpha;
extern cvar_t *r_wateralpha;
extern cvar_t *r_dynamic;
extern cvar_t* r_novis;
extern cvar_t *r_mmx;
extern cvar_t *r_traceglow;
extern cvar_t *r_wadtextures;
extern cvar_t *r_glowshellfreq;
extern cvar_t *r_detailtextures;
extern cvar_t *r_cullsequencebox;

extern cvar_t *gl_vsync;
extern cvar_t *gl_ztrick;
extern cvar_t *gl_finish;
extern cvar_t *gl_clear;
extern cvar_t *gl_clearcolor;
extern cvar_t *gl_cull;
extern cvar_t *gl_texsort;
extern cvar_t *gl_smoothmodels;
extern cvar_t *gl_affinemodels;
extern cvar_t *gl_flashblend;
extern cvar_t *gl_playermip;
extern cvar_t *gl_nocolors;
extern cvar_t *gl_keeptjunctions;
extern cvar_t *gl_reporttjunctions;
extern cvar_t *gl_wateramp;
extern cvar_t *gl_dither;
extern cvar_t *gl_spriteblend;
extern cvar_t *gl_polyoffset;
extern cvar_t *gl_lightholes;
extern cvar_t *gl_zmax;
extern cvar_t *gl_alphamin;
extern cvar_t *gl_overdraw;
extern cvar_t *gl_watersides;
extern cvar_t *gl_overbright;
extern cvar_t *gl_envmapsize;
extern cvar_t *gl_flipmatrix;
extern cvar_t *gl_monolights;
extern cvar_t *gl_fog;
extern cvar_t *gl_wireframe;
extern cvar_t *gl_ansio;
extern cvar_t *developer;
extern cvar_t *gl_round_down;
extern cvar_t *gl_picmip;
extern cvar_t *gl_max_size;

extern cvar_t *v_texgamma;
extern cvar_t *v_lightgamma;
extern cvar_t *v_brightness;
extern cvar_t *v_gamma;
extern cvar_t *v_lambert;

extern cvar_t *cl_righthand;
extern cvar_t *chase_active;
extern cvar_t *spec_pip;

extern cvar_t *dev_overview_color;

extern cvar_t *r_gamma_blend;

extern cvar_t *r_linear_blend_shift;

extern cvar_t *r_linear_fog_shift;

extern cvar_t *r_linear_fog_shiftz;

extern cvar_t * r_fog_trans;

extern cvar_t* r_drawlowerbody;

extern cvar_t* r_drawlowerbodypitch;

extern cvar_t* r_drawlowerbodyclipnear;

extern cvar_t* r_drawlowerbodyclipfar;

extern cvar_t* r_sprite_lerping;

extern cvar_t* r_detailskytextures;

extern cvar_t* r_leaf_lazy_load;

extern cvar_t* r_studio_lazy_load;

extern cvar_t* r_studio_unload;

extern cvar_t* r_wsurf_parallax_scale;

extern cvar_t* r_wsurf_sky_fog;

extern cvar_t* gl_nearplane;

//extern cvar_t* viewmodel_nearplane;

//extern cvar_t* viewmodel_farplane;

//extern cvar_t* viewmodel_scale;

void GammaToLinear(float *color);
void R_LoadSkyBox_SvEngine(const char *name);
void R_LoadSkys(void);
void Mod_Init(void);
void BuildGammaTable(float g);
void V_RenderView(void);
void R_RenderView(void);
void R_RenderScene(void);
void R_RenderView_SvEngine(int viewIdx);
qboolean R_CullBox(vec3_t mins, vec3_t maxs);
qboolean Host_IsSinglePlayerGame();
bool AllowCheats();
void R_ForceCVars(qboolean mp);
void R_NewMap(void);
void GL_BuildLightmaps(void);
void Host_ClearMemory(qboolean bQuite);
void R_Init(void);
void R_Shutdown(void);
void R_SetupGL(void);
void R_SetupGLForViewModel(void);
void R_MarkLeaves(void);
void R_PrepareDrawWorld(void);
void R_DrawWorld(void);
void R_DrawSkyBox(void);
void R_CheckVariables(void);
void R_AnimateLight(void);
void R_SetupFrame(void);
void R_SetFrustum(void);
void R_SetupSceneUBO(void);
void R_SetupCameraUBO(void);
void R_GameFrameStart();
void R_RenderFrameStart();
void R_RenderEndFrame();
mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model);
void R_RecursiveWorldNode(mnode_t *node);
void R_DrawParticles(void);
void R_RotateForEntity(cl_entity_t *ent, float out[4][4]);
void R_RotateForTransform(const float* in_origin, const float* in_angles);
void R_SetRenderMode(cl_entity_t *pEntity);
float *R_GetAttachmentPoint(int entity, int attachment);
void R_DrawBrushModel(cl_entity_t *entity);
void R_DrawSpriteModel(cl_entity_t *entity);
entity_state_t *R_GetPlayerState(int index);
bool CL_IsDevOverviewMode(void);
int CL_FxBlend(cl_entity_t *entity);
void R_DrawCurrentEntity(bool bTransparent);
void R_DrawEntitiesOnList(void);
void R_DrawTEntitiesOnList(int onlyClientDraw);
void R_AddTEntity(cl_entity_t *pEnt);
void R_ResetLatched_Patched(cl_entity_t* ent, qboolean full_reset);
void GL_Shutdown(void);
void GL_Init(void);
void GL_BeginRendering(int *x, int *y, int *width, int *height);
void GL_EndRendering(void);
GLuint GL_GenTexture(void);
GLuint GL_GenBuffer(void);
GLuint GL_GenVAO(void);
void GL_ClearFBO(FBO_Container_t* s);
void GL_FreeFBO(FBO_Container_t* s);
void GL_DeleteTexture(GLuint texid);
void GL_DeleteBuffer(GLuint buf);
void GL_DeleteVAO(GLuint VAO);
void GL_BindVAO(GLuint VAO);
void GL_BindABO(GLuint ABO);
void GL_UploadSubDataToUBO(GLuint UBO, size_t offset, size_t size, const void* data);
void GL_UploadDataToVBOStaticDraw(GLuint VBO, size_t size, const void* data);
void GL_UploadDataToVBODynamicDraw(GLuint VBO, size_t size, const void* data);
void GL_UploadDataToVBOStreamDraw(GLuint VBO, size_t size, const void* data);
void GL_UploadSubDataToVBO(GLuint VBO, size_t offset, size_t size, const void* data);
void GL_UploadDataToEBOStaticDraw(GLuint EBO, size_t size, const void* data);
void GL_UploadDataToEBODynamicDraw(GLuint EBO, size_t size, const void* data);
void GL_UploadDataToEBOStreamDraw(GLuint EBO, size_t size, const void* data);
void GL_UploadSubDataToEBO(GLuint EBO, size_t offset, size_t size, const void* data);
void GL_UploadDataToABOStaticDraw(GLuint ABO, size_t size, const void* data);
void GL_UploadDataToABODynamicDraw(GLuint ABO, size_t size, const void* data);
void GL_BindStatesForVAO(GLuint VAO, const std::function<void()>& bind, const std::function<void()>& unbind);
void GL_BindStatesForVAO(GLuint VAO, GLuint VBO, GLuint EBO, const std::function<void()>& bind, const std::function<void()>& unbind);
void GL_Bind(int texnum);
void GL_SelectTexture(GLenum target);
void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);
void triapi_Shutdown();
void triapi_RenderMode(int mode);
void triapi_Begin(int primitiveCode);
void triapi_End();
void triapi_Color4f(float r, float g, float b, float a);
void triapi_Color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void triapi_Vertex3fv(float* v);
void triapi_Vertex3f(float x, float y, float z);
void triapi_TexCoord2f(float s, float t);
void triapi_Brightness(float brightness);
void triapi_Color4fRendermode(float r, float g, float b, float a, int rendermode);
int triapi_BoxInPVS(float* mins, float* maxs);
void triapi_GetMatrix(const int pname, float* matrix);
void triapi_Fog(float* flFogColor, float flStart, float flEnd, qboolean bOn);
void triapi_FogParams(float flDensity, qboolean bFogAffectsSkybox);

void __stdcall SCClient_glBegin(int GLPrimitiveCode);
void __stdcall SCClient_glEnd();
void __stdcall SCClient_glColor4f(float r, float g, float b, float a);

void GL_UnloadTextureByIdentifier(const char* identifier);
void GL_UnloadTextures(void);
int GL_LoadTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal);
int GL_LoadTexture2(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter);
void GL_InitShaders(void);
void GL_FreeShaders(void);
texture_t *Draw_DecalTexture(int index);
void Draw_MiptexTexture(cachewad_t *wad, byte *data);
mbasenode_t* PVSNode(mbasenode_t* basenode, vec3_t emins, vec3_t emaxs);
void EmitWaterPolys(msurface_t *fa, int direction);
void R_DecalShootInternal(texture_t *ptexture, int index, int entity, int modelIndex, vec3_t position, int flags, float flScale);
void __fastcall enginesurface_drawSetTextureFile(void* pthis, int, int textureId, const char* filename, qboolean hardwareFilter, bool forceReload);
int __fastcall enginesurface_createNewTextureID(void* pthis, int);
void __fastcall enginesurface_drawFlushText(void *pthis, int dummy);
void* Draw_CustomCacheGet(cachewad_t* wad, void* raw, int rawsize, int index);
void* Draw_CacheGet(cachewad_t* wad, int index);
int SignbitsForPlane(mplane_t *out);
qboolean R_ParseStringAsColor1(const char *string, float *vec);
qboolean R_ParseStringAsColor2(const char *string, float *vec);
qboolean R_ParseStringAsColor3(const char *string, float *vec);
qboolean R_ParseStringAsColor4(const char *string, float *vec);
qboolean R_ParseStringAsVector1(const char *string, float *vec);
qboolean R_ParseStringAsVector2(const char *string, float *vec);
qboolean R_ParseStringAsVector3(const char *string, float *vec);
qboolean R_ParseStringAsVector4(const char *string, float *vec);
qboolean R_ParseCvarAsColor1(cvar_t *cvar, float *vec);
qboolean R_ParseCvarAsColor2(cvar_t *cvar, float *vec);
qboolean R_ParseCvarAsColor3(cvar_t *cvar, float *vec);
qboolean R_ParseCvarAsColor4(cvar_t *cvar, float *vec);
qboolean R_ParseCvarAsVector1(cvar_t *cvar, float *vec);
qboolean R_ParseCvarAsVector2(cvar_t *cvar, float *vec);
qboolean R_ParseCvarAsVector3(cvar_t *cvar, float *vec);
qboolean R_ParseCvarAsVector4(cvar_t *cvar, float *vec);
colorVec R_LightPoint(vec3_t p);
void *R_GetRefDef(void);
GLuint GL_GenTextureRGBA8(int w, int h, bool immutable);

void GL_CreateDepthTexture(int texid, int w, int h, bool immutable);
GLuint GL_GenDepthTexture(int w, int h, bool immutable);

void GL_CreateDepthStencilTexture(int texid, int w, int h, bool immutable);
GLuint GL_GenDepthStencilTexture(int w, int h, bool immutable);

GLuint GL_CreateDepthViewForDepthTexture(int texId);
GLuint GL_CreateStencilViewForDepthTexture(int texId);

GLuint GL_GenTextureColorFormat(int w, int h, int iInternalFormat, bool filter, float *borderColor, bool immutable);
void GL_CreateTextureColorFormat(int texid, int w, int h, int iInternalFormat, bool filter, float *borderColor, bool immutable);

GLuint GL_GenTextureArrayColorFormat(int w, int h, int depth, int iInternalFormat, bool filter, float *borderColor, bool immutable);
void GL_CreateTextureArrayColorFormat(int texid, int w, int h, int depth, int iInternalFormat, bool filter, float *borderColor, bool immutable);

GLuint GL_GenShadowTexture(int w, int h, float *borderColor, bool immutable);
void GL_CreateShadowTexture(int texid, int w, int h, float *borderColor, bool immutable);

void GL_SetCurrentSceneFBO(FBO_Container_t* src);
FBO_Container_t* GL_GetCurrentSceneFBO();
FBO_Container_t* GL_GetCurrentRenderingFBO();

void GL_BindFrameBuffer(FBO_Container_t *fbo);
void GL_BindFrameBufferWithTextures(FBO_Container_t *fbo, GLuint color, GLuint depth, GLuint depth_stencil, GLsizei width, GLsizei height);

void GL_GenFrameBuffer(FBO_Container_t *s);
void GL_FrameBufferColorTexture(FBO_Container_t *s, GLuint iInternalFormat);
void GL_FrameBufferDepthTexture(FBO_Container_t *s, GLuint iInternalFormat);
void GL_FrameBufferColorTextureHBAO(FBO_Container_t *s);
void GL_FrameBufferColorTextureDeferred(
	FBO_Container_t *s, 
	GLuint iInternalColorFormat, 
	GLuint iInternalColorFormat2,
	GLuint iInternalColorFormat3,
	GLuint iInternalColorFormat4);
void GL_FrameBufferColorTextureOITBlend(FBO_Container_t *s);

gltexture_t *GL_LoadTextureEx(const char* identifier, GL_TEXTURETYPE textureType, gl_loadtexture_context_t* context);
bool R_LoadTextureFromFile(const char* filename, const char* identifier, GL_TEXTURETYPE textureType, bool mipmap, gl_loadtexture_result_t* result);
int R_LoadRGBA8TextureFromMemory(const char* identifier, const void* data, int width, int height, GL_TEXTURETYPE type, bool mipmap);

bool LoadDDS(const char* filename, const char* pathId, gl_loadtexture_context_t* context);
bool LoadImageGenericMemoryIO(const char* identifier, byte *data, size_t dataSize, gl_loadtexture_context_t* context);
bool LoadImageGenericFileIO(const char* filename, const char* pathId, gl_loadtexture_context_t* context);
bool SaveImageGenericRGB8(const char *filename, const char* pathId, int width, int height, const void *data);
bool SaveImageGenericRGBA8(const char* filename, const char* pathId, int width, int height, const void *data);

cubemap_t *R_FindCubemap(float *origin);
void R_LoadCubemap(cubemap_t *cubemap);
void R_BuildCubemaps_f(void);

void R_SaveProgramStates_f(void);
void R_LoadProgramStates_f(void);

void R_LoadLegacyOpenGLMatrixForViewModel();
void R_LoadLegacyOpenGLMatrixForWorld();

void R_Reload_f(void);
void R_DumpTextures_f(void);

void COM_FileBase(const char *in, char *out);

//Framebuffer
void GL_PushFrameBuffer(void);
void GL_PopFrameBuffer(void);

bool R_IsRenderingGBuffer();
bool R_IsRenderingGammaBlending();
bool R_IsRenderingShadowView(void);
bool R_IsRenderingWaterView(void);
bool R_IsRenderingReflectView(void);
bool R_IsRenderingRefractView(void);
bool R_IsLowerBodyEntity(cl_entity_t* ent);
bool R_IsRenderingLowerBody(void);
bool R_IsRenderingClippedLowerBody(void);
bool R_IsRenderingPortal(void);
bool R_IsRenderingFirstPersonView();
bool R_IsRenderingPreViewModel();
bool R_IsRenderingViewModel();
bool R_IsRenderingFlippedViewModel(void);

bool R_IsDeferredRenderingEnabled(void);

//Fog
bool R_IsRenderingFog();
void R_DisableRenderingFog();
void R_InhibitRenderingFog();
void R_RestoreRenderingFog();
void R_RenderWaterFog(void);
void R_RenderSvenFog(void);
void R_RenderUserFog(void);

//refdef
void R_PushRefDef(void);
void R_UpdateRefDef(void);
void R_PopRefDef(void);

void GL_FreeTextureEntry(gltexture_t *glt);
void GL_PushMatrix(void);
void GL_PopMatrix(void);

void GL_PushDrawState(void);
void GL_PopDrawState(void);

void GL_Begin2D(void);
void GL_Begin2DEx(int width, int height);
void GL_End2D(void);

void GL_ClearColor(vec4_t color);
void GL_ClearDepthStencil(float depth, int stencilref, int stencilmask);
void GL_ClearColorDepthStencil(vec4_t color, float depth, int stencilref, int stencilmask);
void GL_ClearStencil(int stencilmask);

void GL_BeginStencilCompareEqual(int ref, int mask);
void GL_BeginStencilCompareNotEqual(int ref, int mask);
void GL_BeginStencilWrite(int ref, int write_mask);
void GL_BeginStencilCompareEqualWrite(int ref, int compare_mask, int write_mask);
void GL_BeginStencilCompareNotEqualWrite(int ref, int compare_mask, int write_mask);
void GL_EndStencil();

void GL_BeginFullScreenQuad(bool enableDepthTest);
void GL_EndFullScreenQuad(void);

void GL_Texturemode_f(void);
void GL_Texturemode_cb(cvar_t *);

void GL_GenerateHashedTextureIndentifier(const char* identifier, GL_TEXTURETYPE textureType, char* hashedIdentifier, size_t len);
void GL_GenerateHashedTextureIndentifier2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, char* hashedIdentifier, size_t len);
void GL_GenerateHashedTextureIndentifier3(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, int numframes, char* hashedIdentifier, size_t len);

GL_TEXTURETYPE GL_GetTextureTypeFromTextureIdentifier(const char* identifier);
int GL_GetTextureTargetFromTextureEntry(const gltexture_t* glt);
int GL_GetTextureTargetFromTextureIdentifier(const char* identifier);

int EngineGetMaxGLTextures();
int EngineGetNumKnownModel();
int EngineGetMaxKnownModel(void);
int EngineGetModelIndex(model_t *mod);
model_t *EngineGetModelByIndex(int index);
int EngineGetMaxDLights(void);
int EngineGetMaxELights(void);
int EngineGetMaxClientModels(void); 
int EngineGetMaxLightmapTextures(void);
int EngineGetMaxClientEdicts(void);
cl_entity_t *EngineGetClientEntitiesBase(void);
int EngineGetMaxTempEnts(void);
TEMPENTITY *EngineGetTempTentsBase(void);
TEMPENTITY *EngineGetTempTentByIndex(int index);
int EngineGetMaxVisEdicts(void);

int EngineFindPhysEntIndexByEntity(cl_entity_t* ent);

bool EngineIsEntityInVisibleList(cl_entity_t* ent);

float GetFrameRateFromFrameDuration(int frameduration);

int _cdecl SDL_GL_SetAttribute(int attr, int value);

void R_EmitFlashlights();
void R_CreateLowerBodyModel();

extern float r_viewport[4];
extern float r_entity_matrix[4][4];
extern float r_entity_color[4];

extern bool r_draw_analyzingstudio;
extern bool r_draw_deferredtrans;
extern bool r_draw_hasalpha;
extern bool r_draw_hasadditive;
extern bool r_draw_hasface;
extern bool r_draw_hashair;
extern bool r_draw_hasoutline;
extern bool r_draw_shadowcaster;
extern bool r_draw_shadowscene;
extern bool r_draw_opaque;
extern bool r_draw_oitblend;
extern bool r_draw_gammablend;
extern bool r_draw_legacysprite;
extern bool r_draw_reflectview;
extern bool r_draw_refractview;
extern bool r_draw_portalview;

extern int r_renderview_pass;

extern bool g_bIsSvenCoop;
extern bool g_bIsCounterStrike;
extern bool g_bIsAoMDC;

#define BUFFER_OFFSET(i) ((unsigned int *)NULL + (i))
