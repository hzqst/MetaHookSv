#include "gl_local.h"
#include <sstream>

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
SHADER_DEFINE(hbao_blur);
SHADER_DEFINE(hbao_blur2);

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

	pp_gaussianblurh.program = R_CompileShader(pp_common_vscode, NULL, ss.str().c_str(), "pp_common.vsh", NULL, "pp_gaussianblur_h.fsh");
	if (pp_gaussianblurh.program)
	{
		SHADER_UNIFORM(pp_gaussianblurh, tex, "tex");
		SHADER_UNIFORM(pp_gaussianblurh, du, "du");
	}

	//Generate vertical blur code
	if (pp_gaussianblurv.program)
	{
		qglDeleteObjectARB(pp_gaussianblurv.program);
		pp_gaussianblurv.program = 0;
	}

	std::stringstream ss2;

	ss2 << "#version 120\nuniform float du;\nuniform sampler2D tex;\nvoid main()\n{\n vec4 sample = vec4(0.0, 0.0, 0.0, 0.0);\n";
	for (int i = 0; i < samples; ++i)
	{
		ss2 << UTIL_VarArgs(" sample += %f * texture2D( tex, gl_TexCoord[0].xy + vec2(0.0, %f * du) );\n", gauss_weights[i], coord_offsets[i]);
	}
	ss2 << " gl_FragColor = sample;\n}";

	pp_gaussianblurv.program = R_CompileShader(pp_common_vscode, NULL, ss2.str().c_str(), "pp_common.vsh", NULL, "pp_gaussianblur_v.fsh");
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
	qglBindTexture(GL_TEXTURE_2D_ARRAY, hbao_random);
	qglTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16_SNORM, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES);
	qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES, GL_RGBA, GL_SHORT, hbaoRandomShort);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	for (int i = 0; i < MAX_SAMPLES; i++)
	{
		hbao_randomview[i] = GL_GenTexture();
		qglTextureView(hbao_randomview[i], GL_TEXTURE_2D, hbao_random, GL_RGBA16_SNORM, 0, 1, i, 1);
		qglBindTexture(GL_TEXTURE_2D, hbao_randomview[i]);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}

	//FXAA Pass
	pp_fxaa.program = R_CompileShaderFile("renderer\\shader\\pp_fxaa.vsh", NULL, "renderer\\shader\\pp_fxaa.fsh");
	if (pp_fxaa.program)
	{
		SHADER_UNIFORM(pp_fxaa, tex0, "tex0");
		SHADER_UNIFORM(pp_fxaa, rt_w, "rt_w");
		SHADER_UNIFORM(pp_fxaa, rt_h, "rt_h");
	}

	//DownSample Pass
	pp_downsample.program = R_CompileShaderFile("renderer\\shader\\pp_common.vsh", NULL, "renderer\\shader\\pp_downsample.fsh");
	if (pp_downsample.program)
	{
		SHADER_UNIFORM(pp_downsample, tex, "tex");
	}

	//2x2 Downsample Pass
	pp_downsample2x2.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", NULL, "renderer\\shader\\pp_downsample2x2.fsh");
	if (pp_downsample2x2.program)
	{
		SHADER_UNIFORM(pp_downsample2x2, tex, "tex");
		SHADER_UNIFORM(pp_downsample2x2, texelsize, "texelsize");
	}

	//Luminance Downsample Pass
	pp_lumin.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", NULL, "renderer\\shader\\pp_lumin.fsh");
	if (pp_lumin.program)
	{
		SHADER_UNIFORM(pp_lumin, tex, "tex");
		SHADER_UNIFORM(pp_lumin, texelsize, "texelsize");
	}

	//Log Luminance Downsample Pass
	pp_luminlog.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", NULL, "renderer\\shader\\pp_luminlog.fsh");
	if (pp_luminlog.program)
	{
		SHADER_UNIFORM(pp_luminlog, tex, "tex");
		SHADER_UNIFORM(pp_luminlog, texelsize, "texelsize");
	}

	//Exp Luminance Downsample Pass
	pp_luminexp.program = R_CompileShaderFile("renderer\\shader\\pp_common2x2.vsh", NULL, "renderer\\shader\\pp_luminexp.fsh");
	if (pp_luminexp.program)
	{
		SHADER_UNIFORM(pp_luminexp, tex, "tex");
		SHADER_UNIFORM(pp_luminexp, texelsize, "texelsize");
	}

	//Luminance Adaptation Downsample Pass
	pp_luminadapt.program = R_CompileShaderFile("renderer\\shader\\pp_common.vsh", NULL, "renderer\\shader\\pp_luminadapt.fsh");
	if (pp_luminadapt.program)
	{
		SHADER_UNIFORM(pp_luminadapt, curtex, "curtex");
		SHADER_UNIFORM(pp_luminadapt, adatex, "adatex");
		SHADER_UNIFORM(pp_luminadapt, frametime, "frametime");
	}

	//Bright Pass
	pp_brightpass.program = R_CompileShaderFile("renderer\\shader\\pp_brightpass.vsh", NULL, "renderer\\shader\\pp_brightpass.fsh");
	if (pp_brightpass.program)
	{
		SHADER_UNIFORM(pp_brightpass, tex, "tex");
		SHADER_UNIFORM(pp_brightpass, lumtex, "lumtex");
	}

	//Tone mapping
	pp_tonemap.program = R_CompileShaderFile("renderer\\shader\\pp_tonemap.vsh", NULL, "renderer\\shader\\pp_tonemap.fsh");
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
	depth_linearize.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", NULL, "renderer\\shader\\depthlinearize.frag.glsl");
	if (depth_linearize.program)
	{
	}

	depth_linearize_msaa.program = R_CompileShaderFileEx("renderer\\shader\\fullscreenquad.vert.glsl", NULL, "renderer\\shader\\depthlinearize.frag.glsl",
		"#define DEPTHLINEARIZE_MSAA 1\n", NULL, "#define DEPTHLINEARIZE_MSAA 1\n");
	if (depth_linearize_msaa.program)
	{
	}

	hbao_calc_blur.program = R_CompileShaderFile("renderer\\shader\\fullscreenquad.vert.glsl", NULL, "renderer\\shader\\hbao.frag.glsl");
	if (hbao_calc_blur.program)
	{
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
	}

	hbao_blur.program = R_CompileShaderFileEx("renderer\\shader\\fullscreenquad.vert.glsl", NULL, "renderer\\shader\\hbao_blur.frag.glsl",
		"", NULL, "");
	if (hbao_blur.program)
	{
	}

	hbao_blur2.program = R_CompileShaderFileEx("renderer\\shader\\fullscreenquad.vert.glsl", NULL, "renderer\\shader\\hbao_blur.frag.glsl",
		"#define AO_BLUR_PRESENT\n", NULL, "#define AO_BLUR_PRESENT\n");
	if (hbao_blur2.program)
	{

	}

	//gaussian blur code
	R_InitBlur(16);

	r_hdr = gEngfuncs.pfnRegisterVariable("r_hdr", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_blurwidth = gEngfuncs.pfnRegisterVariable("r_hdr_blurwidth", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_exposure = gEngfuncs.pfnRegisterVariable("r_hdr_exposure", "5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_darkness = gEngfuncs.pfnRegisterVariable("r_hdr_darkness", "4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_adaptation = gEngfuncs.pfnRegisterVariable("r_hdr_adaptation", "50.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
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

void R_BeginHUDQuad(void)
{
	qglDisable(GL_BLEND);
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_ALPHA_TEST);
	qglDisable(GL_CULL_FACE);
	
	GL_DisableMultitexture();
	qglEnable(GL_TEXTURE_2D);
	qglColor4f(1, 1, 1, 1);
}

void R_DrawHUDQuad(int w, int h)
{
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, w, h, 0, -1.875, 1.875);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglViewport(0, 0, w, h);

	qglBegin(GL_QUADS);
	qglTexCoord2f(0, 0);
	qglVertex3f(0, h, -1);
	qglTexCoord2f(0, 1);
	qglVertex3f(0, 0, -1);
	qglTexCoord2f(1, 1);
	qglVertex3f(w, 0, -1);
	qglTexCoord2f(1, 0);
	qglVertex3f(w, h, -1);
	qglEnd();
}

void R_DrawHUDQuad_Texture(int tex, int w, int h)
{
	GL_Bind(tex);

	R_DrawHUDQuad(w, h);
}

void R_BlitToScreen(FBO_Container_t *src)
{
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglBlitFramebufferEXT(0, 0, src->iWidth, src->iHeight, 0, 0, glwidth, glheight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void R_BlitToFBO(FBO_Container_t *src, FBO_Container_t *dst)
{
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	qglBlitFramebufferEXT(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void R_DownSample(FBO_Container_t *src, FBO_Container_t *dst, qboolean filter2x2)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0, 0, 0, 1);
	qglClear(GL_COLOR_BUFFER_BIT);

	if(!filter2x2)
	{
		qglUseProgramObjectARB(pp_downsample.program);
		qglUniform1iARB(pp_downsample.tex, 0);
	}
	else
	{
		qglUseProgramObjectARB(pp_downsample2x2.program);
		qglUniform1iARB(pp_downsample2x2.tex, 0);
		qglUniform2fARB(pp_downsample2x2.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}

	R_DrawHUDQuad_Texture(src->s_hBackBufferTex, dst->iWidth, dst->iHeight);

	qglUseProgramObjectARB(0);

	if (!dst->s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, dst->s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, dst->iTextureColorFormat, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_LuminPass(FBO_Container_t *src, FBO_Container_t *dst, int logexp)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);

	if(!logexp)
	{
		qglUseProgramObjectARB(pp_lumin.program);
		qglUniform1iARB(pp_lumin.tex, 0);
		qglUniform2fARB(pp_lumin.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else if(logexp == 1)
	{
		qglUseProgramObjectARB(pp_luminlog.program);
		qglUniform1iARB(pp_luminlog.tex, 0);
		qglUniform2fARB(pp_luminlog.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else
	{
		qglUseProgramObjectARB(pp_luminexp.program);
		qglUniform1iARB(pp_luminexp.tex, 0);
		qglUniform2fARB(pp_luminexp.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}

	R_DrawHUDQuad_Texture(src->s_hBackBufferTex, dst->iWidth, dst->iHeight);

	qglUseProgramObjectARB(0);

	if (!dst->s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, dst->s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_LuminAdaptation(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *ada, double frametime)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglUseProgramObjectARB(pp_luminadapt.program);
	qglUniform1iARB(pp_luminadapt.curtex, 0);
	qglUniform1iARB(pp_luminadapt.adatex, 1);
	qglUniform1fARB(pp_luminadapt.frametime, frametime * clamp(r_hdr_adaptation->value, 1, 100));

	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(ada->s_hBackBufferTex);

	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_DisableMultitexture();

	qglUseProgramObjectARB(0);

	if (!dst->s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, dst->s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, dst->iTextureColorFormat, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_BrightPass(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *lum)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglUseProgramObjectARB(pp_brightpass.program);
	qglUniform1iARB(pp_brightpass.tex, 0);
	qglUniform1iARB(pp_brightpass.lumtex, 1);

	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(lum->s_hBackBufferTex);

	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_DisableMultitexture();

	qglUseProgramObjectARB(0);

	if (!dst->s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, dst->s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, dst->iTextureColorFormat, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_BlurPass(FBO_Container_t *src, FBO_Container_t *dst, qboolean vertical)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);

	if(!vertical)
	{
		qglUseProgramObjectARB(pp_gaussianblurh.program);
		qglUniform1iARB(pp_gaussianblurh.tex, 0);
		qglUniform1fARB(pp_gaussianblurh.du, 1.0f / src->iWidth);
	}
	else
	{
		qglUseProgramObjectARB(pp_gaussianblurv.program);
		qglUniform1iARB(pp_gaussianblurv.tex, 0);
		qglUniform1fARB(pp_gaussianblurv.du, 1.0f / src->iHeight);
	}

	R_DrawHUDQuad_Texture(src->s_hBackBufferTex, dst->iWidth, dst->iHeight);

	qglUseProgramObjectARB(0);

	if (!dst->s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, dst->s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, dst->iTextureColorFormat, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_BrightAccum(FBO_Container_t *blur1, FBO_Container_t *blur2, FBO_Container_t *blur3, FBO_Container_t *dst)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);

	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(blur1->s_hBackBufferTex);
	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_Bind(blur2->s_hBackBufferTex);
	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	GL_Bind(blur3->s_hBackBufferTex);
	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	qglDisable(GL_BLEND);

	if (!dst->s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, dst->s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, dst->iTextureColorFormat, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_ToneMapping(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *blur, FBO_Container_t *lum)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglUseProgramObjectARB(pp_tonemap.program);
	qglUniform1iARB(pp_tonemap.basetex, 0);
	qglUniform1iARB(pp_tonemap.blurtex, 1);
	qglUniform1iARB(pp_tonemap.lumtex, 2);
	qglUniform1fARB(pp_tonemap.blurfactor, clamp(r_hdr_blurwidth->value, 0, 1));
	qglUniform1fARB(pp_tonemap.exposure, clamp(r_hdr_exposure->value, 0.001, 10));
	qglUniform1fARB(pp_tonemap.darkness, clamp(r_hdr_darkness->value, 0.001, 10));
	qglUniform1fARB(pp_tonemap.gamma, 1.0 / v_gamma->value);
	
	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(blur->s_hBackBufferTex);

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, lum->s_hBackBufferTex);

	R_DrawHUDQuad(dst->iWidth, dst->iHeight);

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_2D);
	qglActiveTextureARB(TEXTURE1_SGIS);
	
	GL_DisableMultitexture();

	qglUseProgramObjectARB(0);

	if (!dst->s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, dst->s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, dst->iTextureColorFormat, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_BeginFXAA(int w, int h)
{
	qglUseProgramObjectARB(pp_fxaa.program);
	qglUniform1iARB(pp_fxaa.tex0, 0);
	qglUniform1fARB(pp_fxaa.rt_w, w);
	qglUniform1fARB(pp_fxaa.rt_h, h);
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
	GL_PushMatrix();

	R_BeginHUDQuad();

	if (!s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, s_BackBufferFBO.iTextureColorFormat, 0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight, 0);
	}

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

	if (s_ToneMapFBO.s_hBackBufferFBO)
	{
		R_BlitToFBO(&s_ToneMapFBO, &s_BackBufferFBO);
	}
	else
	{
		qglClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT);

		R_DrawHUDQuad_Texture(s_ToneMapFBO.s_hBackBufferTex, s_ToneMapFBO.iWidth, s_ToneMapFBO.iHeight);
	}

	GL_PopMatrix();
	GL_PopDrawState();
}

int R_DoSSAO(int sampleIndex)
{
	if (!r_ssao || !r_ssao->value)
		return 0;

	if (!s_DepthLinearFBO.s_hBackBufferFBO || !s_HBAOCalcFBO.s_hBackBufferFBO)
		return 0;

	GLfloat ProjMatrix[16];
	qglGetFloatv(GL_PROJECTION_MATRIX, ProjMatrix);

	GL_PushFrameBuffer();
	GL_PushMatrix();
	GL_PushDrawState();

	R_BeginHUDQuad();

	//write to depthlinear
	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_DepthLinearFBO.s_hBackBufferFBO);
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	//MSAA?
	if (sampleIndex >= 0)
	{
		qglUseProgramObjectARB(depth_linearize_msaa.program);
		qglUniform4fARB(0, 4 * r_params.movevars->zmax, 4 - r_params.movevars->zmax, r_params.movevars->zmax, 1.0f);
		qglUniform1iARB(1, sampleIndex);

		qglActiveTextureARB(GL_TEXTURE_2D_MULTISAMPLE);
		qglBindTexture(GL_TEXTURE_2D_MULTISAMPLE, s_MSAAFBO.s_hBackBufferDepthTex);

		R_DrawHUDQuad(glwidth, glheight);

		qglActiveTextureARB(GL_TEXTURE_2D_MULTISAMPLE);
		qglBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	}
	else
	{
		qglUseProgramObjectARB(depth_linearize.program);
		qglUniform4fARB(0, 4 * r_params.movevars->zmax, 4 - r_params.movevars->zmax, r_params.movevars->zmax, 1.0f);

		GL_Bind(s_BackBufferFBO.s_hBackBufferDepthTex);
		R_DrawHUDQuad(glwidth, glheight);
	}

	qglUseProgramObjectARB(0);

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_HBAOCalcFBO.s_hBackBufferFBO);
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	float projInfoPerspective[] = {
		2.0f / (ProjMatrix[4 * 0 + 0]),       // (x) * (R - L)/N
		2.0f / (ProjMatrix[4 * 1 + 1]),       // (y) * (T - B)/N
		-(1.0f - ProjMatrix[4 * 2 + 0]) / ProjMatrix[4 * 0 + 0], // L/N
		-(1.0f + ProjMatrix[4 * 2 + 1]) / ProjMatrix[4 * 1 + 1], // B/N
	};

	float projInfoOrtho[] = {
		2.0f / (ProjMatrix[4 * 0 + 0]),      // ((x) * R - L)
		2.0f / (ProjMatrix[4 * 1 + 1]),      // ((y) * T - B)
		-(1.0f + ProjMatrix[4 * 3 + 0]) / ProjMatrix[4 * 0 + 0], // L
		-(1.0f - ProjMatrix[4 * 3 + 1]) / ProjMatrix[4 * 1 + 1], // B
	};

	int useOrtho = 0;
	int projOrtho = useOrtho;
	auto projInfo = useOrtho ? projInfoOrtho : projInfoPerspective;

	float projScale;
	if (useOrtho) 
	{
		projScale = float(glheight) / (projInfoOrtho[1]);
	}
	else
	{
		projScale = float(glheight) / (tanf(scr_fov_value * 0.5f) * 2.0f);
	}

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

	//setup args for hbao_calc

	qglUseProgramObjectARB(hbao_calc_blur.program);
	qglUniform1iARB(hbao_calc_blur.control_projOrtho, projOrtho);
	qglUniform4fvARB(hbao_calc_blur.control_projInfo, 1, projInfo);
	qglUniform2fvARB(hbao_calc_blur.control_InvFullResolution, 1, InvFullResolution);
	qglUniform2fvARB(hbao_calc_blur.control_InvQuarterResolution, 1, InvQuarterResolution);
	qglUniform1fARB(hbao_calc_blur.control_RadiusToScreen, RadiusToScreen);
	qglUniform1fARB(hbao_calc_blur.control_AOMultiplier, AOMultiplier);
	qglUniform1fARB(hbao_calc_blur.control_NDotVBias, NDotVBias);
	qglUniform1fARB(hbao_calc_blur.control_NegInvR2, NegInvR2);
	qglUniform1fARB(hbao_calc_blur.control_PowExponent, PowExponent);

	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(s_DepthLinearFBO.s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(hbao_randomview[sampleIndex >= 0 ? sampleIndex : 0]);

	R_DrawHUDQuad(glwidth, glheight);

	qglUseProgramObjectARB(0);

	//SSAO blur stage

	//Write to HBAO texture2
	qglDrawBuffer(GL_COLOR_ATTACHMENT1);

	qglUseProgramObjectARB(hbao_blur.program);
	qglUniform1fARB(0, r_ssao_blur_sharpness->value / meters2viewspace);
	qglUniform2fARB(1, 1.0f / float(glwidth), 0);

	GL_DisableMultitexture();
	GL_Bind(s_HBAOCalcFBO.s_hBackBufferTex);
	//GL_EnableMultitexture();
	//GL_Bind(s_DepthLinearFBO.s_hBackBufferTex);

	R_DrawHUDQuad(glwidth, glheight);

	qglUseProgramObjectARB(0);

	//GL_DisableMultitexture();

	//Final output stage, write to main FBO or MSAA FBO.
	if (R_UseMSAA())
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
	else
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	//Merge SSAO result into main scene
	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ZERO, GL_SRC_COLOR);
	qglColor4f(1, 1, 1, 1);

	if (sampleIndex >= 0) {
		qglEnable(GL_SAMPLE_MASK);
		qglSampleMaski(0, 1 << sampleIndex);
	}

	//Stencil for studio model?
	qglEnable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);
	qglStencilFunc(GL_EQUAL, 0, 0xFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	qglUseProgramObjectARB(hbao_blur2.program);
	qglUniform1fARB(0, r_ssao_blur_sharpness->value / meters2viewspace);
	qglUniform2fARB(1, 0, 1.0f / float(glheight));

	GL_Bind(s_HBAOCalcFBO.s_hBackBufferTex2);
	R_DrawHUDQuad(glwidth, glheight);

	qglUseProgramObjectARB(0);

	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);

	qglDisable(GL_SAMPLE_MASK);
	qglSampleMaski(0, ~0);

	GL_PopDrawState();
	GL_PopMatrix();
	GL_PopFrameBuffer();

	return 1;
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

	if (s_BackBufferFBO.s_hBackBufferFBO && s_BackBufferFBO2.s_hBackBufferFBO)
	{
		R_BlitToFBO(&s_BackBufferFBO, &s_BackBufferFBO2);

		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

		R_BeginHUDQuad();
		R_BeginFXAA(glwidth, glheight);
		R_DrawHUDQuad_Texture(s_BackBufferFBO2.s_hBackBufferTex, glwidth, glheight);
		qglUseProgramObjectARB(0);
	}
	else
	{
		qglBindTexture(GL_TEXTURE_2D, s_BackBufferFBO2.s_hBackBufferTex);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, s_BackBufferFBO2.iWidth, s_BackBufferFBO2.iHeight, 0);

		R_BeginHUDQuad();
		R_BeginFXAA(glwidth, glheight);
		R_DrawHUDQuad_Texture(s_BackBufferFBO2.s_hBackBufferTex, glwidth, glheight);
		qglUseProgramObjectARB(0);
	}

	GL_PopMatrix();
	GL_PopDrawState();
}