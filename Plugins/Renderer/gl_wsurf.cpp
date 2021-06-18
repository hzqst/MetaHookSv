#include "gl_local.h"
#include <sstream>
#include <algorithm>

r_worldsurf_t r_wsurf;

cvar_t *r_wsurf_vbo;
cvar_t *r_wsurf_parallax_scale;
cvar_t *r_wsurf_detail;

int r_fog_mode = 0;
float r_fog_control[2];
float r_fog_color[4];
int r_wsurf_drawcall = 0;
int r_wsurf_polys = 0;

wsurf_program_t *g_WSurfProgramTable[WSURF_MAX_STATE];

std::unordered_map<int, detail_texture_cache_t *> g_DetailTextureTable;

std::unordered_map<model_t *, wsurf_model_t *> g_WSurfModelCache;

extern std::vector<deferred_light_t> g_DeferredLights;

void R_ClearWSurfModelCache(void)
{
	for (auto &itor = g_WSurfModelCache.begin(); itor != g_WSurfModelCache.end(); ++itor)
	{
		if (itor->second->hEBO)
		{
			qglDeleteBuffersARB(1, &itor->second->hEBO);
		}

		delete itor->second;
	}
	g_WSurfModelCache.clear();
}

wsurf_program_t *R_UseWSurfProgram(int state)
{
	wsurf_program_t *prog = NULL;

	if(!g_WSurfProgramTable[state])
	{
		std::stringstream defs;

		if (state & WSURF_DIFFUSE_ENABLED)
			defs << "#define DIFFUSE_ENABLED\n";

		if (state & WSURF_LIGHTMAP_ENABLED)
			defs << "#define LIGHTMAP_ENABLED\n";

		if (state & WSURF_DETAILTEXTURE_ENABLED)
			defs << "#define DETAILTEXTURE_ENABLED\n";

		if (state & WSURF_NORMALTEXTURE_ENABLED)
			defs << "#define NORMALTEXTURE_ENABLED\n";

		if (state & WSURF_PARALLAXTEXTURE_ENABLED)
			defs << "#define PARALLAXTEXTURE_ENABLED\n";

		if (state & WSURF_CLIP_ABOVE_ENABLED)
			defs << "#define CLIP_ABOVE_ENABLED\n";

		if (state & WSURF_CLIP_UNDER_ENABLED)
			defs << "#define CLIP_UNDER_ENABLED\n";

		if (state & WSURF_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & WSURF_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if (state & WSURF_TRANSPARENT_ENABLED)
			defs << "#define TRANSPARENT_ENABLED\n";

		auto def = defs.str();

		prog = new wsurf_program_t;

		prog->program = R_CompileShaderFileEx("renderer\\shader\\wsurf_shader.vsh", NULL, "renderer\\shader\\wsurf_shader.fsh", def.c_str(), NULL, def.c_str());
		SHADER_UNIFORM((*prog), diffuseTex, "diffuseTex");
		SHADER_UNIFORM((*prog), lightmapTexArray, "lightmapTexArray");
		SHADER_UNIFORM((*prog), detailTex, "detailTex");
		SHADER_UNIFORM((*prog), normalTex, "normalTex");
		SHADER_UNIFORM((*prog), parallaxTex, "parallaxTex");
		SHADER_UNIFORM((*prog), speed, "speed");
		SHADER_UNIFORM((*prog), entitymatrix, "entitymatrix");
		SHADER_UNIFORM((*prog), clipPlane, "clipPlane");
		SHADER_UNIFORM((*prog), viewpos, "viewpos");
		SHADER_UNIFORM((*prog), parallaxScale, "parallaxScale");
		SHADER_ATTRIB((*prog), s_tangent, "s_tangent");
		SHADER_ATTRIB((*prog), t_tangent, "t_tangent");

		g_WSurfProgramTable[state] = prog;
	}
	else
	{
		prog = g_WSurfProgramTable[state];
	}

	if (!prog->program)
	{
		Sys_ErrorEx("R_UseWSurfProgram: Failed to load program!");
		return NULL;
	}

	qglUseProgramObjectARB(prog->program);

	if (prog->diffuseTex != -1)
		qglUniform1iARB(prog->diffuseTex, 0);

	if (prog->lightmapTexArray != -1)
		qglUniform1iARB(prog->lightmapTexArray, 1);

	if (prog->detailTex != -1)
		qglUniform1iARB(prog->detailTex, 2);

	if (prog->normalTex != -1)
		qglUniform1iARB(prog->normalTex, 3);

	if (prog->parallaxTex != -1)
		qglUniform1iARB(prog->parallaxTex, 4);

	if (prog->entitymatrix != -1)
	{
		if (r_rotate_entity)
			qglUniformMatrix4fvARB(prog->entitymatrix, 1, true, (float *)r_rotate_entity_matrix);
		else
			qglUniformMatrix4fvARB(prog->entitymatrix, 1, false, (float *)r_identity_matrix);
	}

	if (prog->clipPlane != -1)
		qglUniform1fARB(prog->clipPlane, curwater->vecs[2]);

	if (prog->viewpos != -1)
		qglUniform4fARB(prog->viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2], 0);

	if (prog->parallaxScale != -1)
		qglUniform1fARB(prog->parallaxScale, r_wsurf_parallax_scale->value);

	if (prog->s_tangent != -1 && r_wsurf.pCurrentModel)
	{
		qglVertexAttribPointer(prog->s_tangent, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, s_tangent));
		qglEnableVertexAttribArray(prog->s_tangent);
	}

	if (prog->t_tangent != -1 && r_wsurf.pCurrentModel)
	{
		qglVertexAttribPointer(prog->t_tangent, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, t_tangent));
		qglEnableVertexAttribArray(prog->t_tangent);
	}

	return prog;
}

void R_FreeLightmapArray(void)
{
	r_wsurf.iNumLightmapTextures = 0;
	if (r_wsurf.iLightmapTextureArray)
	{
		GL_DeleteTexture(r_wsurf.iLightmapTextureArray);
		r_wsurf.iLightmapTextureArray = 0;
	}
}

void R_FreeVertexBuffer(void)
{
	if (r_wsurf.hVBO)
	{
		qglDeleteBuffersARB(1, &r_wsurf.hVBO);
		r_wsurf.hVBO = 0;
	}

	if (r_wsurf.vVertexBuffer)
	{
		delete[] r_wsurf.vVertexBuffer;
		r_wsurf.vVertexBuffer = NULL;
	}
	r_wsurf.iNumVerts = 0;

	if (r_wsurf.vFaceBuffer)
	{
		delete[] r_wsurf.vFaceBuffer;
		r_wsurf.vFaceBuffer = NULL;
	}
	r_wsurf.iNumFaces = 0;
}

void R_RecursiveWorldNodeGenerateTextureChain(mnode_t *node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents < 0)
		return;

	R_RecursiveWorldNodeGenerateTextureChain(node->children[0]);

	auto c = node->numsurfaces;

	if (c)
	{
		auto surf = r_worldmodel->surfaces + node->firstsurface;

		for (; c; c--, surf++)
		{
			surf->texturechain = surf->texinfo->texture->texturechain;
			surf->texinfo->texture->texturechain = surf;
		}
	}

	R_RecursiveWorldNodeGenerateTextureChain(node->children[1]);
}

void R_GenerateElementBufferIndices(msurface_t *s, brushtexchain_t *texchain, wsurf_model_t *modcache)
{
	auto p = s->polys;
	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	if (s->flags & SURF_DRAWSKY)
	{

	}
	else if (s->flags & SURF_DRAWTURB)
	{

	}
	else if (s->flags & SURF_UNDERWATER)
	{

	}
	else if (s->flags & SURF_DRAWTILED)
	{
		if (texchain->iType == TEXCHAIN_SCROLL)
		{
			for (int i = 0; i < brushface->num_vertexes; ++i)
			{
				modcache->vIndicesBuffer.emplace_back(brushface->start_vertex + i);
				texchain->iVertexCount++;
			}
			modcache->vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
			texchain->iVertexCount++;
			texchain->iFaceCount++;
		}
	}
	else
	{

		if (texchain->iType == TEXCHAIN_STATIC)
		{
			for (int i = 0; i < brushface->num_vertexes; ++i)
			{
				modcache->vIndicesBuffer.emplace_back(brushface->start_vertex + i);
				texchain->iVertexCount++;
			}
			modcache->vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
			texchain->iVertexCount++;
			texchain->iFaceCount++;
		}
	}
}

void R_GenerateElementBuffer(model_t *mod, wsurf_model_t *modcache)
{
	if (mod == r_worldmodel)
	{
		R_RecursiveWorldNodeGenerateTextureChain(mod->nodes);
	}
	else
	{
		auto psurf = &mod->surfaces[mod->firstmodelsurface];
		for (int i = 0; i < mod->nummodelsurfaces; i++, psurf++)
		{
			auto pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				continue;
			}
			else if (psurf->flags & SURF_DRAWSKY)
			{
				continue;
			}

			psurf->texturechain = psurf->texinfo->texture->texturechain;
			psurf->texinfo->texture->texturechain = psurf;
		}
	}

	for (int i = 0; i < mod->numtextures; i++)
	{
		auto t = mod->textures[i];

		if (!t)
			continue;

		if (t->anim_total)
		{
			if (t->name[0] == '-')
			{
				//Construct texchain for random textures

				auto s = t->texturechain;

				if (s)
				{
					if (s->flags & SURF_DRAWSKY)
					{
						t->texturechain = NULL;
						continue;
					}
					else
					{
						if (s->flags & SURF_DRAWTURB)
						{
							t->texturechain = NULL;
							continue;
						}

						brushtexchain_t *texchainArray = new brushtexchain_t[t->anim_total];

						int numtexturechain = 0;
						for (msurface_t *s2 = s; s2; s2 = s2->texturechain)
						{
							numtexturechain++;
						}

						//rtable not initialized?
						if ((*rtable)[0][0] == 0)
						{
							gRefFuncs.R_TextureAnimation(s);
						}

						int *texchainMapper = new int[numtexturechain];
						msurface_t **texchainSurface = new msurface_t*[numtexturechain];

						{
							msurface_t *s2 = s;
							int k = 0;

							for (; s2; s2 = s2->texturechain, ++k)
							{
								texchainSurface[k] = s2;

								int mappingIndex = (*rtable)[(int)((s2->texturemins[0] + (t->width << 16)) / t->width) % 20][(int)((s2->texturemins[1] + (t->height << 16)) / t->height) % 20] % t->anim_total;

								texchainMapper[k] = mappingIndex;
							}
						}

						{
							texture_t *t2 = t;
							int k = 0;
							for (; k < t->anim_total && t2; t2 = t2->anim_next, ++k)
							{
								brushtexchain_t texchain;
								texchain.pTexture = t2;
								texchain.iVertexCount = 0;
								texchain.iFaceCount = 0;
								texchain.iStartIndex = modcache->vIndicesBuffer.size();
								texchain.iType = TEXCHAIN_STATIC;

								for (int n = 0; n < numtexturechain; ++n)
								{
									if (texchainMapper[n] == k)
										R_GenerateElementBufferIndices(texchainSurface[n], &texchain, modcache);
								}

								if (texchain.iVertexCount > 0)
									modcache->vTextureChainStatic.emplace_back(texchain);
							}
						}

						delete []texchainSurface;
						delete []texchainMapper;
						delete []texchainArray;
					}
				}
			}
			else if (t->name[0] == '+')
			{
				//Construct texchain for anim textures

				auto s = t->texturechain;

				if (s)
				{
					if (s->flags & SURF_DRAWSKY)
					{
						t->texturechain = NULL;
						continue;
					}
					else
					{
						if (s->flags & SURF_DRAWTURB)
						{
							t->texturechain = NULL;
							continue;
						}

						brushtexchain_t texchain;

						texchain.pTexture = t;
						texchain.iVertexCount = 0;
						texchain.iFaceCount = 0;
						texchain.iStartIndex = modcache->vIndicesBuffer.size();
						texchain.iType = TEXCHAIN_STATIC;

						for (; s; s = s->texturechain)
						{
							R_GenerateElementBufferIndices(s, &texchain, modcache);
						}

						if (texchain.iVertexCount > 0)
							modcache->vTextureChainAnim.emplace_back(texchain);
					}
				}
			}
		}
		else
		{
			//Construct texchain for static textures

			auto s = t->texturechain;

			if (s)
			{
				if (s->flags & SURF_DRAWSKY)
				{
					t->texturechain = NULL;
					continue;
				}
				else
				{
					if (s->flags & SURF_DRAWTURB)
					{
						t->texturechain = NULL;
						continue;
					}

					brushtexchain_t texchain;

					texchain.pTexture = t;
					texchain.iVertexCount = 0;
					texchain.iFaceCount = 0;
					texchain.iStartIndex = modcache->vIndicesBuffer.size();
					texchain.iType = TEXCHAIN_STATIC;

					for (; s; s = s->texturechain)
					{
						R_GenerateElementBufferIndices(s, &texchain, modcache);
					}

					if (texchain.iVertexCount > 0)
						modcache->vTextureChainStatic.emplace_back(texchain);
				}
			}

			//Construct texchain for scroll textures

			s = t->texturechain;
			if (s)
			{
				if (s->flags & SURF_DRAWSKY)
				{
					t->texturechain = NULL;
					continue;
				}

				if (s->flags & SURF_DRAWTURB)
				{
					t->texturechain = NULL;
					continue;
				}

				brushtexchain_t texchain;

				texchain.pTexture = t;
				texchain.iVertexCount = 0;
				texchain.iFaceCount = 0;
				texchain.iStartIndex = modcache->vIndicesBuffer.size();
				texchain.iType = TEXCHAIN_SCROLL;

				for (; s; s = s->texturechain)
				{
					R_GenerateElementBufferIndices(s, &texchain, modcache);
				}

				if (texchain.iVertexCount > 0)
					modcache->vTextureChainScroll.emplace_back(texchain);
			}
		}

		//End construction

		t->texturechain = NULL;
	}

	modcache->vTextureChainStatic.shrink_to_fit();
	modcache->vTextureChainScroll.shrink_to_fit();
	modcache->vTextureChainAnim.shrink_to_fit();
	modcache->vIndicesBuffer.shrink_to_fit();

	qglGenBuffersARB(1, &modcache->hEBO);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, modcache->hEBO);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(unsigned int) * modcache->vIndicesBuffer.size(), modcache->vIndicesBuffer.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}

void R_GenerateVertexBuffer(void)
{
	brushvertex_t pVertexes[3];
	glpoly_t *poly;
	msurface_t *surf;
	float *v;
	int i, j;	

	int iNumFaces = 0;
	int iCurFace = 0;
	int iNumVerts = 0;
	int iCurVert = 0;

	surf = r_worldmodel->surfaces;

	for(i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		if ((surf[i].flags & (SURF_DRAWSKY | SURF_UNDERWATER)))
			continue;

		for (poly = surf[i].polys; poly; poly = poly->next)
			iNumVerts += 3 + (poly->numverts-3)*3;

		iNumFaces++;
	}

	r_wsurf.vVertexBuffer = new brushvertex_t[iNumVerts];
	r_wsurf.iNumVerts = iNumVerts;

	r_wsurf.vFaceBuffer = new brushface_t[iNumFaces];
	r_wsurf.iNumFaces = iNumFaces;

	for(i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		if ((surf[i].flags & (SURF_DRAWSKY | SURF_UNDERWATER)))
			continue;

		poly = surf[i].polys;

		poly->flags = iCurFace;

		brushface_t *face = &r_wsurf.vFaceBuffer[iCurFace];
		VectorCopy(surf[i].texinfo->vecs[0], face->s_tangent);
		VectorCopy(surf[i].texinfo->vecs[1], face->t_tangent);
		VectorNormalize(face->s_tangent);
		VectorNormalize(face->t_tangent);
		VectorCopy(surf[i].plane->normal, face->normal);
		face->index = i;

		if (surf[i].flags & SURF_PLANEBACK)
			VectorInverse(face->normal);

		if (surf[i].lightmaptexturenum + 1 > r_wsurf.iNumLightmapTextures)
			r_wsurf.iNumLightmapTextures = surf[i].lightmaptexturenum + 1;

		auto t = surf[i].texinfo ? surf[i].texinfo->texture : NULL;

		float detailScale[2] = { 1, 1 };
		float normalScale[2] = { 1, 1 };
		float parallaxScale[2] = { 1, 1 };

		if (t)
		{
			R_BeginDetailTexture(t->gl_texturenum);
			if (r_wsurf.bDetailTexture)
			{
				detailScale[0] = 1.0f / r_wsurf.pDetailTextureCache->tex[WSURF_DETAIL_TEXTURE].scaleX;
				detailScale[1] = 1.0f / r_wsurf.pDetailTextureCache->tex[WSURF_DETAIL_TEXTURE].scaleY;
			}
			if (r_wsurf.bNormalTexture)
			{
				normalScale[0] = 1.0f / r_wsurf.pDetailTextureCache->tex[WSURF_NORMAL_TEXTURE].scaleX;
				normalScale[1] = 1.0f / r_wsurf.pDetailTextureCache->tex[WSURF_NORMAL_TEXTURE].scaleY;
			}
			if (r_wsurf.bParallaxTexture)
			{
				parallaxScale[0] = 1.0f / r_wsurf.pDetailTextureCache->tex[WSURF_PARALLAX_TEXTURE].scaleX;
				parallaxScale[1] = 1.0f / r_wsurf.pDetailTextureCache->tex[WSURF_PARALLAX_TEXTURE].scaleY;
			}
			R_EndDetailTexture();
		}

		face->start_vertex = iCurVert;
		for (poly = surf[i].polys; poly; poly = poly->next)
		{
			v = poly->verts[0];

			for(j = 0; j < 3; j++, v += VERTEXSIZE)
			{
				pVertexes[j].pos[0] = v[0];
				pVertexes[j].pos[1] = v[1];
				pVertexes[j].pos[2] = v[2];
				pVertexes[j].texcoord[0] = v[3];
				pVertexes[j].texcoord[1] = v[4];

				if (t)
					pVertexes[j].texcoord[2] = 1.0f / t->width;
				else
					pVertexes[j].texcoord[2] = 0;

				pVertexes[j].lightmaptexcoord[0] = v[5];
				pVertexes[j].lightmaptexcoord[1] = v[6];
				pVertexes[j].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
				pVertexes[j].detailtexcoord[0] = detailScale[0];
				pVertexes[j].detailtexcoord[1] = detailScale[1];
				pVertexes[j].normaltexcoord[0] = normalScale[0];
				pVertexes[j].normaltexcoord[1] = normalScale[1];
				pVertexes[j].parallaxtexcoord[0] = parallaxScale[0];
				pVertexes[j].parallaxtexcoord[1] = parallaxScale[1];
				pVertexes[j].normal[0] = face->normal[0];
				pVertexes[j].normal[1] = face->normal[1];
				pVertexes[j].normal[2] = face->normal[2];
				pVertexes[j].s_tangent[0] = face->s_tangent[0];
				pVertexes[j].s_tangent[1] = face->s_tangent[1];
				pVertexes[j].s_tangent[2] = face->s_tangent[2];
				pVertexes[j].t_tangent[0] = face->t_tangent[0];
				pVertexes[j].t_tangent[1] = face->t_tangent[1];
				pVertexes[j].t_tangent[2] = face->t_tangent[2];
			}
			memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[0], sizeof(brushvertex_t)); iCurVert++;
			memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[1], sizeof(brushvertex_t)); iCurVert++;
			memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[2], sizeof(brushvertex_t)); iCurVert++;

			for(j = 0; j < (poly->numverts-3); j++, v += VERTEXSIZE)
			{
				memcpy(&pVertexes[1], &pVertexes[2], sizeof(brushvertex_t));

				pVertexes[2].pos[0] = v[0];
				pVertexes[2].pos[1] = v[1];
				pVertexes[2].pos[2] = v[2];
				pVertexes[2].texcoord[0] = v[3];
				pVertexes[2].texcoord[1] = v[4];

				if (t)
					pVertexes[2].texcoord[2] = 1.0f / t->width;
				else
					pVertexes[2].texcoord[2] = 0;

				pVertexes[2].lightmaptexcoord[0] = v[5];
				pVertexes[2].lightmaptexcoord[1] = v[6];
				pVertexes[2].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
				pVertexes[2].detailtexcoord[0] = detailScale[0];
				pVertexes[2].detailtexcoord[1] = detailScale[1];
				pVertexes[2].normaltexcoord[0] = normalScale[0];
				pVertexes[2].normaltexcoord[1] = normalScale[1];
				pVertexes[2].parallaxtexcoord[0] = parallaxScale[0];
				pVertexes[2].parallaxtexcoord[1] = parallaxScale[1];
				pVertexes[2].normal[0] = face->normal[0];
				pVertexes[2].normal[1] = face->normal[1];
				pVertexes[2].normal[2] = face->normal[2];
				pVertexes[2].s_tangent[0] = face->s_tangent[0];
				pVertexes[2].s_tangent[1] = face->s_tangent[1];
				pVertexes[2].s_tangent[2] = face->s_tangent[2];
				pVertexes[2].t_tangent[0] = face->t_tangent[0];
				pVertexes[2].t_tangent[1] = face->t_tangent[1];
				pVertexes[2].t_tangent[2] = face->t_tangent[2];
				memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[0], sizeof(brushvertex_t)); iCurVert++;
				memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[1], sizeof(brushvertex_t)); iCurVert++;
				memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[2], sizeof(brushvertex_t)); iCurVert++;
			}
		}

		face->num_vertexes = iCurVert - face->start_vertex;
		iCurFace++;
	}

	qglGenBuffersARB( 1, &r_wsurf.hVBO );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO );
	qglBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(brushvertex_t) * r_wsurf.iNumVerts, r_wsurf.vVertexBuffer, GL_STATIC_DRAW_ARB );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
}

void R_GenerateLightmapArray(void)
{
	r_wsurf.iLightmapTextureArray = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, BLOCK_WIDTH, BLOCK_HEIGHT, r_wsurf.iNumLightmapTextures, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	for (int i = 0; i < r_wsurf.iNumLightmapTextures; ++i)
	{
		qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, BLOCK_WIDTH, BLOCK_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + 0x10000 * i);
	}
	qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

wsurf_model_t *R_PrepareWSurfVBO(model_t *mod)
{
	wsurf_model_t *modcache = NULL;

	auto itor = g_WSurfModelCache.find(mod);

	if (itor == g_WSurfModelCache.end())
	{
		modcache = new wsurf_model_t;

		R_GenerateElementBuffer(mod, modcache);

		g_WSurfModelCache[mod] = modcache;
	}
	else
	{
		modcache = itor->second;
	}

	return modcache;
}

void R_EnableWSurfVBOSolid(wsurf_model_t *modcache)
{
	if (!r_wsurf_vbo->value)
		return;

	if (r_wsurf.pCurrentModel == modcache)
		return;

	r_wsurf.pCurrentModel = modcache;

	if (!modcache)
	{
		qglDisableVertexAttribArray(1);
		qglDisableVertexAttribArray(0);

		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		qglDisableClientState(GL_NORMAL_ARRAY);
		qglDisableClientState(GL_VERTEX_ARRAY);
	}
	else
	{
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, modcache->hEBO);
		qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
		qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));

		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
}

void R_EnableWSurfVBO(wsurf_model_t *modcache)
{
	if (!r_wsurf_vbo->value)
		return;

	if (r_wsurf.pCurrentModel == modcache)
		return;

	r_wsurf.pCurrentModel = modcache;

	if (!modcache)
	{
		qglDisableVertexAttribArray(1);
		qglDisableVertexAttribArray(0);

		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		qglClientActiveTextureARB(GL_TEXTURE4_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE3_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE2_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		qglDisableClientState(GL_NORMAL_ARRAY);
		qglDisableClientState(GL_VERTEX_ARRAY);
	}
	else
	{
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, modcache->hEBO);
		qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
		qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));

		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		qglClientActiveTextureARB(GL_TEXTURE2_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		qglClientActiveTextureARB(GL_TEXTURE3_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normaltexcoord));
		qglClientActiveTextureARB(GL_TEXTURE4_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, parallaxtexcoord));

		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
}

void R_DrawWSurfVBO(wsurf_model_t *modcache)
{
	if (r_wsurf.bLightmapTexture)
	{
		GL_EnableMultitexture();
		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_TEXTURE_2D_ARRAY);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
	}
	else
	{
		GL_DisableMultitexture();
	}

	if (r_wsurf.bDiffuseTexture)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglEnable(GL_TEXTURE_2D);
	}
	else
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglDisable(GL_TEXTURE_2D);
	}

	//Static texchain

	for (size_t i = 0; i < modcache->vTextureChainStatic.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainStatic[i];

		GL_Bind(texchain.pTexture->gl_texturenum);

		R_BeginDetailTexture(texchain.pTexture->gl_texturenum);

		int WSurfProgramState = 0;

		if (r_wsurf.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
		}

		if (r_wsurf.bLightmapTexture)
		{
			WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
		}

		if (r_wsurf.bDetailTexture)
		{
			WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
		}

		if (r_wsurf.bNormalTexture)
		{
			WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
		}

		if (r_wsurf.bParallaxTexture)
		{
			WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
		}

		if (r_draw_pass == r_draw_reflect && curwater)
		{
			WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
		}

		if (!drawgbuffer && r_fog_mode == GL_LINEAR)
		{
			WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
		}

		if (drawgbuffer)
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		}

		if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
		{
			WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
		}

		auto prog = R_UseWSurfProgram(WSurfProgramState);

		if (prog->speed != -1)
			qglUniform1fARB(prog->speed, 0);

		qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		R_EndDetailTexture();

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.iFaceCount;
	}

	//Animated texchain

	for (size_t i = 0; i < modcache->vTextureChainAnim.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainAnim[i];

		auto base = texchain.pTexture;

		if ((*currententity)->curstate.frame)
		{
			if (base->alternate_anims)
				base = base->alternate_anims;
		}

		int reletive = (int)((*cl_time) * 10.0f) % base->anim_total;

		int count = 0;

		while (base->anim_min > reletive || base->anim_max <= reletive)
		{
			base = base->anim_next;

			if (!base)
				Sys_ErrorEx("R_TextureAnimation: broken cycle");

			if (++count > 100)
				Sys_ErrorEx("R_TextureAnimation: infinite cycle");
		}

		GL_Bind(base->gl_texturenum);

		R_BeginDetailTexture(base->gl_texturenum);

		int WSurfProgramState = 0;

		if (r_wsurf.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
		}

		if (r_wsurf.bLightmapTexture)
		{
			WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
		}

		if (r_wsurf.bDetailTexture)
		{
			WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
		}

		if (r_wsurf.bNormalTexture)
		{
			WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
		}

		if (r_wsurf.bParallaxTexture)
		{
			WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
		}

		if (r_draw_pass == r_draw_reflect && curwater)
		{
			WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
		}

		if (!drawgbuffer && r_fog_mode == GL_LINEAR)
		{
			WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
		}

		if (drawgbuffer)
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		}

		if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
		{
			WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
		}

		auto prog = R_UseWSurfProgram(WSurfProgramState);

		if (prog->speed != -1)
			qglUniform1fARB(prog->speed, 0);

		qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		R_EndDetailTexture();

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.iFaceCount;
	}

	//Use scrolling shader
	if (modcache->vTextureChainScroll.size())
	{
		float scrollSpeed = ((*currententity)->curstate.rendercolor.b + ((*currententity)->curstate.rendercolor.g << 8)) / 16.0;
		if ((*currententity)->curstate.rendercolor.r == 0)
			scrollSpeed = -scrollSpeed;
		scrollSpeed *= (*cl_time);

		for (size_t i = 0; i < modcache->vTextureChainScroll.size(); ++i)
		{
			auto &texchain = modcache->vTextureChainScroll[i];

			GL_Bind(texchain.pTexture->gl_texturenum);

			R_BeginDetailTexture(texchain.pTexture->gl_texturenum);

			wsurf_program_t wprog = { 0 };

			int WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_LIGHTMAP_ENABLED;

			if (r_wsurf.bDetailTexture)
			{
				WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
			}

			if (r_wsurf.bNormalTexture)
			{
				WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
			}

			if (r_wsurf.bParallaxTexture)
			{
				WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
			}

			if (r_draw_pass == r_draw_reflect && curwater)
			{
				WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
			}

			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
			}

			if (drawgbuffer)
			{
				WSurfProgramState |= WSURF_GBUFFER_ENABLED;
			}

			if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
			{
				WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
			}

			auto prog = R_UseWSurfProgram(WSurfProgramState);

			if (prog->speed != -1)
				qglUniform1fARB(prog->speed, scrollSpeed);

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			R_EndDetailTexture();

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}
	}

	if (!r_wsurf.bDiffuseTexture)
	{
		qglEnable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bLightmapTexture)
	{
		GL_SelectTexture(TEXTURE1_SGIS);
		qglDisable(GL_TEXTURE_2D_ARRAY);
		qglEnable(GL_TEXTURE_2D);
	}

	qglUseProgramObjectARB(0);
}

void R_DrawWSurfVBOSolid(wsurf_model_t *modcache)
{
	for (size_t i = 0; i < modcache->vTextureChainStatic.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainStatic[i];

		qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.iFaceCount;
	}

	//Use scrolling shader
	if (modcache->vTextureChainScroll.size())
	{
		for (size_t i = 0; i < modcache->vTextureChainScroll.size(); ++i)
		{
			auto &texchain = modcache->vTextureChainScroll[i];

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}
	}
}

void R_InitWSurf(void)
{
	r_wsurf.hVBO = 0;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bDetailTexture = false;
	r_wsurf.bNormalTexture = false;
	r_wsurf.bParallaxTexture = false;
	r_wsurf.pDetailTextureCache = NULL;
	r_wsurf.pCurrentModel = NULL;
	r_wsurf.iNumBSPEntities = 0;
	r_wsurf.iNumLightmapTextures = 0;
	r_wsurf.iLightmapTextureArray = 0;
	r_wsurf.vVertexBuffer = 0;
	r_wsurf.iNumVerts = 0;
	r_wsurf.vFaceBuffer = 0;
	r_wsurf.iNumFaces = 0;

	memset(g_WSurfProgramTable, 0, sizeof(g_WSurfProgramTable));

	R_ClearBSPEntities();

	r_wsurf_vbo = gEngfuncs.pfnRegisterVariable("r_wsurf_vbo", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_parallax_scale = gEngfuncs.pfnRegisterVariable("r_wsurf_parallax_scale", "0.03", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_detail = gEngfuncs.pfnRegisterVariable("r_wsurf_detail", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}

void R_ShutdownWSurf(void)
{
	for (int i = 0; i < _ARRAYSIZE(g_WSurfProgramTable); ++i)
	{
		if (g_WSurfProgramTable[i])
			delete g_WSurfProgramTable[i];
		g_WSurfProgramTable[i] = NULL;
	}

	R_FreeLightmapArray();
	R_FreeVertexBuffer();
	R_ClearWSurfModelCache();
	R_ClearBSPEntities();
}

void R_LoadDetailTextures(void)
{
	std::string name = gEngfuncs.pfnGetLevelName();
	name = name.substr(0, name.length() - 4);
	name += "_detail.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_Printf("R_LoadDetailTextures: Failed to load detail texture file %s\n", name.c_str());
		return;
	}

	char *ptext = pfile;
	while (1)
	{
		char temp[256];
		char basetexture[256];
		char detailtexture[256];
		char sz_xscale[64];
		char sz_yscale[64];

		ptext = gEngfuncs.COM_ParseFile(ptext, basetexture);

		if (!ptext)
			break;

		if (basetexture[0] == '{')
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, temp);
			strcat(basetexture, temp);

			if (!ptext)
				break;
		}

		ptext = gEngfuncs.COM_ParseFile(ptext, detailtexture);
		if (!ptext)
			break;

		ptext = gEngfuncs.COM_ParseFile(ptext, sz_xscale);

		if (!ptext)
			break;

		if (sz_xscale[0] == '{')
		{
			strcat(detailtexture, sz_xscale);

			ptext = gEngfuncs.COM_ParseFile(ptext, temp);
			if (!ptext)
				break;

			strcat(detailtexture, temp);

			ptext = gEngfuncs.COM_ParseFile(ptext, sz_xscale);
			if (!ptext)
				break;
		}

		ptext = gEngfuncs.COM_ParseFile(ptext, sz_yscale);
		if (!ptext)
			break;

		int texType = WSURF_DETAIL_TEXTURE;

		std::string base = basetexture;

		if (base.find("_PARALLAX") == base.length() - (sizeof("_PARALLAX") - 1))
		{
			base = base.substr(0, base.length() - (sizeof("_PARALLAX") - 1));
			texType = WSURF_PARALLAX_TEXTURE;
		}
		else if (base.find("_NORMAL") == base.length() - (sizeof("_NORMAL") - 1))
		{
			base = base.substr(0, base.length() - (sizeof("_NORMAL") - 1));
			texType = WSURF_NORMAL_TEXTURE;
		}

		auto glt = GL_FindTexture(base.c_str(), GLT_WORLD, NULL, NULL);

		if (!glt)
		{
			gEngfuncs.Con_Printf("R_LoadDetailTextures: Missing basetexture %s\n", base.c_str());
			continue;
		}

		float i_xscale = atof(sz_xscale);
		float i_yscale = atof(sz_yscale);

		detail_texture_cache_t *cache = NULL;

		auto itor = std::find_if(g_DetailTextureTable.begin(), g_DetailTextureTable.end(),
			[&base](const std::pair<int, detail_texture_cache_t *>& pair) {return (pair.second->basetexture == base); });

		if (itor != g_DetailTextureTable.end())
		{
			cache = itor->second;
		}
		else
		{
			cache = new detail_texture_cache_t;
			cache->basetexture = base;
			g_DetailTextureTable[glt] = cache;
		}

		if (cache)
		{
			if (cache->tex[texType].gltexturenum)
			{
				gEngfuncs.Con_Printf("R_LoadDetailTextures: Textype %d already loaded for basetexture %s\n", texType, base.c_str());
				continue;
			}

			int width = 0, height = 0;

			std::string texturePath = "gfx/";
			texturePath += detailtexture;
			if (!V_GetFileExtension(detailtexture))
				texturePath += ".tga";

			int texId = R_LoadTexture(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD);
			if (!texId)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				texId = R_LoadTexture(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD);
			}

			if (!texId)
			{
				gEngfuncs.Con_Printf("R_LoadDetailTextures: Missing detailtexture %s\n", detailtexture);
				continue;
			}

			cache->tex[texType].gltexturenum = texId;
			cache->tex[texType].width = width;
			cache->tex[texType].height = height;
			cache->tex[texType].scaleX = i_xscale;
			cache->tex[texType].scaleY = i_yscale;
		}
	}
	gEngfuncs.COM_FreeFile(pfile);
}

void R_VidInitWSurf(void)
{
	for (auto p : g_DetailTextureTable)
		delete p.second;

	g_DetailTextureTable.clear();
	R_LoadDetailTextures();

	R_FreeLightmapArray();
	R_FreeVertexBuffer();
	R_GenerateVertexBuffer();
	R_GenerateLightmapArray();
	R_ClearWSurfModelCache();
	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities);
	R_LoadBSPEntities();
}

void R_DrawWireFrame(brushface_t *brushface, void(*draw)(brushface_t *face))
{
	if (gl_wireframe->value)
	{
		R_UseGBufferProgram(GBUFFER_TRANSPARENT_ENABLED);
		R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);

		qglColor3f(1, 1, 1);
		qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		qglLineWidth(1);
		if (gl_wireframe->value == 2)
			qglDisable(GL_DEPTH_TEST);

		if ((*mtexenabled))
		{
			GL_DisableMultitexture();
			qglDisable(GL_TEXTURE_2D);

			draw(brushface);

			qglEnable(GL_TEXTURE_2D);
			GL_EnableMultitexture();
		}
		else
		{
			qglDisable(GL_TEXTURE_2D);

			draw(brushface);

			qglEnable(GL_TEXTURE_2D);
		}

		qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (gl_wireframe->value == 2)
			qglEnable(GL_DEPTH_TEST);
	}
}

void R_BeginDetailTexture(int texId)
{
	if (!r_wsurf_detail->value)
		return;

	auto itor = g_DetailTextureTable.find(texId);

	if (itor != g_DetailTextureTable.end())
	{
		auto cache = itor->second;

		if (r_wsurf.bDiffuseTexture && cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum)
		{
			GL_Bind(cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum);
		}

		if (cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum)
		{
			qglActiveTextureARB(TEXTURE2_SGIS);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum);

			r_wsurf.bDetailTexture = true;
		}

		if (cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum)
		{
			qglActiveTextureARB(TEXTURE3_SGIS);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum);

			r_wsurf.bNormalTexture = true;
		}

		if (cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum)
		{
			qglActiveTextureARB(GL_TEXTURE4_ARB);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum);

			r_wsurf.bParallaxTexture = true;
		}

		if (r_wsurf.bDetailTexture || r_wsurf.bNormalTexture || r_wsurf.bParallaxTexture)
		{
			r_wsurf.pDetailTextureCache = cache;
			return;
		}
	}
}

void R_EndDetailTexture(void)
{
	bool bRestore = false;

	if (r_wsurf.bDetailTexture)
	{
		r_wsurf.bDetailTexture = false;
		bRestore = true;

		qglActiveTextureARB(TEXTURE2_SGIS);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bNormalTexture)
	{
		r_wsurf.bNormalTexture = false;
		bRestore = true;

		qglActiveTextureARB(TEXTURE3_SGIS);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bParallaxTexture)
	{
		r_wsurf.bParallaxTexture = false;
		bRestore = true;

		qglActiveTextureARB(GL_TEXTURE4_ARB);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}

	if (bRestore)
	{
		if ((*mtexenabled))
			qglActiveTextureARB(TEXTURE1_SGIS);
		else
			qglActiveTextureARB(TEXTURE0_SGIS);
	}
}

void DrawGLVertex(brushface_t *brushface)
{
	brushvertex_t *vert = &r_wsurf.vVertexBuffer[brushface->start_vertex];

	qglBegin( GL_POLYGON );
	for(int i = 0; i < brushface->num_vertexes; i++, vert++)
	{
		if (r_wsurf.bDiffuseTexture)
			qglMultiTexCoord3fARB(TEXTURE0_SGIS, vert->texcoord[0], vert->texcoord[1], vert->texcoord[2]);

		if (r_wsurf.bLightmapTexture)
			qglMultiTexCoord3fARB(TEXTURE1_SGIS, vert->lightmaptexcoord[0], vert->lightmaptexcoord[1], vert->lightmaptexcoord[2]);

		if (r_wsurf.bDetailTexture)
			qglMultiTexCoord2fARB(TEXTURE2_SGIS, vert->detailtexcoord[0], vert->detailtexcoord[1]);

		if (r_wsurf.bNormalTexture)
			qglMultiTexCoord2fARB(TEXTURE3_SGIS, vert->normaltexcoord[0], vert->normaltexcoord[1]);

		if (r_wsurf.bParallaxTexture)
			qglMultiTexCoord2fARB(GL_TEXTURE4_ARB, vert->parallaxtexcoord[0], vert->parallaxtexcoord[1]);

		if (r_wsurf.iS_Tangent != -1)
			qglVertexAttrib3fv(r_wsurf.iS_Tangent, vert->s_tangent);

		if (r_wsurf.iT_Tangent != -1)
			qglVertexAttrib3fv(r_wsurf.iT_Tangent, vert->t_tangent);

		qglNormal3fv(vert->normal);
		qglVertex3fv(vert->pos);
	}
	qglEnd();

	r_wsurf_polys ++;
	r_wsurf_drawcall ++;
}

void DrawGLPoly(glpoly_t *p)
{
	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	DrawGLVertex(brushface);

	R_DrawWireFrame(brushface, DrawGLVertex);
}

void DrawGLPoly(msurface_t *fa)
{
	auto p = fa->polys;
	return DrawGLPoly(p);
}

void R_DrawSequentialPoly(msurface_t *s, int face)
{
	if ((s->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER)))
	{
		if (s->flags & SURF_DRAWTURB)
		{
			GL_DisableMultitexture();
			GL_Bind(s->texinfo->texture->gl_texturenum);
			EmitWaterPolys(s, face);
			return;
		}
		return;
	}

	R_RenderDynamicLightmaps(s);

	auto p = s->polys;
	auto t = gRefFuncs.R_TextureAnimation(s);

	if (r_wsurf.bLightmapTexture)
	{
		GL_EnableMultitexture();
		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_TEXTURE_2D_ARRAY);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);

		int lightmapnum = s->lightmaptexturenum;

		if (lightmap_modified[lightmapnum])
		{
			lightmap_modified[lightmapnum] = 0;
			if (g_iEngineType == ENGINE_SVENGINE)
			{
				glRect_SvEngine_t *theRect = (glRect_SvEngine_t *)((char *)lightmap_rectchange + sizeof(glRect_SvEngine_t) * lightmapnum);
				qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
				theRect->l = BLOCK_WIDTH;
				theRect->t = BLOCK_HEIGHT;
				theRect->h = 0;
				theRect->w = 0;
			}
			else
			{
				glRect_GoldSrc_t *theRect = (glRect_GoldSrc_t *)((char *)lightmap_rectchange + sizeof(glRect_GoldSrc_t) * lightmapnum);
				qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
				theRect->l = BLOCK_WIDTH;
				theRect->t = BLOCK_HEIGHT;
				theRect->h = 0;
				theRect->w = 0;
			}
		}
	}
	else
	{
		GL_DisableMultitexture();
	}

	if (r_wsurf.bDiffuseTexture)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		GL_Bind(t->gl_texturenum);
	}
	else
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bDiffuseTexture)
	{
		R_BeginDetailTexture(t->gl_texturenum);
	}

	int WSurfProgramState = 0;

	if (r_wsurf.bDiffuseTexture)
	{
		WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
	}

	if (r_wsurf.bLightmapTexture)
	{
		WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
	}

	if (r_wsurf.bDetailTexture)
	{
		WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
	}

	if (r_wsurf.bNormalTexture)
	{
		WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
	}

	if (r_wsurf.bParallaxTexture)
	{
		WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
	}

	if (r_draw_pass == r_draw_reflect && curwater)
	{
		WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
	}

	if (!drawgbuffer && r_fog_mode == GL_LINEAR)
	{
		WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
	}

	if (drawgbuffer)
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}

	if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
	{
		WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
	}

	auto prog = R_UseWSurfProgram(WSurfProgramState);

	float speed = 0;

	if (s->flags & SURF_DRAWTILED)
	{
		speed = ((*currententity)->curstate.rendercolor.b + ((*currententity)->curstate.rendercolor.g << 8)) / 16.0;

		if ((*currententity)->curstate.rendercolor.r == 0)
			speed = -speed;
	}

	if (prog->speed != -1)
	{
		qglUniform1fARB(prog->speed, speed);
	}

	r_wsurf.iS_Tangent = prog->s_tangent;
	r_wsurf.iT_Tangent = prog->t_tangent;

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	DrawGLPoly(p);

	qglUseProgramObjectARB(0);

	R_EndDetailTexture();

	if (r_wsurf.bLightmapTexture)
	{
		GL_SelectTexture(TEXTURE1_SGIS);
		qglDisable(GL_TEXTURE_2D_ARRAY);

		if(*mtexenabled)
			qglEnable(GL_TEXTURE_2D);
		else
			qglDisable(GL_TEXTURE_2D);
	}

	if (!r_wsurf.bDiffuseTexture)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglEnable(GL_TEXTURE_2D);
	}

	if (s->pdecals)
	{
		gDecalSurfs[(*gDecalSurfCount)] = s;
		(*gDecalSurfCount)++;

		if ((*gDecalSurfCount) > MAX_DECALSURFS)
			Sys_ErrorEx("Too many decal surfaces!\n");

		if (r_wsurf.bDiffuseTexture)
		{
			R_DrawDecals(r_wsurf.bLightmapTexture ? true : false);
		}
	}
}

char *ValueForKey(bspentity_t *ent, char *key)
{
   for (epair_t  *pEPair = ent->epairs; pEPair; pEPair = pEPair->next)
   {
      if (!strcmp(pEPair->key, key) )
         return pEPair->value;
   }
   return NULL;
}

void R_ClearBSPEntities(void)
{
	for(int i = 0; i < r_wsurf.iNumBSPEntities; i++)
	{
		epair_t *pPair = r_wsurf.pBSPEntities[i].epairs;
		while(pPair)
		{
			epair_t *pFree = pPair;
			pPair = pFree->next;

			delete [] pFree->key;
			delete [] pFree->value;
			delete pFree;
		}
		r_wsurf.pBSPEntities[i].epairs = NULL;
		r_wsurf.pBSPEntities[i].classname = NULL;
		VectorClear(r_wsurf.pBSPEntities[i].origin);
	}
	
	r_wsurf.iNumBSPEntities = 0;

	r_light_env_angles_exists = false;
	r_light_env_color_exists = false;
	VectorClear(r_light_env_angles);
	g_DeferredLights.clear();
}

bspentity_t *current_parse_entity = NULL;
char com_token[4096];

bool R_ParseBSPEntityKeyValue(const char *classname, const char *keyname, const char *value)
{
	if (classname == NULL)
	{
		if (r_wsurf.iNumBSPEntities >= MAX_MAP_BSPENTITY)
			return false;

		current_parse_entity = &r_wsurf.pBSPEntities[r_wsurf.iNumBSPEntities];
		r_wsurf.iNumBSPEntities++;

		current_parse_entity->classname = NULL;
		current_parse_entity->epairs = NULL;
		VectorClear(current_parse_entity->origin);
	}

	if (current_parse_entity)
	{
		auto epairs = new epair_t;
		auto keynamelen = strlen(keyname);
		epairs->key = new char[keynamelen + 1];
		strncpy(epairs->key, keyname, keynamelen);
		epairs->key[keynamelen] = 0;

		auto valuelen = strlen(value);
		epairs->value = new char[valuelen + 1];
		strncpy(epairs->value, value, valuelen);
		epairs->value[valuelen] = 0;

		if (!strcmp(keyname, "origin"))
		{
			sscanf(value, "%f %f %f", &current_parse_entity->origin[0], &current_parse_entity->origin[1], &current_parse_entity->origin[2]);
		}

		if (!strcmp(keyname, "classname"))
		{
			current_parse_entity->classname = epairs->value;
		}

		epairs->next = current_parse_entity->epairs;
		current_parse_entity->epairs = epairs;

		return true;
	}

	return false;
}

bool R_ParseBSPEntityClassname(char *szInputStream, char *classname)
{
	char szKeyName[256];

	// key
	szInputStream = gEngfuncs.COM_ParseFile(szInputStream, com_token);
	while (szInputStream && com_token[0] != '}')
	{
		strncpy(szKeyName, com_token, sizeof(szKeyName) - 1);
		szKeyName[sizeof(szKeyName) - 1] = 0;

		szInputStream = gEngfuncs.COM_ParseFile(szInputStream, com_token);

		if (!strcmp(szKeyName, "classname"))
		{
			R_ParseBSPEntityKeyValue(NULL, szKeyName, com_token);

			strcpy(classname, com_token);

			return true;
		}

		if (!szInputStream)
		{
			break;
		}

		szInputStream = gEngfuncs.COM_ParseFile(szInputStream, com_token);
	}

	return false;
}

char *R_ParseBSPEntity(char *data)
{
	char keyname[256] = { 0 };
	char classname[256] = { 0 };

	if (R_ParseBSPEntityClassname(data, classname))
	{
		while (1)
		{
			data = gEngfuncs.COM_ParseFile(data, com_token);
			if (com_token[0] == '}')
			{
				break;
			}
			if (!data)
			{
				Sys_ErrorEx("R_ParseBSPEntity: EOF without closing brace");
			}

			strncpy(keyname, com_token, sizeof(keyname) - 1);
			keyname[sizeof(keyname) - 1] = 0;
			// Remove tail spaces
			for (int n = strlen(keyname) - 1; n >= 0 && keyname[n] == ' '; n--)
			{
				keyname[n] = 0;
			}

			data = gEngfuncs.COM_ParseFile(data, com_token);
			if (!data)
			{
				Sys_ErrorEx("R_ParseBSPEntity: EOF without closing brace");
			}
			if (com_token[0] == '}')
			{
				Sys_ErrorEx("R_ParseBSPEntity: closing brace without data");
			}

			if (!strcmp(classname, com_token))
			{
				continue;
			}

			R_ParseBSPEntityKeyValue(classname, keyname, com_token);
		}
	}
	else
	{
		gEngfuncs.Con_Printf("R_ParseBSPEntity: missing classname, try next section.");
		while (1)
		{
			data = gEngfuncs.COM_ParseFile(data, com_token);
			if (!data)
			{
				break;
			}
			if (com_token[0] == '}')
			{
				break;
			}
		}
	}

	current_parse_entity = NULL;

	return data;
}

void R_ParseBSPEntities(char *data)
{
	while (1)
	{
		data = gEngfuncs.COM_ParseFile(data, com_token);
		if (!data)
		{
			break;
		}
		if (com_token[0] != '{')
		{
			Sys_ErrorEx("R_ParseBSPEntities: found %s when expecting {", com_token);
			return;
		}
		data = R_ParseBSPEntity(data);
	}
}

void R_LoadBSPEntities(void)
{
	for(int i = 0; i < r_wsurf.iNumBSPEntities; i++)
	{
		bspentity_t *ent = &r_wsurf.pBSPEntities[i];

		char *classname = ent->classname;

		if(!classname)
			continue;

		if (!strcmp(classname, "light"))
		{
			int color[4] = {255, 255, 255, 255};
			char *s_light = ValueForKey(ent, "_light");
			if (s_light)
			{
				sscanf(s_light, "%d %d %d %d", &color[0], &color[1], &color[2], &color[3]);
			}

			float org[3] = { 0 };
			char *s_origin = ValueForKey(ent, "origin");
			if (s_origin)
			{
				sscanf(s_origin, "%f %f %f", &org[0], &org[1], &org[2]);
			}

			float fade = 1;
			char *s_fade = ValueForKey(ent, "_fade");
			if (s_fade)
			{
				sscanf(s_fade, "%f", &fade);
			}

			float col[4] = { color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, color[3] / 255.0f };

			g_DeferredLights.emplace_back(0, org, col, fade);
		}

		else if (!strcmp(classname, "light_environment"))
		{
			char *light = ValueForKey(ent, "_light");
			if (light)
			{
				sscanf(light, "%d %d %d %d", &r_light_env_color[0], &r_light_env_color[1], &r_light_env_color[2], &r_light_env_color[3]);
				r_light_env_color_exists = true;
			}
			
			char *pitch = ValueForKey(ent, "pitch");
			if (pitch)
			{
				sscanf(pitch, "%f", &r_light_env_angles[0]);

				r_light_env_angles[0] += 180;
				if (r_light_env_angles[0] > 360)
					r_light_env_angles[0] -= 360;
			}

			char *angle = ValueForKey(ent, "angles");
			if (angle)
			{
				vec3_t ang = { 0 };
				sscanf(angle, "%f %f %f", &ang[0], &ang[1], &ang[2]);
				if (ang[0] == 0 && ang[1] == 0 && ang[2] == 0)
				{

				}
				else
				{
					if (pitch)
					{
						ang[0] = r_light_env_angles[0];
					}

					VectorCopy(ang, r_light_env_angles);
					r_light_env_angles[1] += 180;
					if (r_light_env_angles[1] > 360)
						r_light_env_angles[1] -= 360;
					r_light_env_angles_exists = true;
				}
			}
		}
	}//end for
}

void R_DrawSequentialPolyVBO(msurface_t *s)
{
	R_RenderDynamicLightmaps(s);

	auto lightmapnum = s->lightmaptexturenum;

	if (lightmap_modified[lightmapnum])
	{
		lightmap_modified[lightmapnum] = 0;
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			glRect_SvEngine_t *theRect = (glRect_SvEngine_t *)((char *)lightmap_rectchange + sizeof(glRect_SvEngine_t) * lightmapnum);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
			qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
		else
		{
			glRect_GoldSrc_t *theRect = (glRect_GoldSrc_t *)((char *)lightmap_rectchange + sizeof(glRect_GoldSrc_t) * lightmapnum);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
			qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
	}

	if (s->pdecals)
	{
		gDecalSurfs[(*gDecalSurfCount)] = s;
		(*gDecalSurfCount)++;

		if ((*gDecalSurfCount) > MAX_DECALSURFS)
			Sys_ErrorEx("Too many decal surfaces!\n");

		if (r_wsurf.bDiffuseTexture)
		{
			R_DrawDecals(r_wsurf.bLightmapTexture ? true : false);
		}
	}
}

void R_RecursiveWorldNodeVBO(mnode_t *node)
{
	int c, side;
	mplane_t *plane;
	msurface_t *surf;
	float dot;

	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != (*r_visframecount))
		return;

	if (R_CullBox(node->minmaxs, node->minmaxs + 3))
		return;

	if (node->contents < 0)
	{
		auto pleaf = (mleaf_t *)node;

		auto mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = (*r_framecount);
				mark++;
			} while (--c);
		}

		//if (pleaf->efrags)
		//	R_StoreEfrags(&pleaf->efrags);

		return;
	}

	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
	{
		dot = r_refdef->vieworg[0] - plane->dist;
		break;
	}

	case PLANE_Y:
	{
		dot = r_refdef->vieworg[1] - plane->dist;
		break;
	}

	case PLANE_Z:
	{
		dot = r_refdef->vieworg[2] - plane->dist;
		break;
	}

	default:
	{
		dot = DotProduct(r_refdef->vieworg, plane->normal) - plane->dist;
		break;
	}
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

	R_RecursiveWorldNodeVBO(node->children[side]);

	c = node->numsurfaces;

	if (c)
	{
		surf = r_worldmodel->surfaces + node->firstsurface;

		if (dot < 0 - BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;

		for (; c; c--, surf++)
		{
			if (surf->visframe != (*r_framecount))
				continue;

			if (!(surf->flags & SURF_UNDERWATER) && ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
				continue;

			if (surf->flags & SURF_DRAWSKY)
			{
				surf->texturechain = (*skychain);
				(*skychain) = surf;
			}
			else if (surf->flags & SURF_DRAWTURB)
			{
				surf->texturechain = (*waterchain);
				(*waterchain) = surf;
			}
			else
			{
				R_DrawSequentialPolyVBO(surf);
			}
		}
	}

	R_RecursiveWorldNodeVBO(node->children[!side]);
}

void R_DrawBrushModel(cl_entity_t *e)
{
	int i;
	int k;
	vec3_t mins, maxs;
	float dot;
	mplane_t *pplane;
	model_t *clmodel;
	qboolean rotated;

	(*currententity) = e;
	(*currenttexture) = -1;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;

		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(e->origin, clmodel->mins, mins);
		VectorAdd(e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;

	qglColor3f(1, 1, 1);
	memset(lightmap_polys, 0, sizeof(glpoly_t *) * 1024);

	VectorSubtract(r_refdef->vieworg, e->origin, modelorg);

	if (rotated)
	{
		vec3_t temp;
		vec3_t forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	if (!r_light_dynamic->value)
	{
		if (clmodel->firstmodelsurface != 0)
		{
			int max_dlights;

			if (g_iEngineType == ENGINE_SVENGINE)
			{
				max_dlights = 256;
			}
			else
			{
				max_dlights = 32;
			}

			for (k = 0; k < max_dlights; k++)
			{
				vec3_t saveOrigin;

				if ((cl_dlights[k].die < (*cl_time)) || (!cl_dlights[k].radius))
					continue;

				VectorCopy(cl_dlights[k].origin, saveOrigin);
				VectorSubtract(cl_dlights[k].origin, e->origin, cl_dlights[k].origin);

				//R_MarkLights(&cl_dlights[k], 1 << k, clmodel->nodes + clmodel->hulls[0].firstclipnode);
				VectorCopy(saveOrigin, cl_dlights[k].origin);
			}
		}
	}

	qglPushMatrix();
	R_RotateForEntity(e->origin, e);

	R_SetRenderMode(e);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	qglEnable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);
	qglStencilFunc(GL_ALWAYS, 0, 0xFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		r_wsurf.bDiffuseTexture = false;
		r_wsurf.bLightmapTexture = false;
	}
	else if ((*currententity)->curstate.rendermode == kRenderTransAlpha || (*currententity)->curstate.rendermode == kRenderNormal)
	{
		r_wsurf.bDiffuseTexture = true;
		r_wsurf.bLightmapTexture = true;
	}
	else
	{
		r_wsurf.bDiffuseTexture = true;
		r_wsurf.bLightmapTexture = false;
	}

	if (r_wsurf_vbo->value)
	{
		auto modcache = R_PrepareWSurfVBO(clmodel);

		R_EnableWSurfVBO(modcache);

		R_DrawWSurfVBO(modcache);

		R_EnableWSurfVBO(NULL);

		auto psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
		for (i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				if (pplane->type != PLANE_Z)
					continue;

				if (mins[2] + 1.0 >= pplane->dist)
					continue;
			}

			dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

			if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
			{
				//fuck water
				if (psurf->flags & SURF_DRAWTURB)
				{
					R_SetRenderMode(e);
					R_DrawSequentialPoly(psurf, 0);
				}
				else
				{
					R_DrawSequentialPolyVBO(psurf);
				}
			}
			else
			{
				if (psurf->flags & SURF_DRAWTURB)
				{
					R_SetRenderMode(e);
					R_DrawSequentialPoly(psurf, 1);
				}
			}
		}

	}
	else
	{
		auto psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
		for (i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				if (pplane->type != PLANE_Z)
					continue;

				if (mins[2] + 1.0 >= pplane->dist)
					continue;
			}

			dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

			if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
			{
				R_SetRenderMode(e);
				R_DrawSequentialPoly(psurf, 0);
			}
			else
			{
				if (psurf->flags & SURF_DRAWTURB)
				{
					R_SetRenderMode(e);
					R_DrawSequentialPoly(psurf, 1);
				}
			}
		}
	}

	if ((*currententity)->curstate.rendermode != kRenderNormal)
	{
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		qglDisable(GL_BLEND);
	}

	qglPopMatrix();
	qglDepthMask(1);
	qglDisable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_NOTEQUAL, 0);
	qglDisable(GL_BLEND);

	qglStencilMask(0);
	qglEnable(GL_STENCIL_TEST);

	r_rotate_entity = false;
}

void R_DrawWorld(void)
{
	R_BeginRenderGBuffer();

	VectorCopy(r_refdef->vieworg, modelorg);

	cl_entity_t tempent = { 0 };
	tempent.model = r_worldmodel;
	tempent.curstate.rendercolor.r = cshift_water->destcolor[0];
	tempent.curstate.rendercolor.g = cshift_water->destcolor[1];
	tempent.curstate.rendercolor.b = cshift_water->destcolor[2];

	(*currententity) = &tempent;
	*currenttexture = -1;

	qglColor3f(1.0f, 1.0f, 1.0f);
	memset(lightmap_polys, 0, sizeof(glpoly_t *) * 1024);

	qglEnable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);
	qglStencilFunc(GL_ALWAYS, 0, 0xFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	GL_DisableMultitexture();
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	r_wsurf.bDiffuseTexture = true;
	r_wsurf.bLightmapTexture = true;

	if (r_wsurf_vbo->value)
	{
		auto modcache = R_PrepareWSurfVBO(r_worldmodel);

		R_EnableWSurfVBO(modcache);

		R_DrawWSurfVBO(modcache);

		R_EnableWSurfVBO(NULL);
		
		(*gDecalSurfCount) = 0;
		R_RecursiveWorldNodeVBO(r_worldmodel->nodes);
		(*gDecalSurfCount) = 0;
	}
	else
	{
		(*gDecalSurfCount) = 0;
		R_RecursiveWorldNode(r_worldmodel->nodes);
		(*gDecalSurfCount) = 0;
	}

	(*currententity) = gEngfuncs.GetEntityByIndex(0);

	GL_DisableMultitexture();

	R_DrawSkyBox();

	(*skychain) = 0;

	if ((*waterchain))
	{
		for (auto s = (*waterchain); s; s = s->texturechain)
		{
			qglColor4ub(255, 255, 255, 255);
			GL_Bind(s->texinfo->texture->gl_texturenum);
			EmitWaterPolys(s, 0);
		}
		(*waterchain) = 0;
	}

	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);
}