#include "gl_local.h"

#include <sstream>
#include <algorithm>
#include <set>

CWorldSurfaceRenderer g_WorldSurfaceRenderer;

float r_shadow_matrix[3][16] = { 0 };
float r_world_matrix_inv[16] = { 0 };
float r_projection_matrix_inv[16] = { 0 };

float r_viewmodel_projection_matrix[16] = { 0 };
float r_viewmodel_projection_matrix_inv[16] = { 0 };

vec3_t r_frustum_origin[4] = { 0 };
vec3_t r_frustum_vec[4] = { 0 };
float r_znear = 0;
float r_zfar = 0;
bool r_ortho = false;
int r_wsurf_drawcall = 0;
int r_wsurf_polys = 0;

int g_iCurrentFrameLeafLoadCount = 0;

std::unordered_map <program_state_t, wsurf_program_t> g_WSurfProgramTable;

std::unordered_map <int, detail_texture_cache_t *> g_DetailTextureTable;

std::unordered_map <std::string, detail_texture_cache_t *> g_DecalTextureTable;

std::vector<CWorldSurfaceModel *> g_WorldSurfaceModels;

std::vector<CWorldSurfaceWorldModel*> g_WorldSurfaceWorldModels;

CWorldSurfaceLeaf::~CWorldSurfaceLeaf()
{
	if (hABO)
	{
		GL_DeleteBuffer(hABO);
	}

	for (auto pWaterSurfaceModel : vWaterSurfaceModels)
	{
		delete pWaterSurfaceModel;
	}

	vWaterSurfaceModels.clear();
}

CWorldSurfaceWorldModel::~CWorldSurfaceWorldModel()
{
	if (hEBO)
	{
		GL_DeleteBuffer(hEBO);
	}

	for (int i = WSURF_VBO_POSITION; i < WSURF_VBO_MAX; ++i)
	{
		if (hVBO[i])
		{
			GL_DeleteBuffer(hVBO[i]);
			hVBO[i] = 0;
		}
	}

	for (const auto &it : VAOMap)
	{
		auto hVAO = it.second;

		GL_DeleteVAO(hVAO);
	}
	VAOMap.clear();
}

CWorldSurfaceModel::~CWorldSurfaceModel()
{
	for (auto pLeaf : vLeaves)
	{
		delete pLeaf;
	}
}

void R_FreeWorldSurfaceModels(model_t *mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex < g_WorldSurfaceModels.size() && g_WorldSurfaceModels[modelindex])
	{
		gEngfuncs.Con_DPrintf("R_FreeWorldSurfaceModels: [%s] freed.\n", mod->name);

		delete g_WorldSurfaceModels[modelindex];
		g_WorldSurfaceModels[modelindex] = nullptr;

		return;
	}
	gEngfuncs.Con_DPrintf("R_FreeWorldSurfaceModels: Could not found world surface model for [%s].\n", mod->name);
}

void R_FreeWorldSurfaceWorldModels(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex < g_WorldSurfaceWorldModels.size() && g_WorldSurfaceWorldModels[modelindex])
	{
		gEngfuncs.Con_DPrintf("R_FreeWorldSurfaceWorldModels: [%s] freed.\n", mod->name);

		delete g_WorldSurfaceWorldModels[modelindex];
		g_WorldSurfaceWorldModels[modelindex] = nullptr;

		return;
	}
	gEngfuncs.Con_DPrintf("R_FreeWorldSurfaceWorldModels: Could not found world surface world model for [%s].\n", mod->name);
}

void R_ClearWorldSurfaceModels(void)
{
	for (size_t i = 0;i < g_WorldSurfaceModels.size(); ++i)
	{
		if (g_WorldSurfaceModels[i])
		{
			delete g_WorldSurfaceModels[i];
			g_WorldSurfaceModels[i] = nullptr;
		}
	}
}

void R_ClearWorldSurfaceWorldModels(void)
{
	for (size_t i = 0; i < g_WorldSurfaceWorldModels.size(); ++i)
	{
		if (g_WorldSurfaceWorldModels[i])
		{
			delete g_WorldSurfaceWorldModels[i];
			g_WorldSurfaceWorldModels[i] = nullptr;
		}
	}
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

		if ((state & WSURF_OIT_BLEND_ENABLED) && g_bUseOITBlend)
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

		defs << "#define SHADOW_TEXTURE_OFFSET (1.0 / " << std::dec << r_shadow_texture.size << ".0)\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\wsurf_shader.vert.glsl", "renderer\\shader\\wsurf_shader.frag.glsl", def.c_str(), def.c_str(), NULL);

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
	if (g_WorldSurfaceRenderer.hDecalVAO)
	{
		GL_DeleteVAO(g_WorldSurfaceRenderer.hDecalVAO);
		g_WorldSurfaceRenderer.hDecalVAO = 0;
	}

	if (g_WorldSurfaceRenderer.hDecalVBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hDecalVBO);
		g_WorldSurfaceRenderer.hDecalVBO = 0;
	}

	if (g_WorldSurfaceRenderer.hDecalEBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hDecalEBO);
		g_WorldSurfaceRenderer.hDecalEBO = 0;
	}

	if (g_WorldSurfaceRenderer.hSceneUBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hSceneUBO);
		g_WorldSurfaceRenderer.hSceneUBO = 0;
	}

	if (g_WorldSurfaceRenderer.hCameraUBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hCameraUBO);
		g_WorldSurfaceRenderer.hCameraUBO = 0;
	}

	if (g_WorldSurfaceRenderer.hEntityUBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hEntityUBO);
		g_WorldSurfaceRenderer.hEntityUBO = 0;
	}

	if (g_WorldSurfaceRenderer.hDLightUBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hDLightUBO);
		g_WorldSurfaceRenderer.hDLightUBO = 0;
	}

	if (g_WorldSurfaceRenderer.hDecalSSBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hDecalSSBO);
		g_WorldSurfaceRenderer.hDecalSSBO = 0;
	}

	if (g_WorldSurfaceRenderer.hSkyboxSSBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hSkyboxSSBO);
		g_WorldSurfaceRenderer.hSkyboxSSBO = 0;
	}

	if (g_WorldSurfaceRenderer.hDetailSkyboxSSBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hDetailSkyboxSSBO);
		g_WorldSurfaceRenderer.hDetailSkyboxSSBO = 0;
	}

	if (g_WorldSurfaceRenderer.hOITFragmentSSBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hOITFragmentSSBO);
		g_WorldSurfaceRenderer.hOITFragmentSSBO = 0;
	}

	if (g_WorldSurfaceRenderer.hOITNumFragmentSSBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hOITNumFragmentSSBO);
		g_WorldSurfaceRenderer.hOITNumFragmentSSBO = 0;
	}

	if (g_WorldSurfaceRenderer.hOITAtomicSSBO)
	{
		GL_DeleteBuffer(g_WorldSurfaceRenderer.hOITAtomicSSBO);
		g_WorldSurfaceRenderer.hOITAtomicSSBO = 0;
	}
}

void R_FreeLightmapTextures()
{
	for (int lightmap_idx = 0; lightmap_idx < MAXLIGHTMAPS; ++lightmap_idx)
	{
		if (g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx])
		{
			gEngfuncs.Con_DPrintf("R_FreeLightmapTextures: delete texid [%d].\n", g_WorldSurfaceRenderer.iLightmapTextureArray);
			GL_DeleteTexture(g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx]);
			g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx] = 0;
		}
	}
}

void R_RecursiveFindLeaves(mbasenode_t *basenode, std::set<mleaf_t *> &vLeafs)
{
	if (basenode->contents < 0)
	{
		auto pleaf = (mleaf_t *)basenode;

		vLeafs.emplace(pleaf);
		return;
	}

	auto node = (mnode_t*)basenode;

	R_RecursiveFindLeaves(node->children[0], vLeafs);

	R_RecursiveFindLeaves(node->children[1], vLeafs);
}

void R_MarkPVSForLeaf(mleaf_t *leaf, int visframecount)
{
	//Decompress vis bytes from world model.

	auto vis = Mod_LeafPVS(leaf, (*cl_worldmodel));

	for (int i = 0; i < (*cl_worldmodel)->numleafs; i++)
	{
		if ((byte)(1 << (i & 7)) & vis[i >> 3])
		{
			auto basenode = (mbasenode_t *)&(*cl_worldmodel)->leafs[i + 1];

			do
			{
				if (basenode->visframe == visframecount)
					break;

				basenode->visframe = visframecount;

				basenode = basenode->parent;

			} while (basenode);
		}
	}
}

void R_RecursiveMarkSurfaces(mbasenode_t *basenode, int visframecount, int framecount)
{
	if (basenode->contents == CONTENTS_SOLID)
		return;

	if (basenode->visframe != visframecount)
		return;

	if (basenode->contents < 0)
	{
		auto leaf = (mleaf_t *)basenode;

		auto marks = leaf->firstmarksurface;
		auto nummarks = leaf->nummarksurfaces;

		for (int i = 0; i < nummarks; ++i)
		{
			marks[i]->visframe = framecount;
		}
		return;
	}

	auto node = (mnode_t*)basenode;

	R_RecursiveMarkSurfaces(node->children[0], visframecount, framecount);

	R_RecursiveMarkSurfaces(node->children[1], visframecount, framecount);
}

void R_CollectWaters(model_t *mod, msurface_t* surf, int direction, CWorldSurfaceWorldModel* pWorldModel, CWorldSurfaceLeaf* pLeaf)
{
	auto pWaterModel = R_GetWaterSurfaceModel(mod, surf, direction, pWorldModel, pLeaf);

	if (std::find(pLeaf->vWaterSurfaceModels.begin(), pLeaf->vWaterSurfaceModels.end(), pWaterModel) == pLeaf->vWaterSurfaceModels.end())
	{
		pLeaf->vWaterSurfaceModels.emplace_back(pWaterModel);
	}
}

void R_RecursiveLinkTextureChain(model_t *mod, mbasenode_t *basenode, int visframecount, int framecount, CWorldSurfaceWorldModel *pWorldModel, CWorldSurfaceLeaf* pLeaf)
{
	if (basenode->contents == CONTENTS_SOLID)
		return;

	if (basenode->visframe != visframecount)
		return;

	if (basenode->contents < 0)
		return;

	auto node = (mnode_t*)basenode;

	R_RecursiveLinkTextureChain(mod, node->children[0], visframecount, framecount, pWorldModel, pLeaf);

	for (int i = 0; i < node->numsurfaces; ++i)
	{
		auto surf = R_GetWorldSurfaceByIndex(mod, node->firstsurface + i);

		if (surf->visframe != framecount)
		{
			continue;
		}

		//if (surf->flags & SURF_DRAWSKY)
		//{
		//	continue;
		//}

		if (surf->flags & SURF_DRAWTURB)
		{
			R_CollectWaters(mod, surf, 0, pWorldModel, pLeaf);
			continue;
		}

		surf->texturechain = surf->texinfo->texture->texturechain;
		surf->texinfo->texture->texturechain = surf;
	}

	R_RecursiveLinkTextureChain(mod, node->children[1], visframecount, framecount, pWorldModel, pLeaf);
}

void R_BrushModelLinkTextureChain(model_t *mod, CWorldSurfaceWorldModel *pWorldModel, CWorldSurfaceLeaf* pLeaf)
{
	for (int i = 0; i < mod->nummodelsurfaces; i++)
	{
		auto surf = R_GetWorldSurfaceByIndex(mod, mod->firstmodelsurface + i);

		auto pplane = surf->plane;

		//if (surf->flags & SURF_DRAWSKY)
		//{
		//	continue;
		//}

		if (surf->flags & SURF_DRAWTURB)
		{
			//Skip non-Z planes
			if (pplane->type != PLANE_Z)
				continue;

			//Skip bottom ?
			R_CollectWaters(mod, surf, 0, pWorldModel, pLeaf);
			R_CollectWaters(mod, surf, 1, pWorldModel, pLeaf);
			continue;
		}

		surf->texturechain = surf->texinfo->texture->texturechain;
		surf->texinfo->texture->texturechain = surf;
	}
}

void R_GenerateIndicesForTexChain(model_t *mod, msurface_t *surf, CWorldSurfaceBrushTexChain *texchain, CWorldSurfaceWorldModel* pWorldModel, std::vector<CDrawIndexAttrib>& vDrawAttribBuffer)
{
	auto surfIndex = R_GetWorldSurfaceIndex(pWorldModel->mod, surf);

	if (surfIndex == -1)
	{
		Sys_Error("R_GenerateIndicesForTexChain: invalid surfIndex!");
		return;
	}

	auto &brushface = pWorldModel->vFaceBuffer[surfIndex];

	if (surf->flags & SURF_DRAWSKY)
	{
		if (texchain->type == TEXCHAIN_SKY)
		{
			CDrawIndexAttrib drawAttrib;

			drawAttrib.FirstIndexLocation = brushface.start_index;
			drawAttrib.NumIndices = brushface.index_count;

			vDrawAttribBuffer.emplace_back(drawAttrib);

			texchain->drawCount++;
			texchain->polyCount += brushface.poly_count;
		}
	}
	else if (surf->flags & SURF_DRAWTURB)
	{

	}
	else if (surf->flags & SURF_UNDERWATER)
	{

	}
	else if (surf->flags & SURF_DRAWTILED)
	{
		if (texchain->type == TEXCHAIN_SCROLL)
		{
			CDrawIndexAttrib drawAttrib;

			drawAttrib.FirstIndexLocation = brushface.start_index;
			drawAttrib.NumIndices = brushface.index_count;

			vDrawAttribBuffer.emplace_back(drawAttrib);

			texchain->drawCount++;
			texchain->polyCount += brushface.poly_count;
		}
	}
	else
	{
		if (texchain->type == TEXCHAIN_STATIC)
		{
			CDrawIndexAttrib drawAttrib;

			drawAttrib.FirstIndexLocation = brushface.start_index;
			drawAttrib.NumIndices = brushface.index_count;

			vDrawAttribBuffer.emplace_back(drawAttrib);

			texchain->drawCount++;
			texchain->polyCount += brushface.poly_count;
		}
	}
}

void R_GenerateResourceForWaterModels(model_t *mod, CWorldSurfaceWorldModel *pWorldModel, CWorldSurfaceLeaf* pLeaf)
{
	for (size_t i = 0; i < pLeaf->vWaterSurfaceModels.size(); ++i)
	{
		auto pWaterModel = pLeaf->vWaterSurfaceModels[i];

		if (pWaterModel->vDrawAttribBuffer.size() > 0)
		{
			pWaterModel->hABO = GL_GenBuffer();

			GL_UploadDataToABOStaticDraw(pWaterModel->hABO, sizeof(CDrawIndexAttrib) * pWaterModel->vDrawAttribBuffer.size(), pWaterModel->vDrawAttribBuffer.data());

			pWaterModel->drawCount = pWaterModel->vDrawAttribBuffer.size();
		}
	}
}

void R_GenerateTexChain(model_t *mod, CWorldSurfaceWorldModel* pWorldModel, CWorldSurfaceLeaf *pLeaf, int iTexChainPass, std::vector<CDrawIndexAttrib>& vDrawAttribBuffer)
{
	for (int i = 0; i < mod->numtextures; i++)
	{
		auto t = mod->textures[i];

		if (!t)
		{
			if(g_iEngineType == ENGINE_SVENGINE)
				t = (*r_missingtexture);
			else
				t = (*r_notexture_mip);
		}

		if (!t)
			continue;

		bool bIsSkyTexture = (0 == strcmp(t->name, "sky")) ? true : false;

		if (iTexChainPass == 1 && bIsSkyTexture)
		{
			auto s = t->texturechain;

			if (s)
			{
				CWorldSurfaceBrushTexChain texchain;

				texchain.type = TEXCHAIN_SKY;
				texchain.texture = t;
				texchain.detailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
				texchain.drawCount = 0;
				texchain.polyCount = 0;
				texchain.startDrawOffset = (uint32_t)vDrawAttribBuffer.size() * sizeof(CDrawIndexAttrib);

				for (; s; s = s->texturechain)
				{
					R_GenerateIndicesForTexChain(mod, s, &texchain, pWorldModel, vDrawAttribBuffer);
				}

				if (texchain.drawCount > 0)
					pLeaf->TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_SKY] = texchain;
			}

			//End construction

			t->texturechain = NULL;
		}
		else if (iTexChainPass == 0 && t->anim_total && !bIsSkyTexture)
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

						int numtexturechain = 0;
						for (msurface_t *s2 = s; s2; s2 = s2->texturechain)
						{
							numtexturechain++;
						}

						//rtable not initialized?
						if ((*rtable)[0][0] == 0)
						{
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
								CWorldSurfaceBrushTexChain texchain;
								texchain.type = TEXCHAIN_STATIC;
								texchain.texture = t2;
								texchain.detailTextureCache = R_FindDetailTextureCache(t2->gl_texturenum);
								texchain.drawCount = 0;
								texchain.polyCount = 0;
								texchain.startDrawOffset = (uint32_t)vDrawAttribBuffer.size() * sizeof(CDrawIndexAttrib);

								for (int n = 0; n < numtexturechain; ++n)
								{
									if (texchainMapper[n] == k)
										R_GenerateIndicesForTexChain(mod, texchainSurface[n], &texchain, pWorldModel, vDrawAttribBuffer);
								}

								if (texchain.drawCount > 0)
									pLeaf->vTextureChainList[WSURF_TEXCHAIN_LIST_STATIC].emplace_back(texchain);
							}
						}

						delete[]texchainSurface;
						delete[]texchainMapper;
					}
				}

				//End construction

				t->texturechain = NULL;
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

						CWorldSurfaceBrushTexChain texchain;

						texchain.type = TEXCHAIN_STATIC;
						texchain.texture = t;
						texchain.detailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
						texchain.drawCount = 0;
						texchain.polyCount = 0;
						texchain.startDrawOffset = (uint32_t)vDrawAttribBuffer.size() * sizeof(CDrawIndexAttrib);

						for (; s; s = s->texturechain)
						{
							R_GenerateIndicesForTexChain(mod, s, &texchain, pWorldModel, vDrawAttribBuffer);
						}

						if (texchain.drawCount > 0)
							pLeaf->vTextureChainList[WSURF_TEXCHAIN_LIST_ANIM].emplace_back(texchain);
					}
				}


				//End construction

				t->texturechain = NULL;
			}
		}
		else if(iTexChainPass == 0 && !bIsSkyTexture)
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

					CWorldSurfaceBrushTexChain texchain;

					texchain.type = TEXCHAIN_STATIC;
					texchain.texture = t;
					texchain.detailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
					texchain.drawCount = 0;
					texchain.polyCount = 0;
					texchain.startDrawOffset = (uint32_t)vDrawAttribBuffer.size() * sizeof(CDrawIndexAttrib);

					for (; s; s = s->texturechain)
					{
						R_GenerateIndicesForTexChain(mod, s, &texchain, pWorldModel, vDrawAttribBuffer);
					}

					if (texchain.drawCount > 0)
						pLeaf->vTextureChainList[WSURF_TEXCHAIN_LIST_STATIC].emplace_back(texchain);
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

				CWorldSurfaceBrushTexChain texchain;

				texchain.type = TEXCHAIN_SCROLL;
				texchain.texture = t;
				texchain.detailTextureCache = R_FindDetailTextureCache(t->gl_texturenum);
				texchain.drawCount = 0;
				texchain.polyCount = 0;
				texchain.startDrawOffset = (uint32_t)vDrawAttribBuffer.size() * sizeof(CDrawIndexAttrib);

				for (; s; s = s->texturechain)
				{
					R_GenerateIndicesForTexChain(mod, s, &texchain, pWorldModel, vDrawAttribBuffer);
				}

				if (texchain.drawCount > 0)
					pLeaf->vTextureChainList[WSURF_TEXCHAIN_LIST_STATIC].emplace_back(texchain);
			}

			//End construction

			t->texturechain = NULL;
		}
	}

	if (iTexChainPass == 0)
	{
		CWorldSurfaceBrushTexChain texchain;

		texchain.type = TEXCHAIN_STATIC;
		texchain.texture = nullptr;
		texchain.detailTextureCache = nullptr;
		texchain.drawCount = (uint32_t)vDrawAttribBuffer.size();
		texchain.polyCount = 0;
		texchain.startDrawOffset = 0;

		pLeaf->TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_SOLID] = texchain;
	}
	else if (iTexChainPass == 1)
	{
		CWorldSurfaceBrushTexChain texchain;

		texchain.type = TEXCHAIN_STATIC;
		texchain.texture = nullptr;
		texchain.detailTextureCache = nullptr;
		texchain.drawCount = (uint32_t)vDrawAttribBuffer.size();
		texchain.polyCount = 0;
		texchain.startDrawOffset = 0;

		pLeaf->TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_SOLID_WITH_SKY] = texchain;
	}
}

void R_GenerateWorldSurfaceModelLeafInternal(
	CWorldSurfaceModel* pModel,
	model_t* mod,
	mleaf_t* leaf,
	int leafIndex,
	int framecount,
	int visframecount,
	std::vector<CDrawIndexAttrib>& vDrawAttribBuffer)
{
	R_MarkPVSForLeaf(leaf, visframecount);

	auto pLeaf = new CWorldSurfaceLeaf();

	pLeaf->pModel = pModel;

	R_RecursiveMarkSurfaces(mod->nodes, visframecount, framecount);

	R_RecursiveLinkTextureChain(mod, mod->nodes, visframecount, framecount, pModel->pWorldModel, pLeaf);

	R_GenerateResourceForWaterModels(mod, pModel->pWorldModel, pLeaf);

	R_GenerateTexChain(mod, pModel->pWorldModel, pLeaf, 0, vDrawAttribBuffer);
	R_GenerateTexChain(mod, pModel->pWorldModel, pLeaf, 1, vDrawAttribBuffer);

	if (vDrawAttribBuffer.size() > 0)
	{
		pLeaf->hABO = GL_GenBuffer();

		GL_UploadDataToABOStaticDraw(pLeaf->hABO, sizeof(CDrawIndexAttrib) * vDrawAttribBuffer.size(), vDrawAttribBuffer.data());
	}

	pModel->vLeaves[leafIndex] = pLeaf;
}

void R_GenerateWorldSurfaceModelLeaf(CWorldSurfaceModel* pModel, int leafIndex)
{
	if (leafIndex != -1 && leafIndex < pModel->vLeaves.size() && pModel->vLeaves[leafIndex])
	{
		gEngfuncs.Con_DPrintf(__FUNCTION__": leafIndex %d already has a pLeaf!\n", leafIndex);
		return;
	}

	std::vector<CDrawIndexAttrib> vDrawAttribBuffer;

	vDrawAttribBuffer.reserve(4096);

	auto mod = pModel->mod;

	auto leaf = R_GetWorldLeafByIndex(mod, leafIndex);

	int visframecount = 0;
	int framecount = 0;

	framecount++;
	visframecount++;

	R_GenerateWorldSurfaceModelLeafInternal(pModel, mod, leaf, leafIndex, framecount, visframecount, vDrawAttribBuffer);
}

IDeferredFrameTask* R_CreateDeferredFrameLoadLeafTask(CWorldSurfaceModel* pModel, int leafIndex)
{
	class CDeferredFrameLoadLeafTask : public IDeferredFrameTask
	{
	private:
		CWorldSurfaceModel* m_pModel{};
		int m_leafIndex{};
	public:
		CDeferredFrameLoadLeafTask(CWorldSurfaceModel* pModel, int leafIndex)
		{
			m_pModel = pModel;
			m_leafIndex = leafIndex;
		}

		void Destroy() override
		{
			delete this;
		}

		bool Run() override
		{
			if (g_iCurrentFrameLeafLoadCount > 1)
				return false;

			gEngfuncs.Con_DPrintf(__FUNCTION__": Generating leaf for leaf index %d.\n", m_leafIndex);

			R_GenerateWorldSurfaceModelLeaf(m_pModel, m_leafIndex);

			g_iCurrentFrameLeafLoadCount++;

			return true;
		}

		int GetFlags() const override
		{
			return DEFERRED_FRAME_TASK_DESTROY_ON_CHANGE_LEVEL;
		}
	};

	return new CDeferredFrameLoadLeafTask(pModel, leafIndex);
}

/*
	Generate leaf array for brush model
*/

CWorldSurfaceModel* R_GenerateWorldSurfaceModel(model_t *mod)
{
	std::vector<CDrawIndexAttrib> vDrawAttribBuffer;

	vDrawAttribBuffer.reserve(4096);

	auto pModel = new CWorldSurfaceModel();

	pModel->mod = mod;

	if (mod == (*cl_worldmodel))
	{
		auto pWorldModel = R_GetWorldSurfaceWorldModel(mod);

		pModel->pWorldModel = pWorldModel;

		std::set<mleaf_t *> vPossibleLeafs;
		R_RecursiveFindLeaves(mod->nodes, vPossibleLeafs);

		pModel->vLeaves.resize(vPossibleLeafs.size());

		int visframecount = 0;
		int framecount = 0;

		if ((int)r_leaf_lazy_load->value <= 0)
		{
			for (auto leaf : vPossibleLeafs)
			{
				vDrawAttribBuffer.clear();

				int leafIndex = R_GetWorldLeafIndex(mod, leaf);

				framecount++;
				visframecount++;

				R_GenerateWorldSurfaceModelLeafInternal(pModel, mod, leaf, leafIndex, framecount, visframecount, vDrawAttribBuffer);
			}
		}
		else if ((int)r_leaf_lazy_load->value == 1)
		{
			for (auto leaf : vPossibleLeafs)
			{
				int leafIndex = R_GetWorldLeafIndex(mod, leaf);

				R_AddDeferredFrameTask(R_CreateDeferredFrameLoadLeafTask(pModel, leafIndex));
			}
		}
 	}
	else
	{
		vDrawAttribBuffer.clear();

		auto worldmodel = R_FindWorldModelByModel(mod);

		auto pWorldModel = R_GetWorldSurfaceWorldModel(worldmodel);

		pModel->pWorldModel = pWorldModel;

		auto pLeaf = new CWorldSurfaceLeaf;

		pLeaf->pModel = pModel;

		R_BrushModelLinkTextureChain(mod, pWorldModel, pLeaf);

		R_GenerateResourceForWaterModels(mod, pWorldModel, pLeaf);

		R_GenerateTexChain(mod, pWorldModel, pLeaf, 0, vDrawAttribBuffer);
		R_GenerateTexChain(mod, pWorldModel, pLeaf, 1, vDrawAttribBuffer);

		if (vDrawAttribBuffer.size() > 0)
		{
			pLeaf->hABO = GL_GenBuffer();

			GL_UploadDataToABOStaticDraw(pLeaf->hABO, sizeof(CDrawIndexAttrib) * vDrawAttribBuffer.size(), vDrawAttribBuffer.data());
		}

		pModel->vLeaves.emplace_back(pLeaf);
	}
	pModel->vLeaves.shrink_to_fit();

	return pModel;
}

CWorldSurfaceModel* R_GetWorldSurfaceModel(model_t* mod)
{
	if (mod->type != mod_brush)
	{
		Sys_Error("R_GetWorldSurfaceModel: invalid mod type!");
		return NULL;
	}
	
	auto modelindex = EngineGetModelIndex(mod);

	if (modelindex == -1)
		return NULL;

	if (modelindex >= (int)g_WorldSurfaceModels.size())
	{
		g_WorldSurfaceModels.resize(modelindex + 1);
	}

	auto pModel = g_WorldSurfaceModels[modelindex];

	if (!pModel)
	{
		pModel = R_GenerateWorldSurfaceModel(mod);

		g_WorldSurfaceModels[modelindex] = pModel;
	}

	return pModel;
}

int R_FindTextureIdByTexture(model_t *mod, texture_t *ptex)
{
	for (int i = 0; i < mod->numtextures; ++i)
	{
		if (mod->textures[i] == ptex)
			return i;
	}

	return -1;
}

msurface_t* R_GetWorldSurfaceByIndex(model_t *mod, int index)
{
	msurface_t* surf;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		surf = (((msurface_hl25_t*)mod->surfaces) + index);
	}
	else
	{
		surf = mod->surfaces + index;
	}

	return surf;
}

int R_GetWorldSurfaceIndex(model_t* mod, msurface_t*surf)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		auto surf25 = (msurface_hl25_t*)surf;
		auto surfbase = (msurface_hl25_t*)mod->surfaces;
		auto surfend = surfbase + mod->numsurfaces;

		if(surf25 >= surfbase && surf25 < surfend)
			return surf25 - surfbase;

		return -1;
	}

	auto surfbase = mod->surfaces;
	auto surfend = surfbase + mod->numsurfaces;

	if (surf >= surfbase && surf < surfend)
		return surf - surfbase;

	return -1;
}

mleaf_t* R_GetWorldLeafByIndex(model_t* mod, int index)
{
	return mod->leafs + index;
}

int R_GetWorldLeafIndex(model_t* mod, mleaf_t* leaf)
{
	return leaf - mod->leafs;
}

model_t* R_FindWorldModelByModel(model_t* m)
{
	for (auto mod : g_WorldSurfaceRenderer.vWorldModels)
	{
		if (mod->vertexes == m->vertexes)
			return mod;
	}
	return nullptr;
}

model_t* R_FindWorldModelBySurface(msurface_t* psurf)
{
	for (auto mod : g_WorldSurfaceRenderer.vWorldModels)
	{
		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			auto psurf_hl25 = (msurface_hl25_t*)psurf;
			auto pbase_hl25 = (msurface_hl25_t*)mod->surfaces;
			auto pend_hl25 = (msurface_hl25_t*)R_GetWorldSurfaceByIndex(mod, mod->numsurfaces);

			if (psurf_hl25 >= pbase_hl25 && psurf_hl25 < pend_hl25)
			{
				return mod;
			}
		}
		else
		{
			auto pbase = mod->surfaces;
			auto pend = R_GetWorldSurfaceByIndex(mod, mod->numsurfaces);

			if (psurf >= pbase && psurf < pend)
			{
				return mod;
			}
		}
	}

	return nullptr;
}

model_t* R_FindWorldModelByNode(mnode_t* pnode)
{
	for (auto mod : g_WorldSurfaceRenderer.vWorldModels)
	{
		auto pbase = mod->nodes;
		auto pend = pbase + mod->numnodes;

		if (pnode >= pbase && pnode < pend)
		{
			return mod;
		}
	}

	return nullptr;
}

GLuint R_BindVAOForWorldSurfaceWorldModel(CWorldSurfaceWorldModel* pWorldModel, int VBOStates)
{
	auto it = pWorldModel->VAOMap.find(VBOStates);

	if (it != pWorldModel->VAOMap.end())
	{
		return it->second;
	}

	auto hVAO = GL_GenVAO();

	GL_BindStatesForVAO(hVAO, [pWorldModel, VBOStates]() {

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pWorldModel->hEBO);

		if (VBOStates & (1 << WSURF_VBO_POSITION))
		{
			glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_POSITION]);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_POSITION);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMAL);
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_POSITION, 3, GL_FLOAT, false, sizeof(brushvertexpos_t), OFFSET(brushvertexpos_t, pos));
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMAL, 3, GL_FLOAT, false, sizeof(brushvertexpos_t), OFFSET(brushvertexpos_t, normal));
		}
		if (VBOStates & (1 << WSURF_VBO_DIFFUSE))
		{
			glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_DIFFUSE]);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_TEXCOORD, 3, GL_FLOAT, false, sizeof(brushvertexdiffuse_t), OFFSET(brushvertexdiffuse_t, texcoord));
		}
		if (VBOStates & (1 << WSURF_VBO_LIGHTMAP))
		{
			glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_LIGHTMAP]);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_STYLES);
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD, 3, GL_FLOAT, false, sizeof(brushvertexlightmap_t), OFFSET(brushvertexlightmap_t, lightmaptexcoord));
			glVertexAttribIPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_STYLES, 4, GL_UNSIGNED_BYTE, sizeof(brushvertexlightmap_t), OFFSET(brushvertexlightmap_t, styles));
		}
		if (VBOStates & (1 << WSURF_VBO_NORMAL))
		{
			glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_NORMAL]);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD);
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_S_TANGENT, 3, GL_FLOAT, false, sizeof(brushvertexnormal_t), OFFSET(brushvertexnormal_t, s_tangent));
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_T_TANGENT, 3, GL_FLOAT, false, sizeof(brushvertexnormal_t), OFFSET(brushvertexnormal_t, t_tangent));
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertexnormal_t), OFFSET(brushvertexnormal_t, normaltexcoord));
		}
		if (VBOStates & (1 << WSURF_VBO_DETAIL))
		{
			glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_DETAIL]);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD);
			glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD);
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertexdetail_t), OFFSET(brushvertexdetail_t, replacetexcoord));
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertexdetail_t), OFFSET(brushvertexdetail_t, detailtexcoord));
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertexdetail_t), OFFSET(brushvertexdetail_t, parallaxtexcoord));
			glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(brushvertexdetail_t), OFFSET(brushvertexdetail_t, speculartexcoord));
		}
	},
	[]()
	{
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_POSITION);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMAL);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD);
			glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_STYLES);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	});

	pWorldModel->VAOMap[VBOStates] = hVAO;

	return hVAO;
}

void R_PolygonToTriangleList(const std::vector<vertex3f_t>& vPolyVertices, std::vector<uint32_t>& vOutIndiceBuffer)
{
	// 清空输出缓冲区
	vOutIndiceBuffer.clear();
	
	// 需要至少3个顶点才能形成三角形
	if (vPolyVertices.size() < 3)
		return;
	
	// 如果只有3个顶点，直接形成一个三角形
	if (vPolyVertices.size() == 3)
	{
		vOutIndiceBuffer.push_back(0);
		vOutIndiceBuffer.push_back(1);
		vOutIndiceBuffer.push_back(2);
		return;
	}
	
	// 对于更多顶点，使用ear-clipping算法
	std::vector<uint32_t> indices;
	indices.reserve(vPolyVertices.size());
	for (size_t i = 0; i < vPolyVertices.size(); ++i)
	{
		indices.push_back(static_cast<uint32_t>(i));
	}
	
	// 检查三个顶点是否形成有效的三角形（非退化）
	auto isValidTriangle = [&vPolyVertices](uint32_t i0, uint32_t i1, uint32_t i2) -> bool {
		const auto& v0 = vPolyVertices[i0].pos;
		const auto& v1 = vPolyVertices[i1].pos;
		const auto& v2 = vPolyVertices[i2].pos;
		
		vec3_t edge1 = { v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2] };
		vec3_t edge2 = { v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2] };
		
		vec3_t cross;
		CrossProduct(edge1, edge2, cross);
		
		float length = VectorLength(cross); //避免退化三角形
		return length > 0.001f; // Fix #648 1e-6f; 
	};
	
	// 检查点是否在三角形内（使用重心坐标）
	auto isPointInTriangle = [&vPolyVertices](const vec3_t& p, uint32_t i0, uint32_t i1, uint32_t i2) -> bool {
		const auto& v0 = vPolyVertices[i0].pos;
		const auto& v1 = vPolyVertices[i1].pos;
		const auto& v2 = vPolyVertices[i2].pos;
		
		vec3_t v0v1 = { v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2] };
		vec3_t v0v2 = { v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2] };
		vec3_t v0p = { p[0] - v0[0], p[1] - v0[1], p[2] - v0[2] };
		
		float dot00 = DotProduct(v0v2, v0v2);
		float dot01 = DotProduct(v0v2, v0v1);
		float dot02 = DotProduct(v0v2, v0p);
		float dot11 = DotProduct(v0v1, v0v1);
		float dot12 = DotProduct(v0v1, v0p);
		
		float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
		float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
		
		return (u >= 0) && (v >= 0) && (u + v <= 1);
	};
	
	// 检查是否为耳朵（ear）
	auto isEar = [&](size_t idx) -> bool {
		if (indices.size() < 3) return false;
		
		size_t prev = (idx + indices.size() - 1) % indices.size();
		size_t next = (idx + 1) % indices.size();
		
		uint32_t i0 = indices[prev];
		uint32_t i1 = indices[idx];
		uint32_t i2 = indices[next];
		
		// 检查是否形成有效三角形
		if (!isValidTriangle(i0, i1, i2))
			return false;
		
		// 检查是否有其他顶点在这个三角形内
		for (size_t i = 0; i < indices.size(); ++i)
		{
			if (i == prev || i == idx || i == next)
				continue;
				
			if (isPointInTriangle(vPolyVertices[indices[i]].pos, i0, i1, i2))
				return false;
		}
		
		return true;
	};
	
	// Ear-clipping主循环
	while (indices.size() > 3)
	{
		bool earFound = false;
		
		for (size_t i = 0; i < indices.size(); ++i)
		{
			if (isEar(i))
			{
				// 找到耳朵，添加三角形
				size_t prev = (i + indices.size() - 1) % indices.size();
				size_t next = (i + 1) % indices.size();
				
				vOutIndiceBuffer.push_back(indices[prev]);
				vOutIndiceBuffer.push_back(indices[i]);
				vOutIndiceBuffer.push_back(indices[next]);
				
				// 移除当前顶点
				indices.erase(indices.begin() + i);
				earFound = true;
				break;
			}
		}
		
		// 如果没有找到耳朵，说明多边形可能有问题，使用扇形三角化作为后备方案
		if (!earFound)
		{
			gEngfuncs.Con_DPrintf("R_PolygonToTriangleList: No ear found, falling back to fan triangulation\n");
			
			vOutIndiceBuffer.clear();
			for (size_t i = 1; i < vPolyVertices.size() - 1; ++i)
			{
				vOutIndiceBuffer.push_back(0);
				vOutIndiceBuffer.push_back(static_cast<uint32_t>(i));
				vOutIndiceBuffer.push_back(static_cast<uint32_t>(i + 1));
			}
			return;
		}
	}
	
	// 添加最后一个三角形
	if (indices.size() == 3)
	{
		vOutIndiceBuffer.push_back(indices[0]);
		vOutIndiceBuffer.push_back(indices[1]);
		vOutIndiceBuffer.push_back(indices[2]);
	}
}

CWorldSurfaceWorldModel* R_GenerateWorldSurfaceWorldModel(model_t *mod)
{
	std::vector<brushvertexpos_t> vVertexPosBuffer;
	std::vector<brushvertexdiffuse_t> vVertexDiffuseBuffer;
	std::vector<brushvertexlightmap_t> vVertexLightmapBuffer;
	std::vector<brushvertexnormal_t> vVertexNormalBuffer;
	std::vector<brushvertexdetail_t> vVertexDetailBuffer;
	std::vector<uint32_t> vIndiceBuffer;

	vVertexPosBuffer.reserve(0x10000);
	vVertexDiffuseBuffer.reserve(0x10000);
	vVertexLightmapBuffer.reserve(0x10000);
	vVertexNormalBuffer.reserve(0x10000);
	vVertexDetailBuffer.reserve(0x10000);
	vIndiceBuffer.reserve(0x10000);

	auto pWorldModel = new CWorldSurfaceWorldModel;

	pWorldModel->mod = mod;

	pWorldModel->vFaceBuffer.resize(mod->numsurfaces);

	for(int i = 0; i < mod->numsurfaces; i++)
	{
		auto surf = R_GetWorldSurfaceByIndex(mod, i);
		auto poly = surf->polys;
		auto pBrushFace = &pWorldModel->vFaceBuffer[i];

		VectorCopy(surf->texinfo->vecs[0], pBrushFace->s_tangent);
		VectorCopy(surf->texinfo->vecs[1], pBrushFace->t_tangent);
		VectorNormalize(pBrushFace->s_tangent);
		VectorNormalize(pBrushFace->t_tangent);
		VectorCopy(surf->plane->normal, pBrushFace->normal);
		pBrushFace->index = i;
		pBrushFace->flags = surf->flags;

		if (surf->flags & SURF_PLANEBACK)
			VectorInverse(pBrushFace->normal);

		if (surf->lightmaptexturenum + 1 > g_WorldSurfaceRenderer.iNumLightmapTextures)
			g_WorldSurfaceRenderer.iNumLightmapTextures = surf->lightmaptexturenum + 1;

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

		if (pBrushFace->flags & SURF_DRAWTURB)
		{
			if (1)
			{
				uint32_t nBrushStartIndex = (uint32_t)vIndiceBuffer.size();

				for (poly = surf->polys; poly; poly = poly->next)
				{
					uint32_t nPolyStartIndex = (uint32_t)vVertexPosBuffer.size();

					std::vector<vertex3f_t> vPolyVertices;

					float* v = poly->verts[0];

					for (int j = 0; j < poly->numverts; j++, v += VERTEXSIZE)
					{
						vertex3f_t tempVertex;
						tempVertex.pos[0] = v[0];
						tempVertex.pos[1] = v[1];
						tempVertex.pos[2] = v[2];

						brushvertexpos_t tempVertexPos;
						tempVertexPos.pos[0] = v[0];
						tempVertexPos.pos[1] = v[1];
						tempVertexPos.pos[2] = v[2];
						tempVertexPos.normal[0] = pBrushFace->normal[0];
						tempVertexPos.normal[1] = pBrushFace->normal[1];
						tempVertexPos.normal[2] = pBrushFace->normal[2];

						brushvertexdiffuse_t tempVertexDiffuse;
						tempVertexDiffuse.texcoord[0] = v[3];
						tempVertexDiffuse.texcoord[1] = v[4];
						tempVertexDiffuse.texcoord[2] = (ptexture && (pBrushFace->flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;

						brushvertexlightmap_t tempVertexLightmap;
						tempVertexLightmap.lightmaptexcoord[0] = v[5];
						tempVertexLightmap.lightmaptexcoord[1] = v[6];
						tempVertexLightmap.lightmaptexcoord[2] = surf->lightmaptexturenum;
						memcpy(&tempVertexLightmap.styles, surf->styles, sizeof(surf->styles));

						brushvertexnormal_t tempVertexNormal;
						tempVertexNormal.s_tangent[0] = pBrushFace->s_tangent[0];
						tempVertexNormal.s_tangent[1] = pBrushFace->s_tangent[1];
						tempVertexNormal.s_tangent[2] = pBrushFace->s_tangent[2];
						tempVertexNormal.t_tangent[0] = pBrushFace->t_tangent[0];
						tempVertexNormal.t_tangent[1] = pBrushFace->t_tangent[1];
						tempVertexNormal.t_tangent[2] = pBrushFace->t_tangent[2];
						tempVertexNormal.normaltexcoord[0] = normalScale[0];
						tempVertexNormal.normaltexcoord[1] = normalScale[1];

						brushvertexdetail_t tempVertexDetail;
						tempVertexDetail.replacetexcoord[0] = replaceScale[0];
						tempVertexDetail.replacetexcoord[1] = replaceScale[1];
						tempVertexDetail.detailtexcoord[0] = detailScale[0];
						tempVertexDetail.detailtexcoord[1] = detailScale[1];
						tempVertexDetail.parallaxtexcoord[0] = parallaxScale[0];
						tempVertexDetail.parallaxtexcoord[1] = parallaxScale[1];
						tempVertexDetail.speculartexcoord[0] = specularScale[0];
						tempVertexDetail.speculartexcoord[1] = specularScale[1];

						vVertexPosBuffer.emplace_back(tempVertexPos);
						vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse);
						vVertexLightmapBuffer.emplace_back(tempVertexLightmap);
						vVertexNormalBuffer.emplace_back(tempVertexNormal);
						vVertexDetailBuffer.emplace_back(tempVertexDetail);
						vPolyVertices.emplace_back(tempVertex);
					}

					std::vector<uint32_t> vTriangleListIndices;
					R_PolygonToTriangleList(vPolyVertices, vTriangleListIndices);

					for (size_t k = 0; k < vTriangleListIndices.size(); ++k)
					{
						vIndiceBuffer.emplace_back(nPolyStartIndex + vTriangleListIndices[k]);
					}

					pBrushFace->poly_count++;
				}
				uint32_t nBrushCurrentIndex = (uint32_t)vIndiceBuffer.size();
				pBrushFace->start_index = nBrushStartIndex;
				pBrushFace->index_count = nBrushCurrentIndex - nBrushStartIndex;
			}
			if (1)
			{
				uint32_t nBrushStartIndex = (uint32_t)vIndiceBuffer.size();

				for (poly = surf->polys; poly; poly = poly->next)
				{
					uint32_t nPolyStartIndex = (uint32_t)vVertexPosBuffer.size();

					std::vector<vertex3f_t> vPolyVertices;

					float* v = poly->verts[0];

					for (int j = 0; j < poly->numverts; j++, v += VERTEXSIZE)
					{
						vertex3f_t tempVertex;
						tempVertex.pos[0] = v[0];
						tempVertex.pos[1] = v[1];
						tempVertex.pos[2] = v[2];

						brushvertexpos_t tempVertexPos;
						tempVertexPos.pos[0] = v[0];
						tempVertexPos.pos[1] = v[1];
						tempVertexPos.pos[2] = v[2];
						tempVertexPos.normal[0] = pBrushFace->normal[0];
						tempVertexPos.normal[1] = pBrushFace->normal[1];
						tempVertexPos.normal[2] = pBrushFace->normal[2];

						brushvertexdiffuse_t tempVertexDiffuse;
						tempVertexDiffuse.texcoord[0] = v[3];
						tempVertexDiffuse.texcoord[1] = v[4];
						tempVertexDiffuse.texcoord[2] = (ptexture && (pBrushFace->flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;

						brushvertexlightmap_t tempVertexLightmap;
						tempVertexLightmap.lightmaptexcoord[0] = v[5];
						tempVertexLightmap.lightmaptexcoord[1] = v[6];
						tempVertexLightmap.lightmaptexcoord[2] = surf->lightmaptexturenum;
						memcpy(&tempVertexLightmap.styles, surf->styles, sizeof(surf->styles));

						brushvertexnormal_t tempVertexNormal;
						tempVertexNormal.s_tangent[0] = pBrushFace->s_tangent[0];
						tempVertexNormal.s_tangent[1] = pBrushFace->s_tangent[1];
						tempVertexNormal.s_tangent[2] = pBrushFace->s_tangent[2];
						tempVertexNormal.t_tangent[0] = pBrushFace->t_tangent[0];
						tempVertexNormal.t_tangent[1] = pBrushFace->t_tangent[1];
						tempVertexNormal.t_tangent[2] = pBrushFace->t_tangent[2];
						tempVertexNormal.normaltexcoord[0] = normalScale[0];
						tempVertexNormal.normaltexcoord[1] = normalScale[1];

						VectorInverse(tempVertexPos.normal);
						VectorInverse(tempVertexNormal.s_tangent);
						VectorInverse(tempVertexNormal.t_tangent);

						brushvertexdetail_t tempVertexDetail;
						tempVertexDetail.replacetexcoord[0] = replaceScale[0];
						tempVertexDetail.replacetexcoord[1] = replaceScale[1];
						tempVertexDetail.detailtexcoord[0] = detailScale[0];
						tempVertexDetail.detailtexcoord[1] = detailScale[1];
						tempVertexDetail.parallaxtexcoord[0] = parallaxScale[0];
						tempVertexDetail.parallaxtexcoord[1] = parallaxScale[1];
						tempVertexDetail.speculartexcoord[0] = specularScale[0];
						tempVertexDetail.speculartexcoord[1] = specularScale[1];

						vVertexPosBuffer.emplace_back(tempVertexPos);
						vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse);
						vVertexLightmapBuffer.emplace_back(tempVertexLightmap);
						vVertexNormalBuffer.emplace_back(tempVertexNormal);
						vVertexDetailBuffer.emplace_back(tempVertexDetail);
						vPolyVertices.emplace_back(tempVertex);
					}

					std::vector<uint32_t> vTriangleListIndices;
					R_PolygonToTriangleList(vPolyVertices, vTriangleListIndices);

					for (size_t k = 0; k < vTriangleListIndices.size(); ++k)
					{
						vIndiceBuffer.emplace_back(nPolyStartIndex + vTriangleListIndices[vTriangleListIndices.size() - 1 - k]);
					}

					pBrushFace->poly_count++;
				}
				uint32_t nBrushCurrentIndex = (uint32_t)vIndiceBuffer.size();
				pBrushFace->reverse_start_index = nBrushStartIndex;
				pBrushFace->reverse_index_count = nBrushCurrentIndex - nBrushStartIndex;
			}
		}
		else
		{
			uint32_t nBrushStartIndex = (uint32_t)vIndiceBuffer.size();

			for (poly = surf->polys; poly; poly = poly->next)
			{
				uint32_t nPolyStartIndex = (uint32_t)vVertexPosBuffer.size();

				std::vector<vertex3f_t> vPolyVertices;

				float *v = poly->verts[0];

				vertex3f_t tempVertex[3];
				brushvertexpos_t tempVertexPos[3];
				brushvertexdiffuse_t tempVertexDiffuse[3];
				brushvertexlightmap_t tempVertexLightmap[3];
				brushvertexnormal_t tempVertexNormal[3];
				brushvertexdetail_t tempVertexDetail[3];

				for (int j = 0; j < 3; j++, v += VERTEXSIZE)
				{
					tempVertex[j].pos[0] = v[0];
					tempVertex[j].pos[1] = v[1];
					tempVertex[j].pos[2] = v[2];

					tempVertexPos[j].pos[0] = v[0];
					tempVertexPos[j].pos[1] = v[1];
					tempVertexPos[j].pos[2] = v[2];
					tempVertexPos[j].normal[0] = pBrushFace->normal[0];
					tempVertexPos[j].normal[1] = pBrushFace->normal[1];
					tempVertexPos[j].normal[2] = pBrushFace->normal[2];

					tempVertexDiffuse[j].texcoord[0] = v[3];
					tempVertexDiffuse[j].texcoord[1] = v[4];
					tempVertexDiffuse[j].texcoord[2] = (ptexture && (pBrushFace->flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;

					tempVertexLightmap[j].lightmaptexcoord[0] = v[5];
					tempVertexLightmap[j].lightmaptexcoord[1] = v[6];
					tempVertexLightmap[j].lightmaptexcoord[2] = surf->lightmaptexturenum;
					memcpy(&tempVertexLightmap[j].styles, surf->styles, sizeof(surf->styles));

					tempVertexNormal[j].s_tangent[0] = pBrushFace->s_tangent[0];
					tempVertexNormal[j].s_tangent[1] = pBrushFace->s_tangent[1];
					tempVertexNormal[j].s_tangent[2] = pBrushFace->s_tangent[2];
					tempVertexNormal[j].t_tangent[0] = pBrushFace->t_tangent[0];
					tempVertexNormal[j].t_tangent[1] = pBrushFace->t_tangent[1];
					tempVertexNormal[j].t_tangent[2] = pBrushFace->t_tangent[2];
					tempVertexNormal[j].normaltexcoord[0] = normalScale[0];
					tempVertexNormal[j].normaltexcoord[1] = normalScale[1];

					tempVertexDetail[j].replacetexcoord[0] = replaceScale[0];
					tempVertexDetail[j].replacetexcoord[1] = replaceScale[1];
					tempVertexDetail[j].detailtexcoord[0] = detailScale[0];
					tempVertexDetail[j].detailtexcoord[1] = detailScale[1];
					tempVertexDetail[j].parallaxtexcoord[0] = parallaxScale[0];
					tempVertexDetail[j].parallaxtexcoord[1] = parallaxScale[1];
					tempVertexDetail[j].speculartexcoord[0] = specularScale[0];
					tempVertexDetail[j].speculartexcoord[1] = specularScale[1];
				}
				vVertexPosBuffer.emplace_back(tempVertexPos[0]);
				vVertexPosBuffer.emplace_back(tempVertexPos[1]);
				vVertexPosBuffer.emplace_back(tempVertexPos[2]);
				vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse[0]);
				vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse[1]);
				vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse[2]);
				vVertexLightmapBuffer.emplace_back(tempVertexLightmap[0]);
				vVertexLightmapBuffer.emplace_back(tempVertexLightmap[1]);
				vVertexLightmapBuffer.emplace_back(tempVertexLightmap[2]);
				vVertexNormalBuffer.emplace_back(tempVertexNormal[0]);
				vVertexNormalBuffer.emplace_back(tempVertexNormal[1]);
				vVertexNormalBuffer.emplace_back(tempVertexNormal[2]);
				vVertexDetailBuffer.emplace_back(tempVertexDetail[0]);
				vVertexDetailBuffer.emplace_back(tempVertexDetail[1]);
				vVertexDetailBuffer.emplace_back(tempVertexDetail[2]);
				vPolyVertices.emplace_back(tempVertex[0]);
				vPolyVertices.emplace_back(tempVertex[1]);
				vPolyVertices.emplace_back(tempVertex[2]);

				for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
				{
					memcpy(&tempVertex[1], &tempVertex[2], sizeof(vertex3f_t));
					memcpy(&tempVertexPos[1], &tempVertexPos[2], sizeof(brushvertexpos_t));
					memcpy(&tempVertexDiffuse[1], &tempVertexDiffuse[2], sizeof(brushvertexdiffuse_t));
					memcpy(&tempVertexLightmap[1], &tempVertexLightmap[2], sizeof(brushvertexlightmap_t));
					memcpy(&tempVertexNormal[1], &tempVertexNormal[2], sizeof(brushvertexnormal_t));
					memcpy(&tempVertexDetail[1], &tempVertexDetail[2], sizeof(brushvertexdetail_t));

					tempVertex[2].pos[0] = v[0];
					tempVertex[2].pos[1] = v[1];
					tempVertex[2].pos[2] = v[2];

					tempVertexPos[2].pos[0] = v[0];
					tempVertexPos[2].pos[1] = v[1];
					tempVertexPos[2].pos[2] = v[2];
					tempVertexPos[2].normal[0] = pBrushFace->normal[0];
					tempVertexPos[2].normal[1] = pBrushFace->normal[1];
					tempVertexPos[2].normal[2] = pBrushFace->normal[2];

					tempVertexDiffuse[2].texcoord[0] = v[3];
					tempVertexDiffuse[2].texcoord[1] = v[4];
					tempVertexDiffuse[2].texcoord[2] = (ptexture && (pBrushFace->flags & SURF_DRAWTILED)) ? 1.0f / ptexture->width : 0;

					tempVertexLightmap[2].lightmaptexcoord[0] = v[5];
					tempVertexLightmap[2].lightmaptexcoord[1] = v[6];
					tempVertexLightmap[2].lightmaptexcoord[2] = surf->lightmaptexturenum;
					memcpy(&tempVertexLightmap[2].styles, surf->styles, sizeof(surf->styles));

					tempVertexNormal[2].s_tangent[0] = pBrushFace->s_tangent[0];
					tempVertexNormal[2].s_tangent[1] = pBrushFace->s_tangent[1];
					tempVertexNormal[2].s_tangent[2] = pBrushFace->s_tangent[2];
					tempVertexNormal[2].t_tangent[0] = pBrushFace->t_tangent[0];
					tempVertexNormal[2].t_tangent[1] = pBrushFace->t_tangent[1];
					tempVertexNormal[2].t_tangent[2] = pBrushFace->t_tangent[2];
					tempVertexNormal[2].normaltexcoord[0] = normalScale[0];
					tempVertexNormal[2].normaltexcoord[1] = normalScale[1];

					tempVertexDetail[2].replacetexcoord[0] = replaceScale[0];
					tempVertexDetail[2].replacetexcoord[1] = replaceScale[1];
					tempVertexDetail[2].detailtexcoord[0] = detailScale[0];
					tempVertexDetail[2].detailtexcoord[1] = detailScale[1];
					tempVertexDetail[2].parallaxtexcoord[0] = parallaxScale[0];
					tempVertexDetail[2].parallaxtexcoord[1] = parallaxScale[1];
					tempVertexDetail[2].speculartexcoord[0] = specularScale[0];
					tempVertexDetail[2].speculartexcoord[1] = specularScale[1];

					vVertexPosBuffer.emplace_back(tempVertexPos[0]);
					vVertexPosBuffer.emplace_back(tempVertexPos[1]);
					vVertexPosBuffer.emplace_back(tempVertexPos[2]);
					vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse[0]);
					vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse[1]);
					vVertexDiffuseBuffer.emplace_back(tempVertexDiffuse[2]);
					vVertexLightmapBuffer.emplace_back(tempVertexLightmap[0]);
					vVertexLightmapBuffer.emplace_back(tempVertexLightmap[1]);
					vVertexLightmapBuffer.emplace_back(tempVertexLightmap[2]);
					vVertexNormalBuffer.emplace_back(tempVertexNormal[0]);
					vVertexNormalBuffer.emplace_back(tempVertexNormal[1]);
					vVertexNormalBuffer.emplace_back(tempVertexNormal[2]);
					vVertexDetailBuffer.emplace_back(tempVertexDetail[0]);
					vVertexDetailBuffer.emplace_back(tempVertexDetail[1]);
					vVertexDetailBuffer.emplace_back(tempVertexDetail[2]);
					vPolyVertices.emplace_back(tempVertex[0]);
					vPolyVertices.emplace_back(tempVertex[1]);
					vPolyVertices.emplace_back(tempVertex[2]);
				}

				std::vector<uint32_t> vTriangleListIndices;
				R_PolygonToTriangleList(vPolyVertices, vTriangleListIndices);

				for (size_t k = 0; k < vTriangleListIndices.size(); ++k)
				{
					vIndiceBuffer.emplace_back(nPolyStartIndex + vTriangleListIndices[k]);
				}

				pBrushFace->poly_count ++;
			}

			uint32_t nBrushCurrentIndex = (uint32_t)vIndiceBuffer.size();
			pBrushFace->start_index = nBrushStartIndex;
			pBrushFace->index_count = nBrushCurrentIndex - nBrushStartIndex;
		}
	}

	pWorldModel->hEBO = GL_GenBuffer();
	GL_UploadDataToEBOStaticDraw(pWorldModel->hEBO, sizeof(uint32_t) * vIndiceBuffer.size(), vIndiceBuffer.data());

	pWorldModel->hVBO[WSURF_VBO_POSITION] = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_POSITION], sizeof(brushvertexpos_t) * vVertexPosBuffer.size(), vVertexPosBuffer.data());

	pWorldModel->hVBO[WSURF_VBO_DIFFUSE] = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_DIFFUSE], sizeof(brushvertexdiffuse_t) * vVertexDiffuseBuffer.size(), vVertexDiffuseBuffer.data());

	pWorldModel->hVBO[WSURF_VBO_LIGHTMAP] = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_LIGHTMAP], sizeof(brushvertexlightmap_t) * vVertexLightmapBuffer.size(), vVertexLightmapBuffer.data());

	pWorldModel->hVBO[WSURF_VBO_NORMAL] = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_NORMAL], sizeof(brushvertexnormal_t) * vVertexNormalBuffer.size(), vVertexNormalBuffer.data());

	pWorldModel->hVBO[WSURF_VBO_DETAIL] = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_DETAIL], sizeof(brushvertexdetail_t) * vVertexDetailBuffer.size(), vVertexDetailBuffer.data());

	return pWorldModel;
}

CWorldSurfaceWorldModel* R_GetWorldSurfaceWorldModel(model_t* mod)
{
	if (mod->type != mod_brush)
	{
		Sys_Error("R_GetWorldSurfaceWorldModel: invalid mod type!");
		return nullptr;
	}

	if (mod->name[0] == '*')
	{
		Sys_Error("R_GetWorldSurfaceWorldModel: invalid name \"%s\"!", mod->name);
		return nullptr;
	}

	int modelindex = EngineGetModelIndex(mod);

	if (modelindex < (int)g_WorldSurfaceWorldModels.size() && g_WorldSurfaceWorldModels[modelindex])
	{
		return g_WorldSurfaceWorldModels[modelindex];
	}

	if (modelindex >= (int)g_WorldSurfaceWorldModels.size())
	{
		g_WorldSurfaceWorldModels.resize(modelindex + 1);
	}

	auto pWorldModel = R_GenerateWorldSurfaceWorldModel(mod);

	g_WorldSurfaceWorldModels[modelindex] = pWorldModel;

	return pWorldModel;
}

void R_GenerateSceneUBO(void)
{
	if (g_WorldSurfaceRenderer.hSceneUBO)
		return;

	g_WorldSurfaceRenderer.hSceneUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, g_WorldSurfaceRenderer.hSceneUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(scene_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_SCENE_UBO, g_WorldSurfaceRenderer.hSceneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	g_WorldSurfaceRenderer.hCameraUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, g_WorldSurfaceRenderer.hCameraUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(camera_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_CAMERA_UBO, g_WorldSurfaceRenderer.hCameraUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	g_WorldSurfaceRenderer.hDLightUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, g_WorldSurfaceRenderer.hDLightUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(dlight_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_DLIGHT_UBO, g_WorldSurfaceRenderer.hDLightUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	g_WorldSurfaceRenderer.hEntityUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, g_WorldSurfaceRenderer.hEntityUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(entity_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//15 MBytes of VRAM
	g_WorldSurfaceRenderer.hDecalVBO = GL_GenBuffer();
	GL_UploadDataToVBODynamicDraw(g_WorldSurfaceRenderer.hDecalVBO, sizeof(decalvertex_t) * MAX_DECALVERTS * MAX_DECALS, NULL);

	g_WorldSurfaceRenderer.hDecalEBO = GL_GenBuffer();
	GL_UploadDataToEBODynamicDraw(g_WorldSurfaceRenderer.hDecalEBO, sizeof(uint32_t) * MAX_DECALINDICES * MAX_DECALS, NULL);

	g_WorldSurfaceRenderer.hDecalVAO = GL_GenVAO();

	GL_BindStatesForVAO(
		g_WorldSurfaceRenderer.hDecalVAO, 
		g_WorldSurfaceRenderer.hDecalVBO, 
		g_WorldSurfaceRenderer.hDecalEBO,
	[]() {
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_POSITION);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMAL);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD);
		glEnableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_STYLES);
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_POSITION, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, pos));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMAL, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, normal));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_S_TANGENT, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, s_tangent));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_T_TANGENT, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, t_tangent));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_TEXCOORD, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, texcoord));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD, 3, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, lightmaptexcoord));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, replacetexcoord));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, detailtexcoord));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, normaltexcoord));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, parallaxtexcoord));
		glVertexAttribPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD, 2, GL_FLOAT, false, sizeof(decalvertex_t), OFFSET(decalvertex_t, speculartexcoord));
		glVertexAttribIPointer(WSURF_VERTEX_ATTRIBUTE_INDEX_STYLES, 4, GL_UNSIGNED_BYTE, sizeof(decalvertex_t), OFFSET(decalvertex_t, styles));
	},
	[]() {
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_POSITION);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMAL);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_S_TANGENT);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_T_TANGENT);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD);
		glDisableVertexAttribArray(WSURF_VERTEX_ATTRIBUTE_INDEX_STYLES);
	});
	
	if (g_bUseOITBlend)
	{
		size_t fragmentBufferSizeBytes = sizeof(FragmentNode) * MAX_NUM_NODES * glwidth * glheight;

		g_WorldSurfaceRenderer.hOITFragmentSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_WorldSurfaceRenderer.hOITFragmentSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, fragmentBufferSizeBytes, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_OIT_FRAGMENT_SSBO, g_WorldSurfaceRenderer.hOITFragmentSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		size_t numFragmentBufferSizeBytes = sizeof(uint32_t) * glwidth * glheight;

		g_WorldSurfaceRenderer.hOITNumFragmentSSBO = GL_GenBuffer();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_WorldSurfaceRenderer.hOITNumFragmentSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numFragmentBufferSizeBytes, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_OIT_NUMFRAGMENT_SSBO, g_WorldSurfaceRenderer.hOITNumFragmentSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		g_WorldSurfaceRenderer.hOITAtomicSSBO = GL_GenBuffer();
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, g_WorldSurfaceRenderer.hOITAtomicSSBO);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BINDING_POINT_OIT_COUNTER_SSBO, g_WorldSurfaceRenderer.hOITAtomicSSBO);
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
	memset(g_WorldSurfaceRenderer.vCachedDecals, 0, sizeof(g_WorldSurfaceRenderer.vCachedDecals));
}

void R_DrawWorldSurfaceLeafBegin(CWorldSurfaceLeaf* pLeaf, int VBOStates)
{
	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	auto hVAO = R_BindVAOForWorldSurfaceWorldModel(pLeaf->pModel->pWorldModel, VBOStates);

	GL_BindVAO(hVAO);
	GL_BindABO(pLeaf->hABO);
}

void R_DrawWorldSurfaceLeafEnd()
{
	GL_BindABO(0);
	GL_BindVAO(0);
	glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
}

void R_DrawWorldSurfaceLeafSolid(CWorldSurfaceLeaf *pLeaf, bool bWithSky)
{
	const auto& texchain = bWithSky ? pLeaf->TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_SOLID_WITH_SKY] : pLeaf->TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_SOLID];

	if (!texchain.drawCount)
		return;

	program_state_t WSurfProgramState = 0;

	if (R_IsRenderingGBuffer())
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}

	if (R_IsRenderingShadowView())
	{
		WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
	}

	if (R_IsRenderingWaterView())
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	R_DrawWorldSurfaceLeafBegin(pLeaf, (1 << WSURF_VBO_POSITION));

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	if (r_draw_opaque)
	{
		GL_BeginStencilWrite(STENCIL_MASK_WORLD | STENCIL_MASK_HAS_DECAL, STENCIL_MASK_ALL);
	}
	else
	{
		GL_BeginStencilWrite(STENCIL_MASK_HAS_DECAL, STENCIL_MASK_HAS_DECAL);
	}

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(texchain.startDrawOffset), texchain.drawCount, 0);

	r_wsurf_drawcall++;
	r_wsurf_polys += texchain.polyCount;

	GL_EndStencil();

	GL_UseProgram(0);

	R_DrawWorldSurfaceLeafEnd();

}//R_DrawWorldSurfaceLeafSolid

void R_DrawWorldSurfaceLeafStatic(CWorldSurfaceModel *pModel, CWorldSurfaceLeaf* pLeaf, bool bUseZPrePass)
{
	const auto& vTexChainList = pLeaf->vTextureChainList[WSURF_TEXCHAIN_LIST_STATIC];

	for (size_t i = 0; i < vTexChainList.size(); ++i)
	{
		const auto &texchain = vTexChainList[i];

		auto base = texchain.texture;

		program_state_t WSurfProgramState = 0;

		if (g_WorldSurfaceRenderer.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;

			GL_Bind(base->gl_texturenum);

			R_BeginDetailTextureByDetailTextureCache(texchain.detailTextureCache, &WSurfProgramState);
		}

		if (g_WorldSurfaceRenderer.bLightmapTexture)
		{
			WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;

			if (r_fullbright->value || !(*cl_worldmodel)->lightdata)
			{
				WSurfProgramState |= WSURF_FULLBRIGHT_ENABLED;
			}

			if ((*filterMode) != 0)
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

		if (g_WorldSurfaceRenderer.bShadowmapTexture)
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

		if (R_IsRenderingWaterView())
		{
			WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
		}

		if (g_bPortalClipPlaneEnabled[0])
		{
			WSurfProgramState |= WSURF_CLIP_ENABLED;
		}

		if (R_IsRenderingShadowView())
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		if (R_IsRenderingGBuffer())
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
			WSurfProgramState &= ~(WSURF_LEGACY_DLIGHT_ENABLED);
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

		if (!R_IsRenderingGBuffer())
		{
			if ((WSurfProgramState & WSURF_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((WSurfProgramState & WSURF_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
			{
				if (R_IsRenderingFog())
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
			}
		}

		if (R_IsRenderingGammaBlending())
		{
			WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (WSurfProgramState & (WSURF_ALPHA_BLEND_ENABLED | WSURF_ADDITIVE_BLEND_ENABLED)))
		{
			WSurfProgramState |= WSURF_OIT_BLEND_ENABLED;
		}

		int VBOStates = (1 << WSURF_VBO_POSITION);

		if (WSurfProgramState & (WSURF_DIFFUSE_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_DIFFUSE);
		}
		if (WSurfProgramState & (WSURF_LIGHTMAP_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_LIGHTMAP);
		}
		if (WSurfProgramState & (WSURF_NORMALTEXTURE_ENABLED | WSURF_PARALLAXTEXTURE_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_NORMAL);
		}
		if (WSurfProgramState & (WSURF_REPLACETEXTURE_ENABLED | WSURF_DETAILTEXTURE_ENABLED | WSURF_PARALLAXTEXTURE_ENABLED | WSURF_SPECULARTEXTURE_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_DETAIL);
		}

		R_DrawWorldSurfaceLeafBegin(pLeaf, VBOStates);

		if (r_draw_opaque)
		{
			GL_BeginStencilWrite(STENCIL_MASK_WORLD | STENCIL_MASK_HAS_DECAL, STENCIL_MASK_ALL);
		}
		else
		{
			GL_BeginStencilWrite(STENCIL_MASK_HAS_DECAL, STENCIL_MASK_HAS_DECAL);
		}

		wsurf_program_t prog = { 0 };
		R_UseWSurfProgram(WSurfProgramState, &prog);

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(texchain.startDrawOffset), texchain.drawCount, 0);

		R_EndDetailTexture(WSurfProgramState);

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.polyCount;

		GL_UseProgram(0);

		GL_EndStencil();

		R_DrawWorldSurfaceLeafEnd();
	}
}//R_DrawWorldSurfaceLeafStatic

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
					{
						Sys_Error("R_TextureAnimation: broken cycle");
					}
					if (++loop_count > 100)
					{
						Sys_Error("R_TextureAnimation: infinite cycle");
					}
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
			{
				Sys_Error("R_TextureAnimation: broken cycle");
			}
			if (++loop_count > 100)
			{
				Sys_Error("R_TextureAnimation: infinite cycle");
			}
		}
	}
	return base;
}

void R_DrawWorldSurfaceLeafAnim(CWorldSurfaceModel *pModel, CWorldSurfaceLeaf* pLeaf, bool bUseZPrePass)
{
	const auto& vTexChainList = pLeaf->vTextureChainList[WSURF_TEXCHAIN_LIST_ANIM];

	for (size_t i = 0; i < vTexChainList.size(); ++i)
	{
		auto &texchain = vTexChainList[i];

		auto texture = R_GetAnimatedTexture(texchain.texture);

		program_state_t WSurfProgramState = 0;

		if (g_WorldSurfaceRenderer.bDiffuseTexture)
		{
			WSurfProgramState |= WSURF_DIFFUSE_ENABLED;

			GL_Bind(texture->gl_texturenum);

			R_BeginDetailTextureByGLTextureId(texture->gl_texturenum, &WSurfProgramState);
		}

		if (g_WorldSurfaceRenderer.bLightmapTexture)
		{
			WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;

			if (r_fullbright->value || !(*cl_worldmodel)->lightdata)
			{
				WSurfProgramState |= WSURF_FULLBRIGHT_ENABLED;
			}

			if ((*filterMode) != 0)
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

		if (g_WorldSurfaceRenderer.bShadowmapTexture)
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

		if (R_IsRenderingWaterView())
		{
			WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
		}

		if (g_bPortalClipPlaneEnabled[0])
		{
			WSurfProgramState |= WSURF_CLIP_ENABLED;
		}

		if (R_IsRenderingGBuffer())
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;

			WSurfProgramState &= ~(WSURF_LEGACY_DLIGHT_ENABLED);
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

		if (!R_IsRenderingGBuffer())
		{
			if ((WSurfProgramState & WSURF_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((WSurfProgramState & WSURF_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
			{
				if (R_IsRenderingFog())
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
			}
		}

		if (R_IsRenderingGammaBlending())
		{
			WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (WSurfProgramState & (WSURF_ALPHA_BLEND_ENABLED | WSURF_ADDITIVE_BLEND_ENABLED)))
		{
			WSurfProgramState |= WSURF_OIT_BLEND_ENABLED;
		}

		if (R_IsRenderingShadowView())
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		int VBOStates = (1 << WSURF_VBO_POSITION);

		if (WSurfProgramState & (WSURF_DIFFUSE_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_DIFFUSE);
		}
		if (WSurfProgramState & (WSURF_LIGHTMAP_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_LIGHTMAP);
		}
		if (WSurfProgramState & (WSURF_NORMALTEXTURE_ENABLED | WSURF_PARALLAXTEXTURE_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_NORMAL);
		}
		if (WSurfProgramState & (WSURF_REPLACETEXTURE_ENABLED | WSURF_DETAILTEXTURE_ENABLED | WSURF_PARALLAXTEXTURE_ENABLED | WSURF_SPECULARTEXTURE_ENABLED))
		{
			VBOStates |= (1 << WSURF_VBO_DETAIL);
		}

		R_DrawWorldSurfaceLeafBegin(pLeaf, VBOStates);

		if (r_draw_opaque)
		{
			GL_BeginStencilWrite(STENCIL_MASK_WORLD | STENCIL_MASK_HAS_DECAL, STENCIL_MASK_ALL);
		}
		else
		{
			GL_BeginStencilWrite(STENCIL_MASK_HAS_DECAL, STENCIL_MASK_HAS_DECAL);
		}

		wsurf_program_t prog = { 0 };
		R_UseWSurfProgram(WSurfProgramState, &prog);

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(texchain.startDrawOffset), texchain.drawCount, 0);

		R_EndDetailTexture(WSurfProgramState);

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.polyCount;

		GL_UseProgram(0);

		GL_EndStencil();

		R_DrawWorldSurfaceLeafEnd();
	}
}//R_DrawWorldSurfaceLeafAnim

void R_DrawWorldSurfaceLeafSky(CWorldSurfaceModel* pModel, CWorldSurfaceLeaf* pLeaf, bool bUseZPrePass)
{
	const auto& texchain = pLeaf->TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_SKY];

	if (!texchain.drawCount)
		return;

	auto texture = texchain.texture;

	program_state_t WSurfProgramState = 0;

	if (texture)
	{
		WSurfProgramState |= WSURF_DIFFUSE_ENABLED;

		GL_Bind(texture->gl_texturenum);
	}

	if (R_IsRenderingWaterView())
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}

	if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	if (R_IsRenderingGBuffer())
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;

		WSurfProgramState &= ~(WSURF_LEGACY_DLIGHT_ENABLED);
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

	if ((int)r_wsurf_sky_fog->value >= 1)
	{
		if (!R_IsRenderingGBuffer())
		{
			if ((WSurfProgramState & WSURF_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((WSurfProgramState & WSURF_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
			{
				if (R_IsRenderingFog())
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
			}
		}
	}

	if (R_IsRenderingGammaBlending())
	{
		WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
	}

	if (r_draw_oitblend && (WSurfProgramState & (WSURF_ALPHA_BLEND_ENABLED | WSURF_ADDITIVE_BLEND_ENABLED)))
	{
		WSurfProgramState |= WSURF_OIT_BLEND_ENABLED;
	}

	if (R_IsRenderingShadowView())
	{
		WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
	}

	int VBOStates = (1 << WSURF_VBO_POSITION);

	if (WSurfProgramState & (WSURF_DIFFUSE_ENABLED))
	{
		VBOStates |= (1 << WSURF_VBO_DIFFUSE);
	}
	if (WSurfProgramState & (WSURF_LIGHTMAP_ENABLED))
	{
		VBOStates |= (1 << WSURF_VBO_LIGHTMAP);
	}
	if (WSurfProgramState & (WSURF_NORMALTEXTURE_ENABLED | WSURF_PARALLAXTEXTURE_ENABLED))
	{
		VBOStates |= (1 << WSURF_VBO_NORMAL);
	}
	if (WSurfProgramState & (WSURF_REPLACETEXTURE_ENABLED | WSURF_DETAILTEXTURE_ENABLED | WSURF_PARALLAXTEXTURE_ENABLED | WSURF_SPECULARTEXTURE_ENABLED))
	{
		VBOStates |= (1 << WSURF_VBO_DETAIL);
	}

	R_DrawWorldSurfaceLeafBegin(pLeaf, VBOStates);

	if (r_draw_opaque)
	{
		GL_BeginStencilWrite(STENCIL_MASK_WORLD | STENCIL_MASK_NO_SHADOW, STENCIL_MASK_ALL);
	}
	else
	{
		GL_BeginStencilWrite(STENCIL_MASK_NO_SHADOW, STENCIL_MASK_NO_SHADOW);
	}

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(texchain.startDrawOffset), texchain.drawCount, 0);

	r_wsurf_drawcall++;
	r_wsurf_polys += texchain.polyCount;

	GL_UseProgram(0);

	GL_EndStencil();

	R_DrawWorldSurfaceLeafEnd();

}//R_DrawWorldSurfaceLeafSky

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
	if (R_IsRenderingShadowView())
		return false;

	return (r_wsurf_zprepass->value > 0) ? true : false;
}

void R_DrawWorldSurfaceModel(CWorldSurfaceModel *pModel, cl_entity_t *ent)
{
	entity_ubo_t EntityUBO;

	memcpy(EntityUBO.entityMatrix, r_entity_matrix, sizeof(mat4));
	memcpy(EntityUBO.color, r_entity_color, sizeof(vec4));
	EntityUBO.scrollSpeed = R_ScrollSpeed();

	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_ENTITY_UBO, g_WorldSurfaceRenderer.hEntityUBO);

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hEntityUBO, 0, sizeof(EntityUBO), &EntityUBO);

	if (g_WorldSurfaceRenderer.bShadowmapTexture)
	{
		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_SHADOWMAP_TEXTURE);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r_shadow_texture.color_array_as_depth);
		glActiveTexture(GL_TEXTURE0);
	}

	if (g_WorldSurfaceRenderer.bLightmapTexture)
	{
		for (int lightmap_idx = 0; lightmap_idx < MAXLIGHTMAPS; ++lightmap_idx)
		{
			if (g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx])
			{
				glActiveTexture(GL_TEXTURE0 + WSURF_BIND_LIGHTMAP_TEXTURE_0 + lightmap_idx);
				glBindTexture(GL_TEXTURE_2D_ARRAY, g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx]);
				glActiveTexture(GL_TEXTURE0);
			}
		}
	}

	bool bUseZPrePass = false;

	CWorldSurfaceLeaf* pLeaf = NULL;

	if (pModel->mod == (*cl_worldmodel))
	{
		int leafIndex = R_GetWorldLeafIndex((*cl_worldmodel), (*r_viewleaf));

		if (leafIndex >= 0 && leafIndex < (int)pModel->vLeaves.size())
		{
			pLeaf = pModel->vLeaves[leafIndex];

			if (!pLeaf && (int)r_leaf_lazy_load->value >= 1)
			{
				R_GenerateWorldSurfaceModelLeaf(pModel, leafIndex);

				pLeaf = pModel->vLeaves[leafIndex];
			}

			if (pLeaf && pLeaf->hABO)
			{
				if (R_IsRenderingWaterView())
				{
					auto pNoVisLeaf = pModel->vLeaves[0];

					if (!pNoVisLeaf && (int)r_leaf_lazy_load->value >= 1)
					{
						R_GenerateWorldSurfaceModelLeaf(pModel, 0);

						pNoVisLeaf = pModel->vLeaves[0];
					}

					if (pNoVisLeaf && pNoVisLeaf->hABO)
					{
						if (R_ShouldDrawZPrePass())
						{
							glColorMask(0, 0, 0, 0);

							R_DrawWorldSurfaceLeafSolid(pNoVisLeaf, true);

							glColorMask(1, 1, 1, 1);

							glDepthFunc(GL_EQUAL);

							bUseZPrePass = true;
						}

						glColorMask(0, 0, 0, 0);

						R_DrawWorldSurfaceLeafSky(pModel, pNoVisLeaf, false);

						glColorMask(1, 1, 1, 1);

						R_DrawWorldSurfaceLeafStatic(pModel, pNoVisLeaf, false);
						R_DrawWorldSurfaceLeafAnim(pModel, pNoVisLeaf, false);

						if (bUseZPrePass)
						{
							glDepthFunc(GL_LEQUAL);
						}
					}
				}
				else
				{
					if (R_ShouldDrawZPrePass())
					{
						glColorMask(0, 0, 0, 0);

						R_DrawWorldSurfaceLeafSolid(pLeaf, true);

						glColorMask(1, 1, 1, 1);

						glDepthFunc(GL_EQUAL);

						bUseZPrePass = true;
					}

					glColorMask(0, 0, 0, 0);

					R_DrawWorldSurfaceLeafSky(pModel, pLeaf, bUseZPrePass);

					glColorMask(1, 1, 1, 1);

					R_DrawWorldSurfaceLeafStatic(pModel, pLeaf, bUseZPrePass);
					R_DrawWorldSurfaceLeafAnim(pModel, pLeaf, bUseZPrePass);

					if (bUseZPrePass)
					{
						glDepthFunc(GL_LEQUAL);
					}
				}
			}
		}
		else
		{
			Sys_Error("R_DrawWorldSurfaceModel: Invalid leaf index %d", leafIndex);
		}
	}
	else
	{
		if (pModel->vLeaves.size() >= 1)
		{
			pLeaf = pModel->vLeaves[0];

			if (pLeaf && pLeaf->hABO)
			{
				R_DrawWorldSurfaceLeafStatic(pModel, pLeaf, bUseZPrePass);
				R_DrawWorldSurfaceLeafAnim(pModel, pLeaf, bUseZPrePass);
			}
		}
		else
		{
			Sys_Error("R_DrawWSurfVBO: Invalid leaf index");
		}
	}

	R_DrawDecals(ent);

	GL_ClearStencil(STENCIL_MASK_HAS_DECAL);

	if (g_WorldSurfaceRenderer.bShadowmapTexture)
	{
		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_SHADOWMAP_TEXTURE);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glActiveTexture(GL_TEXTURE0);
	}

	if (g_WorldSurfaceRenderer.bLightmapTexture)
	{
		for (int lightmap_idx = 0; lightmap_idx < MAXLIGHTMAPS; ++lightmap_idx)
		{
			if (g_WorldSurfaceRenderer.iLightmapTextureArray[lightmap_idx])
			{
				glActiveTexture(GL_TEXTURE0 + WSURF_BIND_LIGHTMAP_TEXTURE_0 + lightmap_idx);
				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			}
		}
		glActiveTexture(GL_TEXTURE0);
	}

	if (pLeaf)
	{
		R_DrawWaters(pModel, pLeaf, ent);
	}
}

void R_InitWSurf(void)
{
	r_wsurf_parallax_scale = gEngfuncs.pfnRegisterVariable("r_wsurf_parallax_scale", "-0.02", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_sky_fog = gEngfuncs.pfnRegisterVariable("r_wsurf_sky_fog", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_zprepass = gEngfuncs.pfnRegisterVariable("r_wsurf_zprepass", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	R_ClearBSPEntities();
}

void R_FreeWorldResources(void)
{
	R_ClearDecalCache();
	R_ClearDetailTextureCache();

	R_ClearBSPEntities();

	R_FreeLightmapTextures();

	R_ClearWorldSurfaceModels();
	R_ClearWorldSurfaceWorldModels();
}

void R_LoadWorldResources(void)
{
	R_LoadMapDetailTextures();
	R_LoadBaseDetailTextures();
	R_LoadBaseDecalTextures();

	std::vector<bspentity_t*> vBSPEntities;

	R_ParseBSPEntities((*cl_worldmodel)->entities, vBSPEntities);
	R_LoadExternalEntities(vBSPEntities);
	R_LoadBSPEntities(vBSPEntities);

	for (auto ent : vBSPEntities)
	{
		delete ent;
	}

	for (int j = 1; j < EngineGetMaxClientModels(); j++)
	{
		auto mod = gEngfuncs.hudGetModelByIndex(j);

		if (!mod)
			break;

		if (mod->type == mod_brush)
		{
			R_GetWorldSurfaceModel(mod);
		}
	}
}

void R_ShutdownWSurf(void)
{
	g_WSurfProgramTable.clear();

	R_ClearDecalCache();
	R_ClearDetailTextureCache();

	R_ClearWorldSurfaceModels();
	R_ClearWorldSurfaceWorldModels();

	R_FreeLightmapTextures();

	R_ClearBSPEntities();

	R_FreeSceneUBO();
}

void R_LoadDecalTextures(const char * pFileContent)
{
	auto ptext = pFileContent;
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
				gEngfuncs.Con_DPrintf("R_LoadDecalTextures: \"%s\" already exists for basetexture \"%s\".\n", textypeNames[texType], base.c_str());
				continue;
			}

			bool bLoaded = false;
			gl_loadtexture_result_t loadResult;
			std::string texturePath;

			//Texture name starts with "maps\\" or "maps/"
			if (!bLoaded &&
				!strnicmp(detailtexture, "maps", sizeof("maps") - 1) &&
				(detailtexture[sizeof("maps") - 1] == '\\' || detailtexture[sizeof("maps") - 1] == '/'))
			{
				texturePath = detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_WORLD, textypeHasMipmap[texType], &loadResult);
			}

			//Search under gfx
			if (!bLoaded)
			{
				texturePath = "gfx/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_WORLD, textypeHasMipmap[texType], &loadResult);
			}

			//Search under renderer/texture
			if (!bLoaded)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_WORLD, textypeHasMipmap[texType], &loadResult);
			}

			if (!bLoaded)
			{
				gEngfuncs.Con_DPrintf("R_LoadDecalTextures: Failed to load \"%s\" as \"%s\" for basetexture \"%s\".\n", detailtexture, textypeNames[texType], base.c_str());
				continue;
			}

			cache->tex[texType].gltexturenum = loadResult.gltexturenum;
			cache->tex[texType].width = loadResult.width;
			cache->tex[texType].height = loadResult.height;
			cache->tex[texType].scaleX = i_xscale;
			cache->tex[texType].scaleY = i_yscale;
		}
	}
}

void R_LoadBaseDecalTextures(void)
{
	char *pfile = (char *)gEngfuncs.COM_LoadFile("renderer/decal_textures.txt", 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_DPrintf("R_LoadBaseDecalTextures: No decal texture file \"renderer/decal_textures.txt\"\n");
		return;
	}

	R_LoadDecalTextures(pfile);

	gEngfuncs.COM_FreeFile(pfile);
}

void R_LoadDetailTextures(const char *pFileContent)
{
	auto ptext = pFileContent;
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

			bool bLoaded = false;
			gl_loadtexture_result_t loadResult;
			std::string texturePath;

			//Texture name starts with "maps\\" or "maps/"
			if (!bLoaded &&
				!strnicmp(detailtexture, "maps", sizeof("maps") - 1) &&
				(detailtexture[sizeof("maps") - 1] == '\\' || detailtexture[sizeof("maps") - 1] == '/'))
			{
				texturePath = detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_WORLD, textypeHasMipmap[texType], &loadResult);
			}

			//Search under gfx
			if (!bLoaded)
			{
				texturePath = "gfx/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_WORLD, textypeHasMipmap[texType], &loadResult);
			}

			//Search under renderer/texture
			if (!bLoaded)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_WORLD, textypeHasMipmap[texType], &loadResult);
			}

			if (!bLoaded)
			{
				gEngfuncs.Con_DPrintf("R_LoadDetailTextures: Failed to load %s as %s for basetexture %s\n", detailtexture, textypeNames[texType], base.c_str());
				continue;
			}

			cache->tex[texType].gltexturenum = loadResult.gltexturenum;
			cache->tex[texType].width = loadResult.width;
			cache->tex[texType].height = loadResult.height;
			cache->tex[texType].scaleX = i_xscale;
			cache->tex[texType].scaleY = i_yscale;
		}
	}
}

void R_LoadBaseDetailTextures(void)
{
	char *pfile = (char *)gEngfuncs.COM_LoadFile("renderer/detail_textures.txt", 5, NULL);
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

	RemoveFileExtension(name);

	name += "_detail.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile(name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_DPrintf("R_LoadMapDetailTextures: No detail texture file %s\n", name.c_str());
		return;
	}

	R_LoadDetailTextures(pfile);

	gEngfuncs.COM_FreeFile(pfile);
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
		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_DETAIL_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
	}

	if (cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum)
	{
		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_NORMAL_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
	}

	if (cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum)
	{
		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_PARALLAX_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum);

		if (WSurfProgramState)
			*WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
	}

	if (cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum)
	{
		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_SPECULAR_TEXTURE);
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

		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_DETAIL_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (WSurfProgramState & WSURF_NORMALTEXTURE_ENABLED)
	{
		bRestore = true;

		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_NORMAL_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (WSurfProgramState & WSURF_PARALLAXTEXTURE_ENABLED)
	{
		bRestore = true;

		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_PARALLAX_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (WSurfProgramState & WSURF_SPECULARTEXTURE_ENABLED)
	{
		bRestore = true;

		glActiveTexture(GL_TEXTURE0 + WSURF_BIND_SPECULAR_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	if (bRestore)
	{
		glActiveTexture(GL_TEXTURE0);
	}
}

const char *ValueForKey(bspentity_t *ent, const char *key)
{
   for (epair_t  *pEPair = ent->epairs; pEPair; pEPair = pEPair->next)
   {
      if (!strcmp(pEPair->key, key) )
         return pEPair->value;
   }
   return NULL;
}

const char* ValueForKeyEx(bspentity_t* ent, const char* key,epair_t **ppLastEPair)
{
	if ((*ppLastEPair))
	{
		for (epair_t* pEPair = (*ppLastEPair)->next; pEPair; pEPair = pEPair->next)
		{
			if (!strcmp(pEPair->key, key))
			{
				(*ppLastEPair) = pEPair;
				return pEPair->value;
			}
		}

	}
	else
	{
		for (epair_t* pEPair = ent->epairs; pEPair; pEPair = pEPair->next)
		{
			if (!strcmp(pEPair->key, key))
			{
				(*ppLastEPair) = pEPair;
				return pEPair->value;
			}
		}
	}
	*ppLastEPair = NULL;
	return NULL;
}

void ValueForKeyExArray(bspentity_t* ent, const char* key, std::vector<const char *> &strArray)
{
	const char* flags = NULL;
	epair_t* ep = NULL;
	do
	{
		flags = ValueForKeyEx(ent, "flags", &ep);

		if (flags) {
			strArray.emplace_back(flags);
		}

	} while (flags);
}

void R_ClearBSPEntities()
{
	g_EnvWaterControls.clear();
	r_flashlight_cone_texture_name.clear();
	g_DynamicLights.clear();
}

static bspentity_t *current_parse_entity = NULL;
static char com_token[4096];

static bool R_ParseBSPEntityKeyValue(
	const char *classname,
	const char *keyname, 
	const char *value, 
	std::vector<bspentity_t*> &vBSPEntities)
{
	if (classname == NULL)
	{
		current_parse_entity = new bspentity_t;

		vBSPEntities.emplace_back(current_parse_entity);

		if (!current_parse_entity)
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

static bool R_ParseBSPEntityClassname(
	const char *szInputStream, 
	char *classname,
	size_t classname_len,
	std::vector<bspentity_t*>& vBSPEntities)
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
			R_ParseBSPEntityKeyValue(NULL, szKeyName, com_token, vBSPEntities);

			strncpy(classname, com_token, classname_len);
			classname[classname_len - 1] = 0;

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

static const char *R_ParseBSPEntity(
	const char *data,
	std::vector<bspentity_t*>& vBSPEntities)
{
	char keyname[256] = { 0 };
	char classname[256] = { 0 };

	if (R_ParseBSPEntityClassname(data, classname, sizeof(classname), vBSPEntities))
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
				Sys_Error("R_ParseBSPEntity: EOF without closing brace");
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
				Sys_Error("R_ParseBSPEntity: EOF without closing brace");
			}

			if (com_token[0] == '}')
			{
				Sys_Error("R_ParseBSPEntity: closing brace without data");
			}

			if (!strcmp(classname, com_token))
			{
				continue;
			}

			R_ParseBSPEntityKeyValue(classname, keyname, com_token, vBSPEntities);
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

void R_ParseBSPEntities(const char *data, std::vector<bspentity_t *> &vBSPEntities)
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
			Sys_Error("R_ParseBSPEntities: found %s when expecting {", com_token);
			return;
		}

		data = R_ParseBSPEntity(data, vBSPEntities);
	}
}

void R_LoadExternalEntities(std::vector<bspentity_t*>& vBSPEntities)
{
	std::string fullPath = gEngfuncs.pfnGetLevelName();

	RemoveFileExtension(fullPath);

	fullPath += "_entity.txt";

	auto pFile = (char *)gEngfuncs.COM_LoadFile(fullPath.c_str(), 5, NULL);

	if (!pFile)
	{
		fullPath = "renderer/default_entity.txt";

		pFile = (char *)gEngfuncs.COM_LoadFile(fullPath.c_str(), 5, NULL);

		if (!pFile)
		{
			gEngfuncs.Con_DPrintf("R_LoadExternalEntities: Could not load \"%s\".\n", fullPath);
			return;
		}
	}

	R_ParseBSPEntities(pFile, vBSPEntities);

	gEngfuncs.COM_FreeFile(pFile);
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

	auto name_string = ValueForKey(ent, "name");
	if (name_string)
	{
		cubemap.name = name_string;
	}

	auto origin_string = ValueForKey(ent, "origin");
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

	auto cubemapsize_string = ValueForKey(ent, "cubemapsize");
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

	auto radius_string = ValueForKey(ent, "radius");
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

	auto extension_string = ValueForKey(ent, "extension");
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

	auto origin_string = ValueForKey(ent, "origin");
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

	auto color_string = ValueForKey(ent, "_light");
	if (color_string)
	{
		float temp[4];
		if (sscanf(color_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			dynlight.color[0] = math_clamp(temp[0], 0, 255) / 255.0f;
			dynlight.color[1] = math_clamp(temp[1], 0, 255) / 255.0f;
			dynlight.color[2] = math_clamp(temp[2], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_light\" in entity \"light_dynamic\"\n");
		}
	}

	auto distance_string = ValueForKey(ent, "_distance");
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

	auto ambient_string = ValueForKey(ent, "_ambient");
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

	auto diffuse_string = ValueForKey(ent, "_diffuse");
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

	auto specular_string = ValueForKey(ent, "_specular");
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

	auto specularpow_string = ValueForKey(ent, "_specularpow");
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
	auto pWaterControl = new CEnvWaterControl;

	auto basetexture_string = ValueForKey(ent, "basetexture");
	if (basetexture_string)
	{
		pWaterControl->basetexture = basetexture_string;
		if (pWaterControl->basetexture[pWaterControl->basetexture.length() - 1] == '*')
		{
			pWaterControl->wildcard = pWaterControl->basetexture.substr(0, pWaterControl->basetexture.length() - 1);
		}
	}

	auto normalmap_string = ValueForKey(ent, "normalmap");

	if (normalmap_string)
	{
		pWaterControl->normalmap = normalmap_string;
	}

	auto fresnelfactor_string = ValueForKey(ent, "fresnelfactor");
	if (fresnelfactor_string)
	{
		float temp[4];
		if (sscanf(fresnelfactor_string, "%f %f %f %f", &temp[0], &temp[1], &temp[2], &temp[3]) == 4)
		{
			pWaterControl->fresnelfactor[0] = math_clamp(temp[0], 0, 999999);
			pWaterControl->fresnelfactor[1] = math_clamp(temp[1], 0, 999999);
			pWaterControl->fresnelfactor[2] = math_clamp(temp[2], 0, 999999);
			pWaterControl->fresnelfactor[3] = math_clamp(temp[3], 0, 1);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"fresnelfactor\" in entity \"env_water_control\", 4 floats are required.\n");
		}
	}

	auto normfactor_string = ValueForKey(ent, "normfactor");
	if (normfactor_string)
	{
		float temp[4];
		if (sscanf(normfactor_string, "%f", &temp[0]) == 1)
		{
			pWaterControl->normfactor = math_clamp(temp[0], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"normfactor\" in entity \"env_water_control\", 2 floats are required.\n");
		}
	}

	auto depthfactor_string = ValueForKey(ent, "depthfactor");
	if (depthfactor_string)
	{
		float temp[4];
		if (sscanf(depthfactor_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			pWaterControl->depthfactor[0] = math_clamp(temp[0], 0, 10);
			pWaterControl->depthfactor[1] = math_clamp(temp[1], 0, 10);
			pWaterControl->depthfactor[2] = math_clamp(temp[2], 0, 999999);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"depthfactor\" in entity \"env_water_control\", 3 floats are required.\n");
		}
	}

	auto minheight_string = ValueForKey(ent, "minheight");
	if (minheight_string)
	{
		float temp[4];
		if (sscanf(minheight_string, "%f", &temp[0]) == 1)
		{
			pWaterControl->minheight = math_clamp(temp[0], 0, 10000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"minheight\" in entity \"env_water_control\", 1 float is required.\n");
		}
	}

	auto maxtrans_string = ValueForKey(ent, "maxtrans");
	if (maxtrans_string)
	{
		float temp[4];
		if (sscanf(maxtrans_string, "%f", &temp[0]) == 1)
		{
			pWaterControl->maxtrans = math_clamp(temp[0], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"maxtrans\" in entity \"env_water_control\", 1 float is required.\n");
		}
	}

	auto speedrate_string = ValueForKey(ent, "speedrate");
	if (speedrate_string)
	{
		float temp[4];
		if (sscanf(speedrate_string, "%f", &temp[0]) == 1)
		{
			pWaterControl->speedrate = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"speedrate\" in entity \"env_water_control\", 1 float is required.\n");
		}
	}

	auto level_string = ValueForKey(ent, "level");
	if (level_string)
	{
		if (!strcmp(level_string, "WATER_LEVEL_LEGACY"))
		{
			pWaterControl->level = WATER_LEVEL_LEGACY;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_SKYBOX"))
		{
			pWaterControl->level = WATER_LEVEL_REFLECT_SKYBOX;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_WORLD"))
		{
			pWaterControl->level = WATER_LEVEL_REFLECT_WORLD;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_ENTITY"))
		{
			pWaterControl->level = WATER_LEVEL_REFLECT_ENTITY;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_REFLECT_SSR"))
		{
			pWaterControl->level = WATER_LEVEL_REFLECT_SSR;
		}
		else if (!strcmp(level_string, "WATER_LEVEL_LEGACY_RIPPLE"))
		{
			pWaterControl->level = WATER_LEVEL_LEGACY_RIPPLE;
		}
		else
		{
			int lv = 0;
			if (sscanf(level_string, "%d", &lv) == 1)
			{
				pWaterControl->level = math_clamp(lv, WATER_LEVEL_LEGACY, WATER_LEVEL_MAX - 1);
			}
			else
			{
				gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"level\" in entity \"env_water_control\", 1 integer is required\n");
			}
		}
	}

	if (pWaterControl->basetexture.length())
	{
		g_EnvWaterControls.emplace_back(pWaterControl);
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
	R_ParseMapCvarSetMapValue(r_flashlight_enable, ValueForKey(ent, "enable"));
	R_ParseMapCvarSetMapValue(r_flashlight_ambient, ValueForKey(ent, "ambient"));
	R_ParseMapCvarSetMapValue(r_flashlight_diffuse, ValueForKey(ent, "diffuse"));
	R_ParseMapCvarSetMapValue(r_flashlight_specular, ValueForKey(ent, "specular"));
	R_ParseMapCvarSetMapValue(r_flashlight_specularpow, ValueForKey(ent, "specularpow"));
	R_ParseMapCvarSetMapValue(r_flashlight_attachment, ValueForKey(ent, "attachment"));
	R_ParseMapCvarSetMapValue(r_flashlight_distance, ValueForKey(ent, "distance"));
	R_ParseMapCvarSetMapValue(r_flashlight_cone_cosine, ValueForKey(ent, "cone_cosine"));
	
	auto cone_texture = ValueForKey(ent, "cone_texture");
	if (cone_texture)
	{
		r_flashlight_cone_texture_name = cone_texture;
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

void R_ParseBSPEntity_Env_Studio_Control(bspentity_t* ent)
{
	R_ParseMapCvarSetMapValue(r_studio_base_specular, ValueForKey(ent, "base_specular"));
	R_ParseMapCvarSetMapValue(r_studio_celshade_specular, ValueForKey(ent, "celshade_specular"));
}

void R_LoadBSPEntities(std::vector<bspentity_t*>& vBSPEntities)
{
	for(auto ent : vBSPEntities)
	{
		auto classname = ent->classname;

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

		else if (!strcmp(classname, "env_studio_control"))
		{
			R_ParseBSPEntity_Env_Studio_Control(ent);
		}
	}//end for
}

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

	R_RotateForEntity(e);

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	R_SetRenderMode(e);

	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		g_WorldSurfaceRenderer.bDiffuseTexture = false;
		g_WorldSurfaceRenderer.bLightmapTexture = false;
	}
	else if ((*currententity)->curstate.rendermode == kRenderTransAlpha || (*currententity)->curstate.rendermode == kRenderNormal)
	{
		g_WorldSurfaceRenderer.bDiffuseTexture = true;
		g_WorldSurfaceRenderer.bLightmapTexture = true;
	}
	else
	{
		g_WorldSurfaceRenderer.bDiffuseTexture = true;
		g_WorldSurfaceRenderer.bLightmapTexture = false;
	}

	g_WorldSurfaceRenderer.bShadowmapTexture = false;

	if(R_ShouldRenderShadowScene() && r_draw_opaque)
		g_WorldSurfaceRenderer.bShadowmapTexture = true;

	auto pModel = R_GetWorldSurfaceModel(clmodel);

	if (!pModel)
		return;

	R_DrawWorldSurfaceModel(pModel, e);

	glDepthMask(GL_TRUE);
	//TODO: Fixed-function can be done in shader
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);
	glDisable(GL_BLEND);
}

void R_SetupCameraUBO(void)
{
	camera_ubo_t CameraUBO;

	memcpy(CameraUBO.viewMatrix, r_world_matrix, sizeof(mat4));
	memcpy(CameraUBO.projMatrix, r_projection_matrix, sizeof(mat4));
	memcpy(CameraUBO.invViewMatrix, r_world_matrix_inv, sizeof(mat4));
	memcpy(CameraUBO.invProjMatrix, r_projection_matrix_inv, sizeof(mat4));

	auto CurrentSceneFBO = GL_GetCurrentSceneFBO();

	if (CurrentSceneFBO)
	{
		CameraUBO.viewport[0] = CurrentSceneFBO->iWidth;
		CameraUBO.viewport[1] = CurrentSceneFBO->iHeight;
		CameraUBO.viewport[2] = MAX_NUM_NODES * glwidth * glheight;
		CameraUBO.viewport[3] = 0;
	}
	else
	{
		CameraUBO.viewport[0] = glwidth;
		CameraUBO.viewport[1] = glheight;
		CameraUBO.viewport[2] = MAX_NUM_NODES * glwidth * glheight;
		CameraUBO.viewport[3] = 0;
	}
	memcpy(CameraUBO.frustum[0], r_frustum_origin[0], sizeof(vec3_t));
	memcpy(CameraUBO.frustum[1], r_frustum_origin[1], sizeof(vec3_t));
	memcpy(CameraUBO.frustum[2], r_frustum_origin[2], sizeof(vec3_t));
	memcpy(CameraUBO.frustum[3], r_frustum_origin[3], sizeof(vec3_t));
	memcpy(CameraUBO.viewpos, (*r_refdef.vieworg), sizeof(vec3_t));
	memcpy(CameraUBO.vpn, vpn, sizeof(vec3_t));
	memcpy(CameraUBO.vright, vright, sizeof(vec3_t));
	memcpy(CameraUBO.vup, vup, sizeof(vec3_t));
	memcpy(CameraUBO.r_origin, r_origin, sizeof(vec3_t));

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);
}

void R_SetupSceneUBO(void)
{
	scene_ubo_t SceneUBO;

	memcpy(SceneUBO.shadowMatrix[0], r_shadow_matrix[0], sizeof(mat4));
	memcpy(SceneUBO.shadowMatrix[1], r_shadow_matrix[1], sizeof(mat4));
	memcpy(SceneUBO.shadowMatrix[2], r_shadow_matrix[2], sizeof(mat4));

	vec3_t vforward;
	gEngfuncs.pfnAngleVectors(r_shadow_angles->GetValues(), vforward, NULL, NULL);
	memcpy(SceneUBO.shadowDirection, vforward, sizeof(vec3_t));
	memcpy(SceneUBO.shadowColor, r_shadow_color->GetValues(), sizeof(vec3_t));
	
	SceneUBO.shadowColor[3] = r_shadow_intensity->GetValue();
	
	memcpy(SceneUBO.shadowFade, r_shadow_distfade->GetValues(), sizeof(vec2_t));
	memcpy(&SceneUBO.shadowFade[2], r_shadow_lumfade->GetValues(), sizeof(vec2_t));

	//normal[0] * x+ normal[1] * y+ normal[2] * z = normal[0] * vert[0] +normal[1] * vert[1] +normal[2] * vert[2]

	if (R_IsRenderingReflectView())
	{
		float equation[4] = { g_CurrentReflectCache->normal[0], g_CurrentReflectCache->normal[1], g_CurrentReflectCache->normal[2], -g_CurrentReflectCache->planedist };
		memcpy(SceneUBO.clipPlane, equation, sizeof(vec4_t));
	}
	else if (R_IsRenderingRefractView())
	{
		float equation[4] = { g_CurrentReflectCache->normal[0], g_CurrentReflectCache->normal[1], -g_CurrentReflectCache->normal[2], g_CurrentReflectCache->planedist };
		memcpy(SceneUBO.clipPlane, equation, sizeof(vec4_t));
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		memcpy(SceneUBO.clipPlane, g_PortalClipPlane[0], sizeof(vec4_t));
	}
	else
	{
		memset(SceneUBO.clipPlane, 0, sizeof(vec4_t));
	}

	//Fog colors are converted to linear space before use.
	memcpy(SceneUBO.fogColor, r_fog_color, sizeof(vec4_t));
	//GammaToLinear(SceneUBO.fogColor);

	SceneUBO.fogStart = r_fog_control[0];
	SceneUBO.fogEnd = r_fog_control[1];
	SceneUBO.fogDensity = r_fog_control[2];
	SceneUBO.cl_time = (*cl_time);

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
	SceneUBO.r_alphamin = gl_alphamin->value;
	SceneUBO.r_additive_shift = r_additive_shift->value;

	if (gl_overbright->value)
		SceneUBO.r_lightscale = 1;
	else
		SceneUBO.r_lightscale = ((pow(2.0f, 1.0f / v_lightgamma->value) * 256) + 0.5) / 256.0f;

	SceneUBO.r_filtercolor[0] = *filterColorRed;
	SceneUBO.r_filtercolor[1] = *filterColorGreen;
	SceneUBO.r_filtercolor[2] = *filterColorBlue;
	SceneUBO.r_filtercolor[3] = *filterBrightness;

	//Use vec4[256/4] instead of float[256] to save vram, float[256] in std140 costs 16 * 256 instead of 4 * 256 bytes due to alignment
	for (int i = 0; i < 256; ++i)
	{
		SceneUBO.r_lightstylevalue[i / 4][i % 4] = d_lightstylevalue[i] * (1.0f / 264.0f);
	}

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hSceneUBO, 0, sizeof(SceneUBO), &SceneUBO);
}

void R_SetupDLightUBO(void)
{
	dlight_ubo_t DLightUBO = { 0 };

	g_WorldSurfaceRenderer.iNumLegacyDLights = 0;

	if (!R_CanRenderGBuffer())
	{
		const auto PointLightCallback = [](PointLightCallbackArgs *args, void *context)
		{
			auto DLightUBO = (dlight_ubo_t*)(context);

				DLightUBO->origin_radius[g_WorldSurfaceRenderer.iNumLegacyDLights][0] = args->origin[0];
				DLightUBO->origin_radius[g_WorldSurfaceRenderer.iNumLegacyDLights][1] = args->origin[1];
				DLightUBO->origin_radius[g_WorldSurfaceRenderer.iNumLegacyDLights][2] = args->origin[2];
				DLightUBO->origin_radius[g_WorldSurfaceRenderer.iNumLegacyDLights][3] = args->radius;

				DLightUBO->color_minlight[g_WorldSurfaceRenderer.iNumLegacyDLights][0] = args->color[0];
				DLightUBO->color_minlight[g_WorldSurfaceRenderer.iNumLegacyDLights][1] = args->color[1];
				DLightUBO->color_minlight[g_WorldSurfaceRenderer.iNumLegacyDLights][2] = args->color[2];

				g_WorldSurfaceRenderer.iNumLegacyDLights++;
		};

		const auto SpotlightCallback = [](SpotLightCallbackArgs *args, void *context)
		{
			auto DLightUBO = (dlight_ubo_t*)(context);
		};

		R_IterateDynamicLights(PointLightCallback, SpotlightCallback, &DLightUBO);
	}

	DLightUBO.active_dlights[0] = g_WorldSurfaceRenderer.iNumLegacyDLights;

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hDLightUBO, 0, sizeof(DLightUBO), &DLightUBO);
}

/*
	Purpose : Setup texture states and SceneUBO for DrawWorld
*/

void R_PrepareDrawWorld(void)
{
	g_WorldSurfaceRenderer.bDiffuseTexture = true;
	g_WorldSurfaceRenderer.bLightmapTexture = false;
	g_WorldSurfaceRenderer.bShadowmapTexture = false;

	//Shall we put this in shadow pass?
	if (R_ShouldRenderShadowScene())
	{
		g_WorldSurfaceRenderer.bShadowmapTexture = true;

		const float bias[16] = {
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f
		};

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

	R_SetupSceneUBO();
	R_SetupCameraUBO();
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

	// 1:1 copy from R_DrawWorld, but with hw.dll!r_worldentity instead of stack entity.

	VectorCopy((*r_refdef.vieworg), modelorg);

	(*currententity) = r_worldentity;
	(*currenttexture) = -1;

	r_worldentity->curstate.rendercolor.r = gWaterColor->r;
	r_worldentity->curstate.rendercolor.g = gWaterColor->g;
	r_worldentity->curstate.rendercolor.b = gWaterColor->b;

	GL_DisableMultitexture();

	//Just for backward-compatibility
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	R_DrawSkyBox();

	//Skip world meshes if we are drawing reflect texture for skybox.
	if (R_IsRenderingReflectView() && g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_SKYBOX)
	{
		
	}
	else
	{
		g_WorldSurfaceRenderer.bDiffuseTexture = true;
		g_WorldSurfaceRenderer.bLightmapTexture = true;

		auto pModel = R_GetWorldSurfaceModel((*cl_worldmodel));

		if (pModel)
		{
			R_DrawWorldSurfaceModel(pModel, (*currententity));
		}
	}

	GL_DisableMultitexture();
}