#include "gl_local.h"
#include "gl_water.h"

float turbsin[] =
{
	#include "gl_warp_sin.h"
};

#define TURBSCALE (256.0 / (2 * M_PI))

colorVec *gWaterColor;
cshift_t *cshift_water;

void EmitWaterPolys(msurface_t *fa, int direction)
{
	float *v;
	int i;
	float s, t, os, ot;
	float scale;
	float tempVert[3];
	unsigned char *pSourcePalette;
	int useProgram = 0, dontShader = false;

	if (r_draw_pass == r_draw_reflect)
		return;

	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	float clientTime = (*cl_time);

	auto p = fa->polys;
	tempVert[0] = p->verts[0][0];
	tempVert[1] = p->verts[0][1];
	tempVert[2] = p->verts[0][2];

	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	pSourcePalette = fa->texinfo->texture->pPal;
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

	if ((*currententity) != r_worldentity)
	{
		VectorAdd(tempVert, (*currententity)->curstate.origin, tempVert);
	}
	
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	vec3_t normal;
	VectorCopy(brushface->normal, normal);
	if (direction == 1)
		VectorInverse(normal);

	if(r_water && r_water->value && !dontShader)
	{
		auto bAboveWater = R_IsAboveWater(tempVert);
		auto waterObject = R_GetActiveWater((*currententity), fa->texinfo->texture ? fa->texinfo->texture->name : "", tempVert, normal, gWaterColor, bAboveWater);

		if(waterObject && waterObject->ready)
		{
			int programState = 0;

			float alpha = 1;
			if ((*currententity)->curstate.rendermode == kRenderTransTexture || (*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				alpha = (*r_blend);

				if (alpha > waterObject->maxtrans)
					alpha = waterObject->maxtrans;

				programState |= WATER_REFRACT_ENABLED;

				if (bAboveWater)
					programState |= WATER_DEPTH_ENABLED;

				if (!refractmap_ready)
				{
					s_WaterFBO.s_hBackBufferTex = refractmap;
					s_WaterFBO.s_hBackBufferDepthTex = depthrefrmap;

					glBindFramebufferEXT(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
					glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferTex, 0);
					glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferDepthTex, 0);
					
					glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
					glBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
					glBlitFramebufferEXT(
						0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight,
						0, 0, s_WaterFBO.iWidth, s_WaterFBO.iHeight,
						GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
					glBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

					refractmap_ready = true;
				}
			}

			if (!bAboveWater)
				programState |= WATER_UNDERWATER_ENABLED;

			if (drawgbuffer)
				programState |= WATER_GBUFFER_ENABLED;

			water_program_t prog = { 0 };
			R_UseWaterProgram(programState, &prog);

			if (prog.u_watercolor != -1)
				glUniform4f(prog.u_watercolor, waterObject->color.r / 255.0f, waterObject->color.g / 255.0f, waterObject->color.b / 255.0f, alpha);

			if (prog.u_depthfactor != -1)
				glUniform2fARB(prog.u_depthfactor, waterObject->depthfactor[0], waterObject->depthfactor[1]);

			if (prog.u_fresnelfactor != -1)
				glUniform1f(prog.u_fresnelfactor, waterObject->fresnelfactor);

			if (prog.u_normfactor != -1)
				glUniform1f(prog.u_normfactor, waterObject->normfactor);

			if (!r_draw_nontransparent)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}

			GL_SelectTexture(GL_TEXTURE0);
			GL_Bind(waterObject->normalmap);

			GL_EnableMultitexture();
			GL_Bind(refractmap);

			glActiveTexture(GL_TEXTURE2);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, waterObject->reflectmap);

			glActiveTexture(GL_TEXTURE3);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, depthrefrmap);

			useProgram = 1;
		}
	}

	if (!useProgram)
	{
		int WSurfProgramState = WSURF_DIFFUSE_ENABLED;
		if ((*currententity)->curstate.rendermode == kRenderTransTexture || (*currententity)->curstate.rendermode == kRenderTransAdd)
			WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;

		R_UseWSurfProgram(WSurfProgramState, NULL);
	}

	if (fa->polys->verts[0][2] >= r_refdef->vieworg[2])
		scale = -(*currententity)->curstate.scale;
	else
		scale = (*currententity)->curstate.scale;

	if (useProgram)
		scale = 0;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(4);

	vec3_t vertexArray[128];
	vec3_t normalArray[128];
	vec2_t texcoordArray[128];

	for (p = fa->polys; p; p = p->next)
	{
		int numVertex = 0;

		if (direction)
			v = p->verts[p->numverts - 1];
		else
			v = p->verts[0];

		for (i = 0; i < p->numverts; i++)
		{
			if(!useProgram)
			{
				s = (turbsin[(int)((clientTime * 160) + v[0] + v[1]) & 255] + 8) + (turbsin[(int)((clientTime * 171) + v[0] * 5 - v[1]) & 255] + 8) * 0.8;

				tempVert[0] = v[0];
				tempVert[1] = v[1];
				tempVert[2] = v[2] + (s * scale);

				os = v[3];
				ot = v[4];

				s = os + turbsin[(int)((ot * 0.125 + clientTime) * TURBSCALE) & 255];
				s *= (1.0 / 64);

				t = ot + turbsin[(int)((os * 0.125 + clientTime) * TURBSCALE) & 255];
				t *= (1.0 / 64);

				texcoordArray[numVertex][0] = s;
				texcoordArray[numVertex][1] = t;
			}
			else
			{
				tempVert[0] = v[0];
				tempVert[1] = v[1];
				tempVert[2] = v[2];

				os = v[3];
				ot = v[4];

				texcoordArray[numVertex][0] = os / 128;
				texcoordArray[numVertex][1] = ot / 128;
			}

			VectorCopy(tempVert, vertexArray[numVertex]);
			VectorCopy(normal, normalArray[numVertex]);

			numVertex++;

			if (direction)
				v -= VERTEXSIZE;
			else
				v += VERTEXSIZE;
		}

		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, vertexArray);
		glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, normalArray);
		glVertexAttribPointer(4, 2, GL_FLOAT, false, 0, texcoordArray);
		glDrawArrays(GL_POLYGON, 0, numVertex);

		r_wsurf_drawcall++;
		r_wsurf_polys++;
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(4);

	if(useProgram)
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		GL_DisableMultitexture();
	}

	GL_UseProgram(0);

	glStencilMask(0);
	glDisable(GL_STENCIL_TEST);
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

	if (!drawgbuffer && r_fog_mode == GL_LINEAR)
	{
		WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
	}

	if (r_draw_pass == r_draw_reflect && curwater)
	{
		WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
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