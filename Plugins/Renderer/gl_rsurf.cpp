#include "gl_local.h"
#include "zone.h"

int skytexturenum;

msurface_t **skychain = NULL;
msurface_t **waterchain = NULL;
int *gl_texsort_value = NULL;

//engine
byte *lightmaps;
int *lightmap_textures;
void *lightmap_rectchange;
int *lightmap_modified;
glpoly_t **lightmap_polys;
int *c_brush_polys;
int *c_alias_polys;
int *d_lightstylevalue;
dlight_t *cl_dlights;
int *r_dlightactive;
int *gDecalSurfCount;
msurface_t **gDecalSurfs;
decal_t *gDecalPool;
decalcache_t *gDecalCache;

//renderer
qboolean lightmap_updateing;

void R_RecursiveWorldNode(mnode_t *node)
{
	gRefFuncs.R_RecursiveWorldNode(node);
}

void R_MarkLeaves(void)
{
	//Don't clip bsp nodes when rendering refract or reflect view for non-transparent water.
	if (r_water_novis->value > 0)
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

	(*c_brush_polys)++;

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

void R_BlendLightmaps(void)
{
	return;
}