#include "gl_local.h"
#include <algorithm>

msurface_t **skychain = NULL;
msurface_t **waterchain = NULL;

//engine
byte *lightmaps = NULL;
int *lightmap_textures = NULL;
void *lightmap_rectchange = NULL;
int *lightmap_modified = NULL;
glpoly_t **lightmap_polys = NULL;
int *d_lightstylevalue = NULL;
dlight_t *cl_dlights = NULL;
dlight_t* cl_elights = NULL;
int *r_dlightactive = NULL;
int *gDecalSurfCount = NULL;
msurface_t **gDecalSurfs = NULL;
decal_t *gDecalPool = NULL;
decalcache_t *gDecalCache = NULL;

CDecalDrawBatch g_DecalDrawBatch = { 0 };

void R_RecursiveWorldNode(mnode_t *node)
{
	gPrivateFuncs.R_RecursiveWorldNode(node);
}

void R_AddDynamicLights(msurface_t *surf)
{
	//All moved to shader
}

decal_t* EngineGetDecalByIndex(int index)
{
	return &gDecalPool[index];
}

int EngineGetMaxDecalCount()
{
	return MAX_DECALS;
}

void R_PrepareDecals(void)
{
	for (int i = 0; i < EngineGetMaxDecalCount(); ++i)
	{
		auto decal = EngineGetDecalByIndex(i);

		if (decal->psurface)
		{
			auto ent = gEngfuncs.GetEntityByIndex(decal->entityIndex);

			auto pEntityComponentContainer = R_GetEntityComponentContainer(ent, true);

			if (pEntityComponentContainer)
			{
				pEntityComponentContainer->Decals.emplace_back(decal);
			}
		}
	}
}

//It's 40 x 40 for SvEngine and 18 x 18 for GoldSrc
static colorVec blocklights[40 * 40];

static int allocated[MAX_LIGHTMAPS_SVENGINE][BLOCK_WIDTH];

void R_BuildLightMap(msurface_t *psurf, byte *dest, int stride, int lightmap_idx)
{
	int maxSize = sizeof(blocklights) / sizeof(colorVec);

	int smax = (psurf->extents[0] >> 4) + 1;
	int tmax = (psurf->extents[1] >> 4) + 1;
	int size = smax * tmax;
	auto lightmap = ((color24 *)psurf->samples);

	if (size > maxSize)
	{
		Sys_Error("R_BuildLightMap: lightmap for texture %s too large (%d x %d = %d luxels); cannot exceed %d\n", psurf->texinfo->texture, smax, tmax, size, maxSize);
	}

	if (lightmap)
	{
		lightmap += size * lightmap_idx;

		if (psurf->styles[lightmap_idx] != 255)
		{
			for (int i = 0; i < size; i++)
			{
				blocklights[i].r = lightmap[i].r;
				blocklights[i].g = lightmap[i].g;
				blocklights[i].b = lightmap[i].b;
			}
			g_WorldSurfaceRenderer.iLightmapUsedBits |= (1 << lightmap_idx);
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				blocklights[i].r = 0;
				blocklights[i].g = 0;
				blocklights[i].b = 0;
			}
		}
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			blocklights[i].r = 255;
			blocklights[i].g = 255;
			blocklights[i].b = 255;
		}
	}

	//Dynamic lights should be calculated in shader

	stride -= smax * 4;

	int k = 0;
	for (int i = 0; i < tmax; i++, dest += stride)
	{
		for (int j = 0; j < smax; j++)
		{
			dest[0] = blocklights[k].r;
			dest[1] = blocklights[k].g;
			dest[2] = blocklights[k].b;
			dest[3] = 255;

			k ++;
			dest += LIGHTMAP_BYTES;
		}
	}
}

void R_RenderDynamicLightmaps(msurface_t *fa)
{
	//All moved to shader
}

int AllocBlock(int w, int h, int *x, int *y)
{
	int i, j;
	int best, best2;
	int texnum;

	for (texnum = 0; texnum < EngineGetMaxLightmapTextures(); texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i = 0; i < BLOCK_WIDTH - w; i++)
		{
			best2 = 0;

			for (j = 0; j < w; j++)
			{
				if (allocated[texnum][i + j] >= best)
					break;

				if (allocated[texnum][i + j] > best2)
					best2 = allocated[texnum][i + j];
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
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error("AllocBlock: full");
	return 0;
}

void R_AllocateSurfaceLightmap(model_t *mod, msurface_t *surf)
{
	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	if ((surf->flags & SURF_DRAWTILED) && surf->texinfo->flags & TEX_SPECIAL)
		return;

	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;

	surf->lightmaptexturenum = AllocBlock(smax, tmax, &surf->light_s, &surf->light_t);

	if (surf->lightmaptexturenum + 1 > g_WorldSurfaceRenderer.iNumLightmapTextures)
		g_WorldSurfaceRenderer.iNumLightmapTextures = surf->lightmaptexturenum + 1;
}

void R_BuildSurfaceLightmap(model_t *mod, msurface_t *surf, int lightmap_idx)
{
	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	if ((surf->flags & SURF_DRAWTILED) && surf->texinfo->flags & TEX_SPECIAL)
		return;

	auto base = lightmaps + surf->lightmaptexturenum * LIGHTMAP_BYTES * BLOCK_WIDTH * BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES, lightmap_idx);
}

void R_BuildSurfaceDisplayList(model_t *mod, mvertex_t *vertbase, msurface_t *fa)
{
	int i, lindex, lnumverts;
	medge_t *pedges, *r_pedge;
	float *vec;
	float s, t;

	pedges = mod->edges;
	lnumverts = fa->numedges;

	auto poly = (glpoly_t *)Hunk_AllocName(sizeof(glpoly_t) + (lnumverts - 4) * VERTEXSIZE * sizeof(float), "unknown");
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = mod->surfedges[fa->firstedge + i];

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

#define COLINEAR_EPSILON 0.001

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

void GL_BuildLightmaps(void)
{
	//Should always be zero when loading a new map
	g_WorldSurfaceRenderer.iNumLightmapTextures = 0;
	g_WorldSurfaceRenderer.vWorldModels.clear();

	memset(allocated, 0, sizeof(allocated));

	//This moved to end of R_NewMap
	//(*r_framecount) = 1;

	for (int j = 1; j < EngineGetMaxClientModels(); j++)
	{
		auto mod = gEngfuncs.hudGetModelByIndex(j);

		if (!mod)
			break;

		if (mod->type == mod_brush && mod->name[0] != '*')
		{
			//Generate vertex buffer and iNumLightmapTextures first

			g_WorldSurfaceRenderer.vWorldModels.emplace_back(mod);
		}
	}

	for (auto mod : g_WorldSurfaceRenderer.vWorldModels)
	{
		for (int i = 0; i < mod->numsurfaces; i++)
		{
			//Allocate lightmap texture from empty slot
			auto surf = R_GetWorldSurfaceByIndex(mod, i);

			//Allocate blocks for lightmap
			R_AllocateSurfaceLightmap(mod, surf);

			//Build glpolys from blocks
			if (!(surf->flags & SURF_DRAWTURB))
			{
				R_BuildSurfaceDisplayList(mod, mod->vertexes, surf);
			}
		}
	}

	if (g_WorldSurfaceRenderer.vWorldModels.empty())
		return;

	g_WorldSurfaceRenderer.iLightmapUsedBits = 0;

	for (int lightmap_idx = 0; lightmap_idx < MAXLIGHTMAPS; ++lightmap_idx)
	{
		g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx] = GL_GenTexture();

		glBindTexture(GL_TEXTURE_2D_ARRAY, g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Can this be GL_RGB8 to save VRAM? idk

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, BLOCK_WIDTH, BLOCK_HEIGHT, g_WorldSurfaceRenderer.iNumLightmapTextures);
		//glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, BLOCK_WIDTH, BLOCK_HEIGHT, g_WorldSurfaceRenderer.iNumLightmapTextures, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		memset(lightmaps, 0, sizeof(BLOCK_WIDTH * BLOCK_HEIGHT * LIGHTMAP_BYTES) * g_WorldSurfaceRenderer.iNumLightmapTextures);

		for (auto mod : g_WorldSurfaceRenderer.vWorldModels)
		{
			for (int i = 0; i < mod->numsurfaces; i++)
			{
				//Fill lightmap color bytes into lightmaps[]

				auto surf = R_GetWorldSurfaceByIndex(mod, i);

				R_BuildSurfaceLightmap(mod, surf, lightmap_idx);
			}
		}

		//Upload bytes to GPU
		for (int i = 0; i < g_WorldSurfaceRenderer.iNumLightmapTextures; ++i)
		{
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, BLOCK_WIDTH, BLOCK_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + BLOCK_WIDTH * BLOCK_HEIGHT * LIGHTMAP_BYTES * i);
		}

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}
}

colorVec RecursiveLightPoint(mbasenode_t *basenode, vec3_t start, vec3_t end)
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

	if (basenode->contents < 0)
	{
		c.r = 0;
		c.g = 0;
		c.b = 0;
		c.a = 0;
		return c;
	}

	auto node = (mnode_t*)basenode;
	auto mod = R_FindWorldModelByNode(node);

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

	for (i = 0; i < node->numsurfaces; i++)
	{
		auto surf = R_GetWorldSurfaceByIndex(mod, node->firstsurface + i);

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

			//if (c.r == 0)
			//	c.r = 1;
		}

		return c;
	}

	return RecursiveLightPoint(node->children[!side], mid, end);
}

colorVec R_LightVec(vec3_t start, vec3_t end)
{
	colorVec c;

	if ((*cl_worldmodel)->lightdata)
	{
		c = RecursiveLightPoint((*cl_worldmodel)->nodes, start, end);

		c.r += (*r_refdef.ambientlight).r;
		c.g += (*r_refdef.ambientlight).g;
		c.b += (*r_refdef.ambientlight).b;

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

	return RecursiveLightPoint((*cl_worldmodel)->nodes, p, end);
}

static int R_DecalIndex(decal_t *pdecal)
{
	return (pdecal - gDecalPool);
}

#define DECAL_CACHEENTRY	256

static int R_DecalCacheIndex(int index)
{
	return index & (DECAL_CACHEENTRY - 1);
}

static decalcache_t *R_DecalCacheSlot(int decalIndex)
{
	int				cacheIndex;

	cacheIndex = R_DecalCacheIndex(decalIndex);	// Find the cache slot

	return gDecalCache + cacheIndex;
}

// Release the cache entry for this decal
static void R_DecalCacheClear(decal_t *pdecal)
{
	int				index;
	decalcache_t	*pCache;

	index = R_DecalIndex(pdecal);
	pCache = R_DecalCacheSlot(index);		// Find the cache slot

	if (pCache->decalIndex == index)		// If this is the decal that's cached here, clear it.
		pCache->decalIndex = -1;
}

static float vert[MAX_DECALCLIPVERT][VERTEXSIZE];
static float outvert[MAX_DECALCLIPVERT][VERTEXSIZE];

// Generate lighting coordinates at each vertex for decal vertices v[] on surface psurf
void R_DecalVertsLight(float *v, msurface_t *psurf, int vertCount)
{
	int j;
	float s, t;

	for (j = 0; j < vertCount; j++, v += VERTEXSIZE)
	{
		s = DotProduct(v, psurf->texinfo->vecs[0]) + psurf->texinfo->vecs[0][3];
		s -= psurf->texturemins[0];
		s += psurf->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16;

		t = DotProduct(v, psurf->texinfo->vecs[1]) + psurf->texinfo->vecs[1][3];
		t -= psurf->texturemins[1];
		t += psurf->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16;

		v[5] = s;
		v[6] = t;
	}
}

// Quick and dirty sutherland Hodgman clipper
// Clip polygon to decal in texture space
// JAY: This code is lame, change it later.  It does way too much work per frame
// It can be made to recursively call the clipping code and only copy the vertex list once
int Inside(float *vert, int edge)
{
	switch (edge) {
	case 0:		// left
		if (vert[3] > 0.0)
			return 1;
		return 0;
	case 1:		// right
		if (vert[3] < 1.0)
			return 1;
		return 0;

	case 2:		// top
		if (vert[4] > 0.0)
			return 1;
		return 0;

	case 3:
		if (vert[4] < 1.0)
			return 1;
		return 0;
	}
	return 0;
}

void Intersect(float *one, float *two, int edge, float *out)
{
	float t;

	// t is the parameter of the line between one and two clipped to the edge
	// or the fraction of the clipped point between one & two
	// vert[3] is u
	// vert[4] is v
	// vert[0], vert[1], vert[2] is X, Y, Z
	if (edge < 2) {
		if (edge == 0) {	// left
			t = ((one[3] - 0) / (one[3] - two[3]));
			out[3] = 0;
		}
		else {				// right
			t = ((one[3] - 1) / (one[3] - two[3]));
			out[3] = 1;
		}
		out[4] = one[4] + (two[4] - one[4]) * t;
	}
	else {
		if (edge == 2) {	// top
			t = ((one[4] - 0) / (one[4] - two[4]));
			out[4] = 0;
		}
		else {				// bottom
			t = ((one[4] - 1) / (one[4] - two[4]));
			out[4] = 1;
		}
		out[3] = one[3] + (two[3] - one[3]) * t;
	}
	out[0] = one[0] + (two[0] - one[0]) * t;
	out[1] = one[1] + (two[1] - one[1]) * t;
	out[2] = one[2] + (two[2] - one[2]) * t;
}

int SHClip(float *vert, int vertCount, float *out, int edge)
{
	int		j, outCount;
	float	*s, *p;

	outCount = 0;

	s = &vert[(vertCount - 1) * VERTEXSIZE];
	for (j = 0; j < vertCount; j++) {
		p = &vert[j * VERTEXSIZE];
		if (Inside(p, edge)) {
			if (Inside(s, edge)) {
				// Add a vertex and advance out to next vertex
				memcpy(out, p, sizeof(float)*VERTEXSIZE);
				outCount++;
				out += VERTEXSIZE;
			}
			else {
				Intersect(s, p, edge, out);
				out += VERTEXSIZE;
				outCount++;
				memcpy(out, p, sizeof(float)*VERTEXSIZE);
				outCount++;
				out += VERTEXSIZE;
			}
		}
		else {
			if (Inside(s, edge)) {
				Intersect(p, s, edge, out);
				out += VERTEXSIZE;
				outCount++;
			}
		}

		s = p;
	}

	return outCount;
}

//-----------------------------------------------------------------------------
// Generate clipped vertex list for decal pdecal projected onto polygon psurf
//-----------------------------------------------------------------------------
float *R_DecalVertsClip(
	float *poutVerts,
	decal_t *pdecal,
	msurface_t *psurf,
	texture_t *ptexture,
	int *pvertCount)
{
	float *v;
	float scalex, scaley;
	int j, outCount;

	scalex = (psurf->texinfo->texture->width * pdecal->scale) / (float)ptexture->width;
	scaley = (psurf->texinfo->texture->height * pdecal->scale) / (float)ptexture->height;

	if (poutVerts == NULL)
		poutVerts = (float *)&vert[0];

	v = psurf->polys->verts[0];

	for (j = 0; j < psurf->polys->numverts; j++, v += VERTEXSIZE)
	{
		VectorCopy(v, vert[j]);
		vert[j][3] = (v[3] - pdecal->dx) * scalex;
		vert[j][4] = (v[4] - pdecal->dy) * scaley;

		if (pdecal->flags & FDECAL_HFLIP)
			vert[j][3] = 1 - vert[j][3];

		if (pdecal->flags & FDECAL_VFLIP)
			vert[j][4] = 1 - vert[j][4];
	}

	// Clip the polygon to the decal texture space
	outCount = SHClip((float *)&vert[0], psurf->polys->numverts, (float *)&outvert[0], 0);
	outCount = SHClip((float *)&outvert[0], outCount, (float *)&vert[0], 1);
	outCount = SHClip((float *)&vert[0], outCount, (float *)&outvert[0], 2);
	outCount = SHClip((float *)&outvert[0], outCount, poutVerts, 3);

	if (outCount)
	{
		if (pdecal->flags & FDECAL_CLIPTEST)
		{
			pdecal->flags &= ~FDECAL_CLIPTEST;	// We're doing the test

			// If there are exactly 4 verts and they are all 0,1 tex coords, then we've got an unclipped decal
			// A more precise test would be to calculate the texture area and make sure it's one, but this
			// should work as well.
			if (outCount == 4)
			{
				int clipped = 0;
				float s, t;

				v = poutVerts;
				for (j = 0; j < outCount && !clipped; j++, v += VERTEXSIZE)
				{
					s = v[3];
					t = v[4];

					if ((s != 0.0 && s != 1.0) || (t != 0.0 && t != 1.0))
						clipped = 1;
				}

				// We didn't need to clip this decal, it's a quad covering the full texture space, optimize
				// subsequent frames.
				if (!clipped)
					pdecal->flags |= FDECAL_NOCLIP;
			}
		}
	}

	*pvertCount = outCount;
	return poutVerts;
}

static float *R_DecalVertsNoclip(decal_t *pdecal, msurface_t *psurf, texture_t *ptexture, qboolean bMultitexture)
{
	float			*vlist;
	decalcache_t	*pCache;
	int				decalIndex;
	int				outCount;

	decalIndex = R_DecalIndex(pdecal);
	pCache = R_DecalCacheSlot(decalIndex);

	// Is the decal cached?
	if (pCache->decalIndex == decalIndex)
	{
		return (float *)&pCache->decalVert[0];
	}

	pCache->decalIndex = decalIndex;

	vlist = pCache->decalVert[0];

	// Use the old code for now, and just cache them
	vlist = R_DecalVertsClip(vlist, pdecal, psurf, ptexture, &outCount);

	R_DecalVertsLight(vlist, psurf, 4);

	return vlist;
}

void R_UploadDecalTextures(int decalIndex, texture_t* ptexture, const std::shared_ptr<CWorldSurfaceRenderMaterial>& pRenderMaterial)
{
	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltexturenum = ptexture->gl_texturenum;
	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltexturewidth = ptexture->width;
	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltextureheight = ptexture->height;
	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].pRenderMaterial = pRenderMaterial;

	auto matId = R_FindWorldMaterialId(ptexture->gl_texturenum);

	if (matId == (uint32_t)-1)
	{
		world_material_t mat;

		mat.detailtexcoord[0] = 1;
		mat.detailtexcoord[1] = 1;
		mat.normaltexcoord[0] = 1;
		mat.normaltexcoord[1] = 1;
		mat.parallaxtexcoord[0] = 1;
		mat.parallaxtexcoord[1] = 1;
		mat.speculartexcoord[0] = 1;
		mat.speculartexcoord[1] = 1;

		if (pRenderMaterial)
		{
			if (pRenderMaterial->textures[WSURF_DETAIL_TEXTURE].gltexturenum)
			{
				mat.detailtexcoord[0] = pRenderMaterial->textures[WSURF_DETAIL_TEXTURE].scaleX;
				mat.detailtexcoord[1] = pRenderMaterial->textures[WSURF_DETAIL_TEXTURE].scaleY;
			}
			if (pRenderMaterial->textures[WSURF_NORMAL_TEXTURE].gltexturenum)
			{
				mat.normaltexcoord[0] = pRenderMaterial->textures[WSURF_NORMAL_TEXTURE].scaleX;
				mat.normaltexcoord[1] = pRenderMaterial->textures[WSURF_NORMAL_TEXTURE].scaleY;
			}
			if (pRenderMaterial->textures[WSURF_PARALLAX_TEXTURE].gltexturenum)
			{
				mat.parallaxtexcoord[0] = pRenderMaterial->textures[WSURF_PARALLAX_TEXTURE].scaleX;
				mat.parallaxtexcoord[1] = pRenderMaterial->textures[WSURF_PARALLAX_TEXTURE].scaleY;
			}
			if (pRenderMaterial->textures[WSURF_SPECULAR_TEXTURE].gltexturenum)
			{
				mat.speculartexcoord[0] = pRenderMaterial->textures[WSURF_SPECULAR_TEXTURE].scaleX;
				mat.speculartexcoord[1] = pRenderMaterial->textures[WSURF_SPECULAR_TEXTURE].scaleY;
			}
		}

		matId = (uint32_t)g_WorldSurfaceRenderer.vWorldMaterials.size();

		g_WorldSurfaceRenderer.vWorldMaterialTextureMapping.emplace_back(ptexture->gl_texturenum);
		g_WorldSurfaceRenderer.vWorldMaterials.emplace_back(mat);

		GL_UploadDataToSSBOStaticDraw(g_WorldSurfaceRenderer.hMaterialSSBO, sizeof(world_material_t) * g_WorldSurfaceRenderer.vWorldMaterials.size(), g_WorldSurfaceRenderer.vWorldMaterials.data());
	}

	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].matId = matId;
}

void R_UploadDecalVertexBuffer(int decalIndex, int vertCount, float *v, msurface_t *surf, CWorldSurfaceRenderMaterial* pRenderMaterial)
{
	auto worldmodel = R_FindWorldModelBySurface(surf);

	if (!worldmodel)
	{
		Sys_Error("R_UploadDecalVertexBuffer: Failed to get worldmodel by surface");
		return;
	}

	auto pWorldModel = R_GetWorldSurfaceWorldModel(worldmodel);

	if (!pWorldModel)
	{
		Sys_Error("R_UploadDecalVertexBuffer: Failed to R_GetWorldSurfaceWorldModel");
		return;
	}

	auto surfIndex = R_GetWorldSurfaceIndex(worldmodel, surf);

	if (surfIndex == -1)
	{
		Sys_Error("R_UploadDecalVertexBuffer: invalid surfIndex!");
		return;
	}

	const auto brushface = &pWorldModel->m_vFaceBuffer[surfIndex];

	std::vector<vertex3f_t> vPolyVertices;
	std::vector<decalvertex_t> vVertexDataBuffer;
	decalinstancedata_t vInstanceDataBuffer[1];

	vPolyVertices.reserve(MAX_DECALVERTS);
	vVertexDataBuffer.reserve(MAX_DECALVERTS);

	for (int j = 0; j < vertCount && j < MAX_DECALVERTS; ++j)
	{
		vertex3f_t tempVertex;

		tempVertex.v[0] = v[0];
		tempVertex.v[1] = v[1];
		tempVertex.v[2] = v[2];

		decalvertex_t tempVertexData;

		tempVertexData.pos[0] = v[0];
		tempVertexData.pos[1] = v[1];
		tempVertexData.pos[2] = v[2];

		tempVertexData.texcoord[0] = v[3];
		tempVertexData.texcoord[1] = v[4];

		tempVertexData.lightmaptexcoord[0] = v[5];
		tempVertexData.lightmaptexcoord[1] = v[6];

		vPolyVertices.emplace_back(tempVertex);
		vVertexDataBuffer.emplace_back(tempVertexData);

		v += VERTEXSIZE;
	}

	decalinstancedata_t tempInstanceData;

	tempInstanceData.normal[0] = brushface->normal[0];
	tempInstanceData.normal[1] = brushface->normal[1];
	tempInstanceData.normal[2] = brushface->normal[2];
	tempInstanceData.s_tangent[0] = brushface->s_tangent[0];
	tempInstanceData.s_tangent[1] = brushface->s_tangent[1];
	tempInstanceData.s_tangent[2] = brushface->s_tangent[2];
	tempInstanceData.t_tangent[0] = brushface->t_tangent[0];
	tempInstanceData.t_tangent[1] = brushface->t_tangent[1];
	tempInstanceData.t_tangent[2] = brushface->t_tangent[2];

	tempInstanceData.lightmaptexturenum[0] = surf->lightmaptexturenum;
	tempInstanceData.lightmaptexturenum[1] = 0;

	memcpy(&tempInstanceData.styles, surf->styles, sizeof(surf->styles));

	tempInstanceData.matId = g_WorldSurfaceRenderer.vCachedDecals[decalIndex].matId;

	vInstanceDataBuffer[0] = tempInstanceData;

	std::vector<uint32_t> vTriangleListIndices;

	vTriangleListIndices.reserve(MAX_DECALINDICES);

	R_PolygonToTriangleList(vPolyVertices, vTriangleListIndices);

	for (size_t i = 0; i < vTriangleListIndices.size(); ++i)
	{
		vTriangleListIndices[i] += decalIndex * MAX_DECALVERTS;
	}

	GL_UploadSubDataToVBO(g_WorldSurfaceRenderer.hDecalVBO[WSURF_VBO_VERTEX], sizeof(decalvertex_t) * MAX_DECALVERTS * decalIndex, sizeof(decalvertex_t) * vVertexDataBuffer.size(), vVertexDataBuffer.data());
	GL_UploadSubDataToVBO(g_WorldSurfaceRenderer.hDecalVBO[WSURF_VBO_INSTANCE], sizeof(decalinstancedata_t) * _countof(vInstanceDataBuffer) * decalIndex, sizeof(decalinstancedata_t) * _countof(vInstanceDataBuffer), vInstanceDataBuffer);
	GL_UploadSubDataToEBO(g_WorldSurfaceRenderer.hDecalEBO, sizeof(uint32_t) * MAX_DECALINDICES * decalIndex, sizeof(uint32_t) * vTriangleListIndices.size(), vTriangleListIndices.data());

	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].startIndex = MAX_DECALINDICES * decalIndex;
	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].indiceCount = vTriangleListIndices.size();
	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].startInstance = decalIndex;
	g_WorldSurfaceRenderer.vCachedDecals[decalIndex].instanceCount = 1;
}

bool R_IsDecalCacheInvalidated(int decalIndex, texture_t *texture, const std::shared_ptr<CWorldSurfaceRenderMaterial>& pRenderMaterial)
{
	if (g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltexturenum != texture->gl_texturenum ||
		g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltexturewidth != texture->width ||
		g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltextureheight != texture->height ||
		g_WorldSurfaceRenderer.vCachedDecals[decalIndex].pRenderMaterial != pRenderMaterial)
	{
		return true;
	}

	return false;
}

void R_DrawDecals(cl_entity_t *ent)
{
	if (CL_IsDevOverviewMode())
		return;

	if (R_IsRenderingShadowView())
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		if (ent->curstate.effects & EF_NODECALS)
			return;
	}

	auto pEntityComponentContainer = R_GetEntityComponentContainer(ent, false);

	if (!pEntityComponentContainer)
		return;

	if (pEntityComponentContainer->Decals.empty())
		return;

	g_DecalDrawBatch.BatchCount = 0;

	for (size_t i = 0; i < pEntityComponentContainer->Decals.size(); i++)
	{
		auto plist = pEntityComponentContainer->Decals[i];

		if (plist)
		{
			int decalIndex = R_DecalIndex(plist);

			auto ptexture = Draw_DecalTexture(plist->texture);

			auto pRenderMaterial = R_FindDecalTextureCache(ptexture->name);

			//Build VBO data for this decal if not built yet
			if (!(plist->flags & FDECAL_VBO) || R_IsDecalCacheInvalidated(decalIndex, ptexture, pRenderMaterial))
			{
				int vertCount = 0;
				float *v = nullptr;

				auto psurf = plist->psurface;

				if (plist->flags & FDECAL_NOCLIP)
				{
					v = R_DecalVertsNoclip(plist, psurf, ptexture, g_WorldSurfaceRenderer.bLightmapTexture);
					vertCount = 4;
				}
				else
				{
					v = R_DecalVertsClip(NULL, plist, psurf, ptexture, &vertCount);

					if (vertCount > 0 && g_WorldSurfaceRenderer.bLightmapTexture)
					{
						R_DecalVertsLight(v, psurf, vertCount);
					}
				}

				if (vertCount > 0)
				{
					R_UploadDecalTextures(decalIndex, ptexture, pRenderMaterial);
					R_UploadDecalVertexBuffer(decalIndex, vertCount, v, psurf, pRenderMaterial.get());
				}
				else
				{
					g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltexturenum = 0;
					g_WorldSurfaceRenderer.vCachedDecals[decalIndex].indiceCount = 0;
					g_WorldSurfaceRenderer.vCachedDecals[decalIndex].instanceCount = 0;
				}

				//Mark this decal as ready
				plist->flags |= FDECAL_VBO;
			}

			if (g_WorldSurfaceRenderer.vCachedDecals[decalIndex].indiceCount > 0)
			{
				if (g_DecalDrawBatch.BatchCount < MAX_DECALS)
				{
					g_DecalDrawBatch.GLTextureId[g_DecalDrawBatch.BatchCount] = g_WorldSurfaceRenderer.vCachedDecals[decalIndex].gltexturenum;
					g_DecalDrawBatch.StartIndex[g_DecalDrawBatch.BatchCount] = g_WorldSurfaceRenderer.vCachedDecals[decalIndex].startIndex;
					g_DecalDrawBatch.IndiceCount[g_DecalDrawBatch.BatchCount] = g_WorldSurfaceRenderer.vCachedDecals[decalIndex].indiceCount;
					g_DecalDrawBatch.pRenderMaterial[g_DecalDrawBatch.BatchCount] = g_WorldSurfaceRenderer.vCachedDecals[decalIndex].pRenderMaterial.get();
					g_DecalDrawBatch.StartInstance[g_DecalDrawBatch.BatchCount] = g_WorldSurfaceRenderer.vCachedDecals[decalIndex].startInstance;
					g_DecalDrawBatch.InstanceCount[g_DecalDrawBatch.BatchCount] = g_WorldSurfaceRenderer.vCachedDecals[decalIndex].instanceCount;
					++g_DecalDrawBatch.BatchCount;
				}
			}
		}
	}

	GL_BeginDebugGroup("R_DrawDecals");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);

	GL_BeginStencilCompareEqual(STENCIL_MASK_HAS_DECAL, STENCIL_MASK_HAS_DECAL);

	//Decal only affects diffuse, normal and specular
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE | GBUFFER_MASK_WORLDNORM | GBUFFER_MASK_SPECULAR);

	//Use AlphaBlend to blend with GBuffer.diffuse
	R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);

		if (gl_ztrick && gl_ztrick->value)
			glPolygonOffset(1, gl_polyoffset->value);
		else
			glPolygonOffset(-1, -gl_polyoffset->value);
	}
	
	program_state_t WSurfProgramState = WSURF_DECAL_ENABLED | WSURF_DIFFUSE_ENABLED;

	//Mix lightmap if not deferred
	if (g_WorldSurfaceRenderer.bLightmapTexture)
	{
		WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;

		if (r_fullbright->value || !(*cl_worldmodel)->lightdata)
		{
			WSurfProgramState |= WSURF_FULLBRIGHT_ENABLED;
		}

		if (*filterMode != 0)
		{
			WSurfProgramState |= WSURF_COLOR_FILTER_ENABLED;
		}

		if (g_WorldSurfaceRenderer.iNumLegacyDLights)
		{
			WSurfProgramState |= WSURF_LEGACY_DLIGHT_ENABLED;
		}

		if (g_WorldSurfaceRenderer.iLightmapUsedBits & (1 << 0))
		{
			WSurfProgramState |= WSURF_LIGHTMAP_INDEX_0_ENABLED;
		}
		if (g_WorldSurfaceRenderer.iLightmapUsedBits & (1 << 1))
		{
			WSurfProgramState |= WSURF_LIGHTMAP_INDEX_1_ENABLED;
		}
		if (g_WorldSurfaceRenderer.iLightmapUsedBits & (1 << 2))
		{
			WSurfProgramState |= WSURF_LIGHTMAP_INDEX_2_ENABLED;
		}
		if (g_WorldSurfaceRenderer.iLightmapUsedBits & (1 << 3))
		{
			WSurfProgramState |= WSURF_LIGHTMAP_INDEX_3_ENABLED;
		}
	}

	if (R_IsRenderingWaterView())
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	//Transparency
	if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha && (*currententity)->curstate.rendermode != kRenderTransColor)
	{
		if ((*currententity)->curstate.rendermode == kRenderTransAdd || (*currententity)->curstate.rendermode == kRenderGlow)
			WSurfProgramState |= WSURF_ADDITIVE_BLEND_ENABLED;
		else
			WSurfProgramState |= WSURF_ALPHA_BLEND_ENABLED;

		if (r_draw_oitblend)
		{
			WSurfProgramState |= WSURF_OIT_BLEND_ENABLED;
		}
	}

	if (R_IsRenderingGammaBlending())
	{
		WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
	}

	if (!R_IsRenderingGBuffer())
	{
		if ((WSurfProgramState & WSURF_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
		{

		}
		else if ((WSurfProgramState & WSURF_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
		{

		}
		else if (R_IsRenderingFog())
		{
			if (r_fog_mode == GL_LINEAR)
			{
				WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
			}
			else if (r_fog_mode == GL_EXP)
			{
				WSurfProgramState |= WSURF_EXP_FOG_ENABLED;
			}
			else if (r_fog_mode == GL_EXP2)
			{
				WSurfProgramState |= WSURF_EXP2_FOG_ENABLED;
			}

			if (!R_IsRenderingGammaBlending() && r_linear_fog_shift->value > 0)
			{
				WSurfProgramState |= WSURF_LINEAR_FOG_SHIFT_ENABLED;
			}
		}
	}

	if (R_IsRenderingGBuffer())
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		WSurfProgramState &= ~(WSURF_LEGACY_DLIGHT_ENABLED);
	}

	GL_BindVAO(g_WorldSurfaceRenderer.hDecalVAO);

	if (g_DecalDrawBatch.BatchCount > 0)
	{
		for (int i = 0; i < g_DecalDrawBatch.BatchCount; ++i)
		{
			program_state_t WSurfProgramStateDetail = WSurfProgramState;

			GL_BindTextureUnit(WSURF_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, g_DecalDrawBatch.GLTextureId[i]);

			if (g_DecalDrawBatch.pRenderMaterial[i])
			{
				R_BeginDetailTextureFromRenderMaterial(g_DecalDrawBatch.pRenderMaterial[i], &WSurfProgramStateDetail);
			}

			wsurf_program_t prog = { 0 };

			R_UseWSurfProgram(WSurfProgramStateDetail, &prog);

			glDrawElementsInstancedBaseInstance(
				GL_TRIANGLES,
				g_DecalDrawBatch.IndiceCount[i],
				GL_UNSIGNED_INT,
				BUFFER_OFFSET(g_DecalDrawBatch.StartIndex[i]),
				g_DecalDrawBatch.InstanceCount[i],
				g_DecalDrawBatch.StartInstance[i]);

			R_EndDetailTexture(WSurfProgramStateDetail);
		}
	}

	GL_BindTextureUnit(WSURF_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	GL_EndStencil();

	(*gDecalSurfCount) = 0;

	GL_EndDebugGroup();
}