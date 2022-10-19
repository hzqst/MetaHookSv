#include "gl_local.h"
#include <sstream>
#include <algorithm>

r_worldsurf_t r_wsurf;

cvar_t *r_wsurf_parallax_scale;
cvar_t *r_wsurf_sky_fog;
cvar_t *r_wsurf_zprepass;

int r_fog_mode = 0;
float r_fog_control[3] = { 0 };
float r_fog_color[4] = { 0 };
float r_shadow_matrix[3][16];
float r_world_matrix_inv[16];
float r_proj_matrix_inv[16];
vec3_t r_frustum_origin[4];
vec3_t r_frustum_vec[4];
float r_znear = 0;
float r_zfar = 0;
bool r_ortho = false;
int r_wsurf_drawcall = 0;
int r_wsurf_polys = 0;

vec3_t r_entity_mins, r_entity_maxs;

std::unordered_map <int, wsurf_program_t> g_WSurfProgramTable;

std::unordered_map <int, detail_texture_cache_t *> g_DetailTextureTable;

std::unordered_map <std::string, detail_texture_cache_t *> g_DecalTextureTable;

std::vector<wsurf_vbo_t *> g_WSurfVBOCache;

int EngineGetMaxDLight(void)
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

	return max_dlights;
}

void R_ClearWSurfVBOCache(void)
{
	for (size_t i =0 ;i < g_WSurfVBOCache.size(); ++i)
	{
		if (g_WSurfVBOCache[i])
		{
			auto &VBOCache = g_WSurfVBOCache[i];
			if (VBOCache->hEntityUBO)
			{
				GL_DeleteBuffer(VBOCache->hEntityUBO);
				VBOCache->hEntityUBO = NULL;
			}
			if (VBOCache->hDecalEBO)
			{
				GL_DeleteBuffer(VBOCache->hDecalEBO);
				VBOCache->hDecalEBO = NULL;
			}

			for (size_t j = 0; j < VBOCache->vLeaves.size(); ++j)
			{
				auto VBOLeaf = VBOCache->vLeaves[j];

				if (VBOLeaf != VBOCache->pNoVisLeaf)
				{
					if (VBOLeaf->hEBO)
					{
						GL_DeleteBuffer(VBOLeaf->hEBO);
						VBOLeaf->hEBO = NULL;
					}

					for (size_t k = 0; k < WSURF_TEXCHAIN_MAX; ++k)
					{
						for (size_t l = 0; l < VBOLeaf->vDrawBatch[k].size(); ++l)
						{
							delete VBOLeaf->vDrawBatch[k][l];
						}
					}
					delete VBOLeaf;
				}
			}
			
			if (VBOCache->pNoVisLeaf)
			{
				if (VBOCache->pNoVisLeaf->hEBO)
				{
					GL_DeleteBuffer(VBOCache->pNoVisLeaf->hEBO);
					VBOCache->pNoVisLeaf->hEBO = NULL;
				}

				for (size_t k = 0; k < WSURF_TEXCHAIN_MAX; ++k)
				{
					for (size_t l = 0; l < VBOCache->pNoVisLeaf->vDrawBatch[k].size(); ++l)
					{
						delete VBOCache->pNoVisLeaf->vDrawBatch[k][l];
					}
				}
				delete VBOCache->pNoVisLeaf;
			}
			
			delete g_WSurfVBOCache[i];
		}
		g_WSurfVBOCache[i] = NULL;
	}
	g_WSurfVBOCache.clear();
}

const program_state_name_t s_WSurfProgramStateName[] = {
{ WSURF_DIFFUSE_ENABLED				,"WSURF_DIFFUSE_ENABLED"},
{ WSURF_LIGHTMAP_ENABLED			,"WSURF_LIGHTMAP_ENABLED"},
{ WSURF_REPLACETEXTURE_ENABLED		,"WSURF_REPLACETEXTURE_ENABLED"},
{ WSURF_DETAILTEXTURE_ENABLED		,"WSURF_DETAILTEXTURE_ENABLED"},
{ WSURF_NORMALTEXTURE_ENABLED		,"WSURF_NORMALTEXTURE_ENABLED"},
{ WSURF_PARALLAXTEXTURE_ENABLED		,"WSURF_PARALLAXTEXTURE_ENABLED"},
{ WSURF_SPECULARTEXTURE_ENABLED		,"WSURF_SPECULARTEXTURE_ENABLED"},
{ WSURF_LINEAR_FOG_ENABLED			,"WSURF_LINEAR_FOG_ENABLED"},
{ WSURF_EXP_FOG_ENABLED				,"WSURF_EXP_FOG_ENABLED"},
{ WSURF_EXP2_FOG_ENABLED			,"WSURF_EXP2_FOG_ENABLED"},
{ WSURF_GBUFFER_ENABLED				,"WSURF_GBUFFER_ENABLED"},
{ WSURF_TRANSPARENT_ENABLED			,"WSURF_TRANSPARENT_ENABLED"},
{ WSURF_SHADOW_CASTER_ENABLED		,"WSURF_SHADOW_CASTER_ENABLED"},
{ WSURF_SHADOWMAP_ENABLED			,"WSURF_SHADOWMAP_ENABLED"},
{ WSURF_SHADOWMAP_HIGH_ENABLED		,"WSURF_SHADOWMAP_HIGH_ENABLED"},
{ WSURF_SHADOWMAP_MEDIUM_ENABLED	,"WSURF_SHADOWMAP_MEDIUM_ENABLED"},
{ WSURF_SHADOWMAP_LOW_ENABLED		,"WSURF_SHADOWMAP_LOW_ENABLED"},
{ WSURF_BINDLESS_ENABLED			,"WSURF_BINDLESS_ENABLED"},
{ WSURF_SKYBOX_ENABLED				,"WSURF_SKYBOX_ENABLED"},
{ WSURF_DECAL_ENABLED				,"WSURF_DECAL_ENABLED"},
{ WSURF_CLIP_ENABLED				,"WSURF_CLIP_ENABLED"},
{ WSURF_CLIP_WATER_ENABLED			,"WSURF_CLIP_WATER_ENABLED"},
{ WSURF_OIT_ALPHA_BLEND_ENABLED		,"WSURF_OIT_ALPHA_BLEND_ENABLED"},
{ WSURF_OIT_ADDITIVE_BLEND_ENABLED	,"WSURF_OIT_ADDITIVE_BLEND_ENABLED"},
};

void R_SaveWSurfProgramStates(void)
{
	std::stringstream ss;
	for (auto &p : g_WSurfProgramTable)
	{
		if (p.first == 0)
		{
			ss << "NONE";
		}
		else
		{
			for (int i = 0; i < _ARRAYSIZE(s_WSurfProgramStateName); ++i)
			{
				if (p.first & s_WSurfProgramStateName[i].state)
				{
					ss << s_WSurfProgramStateName[i].name << " ";
				}
			}
		}
		ss << "\n";
	}

	auto FileHandle = g_pFileSystem->Open("renderer/shader/wsurf_cache.txt", "wt");
	if (FileHandle)
	{
		auto str = ss.str();
		g_pFileSystem->Write(str.data(), str.length(), FileHandle);
		g_pFileSystem->Close(FileHandle);
	}
}

void R_LoadWSurfProgramStates(void)
{
	auto FileHandle = g_pFileSystem->Open("renderer/shader/wsurf_cache.txt", "rt");
	if (FileHandle)
	{
		char szReadLine[4096];
		while (!g_pFileSystem->EndOfFile(FileHandle))
		{
			g_pFileSystem->ReadLine(szReadLine, sizeof(szReadLine) - 1, FileHandle);
			szReadLine[sizeof(szReadLine) - 1] = 0;

			int ProgramState = -1;
			bool quoted = false;
			char token[256];
			char *p = szReadLine;
			while (1)
			{
				p = g_pFileSystem->ParseFile(p, token, &quoted);
				if (token[0])
				{
					if (!strcmp(token, "NONE"))
					{
						ProgramState = 0;
						break;
					}
					else
					{
						for (int i = 0; i < _ARRAYSIZE(s_WSurfProgramStateName); ++i)
						{
							if (!strcmp(token, s_WSurfProgramStateName[i].name))
							{
								if(ProgramState == -1)
									ProgramState = 0;
								ProgramState |= s_WSurfProgramStateName[i].state;
							}
						}
					}
				}
				else
				{
					break;
				}

				if (!p)
					break;
			}

			if(ProgramState != -1)
				R_UseWSurfProgram(ProgramState, NULL);
		}
		g_pFileSystem->Close(FileHandle);
	}

	GL_UseProgram(0);
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

		if (state & WSURF_REPLACETEXTURE_ENABLED)
			defs << "#define REPLACETEXTURE_ENABLED\n";

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

		if (state & WSURF_EXP_FOG_ENABLED)
			defs << "#define EXP_FOG_ENABLED\n";

		if (state & WSURF_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

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

		if (state & WSURF_SKYBOX_ENABLED)
			defs << "#define SKYBOX_ENABLED\n";

		if (state & WSURF_DECAL_ENABLED)
			defs << "#define DECAL_ENABLED\n";

		if (state & WSURF_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		if (state & WSURF_CLIP_WATER_ENABLED)
			defs << "#define CLIP_WATER_ENABLED\n";

		if (state & WSURF_OIT_ALPHA_BLEND_ENABLED)
			defs << "#define OIT_ALPHA_BLEND_ENABLED\n";

		if (state & WSURF_OIT_ADDITIVE_BLEND_ENABLED)
			defs << "#define OIT_ADDITIVE_BLEND_ENABLED\n";

		if(glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define UINT64_ENABLED\n";
	
		defs << "#define SHADOW_TEXTURE_OFFSET (1.0 / " << std::dec << r_shadow_texture.size << ".0)\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\wsurf_shader.vsh", "renderer\\shader\\wsurf_shader.fsh", def.c_str(), def.c_str(), NULL);

		SHADER_UNIFORM(prog, u_parallaxScale, "u_parallaxScale");

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

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		g_pMetaHookAPI->SysError("R_UseWSurfProgram: Failed to load program!");
	}
}

void R_FreeSceneUBO(void)
{
	if (r_wsurf.hDecalVBO)
	{
		GL_DeleteBuffer(r_wsurf.hDecalVBO);
		r_wsurf.hDecalVBO = 0;
	}

	if (r_wsurf.hSceneUBO)
	{
		GL_DeleteBuffer(r_wsurf.hSceneUBO);
		r_wsurf.hSceneUBO = 0;
	}

	if (r_wsurf.hDecalSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hDecalSSBO);
		r_wsurf.hDecalSSBO = 0;
	}

	if (r_wsurf.hSkyboxSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hSkyboxSSBO);
		r_wsurf.hSkyboxSSBO = 0;
	}

	if (r_wsurf.hOITFragmentSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hOITFragmentSSBO);
		r_wsurf.hOITFragmentSSBO = 0;
	}

	if (r_wsurf.hOITNumFragmentSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hOITNumFragmentSSBO);
		r_wsurf.hOITNumFragmentSSBO = 0;
	}

	if (r_wsurf.hOITAtomicSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hOITAtomicSSBO);
		r_wsurf.hOITAtomicSSBO = 0;
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

void R_FreeWorldTextures(void)
{
	if (r_wsurf.hWorldSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hWorldSSBO);
		r_wsurf.hWorldSSBO = 0;
	}
}

void R_FreeVertexBuffer(void)
{
	if (r_wsurf.hSceneVBO)
	{
		GL_DeleteBuffer(r_wsurf.hSceneVBO);
		r_wsurf.hSceneVBO = 0;
	}
	r_wsurf.vFaceBuffer.clear();
}

void R_BrushLinkTextureChain(model_t *mod)
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

void R_RecursiveLinkTextureChain(mnode_t *node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != (*r_visframecount))
		return;

	if (node->contents < 0)
		return;

	R_RecursiveLinkTextureChain(node->children[0]);

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

	R_RecursiveLinkTextureChain(node->children[1]);
}

void R_GenerateIndicesForTexChain(msurface_t *s, brushtexchain_t *texchain, std::vector<unsigned int> &vIndicesBuffer)
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

void R_SortTextureChain(wsurf_vbo_leaf_t *vboleaf, int iTexchainType)
{
	vboleaf->vTextureChain[iTexchainType].shrink_to_fit();

	for (size_t i = 0; i < vboleaf->vTextureChain[iTexchainType].size(); ++i)
	{
		auto &texchain = vboleaf->vTextureChain[iTexchainType][i];

		auto pcache = texchain.pDetailTextureCache;
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

	std::sort(vboleaf->vTextureChain[iTexchainType].begin(), vboleaf->vTextureChain[iTexchainType].end(), [](const brushtexchain_t &a, const brushtexchain_t &b) {
		return b.iDetailTextureFlags > a.iDetailTextureFlags;
	});
}

void R_GenerateDrawBatch(wsurf_vbo_leaf_t *vboleaf, int iTexchainType, int iDrawBatchType)
{
	int detailTextureFlags = -1;
	wsurf_vbo_batch_t *batch = NULL;

	for (size_t i = 0; i < vboleaf->vTextureChain[iTexchainType].size(); ++i)
	{
		auto &texchain = vboleaf->vTextureChain[iTexchainType][i];

		if (texchain.iDetailTextureFlags != detailTextureFlags)
		{
			if (batch)
			{
				batch->vStartIndex.shrink_to_fit();
				batch->vIndiceCount.shrink_to_fit();
				vboleaf->vDrawBatch[iDrawBatchType].emplace_back(batch);
				batch = NULL;
			}

			detailTextureFlags = texchain.iDetailTextureFlags;			
		}

		if (!batch)
		{
			batch = new wsurf_vbo_batch_t;
			batch->iBaseDrawId = i * 5;
		}

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
		vboleaf->vDrawBatch[iDrawBatchType].emplace_back(batch);
		batch = NULL;
	}
}

void R_GenerateTexChain(model_t *mod, wsurf_vbo_leaf_t *vboleaf, std::vector<unsigned int> &vIndicesBuffer)
{
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
				texchain.pDetailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
				texchain.iIndiceCount = 0;
				texchain.iPolyCount = 0;
				texchain.iStartIndex = vIndicesBuffer.size();
				texchain.iType = TEXCHAIN_SKY;

				for (; s; s = s->texturechain)
				{
					R_GenerateIndicesForTexChain(s, &texchain, vIndicesBuffer);
				}

				if (texchain.iIndiceCount > 0)
					vboleaf->TextureChainSky = texchain;
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
								texchain.pDetailTextureCache = R_FindDetailTextureCache(t2->gl_texturenum);
								texchain.iIndiceCount = 0;
								texchain.iPolyCount = 0;
								texchain.iStartIndex = vIndicesBuffer.size();
								texchain.iType = TEXCHAIN_STATIC;

								for (int n = 0; n < numtexturechain; ++n)
								{
									if (texchainMapper[n] == k)
										R_GenerateIndicesForTexChain(texchainSurface[n], &texchain, vIndicesBuffer);
								}

								if (texchain.iIndiceCount > 0)
									vboleaf->vTextureChain[WSURF_TEXCHAIN_STATIC].emplace_back(texchain);
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
						texchain.pDetailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
						texchain.iIndiceCount = 0;
						texchain.iPolyCount = 0;
						texchain.iStartIndex = vIndicesBuffer.size();
						texchain.iType = TEXCHAIN_STATIC;

						for (; s; s = s->texturechain)
						{
							R_GenerateIndicesForTexChain(s, &texchain, vIndicesBuffer);
						}

						if (texchain.iIndiceCount > 0)
							vboleaf->vTextureChain[WSURF_TEXCHAIN_ANIM].emplace_back(texchain);
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
					texchain.pDetailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
					texchain.iIndiceCount = 0;
					texchain.iPolyCount = 0;
					texchain.iStartIndex = vIndicesBuffer.size();
					texchain.iType = TEXCHAIN_STATIC;

					for (; s; s = s->texturechain)
					{
						R_GenerateIndicesForTexChain(s, &texchain, vIndicesBuffer);
					}

					if (texchain.iIndiceCount > 0)
						vboleaf->vTextureChain[WSURF_TEXCHAIN_STATIC].emplace_back(texchain);
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
				texchain.pDetailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
				texchain.iIndiceCount = 0;
				texchain.iPolyCount = 0;
				texchain.iStartIndex = vIndicesBuffer.size();
				texchain.iType = TEXCHAIN_SCROLL;

				for (; s; s = s->texturechain)
				{
					R_GenerateIndicesForTexChain(s, &texchain, vIndicesBuffer);
				}

				if (texchain.iIndiceCount > 0)
					vboleaf->vTextureChain[WSURF_TEXCHAIN_STATIC].emplace_back(texchain);
			}
		}

		//End construction

		t->texturechain = NULL;
	}
}

void R_GenerateBufferStorage(model_t *mod, wsurf_vbo_t *modvbo)
{
	if (mod == r_worldmodel)
	{
		//(*r_visframecount) = 0;

		modvbo->pNoVisLeaf = new wsurf_vbo_leaf_t;

		for (int i = 0; i < r_worldmodel->numframes; ++i)
		{
			bool bNoVis = R_MarkPVSLeaves(i);

			auto vboleaf = (bNoVis) ? modvbo->pNoVisLeaf : new wsurf_vbo_leaf_t;

			modvbo->vLeaves.emplace_back(vboleaf);

			if (!vboleaf->bInit)
			{
				R_RecursiveLinkTextureChain(mod->nodes);

				std::vector<unsigned int> vIndicesBuffer;

				R_GenerateTexChain(mod, vboleaf, vIndicesBuffer);

				R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_STATIC);
				R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_ANIM);

				R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_STATIC);
				R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_SOLID);
				R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_ANIM, WSURF_DRAWBATCH_SOLID);

				vboleaf->hEBO = GL_GenBuffer();
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboleaf->hEBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * vIndicesBuffer.size(), vIndicesBuffer.data(), GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

				vboleaf->bInit = true;
			}
		}

		//(*r_visframecount) = 0;
	}
	else
	{
		auto vboleaf = new wsurf_vbo_leaf_t;

		modvbo->vLeaves.emplace_back(vboleaf);

		R_BrushLinkTextureChain(mod);

		std::vector<unsigned int> vIndicesBuffer;

		R_GenerateTexChain(mod, vboleaf, vIndicesBuffer);

		R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_STATIC);
		R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_ANIM);

		R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_STATIC);
		R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_SOLID);
		R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_ANIM, WSURF_DRAWBATCH_SOLID);

		vboleaf->hEBO = GL_GenBuffer();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboleaf->hEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * vIndicesBuffer.size(), vIndicesBuffer.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	modvbo->hEntityUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, modvbo->hEntityUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(entity_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void R_GenerateWorldTextures(void)
{
	if (bUseBindless)
	{
		std::vector <GLuint64> ssbo;

		ssbo.resize(r_worldmodel->numtextures * WSURF_MAX_TEXTURE);

		for (int i = 0; i < r_worldmodel->numtextures; i++)
		{
			auto t = r_worldmodel->textures[i];

			if (!t)
				continue;

			auto handle = glGetTextureHandleARB(t->gl_texturenum);
			glMakeTextureHandleResidentARB(handle);

			ssbo[i * WSURF_MAX_TEXTURE + WSURF_DIFFUSE_TEXTURE] = handle;
			
			//zero it?
			for (int j = WSURF_REPLACE_TEXTURE; j < WSURF_MAX_TEXTURE; ++j)
				ssbo[i * WSURF_MAX_TEXTURE + j] = 0;

			auto pcache = R_FindDetailTextureCache(t->gl_texturenum);
			if (pcache)
			{
				for (int j = WSURF_REPLACE_TEXTURE; j < WSURF_MAX_TEXTURE; ++j)
				{
					if (pcache->tex[j].gltexturenum)
					{
						auto handle = glGetTextureHandleARB(pcache->tex[j].gltexturenum);
						glMakeTextureHandleResidentARB(handle);

						ssbo[i * WSURF_MAX_TEXTURE + j] = handle;
					}
				}
			}
		}

		r_wsurf.hWorldSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_wsurf.hWorldSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * ssbo.size(), ssbo.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

void Mod_LoadBrushModel(model_t *mod, void *buffer)
{
	gRefFuncs.Mod_LoadBrushModel(mod, buffer);

	auto header = (dheader_t * )buffer;

	auto lump = &header->lumps[LUMP_LEAFS];

	auto count = lump->filelen / sizeof(dleaf_t);

	//Get correct leaf count here!!!

	mod->numframes = count;
	//mod->numleafs |= ((count << 16) & 0xFFFF0000ul);
}

int R_FindTextureIdByTexture(texture_t *ptex)
{
	for (int i = 0; i < r_worldmodel->numtextures; ++i)
	{
		if (r_worldmodel->textures[i] == ptex)
			return i;
	}

	return -1;
}

void R_GenerateVertexBuffer(void)
{
	std::vector<brushvertex_t> vVertexBuffer;

	brushvertex_t Vertexes[3];
	int iNumFaces = 0;
	int iNumVerts = 0;

	auto surf = r_worldmodel->surfaces;

	r_wsurf.vFaceBuffer.resize(r_worldmodel->numsurfaces);

	for(int i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		auto poly = surf[i].polys;

		poly->flags = i;

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

		float replaceScale[2] = { 1, 1 };
		float detailScale[2] = { 1, 1 };
		float normalScale[2] = { 1, 1 };
		float parallaxScale[2] = { 1, 1 };
		float specularScale[2] = { 1, 1 };

		if (ptexture)
		{
			auto pcache = R_FindDetailTextureCache(ptexture->gl_texturenum);
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
					Vertexes[0].normal[0] = brushface->normal[0];
					Vertexes[0].normal[1] = brushface->normal[1];
					Vertexes[0].normal[2] = brushface->normal[2];
					Vertexes[0].s_tangent[0] = brushface->s_tangent[0];
					Vertexes[0].s_tangent[1] = brushface->s_tangent[1];
					Vertexes[0].s_tangent[2] = brushface->s_tangent[2];
					Vertexes[0].t_tangent[0] = brushface->t_tangent[0];
					Vertexes[0].t_tangent[1] = brushface->t_tangent[1];
					Vertexes[0].t_tangent[2] = brushface->t_tangent[2];

					Vertexes[0].texcoord[0] = v[3];
					Vertexes[0].texcoord[1] = v[4];
					Vertexes[0].texcoord[2] = (ptexture && (brushface->flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;
					Vertexes[0].lightmaptexcoord[0] = v[5];
					Vertexes[0].lightmaptexcoord[1] = v[6];
					Vertexes[0].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
					Vertexes[0].replacetexcoord[0] = replaceScale[0];
					Vertexes[0].replacetexcoord[1] = replaceScale[1];
					Vertexes[0].detailtexcoord[0] = detailScale[0];
					Vertexes[0].detailtexcoord[1] = detailScale[1];
					Vertexes[0].normaltexcoord[0] = normalScale[0];
					Vertexes[0].normaltexcoord[1] = normalScale[1];
					Vertexes[0].parallaxtexcoord[0] = parallaxScale[0];
					Vertexes[0].parallaxtexcoord[1] = parallaxScale[1];
					Vertexes[0].speculartexcoord[0] = specularScale[0];
					Vertexes[0].speculartexcoord[1] = specularScale[1];
					Vertexes[0].texindex = R_FindTextureIdByTexture(ptexture);

					vVertexBuffer.emplace_back(Vertexes[0]);
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
					Vertexes[j].normal[0] = brushface->normal[0];
					Vertexes[j].normal[1] = brushface->normal[1];
					Vertexes[j].normal[2] = brushface->normal[2];
					Vertexes[j].s_tangent[0] = brushface->s_tangent[0];
					Vertexes[j].s_tangent[1] = brushface->s_tangent[1];
					Vertexes[j].s_tangent[2] = brushface->s_tangent[2];
					Vertexes[j].t_tangent[0] = brushface->t_tangent[0];
					Vertexes[j].t_tangent[1] = brushface->t_tangent[1];
					Vertexes[j].t_tangent[2] = brushface->t_tangent[2];

					Vertexes[j].texcoord[0] = v[3];
					Vertexes[j].texcoord[1] = v[4];
					Vertexes[j].texcoord[2] = (ptexture && (brushface->flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;
					Vertexes[j].lightmaptexcoord[0] = v[5];
					Vertexes[j].lightmaptexcoord[1] = v[6];
					Vertexes[j].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
					Vertexes[j].replacetexcoord[0] = replaceScale[0];
					Vertexes[j].replacetexcoord[1] = replaceScale[1];
					Vertexes[j].detailtexcoord[0] = detailScale[0];
					Vertexes[j].detailtexcoord[1] = detailScale[1];
					Vertexes[j].normaltexcoord[0] = normalScale[0];
					Vertexes[j].normaltexcoord[1] = normalScale[1];
					Vertexes[j].parallaxtexcoord[0] = parallaxScale[0];
					Vertexes[j].parallaxtexcoord[1] = parallaxScale[1];
					Vertexes[j].speculartexcoord[0] = specularScale[0];
					Vertexes[j].speculartexcoord[1] = specularScale[1];
					Vertexes[j].texindex = R_FindTextureIdByTexture(ptexture);
				}
				vVertexBuffer.emplace_back(Vertexes[0]);
				vVertexBuffer.emplace_back(Vertexes[1]);
				vVertexBuffer.emplace_back(Vertexes[2]);
				iNumVerts += 3;

				for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
				{
					memcpy(&Vertexes[1], &Vertexes[2], sizeof(brushvertex_t));

					Vertexes[2].pos[0] = v[0];
					Vertexes[2].pos[1] = v[1];
					Vertexes[2].pos[2] = v[2];
					Vertexes[2].normal[0] = brushface->normal[0];
					Vertexes[2].normal[1] = brushface->normal[1];
					Vertexes[2].normal[2] = brushface->normal[2];
					Vertexes[2].s_tangent[0] = brushface->s_tangent[0];
					Vertexes[2].s_tangent[1] = brushface->s_tangent[1];
					Vertexes[2].s_tangent[2] = brushface->s_tangent[2];
					Vertexes[2].t_tangent[0] = brushface->t_tangent[0];
					Vertexes[2].t_tangent[1] = brushface->t_tangent[1];
					Vertexes[2].t_tangent[2] = brushface->t_tangent[2];

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
					Vertexes[2].texindex = R_FindTextureIdByTexture(ptexture);

					vVertexBuffer.emplace_back(Vertexes[0]);
					vVertexBuffer.emplace_back(Vertexes[1]);
					vVertexBuffer.emplace_back(Vertexes[2]);
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
	glBufferData( GL_ARRAY_BUFFER, sizeof(brushvertex_t) * vVertexBuffer.size(), vVertexBuffer.data(), GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

void R_GenerateSceneUBO(void)
{
	if (r_wsurf.hSceneUBO)
		return;

	r_wsurf.hSceneUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, r_wsurf.hSceneUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(scene_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_SCENE_UBO, r_wsurf.hSceneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//3.5 MBytes of VRAM
	r_wsurf.hDecalVBO = GL_GenBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, r_wsurf.hDecalVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(decalvertex_t) * MAX_DECALVERTS * MAX_DECALS, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (bUseBindless)
	{
		r_wsurf.hDecalSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_wsurf.hDecalSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * MAX_DECALS, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		r_wsurf.hSkyboxSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_wsurf.hSkyboxSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * 6, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	if (bUseOITBlend)
	{
		size_t fragmentBufferSizeBytes = sizeof(FragmentNode) * MAX_NUM_NODES * glwidth * glheight;

		r_wsurf.hOITFragmentSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_wsurf.hOITFragmentSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, fragmentBufferSizeBytes, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_OIT_FRAGMENT_SSBO, r_wsurf.hOITFragmentSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		size_t numFragmentBufferSizeBytes = sizeof(uint32_t) * glwidth * glheight;

		r_wsurf.hOITNumFragmentSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_wsurf.hOITNumFragmentSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numFragmentBufferSizeBytes, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_OIT_NUMFRAGMENT_SSBO, r_wsurf.hOITNumFragmentSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		r_wsurf.hOITAtomicSSBO = GL_GenBuffer();
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, r_wsurf.hOITAtomicSSBO);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BINDING_POINT_OIT_COUNTER_SSBO, r_wsurf.hOITAtomicSSBO);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	}
}

void R_ClearDetailTextureCache(void)
{
	for (auto p : g_DetailTextureTable)
		delete p.second;

	g_DetailTextureTable.clear();

	for (auto p : g_DecalTextureTable)
		delete p.second;

	g_DecalTextureTable.clear();
}

void R_ClearDecalCache(void)
{
	memset(r_wsurf.vDecalGLTextures, 0, sizeof(r_wsurf.vDecalGLTextures));
	memset(r_wsurf.vDecalDetailTextures, 0, sizeof(r_wsurf.vDecalDetailTextures));
	memset(r_wsurf.vDecalStartIndex, 0, sizeof(r_wsurf.vDecalStartIndex));
	memset(r_wsurf.vDecalVertexCount, 0, sizeof(r_wsurf.vDecalVertexCount));
}

void R_GenerateLightmapArray(void)
{
	if (!r_wsurf.iLightmapTextureArray)
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

	wsurf_vbo_t *modvbo = g_WSurfVBOCache[modelindex];

	if (!modvbo)
	{
		modvbo = new wsurf_vbo_t;

		R_GenerateBufferStorage(mod, modvbo);

		modvbo->pModel = mod;

		g_WSurfVBOCache[modelindex] = modvbo;
	}

	return modvbo;
}

void R_DrawWSurfVBOSolid(wsurf_vbo_leaf_t *vboleaf)
{
	int WSurfProgramState = 0;

	if (drawgbuffer)
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}

	if (r_draw_shadowcaster)
	{
		WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
	}

	if (r_draw_reflectview)
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	auto &drawBatches = vboleaf->vDrawBatch[WSURF_DRAWBATCH_SOLID];

	for (size_t i = 0; i < drawBatches.size(); ++i)
	{
		auto &batch = drawBatches[i];

		glMultiDrawElements(GL_POLYGON, batch->vIndiceCount.data(), GL_UNSIGNED_INT, (const void **)batch->vStartIndex.data(), batch->iDrawCount);

		r_wsurf_drawcall++;
		r_wsurf_polys += batch->iPolyCount;
	}
}

void R_DrawWSurfVBOStatic(wsurf_vbo_leaf_t *modvbo, bool bUseZPrePass)
{
	if(bUseBindless)
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

		//Don't do clipping when z-prepass used
		if (!bUseZPrePass)
		{
			if (r_draw_reflectview)
			{
				WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
			}
			else if (g_bPortalClipPlaneEnabled[0])
			{
				WSurfProgramState |= WSURF_CLIP_ENABLED;
			}
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

		if (r_draw_shadowcaster)
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		if (drawgbuffer)
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		}

		if (r_draw_oitblend)
		{
			if((*currententity)->curstate.rendermode == kRenderTransAdd)
				WSurfProgramState |= WSURF_OIT_ADDITIVE_BLEND_ENABLED;
			else
				WSurfProgramState |= WSURF_OIT_ALPHA_BLEND_ENABLED;
		}

		if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
		{
			WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
		}

		auto &drawBatches = modvbo->vDrawBatch[WSURF_DRAWBATCH_STATIC];
		for (size_t i = 0; i < drawBatches.size(); ++i)
		{
			auto &batch = drawBatches[i];

			int WSurfProgramStateBatch = WSurfProgramState;

			if (r_detailtextures->value)
			{
				if (batch->iDetailTextureFlags & (1 << WSURF_REPLACE_TEXTURE))
				{
					WSurfProgramStateBatch |= WSURF_REPLACETEXTURE_ENABLED;
				}

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
		for (size_t i = 0; i < modvbo->vTextureChain[WSURF_TEXCHAIN_STATIC].size(); ++i)
		{
			auto &texchain = modvbo->vTextureChain[WSURF_TEXCHAIN_STATIC][i];

			auto base = texchain.pTexture;

			int WSurfProgramState = 0;

			if (r_wsurf.bDiffuseTexture)
			{
				WSurfProgramState |= WSURF_DIFFUSE_ENABLED;

				GL_Bind(base->gl_texturenum);

				R_BeginDetailTextureByDetailTextureCache(texchain.pDetailTextureCache, &WSurfProgramState);
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

			if (!bUseZPrePass)
			{
				if (r_draw_reflectview)
				{
					WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
				}
				else if (g_bPortalClipPlaneEnabled[0])
				{
					WSurfProgramState |= WSURF_CLIP_ENABLED;
				}
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

			if (r_draw_shadowcaster)
			{
				WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
			}

			if (drawgbuffer)
			{
				WSurfProgramState |= WSURF_GBUFFER_ENABLED;
			}

			if (r_draw_oitblend)
			{
				if ((*currententity)->curstate.rendermode == kRenderTransAdd)
					WSurfProgramState |= WSURF_OIT_ADDITIVE_BLEND_ENABLED;
				else
					WSurfProgramState |= WSURF_OIT_ALPHA_BLEND_ENABLED;
			}

			if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
			{
				WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
			}

			wsurf_program_t prog = { 0 };
			R_UseWSurfProgram(WSurfProgramState, &prog);

			glDrawElements(GL_POLYGON, texchain.iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			R_EndDetailTexture(WSurfProgramState);

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iPolyCount;
		}
	}
}

texture_t *R_GetAnimatedTexture(texture_t *base)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		if ((*currententity)->curstate.effects & EF_FRAMEANIMTEXTURES)
		{
			if ((*currententity)->curstate.frame > 0)
			{
				int frame_count = 0;
				int total_frame = (*currententity)->curstate.frame;
				do
				{
					if (base->anim_next)
						base = base->anim_next;
					++frame_count;
				} while (frame_count < total_frame);
			}
		}
		else
		{
			if ((*currententity)->curstate.frame && base->alternate_anims)
				base = base->alternate_anims;

			if (!((*currententity)->curstate.effects & EF_NOANIMTEXTURES))
			{
				int reletive = (int)((*cl_time) * 10.0f) % base->anim_total;

				int loop_count = 0;

				while (base->anim_min > reletive || base->anim_max <= reletive)
				{
					base = base->anim_next;

					if (!base)
						g_pMetaHookAPI->SysError("R_TextureAnimation: broken cycle");

					if (++loop_count > 100)
						g_pMetaHookAPI->SysError("R_TextureAnimation: infinite cycle");
				}
			}
		}
	}
	else
	{
		if ((*currententity)->curstate.frame && base->alternate_anims)
			base = base->alternate_anims;

		int reletive = (int)((*cl_time) * 10.0f) % base->anim_total;

		int loop_count = 0;

		while (base->anim_min > reletive || base->anim_max <= reletive)
		{
			base = base->anim_next;

			if (!base)
				g_pMetaHookAPI->SysError("R_TextureAnimation: broken cycle");

			if (++loop_count > 100)
				g_pMetaHookAPI->SysError("R_TextureAnimation: infinite cycle");
		}
	}
	return base;
}

void R_DrawWSurfVBOAnim(wsurf_vbo_leaf_t *vboleaf, bool bUseZPrePass)
{
	for (size_t i = 0; i < vboleaf->vTextureChain[WSURF_TEXCHAIN_ANIM].size(); ++i)
	{
		auto &texchain = vboleaf->vTextureChain[WSURF_TEXCHAIN_ANIM][i];

		auto texture = R_GetAnimatedTexture(texchain.pTexture);

		int WSurfProgramState = 0;

		if (r_wsurf.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;

			GL_Bind(texture->gl_texturenum);

			R_BeginDetailTextureByGLTextureId(texture->gl_texturenum, &WSurfProgramState);
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

		if (!bUseZPrePass)
		{
			if (r_draw_reflectview)
			{
				WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
			}
			else if (g_bPortalClipPlaneEnabled[0])
			{
				WSurfProgramState |= WSURF_CLIP_ENABLED;
			}
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

		if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
		{
			WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
		}

		if (r_draw_shadowcaster)
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		wsurf_program_t prog = { 0 };
		R_UseWSurfProgram(WSurfProgramState, &prog);

		glDrawElements(GL_POLYGON, texchain.iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		R_EndDetailTexture(WSurfProgramState);

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

bool R_ShouldDrawZPrePass(void)
{
	if (r_draw_shadowcaster)
		return false;

	return (r_wsurf_zprepass->value > 0) ? true : false;
}

void R_DrawWSurfVBO(wsurf_vbo_t *modvbo, cl_entity_t *ent)
{
	static glprofile_t profile_DrawWSurfVBO;
	GL_BeginProfile(&profile_DrawWSurfVBO, "R_DrawWSurfVBO");

	entity_ubo_t EntityUBO;

	memcpy(EntityUBO.entityMatrix, r_entity_matrix, sizeof(mat4));
	memcpy(EntityUBO.color, r_entity_color, sizeof(vec4));
	EntityUBO.scrollSpeed = R_ScrollSpeed();

	glNamedBufferSubData(modvbo->hEntityUBO, 0, sizeof(EntityUBO), &EntityUBO);

	//Begin WorldSurface Rendering

	glBindBuffer(GL_ARRAY_BUFFER, r_wsurf.hSceneVBO);

	if(bUseBindless)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_TEXTURE_SSBO, r_wsurf.hWorldSSBO);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_ENTITY_UBO, modvbo->hEntityUBO);

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

	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_POSITION, 4, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_NORMAL, 4, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_S_TANGENT, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, s_tangent));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_T_TANGENT, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, t_tangent));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_TEXCOORD, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, replacetexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, normaltexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, parallaxtexcoord));
	glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, speculartexcoord));
	glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_EXTRA, 1, GL_INT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texindex));

	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	if (r_wsurf.bShadowmapTexture)
	{
		glActiveTexture(GL_TEXTURE6);

		glEnable(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r_shadow_texture.color_array);

		glActiveTexture(GL_TEXTURE0);
	}

	if (r_wsurf.bLightmapTexture)
	{
		glActiveTexture(GL_TEXTURE1);

		glEnable(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);

		glActiveTexture(GL_TEXTURE0);
	}

	bool bUseZPrePass = false;

	//Brush surfaces always use stencil = 0
	if (r_draw_opaque)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	if (modvbo->pModel == r_worldmodel)
	{
		auto leafindex = ((*r_viewleaf) - r_worldmodel->leafs);
		if (leafindex >= 0 && leafindex < (int)modvbo->vLeaves.size())
		{
			auto vboleaf = modvbo->vLeaves[leafindex];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboleaf->hEBO);

			if (R_ShouldDrawZPrePass())
			{
				glColorMask(0, 0, 0, 0);

				R_DrawWSurfVBOSolid(vboleaf);

				glColorMask(1, 1, 1, 1);

				glDepthFunc(GL_EQUAL);

				bUseZPrePass = true;
			}

			R_DrawWSurfVBOStatic(vboleaf, bUseZPrePass);
			R_DrawWSurfVBOAnim(vboleaf, bUseZPrePass);
		}
		else
		{
			g_pMetaHookAPI->SysError("R_DrawWSurfVBO: Invalid leaf index");
		}
	}
	else
	{
		if (modvbo->vLeaves.size() >= 1)
		{
			auto vboleaf = modvbo->vLeaves[0];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboleaf->hEBO);

			R_DrawWSurfVBOStatic(vboleaf, bUseZPrePass);
			R_DrawWSurfVBOAnim(vboleaf, bUseZPrePass);
		}
		else
		{
			g_pMetaHookAPI->SysError("Invalid leaf index");
		}
	}
	
	glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

	if (bUseZPrePass)
	{
		glDepthFunc(GL_LEQUAL);
	}

	//End WorldSurface Rendering

	//Collect decals and waters

	(*gDecalSurfCount) = 0;

	if (modvbo->pModel == r_worldmodel)
	{
		(*currententity) = r_worldentity;

		R_RecursiveWorldNodeVBO(r_worldmodel->nodes);
	}
	else
	{
		auto clmodel = modvbo->pModel;
		auto psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
		for (int i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			auto pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				if (pplane->type != PLANE_Z)
					continue;

				if (g_iEngineType == ENGINE_SVENGINE)
				{
					if (r_entity_mins[2] >= pplane->dist)
						continue;
				}
				else
				{
					if (r_entity_mins[2] + 1.0f >= pplane->dist)
						continue;
				}
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

	R_DrawDecals(modvbo);

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

	//Waters don't have lightmap textures

	R_DrawWaters(ent);

	GL_UseProgram(0);

	GL_EndProfile(&profile_DrawWSurfVBO);
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
	r_wsurf.bDiffuseTexture = false;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bShadowmapTexture = false;
	r_wsurf.iNumLightmapTextures = 0;
	r_wsurf.iLightmapTextureArray = 0;
	
	r_wsurf_parallax_scale = gEngfuncs.pfnRegisterVariable("r_wsurf_parallax_scale", "-0.02", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_sky_fog = gEngfuncs.pfnRegisterVariable("r_wsurf_sky_fog", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_zprepass = gEngfuncs.pfnRegisterVariable("r_wsurf_zprepass", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	R_ClearBSPEntities();
}

void R_NewMapWSurf(void)
{
	R_ClearDecalCache();
	R_ClearDetailTextureCache();

	R_LoadMapDetailTextures();
	R_LoadBaseDetailTextures();
	R_LoadBaseDecalTextures();

	R_FreeLightmapArray();
	R_FreeVertexBuffer();
	R_FreeWorldTextures();

	R_GenerateWorldTextures();
	R_GenerateVertexBuffer();
	R_GenerateLightmapArray();

	R_ClearWSurfVBOCache();
	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities, NULL);
	R_LoadExternalEntities();
	R_LoadBSPEntities();
}

void R_ShutdownWSurf(void)
{
	g_WSurfProgramTable.clear();

	R_ClearDecalCache();
	R_ClearDetailTextureCache();

	R_FreeLightmapArray();
	R_FreeVertexBuffer();
	R_ClearWSurfVBOCache();
	R_ClearBSPEntities();

	R_FreeSceneUBO();
}

void R_LoadDecalTextures(const char *pfile)
{
	char *ptext = (char *)pfile;
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

		if ((basetexture[0] == '{' || basetexture[0] == '}'))
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

		if ((sz_xscale[0] == '{' || sz_xscale[0] == '}'))
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
		int texType = WSURF_REPLACE_TEXTURE;

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

		float i_xscale = atof(sz_xscale);
		float i_yscale = atof(sz_yscale);

		detail_texture_cache_t *cache = NULL;

		auto itor = g_DecalTextureTable.find(base);

		if (itor != g_DecalTextureTable.end())
		{
			cache = itor->second;
		}
		else
		{
			cache = new detail_texture_cache_t;
			cache->basetexture = base;
			g_DecalTextureTable[base] = cache;
		}

		const char *textypeNames[] = {
			"WSURF_DIFFUSE_TEXTURE",
			"WSURF_REPLACE_TEXTURE",
			"WSURF_DETAIL_TEXTURE",
			"WSURF_NORMAL_TEXTURE",
			"WSURF_PARALLAX_TEXTURE",
			"WSURF_SPECULAR_TEXTURE",
		};

		bool textypeHasMipmap[] = {
			true,
			true,
			true,
			true,
			true
		};

		if (cache)
		{
			if (cache->tex[texType].gltexturenum)
			{
				gEngfuncs.Con_DPrintf("R_LoadDecalTextures: %s already exists for basetexture %s\n", textypeNames[texType], base.c_str());
				continue;
			}

			int width = 0, height = 0;

			std::string texturePath = "gfx/";
			texturePath += detailtexture;
			if (!V_GetFileExtension(detailtexture))
				texturePath += ".tga";

			int texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, false);
			if (!texId)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, true);
			}

			if (!texId)
			{
				gEngfuncs.Con_DPrintf("R_LoadDecalTextures: Failed to load %s as %s for basetexture %s\n", detailtexture, textypeNames[texType], base.c_str());
				continue;
			}

			cache->tex[texType].gltexturenum = texId;
			cache->tex[texType].width = width;
			cache->tex[texType].height = height;
			cache->tex[texType].scaleX = i_xscale;
			cache->tex[texType].scaleY = i_yscale;
		}
	}
}

void R_LoadBaseDecalTextures(void)
{
	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)"renderer/decal_textures.txt", 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_DPrintf("R_LoadBaseDecalTextures: No decal texture file \"renderer/decal_textures.txt\"\n");
		return;
	}

	R_LoadDecalTextures(pfile);

	gEngfuncs.COM_FreeFile(pfile);
}

void R_LoadDetailTextures(const char *pfile)
{
	char *ptext = (char *)pfile;
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

		if ((basetexture[0] == '{' || basetexture[0] == '}'))
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

		if ((sz_xscale[0] == '{' || sz_xscale[0] == '}'))
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
			gEngfuncs.Con_DPrintf("R_LoadDetailTextures: Missing basetexture %s\n", base.c_str());
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
			"WSURF_DIFFUSE_TEXTURE",
			"WSURF_REPLACE_TEXTURE",
			"WSURF_DETAIL_TEXTURE",
			"WSURF_NORMAL_TEXTURE",
			"WSURF_PARALLAX_TEXTURE",
			"WSURF_SPECULAR_TEXTURE",
		};

		bool textypeHasMipmap[] = {
			true,
			true,
			true,
			true,
			true
		};

		if (cache)
		{
			if (cache->tex[texType].gltexturenum)
			{
				gEngfuncs.Con_DPrintf("R_LoadDetailTextures: %s already exists for basetexture %s\n", textypeNames[texType], base.c_str());
				continue;
			}

			int width = 0, height = 0;

			std::string texturePath = "gfx/";
			texturePath += detailtexture;
			if (!V_GetFileExtension(detailtexture))
				texturePath += ".tga";

			int texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, false);
			if (!texId)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, true);
			}

			if (!texId)
			{
				gEngfuncs.Con_DPrintf("R_LoadDetailTextures: Failed to load %s as %s for basetexture %s\n", detailtexture, textypeNames[texType], base.c_str());
				continue;
			}

			cache->tex[texType].gltexturenum = texId;
			cache->tex[texType].width = width;
			cache->tex[texType].height = height;
			cache->tex[texType].scaleX = i_xscale;
			cache->tex[texType].scaleY = i_yscale;
		}
	}
}

void R_LoadBaseDetailTextures(void)
{
	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)"renderer/detail_textures.txt", 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_DPrintf("R_LoadBaseDetailTextures: No detail texture file \"renderer/detail_textures.txt\"\n");
		return;
	}

	R_LoadDetailTextures(pfile);

	gEngfuncs.COM_FreeFile(pfile);
}

void R_LoadMapDetailTextures(void)
{
	std::string name = gEngfuncs.pfnGetLevelName();
	name = name.substr(0, name.length() - 4);
	name += "_detail.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_DPrintf("R_LoadMapDetailTextures: No detail texture file %s\n", name.c_str());
		return;
	}

	R_LoadDetailTextures(pfile);

	gEngfuncs.COM_FreeFile(pfile);
}


void R_DrawWireFrame(brushface_t *brushface, void(*draw)(brushface_t *face))
{

}

detail_texture_cache_t *R_FindDecalTextureCache(const std::string &decalname)
{
	auto itor = g_DecalTextureTable.find(decalname);

	if (itor != g_DecalTextureTable.end())
	{
		auto cache = itor->second;

		if (cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum || 
			cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum ||
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

detail_texture_cache_t *R_FindDetailTextureCache(int texId)
{
	auto itor = g_DetailTextureTable.find(texId);

	if (itor != g_DetailTextureTable.end())
	{
		auto cache = itor->second;

		if (cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum || 
			cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum ||
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

void R_BeginDetailTextureByDetailTextureCache(detail_texture_cache_t *cache, int *WSurfProgramState)
{
	if (!r_detailtextures->value)
		return;

	if (!cache)
		return;

	if (cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum)
	{
		GL_Bind(cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_REPLACETEXTURE_ENABLED;
	}

	if (cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum)
	{
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
	}

	if (cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum)
	{
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
	}

	if (cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum)
	{
		glActiveTexture(GL_TEXTURE4);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
	}

	if (cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum)
	{
		glActiveTexture(GL_TEXTURE5);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_SPECULARTEXTURE_ENABLED;
	}
}

void R_BeginDetailTextureByGLTextureId(int gltexturenum, int *WSurfProgramState)
{
	if (!r_detailtextures->value)
		return;

	auto itor = g_DetailTextureTable.find(gltexturenum);

	if (itor != g_DetailTextureTable.end())
	{
		auto cache = itor->second;

		R_BeginDetailTextureByDetailTextureCache(cache, WSurfProgramState);
	}
}

void R_EndDetailTexture(int WSurfProgramState)
{
	bool bRestore = false;

	if (WSurfProgramState & WSURF_DETAILTEXTURE_ENABLED)
	{
		bRestore = true;

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (WSurfProgramState & WSURF_NORMALTEXTURE_ENABLED)
	{
		bRestore = true;

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (WSurfProgramState & WSURF_PARALLAXTEXTURE_ENABLED)
	{
		bRestore = true;

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (WSurfProgramState & WSURF_SPECULARTEXTURE_ENABLED)
	{
		bRestore = true;

		glActiveTexture(GL_TEXTURE5);
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
	r_flashlight_cone_texture_name.clear();
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
				g_pMetaHookAPI->SysError("R_ParseBSPEntity: EOF without closing brace");
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
				g_pMetaHookAPI->SysError("R_ParseBSPEntity: EOF without closing brace");
			}
			if (com_token[0] == '}')
			{
				g_pMetaHookAPI->SysError("R_ParseBSPEntity: closing brace without data");
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
			g_pMetaHookAPI->SysError("R_ParseBSPEntities: found %s when expecting {", com_token);
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
		//gEngfuncs.Con_Printf("R_LoadExternalEntities: No external entity file %s\n", name.c_str());

		name = "renderer/default_entity.txt";

		pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
		if (!pfile)
		{
			//gEngfuncs.Con_Printf("R_LoadExternalEntities: No default external entity file %s\n", name.c_str());

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
		float temp[4];

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
		float temp[4];
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
		float temp[4];
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
		float temp[4];
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
		float temp[4];
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
		float temp[4];
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
		float temp[4];
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
	water_control_t control;
	control.fresnelfactor[0] = 0;
	control.fresnelfactor[1] = 0;
	control.fresnelfactor[2] = 0;
	control.fresnelfactor[3] = 0;
	control.depthfactor[0] = 0;
	control.depthfactor[1] = 0;
	control.depthfactor[2] = 0;
	control.normfactor = 0;
	control.minheight = 0;
	control.maxtrans = 0;
	control.speedrate = 1;
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
		float temp[4];
		if (sscanf(fresnelfactor_string, "%f %f %f %f", &temp[0], &temp[1], &temp[2], &temp[3]) == 4)
		{
			control.fresnelfactor[0] = clamp(temp[0], 0, 999999);
			control.fresnelfactor[1] = clamp(temp[1], 0, 999999);
			control.fresnelfactor[2] = clamp(temp[2], 0, 999999);
			control.fresnelfactor[3] = clamp(temp[3], 0, 1);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"fresnelfactor\" in entity \"env_water_control\", 4 floats are required.\n");
		}
	}

	char *normfactor_string = ValueForKey(ent, "normfactor");
	if (normfactor_string)
	{
		float temp[4];
		if (sscanf(normfactor_string, "%f", &temp[0]) == 1)
		{
			control.normfactor = clamp(temp[0], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"normfactor\" in entity \"env_water_control\", 2 floats are required.\n");
		}
	}

	char *depthfactor_string = ValueForKey(ent, "depthfactor");
	if (depthfactor_string)
	{
		float temp[4];
		if (sscanf(depthfactor_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			control.depthfactor[0] = clamp(temp[0], 0, 10);
			control.depthfactor[1] = clamp(temp[1], 0, 10);
			control.depthfactor[2] = clamp(temp[2], 0, 999999);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"depthfactor\" in entity \"env_water_control\", 3 floats are required.\n");
		}
	}

	char *minheight_string = ValueForKey(ent, "minheight");
	if (minheight_string)
	{
		float temp[4];
		if (sscanf(minheight_string, "%f", &temp[0]) == 1)
		{
			control.minheight = clamp(temp[0], 0, 10000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"minheight\" in entity \"env_water_control\", 1 float is required.\n");
		}
	}

	char *maxtrans_string = ValueForKey(ent, "maxtrans");
	if (maxtrans_string)
	{
		float temp[4];
		if (sscanf(maxtrans_string, "%f", &temp[0]) == 1)
		{
			control.maxtrans = clamp(temp[0], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"maxtrans\" in entity \"env_water_control\", 1 float is required.\n");
		}
	}

	char *speedrate_string = ValueForKey(ent, "speedrate");
	if (speedrate_string)
	{
		float temp[4];
		if (sscanf(speedrate_string, "%f", &temp[0]) == 1)
		{
			control.speedrate = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"speedrate\" in entity \"env_water_control\", 1 float is required.\n");
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
		else if (!strcmp(level_string, "WATER_LEVEL_LEGACY_RIPPLE"))
		{
			control.level = WATER_LEVEL_LEGACY_RIPPLE;
		}
		else
		{
			int lv;
			if (sscanf(level_string, "%d", &lv) == 1)
			{
				control.level = clamp(lv, WATER_LEVEL_LEGACY, WATER_LEVEL_MAX - 1);
			}
			else
			{
				gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"level\" in entity \"env_water_control\", 1 integer is required\n");
			}
		}
	}

	if (control.basetexture.length())
	{
		r_water_controls.emplace_back(control);
	}
}

void R_ParseBSPEntity_Env_DynamicLight_Control(bspentity_t *ent)
{
	R_ParseMapCvarSetMapValue(r_dynlight_ambient, ValueForKey(ent, "ambient"));
	R_ParseMapCvarSetMapValue(r_dynlight_diffuse, ValueForKey(ent, "diffuse"));
	R_ParseMapCvarSetMapValue(r_dynlight_specular, ValueForKey(ent, "specular"));
	R_ParseMapCvarSetMapValue(r_dynlight_specularpow, ValueForKey(ent, "specularpow"));
}

void R_ParseBSPEntity_Env_FlashLight_Control(bspentity_t *ent)
{
	R_ParseMapCvarSetMapValue(r_flashlight_ambient, ValueForKey(ent, "ambient"));
	R_ParseMapCvarSetMapValue(r_flashlight_diffuse, ValueForKey(ent, "diffuse"));
	R_ParseMapCvarSetMapValue(r_flashlight_specular, ValueForKey(ent, "specular"));
	R_ParseMapCvarSetMapValue(r_flashlight_specularpow, ValueForKey(ent, "specularpow"));
	R_ParseMapCvarSetMapValue(r_flashlight_attachment, ValueForKey(ent, "attachment"));
	R_ParseMapCvarSetMapValue(r_flashlight_distance, ValueForKey(ent, "distance"));
	R_ParseMapCvarSetMapValue(r_flashlight_cone_cosine, ValueForKey(ent, "cone_cosine"));
	
	char *cone_texture_string = ValueForKey(ent, "cone_texture");
	if (cone_texture_string)
	{
		r_flashlight_cone_texture_name = cone_texture_string;
	}
}

void R_ParseBSPEntity_Env_HDR_Control(bspentity_t *ent)
{
	R_ParseMapCvarSetMapValue(r_hdr_adaptation, ValueForKey(ent, "adaptation"));
	R_ParseMapCvarSetMapValue(r_hdr_blurwidth, ValueForKey(ent, "blurwidth"));
	R_ParseMapCvarSetMapValue(r_hdr_darkness, ValueForKey(ent, "darkness"));
	R_ParseMapCvarSetMapValue(r_hdr_exposure, ValueForKey(ent, "exposure"));
}

void R_ParseBSPEntity_Env_Shadow_Control(bspentity_t *ent)
{
	R_ParseMapCvarSetMapValue(r_shadow_color, ValueForKey(ent, "color"));
	R_ParseMapCvarSetMapValue(r_shadow_intensity, ValueForKey(ent, "intensity"));
	R_ParseMapCvarSetMapValue(r_shadow_angles, ValueForKey(ent, "angles"));
	R_ParseMapCvarSetMapValue(r_shadow_distfade, ValueForKey(ent, "distfade"));
	R_ParseMapCvarSetMapValue(r_shadow_lumfade, ValueForKey(ent, "lumfade"));
	R_ParseMapCvarSetMapValue(r_shadow_high_distance, ValueForKey(ent, "high_distance"));
	R_ParseMapCvarSetMapValue(r_shadow_high_scale, ValueForKey(ent, "high_scale"));
	R_ParseMapCvarSetMapValue(r_shadow_medium_distance, ValueForKey(ent, "medium_distance"));
	R_ParseMapCvarSetMapValue(r_shadow_medium_scale, ValueForKey(ent, "medium_scale"));
	R_ParseMapCvarSetMapValue(r_shadow_low_distance, ValueForKey(ent, "low_distance"));
	R_ParseMapCvarSetMapValue(r_shadow_low_scale, ValueForKey(ent, "low_scale"));
}

void R_ParseBSPEntity_Env_SSR_Control(bspentity_t *ent)
{
	R_ParseMapCvarSetMapValue(r_ssr_ray_step, ValueForKey(ent, "ray_step"));
	R_ParseMapCvarSetMapValue(r_ssr_iter_count, ValueForKey(ent, "iter_count"));
	R_ParseMapCvarSetMapValue(r_ssr_distance_bias, ValueForKey(ent, "distance_bias"));
	R_ParseMapCvarSetMapValue(r_ssr_exponential_step, ValueForKey(ent, "exponential_step"));
	R_ParseMapCvarSetMapValue(r_ssr_adaptive_step, ValueForKey(ent, "adaptive_step"));
	R_ParseMapCvarSetMapValue(r_ssr_binary_search, ValueForKey(ent, "binary_search"));
	R_ParseMapCvarSetMapValue(r_ssr_fade, ValueForKey(ent, "fade"));
}

void R_LoadBSPEntities(void)
{
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

		else if (!strcmp(classname, "env_dynamiclight_control"))
		{
			R_ParseBSPEntity_Env_DynamicLight_Control(ent);
		}

		else if (!strcmp(classname, "env_flashlight_control"))
		{
			R_ParseBSPEntity_Env_FlashLight_Control(ent);
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
			g_pMetaHookAPI->SysError("Too many decal surfaces!\n");
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

		return;
	}

	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
	{
		dot = (*r_refdef.vieworg)[0] - plane->dist;
		break;
	}

	case PLANE_Y:
	{
		dot = (*r_refdef.vieworg)[1] - plane->dist;
		break;
	}

	case PLANE_Z:
	{
		dot = (*r_refdef.vieworg)[2] - plane->dist;
		break;
	}

	default:
	{
		dot = DotProduct((*r_refdef.vieworg), plane->normal) - plane->dist;
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

			}
			else if (surf->flags & SURF_DRAWTURB)
			{
				EmitWaterPolys(surf, 0);
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
	qboolean rotated;

	(*currententity) = e;
	(*currenttexture) = -1;

	auto clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;

		for (int i = 0; i < 3; i++)
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

	VectorSubtract((*r_refdef.vieworg), e->origin, modelorg);

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
			int max_dlights = EngineGetMaxDLight();

			if (g_iEngineType != ENGINE_SVENGINE)
			{
				if (gl_flashblend && gl_flashblend->value)
					goto skip_marklight;
			}

			for (int k = 0; k < max_dlights; k++)
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

	if(R_ShouldRenderShadowScene() && r_draw_opaque)
		r_wsurf.bShadowmapTexture = true;

	auto modvbo = R_PrepareWSurfVBO(clmodel);

	R_DrawWSurfVBO(modvbo, e);

	glDepthMask(1);
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);
	glDisable(GL_BLEND);

	//No stencil write later
	glDisable(GL_STENCIL_TEST);
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
	SceneUBO.viewport[0] = glwidth;
	SceneUBO.viewport[1] = glheight;
	SceneUBO.viewport[2] = MAX_NUM_NODES * glwidth * glheight;
	SceneUBO.viewport[3] = 0;
	memcpy(SceneUBO.frustumpos[0], r_frustum_origin[0], sizeof(vec3_t));
	memcpy(SceneUBO.frustumpos[1], r_frustum_origin[1], sizeof(vec3_t));
	memcpy(SceneUBO.frustumpos[2], r_frustum_origin[2], sizeof(vec3_t));
	memcpy(SceneUBO.frustumpos[3], r_frustum_origin[3], sizeof(vec3_t));
	memcpy(SceneUBO.viewpos, (*r_refdef.vieworg), sizeof(vec3_t));
	memcpy(SceneUBO.vpn, vpn, sizeof(vec3_t));
	memcpy(SceneUBO.vright, vright, sizeof(vec3_t));
	memcpy(SceneUBO.vup, vup, sizeof(vec3_t));

	vec3_t vforward;
	gEngfuncs.pfnAngleVectors(r_shadow_angles->GetValues(), vforward, NULL, NULL);
	memcpy(SceneUBO.shadowDirection, vforward, sizeof(vec3_t));
	memcpy(SceneUBO.shadowColor, r_shadow_color->GetValues(), sizeof(vec3_t));
	SceneUBO.shadowColor[3] = r_shadow_intensity->GetValue();
	memcpy(SceneUBO.shadowFade, r_shadow_distfade->GetValues(), sizeof(vec2_t));
	memcpy(&SceneUBO.shadowFade[2], r_shadow_lumfade->GetValues(), sizeof(vec2_t));

	//normal[0] * x+ normal[1] * y+ normal[2] * z = normal[0] * vert[0] +normal[1] * vert[1] +normal[2] * vert[2]

	if (r_draw_reflectview)
	{
		float equation[4] = { curwater->normal[0], curwater->normal[1], curwater->normal[2], -curwater->plane };
		memcpy(SceneUBO.clipPlane, equation, sizeof(vec4_t));
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		memcpy(SceneUBO.clipPlane, g_PortalClipPlane[0], sizeof(vec4_t));
	}

	//Fog colors are converted to linear space before use.
	memcpy(SceneUBO.fogColor, r_fog_color, sizeof(vec4_t));
	GammaToLinear(SceneUBO.fogColor);

	SceneUBO.fogStart = r_fog_control[0];
	SceneUBO.fogEnd = r_fog_control[1];
	SceneUBO.fogDensity = r_fog_control[2];
	SceneUBO.time = (*cl_time);

	float r_g = 1.0f / v_gamma->value;

	float r_g3;
	if (v_brightness->value <= 0.0f)
		r_g3 = 0.125f;
	else if (v_brightness->value > 1.0f)
		r_g3 = 0.05f;
	else
		r_g3 = 0.125f - (v_brightness->value * v_brightness->value) * 0.075f;

	SceneUBO.r_g = r_g;
	SceneUBO.r_g3 = r_g3;
	SceneUBO.v_brightness = v_brightness->value;
	SceneUBO.v_lightgamma = v_lightgamma->value;
	SceneUBO.v_lambert = v_lambert->value;
	SceneUBO.v_gamma = v_gamma->value;
	SceneUBO.v_texgamma = v_texgamma->value;
	SceneUBO.z_near = r_znear;
	SceneUBO.z_far = r_zfar;
	SceneUBO.r_alpha_shift = r_alpha_shift->value;
	SceneUBO.r_additive_shift = r_additive_shift->value;

	glNamedBufferSubData(r_wsurf.hSceneUBO, 0, sizeof(SceneUBO), &SceneUBO);
}

void R_DrawWorld(void)
{
	r_draw_opaque = true;

	const float r_identity_matrix[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	memcpy(r_entity_matrix, r_identity_matrix, sizeof(r_entity_matrix));
	r_entity_color[0] = 1;
	r_entity_color[1] = 1;
	r_entity_color[2] = 1;
	r_entity_color[3] = 1;

	R_BeginRenderGBuffer();

	VectorCopy((*r_refdef.vieworg), modelorg);

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

	r_wsurf.bDiffuseTexture = true;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bShadowmapTexture = false;

	//Setup shadow

	//Shall we put this in shadow pass?
	if (R_ShouldRenderShadowScene())
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

	//Skybox always use stencil = 0xFC

	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0xFC, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	R_DrawSkyBox();

	if (!(r_draw_reflectview && curwater->level == WATER_LEVEL_REFLECT_SKYBOX))
	{
		r_wsurf.bDiffuseTexture = true;
		r_wsurf.bLightmapTexture = true;

		auto modvbo = R_PrepareWSurfVBO(r_worldmodel);

		R_DrawWSurfVBO(modvbo, (*currententity));
	}

	GL_DisableMultitexture();

	//No stencil write later
	glDisable(GL_STENCIL_TEST);
}