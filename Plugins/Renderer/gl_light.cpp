#include "gl_local.h"

int r_light_env_color[4] = { 0 };
vec3_t r_light_env_angles = {0};
qboolean r_light_env_enabled = false;

cvar_t *r_light_dynamic = NULL;
cvar_t *r_light_debug = NULL;

cvar_t *r_light_ambient = NULL;
cvar_t *r_light_diffuse = NULL;
cvar_t *r_light_specular = NULL;
cvar_t *r_light_specularpow = NULL;

cvar_t *r_flashlight_distance = NULL;
cvar_t *r_flashlight_cone = NULL;

bool drawpolynocolor = false;
bool drawgbuffer = false;

SHADER_DEFINE(gbuffer1);
SHADER_DEFINE(gbuffer2);
SHADER_DEFINE(gbuffer3);
SHADER_DEFINE(dlight_spot);
SHADER_DEFINE(dlight_point);
SHADER_DEFINE(dlight_final);

void R_InitLight(void)
{
	if (gl_shader_support)
	{
		char *gbuffer_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\gbuffer_shader.vsh", 5, 0);
		char *gbuffer_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\gbuffer_shader.fsh", 5, 0);

		if (!gbuffer_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\gbuffer_shader.vsh\" not found!");
		}
		if (!gbuffer_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\gbuffer_shader.fsh\" not found!");
		}

		if (gbuffer_vscode && gbuffer_fscode)
		{
			gbuffer1.program = R_CompileShaderEx(gbuffer_vscode, gbuffer_fscode, "gbuffer_shader.vsh", "gbuffer_shader.fsh", "", "");
			if (gbuffer1.program)
			{
				SHADER_UNIFORM(gbuffer1, diffuseTex, "diffuseTex");
			}
			gbuffer2.program = R_CompileShaderEx(gbuffer_vscode, gbuffer_fscode, "gbuffer_shader.vsh", "gbuffer_shader.fsh",
				"#define LIGHTMAP_ENABLED", "#define LIGHTMAP_ENABLED");
			if (gbuffer2.program)
			{
				SHADER_UNIFORM(gbuffer2, diffuseTex, "diffuseTex");
				SHADER_UNIFORM(gbuffer2, lightmapTex, "lightmapTex");
			}
			gbuffer3.program = R_CompileShaderEx(gbuffer_vscode, gbuffer_fscode, "gbuffer_shader.vsh", "gbuffer_shader.fsh",
				"#define LIGHTMAP_ENABLED\n#define DETAILTEXTURE_ENABLED", "#define LIGHTMAP_ENABLED\n#define DETAILTEXTURE_ENABLED");
			if (gbuffer3.program)
			{
				SHADER_UNIFORM(gbuffer3, diffuseTex, "diffuseTex");
				SHADER_UNIFORM(gbuffer3, lightmapTex, "lightmapTex");
				SHADER_UNIFORM(gbuffer3, detailTex, "detailTex");
			}
			gEngfuncs.COM_FreeFile((void *)gbuffer_vscode);
			gEngfuncs.COM_FreeFile((void *)gbuffer_fscode);
		}
	}

	if (gl_shader_support)
	{
		char *dlight_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\dlight_shader.vsh", 5, 0);
		char *dlight_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\dlight_shader.fsh", 5, 0);

		if (!dlight_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\dlight_shader.vsh\" not found!");
		}
		if (!dlight_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\dlight_shader.fsh\" not found!");
		}

		if (dlight_vscode && dlight_fscode)
		{
			dlight_spot.program = R_CompileShaderEx(dlight_vscode, dlight_fscode, "dlight_shader.vsh", "dlight_shader.fsh", 
				"#define LIGHT_PASS", "#define LIGHT_PASS\n#define LIGHT_PASS_SPOT");
			if (dlight_spot.program)
			{
				SHADER_UNIFORM(dlight_spot, positionTex, "positionTex");
				SHADER_UNIFORM(dlight_spot, normalTex, "normalTex");
				SHADER_UNIFORM(dlight_spot, viewpos, "viewpos");
				SHADER_UNIFORM(dlight_spot, lightdir, "lightdir");
				SHADER_UNIFORM(dlight_spot, lightpos, "lightpos");
				SHADER_UNIFORM(dlight_spot, lightcolor, "lightcolor");
				SHADER_UNIFORM(dlight_spot, lightcone, "lightcone");
				SHADER_UNIFORM(dlight_spot, lightradius, "lightradius");
				SHADER_UNIFORM(dlight_spot, lightambient, "lightambient");
				SHADER_UNIFORM(dlight_spot, lightdiffuse, "lightdiffuse");
				SHADER_UNIFORM(dlight_spot, lightspecular, "lightspecular");
				SHADER_UNIFORM(dlight_spot, lightspecularpow, "lightspecularpow");
			}

			dlight_point.program = R_CompileShaderEx(dlight_vscode, dlight_fscode, "dlight_shader.vsh", "dlight_shader.fsh",
				"#define LIGHT_PASS", "#define LIGHT_PASS\n#define LIGHT_PASS_POINT");
			if (dlight_point.program)
			{
				SHADER_UNIFORM(dlight_point, positionTex, "positionTex");
				SHADER_UNIFORM(dlight_point, normalTex, "normalTex");
				SHADER_UNIFORM(dlight_point, viewpos, "viewpos");
				SHADER_UNIFORM(dlight_point, lightpos, "lightpos");
				SHADER_UNIFORM(dlight_point, lightcolor, "lightcolor");
				SHADER_UNIFORM(dlight_point, lightradius, "lightradius");
				SHADER_UNIFORM(dlight_point, lightambient, "lightambient");
				SHADER_UNIFORM(dlight_point, lightdiffuse, "lightdiffuse");
				SHADER_UNIFORM(dlight_point, lightspecular, "lightspecular");
				SHADER_UNIFORM(dlight_point, lightspecularpow, "lightspecularpow");
			}

			dlight_final.program = R_CompileShaderEx(dlight_vscode, dlight_fscode, "dlight_shader.vsh", "dlight_shader.fsh", 
				"#define FINAL_PASS", "#define FINAL_PASS");
			if (dlight_final.program)
			{
				SHADER_UNIFORM(dlight_final, diffuseTex, "diffuseTex");
				SHADER_UNIFORM(dlight_final, lightmapTex, "lightmapTex");
				SHADER_UNIFORM(dlight_final, depthTex, "depthTex");
			}
			gEngfuncs.COM_FreeFile((void *)dlight_vscode);
			gEngfuncs.COM_FreeFile((void *)dlight_fscode);
		}
	}

	r_light_env_color[0] = 0;
	r_light_env_color[1] = 0;
	r_light_env_color[2] = 0;
	r_light_env_color[3] = 0;

	r_light_env_angles[0] = 90;
	r_light_env_angles[1] = 0;
	r_light_env_angles[2] = 0;

	r_light_env_enabled = false;

	r_light_dynamic = gEngfuncs.pfnRegisterVariable("r_light_dynamic", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_debug = gEngfuncs.pfnRegisterVariable("r_light_debug", "0", FCVAR_CLIENTDLL);

	r_light_ambient = gEngfuncs.pfnRegisterVariable("r_light_ambient", "0.35", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_diffuse = gEngfuncs.pfnRegisterVariable("r_light_diffuse", "0.35", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_specular = gEngfuncs.pfnRegisterVariable("r_light_specular", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_specularpow = gEngfuncs.pfnRegisterVariable("r_light_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_flashlight_distance = gEngfuncs.pfnRegisterVariable("r_flashlight_distance", "2000", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_cone = gEngfuncs.pfnRegisterVariable("r_flashlight_cone", "0.9", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	
	drawpolynocolor = false;
	drawgbuffer = false;
}

bool R_IsDLightFlashlight(dlight_t *dl)
{
	if (dl->key >= 1 && dl->key <= 32)
	{
		auto ent = gEngfuncs.GetEntityByIndex(dl->key);

		if (ent->curstate.effects & EF_DIMLIGHT)
		{
			return true;
		}
	}

	return false;
}

void R_SetRenderGBufferDecal(void)
{
	if (!drawgbuffer)
	{
		return;
	}
	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	qglDrawBuffers(1, attachments);
}

void R_SetRenderGBufferWater(void)
{
	if (!drawgbuffer)
	{
		return;
	}

	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	qglDrawBuffers(3, attachments);
}

void R_SetRenderGBufferAll(void)
{
	if (!drawgbuffer)
	{
		return;
	}

	GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	qglDrawBuffers(4, attachments);
}

void R_SetGBufferRenderState(int state)
{
	if (!drawgbuffer)
	{
		return;
	}

	//diffuse only, use glColor as lightmapColor
	if (state == 1)
	{
		qglUseProgramObjectARB(gbuffer1.program);
		qglUniform1iARB(gbuffer1.diffuseTex, 0);
	}
	//lightmap
	else if (state == 2)
	{
		qglUseProgramObjectARB(gbuffer2.program);
		qglUniform1iARB(gbuffer2.diffuseTex, 0);
		qglUniform1iARB(gbuffer2.lightmapTex, 1);
	}
	//lightmap + detailtexture
	else if (state == 3)
	{		
		qglUseProgramObjectARB(gbuffer3.program);
		qglUniform1iARB(gbuffer3.diffuseTex, 0);
		qglUniform1iARB(gbuffer3.lightmapTex, 1);
		qglUniform1iARB(gbuffer3.detailTex, 2);
	}
}

void R_BeginRenderGBuffer(void)
{
	if (drawrefract || drawreflect)
		return;

	if (!r_light_dynamic->value)
		return;

	drawgbuffer = true;

	GL_PushFrameBuffer();

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);

	R_SetGBufferRenderState(2);
	R_SetRenderGBufferAll();

	qglClearColor(0, 0, 0, 1);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void R_EndRenderGBuffer(void)
{
	if (!drawgbuffer)
		return;

	drawgbuffer = false;

	GL_PushDrawState();
	GL_PushMatrix();

	//Write to GBuffer->lightmap only
	qglDrawBuffer(GL_COLOR_ATTACHMENT1);

	R_BeginHUDQuad();

	//Begin light pass
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);

	//Position texture
	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(s_GBufferFBO.s_hBackBufferTex3);

	//Normal texture
	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferTex4);

	int max_dlight;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		max_dlight = 256;
	}
	else
	{
		max_dlight = 32;
	}

	dlight_t *dl = cl_dlights;

	for (int i = 0; i < max_dlight; i++, dl++)
	{
		if (dl->die < (*cl_time) || !dl->radius)
			continue;

		if (R_IsDLightFlashlight(dl))
		{
			vec3_t dlight_origin;
			vec3_t dlight_angle;
			vec3_t dlight_vforward;
			vec3_t dlight_vright;
			vec3_t dlight_vup;

			//Spot Light
			auto ent = gEngfuncs.GetEntityByIndex(dl->key);

			vec3_t org;
			if (ent == gEngfuncs.GetLocalPlayer())
			{
				VectorCopy(r_refdef->viewangles, dlight_angle);
				gEngfuncs.pfnAngleVectors(dlight_angle, dlight_vforward, dlight_vright, dlight_vup);

				VectorCopy(r_refdef->vieworg, org);
				VectorMA(org, 8, dlight_vup, org);
				VectorMA(org, 10, dlight_vright, org);

				VectorCopy(org, dlight_origin);
			}
			else
			{
				VectorCopy(ent->angles, dlight_angle);
				gEngfuncs.pfnAngleVectors(dlight_angle, dlight_vforward, dlight_vright, dlight_vup);

				VectorCopy(ent->origin, org);
				VectorMA(org, 8, dlight_vup, org);
				VectorMA(org, 10, dlight_vright, org);

				VectorCopy(org, dlight_origin);
			}

			qglUseProgramObjectARB(dlight_spot.program);
			qglUniform1iARB(dlight_spot.positionTex, 0);
			qglUniform1iARB(dlight_spot.normalTex, 1);
			qglUniform3fARB(dlight_spot.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
			qglUniform3fARB(dlight_spot.lightdir, dlight_vforward[0], dlight_vforward[1], dlight_vforward[2]);
			qglUniform3fARB(dlight_spot.lightpos, dlight_origin[0], dlight_origin[1], dlight_origin[2]);
			qglUniform3fARB(dlight_spot.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
			qglUniform1fARB(dlight_spot.lightcone, r_flashlight_cone->value);
			qglUniform1fARB(dlight_spot.lightradius, r_flashlight_distance->value);
			qglUniform1fARB(dlight_spot.lightambient, r_light_ambient->value);
			qglUniform1fARB(dlight_spot.lightdiffuse, r_light_diffuse->value);
			qglUniform1fARB(dlight_spot.lightspecular, r_light_specular->value);
			qglUniform1fARB(dlight_spot.lightspecularpow, r_light_specularpow->value);

			R_DrawHUDQuad(glwidth, glheight);
		}
		else
		{
			vec3_t dlight_origin;

			//Point Light
			VectorCopy(dl->origin, dlight_origin);

			qglUseProgramObjectARB(dlight_point.program);
			qglUniform1iARB(dlight_point.positionTex, 0);
			qglUniform1iARB(dlight_point.normalTex, 1);
			qglUniform3fARB(dlight_point.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
			qglUniform3fARB(dlight_point.lightpos, dlight_origin[0], dlight_origin[1], dlight_origin[2]);
			qglUniform3fARB(dlight_point.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
			qglUniform1fARB(dlight_point.lightradius, dl->radius);
			qglUniform1fARB(dlight_point.lightambient, r_light_ambient->value);
			qglUniform1fARB(dlight_point.lightdiffuse, r_light_diffuse->value);
			qglUniform1fARB(dlight_point.lightspecular, r_light_specular->value);
			qglUniform1fARB(dlight_point.lightspecularpow, r_light_specularpow->value);

			R_DrawHUDQuad(glwidth, glheight);
		}
	}

	//End light pass
	qglUseProgramObjectARB(0);

	//Begin shading pass, write to main FBO?
	GL_PopFrameBuffer();
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	//Bind new lightmap at texture1
	qglUseProgramObjectARB(dlight_final.program);
	qglUniform1iARB(dlight_final.diffuseTex, 0);
	qglUniform1iARB(dlight_final.lightmapTex, 1);
	qglUniform1iARB(dlight_final.depthTex, 2);

	//Allow depth data to be transfered to main FBO
	qglDisable(GL_BLEND);
	qglEnable(GL_DEPTH_TEST);
	qglDepthMask(GL_TRUE);

	//Diffuse texture (for merging)
	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(s_GBufferFBO.s_hBackBufferTex);

	//Lightmap texture (for merging)
	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferTex2);

	//Depth texture (for direct-writing)
	qglActiveTextureARB(TEXTURE2_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

	R_DrawHUDQuad(glwidth, glheight);

	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_2D);
	qglActiveTextureARB(TEXTURE1_SGIS);

	//FXAA for shading stage when MSAA available for all other render stage
	if (r_fxaa->value && s_MSAAFBO.s_hBackBufferFBO && (!drawreflect && !drawrefract))
	{
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
		qglBlitFramebufferEXT(0, 0, s_MSAAFBO.iWidth, s_MSAAFBO.iHeight, 0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);

		GL_DisableMultitexture();
		qglDisable(GL_BLEND);
		qglDisable(GL_DEPTH_TEST);
		qglDepthMask(GL_FALSE);

		R_BeginFXAA(glwidth, glheight);		
		R_DrawHUDQuad_Texture(s_BackBufferFBO.s_hBackBufferTex, glwidth, glheight);
	}

	qglUseProgramObjectARB(0);

	GL_PopMatrix();
	GL_PopDrawState();
}