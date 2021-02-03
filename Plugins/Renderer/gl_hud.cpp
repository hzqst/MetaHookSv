#include "gl_local.h"

qboolean drawhudinworld = false;
qboolean draw3dhud = false;
int last_luminance = 0;

SHADER_DEFINE(hud_drawcolormask);
SHADER_DEFINE(hud_drawhudmask);
SHADER_DEFINE(hud_drawroundrect);
SHADER_DEFINE(pp_fxaa);

//HDR uniform
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

cvar_t *r_hdr = NULL;
cvar_t *r_hdr_blurwidth = NULL;
cvar_t *r_hdr_exposure = NULL;
cvar_t *r_hdr_darkness = NULL;
cvar_t *r_hdr_adaptation = NULL;
cvar_t *r_hudinworld_debug = NULL;
cvar_t *r_hdr_debug = NULL;

int r_hudinworld_texture = 0;

void R_InitRefHUD(void)
{
	if(gl_shader_support)
	{
		//FXAA Pass
		char *pp_fxaa_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_fxaa.vsh", 5, 0);
		char *pp_fxaa_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_fxaa.fsh", 5, 0);
		if(pp_fxaa_vscode && pp_fxaa_fscode)
		{
			pp_fxaa.program = R_CompileShader(pp_fxaa_vscode, pp_fxaa_fscode, "pp_fxaa.vsh", "pp_fxaa.fsh");
			if(pp_fxaa.program)
			{
				SHADER_UNIFORM(pp_fxaa, tex0, "tex0");
				SHADER_UNIFORM(pp_fxaa, rt_w, "rt_w");
				SHADER_UNIFORM(pp_fxaa, rt_h, "rt_h");
			}
			gEngfuncs.COM_FreeFile(pp_fxaa_vscode);
			gEngfuncs.COM_FreeFile(pp_fxaa_fscode);
		}
		if (!pp_fxaa_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_fxaa.vsh\" not found!");
		}
		if (!pp_fxaa_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_fxaa.fsh\" not found!");
		}

		//DownSample Pass
		char *pp_common_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_common.vsh", 5, 0);
		char *pp_downsample_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_downsample.fsh", 5, 0);
		if(pp_common_vscode && pp_downsample_fscode)
		{
			pp_downsample.program = R_CompileShader(pp_common_vscode, pp_downsample_fscode, "pp_common.vsh", "pp_downsample.fsh");
			if(pp_downsample.program)
			{
				SHADER_UNIFORM(pp_downsample, tex, "tex");
			}
			gEngfuncs.COM_FreeFile(pp_downsample_fscode);
		}
		if (!pp_common_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_common.vsh\" not found!");
		}
		if (!pp_downsample_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_downsample.fsh\" not found!");
		}

		//2x2 Downsample Pass
		char *pp_common2x2_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_common2x2.vsh", 5, 0);
		char *pp_downsample2x2_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_downsample2x2.fsh", 5, 0);
		if(pp_common2x2_vscode && pp_downsample2x2_fscode)
		{
			pp_downsample2x2.program = R_CompileShader(pp_common2x2_vscode, pp_downsample2x2_fscode, "pp_common2x2.vsh", "pp_downsample2x2.fsh");
			if(pp_downsample2x2.program)
			{
				SHADER_UNIFORM(pp_downsample2x2, tex, "tex");
				SHADER_UNIFORM(pp_downsample2x2, texelsize, "texelsize");
			}
			gEngfuncs.COM_FreeFile(pp_downsample2x2_fscode);
		}	
		if (!pp_common2x2_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_common2x2.vsh\" not found!");
		}
		if (!pp_downsample2x2_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_downsample2x2.fsh\" not found!");
		}

		//Luminance Downsample Pass
		char *pp_lumin_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_lumin.fsh", 5, 0);
		if(pp_common2x2_vscode && pp_lumin_fscode)
		{
			pp_lumin.program = R_CompileShader(pp_common2x2_vscode, pp_lumin_fscode, "pp_common2x2.vsh", "pp_lumin.fsh");
			if(pp_lumin.program)
			{
				SHADER_UNIFORM(pp_lumin, tex, "tex");
				SHADER_UNIFORM(pp_lumin, texelsize, "texelsize");
			}
			gEngfuncs.COM_FreeFile(pp_lumin_fscode);
		}
		if (!pp_lumin_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_lumin.fsh\" not found!");
		}

		//Log Luminance Downsample Pass
		char *pp_luminlog_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_luminlog.fsh", 5, 0);
		if(pp_common2x2_vscode && pp_luminlog_fscode)
		{
			pp_luminlog.program = R_CompileShader(pp_common2x2_vscode, pp_luminlog_fscode, "pp_common2x2.vsh", "pp_luminlog.fsh");
			if(pp_luminlog.program)
			{
				SHADER_UNIFORM(pp_luminlog, tex, "tex");
				SHADER_UNIFORM(pp_luminlog, texelsize, "texelsize");
			}
			gEngfuncs.COM_FreeFile(pp_luminlog_fscode);
		}
		if (!pp_luminlog_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_luminlog.fsh\" not found!");
		}

		//Exp Luminance Downsample Pass
		char *pp_luminexp_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_luminexp.fsh", 5, 0);
		if(pp_common2x2_vscode && pp_luminexp_fscode)
		{
			pp_luminexp.program = R_CompileShader(pp_common2x2_vscode, pp_luminexp_fscode, "pp_common2x2.vsh", "pp_luminexp.fsh");
			if(pp_luminexp.program)
			{
				SHADER_UNIFORM(pp_luminexp, tex, "tex");
				SHADER_UNIFORM(pp_luminexp, texelsize, "texelsize");
			}
			gEngfuncs.COM_FreeFile(pp_luminexp_fscode);
		}
		if (!pp_luminexp_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_luminexp.fsh\" not found!");
		}

		//Luminance Adaptation Downsample Pass
		char *pp_luminadapt_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_luminadapt.fsh", 5, 0);
		if(pp_common_vscode && pp_luminadapt_fscode)
		{
			pp_luminadapt.program = R_CompileShader(pp_common_vscode, pp_luminadapt_fscode, "pp_common.vsh", "pp_luminadapt.fsh");
			if(pp_luminadapt.program)
			{
				SHADER_UNIFORM(pp_luminadapt, curtex, "curtex");
				SHADER_UNIFORM(pp_luminadapt, adatex, "adatex");
				SHADER_UNIFORM(pp_luminadapt, frametime, "frametime");
			}
			gEngfuncs.COM_FreeFile(pp_luminadapt_fscode);
		}
		if (!pp_luminadapt_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_luminadapt.fsh\" not found!");
		}

		//Bright Pass
		char *pp_brightpass_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_brightpass.vsh", 5, 0);
		char *pp_brightpass_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_brightpass.fsh", 5, 0);
		if(pp_brightpass_vscode && pp_brightpass_fscode)
		{
			pp_brightpass.program = R_CompileShader(pp_brightpass_vscode, pp_brightpass_fscode, "pp_brightpass.vsh", "pp_brightpass.fsh");
			if(pp_brightpass.program)
			{
				SHADER_UNIFORM(pp_brightpass, tex, "tex");
				SHADER_UNIFORM(pp_brightpass, lumtex, "lumtex");
			}
			gEngfuncs.COM_FreeFile(pp_brightpass_vscode);
			gEngfuncs.COM_FreeFile(pp_brightpass_fscode);
		}
		if (!pp_brightpass_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_brightpass.vsh\" not found!");
		}
		if (!pp_brightpass_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_brightpass.fsh\" not found!");
		}

		//Tone mapping
		char *pp_tonemap_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_tonemap.vsh", 5, 0);
		char *pp_tonemap_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\pp_tonemap.fsh", 5, 0);
		if(pp_tonemap_vscode && pp_tonemap_fscode)
		{
			pp_tonemap.program = R_CompileShader(pp_tonemap_vscode, pp_tonemap_fscode, "pp_tonemap.vsh", "pp_tonemap.fsh");
			if(pp_tonemap.program)
			{
				SHADER_UNIFORM(pp_tonemap, basetex, "basetex");
				SHADER_UNIFORM(pp_tonemap, blurtex, "blurtex");
				SHADER_UNIFORM(pp_tonemap, lumtex, "lumtex");
				SHADER_UNIFORM(pp_tonemap, blurfactor, "blurfactor");
				SHADER_UNIFORM(pp_tonemap, exposure, "exposure");
				SHADER_UNIFORM(pp_tonemap, darkness, "darkness");
				SHADER_UNIFORM(pp_tonemap, gamma, "gamma");
			}
			gEngfuncs.COM_FreeFile(pp_tonemap_vscode);
			gEngfuncs.COM_FreeFile(pp_tonemap_fscode);
		}
		if (!pp_tonemap_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_tonemap.vsh\" not found!");
		}
		if (!pp_tonemap_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\pp_tonemap.fsh\" not found!");
		}

		char *hud_drawroundrect_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\hud_drawroundrect.vsh", 5, 0);
		char *hud_drawroundrect_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\hud_drawroundrect.fsh", 5, 0);
		if(hud_drawroundrect_vscode && hud_drawroundrect_fscode)
		{
			hud_drawroundrect.program = R_CompileShader(hud_drawroundrect_vscode, hud_drawroundrect_fscode, "hud_drawroundrect.vsh", "hud_drawroundrect.fsh");
			if(hud_drawroundrect.program)
			{
				SHADER_UNIFORM(hud_drawroundrect, base, "base");
				SHADER_UNIFORM(hud_drawroundrect, center, "center");
				SHADER_UNIFORM(hud_drawroundrect, radius, "radius");
				SHADER_UNIFORM(hud_drawroundrect, blurdist, "blurdist");
			}
			gEngfuncs.COM_FreeFile(hud_drawroundrect_vscode);
			gEngfuncs.COM_FreeFile(hud_drawroundrect_fscode);
		}


		char *hud_drawhudmask_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\hud_drawhudmask.vsh", 5, 0);
		char *hud_drawhudmask_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\hud_drawhudmask.fsh", 5, 0);
		if(hud_drawhudmask_vscode && hud_drawhudmask_fscode)
		{
			hud_drawhudmask.program = R_CompileShader(hud_drawhudmask_vscode, hud_drawhudmask_fscode, "hud_drawhudmask.vsh", "hud_drawhudmask.fsh");
			if(hud_drawhudmask.program)
			{
				SHADER_UNIFORM(hud_drawhudmask, base, "base");
				SHADER_UNIFORM(hud_drawhudmask, src_col, "src_col");
			}
			gEngfuncs.COM_FreeFile(hud_drawhudmask_vscode);
			gEngfuncs.COM_FreeFile(hud_drawhudmask_fscode);
		}

		char *hud_drawcolormask_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\hud_drawcolormask.fsh", 5, 0);
		if(pp_common_vscode && hud_drawcolormask_fscode)
		{
			hud_drawcolormask.program = R_CompileShader(pp_common_vscode, hud_drawcolormask_fscode, "pp_common.vsh", "hud_drawcolormask.fsh");
			if(hud_drawcolormask.program)
			{
				SHADER_UNIFORM(hud_drawcolormask, base, "base");
				SHADER_UNIFORM(hud_drawcolormask, alpha_range, "alpha_range");
			}
			gEngfuncs.COM_FreeFile(hud_drawcolormask_fscode);
		}

		//gaussian blur code
		if(pp_common_vscode)
		{
			R_InitBlur(pp_common_vscode, 16);
			gEngfuncs.COM_FreeFile(pp_common_vscode);
		}
		if(pp_common2x2_vscode)
		{
			gEngfuncs.COM_FreeFile(pp_common2x2_vscode);
		}
	}
	r_hdr = gEngfuncs.pfnRegisterVariable("r_hdr", "1", FCVAR_ARCHIVE);
	r_hdr_blurwidth = gEngfuncs.pfnRegisterVariable("r_hdr_blurwidth", "0.1", FCVAR_ARCHIVE);
	r_hdr_exposure = gEngfuncs.pfnRegisterVariable("r_hdr_exposure", "5.0", FCVAR_ARCHIVE);
	r_hdr_darkness = gEngfuncs.pfnRegisterVariable("r_hdr_darkness", "3.5", FCVAR_ARCHIVE);
	r_hdr_adaptation = gEngfuncs.pfnRegisterVariable("r_hdr_adaptation", "50.0", FCVAR_ARCHIVE);
	r_hudinworld_debug = gEngfuncs.pfnRegisterVariable("r_hudinworld_debug", "0", FCVAR_ARCHIVE);
	r_hdr_debug = gEngfuncs.pfnRegisterVariable("r_hdr_debug", "0", FCVAR_ARCHIVE);

	last_luminance = 0;

	if(bDoHDR)
	{
		if(!pp_downsample.program)
			bDoHDR = false;
		else if(!pp_downsample2x2.program)
			bDoHDR = false;
		else if(!pp_luminlog.program)
			bDoHDR = false;
		else if(!pp_lumin.program)
			bDoHDR = false;
		else if(!pp_luminexp.program)
			bDoHDR = false;
		else if(!pp_luminadapt.program)
			bDoHDR = false;
		else if(!pp_brightpass.program)
			bDoHDR = false;
		else if(!pp_gaussianblurv.program)
			bDoHDR = false;
		else if(!pp_gaussianblurh.program)
			bDoHDR = false;
		else if(!pp_tonemap.program)
			bDoHDR = false;
	}
}

float *R_GenerateGaussianWeights(int kernelRadius)
{
	int size = kernelRadius * 2 + 1;

	float x;
	float s        = floor(kernelRadius / 4.0f);  
	float *weights = new float [size];

	float sum = 0.0f;
	for(int i = 0; i < size; i++)
	{
		x          = (float)(i - kernelRadius);

		// True Gaussian
		weights[i] = expf(-x*x/(2.0f*s*s)) / (s*sqrtf(2.0f*M_PI));

		// This sum of exps is not really a separable kernel but produces a very interesting star-shaped effect
		//weights[i] = expf( -0.0625f * x * x ) + 2 * expf( -0.25f * x * x ) + 4 * expf( - x * x ) + 8 * expf( - 4.0f * x * x ) + 16 * expf( - 16.0f * x * x ) ;
		sum += weights[i];
	}

	for(int i = 0; i < size; i++)
		weights[i] /= sum;

	return weights;
}

void R_CaculateGaussianBilinear(float *coordOffset, float *gaussWeight, int maxSamples )
{
    int i=0;

	//  store all the intermediate offsets & weights, then compute the bilinear
    //  taps in a second pass
    float *tmpWeightArray = R_GenerateGaussianWeights( maxSamples );

    // Bilinear filtering taps 
    // Ordering is left to right.
    float sScale;
    float sFrac;

    for( i = 0; i < maxSamples; i++ )
    {
        sScale = tmpWeightArray[i*2 + 0] + tmpWeightArray[i*2 + 1];
        sFrac  = tmpWeightArray[i*2 + 1] / sScale;

        coordOffset[i] = ( (2.0f*i - maxSamples) + sFrac );
        gaussWeight[i] = sScale;
    }

    delete []tmpWeightArray;
}

char *UTIL_VarArgs(char *format, ...);

void R_InitBlur(const char *vs_code, int samples)
{
	if(!bDoHDR)
		return;

	float coord_offsets[MAX_GAUSSIAN_SAMPLES];
	float gauss_weights[MAX_GAUSSIAN_SAMPLES];
	char code[4096];

	R_CaculateGaussianBilinear(coord_offsets, gauss_weights, min(samples, MAX_GAUSSIAN_SAMPLES));

	//Generate horizonal blur code
	if(pp_gaussianblurh.program)
	{
		qglDeleteObjectARB(pp_gaussianblurh.program);
		pp_gaussianblurh.program = 0;
	}

	sprintf(code, "#version 120\nuniform float du;\nuniform sampler2D tex;\nvoid main()\n{\n vec4 sample = vec4(0.0, 0.0, 0.0, 0.0);\n");
	for(int i = 0; i < samples; ++i)
	{
		strcat(code, UTIL_VarArgs(" sample += %f * texture2D( tex, gl_TexCoord[0].xy + vec2(%f * du, 0.0) );\n", gauss_weights[i], coord_offsets[i]));
	}
	strcat(code, " gl_FragColor = sample;\n}");

	pp_gaussianblurh.program = R_CompileShader(vs_code, code, "pp_common.vsh", "pp_gaussianblur_h.fsh");
	if(pp_gaussianblurh.program)
	{
		SHADER_UNIFORM(pp_gaussianblurh, tex, "tex");
		SHADER_UNIFORM(pp_gaussianblurh, du, "du");
	}

	//Generate vertical blur code
	if(pp_gaussianblurv.program)
	{
		qglDeleteObjectARB(pp_gaussianblurv.program);
		pp_gaussianblurv.program = 0;
	}
	
	sprintf(code, "#version 120\nuniform float du;\nuniform sampler2D tex;\nvoid main()\n{\n vec4 sample = vec4(0.0, 0.0, 0.0, 0.0);\n");
	for(int i = 0; i < samples; ++i)
	{
		strcat(code, UTIL_VarArgs(" sample += %f * texture2D( tex, gl_TexCoord[0].xy + vec2(0.0, %f * du) );\n", gauss_weights[i], coord_offsets[i]));
	}
	strcat(code, " gl_FragColor = sample;\n}");

	pp_gaussianblurv.program = R_CompileShader(vs_code, code, "pp_common.vsh", "pp_gaussianblur_v.fsh");
	if(pp_gaussianblurv.program)
	{
		SHADER_UNIFORM(pp_gaussianblurv, tex, "tex");
		SHADER_UNIFORM(pp_gaussianblurv, du, "du");
	}
}

void R_BeginHUDQuad(void)
{
	qglDisable(GL_BLEND);
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_ALPHA_TEST);
	qglDisable(GL_CULL_FACE);
	
	GL_SelectTexture(TEXTURE0_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglColor4f(1, 1, 1, 1);
}

void R_DrawHUDQuad_Texture(int tex, int w, int h)
{
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, w, h, 0, -1.875, 1.875);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglViewport(0, 0, w, h);

	GL_Bind(tex);
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

void R_BlitToScreen(FBO_Container_t *src)
{
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	qglClearColor(0.0, 1.0, 0.0, 0.25);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (bDoDirectBlit)
	{
		qglBlitFramebufferEXT(0, 0, src->iWidth, src->iHeight, 0, 0, glwidth, glheight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
	else
	{
		R_DrawHUDQuad_Texture(src->s_hBackBufferTex, glwidth, glheight);
	}
}

void R_BlitToFBO(FBO_Container_t *src, FBO_Container_t *dst)
{
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, src->s_hBackBufferFBO);

	qglClearColor(0.0, 1.0, 0.0, 0.25);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (bDoDirectBlit)
	{
		qglBlitFramebufferEXT(0, 0, src->iWidth, src->iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
	else
	{
		R_DrawHUDQuad_Texture(src->s_hBackBufferTex, dst->iWidth, dst->iHeight);
	}
}

void R_DownSample(FBO_Container_t *src, FBO_Container_t *dst, qboolean filter2x2)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0.0, 1.0, 0.0, 0.25);
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

	qglClearColor(0.0, 1.0, 0.0, 0.25);
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
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, dst->iTextureColorFormat, 0, 0, dst->iWidth, dst->iHeight, 0);
	}
}

void R_LuminAdaptation(FBO_Container_t *src, FBO_Container_t *dst, FBO_Container_t *ada, double frametime)
{
	if (dst->s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, dst->s_hBackBufferFBO);
	}

	qglClearColor(0.0, 1.0, 0.0, 0.25);
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

	qglClearColor(0.0, 1.0, 0.0, 0.25);
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

	qglClearColor(0.0, 1.0, 0.0, 0.25);
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

	qglClearColor(0.0, 0.0, 0.0, 0.25);
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

	qglClearColor(0.0, 1.0, 0.0, 0.25);
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

void R_SetupGL_3DHUD(void)
{
	/*qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();

	float fov_x = 90;
	float fov_y = CalcFov(fov_x, glheight*4/3, glheight);
	screenaspect = (float)4 / 3;
	MYgluPerspective(fov_y, screenaspect, 4, 8000);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglRotatef(-90, 1, 0, 0);
	qglRotatef(90, 0, 0, 1);
	qglRotatef(-r_refdef->viewangles[2], 1, 0, 0);
	qglRotatef(-r_refdef->viewangles[0], 0, 1, 0);
	qglRotatef(-r_refdef->viewangles[1], 0, 0, 1);
	qglTranslatef(-r_refdef->vieworg[0], -r_refdef->vieworg[1], -r_refdef->vieworg[2]);*/
}

void GLSetupHud(int w, int h)
{
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, w, h, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
}

void GLBeginHud(void)
{
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_CULL_FACE);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_NOTEQUAL, 0);

	qglColor4f(1, 1, 1, 1);
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	qglEnable(GL_TEXTURE_2D);
}

void GLEndHud(void)
{
	qglEnable(GL_CULL_FACE);
	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
	qglDepthMask(1);
}

int R_Get3DHUDTexture(void)
{
	return s_3DHUDFBO.s_hBackBufferTex;
}

void R_Draw3DHUDQuad(int x, int y, int left, int top)
{
	float texcoord[4];
	texcoord[0] = (float)(glwidth / 2 - left) / glwidth;
	texcoord[1] = (float)(glheight / 2 - top) / glheight;
	texcoord[2] = (float)(glwidth / 2 + left) / glwidth;
	texcoord[3] = (float)(glheight / 2 + top) / glheight;

	qglBindTexture(GL_TEXTURE_2D, s_3DHUDFBO.s_hBackBufferTex);
	qglBegin(GL_QUADS);
	qglTexCoord2f(texcoord[0], texcoord[3]);		
	qglVertex3f(x-left, y-top, 0);
	qglTexCoord2f(texcoord[2], texcoord[3]);
	qglVertex3f(x+left, y-top, 0);
	qglTexCoord2f(texcoord[2], texcoord[1]);
	qglVertex3f(x+left, y+top, 0);
	qglTexCoord2f(texcoord[0], texcoord[1]);
	qglVertex3f(x-left, y+top, 0);
	qglEnd();
}

void R_BeginDrawTrianglesInHUD_Direct(int x, int y)
{
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();

	R_SetupGL_3DHUD();

	GLEndHud();
	
	qglViewport(x-(glheight*4/3)/2, glheight/2-y, glheight*4/3, glheight);

	qglClear(GL_DEPTH_BUFFER_BIT);

	draw3dhud = true;
}

void R_BeginDrawTrianglesInHUD_FBO(int x, int y, int left, int top)
{
	qglEnable(GL_DEPTH_TEST);

	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_3DHUDFBO.s_hBackBufferFBO);

	qglClearColor(0.0, 0.0, 0.0, 0.0);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	qglBlitFramebufferEXT(x-left, glheight-(y-top), x+left, glheight-(y+top), glwidth/2-left, glheight-(glheight/2-top), glwidth/2+left, glheight-(glheight/2+top), GL_COLOR_BUFFER_BIT, GL_LINEAR);

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();

	R_SetupGL_3DHUD();

	GLEndHud();

	qglViewport((glwidth - glheight*4/3)/2, 0, glheight*4/3, glheight);

	draw3dhud = true;
}

void R_FinishDrawTrianglesInHUD(void)
{
	draw3dhud = false;

	GLBeginHud();
	qglViewport(0, 0, glwidth, glheight);

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();
}

void R_BeginDrawHUDInWorld(int texid, int w, int h)
{
	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_HUDInWorldFBO.s_hBackBufferFBO);

	r_hudinworld_texture = texid;

	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);
	
	qglClearColor(0.0, 0.0, 0.0, 0.0);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();

	GLSetupHud(w, h);
	GLBeginHud();
	qglViewport(0, 0, w, h);

	drawhudinworld = true;
}

void R_FinishDrawHUDInWorld(void)
{
	drawhudinworld = false;

	GLEndHud();
	qglViewport(0, 0, glwidth, glheight);

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();
}

void R_BeginDrawRoundRect(int centerX, int centerY, float radius, float blurdist)
{
	if(!hud_drawroundrect.program)
		return;

	qglUseProgramObjectARB(hud_drawroundrect.program);
	qglUniform1iARB(hud_drawroundrect.base, 0);
	qglUniform2fARB(hud_drawroundrect.center, (float)centerX, (float)centerY);
	qglUniform1fARB(hud_drawroundrect.radius, radius);
	qglUniform1fARB(hud_drawroundrect.blurdist, blurdist);
}

void R_BeginFXAA(int w, int h)
{
	qglUseProgramObjectARB(pp_fxaa.program);
	qglUniform1iARB(pp_fxaa.tex0, 0);
	qglUniform1fARB(pp_fxaa.rt_w, w);
	qglUniform1fARB(pp_fxaa.rt_h, h);
}

void R_BeginDrawHudMask(int r, int g, int b)
{
	if(!hud_drawhudmask.program)
		return;

	qglUseProgramObjectARB(hud_drawhudmask.program);
	qglUniform1iARB(hud_drawhudmask.base, 0);
	qglUniform3fARB(hud_drawhudmask.src_col, r / 255.0f, g / 255.0f, b / 255.0f);
}

void R_BeginDrawColorMask(int minAlpha, int maxAlpha)
{
	if(!hud_drawcolormask.program)
		return;

	qglUseProgramObjectARB(hud_drawcolormask.program);
	qglUniform1iARB(hud_drawcolormask.base, 0);
	qglUniform2fARB(hud_drawcolormask.offset, 1.0f / 256, 1.0f / 256);
	qglUniform2fARB(hud_drawcolormask.alpha_range, minAlpha / 255.0, maxAlpha / 255.0f);
}