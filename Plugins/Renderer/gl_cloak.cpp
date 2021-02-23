#include "gl_local.h"

int cloak_texture = 0;

SHADER_DEFINE(cloak);

SHADER_DEFINE(conc);

cvar_t *r_cloak_debug;

void R_InitCloak(void)
{
	/*if(gl_shader_support)
	{
		const char *cloak_vscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\cloak_shader.vsh", 5, 0);
		const char *cloak_fscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\cloak_shader.fsh", 5, 0);
		if(cloak_vscode && cloak_fscode)
		{
			cloak.program = R_CompileShader(cloak_vscode, cloak_fscode, "cloak_shader.vsh", "cloak_shader.fsh");
			if(cloak.program)
			{
				SHADER_UNIFORM(cloak, refract, "refract");
				SHADER_UNIFORM(cloak, eyepos, "eyepos");
				SHADER_UNIFORM(cloak, cloakfactor, "cloakfactor");
				SHADER_UNIFORM(cloak, refractamount, "refractamount");
			}
		}
		gEngfuncs.COM_FreeFile((void *)cloak_vscode);
		gEngfuncs.COM_FreeFile((void *)cloak_fscode);

		const char *conc_vscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\conc_shader.vsh", 5, 0);
		const char *conc_fscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\conc_shader.fsh", 5, 0);
		if(conc_vscode && conc_fscode)
		{
			conc.program = R_CompileShader(conc_vscode, conc_fscode, "conc_shader.vsh", "conc_shader.fsh");
			if(conc.program)
			{
				SHADER_UNIFORM(conc, refractmap, "refractmap");
				SHADER_UNIFORM(conc, normalmap, "normalmap");
				SHADER_UNIFORM(conc, packedfactor, "packedfactor");
			}
		}

		if(!conc_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\conc_shader.vsh\" not found!");
		}
		if (!conc_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\conc_shader.fsh\" not found!");
		}
		gEngfuncs.COM_FreeFile((void *)conc_vscode);
		gEngfuncs.COM_FreeFile((void *)conc_fscode);
	}*/

	if(!s_CloakFBO.s_hBackBufferFBO)
	{
		cloak_texture = GL_GenTexture();
		GL_Bind(cloak_texture);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glwidth, glheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}
	else
	{
		cloak_texture = s_CloakFBO.s_hBackBufferTex;
	}

	r_cloak_debug = gEngfuncs.pfnRegisterVariable("r_cloak_debug", "0", FCVAR_CLIENTDLL);
}

void R_RenderCloakTexture(void)
{
	if(s_CloakFBO.s_hBackBufferFBO && s_BackBufferFBO.s_hBackBufferFBO)
	{
		GL_PushFrameBuffer();

		if(s_MSAAFBO.s_hBackBufferFBO)
			qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
		else
			qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_CloakFBO.s_hBackBufferFBO);

		qglBlitFramebufferEXT(0, 0, glwidth, glheight, 0, 0, s_CloakFBO.iWidth, s_CloakFBO.iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		GL_PopFrameBuffer();
	}
	else
	{
		GL_Bind(cloak_texture);
		qglEnable(GL_TEXTURE_2D);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, glwidth, glheight);
	}
}

void R_BeginRenderConc(float flBlurFactor, float flRefractFactor)
{
	qglUseProgramObjectARB(conc.program);
	qglUniform1iARB(conc.normalmap, 0);
	qglUniform1iARB(conc.refractmap, 1);
	qglUniform2fARB(conc.packedfactor, flBlurFactor, flRefractFactor);
}

int R_GetCloakTexture(void)
{
	return cloak_texture;
}