#pragma once

#include "gl_cvar.h"
#include "enginedef.h"

#define LUMIN1x1_BUFFERS 3
#define DOWNSAMPLE_BUFFERS 2
#define LUMIN_BUFFERS 3
#define BLUR_BUFFERS 3

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
}pp_downsample_program_t;

typedef struct
{
	int program;
	int texelsize;
}pp_downsample2x2_program_t;

typedef struct
{
	int program;
	int texelsize;
}pp_lumindown_program_t;

typedef struct
{
	int program;
	int texelsize;
}pp_luminlog_program_t;

typedef struct
{
	int program;
	int texelsize;
}pp_luminexp_program_t;

typedef struct
{
	int program;
	int frametime;
}pp_luminadapt_program_t;

typedef struct
{
	int program;
	int baseTex;
	int lumTex;
}pp_brightpass_program_t;

typedef struct
{
	int program;
}pp_gaussianblurv_program_t, pp_gaussianblurh_program_t;

typedef struct
{
	int program;
	int baseTex;
	int blurTex;
	int lumTex;
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
}depth_clear_program_t;

typedef struct
{
	int program;
}oitbuffer_clear_program_t;

typedef struct
{
	int program;
}blit_oitblend_program_t;

typedef struct
{
	int program;
}gamma_correction_program_t;

typedef struct
{
	int program;
}gamma_uncorrection_program_t;

typedef struct
{
	int program;
}under_water_effect_program_t;

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

	int control_Fog;
}hbao_calc_blur_program_t, hbao_calc_blur_fog_program_t;

typedef struct
{
	int program;	
}hbao_blur_program_t, hbao_blur2_program_t;

typedef struct
{
	int program;
	int basetex;
	int layer;
}hud_debug_program_t;

extern cvar_t *r_hdr;
extern cvar_t *r_hdr_debug;
extern MapConVar *r_hdr_blurwidth;
extern MapConVar *r_hdr_exposure;
extern MapConVar *r_hdr_darkness;
extern MapConVar *r_hdr_adaptation;
extern cvar_t *r_ssao;
extern cvar_t *r_ssao_debug;
extern MapConVar *r_ssao_radius;
extern MapConVar *r_ssao_intensity;
extern MapConVar *r_ssao_bias;
extern MapConVar *r_ssao_blur_sharpness;
extern cvar_t *r_fxaa;

extern int last_luminance;

extern SHADER_DEFINE(depth_clear);
extern SHADER_DEFINE(oitbuffer_clear);
extern SHADER_DEFINE(blit_oitblend);

void R_BlendFinalBuffer(void);
void R_BlendOITBuffer(void);
void R_ClearOITBuffer(void);
void R_LinearizeDepth(FBO_Container_t *src);
void R_AmbientOcclusion(void);
bool R_IsGammaBlendEnabled();
void R_GammaCorrection(void);
void R_GammaUncorrection(void);
void R_GammaUncorrectionEx(FBO_Container_t* src, FBO_Container_t* dst);
bool R_IsAmbientOcclusionEnabled(void);
void R_HDR(void);
bool R_IsHDREnabled(void);
void R_FXAA(void);
bool R_IsFXAAEnabled(void);
void R_UnderWaterEffect(void);
bool R_IsUnderWaterEffectEnabled(void);
void GL_BlitFrameFufferToScreen(FBO_Container_t *src);
void GL_BlitFrameBufferToFrameBufferColorOnly(FBO_Container_t *src, FBO_Container_t *dst);
void GL_BlitFrameBufferToFrameBufferColorDepth(FBO_Container_t *src, FBO_Container_t *dst);
void GL_BlitFrameBufferToFrameBufferColorDepthStencil(FBO_Container_t* src, FBO_Container_t* dst);
void GL_BlitFrameBufferToFrameBufferStencilOnly(FBO_Container_t* src, FBO_Container_t* dst);
void GL_BlitFrameBufferToFrameBufferDepthStencil(FBO_Container_t* src, FBO_Container_t* dst);
void R_DrawHUDQuad(int w, int h);
void R_DrawHUDQuad_Texture(int tex, int w, int h);
void R_ShutdownPostProcess(void);
void R_InitPostProcess(void);

#define HUD_DEBUG_TEXARRAY 1
#define HUD_DEBUG_SHADOW 2

void R_UseHudDebugProgram(program_state_t state, hud_debug_program_t *progOutput);