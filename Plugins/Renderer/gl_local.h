#pragma once

#include <metahook.h>
#include <math.h>
#include <assert.h>
#include <mathlib.h>
#include <archtypes.h>
#include <const.h>
#include <custom.h>
#include <ref_params.h>
#include <cvardef.h>
#include <studio.h>
#include <r_studioint.h>
#include <pm_movevars.h>

#include "plugins.h"

#include "exportfuncs.h"

#include "bspfile.h"
#include "qgl.h"

#include "gl_const.h"
#include "gl_shader.h"
#include "gl_model2.h"
#include "gl_model.h"
#include "enginedef.h"

#include "gl_util.h"
#include "gl_water.h"
#include "gl_studio.h"
#include "gl_hud.h"
#include "gl_shadow.h"
#include "gl_wsurf.h"
#include "gl_draw.h"
#include "gl_3dsky.h"
#include "gl_cloak.h"

#include "ref_int_internal.h"

extern refdef_t *r_refdef;
extern ref_params_t r_params;

extern cl_entity_t *r_worldentity;
extern model_t *r_worldmodel;
extern int *cl_numvisedicts;
extern int cl_maxvisedicts;
extern cl_entity_t **cl_visedicts;
extern cl_entity_t **currententity;
extern int *maxTransObjs;
extern int *numTransObjs;
extern transObjRef **transObjects;

extern RECT *window_rect;

extern float *videowindowaspect;
extern float *windowvideoaspect;
extern float videowindowaspect_old;
extern float windowvideoaspect_old;

extern GLuint drawframebuffer;
extern GLuint readframebuffer;

extern float scr_fov_value;
extern mplane_t *frustum;
extern mleaf_t **r_viewleaf, **r_oldviewleaf;
extern texture_t *r_notexture_mip;

extern int mirrortexturenum;
extern qboolean mirror;
extern mplane_t *mirror_plane;

extern float yfov;
extern float screenaspect;

extern vec_t *vup;
extern vec_t *vpn;
extern vec_t *vright;
extern vec_t *r_origin;
extern vec_t *modelorg;
extern vec_t *r_entorigin;

extern int *r_framecount;
extern int *r_visframecount;

extern frame_t *cl_frames;
extern int size_of_frame;
extern int *cl_parsecount;
extern int *cl_waterlevel;
extern double *cl_time;
extern double *cl_oldtime;

//gl extension
extern qboolean gl_framebuffer_object;
extern qboolean gl_shader_support;
extern qboolean gl_program_support;
extern qboolean gl_msaa_support;
extern qboolean gl_msaa_blit_support;
extern qboolean gl_csaa_support;
extern qboolean gl_float_buffer_support;
extern qboolean gl_s3tc_compression_support;

extern int gl_mtexable;
extern int gl_max_texture_size;
extern float gl_max_ansio;
extern float gl_force_ansio;
extern int gl_msaa_samples;
extern int gl_csaa_samples;

extern int *gl_msaa_fbo;
extern int *gl_backbuffer_fbo;

extern int glwidth;
extern int glheight;

extern qboolean bDoMSAAFBO;
extern qboolean bDoScaledFBO;
extern qboolean bDoDirectBlit;
extern qboolean bDoHDR;
extern qboolean bNoStretchAspect;

extern FBO_Container_t s_MSAAFBO;
extern FBO_Container_t s_BackBufferFBO;
extern FBO_Container_t s_BackBufferFBO2;
extern FBO_Container_t s_DownSampleFBO[DOWNSAMPLE_BUFFERS];
extern FBO_Container_t s_LuminFBO[LUMIN_BUFFERS];
extern FBO_Container_t s_Lumin1x1FBO[LUMIN1x1_BUFFERS];
extern FBO_Container_t s_BrightPassFBO;
extern FBO_Container_t s_BlurPassFBO[BLUR_BUFFERS][2];
extern FBO_Container_t s_BrightAccumFBO;
extern FBO_Container_t s_ToneMapFBO;
extern FBO_Container_t s_DepthLinearFBO;
extern FBO_Container_t s_HBAOCalcFBO;
extern FBO_Container_t s_CloakFBO;

extern int skytexturenum;

extern msurface_t *skychain;
extern msurface_t *waterchain;

extern int *gSkyTexNumber;
extern skybox_t *skymins;
extern skybox_t *skymaxs;

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
extern cvar_t *r_novis;
extern cvar_t *r_mmx;
extern cvar_t *r_traceglow;
extern cvar_t *r_wadtextures;
extern cvar_t *r_glowshellfreq;
extern cvar_t *r_detailtextures;

extern cvar_t *gl_vsync;
extern cvar_t *gl_ztrick;
extern cvar_t *gl_finish;
extern cvar_t *gl_clear;
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
extern cvar_t *gl_round_down;
extern cvar_t *gl_picmip;
extern cvar_t *gl_max_size;

extern cvar_t *v_lightgamma;
extern cvar_t *v_brightness;
extern cvar_t *v_gamma;
extern cvar_t *cl_righthand;

void R_FillAddress(void);
void R_InstallHook(void);
void R_RenderView(void);
void R_RenderScene(void);
void R_RenderView_SvEngine(int a1);
qboolean R_CullBox(vec3_t mins, vec3_t maxs);
void R_RotateForEntity(vec_t *origin, cl_entity_t *e);
void R_Clear(void);
void R_ForceCVars(qboolean mp);
void R_UploadLightmaps(void);
void R_NewMap(void);
void R_Init(void);
void R_VidInit(void);
void R_Shutdown(void);
void R_InitTextures(void);
void R_FreeTextures(void);
void R_InitShaders(void);
void R_FreeShaders(void);
void R_SetupFrame(void);
void R_SetFrustum(void);
void R_SetupGL(void);
void R_MarkLeaves(void);
void R_SetFrustum(void);
void R_CalcRefdef(struct ref_params_s *pparams);
void R_DrawWorld(void);
void R_DrawWaterSurfaces(void);
void R_DrawSkyChain(msurface_t *s);
void R_ClearSkyBox(void);
void R_DrawSkyBox(void);
void R_DrawEntitiesOnList(void);
void R_DrawSequentialPoly(msurface_t *s, int face);
float *R_GetAttachmentPoint(int entity, int attachment);
void R_DrawBrushModel(cl_entity_t *entity);
void R_DrawSpriteModel(cl_entity_t *entity);
void R_GetSpriteAxes(cl_entity_t *entity, int type, float *vforwrad, float *vright, float *vup);
void R_SpriteColor(mcolor24_t *col, cl_entity_t *entity, int renderamt);
float GlowBlend(cl_entity_t *entity);
int CL_FxBlend(cl_entity_t *entity);
void R_RenderCurrentEntity(void);
void R_DrawTEntitiesOnList(int onlyClientDraw);
void R_AllocObjects(int nMax);
void R_AddTEntity(cl_entity_t *pEnt);
void R_SortTEntities(void);

void GL_Init(void);
void GL_BeginRendering(int *x, int *y, int *width, int *height);
void GL_EndRendering(void);
void GL_BuildLightmaps(void);
void GL_InitExtensions(void);
bool GL_Support(int r_ext);
void GL_SetDefaultState(void);
GLuint GL_GenTexture(void);
void GL_DeleteTexture(GLuint tex);
void GL_Bind(int texnum);
void GL_SelectTexture(GLenum target);
void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);
int GL_LoadTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal);
int GL_LoadTexture2(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter);
texture_t *Draw_DecalTexture(int index);
void Draw_MiptexTexture(cachewad_t *wad, byte *data);

void Draw_UpdateAnsios(void);
void Draw_Init(void);

void EmitWaterPolys(msurface_t *fa, int direction);
void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
float CalcFov(float fov_x, float width, float height);
int SignbitsForPlane(mplane_t *out);
void BuildSurfaceDisplayList(msurface_t *fa);

refdef_t *R_GetRefDef(void);
int R_GetDrawPass(void);
GLuint R_GLGenTextureRGBA8(int w, int h);

void R_GLUploadDepthTexture(int texid, int w, int h);
GLuint R_GLGenDepthTexture(int w, int h);

GLuint R_GLGenTextureColorFormat(int w, int h, int iInternalFormat);
void R_GLUploadTextureColorFormat(int texid, int w, int h, int iInternalFormat);

GLuint R_GLGenShadowTexture(int w, int h);
void R_GLUploadShadowTexture(int texid, int w, int h);

byte *R_GetTexLoaderBuffer(int *bufsize);
gltexture_t *R_GetCurrentGLTexture(void);
int GL_LoadTextureEx(const char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, qboolean ansio);
int R_LoadTextureEx(const char *filepath, const char *name, int *width, int *height, GL_TEXTURETYPE type, qboolean mipmap, qboolean ansio);

void GL_UploadDXT(byte *data, int width, int height, qboolean mipmap, qboolean ansio);
int LoadDDS(const char *filename, byte *buf, int bufSize, int *width, int *height);
int LoadImageGeneric(const char *filename, byte *buf, int bufSize, int *width, int *height);
int SaveImageGeneric(const char *filename, int width, int height, byte *data);

//framebuffer
void R_PushFrameBuffer(void);
void R_PopFrameBuffer(void);
void R_GLBindFrameBuffer(GLenum target, GLuint framebuffer);

//refdef
void R_PushRefDef(void);
void R_UpdateRefDef(void);
void R_PopRefDef(void);
float *R_GetSavedViewOrg(void);
int R_GetDrawPass(void);
int R_GetSupportExtension(void);
//void R_LoadRendererEntities(void);
void GL_FreeTexture(gltexture_t *glt);
void R_InitRefHUD(void);
void R_PushMatrix(void);
void R_PopMatrix(void);

//for screenshot
byte *R_GetSCRCaptureBuffer(int *bufsize);
void CL_ScreenShot_f(void);

//player state for StudioDrawPlayer
entity_state_t *R_GetPlayerState(int index);
entity_state_t *R_GetCurrentDrawPlayerState(int parsecount);

extern vec3_t save_vieworg[MAX_SAVEREFDEF_STACK];
extern vec3_t save_viewang[MAX_SAVEREFDEF_STACK];
extern int save_refdefstack;

extern double g_flFrameTime;
extern int last_luminance;