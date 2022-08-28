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

GLint g_DrawDecalTextureId[MAX_DECALS];
GLint g_DrawDecalStartIndex[MAX_DECALS];
GLsizei g_DrawDecalVertexCount[MAX_DECALS];
int g_DrawDecalCount = 0;

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
	int i, outCount;
	texture_t *ptexture;
	msurface_t *psurf;
	float *v;

	if (!(*gDecalSurfCount))
		return;

	if (CL_IsDevOverviewMode())
		return;

	//if (g_iEngineType == ENGINE_SVENGINE)
	{
		if ((*currententity)->curstate.effects & EF_NODECALS)
			return;
	}

	g_DrawDecalCount = 0;

	for (i = 0; i < (*gDecalSurfCount); i++)
	{
		psurf = gDecalSurfs[i];
		plist = psurf->pdecals;

		while (plist)
		{
			int decalIndex = R_DecalIndex(plist);

			if (!(plist->flags & FDECAL_VBO))
			{
				ptexture = Draw_DecalTexture(plist->texture);

				if (plist->flags & FDECAL_NOCLIP)
				{
					v = R_DecalVertsNoclip(plist, psurf, ptexture, r_wsurf.bLightmapTexture);
					outCount = 4;
				}
				else
				{
					v = R_DecalVertsClip(NULL, plist, psurf, ptexture, &outCount);
					if (outCount && r_wsurf.bLightmapTexture)
					{
						R_DecalVertsLight(v, psurf, outCount);
					}
				}

				if (outCount)
				{
					if (bUseBindless)
					{
						GLuint64 handle = 0;
						//Texture handle changed ?
						if (r_wsurf.vDecalGLTextures[decalIndex] != ptexture->gl_texturenum)
						{
							r_wsurf.vDecalGLTextures[decalIndex] = ptexture->gl_texturenum;

							handle = glGetTextureHandleARB(ptexture->gl_texturenum);
							glMakeTextureHandleResidentARB(handle);

							r_wsurf.vDecalGLTextureHandles[decalIndex] = handle;

							glNamedBufferSubData(r_wsurf.hDecalSSBO, sizeof(GLuint64) * decalIndex, sizeof(GLuint64), &handle);
						}
						else
						{
							handle = r_wsurf.vDecalGLTextureHandles[decalIndex];
						}
					}

					decalvertex_t vertex[MAX_DECALVERTS];

					for (int j = 0; j < outCount && j < MAX_DECALVERTS; ++j)
					{
						memcpy(&vertex[j].pos, &v[0], sizeof(float) * 3);

						memcpy(&vertex[j].texcoord, &v[3], sizeof(float) * 2);
						vertex[j].texcoord[2] = 0;

						memcpy(&vertex[j].lightmaptexcoord, &v[5], sizeof(float) * 2);
						vertex[j].lightmaptexcoord[2] = psurf->lightmaptexturenum;

						vertex[j].decalindex = decalIndex;

						v += VERTEXSIZE;
					}

					glNamedBufferSubData(r_wsurf.hDecalVBO, sizeof(decalvertex_t) * MAX_DECALVERTS * decalIndex, sizeof(decalvertex_t) * outCount, vertex);

					r_wsurf.vDecalGLTextures[decalIndex] = ptexture->gl_texturenum;
					r_wsurf.vDecalStartIndex[decalIndex] = MAX_DECALVERTS * decalIndex;
					r_wsurf.vDecalVertexCount[decalIndex] = outCount;
				}
				else
				{
					r_wsurf.vDecalGLTextures[decalIndex] = ptexture->gl_texturenum;
					r_wsurf.vDecalStartIndex[decalIndex] = 0;
					r_wsurf.vDecalVertexCount[decalIndex] = 0;
				}

				plist->flags |= FDECAL_VBO;
			}

			if (r_wsurf.vDecalVertexCount[decalIndex] && g_DrawDecalCount < MAX_DECALS)
			{
				g_DrawDecalTextureId[g_DrawDecalCount] = r_wsurf.vDecalGLTextures[decalIndex];
				g_DrawDecalStartIndex[g_DrawDecalCount] = r_wsurf.vDecalStartIndex[decalIndex];
				g_DrawDecalVertexCount[g_DrawDecalCount] = r_wsurf.vDecalVertexCount[decalIndex];
				++g_DrawDecalCount;
			}

			plist = plist->pnext;
		}
	}

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);

	//Decal only affects diffuse channel
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);

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

	if (bUseBindless)
	{
		WSurfProgramState |= WSURF_BINDLESS_ENABLED;
	}

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

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	glBindBuffer(GL_ARRAY_BUFFER, r_wsurf.hDecalVBO);

	if (bUseBindless)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_DECAL_SSBO, r_wsurf.hDecalSSBO);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(10);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, pos));
	glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, texcoord));
	glVertexAttribPointer(5, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, lightmaptexcoord));
	glVertexAttribIPointer(10, 1, GL_INT, sizeof(decalvertex_t), OFFSET(decalvertex_t, decalindex));

	if (WSurfProgramState & WSURF_BINDLESS_ENABLED)
	{
		glMultiDrawArrays(GL_POLYGON, g_DrawDecalStartIndex, g_DrawDecalVertexCount, g_DrawDecalCount);
		r_wsurf_polys += g_DrawDecalCount;
		r_wsurf_drawcall++;
	}
	else
	{
		for (int i = 0; i < g_DrawDecalCount; ++i)
		{
			GL_Bind(g_DrawDecalTextureId[i]);
			glDrawArrays(GL_POLYGON, g_DrawDecalStartIndex[i], g_DrawDecalVertexCount[i]);			
		}
		r_wsurf_polys += g_DrawDecalCount;
		r_wsurf_drawcall += g_DrawDecalCount;
	}
	
	GL_UseProgram(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(5);
	glDisableVertexAttribArray(10);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDepthMask(1);

	(*gDecalSurfCount) = 0;
	g_DrawDecalCount = 0;
}