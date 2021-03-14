#include "gl_local.h"
#include <sstream>

int r_light_env_color[4] = { 0 };
qboolean r_light_env_color_exists = false;
vec3_t r_light_env_angles = {0};
qboolean r_light_env_angles_exists = false;

cvar_t *r_light_dynamic = NULL;
cvar_t *r_light_debug = NULL;

cvar_t *r_light_ambient = NULL;
cvar_t *r_light_diffuse = NULL;
cvar_t *r_light_specular = NULL;
cvar_t *r_light_specularpow = NULL;

cvar_t *r_flashlight_distance = NULL;
cvar_t *r_flashlight_cone = NULL;

bool drawgbuffer = false;

int gbuffer_mask = -1;

std::unordered_map<int, gbuffer_program_t> g_GBufferProgramTable;

#if 0

std::vector<deferred_decal_t> g_DeferredDecals;

void R_DecalShootInternal(texture_t *ptexture, int index, int entity, int modelIndex, vec3_t position, int flags, float flScale)
{
	deferred_decal_t decal;

	decal.texture = ptexture;
	VectorCopy(position, decal.org);
	decal.width = ptexture->width * flScale;
	decal.height = ptexture->height * flScale;

	g_DeferredDecals.emplace_back(decal);
}
#endif

void R_UseGBufferProgram(int state, gbuffer_program_t *progOutput)
{
	if (!drawgbuffer)
		return;

	gbuffer_program_t prog = { 0 };

	auto itor = g_GBufferProgramTable.find(state);
	if (itor == g_GBufferProgramTable.end())
	{
		std::stringstream defs;

		if (state & GBUFFER_DIFFUSE_ENABLED)
			defs << "#define DIFFUSE_ENABLED\n";

		if (state & GBUFFER_LIGHTMAP_ENABLED)
			defs << "#define LIGHTMAP_ENABLED\n";

		if (state & GBUFFER_DETAILTEXTURE_ENABLED)
			defs << "#define DETAILTEXTURE_ENABLED\n";

		if (state & GBUFFER_LIGHTMAP_ARRAY_ENABLED)
			defs << "#define LIGHTMAP_ARRAY_ENABLED\n";

		if (state & GBUFFER_TRANSPARENT_ENABLED)
			defs << "#define TRANSPARENT_ENABLED\n";

		if (state & GBUFFER_SCROLL_ENABLED)
			defs << "#define SCROLL_ENABLED\n";

		if (state & GBUFFER_ROTATE_ENABLED)
			defs << "#define ROTATE_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\gbuffer_shader.vsh", NULL, "renderer\\shader\\gbuffer_shader.fsh", def.c_str(), NULL, def.c_str());
		if (prog.program)
		{
			SHADER_UNIFORM(prog, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(prog, lightmapTex, "lightmapTex");
			SHADER_UNIFORM(prog, lightmapTexArray, "lightmapTexArray");
			SHADER_UNIFORM(prog, detailTex, "detailTex");
			SHADER_UNIFORM(prog, speed, "speed");
			SHADER_UNIFORM(prog, entitymatrix, "entitymatrix");
		}

		g_GBufferProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		qglUseProgramObjectARB(prog.program);

		if (prog.diffuseTex != -1)
			qglUniform1iARB(prog.diffuseTex, 0);
		if (prog.lightmapTexArray != -1)
			qglUniform1iARB(prog.lightmapTexArray, 1);
		if (prog.lightmapTex != -1)
			qglUniform1iARB(prog.lightmapTex, 1);
		if (prog.detailTex != -1)
			qglUniform1iARB(prog.detailTex, 2);
		if (prog.entitymatrix != -1)
		{
			if(r_rotate_entity)
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, true, (float *)r_rotate_entity_matrix);
			else
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, false, (float *)r_identity_matrix);
		}

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseGBufferProgram: Failed to load program!");
	}
}

void R_UseGBufferProgram(int state)
{
	return R_UseGBufferProgram(state, NULL);
}

std::unordered_map<int, dlight_program_t> g_DLightProgramTable;

void R_UseDLightProgram(int state, dlight_program_t *progOutput)
{
	dlight_program_t prog = { 0 };

	auto itor = g_DLightProgramTable.find(state);
	if (itor == g_DLightProgramTable.end())
	{
		std::stringstream defs;

		if (state & DLIGHT_DECAL_PASS)
			defs << "#define DECAL_PASS\n";

		if (state & DLIGHT_LIGHT_PASS)
			defs << "#define LIGHT_PASS\n";

		if (state & DLIGHT_LIGHT_PASS_SPOT)
			defs << "#define LIGHT_PASS_SPOT\n";

		if (state & DLIGHT_LIGHT_PASS_POINT)
			defs << "#define LIGHT_PASS_POINT\n";

		if (state & DLIGHT_FINAL_PASS)
			defs << "#define FINAL_PASS\n";

		if (state & DLIGHT_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\dlight_shader.vsh", NULL, "renderer\\shader\\dlight_shader.fsh", def.c_str(), NULL, def.c_str());
		if (prog.program)
		{
			SHADER_UNIFORM(prog, positionTex, "positionTex");
			SHADER_UNIFORM(prog, normalTex, "normalTex");
			SHADER_UNIFORM(prog, viewpos, "viewpos");
			SHADER_UNIFORM(prog, lightdir, "lightdir");
			SHADER_UNIFORM(prog, lightpos, "lightpos");
			SHADER_UNIFORM(prog, lightcolor, "lightcolor");
			SHADER_UNIFORM(prog, lightcone, "lightcone");
			SHADER_UNIFORM(prog, lightradius, "lightradius");
			SHADER_UNIFORM(prog, lightambient, "lightambient");
			SHADER_UNIFORM(prog, lightdiffuse, "lightdiffuse");
			SHADER_UNIFORM(prog, lightspecular, "lightspecular");
			SHADER_UNIFORM(prog, lightspecularpow, "lightspecularpow");

			SHADER_UNIFORM(prog, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(prog, lightmapTex, "lightmapTex");
			SHADER_UNIFORM(prog, additiveTex, "additiveTex");
			SHADER_UNIFORM(prog, depthTex, "depthTex");
			SHADER_UNIFORM(prog, clipInfo, "clipInfo");

			SHADER_UNIFORM(prog, decalTex, "decalTex");
			SHADER_UNIFORM(prog, decalToWorldMatrix, "decalToWorldMatrix");
			SHADER_UNIFORM(prog, worldToDecalMatrix, "worldToDecalMatrix");
			SHADER_UNIFORM(prog, decalScale, "decalScale");
			SHADER_UNIFORM(prog, decalCenter, "decalCenter");
		}

		g_DLightProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		qglUseProgramObjectARB(prog.program);

		if (prog.positionTex != -1)
			qglUniform1iARB(prog.positionTex, 0);
		if (prog.normalTex != -1)
			qglUniform1iARB(prog.normalTex, 1);

		if (prog.diffuseTex != -1)
			qglUniform1iARB(prog.diffuseTex, 0);
		if (prog.lightmapTex != -1)
			qglUniform1iARB(prog.lightmapTex, 1);
		if (prog.additiveTex != -1)
			qglUniform1iARB(prog.additiveTex, 2);
		if (prog.depthTex != -1)
			qglUniform1iARB(prog.depthTex, 3);

		if (prog.decalTex != -1)
			qglUniform1iARB(prog.decalTex, 2);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseDLightProgram: Failed to load program!");
	}
}

void R_UseDLightProgram(int state)
{
	return R_UseDLightProgram(state, NULL);
}

void R_ShutdownLight(void)
{
	g_GBufferProgramTable.clear();
	g_DLightProgramTable.clear();
}

void R_InitLight(void)
{
	r_light_env_color[0] = 0;
	r_light_env_color[1] = 0;
	r_light_env_color[2] = 0;
	r_light_env_color[3] = 0;

	r_light_env_angles[0] = 90;
	r_light_env_angles[1] = 0;
	r_light_env_angles[2] = 0;

	r_light_env_color_exists = false;
	r_light_env_angles_exists = false;

	r_light_dynamic = gEngfuncs.pfnRegisterVariable("r_light_dynamic", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_debug = gEngfuncs.pfnRegisterVariable("r_light_debug", "0", FCVAR_CLIENTDLL);

	r_light_ambient = gEngfuncs.pfnRegisterVariable("r_light_ambient", "0.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_diffuse = gEngfuncs.pfnRegisterVariable("r_light_diffuse", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_specular = gEngfuncs.pfnRegisterVariable("r_light_specular", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_specularpow = gEngfuncs.pfnRegisterVariable("r_light_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_flashlight_distance = gEngfuncs.pfnRegisterVariable("r_flashlight_distance", "2000", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_cone = gEngfuncs.pfnRegisterVariable("r_flashlight_cone", "0.9", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

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

void R_SetGBufferMask(int mask)
{
	if (!drawgbuffer)
		return;

	if (gbuffer_mask == mask)
		return;

	gbuffer_mask = mask;

	GLuint attachments[6] = {0};
	int attachCount = 0;

	if (mask & GBUFFER_MASK_DIFFUSE)
	{
		attachments[attachCount] = GL_COLOR_ATTACHMENT0;
		attachCount++;
	}
	if (mask & GBUFFER_MASK_LIGHTMAP)
	{
		attachments[attachCount] = GL_COLOR_ATTACHMENT1;
		attachCount++;
	}
	if (mask & GBUFFER_MASK_WORLD)
	{
		attachments[attachCount] = GL_COLOR_ATTACHMENT2;
		attachCount++;
	}
	if (mask & GBUFFER_MASK_NORMAL)
	{
		attachments[attachCount] = GL_COLOR_ATTACHMENT3;
		attachCount++;
	}
	if (mask & GBUFFER_MASK_ADDITIVE)
	{
		attachments[attachCount] = GL_COLOR_ATTACHMENT4;
		attachCount++;
	}

	qglDrawBuffers(attachCount, attachments);
}

void R_BeginRenderGBuffer(void)
{
	if (r_draw_pass)
		return;

	if (!r_light_dynamic->value)
		return;

	if (!s_GBufferFBO.s_hBackBufferFBO)
		return;

	drawgbuffer = true;
	gbuffer_mask = -1;

	GL_PushFrameBuffer();

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	qglClearColor(0, 0, 0, 1);
	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglDepthMask(GL_TRUE);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglStencilMask(0);
}

void R_EndRenderGBuffer(void)
{
	if (!drawgbuffer)
		return;

	drawgbuffer = false;
	gbuffer_mask = -1;

	GL_PushDrawState();
	GL_PushMatrix();

	qglDisable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);

#if 0
	//Write to GBuffer->diffuse only
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask(GL_FALSE);
	qglDisable(GL_CULL_FACE);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);

		if (gl_ztrick && gl_ztrick->value)
			qglPolygonOffset(1, gl_polyoffset->value);
		else
			qglPolygonOffset(-1, -gl_polyoffset->value);
	}

	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(s_GBufferFBO.s_hBackBufferTex3);

	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferTex4);

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglEnable(GL_TEXTURE_2D);

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBOCube);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_wsurf.hEBOCube);
	qglVertexPointer(4, GL_FLOAT, sizeof(float[4]), 0);

	dlight_program_t progDecal = { 0 };
	R_UseDLightProgram(DLIGHT_DECAL_PASS, &progDecal);

	for (size_t i = 0; i < g_DeferredDecals.size(); ++i)
	{
		auto &decal = g_DeferredDecals[i];

		qglBindTexture(GL_TEXTURE_2D, decal.texture->gl_texturenum);

		vec3_t angles = { 0 };

		float decalmatrix[4][4] = {0};
		memcpy(decalmatrix, r_identity_matrix, sizeof(r_identity_matrix));
		Matrix4x4_CreateFromEntity(decalmatrix, angles, decal.org, 1);

		float invertdecalmatrix[4][4] = { 0 };
		InvertMatrix((const float *)decalmatrix, (float *)invertdecalmatrix);

		qglUniformMatrix4fvARB(progDecal.decalToWorldMatrix, 1, true, (float *)decalmatrix);
		qglUniformMatrix4fvARB(progDecal.worldToDecalMatrix, 1, true, (float *)invertdecalmatrix);
		qglUniform1fARB(progDecal.decalScale, decal.width);
		qglUniform4fARB(progDecal.decalCenter, decal.org[0], decal.org[1], decal.org[2], 1);

		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	}

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	qglDisableClientState(GL_VERTEX_ARRAY);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_2D);
#endif

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

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLIGHT_LIGHT_PASS | DLIGHT_LIGHT_PASS_SPOT, &prog);
			qglUniform4fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2], 1.0f);
			qglUniform4fARB(prog.lightdir, dlight_vforward[0], dlight_vforward[1], dlight_vforward[2], 0.0f);
			qglUniform4fARB(prog.lightpos, dlight_origin[0], dlight_origin[1], dlight_origin[2], 1.0f);
			qglUniform3fARB(prog.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
			qglUniform1fARB(prog.lightcone, r_flashlight_cone->value);
			qglUniform1fARB(prog.lightradius, r_flashlight_distance->value);
			qglUniform1fARB(prog.lightambient, r_light_ambient->value);
			qglUniform1fARB(prog.lightdiffuse, r_light_diffuse->value);
			qglUniform1fARB(prog.lightspecular, r_light_specular->value);
			qglUniform1fARB(prog.lightspecularpow, r_light_specularpow->value);

			R_DrawHUDQuad(glwidth, glheight);
		}
		else
		{
			vec3_t dlight_origin;

			//Point Light
			VectorCopy(dl->origin, dlight_origin);

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLIGHT_LIGHT_PASS | DLIGHT_LIGHT_PASS_POINT, &prog);
			qglUniform4fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2], 1.0f);
			qglUniform4fARB(prog.lightpos, dlight_origin[0], dlight_origin[1], dlight_origin[2], 1.0f);
			qglUniform3fARB(prog.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
			qglUniform1fARB(prog.lightradius, dl->radius);
			qglUniform1fARB(prog.lightambient, r_light_ambient->value);
			qglUniform1fARB(prog.lightdiffuse, r_light_diffuse->value);
			qglUniform1fARB(prog.lightspecular, r_light_specular->value);
			qglUniform1fARB(prog.lightspecularpow, r_light_specularpow->value);

			R_DrawHUDQuad(glwidth, glheight);
		}
	}

	//Begin shading pass, write to main FBO?
	GL_PopFrameBuffer();
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	//Allow depth data to be transfered to main FBO?
	
	int FinalProgramState = DLIGHT_FINAL_PASS;

	if (r_wsurf_fogmode == GL_LINEAR)
		FinalProgramState |= DLIGHT_LINEAR_FOG_ENABLED;

	dlight_program_t finalProg = { 0 };

	R_UseDLightProgram(FinalProgramState, &finalProg);

	if (finalProg.clipInfo != -1)
	{
		qglUniform4fARB(finalProg.clipInfo, 4 * r_params.movevars->zmax, 4 - r_params.movevars->zmax, r_params.movevars->zmax, 1.0f);
	}

	//Diffuse texture (for merging)
	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(s_GBufferFBO.s_hBackBufferTex);

	//Lightmap texture (for merging)
	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferTex2);

	//Additive texture
	qglActiveTextureARB(TEXTURE2_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex5);
	
	//Depth texture
	qglActiveTextureARB(TEXTURE3_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

	R_DrawHUDQuad(glwidth, glheight);

	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_2D);

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_2D);

	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);

	if (R_UseMSAA())
	{
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
		qglBlitFramebufferEXT(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight,
			0, 0, s_MSAAFBO.iWidth, s_MSAAFBO.iHeight,
			GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
			/*
			According to OpenGL API document
			https://www.khronos.org/opengl/wiki_opengl/index.php?title=GLAPI/glBlitFramebuffer&printable=yes

			 GL_LINEAR is only a valid interpolation method for the color buffer.
			 If filter is not GL_NEAREST and mask includes GL_DEPTH_BUFFER_BIT or GL_STENCIL_BUFFER_BIT,
			 no data is transferred and a GL_INVALID_OPERATION error is generated.
			*/
			GL_NEAREST);

		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
	}
	else
	{
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
		qglBlitFramebufferEXT(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight,
			0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight,
			GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
			GL_NEAREST);
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	}

	//FXAA for shading stage when MSAA available for all other render stage
	if (r_fxaa->value && R_UseMSAA() && (!r_draw_pass && !g_SvEngine_DrawPortalView))
	{
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
		qglBlitFramebufferEXT(0, 0, s_MSAAFBO.iWidth, s_MSAAFBO.iHeight, 0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);

		GL_DisableMultitexture();
		qglDisable(GL_BLEND);
		qglDisable(GL_DEPTH_TEST);
		qglDisable(GL_ALPHA_TEST);
		qglDepthMask(GL_FALSE);

		R_BeginFXAA(glwidth, glheight);		
		R_DrawHUDQuad_Texture(s_BackBufferFBO.s_hBackBufferTex, glwidth, glheight);
	}

	qglUseProgramObjectARB(0);

	GL_PopMatrix();
	GL_PopDrawState();
}