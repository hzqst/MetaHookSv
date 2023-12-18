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

std::unordered_map <program_state_t, wsurf_program_t> g_WSurfProgramTable;

std::unordered_map <int, detail_texture_cache_t *> g_DetailTextureTable;

std::unordered_map <std::string, detail_texture_cache_t *> g_DecalTextureTable;

std::vector<wsurf_vbo_t *> g_WSurfVBOCache;

void R_ClearWSurfVBOCache(void)
{
	for (size_t i = 0;i < g_WSurfVBOCache.size(); ++i)
	{
		if (g_WSurfVBOCache[i])
		{
			auto &VBOCache = g_WSurfVBOCache[i];

			if (VBOCache->hEntityUBO)
			{
				GL_DeleteBuffer(VBOCache->hEntityUBO);
				VBOCache->hEntityUBO = 0;
			}

			for (size_t j = 0; j < VBOCache->vLeaves.size(); ++j)
			{
				auto VBOLeaf = VBOCache->vLeaves[j];

				if (VBOLeaf->hVAO)
				{
					GL_DeleteVAO(VBOLeaf->hVAO);
					VBOLeaf->hVAO = 0;
				}

				if (VBOLeaf->hEBO)
				{
					GL_DeleteBuffer(VBOLeaf->hEBO);
					VBOLeaf->hEBO = 0;
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

			VBOCache->vLeaves.clear();
			
			delete g_WSurfVBOCache[i];
		}
	}
	g_WSurfVBOCache.clear();
}

const program_state_mapping_t s_WSurfProgramStateName[] = {
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
{ WSURF_ALPHA_BLEND_ENABLED			,"WSURF_ALPHA_BLEND_ENABLED"},
{ WSURF_ADDITIVE_BLEND_ENABLED		,"WSURF_ADDITIVE_BLEND_ENABLED"},
{ WSURF_OIT_BLEND_ENABLED			,"WSURF_OIT_BLEND_ENABLED"},
{ WSURF_GAMMA_BLEND_ENABLED			,"WSURF_GAMMA_BLEND_ENABLED"},
{ WSURF_FULLBRIGHT_ENABLED			,"WSURF_FULLBRIGHT_ENABLED"},
{ WSURF_COLOR_FILTER_ENABLED		,"WSURF_COLOR_FILTER_ENABLED"},
{ WSURF_LIGHTMAP_INDEX_0_ENABLED	,"WSURF_LIGHTMAP_INDEX_0_ENABLED"},
{ WSURF_LIGHTMAP_INDEX_1_ENABLED	,"WSURF_LIGHTMAP_INDEX_1_ENABLED"},
{ WSURF_LIGHTMAP_INDEX_2_ENABLED	,"WSURF_LIGHTMAP_INDEX_2_ENABLED"},
{ WSURF_LIGHTMAP_INDEX_3_ENABLED	,"WSURF_LIGHTMAP_INDEX_3_ENABLED"},
{ WSURF_LEGACY_DLIGHT_ENABLED		,"WSURF_LEGACY_DLIGHT_ENABLED"},
{ WSURF_ALPHA_SOLID_ENABLED			,"WSURF_ALPHA_SOLID_ENABLED"},
};

void R_SaveWSurfProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto &p : g_WSurfProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/wsurf_cache.txt", states, s_WSurfProgramStateName, _ARRAYSIZE(s_WSurfProgramStateName));
}

void R_LoadWSurfProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/wsurf_cache.txt", s_WSurfProgramStateName, _ARRAYSIZE(s_WSurfProgramStateName), [](program_state_t state) {

		R_UseWSurfProgram(state, NULL);

	});
}

void R_UseWSurfProgram(program_state_t state, wsurf_program_t *progOutput)
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

		if ((state & WSURF_BINDLESS_ENABLED) && bUseBindless)
			defs << "#define BINDLESS_ENABLED\n";

		if (state & WSURF_SKYBOX_ENABLED)
			defs << "#define SKYBOX_ENABLED\n";

		if (state & WSURF_DECAL_ENABLED)
			defs << "#define DECAL_ENABLED\n";

		if (state & WSURF_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		if (state & WSURF_CLIP_WATER_ENABLED)
			defs << "#define CLIP_WATER_ENABLED\n";

		if (state & WSURF_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & WSURF_ADDITIVE_BLEND_ENABLED)
			defs << "#define ADDITIVE_BLEND_ENABLED\n";

		if ((state & WSURF_OIT_BLEND_ENABLED) && bUseOITBlend)
			defs << "#define OIT_BLEND_ENABLED\n";

		if (state & WSURF_GAMMA_BLEND_ENABLED)
			defs << "#define GAMMA_BLEND_ENABLED\n";

		if (state & WSURF_FULLBRIGHT_ENABLED)
			defs << "#define FULLBRIGHT_ENABLED\n";

		if (state & WSURF_COLOR_FILTER_ENABLED)
			defs << "#define COLOR_FILTER_ENABLED\n";

		if (state & WSURF_LIGHTMAP_INDEX_0_ENABLED)
			defs << "#define LIGHTMAP_INDEX_0_ENABLED\n";

		if (state & WSURF_LIGHTMAP_INDEX_1_ENABLED)
			defs << "#define LIGHTMAP_INDEX_1_ENABLED\n";

		if (state & WSURF_LIGHTMAP_INDEX_2_ENABLED)
			defs << "#define LIGHTMAP_INDEX_2_ENABLED\n";

		if (state & WSURF_LIGHTMAP_INDEX_3_ENABLED)
			defs << "#define LIGHTMAP_INDEX_3_ENABLED\n";

		if (state & WSURF_LEGACY_DLIGHT_ENABLED)
			defs << "#define LEGACY_DLIGHT_ENABLED\n";

		if (state & WSURF_ALPHA_SOLID_ENABLED)
			defs << "#define ALPHA_SOLID_ENABLED\n";

		if (glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define NV_BINDLESS_ENABLED\n";

		else if (glewIsSupported("GL_ARB_gpu_shader_int64"))
			defs << "#define INT64_BINDLESS_ENABLED\n";

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
	if (r_wsurf.hDecalVAO)
	{
		GL_DeleteVAO(r_wsurf.hDecalVAO);
		r_wsurf.hDecalVAO = 0;
	}

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

	if (r_wsurf.hDLightUBO)
	{
		GL_DeleteBuffer(r_wsurf.hDLightUBO);
		r_wsurf.hDLightUBO = 0;
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

	if (r_wsurf.hDetailSkyboxSSBO)
	{
		GL_DeleteBuffer(r_wsurf.hDetailSkyboxSSBO);
		r_wsurf.hDetailSkyboxSSBO = 0;
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

void R_FreeLightmapTextures(void)
{
	if (r_wsurf.iLightmapTextureArray)
	{
		GL_DeleteTexture(r_wsurf.iLightmapTextureArray);
		r_wsurf.iLightmapTextureArray = 0;
	}
}

void R_FreeBindlessTexturesForWorld(void)
{
	if (bUseBindless)
	{
		for (auto handle : r_wsurf.vBindlessTextureHandles)
		{
			if (glIsTextureHandleResidentARB(handle))
			{
				glMakeTextureHandleNonResidentARB(handle);
			}
		}
		r_wsurf.vBindlessTextureHandles.clear();
	}

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

void R_RecursiveFindLeaves(mnode_t *node, std::set<mleaf_t *> &vLeafs)
{
	if (node->contents < 0)
	{
		auto pleaf = (mleaf_t *)node;

		vLeafs.emplace(pleaf);
		return;
	}

	R_RecursiveFindLeaves(node->children[0], vLeafs);

	R_RecursiveFindLeaves(node->children[1], vLeafs);
}

void R_MarkPVSForLeaf(mleaf_t *leaf)
{
	(*r_visframecount)++;

	auto vis = Mod_LeafPVS(leaf, r_worldmodel);

	for (int j = 0; j < r_worldmodel->numleafs; j++)
	{
		if (vis[j >> 3] & (1 << (j & 7)))
		{
			auto node = (mnode_t *)&r_worldmodel->leafs[j + 1];

			do
			{
				if (node->visframe == (*r_visframecount))
					break;

				node->visframe = (*r_visframecount);
				node = node->parent;

			} while (node);
		}
	}
}

void R_RecursiveMarkSurfaces(mnode_t *node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	//r_visframecount is updated only when you encounter a new leaf
	//while r_framecount is updated every new frame
	if (node->visframe != (*r_visframecount))
		return;

	if (node->contents < 0)
	{
		auto pleaf = (mleaf_t *)node;

		auto mark = pleaf->firstmarksurface;
		auto c = pleaf->nummarksurfaces;

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

	R_RecursiveMarkSurfaces(node->children[0]);

	R_RecursiveMarkSurfaces(node->children[1]);
}

void R_CollectWaterVBO(msurface_t *surf, int direction, wsurf_vbo_leaf_t *leaf)
{
	auto WaterVBO = R_CreateWaterVBO(surf, direction, leaf);

	if (std::find(leaf->vWaterVBO.begin(), leaf->vWaterVBO.end(), WaterVBO) == leaf->vWaterVBO.end())
	{
		leaf->vWaterVBO.emplace_back(WaterVBO);
	}
}

void R_RecursiveLinkTextureChain(mnode_t *node, wsurf_vbo_leaf_t *leaf)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	//r_visframecount is updated only when you encounter a new leaf
	//while r_framecount is updated every new frame
	if (node->visframe != (*r_visframecount))
		return;

	if (node->contents < 0)
		return;

	R_RecursiveLinkTextureChain(node->children[0], leaf);

	auto c = node->numsurfaces;

	if (c)
	{
		int surf_index = 0;
		
		for (; c; c--, surf_index ++)
		{
			msurface_t* surf;

			if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			{
				surf = (((msurface_hl25_t*)r_worldmodel->surfaces) + node->firstsurface + surf_index);
			}
			else
			{
				surf = r_worldmodel->surfaces + node->firstsurface + surf_index;
			}

			// Filter out invisible surfaces
			if (surf->visframe != (*r_framecount))
				continue;

			if (surf->flags & SURF_DRAWSKY)
			{
				continue;
			}
			if (surf->flags & SURF_DRAWTURB)
			{
				R_CollectWaterVBO(surf, 0, leaf);
				continue;
			}

			surf->texturechain = surf->texinfo->texture->texturechain;
			surf->texinfo->texture->texturechain = surf;
		}
	}

	R_RecursiveLinkTextureChain(node->children[1], leaf);
}

void R_BrushModelLinkTextureChain(model_t *mod, wsurf_vbo_leaf_t *leaf)
{
	msurface_t* surf;

	for (int i = 0; i < mod->nummodelsurfaces; i++)
	{
		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			surf = (((msurface_hl25_t*)mod->surfaces) + mod->firstmodelsurface + i);
		}
		else
		{
			surf = mod->surfaces + mod->firstmodelsurface + i;
		}


		auto pplane = surf->plane;

		if (surf->flags & SURF_DRAWSKY)
		{
			continue;
		}
		if (surf->flags & SURF_DRAWTURB)
		{
			//Skip non-Z planes
			if (pplane->type != PLANE_Z)
				continue;

			//Skip bottom ?
			
			R_CollectWaterVBO(surf, 0, leaf);
			R_CollectWaterVBO(surf, 1, leaf);
			continue;
		}

		surf->texturechain = surf->texinfo->texture->texturechain;
		surf->texinfo->texture->texturechain = surf;
	}
}

void R_GenerateIndicesForTexChain(msurface_t *s, brushtexchain_t *texchain, std::vector<GLuint> &vIndicesBuffer)
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

void R_SortTextureChain(wsurf_vbo_leaf_t *vboleaf, int iTexchainId)
{
	vboleaf->vTextureChain[iTexchainId].shrink_to_fit();

	for (size_t i = 0; i < vboleaf->vTextureChain[iTexchainId].size(); ++i)
	{
		auto &texchain = vboleaf->vTextureChain[iTexchainId][i];

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

	std::sort(vboleaf->vTextureChain[iTexchainId].begin(), vboleaf->vTextureChain[iTexchainId].end(), [](const brushtexchain_t &a, const brushtexchain_t &b) {
		return b.iDetailTextureFlags > a.iDetailTextureFlags;
	});
}

void R_GenerateDrawBatch(wsurf_vbo_leaf_t *vboleaf, int iTexchainId, int iDrawBatchId)
{
	int iDetailTextureFlags = -1;
	wsurf_vbo_batch_t *batch = NULL;

	for (size_t i = 0; i < vboleaf->vTextureChain[iTexchainId].size(); ++i)
	{
		auto &texchain = vboleaf->vTextureChain[iTexchainId][i];

		if (texchain.iDetailTextureFlags != iDetailTextureFlags)
		{
			if (batch)
			{
				batch->vStartIndex.shrink_to_fit();
				batch->vIndiceCount.shrink_to_fit();
				vboleaf->vDrawBatch[iDrawBatchId].emplace_back(batch);
				batch = NULL;
			}

			iDetailTextureFlags = texchain.iDetailTextureFlags;
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
		vboleaf->vDrawBatch[iDrawBatchId].emplace_back(batch);
		batch = NULL;
	}
}

void R_GenerateWaterStorages(model_t *mod, wsurf_vbo_leaf_t *vboleaf)
{
	for (size_t i = 0; i < vboleaf->vWaterVBO.size(); ++i)
	{
		auto &WaterVBO = vboleaf->vWaterVBO[i];

		if (WaterVBO->vIndicesBuffer)
		{
			WaterVBO->hEBO = GL_GenBuffer();

			GL_UploadDataToEBO(WaterVBO->hEBO, sizeof(unsigned int) * WaterVBO->vIndicesBuffer->size(), WaterVBO->vIndicesBuffer->data());

			WaterVBO->hVAO = GL_GenVAO();

			GL_BindVAO(WaterVBO->hVAO);
			GL_BindStatesForVAO(WaterVBO->hVAO, r_wsurf.hSceneVBO, WaterVBO->hEBO,
			[]() {
				glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_POSITION);
				glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMAL);
				glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
				glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
				glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
				glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_POSITION, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
				glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_NORMAL, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
				glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_S_TANGENT, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, s_tangent));
				glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_T_TANGENT, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, t_tangent));
				glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_TEXCOORD, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			},
			[]() {
				glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_POSITION);
				glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMAL);
				glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
				glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
				glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
			});

			WaterVBO->iIndicesCount = WaterVBO->vIndicesBuffer->size();

			delete WaterVBO->vIndicesBuffer;
			WaterVBO->vIndicesBuffer = NULL;
		}
	}
}

void R_GenerateTexChain(model_t *mod, wsurf_vbo_leaf_t *vboleaf, std::vector<GLuint> &vIndicesBuffer)
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
							//gRefFuncs.R_TextureAnimation(s);
							for (auto tu = 0; tu < 20; tu++)
							{
								for (auto tv = 0; tv < 20; tv++)
								{
									(*rtable)[tu][tv] = gEngfuncs.pfnRandomLong(0, 0x7FFF);
								}
							}
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

/*
Generate leaf array for brush model
*/

void R_GenerateBufferStorage(model_t *mod, wsurf_vbo_t *modvbo)
{
	std::vector<GLuint> vIndicesBuffer;

	if (mod == r_worldmodel)
	{
		std::set<mleaf_t *> vPossibleLeafs;
		R_RecursiveFindLeaves(mod->nodes, vPossibleLeafs);

		modvbo->vLeaves.resize(vPossibleLeafs.size());

		for (auto &leaf : vPossibleLeafs)
		{
			int leafindex = leaf - mod->leafs;
			
			(*r_framecount)++;

			R_MarkPVSForLeaf(leaf);

			auto vboleaf = new wsurf_vbo_leaf_t;

			R_RecursiveMarkSurfaces(mod->nodes);
			R_RecursiveLinkTextureChain(mod->nodes, vboleaf);

			R_GenerateWaterStorages(mod, vboleaf);

			R_GenerateTexChain(mod, vboleaf, vIndicesBuffer);

			R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_STATIC);
			R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_ANIM);

			R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_STATIC);
			R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_SOLID);
			R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_ANIM, WSURF_DRAWBATCH_SOLID);

			vboleaf->hEBO = GL_GenBuffer();
			GL_UploadDataToEBO(vboleaf->hEBO, sizeof(GLuint) * vIndicesBuffer.size(), vIndicesBuffer.data());

			vboleaf->hVAO = GL_GenVAO();
			GL_BindStatesForVAO(vboleaf->hVAO, r_wsurf.hSceneVBO, vboleaf->hEBO,
			[]() {
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
				glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXINDEX);
				glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_STYLES);
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
				glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_TEXINDEX, 1, GL_INT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texindex));
				glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_STYLES, 4, GL_UNSIGNED_BYTE, sizeof(brushvertex_t), OFFSET(brushvertex_t, styles));
			},
			[]() {
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
				glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXINDEX);
				glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_STYLES);
			});

			vIndicesBuffer.clear();

			modvbo->vLeaves[leafindex] = vboleaf;
		}
 	}
	else
	{
		auto vboleaf = new wsurf_vbo_leaf_t;

		R_BrushModelLinkTextureChain(mod, vboleaf);

		R_GenerateWaterStorages(mod, vboleaf);

		R_GenerateTexChain(mod, vboleaf, vIndicesBuffer);

		R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_STATIC);
		R_SortTextureChain(vboleaf, WSURF_TEXCHAIN_ANIM);

		R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_STATIC);
		R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_STATIC, WSURF_DRAWBATCH_SOLID);
		R_GenerateDrawBatch(vboleaf, WSURF_TEXCHAIN_ANIM, WSURF_DRAWBATCH_SOLID);

		vboleaf->hEBO = GL_GenBuffer();
		GL_UploadDataToEBO(vboleaf->hEBO, sizeof(GLuint)* vIndicesBuffer.size(), vIndicesBuffer.data());

		vboleaf->hVAO = GL_GenVAO();
		GL_BindStatesForVAO(vboleaf->hVAO, r_wsurf.hSceneVBO, vboleaf->hEBO,
		[]() {
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
			glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXINDEX);
			glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_STYLES);
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
			glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_TEXINDEX, 1, GL_INT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texindex));
			glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_STYLES, 4, GL_UNSIGNED_BYTE, sizeof(brushvertex_t), OFFSET(brushvertex_t, styles));
		},
		[]() {
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
			glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXINDEX);
			glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_STYLES);
		});

		modvbo->vLeaves.resize(1);
		modvbo->vLeaves[0] = vboleaf;
	}

	modvbo->hEntityUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, modvbo->hEntityUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(entity_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void R_CreateBindlessTexturesForWorld(void)
{
	if (!r_worldmodel)
		return;

	if (bUseBindless)
	{
		std::vector <GLuint64> ssbo;

		ssbo.resize(r_worldmodel->numtextures * WSURF_MAX_TEXTURE);

		for (int i = 0; i < r_worldmodel->numtextures; i++)
		{
			auto t = r_worldmodel->textures[i];

			if (!t)
				continue;

			if (!t->gl_texturenum)
				continue;

			auto handle = glGetTextureHandleARB(t->gl_texturenum);

			if (!glIsTextureHandleResidentARB(handle))
			{
				glMakeTextureHandleResidentARB(handle);
			}

			r_wsurf.vBindlessTextureHandles.emplace_back(handle);

			ssbo[i * WSURF_MAX_TEXTURE + WSURF_DIFFUSE_TEXTURE] = handle;
			
			for (int j = WSURF_REPLACE_TEXTURE; j < WSURF_MAX_TEXTURE; ++j)
			{
				ssbo[i * WSURF_MAX_TEXTURE + j] = 0;
			}

			auto pcache = R_FindDetailTextureCache(t->gl_texturenum);

			if (pcache)
			{
				for (int j = WSURF_REPLACE_TEXTURE; j < WSURF_MAX_TEXTURE; ++j)
				{
					if (pcache->tex[j].gltexturenum)
					{
						auto handle = glGetTextureHandleARB(pcache->tex[j].gltexturenum);
						if (!glIsTextureHandleResidentARB(handle))
						{
							glMakeTextureHandleResidentARB(handle);
						}

						r_wsurf.vBindlessTextureHandles.emplace_back(handle);

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

#if 0
void Mod_LoadBrushModel(model_t *mod, void *buffer)
{
	auto current_loadmodel = (*loadmodel);

	gRefFuncs.Mod_LoadBrushModel(mod, buffer);
}
#endif

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

	r_wsurf.vFaceBuffer.resize(r_worldmodel->numsurfaces);

	for(int i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		msurface_t* surf;

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			surf = (((msurface_hl25_t *)r_worldmodel->surfaces) + i);
		}
		else
		{
			surf = r_worldmodel->surfaces + i;
		}

		auto poly = surf->polys;

		poly->flags = i;

		brushface_t *brushface = &r_wsurf.vFaceBuffer[iNumFaces];

		VectorCopy(surf->texinfo->vecs[0], brushface->s_tangent);
		VectorCopy(surf->texinfo->vecs[1], brushface->t_tangent);
		VectorNormalize(brushface->s_tangent);
		VectorNormalize(brushface->t_tangent);
		VectorCopy(surf->plane->normal, brushface->normal);
		brushface->index = i;
		brushface->flags = surf->flags;

		if (surf->flags & SURF_PLANEBACK)
			VectorInverse(brushface->normal);

		if (surf->lightmaptexturenum + 1 > r_wsurf.iNumLightmapTextures)
			r_wsurf.iNumLightmapTextures = surf->lightmaptexturenum + 1;

		auto ptexture = surf->texinfo ? surf->texinfo->texture : NULL;

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
			for (poly = surf->polys; poly; poly = poly->next)
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
					Vertexes[0].lightmaptexcoord[2] = surf->lightmaptexturenum;
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
					memcpy(&Vertexes[0].styles, surf->styles, sizeof(surf->styles));

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

			for (poly = surf->polys; poly; poly = poly->next)
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
					Vertexes[j].lightmaptexcoord[2] = surf->lightmaptexturenum;
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
					memcpy(&Vertexes[j].styles, surf->styles, sizeof(surf->styles));
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
					Vertexes[2].texcoord[2] = (ptexture && (surf->flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;
					Vertexes[2].lightmaptexcoord[0] = v[5];
					Vertexes[2].lightmaptexcoord[1] = v[6];
					Vertexes[2].lightmaptexcoord[2] = surf->lightmaptexturenum;
					Vertexes[2].detailtexcoord[0] = detailScale[0];
					Vertexes[2].detailtexcoord[1] = detailScale[1];
					Vertexes[2].normaltexcoord[0] = normalScale[0];
					Vertexes[2].normaltexcoord[1] = normalScale[1];
					Vertexes[2].parallaxtexcoord[0] = parallaxScale[0];
					Vertexes[2].parallaxtexcoord[1] = parallaxScale[1];
					Vertexes[2].speculartexcoord[0] = specularScale[0];
					Vertexes[2].speculartexcoord[1] = specularScale[1];
					Vertexes[2].texindex = R_FindTextureIdByTexture(ptexture);
					memcpy(&Vertexes[2].styles, surf->styles, sizeof(surf->styles));

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

	r_wsurf.hDLightUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, r_wsurf.hDLightUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(dlight_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_DLIGHT_UBO, r_wsurf.hDLightUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//3.5 MBytes of VRAM
	r_wsurf.hDecalVBO = GL_GenBuffer();
	GL_UploadDataToVBO(r_wsurf.hDecalVBO, sizeof(decalvertex_t) * MAX_DECALVERTS * MAX_DECALS, NULL);

	r_wsurf.hDecalVAO = GL_GenVAO();
	GL_BindStatesForVAO(r_wsurf.hDecalVAO, r_wsurf.hDecalVBO, 0,
	[]() {
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
		glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_DECALINDEX);
		glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_STYLES);
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
		glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_DECALINDEX, 1, GL_INT, sizeof(decalvertex_t), OFFSET(decalvertex_t, decalindex));
		glVertexAttribIPointer(VERTEX_ATTRIBUTE_INDEX_STYLES, 4, GL_UNSIGNED_BYTE, sizeof(decalvertex_t), OFFSET(decalvertex_t, styles));
	},
	[]() {
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
		glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_DECALINDEX);
		glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_STYLES);
	});
	
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

		r_wsurf.hDetailSkyboxSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_wsurf.hDetailSkyboxSSBO);
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
	program_state_t WSurfProgramState = 0;

	if (R_IsRenderingGBuffer())
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

void R_DrawWSurfVBOBegin(wsurf_vbo_leaf_t* vboleaf)
{
	GL_BindVAO(vboleaf->hVAO);

	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
}

void R_DrawWSurfVBOEnd()
{
	glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	GL_BindVAO(0);
}

void R_DrawWSurfVBOStatic(wsurf_vbo_leaf_t * vboleaf, bool bUseZPrePass)
{
	if(bUseBindless && gl_bindless->value)
	{
		program_state_t WSurfProgramState = WSURF_BINDLESS_ENABLED;

		if (r_wsurf.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
		}

		if (r_wsurf.bLightmapTexture)
		{
			WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;

			if (r_fullbright->value || !r_worldmodel->lightdata)
			{
				WSurfProgramState |= WSURF_FULLBRIGHT_ENABLED;
			}

			if (*filterMode != 0)
			{
				WSurfProgramState |= WSURF_COLOR_FILTER_ENABLED;
			}

			if (!r_light_dynamic->value && r_wsurf.iLightmapLegacyDLights)
			{
				WSurfProgramState |= WSURF_LEGACY_DLIGHT_ENABLED;
			}

			if (r_wsurf.iLightmapUsedBits & (1 << 0))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_0_ENABLED;
			}
			if (r_wsurf.iLightmapUsedBits & (1 << 1))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_1_ENABLED;
			}
			if (r_wsurf.iLightmapUsedBits & (1 << 2))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_2_ENABLED;
			}
			if (r_wsurf.iLightmapUsedBits & (1 << 3))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_3_ENABLED;
			}
		}

		if (r_wsurf.bShadowmapTexture)
		{
			WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

			if (shadow_numvisedicts[0] > 0)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_HIGH_ENABLED;
			}
			if (shadow_numvisedicts[1] > 0)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_MEDIUM_ENABLED;
			}
			if (shadow_numvisedicts[2] > 0)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_LOW_ENABLED;
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

		if (!R_IsRenderingGBuffer())
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
		}

		if (r_draw_shadowcaster)
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		if (R_IsRenderingGBuffer())
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		}

		if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
		{
			if ((*currententity)->curstate.rendermode == kRenderTransAdd || (*currententity)->curstate.rendermode == kRenderGlow)
				WSurfProgramState |= WSURF_ADDITIVE_BLEND_ENABLED;
			else
				WSurfProgramState |= WSURF_ALPHA_BLEND_ENABLED;
		}

		if ((*currententity)->curstate.rendermode == kRenderTransAlpha)
		{
			WSurfProgramState |= WSURF_ALPHA_SOLID_ENABLED;
		}

		if (r_draw_gammablend)
		{
			WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (WSurfProgramState & (WSURF_ALPHA_BLEND_ENABLED | WSURF_ADDITIVE_BLEND_ENABLED)))
		{
			WSurfProgramState |= WSURF_OIT_BLEND_ENABLED;
		}

		auto &drawBatches = vboleaf->vDrawBatch[WSURF_DRAWBATCH_STATIC];
		for (size_t i = 0; i < drawBatches.size(); ++i)
		{
			auto &batch = drawBatches[i];

			program_state_t WSurfProgramStateBatch = WSurfProgramState;

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
		for (size_t i = 0; i < vboleaf->vTextureChain[WSURF_TEXCHAIN_STATIC].size(); ++i)
		{
			auto &texchain = vboleaf->vTextureChain[WSURF_TEXCHAIN_STATIC][i];

			auto base = texchain.pTexture;

			program_state_t WSurfProgramState = 0;

			if (r_wsurf.bDiffuseTexture)
			{
				WSurfProgramState |= WSURF_DIFFUSE_ENABLED;

				GL_Bind(base->gl_texturenum);

				R_BeginDetailTextureByDetailTextureCache(texchain.pDetailTextureCache, &WSurfProgramState);
			}

			if (r_wsurf.bLightmapTexture)
			{
				WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;

				if (r_fullbright->value || !r_worldmodel->lightdata)
				{
					WSurfProgramState |= WSURF_FULLBRIGHT_ENABLED;
				}

				if (*filterMode != 0)
				{
					WSurfProgramState |= WSURF_COLOR_FILTER_ENABLED;
				}

				if (!r_light_dynamic->value && r_wsurf.iLightmapLegacyDLights)
				{
					WSurfProgramState |= WSURF_LEGACY_DLIGHT_ENABLED;
				}

				if (r_wsurf.iLightmapUsedBits & (1 << 0))
				{
					WSurfProgramState |= WSURF_LIGHTMAP_INDEX_0_ENABLED;
				}
				if (r_wsurf.iLightmapUsedBits & (1 << 1))
				{
					WSurfProgramState |= WSURF_LIGHTMAP_INDEX_1_ENABLED;
				}
				if (r_wsurf.iLightmapUsedBits & (1 << 2))
				{
					WSurfProgramState |= WSURF_LIGHTMAP_INDEX_2_ENABLED;
				}
				if (r_wsurf.iLightmapUsedBits & (1 << 3))
				{
					WSurfProgramState |= WSURF_LIGHTMAP_INDEX_3_ENABLED;
				}
			}

			if (r_wsurf.bShadowmapTexture)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

				if (shadow_numvisedicts[0] > 0)
				{
					WSurfProgramState |= WSURF_SHADOWMAP_HIGH_ENABLED;
				}
				if (shadow_numvisedicts[1] > 0)
				{
					WSurfProgramState |= WSURF_SHADOWMAP_MEDIUM_ENABLED;
				}
				if (shadow_numvisedicts[2] > 0)
				{
					WSurfProgramState |= WSURF_SHADOWMAP_LOW_ENABLED;
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

			if (!R_IsRenderingGBuffer())
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
			}

			if (r_draw_shadowcaster)
			{
				WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
			}

			if (R_IsRenderingGBuffer())
			{
				WSurfProgramState |= WSURF_GBUFFER_ENABLED;
			}

			if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
			{
				if ((*currententity)->curstate.rendermode == kRenderTransAdd || (*currententity)->curstate.rendermode == kRenderGlow)
					WSurfProgramState |= WSURF_ADDITIVE_BLEND_ENABLED;
				else
					WSurfProgramState |= WSURF_ALPHA_BLEND_ENABLED;
			}

			if ((*currententity)->curstate.rendermode == kRenderTransAlpha)
			{
				WSurfProgramState |= WSURF_ALPHA_SOLID_ENABLED;
			}

			if (r_draw_gammablend)
			{
				WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
			}

			if (r_draw_oitblend && (WSurfProgramState & (WSURF_ALPHA_BLEND_ENABLED | WSURF_ADDITIVE_BLEND_ENABLED)))
			{
				WSurfProgramState |= WSURF_OIT_BLEND_ENABLED;
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

		program_state_t WSurfProgramState = 0;

		if (r_wsurf.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;

			GL_Bind(texture->gl_texturenum);

			R_BeginDetailTextureByGLTextureId(texture->gl_texturenum, &WSurfProgramState);
		}

		if (r_wsurf.bLightmapTexture)
		{
			WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;

			if (r_fullbright->value || !r_worldmodel->lightdata)
			{
				WSurfProgramState |= WSURF_FULLBRIGHT_ENABLED;
			}

			if (*filterMode != 0)
			{
				WSurfProgramState |= WSURF_COLOR_FILTER_ENABLED;
			}

			if (!r_light_dynamic->value && r_wsurf.iLightmapLegacyDLights)
			{
				WSurfProgramState |= WSURF_LEGACY_DLIGHT_ENABLED;
			}

			if (r_wsurf.iLightmapUsedBits & (1 << 0))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_0_ENABLED;
			}
			if (r_wsurf.iLightmapUsedBits & (1 << 1))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_1_ENABLED;
			}
			if (r_wsurf.iLightmapUsedBits & (1 << 2))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_2_ENABLED;
			}
			if (r_wsurf.iLightmapUsedBits & (1 << 3))
			{
				WSurfProgramState |= WSURF_LIGHTMAP_INDEX_3_ENABLED;
			}
		}

		if (r_wsurf.bShadowmapTexture)
		{
			WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

			if (shadow_numvisedicts[0] > 0)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_HIGH_ENABLED;
			}
			if (shadow_numvisedicts[1] > 0)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_MEDIUM_ENABLED;
			}
			if (shadow_numvisedicts[2] > 0)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_LOW_ENABLED;
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

		if (!R_IsRenderingGBuffer())
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
		}

		if (R_IsRenderingGBuffer())
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		}

		if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
		{
			if ((*currententity)->curstate.rendermode == kRenderTransAdd || (*currententity)->curstate.rendermode == kRenderGlow)
				WSurfProgramState |= WSURF_ADDITIVE_BLEND_ENABLED;
			else
				WSurfProgramState |= WSURF_ALPHA_BLEND_ENABLED;
		}

		if ((*currententity)->curstate.rendermode == kRenderTransAlpha)
		{
			WSurfProgramState |= WSURF_ALPHA_SOLID_ENABLED;
		}

		if (r_draw_gammablend)
		{
			WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (WSurfProgramState & (WSURF_ALPHA_BLEND_ENABLED | WSURF_ADDITIVE_BLEND_ENABLED)))
		{
			WSurfProgramState |= WSURF_OIT_BLEND_ENABLED;
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
	entity_ubo_t EntityUBO;

	memcpy(EntityUBO.entityMatrix, r_entity_matrix, sizeof(mat4));
	memcpy(EntityUBO.color, r_entity_color, sizeof(vec4));
	EntityUBO.scrollSpeed = R_ScrollSpeed();

	if (glNamedBufferSubData)
	{
		glNamedBufferSubData(modvbo->hEntityUBO, 0, sizeof(EntityUBO), &EntityUBO);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, modvbo->hEntityUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(EntityUBO), &EntityUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	if (bUseBindless)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_TEXTURE_SSBO, r_wsurf.hWorldSSBO);
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_ENTITY_UBO, modvbo->hEntityUBO);

	if (r_wsurf.bShadowmapTexture)
	{
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r_shadow_texture.depth_array);
		glActiveTexture(GL_TEXTURE0);
	}

	if (r_wsurf.bLightmapTexture)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
		glActiveTexture(GL_TEXTURE0);
	}

	bool bUseZPrePass = false;

	if (r_draw_opaque)
	{
		GL_BeginStencilWrite(STENCIL_MASK_WORLD | STENCIL_MASK_HAS_DECAL, STENCIL_MASK_ALL);
	}
	else
	{
		GL_BeginStencilWrite(STENCIL_MASK_HAS_DECAL, STENCIL_MASK_HAS_DECAL);
	}

	wsurf_vbo_leaf_t *vboleaf = NULL;

	if (modvbo->pModel == r_worldmodel)
	{
		int leafindex = ((*r_viewleaf) - r_worldmodel->leafs);
		if (leafindex >= 0 && leafindex < (int)modvbo->vLeaves.size())
		{
			vboleaf = modvbo->vLeaves[leafindex];

			R_DrawWSurfVBOBegin(vboleaf);

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

			R_DrawWSurfVBOEnd();
		}
		else
		{
			g_pMetaHookAPI->SysError("R_DrawWSurfVBO: Invalid leaf index %d", leafindex);
		}
	}
	else
	{
		if (modvbo->vLeaves.size() >= 1)
		{
			vboleaf = modvbo->vLeaves[0];

			R_DrawWSurfVBOBegin(vboleaf);

			R_DrawWSurfVBOStatic(vboleaf, bUseZPrePass);
			R_DrawWSurfVBOAnim(vboleaf, bUseZPrePass);

			R_DrawWSurfVBOEnd();
		}
		else
		{
			g_pMetaHookAPI->SysError("R_DrawWSurfVBO: Invalid leaf index");
		}
	}
	
	if (bUseZPrePass)
	{
		glDepthFunc(GL_LEQUAL);
	}

	GL_EndStencil();

	R_DrawDecals(ent);

	GL_ClearStencil(STENCIL_MASK_HAS_DECAL);

	if (r_wsurf.bShadowmapTexture)
	{
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glActiveTexture(GL_TEXTURE0);
	}

	if (r_wsurf.bLightmapTexture)
	{
		glActiveTexture(GL_TEXTURE1);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glActiveTexture(GL_TEXTURE0);
	}

	R_DrawWaters(vboleaf, ent);
}

//Just for debugging
#if 0
#define HUNK_NAME_LEN 64

typedef struct
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[HUNK_NAME_LEN];
} hunk_t;

#define HUNK_SENTINAL 0x1DF001ED

void Hunk_Check()
{
	byte **hunk_base = (byte **)((char *)g_dwEngineBase + 0x969E87C - 0x1D00000);
	int *hunk_low_used = (int *)((char *)g_dwEngineBase + 0x969E884 - 0x1D00000);
	int *hunk_size = (int *)((char *)g_dwEngineBase + 0x969E880 - 0x1D00000);

	for (auto h = (hunk_t *)(*hunk_base); (byte *)h != (*hunk_base) + (*hunk_low_used); )
	{
		gEngfuncs.Con_Printf("hunk %p %X %s\n", (byte *)h - (*hunk_base), h->size, h->name);

		if (h->sentinal != HUNK_SENTINAL)
			g_pMetaHookAPI->SysError("Hunk_Check: trashed sentinal");
		if (h->size < 16 || h->size + (byte *)h - (*hunk_base) > *hunk_size)
			g_pMetaHookAPI->SysError("Hunk_Check: bad size");

		h = (hunk_t *)((byte *)h + h->size);
	}
}
#endif

void R_PrebuildWSurfVBO(void)
{
	//TODO: use CL_GetModelByIndex instead ? idk
	for (int i = 0; i < (*mod_numknown); ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_brush && mod->needload == NL_PRESENT)
		{
			R_PrepareWSurfVBO(mod);
		}
	}
}

void R_Reload_f(void)
{
	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities, R_ParseBSPEntity_DefaultAllocator);
	R_LoadExternalEntities();
	R_LoadBSPEntities();

	gEngfuncs.Con_Printf("Entities reloaded\n");
}

void R_InitWSurf(void)
{
	r_wsurf.hSceneVBO = 0;
	r_wsurf.hSceneUBO = 0;
	r_wsurf.hDLightUBO = 0;
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

	R_FreeVertexBuffer();
	R_FreeBindlessTexturesForWorld();

	R_CreateBindlessTexturesForWorld();
	R_GenerateVertexBuffer();

	R_ClearWSurfVBOCache();

	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities, R_ParseBSPEntity_DefaultAllocator);
	R_LoadExternalEntities();
	R_LoadBSPEntities();

	R_PrebuildWSurfVBO();
}

void R_ShutdownWSurf(void)
{
	g_WSurfProgramTable.clear();

	R_ClearDecalCache();
	R_ClearDetailTextureCache();

	R_FreeLightmapTextures();
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

			int texId = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, false);
			if (!texId)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				texId = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, true);
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

			int texId = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, false);
			if (!texId)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				texId = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, textypeHasMipmap[texType], true, true);
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

void R_BeginDetailTextureByDetailTextureCache(detail_texture_cache_t *cache, program_state_t *WSurfProgramState)
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

void R_BeginDetailTextureByGLTextureId(int gltexturenum, program_state_t *WSurfProgramState)
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

void R_EndDetailTexture(program_state_t WSurfProgramState)
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

char *ValueForKey(bspentity_t *ent, const char *key)
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

static bspentity_t *current_parse_entity = NULL;
static char com_token[4096];

bspentity_t *R_ParseBSPEntity_DefaultAllocator(void)
{
	size_t len = r_wsurf.vBSPEntities.size();

	r_wsurf.vBSPEntities.resize(len + 1);

	return &r_wsurf.vBSPEntities[len];
}

static bool R_ParseBSPEntityKeyValue(const char *classname, const char *keyname, const char *value, fnParseBSPEntity_Allocator parse_allocator)
{
	if (classname == NULL)
	{
		current_parse_entity = parse_allocator();
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

static bool R_ParseBSPEntityClassname(char *szInputStream, char *classname, fnParseBSPEntity_Allocator parse_allocator)
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
			R_ParseBSPEntityKeyValue(NULL, szKeyName, com_token, parse_allocator);

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

static char *R_ParseBSPEntity(char *data, fnParseBSPEntity_Allocator parse_allocator)
{
	char keyname[256] = { 0 };
	char classname[256] = { 0 };

	if (R_ParseBSPEntityClassname(data, classname, parse_allocator))
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

			R_ParseBSPEntityKeyValue(classname, keyname, com_token, parse_allocator);
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

void R_ParseBSPEntities(char *data, fnParseBSPEntity_Allocator parse_allocator)
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
			g_pMetaHookAPI->SysError("R_ParseBSPEntities: found %s when expecting {", com_token);
			return;
		}
		data = R_ParseBSPEntity(data, parse_allocator);
	}
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
		name = "renderer/default_entity.txt";

		pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
		if (!pfile)
		{
			return;
		}
	}

	R_ParseBSPEntities(pfile, R_ParseBSPEntity_DefaultAllocator);

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
	R_ParseMapCvarSetMapValue(r_dynlight_radius_scale, ValueForKey(ent, "radius_scale"));
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
	//This has been moved to shader
#if 0
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
#endif

	if (s->pdecals)
	{
		gDecalSurfs[(*gDecalSurfCount)] = s;
		(*gDecalSurfCount)++;

		if ((*gDecalSurfCount) > MAX_DECALSURFS)
			g_pMetaHookAPI->SysError("Too many decal surfaces!\n");
	}
}
#if 0
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
#endif

void R_DrawBrushModel(cl_entity_t *e)
{
	qboolean rotated;

	(*currententity) = e;
	(*currenttexture) = -1;

	auto clmodel = e->model;

	vec3_t entity_mins, entity_maxs;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;

		for (int i = 0; i < 3; i++)
		{
			entity_mins[i] = e->origin[i] - clmodel->radius;
			entity_maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;

		VectorAdd(e->origin, clmodel->mins, entity_mins);
		VectorAdd(e->origin, clmodel->maxs, entity_maxs);
	}

	if (R_CullBox(entity_mins, entity_maxs))
		return;

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

	//We don't need this anymore
#if 0
	if (!r_light_dynamic->value)
	{
		if (clmodel->firstmodelsurface != 0)
		{
			int max_dlights = EngineGetMaxDLights();

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
#endif

	R_RotateForEntity(e);

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

	glDepthMask(GL_TRUE);
	//TODO: Fixed-function can be done in shader
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);
	glDisable(GL_BLEND);
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
		float equation[4] = { g_CurrentReflectCache->normal[0], g_CurrentReflectCache->normal[1], g_CurrentReflectCache->normal[2], -g_CurrentReflectCache->planedist };
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
	SceneUBO.alphamin = gl_alphamin->value;
	SceneUBO.r_additive_shift = r_additive_shift->value;

	if (gl_overbright->value)
		SceneUBO.r_lightscale = 1;
	else
		SceneUBO.r_lightscale = ((pow(2.0f, 1.0f / v_lightgamma->value) * 256) + 0.5) / 256;

	SceneUBO.r_filtercolor[0] = *filterColorRed;
	SceneUBO.r_filtercolor[1] = *filterColorGreen;
	SceneUBO.r_filtercolor[2] = *filterColorBlue;
	SceneUBO.r_filtercolor[3] = *filterBrightness;

	//Use vec4[256/4] instead of float[256] to save vram, float[256] in std140 costs 16 * 256 instead of 4 * 256 bytes due to alignment
	for (int i = 0; i < 256; ++i)
	{
		SceneUBO.r_lightstylevalue[i / 4][i % 4] = d_lightstylevalue[i] * (1.0f / 264.0f);
	}
	if (glNamedBufferSubData)
	{
		glNamedBufferSubData(r_wsurf.hSceneUBO, 0, sizeof(SceneUBO), &SceneUBO);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, r_wsurf.hSceneUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneUBO), &SceneUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

void R_SetupDLightUBO(void)
{
	dlight_ubo_t DLightUBO = { 0 };

	r_wsurf.iLightmapLegacyDLights = 0;

	if (!r_light_dynamic->value)
	{
		int max_dlight = EngineGetMaxDLights();
		dlight_t *dl = cl_dlights;
		float curtime = (*cl_time);

		for (int i = 0; i < max_dlight; i++, dl++)
		{
			if (dl->die < curtime || !dl->radius)
				continue;

			DLightUBO.origin_radius[r_wsurf.iLightmapLegacyDLights][0] = dl->origin[0];
			DLightUBO.origin_radius[r_wsurf.iLightmapLegacyDLights][1] = dl->origin[1];
			DLightUBO.origin_radius[r_wsurf.iLightmapLegacyDLights][2] = dl->origin[2];
			DLightUBO.origin_radius[r_wsurf.iLightmapLegacyDLights][3] = dl->radius;

			DLightUBO.color_minlight[r_wsurf.iLightmapLegacyDLights][0] = (float)dl->color.r / 255.0f;
			DLightUBO.color_minlight[r_wsurf.iLightmapLegacyDLights][1] = (float)dl->color.g / 255.0f;
			DLightUBO.color_minlight[r_wsurf.iLightmapLegacyDLights][2] = (float)dl->color.b / 255.0f;

			DLightUBO.color_minlight[r_wsurf.iLightmapLegacyDLights][3] = dl->minlight;
			r_wsurf.iLightmapLegacyDLights++;
		}
	}

	DLightUBO.active_dlights[0] = r_wsurf.iLightmapLegacyDLights;

	if (glNamedBufferSubData)
	{
		glNamedBufferSubData(r_wsurf.hDLightUBO, 0, sizeof(DLightUBO), &DLightUBO);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, r_wsurf.hDLightUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(DLightUBO), &DLightUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

void R_PrepareDrawWorld(void)
{
	r_wsurf.bDiffuseTexture = true;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bShadowmapTexture = false;

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
	R_SetupDLightUBO();
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

	//TODO: what the heck is this ???
	r_worldentity->curstate.rendercolor.r = gWaterColor->r;
	r_worldentity->curstate.rendercolor.g = gWaterColor->g;
	r_worldentity->curstate.rendercolor.b = gWaterColor->b;

	(*currententity) = r_worldentity;
	(*currenttexture) = -1;

	GL_DisableMultitexture();

	//Just for backward-compatibility
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	R_DrawSkyBox();

	if (r_draw_reflectview && g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_SKYBOX)
	{
		
	}
	else
	{
		r_wsurf.bDiffuseTexture = true;
		r_wsurf.bLightmapTexture = true;

		auto modvbo = R_PrepareWSurfVBO(r_worldmodel);

		R_DrawWSurfVBO(modvbo, (*currententity));
	}

	GL_DisableMultitexture();
}