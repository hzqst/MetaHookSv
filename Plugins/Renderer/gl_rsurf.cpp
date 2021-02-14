#include "gl_local.h"
#include "zone.h"

int skytexturenum;

msurface_t *skychain = NULL;
msurface_t *waterchain = NULL;

float current_lightgamma = -1.0;
float current_brightness = -1.0;
float current_gamma = -1.0;

//engine
byte *lightmaps;
int *gDecalSurfCount;
msurface_t **gDecalSurfs;
int *lightmap_textures;
void *lightmap_rectchange;
int *lightmap_modified;
int *c_brush_polys;
int *d_lightstylevalue;
dlight_t *cl_dlights;

//renderer
qboolean lightmap_updateing;

void R_RecursiveWorldNode(mnode_t *node)
{
	gRefFuncs.R_RecursiveWorldNode(node);
}

void R_MarkLeaves(void)
{
	//Don't clip bsp nodes when rendering refract or reflect view for non-transparent water.
	if (r_water_novis && r_water_novis->value > 0)
	{
		if (drawrefract)
		{
			if (curwater && curwater->color.a == 255)
			{
				r_novis->value = 1;
			}
		}
		else if (drawreflect)
		{
			r_novis->value = 1;
		}
	}

	gRefFuncs.R_MarkLeaves();

	r_novis->value = 0;
}

void R_UploadLightmaps(void)
{
	if (v_lightgamma->value < 1.8)
	{
		Cvar_DirectSet(v_lightgamma, "1.8");
	}
	if (current_lightgamma != v_lightgamma->value || current_brightness != v_brightness->value || current_gamma != v_gamma->value)
	{
		lightmap_updateing = true;
		GL_BuildLightmaps();
		lightmap_updateing = false;
	}
}

void R_AddDynamicLights(msurface_t *surf)
{

}

void R_RenderDynamicLightmaps(msurface_t *fa)
{
	return gRefFuncs.R_RenderDynamicLightmaps(fa);

	byte *base;
	int maps;
	int smax, tmax;

	(*c_brush_polys)++;

	if (fa->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;
	}

	//Disable original dlight
	if (0/*fa->dlightframe == (*r_framecount) || fa->cached_dlight*/)
	{
dynamic:
		if (r_dynamic->value)
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			if (g_iEngineType == ENGINE_SVENGINE)
			{
				glRect_SvEngine_t *theRect = (glRect_SvEngine_t *)((char *)lightmap_rectchange + sizeof(glRect_SvEngine_t) * fa->lightmaptexturenum);

				if (fa->light_t < theRect->t)
				{
					if (theRect->h)
						theRect->h += theRect->t - fa->light_t;

					theRect->t = fa->light_t;
				}

				if (fa->light_s < theRect->l)
				{
					if (theRect->w)
						theRect->w += theRect->l - fa->light_s;

					theRect->l = fa->light_s;
				}

				smax = (fa->extents[0] >> 4) + 1;
				tmax = (fa->extents[1] >> 4) + 1;

				if ((theRect->w + theRect->l) < (fa->light_s + smax))
					theRect->w = (fa->light_s - theRect->l) + smax;

				if ((theRect->h + theRect->t) < (fa->light_t + tmax))
					theRect->h = (fa->light_t - theRect->t) + tmax;
			}
			else
			{
				glRect_t *theRect = (glRect_t *)((char *)lightmap_rectchange + sizeof(glRect_t) * fa->lightmaptexturenum);

				if (fa->light_t < theRect->t)
				{
					if (theRect->h)
						theRect->h += theRect->t - fa->light_t;

					theRect->t = fa->light_t;
				}

				if (fa->light_s < theRect->l)
				{
					if (theRect->w)
						theRect->w += theRect->l - fa->light_s;

					theRect->l = fa->light_s;
				}

				smax = (fa->extents[0] >> 4) + 1;
				tmax = (fa->extents[1] >> 4) + 1;

				if ((theRect->w + theRect->l) < (fa->light_s + smax))
					theRect->w = (fa->light_s - theRect->l) + smax;

				if ((theRect->h + theRect->t) < (fa->light_t + tmax))
					theRect->h = (fa->light_t - theRect->t) + tmax;
			}

			base = lightmaps + fa->lightmaptexturenum * LIGHTMAP_BYTES * BLOCK_WIDTH * BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * LIGHTMAP_BYTES + fa->light_s * LIGHTMAP_BYTES;
			gRefFuncs.R_BuildLightMap(fa, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
		}
	}
}

void BuildSurfaceDisplayList(msurface_t *fa, model_t *model)
{
	int i, lindex, lnumverts;
	medge_t *pedges, *r_pedge;
	int vertpage;
	float *vec;
	float s, t;
	glpoly_t *poly;
	mvertex_t *vertbase;

	if (lightmap_updateing)
		return;

	vertbase = model->vertexes;
	pedges = model->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	poly = (glpoly_t *)Hunk_Alloc(sizeof(glpoly_t) + (lnumverts - 4) * VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = model->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = vertbase[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = vertbase[r_pedge->v[1]].position;
		}

		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy(vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	if (!gl_keeptjunctions->value && !(fa->flags & SURF_UNDERWATER))
	{
		for (i = 0; i < lnumverts; ++i)
		{
			vec3_t v1, v2;
			float *prev, *thisPoint, *next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			thisPoint = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract(thisPoint, prev, v1);
			VectorNormalize(v1);
			VectorSubtract(next, prev, v2);
			VectorNormalize(v2);

			if ((fabs(v1[0] - v2[0]) <= COLINEAR_EPSILON) && (fabs(v1[1] - v2[1]) <= COLINEAR_EPSILON) && (fabs(v1[2] - v2[2]) <= COLINEAR_EPSILON))
			{
				int j;

				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;

					for (k = 0; k < VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}

				--lnumverts;
				--i;
			}
		}
	}

	poly->numverts = lnumverts;
}

int R_LightmapAllocBlock(int w, int h, int *x, int *y)
{
	int i, j;
	int best, best2;
	int texnum;

	for (texnum = 0; texnum < MAX_LIGHTMAPS; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i = 0; i < BLOCK_WIDTH - w; i++)
		{
			best2 = 0;

			for (j = 0; j < w; j++)
			{
				if (lightmaps[texnum * BLOCK_WIDTH + i + j] >= best)
					break;

				if (lightmaps[texnum * BLOCK_WIDTH + i + j] > best2)
					best2 = lightmaps[texnum * BLOCK_WIDTH + i + j];
			}

			if (j == w)
			{
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i = 0; i < w; i++)
			lightmaps[texnum * BLOCK_WIDTH + (*x) + i] = best + h;

		return texnum;
	}

	Sys_ErrorEx("AllocBlock: full");
	return 0;
}

void GL_CreateSurfaceLightmap(msurface_t *surf)
{
	int smax, tmax;
	byte *base;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	if ((surf->flags & SURF_DRAWTILED) && surf->texinfo->flags & TEX_SPECIAL)
		return;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;

	surf->lightmaptexturenum = R_LightmapAllocBlock(smax, tmax, &surf->light_s, &surf->light_t);
	base = lightmaps + surf->lightmaptexturenum * LIGHTMAP_BYTES * BLOCK_WIDTH * BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;
	gRefFuncs.R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
}

void GL_BuildLightmaps(void)
{
	current_lightgamma = v_lightgamma->value;
	current_brightness = v_brightness->value;
	current_gamma = v_gamma->value;

	int i, j;
	model_t *m;

	memset(lightmaps, 0, sizeof(int)*BLOCK_WIDTH*MAX_LIGHTMAPS);

	*r_framecount = 1;

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		if (!lightmap_textures[i])
			lightmap_textures[i] = GL_GenTexture();
	}

	for (j = 1; j < MAX_MODELS; j++)
	{
		m = IEngineStudio.GetModelByIndex(j);

		if (!m)
			break;

		if (m->name[0] == '*')
			continue;

		for (i = 0; i < m->numsurfaces; i++)
		{
			GL_CreateSurfaceLightmap(m->surfaces + i);

			if (m->surfaces[i].flags & SURF_DRAWTURB)
				continue;

			BuildSurfaceDisplayList(m->surfaces + i, m);
		}
	}

	GL_SelectTexture(TEXTURE1_SGIS);

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		if (!lightmaps[i * BLOCK_WIDTH + 0])
			break;

		GL_Bind(lightmap_textures[i]);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		qglTexImage2D(GL_TEXTURE_2D, 0, 4, BLOCK_WIDTH, BLOCK_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + i * BLOCK_WIDTH * BLOCK_HEIGHT * LIGHTMAP_BYTES);
	}

 	GL_SelectTexture(TEXTURE0_SGIS);
}

void R_DecalMPoly(float *v, texture_t *ptexture, msurface_t *psurf, int vertCount)
{
	int j;

	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(ptexture->gl_texturenum);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	GL_EnableMultitexture();
	GL_Bind(lightmap_textures[psurf->lightmaptexturenum]);

	qglBegin(GL_POLYGON);
	for (j = 0; j < vertCount; j++, v += VERTEXSIZE)
	{
		qglMultiTexCoord2fARB(TEXTURE0_SGIS, v[3], v[4]);
		qglMultiTexCoord2fARB(TEXTURE1_SGIS, v[5], v[6]);
		qglVertex3fv(v);
	}
	qglEnd();

	qglColor4ub(255, 255, 255, 255);
}