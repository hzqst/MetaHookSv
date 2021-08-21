#include "gl_local.h"
#include <sstream>
#include <algorithm>

r_worldsurf_t r_wsurf;

cvar_t *r_wsurf_parallax_scale;
cvar_t *r_wsurf_sky_occlusion;
cvar_t *r_wsurf_detail;

int r_fog_mode = 0;
float r_fog_control[2] = { 0 };
float r_fog_color[4] = {0};
float r_shadow_matrix[3][16];
float r_world_matrix_inv[16];
float r_proj_matrix_inv[16];
vec3_t r_frustum_origin[4];
vec3_t r_frustum_vec[4];
float r_near_z = 0;
float r_far_z = 0;
bool r_ortho = false;
int r_wsurf_drawcall = 0;
int r_wsurf_polys = 0;

vec3_t r_entity_mins, r_entity_maxs;

std::unordered_map <int, wsurf_program_t> g_WSurfProgramTable;

std::unordered_map<int, detail_texture_cache_t *> g_DetailTextureTable;

std::vector<wsurf_vbo_t *> g_WSurfVBOCache;

void R_ClearWSurfVBOCache(void)
{
	for (size_t i =0 ;i < g_WSurfVBOCache.size(); ++i)
	{
		if (g_WSurfVBOCache[i])
		{
			auto &VBOCache = g_WSurfVBOCache[i];
			if (VBOCache->hEBO)
			{
				GL_DeleteBuffer(VBOCache->hEBO);
			}
			if (VBOCache->hEntityUBO)
			{
				GL_DeleteBuffer(VBOCache->hEntityUBO);
			}
			if (VBOCache->hTextureSSBO)
			{
				GL_DeleteBuffer(VBOCache->hTextureSSBO);
			}
			if (VBOCache->hDecalEBO)
			{
				GL_DeleteBuffer(VBOCache->hDecalEBO);
			}
			for (size_t j = 0; j < WSURF_TEXCHAIN_MAX; ++j)
			{
				for (size_t k = 0; k < VBOCache->vDrawBatch[j].size(); ++k)
				{
					delete VBOCache->vDrawBatch[j][k];
				}
			}
			delete g_WSurfVBOCache[i];
		}
		g_WSurfVBOCache[i] = NULL;
	}
	g_WSurfVBOCache.clear();
}

void R_UseWSurfProgram(int state, wsurf_program_t *progOutput)
{
	wsurf_program_t prog = { 0 };

	auto itor = g_WSurfProgramTable.find(state);
	if (itor == g_WSurfProgramTable.end())
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

		if (state & WSURF_SPECULARTEXTURE_ENABLED)
			defs << "#define SPECULARTEXTURE_ENABLED\n";

		if (state & WSURF_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & WSURF_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if (state & WSURF_TRANSPARENT_ENABLED)
			defs << "#define TRANSPARENT_ENABLED\n";

		if (state & WSURF_SHADOW_CASTER_ENABLED)
			defs << "#define SHADOW_CASTER_ENABLED\n";

		if (state & WSURF_SHADOWMAP_ENABLED)
			defs << "#define SHADOWMAP_ENABLED\n";

		if (state & WSURF_SHADOWMAP_HIGH_ENABLED)
			defs << "#define SHADOWMAP_HIGH_ENABLED\n";

		if (state & WSURF_SHADOWMAP_MEDIUM_ENABLED)
			defs << "#define SHADOWMAP_MEDIUM_ENABLED\n";

		if (state & WSURF_SHADOWMAP_LOW_ENABLED)
			defs << "#define SHADOWMAP_LOW_ENABLED\n";

		if (state & WSURF_BINDLESS_ENABLED)
			defs << "#define BINDLESS_ENABLED\n";

		if (state & WSURF_LEGACY_ENABLED)
			defs << "#define LEGACY_ENABLED\n";

		if (state & WSURF_DECAL_ENABLED)
			defs << "#define DECAL_ENABLED\n";

		if (state & WSURF_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		if(glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define UINT64_ENABLED\n";
	
		defs << "#define SHADOW_TEXTURE_OFFSET (1.0 / " << std::dec << shadow_texture_size << ".0)\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\wsurf_shader.vsh", "renderer\\shader\\wsurf_shader.fsh", def.c_str(), def.c_str(), NULL);

		SHADER_UNIFORM(prog, u_parallaxScale, "u_parallaxScale");
		SHADER_UNIFORM(prog, u_color, "u_color");

		g_WSurfProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (prog.u_parallaxScale != -1)
			glUniform1f(prog.u_parallaxScale, r_wsurf_parallax_scale->value);

		if (prog.u_color != -1)
			glUniform4f(prog.u_color, 1, 1, 1, 1);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseWSurfProgram: Failed to load program!");
	}
}

void R_FreeSceneUBO(void)
{
	if (r_wsurf.hSceneUBO)
	{
		GL_DeleteBuffer(r_wsurf.hSceneUBO);
		r_wsurf.hSceneUBO = 0;
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, r_wsurf.hSceneUBO);
	}
	if (r_wsurf.hDecalVBO)
	{
		GL_DeleteBuffer(r_wsurf.hDecalVBO);
		r_wsurf.hDecalVBO = 0;
	}
	if (r_wsurf.hDecalTextureSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hDecalTextureSSBO);
		r_wsurf.hDecalTextureSSBO = 0;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, r_wsurf.hDecalTextureSSBO);
	}
}

void R_FreeLightmapArray(void)
{
	if (r_wsurf.iLightmapTextureArray)
	{
		GL_DeleteTexture(r_wsurf.iLightmapTextureArray);
		r_wsurf.iLightmapTextureArray = 0;
	}
	r_wsurf.iNumLightmapTextures = 0;
}

void R_FreeVertexBuffer(void)
{
	if (r_wsurf.hSceneVBO)
	{
		GL_DeleteBuffer(r_wsurf.hSceneVBO);
		r_wsurf.hSceneVBO = 0;
	}

	r_wsurf.vVertexBuffer.clear();
	r_wsurf.vFaceBuffer.clear();
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

void R_GenerateIndicesForTexChain(msurface_t *s, brushtexchain_t *texchain, wsurf_vbo_t *modcache, std::vector<unsigned int> &vIndicesBuffer)
{
	auto p = s->polys;
	auto &brushface = r_wsurf.vFaceBuffer[p->flags];

	if (s->flags & SURF_DRAWSKY)
	{
		if (texchain->iType == TEXCHAIN_SKY)
		{
			for (int i = 0; i < brushface.num_polys; ++i)
			{
				for (int j = 0; j < brushface.num_vertexes[i]; ++j)
				{
					vIndicesBuffer.emplace_back(brushface.start_vertex[i] + j);
					texchain->iIndiceCount++;
				}
				vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
				texchain->iIndiceCount++;
				texchain->iPolyCount++;
			}
		}
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
			for (int i = 0; i < brushface.num_polys; ++i)
			{
				for (int j = 0; j < brushface.num_vertexes[i]; ++j)
				{
					vIndicesBuffer.emplace_back(brushface.start_vertex[i] + j);
					texchain->iIndiceCount++;
				}
				vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
				texchain->iIndiceCount++;
				texchain->iPolyCount++;
			}
		}
	}
	else
	{

		if (texchain->iType == TEXCHAIN_STATIC)
		{
			for (int i = 0; i < brushface.num_polys; ++i)
			{
				for (int j = 0; j < brushface.num_vertexes[i]; ++j)
				{
					vIndicesBuffer.emplace_back(brushface.start_vertex[i] + j);
					texchain->iIndiceCount++;
				}
				vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
				texchain->iIndiceCount++;
				texchain->iPolyCount++;
			}
		}
	}
}

void R_SortTextureChain(wsurf_vbo_t *modcache, int iTexchainType)
{
	modcache->vTextureChain[iTexchainType].shrink_to_fit();

	for (size_t i = 0; i < modcache->vTextureChain[iTexchainType].size(); ++i)
	{
		auto &texchain = modcache->vTextureChain[iTexchainType][i];

		auto pcache = R_FindDetailTextureCache(texchain.pTexture->gl_texturenum);
		if (pcache)
		{
			for (int j = WSURF_REPLACE_TEXTURE; j < WSURF_MAX_TEXTURE; ++j)
			{
				if (pcache->tex[j].gltexturenum)
				{
					texchain.iDetailTextureFlags |= (1 << j);
				}
			}
		}
		else
		{
			texchain.iDetailTextureFlags = 0;
		}
	}

	std::sort(modcache->vTextureChain[iTexchainType].begin(), modcache->vTextureChain[iTexchainType].end(), [](const brushtexchain_t &a, const brushtexchain_t &b) {
		return b.iDetailTextureFlags < a.iDetailTextureFlags;
	});
}

void R_GenerateDrawBatch(wsurf_vbo_t *modcache, int iTexchainType, int iDrawBatchType, std::vector<GLuint64> *ssbo)
{
	int detailTextureFlags = -1;
	wsurf_vbo_batch_t *batch = NULL;

	for (size_t i = 0; i < modcache->vTextureChain[iTexchainType].size(); ++i)
	{
		auto &texchain = modcache->vTextureChain[iTexchainType][i];

		if (ssbo)
		{
			if (!bNoBindless)
			{
				auto handle = glGetTextureHandleARB(texchain.pTexture->gl_texturenum);
				glMakeTextureHandleResidentARB(handle);

				int drawIndex = ssbo->size();

				(*ssbo).resize(drawIndex + 5);

				(*ssbo)[drawIndex + 0] = handle;

				auto pcache = R_FindDetailTextureCache(texchain.pTexture->gl_texturenum);
				if (pcache)
				{
					for (int j = WSURF_REPLACE_TEXTURE; j < WSURF_MAX_TEXTURE; ++j)
					{
						if (pcache->tex[j].gltexturenum)
						{
							auto handle = glGetTextureHandleARB(pcache->tex[j].gltexturenum);
							glMakeTextureHandleResidentARB(handle);

							(*ssbo)[drawIndex + j] = handle;
						}
					}
				}
			}
		}

		if (ssbo && texchain.iDetailTextureFlags != detailTextureFlags)
		{
			if (batch)
			{
				batch->vStartIndex.shrink_to_fit();
				batch->vIndiceCount.shrink_to_fit();
				modcache->vDrawBatch[iDrawBatchType].emplace_back(batch);
				batch = NULL;
			}

			detailTextureFlags = texchain.iDetailTextureFlags;			
		}

		if(!batch)
			batch = new wsurf_vbo_batch_t;

		batch->vStartIndex.emplace_back(BUFFER_OFFSET(texchain.iStartIndex));
		batch->vIndiceCount.emplace_back(texchain.iIndiceCount);
		batch->iDrawCount++;
		batch->iPolyCount += texchain.iPolyCount;
		batch->iDetailTextureFlags = texchain.iDetailTextureFlags;
	}

	if (batch)
	{
		batch->vStartIndex.shrink_to_fit();
		batch->vIndiceCount.shrink_to_fit();
		modcache->vDrawBatch[iDrawBatchType].emplace_back(batch);
	}
}

void R_GenerateTexChain(model_t *mod, wsurf_vbo_t *modcache, std::vector<unsigned int> &vIndicesBuffer)
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

		if (!strcmp(t->name, "sky"))
		{
			auto s = t->texturechain;

			if (s)
			{
				brushtexchain_t texchain;

				texchain.pTexture = t;
				texchain.iIndiceCount = 0;
				texchain.iPolyCount = 0;
				texchain.iStartIndex = vIndicesBuffer.size();
				texchain.iType = TEXCHAIN_SKY;

				for (; s; s = s->texturechain)
				{
					R_GenerateIndicesForTexChain(s, &texchain, modcache, vIndicesBuffer);
				}

				if (texchain.iIndiceCount > 0)
					modcache->TextureChainSky = texchain;
			}
		}
		else if (t->anim_total)
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
								texchain.iIndiceCount = 0;
								texchain.iPolyCount = 0;
								texchain.iStartIndex = vIndicesBuffer.size();
								texchain.iType = TEXCHAIN_STATIC;

								for (int n = 0; n < numtexturechain; ++n)
								{
									if (texchainMapper[n] == k)
										R_GenerateIndicesForTexChain(texchainSurface[n], &texchain, modcache, vIndicesBuffer);
								}

								if (texchain.iIndiceCount > 0)
									modcache->vTextureChain[WSURF_TEXCHAIN_STATIC].emplace_back(texchain);
							}
						}

						delete[]texchainSurface;
						delete[]texchainMapper;
						delete[]texchainArray;
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
						texchain.iIndiceCount = 0;
						texchain.iPolyCount = 0;
						texchain.iStartIndex = vIndicesBuffer.size();
						texchain.iType = TEXCHAIN_STATIC;

						for (; s; s = s->texturechain)
						{
							R_GenerateIndicesForTexChain(s, &texchain, modcache, vIndicesBuffer);
						}

						if (texchain.iIndiceCount > 0)
							modcache->vTextureChain[WSURF_TEXCHAIN_ANIM].emplace_back(texchain);
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
					texchain.iIndiceCount = 0;
					texchain.iPolyCount = 0;
					texchain.iStartIndex = vIndicesBuffer.size();
					texchain.iType = TEXCHAIN_STATIC;

					for (; s; s = s->texturechain)
					{
						R_GenerateIndicesForTexChain(s, &texchain, modcache, vIndicesBuffer);
					}

					if (texchain.iIndiceCount > 0)
						modcache->vTextureChain[WSURF_TEXCHAIN_STATIC].emplace_back(texchain);
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
				texchain.iIndiceCount = 0;
				texchain.iPolyCount = 0;
				texchain.iStartIndex = vIndicesBuffer.size();
				texchain.iType = TEXCHAIN_SCROLL;

				for (; s; s = s->texturechain)
				{
					R_GenerateIndicesForTexChain(s, &texchain, modcache, vIndicesBuffer);
				}

				if (texchain.iIndiceCount > 0)
					modcache->vTextureChain[WSURF_TEXCHAIN_STATIC].emplace_back(texchain);
			}
		}

		//End construction

		t->texturechain = NULL;
	}
}

void R_GenerateBufferStorage(model_t *mod, wsurf_vbo_t *modcache)
{
	std::vector<unsigned int> vIndicesBuffer;

	R_GenerateTexChain(mod, modcache, vIndicesBuffer);

	//Generate detailtexture flags and Sort texchains by detailtexture flags
	std::vector<GLuint64> ssbo;

	R_SortTextureChain(modcache, WSURF_TEXCHAIN_STATIC);
	R_SortTextureChain(modcache, WSURF_TEXCHAIN_ANIM);

	R_GenerateDrawBatch(modcache, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_STATIC, &ssbo);

	R_GenerateDrawBatch(modcache, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_SOLID, NULL);
	R_GenerateDrawBatch(modcache, WSURF_TEXCHAIN_ANIM, WSURF_DRAWBATCH_SOLID, NULL);

	modcache->hTextureSSBO = GL_GenBuffer();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, modcache->hTextureSSBO);	
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * (ssbo.size()), ssbo.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	modcache->hEBO = GL_GenBuffer();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modcache->hEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * vIndicesBuffer.size(), vIndicesBuffer.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	modcache->hEntityUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, modcache->hEntityUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(entity_ubo_t), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void R_GenerateVertexBuffer(void)
{
	brushvertex_t Vertexes[3];
	int iNumFaces = 0;
	int iNumVerts = 0;

	auto surf = r_worldmodel->surfaces;

	r_wsurf.vFaceBuffer.resize(r_worldmodel->numsurfaces);

	for(int i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		//if ((surf[i].flags & (SURF_UNDERWATER)))
		//	continue;

		auto poly = surf[i].polys;

		poly->flags = iNumFaces;

		brushface_t *brushface = &r_wsurf.vFaceBuffer[iNumFaces];

		VectorCopy(surf[i].texinfo->vecs[0], brushface->s_tangent);
		VectorCopy(surf[i].texinfo->vecs[1], brushface->t_tangent);
		VectorNormalize(brushface->s_tangent);
		VectorNormalize(brushface->t_tangent);
		VectorCopy(surf[i].plane->normal, brushface->normal);
		brushface->index = i;
		brushface->flags = surf[i].flags;

		if (surf[i].flags & SURF_PLANEBACK)
			VectorInverse(brushface->normal);

		if (surf[i].lightmaptexturenum + 1 > r_wsurf.iNumLightmapTextures)
			r_wsurf.iNumLightmapTextures = surf[i].lightmaptexturenum + 1;

		auto ptexture = surf[i].texinfo ? surf[i].texinfo->texture : NULL;

		float detailScale[2] = { 1, 1 };
		float normalScale[2] = { 1, 1 };
		float parallaxScale[2] = { 1, 1 };
		float specularScale[2] = { 1, 1 };

		if (ptexture)
		{
			auto pcache = R_FindDetailTextureCache(ptexture->gl_texturenum);
			if (pcache)
			{
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
		}

		if (brushface->flags & SURF_DRAWTURB)
		{
			for (poly = surf[i].polys; poly; poly = poly->next)
			{
				int iStartVert = iNumVerts;

				brushface->start_vertex.emplace_back(iStartVert);

				float *v = poly->verts[0];

				for (int j = 0; j < poly->numverts; j++, v += VERTEXSIZE)
				{
					Vertexes[0].pos[0] = v[0];
					Vertexes[0].pos[1] = v[1];
					Vertexes[0].pos[2] = v[2];
					Vertexes[0].texcoord[0] = v[3];
					Vertexes[0].texcoord[1] = v[4];
					Vertexes[0].texcoord[2] = (ptexture && (surf[i].flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;
					Vertexes[0].lightmaptexcoord[0] = v[5];
					Vertexes[0].lightmaptexcoord[1] = v[6];
					Vertexes[0].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
					Vertexes[0].detailtexcoord[0] = detailScale[0];
					Vertexes[0].detailtexcoord[1] = detailScale[1];
					Vertexes[0].normaltexcoord[0] = normalScale[0];
					Vertexes[0].normaltexcoord[1] = normalScale[1];
					Vertexes[0].parallaxtexcoord[0] = parallaxScale[0];
					Vertexes[0].parallaxtexcoord[1] = parallaxScale[1];
					Vertexes[0].speculartexcoord[0] = specularScale[0];
					Vertexes[0].speculartexcoord[1] = specularScale[1];
					Vertexes[0].normal[0] = brushface->normal[0];
					Vertexes[0].normal[1] = brushface->normal[1];
					Vertexes[0].normal[2] = brushface->normal[2];
					Vertexes[0].s_tangent[0] = brushface->s_tangent[0];
					Vertexes[0].s_tangent[1] = brushface->s_tangent[1];
					Vertexes[0].s_tangent[2] = brushface->s_tangent[2];
					Vertexes[0].t_tangent[0] = brushface->t_tangent[0];
					Vertexes[0].t_tangent[1] = brushface->t_tangent[1];
					Vertexes[0].t_tangent[2] = brushface->t_tangent[2];

					r_wsurf.vVertexBuffer.emplace_back(Vertexes[0]);
					iNumVerts++;
				}

				brushface->num_vertexes.emplace_back(iNumVerts - iStartVert);
				brushface->num_polys++;
			}
		}
		else
		{
			int iStartVert = iNumVerts;

			brushface->start_vertex.emplace_back(iStartVert);

			for (poly = surf[i].polys; poly; poly = poly->next)
			{
				float *v = poly->verts[0];

				for (int j = 0; j < 3; j++, v += VERTEXSIZE)
				{
					Vertexes[j].pos[0] = v[0];
					Vertexes[j].pos[1] = v[1];
					Vertexes[j].pos[2] = v[2];
					Vertexes[j].texcoord[0] = v[3];
					Vertexes[j].texcoord[1] = v[4];
					Vertexes[j].texcoord[2] = (ptexture && (surf[i].flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;
					Vertexes[j].lightmaptexcoord[0] = v[5];
					Vertexes[j].lightmaptexcoord[1] = v[6];
					Vertexes[j].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
					Vertexes[j].detailtexcoord[0] = detailScale[0];
					Vertexes[j].detailtexcoord[1] = detailScale[1];
					Vertexes[j].normaltexcoord[0] = normalScale[0];
					Vertexes[j].normaltexcoord[1] = normalScale[1];
					Vertexes[j].parallaxtexcoord[0] = parallaxScale[0];
					Vertexes[j].parallaxtexcoord[1] = parallaxScale[1];
					Vertexes[j].speculartexcoord[0] = specularScale[0];
					Vertexes[j].speculartexcoord[1] = specularScale[1];
					Vertexes[j].normal[0] = brushface->normal[0];
					Vertexes[j].normal[1] = brushface->normal[1];
					Vertexes[j].normal[2] = brushface->normal[2];
					Vertexes[j].s_tangent[0] = brushface->s_tangent[0];
					Vertexes[j].s_tangent[1] = brushface->s_tangent[1];
					Vertexes[j].s_tangent[2] = brushface->s_tangent[2];
					Vertexes[j].t_tangent[0] = brushface->t_tangent[0];
					Vertexes[j].t_tangent[1] = brushface->t_tangent[1];
					Vertexes[j].t_tangent[2] = brushface->t_tangent[2];
				}
				r_wsurf.vVertexBuffer.emplace_back(Vertexes[0]);
				r_wsurf.vVertexBuffer.emplace_back(Vertexes[1]);
				r_wsurf.vVertexBuffer.emplace_back(Vertexes[2]);
				iNumVerts += 3;

				for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
				{
					memcpy(&Vertexes[1], &Vertexes[2], sizeof(brushvertex_t));

					Vertexes[2].pos[0] = v[0];
					Vertexes[2].pos[1] = v[1];
					Vertexes[2].pos[2] = v[2];
					Vertexes[2].texcoord[0] = v[3];
					Vertexes[2].texcoord[1] = v[4];
					Vertexes[2].texcoord[2] = (ptexture && (surf[i].flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;
					Vertexes[2].lightmaptexcoord[0] = v[5];
					Vertexes[2].lightmaptexcoord[1] = v[6];
					Vertexes[2].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
					Vertexes[2].detailtexcoord[0] = detailScale[0];
					Vertexes[2].detailtexcoord[1] = detailScale[1];
					Vertexes[2].normaltexcoord[0] = normalScale[0];
					Vertexes[2].normaltexcoord[1] = normalScale[1];
					Vertexes[2].parallaxtexcoord[0] = parallaxScale[0];
					Vertexes[2].parallaxtexcoord[1] = parallaxScale[1];
					Vertexes[2].speculartexcoord[0] = specularScale[0];
					Vertexes[2].speculartexcoord[1] = specularScale[1];
					Vertexes[2].normal[0] = brushface->normal[0];
					Vertexes[2].normal[1] = brushface->normal[1];
					Vertexes[2].normal[2] = brushface->normal[2];
					Vertexes[2].s_tangent[0] = brushface->s_tangent[0];
					Vertexes[2].s_tangent[1] = brushface->s_tangent[1];
					Vertexes[2].s_tangent[2] = brushface->s_tangent[2];
					Vertexes[2].t_tangent[0] = brushface->t_tangent[0];
					Vertexes[2].t_tangent[1] = brushface->t_tangent[1];
					Vertexes[2].t_tangent[2] = brushface->t_tangent[2];
					r_wsurf.vVertexBuffer.emplace_back(Vertexes[0]);
					r_wsurf.vVertexBuffer.emplace_back(Vertexes[1]);
					r_wsurf.vVertexBuffer.emplace_back(Vertexes[2]);
					iNumVerts += 3;
				}
			}

			brushface->num_vertexes.emplace_back(iNumVerts - iStartVert);
			brushface->num_polys++;
		}

		iNumFaces++;
	}

	r_wsurf.hSceneVBO = GL_GenBuffer();
	glBindBuffer( GL_ARRAY_BUFFER, r_wsurf.hSceneVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof(brushvertex_t) * r_wsurf.vVertexBuffer.size(), r_wsurf.vVertexBuffer.data(), GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

void R_GenerateSceneUBO(void)
{
	if (r_wsurf.hSceneUBO)
		return;

	r_wsurf.hSceneUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, r_wsurf.hSceneUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(scene_ubo_t), NULL, GL_STREAM_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, r_wsurf.hSceneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//3.5 MBytes of VRAM
	r_wsurf.hDecalVBO = GL_GenBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, r_wsurf.hDecalVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(decalvertex_t) * MAX_DECALVERTS * MAX_DECALS, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	r_wsurf.hDecalTextureSSBO = GL_GenBuffer();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_wsurf.hDecalTextureSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * MAX_DECALS, NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, r_wsurf.hDecalTextureSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void R_ClearDecalCache(void)
{
	if(r_wsurf.vDecalGLTextures.size() < MAX_DECALS)
		r_wsurf.vDecalGLTextures.resize(MAX_DECALS);

	memset(r_wsurf.vDecalGLTextures.data(), 0, sizeof(int) * r_wsurf.vDecalGLTextures.size());

	if (r_wsurf.vDecalGLTextureHandles.size() < MAX_DECALS)
		r_wsurf.vDecalGLTextureHandles.resize(MAX_DECALS);

	memset(r_wsurf.vDecalGLTextureHandles.data(), 0, sizeof(GLuint64) * r_wsurf.vDecalGLTextureHandles.size());

	if (r_wsurf.vDecalStartIndex.size() < MAX_DECALS)
		r_wsurf.vDecalStartIndex.resize(MAX_DECALS);

	memset(r_wsurf.vDecalStartIndex.data(), 0, sizeof(GLint) * r_wsurf.vDecalStartIndex.size());

	if (r_wsurf.vDecalVertexCount.size() < MAX_DECALS)
		r_wsurf.vDecalVertexCount.resize(MAX_DECALS);

	memset(r_wsurf.vDecalVertexCount.data(), 0, sizeof(GLsizei) * r_wsurf.vDecalVertexCount.size());
}

void R_GenerateLightmapArray(void)
{
	r_wsurf.iLightmapTextureArray = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, BLOCK_WIDTH, BLOCK_HEIGHT, r_wsurf.iNumLightmapTextures, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	for (int i = 0; i < r_wsurf.iNumLightmapTextures; ++i)
	{
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, BLOCK_WIDTH, BLOCK_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + 0x10000 * i);
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

wsurf_vbo_t *R_PrepareWSurfVBO(model_t *mod)
{
	auto modelindex = EngineGetModelIndex(mod);

	if (modelindex == -1)
		return NULL;

	if (modelindex >= (int)g_WSurfVBOCache.size())
	{
		g_WSurfVBOCache.resize(modelindex + 1);
	}

	wsurf_vbo_t *modcache = g_WSurfVBOCache[modelindex];

	if (!modcache)
	{
		modcache = new wsurf_vbo_t;

		R_GenerateBufferStorage(mod, modcache);

		modcache->pModel = mod;

		g_WSurfVBOCache[modelindex] = modcache;
	}

	return modcache;
}

void R_EnableWSurfVBO(wsurf_vbo_t *modcache)
{
	if (r_wsurf.pCurrentModel == modcache)
		return;

	r_wsurf.pCurrentModel = modcache;

	if (modcache)
	{
		glBindBuffer(GL_ARRAY_BUFFER, r_wsurf.hSceneVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modcache->hEBO);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, modcache->hEntityUBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, modcache->hTextureSSBO);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glEnableVertexAttribArray(5);
		glEnableVertexAttribArray(6);
		glEnableVertexAttribArray(7);
		glEnableVertexAttribArray(8);
		glEnableVertexAttribArray(9);

		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
		glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, s_tangent));
		glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, t_tangent));
		glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		glVertexAttribPointer(5, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		glVertexAttribPointer(6, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		glVertexAttribPointer(7, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, normaltexcoord));
		glVertexAttribPointer(8, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, parallaxtexcoord));
		glVertexAttribPointer(9, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, speculartexcoord));

		glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
	else
	{
		glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4);
		glDisableVertexAttribArray(5);
		glDisableVertexAttribArray(6);
		glDisableVertexAttribArray(7);
		glDisableVertexAttribArray(8);
		glDisableVertexAttribArray(9);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);		
	}
}

void R_DrawWSurfVBOSolid(wsurf_vbo_t *modcache)
{
	auto &drawBatches = modcache->vDrawBatch[WSURF_DRAWBATCH_SOLID];

	for (size_t i = 0; i < drawBatches.size(); ++i)
	{
		auto &batch = drawBatches[i];

		glMultiDrawElements(GL_POLYGON, batch->vIndiceCount.data(), GL_UNSIGNED_INT, (const void **)batch->vStartIndex.data(), batch->iDrawCount);

		r_wsurf_drawcall++;
		r_wsurf_polys += batch->iPolyCount;
	}
}

void R_DrawWSurfVBOStatic(wsurf_vbo_t *modcache)
{
	if(!bNoBindless)
	{
		int WSurfProgramState = WSURF_BINDLESS_ENABLED;

		if (r_wsurf.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
		}

		if (r_wsurf.bLightmapTexture)
		{
			WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
		}

		if (r_wsurf.bShadowmapTexture)
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

		if (r_draw_pass == r_draw_reflect && curwater)
		{
			WSurfProgramState |= WSURF_CLIP_ENABLED;
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

		if ((*currententity)->curstate.renderfx == kRenderFxShadowCaster)
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		auto &drawBatches = modcache->vDrawBatch[WSURF_DRAWBATCH_STATIC];
		for (size_t i = 0; i < drawBatches.size(); ++i)
		{
			auto &batch = drawBatches[i];

			int WSurfProgramStateBatch = WSurfProgramState;

			if (batch->iDetailTextureFlags & (1 << WSURF_DETAIL_TEXTURE))
			{
				WSurfProgramStateBatch |= WSURF_DETAILTEXTURE_ENABLED;
			}

			if (batch->iDetailTextureFlags & (1 << WSURF_NORMAL_TEXTURE))
			{
				WSurfProgramStateBatch |= WSURF_NORMALTEXTURE_ENABLED;
			}

			if (batch->iDetailTextureFlags & (1 << WSURF_PARALLAX_TEXTURE))
			{
				WSurfProgramStateBatch |= WSURF_PARALLAXTEXTURE_ENABLED;
			}

			if (batch->iDetailTextureFlags & (1 << WSURF_SPECULAR_TEXTURE))
			{
				WSurfProgramStateBatch |= WSURF_SPECULARTEXTURE_ENABLED;
			}

			wsurf_program_t prog = { 0 };
			R_UseWSurfProgram(WSurfProgramStateBatch, &prog);

			glMultiDrawElements(GL_POLYGON, batch->vIndiceCount.data(), GL_UNSIGNED_INT, (const void **)batch->vStartIndex.data(), batch->iDrawCount);

			r_wsurf_drawcall++;
			r_wsurf_polys += batch->iPolyCount;
		}
	}
	else 
	{
		for (size_t i = 0; i < modcache->vTextureChain[WSURF_TEXCHAIN_STATIC].size(); ++i)
		{
			auto &texchain = modcache->vTextureChain[WSURF_TEXCHAIN_STATIC][i];

			auto base = texchain.pTexture;

			if (r_wsurf.bDiffuseTexture)
			{
				GL_Bind(base->gl_texturenum);
				R_BeginDetailTexture(base->gl_texturenum);
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

			if (r_wsurf.bShadowmapTexture)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

				for (int j = 0; j < 3; ++j)
				{
					if (shadow_numvisedicts[j] > 0)
					{
						WSurfProgramState |= (WSURF_SHADOWMAP_HIGH_ENABLED << j);
					}
				}
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

			if (r_wsurf.bSpecularTexture)
			{
				WSurfProgramState |= WSURF_SPECULARTEXTURE_ENABLED;
			}

			if (r_draw_pass == r_draw_reflect && curwater)
			{
				WSurfProgramState |= WSURF_CLIP_ENABLED;
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

			if ((*currententity)->curstate.renderfx == kRenderFxShadowCaster)
			{
				WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
			}

			wsurf_program_t prog = { 0 };
			R_UseWSurfProgram(WSurfProgramState, &prog);

			glDrawElements(GL_POLYGON, texchain.iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			R_EndDetailTexture();

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iPolyCount;
		}
	}
}

void R_DrawWSurfVBOAnim(wsurf_vbo_t *modcache)
{
	for (size_t i = 0; i < modcache->vTextureChain[WSURF_TEXCHAIN_ANIM].size(); ++i)
	{
		auto &texchain = modcache->vTextureChain[WSURF_TEXCHAIN_ANIM][i];

		auto base = texchain.pTexture;

		if ((*currententity)->curstate.effects & EF_SNIPERLASER)
		{
			int frame_count = 0;
			int total_frame = (*currententity)->curstate.frame;
			do
			{
				if (base->anim_next)
					base = base->anim_next;
				++frame_count;
			} while (frame_count < (*currententity)->curstate.frame);
		}
		else
		{
			if ((*currententity)->curstate.frame && base->alternate_anims)
				base = base->alternate_anims;

			if (!((*currententity)->curstate.effects & EF_NIGHTVISION))
			{
				int reletive = (int)((*cl_time) * 10.0f) % base->anim_total;

				int loop_count = 0;

				while (base->anim_min > reletive || base->anim_max <= reletive)
				{
					base = base->anim_next;

					if (!base)
						Sys_ErrorEx("R_TextureAnimation: broken cycle");

					if (++loop_count > 100)
						Sys_ErrorEx("R_TextureAnimation: infinite cycle");
				}
			}
		}

		if (r_wsurf.bDiffuseTexture)
		{
			GL_Bind(base->gl_texturenum);
			R_BeginDetailTexture(base->gl_texturenum);
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

		if (r_wsurf.bShadowmapTexture)
		{
			WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

			for (int j = 0; j < 3; ++j)
			{
				if (shadow_numvisedicts[j] > 0)
				{
					WSurfProgramState |= (WSURF_SHADOWMAP_HIGH_ENABLED << j);
				}
			}
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

		if (r_wsurf.bSpecularTexture)
		{
			WSurfProgramState |= WSURF_SPECULARTEXTURE_ENABLED;
		}

		if (r_draw_pass == r_draw_reflect && curwater)
		{
			WSurfProgramState |= WSURF_CLIP_ENABLED;
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

		if ((*currententity)->curstate.renderfx == kRenderFxShadowCaster)
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		wsurf_program_t prog = { 0 };
		R_UseWSurfProgram(WSurfProgramState, &prog);

		glDrawElements(GL_POLYGON, texchain.iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		R_EndDetailTexture();

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.iPolyCount;
	}
}

float R_ScrollSpeed(void)
{
	float scrollSpeed = ((*currententity)->curstate.rendercolor.b + ((*currententity)->curstate.rendercolor.g << 8)) / 16.0;

	if ((*currententity)->curstate.rendercolor.r == 0)
		scrollSpeed = -scrollSpeed;

	scrollSpeed *= (*cl_time);

	return scrollSpeed;
}

void R_DrawWSurfVBO(wsurf_vbo_t *modcache, cl_entity_t *ent)
{
	entity_ubo_t EntityUBO;

	memcpy(EntityUBO.entityMatrix, r_entity_matrix, sizeof(mat4));
	memcpy(EntityUBO.color, r_entity_color, sizeof(vec4));
	EntityUBO.scrollSpeed = R_ScrollSpeed();

	glNamedBufferSubData(modcache->hEntityUBO, 0, sizeof(EntityUBO), &EntityUBO);

	//This only applies to world rendering
	if(modcache->pModel == r_worldmodel && r_wsurf_sky_occlusion->value)
	{
		//Sky surface uses stencil = 1
		glStencilFunc(GL_ALWAYS, 1, 0xFF);

		glColorMask(0, 0, 0, 0);

		auto &texchain = modcache->TextureChainSky;

		int WSurfProgramState = 0;

		if (!drawgbuffer && r_fog_mode == GL_LINEAR)
		{
			WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
		}

		if (drawgbuffer)
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		}

		wsurf_program_t prog = { 0 };
		R_UseWSurfProgram(WSurfProgramState, &prog);

		glDrawElements(GL_POLYGON, texchain.iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		r_wsurf_polys += texchain.iPolyCount;
		r_wsurf_drawcall++;

		glColorMask(1, 1, 1, 1);
	}

	//World alway use stencil = 0
	glStencilFunc(GL_ALWAYS, 0, 0xFF);

	if (r_wsurf.bShadowmapTexture)
	{
		glActiveTexture(GL_TEXTURE6);

		glEnable(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_texture_color);

		glActiveTexture(GL_TEXTURE0);
	}

	if (r_wsurf.bLightmapTexture)
	{
		glActiveTexture(GL_TEXTURE1);

		glEnable(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);

		glActiveTexture(GL_TEXTURE0);
	}

	//Static texchain

	R_DrawWSurfVBOStatic(modcache);

	R_DrawWSurfVBOAnim(modcache);

	//Prepare to collect decals
	(*gDecalSurfCount) = 0;

	//No draw, collect waterchain and decals
	if (modcache->pModel == r_worldmodel)
	{
		R_RecursiveWorldNodeVBO(r_worldmodel->nodes);

		(*currententity) = gEngfuncs.GetEntityByIndex(0);

		if ((*waterchain))
		{
			for (auto s = (*waterchain); s; s = s->texturechain)
			{
				EmitWaterPolys(s, 0);
			}
			(*waterchain) = 0;
		}
	}
	else
	{
		auto clmodel = modcache->pModel;
		auto psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
		for (int i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			auto pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				if (pplane->type != PLANE_Z)
					continue;

				if (r_entity_mins[2] + 1.0 >= pplane->dist)
					continue;
			}

			auto dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

			if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
			{
				if (psurf->flags & SURF_DRAWTURB)
				{
					EmitWaterPolys(psurf, 0);
				}
				else
				{
					//No draw, collect decals
					R_DrawSequentialPolyVBO(psurf);
				}
			}
			else
			{
				if (psurf->flags & SURF_DRAWTURB)
				{
					EmitWaterPolys(psurf, 1);
				}
			}
		}
	}

	//World, Entity paints decals here
	
	R_DrawDecals();

	R_DrawWaters();

	if (r_wsurf.bShadowmapTexture)
	{
		glActiveTexture(GL_TEXTURE6);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glDisable(GL_TEXTURE_2D_ARRAY);

		glActiveTexture(GL_TEXTURE0);
	}

	if (r_wsurf.bLightmapTexture)
	{
		glActiveTexture(GL_TEXTURE1);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glDisable(GL_TEXTURE_2D_ARRAY);

		glActiveTexture(GL_TEXTURE0);
	}

	//This only applies to world rendering, clear depth for sky surface
	if (modcache->pModel == r_worldmodel && r_wsurf_sky_occlusion->value)
	{
		//Overwrite sky surface (stencil = 1) with initial depth

		GL_Begin2D();

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);
		glStencilMask(1);
		glColorMask(0, 0, 0, 0);
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		GL_UseProgram(depth_clear.program);
		R_DrawHUDQuad(glwidth, glheight);		

		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilMask(0xFF);
		glColorMask(1, 1, 1, 1);
		glDepthFunc(GL_LEQUAL);

		GL_End2D();
	}

	GL_UseProgram(0);
}

void R_Reload_f(void)
{
	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities, NULL);
	R_LoadExternalEntities();
	R_LoadBSPEntities();

	gEngfuncs.Con_Printf("Entities reloaded\n");
}

void R_InitWSurf(void)
{
	r_wsurf.hSceneVBO = 0;
	r_wsurf.hSceneUBO = 0;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bDetailTexture = false;
	r_wsurf.bNormalTexture = false;
	r_wsurf.bParallaxTexture = false;
	r_wsurf.pCurrentModel = NULL;
	r_wsurf.iNumLightmapTextures = 0;
	r_wsurf.iLightmapTextureArray = 0;

	R_ClearBSPEntities();
	
	r_wsurf_parallax_scale = gEngfuncs.pfnRegisterVariable("r_wsurf_parallax_scale", "-0.02", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_sky_occlusion = gEngfuncs.pfnRegisterVariable("r_wsurf_sky_occlusion", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_detail = gEngfuncs.pfnRegisterVariable("r_wsurf_detail", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}

void R_ShutdownWSurf(void)
{
	g_WSurfProgramTable.clear();

	R_FreeLightmapArray();
	R_FreeVertexBuffer();
	R_ClearWSurfVBOCache();
	R_ClearBSPEntities();

	R_FreeSceneUBO();
}

void R_LoadDetailTextures(void)
{
	std::string name = gEngfuncs.pfnGetLevelName();
	name = name.substr(0, name.length() - 4);
	name += "_detail.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_DPrintf("R_LoadDetailTextures: No detail texture file %s\n", name.c_str());
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

		//Default: load as detail texture
		int texType = WSURF_DETAIL_TEXTURE;

		std::string base = basetexture;

		if (base.length() > (sizeof("_PARALLAX") - 1) && !strcmp(&base[base.length() - (sizeof("_PARALLAX") - 1)], "_PARALLAX"))
		{
			base = base.substr(0, base.length() - (sizeof("_PARALLAX") - 1));
			texType = WSURF_PARALLAX_TEXTURE;
		}
		else if (base.length() > (sizeof("_NORMAL") - 1) && !strcmp(&base[base.length() - (sizeof("_NORMAL") - 1)], "_NORMAL"))
		{
			base = base.substr(0, base.length() - (sizeof("_NORMAL") - 1));
			texType = WSURF_NORMAL_TEXTURE;
		}
		else if (base.length() > (sizeof("_REPLACE") - 1) && !strcmp(&base[base.length() - (sizeof("_REPLACE") - 1)], "_REPLACE"))
		{
			base = base.substr(0, base.length() - (sizeof("_REPLACE") - 1));
			texType = WSURF_REPLACE_TEXTURE;
		}
		else if (base.length() > (sizeof("_DETAIL") - 1) && !strcmp(&base[base.length() - (sizeof("_DETAIL") - 1)], "_DETAIL"))
		{
			base = base.substr(0, base.length() - (sizeof("_DETAIL") - 1));
			texType = WSURF_DETAIL_TEXTURE;
		}
		else if (base.length() > (sizeof("_SPECULAR") - 1) && !strcmp(&base[base.length() - (sizeof("_SPECULAR") - 1)], "_SPECULAR"))
		{
			base = base.substr(0, base.length() - (sizeof("_SPECULAR") - 1));
			texType = WSURF_SPECULAR_TEXTURE;
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

		const char *textypeNames[] = {
			"WSURF_REPLACE_TEXTURE",
			"WSURF_DETAIL_TEXTURE",
			"WSURF_NORMAL_TEXTURE",
			"WSURF_PARALLAX_TEXTURE",
			"WSURF_SPECULAR_TEXTURE",
		};

		if (cache)
		{
			if (cache->tex[texType].gltexturenum)
			{
				gEngfuncs.Con_Printf("R_LoadDetailTextures: %s already exists for basetexture %s\n", textypeNames[texType], base.c_str());
				continue;
			}

			int width = 0, height = 0;

			std::string texturePath = "gfx/";
			texturePath += detailtexture;
			if (!V_GetFileExtension(detailtexture))
				texturePath += ".tga";

			int texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, true, true);
			if (!texId)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, true, true);
			}

			if (!texId)
			{
				gEngfuncs.Con_Printf("R_LoadDetailTextures: Failed to load %s as %s for basetexture %s\n", detailtexture, textypeNames[texType], base.c_str());
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

void R_NewMapWSurf(void)
{
	R_GenerateSceneUBO();

	R_ClearDecalCache();

	for (auto p : g_DetailTextureTable)
		delete p.second;

	g_DetailTextureTable.clear();
	R_LoadDetailTextures();

	R_FreeLightmapArray();
	R_FreeVertexBuffer();

	R_GenerateVertexBuffer();
	R_GenerateLightmapArray();

	R_ClearWSurfVBOCache();
	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities, NULL);
	R_LoadExternalEntities();
	R_LoadBSPEntities();
}

void R_DrawWireFrame(brushface_t *brushface, void(*draw)(brushface_t *face))
{

}

detail_texture_cache_t *R_FindDetailTextureCache(int texId)
{
	auto itor = g_DetailTextureTable.find(texId);

	if (itor != g_DetailTextureTable.end())
	{
		auto cache = itor->second;

		if (cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum ||
			cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum ||
			cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum ||
			cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum
			)
		{
			return cache;
		}
	}

	return NULL;
}

void R_BeginDetailTexture(int texId)
{
	if (!r_wsurf_detail->value)
		return;

	auto itor = g_DetailTextureTable.find(texId);

	if (itor != g_DetailTextureTable.end())
	{
		auto cache = itor->second;

		if (cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum)
		{
			GL_Bind(cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum);
		}

		if (cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum)
		{
			glActiveTexture(GL_TEXTURE2);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum);

			r_wsurf.bDetailTexture = true;
		}

		if (cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum)
		{
			glActiveTexture(GL_TEXTURE3);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum);

			r_wsurf.bNormalTexture = true;
		}

		if (cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum)
		{
			glActiveTexture(GL_TEXTURE4_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum);

			r_wsurf.bParallaxTexture = true;
		}

		if (cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum)
		{
			glActiveTexture(GL_TEXTURE5_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum);

			r_wsurf.bSpecularTexture = true;
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

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bNormalTexture)
	{
		r_wsurf.bNormalTexture = false;
		bRestore = true;

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bParallaxTexture)
	{
		r_wsurf.bParallaxTexture = false;
		bRestore = true;

		glActiveTexture(GL_TEXTURE4_ARB);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bSpecularTexture)
	{
		r_wsurf.bSpecularTexture = false;
		bRestore = true;

		glActiveTexture(GL_TEXTURE5_ARB);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (bRestore)
	{
		glActiveTexture(*oldtarget);
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

void FreeBSPEntity(bspentity_t *ent)
{
	epair_t *pPair = ent->epairs;
	while (pPair)
	{
		epair_t *pFree = pPair;
		pPair = pFree->next;

		delete[] pFree->key;
		delete[] pFree->value;
		delete pFree;
	}
	ent->epairs = NULL;
	ent->classname = NULL;
	VectorClear(ent->origin);
}

void R_ClearBSPEntities(void)
{
	for(size_t i = 0; i < r_wsurf.vBSPEntities.size(); i++)
	{
		FreeBSPEntity(&r_wsurf.vBSPEntities[i]);
	}
	r_wsurf.vBSPEntities.clear();
	r_water_controls.clear();
	g_DynamicLights.clear();
}

static fnParseBSPEntity_Allocator current_parse_allocator = NULL;
static bspentity_t *current_parse_entity = NULL;
static char com_token[4096];

bspentity_t *R_ParseBSPEntity_DefaultAllocator(void)
{
	size_t len = r_wsurf.vBSPEntities.size();

	r_wsurf.vBSPEntities.resize(len + 1);

	return &r_wsurf.vBSPEntities[len];
}

static bool R_ParseBSPEntityKeyValue(const char *classname, const char *keyname, const char *value)
{
	if (classname == NULL)
	{
		current_parse_entity = current_parse_allocator();
		if(!current_parse_entity)
			return false;

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

static bool R_ParseBSPEntityClassname(char *szInputStream, char *classname)
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

			strncpy(classname, com_token, 255);
			classname[255] = 0;

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

static char *R_ParseBSPEntity(char *data)
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

void R_ParseBSPEntities(char *data, fnParseBSPEntity_Allocator allocator)
{

	if (allocator)
		current_parse_allocator = allocator; 
	else
		current_parse_allocator = R_ParseBSPEntity_DefaultAllocator;

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
	current_parse_allocator = NULL;
}

void R_LoadExternalEntities(void)
{
	std::string name;
	
	name = gEngfuncs.pfnGetLevelName();
	name = name.substr(0, name.length() - 4);
	name += "_entity.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_Printf("R_LoadExternalEntities: No external entity file %s\n", name.c_str());

		name = "renderer/default_entity.txt";

		pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
		if (!pfile)
		{
			gEngfuncs.Con_Printf("R_LoadExternalEntities: No default external entity file %s\n", name.c_str());

			return;
		}
	}
	R_ParseBSPEntities(pfile, NULL);

	gEngfuncs.COM_FreeFile(pfile);
}

#if 0
void R_ParseBSPEntity_Env_Cubemap(bspentity_t *ent)
{
	float temp[4];

	cubemap_t cubemap;
	cubemap.cubetex = 0;
	cubemap.size = 0;
	cubemap.radius = 0;
	cubemap.origin[0] = 0;
	cubemap.origin[1] = 0;
	cubemap.origin[2] = 0;
	cubemap.extension = "tga";

	char *name_string = ValueForKey(ent, "name");
	if (name_string)
	{
		cubemap.name = name_string;
	}

	char *origin_string = ValueForKey(ent, "origin");
	if (origin_string)
	{
		if (sscanf(origin_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			cubemap.origin[0] = temp[0];
			cubemap.origin[1] = temp[1];
			cubemap.origin[2] = temp[2];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"origin\" in entity \"env_cubemap\"\n");
		}
	}

	char *cubemapsize_string = ValueForKey(ent, "cubemapsize");
	if (cubemapsize_string)
	{
		int size = 0;
		if (sscanf(cubemapsize_string, "%d", &size) == 1)
		{
			cubemap.size = size;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"cubemapsize\" in entity \"env_cubemap\"\n");
		}
	}

	char *radius_string = ValueForKey(ent, "radius");
	if (radius_string)
	{
		if (sscanf(radius_string, "%f", &temp[0]) == 1 && temp[0] > 0)
		{
			cubemap.radius = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"radius\" in entity \"env_cubemap\"\n");
		}
	}

	char *extension_string = ValueForKey(ent, "extension");
	if (extension_string)
	{
		cubemap.extension = extension_string;
	}

	if (cubemap.name.length() > 0 && cubemap.radius > 0)
	{
		R_LoadCubemap(&cubemap);

		r_cubemaps.emplace_back(cubemap);
	}
}
#endif

void R_ParseBSPEntity_Light_Dynamic(bspentity_t *ent)
{
	float temp[4];

	light_dynamic_t dynlight;
	dynlight.type = DLIGHT_POINT;
	VectorClear(dynlight.origin);
	VectorClear(dynlight.color);
	dynlight.distance = 0;
	dynlight.ambient = 0;
	dynlight.diffuse = 0;
	dynlight.specular = 0;
	dynlight.specularpow = 0;

	char *origin_string = ValueForKey(ent, "origin");
	if (origin_string)
	{
		if (sscanf(origin_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			dynlight.origin[0] = temp[0];
			dynlight.origin[1] = temp[1];
			dynlight.origin[2] = temp[2];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"origin\" in entity \"light_dynamic\"\n");
		}
	}

	char *color_string = ValueForKey(ent, "_light");
	if (color_string)
	{
		if (sscanf(color_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			dynlight.color[0] = clamp(temp[0], 0, 255) / 255.0f;
			dynlight.color[1] = clamp(temp[1], 0, 255) / 255.0f;
			dynlight.color[2] = clamp(temp[2], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_light\" in entity \"light_dynamic\"\n");
		}
	}

	char *distance_string = ValueForKey(ent, "_distance");
	if (distance_string)
	{
		if (sscanf(distance_string, "%f", &temp[0]) == 1)
		{
			dynlight.distance = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_distance\" in entity \"light_dynamic\"\n");
		}
	}

	char *ambient_string = ValueForKey(ent, "_ambient");
	if (ambient_string)
	{
		if (sscanf(ambient_string, "%f", &temp[0]) == 1)
		{
			dynlight.ambient = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_ambient\" in entity \"light_dynamic\"\n");
		}
	}

	char *diffuse_string = ValueForKey(ent, "_diffuse");
	if (diffuse_string)
	{
		if (sscanf(diffuse_string, "%f", &temp[0]) == 1)
		{
			dynlight.diffuse = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_diffuse\" in entity \"light_dynamic\"\n");
		}
	}

	char *specular_string = ValueForKey(ent, "_specular");
	if (specular_string)
	{
		if (sscanf(specular_string, "%f", &temp[0]) == 1)
		{
			dynlight.specular = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_specular\" in entity \"light_dynamic\"\n");
		}
	}

	char *specularpow_string = ValueForKey(ent, "_specularpow");
	if (specularpow_string)
	{
		if (sscanf(specularpow_string, "%f", &temp[0]) == 1)
		{
			dynlight.specularpow = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_specularpow\" in entity \"light_dynamic\"\n");
		}
	}

	g_DynamicLights.emplace_back(dynlight);
}

void R_ParseBSPEntity_Env_Water_Control(bspentity_t *ent)
{
	float temp[4];

	water_control_t control;
	control.fresnelfactor = 0;
	control.depthfactor[0] = 0;
	control.depthfactor[1] = 0;
	control.normfactor = 0;
	control.minheight = 0;
	control.maxtrans = 0;
	control.level = WATER_LEVEL_REFLECT_SKYBOX;

	char *basetexture_string = ValueForKey(ent, "basetexture");
	if (basetexture_string)
	{
		control.basetexture = basetexture_string;
		if (control.basetexture[control.basetexture.length() - 1] == '*')
		{
			control.wildcard = control.basetexture.substr(0, control.basetexture.length() - 1);
		}
	}

	char *normalmap_string = ValueForKey(ent, "normalmap");
	if (normalmap_string)
	{
		control.normalmap = normalmap_string;
	}

	char *fresnelfactor_string = ValueForKey(ent, "fresnelfactor");
	if (fresnelfactor_string)
	{
		if (sscanf(fresnelfactor_string, "%f", &temp[0]) == 1)
		{
			control.fresnelfactor = clamp(temp[0], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"fresnelfactor\" in entity \"env_water_control\"\n");
		}
	}

	char *normfactor_string = ValueForKey(ent, "normfactor");
	if (normfactor_string)
	{
		if (sscanf(normfactor_string, "%f", &temp[0]) == 1)
		{
			control.normfactor = clamp(temp[0], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"normfactor\" in entity \"env_water_control\"\n");
		}
	}

	char *depthfactor_string = ValueForKey(ent, "depthfactor");
	if (depthfactor_string)
	{
		if (sscanf(depthfactor_string, "%f %f", &temp[0], &temp[1]) == 2)
		{
			control.depthfactor[0] = clamp(temp[0], 0, 10);
			control.depthfactor[1] = clamp(temp[1], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"depthfactor\" in entity \"env_water_control\"\n");
		}
	}

	char *minheight_string = ValueForKey(ent, "minheight");
	if (minheight_string)
	{
		if (sscanf(minheight_string, "%f", &temp[0]) == 1)
		{
			control.minheight = clamp(temp[0], 0, 10000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"minheight\" in entity \"env_water_control\"\n");
		}
	}

	char *maxtrans_string = ValueForKey(ent, "maxtrans");
	if (maxtrans_string)
	{
		if (sscanf(maxtrans_string, "%f", &temp[0]) == 1)
		{
			control.maxtrans = clamp(temp[0], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"maxtrans\" in entity \"env_water_control\"\n");
		}
	}

	char *level_string = ValueForKey(ent, "level");
	if (level_string)
	{
		if (!strcmp(level_string, "WATER_LEVEL_LEGACY"))
		{
			control.level = WATER_LEVEL_LEGACY;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_SKYBOX"))
		{
			control.level = WATER_LEVEL_REFLECT_SKYBOX;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_WORLD"))
		{
			control.level = WATER_LEVEL_REFLECT_WORLD;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_ENTITY"))
		{
			control.level = WATER_LEVEL_REFLECT_ENTITY;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_SSR"))
		{
			control.level = WATER_LEVEL_REFLECT_SSR;
		}
		else
		{
			int lv;
			if (sscanf(level_string, "%d", &lv) == 1)
			{
				control.level = clamp(lv, WATER_LEVEL_LEGACY, WATER_LEVEL_REFLECT_SSR);
			}
			else
			{
				gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"level\" in entity \"env_water_control\"\n");
			}
		}
	}

	if (control.basetexture.length())
	{
		r_water_controls.emplace_back(control);
	}
}

void R_ParseBSPEntity_Env_HDR_Control(bspentity_t *ent)
{
	float temp[4];

	char *adaptation_string = ValueForKey(ent, "adaptation");
	if (adaptation_string)
	{
		if (sscanf(adaptation_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.adaptation = clamp(temp[0], 0.1, 100);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"adaptation\" in entity \"env_hdr_control\"\n");
		}
	}
	char *blurwidth_string = ValueForKey(ent, "blurwidth");
	if (blurwidth_string)
	{
		if (sscanf(blurwidth_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.blurwidth = clamp(temp[0], 0, 1);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"blurwidth\" in entity \"env_hdr_control\"\n");
		}
	}
	char *darkness_string = ValueForKey(ent, "darkness");
	if (darkness_string)
	{
		if (sscanf(darkness_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.darkness = clamp(temp[0], 0.01, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"darkness\" in entity \"env_hdr_control\"\n");
		}
	}
	char *exposure_string = ValueForKey(ent, "exposure");
	if (exposure_string)
	{
		if (sscanf(exposure_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.exposure = clamp(temp[0], 0.01, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"exposure\" in entity \"env_hdr_control\"\n");
		}
	}
	char *disablehdr_string = ValueForKey(ent, "disablehdr");
	if (disablehdr_string)
	{
		if (atoi(disablehdr_string) > 0)
		{
			r_hdr_control.enabled = false;
		}
	}
}

void R_ParseBSPEntity_Env_Shadow_Control(bspentity_t *ent)
{
	float temp[4];

	char *color_string = ValueForKey(ent, "color");
	if (color_string)
	{
		if (sscanf(color_string, "%f %f %f %f", &temp[0], &temp[1], &temp[2], &temp[3]) == 4)
		{
			r_shadow_control.color[0] = clamp(temp[0], 0, 255) / 255.0f;
			r_shadow_control.color[1] = clamp(temp[1], 0, 255) / 255.0f;
			r_shadow_control.color[2] = clamp(temp[2], 0, 255) / 255.0f;
			r_shadow_control.color[3] = clamp(temp[3], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"color\" in entity \"env_shadow_control\"\n");
		}
	}

	char *angles_string = ValueForKey(ent, "angles");
	if (angles_string)
	{
		if (sscanf(angles_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			r_shadow_control.angles[0] = clamp(temp[0], -360, 360);
			r_shadow_control.angles[1] = clamp(temp[1], -360, 360);
			r_shadow_control.angles[2] = clamp(temp[2], -360, 360);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"angles\" in entity \"env_shadow_control\"\n");
		}
	}

	char *distfade_string = ValueForKey(ent, "distfade");
	if (distfade_string)
	{
		if (sscanf(distfade_string, "%f %f", &temp[0], &temp[1]) == 2)
		{
			r_shadow_control.distfade[0] = clamp(temp[0], 0, 10000);
			r_shadow_control.distfade[1] = clamp(temp[1], r_shadow_control.distfade[0], 10000) - r_shadow_control.distfade[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"distfade\" in entity \"env_shadow_control\"\n");
		}
	}

	char *lumfade_string = ValueForKey(ent, "lumfade");
	if (lumfade_string)
	{
		if (sscanf(lumfade_string, "%f %f", &temp[0], &temp[1]) == 2)
		{
			r_shadow_control.lumfade[0] = clamp(temp[0], 0, 255) / 255.0f;
			r_shadow_control.lumfade[1] = clamp(temp[1], 0, 255) / 255.0f;

			if (r_shadow_control.lumfade[0] < r_shadow_control.lumfade[1] + 0.01f)
				r_shadow_control.lumfade[0] = r_shadow_control.lumfade[1] + 0.01f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"lumfade\" in entity \"env_shadow_control\"\n");
		}
	}

	char *disableallshadows_string = ValueForKey(ent, "disableallshadows");
	if (disableallshadows_string)
	{
		if (atoi(disableallshadows_string) > 0)
		{
			r_shadow_control.enabled = false;
		}
	}

	char *high_distance_string = ValueForKey(ent, "high_distance");
	if (high_distance_string)
	{
		if (sscanf(high_distance_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[0][0] = clamp(temp[0], 0, 100000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"high_distance\" in entity \"env_shadow_control\"\n");
		}
	}

	char *high_scale_string = ValueForKey(ent, "high_scale");
	if (high_scale_string)
	{
		if (sscanf(high_scale_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[0][1] = clamp(temp[0], 0.1, 8);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"high_scale\" in entity \"env_shadow_control\"\n");
		}
	}

	char *medium_distance_string = ValueForKey(ent, "medium_distance");
	if (medium_distance_string)
	{
		if (sscanf(medium_distance_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[1][0] = clamp(temp[0], 0, 100000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"medium_distance\" in entity \"env_shadow_control\"\n");
		}
	}

	char *medium_scale_string = ValueForKey(ent, "medium_scale");
	if (medium_scale_string)
	{
		if (sscanf(medium_scale_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[1][1] = clamp(temp[0], 0.1, 8);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"medium_scale\" in entity \"env_shadow_control\"\n");
		}
	}

	char *low_distance_string = ValueForKey(ent, "low_distance");
	if (low_distance_string)
	{
		if (sscanf(low_distance_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[2][0] = clamp(temp[0], 0, 100000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"low_distance\" in entity \"env_shadow_control\"\n");
		}
	}

	char *low_scale_string = ValueForKey(ent, "low_scale");
	if (low_scale_string)
	{
		if (sscanf(low_scale_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[2][1] = clamp(temp[0], 0.1, 8);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"low_scale\" in entity \"env_shadow_control\"\n");
		}
	}
}

void R_ParseBSPEntity_Env_SSR_Control(bspentity_t *ent)
{
	float temp[4];

	char *ray_step_string = ValueForKey(ent, "ray_step");
	if (ray_step_string)
	{
		if (sscanf(ray_step_string, "%f", &temp[0]) == 1)
		{
			r_ssr_control.ray_step = clamp(temp[0], 0.01, 10.0);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"ray_step\" in entity \"env_ssr_control\"\n");
		}
	}

	char *iter_count_string = ValueForKey(ent, "iter_count");
	if (iter_count_string)
	{
		int temp1 = 0;
		if (sscanf(iter_count_string, "%d", &temp1) == 1)
		{
			r_ssr_control.iter_count = clamp(temp1, 10, 300);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"iter_count\" in entity \"env_ssr_control\"\n");
		}
	}

	char *distance_bias_string = ValueForKey(ent, "distance_bias");
	if (distance_bias_string)
	{
		if (sscanf(distance_bias_string, "%f", &temp[0]) == 1)
		{
			r_ssr_control.distance_bias = clamp(temp[0], 0.01, 10.0);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"distance_bias\" in entity \"env_ssr_control\"\n");
		}
	}

	char *exponential_step_string = ValueForKey(ent, "exponential_step");
	if (exponential_step_string)
	{
		if (atoi(exponential_step_string) > 0)
		{
			r_ssr_control.exponential_step = true;
		}
		else
		{
			r_ssr_control.exponential_step = false;
		}
	}

	char *adaptive_step_string = ValueForKey(ent, "adaptive_step");
	if (adaptive_step_string)
	{
		if (atoi(adaptive_step_string) > 0)
		{
			r_ssr_control.adaptive_step = true;
		}
		else
		{
			r_ssr_control.adaptive_step = false;
		}
	}

	char *binary_search_string = ValueForKey(ent, "binary_search");
	if (binary_search_string)
	{
		if (atoi(binary_search_string) > 0)
		{
			r_ssr_control.binary_search = true;
		}
		else
		{
			r_ssr_control.binary_search = false;
		}
	}

	char *fade_string = ValueForKey(ent, "fade");
	if (fade_string)
	{
		if (sscanf(fade_string, "%f %f", &temp[0], &temp[1]) == 2)
		{
			r_ssr_control.fade[0] = clamp(temp[0], 0, 10);
			r_ssr_control.fade[1] = clamp(temp[1], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"fade\" in entity \"env_ssr_control\"\n");
		}
	}

	char *disablessr_string = ValueForKey(ent, "disablessr");
	if (disablessr_string)
	{
		if (atoi(disablessr_string) > 0)
		{
			r_ssr_control.enabled = false;
		}
	}
}

void R_LoadShadowControl(void)
{
	//Initialize shadow control, enabled by default

	r_shadow_control.enabled = true;

	float temp[4];
	if (sscanf(r_shadow_color->string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
	{
		r_shadow_control.color[0] = clamp(temp[0], 0, 255) / 255.0f;
		r_shadow_control.color[1] = clamp(temp[1], 0, 255) / 255.0f;
		r_shadow_control.color[2] = clamp(temp[2], 0, 255) / 255.0f;
	}
	else
	{
		r_shadow_control.color[0] = 0;
		r_shadow_control.color[1] = 0;
		r_shadow_control.color[2] = 0;

		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse r_shadow_color\n");
	}

	if (sscanf(r_shadow_angles->string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
	{
		r_shadow_control.angles[0] = clamp(temp[0], -360, 360);
		r_shadow_control.angles[1] = clamp(temp[1], -360, 360);
		r_shadow_control.angles[2] = clamp(temp[2], -360, 360);
	}
	else
	{
		r_shadow_control.angles[0] = 90;
		r_shadow_control.angles[1] = 0;
		r_shadow_control.angles[2] = 0;

		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse r_shadow_angles\n");
	}

	if (sscanf(r_shadow_distfade->string, "%f %f", &temp[0], &temp[1]) == 2)
	{
		r_shadow_control.distfade[0] = clamp(temp[0], 0, 10000);
		r_shadow_control.distfade[1] = clamp(temp[1], r_shadow_control.distfade[0], 10000) - r_shadow_control.distfade[0];
	}
	else
	{
		r_shadow_control.distfade[0] = 64;
		r_shadow_control.distfade[1] = 128 - 64;

		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse r_shadow_distfade\n");
	}

	if (sscanf(r_shadow_lumfade->string, "%f %f", &temp[0], &temp[1]) == 2)
	{
		r_shadow_control.lumfade[0] = clamp(temp[0], 0, 255) / 255.0f;
		r_shadow_control.lumfade[1] = clamp(temp[1], 0, 255) / 255.0f;

		if (r_shadow_control.lumfade[0] < r_shadow_control.lumfade[1] + 0.01f)
			r_shadow_control.lumfade[0] = r_shadow_control.lumfade[1] + 0.01f;
	}
	else
	{
		r_shadow_control.lumfade[0] = clamp(64, 0, 255) / 255.0f;
		r_shadow_control.lumfade[1] = clamp(0, 0, 255) / 255.0f;

		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse r_shadow_lumfade\n");
	}

	r_shadow_control.intensity = clamp(r_shadow_intensity->value, 0, 1);

	r_shadow_control.quality[0][0] = clamp(r_shadow_high_distance->value, 0, 100000);
	r_shadow_control.quality[0][1] = clamp(r_shadow_high_scale->value, 0.1, 8);

	r_shadow_control.quality[1][0] = clamp(r_shadow_medium_distance->value, 0, 100000);
	r_shadow_control.quality[1][1] = clamp(r_shadow_medium_scale->value, 0.1, 8);

	r_shadow_control.quality[2][0] = clamp(r_shadow_low_distance->value, 0, 100000);
	r_shadow_control.quality[2][1] = clamp(r_shadow_low_scale->value, 0.1, 8);

}

void R_LoadHDRControl(void)
{
	//Initialize hdr control, enabled by default

	r_hdr_control.enabled = true;

	r_hdr_control.adaptation = clamp(r_hdr_adaptation->value, 0.1, 100);
	r_hdr_control.blurwidth = clamp(r_hdr_blurwidth->value, 0, 1);
	r_hdr_control.darkness = clamp(r_hdr_darkness->value, 0.01, 10);
	r_hdr_control.exposure = clamp(r_hdr_exposure->value, 0.01, 10);
}

void R_LoadWaterControl(void)
{

}

void R_LoadSSRControl(void)
{
	r_ssr_control.enabled = true;

	r_ssr_control.ray_step = clamp(r_ssr_ray_step->value, 0.01, 10.0);
	r_ssr_control.iter_count = clamp((int)r_ssr_iter_count->value, 10, 300);
	r_ssr_control.distance_bias = clamp(r_ssr_distance_bias->value, 0.01, 10.0);
	r_ssr_control.adaptive_step = r_ssr_adaptive_step->value ? true : false;
	r_ssr_control.exponential_step = r_ssr_exponential_step->value ? true : false;
	r_ssr_control.binary_search = r_ssr_binary_search->value ? true : false;

	float temp[4];
	if (sscanf(r_ssr_fade->string, "%f %f", &temp[0], &temp[1]) == 2)
	{
		r_ssr_control.fade[0] = clamp(temp[0], 0, 10.0);
		r_ssr_control.fade[1] = clamp(temp[1], 0, 10.0);
	}
	else
	{
		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"r_ssr_fade\"\n");
	}
}

void R_LoadBSPEntities(void)
{
	R_LoadShadowControl();

	R_LoadHDRControl();

	R_LoadWaterControl();

	R_LoadSSRControl();

	for(size_t i = 0; i < r_wsurf.vBSPEntities.size(); i++)
	{
		bspentity_t *ent = &r_wsurf.vBSPEntities[i];

		char *classname = ent->classname;

		if(!classname)
			continue;
#if 0
		if (!strcmp(classname, "env_cubemap"))
		{
			R_ParseBSPEntity_Env_Cubemap(ent);
		}
#endif	
		else if (!strcmp(classname, "light_dynamic"))
		{
			R_ParseBSPEntity_Light_Dynamic(ent);
		}

		else if (!strcmp(classname, "env_water_control"))
		{
			R_ParseBSPEntity_Env_Water_Control(ent);
		}

		else if (!strcmp(classname, "env_hdr_control"))
		{
			R_ParseBSPEntity_Env_HDR_Control(ent);
		}

		else if (!strcmp(classname, "env_shadow_control"))
		{
			R_ParseBSPEntity_Env_Shadow_Control(ent);			
		}

		else if (!strcmp(classname, "env_ssr_control"))
		{
			R_ParseBSPEntity_Env_SSR_Control(ent);
		}
	}//end for

	gEngfuncs.pfnAngleVectors(r_shadow_control.angles, r_shadow_control.vforward, r_shadow_control.vright, r_shadow_control.vup);
}

void R_DrawSequentialPolyVBO(msurface_t *s)
{
	R_RenderDynamicLightmaps(s);

	auto lightmapnum = s->lightmaptexturenum;

	if (lightmap_modified[lightmapnum])
	{
		lightmap_modified[lightmapnum] = 0;

		glRect_t *theRect = (glRect_t *)((char *)lightmap_rectchange + sizeof(glRect_t) * lightmapnum);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		theRect->l = BLOCK_WIDTH;
		theRect->t = BLOCK_HEIGHT;
		theRect->h = 0;
		theRect->w = 0;
	}

	if (s->pdecals)
	{
		gDecalSurfs[(*gDecalSurfCount)] = s;
		(*gDecalSurfCount)++;

		if ((*gDecalSurfCount) > MAX_DECALSURFS)
			Sys_ErrorEx("Too many decal surfaces!\n");
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
			r_entity_mins[i] = e->origin[i] - clmodel->radius;
			r_entity_maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;

		VectorAdd(e->origin, clmodel->mins, r_entity_mins);
		VectorAdd(e->origin, clmodel->maxs, r_entity_maxs);
	}

	if (R_CullBox(r_entity_mins, r_entity_maxs))
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 1024);
	}
	else
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 64);
	}

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

				if (gl_flashblend && gl_flashblend->value)
					goto skip_marklight;
			}

			for (k = 0; k < max_dlights; k++)
			{
				vec3_t saveOrigin;

				if ((cl_dlights[k].die < (*cl_time)) || (!cl_dlights[k].radius))
					continue;

				VectorCopy(cl_dlights[k].origin, saveOrigin);
				VectorSubtract(cl_dlights[k].origin, e->origin, cl_dlights[k].origin);

				gRefFuncs.R_MarkLights(&cl_dlights[k], 1 << k, clmodel->nodes + clmodel->hulls[0].firstclipnode);
				VectorCopy(saveOrigin, cl_dlights[k].origin);
			}
		}
	}
skip_marklight:

	R_RotateForEntity(e->origin, e);

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	R_SetRenderMode(e);

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

	r_wsurf.bShadowmapTexture = false;

	if(R_ShouldRenderShadowScene(1) && r_draw_nontransparent)
		r_wsurf.bShadowmapTexture = true;

	auto modcache = R_PrepareWSurfVBO(clmodel);

	R_EnableWSurfVBO(modcache);

	R_DrawWSurfVBO(modcache, e);

	R_EnableWSurfVBO(NULL);

	glDepthMask(1);
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);
	glDisable(GL_BLEND);

	glStencilMask(0);
	glEnable(GL_STENCIL_TEST);

	//Restore matrix
	memcpy(r_entity_matrix, r_identity_matrix, sizeof(r_entity_matrix));
}

void R_SetupSceneUBO(void)
{
	scene_ubo_t SceneUBO;

	memcpy(SceneUBO.viewMatrix, r_world_matrix, sizeof(mat4));
	memcpy(SceneUBO.projMatrix, r_projection_matrix, sizeof(mat4));
	memcpy(SceneUBO.invViewMatrix, r_world_matrix_inv, sizeof(mat4));
	memcpy(SceneUBO.invProjMatrix, r_proj_matrix_inv, sizeof(mat4));
	memcpy(SceneUBO.shadowMatrix[0], r_shadow_matrix[0], sizeof(mat4));
	memcpy(SceneUBO.shadowMatrix[1], r_shadow_matrix[1], sizeof(mat4));
	memcpy(SceneUBO.shadowMatrix[2], r_shadow_matrix[2], sizeof(mat4));
	memcpy(SceneUBO.viewpos, r_refdef->vieworg, sizeof(vec3_t));
	memcpy(SceneUBO.vpn, vpn, sizeof(vec3_t));
	memcpy(SceneUBO.vright, vright, sizeof(vec3_t));
	memcpy(SceneUBO.shadowDirection, &r_shadow_control.vforward, sizeof(vec3_t));
	memcpy(SceneUBO.shadowColor, &r_shadow_control.color, sizeof(vec3_t));
	SceneUBO.shadowColor[3] = r_shadow_control.intensity;
	memcpy(SceneUBO.shadowFade, &r_shadow_control.distfade, sizeof(vec2_t));
	memcpy(&SceneUBO.shadowFade[2], &r_shadow_control.lumfade, sizeof(vec2_t));

	//normal[0] * x+ normal[1] * y+ normal[2] * z = normal[0] * vert[0] +normal[1] * vert[1] +normal[2] * vert[2]

	if (r_draw_pass == r_draw_reflect && curwater)
	{
		float equation[4] = { curwater->normal[0], curwater->normal[1], curwater->normal[2], -curwater->plane };
		memcpy(SceneUBO.clipPlane, equation, sizeof(vec4_t));
	}

	memcpy(SceneUBO.fogColor, r_fog_color, sizeof(vec4_t));
	SceneUBO.fogStart = r_fog_control[0];
	SceneUBO.fogEnd = r_fog_control[1];
	SceneUBO.time = (*cl_time);

	float r_g = 1.0f / v_gamma->value;

	float r_g3;
	if (v_brightness->value <= 0.0f)
		r_g3 = 0.125f;
	else if (v_brightness->value > 1.0f)
		r_g3 = 0.05f;
	else
		r_g3 = 0.125f - (v_brightness->value * v_brightness->value) * 0.075f;

	SceneUBO.r_g1 = r_g;
	SceneUBO.r_g3 = r_g3;
	SceneUBO.v_brightness = v_brightness->value;
	SceneUBO.v_lightgamma = v_lightgamma->value;
	SceneUBO.v_lambert = v_lambert->value;

	glNamedBufferSubData(r_wsurf.hSceneUBO, 0, sizeof(SceneUBO), &SceneUBO);
}

void R_DrawWorld(void)
{
	r_draw_nontransparent = true;

	InvertMatrix(r_world_matrix, r_world_matrix_inv);
	InvertMatrix(r_projection_matrix, r_proj_matrix_inv);
	memcpy(r_entity_matrix, r_identity_matrix, sizeof(r_entity_matrix));

	R_BeginRenderGBuffer();

	VectorCopy(r_refdef->vieworg, modelorg);

	cl_entity_t tempent = { 0 };
	tempent.model = r_worldmodel;
	tempent.curstate.rendercolor.r = cshift_water->destcolor[0];
	tempent.curstate.rendercolor.g = cshift_water->destcolor[1];
	tempent.curstate.rendercolor.b = cshift_water->destcolor[2];

	(*currententity) = &tempent;
	*currenttexture = -1;

	glColor3f(1.0f, 1.0f, 1.0f);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 1024);
	}
	else
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 64);
	}

	GL_DisableMultitexture();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//Capture previous fog settings from R_RenderScene
	if (glIsEnabled(GL_FOG))
	{
		glGetIntegerv(GL_FOG_MODE, &r_fog_mode);

		if (r_fog_mode == GL_LINEAR)
		{
			glGetFloatv(GL_FOG_START, &r_fog_control[0]);
			glGetFloatv(GL_FOG_END, &r_fog_control[1]);
			glGetFloatv(GL_FOG_COLOR, r_fog_color);
		}
	}

	r_wsurf.bDiffuseTexture = true;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bShadowmapTexture = false;

	//Setup shadow

	if (R_ShouldRenderShadowScene(1))
	{
		r_wsurf.bShadowmapTexture = true;

		const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f };

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		for (int i = 0; i < 3; ++i)
		{
			if (shadow_numvisedicts[i] > 0)
			{
				glLoadIdentity();
				glLoadMatrixf(bias);
				glMultMatrixf(shadow_projmatrix[i]);
				glMultMatrixf(shadow_mvmatrix[i]);
				glGetFloatv(GL_TEXTURE_MATRIX, r_shadow_matrix[i]);
			}
		}
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}

	//Setup Scene UBO

	R_SetupSceneUBO();

	//Skybox use stencil = 1

	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	R_DrawSkyBox();

	if (r_draw_pass == r_draw_reflect && curwater->level == WATER_LEVEL_REFLECT_SKYBOX)
		goto skip_world;

	//World uses stencil = 0

	glStencilFunc(GL_ALWAYS, 0, 0xFF);

	r_wsurf.bDiffuseTexture = true;
	r_wsurf.bLightmapTexture = true;

	auto modcache = R_PrepareWSurfVBO(r_worldmodel);

	R_EnableWSurfVBO(modcache);

	R_DrawWSurfVBO(modcache, (*currententity));

	R_EnableWSurfVBO(NULL);

skip_world:

	GL_DisableMultitexture();

	//No stencil write later
	glStencilMask(0);
	glDisable(GL_STENCIL_TEST);
}