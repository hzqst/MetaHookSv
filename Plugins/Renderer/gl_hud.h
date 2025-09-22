#pragma once

#include "gl_cvar.h"
#include "gl_common.h"
#include "enginedef.h"

#define LUMIN1x1_BUFFERS 3
#define DOWNSAMPLE_BUFFERS 2
#define LUMIN_BUFFERS 3
#define BLUR_BUFFERS 3

typedef struct
{
	int program;
}pp_fxaa_program_t;

typedef struct
{
	int program;
}pp_downsample_program_t;

typedef struct
{
	int program;
}pp_downsample2x2_program_t;

typedef struct
{
	int program;
}pp_lumindown_program_t;

typedef struct
{
	int program;
}pp_luminlog_program_t;

typedef struct
{
	int program;
}pp_luminexp_program_t;

typedef struct
{
	int program;
}pp_luminadapt_program_t;

typedef struct
{
	int program;
}pp_brightpass_program_t;

typedef struct
{
	int program;
}pp_gaussianblurv_program_t, pp_gaussianblurh_program_t;

typedef struct
{
	int program;
}pp_tonemap_program_t;

typedef struct
{
	int program;
}depth_linearize_program_t;

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
}copy_color_program_t;

typedef struct
{
	int program;
}copy_color_halo_add_program_t;

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

typedef struct drawtexturedrect_program_s
{
	int program;
}drawtexturedrect_program_t;

typedef struct drawfilledrect_program_s
{
	int program;
}drawfilledrect_program_t;

extern cvar_t *r_hdr;

extern MapConVar *r_hdr_blurwidth;
extern MapConVar *r_hdr_exposure;
extern MapConVar *r_hdr_darkness;
extern MapConVar *r_hdr_adaptation;

extern cvar_t *r_ssao;
extern MapConVar *r_ssao_radius;
extern MapConVar *r_ssao_intensity;
extern MapConVar *r_ssao_bias;
extern MapConVar *r_ssao_blur_sharpness;
extern cvar_t *r_fxaa;

extern int last_luminance;

extern SHADER_DEFINE(oitbuffer_clear);
extern SHADER_DEFINE(blit_oitblend);

void R_DownSample(FBO_Container_t* src_color, FBO_Container_t* src_stencil, FBO_Container_t* dst, bool bUseFilter2x2, bool bUseStencilFilter);
void R_BlurPass(FBO_Container_t* src, FBO_Container_t* dst, float scale, bool vertical);

void R_CopyColor(FBO_Container_t* src, FBO_Container_t* dst);
void R_CopyColorHaloAdd(FBO_Container_t* src, FBO_Container_t* dst);
void R_BlendOITBuffer(FBO_Container_t* src, FBO_Container_t* dst);
void R_ClearOITBuffer(void);
void R_LinearizeDepth(FBO_Container_t *src, FBO_Container_t* dst);
void R_AmbientOcclusion(FBO_Container_t* src, FBO_Container_t* dst);
bool R_IsGammaBlendEnabled();
void R_GammaCorrection(FBO_Container_t* src, FBO_Container_t* dst);
void R_GammaUncorrection(FBO_Container_t* src, FBO_Container_t* dst);
bool R_IsAmbientOcclusionEnabled(void);
void R_HDR(FBO_Container_t* src_color, FBO_Container_t* src_stencil, FBO_Container_t*);
bool R_IsHDREnabled(void);
void R_FXAA(FBO_Container_t* src, FBO_Container_t* dst);
bool R_IsFXAAEnabled(void);
void R_UnderWaterEffect(FBO_Container_t* src, FBO_Container_t* dst);
bool R_IsUnderWaterEffectEnabled(void);
void GL_BlitFrameFufferToScreen(FBO_Container_t *src);
void GL_BlitFrameBufferToFrameBufferColorOnly(FBO_Container_t *src, FBO_Container_t *dst);
void GL_BlitFrameBufferToFrameBufferColorDepth(FBO_Container_t *src, FBO_Container_t *dst);
void GL_BlitFrameBufferToFrameBufferColorDepthStencil(FBO_Container_t* src, FBO_Container_t* dst);
void GL_BlitFrameBufferToFrameBufferStencilOnly(FBO_Container_t* src, FBO_Container_t* dst);
void GL_BlitFrameBufferToFrameBufferDepthStencil(FBO_Container_t* src, FBO_Container_t* dst);

void R_InitHUD(void);
void R_ShutdownHUD(void);

void R_DrawTexturedRect(int gltexturenum, const texturedrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata);
void R_DrawFilledRect(const filledrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata);

void R_DrawTexturedQuad(int gltexturenum, int x0, int y0, int x1, int y1, const float* color4v, uint64_t programState, const char* debugMetadata);
void R_DrawFilledQuad(int x0, int y0, int x1, int y1, const float* color4v, uint64_t programState, const char* debugMetadata);

#define HUD_DEBUG_TEXARRAY 1
#define HUD_DEBUG_SHADOW 2

void R_UseHudDebugProgram(program_state_t state, hud_debug_program_t *progOutput);

#define DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED 0x1ull
#define DRAW_TEXTURED_RECT_ADDITIVE_BLEND_ENABLED 0x2ull
#define DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED 0x4ull
#define DRAW_TEXTURED_RECT_SCISSOR_ENABLED 0x8ull
#define DRAW_TEXTURED_RECT_ALPHA_TEST_ENABLED 0x10ull

#define DRAW_FILLED_RECT_ALPHA_BLEND_ENABLED 0x1ull
#define DRAW_FILLED_RECT_ADDITIVE_BLEND_ENABLED 0x2ull
#define DRAW_FILLED_RECT_ALPHA_BASED_ADDITIVE_ENABLED 0x4ull
#define DRAW_FILLED_RECT_ZERO_SRC_ALPHA_BLEND_ENABLED 0x8ull
#define DRAW_FILLED_RECT_SCISSOR_ENABLED 0x8ull
#define DRAW_FILLED_RECT_LINE_ENABLED 0x10ull

void R_SaveDrawTexturedRectProgramStates(void);
void R_LoadDrawTexturedRectProgramStates(void);
void R_UseDrawTexturedRectProgram(program_state_t state, drawtexturedrect_program_t* progOutput);

void R_SaveDrawFilledRectProgramStates(void);
void R_LoadDrawFilledRectProgramStates(void);
void R_UseDrawFilledRectProgram(program_state_t state, drawfilledrect_program_t* progOutput);