#include "gl_local.h"
#include <algorithm>

msurface_t **skychain = NULL;
msurface_t **waterchain = NULL;

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

decal_drawbatch_t g_DecalBaseDrawBatch = { 0 };
decal_drawbatch_t g_DecalDetailDrawBatch = { 0 };

void R_RecursiveWorldNode(mnode_t *node)
{
	gRefFuncs.R_RecursiveWorldNode(node);
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

	return RecursiveLightPoint(r_worldmodel->nodes, p, end);
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

#define MAX_DECALCLIPVERT		32
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

void R_DrawDecals(wsurf_vbo_t *modcache)
{
	decal_t *plist;
	int i, vertCount;
	texture_t *ptexture;
	msurface_t *psurf;
	float *v;

	if (!(*gDecalSurfCount))
		return;

	if (CL_IsDevOverviewMode())
		return;

	if (r_draw_shadowcaster)
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		if ((*currententity)->curstate.effects & EF_NODECALS)
			return;
	}

	g_DecalBaseDrawBatch.BatchCount = 0;
	g_DecalDetailDrawBatch.BatchCount = 0;

	for (i = 0; i < (*gDecalSurfCount); i++)
	{
		psurf = gDecalSurfs[i];
		plist = psurf->pdecals;

		while (plist)
		{
			int decalIndex = R_DecalIndex(plist);

			//Build VBO data for this decal if not built yet
			if (!(plist->flags & FDECAL_VBO))
			{
				ptexture = Draw_DecalTexture(plist->texture);

				auto pcache = R_FindDecalTextureCache(ptexture->name);

				if (plist->flags & FDECAL_NOCLIP)
				{
					v = R_DecalVertsNoclip(plist, psurf, ptexture, r_wsurf.bLightmapTexture);
					vertCount = 4;
				}
				else
				{
					v = R_DecalVertsClip(NULL, plist, psurf, ptexture, &vertCount);
					if (vertCount > 0 && r_wsurf.bLightmapTexture)
					{
						R_DecalVertsLight(v, psurf, vertCount);
					}
				}

				if (vertCount > 0)
				{
					if (bUseBindless)
					{
						//Texture handle changed ?
						if (r_wsurf.vDecalGLTextures[decalIndex] != ptexture->gl_texturenum)
						{
							r_wsurf.vDecalGLTextures[decalIndex] = ptexture->gl_texturenum;

							GLuint64 vDecalGLTextureHandles[WSURF_MAX_TEXTURE] = { 0 };

							auto handle = glGetTextureHandleARB(ptexture->gl_texturenum);
							glMakeTextureHandleResidentARB(handle);

							vDecalGLTextureHandles[WSURF_DIFFUSE_TEXTURE] = handle;

							if (pcache)
							{
								r_wsurf.vDecalDetailTextures[decalIndex] = pcache;

								for (int k = WSURF_REPLACE_TEXTURE; k < WSURF_MAX_TEXTURE; ++k)
								{
									if (pcache->tex[k].gltexturenum)
									{
										handle = glGetTextureHandleARB(pcache->tex[k].gltexturenum);
										glMakeTextureHandleResidentARB(handle);

										vDecalGLTextureHandles[k] = handle;
									}
									else
									{
										vDecalGLTextureHandles[k] = 0;
									}
								}
							}
							else
							{
								r_wsurf.vDecalDetailTextures[decalIndex] = NULL;

								for (int k = WSURF_REPLACE_TEXTURE; k < WSURF_MAX_TEXTURE; ++k)
								{
									vDecalGLTextureHandles[k] = 0;
								}
							}

							glNamedBufferSubData(r_wsurf.hDecalSSBO, sizeof(vDecalGLTextureHandles) * decalIndex, sizeof(vDecalGLTextureHandles), vDecalGLTextureHandles);
						}
					}

					decalvertex_t vertexArray[MAX_DECALVERTS] = {0};

					for (int j = 0; j < vertCount && j < MAX_DECALVERTS; ++j)
					{
						vertexArray[j].pos[0] = v[0];
						vertexArray[j].pos[1] = v[1];
						vertexArray[j].pos[2] = v[2];

						vertexArray[j].texcoord[0] = v[3];
						vertexArray[j].texcoord[1] = v[4];
						vertexArray[j].texcoord[2] = 0;

						vertexArray[j].lightmaptexcoord[0] = v[5];
						vertexArray[j].lightmaptexcoord[1] = v[6];
						vertexArray[j].lightmaptexcoord[2] = psurf->lightmaptexturenum;

						float replaceScale[2] = { 1,1 };
						float detailScale[2] = { 1,1 };
						float normalScale[2] = { 1,1 };
						float parallaxScale[2] = { 1,1 };
						float specularScale[2] = { 1,1 };

						if (pcache)
						{
							if (pcache->tex[WSURF_REPLACE_TEXTURE].gltexturenum)
							{
								replaceScale[0] = pcache->tex[WSURF_REPLACE_TEXTURE].scaleX;
								replaceScale[1] = pcache->tex[WSURF_REPLACE_TEXTURE].scaleY;
							}
							if (pcache->tex[WSURF_DETAIL_TEXTURE].gltexturenum)
							{
								detailScale[0] = pcache->tex[WSURF_DETAIL_TEXTURE].scaleX;
								detailScale[1] = pcache->tex[WSURF_DETAIL_TEXTURE].scaleY;
							}
							if (pcache->tex[WSURF_NORMAL_TEXTURE].gltexturenum)
							{
								normalScale[0] = pcache->tex[WSURF_NORMAL_TEXTURE].scaleX;
								normalScale[1] = pcache->tex[WSURF_NORMAL_TEXTURE].scaleY;
							}
							if (pcache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum)
							{
								parallaxScale[0] = pcache->tex[WSURF_PARALLAX_TEXTURE].scaleX;
								parallaxScale[1] = pcache->tex[WSURF_PARALLAX_TEXTURE].scaleY;
							}
							if (pcache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum)
							{
								specularScale[0] = pcache->tex[WSURF_SPECULAR_TEXTURE].scaleX;
								specularScale[1] = pcache->tex[WSURF_SPECULAR_TEXTURE].scaleY;
							}
						}

						vertexArray[j].replacetexcoord[0] = replaceScale[0];
						vertexArray[j].replacetexcoord[1] = replaceScale[1];
						vertexArray[j].detailtexcoord[0] = detailScale[0];
						vertexArray[j].detailtexcoord[1] = detailScale[1];
						vertexArray[j].normaltexcoord[0] = normalScale[0];
						vertexArray[j].normaltexcoord[1] = normalScale[1];
						vertexArray[j].parallaxtexcoord[0] = parallaxScale[0];
						vertexArray[j].parallaxtexcoord[1] = parallaxScale[1];
						vertexArray[j].speculartexcoord[0] = specularScale[0];
						vertexArray[j].speculartexcoord[1] = specularScale[1];

						auto poly = psurf->polys;

						auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

						vertexArray[j].normal[0] = brushface->normal[0];
						vertexArray[j].normal[1] = brushface->normal[1];
						vertexArray[j].normal[2] = brushface->normal[2];
						vertexArray[j].s_tangent[0] = brushface->s_tangent[0];
						vertexArray[j].s_tangent[1] = brushface->s_tangent[1];
						vertexArray[j].s_tangent[2] = brushface->s_tangent[2];
						vertexArray[j].t_tangent[0] = brushface->t_tangent[0];
						vertexArray[j].t_tangent[1] = brushface->t_tangent[1];
						vertexArray[j].t_tangent[2] = brushface->t_tangent[2];

						vertexArray[j].decalindex = decalIndex;

						v += VERTEXSIZE;
					}

					glNamedBufferSubData(r_wsurf.hDecalVBO, sizeof(decalvertex_t) * MAX_DECALVERTS * decalIndex, sizeof(decalvertex_t) * vertCount, vertexArray);

					r_wsurf.vDecalStartIndex[decalIndex] = MAX_DECALVERTS * decalIndex;
					r_wsurf.vDecalVertexCount[decalIndex] = vertCount;
				}
				else
				{
					r_wsurf.vDecalGLTextures[decalIndex] = 0;
					r_wsurf.vDecalDetailTextures[decalIndex] = NULL;
					r_wsurf.vDecalStartIndex[decalIndex] = 0;
					r_wsurf.vDecalVertexCount[decalIndex] = 0;
				}

				//Mark this decal as ready
				plist->flags |= FDECAL_VBO;
			}

			if (r_wsurf.vDecalVertexCount[decalIndex] > 0)
			{
				if (!r_wsurf.vDecalDetailTextures[decalIndex] && g_DecalBaseDrawBatch.BatchCount < MAX_DECALS)
				{
					g_DecalBaseDrawBatch.GLTextureId[g_DecalBaseDrawBatch.BatchCount] = r_wsurf.vDecalGLTextures[decalIndex];
					g_DecalBaseDrawBatch.DetailTextureCaches[g_DecalBaseDrawBatch.BatchCount] = r_wsurf.vDecalDetailTextures[decalIndex];
					g_DecalBaseDrawBatch.StartIndex[g_DecalBaseDrawBatch.BatchCount] = r_wsurf.vDecalStartIndex[decalIndex];
					g_DecalBaseDrawBatch.VertexCount[g_DecalBaseDrawBatch.BatchCount] = r_wsurf.vDecalVertexCount[decalIndex];
					++g_DecalBaseDrawBatch.BatchCount;
				}
				else if (r_wsurf.vDecalDetailTextures[decalIndex] && g_DecalDetailDrawBatch.BatchCount < MAX_DECALS)
				{
					g_DecalDetailDrawBatch.GLTextureId[g_DecalDetailDrawBatch.BatchCount] = r_wsurf.vDecalGLTextures[decalIndex];
					g_DecalDetailDrawBatch.DetailTextureCaches[g_DecalDetailDrawBatch.BatchCount] = r_wsurf.vDecalDetailTextures[decalIndex];
					g_DecalDetailDrawBatch.StartIndex[g_DecalDetailDrawBatch.BatchCount] = r_wsurf.vDecalStartIndex[decalIndex];
					g_DecalDetailDrawBatch.VertexCount[g_DecalDetailDrawBatch.BatchCount] = r_wsurf.vDecalVertexCount[decalIndex];
					++g_DecalDetailDrawBatch.BatchCount;
				}
			}

			plist = plist->pnext;
		}
	}

	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_NOTEQUAL, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);

	//Decal only affects diffuse and normal channel
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE | GBUFFER_MASK_WORLDNORM);

	//Use alphablend to blend with gbuffer
	R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);

		if (gl_ztrick && gl_ztrick->value)
			glPolygonOffset(1, gl_polyoffset->value);
		else
			glPolygonOffset(-1, -gl_polyoffset->value);
	}
	
	int WSurfProgramState = WSURF_DECAL_ENABLED | WSURF_DIFFUSE_ENABLED;

	//Mix lightmap if not deferred
	if (r_wsurf.bLightmapTexture && !drawgbuffer)
	{
		WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
	}

	//Mix shadow if not deferred
	if (r_wsurf.bShadowmapTexture && !drawgbuffer)
	{
		WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

		for (int i = 0; i < 3; ++i)
		{
			if (shadow_numvisedicts[i] > 0)
			{
				WSurfProgramState |= (WSURF_SHADOWMAP_HIGH_ENABLED << i);
			}
		}
	}

	if (r_draw_reflectview)
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	if (!drawgbuffer && r_fog_mode == GL_LINEAR)
	{
		WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
	}
	else if (!drawgbuffer && r_fog_mode == GL_EXP)
	{
		WSurfProgramState |= WSURF_EXP_FOG_ENABLED;
	}
	else if (!drawgbuffer && r_fog_mode == GL_EXP2)
	{
		WSurfProgramState |= WSURF_EXP2_FOG_ENABLED;
	}

	if (drawgbuffer)
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}
	
	//Do we really need this?
	if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
	{
		WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
	}

	if (r_draw_oitblend)
	{
		/*if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			WSurfProgramState |= WSURF_OIT_ADDITIVE_BLEND_ENABLED;
		else*/

		WSurfProgramState |= WSURF_OIT_ALPHA_BLEND_ENABLED;
	}

	glBindBuffer(GL_ARRAY_BUFFER, r_wsurf.hDecalVBO);

	if (bUseBindless)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_DECAL_SSBO, r_wsurf.hDecalSSBO);

	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_POSITION);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMAL);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_EXTRA);

	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_POSITION, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, pos));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_NORMAL, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, normal));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_S_TANGENT, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, s_tangent));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_T_TANGENT, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, t_tangent));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_TEXCOORD, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, texcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, lightmaptexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, replacetexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, detailtexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, normaltexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, parallaxtexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, speculartexcoord));
	glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_EXTRA, 1, GL_INT, sizeof(decalvertex_t), OFFSET(decalvertex_t, decalindex));

	if (g_DecalBaseDrawBatch.BatchCount > 0)
	{
		int WSurfProgramStateBase = WSurfProgramState;

		if (bUseBindless)
		{
			WSurfProgramStateBase |= WSURF_BINDLESS_ENABLED;
		}

		wsurf_program_t prog = { 0 };
		R_UseWSurfProgram(WSurfProgramStateBase, &prog);

		if (WSurfProgramStateBase & WSURF_BINDLESS_ENABLED)
		{
			glMultiDrawArrays(GL_POLYGON, g_DecalBaseDrawBatch.StartIndex, g_DecalBaseDrawBatch.VertexCount, g_DecalBaseDrawBatch.BatchCount);
			r_wsurf_polys += g_DecalBaseDrawBatch.BatchCount;
			r_wsurf_drawcall++;
		}
		else
		{
			for (int i = 0; i < g_DecalBaseDrawBatch.BatchCount; ++i)
			{
				GL_Bind(g_DecalBaseDrawBatch.GLTextureId[i]);
				glDrawArrays(GL_POLYGON, g_DecalBaseDrawBatch.StartIndex[i], g_DecalBaseDrawBatch.VertexCount[i]);
			}
			r_wsurf_polys += g_DecalBaseDrawBatch.BatchCount;
			r_wsurf_drawcall += g_DecalBaseDrawBatch.BatchCount;
		}
	}

	if (g_DecalDetailDrawBatch.BatchCount > 0)
	{
		for (int i = 0; i < g_DecalDetailDrawBatch.BatchCount; ++i)
		{
			int WSurfProgramStateDetail = WSurfProgramState;

			GL_Bind(g_DecalDetailDrawBatch.GLTextureId[i]);

			R_BeginDetailTextureByDetailTextureCache(g_DecalDetailDrawBatch.DetailTextureCaches[i], &WSurfProgramStateDetail);

			wsurf_program_t prog = { 0 };
			R_UseWSurfProgram(WSurfProgramStateDetail, &prog);

			glDrawArrays(GL_POLYGON, g_DecalDetailDrawBatch.StartIndex[i], g_DecalDetailDrawBatch.VertexCount[i]);

			R_EndDetailTexture(WSurfProgramStateDetail);
		}

		r_wsurf_polys += g_DecalDetailDrawBatch.BatchCount;
		r_wsurf_drawcall += g_DecalDetailDrawBatch.BatchCount;
	}

	GL_UseProgram(0);

	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_POSITION);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMAL);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD);
	glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_EXTRA);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	//glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDepthMask(1);

	(*gDecalSurfCount) = 0;
}