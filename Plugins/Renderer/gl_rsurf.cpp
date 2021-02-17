#include "gl_local.h"
#include "zone.h"

int skytexturenum;

msurface_t **skychain = NULL;
msurface_t **waterchain = NULL;

float current_lightgamma = -1.0;
float current_brightness = -1.0;
float current_gamma = -1.0;

//engine
byte *lightmaps;
int *lightmap_textures;
void *lightmap_rectchange;
int *lightmap_modified;
int *c_brush_polys;
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

void R_BuildLightMap(msurface_t *psurf, byte *dest, int stride)
{
	if (r_light_dynamic->value)
	{
		auto save_dlightactive = (*r_dlightactive);
		(*r_dlightactive) = 0;

		gRefFuncs.R_BuildLightMap(psurf, dest, stride);

		(*r_dlightactive) = save_dlightactive;
	}
	else
	{
		gRefFuncs.R_BuildLightMap(psurf, dest, stride);
	}
}