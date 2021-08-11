#include "gl_local.h"
#include "gl_water.h"

float turbsin[] =
{
	#include "gl_warp_sin.h"
};

#define TURBSCALE (256.0 / (2 * M_PI))

colorVec *gWaterColor;
cshift_t *cshift_water;

void EmitWaterPolysWireFrame(msurface_t *fa, int direction, qboolean useProgram)
{
	/*glpoly_t *p;
	float *v;
	int i;
	float s;
	float scale;
	float tempVert[3];

	float clientTime = (*cl_time);

	if (gl_wireframe->value)
	{
		R_UseGBufferProgram(GBUFFER_TRANSPARENT_ENABLED);
		R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);

		if (fa->polys->verts[0][2] >= r_refdef->vieworg[2])
			scale = (*currententity)->curstate.scale;
		else
			scale = -(*currententity)->curstate.scale;

		if (gl_wireframe->value == 2)
			qglDisable(GL_DEPTH_TEST);

		qglColor3f(1, 1, 1);

		for (p = fa->polys; p; p = p->next)
		{
			qglBegin(GL_LINE_LOOP);

			if (direction)
				v = p->verts[p->numverts - 1];
			else
				v = p->verts[0];

			for (i = 0; i < p->numverts; i++)
			{
				tempVert[0] = v[0];
				tempVert[1] = v[1];
				tempVert[2] = v[2];

				if(!useProgram)
				{
					s = turbsin[(int)((clientTime * 160) + v[0] + v[1]) & 255];
					s += 8;
					s += turbsin[(int)((clientTime * 171) + v[0] * 5 - v[1]) & 255];
					s *= 8;
					s *= scale * 0.8;
					tempVert[2] += s;
				}

				qglVertex3fv(tempVert);

				if (direction)
					v -= VERTEXSIZE;
				else
					v += VERTEXSIZE;
			}
			qglEnd();
		}

		if (gl_wireframe->value == 2)
			qglEnable(GL_DEPTH_TEST);
	}*/
}

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

	qglEnable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);
	qglStencilFunc(GL_ALWAYS, 1, 0xFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

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

					qglBindFramebufferEXT(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
					qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferTex, 0);
					qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferDepthTex, 0);
					
					qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
					qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
					qglBlitFramebufferEXT(
						0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight,
						0, 0, s_WaterFBO.iWidth, s_WaterFBO.iHeight,
						GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
					qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

					refractmap_ready = true;
				}
			}

			if (!bAboveWater)
				programState |= WATER_UNDERWATER_ENABLED;

			if (drawgbuffer)
				programState |= WATER_GBUFFER_ENABLED;

			water_program_t prog = { 0 };
			R_UseWaterProgram(programState, &prog);

			if (prog.watercolor != -1)
				qglUniform4fARB(prog.watercolor, waterObject->color.r / 255.0f, waterObject->color.g / 255.0f, waterObject->color.b / 255.0f, alpha);
			if (prog.eyepos != -1)
				qglUniform4fARB(prog.eyepos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2], 1.0);
			if (prog.time != -1)
				qglUniform1fARB(prog.time, clientTime);
			if (prog.fresnelfactor != -1)
				qglUniform1fARB(prog.fresnelfactor, waterObject->fresnelfactor);
			if (prog.depthfactor != -1)
				qglUniform2fARB(prog.depthfactor, waterObject->depthfactor[0], waterObject->depthfactor[1]);
			if (prog.normfactor != -1)
				qglUniform1fARB(prog.normfactor, waterObject->normfactor);

			qglEnable(GL_BLEND);
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			GL_SelectTexture(TEXTURE0_SGIS);
			GL_Bind(waterObject->normalmap);

			GL_EnableMultitexture();
			GL_Bind(refractmap);

			if (prog.reflectmap != -1)
			{
				qglActiveTextureARB(TEXTURE2_SGIS);
				qglEnable(GL_TEXTURE_2D);
				qglBindTexture(GL_TEXTURE_2D, waterObject->reflectmap);
			}

			if (prog.depthrefrmap != -1)
			{
				qglActiveTextureARB(TEXTURE3_SGIS);
				qglEnable(GL_TEXTURE_2D);
				qglBindTexture(GL_TEXTURE_2D, depthrefrmap);
			}

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

	for (p = fa->polys; p; p = p->next)
	{
		qglBegin(GL_POLYGON);

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
			}
			else
			{
				tempVert[0] = v[0];
				tempVert[1] = v[1];
				tempVert[2] = v[2];

				os = v[3];
				ot = v[4];
			}

			if(!useProgram)
			{
				qglTexCoord2f(s, t);
			}
			else
			{
				qglMultiTexCoord2fARB(TEXTURE0_SGIS, os, ot);
			}

			qglNormal3fv(normal);
			qglVertex3fv(tempVert);

			if (direction)
				v -= VERTEXSIZE;
			else
				v += VERTEXSIZE;
		}

		qglEnd();

		r_wsurf_drawcall++;
		r_wsurf_polys++;
	}

	if(useProgram)
	{
		qglActiveTextureARB(TEXTURE3_SGIS);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);

		qglActiveTextureARB(TEXTURE2_SGIS);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);

		qglActiveTextureARB(TEXTURE1_SGIS);
		GL_DisableMultitexture();
	}

	GL_UseProgram(0);

	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);

	//EmitWaterPolysWireFrame(fa, direction, useProgram);
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
	qglDisable(GL_BLEND);
	qglColor4f(1, 1, 1, 1);
	qglDepthMask(0);

	int WSurfProgramState = WSURF_DIFFUSE_ENABLED;

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

	if (prog.speed != -1)
		qglUniform1fARB(prog.speed, 0);

	float zFar = (r_params.movevars) ? r_params.movevars->zmax : 4096;

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

		vec3_t positionArray[4];
		vec2_t texCoordArray[4];

		GL_Bind(gSkyTexNumber[order]);

		MakeSkyVec(-1.0f, -1.0f, i, zFar, positionArray[0], texCoordArray[0]);
		MakeSkyVec(-1.0f, 1.0f, i, zFar, positionArray[1], texCoordArray[1]);
		MakeSkyVec(1.0f, 1.0f, i, zFar, positionArray[2], texCoordArray[2]);
		MakeSkyVec(1.0f, -1.0f, i, zFar, positionArray[3], texCoordArray[3]);

		qglBegin(GL_QUADS);
		for (int j = 0; j < 4; ++j)
		{
			qglTexCoord2fv(texCoordArray[j]);
			qglVertex3fv(positionArray[j]);
		}
		qglEnd();
	}

	qglDepthMask(1);
	GL_UseProgram(0);
}