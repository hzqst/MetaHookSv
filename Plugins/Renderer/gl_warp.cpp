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
	glpoly_t *p;
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
	}
}

void EmitWaterPolys(msurface_t *fa, int direction)
{
	float *v;
	int i;
	float s, t, os, ot;
	float scale;
	float tempVert[3];
	unsigned char *pSourcePalette;
	qboolean useProgram = false, dontShader = false;

	if (drawreflect)
		return;

	if (fa->texinfo->texture)
	{
		if (0 == strncmp(fa->texinfo->texture->name, "!toxi", sizeof("!toxi") - 1) ||
			0 == strcmp(fa->texinfo->texture->name, "!radio"))
		{
			dontShader = true;
		}
	}

	if (drawrefract)
	{
		if ((*currententity) == r_worldentity)
			dontShader = true;
		else
			return;
	}

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

	gWaterColor->a = 255;
	if ((*currententity)->curstate.rendermode == kRenderTransTexture)
		gWaterColor->a = (*r_blend) * 255;

	if ((*currententity) != r_worldentity)
	{
		VectorAdd(tempVert, (*currententity)->curstate.origin, tempVert);
	}
	
	R_SetVBOState(VBOSTATE_OFF);

	R_UseGBufferProgram(GBUFFER_DIFFUSE_ENABLED);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	if(r_water && r_water->value && water.program && !dontShader)
	{
		r_water_t *waterObject = NULL;

		bool bAboveWater = *cl_waterlevel <= 2 ? true : false;

		if ((*currententity) != r_worldentity && bAboveWater && (*currententity)->model->maxs[2] - (*currententity)->model->mins[2] < r_water_minheight->value)
		{

		}
		else
		{
			waterObject = R_GetActiveWater((*currententity), tempVert, gWaterColor);
		}

		if(waterObject && waterObject->refractmap_ready && ((waterObject->reflectmap_ready && bAboveWater) || !bAboveWater))
		{

			float alpha = 1;
			if ((*currententity)->curstate.rendermode == kRenderTransTexture)
				alpha = (*r_blend);

			if (bAboveWater)
			{
				if (drawgbuffer)
				{
					qglUseProgramObjectARB(watergbuffer.program);
					qglUniform4fARB(watergbuffer.waterfogcolor, waterObject->color.r / 255.0f, waterObject->color.g / 255.0f, waterObject->color.b / 255.0f, alpha);
					qglUniform3fARB(watergbuffer.eyepos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
					qglUniform1fARB(watergbuffer.time, clientTime);
					qglUniform1fARB(watergbuffer.fresnel, clamp(r_water_fresnel->value, 0.0, 10.0));
					qglUniform1fARB(watergbuffer.depthfactor, clamp(r_water_depthfactor->value, 0.0, 1000.0));
					qglUniform1fARB(watergbuffer.normfactor, clamp(r_water_normfactor->value, 0.0, 1000.0));
					qglUniform1iARB(watergbuffer.normalmap, 0);
					qglUniform1iARB(watergbuffer.refractmap, 1);
					qglUniform1iARB(watergbuffer.reflectmap, 2);
					qglUniform1iARB(watergbuffer.depthrefrmap, 3);
				}
				else
				{
					qglUseProgramObjectARB(water.program);
					qglUniform4fARB(water.waterfogcolor, waterObject->color.r / 255.0f, waterObject->color.g / 255.0f, waterObject->color.b / 255.0f, alpha);
					qglUniform3fARB(water.eyepos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
					qglUniform1fARB(water.time, clientTime);
					qglUniform1fARB(water.fresnel, clamp(r_water_fresnel->value, 0.0, 10.0));
					qglUniform1fARB(water.depthfactor, clamp(r_water_depthfactor->value, 0.0, 1000.0));
					qglUniform1fARB(water.normfactor, clamp(r_water_normfactor->value, 0.0, 1000.0));
					qglUniform1iARB(water.normalmap, 0);
					qglUniform1iARB(water.refractmap, 1);
					qglUniform1iARB(water.reflectmap, 2);
					qglUniform1iARB(water.depthrefrmap, 3);
				}
			}
			else
			{
				if (drawgbuffer)
				{
					qglUseProgramObjectARB(underwatergbuffer.program);
					qglUniform4fARB(underwatergbuffer.waterfogcolor, waterObject->color.r / 255.0f, waterObject->color.g / 255.0f, waterObject->color.b / 255.0f, alpha);
					qglUniform3fARB(underwatergbuffer.eyepos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
					qglUniform1fARB(underwatergbuffer.time, clientTime);
					qglUniform1fARB(underwatergbuffer.normfactor, clamp(r_water_normfactor->value, 0.0, 1000.0));
					qglUniform1iARB(underwatergbuffer.normalmap, 0);
					qglUniform1iARB(underwatergbuffer.refractmap, 1);
				}
				else
				{
					qglUseProgramObjectARB(underwater.program);
					qglUniform4fARB(underwater.waterfogcolor, waterObject->color.r / 255.0f, waterObject->color.g / 255.0f, waterObject->color.b / 255.0f, alpha);
					qglUniform3fARB(underwater.eyepos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
					qglUniform1fARB(underwater.time, clientTime);
					qglUniform1fARB(underwater.normfactor, clamp(r_water_normfactor->value, 0.0, 1000.0));
					qglUniform1iARB(underwater.normalmap, 0);
					qglUniform1iARB(underwater.refractmap, 1);
				}
			}

			qglEnable(GL_BLEND);
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			GL_SelectTexture(TEXTURE0_SGIS);
			GL_Bind(water_normalmap);

			GL_EnableMultitexture();
			GL_Bind(waterObject->refractmap);

			qglActiveTextureARB(TEXTURE2_SGIS);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, waterObject->reflectmap);

			qglActiveTextureARB(TEXTURE3_SGIS);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, waterObject->depthrefrmap);

			useProgram = true;
		}
	}

	if (fa->polys->verts[0][2] >= r_refdef->vieworg[2])
		scale = (*currententity)->curstate.scale;
	else
		scale = -(*currententity)->curstate.scale;

	if (drawrefract)
		scale = 0;

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

			qglNormal3fv(brushface->normal);
			qglVertex3fv(tempVert);

			if (direction)
				v -= VERTEXSIZE;
			else
				v += VERTEXSIZE;
		}

		qglEnd();
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

		qglUseProgramObjectARB(0);
	}

	EmitWaterPolysWireFrame(fa, direction, useProgram);
}

int *gSkyTexNumber;

skybox_t *skymins;
skybox_t *skymaxs;

vec3_t skyclip[6] =
{
	{ 1, 1, 0 },
	{ 1, -1, 0 },
	{ 0, -1, 1 },
	{ 0, 1, 1 },
	{ 1, 0, 1 },
	{ -1, 0, 1 } 
};

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

void DrawSkyPolygon(int nump, vec3_t vecs)
{
	int i, j;
	vec3_t v, av;
	float s, t, dv;
	int axis;
	float *vp;

	VectorCopy(vec3_origin, v);

	for (i = 0, vp = vecs; i < nump; i++, vp += 3)
	{
		VectorAdd(vp, v, v);
	}

	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);

	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	for (i = 0; i < nump; i++, vecs += 3)
	{
		j = vec_to_st[axis][2];

		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];

		j = vec_to_st[axis][0];

		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j - 1] / dv;

		j = vec_to_st[axis][1];

		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j - 1] / dv;

		if (s < skymins->v[0][axis])
			skymins->v[0][axis] = s;

		if (t < skymins->v[1][axis])
			skymins->v[1][axis] = t;

		if (s > skymaxs->v[0][axis])
			skymaxs->v[0][axis] = s;

		if (t > skymaxs->v[1][axis])
			skymaxs->v[1][axis] = t;
	}
}

#define MAX_CLIP_VERTS 64
#define ON_EPSILON 0.1

void ClipSkyPolygon(int nump, vec3_t vecs, int stage)
{
	float *norm;
	float *v;
	qboolean front, back;
	float d, e;
	float dists[MAX_CLIP_VERTS];
	int sides[MAX_CLIP_VERTS];
	vec3_t newv[2][MAX_CLIP_VERTS];
	int newc[2];
	int i, j;

	if (nump > MAX_CLIP_VERTS - 2)
	{
		gEngfuncs.Con_Printf("ClipSkyPolygon: MAX_CLIP_VERTS");
	}

	if (stage == 6)
	{
		DrawSkyPolygon(nump, vecs);
		return;
	}

	front = back = false;
	norm = skyclip[stage];

	for (i = 0, v = vecs; i < nump; i++, v += 3)
	{
		d = DotProduct(v, norm);

		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;

		dists[i] = d;
	}

	if (!front || !back)
	{
		ClipSkyPolygon(nump, vecs, stage + 1);
		return;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy(vecs, (vecs + (i * 3)));
	newc[0] = newc[1] = 0;

	for (i = 0, v = vecs; i < nump; i++, v += 3)
	{
		switch (sides[i])
		{
			case SIDE_FRONT:
			{
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				break;
			}

			case SIDE_BACK:
			{
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
			}

			case SIDE_ON:
			{
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
			}
		}

		if (sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i + 1]);

		for (j = 0; j < 3; j++)
		{
			e = v[j] + d*(v[j + 3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}

		newc[0]++;
		newc[1]++;
	}

	ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
	ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

void R_DrawSkyChain(msurface_t *s)
{
	msurface_t *fa;

	int i;
	vec3_t verts[MAX_CLIP_VERTS];
	glpoly_t *p;

	for (fa = s; fa; fa = fa->texturechain)
	{
		for (p = fa->polys; p; p = p->next)
		{
			for (i = 0; i < p->numverts; i++)
			{
				VectorSubtract(p->verts[i], r_origin, verts[i]);
			}

			ClipSkyPolygon(p->numverts, verts[0], 0);
		}
	}

	R_UseGBufferProgram(GBUFFER_DIFFUSE_ENABLED);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	R_DrawSkyBox();
}

void R_ClearSkyBox(void)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		skymins->v[0][i] = skymins->v[1][i] = 9999;
		skymaxs->v[0][i] = skymaxs->v[1][i] = -9999;
	}
}

void MakeSkyVec(float s, float t, int axis)
{
	vec3_t v, b;
	int j, k;
	float zmax;
	
	if(r_params.movevars)
		zmax = r_params.movevars->zmax * 0.57735002;
	else
		zmax = 4096 * 0.57735002;

	b[0] = s * zmax;
	b[1] = t * zmax;
	b[2] = zmax;

	for (j = 0; j < 3; j++)
	{
		k = st_to_vec[axis][j];

		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];

		v[j] += r_origin[j];
	}

	s = (s + 1) * 0.5f;
	t = (t + 1) * 0.5f;

	if (s < 1.0 / 512)
		s = 1.0 / 512;
	else if (s > 511.0 / 512)
		s = 511.0 / 512;
	if (t < 1.0 / 512)
		t = 1.0 / 512;
	else if (t > 511.0 / 512)
		t = 511.0 / 512;

	t = 1.0f - t;

	qglTexCoord2f(s, t);
	//qglNormal3fv();
	qglVertex3fv(v);
}

int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };

void R_DrawSkyBox(void)
{
	int i, order;

	/*float vNormalTable[6][3] = { 
		{-1, 0, 0},
		{1, 0, 0},
		{0, -1, 0},
		{0, 1, 0},
		{0, 0, -1},
		{0, 0, 1}
	};*/

	float vNormalTable[6][3] = {
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0}
	};

	for (i = 0; i < 6; i++)
	{
		if (skymins->v[0][i] >= skymaxs->v[0][i] || skymins->v[1][i] >= skymaxs->v[1][i])
			continue;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			order = i;
		}
		else
		{
			order = skytexorder[i];
		}

		/*if(r_wsurf_sky->value > 0 && r_wsurf.iSkyTextures[order])
		{
			GL_Bind(r_wsurf.iSkyTextures[order]);
		}
		else
		{*/
			GL_Bind(gSkyTexNumber[order]);
		//}

		qglBegin(GL_QUADS);
		qglNormal3fv(vNormalTable[i]);
		MakeSkyVec(skymins->v[0][i], skymins->v[1][i], i);
		MakeSkyVec(skymins->v[0][i], skymaxs->v[1][i], i);
		MakeSkyVec(skymaxs->v[0][i], skymaxs->v[1][i], i);
		MakeSkyVec(skymaxs->v[0][i], skymins->v[1][i], i);
		qglEnd();
	}
}