#include "gl_local.h"
#include <sstream>

r_hdr_control_t r_hdr_control;

//HDR
int last_luminance = 0;

//HBAO
#define AO_RANDOMTEX_SIZE 4
static const int  NUM_MRT = 8;
static const int  HBAO_RANDOM_SIZE = AO_RANDOMTEX_SIZE;
static const int  HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE * HBAO_RANDOM_SIZE;
static const int  MAX_SAMPLES = 16;

GLuint hbao_random = 0;
GLuint hbao_randomview[MAX_SAMPLES] = { 0 };
vec4_t m_hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES] = { 0 };

//FXAA
SHADER_DEFINE(pp_fxaa);

//HDR
SHADER_DEFINE(pp_downsample);
SHADER_DEFINE(pp_downsample2x2);
SHADER_DEFINE(pp_lumin);
SHADER_DEFINE(pp_luminlog);
SHADER_DEFINE(pp_luminexp);
SHADER_DEFINE(pp_luminadapt);
SHADER_DEFINE(pp_brightpass);
SHADER_DEFINE(pp_gaussianblurv);
SHADER_DEFINE(pp_gaussianblurh);
SHADER_DEFINE(pp_tonemap);

//HBAO
SHADER_DEFINE(depth_linearize);
SHADER_DEFINE(depth_linearize_msaa);
SHADER_DEFINE(hbao_calc_blur);
SHADER_DEFINE(hbao_calc_blur_fog);
SHADER_DEFINE(hbao_blur);
SHADER_DEFINE(hbao_blur2);

SHADER_DEFINE(depth_clear);
SHADER_DEFINE(linkedlist_clear);
SHADER_DEFINE(blit_oitblend);

cvar_t *r_hdr = NULL;
cvar_t *r_hdr_blurwidth = NULL;
cvar_t *r_hdr_exposure = NULL;
cvar_t *r_hdr_darkness = NULL;
cvar_t *r_hdr_adaptation = NULL;
cvar_t *r_hdr_debug = NULL;

cvar_t *r_fxaa = NULL;

cvar_t *r_ssao = NULL;
cvar_t *r_ssao_debug = NULL;
cvar_t *r_ssao_radius = NULL;
cvar_t *r_ssao_intensity = NULL;
cvar_t *r_ssao_bias = NULL;
cvar_t *r_ssao_blur_sharpness = NULL;
cvar_t *r_ssao_studio_model = NULL;

std::unordered_map<int, hud_debug_program_t> g_HudDebugProgramTable;

void R_UseHudDebugProgram(int state, hud_debug_program_t *progOutput)
{
	hud_debug_program_t prog = { 0 };

	auto itor = g_HudDebugProgramTable.find(state);
	if (itor == g_HudDebugProgramTable.end())
	{
		std::stringstream defs;

		if (state & HUD_DEBUG_TEXARRAY)
			defs << "#define TEXARRAY_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\hud_debug.vsh", "renderer\\shader\\hud_debug.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, basetex, "basetex");
			SHADER_UNIFORM(prog, layer , "layer");
		}

		g_HudDebugProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (prog.basetex != -1)
			glUniform1i(prog.basetex, 0);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseHudDebugProgram: Failed to load program!");
	}
}

float *R_GenerateGaussianWeights(int kernelRadius)
{
	int size = kernelRadius * 2 + 1;

	float x;
	float s = floor(kernelRadius / 4.0f);
	float *weights = new float[size];

	float sum = 0.0f;
	for (int i = 0; i < size; i++)
	{
		x = (float)(i - kernelRadius);

		// True Gaussian
		weights[i] = expf(-x * x / (2.0f*s*s)) / (s*sqrtf(2.0f*M_PI));

		// This sum of exps is not really a separable kernel but produces a very interesting star-shaped effect
		//weights[i] = expf( -0.0625f * x * x ) + 2 * expf( -0.25f * x * x ) + 4 * expf( - x * x ) + 8 * expf( - 4.0f * x * x ) + 16 * expf( - 16.0f * x * x ) ;
		sum += weights[i];
	}

	for (int i = 0; i < size; i++)
		weights[i] /= sum;

	return weights;
}

void R_CaculateGaussianBilinear(float *coordOffset, float *gaussWeight, int maxSamples)
{
	int i = 0;

	//  store all the intermediate offsets & weights, then compute the bilinear
	//  taps in a second pass
	float *tmpWeightArray = R_GenerateGaussianWeights(maxSamples);

	// Bilinear filtering taps 
	// Ordering is left to right.
	float sScale;
	float sFrac;

	for (i = 0; i < maxSamples; i++)
	{
		sScale = tmpWeightArray[i * 2 + 0] + tmpWeightArray[i * 2 + 1];
		sFrac = tmpWeightArray[i * 2 + 1] / sScale;

		coordOffset[i] = ((2.0f*i - maxSamples) + sFrac);
		gaussWeight[i] = sScale;
	}

	delete[]tmpWeightArray;
}

char *UTIL_VarArgs(char *format, ...);

void R_InitBlur(int samples)
{
	auto pp_common_vscode = (char *)gEngfuncs.COM_LoadFile((char *)"renderer\\shader\\pp_common.vsh", 5, 0);

	if (!pp_common_vscode)
		return;

	float coord_offsets[MAX_GAUSSIAN_SAMPLES];
	float gauss_weights[MAX_GAUSSIAN_SAMPLES];
	
	R_CaculateGaussianBilinear(coord_offsets, gauss_weights, min(samples, MAX_GAUSSIAN_SAMPLES));

	std::stringstream ss;

	//Generate horizonal blur code
	ss << "#version 120\nuniform float du;\nuniform sampler2D tex;\nvoid main()\n{\n vec4 sample = vec4(0.0, 0.0, 0.0, 0.0);\n";
	for (int i = 0; i < samples; ++i)
	{
		ss << UTIL_VarArgs(" sample += %f * texture2D( tex, gl_TexCoord[0].xy + vec2(%f * du, 0.0) );\n", gauss_weights[i], coord_offsets[i]);
	}
	ss << " gl_FragColor = sample;\n}";

	pp_gaussianblurh.program = R_CompileShader(pp_common_vscode, ss.str().c_str(), "pp_common.vsh", "pp_gaussianblur_h.fsh", NULL);
	if (pp_gaussianblurh.program)
	{
		SHADER_UNIFORM(pp_gaussianblurh, tex, "tex");
		SHADER_UNIFORM(pp_gaussianblurh, du, "du");
	}

	//Generate vertical blur code
	if (pp_gaussianblurv.program)
	{
		glDeleteObjectARB(pp_gaussianblurv.program);
		pp_gaussianblurv.program = 0;
	}

	std::stringstream ss2;

	ss2 << "#version 120\nuniform float du;\nuniform sampler2D tex;\nvoid main()\n{\n vec4 sample = vec4(0.0, 0.0, 0.0, 0.0);\n";
	for (int i = 0; i < samples; ++i)
	{
		ss2 << UTIL_VarArgs(" sample += %f * texture2D( tex, gl_TexCoord[0].xy + vec2(0.0, %f * du) );\n", gauss_weights[i], coord_offsets[i]);
	}
	ss2 << " gl_FragColor = sample;\n}";

	pp_gaussianblurv.program = R_CompileShader(pp_common_vscode, ss2.str().c_str(), "pp_common.vsh", "pp_gaussianblur_v.fsh", NULL);
	if (pp_gaussianblurv.program)
	{
		SHADER_UNIFORM(pp_gaussianblurv, tex, "tex");
		SHADER_UNIFORM(pp_gaussianblurv, du, "du");
	}

	gEngfuncs.COM_FreeFile(pp_common_vscode);
}

void R_InitGLHUD(void)
{
	float numDir = 8; // keep in sync to glsl

	signed short hbaoRandomShort[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES * 4];

	for (int i = 0; i < HBAO_RANDOM_ELEMENTS * MAX_SAMPLES; i++)
	{
		float Rand1 = gEngfuncs.pfnRandomFloat(0, 1);
		float Rand2 = gEngfuncs.pfnRandomFloat(0, 1);

		// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
		float Angle = 2.f * M_PI * Rand1 / numDir;
		m_hbaoRandom[i][0] = cosf(Angle);
		m_hbaoRandom[i][1] = sinf(Angle);
		m_hbaoRandom[i][2] = Rand2;
		m_hbaoRandom[i][3] = 0;
#define SCALE ((1<<15))
		hbaoRandomShort[i * 4 + 0] = (signed short)(SCALE*m_hbaoRandom[i][0]);
		hbaoRandomShort[i * 4 + 1] = (signed short)(SCALE*m_hbaoRandom[i][1]);
		hbaoRandomShort[i * 4 + 2] = (signed short)(SCALE*m_hbaoRandom[i][2]);
		hbaoRandomShort[i * 4 + 3] = (signed short)(SCALE*m_hbaoRandom[i][3]);
#undef SCALE
	}

	hbao_random = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D_ARRAY, hbao_random);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16_SNORM, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES, GL_RGBA, GL_SHORT, hbaoRandomShort);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	for (int i = 0; i < MAX_SAMPLES; i++)
	{
		hbao_randomview[i] = GL_GenTexture();
		glTextureView(hbao_randomview[i], GL_TEXTURE_2D, hbao_random, GL_RGBA16_SNORM, 0, 1, i, 1);
		glBindTexture(GL_TEXTURE_2D, hbao_randomview[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//FXAA Pass
	pp_fxaa.program = R_CompileShaderFile("renderer\\shader\\pp_fxaa.vsh", "renderer\\shader\\pp_fxaa.fsh", NULL);
	if (pp_fxaa.program)
	{
		SHADER_UNIFORM(pp_fxaa, tex0, "tex0");
		SHADER_UNIFORM(pp_fxaa, rt_w, "rt_w");
		SHADER_UNIFORM(pp_fxaa, rt_h, "rt_h");
	}

	//DownSample Pass
	pp_downsample.program = R_CompileShaderFile("renderer\\shader\\pp_common.vsh", "renderer\\shader\\pp_downsample.fsh", NULL);
	if (pp_downsample.program)
	{
		SHADER_UNIFORM(pp_downsample, tex, "tex");
	}

	//2x2 Downsample Pass
	pp_downsample2x2.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", "renderer\\shader\\pp_downsample2x2.fsh", NULL);
	if (pp_downsample2x2.program)
	{
		SHADER_UNIFORM(pp_downsample2x2, tex, "tex");
		SHADER_UNIFORM(pp_downsample2x2, texelsize, "texelsize");
	}

	//Luminance Downsample Pass
	pp_lumin.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", "renderer\\shader\\pp_lumin.fsh", NULL);
	if (pp_lumin.program)
	{
		SHADER_UNIFORM(pp_lumin, tex, "tex");
		SHADER_UNIFORM(pp_lumin, texelsize, "texelsize");
	}

	//Log Luminance Downsample Pass
	pp_luminlog.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", "renderer\\shader\\pp_luminlog.fsh", NULL);
	if (pp_luminlog.program)
	{
		SHADER_UNIFORM(pp_luminlog, tex, "tex");
		SHADER_UNIFORM(pp_luminlog, texelsize, "texelsize");
	}

	//Exp Luminance Downsample Pass
	pp_luminexp.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", "renderer\\shader\\pp_luminexp.fsh", NULL);
	if (pp_luminexp.program)
	{
		SHADER_UNIFORM(pp_luminexp, tex, "tex");
		SHADER_UNIFORM(pp_luminexp, texelsize, "texelsize");
	}

	//Luminance Adaptation Downsample Pass
	pp_luminadapt.program = R_CompileShaderFile("renderer\\shader\\pp_common.vsh", "renderer\\shader\\pp_luminadapt.fsh", NULL);
	if (pp_luminadapt.program)
	{
		SHADER_UNIFORM(pp_luminadapt, curtex, "curtex");
		SHADER_UNIFORM(pp_luminadapt, adatex, "adatex");
		SHADER_UNIFORM(pp_luminadapt, frametime, "frametime");
	}

	//Bright Pass
	pp_brightpass.program = R_CompileShaderFile("renderer\\shader\\pp_brightpass.vsh", "renderer\\shader\\pp_brightpass.fsh", NULL);
	if (pp_brightpass.program)
	{
		SHADER_UNIFORM(pp_brightpass, tex, "tex");
		SHADER_UNIFORM(pp_brightpass, lumtex, "lumtex");
	}

	//Tone mapping
	pp_tonemap.program = R_CompileShaderFile("renderer\\shader\\pp_tonemap.vsh", "renderer\\shader\\pp_tonemap.fsh", NULL);
	if (pp_tonemap.program)
	{
		SHADER_UNIFORM(pp_tonemap, basetex, "basetex");
		SHADER_UNIFORM(pp_tonemap, blurtex, "blurtex");
		SHADER_UNIFORM(pp_tonemap, lumtex, "lumtex");
		SHADER_UNIFORM(pp_tonemap, blurfactor, "blurfactor");
		SHADER_UNIFORM(pp_tonemap, exposure, "exposure");
		SHADER_UNIFORM(pp_tonemap, darkness, "darkness");
		SHADER_UNIFORM(pp_tonemap, gamma, "gamma");
	}

	//SSAO
	depth_linearize.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\depthlinearize.frag.glsl", NULL);

	depth_linearize_msaa.program = R_CompileShaderFileEx("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\depthlinearize.frag.glsl",
		"#define DEPTHLINEARIZE_MSAA 1\n", "#define DEPTHLINEARIZE_MSAA 1\n", NULL);

	hbao_calc_blur.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\hbao.frag.glsl", NULL);
	
	depth_clear.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\depthclear.frag.glsl", NULL);

	linkedlist_clear.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\linkedlist_clear.frag.glsl", NULL);
	
	blit_oitblend.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\blit_oitblend.frag.glsl", NULL);

	SHADER_UNIFORM(hbao_calc_blur, texLinearDepth, "texLinearDepth");
	SHADER_UNIFORM(hbao_calc_blur, texRandom, "texRandom");
	SHADER_UNIFORM(hbao_calc_blur, control_RadiusToScreen, "control_RadiusToScreen");
	SHADER_UNIFORM(hbao_calc_blur, control_projOrtho, "control_projOrtho");
	SHADER_UNIFORM(hbao_calc_blur, control_projInfo, "control_projInfo");
	SHADER_UNIFORM(hbao_calc_blur, control_PowExponent, "control_PowExponent");
	SHADER_UNIFORM(hbao_calc_blur, control_InvQuarterResolution, "control_InvQuarterResolution");
	SHADER_UNIFORM(hbao_calc_blur, control_AOMultiplier, "control_AOMultiplier");
	SHADER_UNIFORM(hbao_calc_blur, control_InvFullResolution, "control_InvFullResolution");
	SHADER_UNIFORM(hbao_calc_blur, control_NDotVBias, "control_NDotVBias");
	SHADER_UNIFORM(hbao_calc_blur, control_NegInvR2, "control_NegInvR2");

	hbao_calc_blur_fog.program = R_CompileShaderFileEx("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\hbao.frag.glsl", 
		"#define LINEAR_FOG_ENABLED\n", "#define LINEAR_FOG_ENABLED\n", NULL);

	SHADER_UNIFORM(hbao_calc_blur_fog, texLinearDepth, "texLinearDepth");
	SHADER_UNIFORM(hbao_calc_blur_fog, texRandom, "texRandom");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_RadiusToScreen, "control_RadiusToScreen");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_projOrtho, "control_projOrtho");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_projInfo, "control_projInfo");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_PowExponent, "control_PowExponent");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_InvQuarterResolution, "control_InvQuarterResolution");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_AOMultiplier, "control_AOMultiplier");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_InvFullResolution, "control_InvFullResolution");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_NDotVBias, "control_NDotVBias");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_NegInvR2, "control_NegInvR2");
	SHADER_UNIFORM(hbao_calc_blur_fog, control_Fog, "control_Fog");

	hbao_blur.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\hbao_blur.frag.glsl", NULL);

	hbao_blur2.program = R_CompileShaderFileEx("renderer\\shader\\fullscreenquad.vert.glsl", "renderer\\shader\\hbao_blur.frag.glsl",
		"#define AO_BLUR_PRESENT\n", "#define AO_BLUR_PRESENT\n", NULL);

	R_InitBlur(16);

	r_hdr = gEngfuncs.pfnRegisterVariable("r_hdr", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_blurwidth = gEngfuncs.pfnRegisterVariable("r_hdr_blurwidth", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_exposure = gEngfuncs.pfnRegisterVariable("r_hdr_exposure", "5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_darkness = gEngfuncs.pfnRegisterVariable("r_hdr_darkness", "4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_adaptation = gEngfuncs.pfnRegisterVariable("r_hdr_adaptation", "50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_debug = gEngfuncs.pfnRegisterVariable("r_hdr_debug", "0",  FCVAR_CLIENTDLL);

	r_fxaa = gEngfuncs.pfnRegisterVariable("r_fxaa", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_ssao = gEngfuncs.pfnRegisterVariable("r_ssao", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_debug = gEngfuncs.pfnRegisterVariable("r_ssao_debug", "0",  FCVAR_CLIENTDLL);
	r_ssao_radius = gEngfuncs.pfnRegisterVariable("r_ssao_radius", "30.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_intensity = gEngfuncs.pfnRegisterVariable("r_ssao_intensity", "0.6", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_bias = gEngfuncs.pfnRegisterVariable("r_ssao_bias", "0.2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_blur_sharpness = gEngfuncs.pfnRegisterVariable("r_ssao_blur_sharpness", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	last_luminance = 0;
}

void R_DrawHUDQuadFrustum(int w, int h)
{
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glColor3fv(r_frustum_origin[0]);
	glVertex3f(0, h, -1);

	glTexCoord2f(0, 1);
	glColor3fv(r_frustum_origin[1]);
	glVertex3f(0, 0, -1);

	glTexCoord2f(1, 1);
	glColor3fv(r_frustum_origin[2]);
	glVertex3f(w, 0, -1);

	glTexCoord2f(1, 0);
	glColor3fv(r_frustum_origin[3]);
	glVertex3f(w, h, -1);

	glEnd();
}

void R_DrawHUDQuad(int w, int h)
{
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex3f(0, h, -1);

	glTexCoord2f(0, 1);
	glVertex3f(0, 0, -1);

	glTexCoord2f(1, 1);
	glVertex3f(w, 0, -1);

	glTexCoord2f(1, 0);
	glVertex3f(w, h, -1);

	glEnd();
}

void R_DrawHUDQuad_Texture(int tex, int w, int h)
{
	GL_Bind(tex);

	R_DrawHUDQuad(w, h);
}

void R_BlitToScreen(FBO_Container_t *src)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, glwidth, glheight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void R_BlitToFBO(FBO_Container_t *src, FBO_Container_t *dst)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void R_DownSample(FBO_Container_t *src, FBO_Container_t *dst, qboolean filter2x2)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if(!filter2x2)
	{
		GL_UseProgram(pp_downsample.program);
		glUniform1i(pp_downsample.tex, 0);
	}
	else
	{
		GL_UseProgram(pp_downsample2x2.program);
		glUniform1i(pp_downsample2x2.tex, 0);
		glUniform2f(pp_downsample2x2.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}

	GL_Begin2DEx(dst->iWidth, dst->iHeight);

	R_DrawHUDQuad_Texture(src->s_hBackBufferTex, dst->iWidth, dst->iHeight);
}

void R_LuminPass(FBO_Container_t *src, FBO_Container_t *dst, int logexp)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	if(!logexp)
	{
		GL_UseProgram(pp_lumin.program);
		glUniform1i(pp_lumin.tex, 0);
		glUniform2f(pp_lumin.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else if(logexp == 1)
	{
		GL_UseProgram(pp_luminlog.program);
		glUniform1i(pp_luminlog.tex, 0);
		glUniform2f(pp_luminlog.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else
	{
		GL_UseProgram(pp_luminexp.program);
		glUniform1i(pp_luminexp.tex, 0);
		glUniform2f(pp_luminexp.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}

	GL_Begin2DEx(dst->iWidth, dst->iHeight);

	R_DrawHUDQuad_Texture(src->s_hBackBufferTex, dst->iWidth, dst->iHeight);
}

void R_LuminAdaptation(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *ada, double frametime)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_UseProgram(pp_luminadapt.program);
	glUniform1i(pp_luminadapt.curtex, 0);
	glUniform1i(pp_luminadapt.adatex, 1);
	glUniform1f(pp_luminadapt.frametime, frametime * clamp(r_hdr_control.adaptation, 0.1, 100));

	GL_SelectTexture(GL_TEXTURE0);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(ada->s_hBackBufferTex);

	GL_Begin2DEx(dst->iWidth, dst->iHeight);

	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_DisableMultitexture();
}

void R_BrightPass(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *lum)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_UseProgram(pp_brightpass.program);
	glUniform1i(pp_brightpass.tex, 0);
	glUniform1i(pp_brightpass.lumtex, 1);

	GL_SelectTexture(GL_TEXTURE0);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(lum->s_hBackBufferTex);

	GL_Begin2DEx(dst->iWidth, dst->iHeight);

	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_DisableMultitexture();

	GL_UseProgram(0);
}

void R_BlurPass(FBO_Container_t *src, FBO_Container_t *dst, qboolean vertical)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	if(!vertical)
	{
		GL_UseProgram(pp_gaussianblurh.program);
		glUniform1i(pp_gaussianblurh.tex, 0);
		glUniform1f(pp_gaussianblurh.du, 1.0f / src->iWidth);
	}
	else
	{
		GL_UseProgram(pp_gaussianblurv.program);
		glUniform1i(pp_gaussianblurv.tex, 0);
		glUniform1f(pp_gaussianblurv.du, 1.0f / src->iHeight);
	}

	GL_Begin2DEx(dst->iWidth, dst->iHeight);

	R_DrawHUDQuad_Texture(src->s_hBackBufferTex, dst->iWidth, dst->iHeight);
}

void R_BrightAccum(FBO_Container_t *blur1, FBO_Container_t *blur2, FBO_Container_t *blur3, FBO_Container_t *dst)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	GL_UseProgram(0);
	GL_Begin2DEx(dst->iWidth, dst->iHeight);

	GL_Bind(blur1->s_hBackBufferTex);
	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_Bind(blur2->s_hBackBufferTex);
	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_Bind(blur3->s_hBackBufferTex);
	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	glDisable(GL_BLEND);
}

void R_ToneMapping(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *blur, FBO_Container_t *lum)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_UseProgram(pp_tonemap.program);
	glUniform1i(pp_tonemap.basetex, 0);
	glUniform1i(pp_tonemap.blurtex, 1);
	glUniform1i(pp_tonemap.lumtex, 2);
	glUniform1f(pp_tonemap.blurfactor, clamp(r_hdr_control.blurwidth, 0, 1));
	glUniform1f(pp_tonemap.exposure, clamp(r_hdr_control.exposure, 0.001, 10));
	glUniform1f(pp_tonemap.darkness, clamp(r_hdr_control.darkness, 0.001, 10));
	glUniform1f(pp_tonemap.gamma, 1.0 / v_gamma->value);
	
	GL_SelectTexture(GL_TEXTURE0);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(blur->s_hBackBufferTex);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, lum->s_hBackBufferTex);

	GL_Begin2DEx(dst->iWidth, dst->iHeight);

	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	GL_DisableMultitexture();

	GL_UseProgram(0);
}

void R_DoHDR(void)
{
	if (!r_hdr->value)
		return;

	if (r_draw_pass)
		return;

	if (g_SvEngine_DrawPortalView)
		return;

	GL_PushDrawState();

	GL_Begin2D();
	glDisable(GL_BLEND);
	GL_DisableMultitexture();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1, 1, 1, 1);

	//Downsample backbuffer
	R_DownSample(&s_BackBufferFBO, &s_DownSampleFBO[0], false);//(1->1/4)
	R_DownSample(&s_DownSampleFBO[0], &s_DownSampleFBO[1], false);//(1/4)->(1/16)

	//Log Luminance DownSample from .. (HDRColor to 32RF)
	R_LuminPass(&s_DownSampleFBO[1], &s_LuminFBO[0], 1);//(1/16)->64x64

	//Luminance DownSample from..
	R_LuminPass(&s_LuminFBO[0], &s_LuminFBO[1], 0);//64x64->16x16
	R_LuminPass(&s_LuminFBO[1], &s_LuminFBO[2], 0);//16x16->4x4
	//exp Luminance DownSample from..
	R_LuminPass(&s_LuminFBO[2], &s_Lumin1x1FBO[2], 2);//4x4->1x1

	//Luminance Adaptation
	R_LuminAdaptation(&s_Lumin1x1FBO[2], &s_Lumin1x1FBO[!last_luminance], &s_Lumin1x1FBO[last_luminance], *cl_time - *cl_oldtime);
	last_luminance = !last_luminance;

	//Bright Pass (with 1/16)
	R_BrightPass(&s_DownSampleFBO[1], &s_BrightPassFBO, &s_Lumin1x1FBO[last_luminance]);

	//Gaussian Blur Pass (with bright pass)
	R_BlurPass(&s_BrightPassFBO, &s_BlurPassFBO[0][0], false);
	R_BlurPass(&s_BlurPassFBO[0][0], &s_BlurPassFBO[0][1], true);
	//Blur again and downsample from 1/16 to 1/32
	R_BlurPass(&s_BlurPassFBO[0][1], &s_BlurPassFBO[1][0], false);
	R_BlurPass(&s_BlurPassFBO[1][0], &s_BlurPassFBO[1][1], true);
	//Blur again and downsample from 1/32 to 1/64
	R_BlurPass(&s_BlurPassFBO[1][1], &s_BlurPassFBO[2][0], false);
	R_BlurPass(&s_BlurPassFBO[2][0], &s_BlurPassFBO[2][1], true);

	//Accumulate all blurred textures
	R_BrightAccum(&s_BlurPassFBO[0][1], &s_BlurPassFBO[1][1], &s_BlurPassFBO[2][1], &s_BrightAccumFBO);

	//Tone mapping
	R_ToneMapping(&s_BackBufferFBO, &s_ToneMapFBO, &s_BrightAccumFBO, &s_Lumin1x1FBO[last_luminance]);

	R_BlitToFBO(&s_ToneMapFBO, &s_BackBufferFBO);

	GL_PopDrawState();
}

void R_BeginFXAA(int w, int h)
{
	GL_UseProgram(pp_fxaa.program);
	glUniform1i(pp_fxaa.tex0, 0);
	glUniform1f(pp_fxaa.rt_w, w);
	glUniform1f(pp_fxaa.rt_h, h);
}

void R_DoFXAA(void)
{
	if (!r_fxaa->value)
		return;

	if (!pp_fxaa.program)
		return;

	if (r_draw_pass)
		return;

	if (g_SvEngine_DrawPortalView)
		return;

	GL_PushDrawState();
	GL_PushMatrix();

	R_BlitToFBO(&s_BackBufferFBO, &s_BackBufferFBO2);

	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	R_BeginFXAA(glwidth, glheight);

	GL_Begin2D();
	GL_DisableMultitexture();
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	R_DrawHUDQuad_Texture(s_BackBufferFBO2.s_hBackBufferTex, glwidth, glheight);

	GL_UseProgram(0);

	GL_PopMatrix();
	GL_PopDrawState();
}

void R_LinearizeDepth(FBO_Container_t *fbo)
{
	glBindFramebuffer(GL_FRAMEBUFFER, s_DepthLinearFBO.s_hBackBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glDisable(GL_BLEND);

	GL_UseProgram(depth_linearize.program);
	glUniform4f(0, r_near_z * r_far_z, r_near_z - r_far_z, r_far_z, r_ortho ? 0 : 1);
	
	GL_DisableMultitexture();
	GL_Bind(fbo->s_hBackBufferDepthTex);

	R_DrawHUDQuad(glwidth, glheight);
}

void R_DoSSAO(void)
{
	if (!r_ssao->value)
		return;

	if (r_refdef->onlyClientDraws || r_draw_pass || g_SvEngine_DrawPortalView)
		return;

	if ((*r_xfov) < 75)
		return;

	//Prepare parameters

	const float *ProjMatrix = r_projection_matrix;

	float projInfoPerspective[] = {
		2.0f / (ProjMatrix[4 * 0 + 0]),       // (x) * (R - L)/N
		2.0f / (ProjMatrix[4 * 1 + 1]),       // (y) * (T - B)/N
		-(1.0f - ProjMatrix[4 * 2 + 0]) / ProjMatrix[4 * 0 + 0], // L/N
		-(1.0f + ProjMatrix[4 * 2 + 1]) / ProjMatrix[4 * 1 + 1], // B/N
	};

	float projScale = float(glheight) / (tanf(r_yfov * 0.5f) * 2.0f);

	// radius
	float meters2viewspace = 1.0f;
	float R = r_ssao_radius->value * meters2viewspace;
	auto R2 = R * R;
	auto NegInvR2 = -1.0f / R2;
	auto RadiusToScreen = R * 0.5f * projScale;

	// ao
	auto PowExponent = max(r_ssao_intensity->value, 0.0f);
	auto NDotVBias = min(max(0.0f, r_ssao_bias->value), 1.0f);
	auto AOMultiplier = 1.0f / (1.0f - NDotVBias);

	// resolution
	int quarterWidth = ((glwidth + 3) / 4);
	int quarterHeight = ((glheight + 3) / 4);

	vec2_t InvQuarterResolution;
	InvQuarterResolution[0] = 1.0f / float(quarterWidth);
	InvQuarterResolution[1] = 1.0f / float(quarterHeight);
		
	vec2_t InvFullResolution;
	InvFullResolution[0] = 1.0f / float(glwidth);
	InvFullResolution[1] = 1.0f / float(glheight);

	GL_PushDrawState();

	GL_Begin2D();

	//depth linearize?

	R_LinearizeDepth(&s_BackBufferFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, s_HBAOCalcFBO.s_hBackBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	//setup args for hbao_calc
	if (r_fog_mode == GL_LINEAR)
	{
		GL_UseProgram(hbao_calc_blur_fog.program);
		glUniform4fv(hbao_calc_blur_fog.control_projInfo, 1, projInfoPerspective);
		glUniform2fv(hbao_calc_blur_fog.control_InvFullResolution, 1, InvFullResolution);
		glUniform2fv(hbao_calc_blur_fog.control_InvQuarterResolution, 1, InvQuarterResolution);
		glUniform1i(hbao_calc_blur_fog.control_projOrtho, 0);
		glUniform1f(hbao_calc_blur_fog.control_RadiusToScreen, RadiusToScreen);
		glUniform1f(hbao_calc_blur_fog.control_AOMultiplier, AOMultiplier);
		glUniform1f(hbao_calc_blur_fog.control_NDotVBias, NDotVBias);
		glUniform1f(hbao_calc_blur_fog.control_NegInvR2, NegInvR2);
		glUniform1f(hbao_calc_blur_fog.control_PowExponent, PowExponent);
		glUniform2f(hbao_calc_blur_fog.control_Fog, r_fog_control[0], r_fog_control[1]);
	}
	else
	{
		GL_UseProgram(hbao_calc_blur.program);
		glUniform4fv(hbao_calc_blur.control_projInfo, 1, projInfoPerspective);
		glUniform2fv(hbao_calc_blur.control_InvFullResolution, 1, InvFullResolution);
		glUniform2fv(hbao_calc_blur.control_InvQuarterResolution, 1, InvQuarterResolution);
		glUniform1i(hbao_calc_blur.control_projOrtho, 0);
		glUniform1f(hbao_calc_blur.control_RadiusToScreen, RadiusToScreen);
		glUniform1f(hbao_calc_blur.control_AOMultiplier, AOMultiplier);
		glUniform1f(hbao_calc_blur.control_NDotVBias, NDotVBias);
		glUniform1f(hbao_calc_blur.control_NegInvR2, NegInvR2);
		glUniform1f(hbao_calc_blur.control_PowExponent, PowExponent);
	}

	//Texture unit 0 = linearized depth
	GL_SelectTexture(GL_TEXTURE0);
	GL_Bind(s_DepthLinearFBO.s_hBackBufferTex);

	//Texture unit 1 = random texture
	GL_EnableMultitexture();
	GL_Bind(hbao_randomview[0]);

	R_DrawHUDQuad(glwidth, glheight);

	//SSAO blur stage

	//Write to HBAO texture2
	glDrawBuffer(GL_COLOR_ATTACHMENT1);

	GL_UseProgram(hbao_blur.program);
	glUniform1f(0, r_ssao_blur_sharpness->value / meters2viewspace);
	glUniform2f(1, 1.0f / float(glwidth), 0);

	//Texture unit 0 = calc
	GL_DisableMultitexture();
	GL_Bind(s_HBAOCalcFBO.s_hBackBufferTex);

	R_DrawHUDQuad(glwidth, glheight);

	//Write to main framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	//Merge SSAO result into main scene
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	glColor4f(1, 1, 1, 1);

	//Don't draw SSAO shadow on studio models, water, etc...
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glStencilFunc(GL_EQUAL, 0, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_UseProgram(hbao_blur2.program);
	glUniform1f(0, r_ssao_blur_sharpness->value / meters2viewspace);
	glUniform2f(1, 0, 1.0f / float(glheight));

	//Texture unit 0 = calc2
	GL_Bind(s_HBAOCalcFBO.s_hBackBufferTex2);

	R_DrawHUDQuad(glwidth, glheight);

	GL_UseProgram(0);

	glStencilMask(0);
	glDisable(GL_STENCIL_TEST);

	GL_PopDrawState();

	GL_End2D();
}
