#include "gl_local.h"
#include "triangleapi.h"
#include "mathlib2.h"

#include <sstream>
#include <algorithm>

static std::vector<studio_vbo_t*> g_StudioVBOCache;

static std::unordered_map<int, studio_vbo_material_t*> g_StudioVBOMaterialCache;

static std::unordered_map<int, studio_skin_cache_t *> g_StudioSkinCache;

static std::unordered_map<studio_bone_handle, studio_bone_cache*, studio_bone_hasher> g_StudioBoneCacheManager;

static std::unordered_map<program_state_t, studio_program_t> g_StudioProgramTable;

static studio_bone_cache g_StudioBoneCaches[MAX_STUDIO_BONE_CACHES];

static studio_bone_cache* g_pStudioBoneFreeCaches = NULL;

static studio_vbo_t* g_CurrentVBOCache = NULL;

static cache_user_t model_texture_cache[MAX_KNOWN_MODELS_SVENGINE][MAX_SKINS];

//Engine private vars

model_t* cl_sprite_white = NULL;
model_t* cl_shellchrome = NULL;
mstudiomodel_t** psubmodel = NULL;
mstudiobodyparts_t** pbodypart = NULL;
studiohdr_t** pstudiohdr = NULL;
model_t** r_model = NULL;
float* r_blend = NULL;
auxvert_t** pauxverts = NULL;
float** pvlightvalues = NULL;
auxvert_t(*auxverts)[MAXSTUDIOVERTS] = NULL;
vec3_t(*lightvalues)[MAXSTUDIOVERTS] = NULL;
float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*rotationmatrix)[3][4] = NULL;
#if 0
int(*g_NormalIndex)[MAXSTUDIOVERTS] = NULL;
#endif
int(*chrome)[MAXSTUDIOVERTS][2] = NULL;
int(*chromeage)[MAXSTUDIOBONES] = NULL;
cl_entity_t* cl_viewent = NULL;
int* g_ForcedFaceFlags = NULL;
int* lightgammatable = NULL;
byte* texgammatable = NULL;
float* g_ChromeOrigin = NULL;
int* r_ambientlight = NULL;
float* r_shadelight = NULL;
vec3_t* r_blightvec = NULL;
vec3_t* r_plightvec = NULL;
float* r_colormix = NULL;
void* tmp_palette = NULL;
int* r_smodels_total = NULL;
int* r_amodels_drawn = NULL;
dlight_t* (*locallight)[3] = NULL;
int* numlight = NULL;
int* r_topcolor = NULL;
int* r_bottomcolor = NULL;

#if 0//unused
player_model_t(*DM_PlayerState)[MAX_CLIENTS] = NULL;
skin_t (*DM_RemapSkin)[64][MAX_SKINS] = NULL;
skin_t* (*pDM_RemapSkin)[2528][MAX_SKINS] = NULL;
int* r_remapindex = NULL;
#endif

//Stats

int r_studio_drawcall = 0;
int r_studio_polys = 0;

//Cvars

cvar_t* r_studio_debug = NULL;

MapConVar* r_studio_base_specular = NULL;
MapConVar* r_studio_celshade_specular = NULL;

cvar_t* r_studio_viewmodel_lightdir_adjust = NULL;

cvar_t* r_studio_celshade = NULL;
cvar_t* r_studio_celshade_midpoint = NULL;
cvar_t* r_studio_celshade_softness = NULL;
cvar_t* r_studio_celshade_shadow_color = NULL;
cvar_t* r_studio_celshade_head_offset = NULL;
cvar_t* r_studio_celshade_lightdir_adjust = NULL;

cvar_t* r_studio_outline = NULL;
cvar_t* r_studio_outline_size = NULL;
cvar_t* r_studio_outline_dark = NULL;

cvar_t* r_studio_rimlight_power = NULL;
cvar_t* r_studio_rimlight_smooth = NULL;
cvar_t* r_studio_rimlight_smooth2 = NULL;
cvar_t* r_studio_rimlight_color = NULL;

cvar_t* r_studio_rimdark_power = NULL;
cvar_t* r_studio_rimdark_smooth = NULL;
cvar_t* r_studio_rimdark_smooth2 = NULL;
cvar_t* r_studio_rimdark_color = NULL;

cvar_t* r_studio_hair_specular_exp = NULL;
cvar_t* r_studio_hair_specular_intensity = NULL;
cvar_t* r_studio_hair_specular_noise = NULL;
cvar_t* r_studio_hair_specular_exp2 = NULL;
cvar_t* r_studio_hair_specular_intensity2 = NULL;
cvar_t* r_studio_hair_specular_noise2 = NULL;
cvar_t* r_studio_hair_specular_smooth = NULL;

cvar_t* r_studio_hair_shadow = NULL;
cvar_t* r_studio_hair_shadow_offset = NULL;

cvar_t* r_studio_legacy_dlight = NULL;
cvar_t* r_studio_legacy_elight = NULL;
cvar_t* r_studio_legacy_elight_radius_scale = NULL;

cvar_t* r_studio_bone_caches = NULL;

cvar_t* r_studio_external_textures = NULL;

bool R_StudioHasOutline()
{
	return r_studio_outline->value > 0 && ((*pstudiohdr)->flags & EF_OUTLINE);
}

bool R_StudioHasHairShadow()
{
	return r_draw_hashair && r_draw_hasface && r_studio_hair_shadow->value > 0 && !r_draw_shadowcaster;
}

void R_StudioClearAllBoneCaches()
{
	for (int i = 0; i < MAX_STUDIO_BONE_CACHES - 1; i++)
		g_StudioBoneCaches[i].m_next = &g_StudioBoneCaches[i + 1];

	g_StudioBoneCaches[MAX_STUDIO_BONE_CACHES - 1].m_next = NULL;

	g_pStudioBoneFreeCaches = &g_StudioBoneCaches[0];

	g_StudioBoneCacheManager.clear();
}

void R_StudioBoneCaches_StartFrame()
{
	R_StudioClearAllBoneCaches();
}

studio_bone_cache* R_StudioBoneCacheAlloc()
{
	if (!g_pStudioBoneFreeCaches)
	{
		gEngfuncs.Con_DPrintf("Studio bone caches overflow!\n");
		return NULL;
	}

	auto pTemp = g_pStudioBoneFreeCaches;
	g_pStudioBoneFreeCaches = pTemp->m_next;

	pTemp->m_next = NULL;

	return pTemp;
}

void R_StudioBoneCacheFree(studio_bone_cache* pTemp)
{
	pTemp->m_next = g_pStudioBoneFreeCaches;
	g_pStudioBoneFreeCaches = pTemp;
}

void R_PrepareStudioVBOSubmodel(
	studiohdr_t* studiohdr, mstudiomodel_t* submodel,
	std::vector<studio_vbo_vertex_t>& vVertex,
	std::vector<unsigned int>& vIndices,
	studio_vbo_submodel_t* vboSubmodel)
{
	auto pstudioverts = (vec3_t*)((byte*)studiohdr + submodel->vertindex);
	auto pstudionorms = (vec3_t*)((byte*)studiohdr + submodel->normindex);
	auto pvertbone = ((byte*)studiohdr + submodel->vertinfoindex);
	auto pnormbone = ((byte*)studiohdr + submodel->norminfoindex);

	vboSubmodel->vMesh.reserve(submodel->nummesh);

	for (int k = 0; k < submodel->nummesh; k++)
	{
		auto pmesh = (mstudiomesh_t*)((byte*)studiohdr + submodel->meshindex) + k;

		auto ptricmds = (short*)((byte*)studiohdr + pmesh->triindex);

		int iStartVertex = vVertex.size();
		int iNumVertex = 0;

		studio_vbo_mesh_t VBOMesh;
		VBOMesh.iStartIndex = vIndices.size();

		int t;
		while (t = *(ptricmds++))
		{
			if (t < 0)
			{
				t = -t;
				//GL_TRIANGLE_FAN;
				int first = -1;
				int prv0 = -1;
				int prv1 = -1;
				int prv2 = -1;
				for (; t > 0; t--, ptricmds += 4)
				{
					if (prv0 != -1 && prv1 != -1 && prv2 != -1)
					{
						vIndices.emplace_back(iStartVertex + first);
						vIndices.emplace_back(iStartVertex + prv2);
						VBOMesh.iIndiceCount += 2;
					}

					vIndices.emplace_back(iStartVertex + iNumVertex);
					VBOMesh.iIndiceCount++;
					VBOMesh.iPolyCount++;
					//VBOMesh.mesh = pmesh;
					VBOMesh.iMeshIndex = k;

					if (first == -1)
						first = iNumVertex;

					prv0 = prv1;
					prv1 = prv2;
					prv2 = iNumVertex;

					vVertex.emplace_back(pstudioverts[ptricmds[0]], pstudionorms[ptricmds[1]], (float)ptricmds[2], (float)ptricmds[3], (int)pvertbone[ptricmds[0]], (int)pnormbone[ptricmds[1]]);
					iNumVertex++;
				}
			}
			else
			{
				//GL_TRIANGLE_STRIP;
				int prv0 = -1;
				int prv1 = -1;
				int prv2 = -1;
				int iNumTri = 0;
				for (; t > 0; t--, ptricmds += 4)
				{
					if (prv0 != -1 && prv1 != -1 && prv2 != -1)
					{
						if ((iNumTri + 1) % 2 == 0)
						{
							vIndices.emplace_back(iStartVertex + prv2);
							vIndices.emplace_back(iStartVertex + prv1);
						}
						else
						{
							vIndices.emplace_back(iStartVertex + prv1);
							vIndices.emplace_back(iStartVertex + prv2);
						}
						VBOMesh.iIndiceCount += 2;
					}

					vIndices.emplace_back(iStartVertex + iNumVertex);
					VBOMesh.iIndiceCount++;
					VBOMesh.iPolyCount++;
					//VBOMesh.mesh = pmesh;
					VBOMesh.iMeshIndex = k;

					iNumTri++;

					prv0 = prv1;
					prv1 = prv2;
					prv2 = iNumVertex;

					vVertex.emplace_back(pstudioverts[ptricmds[0]], pstudionorms[ptricmds[1]], (float)ptricmds[2], (float)ptricmds[3], (int)pvertbone[ptricmds[0]], (int)pnormbone[ptricmds[1]]);
					iNumVertex++;
				}
			}
		}

		vboSubmodel->vMesh.emplace_back(VBOMesh);
	}
}

studio_vbo_t* R_GetStudioVBOFromStudioHeader(studiohdr_t* studiohdr)
{
	studio_vbo_t* VBOData = NULL;

	if (studiohdr->soundtable > 0 && studiohdr->soundtable - 1 < (int)g_StudioVBOCache.size())
	{
		VBOData = (studio_vbo_t*)g_StudioVBOCache[studiohdr->soundtable - 1];

		if (VBOData)
		{
			return VBOData;
		}
	}

	return VBOData;
}

void R_AllocSlotForStudioVBO(studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{
	for (size_t i = 0; i < g_StudioVBOCache.size(); ++i)
	{
		if (g_StudioVBOCache[i] == NULL)
		{
			g_StudioVBOCache[i] = VBOData;
			studiohdr->soundtable = (int)i + 1;
			return;
		}
	}

	studiohdr->soundtable = (int)g_StudioVBOCache.size() + 1;
	g_StudioVBOCache.emplace_back(VBOData);
}

studio_vbo_t* R_PrepareStudioVBO(studiohdr_t* studiohdr)
{
	if (!studiohdr->numbodyparts)
		return NULL;

	studio_vbo_t* VBOData = R_GetStudioVBOFromStudioHeader(studiohdr);

	if (VBOData)
		return VBOData;

	VBOData = new studio_vbo_t;
	//VBOData->studiohdr = studiohdr;

	R_AllocSlotForStudioVBO(studiohdr, VBOData);

	std::vector<studio_vbo_vertex_t> vVertex;
	std::vector<GLuint> vIndices;

	for (int i = 0; i < studiohdr->numbodyparts; i++)
	{
		auto bodypart = (mstudiobodyparts_t*)((byte*)studiohdr + studiohdr->bodypartindex) + i;

		if (bodypart->modelindex && bodypart->nummodels)
		{
			for (int j = 0; j < bodypart->nummodels; ++j)
			{
				auto submodel = (mstudiomodel_t*)((byte*)studiohdr + bodypart->modelindex) + j;

				studio_vbo_submodel_t* vboSubmodel = new studio_vbo_submodel_t;
				//vboSubmodel->submodel = submodel;

				vboSubmodel->iSubmodelIndex = j;

				R_PrepareStudioVBOSubmodel(studiohdr, submodel, vVertex, vIndices, vboSubmodel);

				VBOData->vSubmodels.emplace_back(vboSubmodel);

				submodel->groupindex = VBOData->vSubmodels.size();
			}
		}
	}

	VBOData->hVBO = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(VBOData->hVBO, vVertex.size() * sizeof(studio_vbo_vertex_t), vVertex.data());

	VBOData->hEBO = GL_GenBuffer();
	GL_UploadDataToEBOStaticDraw(VBOData->hEBO, vIndices.size() * sizeof(GLuint), vIndices.data());

	VBOData->hVAO = GL_GenVAO();
	GL_BindStatesForVAO(VBOData->hVAO, VBOData->hVBO, VBOData->hEBO,
		[]() {
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, pos));
		glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, normal));
		glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, texcoord));
		glVertexAttribIPointer(3, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
	},
		[]() {
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
	});

	VBOData->hStudioUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, VBOData->hStudioUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(studio_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	VBOData->celshade_control.base_specular.Init(r_studio_base_specular, 2, ConVar_None);
	VBOData->celshade_control.celshade_specular.Init(r_studio_celshade_specular, 4, ConVar_None);

	VBOData->celshade_control.celshade_midpoint.Init(r_studio_celshade_midpoint, 1, ConVar_None);
	VBOData->celshade_control.celshade_softness.Init(r_studio_celshade_softness, 1, ConVar_None);
	VBOData->celshade_control.celshade_shadow_color.Init(r_studio_celshade_shadow_color, 3, ConVar_Color255);
	VBOData->celshade_control.celshade_head_offset.Init(r_studio_celshade_head_offset, 3, ConVar_None);
	VBOData->celshade_control.celshade_lightdir_adjust.Init(r_studio_celshade_lightdir_adjust, 2, ConVar_None);

	VBOData->celshade_control.outline_size.Init(r_studio_outline_size, 1, ConVar_None);
	VBOData->celshade_control.outline_dark.Init(r_studio_outline_dark, 1, ConVar_None);

	VBOData->celshade_control.rimlight_power.Init(r_studio_rimlight_power, 1, ConVar_None);
	VBOData->celshade_control.rimlight_smooth.Init(r_studio_rimlight_smooth, 1, ConVar_None);
	VBOData->celshade_control.rimlight_smooth2.Init(r_studio_rimlight_smooth2, 2, ConVar_None);
	VBOData->celshade_control.rimlight_color.Init(r_studio_rimlight_color, 3, ConVar_Color255);

	VBOData->celshade_control.rimdark_power.Init(r_studio_rimdark_power, 1, ConVar_None);
	VBOData->celshade_control.rimdark_smooth.Init(r_studio_rimdark_smooth, 1, ConVar_None);
	VBOData->celshade_control.rimdark_smooth2.Init(r_studio_rimdark_smooth2, 2, ConVar_None);
	VBOData->celshade_control.rimdark_color.Init(r_studio_rimdark_color, 3, ConVar_Color255);

	VBOData->celshade_control.hair_specular_exp.Init(r_studio_hair_specular_exp, 1, ConVar_None);
	VBOData->celshade_control.hair_specular_exp2.Init(r_studio_hair_specular_exp2, 1, ConVar_None);
	VBOData->celshade_control.hair_specular_noise.Init(r_studio_hair_specular_noise, 4, ConVar_None);
	VBOData->celshade_control.hair_specular_noise2.Init(r_studio_hair_specular_noise2, 4, ConVar_None);
	VBOData->celshade_control.hair_specular_intensity.Init(r_studio_hair_specular_intensity, 3, ConVar_None);
	VBOData->celshade_control.hair_specular_intensity2.Init(r_studio_hair_specular_intensity2, 3, ConVar_None);
	VBOData->celshade_control.hair_specular_smooth.Init(r_studio_hair_specular_smooth, 2, ConVar_None);
	VBOData->celshade_control.hair_shadow_offset.Init(r_studio_hair_shadow_offset, 2, ConVar_None);

	return VBOData;
}

void R_StudioClearVBOCache(void)
{
	for (size_t i = 0; i < g_StudioVBOCache.size(); ++i)
	{
		if (g_StudioVBOCache[i])
		{
			auto VBOData = g_StudioVBOCache[i];

			if (VBOData->hVAO)
			{
				GL_DeleteVAO(VBOData->hVAO);
			}
			if (VBOData->hVBO)
			{
				GL_DeleteBuffer(VBOData->hVBO);
			}
			if (VBOData->hEBO)
			{
				GL_DeleteBuffer(VBOData->hEBO);
			}
			if (VBOData->hStudioUBO)
			{
				GL_DeleteBuffer(VBOData->hStudioUBO);
			}
			for (auto subm : VBOData->vSubmodels)
			{
				delete subm;
			}

			delete VBOData;

			g_StudioVBOCache[i] = NULL;
		}
	}
}

void R_StudioReloadVBOCache(void)
{
	//Reload VBOCache for all existing models
	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_studio)
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				if (!strcmp(mod->name, "models/player.mdl"))
					r_playermodel = mod;

				auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);

				if (studiohdr)
				{
					auto VBOData = R_PrepareStudioVBO(studiohdr);

					if (VBOData)
					{
						R_StudioLoadExternalFile(mod, studiohdr, VBOData);
						R_StudioLoadTextureModel(mod, studiohdr);
					}
				}
			}
		}
	}
}

void R_StudioTextureAddReferences(model_t* mod, studiohdr_t* studiohdr, std::set<int> &textures)
{
	if (studiohdr->textureindex != 0)
	{
		for (int i = 0; i < studiohdr->numtextures; ++i)
		{
			auto ptexture = (mstudiotexture_t*)((byte*)studiohdr + studiohdr->textureindex);
			ptexture += i;

			if (ptexture->index > 0)
			{
				textures.emplace(ptexture->index);

				gEngfuncs.Con_DPrintf("R_AddReferencedTextures: [tex] [%d].\n", ptexture->index);

				auto VBOMaterial = R_StudioGetVBOMaterialFromTextureId(ptexture->index);

				if (VBOMaterial)
				{
					for (int j = 0; j < STUDIO_MAX_TEXTURE - 1; ++j)
					{
						if (VBOMaterial->textures[j].gltexturenum)
						{
							textures.emplace(VBOMaterial->textures[j].gltexturenum);

							gEngfuncs.Con_DPrintf("R_AddReferencedTextures: [vbotex] [%d].\n", VBOMaterial->textures[j].gltexturenum);
						}
					}
				}
			}
		}
	}
}

void R_UseStudioProgram(program_state_t state, studio_program_t* progOutput)
{
	//Fix bogus state
	if ((state & STUDIO_NF_CELSHADE_EXTENSIONBITS) && !(state & STUDIO_NF_CELSHADE))
	{
		state |= STUDIO_NF_CELSHADE;
	}

	studio_program_t prog = { 0 };

	auto itor = g_StudioProgramTable.find(state);
	if (itor == g_StudioProgramTable.end())
	{
		std::stringstream defs;

		if (state & STUDIO_NF_FLATSHADE)
			defs << "#define STUDIO_NF_FLATSHADE\n";

		if (state & STUDIO_NF_CHROME)
			defs << "#define STUDIO_NF_CHROME\n";

		if (state & STUDIO_NF_FULLBRIGHT)
			defs << "#define STUDIO_NF_FULLBRIGHT\n";

		if (state & STUDIO_NF_ALPHA)
			defs << "#define STUDIO_NF_ALPHA\n";

		if (state & STUDIO_NF_ADDITIVE)
			defs << "#define STUDIO_NF_ADDITIVE\n";

		if (state & STUDIO_NF_MASKED)
			defs << "#define STUDIO_NF_MASKED\n";

		if (state & STUDIO_NF_CELSHADE)
			defs << "#define STUDIO_NF_CELSHADE\n";

		if (state & STUDIO_NF_CELSHADE_FACE)
			defs << "#define STUDIO_NF_CELSHADE_FACE\n";

		if (state & STUDIO_NF_CELSHADE_HAIR)
			defs << "#define STUDIO_NF_CELSHADE_HAIR\n";

		if (state & STUDIO_NF_CELSHADE_HAIR_H)
			defs << "#define STUDIO_NF_CELSHADE_HAIR_H\n";

		if (state & STUDIO_NF_DOUBLE_FACE)
			defs << "#define STUDIO_NF_DOUBLE_FACE\n";

		if (state & STUDIO_NF_OVERBRIGHT)
			defs << "#define STUDIO_NF_OVERBRIGHT\n";

		if (state & STUDIO_NF_NOOUTLINE)
			defs << "#define STUDIO_NF_NOOUTLINE\n";

		if (state & STUDIO_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if (state & STUDIO_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & STUDIO_EXP_FOG_ENABLED)
			defs << "#define EXP_FOG_ENABLED\n";

		if (state & STUDIO_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

		if (state & STUDIO_SHADOW_CASTER_ENABLED)
			defs << "#define SHADOW_CASTER_ENABLED\n";

		if (state & STUDIO_GLOW_SHELL_ENABLED)
			defs << "#define GLOW_SHELL_ENABLED\n";

		if (state & STUDIO_OUTLINE_ENABLED)
			defs << "#define OUTLINE_ENABLED\n";

		if (state & STUDIO_HAIR_SHADOW_ENABLED)
			defs << "#define HAIR_SHADOW_ENABLED\n";

		if (state & STUDIO_CLIP_WATER_ENABLED)
			defs << "#define CLIP_WATER_ENABLED\n";

		if (state & STUDIO_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		if (state & STUDIO_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & STUDIO_ADDITIVE_BLEND_ENABLED)
			defs << "#define ADDITIVE_BLEND_ENABLED\n";

		if ((state & STUDIO_OIT_BLEND_ENABLED) && bUseOITBlend)
			defs << "#define OIT_BLEND_ENABLED\n";

		if (state & STUDIO_GAMMA_BLEND_ENABLED)
			defs << "#define GAMMA_BLEND_ENABLED\n";

		if (state & STUDIO_ADDITIVE_RENDER_MODE_ENABLED)
			defs << "#define ADDITIVE_RENDER_MODE_ENABLED\n";

		if (state & STUDIO_NORMALTEXTURE_ENABLED)
			defs << "#define NORMALTEXTURE_ENABLED\n";

		if (state & STUDIO_PARALLAXTEXTURE_ENABLED)
			defs << "#define PARALLAXTEXTURE_ENABLED\n";

		if (state & STUDIO_SPECULARTEXTURE_ENABLED)
			defs << "#define SPECULARTEXTURE_ENABLED\n";

		if (state & STUDIO_DEBUG_ENABLED)
			defs << "#define STUDIO_DEBUG_ENABLED\n";

		if (state & STUDIO_PACKED_DIFFUSETEXTURE_ENABLED)
			defs << "#define PACKED_DIFFUSETEXTURE_ENABLED\n";

		if (state & STUDIO_PACKED_NORMALTEXTURE_ENABLED)
			defs << "#define PACKED_NORMALTEXTURE_ENABLED\n";

		if (state & STUDIO_PACKED_PARALLAXTEXTURE_ENABLED)
			defs << "#define PACKED_PARALLAXTEXTURE_ENABLED\n";

		if (state & STUDIO_PACKED_SPECULARTEXTURE_ENABLED)
			defs << "#define PACKED_SPECULARTEXTURE_ENABLED\n";

		if (state & STUDIO_ANIMATED_TEXTURE_ENABLED)
			defs << "#define ANIMATED_TEXTURE_ENABLED\n";

		if (state & STUDIO_REVERT_NORMAL_ENABLED)
			defs << "#define REVERT_NORMAL_ENABLED\n";

		if (state & STUDIO_STENCIL_TEXTURE_ENABLED)
			defs << "#define STENCIL_TEXTURE_ENABLED\n";

		if (glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define NV_BINDLESS_ENABLED\n";

		else if (glewIsSupported("GL_ARB_gpu_shader_int64"))
			defs << "#define INT64_BINDLESS_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\studio_shader.vsh", "renderer\\shader\\studio_shader.fsh", def.c_str(), def.c_str(), NULL);
		
		if (prog.program)
		{
			SHADER_UNIFORM(prog, r_base_specular, "r_base_specular");
			SHADER_UNIFORM(prog, r_celshade_specular, "r_celshade_specular");
			SHADER_UNIFORM(prog, r_celshade_softness, "r_celshade_softness");
			SHADER_UNIFORM(prog, r_celshade_shadow_color, "r_celshade_shadow_color");
			SHADER_UNIFORM(prog, r_celshade_head_offset, "r_celshade_head_offset");
			SHADER_UNIFORM(prog, r_celshade_lightdir_adjust, "r_celshade_lightdir_adjust");
			SHADER_UNIFORM(prog, r_rimlight_power, "r_rimlight_power");
			SHADER_UNIFORM(prog, r_rimlight_smooth, "r_rimlight_smooth");
			SHADER_UNIFORM(prog, r_rimlight_smooth2, "r_rimlight_smooth2");
			SHADER_UNIFORM(prog, r_rimlight_color, "r_rimlight_color");
			SHADER_UNIFORM(prog, r_rimdark_power, "r_rimdark_power");
			SHADER_UNIFORM(prog, r_rimdark_smooth, "r_rimdark_smooth");
			SHADER_UNIFORM(prog, r_rimdark_smooth2, "r_rimdark_smooth2");
			SHADER_UNIFORM(prog, r_rimdark_color, "r_rimdark_color");
			SHADER_UNIFORM(prog, r_hair_specular_exp, "r_hair_specular_exp");
			SHADER_UNIFORM(prog, r_hair_specular_noise, "r_hair_specular_noise");
			SHADER_UNIFORM(prog, r_hair_specular_intensity, "r_hair_specular_intensity");
			SHADER_UNIFORM(prog, r_hair_specular_exp2, "r_hair_specular_exp2");
			SHADER_UNIFORM(prog, r_hair_specular_noise2, "r_hair_specular_noise2");
			SHADER_UNIFORM(prog, r_hair_specular_intensity2, "r_hair_specular_intensity2");
			SHADER_UNIFORM(prog, r_hair_specular_smooth, "r_hair_specular_smooth");
			SHADER_UNIFORM(prog, r_hair_shadow_offset, "r_hair_shadow_offset");
			SHADER_UNIFORM(prog, r_outline_dark, "r_outline_dark");
			SHADER_UNIFORM(prog, r_uvscale, "r_uvscale");
			SHADER_UNIFORM(prog, r_packed_stride, "r_packed_stride");
			SHADER_UNIFORM(prog, r_packed_index, "r_packed_index");
			SHADER_UNIFORM(prog, r_framerate_numframes, "r_framerate_numframes");
		}

		g_StudioProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (prog.r_base_specular != -1)
		{
			vec4_t values = { 0 };

			if (g_CurrentVBOCache)
			{
				g_CurrentVBOCache->celshade_control.base_specular.GetValues(values);
			}
			else
			{
				r_studio_base_specular->FetchValues(values);
			}

			glUniform2f(prog.r_base_specular, values[0], values[1]);
		}

		if (prog.r_celshade_specular != -1)
		{
			vec4_t values = { 0 };

			if (g_CurrentVBOCache)
			{
				g_CurrentVBOCache->celshade_control.celshade_specular.GetValues(values);
			}
			else
			{
				r_studio_celshade_specular->FetchValues(values);
			}

			glUniform4f(prog.r_celshade_specular, values[0], values[1], values[2], values[3]);
		}

		if (prog.r_celshade_midpoint != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_celshade_midpoint, g_CurrentVBOCache->celshade_control.celshade_midpoint.GetValue());
			}
			else
			{
				glUniform1f(prog.r_celshade_midpoint, r_studio_celshade_midpoint->value);
			}
		}

		if (prog.r_celshade_softness != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_celshade_softness, g_CurrentVBOCache->celshade_control.celshade_softness.GetValue());
			}
			else
			{
				glUniform1f(prog.r_celshade_softness, r_studio_celshade_softness->value);
			}
		}

		if (prog.r_celshade_shadow_color != -1)
		{
			vec3_t color = { 0 };

			if (g_CurrentVBOCache)
			{
				g_CurrentVBOCache->celshade_control.celshade_shadow_color.GetValues(color);
			}
			else
			{
				R_ParseCvarAsColor3(r_studio_celshade_shadow_color, color);
			}

			glUniform3f(prog.r_celshade_shadow_color, color[0], color[1], color[2]);
		}

		if (prog.r_celshade_head_offset != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec3_t offset = { 0 };
				g_CurrentVBOCache->celshade_control.celshade_head_offset.GetValues(offset);
				glUniform3f(prog.r_celshade_head_offset, offset[0], offset[1], offset[2]);
			}
			else
			{
				vec3_t offset = { 0 };
				R_ParseCvarAsVector3(r_studio_celshade_head_offset, offset);
				glUniform3f(prog.r_celshade_head_offset, offset[0], offset[1], offset[2]);
			}
		}

		if (prog.r_celshade_lightdir_adjust != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec2_t value = { 0 };
				g_CurrentVBOCache->celshade_control.celshade_lightdir_adjust.GetValues(value);
				glUniform2f(prog.r_celshade_lightdir_adjust, value[0], value[1]);
			}
			else
			{
				vec2_t value = { 0 };
				R_ParseCvarAsVector2(r_studio_celshade_lightdir_adjust, value);
				glUniform2f(prog.r_celshade_lightdir_adjust, value[0], value[1]);
			}
		}

		if (prog.r_outline_dark != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_outline_dark, g_CurrentVBOCache->celshade_control.outline_dark.GetValue());
			}
			else
			{
				glUniform1f(prog.r_outline_dark, r_studio_outline_dark->value);
			}
		}

		if (prog.r_rimlight_power != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_rimlight_power, g_CurrentVBOCache->celshade_control.rimlight_power.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimlight_power, r_studio_rimlight_power->value);
			}
		}

		if (prog.r_rimlight_smooth != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_rimlight_smooth, g_CurrentVBOCache->celshade_control.rimlight_smooth.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimlight_smooth, r_studio_rimlight_smooth->value);
			}
		}

		if (prog.r_rimlight_smooth2 != -1)
		{
			vec2_t values = { 0 };

			if (g_CurrentVBOCache)
			{
				g_CurrentVBOCache->celshade_control.rimlight_smooth2.GetValues(values);
			}
			else
			{
				R_ParseCvarAsVector2(r_studio_rimlight_color, values);
			}

			glUniform2f(prog.r_rimlight_smooth2, values[0], values[1]);
		}

		if (prog.r_rimlight_color != -1)
		{
			vec3_t color = { 0 };
			if (g_CurrentVBOCache)
			{
				g_CurrentVBOCache->celshade_control.rimlight_color.GetValues(color);
			}
			else
			{
				R_ParseCvarAsColor3(r_studio_rimlight_color, color);
			}
			glUniform3f(prog.r_rimlight_color, color[0], color[1], color[2]);
		}

		if (prog.r_rimdark_power != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_rimdark_power, g_CurrentVBOCache->celshade_control.rimdark_power.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimdark_power, r_studio_rimdark_power->value);
			}
		}

		if (prog.r_rimdark_smooth != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_rimdark_smooth, g_CurrentVBOCache->celshade_control.rimdark_smooth.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimdark_smooth, r_studio_rimdark_smooth->value);
			}
		}

		if (prog.r_rimdark_smooth2 != -1)
		{
			vec2_t values = { 0 };

			if (g_CurrentVBOCache)
			{
				g_CurrentVBOCache->celshade_control.rimdark_smooth2.GetValues(values);
			}
			else
			{
				R_ParseCvarAsVector2(r_studio_rimdark_color, values);
			}

			glUniform2f(prog.r_rimdark_smooth2, values[0], values[1]);
		}

		if (prog.r_rimdark_color != -1)
		{
			vec3_t color = { 0 };

			if (g_CurrentVBOCache)
			{
				g_CurrentVBOCache->celshade_control.rimdark_color.GetValues(color);
			}
			else
			{
				R_ParseCvarAsColor3(r_studio_rimdark_color, color);
			}

			glUniform3f(prog.r_rimdark_color, color[0], color[1], color[2]);
		}

		if (prog.r_hair_specular_exp != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_hair_specular_exp, g_CurrentVBOCache->celshade_control.hair_specular_exp.GetValue());
			}
			else
			{
				glUniform1f(prog.r_hair_specular_exp, r_studio_hair_specular_exp->value);
			}
		}

		if (prog.r_hair_specular_noise != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec4_t values = { 0 };
				g_CurrentVBOCache->celshade_control.hair_specular_noise.GetValues(values);
				glUniform4f(prog.r_hair_specular_noise, values[0], values[1], values[2], values[3]);
			}
			else
			{
				vec4_t values = { 0 };
				R_ParseCvarAsVector4(r_studio_hair_specular_noise, values);
				glUniform4f(prog.r_hair_specular_noise, values[0], values[1], values[2], values[3]);
			}
		}

		if (prog.r_hair_specular_intensity != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec3_t values = { 0 };
				g_CurrentVBOCache->celshade_control.hair_specular_intensity.GetValues(values);
				glUniform3f(prog.r_hair_specular_intensity, values[0], values[1], values[2]);
			}
			else
			{
				vec3_t values = { 0 };
				R_ParseCvarAsVector3(r_studio_hair_specular_intensity, values);
				glUniform3f(prog.r_hair_specular_intensity, values[0], values[1], values[2]);
			}
		}

		if (prog.r_hair_specular_exp2 != -1)
		{
			if (g_CurrentVBOCache)
			{
				glUniform1f(prog.r_hair_specular_exp2, g_CurrentVBOCache->celshade_control.hair_specular_exp2.GetValue());
			}
			else
			{
				glUniform1f(prog.r_hair_specular_exp2, r_studio_hair_specular_exp2->value);
			}
		}

		if (prog.r_hair_specular_noise2 != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec4_t values = { 0 };
				g_CurrentVBOCache->celshade_control.hair_specular_noise2.GetValues(values);
				glUniform4f(prog.r_hair_specular_noise2, values[0], values[1], values[2], values[3]);
			}
			else
			{
				vec4_t values = { 0 };
				R_ParseCvarAsVector4(r_studio_hair_specular_noise2, values);
				glUniform4f(prog.r_hair_specular_noise2, values[0], values[1], values[2], values[3]);
			}
		}

		if (prog.r_hair_specular_intensity2 != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec3_t values = { 0 };
				g_CurrentVBOCache->celshade_control.hair_specular_intensity2.GetValues(values);
				glUniform3f(prog.r_hair_specular_intensity2, values[0], values[1], values[2]);
			}
			else
			{
				vec3_t values = { 0 };
				R_ParseCvarAsVector3(r_studio_hair_specular_intensity2, values);
				glUniform3f(prog.r_hair_specular_intensity2, values[0], values[1], values[2]);
			}
		}

		if (prog.r_hair_specular_smooth != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec2_t values = { 0 };
				g_CurrentVBOCache->celshade_control.hair_specular_smooth.GetValues(values);
				glUniform2f(prog.r_hair_specular_smooth, values[0], values[1]);
			}
			else
			{
				vec2_t values = { 0 };
				R_ParseCvarAsVector2(r_studio_hair_specular_smooth, values);
				glUniform2f(prog.r_hair_specular_smooth, values[0], values[1]);
			}
		}

		if (prog.r_hair_shadow_offset != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec2_t values = { 0 };
				g_CurrentVBOCache->celshade_control.hair_shadow_offset.GetValues(values);
				glUniform2f(prog.r_hair_shadow_offset, values[0], values[1]);
			}
			else
			{
				vec2_t values = { 0 };
				R_ParseCvarAsVector2(r_studio_hair_shadow_offset, values);
				glUniform2f(prog.r_hair_shadow_offset, values[0], values[1]);
			}
		}

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		g_pMetaHookAPI->SysError("R_UseStudioProgram: Failed to load program!");
	}
}

const program_state_mapping_t s_StudioProgramStateName[] = {
{ STUDIO_GBUFFER_ENABLED				,"STUDIO_GBUFFER_ENABLED"					},
{ STUDIO_LINEAR_FOG_ENABLED				,"STUDIO_LINEAR_FOG_ENABLED"				},
{ STUDIO_EXP_FOG_ENABLED				,"STUDIO_EXP_FOG_ENABLED"					},
{ STUDIO_EXP2_FOG_ENABLED				,"STUDIO_EXP2_FOG_ENABLED"					},
{ STUDIO_SHADOW_CASTER_ENABLED			,"STUDIO_SHADOW_CASTER_ENABLED"				},
{ STUDIO_GLOW_SHELL_ENABLED				,"STUDIO_GLOW_SHELL_ENABLED"				},
{ STUDIO_OUTLINE_ENABLED				,"STUDIO_OUTLINE_ENABLED"					},
{ STUDIO_HAIR_SHADOW_ENABLED			,"STUDIO_HAIR_SHADOW_ENABLED"				},
{ STUDIO_CLIP_WATER_ENABLED				,"STUDIO_CLIP_WATER_ENABLED"				},
{ STUDIO_CLIP_ENABLED					,"STUDIO_CLIP_ENABLED"						},
{ STUDIO_ALPHA_BLEND_ENABLED			,"STUDIO_ALPHA_BLEND_ENABLED"				},
{ STUDIO_ADDITIVE_BLEND_ENABLED			,"STUDIO_ADDITIVE_BLEND_ENABLED"			},
{ STUDIO_OIT_BLEND_ENABLED				,"STUDIO_OIT_BLEND_ENABLED"					},
{ STUDIO_GAMMA_BLEND_ENABLED			,"STUDIO_GAMMA_BLEND_ENABLED"				},
{ STUDIO_ADDITIVE_RENDER_MODE_ENABLED	,"STUDIO_ADDITIVE_RENDER_MODE_ENABLED"		},
{ STUDIO_NORMALTEXTURE_ENABLED			,"STUDIO_NORMALTEXTURE_ENABLED"				},
{ STUDIO_PARALLAXTEXTURE_ENABLED		,"STUDIO_PARALLAXTEXTURE_ENABLED"			},
{ STUDIO_SPECULARTEXTURE_ENABLED		,"STUDIO_SPECULARTEXTURE_ENABLED"			},
{ STUDIO_DEBUG_ENABLED					,"STUDIO_DEBUG_ENABLED"						},
{ STUDIO_PACKED_DIFFUSETEXTURE_ENABLED	,"STUDIO_PACKED_DIFFUSETEXTURE_ENABLED"		},
{ STUDIO_PACKED_NORMALTEXTURE_ENABLED	,"STUDIO_PACKED_NORMALTEXTURE_ENABLED"		},
{ STUDIO_PACKED_PARALLAXTEXTURE_ENABLED	,"STUDIO_PACKED_PARALLAXTEXTURE_ENABLED"	},
{ STUDIO_PACKED_SPECULARTEXTURE_ENABLED	,"STUDIO_PACKED_SPECULARTEXTURE_ENABLED"	},
{ STUDIO_ANIMATED_TEXTURE_ENABLED		,"STUDIO_ANIMATED_TEXTURE_ENABLED"			},
{ STUDIO_REVERT_NORMAL_ENABLED			,"STUDIO_REVERT_NORMAL_ENABLED"				},
{ STUDIO_STENCIL_TEXTURE_ENABLED		,"STUDIO_STENCIL_TEXTURE_ENABLED"			},

{ STUDIO_NF_FLATSHADE					,"STUDIO_NF_FLATSHADE"		},
{ STUDIO_NF_CHROME						,"STUDIO_NF_CHROME"			},
{ STUDIO_NF_FULLBRIGHT					,"STUDIO_NF_FULLBRIGHT"		},
{ STUDIO_NF_NOMIPS						,"STUDIO_NF_NOMIPS"			},
{ STUDIO_NF_ALPHA						,"STUDIO_NF_ALPHA"			},
{ STUDIO_NF_ADDITIVE					,"STUDIO_NF_ADDITIVE"		},
{ STUDIO_NF_MASKED						,"STUDIO_NF_MASKED"			},
{ STUDIO_NF_CELSHADE					,"STUDIO_NF_CELSHADE"		},
{ STUDIO_NF_CELSHADE_FACE				,"STUDIO_NF_CELSHADE_FACE"	},
{ STUDIO_NF_CELSHADE_HAIR				,"STUDIO_NF_CELSHADE_HAIR"	},
{ STUDIO_NF_CELSHADE_HAIR_H				,"STUDIO_NF_CELSHADE_HAIR_H"},
{ STUDIO_NF_DOUBLE_FACE					,"STUDIO_NF_DOUBLE_FACE"	},
{ STUDIO_NF_OVERBRIGHT					,"STUDIO_NF_OVERBRIGHT"		},
{ STUDIO_NF_NOOUTLINE					,"STUDIO_NF_NOOUTLINE"		},
};

void R_SaveStudioProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto& p : g_StudioProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/studio_cache.txt", states, s_StudioProgramStateName, _ARRAYSIZE(s_StudioProgramStateName));
}

void R_LoadStudioProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/studio_cache.txt", s_StudioProgramStateName, _ARRAYSIZE(s_StudioProgramStateName), [](program_state_t state) {

		R_UseStudioProgram(state, NULL);

	});
}

void R_ShutdownStudio(void)
{
	g_StudioProgramTable.clear();

	R_StudioClearAllBoneCaches();
	R_StudioFlushAllSkins();
	R_StudioClearVBOCache();
}

void R_InitStudio(void)
{
	r_studio_debug = gEngfuncs.pfnRegisterVariable("r_studio_debug", "0", FCVAR_CLIENTDLL);

	r_studio_viewmodel_lightdir_adjust = gEngfuncs.pfnRegisterVariable("r_studio_viewmodel_lightdir_adjust", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_celshade = gEngfuncs.pfnRegisterVariable("r_studio_celshade", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_midpoint = gEngfuncs.pfnRegisterVariable("r_studio_celshade_midpoint", "-0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_softness = gEngfuncs.pfnRegisterVariable("r_studio_celshade_softness", "0.05", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_shadow_color = gEngfuncs.pfnRegisterVariable("r_studio_celshade_shadow_color", "160 150 150", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_head_offset = gEngfuncs.pfnRegisterVariable("r_studio_celshade_head_offset", "3.5 2 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_lightdir_adjust = gEngfuncs.pfnRegisterVariable("r_studio_celshade_lightdir_adjust", "0.01 0.001", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_outline = gEngfuncs.pfnRegisterVariable("r_studio_outline", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_outline_size = gEngfuncs.pfnRegisterVariable("r_studio_outline_size", "3.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_outline_dark = gEngfuncs.pfnRegisterVariable("r_studio_outline_dark", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_rimlight_power = gEngfuncs.pfnRegisterVariable("r_studio_rimlight_power", "5.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimlight_smooth = gEngfuncs.pfnRegisterVariable("r_studio_rimlight_smooth", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimlight_smooth2 = gEngfuncs.pfnRegisterVariable("r_studio_rimlight_smooth2", "0.0 0.3", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimlight_color = gEngfuncs.pfnRegisterVariable("r_studio_rimlight_color", "40 40 40", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimdark_power = gEngfuncs.pfnRegisterVariable("r_studio_rimdark_power", "5.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimdark_smooth = gEngfuncs.pfnRegisterVariable("r_studio_rimdark_smooth", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimdark_smooth2 = gEngfuncs.pfnRegisterVariable("r_studio_rimdark_smooth2", "0.0 0.3", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimdark_color = gEngfuncs.pfnRegisterVariable("r_studio_rimdark_color", "50 50 50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_hair_specular_exp = gEngfuncs.pfnRegisterVariable("r_studio_hair_specular_exp", "256", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_specular_intensity = gEngfuncs.pfnRegisterVariable("r_studio_hair_specular_intensity", "1 1 1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_specular_noise = gEngfuncs.pfnRegisterVariable("r_studio_hair_specular_noise", "512 1024 0.1 0.15", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_specular_exp2 = gEngfuncs.pfnRegisterVariable("r_studio_hair_specular_exp2", "8", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_specular_intensity2 = gEngfuncs.pfnRegisterVariable("r_studio_hair_specular_intensity2", "0.8 0.8 0.8", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_specular_noise2 = gEngfuncs.pfnRegisterVariable("r_studio_hair_specular_noise2", "240 320 0.05 0.06", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_specular_smooth = gEngfuncs.pfnRegisterVariable("r_studio_hair_specular_smooth", "0.0 0.3", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_shadow = gEngfuncs.pfnRegisterVariable("r_studio_hair_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_hair_shadow_offset = gEngfuncs.pfnRegisterVariable("r_studio_hair_shadow_offset", "0.3 -0.3", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	//Does legacy dynamic light affects studio models?
	r_studio_legacy_dlight = gEngfuncs.pfnRegisterVariable("r_studio_legacy_dlight", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	//Does legacy entity light affects studio models?
	r_studio_legacy_elight = gEngfuncs.pfnRegisterVariable("r_studio_legacy_elight", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_legacy_elight_radius_scale = gEngfuncs.pfnRegisterVariable("r_studio_legacy_elight_radius_scale", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	//Cache bones to save CPU resources?
	r_studio_bone_caches = gEngfuncs.pfnRegisterVariable("r_studio_bone_caches", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_external_textures = gEngfuncs.pfnRegisterVariable("r_studio_external_textures", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_base_specular = R_RegisterMapCvar("r_studio_base_specular", "1.0 2.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 2, ConVar_None);
	r_studio_celshade_specular = R_RegisterMapCvar("r_studio_celshade_specular", "1.0  36.0  0.4  0.6", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 4, ConVar_None);
}

bool R_IsFlippedViewModel(void)
{
	if (cl_righthand && cl_righthand->value > 0)
	{
		if (cl_viewent == (*currententity))
			return true;
	}

	return false;
}

studiohdr_t* R_StudioGetTextures(const model_t* psubm)
{
	if ((*pstudiohdr)->textureindex == 0)
	{
		auto texmodel = (model_t*)psubm->texinfo;
		if (texmodel)
		{
			auto ptexturehdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(texmodel);

			//Fix: could be nullptr ?
			if (ptexturehdr)
				return ptexturehdr;
		}
		else
		{
			return NULL;
		}
	}

	return (*pstudiohdr);
}

void R_StudioLoadTextureModel(model_t* mod, studiohdr_t* studiohdr)
{
	if (studiohdr->textureindex == 0 && !mod->texinfo)
	{
		//This is actually 260 instead of 256
		char modelname[260];
		strncpy(modelname, mod->name, sizeof(modelname) - 2);
		modelname[sizeof(modelname) - 2] = 0;

		strcpy(&modelname[strlen(modelname) - 4], "T.mdl");

		auto texmodel = IEngineStudio.Mod_ForName(modelname, true);
		mod->texinfo = (mtexinfo_t*)texmodel;
		auto ptexturehdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(texmodel);
		strncpy(ptexturehdr->name, modelname, sizeof(ptexturehdr->name) - 1);
		ptexturehdr->name[sizeof(ptexturehdr->name) - 1] = 0;
	}
}

void R_StudioSetupVBOMaterial(const studio_vbo_t* VBOData, const studio_vbo_material_t* VBOMaterial, studio_setupskin_context_t* context)
{
	if (r_studio_external_textures->value > 0)
	{
		const auto& ReplaceTexture = VBOMaterial->textures[STUDIO_REPLACE_TEXTURE - 1];
		if (ReplaceTexture.gltexturenum)
		{
			if (ReplaceTexture.numframes)
			{
				glActiveTexture(GL_TEXTURE0 + STUDIO_RESERVED_TEXTURE_ANIMATED);
				glBindTexture(GL_TEXTURE_2D_ARRAY, ReplaceTexture.gltexturenum);
				glActiveTexture((*oldtarget));

				context->numframes = ReplaceTexture.numframes;
				context->framerate = ReplaceTexture.framerate;

				(*context->StudioProgramState) |= STUDIO_ANIMATED_TEXTURE_ENABLED;
			}
			else
			{
				GL_Bind(ReplaceTexture.gltexturenum);
			}

			if (ReplaceTexture.scaleX > 0)
			{
				context->width = ReplaceTexture.width * ReplaceTexture.scaleX;
			}
			else if (ReplaceTexture.scaleX < 0)
			{
				context->width = context->width * ReplaceTexture.scaleX * -1;
			}

			if (ReplaceTexture.scaleY > 0)
			{
				context->height = ReplaceTexture.height * ReplaceTexture.scaleY;
			}
			else if (ReplaceTexture.scaleY < 0)
			{
				context->height = context->height * ReplaceTexture.scaleY * -1;
			}
		}

		const auto& NormalTexture = VBOMaterial->textures[STUDIO_NORMAL_TEXTURE - 1];
		if (NormalTexture.gltexturenum)
		{
			glActiveTexture(GL_TEXTURE0 + STUDIO_NORMAL_TEXTURE);
			glBindTexture(GL_TEXTURE_2D, NormalTexture.gltexturenum);
			glActiveTexture((*oldtarget));

			(*context->StudioProgramState) |= STUDIO_NORMALTEXTURE_ENABLED;
		}

		const auto& SpecularTexture = VBOMaterial->textures[STUDIO_SPECULAR_TEXTURE - 1];
		if (SpecularTexture.gltexturenum)
		{
			glActiveTexture(GL_TEXTURE0 + STUDIO_SPECULAR_TEXTURE);
			glBindTexture(GL_TEXTURE_2D, SpecularTexture.gltexturenum);
			glActiveTexture((*oldtarget));

			(*context->StudioProgramState) |= STUDIO_SPECULARTEXTURE_ENABLED;
		}
	}
}

size_t safe_strlen(const char* str, size_t maxChars)
{
	size_t		count;

	count = 0;
	while (str[count] && count < maxChars)
		count++;

	return count;
}

void R_ParsePackedSkinInternal(const char* texture, int i, studio_setupskin_context_t* context)
{
	char sz[2];
	if (texture[i] == '_')
	{
		switch (texture[i + 1])
		{
		case 'D':
		{
			if (context->packedDiffuseIndex == -1)
			{
				sz[0] = texture[i + 2];
				sz[1] = 0;
				context->packedDiffuseIndex = atoi(sz);
				context->packedCount++;
				(*context->StudioProgramState) |= STUDIO_PACKED_DIFFUSETEXTURE_ENABLED;
			}
			break;
		}
		case 'N':
		{
			if (context->packedNormalIndex == -1)
			{
				sz[0] = texture[i + 2];
				sz[1] = 0;
				context->packedNormalIndex = atoi(sz);
				context->packedCount++;
				(*context->StudioProgramState) |= STUDIO_PACKED_NORMALTEXTURE_ENABLED;
			}
			break;
		}
		case 'P':
		{
			if (context->packedParallaxIndex == -1)
			{
				sz[0] = texture[i + 2];
				sz[1] = 0;
				context->packedParallaxIndex = atoi(sz);
				context->packedCount++;
				(*context->StudioProgramState) |= STUDIO_PACKED_PARALLAXTEXTURE_ENABLED;
			}
			break;
		}
		case 'S':
		{
			if (context->packedSpecularIndex == -1)
			{
				sz[0] = texture[i + 2];
				sz[1] = 0;
				context->packedSpecularIndex = atoi(sz);
				context->packedCount++;
				(*context->StudioProgramState) |= STUDIO_PACKED_SPECULARTEXTURE_ENABLED;
			}
			break;
		}
		}
	}
}

void R_ParsePackedSkin(const char* texture, studio_setupskin_context_t* context)
{
	if (!strnicmp(texture, "Packed_", sizeof("Packed_") - 1))
	{
		auto len = safe_strlen(texture, 64);
		if (len >= sizeof("Packed_D0") - 1)
		{
			R_ParsePackedSkinInternal(texture, sizeof("Packed") - 1, context);
		}
		if (len >= sizeof("Packed_D0_D0") - 1)
		{
			R_ParsePackedSkinInternal(texture, sizeof("Packed_D0") - 1, context);
		}
		if (len >= sizeof("Packed_D0_D0_D0") - 1)
		{
			R_ParsePackedSkinInternal(texture, sizeof("Packed_D0_D0") - 1, context);
		}
		if (len >= sizeof("Packed_D0_D0_D0_D0") - 1)
		{
			R_ParsePackedSkinInternal(texture, sizeof("Packed_D0_D0_D0") - 1, context);
		}
	}
}

bool R_ParseRemapSkin(const char* texture, int* low, int* mid, int* high)
{
	char sz[32];

	if (!strnicmp(texture, "Remap", sizeof("Remap") - 1))
	{
		auto len = safe_strlen(texture, 64);

		if (len == 18 || len == 22)
		{
			if (len != 18 || texture[5] == 'c' || texture[5] == 'C')
			{
				memset(sz, 0, sizeof(sz));
				strncpy(sz, &texture[7], 3);
				sz[3] = 0;

				(*low) = atoi(sz);
				(*low) = math_clamp((*low), 0, 255);

				memset(sz, 0, sizeof(sz));
				strncpy(sz, &texture[11], 3);
				sz[3] = 0;

				(*mid) = atoi(sz);
				(*mid) = math_clamp((*mid), 0, 255);

				if (len == 22)
				{
					memset(sz, 0, sizeof(sz));
					strncpy(sz, &texture[15], 3);
					sz[3] = 0;

					(*high) = atoi(sz);
					(*high) = math_clamp((*high), 0, 255);
				}
				else
				{
					*high = 0;
				}

				return true;
			}
		}
	}

	return false;
}

void R_StudioFlushAllSkins()
{
	for(auto &itor : g_StudioSkinCache)
	{
		delete itor.second;
	}

	g_StudioSkinCache.clear();
}

void R_StudioFlushSkins(int keynum)
{
#if 0
	for (int index = 0; index < MAX_SKINS; index++)
	{
		if ((*pDM_RemapSkin)[keynum][index])
			(*pDM_RemapSkin)[keynum][index]->model = NULL;
	}
#endif

#if 1
	const auto& itor = g_StudioSkinCache.find(keynum);

	if (itor != g_StudioSkinCache.end())
	{
		auto pSkinCache = itor->second;

		if (pSkinCache)
		{
			for (int index = 0; index < MAX_SKINS; index++)
			{
				pSkinCache->skins[index].model = NULL;
			}
		}
	}
#endif
}

skin_t* R_StudioGetSkin(int keynum, int index)
{
#if 0

	if (index >= MAX_SKINS)
		index = 0;

	auto pskin = (*pDM_RemapSkin)[keynum][index];

	if (!pskin || pskin->keynum != keynum)
	{
		pskin = &(*DM_RemapSkin)[(*r_remapindex)][index];
		(*r_remapindex) = ((*r_remapindex) + 1) % 64;
		(*pDM_RemapSkin)[keynum][index] = pskin;
		pskin->keynum = keynum;
		pskin->topcolor = -1;
		pskin->bottomcolor = -1;
	}

	return pskin;
#endif

#if 0
	return gPrivateFuncs.R_StudioGetSkin(keynum, index);
#endif

#if 1

	if (index >= MAX_SKINS)
		index = 0;

	if (index < 0)
		index = 0;

	const auto& itor = g_StudioSkinCache.find(keynum);

	if (itor != g_StudioSkinCache.end())
	{
		return &itor->second->skins[index];
	}
	else
	{
		auto pSkinCache = new (std::nothrow) studio_skin_cache_t;

		if (pSkinCache)
		{
			memset(pSkinCache, 0, sizeof(studio_skin_cache_t));

			pSkinCache->skins[index].keynum = keynum;
			pSkinCache->skins[index].topcolor = -1;
			pSkinCache->skins[index].bottomcolor = -1;
			g_StudioSkinCache[keynum] = pSkinCache;

			return &pSkinCache->skins[index];
		}
	}

	return NULL;
#endif
}

byte* R_StudioReloadSkin(model_t* pModel, int index, skin_t* pskin)
{
	int					modelindex;
	cache_user_t* pCache;
	model_texture_cache_t* pData;

	byte* pbase;
	int size;

	modelindex = pModel - mod_known;

	if (modelindex < 0 || modelindex >= EngineGetMaxKnownModel())
		return NULL;

	pCache = &model_texture_cache[modelindex][index];

	if (Cache_Check(pCache))
	{
		pData = (model_texture_cache_t*)pCache->data;
	}
	else
	{
		int fileLength = 0;

		pbase = gEngfuncs.COM_LoadFile(pModel->name, 5, &fileLength);

		if (!pbase)
			return NULL;

		auto studiohdr = (studiohdr_t*)pbase;

		if (fileLength < sizeof(studiohdr_t)) {
			gEngfuncs.COM_FreeFile(pbase);
			return NULL;
		}

		auto ptexture = (mstudiotexture_t*)((byte *)studiohdr + studiohdr->textureindex);

		ptexture += index;

		if ((byte*)ptexture < (byte*)studiohdr) {
			gEngfuncs.COM_FreeFile(pbase);
			return NULL;
		}

		if ((byte *)ptexture + sizeof(mstudiotexture_t) > (byte*)studiohdr + fileLength) {
			gEngfuncs.COM_FreeFile(pbase);
			return NULL;
		}

		size = ptexture->height * ptexture->width;

		if (pbase + ptexture->index < pbase) {
			gEngfuncs.COM_FreeFile(pbase);
			return NULL;
		}

		if (pbase + ptexture->index + size > pbase + fileLength) {
			gEngfuncs.COM_FreeFile(pbase);
			return NULL;
		}

		Cache_Alloc(pCache, size + 768 + offsetof(model_texture_cache_t, data), pskin->name);

		pData = (model_texture_cache_t*)pCache->data;
		pData->width = ptexture->width;
		pData->height = ptexture->height;
		memcpy(pData->data, pbase + ptexture->index, size + 768);

		gEngfuncs.COM_FreeFile(pbase);
	}

	pskin->index = index;
	pskin->width = pData->width;
	pskin->height = pData->height;
	return pData->data;
}

void PaletteHueReplace(byte* palette, int newHue, int start, int end)
{
	// Convert the new hue value to a range used in the algorithm
	double targetHue = static_cast<double>(newHue) * 1.411764705882353;
	double colorScale = 0.0039215689; // This is 1/255 to scale color values

	if (start > end) {
		return; // No range to process
	}

	// Iterate over the palette entries from start to end
	for (int i = start; i <= end; ++i) {

		if (i >= 256)
			break;

		byte* color = &palette[i * 3]; // Pointer to the current color (RGB)

		// Extract the RGB components and convert to the range [0, 1]
		double red = color[0] * colorScale;
		double green = color[1] * colorScale;
		double blue = color[2] * colorScale;

		// Find the maximum and minimum RGB components manually
		double maxColor = red;
		if (green > maxColor) maxColor = green;
		if (blue > maxColor) maxColor = blue;

		double minColor = red;
		if (green < minColor) minColor = green;
		if (blue < minColor) minColor = blue;

		// Calculate the chroma (difference between max and min color components)
		double chroma = maxColor - minColor;

		// If chroma is 0, the color is grayscale, and hue replacement is not needed
		if (chroma == 0) {
			continue;
		}

		// Calculate the intermediate value used for creating the new color
		double intermediateValue = (1.0 - chroma / maxColor) * maxColor;

		// Calculate the new RGB components based on the target hue
		double newRed, newGreen, newBlue;
		if (targetHue < 120.0) {
			newRed = intermediateValue + chroma * (120.0 - targetHue) / 120.0;
			newGreen = intermediateValue + chroma * targetHue / 120.0;
			newBlue = intermediateValue;
		}
		else if (targetHue < 240.0) {
			newRed = intermediateValue;
			newGreen = intermediateValue + chroma * (240.0 - targetHue) / 120.0;
			newBlue = intermediateValue + chroma * (targetHue - 120.0) / 120.0;
		}
		else {
			newRed = intermediateValue + chroma * (targetHue - 240.0) / 120.0;
			newGreen = intermediateValue;
			newBlue = intermediateValue + chroma * (360.0 - targetHue) / 120.0;
		}

		// Assign the new RGB components to the palette, converting back to [0, 255] range
		color[0] = static_cast<byte>(newRed / colorScale + 0.5); // Adding 0.5 for rounding
		color[1] = static_cast<byte>(newGreen / colorScale + 0.5);
		color[2] = static_cast<byte>(newBlue / colorScale + 0.5);
	}
}

void R_StudioSetupSkinEx(const studio_vbo_t* VBOData, studiohdr_t* ptexturehdr, int index, studio_setupskin_context_t*context)
{
	if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
		return;

	if (!ptexturehdr->textureindex)
		return;

	auto ptexture = (mstudiotexture_t*)((byte*)ptexturehdr + ptexturehdr->textureindex);

#if 1
	if ((*currententity)->index > 0)
	{
		int h = 223;
		int l = 160;
		int m = 191;

		if (!stricmp(ptexture[index].name, "DM_Base.bmp") || R_ParseRemapSkin(ptexture[index].name, &l, &m, &h))
		{
			auto pskin = R_StudioGetSkin((*currententity)->index, index);

			if (pskin)
			{
				if (pskin->model != (*r_model) || pskin->topcolor != (*r_topcolor) || pskin->bottomcolor != (*r_bottomcolor))
				{
					if (pskin->model)
						R_StudioFlushSkins((*currententity)->index);

					auto pTextureData = R_StudioReloadSkin((*r_model), index, pskin);

					if (pTextureData)
					{
						char fullname[1024];
						snprintf(fullname, sizeof(fullname), "%s_%s_%d", ptexturehdr->name, ptexture[index].name, (*currententity)->index);

						byte* orig_palette = pTextureData + (ptexture[index].height * ptexture[index].width);

						byte tmp_palette[768];
						memcpy(tmp_palette, orig_palette, 768);

						pskin->model = (*r_model);
						pskin->bottomcolor = (*r_bottomcolor);
						pskin->topcolor = (*r_topcolor);

						PaletteHueReplace(tmp_palette, pskin->topcolor, l, m);

						if (h != 0)
							PaletteHueReplace(tmp_palette, pskin->bottomcolor, m, h);

						GL_UnloadTextureWithType(fullname, GLT_STUDIO, true);

						pskin->gl_index = GL_LoadTexture(fullname, GLT_STUDIO, pskin->width, pskin->height, pTextureData, false, (ptexture[index].flags & STUDIO_NF_MASKED) ? TEX_TYPE_ALPHA : TEX_TYPE_NONE, tmp_palette);
					}
				}

				if (pskin->gl_index != 0)
				{
					GL_Bind(pskin->gl_index);
					return;
				}
			}
		}
	}

	GL_Bind(ptexture[index].index);

	//Parse packed texture index from texture name...
	R_ParsePackedSkin(ptexture[index].name, context);

#else

	gPrivateFuncs.R_StudioSetupSkin(ptexturehdr, index);

#endif

	if ((*currenttexture) > 0)
	{
		auto VBOMaterial = R_StudioGetVBOMaterialFromTextureId((*currenttexture));

		if (VBOMaterial)
		{
			R_StudioSetupVBOMaterial(VBOData, VBOMaterial, context);
		}
	}
}

void R_StudioDrawVBOBegin(studio_vbo_t* VBOData)
{
	studio_ubo_t StudioUBO;

	g_CurrentVBOCache = VBOData;

	StudioUBO.r_origin[0] = r_origin[0];
	StudioUBO.r_origin[1] = r_origin[1];
	StudioUBO.r_origin[2] = r_origin[2];

	if ((*currententity)->curstate.renderfx == kRenderFxDrawGlowShell)
	{
		StudioUBO.r_origin[0] = cos(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
		StudioUBO.r_origin[1] = sin(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
		StudioUBO.r_origin[2] = cos(r_glowshellfreq->value * (*cl_time) * 0.33f) * 4000.0f;

		StudioUBO.r_color[0] = (float)(*currententity)->curstate.rendercolor.r / 255.0f;
		StudioUBO.r_color[1] = (float)(*currententity)->curstate.rendercolor.g / 255.0f;
		StudioUBO.r_color[2] = (float)(*currententity)->curstate.rendercolor.b / 255.0f;
		StudioUBO.r_color[3] = 1;
	}
	else if ((*currententity)->curstate.rendermode == kRenderTransAdd)
	{
		StudioUBO.r_color[0] = (*r_blend);
		StudioUBO.r_color[1] = (*r_blend);
		StudioUBO.r_color[2] = (*r_blend);
		StudioUBO.r_color[3] = (*r_blend);
	}
	else
	{
		StudioUBO.r_color[0] = r_colormix[0];
		StudioUBO.r_color[1] = r_colormix[1];
		StudioUBO.r_color[2] = r_colormix[2];
		StudioUBO.r_color[3] = (*r_blend);
	}

	StudioUBO.r_ambientlight = (float)(*r_ambientlight);
	StudioUBO.r_shadelight = (*r_shadelight);

	StudioUBO.r_scale = 0;

	if ((*currententity)->curstate.renderfx == kRenderFxDrawGlowShell)
	{
		StudioUBO.r_scale = (*currententity)->curstate.renderamt * 0.05f;
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawOutline)
	{
		StudioUBO.r_scale = g_CurrentVBOCache->celshade_control.outline_size.GetValue() * 0.05f;
	}

	memcpy(StudioUBO.r_plightvec, r_plightvec, sizeof(vec3_t));

	if (r_draw_drawviewmodel && r_studio_viewmodel_lightdir_adjust->value > 0)
	{
		StudioUBO.r_plightvec[2] = StudioUBO.r_plightvec[2] * math_clamp(r_studio_viewmodel_lightdir_adjust->value, 0, 1);

		VectorNormalize(StudioUBO.r_plightvec);
	}

	vec3_t entity_origin = { (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3] };
	memcpy(StudioUBO.entity_origin, entity_origin, sizeof(vec3_t));

	StudioUBO.r_numelight = 0;

	if (r_studio_legacy_elight->value > 0)
	{
		StudioUBO.r_numelight = (*numlight);

		for (int i = 0; i < StudioUBO.r_numelight; ++i)
		{
			StudioUBO.r_elight_color[i][0] = (float)((*locallight)[i]->color.r) / 255.0f;
			StudioUBO.r_elight_color[i][1] = (float)((*locallight)[i]->color.g) / 255.0f;
			StudioUBO.r_elight_color[i][2] = (float)((*locallight)[i]->color.b) / 255.0f;
			StudioUBO.r_elight_color[i][3] = 1;

			GammaToLinear(StudioUBO.r_elight_color[i]);

			StudioUBO.r_elight_origin[i][0] = (*locallight)[i]->origin[0];
			StudioUBO.r_elight_origin[i][1] = (*locallight)[i]->origin[1];
			StudioUBO.r_elight_origin[i][2] = (*locallight)[i]->origin[2];
			StudioUBO.r_elight_origin[i][3] = 0;

			StudioUBO.r_elight_radius[i] = (*locallight)[i]->radius * math_clamp(r_studio_legacy_elight_radius_scale->value, 0.001f, 1000.0f);
		}
	}

	memcpy(StudioUBO.bonematrix, (*pbonetransform), sizeof(mat3x4) * 128);

	if (glNamedBufferSubData)
	{
		glNamedBufferSubData(VBOData->hStudioUBO, 0, sizeof(StudioUBO), &StudioUBO);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, VBOData->hStudioUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(StudioUBO), &StudioUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_STUDIO_UBO, VBOData->hStudioUBO);

	GL_BindVAO(VBOData->hVAO);
}

void R_StudioDrawVBOEnd()
{
	GL_BindVAO(0);

	g_CurrentVBOCache = NULL;
}

void R_StudioDrawVBOMesh_AnalyzePass(
	studio_vbo_t* VBOData,
	studio_vbo_submodel_t* VBOSubmodel,
	studio_vbo_mesh_t* VBOMesh,
	mstudiomesh_t* pmesh,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref,
	const int flags)
{
	//Analysis pass
	if (r_draw_shadowcaster)
	{

	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawGlowShell)
	{

	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawOutline)
	{

	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawAdditiveMeshes)
	{

	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawShadowHair)
	{

	}
	else
	{
		if (flags & STUDIO_NF_ALPHA)
		{
			r_draw_hasalpha = true;
		}

		if (flags & STUDIO_NF_ADDITIVE)
		{
			r_draw_hasadditive = true;
		}

		if (flags & STUDIO_NF_CELSHADE_FACE)
		{
			r_draw_hasface = true;
		}

		if (flags & STUDIO_NF_CELSHADE_HAIR)
		{
			r_draw_hashair = true;
		}
	}
}

void R_StudioDrawVBOMesh_DrawPass(
	studio_vbo_t* VBOData,
	studio_vbo_submodel_t* VBOSubmodel,
	studio_vbo_mesh_t* VBOMesh,
	mstudiomesh_t* pmesh,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref,
	const int flags)
{
	program_state_t StudioProgramState = flags;

	if (r_draw_shadowcaster)
	{
		StudioProgramState |= STUDIO_SHADOW_CASTER_ENABLED;
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawGlowShell)
	{
		StudioProgramState |= (STUDIO_ADDITIVE_BLEND_ENABLED | STUDIO_GLOW_SHELL_ENABLED | STUDIO_NF_CHROME);

		if (StudioProgramState & STUDIO_NF_CELSHADE_ALLBITS)
		{
			StudioProgramState &= ~STUDIO_NF_CELSHADE_ALLBITS;
			StudioProgramState |= STUDIO_NF_FLATSHADE;
		}
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawOutline)
	{
		if (flags & STUDIO_NF_NOOUTLINE)
		{
			return;
		}

		StudioProgramState |= STUDIO_OUTLINE_ENABLED;
		StudioProgramState &= ~(STUDIO_NF_CHROME | STUDIO_NF_ALPHA | STUDIO_NF_ADDITIVE | STUDIO_NF_MASKED | STUDIO_NF_CELSHADE_FACE | STUDIO_NF_CELSHADE_HAIR | STUDIO_NF_CELSHADE_HAIR_H | STUDIO_NF_FULLBRIGHT);
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawAlphaMeshes)
	{
		if (flags & STUDIO_NF_ALPHA)
		{
			StudioProgramState |= STUDIO_ALPHA_BLEND_ENABLED;
		}
		else
		{
			return;
		}
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawAdditiveMeshes)
	{
		if (flags & STUDIO_NF_ADDITIVE)
		{
			StudioProgramState |= STUDIO_ADDITIVE_BLEND_ENABLED;
		}
		else
		{
			return;
		}
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawAlphaMeshes)
	{
		if (flags & STUDIO_NF_ALPHA)
		{
			StudioProgramState |= STUDIO_ALPHA_BLEND_ENABLED;
		}
		else
		{
			return;
		}
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawShadowHair)
	{
		if ((flags & STUDIO_NF_CELSHADE_HAIR) || (flags & STUDIO_NF_CELSHADE_HAIR_H) || (flags & STUDIO_NF_CELSHADE_FACE))
		{
			StudioProgramState |= STUDIO_HAIR_SHADOW_ENABLED;
		}
		else
		{
			return;
		}
	}
	else
	{
		if (flags & STUDIO_NF_ALPHA)
		{
			if (!r_draw_opaque)
			{
				StudioProgramState |= STUDIO_ALPHA_BLEND_ENABLED;
			}
			else
			{
				return;
			}
		}

		if (flags & STUDIO_NF_ADDITIVE)
		{
			if (!r_draw_opaque)
			{
				StudioProgramState |= STUDIO_ADDITIVE_BLEND_ENABLED;
			}
			else
			{
				return;
			}
		}

		if (StudioProgramState & STUDIO_NF_CELSHADE_FACE)
		{
			//Texture unit 6 = Stencil texture
			if (s_BackBufferFBO2.s_hBackBufferStencilView)
			{
				glActiveTexture(GL_TEXTURE0 + STUDIO_RESERVED_TEXTURE_STENCIL);
				glBindTexture(GL_TEXTURE_2D, s_BackBufferFBO2.s_hBackBufferStencilView);
				glActiveTexture(GL_TEXTURE0);

				StudioProgramState |= STUDIO_STENCIL_TEXTURE_ENABLED;
			}
		}
	}

	if (!(StudioProgramState & (STUDIO_ALPHA_BLEND_ENABLED | STUDIO_ADDITIVE_BLEND_ENABLED)) && (*currententity)->curstate.rendermode == kRenderTransAdd)
	{
		StudioProgramState |= STUDIO_ADDITIVE_BLEND_ENABLED;
	}

	if ((*currententity)->curstate.rendermode == kRenderTransAdd)
	{
		StudioProgramState |= STUDIO_ADDITIVE_RENDER_MODE_ENABLED;
	}

	if (!(StudioProgramState & (STUDIO_ALPHA_BLEND_ENABLED | STUDIO_ADDITIVE_BLEND_ENABLED)) && (*currententity)->curstate.rendermode != kRenderNormal)
	{
		StudioProgramState |= STUDIO_ALPHA_BLEND_ENABLED;
	}

	if (r_draw_reflectview)
	{
		StudioProgramState |= STUDIO_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		StudioProgramState |= STUDIO_CLIP_ENABLED;
	}

	if (!R_IsRenderingGBuffer())
	{
		if (r_fog_mode == GL_LINEAR)
		{
			StudioProgramState |= STUDIO_LINEAR_FOG_ENABLED;
		}
		else if (r_fog_mode == GL_EXP)
		{
			StudioProgramState |= STUDIO_EXP_FOG_ENABLED;
		}
		else if (r_fog_mode == GL_EXP2)
		{
			StudioProgramState |= STUDIO_EXP2_FOG_ENABLED;
		}
	}

	if (R_IsRenderingGBuffer())
	{
		StudioProgramState |= STUDIO_GBUFFER_ENABLED;
	}

	if (r_draw_gammablend)
	{
		StudioProgramState |= STUDIO_GAMMA_BLEND_ENABLED;
	}

	if (r_draw_oitblend && (StudioProgramState & (STUDIO_ALPHA_BLEND_ENABLED | STUDIO_ADDITIVE_BLEND_ENABLED)))
	{
		StudioProgramState |= STUDIO_OIT_BLEND_ENABLED;
	}

	studio_setupskin_context_t StudioSetupSkinContext(&StudioProgramState);

	if (r_fullbright->value >= 2)
	{
		gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);

		StudioSetupSkinContext.s = 1.0f / 256.0f;
		StudioSetupSkinContext.t = 1.0f / 256.0f;
	}
	else
	{
		StudioSetupSkinContext.width = ptexture[pskinref[pmesh->skinref]].width;
		StudioSetupSkinContext.height = ptexture[pskinref[pmesh->skinref]].height;

		if (StudioProgramState & STUDIO_GLOW_SHELL_ENABLED)
		{
			gEngfuncs.pTriAPI->SpriteTexture(cl_shellchrome, 0);
		}
		else
		{
			if (ptexturehdr && pskinref)
			{
				R_StudioSetupSkinEx(VBOData, ptexturehdr, pskinref[pmesh->skinref], &StudioSetupSkinContext);
			}
			else
			{
				gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);
			}
		}

		StudioSetupSkinContext.s = 1.0f / StudioSetupSkinContext.width;
		StudioSetupSkinContext.t = 1.0f / StudioSetupSkinContext.height;
	}

	if (StudioProgramState & STUDIO_NF_CHROME)
	{
		if (StudioProgramState & STUDIO_GLOW_SHELL_ENABLED)
		{
			StudioSetupSkinContext.s /= 32.0f;
			StudioSetupSkinContext.t /= 32.0f;
		}
		else
		{
			StudioSetupSkinContext.s = 1.0f / 2048.0f;
			StudioSetupSkinContext.t = 1.0f / 2048.0f;
		}
	}

	if ((StudioProgramState & STUDIO_PACKED_TEXTURE_ALLBITS) && StudioSetupSkinContext.packedCount > 0)
	{
		StudioSetupSkinContext.packedStride = 1.0f / StudioSetupSkinContext.packedCount;
	}

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	if (StudioProgramState & STUDIO_OUTLINE_ENABLED)
	{
		GL_BeginStencilCompareNotEqual(STENCIL_MASK_HAS_OUTLINE, STENCIL_MASK_HAS_OUTLINE);
	}
	else if (StudioProgramState & STUDIO_HAIR_SHADOW_ENABLED)
	{
		//Remove shadow which inside face
		if (StudioProgramState & STUDIO_NF_CELSHADE_FACE)
		{
			GL_BeginStencilWrite(0, STENCIL_MASK_HAS_SHADOW);
		}
		else
		{
			GL_BeginStencilWrite(STENCIL_MASK_HAS_SHADOW, STENCIL_MASK_HAS_SHADOW);
		}
	}
	else
	{
		int iStencilRef = STENCIL_MASK_STUDIO_MODEL;

		if (r_draw_hasoutline)
			iStencilRef |= STENCIL_MASK_HAS_OUTLINE;

		if (StudioProgramState & (STUDIO_NF_FLATSHADE | STUDIO_NF_CELSHADE))
			iStencilRef |= STENCIL_MASK_HAS_FLATSHADE;
		GL_BeginStencilWrite(iStencilRef, STENCIL_MASK_ALL);
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	if (R_IsFlippedViewModel())
	{
		StudioProgramState |= (STUDIO_NF_DOUBLE_FACE | STUDIO_REVERT_NORMAL_ENABLED);
	}

	if (StudioProgramState & STUDIO_NF_DOUBLE_FACE)
	{
		glDisable(GL_CULL_FACE);
	}

	if (StudioProgramState & STUDIO_SHADOW_CASTER_ENABLED)
	{
		//client.dll!StudioRenderFinal enables GL_BLEND and this will mess everything up.
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
	else if (StudioProgramState & STUDIO_HAIR_SHADOW_ENABLED)
	{
		//Disable color, allow depth write-in, only stencil is allowed
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
	else if (r_draw_opaque)
	{
		//Opaque pass
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
	else
	{
		//Transparent pass

		if (StudioProgramState & STUDIO_ALPHA_BLEND_ENABLED)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);

			R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else if (StudioProgramState & STUDIO_ADDITIVE_BLEND_ENABLED)
		{
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);

			R_SetGBufferBlend(GL_ONE, GL_ONE);
		}
		else
		{
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}
	}

	if (r_studio_debug->value > 0)
	{
		StudioProgramState |= STUDIO_DEBUG_ENABLED;
	}

	studio_program_t prog = { 0 };

	R_UseStudioProgram(StudioProgramState, &prog);

	if (prog.r_uvscale != -1)
	{
		glUniform2f(prog.r_uvscale, StudioSetupSkinContext.s, StudioSetupSkinContext.t);
	}

	if (prog.r_packed_stride != -1)
	{
		glUniform1f(prog.r_packed_stride, StudioSetupSkinContext.packedStride);
	}

	if (prog.r_packed_index != -1)
	{
		glUniform4f(prog.r_packed_index, StudioSetupSkinContext.packedDiffuseIndex, StudioSetupSkinContext.packedNormalIndex, StudioSetupSkinContext.packedParallaxIndex, StudioSetupSkinContext.packedSpecularIndex);
	}

	if (prog.r_framerate_numframes != -1)
	{
		glUniform2f(prog.r_framerate_numframes, StudioSetupSkinContext.framerate, StudioSetupSkinContext.numframes);
	}

	if (VBOMesh->iIndiceCount)
	{
		glDrawElements(GL_TRIANGLES, VBOMesh->iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(VBOMesh->iStartIndex));

		++r_studio_drawcall;
		r_studio_polys += VBOMesh->iPolyCount;
	}

	GL_UseProgram(0);

	//Restore textures
	if (StudioProgramState & STUDIO_SPECULARTEXTURE_ENABLED)
	{
		glActiveTexture(GL_TEXTURE0 + STUDIO_SPECULAR_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (StudioProgramState & STUDIO_PARALLAXTEXTURE_ENABLED)
	{
		glActiveTexture(GL_TEXTURE0 + STUDIO_PARALLAX_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (StudioProgramState & STUDIO_NORMALTEXTURE_ENABLED)
	{
		glActiveTexture(GL_TEXTURE0 + STUDIO_NORMAL_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (StudioProgramState & STUDIO_ANIMATED_TEXTURE_ENABLED)
	{
		glActiveTexture(GL_TEXTURE0 + STUDIO_RESERVED_TEXTURE_ANIMATED);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	if (StudioProgramState & STUDIO_STENCIL_TEXTURE_ENABLED)
	{
		//Texture unit 6 = Stencil texture
		glActiveTexture(GL_TEXTURE0 + STUDIO_RESERVED_TEXTURE_STENCIL);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glActiveTexture((*oldtarget));

	//Restore states
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	if (r_draw_opaque)
	{
		GL_EndStencil();
	}
}

void R_StudioDrawVBOMesh(
	studio_vbo_t* VBOData,
	studio_vbo_submodel_t* VBOSubmodel,
	studio_vbo_mesh_t* VBOMesh,
	mstudiomesh_t*pmesh,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref)
{
	int flags = ptexture[pskinref[pmesh->skinref]].flags;

	//Lighting related flags are ignored when r_fullbright >= 2
	if (r_fullbright->value >= 2)
	{
		flags &= STUDIO_NF_FULLBRIGHT_ALLOWBITS;
	}
	else
	{
		flags &= STUDIO_NF_RENDERER_ALLOWBITS;
	}

	if ((*currententity)->curstate.renderfx == kRenderFxDrawGlowShell)
	{
		flags |= STUDIO_NF_CHROME;
	}

	//STUDIO_NF_ALPHA and STUDIO_NF_ADDITIVE is ignored when rendermode not equal to kRenderNormal
	//as those rendermode will ruin STUDIO_NF_ALPHA and STUDIO_NF_ADDITIVE
	if ((*currententity)->curstate.rendermode != kRenderNormal)
	{
		flags &= ~STUDIO_NF_ALPHA;
		flags &= ~STUDIO_NF_ADDITIVE;
	}

	if (!r_studio_celshade->value)
	{
		flags &= ~STUDIO_NF_CELSHADE_ALLBITS;
	}

	if (r_draw_analyzingstudio)
	{
		R_StudioDrawVBOMesh_AnalyzePass(VBOData,
			VBOSubmodel,
			VBOMesh,
			pmesh,
			ptexturehdr,
			ptexture,
			pskinref,
			flags);
	}
	else
	{
		R_StudioDrawVBOMesh_DrawPass(VBOData,
			VBOSubmodel,
			VBOMesh,
			pmesh,
			ptexturehdr,
			ptexture,
			pskinref,
			flags);
	}
}

void R_StudioDrawVBOSubmodel(
	studio_vbo_t* VBOData,
	studio_vbo_submodel_t* VBOSubmodel,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref)
{
	for (size_t i = 0; i < VBOSubmodel->vMesh.size(); i++)
	{
		auto VBOMesh = &VBOSubmodel->vMesh[i];

		auto pmesh = (mstudiomesh_t*)((byte*)(*pstudiohdr) + (*psubmodel)->meshindex) + VBOMesh->iMeshIndex;

		R_StudioDrawVBOMesh(VBOData, VBOSubmodel, VBOMesh, pmesh, ptexturehdr, ptexture, pskinref);
	}
}

void R_StudioDrawVBO(studio_vbo_t* VBOData)
{
	if ((*psubmodel)->groupindex < 1 || (*psubmodel)->groupindex >(int)VBOData->vSubmodels.size()) {
		g_pMetaHookAPI->SysError("R_StudioFindVBOCache: invalid index");
		return;
	}

	auto VBOSubmodel = VBOData->vSubmodels[(*psubmodel)->groupindex - 1];

	auto ptexturehdr = R_StudioGetTextures(*r_model);

	mstudiotexture_t* ptexture = NULL;

	short* pskinref = NULL;

	if (ptexturehdr)
	{
		ptexture = (mstudiotexture_t*)((byte*)ptexturehdr + ptexturehdr->textureindex);

		pskinref = (short*)((byte*)ptexturehdr + ptexturehdr->skinindex);

		if ((*currententity)->curstate.skin > 0 && (*currententity)->curstate.skin < ptexturehdr->numskinfamilies)
			pskinref += ((*currententity)->curstate.skin * ptexturehdr->numskinref);
	}

	if ((*pstudiohdr)->numbones > MAXSTUDIOBONES)
	{
		g_pMetaHookAPI->SysError("R_GLStudioDrawPoints: %s numbones (%d) > MAXSTUDIOBONES (%d)", (*pstudiohdr)->name, (*pstudiohdr)->numbones, MAXSTUDIOBONES);
		return;
	}

	R_StudioDrawVBOSubmodel(VBOData, VBOSubmodel, ptexturehdr, ptexture, pskinref);
}

//Engine exported StudioAPI

void R_GLStudioDrawPoints(void)
{
	auto VBOData = R_PrepareStudioVBO((*pstudiohdr));

	if (!VBOData)
		return;

	R_StudioDrawVBOBegin(VBOData);

	R_StudioDrawVBO(VBOData);

	R_StudioDrawVBOEnd();
}

void R_StudioTransformVector(vec3_t in, vec3_t out)
{
	out[0] = in[0] * (*rotationmatrix)[0][0] + in[1] * (*rotationmatrix)[0][1] + in[2] * (*rotationmatrix)[0][2] + (*rotationmatrix)[0][3];
	out[1] = in[0] * (*rotationmatrix)[1][0] + in[1] * (*rotationmatrix)[1][1] + in[2] * (*rotationmatrix)[1][2] + (*rotationmatrix)[1][3];
	out[2] = in[0] * (*rotationmatrix)[2][0] + in[1] * (*rotationmatrix)[2][1] + in[2] * (*rotationmatrix)[2][2] + (*rotationmatrix)[2][3];
}

qboolean studioapi_StudioCheckBBox(void)
{
	if (!g_bIsSvenCoop)
	{
		return gPrivateFuncs.studioapi_StudioCheckBBox();
	}

	mplane_t			plane;
	vec3_t				mins, maxs;

	int					i;
	mstudioseqdesc_t* pseqdesc;

	vec3_t				p1, p2;

#undef min
#undef max

	vec3_t tempmins, tempmaxs;

	const vec3_t gFakeHullMins = { -16, -16, -16 };
	const vec3_t gFakeHullMaxs = { 16, 16, 16 };

	if (!VectorCompare(vec3_origin, (*pstudiohdr)->bbmin))
	{
		// clipping bounding box
		VectorCopy((*pstudiohdr)->bbmin, tempmins);
		VectorCopy((*pstudiohdr)->bbmax, tempmaxs);

		if ((*currententity)->curstate.scale > 0 && (*currententity)->curstate.scale != 1)
		{
			VectorScale(tempmins, (*currententity)->curstate.scale, tempmins);
			VectorScale(tempmaxs, (*currententity)->curstate.scale, tempmaxs);
		}

		VectorAdd((*currententity)->origin, tempmins, mins);
		VectorAdd((*currententity)->origin, tempmaxs, maxs);
	}
	else if (!VectorCompare(vec3_origin, (*pstudiohdr)->min))
	{
		// movement bounding box
		VectorCopy((*pstudiohdr)->min, tempmins);
		VectorCopy((*pstudiohdr)->max, tempmaxs);

		if ((*currententity)->curstate.scale > 0 && (*currententity)->curstate.scale != 1)
		{
			VectorScale(tempmins, (*currententity)->curstate.scale, tempmins);
			VectorScale(tempmaxs, (*currententity)->curstate.scale, tempmaxs);
		}

		VectorAdd((*currententity)->origin, tempmins, mins);
		VectorAdd((*currententity)->origin, tempmins, maxs);
	}
	else
	{
		// fake bounding box
		VectorCopy(gFakeHullMins, tempmins);
		VectorCopy(gFakeHullMaxs, tempmaxs);

		if ((*currententity)->curstate.scale > 0 && (*currententity)->curstate.scale != 1)
		{
			VectorScale(tempmins, (*currententity)->curstate.scale, tempmins);
			VectorScale(tempmaxs, (*currententity)->curstate.scale, tempmaxs);
		}

		VectorAdd((*currententity)->origin, gFakeHullMins, mins);
		VectorAdd((*currententity)->origin, gFakeHullMaxs, maxs);
	}

	// construct the base bounding box for this frame
	if ((*currententity)->curstate.sequence >= (*pstudiohdr)->numseq)
	{
		(*currententity)->curstate.sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t*)((byte*)(*pstudiohdr) + (*pstudiohdr)->seqindex) + (*currententity)->curstate.sequence;

	for (i = 0; i < 8; i++)
	{
		p1[0] = (i & 1) ? pseqdesc->bbmin[0] : pseqdesc->bbmax[0];
		p1[1] = (i & 2) ? pseqdesc->bbmin[1] : pseqdesc->bbmax[1];
		p1[2] = (i & 4) ? pseqdesc->bbmin[2] : pseqdesc->bbmax[2];

		if ((*currententity)->curstate.scale > 0 && (*currententity)->curstate.scale != 1)
		{
			VectorScale(p1, (*currententity)->curstate.scale, p1);
		}

		R_StudioTransformVector(p1, p2);

		if (p2[0] < mins[0]) mins[0] = p2[0];
		if (p2[0] > maxs[0]) maxs[0] = p2[0];
		if (p2[1] < mins[1]) mins[1] = p2[1];
		if (p2[1] > maxs[1]) maxs[1] = p2[1];
		if (p2[2] < mins[2]) mins[2] = p2[2];
		if (p2[2] > maxs[2]) maxs[2] = p2[2];
	}

	if (Host_IsSinglePlayerGame() || !r_cullsequencebox->value)
	{
		plane.type = 5;
		VectorCopy(vpn, plane.normal);
		plane.dist = DotProduct(plane.normal, r_origin);
		plane.signbits = SignbitsForPlane(&plane);

		if (BoxOnPlaneSide(mins, maxs, &plane) != 2)
			return true;
	}
	else
	{
		if (!R_CullBox(mins, maxs))
			return true;
	}

	return false;
}

void studioapi_StudioDynamicLight(cl_entity_t* ent, alight_t* plight)
{
	//Disable legacy dlight for studio models?
	if (r_light_dynamic->value && !r_studio_legacy_dlight->value)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			float dies[256];

			dlight_t* dl = cl_dlights;
			for (int i = 0; i < 256; i++, dl++)
			{
				dies[i] = dl->die;
				dl->die = 0;
			}

			gPrivateFuncs.studioapi_StudioDynamicLight(ent, plight);

			dl = cl_dlights;
			for (int i = 0; i < 256; i++, dl++)
			{
				dl->die = dies[i];
			}
		}
		else
		{
			float dies[32];

			dlight_t* dl = cl_dlights;
			for (int i = 0; i < 32; i++, dl++)
			{
				dies[i] = dl->die;
				dl->die = 0;
			}

			gPrivateFuncs.studioapi_StudioDynamicLight(ent, plight);

			dl = cl_dlights;
			for (int i = 0; i < 32; i++, dl++)
			{
				dl->die = dies[i];
			}
		}
	}
	else
	{
		gPrivateFuncs.studioapi_StudioDynamicLight(ent, plight);
	}
}

template<typename CallType>
__forceinline void StudioRenderFinal_Template(CallType pfnRenderFinal, void* pthis = nullptr, int dummy = 0)
{
	pfnRenderFinal(pthis, 0);
}

template<typename CallType>
__forceinline void StudioRenderModel_Template(CallType pfnRenderModel, CallType pfnRenderFinal, void* pthis = nullptr, int dummy = 0)
{
	if (r_draw_shadowcaster)
	{
		pfnRenderModel(pthis, dummy);
		return;
	}

	//Process all pending deferred passes on transparent pass
	if (!r_draw_opaque)
	{
		auto EntityComponent = R_GetEntityComponent((*currententity), false);

		if (EntityComponent)
		{
			if (EntityComponent->DeferredStudioPasses.size() > 0)
			{
				for (auto fx : EntityComponent->DeferredStudioPasses)
				{
					int saved_renderfx = (*currententity)->curstate.renderfx;
					int saved_renderamt = (*currententity)->curstate.renderamt;

					(*currententity)->curstate.renderfx = fx;

					pfnRenderModel(pthis, dummy);

					(*currententity)->curstate.renderfx = saved_renderfx;
					(*currententity)->curstate.renderamt = saved_renderamt;
				}

				EntityComponent->DeferredStudioPasses.clear();
				return;
			}
		}
	}

	//Begin analysis pass

	r_draw_hashair = false;
	r_draw_hasface = false;
	r_draw_hasalpha = false;
	r_draw_hasadditive = false;
	r_draw_hasoutline = false;

	r_draw_analyzingstudio = true;

	pfnRenderModel(pthis, dummy);

	if (!r_draw_hasoutline && R_StudioHasOutline())
	{
		r_draw_hasoutline = true;
	}

	//End analysis pass
	r_draw_analyzingstudio = false;

	//Defer additive meshes to transparent pass
	if (r_draw_opaque && r_draw_hasalpha)
	{
		auto comp = R_GetEntityComponent((*currententity), true);

		comp->DeferredStudioPasses.emplace_back(kRenderFxDrawAlphaMeshes);

		r_draw_deferredtrans = true;
	}

	if (r_draw_opaque && r_draw_hasadditive)
	{
		auto comp = R_GetEntityComponent((*currententity), true);

		comp->DeferredStudioPasses.emplace_back(kRenderFxDrawAdditiveMeshes);

		r_draw_deferredtrans = true;
	}

	//Hair pass
	if (R_StudioHasHairShadow())
	{
		GL_BindFrameBuffer(&s_BackBufferFBO2);

		glDrawBuffer(GL_NONE);

		GL_ClearDepthStencil(1, STENCIL_MASK_SKY, STENCIL_MASK_ALL);

		int saved_renderfx = (*currententity)->curstate.renderfx;
		int saved_renderamt = (*currententity)->curstate.renderamt;

		(*currententity)->curstate.renderfx = kRenderFxDrawShadowHair;
		(*currententity)->curstate.renderamt = 0;

		pfnRenderModel(pthis, dummy);

		(*currententity)->curstate.renderfx = saved_renderfx;
		(*currententity)->curstate.renderamt = saved_renderamt;

		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		if (r_draw_gbuffer)
			GL_BindFrameBuffer(&s_GBufferFBO);
		else
			GL_BindFrameBuffer(GL_GetCurrentSceneFBO());
	}

	if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
	{
		int saved_renderfx = (*currententity)->curstate.renderfx;
		int saved_renderamt = (*currententity)->curstate.renderamt;

		//Draw normal pass

		(*currententity)->curstate.renderfx = 0;

		pfnRenderModel(pthis, dummy);

		(*currententity)->curstate.renderfx = saved_renderfx;
		(*currententity)->curstate.renderamt = saved_renderamt;

		//Outline pass
		if (r_draw_hasoutline)
		{
			int saved_renderfx = (*currententity)->curstate.renderfx;
			int saved_renderamt = (*currententity)->curstate.renderamt;

			(*currententity)->curstate.renderfx = kRenderFxDrawOutline;
			(*currententity)->curstate.renderamt = 0;

			pfnRenderModel(pthis, dummy);

			(*currententity)->curstate.renderfx = saved_renderfx;
			(*currententity)->curstate.renderamt = saved_renderamt;
		}

		if (r_draw_opaque)
		{
			//Defer GlowShell to transparent pass

			auto comp = R_GetEntityComponent((*currententity), true);

			comp->DeferredStudioPasses.emplace_back(kRenderFxDrawGlowShell);

			r_draw_deferredtrans = true;
		}
		else
		{
			//Draw GlowShell pass now

			(*currententity)->curstate.renderfx = kRenderFxDrawGlowShell;

			pfnRenderModel(pthis, dummy);

			(*currententity)->curstate.renderfx = saved_renderfx;
		}
	}
	else
	{
		//Draw normal pass
		pfnRenderModel(pthis, dummy);

		//Outline pass
		if (r_draw_hasoutline)
		{
			int saved_renderfx = (*currententity)->curstate.renderfx;
			int saved_renderamt = (*currententity)->curstate.renderamt;

			(*currententity)->curstate.renderfx = kRenderFxDrawOutline;
			(*currententity)->curstate.renderamt = 0;

			pfnRenderModel(pthis, dummy);

			(*currententity)->curstate.renderfx = saved_renderfx;
			(*currententity)->curstate.renderamt = saved_renderamt;
		}

	}

	GL_ClearStencil(STENCIL_MASK_HAS_OUTLINE);

	r_draw_hashair = false;
	r_draw_hasface = false;
	r_draw_hasalpha = false;
	r_draw_hasadditive = false;
	r_draw_hasoutline = false;
}

//Engine StudioRenderer

__forceinline void R_StudioRenderFinal_originalcall_wrapper(void* pthis, int dummy)
{
	gPrivateFuncs.R_StudioRenderFinal();
}

__forceinline void R_StudioRenderModel_originalcall_wrapper(void* pthis, int dummy)
{
	gPrivateFuncs.R_StudioRenderModel();
}

void R_StudioRenderFinal(void)
{
	StudioRenderFinal_Template(R_StudioRenderFinal_originalcall_wrapper);
}

void R_StudioRenderModel(void)
{
	StudioRenderModel_Template(R_StudioRenderModel_originalcall_wrapper, R_StudioRenderFinal_originalcall_wrapper);
}

//Client StudioRenderer

void __fastcall GameStudioRenderer_StudioRenderFinal(void* pthis, int dummy)
{
	StudioRenderFinal_Template(gPrivateFuncs.GameStudioRenderer_StudioRenderFinal, pthis, dummy);
}

void __fastcall GameStudioRenderer_StudioRenderModel(void* pthis, int dummy)
{
	StudioRenderModel_Template(gPrivateFuncs.GameStudioRenderer_StudioRenderModel, GameStudioRenderer_StudioRenderFinal, pthis, dummy);
}

template<typename CallType>
__forceinline void StudioSetupBones_Template(CallType pfnSetupBones, void* pthis = nullptr, int dummy = 0)
{
	//Never cache bones for viewmodel !
	if (!r_studio_bone_caches->value || (*currententity) == cl_viewent)
	{
		pfnSetupBones(pthis, dummy);
		return;
	}

	studio_bone_handle handle(
		(*pstudiohdr)->soundtable,
		(*currententity)->curstate.sequence,
		(*currententity)->curstate.gaitsequence,
		(*currententity)->curstate.frame,
		(*currententity)->origin,
		(*currententity)->angles);

	const auto& itor = g_StudioBoneCacheManager.find(handle);

	if (itor != g_StudioBoneCacheManager.end())
	{
		memcpy((*pbonetransform), itor->second->m_bonetransform, sizeof(itor->second->m_bonetransform));
		memcpy((*plighttransform), itor->second->m_lighttransform, sizeof(itor->second->m_lighttransform));
		return;
	}

	pfnSetupBones(pthis, dummy);

	auto cache = R_StudioBoneCacheAlloc();

	if (cache)
	{
		memcpy(cache->m_bonetransform, (*pbonetransform), sizeof(cache->m_bonetransform));
		memcpy(cache->m_lighttransform, (*plighttransform), sizeof(cache->m_lighttransform));

		g_StudioBoneCacheManager[handle] = cache;
	}
}

template<typename CallType>
void __fastcall StudioMergeBones_Template(CallType pfnMergeBones, void* pthis, int dummy, model_t* pSubModel)
{
	//Never cache bones for viewmodel !
	if (!r_studio_bone_caches->value || (*currententity) == cl_viewent)
	{
		pfnMergeBones(pthis, dummy, pSubModel);
		return;
	}

	studio_bone_handle handle(
		(*pstudiohdr)->soundtable,
		(*currententity)->curstate.sequence,
		(*currententity)->curstate.gaitsequence,
		(*currententity)->curstate.frame,
		(*currententity)->origin,
		(*currententity)->angles);

	const auto& itor = g_StudioBoneCacheManager.find(handle);

	if (itor != g_StudioBoneCacheManager.end())
	{
		memcpy((*pbonetransform), itor->second->m_bonetransform, sizeof(itor->second->m_bonetransform));
		memcpy((*plighttransform), itor->second->m_lighttransform, sizeof(itor->second->m_lighttransform));
		return;
	}

	pfnMergeBones(pthis, dummy, pSubModel);

	auto cache = R_StudioBoneCacheAlloc();

	if (cache)
	{
		memcpy(cache->m_bonetransform, (*pbonetransform), sizeof(cache->m_bonetransform));
		memcpy(cache->m_lighttransform, (*plighttransform), sizeof(cache->m_lighttransform));

		g_StudioBoneCacheManager[handle] = cache;
	}
}

void __fastcall GameStudioRenderer_StudioSetupBones(void* pthis, int dummy)
{
	StudioSetupBones_Template(gPrivateFuncs.GameStudioRenderer_StudioSetupBones, pthis, dummy);
}

void __fastcall GameStudioRenderer_StudioMergeBones(void* pthis, int dummy, model_t* pSubModel)
{
	StudioMergeBones_Template(gPrivateFuncs.GameStudioRenderer_StudioMergeBones, pthis, dummy, pSubModel);
}

void R_StudioFreeAllTexturesInVBOMaterial(studio_vbo_material_t* VBOMaterial)
{
	for (int i = 0; i < STUDIO_MAX_TEXTURE - 1; ++i)
	{
		if (VBOMaterial->textures[i].gltexturenum)
		{
			GL_UnloadTextureByTextureId(VBOMaterial->textures[i].gltexturenum, false);
			VBOMaterial->textures[i].gltexturenum = 0;
		}

		VBOMaterial->textures[i].numframes = 0;
		VBOMaterial->textures[i].framerate = 0;
		VBOMaterial->textures[i].width = 0;
		VBOMaterial->textures[i].height = 0;
		VBOMaterial->textures[i].scaleX = 0;
		VBOMaterial->textures[i].scaleY = 0;
	}
}

bool R_StudioFreeTextureInVBOMaterial(studio_vbo_material_t* VBOMaterial, int gltexturenum)
{
	for (int i = 0; i < STUDIO_MAX_TEXTURE - 1; ++i)
	{
		if (VBOMaterial->textures[i].gltexturenum == gltexturenum)
		{
			GL_UnloadTextureByTextureId(VBOMaterial->textures[i].gltexturenum, false);
			VBOMaterial->textures[i].gltexturenum = 0;

			VBOMaterial->textures[i].width = 0;
			VBOMaterial->textures[i].height = 0;
			VBOMaterial->textures[i].scaleX = 0;
			VBOMaterial->textures[i].scaleY = 0;
			return true;
		}
	}

	return false;
}

/*

Purpose : Free VBOMaterial (if there is) for this gltexturenum.

*/

bool R_StudioFreeVBOMaterialByTextureId(int gltexturenum)
{
	const auto& itor = g_StudioVBOMaterialCache.find(gltexturenum);

	if (itor != g_StudioVBOMaterialCache.end())
	{
		auto VBOMaterial = itor->second;

		if (VBOMaterial)
		{
			R_StudioFreeAllTexturesInVBOMaterial(VBOMaterial);
			delete VBOMaterial;
		}

		g_StudioVBOMaterialCache.erase(itor);
		return true;
	}

	return false;
}

/*

Purpose : 

1. Free all VBOMaterials owned by this glt.
2. Free any VBOMaterial linked to this glt.

*/

void R_StudioFreeTextureCallback(gltexture_t *glt)
{
	if (GL_GetTextureTypeFromTextureIdentifier(glt->identifier) != GLT_STUDIO)
		return;

	if (R_StudioFreeVBOMaterialByTextureId(glt->texnum))
		return;

	for (auto &itor : g_StudioVBOMaterialCache)
	{
		auto VBOMaterial = itor.second;

		if (VBOMaterial)
		{
			if (R_StudioFreeTextureInVBOMaterial(VBOMaterial, glt->texnum))
				return;
		}
	}
}

studio_vbo_material_t* R_StudioGetVBOMaterialFromTextureId(int gltexturenum)
{
	if (gltexturenum > 0)
	{
		const auto& itor = g_StudioVBOMaterialCache.find(gltexturenum);

		if (itor != g_StudioVBOMaterialCache.end())
		{
			return itor->second;
		}
	}

	return NULL;
}

studio_vbo_material_t* R_StudioPrepareVBOMaterial(mstudiotexture_t* texture)
{
	studio_vbo_material_t* VBOMaterial = R_StudioGetVBOMaterialFromTextureId(texture->index);

	if (VBOMaterial)
		return VBOMaterial;

	VBOMaterial = new studio_vbo_material_t();

	g_StudioVBOMaterialCache[texture->index] = VBOMaterial;

	return VBOMaterial;
}

void R_StudioLoadExternalFile_TextureLoad(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData, mstudiotexture_t* ptexture, const char* textureValue, const char* scaleValue, int StudioTextureType)
{
	if (textureValue && textureValue[0])
	{
		auto VBOMaterial = R_StudioPrepareVBOMaterial(ptexture);

		if (VBOMaterial && !VBOMaterial->textures[StudioTextureType - 1].gltexturenum)
		{
			bool bLoaded = false;
			gl_loadtexture_result_t loadResult;
			std::string texturePath;
			
			bool bHasMipmaps = (ptexture->flags & STUDIO_NF_NOMIPS) ? false : true;

			//Texture name starts with "models\\" or "models/"
			if (!bLoaded &&
				!strnicmp(textureValue, "models", sizeof("models") - 1) &&
				(textureValue[sizeof("models") - 1] == '\\' || textureValue[sizeof("models") - 1] == '/'))
			{
				texturePath = textureValue;
				if (!V_GetFileExtension(textureValue))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_STUDIO, bHasMipmaps, &loadResult);
			}

			if (!bLoaded)
			{
				texturePath = "gfx/";
				texturePath += textureValue;

				if (!V_GetFileExtension(textureValue))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_STUDIO, bHasMipmaps, &loadResult);

			}

			if (!bLoaded)
			{
				texturePath = "renderer/texture/";
				texturePath += textureValue;

				if (!V_GetFileExtension(textureValue))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_STUDIO, bHasMipmaps, &loadResult);
			}

			if (!bLoaded)
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile_TextureLoad: Failed to load external texture \"%s\" for basetexture \"%s\"\n", textureValue, ptexture->name);
				return;
			}

			VBOMaterial->textures[StudioTextureType - 1].gltexturenum = loadResult.gltexturenum;
			VBOMaterial->textures[StudioTextureType - 1].numframes = loadResult.numframes;
			VBOMaterial->textures[StudioTextureType - 1].framerate = GetFrameRateFromFrameDuration(loadResult.frameduration);
			VBOMaterial->textures[StudioTextureType - 1].width = loadResult.width;
			VBOMaterial->textures[StudioTextureType - 1].height = loadResult.height;
			VBOMaterial->textures[StudioTextureType - 1].scaleX = 0;
			VBOMaterial->textures[StudioTextureType - 1].scaleY = 0;

			if (scaleValue && scaleValue[0])
			{
				float scales[2] = { 0 };

				if (2 == sscanf(scaleValue, "%f %f", &scales[0], &scales[1]))
				{
					if (scales[0] > 0 || scales[0] < 0)
						VBOMaterial->textures[StudioTextureType - 1].scaleX = scales[0];

					if (scales[1] > 0 || scales[1] < 0)
						VBOMaterial->textures[StudioTextureType - 1].scaleY = scales[1];
				}
				else if (1 == sscanf(scaleValue, "%f", &scales[0]))
				{
					if (scales[0] > 0 || scales[0] < 0)
					{
						VBOMaterial->textures[StudioTextureType - 1].scaleX = scales[0];
						VBOMaterial->textures[StudioTextureType - 1].scaleY = scales[0];
					}
				}
			}
		}
	}
}

void R_StudioLoadExternalFile_TextureFlags(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData, mstudiotexture_t* ptexture, const char* value)
{
#define REGISTER_TEXTURE_FLAGS_KEY_VALUE(name) if (value && !strcmp(value, #name))\
	{\
		ptexture->flags |= name;\
	}\
	if (value && !strcmp(value, "-" #name))\
	{\
		ptexture->flags &= ~name;\
	}

	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_FLATSHADE);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_CHROME);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_FULLBRIGHT);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_NOMIPS);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_ALPHA);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_ADDITIVE);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_MASKED);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_CELSHADE);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_CELSHADE_FACE);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_CELSHADE_HAIR);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_CELSHADE_HAIR_H);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_DOUBLE_FACE);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_OVERBRIGHT);
	REGISTER_TEXTURE_FLAGS_KEY_VALUE(STUDIO_NF_NOOUTLINE);

#undef REGISTER_TEXTURE_FLAGS_KEY_VALUE
}

void R_StudioLoadExternalFile_TextureFlagsArray(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData, mstudiotexture_t* ptexture, const std::vector<const char *> flagsArray)
{
	for (auto flags : flagsArray)
	{
		R_StudioLoadExternalFile_TextureFlags(ent, studiohdr, VBOData, ptexture, flags);
	}
}

void R_StudioLoadExternalFile_Texture(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{

	auto basetexture = ValueForKey(ent, "basetexture");

	if (!basetexture)
	{
		gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"basetexture\" from \"studio_texture\"\n");
		return;
	}

	if (!basetexture[0])
	{
		gEngfuncs.Con_Printf("R_StudioLoadExternalFile: \"basetexture\" cannot be empty\n");
		return;
	}

	if (!studiohdr->textureindex || !studiohdr->numtextures)
	{
		gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Model \"%s\" has no texture\n", studiohdr->name);
		return;
	}

	std::vector<const char*> flagsArray;
	ValueForKeyExArray(ent, "flags", flagsArray);

	auto replacetexture = ValueForKey(ent, "replacetexture");
	auto replacescale = ValueForKey(ent, "replacescale");

	auto normaltexture = ValueForKey(ent, "normaltexture");
	auto normalscale = ValueForKey(ent, "normalscale");

	auto parallaxtexture = ValueForKey(ent, "parallaxtexture");
	auto parallaxscale = ValueForKey(ent, "parallaxscale");

	auto speculartexture = ValueForKey(ent, "speculartexture");
	auto specularscale = ValueForKey(ent, "specularscale");

	for (int i = 0; i < studiohdr->numtextures; ++i)
	{
		auto ptexture = (mstudiotexture_t*)((byte*)studiohdr + studiohdr->textureindex) + i;

		bool bTextureMatched = false;

		if (!strcmp(basetexture, "*"))
		{
			bTextureMatched = true;
		}
		else if (!strcmp(ptexture->name, basetexture))
		{
			bTextureMatched = true;
		}

		if (bTextureMatched)
		{
			R_StudioLoadExternalFile_TextureFlagsArray(ent, studiohdr, VBOData, ptexture, flagsArray);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, VBOData, ptexture, replacetexture, replacescale, STUDIO_REPLACE_TEXTURE);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, VBOData, ptexture, normaltexture, normalscale, STUDIO_NORMAL_TEXTURE);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, VBOData, ptexture, parallaxtexture, parallaxscale, STUDIO_PARALLAX_TEXTURE);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, VBOData, ptexture, speculartexture, specularscale, STUDIO_SPECULAR_TEXTURE);
		}
	}
}

void R_StudioLoadExternalFile_Efx(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{
	auto flags_string = ValueForKey(ent, "flags");

#define REGISTER_EFX_FLAGS_KEY_VALUE(name) if (flags_string && !strcmp(flags_string, #name))\
	{\
		studiohdr->flags |= name; \
	}\
	if (flags_string && !strcmp(flags_string, "-" #name))\
	{\
		studiohdr->flags |= name; \
	}

	REGISTER_EFX_FLAGS_KEY_VALUE(EF_ROCKET);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_GRENADE);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_GIB);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_ROTATE);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_TRACER);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_ZOMGIB);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_TRACER2);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_TRACER3);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_NOSHADELIGHT);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_HITBOXCOLLISIONS);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_FORCESKYLIGHT);
	REGISTER_EFX_FLAGS_KEY_VALUE(EF_OUTLINE);

#undef REGISTER_EFX_FLAGS_KEY_VALUE
}

void R_StudioLoadExternalFile_Celshade(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{
#define REGISTER_CELSHADE_KEY_VALUE(name, parser) if (1)\
	{\
		auto name = ValueForKey(ent, #name);\
		if (name && name[0])\
		{\
			vec4_t values = { 0 };\
			if (parser(name, values))\
			{\
				VBOData->celshade_control.name.SetOverrideValues(values);\
				VBOData->celshade_control.name.SetOverride(true);\
			}\
			else\
			{\
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"" #name  "\" in entity \"studio_celshade_control\"\n");\
			}\
		}\
	}

	REGISTER_CELSHADE_KEY_VALUE(base_specular, R_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(celshade_specular, R_ParseStringAsVector4);
	REGISTER_CELSHADE_KEY_VALUE(celshade_midpoint, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(celshade_softness, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(celshade_shadow_color, R_ParseStringAsColor3);
	REGISTER_CELSHADE_KEY_VALUE(celshade_head_offset, R_ParseStringAsVector3);
	REGISTER_CELSHADE_KEY_VALUE(celshade_lightdir_adjust, R_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(outline_size, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(outline_dark, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_power, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_smooth, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_smooth2, R_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_color, R_ParseStringAsColor3);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_power, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_smooth, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_smooth2, R_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_color, R_ParseStringAsColor3);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_exp, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_intensity, R_ParseStringAsVector3);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_noise, R_ParseStringAsVector4);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_exp2, R_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_intensity2, R_ParseStringAsVector3);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_noise2, R_ParseStringAsVector4);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_smooth, R_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(hair_shadow_offset, R_ParseStringAsVector2);

#undef REGISTER_CELSHADE_KEY_VALUE
}

static std::vector<bspentity_t> g_StudioBSPEntities;

bspentity_t* R_ParseBSPEntity_StudioAllocator(void)
{
	size_t len = g_StudioBSPEntities.size();

	g_StudioBSPEntities.resize(len + 1);

	return &g_StudioBSPEntities[len];
}

void R_StudioLoadExternalFile(model_t* mod, studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{
	if (VBOData->bExternalFileLoaded)
		return;

	VBOData->bExternalFileLoaded = true;

	std::string fullPath = mod->name;

	RemoveFileExtension(fullPath);

	fullPath += "_external.txt";

	auto pFile = (char*)gEngfuncs.COM_LoadFile(fullPath.c_str(), 5, NULL);

	if (!pFile)
	{
		return;
	}

	R_ParseBSPEntities(pFile, R_ParseBSPEntity_StudioAllocator);

	for (size_t i = 0; i < g_StudioBSPEntities.size(); ++i)
	{
		bspentity_t* ent = &g_StudioBSPEntities[i];

		char* classname = ent->classname;

		if (!classname)
			continue;

		if (!strcmp(classname, "studio_texture"))
		{
			R_StudioLoadExternalFile_Texture(ent, studiohdr, VBOData);
		}
		else if (!strcmp(classname, "studio_efx"))
		{
			R_StudioLoadExternalFile_Efx(ent, studiohdr, VBOData);
		}
		else if (!strcmp(classname, "studio_celshade_control"))
		{
			R_StudioLoadExternalFile_Celshade(ent, studiohdr, VBOData);
		}
	}

	for (size_t i = 0; i < g_StudioBSPEntities.size(); i++)
	{
		FreeBSPEntity(&g_StudioBSPEntities[i]);
	}

	g_StudioBSPEntities.clear();

	gEngfuncs.COM_FreeFile(pFile);
}