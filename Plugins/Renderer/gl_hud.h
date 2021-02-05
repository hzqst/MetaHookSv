#pragma once

#include "enginedef.h"

#define MAX_GAUSSIAN_SAMPLES 16
#define LUMIN1x1_BUFFERS 3
#define DOWNSAMPLE_BUFFERS 2
#define LUMIN_BUFFERS 3
#define BLUR_BUFFERS 3

typedef struct
{
	int program;
	int base;
	int src_col;
}hud_drawhudmask_program_t;

typedef struct
{
	int program;
	int base;
	int alpha_range;
	int offset;
}hud_drawcolormask_program_t;

typedef struct
{
	int program;
	int base;
	int center;
	int radius;
	int blurdist;
}hud_drawroundrect_program_t;

typedef struct
{
	int program;
	int tex0;
	int rt_w;
	int rt_h;
}pp_fxaa_program_t;

typedef struct
{
	int program;
	int tex;
}pp_downsample_program_t;

typedef struct
{
	int program;
	int texelsize;
	int tex;
}pp_downsample2x2_program_t;

typedef struct
{
	int program;
	int texelsize;
	int tex;
}pp_lumin_program_t;

typedef struct
{
	int program;
	int texelsize;
	int tex;
}pp_luminlog_program_t;

typedef struct
{
	int program;
	int texelsize;
	int tex;
}pp_luminexp_program_t;

typedef struct
{
	int program;
	int curtex;
	int adatex;
	int frametime;
}pp_luminadapt_program_t;

typedef struct
{
	int program;
	int tex;
	int lumtex;
}pp_brightpass_program_t;

typedef struct
{
	int program;
	int tex;
	int du;
}pp_gaussianblurv_program_t, pp_gaussianblurh_program_t;

typedef struct
{
	int program;
	int basetex;
	int blurtex;
	int lumtex;
	int blurfactor;
	int exposure;
	int darkness;
	int gamma;
}pp_tonemap_program_t;

typedef struct
{
	int program;
}depth_linearize_program_t;

typedef struct
{
	int program;
}depth_linearize_msaa_program_t;

typedef struct
{
	int program;
	int texLinearDepth;
	int texRandom;
	int control_RadiusToScreen;
	int control_projOrtho;
	int control_projInfo;
	int control_PowExponent;
	int control_InvQuarterResolution;
	int control_AOMultiplier;
	int control_InvFullResolution;
	int control_NDotVBias;
	int control_NegInvR2;
}hbao_calc_blur_program_t;

typedef struct
{
	int program;
	
}hbao_blur_program_t;

typedef struct
{
	int program;

}hbao_blur2_program_t;

extern qboolean drawhudinworld;
extern qboolean draw3dhud;
extern int pp_fxaa_program;
extern cvar_t *r_hdr;
extern cvar_t *r_hdr_debug;
extern cvar_t *r_hudinworld_debug;
extern cvar_t *r_ssao;
extern cvar_t *r_ssao_debug;
extern cvar_t *r_ssao_radius;
extern cvar_t *r_ssao_intensity;
extern cvar_t *r_ssao_bias;
extern cvar_t *r_ssao_blur_sharpness;

extern int r_hudinworld_texture;

void R_InitBlur(const char *vs_code, int samples);
void R_BeginHUDQuad(void);

void R_BeginFXAA(int w, int h);
void R_BeginDrawRoundRect(int centerX, int centerY, float radius, float blurdist);
void R_BeginDrawHudMask(int r, int g, int b);

int R_Get3DHUDTexture(void);
void R_Draw3DHUDQuad(int x, int y, int left, int top);
void R_BeginDrawTrianglesInHUD_Direct(int x, int y);
void R_BeginDrawTrianglesInHUD_FBO(int x, int y, int left, int top);
void R_FinishDrawTrianglesInHUD(void);
void R_BeginDrawHUDInWorld(int texid, int w, int h);
void R_FinishDrawHUDInWorld(void);
int R_DoSSAO(int sampleIndex);
void R_DoHDR(void);
void R_DownSample(FBO_Container_t *src, FBO_Container_t *dst, qboolean filter2x2);
void R_LuminPass(FBO_Container_t *src, FBO_Container_t *dst, int logexp);
void R_LuminAdaptation(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *ada, double frametime);
void R_BrightPass(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *lum);
void R_BlurPass(FBO_Container_t *src, FBO_Container_t *dst, qboolean vertical);
void R_BrightAccum(FBO_Container_t *blur1, FBO_Container_t *blur2, FBO_Container_t *blur3, FBO_Container_t *dst);
void R_ToneMapping(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *blur, FBO_Container_t *lum);
void R_BlitToScreen(FBO_Container_t *src);
void R_BlitToFBO(FBO_Container_t *src, FBO_Container_t *dst);
void R_DrawHUDQuad_Texture(int tex, int w, int h);
void GLBeginHud(void);