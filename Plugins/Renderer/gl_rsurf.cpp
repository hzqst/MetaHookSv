#include "gl_local.h"
#include "zone.h"

msurface_t **skychain = NULL;
msurface_t **waterchain = NULL;
int *gl_texsort_value = NULL;

//engine
byte *lightmaps;
int *lightmap_textures;
void *lightmap_rectchange;
int *lightmap_modified;
glpoly_t **lightmap_polys;
int *d_lightstylevalue;
dlight_t *cl_dlights;
int *r_dlightactive;
int *gDecalSurfCount;
msurface_t **gDecalSurfs;
decal_t *gDecalPool;
decalcache_t *gDecalCache;
int *skytexturenum;
int *r_detail_texid;
float *r_detail_texcoord;
float *r_polygon_offset;

void R_RecursiveWorldNode(mnode_t *node)
{
	gRefFuncs.R_RecursiveWorldNode(node);
}

void R_MarkLeaves(void)
{
	//Don't clip bsp nodes when rendering refract or reflect view for non-transparent water.
	if (r_water_novis->value > 0)
	{
		if (r_draw_pass == r_draw_reflect)
		{
			r_novis->value = 1;
		}
	}

	gRefFuncs.R_MarkLeaves();

	r_novis->value = 0;
}

void R_AddDynamicLights(msurface_t *surf)
{
	if (r_light_dynamic->value)
		return;

	return gRefFuncs.R_AddDynamicLights(surf);
}

void R_RenderDynamicLightmaps(msurface_t *fa)
{
	if(!r_light_dynamic->value)
		return gRefFuncs.R_RenderDynamicLightmaps(fa);

	byte *base;
	int maps;
	int smax, tmax;

	if (fa->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	bool dynamic = false;

	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
		{
			dynamic = true;
			break;
		}
	}

	if (dynamic)
	{
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
				glRect_GoldSrc_t *theRect = (glRect_GoldSrc_t *)((char *)lightmap_rectchange + sizeof(glRect_GoldSrc_t) * fa->lightmaptexturenum);

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
			R_BuildLightMap(fa, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
		}
	}
}

void R_BuildLightMap(msurface_t *psurf, byte *dest, int stride)
{
	if (!r_light_dynamic->value)
		return gRefFuncs.R_BuildLightMap(psurf, dest, stride);

	auto save_dlightactive = (*r_dlightactive);
	(*r_dlightactive) = 0;

	gRefFuncs.R_BuildLightMap(psurf, dest, stride);

	(*r_dlightactive) = save_dlightactive;
}

colorVec RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end)
{
	colorVec c;
	float front, back, frac;
	int side;
	mplane_t *plane;
	vec3_t mid;
	msurface_t *surf;
	int s, t, ds, dt;
	int i;
	mtexinfo_t *tex;
	color24 *lightmap;
	unsigned scale;
	int maps;

	if (node->contents < 0)
	{
		c.r = 0;
		c.g = 0;
		c.b = 0;
		c.a = 0;
		return c;
	}

	plane = node->plane;
	front = DotProduct(start, plane->normal) - plane->dist;
	back = DotProduct(end, plane->normal) - plane->dist;
	side = front < 0;

	if ((back < 0) == side)
		return RecursiveLightPoint(node->children[side], start, end);

	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

	c = RecursiveLightPoint(node->children[side], start, mid);

	if (c.r != 0 || c.g != 0 || c.b != 0)
		return c;

	if ((back < 0) == side)
	{
		c.r = 0;
		c.g = 0;
		c.b = 0;
		c.a = 0;
		return c;
	}

	//VectorCopy(mid, lightspot);
	//lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;

	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;

		tex = surf->texinfo;

		s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if (ds > surf->extents[0] || dt > surf->extents[1])
			continue;

		if (!surf->samples)
		{
			c.r = 0;
			c.g = 0;
			c.b = 0;
			c.a = 0;
			return c;
		}

		ds >>= 4;
		dt >>= 4;

		lightmap = (color24 *)surf->samples;

		c.r = 0;
		c.g = 0;
		c.b = 0;
		c.a = 0;

		if (lightmap)
		{
			lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]];
				c.r += lightmap->r * scale;
				c.g += lightmap->g * scale;
				c.b += lightmap->b * scale;
				lightmap += ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
			}

			c.r >>= 8;
			c.g >>= 8;
			c.b >>= 8;

			if (c.r == 0)
				c.r = 1;
		}

		return c;
	}

	return RecursiveLightPoint(node->children[!side], mid, end);
}

colorVec R_LightVec(vec3_t start, vec3_t end)
{
	colorVec c;

	if (r_worldmodel->lightdata)
	{
		c = RecursiveLightPoint(r_worldmodel->nodes, start, end);

		c.r += r_refdef->ambientlight.r;
		c.g += r_refdef->ambientlight.g;
		c.b += r_refdef->ambientlight.b;

		if (c.r > 255)
			c.r = 255;

		if (c.g > 255)
			c.g = 255;

		if (c.b > 255)
			c.b = 255;
	}
	else
	{
		c.r = 255;
		c.g = 255;
		c.b = 255;
		c.a = 0;
	}

	return c;
}

colorVec R_LightPoint(vec3_t p)
{
	vec3_t end;

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	return RecursiveLightPoint(r_worldmodel->nodes, p, end);
}