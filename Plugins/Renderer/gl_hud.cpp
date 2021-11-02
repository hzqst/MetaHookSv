#include "gl_local.h"
#include <sstream>

//HDR
int last_luminance = 0;

#define MAX_GAUSSIAN_SAMPLES 16

#define LUMPASS_DOWN 0
#define LUMPASS_LOG 1
#define LUMPASS_EXP 2

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
SHADER_DEFINE(pp_lumindown);
SHADER_DEFINE(pp_luminlog);
SHADER_DEFINE(pp_luminexp);
SHADER_DEFINE(pp_luminadapt);
SHADER_DEFINE(pp_brightpass);
SHADER_DEFINE(pp_gaussianblurv);
SHADER_DEFINE(pp_gaussianblurh);
SHADER_DEFINE(pp_tonemap);

//HBAO
SHADER_DEFINE(depth_linearize);
SHADER_DEFINE(hbao_calc_blur);
SHADER_DEFINE(hbao_calc_blur_fog);
SHADER_DEFINE(hbao_blur);
SHADER_DEFINE(hbao_blur2);

SHADER_DEFINE(depth_clear);
SHADER_DEFINE(oitbuffer_clear);
SHADER_DEFINE(blit_oitblend);

SHADER_DEFINE(gamma_correction);

cvar_t *r_hdr = NULL;
cvar_t *r_hdr_debug = NULL;
MapConVar *r_hdr_blurwidth = NULL;
MapConVar *r_hdr_exposure = NULL;
MapConVar *r_hdr_darkness = NULL;
MapConVar *r_hdr_adaptation = NULL;

cvar_t *r_fxaa = NULL;

cvar_t *r_ssao = NULL;
cvar_t *r_ssao_debug = NULL;
MapConVar *r_ssao_radius = NULL;
MapConVar *r_ssao_intensity = NULL;
MapConVar *r_ssao_bias = NULL;
MapConVar *r_ssao_blur_sharpness = NULL;

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
		g_pMetaHookAPI->SysError("R_UseHudDebugProgram: Failed to load program!");
	}
}

void R_InitPostProcess(void)
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
	SHADER_UNIFORM(pp_fxaa, tex0, "tex0");
	SHADER_UNIFORM(pp_fxaa, rt_w, "rt_w");
	SHADER_UNIFORM(pp_fxaa, rt_h, "rt_h");

	//DownSample Pass
	pp_downsample.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\down_sample.frag.glsl", NULL);
	
	//2x2 Downsample Pass
	pp_downsample2x2.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\down_sample.frag.glsl", "", "#define DOWNSAMPLE_2X2\n", NULL);
	SHADER_UNIFORM(pp_downsample2x2, texelsize, "texelsize");

	//Luminance Downsample Pass
	pp_lumindown.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_lumpass.frag.glsl", "", "", NULL);
	SHADER_UNIFORM(pp_lumindown, texelsize, "texelsize");

	//Log Luminance Downsample Pass
	pp_luminlog.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_lumpass.frag.glsl", "", "#define LUMPASS_LOG\n", NULL);
	SHADER_UNIFORM(pp_luminlog, texelsize, "texelsize");

	//Exp Luminance Downsample Pass
	pp_luminexp.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_lumpass.frag.glsl", "", "#define LUMPASS_EXP\n", NULL);
	SHADER_UNIFORM(pp_luminexp, texelsize, "texelsize");

	//Luminance Adaptation Downsample Pass
	pp_luminadapt.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_adaption.frag.glsl", "", "", NULL);
	SHADER_UNIFORM(pp_luminadapt, frametime, "frametime");

	//Bright Pass
	pp_brightpass.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_brightpass.frag.glsl", "#define LUMTEX_ENABLED\n", "", NULL);
	SHADER_UNIFORM(pp_brightpass, baseTex, "baseTex");
	SHADER_UNIFORM(pp_brightpass, lumTex, "lumTex");

	//Tone mapping
	pp_tonemap.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hdr_tonemap.frag.glsl", "#define LUMTEX_ENABLED\n#define TONEMAP_ENABLED\n", "", NULL);
	SHADER_UNIFORM(pp_tonemap, baseTex, "baseTex");
	SHADER_UNIFORM(pp_tonemap, blurTex, "blurTex");
	SHADER_UNIFORM(pp_tonemap, lumTex, "lumTex");
	SHADER_UNIFORM(pp_tonemap, blurfactor, "blurfactor");
	SHADER_UNIFORM(pp_tonemap, exposure, "exposure");
	SHADER_UNIFORM(pp_tonemap, darkness, "darkness");

	//SSAO
	depth_linearize.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\depthlinearize.frag.glsl", NULL);

	hbao_calc_blur.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao.frag.glsl", NULL);
	
	depth_clear.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\depthclear.frag.glsl", NULL);

	if (bUseOITBlend)
	{
		oitbuffer_clear.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\oitbuffer_clear.frag.glsl", NULL);

		blit_oitblend.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\blit_oitblend.frag.glsl", NULL);
	}

	gamma_correction.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\gamma_correction.frag.glsl", NULL);

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

	hbao_calc_blur_fog.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao.frag.glsl", 
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

	hbao_blur.program = R_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao_blur.frag.glsl", NULL);

	hbao_blur2.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\hbao_blur.frag.glsl",
		"#define AO_BLUR_PRESENT\n", "#define AO_BLUR_PRESENT\n", NULL);

	pp_gaussianblurh.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\gaussian_blur_16x.frag.glsl", "", "#define BLUR_HORIZONAL\n", NULL);
	
	pp_gaussianblurv.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\gaussian_blur_16x.frag.glsl", "", "#define BLUR_VERTICAL\n", NULL);

	r_hdr = gEngfuncs.pfnRegisterVariable("r_hdr", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_debug = gEngfuncs.pfnRegisterVariable("r_hdr_debug", "0", FCVAR_CLIENTDLL);
	r_hdr_blurwidth = R_RegisterMapCvar("r_hdr_blurwidth", "0.075", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_exposure = R_RegisterMapCvar("r_hdr_exposure", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_darkness = R_RegisterMapCvar("r_hdr_darkness", "1.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_hdr_adaptation = R_RegisterMapCvar("r_hdr_adaptation", "50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_fxaa = gEngfuncs.pfnRegisterVariable("r_fxaa", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_ssao = gEngfuncs.pfnRegisterVariable("r_ssao", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_debug = gEngfuncs.pfnRegisterVariable("r_ssao_debug", "0",  FCVAR_CLIENTDLL);
	r_ssao_radius = R_RegisterMapCvar("r_ssao_radius", "30.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_intensity = R_RegisterMapCvar("r_ssao_intensity", "3.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_bias = R_RegisterMapCvar("r_ssao_bias", "0.2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssao_blur_sharpness = R_RegisterMapCvar("r_ssao_blur_sharpness", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	last_luminance = 0;
}

void R_ShutdownPostProcess(void)
{

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

void GL_BlitFrameFufferToScreen(FBO_Container_t *src)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, glwidth, glheight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void GL_BlitFrameBufferToFrameBufferColorOnly(FBO_Container_t *src, FBO_Container_t *dst)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void GL_BlitFrameBufferToFrameBufferColorDepth(FBO_Container_t *src, FBO_Container_t *dst)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	glBlitFramebuffer(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void R_DownSample(FBO_Container_t *src, FBO_Container_t *dst, qboolean filter2x2)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if(filter2x2)
	{
		GL_UseProgram(pp_downsample2x2.program);
		glUniform2f(pp_downsample2x2.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else
	{
		GL_UseProgram(pp_downsample.program);
	}

	glViewport(glx, gly, dst->iWidth, dst->iHeight);

	GL_Bind(src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void R_LuminPass(FBO_Container_t *src, FBO_Container_t *dst, int type)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	if(type == LUMPASS_LOG)
	{
		GL_UseProgram(pp_luminlog.program);
		glUniform2f(pp_luminlog.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else if(type == LUMPASS_EXP)
	{
		GL_UseProgram(pp_luminexp.program);
		glUniform2f(pp_luminexp.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}
	else
	{
		GL_UseProgram(pp_lumindown.program);
		glUniform2f(pp_lumindown.texelsize, 2.0f / src->iWidth, 2.0f / src->iHeight);
	}

	glViewport(glx, gly, dst->iWidth, dst->iHeight);

	GL_Bind(src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void R_LuminAdaptation(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *ada, double frametime)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_UseProgram(pp_luminadapt.program);
	glUniform1f(pp_luminadapt.frametime, frametime * clamp(r_hdr_adaptation->GetValue(), 0.1, 100));

	glViewport(glx, gly, dst->iWidth, dst->iHeight);

	GL_SelectTexture(GL_TEXTURE0);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(ada->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_DisableMultitexture();
}

void R_BrightPass(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *lum)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_UseProgram(pp_brightpass.program);
	glUniform1i(pp_brightpass.baseTex, 0);
	glUniform1i(pp_brightpass.lumTex, 1);

	glViewport(glx, gly, dst->iWidth, dst->iHeight);

	GL_SelectTexture(GL_TEXTURE0);
	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(lum->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_DisableMultitexture();
}

void R_BlurPass(FBO_Container_t *src, FBO_Container_t *dst, qboolean vertical)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	if(vertical)
	{
		GL_UseProgram(pp_gaussianblurv.program);
		glUniform1f(0, 1.0f / src->iHeight); 
	}
	else
	{
		GL_UseProgram(pp_gaussianblurh.program);
		glUniform1f(0, 1.0f / src->iWidth);
	}

	glViewport(glx, gly, dst->iWidth, dst->iHeight);

	GL_Bind(src->s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void R_BrightAccum(FBO_Container_t *blur1, FBO_Container_t *blur2, FBO_Container_t *blur3, FBO_Container_t *dst)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	GL_UseProgram(pp_downsample.program);
	
	glViewport(glx, gly, dst->iWidth, dst->iHeight);

	GL_Bind(blur1->s_hBackBufferTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_Bind(blur2->s_hBackBufferTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_Bind(blur3->s_hBackBufferTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisable(GL_BLEND);
}

void R_ToneMapping(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *blur, FBO_Container_t *lum)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_UseProgram(pp_tonemap.program);
	glUniform1i(pp_tonemap.baseTex, 0);
	glUniform1i(pp_tonemap.blurTex, 1);
	glUniform1i(pp_tonemap.lumTex, 2);
	glUniform1f(pp_tonemap.blurfactor, clamp(r_hdr_blurwidth->GetValue(), 0, 1));
	glUniform1f(pp_tonemap.exposure, clamp(r_hdr_exposure->GetValue(), 0.001, 10));
	glUniform1f(pp_tonemap.darkness, clamp(r_hdr_darkness->GetValue(), 0.001, 10));

	GL_Bind(src->s_hBackBufferTex);

	GL_EnableMultitexture();
	GL_Bind(blur->s_hBackBufferTex);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, lum->s_hBackBufferTex);

	glViewport(glx, gly, dst->iWidth, dst->iHeight);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	GL_DisableMultitexture();
}

void R_HDR(void)
{
	static glprofile_t profile_DoHDR;
	GL_BeginProfile(&profile_DoHDR, "R_HDR");

	GL_BeginFullScreenQuad(false);

	GL_DisableMultitexture();
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);

	//Downsample backbuffer
	R_DownSample(&s_BackBufferFBO, &s_DownSampleFBO[0], false);//(1->1/4)
	R_DownSample(&s_DownSampleFBO[0], &s_DownSampleFBO[1], false);//(1/4)->(1/16)

	//Log Luminance DownSample from .. (RGB16F to R32F)
	R_LuminPass(&s_DownSampleFBO[1], &s_LuminFBO[0], LUMPASS_LOG);//(1/16)->64x64

	//Luminance DownSample from..
	R_LuminPass(&s_LuminFBO[0], &s_LuminFBO[1], LUMPASS_DOWN);//64x64->16x16
	R_LuminPass(&s_LuminFBO[1], &s_LuminFBO[2], LUMPASS_DOWN);//16x16->4x4

	//exp Luminance DownSample from..
	R_LuminPass(&s_LuminFBO[2], &s_Lumin1x1FBO[2], LUMPASS_EXP);//4x4->1x1

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

	GL_UseProgram(0);

	GL_EndFullScreenQuad();

	GL_BlitFrameBufferToFrameBufferColorOnly(&s_ToneMapFBO, &s_BackBufferFBO);

	GL_EndProfile(&profile_DoHDR);
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

	static glprofile_t profile_DoFXAA;
	GL_BeginProfile(&profile_DoFXAA, "R_DoFXAA");

	GL_PushDrawState();
	GL_PushMatrix();

	GL_BlitFrameBufferToFrameBufferColorOnly(&s_BackBufferFBO, &s_BackBufferFBO2);

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

	GL_EndProfile(&profile_DoFXAA);
}

void R_GammaCorrection(void)
{
	static glprofile_t profile_GammaCorrection;
	GL_BeginProfile(&profile_GammaCorrection, "R_GammaCorrection");

	GL_BlitFrameBufferToFrameBufferColorOnly(&s_BackBufferFBO, &s_BackBufferFBO2);

	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	GL_BeginFullScreenQuad(false);
	glDisable(GL_BLEND);

	GL_UseProgram(gamma_correction.program);

	GL_Bind(s_BackBufferFBO2.s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_UseProgram(0);

	GL_EndFullScreenQuad();

	GL_EndProfile(&profile_GammaCorrection);
}

void R_ClearOITBuffer(void)
{
	GL_BeginFullScreenQuad(false);

	GL_UseProgram(oitbuffer_clear.program);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_UseProgram(0);

	GL_EndFullScreenQuad();

	GLuint val = 0;
	glClearNamedBufferData(r_wsurf.hOITAtomicSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, (const void*)&val);
}

void R_BlendOITBuffer(void)
{
	GL_BlitFrameBufferToFrameBufferColorOnly(&s_BackBufferFBO, &s_BackBufferFBO2);

	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	
	GL_BeginFullScreenQuad(false);

	glDisable(GL_BLEND);

	GL_UseProgram(blit_oitblend.program);	

	GL_Bind(s_BackBufferFBO2.s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_UseProgram(0);

	GL_EndFullScreenQuad();
}

void R_LinearizeDepth(FBO_Container_t *fbo)
{
	glBindFramebuffer(GL_FRAMEBUFFER, s_DepthLinearFBO.s_hBackBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glDisable(GL_BLEND);

	GL_UseProgram(depth_linearize.program);

	glUniform4f(0, r_znear * r_zfar, r_znear - r_zfar, r_zfar, r_ortho ? 0 : 1);
	
	GL_Bind(fbo->s_hBackBufferDepthTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool R_IsSSAOEnabled(void)
{
	if (!r_ssao->value)
		return false;

	if ((*r_refdef.onlyClientDraws) || r_draw_pass || g_SvEngine_DrawPortalView)
		return false;

	if (r_xfov < 75 || r_yfov < 75)
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	return true;
}

void R_AmbientOcclusion(void)
{
	//Prepare parameters
	static glprofile_t profile_AmbientOcclusion;
	GL_BeginProfile(&profile_AmbientOcclusion, "R_AmbientOcclusion");

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
	float R = r_ssao_radius->GetValue() * meters2viewspace;
	auto R2 = R * R;
	auto NegInvR2 = -1.0f / R2;
	auto RadiusToScreen = R * 0.5f * projScale;

	// ao
	auto PowExponent = max(r_ssao_intensity->GetValue(), 0.0f);
	auto NDotVBias = min(max(0.0f, r_ssao_bias->GetValue()), 1.0f);
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

	glBindFramebuffer(GL_FRAMEBUFFER, s_HBAOCalcFBO.s_hBackBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glDisable(GL_BLEND);

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
	GL_Bind(s_DepthLinearFBO.s_hBackBufferTex);

	//Texture unit 1 = random texture
	GL_EnableMultitexture();
	GL_Bind(hbao_randomview[0]);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Disable texture unit 1
	GL_DisableMultitexture();

	//SSAO blur stage

	//Write to HBAO texture2
	glDrawBuffer(GL_COLOR_ATTACHMENT1);

	GL_UseProgram(hbao_blur.program);
	glUniform1f(0, r_ssao_blur_sharpness->GetValue() / meters2viewspace);
	glUniform2f(1, 1.0f / float(glwidth), 0);

	//Texture unit 0 = calc
	GL_Bind(s_HBAOCalcFBO.s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Write to main framebuffer or GBuffer lightmap channel
	if (drawgbuffer)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		//Should we reset drawbuffer?
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	//0 for occluded color, 1 for non-occluded color
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	//Only draw on brush surfaces
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glStencilFunc(GL_EQUAL, 0, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_UseProgram(hbao_blur2.program);
	glUniform1f(0, r_ssao_blur_sharpness->GetValue() / meters2viewspace);
	glUniform2f(1, 0, 1.0f / float(glheight));

	//Texture unit 0 = calc2
	GL_Bind(s_HBAOCalcFBO.s_hBackBufferTex2);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GL_UseProgram(0);

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);

	GL_EndProfile(&profile_AmbientOcclusion);
}
