#include "gl_local.h"
#include "gl_water.h"

colorVec *gWaterColor;
cshift_t *cshift_water;

void R_DrawWaterVBO(water_vbo_t *WaterVBOCache)
{
	if (curwater->framecount != (*r_framecount))
		return;

	if (curwater->rendered_framecount == (*r_framecount))
		return;

	curwater->rendered_framecount = (*r_framecount);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WaterVBOCache->hEBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_TEXTURE_SSBO, WaterVBOCache->hTextureSSBO);
	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	if(r_draw_opaque)
		glStencilFunc(GL_ALWAYS, 1, 0xFF);

	bool bIsAboveWater = R_IsAboveWater(WaterVBOCache->vert, WaterVBOCache->normal);

	if (WaterVBOCache->level >= WATER_LEVEL_REFLECT_SKYBOX && WaterVBOCache->level <= WATER_LEVEL_REFLECT_ENTITY && r_water->value)
	{
		int programState = 0;

		if (bUseBindless)
			programState |= WATER_BINDLESS_ENABLED;

		if ((*currententity)->curstate.rendermode == kRenderTransTexture || (*currententity)->curstate.rendermode == kRenderTransAdd)
		{
			if (gWaterColor->a > WaterVBOCache->maxtrans * 255)
				gWaterColor->a = WaterVBOCache->maxtrans * 255;

			programState |= WATER_REFRACT_ENABLED;

			if (bIsAboveWater)
				programState |= WATER_DEPTH_ENABLED;

			if (!refractmap_ready)
			{
				s_WaterFBO.s_hBackBufferTex = refractmap;
				s_WaterFBO.s_hBackBufferDepthTex = depthrefrmap;

				glBindFramebuffer(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferTex, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferDepthTex, 0);

				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
				glBlitFramebuffer(
					0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight,
					0, 0, s_WaterFBO.iWidth, s_WaterFBO.iHeight,
					GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
				glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

				refractmap_ready = true;
			}
		}

		if (!bIsAboveWater)
		{
			programState |= WATER_UNDERWATER_ENABLED;
		}
		else
		{
			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				programState |= WATER_LINEAR_FOG_ENABLED;
			}
		}

		if (drawgbuffer)
		{
			programState |= WATER_GBUFFER_ENABLED;
		}

		if (r_draw_oitblend)
		{
			programState |= WATER_OIT_ALPHA_BLEND_ENABLED;
		}

		water_program_t prog = { 0 };
		R_UseWaterProgram(programState, &prog);

		if (prog.u_watercolor != -1)
			glUniform4f(prog.u_watercolor, gWaterColor->r / 255.0f, gWaterColor->g / 255.0f, gWaterColor->b / 255.0f, gWaterColor->a / 255.0f);

		if (prog.u_depthfactor != -1)
			glUniform2fARB(prog.u_depthfactor, WaterVBOCache->depthfactor[0], WaterVBOCache->depthfactor[1]);

		if (prog.u_fresnelfactor != -1)
			glUniform1f(prog.u_fresnelfactor, WaterVBOCache->fresnelfactor);

		if (prog.u_normfactor != -1)
			glUniform1f(prog.u_normfactor, WaterVBOCache->normfactor);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (!(programState & WATER_BINDLESS_ENABLED))
		{
			glActiveTexture(GL_TEXTURE2);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, WaterVBOCache->normalmap);

			glActiveTexture(GL_TEXTURE3);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, WaterVBOCache->reflectmap);

			glActiveTexture(GL_TEXTURE4);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, refractmap);

			glActiveTexture(GL_TEXTURE5);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, depthrefrmap);
		}

		glDrawElements(GL_POLYGON,  WaterVBOCache->vIndicesBuffer.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));

		r_wsurf_drawcall++;
		r_wsurf_polys += WaterVBOCache->iPolyCount;

		if (!(programState & WATER_BINDLESS_ENABLED))
		{
			glActiveTexture(GL_TEXTURE5);
			glDisable(GL_TEXTURE_2D);

			glActiveTexture(GL_TEXTURE4);
			glDisable(GL_TEXTURE_2D);

			glActiveTexture(GL_TEXTURE3);
			glDisable(GL_TEXTURE_2D);

			glActiveTexture(GL_TEXTURE2);
			glDisable(GL_TEXTURE_2D);

			glActiveTexture(*oldtarget);
		}

		glDisable(GL_BLEND);
	}
	else
	{
		float scale;

		if (bIsAboveWater)
			scale = (*currententity)->curstate.scale;
		else
			scale = -(*currententity)->curstate.scale;

		int programState = WATER_LEGACY_ENABLED;

		if (bUseBindless)
			programState |= WATER_BINDLESS_ENABLED;

		if (!bIsAboveWater)
		{
		
		}
		else
		{
			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				programState |= WATER_LINEAR_FOG_ENABLED;
			}
		}

		if (drawgbuffer)
		{
			programState |= WATER_GBUFFER_ENABLED;
		}

		if (r_draw_oitblend)
		{
			if ((*currententity)->curstate.rendermode == kRenderTransAdd)
				programState |= WATER_OIT_ADDITIVE_BLEND_ENABLED;
			else
				programState |= WATER_OIT_ALPHA_BLEND_ENABLED;
		}

		water_program_t prog = { 0 };
		R_UseWaterProgram(programState, &prog);

		if (prog.u_watercolor != -1)
			glUniform4f(prog.u_watercolor, WaterVBOCache->color.r / 255.0f, WaterVBOCache->color.g / 255.0f, WaterVBOCache->color.b / 255.0f, gWaterColor->a / 255.0f);

		if (prog.u_scale != -1)
			glUniform1f(prog.u_scale, scale);

		if (!(programState & WATER_BINDLESS_ENABLED))
		{
			GL_Bind(WaterVBOCache->texture->gl_texturenum);
		}

		glDrawElements(GL_POLYGON, WaterVBOCache->vIndicesBuffer.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));

		r_wsurf_drawcall++;
		r_wsurf_polys += WaterVBOCache->iPolyCount;
	}

	glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	GL_UseProgram(0);
}

void R_DrawWaters(void)
{
	if (r_draw_pass == r_draw_reflect)
		return;

	for (size_t i = 0; i < g_WaterVBOCache.size(); ++i)
	{
		curwater = g_WaterVBOCache[i];

		R_DrawWaterVBO(curwater);

		curwater = NULL;
	}
}

void EmitWaterPolys(msurface_t *fa, int direction)
{
	if (r_draw_pass == r_draw_reflect)
		return;

	auto pSourcePalette = fa->texinfo->texture->pPal;
	gWaterColor->r = pSourcePalette[9];
	gWaterColor->g = pSourcePalette[10];
	gWaterColor->b = pSourcePalette[11];
	cshift_water->destcolor[0] = pSourcePalette[9];
	cshift_water->destcolor[1] = pSourcePalette[10];
	cshift_water->destcolor[2] = pSourcePalette[11];
	cshift_water->percent = pSourcePalette[12];

	if (gWaterColor->r == 0 && gWaterColor->g == 0 && gWaterColor->b == 0)
	{
		gWaterColor->r = pSourcePalette[0];
		gWaterColor->g = pSourcePalette[1];
		gWaterColor->b = pSourcePalette[2];
	}

	gWaterColor->a = 255;

	if ((*currententity)->curstate.rendermode == kRenderTransTexture)
		gWaterColor->a = (*r_blend) * 255;

	auto VBOCache = R_PrepareWaterVBO((*currententity), fa);

	VBOCache->framecount = (*r_framecount);
}

int *gSkyTexNumber;

int st_to_vec[6][3] =
{
	{ 3, -1, 2 },
	{ -3, 1, 2 },

	{ 1, 3, 2 },
	{ -1, -3, 2 },

	{ -2, -1, 3 },
	{ 2, -1, -3 }
};

int vec_to_st[6][3] =
{
	{ -2, 3, 1 },
	{ 2, 3, -1 },

	{ 1, 3, 2 },
	{ -1, 3, -2 },

	{ -2, -1, 3 },
	{ -2, 1, -3 }
};

#define SQRT3INV		(0.57735f)		// a little less than 1 / sqrt(3)

void MakeSkyVec(float s, float t, int axis, float zFar, vec3_t position, vec2_t texCoord)
{
	vec3_t		v, b;
	int			j, k;
	float		width;

	static float flScale = SQRT3INV;
	width = zFar * flScale;

	if (s < -1)
		s = -1;
	else if (s > 1)
		s = 1;
	if (t < -1)
		t = -1;
	else if (t > 1)
		t = 1;

	b[0] = s * width;
	b[1] = t * width;
	b[2] = width;

	for (j = 0; j < 3; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
		v[j] += r_origin[j];
	}

	// avoid bilerp seam
	s = (s + 1)*0.5;
	t = (t + 1)*0.5;

	// AV - I'm commenting this out since our skyboxes aren't 512x512 and we don't
	//      modify the textures to deal with the border seam fixup correctly.
	//      The code below was causing seams in the skyboxes.
	
	if (s < 1.0/512)
		s = 1.0/512;
	else if (s > 511.0/512)
		s = 511.0/512;
	if (t < 1.0/512)
		t = 1.0/512;
	else if (t > 511.0/512)
		t = 511.0/512;
	

	t = 1.0 - t;
	VectorCopy(v, position);
	texCoord[0] = s;
	texCoord[1] = t;
}

int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };

void R_DrawSkyBox(void)
{
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	glDepthMask(0);

	int WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_LEGACY_ENABLED;

	if (r_draw_pass == r_draw_reflect && curwater)
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	if (!drawgbuffer && r_fog_mode == GL_LINEAR)
	{
		WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
	}

	if (drawgbuffer)
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	float zFar = (r_params.movevars) ? r_params.movevars->zmax : 4096;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(4);

	for (int i = 0; i < 6; i++)
	{
		int order;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			order = i;
		}
		else
		{
			order = skytexorder[i];
		}

		vec3_t vertexArray[4];
		vec2_t texcoordArray[4];

		GL_Bind(gSkyTexNumber[order]);

		MakeSkyVec(-1.0f, -1.0f, i, zFar, vertexArray[0], texcoordArray[0]);
		MakeSkyVec(-1.0f, 1.0f, i, zFar, vertexArray[1], texcoordArray[1]);
		MakeSkyVec(1.0f, 1.0f, i, zFar, vertexArray[2], texcoordArray[2]);
		MakeSkyVec(1.0f, -1.0f, i, zFar, vertexArray[3], texcoordArray[3]);

		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, vertexArray);
		glVertexAttribPointer(4, 2, GL_FLOAT, false, 0, texcoordArray);
		glDrawArrays(GL_QUADS, 0, 4);
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(4);

	glDepthMask(1);
	GL_UseProgram(0);
}