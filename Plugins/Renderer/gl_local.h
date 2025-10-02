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

#include <IEngineSurface.h>
#include <IVideoMode.h>

#include <set>
#include <map>
#include <atomic>
#include <memory>
#include <sstream>

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
#include "gl_ringbuffer.h"

#define MAX_SAVESTACK 16

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

extern GLuint r_empty_vao;

extern vec4_t g_GLColor;

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

extern bool r_fog_enabled;
extern int r_fog_mode;
extern float r_fog_control[3];
extern float r_fog_color[4];

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

extern EngineSurfaceVertexBuffer_t(*g_VertexBuffer)[MAXVERTEXBUFFERS];;
extern int(*g_iVertexBufferEntriesUsed);

extern CPMBRingBuffer g_TexturedRectVertexBuffer;
extern CPMBRingBuffer g_FilledRectVertexBuffer;
extern CPMBRingBuffer g_RectInstanceBuffer;
extern CPMBRingBuffer g_RectIndexBuffer;

extern std::vector<cl_entity_t*> g_PostProcessGlowStencilEntities;
extern std::vector<cl_entity_t*> g_PostProcessGlowColorEntities;

extern RECT* g_ScissorRect;
extern bool* g_bScissor;

extern IEngineSurface* engineSurface;
extern IEngineSurface_HL25* engineSurface_HL25;

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

extern qboolean* giScissorTest;
extern int* scissor_x;
extern int* scissor_y;
extern int* scissor_width;
extern int* scissor_height;

extern screenfade_t* cl_sf;

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

extern void** (*pmainwindow);

extern float* vid_d3d;

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

extern cvar_t* r_studio_parallel_load;

extern cvar_t* r_wsurf_parallax_scale;

extern cvar_t* r_wsurf_sky_fog;

extern cvar_t* gl_nearplane;

extern cvar_t* r_glow_bloomscale;

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
void __fastcall CVideoMode_Common_DrawStartupGraphic(void* videomode, int dummy, void* window);
void __fastcall CGame_DrawStartupVideo(void* pgame, int dummy, const char *filename, void* window);
void DT_Initialize();
void R_Init(void);
void R_Shutdown(void);
void R_SetupGL(void);
void R_SetupGLForViewModel(void);
void R_MarkLeaves(void);
void R_PrepareDrawWorld(void); 
void R_SetupShadowMatrix(float out[4][4], const float worldMatrix[4][4], const float projMatrix[4][4]);
void R_DrawWorld(void);
void R_DrawSkyBox(void);
void R_CheckVariables(void);
void R_AnimateLight(void);
void R_SetupFrame(void);
void R_SetFrustum(void);
void R_SetFrustum(float xfov, float yfov, float right, float top);
void R_UploadSceneUBO(void);
void R_UploadCameraUBO(void);
void R_SetupCameraView(camera_view_t* view);
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
void GL_Shutdown(void* window, HDC pmaindc, HGLRC pbaseRC);
void GL_Init(void);
qboolean GL_SetMode(void* window, HDC* pmaindc, HGLRC* pbaseRC);
qboolean GL_SetModeLegacy(void* window, HDC* pmaindc, HGLRC* pbaseRC, int fD3D, const char* pszDriver, const char* pszCmdLine);
qboolean GL_SelectPixelFormat(HDC hDC);
void GL_Set2D();
void GL_Finish2D();
void GL_Set2DEx(int x, int y, int width, int height);
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
void GL_UploadDataToUBODynamicDraw(GLuint VBO, size_t size, const void* data);
void GL_UploadSubDataToUBO(GLuint UBO, size_t offset, size_t size, const void* data);
void GL_UploadDataToVBOStaticDraw(GLuint VBO, size_t size, const void* data);
void GL_UploadDataToVBODynamicDraw(GLuint VBO, size_t size, const void* data);
void GL_UploadDataToVBOStreamDraw(GLuint VBO, size_t size, const void* data);
void GL_UploadDataToVBOStreamMap(GLuint VBO, size_t size, const void* data);
void GL_UploadSubDataToVBO(GLuint VBO, size_t offset, size_t size, const void* data);
void GL_UploadDataToEBOStaticDraw(GLuint EBO, size_t size, const void* data);
void GL_UploadDataToEBODynamicDraw(GLuint EBO, size_t size, const void* data);
void GL_UploadDataToEBOStreamDraw(GLuint EBO, size_t size, const void* data);
void GL_UploadDataToEBOStreamMap(GLuint EBO, size_t size, const void* data);
void GL_UploadSubDataToEBO(GLuint EBO, size_t offset, size_t size, const void* data);
void GL_UploadDataToABOStaticDraw(GLuint ABO, size_t size, const void* data);
void GL_UploadDataToABODynamicDraw(GLuint ABO, size_t size, const void* data);
void GL_UploadDataToSSBOStaticDraw(GLuint SSBO, size_t size, const void* data);
void GL_BindStatesForVAO(GLuint VAO, const std::function<void()>& bind);
void GL_BindStatesForVAO(GLuint VAO, GLuint VBO, GLuint EBO, const std::function<void()>& bind);
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

float* R_GetWorldMatrix();
void R_PushWorldMatrix();
void R_PopWorldMatrix();
void R_LoadIdentityForWorldMatrix();
void R_TranslateWorldMatrix(float x, float y, float z);
void R_SetupPlayerViewWorldMatrix(const vec3_t origin, const vec3_t viewangles);

float* R_GetProjectionMatrix();
void R_PushProjectionMatrix();
void R_PopProjectionMatrix();
void R_LoadIdentityForProjectionMatrix();
void R_SetupOrthoProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar, bool NegativeOneToOneZ);
void R_SetupFrustumProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar);
void R_SetupFrustumProjectionMatrixReversedZ(float left, float right, float bottom, float top, float zNear, float zFar);
void R_SetupPerspective(float fovx, float fovy, float zNear, float zFar);
void R_SetupPerspectiveReversedZ(float fovx, float fovy, float zNear, float zFar);

void GL_BeginDebugGroup(const char* name);
void GL_BeginDebugGroupFormat(const char* fmt, ...);
void GL_EndDebugGroup();
void GL_SetTextureDebugName(GLuint textureId, const char* name);
void GL_SetTextureDebugNameFormat(GLuint textureId, const char* fmt, ...);
void GL_SetFrameBufferDebugName(GLuint framebufferId, const char* name);
void GL_SetFrameBufferDebugNameFormat(GLuint framebufferId, const char* fmt, ...);

void __stdcall triapi_glBegin(int GLPrimitiveCode);
void __stdcall triapi_glEnd();
void __stdcall CoreProfile_glColor4f(float r, float g, float b, float a);
void __stdcall CoreProfile_glColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
HGLRC __stdcall CoreProfile_qwglCreateContext(HDC hDC);
const GLubyte* __stdcall CoreProfile_glGetString(GLenum e);
void __stdcall CoreProfile_glAlphaFunc(GLenum func, GLclampf ref);
void __stdcall CoreProfile_glEnable(GLenum cap);
void __stdcall CoreProfile_glDisable(GLenum cap);
void __stdcall CoreProfile_glShadeModel(GLenum mode);
void __stdcall CoreProfile_glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void __stdcall CoreProfile_glTexParameterf(GLenum target, GLenum pname, GLfloat param);
GLboolean __stdcall CoreProfile_glIsEnabled(GLenum cap);
void __stdcall CoreProfile_glBegin(int GLPrimitiveCode);
void __stdcall CoreProfile_glGenTextures(GLsizei n, GLuint* textures);
void* __cdecl CoreProfile_SDL_GL_GetProcAddress(const char* proc);
void* __stdcall CoreProfile_GetProcAddress(HMODULE hModule, const char* proc);
int __cdecl CoreProfile_GL_SetAttribute(int attr, int value);
void* __cdecl CoreProfile_SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags);
int __cdecl CoreProfile_SDL_GL_ExtensionSupported(const char* extension);

void GL_UnloadTextureByIdentifier(const char* identifier);
void GL_UnloadTextures(void);
void GL_LoadFilterTexture(void);
int GL_LoadTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal);
int GL_LoadTexture2(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter);
void GL_InitShaders(void);
void GL_FreeShaders(void);
texture_t *Draw_DecalTexture(int index);
void Draw_MiptexTexture(cachewad_t *wad, byte *data);
mbasenode_t* PVSNode(mbasenode_t* basenode, vec3_t emins, vec3_t emaxs);
void R_DecalShootInternal(texture_t *ptexture, int index, int entity, int modelIndex, vec3_t position, int flags, float flScale);

void staticFreeTextureId(int id);
void __fastcall enginesurface_pushMakeCurrent(void* pthis, int, int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace);
void __fastcall enginesurface_popMakeCurrent(void* pthis, int);
void __fastcall enginesurface_drawFilledRect(void* pthis, int, int x0, int y0, int x1, int y1);
void __fastcall enginesurface_drawOutlinedRect(void* pthis, int, int x0, int y0, int x1, int y1);
void __fastcall enginesurface_drawLine(void* pthis, int, int x0, int y0, int x1, int y1);
void __fastcall enginesurface_drawPolyLine(void* pthis, int, int* px, int* py, int numPoints);
void __fastcall enginesurface_drawSetTextureRGBA(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, qboolean hasAlphaChannel);
void __fastcall enginesurface_drawSetTexture(void* pthis, int, int textureId);
void __fastcall enginesurface_drawTexturedRect(void* pthis, int, int x0, int y0, int x1, int y1);
void __fastcall enginesurface_drawTexturedRectAdd(void* pthis, int, int x0, int y0, int x1, int y1);
void __fastcall enginesurface_drawPrintCharAdd(void* pthis, int, int x, int y, int wide, int tall, float s0, float t0, float s1, float t1);
void __fastcall enginesurface_drawSetTextureFile(void* pthis, int, int textureId, const char* filename, qboolean hardwareFilter, bool forceReload);
int __fastcall enginesurface_createNewTextureID(void* pthis, int);
void __fastcall enginesurface_drawGetTextureSize(void* pthis, int, int textureId, int& wide, int& tall);
bool __fastcall enginesurface_isTextureIDValid(void* pthis, int, int);
void __fastcall enginesurface_drawSetSubTextureRGBA(void* pthis, int, int textureID, int drawX, int drawY, const unsigned char* rgba, int subTextureWide, int subTextureTall);
void __fastcall enginesurface_drawFlushText(void* pthis, int dummy);
void __fastcall enginesurface_drawSetTextureBGRA(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, bool forceUpload);
void __fastcall enginesurface_drawUpdateRegionTextureBGRA(void* pthis, int, int textureID, int x, int y, const unsigned char* pchData, int wide, int tall);

void Draw_Frame(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect);
void Draw_SpriteFrameHoles(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect);
void Draw_SpriteFrameHoles_SvEngine(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect);
void Draw_SpriteFrameAdditive(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect);
void Draw_SpriteFrameAdditive_SvEngine(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect);
void Draw_SpriteFrameGeneric(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect, int src, int dest, int width, int height);
void Draw_SpriteFrameGeneric_SvEngine(mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect, int src, int dest, int width, int height);
void Draw_FillRGBA(int x, int y, int w, int h, int r, int g, int b, int a);
void Draw_FillRGBABlend(int x, int y, int w, int h, int r, int g, int b, int a);
void NET_DrawRect(int x, int y, int w, int h, int r, int g, int b, int a);
void Draw_Pic(int x, int y, qpic_t *pic);
void D_FillRect(vrect_t* r, unsigned char* color);

void* Draw_CustomCacheGet(cachewad_t* wad, void* raw, int rawsize, int index);
void* Draw_CacheGet(cachewad_t* wad, int index);
int SignbitsForPlane(mplane_t *out);
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

GLuint GL_GenShadowTexture(int w, int h, bool immutable);
void GL_CreateShadowTexture(int texid, int w, int h, bool immutable);

GLuint GL_GenCubemapShadowTexture(int w, int h, bool immutable);
void GL_CreateCubemapShadowTexture(int texid, int w, int h, bool immutable);

GLuint GL_GenShadowTextureArray(int w, int h, int depth, bool immutable);
void GL_CreateShadowTextureArray(int texid, int w, int h, int depth, bool immutable);

void GL_SetCurrentSceneFBO(FBO_Container_t* src);
FBO_Container_t* GL_GetCurrentSceneFBO();
FBO_Container_t* GL_GetCurrentRenderingFBO();

void GL_BindFrameBuffer(FBO_Container_t *fbo);
void GL_BindFrameBufferWithTextures(FBO_Container_t *fbo, GLuint color, GLuint depth, GLuint depth_stencil, GLsizei width, GLsizei height);

const char* GL_GetFrameBufferName(FBO_Container_t* s);
void GL_GenFrameBuffer(FBO_Container_t* s, const char* szFrameBufferName);
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

void R_Reload_f(void);
void R_DumpTextures_f(void);

void COM_FileBase(const char *in, char *out);

//Framebuffer
void GL_PushFrameBuffer(void);
void GL_PopFrameBuffer(void);

bool R_IsRenderingGBuffer();
bool R_IsRenderingDeferredLightingScene();
bool R_IsRenderingGammaBlending();
bool R_IsRenderingShadowView();
bool R_IsRenderingWaterView();
bool R_IsRenderingReflectView();
bool R_IsRenderingRefractView();
bool R_IsLowerBodyEntity(cl_entity_t* ent);
bool R_IsRenderingLowerBody();
bool R_IsRenderingClippedLowerBody();
bool R_IsRenderingPortal();
bool R_IsRenderingGlowStencil();
bool R_IsRenderingGlowColor();
bool R_IsRenderingFirstPersonView();
bool R_IsRenderingPreViewModel();
bool R_IsRenderingViewModel();
bool R_IsRenderingFlippedViewModel();
bool R_IsViewmodelAttachmentInternal(cl_entity_t* aiment);
bool R_IsViewmodelAttachment(cl_entity_t* ent);

bool R_IsDeferredRenderingEnabled();

void* Sys_GetMainWindow();

//Fog
bool R_CanRenderFog();
bool R_IsRenderingFog();
void R_DisableRenderingFog();
void R_InhibitRenderingFog();
void R_RestoreRenderingFog();
void R_RenderWaterFog();
void R_RenderSvenFog();
void R_RenderUserFog();

//refdef
void R_PushRefDef(void);
void R_UpdateRefDef(void);
void R_PopRefDef(void);

void GL_FreeTextureEntry(gltexture_t *glt);
void GL_PushMatrix(void);
void GL_PopMatrix(void);

void GL_PushDrawState(void);
void GL_PopDrawState(void);

void GL_BindTextureUnit(int textureUnit, int target, int gltexturenum);

void GL_ClearColor(vec4_t color);
void GL_ClearDepth(float depth);
void GL_ClearDepthStencil(float depth, int stencilref, int stencilmask);
void GL_ClearColorDepthStencil(vec4_t color, float depth, int stencilref, int stencilmask);
void GL_ClearStencil(int stencilmask);

void GL_BeginStencilCompareEqual(int ref, int mask);
void GL_BeginStencilCompareNotEqual(int ref, int mask);
void GL_BeginStencilWrite(int ref, int write_mask);
void GL_EndStencil();

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
extern bool r_draw_shadowview;
extern bool r_draw_multiview;
extern bool r_draw_nofrustumcull;
extern bool r_draw_opaque;
extern bool r_draw_oitblend;
extern bool r_draw_gammablend;
extern bool r_draw_reflectview;
extern bool r_draw_refractview;
extern int r_draw_classify;

extern int r_renderview_pass;

extern bool g_bIsSvenCoop;
extern bool g_bIsCounterStrike;
extern bool g_bIsAoMDC;

#define BUFFER_OFFSET(i) ((unsigned int *)NULL + (i))

#define DRAW_CLASSIFY_WORLD				0x1
#define DRAW_CLASSIFY_SKYBOX			0x2
#define DRAW_CLASSIFY_OPAQUE_ENTITIES	0x4
#define DRAW_CLASSIFY_TRANS_ENTITIES	0x8
#define DRAW_CLASSIFY_PARTICLES			0x10
#define DRAW_CLASSIFY_LOCAL_PLAYER		0x20
#define DRAW_CLASSIFY_DECAL				0x40
#define DRAW_CLASSIFY_WATER				0x80
#define DRAW_CLASSIFY_LIGHTMAP			0x100

#define DRAW_CLASSIFY_ALL				(DRAW_CLASSIFY_WORLD |\
										DRAW_CLASSIFY_SKYBOX | \
										DRAW_CLASSIFY_OPAQUE_ENTITIES | \
										DRAW_CLASSIFY_TRANS_ENTITIES |\
										DRAW_CLASSIFY_PARTICLES |\
										DRAW_CLASSIFY_LOCAL_PLAYER |\
										DRAW_CLASSIFY_DECAL | \
										DRAW_CLASSIFY_WATER | \
										DRAW_CLASSIFY_LIGHTMAP\
)