#include "gl_local.h"
#include "gl_water.h"

colorVec *gWaterColor = NULL;
cshift_t *cshift_water = NULL;
int *gSkyTexNumber = NULL;

void R_DrawWaterVBO(water_vbo_t *WaterVBOCache)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WaterVBOCache->hEBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_TEXTURE_SSBO, WaterVBOCache->hTextureSSBO);

	if (r_draw_opaque)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	bool bIsAboveWater = R_IsAboveWater(WaterVBOCache);

	float color[4];
	color[0] = WaterVBOCache->color.r / 255.0f;
	color[1] = WaterVBOCache->color.g / 255.0f;
	color[2] = WaterVBOCache->color.b / 255.0f;
	color[3] = 1;

	if((*currententity)->curstate.rendermode == kRenderTransTexture)
		color[3] = (*r_blend);

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
				if (drawgbuffer)
				{
					R_BlitGBufferToFrameBuffer(&s_WaterFBO);

					glBindFramebuffer(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
				}
				else
				{
					GL_BlitFrameBufferToFrameBufferColorDepth(&s_BackBufferFBO, &s_WaterFBO);

					glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
				}

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
			glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);

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
			glBindTexture(GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferTex);

			glActiveTexture(GL_TEXTURE5);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferDepthTex);
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
			glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);

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

	GL_UseProgram(0);

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
}

void R_DrawWaters(void)
{
	if (r_draw_pass == r_draw_reflect)
		return;

	if (g_iNumRenderWaterVBOCache)
	{
		glBindBuffer(GL_ARRAY_BUFFER, r_wsurf.hSceneVBO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
		glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, s_tangent));
		glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, t_tangent));
		glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		for (int i = 0; i < g_iNumRenderWaterVBOCache; ++i)
		{
			R_DrawWaterVBO(g_RenderWaterVBOCache[i]);
		}

		glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		g_iNumRenderWaterVBOCache = 0;
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

	//if ((*currententity)->curstate.rendermode == kRenderTransTexture)
	//	gWaterColor->a = (*r_blend) * 255;

	auto VBOCache = R_PrepareWaterVBO((*currententity), fa, direction);

	if (g_iNumRenderWaterVBOCache == 512)
	{
		Sys_ErrorEx("EmitWaterPolys: Too many waters!");
	}

	if (VBOCache->framecount != (*r_framecount))
	{
		VBOCache->framecount = (*r_framecount);
		g_RenderWaterVBOCache[g_iNumRenderWaterVBOCache] = VBOCache;
		g_iNumRenderWaterVBOCache++;
	}
}

void R_DrawSkyBox(void)
{
	if (CL_IsDevOverviewMode())
		return;

	glDisable(GL_BLEND);
	glDepthMask(0);

	int WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_SKYBOX_ENABLED;

	if (bUseBindless)
	{
		WSurfProgramState |= WSURF_BINDLESS_ENABLED;
	}

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

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_SKYBOX_SSBO, r_wsurf.hSkyboxSSBO);

	if (WSurfProgramState & WSURF_BINDLESS_ENABLED)
	{
		glDrawArrays(GL_QUADS, 0, 4 * 6);
	}
	else
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			for (int i = 0; i < 6; ++i)
			{
				GL_Bind(gSkyTexNumber[i]);
				glDrawArrays(GL_QUADS, 0, 4);
			}
		}
		else
		{
			const int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };
			for (int i = 0; i < 6; ++i)
			{
				GL_Bind(gSkyTexNumber[skytexorder[i]]);
				glDrawArrays(GL_QUADS, 0, 4);
			}
		}
	}

	GL_UseProgram(0);

	glDepthMask(1);
}