#include "gl_local.h"
#include "triangleapi.h"
#include "mathlib2.h"
#include "CounterStrike.h"

#include <sstream>
#include <algorithm>

static GLuint g_hStudioUBO{};

static std::vector<CStudioModelRenderData*> g_StudioRenderDataCache;

static std::unordered_map<int, CStudioSkinCache*> g_StudioSkinCache;

static std::unordered_map<CStudioBoneCacheHandle, CStudioBoneCache*, CStudioBoneCacheHasher> g_StudioBoneCacheManager;

static std::unordered_map<program_state_t, studio_program_t> g_StudioProgramTable;

static CStudioBoneCache g_StudioBoneCaches[MAX_STUDIO_BONE_CACHES];

static CStudioBoneCache* g_pStudioBoneFreeCaches = NULL;

static CStudioModelRenderData* g_CurrentRenderData = NULL;

static cache_user_t model_texture_cache[MAX_KNOWN_MODELS_SVENGINE][MAX_SKINS];

class CEnginePlayerInfoStorage
{
public:
	struct player_info_s SavedPlayerInfo{};
	bool Filled{};
};

static CEnginePlayerInfoStorage g_PlayerInfoStorage[MAX_CLIENTS]{};

//Engine private vars

model_t* cl_sprite_white = NULL;
model_t* cl_sprite_shell = NULL;
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
int* numlights = NULL;
int* r_topcolor = NULL;
int* r_bottomcolor = NULL;

extern void* g_pGameStudioRenderer;

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

#if 0
cvar_t* r_studio_viewmodel_lightdir_adjust = NULL;
#endif

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

cvar_t* r_studio_bone_caches = NULL;

cvar_t* r_studio_external_textures = NULL;

cvar_t* r_lowerbody_model_offset = NULL;
cvar_t* r_lowerbody_model_scale = NULL;

CStudioModelRenderData::~CStudioModelRenderData()
{
	if (hVAO)
	{
		GL_DeleteVAO(hVAO);
	}
	if (hVBO)
	{
		GL_DeleteBuffer(hVBO);
	}
	if (hEBO)
	{
		GL_DeleteBuffer(hEBO);
	}
	for (auto pSubmodel : vSubmodels)
	{
		delete pSubmodel;
	}
	for (const auto &itor : mStudioMaterials)
	{
		delete itor.second;
	}
}

bool R_StudioHasOutline()
{
	return r_studio_outline->value > 0 && ((*pstudiohdr)->flags & EF_OUTLINE);
}

bool R_StudioHasHairShadow()
{
	return r_draw_hashair && r_draw_hasface && r_studio_hair_shadow->value > 0 && !R_IsRenderingShadowView();
}

void R_StudioClearVanillaBonesCaches()
{
	//TODO: draw a null model with no bone and no bodypart ?
}

void R_FreeStudioBoneCache(CStudioBoneCache* pStudioBoneCache)
{
	pStudioBoneCache->m_next = g_pStudioBoneFreeCaches;
	g_pStudioBoneFreeCaches = pStudioBoneCache;
}

void R_StudioClearAllBoneCaches()
{
	for (int i = 0; i < MAX_STUDIO_BONE_CACHES - 1; i++)
		g_StudioBoneCaches[i].m_next = &g_StudioBoneCaches[i + 1];

	g_StudioBoneCaches[MAX_STUDIO_BONE_CACHES - 1].m_next = NULL;

	g_pStudioBoneFreeCaches = &g_StudioBoneCaches[0];

	g_StudioBoneCacheManager.clear();
}

CStudioBoneCache* R_AllocStudioBoneCache()
{
	if (!g_pStudioBoneFreeCaches)
	{
		gEngfuncs.Con_DPrintf("R_AllocStudioBoneCache: Studio bone caches overflow!\n");
		return NULL;
	}

	auto pStudioBoneCache = g_pStudioBoneFreeCaches;
	g_pStudioBoneFreeCaches = pStudioBoneCache->m_next;

	pStudioBoneCache->m_next = NULL;

	return pStudioBoneCache;
}

void R_StudioSaveBoneCache(studiohdr_t* studiohdr, int modelindex, int sequence, int gaitsequence, float frame, const float* origin, const float* angles)
{
	auto cache = R_AllocStudioBoneCache();

	if (cache)
	{
		CStudioBoneCacheHandle handle(
			modelindex,
			sequence,
			gaitsequence,
			frame,
			origin,
			angles);

		memcpy(cache->m_bonetransform, (*pbonetransform), sizeof(cache->m_bonetransform));
		memcpy(cache->m_lighttransform, (*plighttransform), sizeof(cache->m_lighttransform));

		g_StudioBoneCacheManager[handle] = cache;
	}
}

bool R_StudioLoadBoneCache(studiohdr_t* studiohdr, int modelindex, int sequence, int gaitsequence, float frame, const float* origin, const float* angles)
{
	CStudioBoneCacheHandle handle(
		modelindex,
		sequence,
		gaitsequence,
		frame,
		origin,
		angles);

	const auto& itor = g_StudioBoneCacheManager.find(handle);

	if (itor != g_StudioBoneCacheManager.end())
	{
		memcpy((*pbonetransform), itor->second->m_bonetransform, sizeof(itor->second->m_bonetransform));
		memcpy((*plighttransform), itor->second->m_lighttransform, sizeof(itor->second->m_lighttransform));
		return true;
	}

	return false;
}

void R_PrepareStudioRenderSubmodel(
	studiohdr_t* studiohdr, mstudiomodel_t* submodel,
	std::vector<CStudioModelRenderVertex>& vVertex,
	std::vector<unsigned int>& vIndices,
	CStudioModelRenderSubModel* vboSubmodel)
{
	auto pstudioverts = (const vec3_t*)((byte*)studiohdr + submodel->vertindex);
	auto pstudionorms = (const vec3_t*)((byte*)studiohdr + submodel->normindex);
	auto pvertbone = ((byte*)studiohdr + submodel->vertinfoindex);
	auto pnormbone = ((byte*)studiohdr + submodel->norminfoindex);

	vboSubmodel->vMesh.reserve(submodel->nummesh);

	for (int k = 0; k < submodel->nummesh; k++)
	{
		auto pmesh = (mstudiomesh_t*)((byte*)studiohdr + submodel->meshindex) + k;

		auto ptricmds = (short*)((byte*)studiohdr + pmesh->triindex);

		int iStartVertex = vVertex.size();
		int iNumVertex = 0;

		CStudioModelRenderMesh VBOMesh;
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

CStudioModelRenderData* R_GetStudioRenderDataFromStudioHeaderFast(studiohdr_t* studiohdr)
{
	if (studiohdr->soundtable < 0 || studiohdr->soundtable >= (int)g_StudioRenderDataCache.size())
		return nullptr;

	auto pRenderData = g_StudioRenderDataCache[studiohdr->soundtable];

	if (pRenderData)
	{
		auto mod = pRenderData->BodyModel;

		if (mod && mod->type == mod_studio)
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT || mod->needload == NL_UNREFERENCED)
			{
				if (mod->cache.data == studiohdr)
				{
					return pRenderData;
				}
			}
		}
	}

	return nullptr;
}

CStudioModelRenderData* R_GetStudioRenderDataFromStudioHeaderSlow(studiohdr_t* studiohdr)
{
	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);

		if (mod && mod->type == mod_studio)
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT || mod->needload == NL_UNREFERENCED)
			{
				if (mod->cache.data == studiohdr)
				{
					auto pRenderData = R_CreateStudioRenderData(mod, studiohdr);

					if (pRenderData)
					{
						R_StudioLoadExternalFile(mod, studiohdr, pRenderData);
						R_StudioLoadTextureModel(mod, studiohdr, pRenderData);

						return pRenderData;
					}
				}
			}
		}
	}

	return nullptr;
}

CStudioModelRenderData* R_GetStudioRenderDataFromModel(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex >= (int)g_StudioRenderDataCache.size())
		return NULL;

	return g_StudioRenderDataCache[modelindex];
}

void R_AllocSlotForStudioRenderData(model_t* mod, studiohdr_t *studiohdr, CStudioModelRenderData* pRenderData)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex >= (int)g_StudioRenderDataCache.size())
	{
		g_StudioRenderDataCache.resize(modelindex + 1);
	}

	g_StudioRenderDataCache[modelindex] = pRenderData;

	studiohdr->soundtable = modelindex;
}

CStudioModelRenderData* R_CreateStudioRenderData(model_t* mod, studiohdr_t* studiohdr)
{
	if (!studiohdr->numbodyparts)
		return NULL;

	auto pRenderData = R_GetStudioRenderDataFromModel(mod);

	if (pRenderData)
	{
		studiohdr->soundtable = EngineGetModelIndex(mod);

		gEngfuncs.Con_DPrintf("R_CreateStudioRenderData: Found modelindex[%d] modname[%s].\n", EngineGetModelIndex(mod), mod->name);

		return pRenderData;
	}

	gEngfuncs.Con_DPrintf("R_CreateStudioRenderData: Create modelindex[%d] modname[%s].\n", EngineGetModelIndex(mod), mod->name);

	pRenderData = new CStudioModelRenderData(mod);

	R_AllocSlotForStudioRenderData(mod, studiohdr, pRenderData);

	std::vector<CStudioModelRenderVertex> vVertex;
	std::vector<GLuint> vIndices;

	for (int i = 0; i < studiohdr->numbodyparts; i++)
	{
		auto bodypart = (mstudiobodyparts_t*)((byte*)studiohdr + studiohdr->bodypartindex) + i;

		if (bodypart->modelindex && bodypart->nummodels)
		{
			for (int j = 0; j < bodypart->nummodels; ++j)
			{
				auto submodel = (mstudiomodel_t*)((byte*)studiohdr + bodypart->modelindex) + j;

				auto pRenderSubmodel = new CStudioModelRenderSubModel;

				R_PrepareStudioRenderSubmodel(studiohdr, submodel, vVertex, vIndices, pRenderSubmodel);

				pRenderData->vSubmodels.emplace_back(pRenderSubmodel);

				auto submodel_byteoffset = (byte*)submodel - (byte*)studiohdr;

				pRenderData->mSubmodels[submodel_byteoffset] = pRenderSubmodel;
			}
		}
	}

	pRenderData->hVBO = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(pRenderData->hVBO, vVertex.size() * sizeof(CStudioModelRenderVertex), vVertex.data());

	pRenderData->hEBO = GL_GenBuffer();
	GL_UploadDataToEBOStaticDraw(pRenderData->hEBO, vIndices.size() * sizeof(GLuint), vIndices.data());

	pRenderData->hVAO = GL_GenVAO();
	GL_BindStatesForVAO(pRenderData->hVAO, pRenderData->hVBO, pRenderData->hEBO,
		[]() {
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(CStudioModelRenderVertex), OFFSET(CStudioModelRenderVertex, pos));
			glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(CStudioModelRenderVertex), OFFSET(CStudioModelRenderVertex, normal));
			glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(CStudioModelRenderVertex), OFFSET(CStudioModelRenderVertex, texcoord));
			glVertexAttribIPointer(3, 2, GL_INT, sizeof(CStudioModelRenderVertex), OFFSET(CStudioModelRenderVertex, vertbone));
		},
		[]() {
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
		});

	pRenderData->CelshadeControl.base_specular.Init(r_studio_base_specular, 2, ConVar_None);
	pRenderData->CelshadeControl.celshade_specular.Init(r_studio_celshade_specular, 4, ConVar_None);

	pRenderData->CelshadeControl.celshade_midpoint.Init(r_studio_celshade_midpoint, 1, ConVar_None);
	pRenderData->CelshadeControl.celshade_softness.Init(r_studio_celshade_softness, 1, ConVar_None);
	pRenderData->CelshadeControl.celshade_shadow_color.Init(r_studio_celshade_shadow_color, 3, ConVar_Color255);
	pRenderData->CelshadeControl.celshade_head_offset.Init(r_studio_celshade_head_offset, 3, ConVar_None);
	pRenderData->CelshadeControl.celshade_lightdir_adjust.Init(r_studio_celshade_lightdir_adjust, 2, ConVar_None);

	pRenderData->CelshadeControl.outline_size.Init(r_studio_outline_size, 1, ConVar_None);
	pRenderData->CelshadeControl.outline_dark.Init(r_studio_outline_dark, 1, ConVar_None);

	pRenderData->CelshadeControl.rimlight_power.Init(r_studio_rimlight_power, 1, ConVar_None);
	pRenderData->CelshadeControl.rimlight_smooth.Init(r_studio_rimlight_smooth, 1, ConVar_None);
	pRenderData->CelshadeControl.rimlight_smooth2.Init(r_studio_rimlight_smooth2, 2, ConVar_None);
	pRenderData->CelshadeControl.rimlight_color.Init(r_studio_rimlight_color, 3, ConVar_Color255);

	pRenderData->CelshadeControl.rimdark_power.Init(r_studio_rimdark_power, 1, ConVar_None);
	pRenderData->CelshadeControl.rimdark_smooth.Init(r_studio_rimdark_smooth, 1, ConVar_None);
	pRenderData->CelshadeControl.rimdark_smooth2.Init(r_studio_rimdark_smooth2, 2, ConVar_None);
	pRenderData->CelshadeControl.rimdark_color.Init(r_studio_rimdark_color, 3, ConVar_Color255);

	pRenderData->CelshadeControl.hair_specular_exp.Init(r_studio_hair_specular_exp, 1, ConVar_None);
	pRenderData->CelshadeControl.hair_specular_exp2.Init(r_studio_hair_specular_exp2, 1, ConVar_None);
	pRenderData->CelshadeControl.hair_specular_noise.Init(r_studio_hair_specular_noise, 4, ConVar_None);
	pRenderData->CelshadeControl.hair_specular_noise2.Init(r_studio_hair_specular_noise2, 4, ConVar_None);
	pRenderData->CelshadeControl.hair_specular_intensity.Init(r_studio_hair_specular_intensity, 3, ConVar_None);
	pRenderData->CelshadeControl.hair_specular_intensity2.Init(r_studio_hair_specular_intensity2, 3, ConVar_None);
	pRenderData->CelshadeControl.hair_specular_smooth.Init(r_studio_hair_specular_smooth, 2, ConVar_None);
	pRenderData->CelshadeControl.hair_shadow_offset.Init(r_studio_hair_shadow_offset, 2, ConVar_None);

	pRenderData->LowerBodyControl.model_origin.Init(r_lowerbody_model_offset, 3, ConVar_None);
	pRenderData->LowerBodyControl.model_scale.Init(r_lowerbody_model_scale, 1, ConVar_None);

	return pRenderData;
}

void R_FreeStudioRenderData(CStudioModelRenderData *pRenderData)
{
	auto mod = pRenderData->BodyModel;

	auto modelindex = EngineGetModelIndex(mod);

	g_StudioRenderDataCache[modelindex] = NULL;

	delete pRenderData;

	gEngfuncs.Con_DPrintf("R_FreeStudioRenderData: modelindex[%d] modname[%s]!\n", EngineGetModelIndex(mod), mod->name);
}

void R_FreeAllUnreferencedStudioRenderData(void)
{
	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);

		if (mod->type == mod_studio)
		{
			if (mod->needload == NL_UNREFERENCED)
			{
				auto pStudioRenderData = R_GetStudioRenderDataFromModel(mod);

				if (pStudioRenderData)
				{
					R_FreeStudioRenderData(pStudioRenderData);
				}
			}
		}
	}
}

void R_FreeAllStudioRenderData(void)
{
	for (size_t i = 0; i < g_StudioRenderDataCache.size(); ++i)
	{
		if (g_StudioRenderDataCache[i])
		{
			delete g_StudioRenderDataCache[i];
			g_StudioRenderDataCache[i] = nullptr;
		}
	}
}

void R_StudioReloadAllStudioRenderData(void)
{
	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_studio)
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);

				if (studiohdr)
				{
					auto pRenderData = R_CreateStudioRenderData(mod, studiohdr);

					if (pRenderData)
					{
						R_StudioLoadExternalFile(mod, studiohdr, pRenderData);
						R_StudioLoadTextureModel(mod, studiohdr, pRenderData);
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

		if ((state & STUDIO_OIT_BLEND_ENABLED) && g_bUseOITBlend)
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

		if (state & STUDIO_CLIP_BONE_ENABLED)
			defs << "#define CLIP_BONE_ENABLED\n";

		if (state & STUDIO_LEGACY_DLIGHT_ENABLED)
			defs << "#define LEGACY_DLIGHT_ENABLED\n";

		if (state & STUDIO_LEGACY_ELIGHT_ENABLED)
			defs << "#define LEGACY_ELIGHT_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\studio_shader.vert.glsl", "renderer\\shader\\studio_shader.frag.glsl", def.c_str(), def.c_str(), NULL);
		
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

			if (g_CurrentRenderData)
			{
				g_CurrentRenderData->CelshadeControl.base_specular.GetValues(values);
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

			if (g_CurrentRenderData)
			{
				g_CurrentRenderData->CelshadeControl.celshade_specular.GetValues(values);
			}
			else
			{
				r_studio_celshade_specular->FetchValues(values);
			}

			glUniform4f(prog.r_celshade_specular, values[0], values[1], values[2], values[3]);
		}

		if (prog.r_celshade_midpoint != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_celshade_midpoint, g_CurrentRenderData->CelshadeControl.celshade_midpoint.GetValue());
			}
			else
			{
				glUniform1f(prog.r_celshade_midpoint, r_studio_celshade_midpoint->value);
			}
		}

		if (prog.r_celshade_softness != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_celshade_softness, g_CurrentRenderData->CelshadeControl.celshade_softness.GetValue());
			}
			else
			{
				glUniform1f(prog.r_celshade_softness, r_studio_celshade_softness->value);
			}
		}

		if (prog.r_celshade_shadow_color != -1)
		{
			vec3_t color = { 0 };

			if (g_CurrentRenderData)
			{
				g_CurrentRenderData->CelshadeControl.celshade_shadow_color.GetValues(color);
			}
			else
			{
				UTIL_ParseCvarAsColor3(r_studio_celshade_shadow_color, color);
			}

			glUniform3f(prog.r_celshade_shadow_color, color[0], color[1], color[2]);
		}

		if (prog.r_celshade_head_offset != -1)
		{
			if (g_CurrentRenderData)
			{
				vec3_t offset = { 0 };
				g_CurrentRenderData->CelshadeControl.celshade_head_offset.GetValues(offset);
				glUniform3f(prog.r_celshade_head_offset, offset[0], offset[1], offset[2]);
			}
			else
			{
				vec3_t offset = { 0 };
				UTIL_ParseCvarAsVector3(r_studio_celshade_head_offset, offset);
				glUniform3f(prog.r_celshade_head_offset, offset[0], offset[1], offset[2]);
			}
		}

		if (prog.r_celshade_lightdir_adjust != -1)
		{
			if (g_CurrentRenderData)
			{
				vec2_t value = { 0 };
				g_CurrentRenderData->CelshadeControl.celshade_lightdir_adjust.GetValues(value);
				glUniform2f(prog.r_celshade_lightdir_adjust, value[0], value[1]);
			}
			else
			{
				vec2_t value = { 0 };
				UTIL_ParseCvarAsVector2(r_studio_celshade_lightdir_adjust, value);
				glUniform2f(prog.r_celshade_lightdir_adjust, value[0], value[1]);
			}
		}

		if (prog.r_outline_dark != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_outline_dark, g_CurrentRenderData->CelshadeControl.outline_dark.GetValue());
			}
			else
			{
				glUniform1f(prog.r_outline_dark, r_studio_outline_dark->value);
			}
		}

		if (prog.r_rimlight_power != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_rimlight_power, g_CurrentRenderData->CelshadeControl.rimlight_power.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimlight_power, r_studio_rimlight_power->value);
			}
		}

		if (prog.r_rimlight_smooth != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_rimlight_smooth, g_CurrentRenderData->CelshadeControl.rimlight_smooth.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimlight_smooth, r_studio_rimlight_smooth->value);
			}
		}

		if (prog.r_rimlight_smooth2 != -1)
		{
			vec2_t values = { 0 };

			if (g_CurrentRenderData)
			{
				g_CurrentRenderData->CelshadeControl.rimlight_smooth2.GetValues(values);
			}
			else
			{
				UTIL_ParseCvarAsVector2(r_studio_rimlight_color, values);
			}

			glUniform2f(prog.r_rimlight_smooth2, values[0], values[1]);
		}

		if (prog.r_rimlight_color != -1)
		{
			vec3_t color = { 0 };
			if (g_CurrentRenderData)
			{
				g_CurrentRenderData->CelshadeControl.rimlight_color.GetValues(color);
			}
			else
			{
				UTIL_ParseCvarAsColor3(r_studio_rimlight_color, color);
			}
			glUniform3f(prog.r_rimlight_color, color[0], color[1], color[2]);
		}

		if (prog.r_rimdark_power != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_rimdark_power, g_CurrentRenderData->CelshadeControl.rimdark_power.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimdark_power, r_studio_rimdark_power->value);
			}
		}

		if (prog.r_rimdark_smooth != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_rimdark_smooth, g_CurrentRenderData->CelshadeControl.rimdark_smooth.GetValue());
			}
			else
			{
				glUniform1f(prog.r_rimdark_smooth, r_studio_rimdark_smooth->value);
			}
		}

		if (prog.r_rimdark_smooth2 != -1)
		{
			vec2_t values = { 0 };

			if (g_CurrentRenderData)
			{
				g_CurrentRenderData->CelshadeControl.rimdark_smooth2.GetValues(values);
			}
			else
			{
				UTIL_ParseCvarAsVector2(r_studio_rimdark_color, values);
			}

			glUniform2f(prog.r_rimdark_smooth2, values[0], values[1]);
		}

		if (prog.r_rimdark_color != -1)
		{
			vec3_t color = { 0 };

			if (g_CurrentRenderData)
			{
				g_CurrentRenderData->CelshadeControl.rimdark_color.GetValues(color);
			}
			else
			{
				UTIL_ParseCvarAsColor3(r_studio_rimdark_color, color);
			}

			glUniform3f(prog.r_rimdark_color, color[0], color[1], color[2]);
		}

		if (prog.r_hair_specular_exp != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_hair_specular_exp, g_CurrentRenderData->CelshadeControl.hair_specular_exp.GetValue());
			}
			else
			{
				glUniform1f(prog.r_hair_specular_exp, r_studio_hair_specular_exp->value);
			}
		}

		if (prog.r_hair_specular_noise != -1)
		{
			if (g_CurrentRenderData)
			{
				vec4_t values = { 0 };
				g_CurrentRenderData->CelshadeControl.hair_specular_noise.GetValues(values);
				glUniform4f(prog.r_hair_specular_noise, values[0], values[1], values[2], values[3]);
			}
			else
			{
				vec4_t values = { 0 };
				UTIL_ParseCvarAsVector4(r_studio_hair_specular_noise, values);
				glUniform4f(prog.r_hair_specular_noise, values[0], values[1], values[2], values[3]);
			}
		}

		if (prog.r_hair_specular_intensity != -1)
		{
			if (g_CurrentRenderData)
			{
				vec3_t values = { 0 };
				g_CurrentRenderData->CelshadeControl.hair_specular_intensity.GetValues(values);
				glUniform3f(prog.r_hair_specular_intensity, values[0], values[1], values[2]);
			}
			else
			{
				vec3_t values = { 0 };
				UTIL_ParseCvarAsVector3(r_studio_hair_specular_intensity, values);
				glUniform3f(prog.r_hair_specular_intensity, values[0], values[1], values[2]);
			}
		}

		if (prog.r_hair_specular_exp2 != -1)
		{
			if (g_CurrentRenderData)
			{
				glUniform1f(prog.r_hair_specular_exp2, g_CurrentRenderData->CelshadeControl.hair_specular_exp2.GetValue());
			}
			else
			{
				glUniform1f(prog.r_hair_specular_exp2, r_studio_hair_specular_exp2->value);
			}
		}

		if (prog.r_hair_specular_noise2 != -1)
		{
			if (g_CurrentRenderData)
			{
				vec4_t values = { 0 };
				g_CurrentRenderData->CelshadeControl.hair_specular_noise2.GetValues(values);
				glUniform4f(prog.r_hair_specular_noise2, values[0], values[1], values[2], values[3]);
			}
			else
			{
				vec4_t values = { 0 };
				UTIL_ParseCvarAsVector4(r_studio_hair_specular_noise2, values);
				glUniform4f(prog.r_hair_specular_noise2, values[0], values[1], values[2], values[3]);
			}
		}

		if (prog.r_hair_specular_intensity2 != -1)
		{
			if (g_CurrentRenderData)
			{
				vec3_t values = { 0 };
				g_CurrentRenderData->CelshadeControl.hair_specular_intensity2.GetValues(values);
				glUniform3f(prog.r_hair_specular_intensity2, values[0], values[1], values[2]);
			}
			else
			{
				vec3_t values = { 0 };
				UTIL_ParseCvarAsVector3(r_studio_hair_specular_intensity2, values);
				glUniform3f(prog.r_hair_specular_intensity2, values[0], values[1], values[2]);
			}
		}

		if (prog.r_hair_specular_smooth != -1)
		{
			if (g_CurrentRenderData)
			{
				vec2_t values = { 0 };
				g_CurrentRenderData->CelshadeControl.hair_specular_smooth.GetValues(values);
				glUniform2f(prog.r_hair_specular_smooth, values[0], values[1]);
			}
			else
			{
				vec2_t values = { 0 };
				UTIL_ParseCvarAsVector2(r_studio_hair_specular_smooth, values);
				glUniform2f(prog.r_hair_specular_smooth, values[0], values[1]);
			}
		}

		if (prog.r_hair_shadow_offset != -1)
		{
			if (g_CurrentRenderData)
			{
				vec2_t values = { 0 };
				g_CurrentRenderData->CelshadeControl.hair_shadow_offset.GetValues(values);
				glUniform2f(prog.r_hair_shadow_offset, values[0], values[1]);
			}
			else
			{
				vec2_t values = { 0 };
				UTIL_ParseCvarAsVector2(r_studio_hair_shadow_offset, values);
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
{ STUDIO_CLIP_BONE_ENABLED				,"STUDIO_CLIP_BONE_ENABLED"					},
{ STUDIO_LEGACY_DLIGHT_ENABLED			,"STUDIO_LEGACY_DLIGHT_ENABLED"				},
{ STUDIO_LEGACY_ELIGHT_ENABLED			,"STUDIO_LEGACY_ELIGHT_ENABLED"				},

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

	R_StudioFlushAllSkins();
	R_FreeAllStudioRenderData();

	if (g_hStudioUBO)
	{
		GL_DeleteBuffer(g_hStudioUBO);
		g_hStudioUBO = 0;
	}
}

void R_InitStudio(void)
{
	g_hStudioUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, g_hStudioUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(studio_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	r_studio_debug = gEngfuncs.pfnRegisterVariable("r_studio_debug", "0", FCVAR_CLIENTDLL);
#if 0
	r_studio_viewmodel_lightdir_adjust = gEngfuncs.pfnRegisterVariable("r_studio_viewmodel_lightdir_adjust", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
#endif
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

	/*
	r_studio_legacy_dlight 0: Completely disable legacy dlight
	r_studio_legacy_dlight 1: Setup studio's internal lighting structure with legacy dlight (Vanilla behavior)
	r_studio_legacy_dlight 2: Use shader to add up all dynamic lights
	*/

	r_studio_legacy_dlight = gEngfuncs.pfnRegisterVariable("r_studio_legacy_dlight", "2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	/*
	r_studio_legacy_elight 0: Completely disable entity dlight
	r_studio_legacy_elight 1: Enable entity dlight (Vanilla behavior)
	*/

	r_studio_legacy_elight = gEngfuncs.pfnRegisterVariable("r_studio_legacy_elight", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	//Cache bones to save CPU resources?
	r_studio_bone_caches = gEngfuncs.pfnRegisterVariable("r_studio_bone_caches", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_external_textures = gEngfuncs.pfnRegisterVariable("r_studio_external_textures", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_base_specular = R_RegisterMapCvar("r_studio_base_specular", "1.0 2.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 2, ConVar_None);
	r_studio_celshade_specular = R_RegisterMapCvar("r_studio_celshade_specular", "1.0  36.0  0.4  0.6", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 4, ConVar_None);

	r_lowerbody_model_offset = gEngfuncs.pfnRegisterVariable("r_lowerbody_model_offset", "0 0 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_lowerbody_model_scale = gEngfuncs.pfnRegisterVariable("r_lowerbody_model_scale", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

studiohdr_t* R_StudioGetTextureHeader(CStudioModelRenderData* pRenderData)
{
	if ((*pstudiohdr)->textureindex == 0 && pRenderData->TextureModel)
	{
		auto ptexturehdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(pRenderData->TextureModel);

		//Fix: could be nullptr ?
		if (ptexturehdr)
			return ptexturehdr;

		return NULL;
	}

	return (*pstudiohdr);
}

void R_StudioLoadTextureModel(model_t* mod, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData)
{
	if (studiohdr->textureindex == 0 && !pRenderData->TextureModel)
	{
		//This is actually 260 instead of 256
		char modelname[260];

		size_t maxmodelname = sizeof(modelname) - 1 - (sizeof("T.mdl"));

		strncpy(modelname, mod->name, maxmodelname);
		modelname[maxmodelname] = 0;

		strcpy(&modelname[strlen(modelname) - 4], "T.mdl");

		auto texmodel = IEngineStudio.Mod_ForName(modelname, true);
		//mod->texinfo = (mtexinfo_t*)texmodel;

		pRenderData->TextureModel = texmodel;

		auto ptexturehdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(texmodel);
		strncpy(ptexturehdr->name, modelname, sizeof(ptexturehdr->name) - 1);
		ptexturehdr->name[sizeof(ptexturehdr->name) - 1] = 0;
	}
}

void R_StudioSetupMaterial(const CStudioModelRenderData* pRenderData, const CStudioModelRenderMaterial* pStudioMaterial, CStudioSetupSkinContext* context)
{
	if (r_studio_external_textures->value > 0)
	{
		const auto& ReplaceTexture = pStudioMaterial->textures[STUDIO_REPLACE_TEXTURE - 1];
		if (ReplaceTexture.gltexturenum)
		{
			if (ReplaceTexture.numframes)
			{
				glActiveTexture(GL_TEXTURE0 + STUDIO_RESERVED_TEXTURE_ANIMATED);
				glBindTexture(GL_TEXTURE_2D_ARRAY, ReplaceTexture.gltexturenum);
				glActiveTexture(GL_TEXTURE0);

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

		const auto& NormalTexture = pStudioMaterial->textures[STUDIO_NORMAL_TEXTURE - 1];

		if (NormalTexture.gltexturenum)
		{
			glActiveTexture(GL_TEXTURE0 + STUDIO_NORMAL_TEXTURE);
			glBindTexture(GL_TEXTURE_2D, NormalTexture.gltexturenum);
			glActiveTexture(GL_TEXTURE0);

			(*context->StudioProgramState) |= STUDIO_NORMALTEXTURE_ENABLED;
		}

		const auto& SpecularTexture = pStudioMaterial->textures[STUDIO_SPECULAR_TEXTURE - 1];

		if (SpecularTexture.gltexturenum)
		{
			glActiveTexture(GL_TEXTURE0 + STUDIO_SPECULAR_TEXTURE);
			glBindTexture(GL_TEXTURE_2D, SpecularTexture.gltexturenum);
			glActiveTexture(GL_TEXTURE0);

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

void R_ParsePackedSkinInternal(const char* texture, int i, CStudioSetupSkinContext* context)
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

void R_ParsePackedSkin(const char* texture, CStudioSetupSkinContext* context)
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
		auto pSkinCache = new (std::nothrow) CStudioSkinCache;

		if (pSkinCache)
		{
			memset(pSkinCache, 0, sizeof(CStudioSkinCache));

			pSkinCache->skins[index].keynum = keynum;
			pSkinCache->skins[index].topcolor = -1;
			pSkinCache->skins[index].bottomcolor = -1;
			g_StudioSkinCache[keynum] = pSkinCache;

			return &pSkinCache->skins[index];
		}
	}

	return nullptr;
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

void R_StudioSetupSkinEx(const CStudioModelRenderData* pRenderData, studiohdr_t* ptexturehdr, int index, CStudioSetupSkinContext*context)
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

						GL_UnloadTextureWithType(fullname, GLT_STUDIO);

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
		auto pStudioMaterial = R_StudioGetMaterialFromTextureId(pRenderData, (*currenttexture));

		if (pStudioMaterial)
		{
			R_StudioSetupMaterial(pRenderData, pStudioMaterial, context);
		}
	}
}

void R_StudioDrawRenderDataBegin(CStudioModelRenderData* pRenderData)
{
	studio_ubo_t StudioUBO = {0};

	g_CurrentRenderData = pRenderData;

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
		StudioUBO.r_scale = g_CurrentRenderData->CelshadeControl.outline_size.GetValue() * 0.05f;
	}

	memcpy(StudioUBO.r_plightvec, r_plightvec, sizeof(vec3_t));

#if 0
	if (R_IsRenderingViewModel() && r_studio_viewmodel_lightdir_adjust->value > 0)
	{
		StudioUBO.r_plightvec[2] = StudioUBO.r_plightvec[2] * math_clamp(r_studio_viewmodel_lightdir_adjust->value, 0, 1);

		VectorNormalize(StudioUBO.r_plightvec);
	}
#endif

	vec3_t entity_origin = { (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3] };
	memcpy(StudioUBO.entity_origin, entity_origin, sizeof(vec3_t));

	StudioUBO.r_numelight = 0;

	if ((int)r_studio_legacy_elight->value >= 1)
	{
		StudioUBO.r_numelight = (*numlights);

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

			StudioUBO.r_elight_radius[i] = math_clamp((*locallight)[i]->radius, 0, 999999);
		}
	}

	memcpy(StudioUBO.bonematrix, (*pbonetransform), sizeof(mat3x4) * 128);

	if (R_IsRenderingClippedLowerBody())
	{
		auto pbones = (mstudiobone_t*)((byte*)(*pstudiohdr) + (*pstudiohdr)->boneindex);

		for (int i = 0; i < (*pstudiohdr)->numbones; ++i)
		{
			if (!(pbones[i].flags & STUDIO_BF_LOWERBODY))
			{
				int slot = i / 32;
				int index = i - slot * 4;
				StudioUBO.r_clipbone[slot] |= (1 << index);
			}
		}
	}

	GL_UploadSubDataToUBO(g_hStudioUBO, 0, sizeof(StudioUBO), &StudioUBO);

	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_STUDIO_UBO, g_hStudioUBO);

	GL_BindVAO(pRenderData->hVAO);
}

void R_StudioDrawMesh_AnalysisPass(
	CStudioModelRenderData* pRenderData,
	CStudioModelRenderSubModel* pRenderSubmodel,
	CStudioModelRenderMesh* pRenderMesh,
	mstudiomesh_t* pmesh,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref,
	const int flags)
{
	//Analysis pass
	if (R_IsRenderingShadowView())
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

void R_StudioDrawMesh_DrawPass(
	CStudioModelRenderData* pRenderData,
	CStudioModelRenderSubModel* pRenderSubmodel,
	CStudioModelRenderMesh* pRenderMesh,
	mstudiomesh_t* pmesh,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref,
	const int flags)
{
	program_state_t StudioProgramState = flags;

	if (R_IsRenderingShadowView())
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

	if (!(StudioProgramState & (STUDIO_ALPHA_BLEND_ENABLED | STUDIO_ADDITIVE_BLEND_ENABLED)) && (*currententity)->curstate.rendermode == kRenderTransAdd )
	{
		StudioProgramState |= STUDIO_ADDITIVE_BLEND_ENABLED;
	}

	if ((*currententity)->curstate.rendermode == kRenderTransAdd)
	{
		StudioProgramState |= STUDIO_ADDITIVE_RENDER_MODE_ENABLED;
	}

	if (!(StudioProgramState & (STUDIO_ALPHA_BLEND_ENABLED | STUDIO_ADDITIVE_BLEND_ENABLED)) && (*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.renderamt < 255)
	{
		StudioProgramState |= STUDIO_ALPHA_BLEND_ENABLED;
	}

	if ((int)r_studio_legacy_dlight->value >= 2)
	{
		StudioProgramState |= STUDIO_LEGACY_DLIGHT_ENABLED;
	}

	if ((int)r_studio_legacy_elight->value >= 1)
	{
		StudioProgramState |= STUDIO_LEGACY_ELIGHT_ENABLED;
	}

	if (R_IsRenderingWaterView())
	{
		StudioProgramState |= STUDIO_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		StudioProgramState |= STUDIO_CLIP_ENABLED;
	}

	if (!R_IsRenderingGBuffer())
	{
		if ((StudioProgramState & STUDIO_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
		{

		}
		else if ((StudioProgramState & STUDIO_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
		{

		}
		else if(R_IsRenderingFog())
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
	}

	if (R_IsRenderingGBuffer())
	{
		StudioProgramState |= STUDIO_GBUFFER_ENABLED;

		StudioProgramState &= ~STUDIO_LEGACY_DLIGHT_ENABLED;
	}

	if (R_IsRenderingGammaBlending())
	{
		StudioProgramState |= STUDIO_GAMMA_BLEND_ENABLED;
	}

	if (r_draw_oitblend && (StudioProgramState & (STUDIO_ALPHA_BLEND_ENABLED | STUDIO_ADDITIVE_BLEND_ENABLED)))
	{
		StudioProgramState |= STUDIO_OIT_BLEND_ENABLED;
	}

	if (R_IsRenderingFlippedViewModel())
	{
		StudioProgramState |= (STUDIO_NF_DOUBLE_FACE | STUDIO_REVERT_NORMAL_ENABLED);
	}

	if (R_IsRenderingClippedLowerBody())
	{
		StudioProgramState |= STUDIO_CLIP_BONE_ENABLED;
	}

	if (r_studio_debug->value > 0)
	{
		StudioProgramState |= STUDIO_DEBUG_ENABLED;
	}

	CStudioSetupSkinContext Context(&StudioProgramState);

	if (r_fullbright->value >= 2)
	{
		gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);

		Context.s = 1.0f / 256.0f;
		Context.t = 1.0f / 256.0f;
	}
	else
	{
		Context.width = ptexture[pskinref[pmesh->skinref]].width;
		Context.height = ptexture[pskinref[pmesh->skinref]].height;

		if (StudioProgramState & STUDIO_GLOW_SHELL_ENABLED)
		{
			gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_shell, 0);
		}
		else
		{
			if (ptexturehdr && pskinref)
			{
				R_StudioSetupSkinEx(pRenderData, ptexturehdr, pskinref[pmesh->skinref], &Context);
			}
			else
			{
				gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);
			}
		}

		Context.s = 1.0f / Context.width;
		Context.t = 1.0f / Context.height;
	}

	if (StudioProgramState & STUDIO_NF_CHROME)
	{
		if (StudioProgramState & STUDIO_GLOW_SHELL_ENABLED)
		{
			Context.s /= 32.0f;
			Context.t /= 32.0f;
		}
		else
		{
			Context.s = 1.0f / 2048.0f;
			Context.t = 1.0f / 2048.0f;
		}
	}

	if ((StudioProgramState & STUDIO_PACKED_TEXTURE_ALLBITS) && Context.packedCount > 0)
	{
		Context.packedStride = 1.0f / Context.packedCount;
	}

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	if (StudioProgramState & STUDIO_OUTLINE_ENABLED)
	{
		//Writing outline...
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
		if (r_draw_opaque)
		{
			int iStencilRef = STENCIL_MASK_WORLD;

			if (r_draw_hasoutline)
				iStencilRef |= STENCIL_MASK_HAS_OUTLINE;

			if (StudioProgramState & (STUDIO_NF_FLATSHADE | STUDIO_NF_CELSHADE))
				iStencilRef |= STENCIL_MASK_HAS_FLATSHADE;

			GL_BeginStencilWrite(iStencilRef, STENCIL_MASK_ALL);
		}
		else
		{
			int iStencilRef = 0;

			if (r_draw_hasoutline)
				iStencilRef |= STENCIL_MASK_HAS_OUTLINE;

			if (StudioProgramState & (STUDIO_NF_FLATSHADE | STUDIO_NF_CELSHADE))
				iStencilRef |= STENCIL_MASK_HAS_FLATSHADE;

			GL_BeginStencilWrite(iStencilRef, STENCIL_MASK_HAS_OUTLINE | STENCIL_MASK_HAS_FLATSHADE);
		}
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	if (StudioProgramState & STUDIO_NF_DOUBLE_FACE)
	{
		glDisable(GL_CULL_FACE);
	}

	if (StudioProgramState & STUDIO_SHADOW_CASTER_ENABLED)
	{
		//client.dll!StudioRenderFinal enables GL_BLEND in GL_SetRenderMode and this will mess everything up. see r_studio.c~studioapi_GL_SetRenderMode~qglEnable( GL_BLEND );
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
			//idk why but Valve uses GL_TRUE anyway, it should be GL_FALSE in general
			glDepthMask(GL_TRUE);

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

	studio_program_t prog = { 0 };

	R_UseStudioProgram(StudioProgramState, &prog);

	if (prog.r_uvscale != -1)
	{
		glUniform2f(prog.r_uvscale, Context.s, Context.t);
	}

	if (prog.r_packed_stride != -1)
	{
		glUniform1f(prog.r_packed_stride, Context.packedStride);
	}

	if (prog.r_packed_index != -1)
	{
		glUniform4f(prog.r_packed_index, Context.packedDiffuseIndex, Context.packedNormalIndex, Context.packedParallaxIndex, Context.packedSpecularIndex);
	}

	if (prog.r_framerate_numframes != -1)
	{
		glUniform2f(prog.r_framerate_numframes, Context.framerate, Context.numframes);
	}

	if (pRenderMesh->iIndiceCount)
	{
		glDrawElements(GL_TRIANGLES, pRenderMesh->iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(pRenderMesh->iStartIndex));

		++r_studio_drawcall;
		r_studio_polys += pRenderMesh->iPolyCount;
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

	glActiveTexture(GL_TEXTURE0);

	//Restore states
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	GL_EndStencil();
}

void R_StudioDrawMesh(
	CStudioModelRenderData* pRenderData,
	CStudioModelRenderSubModel* pRenderSubmodel,
	CStudioModelRenderMesh* pRenderMesh,
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
		R_StudioDrawMesh_AnalysisPass(
			pRenderData,
			pRenderSubmodel,
			pRenderMesh,
			pmesh,
			ptexturehdr,
			ptexture,
			pskinref,
			flags);
	}
	else
	{
		R_StudioDrawMesh_DrawPass(
			pRenderData,
			pRenderSubmodel,
			pRenderMesh,
			pmesh,
			ptexturehdr,
			ptexture,
			pskinref,
			flags);
	}
}

void R_StudioDrawSubmodel(
	CStudioModelRenderData* pRenderData,
	CStudioModelRenderSubModel* VBOSubmodel,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref)
{
	for (size_t i = 0; i < VBOSubmodel->vMesh.size(); i++)
	{
		auto pRenderMesh = &VBOSubmodel->vMesh[i];

		auto pmesh = (mstudiomesh_t*)((byte*)(*pstudiohdr) + (*psubmodel)->meshindex) + pRenderMesh->iMeshIndex;

		R_StudioDrawMesh(pRenderData, VBOSubmodel, pRenderMesh, pmesh, ptexturehdr, ptexture, pskinref);
	}
}

void R_StudioDrawRenderData(CStudioModelRenderData* pRenderData)
{
	auto submodel_byteoffset = (byte*)(*psubmodel) - (byte*)(*pstudiohdr);
	auto found_VBOSubmodel = pRenderData->mSubmodels.find(submodel_byteoffset);

	if (found_VBOSubmodel == pRenderData->mSubmodels.end()) {
		Sys_Error("R_StudioDrawRenderData: invalid submodel!\n submodel_byteoffset = %d\n  psubmodel->name = %s\n  pstudiohdr->name = %s", submodel_byteoffset, (*psubmodel)->name, (*pstudiohdr)->name);
		return;
	}

	auto pRenderSubmodel = found_VBOSubmodel->second;

	auto ptexturehdr = R_StudioGetTextureHeader(pRenderData);

	mstudiotexture_t* ptexture = NULL;

	short* pskinref = NULL;

	if (ptexturehdr)
	{
		ptexture = (mstudiotexture_t*)((byte*)ptexturehdr + ptexturehdr->textureindex);

		pskinref = (short*)((byte*)ptexturehdr + ptexturehdr->skinindex);

		if ((*currententity)->curstate.skin > 0 && (*currententity)->curstate.skin < ptexturehdr->numskinfamilies)
			pskinref += ((*currententity)->curstate.skin * ptexturehdr->numskinref);
	}

	R_StudioDrawSubmodel(pRenderData, pRenderSubmodel, ptexturehdr, ptexture, pskinref);
}

void R_StudioDrawRenderDataEnd()
{
	GL_BindVAO(0);

	g_CurrentRenderData = NULL;
}

//Engine exported StudioAPI

void R_GLStudioDrawPoints(void)
{
	auto pRenderData = R_GetStudioRenderDataFromStudioHeaderFast((*pstudiohdr));

	if (!pRenderData)
	{
		pRenderData = R_GetStudioRenderDataFromStudioHeaderSlow((*pstudiohdr));
	}

	if (!pRenderData)
	{
		Sys_Error("R_GLStudioDrawPoints: no available pRenderData for \"%s\"!", (*pstudiohdr)->name);
		return;
	}

	R_StudioDrawRenderDataBegin(pRenderData);

	R_StudioDrawRenderData(pRenderData);

	R_StudioDrawRenderDataEnd();
}

void R_StudioTransformVector(vec3_t in, vec3_t out)
{
	out[0] = in[0] * (*rotationmatrix)[0][0] + in[1] * (*rotationmatrix)[0][1] + in[2] * (*rotationmatrix)[0][2] + (*rotationmatrix)[0][3];
	out[1] = in[0] * (*rotationmatrix)[1][0] + in[1] * (*rotationmatrix)[1][1] + in[2] * (*rotationmatrix)[1][2] + (*rotationmatrix)[1][3];
	out[2] = in[0] * (*rotationmatrix)[2][0] + in[1] * (*rotationmatrix)[2][1] + in[2] * (*rotationmatrix)[2][2] + (*rotationmatrix)[2][3];
}

void studioapi_RestoreRenderer(void)
{
	glDepthMask(1);

	gPrivateFuncs.studioapi_RestoreRenderer();
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
	float dies[256];

	if ((int)r_studio_legacy_dlight->value != 1)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
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
	if (R_IsRenderingShadowView())
	{
		pfnRenderModel(pthis, dummy);
		return;
	}

	//Process all pending deferred passes on transparent pass
	if (!r_draw_opaque)
	{
		auto pEntityComponentContainer = R_GetEntityComponentContainer((*currententity), false);

		if (pEntityComponentContainer)
		{
			if (pEntityComponentContainer->DeferredStudioPasses.size() > 0)
			{
				for (auto fx : pEntityComponentContainer->DeferredStudioPasses)
				{
					int saved_renderfx = (*currententity)->curstate.renderfx;
					int saved_renderamt = (*currententity)->curstate.renderamt;

					(*currententity)->curstate.renderfx = fx;

					pfnRenderModel(pthis, dummy);

					(*currententity)->curstate.renderfx = saved_renderfx;
					(*currententity)->curstate.renderamt = saved_renderamt;
				}

				pEntityComponentContainer->DeferredStudioPasses.clear();
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
		auto pEntityComponentContainer = R_GetEntityComponentContainer((*currententity), true);

		if (pEntityComponentContainer)
		{
			pEntityComponentContainer->DeferredStudioPasses.emplace_back(kRenderFxDrawAlphaMeshes);
		}

		r_draw_deferredtrans = true;
	}

	if (r_draw_opaque && r_draw_hasadditive)
	{
		auto pEntityComponentContainer = R_GetEntityComponentContainer((*currententity), true);

		if (pEntityComponentContainer)
		{
			pEntityComponentContainer->DeferredStudioPasses.emplace_back(kRenderFxDrawAdditiveMeshes);
		}

		r_draw_deferredtrans = true;
	}

	//Hair pass, write into s_BackBufferFBO2
	if (R_StudioHasHairShadow())
	{
		GL_BindFrameBuffer(&s_BackBufferFBO2);

		glDrawBuffer(GL_NONE);

		GL_ClearDepthStencil(1, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

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

			auto pEntityComponentContainer = R_GetEntityComponentContainer((*currententity), true);

			if (pEntityComponentContainer)
			{
				pEntityComponentContainer->DeferredStudioPasses.emplace_back(kRenderFxDrawGlowShell);
			}

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

	if (r_draw_hashair)
	{
		GL_BindFrameBuffer(&s_BackBufferFBO2);

		GL_ClearStencil(STENCIL_MASK_HAS_SHADOW);

		if (r_draw_gbuffer)
			GL_BindFrameBuffer(&s_GBufferFBO);
		else
			GL_BindFrameBuffer(GL_GetCurrentSceneFBO());
	}

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

void CopyPlayerInfoStudioRenderData(const player_info_t* src, player_info_t* dst)
{
	dst->renderframe = src->renderframe;
	dst->gaitsequence = src->gaitsequence;
	dst->gaitframe = src->gaitframe;
	dst->gaityaw = src->gaityaw;
	VectorCopy(src->prevgaitorigin, dst->prevgaitorigin);
}

/*

	Purpose : StudioDrawPlayer hook handler

*/

template<typename CallType>
__forceinline int StudioDrawPlayer_Template(CallType pfnDrawPlayer, int flags, struct entity_state_s* pplayer, void* pthis = nullptr, int dummy = 0)
{
	int playerindex = pplayer->number - 1;

	int result = 0;

	if (playerindex >= 0 && playerindex < gEngfuncs.GetMaxClients() && playerindex < MAX_CLIENTS)
	{
		auto pPlayerInfo = IEngineStudio.PlayerInfo(playerindex);

		if (g_PlayerInfoStorage[playerindex].Filled)
		{
			CopyPlayerInfoStudioRenderData(&g_PlayerInfoStorage[playerindex].SavedPlayerInfo, pPlayerInfo);
		}

		CopyPlayerInfoStudioRenderData(pPlayerInfo, &g_PlayerInfoStorage[playerindex].SavedPlayerInfo);
		g_PlayerInfoStorage[playerindex].Filled = true;

		if (R_IsRenderingLowerBody())
		{
			//Sniber NMSL
			vec3_t vecSavedModelOrigin = { 0 };
			float flSavedModelScale = 0;
			int iSavedViewEntityIndex_SCClient = 0;

			float* pScale = &(*currententity)->curstate.scale;
			float* pModelPos = (*currententity)->origin;
			VectorCopy(pModelPos, vecSavedModelOrigin);
			
			//Don't emit event (muzzleflash or sound)
			flags &= ~STUDIO_EVENTS;

			flSavedModelScale = (*pScale);

			auto model = IEngineStudio.SetupPlayerModel(playerindex);

			if (g_bIsCounterStrike)
			{
				//Counter-Strike redirects playermodel in a pretty tricky way
				int modelindex = 0;
				model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
			}

			if (model)
			{
				auto pRenderData = R_GetStudioRenderDataFromModel(model);

				if (pRenderData)
				{
					vec3_t viewangles, forward, right, up;
					gEngfuncs.GetViewAngles(viewangles);
					viewangles[0] = 0;
					AngleVectors(viewangles, forward, right, up);

					float model_scale = pRenderData->LowerBodyControl.model_scale.GetValue();

					if (model_scale > 0)
					{
						(*pScale) = model_scale;
					}

					vec3_t model_origin = { 0 };

					if (pRenderData->LowerBodyControl.model_origin.GetValues(model_origin))
					{
						VectorMA(pModelPos, model_origin[0], forward, pModelPos);
						VectorMA(pModelPos, model_origin[1], right, pModelPos);
						VectorMA(pModelPos, model_origin[2], up, pModelPos);
					}
				}
			}

			if (g_ViewEntityIndex_SCClient)
			{
				iSavedViewEntityIndex_SCClient = (*g_ViewEntityIndex_SCClient);
				(*g_ViewEntityIndex_SCClient) = 0;
			}

			result = pfnDrawPlayer(pthis, dummy, flags, pplayer);

			if (g_ViewEntityIndex_SCClient)
			{
				(*g_ViewEntityIndex_SCClient) = iSavedViewEntityIndex_SCClient;
			}

			VectorCopy(vecSavedModelOrigin, pModelPos);
			(*pScale) = flSavedModelScale;
		}
		else
		{
			result = pfnDrawPlayer(pthis, dummy, flags, pplayer);
		}
#if 0
		if (!g_PlayerInfoStorage[playerindex].Filled)
		{
			CopyPlayerInfoStudioRenderData(pPlayerInfo, &g_PlayerInfoStorage[playerindex].ChangedPlayerInfo);

			g_PlayerInfoStorage[playerindex].FilledWithRenderFlags = flags;
			g_PlayerInfoStorage[playerindex].Filled = true;
		}
		else
		{
			if ((g_PlayerInfoStorage[playerindex].FilledWithRenderFlags & STUDIO_RENDER) && !(flags & STUDIO_RENDER))
			{

			}
			else
			{
				CopyPlayerInfoStudioRenderData(pPlayerInfo, &g_PlayerInfoStorage[playerindex].ChangedPlayerInfo);

				g_PlayerInfoStorage[playerindex].FilledWithRenderFlags = flags;
				g_PlayerInfoStorage[playerindex].Filled = true;
			}
		}

#endif
	}
	else
	{
		result = pfnDrawPlayer(pthis, dummy, flags, pplayer);
	}

	return result;
}

__forceinline int R_StudioDrawPlayer_originalcall_wrapper(void* pthis, int dummy, int flags, struct entity_state_s* pplayer)
{
	return gPrivateFuncs.R_StudioDrawPlayer(flags, pplayer);
}

int R_StudioDrawPlayer(int flags, struct entity_state_s* pplayer)
{
	return StudioDrawPlayer_Template(R_StudioDrawPlayer_originalcall_wrapper, flags, pplayer);
}

int __fastcall GameStudioRenderer_StudioDrawPlayer(void* pthis, int dummy, int flags, struct entity_state_s* pplayer)
{
	return StudioDrawPlayer_Template(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer, flags, pplayer, pthis, dummy);
}

template<typename CallType>
__forceinline void StudioSetupBones_Template(CallType pfnSetupBones, void* pthis = nullptr, int dummy = 0)
{
	if (!(*pstudiohdr))
	{
		pfnSetupBones(pthis, dummy);
		return;
	}

	//Never cache bones for viewmodel !
	if (!r_studio_bone_caches->value || R_IsRenderingViewModel() || R_IsRenderingLowerBody())
	{
		pfnSetupBones(pthis, dummy);
		return;
	}

	CStudioBoneCacheHandle handle(
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
}

template<typename CallType>
__forceinline void StudioSaveBones_Template(CallType pfnSaveBones, void* pthis = nullptr, int dummy = 0)
{
	if (!(*pstudiohdr))
	{
		pfnSaveBones(pthis, dummy);
		return;
	}

	//Never cache bones for viewmodel !
	if (!r_studio_bone_caches->value || R_IsRenderingViewModel() || R_IsRenderingLowerBody())
	{
		pfnSaveBones(pthis, dummy);
		return;
	}

	pfnSaveBones(pthis, dummy);

	auto cache = R_AllocStudioBoneCache();

	if (cache)
	{
		CStudioBoneCacheHandle handle(
			(*pstudiohdr)->soundtable,
			(*currententity)->curstate.sequence,
			(*currententity)->curstate.gaitsequence,
			(*currententity)->curstate.frame,
			(*currententity)->origin,
			(*currententity)->angles);

		memcpy(cache->m_bonetransform, (*pbonetransform), sizeof(cache->m_bonetransform));
		memcpy(cache->m_lighttransform, (*plighttransform), sizeof(cache->m_lighttransform));

		g_StudioBoneCacheManager[handle] = cache;
	}
}

template<typename CallType>
void __fastcall StudioMergeBones_Template(CallType pfnMergeBones, void* pthis, int dummy, model_t* pSubModel)
{
	if (!(*pstudiohdr))
	{
		pfnMergeBones(pthis, dummy, pSubModel);
		return;
	}

	//Never cache bones for viewmodel !
	if (!r_studio_bone_caches->value || R_IsRenderingViewModel() || R_IsRenderingLowerBody())
	{
		pfnMergeBones(pthis, dummy, pSubModel);
		return;
	}

	CStudioBoneCacheHandle handle(
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
}

/*
	Purpose : wrapper to call engine StudioSaveBones
*/

__forceinline void R_StudioSaveBones_originalcall_wrapper(void* pthis, int dummy)
{
	return gPrivateFuncs.R_StudioSaveBones();
}

/*
	Purpose : Engine StudioSaveBones hook handler
*/

void R_StudioSaveBones(void)
{
	StudioSaveBones_Template(R_StudioSaveBones_originalcall_wrapper);
}

/*
	Purpose : ClientDLL StudioSaveBones hook handler
*/

void __fastcall GameStudioRenderer_StudioSaveBones(void* pthis, int dummy)
{
	StudioSaveBones_Template(gPrivateFuncs.GameStudioRenderer_StudioSaveBones, pthis, dummy);
}

/*
	Purpose : wrapper to call engine StudioSetupBones
*/

__forceinline void R_StudioMergeBones_originalcall_wrapper(void* pthis, int dummy, model_t* pSubModel)
{
	return gPrivateFuncs.R_StudioMergeBones(pSubModel);
}

/*
	Purpose : Engine StudioSaveBones hook handler
*/

void R_StudioMergeBones(model_t* pSubModel)
{
	StudioMergeBones_Template(R_StudioMergeBones_originalcall_wrapper, 0, 0, pSubModel);
}

/*
	Purpose : ClientDLL StudioMergeBones hook handler
*/

void __fastcall GameStudioRenderer_StudioMergeBones(void* pthis, int dummy, model_t* pSubModel)
{
	StudioMergeBones_Template(gPrivateFuncs.GameStudioRenderer_StudioMergeBones, pthis, dummy, pSubModel);
}

/*
	Purpose : wrapper to call engine StudioSetupBones
*/

__forceinline void R_StudioSetupBones_originalcall_wrapper(void* pthis, int dummy)
{
	return gPrivateFuncs.R_StudioSetupBones();
}

/*
	Purpose : Engine StudioSetupBones hook handler
*/

void R_StudioSetupBones(void)
{
	return StudioSetupBones_Template(R_StudioSetupBones_originalcall_wrapper);
}

/*
	Purpose : ClientDLL StudioSetupBones hook handler
*/

void __fastcall GameStudioRenderer_StudioSetupBones(void* pthis, int dummy)
{
	StudioSetupBones_Template(gPrivateFuncs.GameStudioRenderer_StudioSetupBones, pthis, dummy);
}

CStudioModelRenderMaterial* R_StudioGetMaterialFromTextureId(const CStudioModelRenderData *pRenderData, int gltexturenum)
{
	if (gltexturenum > 0)
	{
		const auto& itor = pRenderData->mStudioMaterials.find(gltexturenum);

		if (itor != pRenderData->mStudioMaterials.end())
		{
			return itor->second;
		}
	}

	return nullptr;
}

CStudioModelRenderMaterial* R_StudioCreateMaterial(CStudioModelRenderData* pRenderData, mstudiotexture_t* texture)
{
	//TODO: hash the texturename ?

	auto pStudioMaterial = R_StudioGetMaterialFromTextureId(pRenderData, texture->index);

	if (pStudioMaterial)
		return pStudioMaterial;

	pStudioMaterial = new CStudioModelRenderMaterial();

	pRenderData->mStudioMaterials[texture->index] = pStudioMaterial;

	return pStudioMaterial;
}

void R_StudioLoadExternalFile_TextureLoad(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData, mstudiotexture_t* ptexture, const char* textureValue, const char* scaleValue, int StudioTextureType)
{
	if (textureValue && textureValue[0])
	{
		auto pStudioMaterial = R_StudioCreateMaterial(pRenderData, ptexture);

		if (pStudioMaterial)
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
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile_TextureLoad: Failed to load external texture \"%s\" for basetexture \"%s\".\n", textureValue, ptexture->name);
				return;
			}

			pStudioMaterial->textures[StudioTextureType - 1].gltexturenum = loadResult.gltexturenum;
			pStudioMaterial->textures[StudioTextureType - 1].numframes = loadResult.numframes;
			pStudioMaterial->textures[StudioTextureType - 1].framerate = GetFrameRateFromFrameDuration(loadResult.frameduration);
			pStudioMaterial->textures[StudioTextureType - 1].width = loadResult.width;
			pStudioMaterial->textures[StudioTextureType - 1].height = loadResult.height;
			pStudioMaterial->textures[StudioTextureType - 1].scaleX = 0;
			pStudioMaterial->textures[StudioTextureType - 1].scaleY = 0;

			if (scaleValue && scaleValue[0])
			{
				float scales[2] = { 0 };

				if (2 == sscanf(scaleValue, "%f %f", &scales[0], &scales[1]))
				{
					if (scales[0] > 0 || scales[0] < 0)
						pStudioMaterial->textures[StudioTextureType - 1].scaleX = scales[0];

					if (scales[1] > 0 || scales[1] < 0)
						pStudioMaterial->textures[StudioTextureType - 1].scaleY = scales[1];
				}
				else if (1 == sscanf(scaleValue, "%f", &scales[0]))
				{
					if (scales[0] > 0 || scales[0] < 0)
					{
						pStudioMaterial->textures[StudioTextureType - 1].scaleX = scales[0];
						pStudioMaterial->textures[StudioTextureType - 1].scaleY = scales[0];
					}
				}
			}
		}
		else
		{
			gEngfuncs.Con_DPrintf("R_StudioLoadExternalFile_TextureLoad: Failed to create material when loading external texture \"%s\" for basetexture \"%s\".\n", textureValue, ptexture->name);
		}
	}
}

void R_StudioLoadExternalFile_TextureFlags(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData, mstudiotexture_t* ptexture, const char* value)
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

void R_StudioLoadExternalFile_TextureFlagsArray(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData, mstudiotexture_t* ptexture, const std::vector<const char *> flagsArray)
{
	for (auto flags : flagsArray)
	{
		R_StudioLoadExternalFile_TextureFlags(ent, studiohdr, pRenderData, ptexture, flags);
	}
}

void R_StudioLoadExternalFile_Texture(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData)
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
			R_StudioLoadExternalFile_TextureFlagsArray(ent, studiohdr, pRenderData, ptexture, flagsArray);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, pRenderData, ptexture, replacetexture, replacescale, STUDIO_REPLACE_TEXTURE);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, pRenderData, ptexture, normaltexture, normalscale, STUDIO_NORMAL_TEXTURE);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, pRenderData, ptexture, parallaxtexture, parallaxscale, STUDIO_PARALLAX_TEXTURE);
			R_StudioLoadExternalFile_TextureLoad(ent, studiohdr, pRenderData, ptexture, speculartexture, specularscale, STUDIO_SPECULAR_TEXTURE);
		}
	}
}

void R_StudioLoadExternalFile_Efx(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData)
{
	auto flags_string = ValueForKey(ent, "flags");

#define REGISTER_EFX_FLAGS_KEY_VALUE(name) if (flags_string && !strcmp(flags_string, #name))\
	{\
		studiohdr->flags |= name; \
	}\
	if (flags_string && !strcmp(flags_string, "-" #name))\
	{\
		studiohdr->flags &= ~name; \
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

void R_StudioLoadExternalFile_BoneInternal(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData, mstudiobone_t* pbone)
{
	auto flags_string = ValueForKey(ent, "flags");

#define REGISTER_BONE_FLAGS_KEY_VALUE(name) if (flags_string && !strcmp(flags_string, #name))\
	{\
		pbone->flags |= name; \
	}\
	if (flags_string && !strcmp(flags_string, "-" #name))\
	{\
		pbone->flags &= ~name; \
	}

	REGISTER_BONE_FLAGS_KEY_VALUE(STUDIO_BF_LOWERBODY);
}

void R_StudioLoadExternalFile_Bone(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData)
{
	auto name_string = ValueForKey(ent, "name");
	if (name_string && name_string[0])
	{
		auto pbones = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

		for (int i = 0; i < studiohdr->numbones; ++i)
		{
			if (!strcmp(pbones[i].name, name_string))
			{
				R_StudioLoadExternalFile_BoneInternal(ent, studiohdr, pRenderData, &pbones[i]);
				return;
			}
		}

		gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to find \"%s\" in entity \"studio_bone\"\n", name_string);
		return;
	}

	auto index_string = ValueForKey(ent, "index");
	if (index_string && index_string[0])
	{
		int boneindex = atoi(index_string);

		if (boneindex >= 0 && boneindex < studiohdr->numbones)
		{
			auto pbones = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

			R_StudioLoadExternalFile_BoneInternal(ent, studiohdr, pRenderData, &pbones[boneindex]);
		}

		gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to find bone index \"%s\" in entity \"studio_bone\"\n", index_string);
		return;
	}
}

void R_StudioLoadExternalFile_Celshade(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData)
{
#define REGISTER_CELSHADE_KEY_VALUE(name, parser) if (1)\
	{\
		auto name = ValueForKey(ent, #name);\
		if (name && name[0])\
		{\
			vec4_t values = { 0 };\
			if (parser(name, values))\
			{\
				pRenderData->CelshadeControl.name.SetOverrideValues(values);\
				pRenderData->CelshadeControl.name.SetOverride(true);\
			}\
			else\
			{\
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"" #name  "\" in entity \"studio_celshade_control\"\n");\
			}\
		}\
	}

	REGISTER_CELSHADE_KEY_VALUE(base_specular, UTIL_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(celshade_specular, UTIL_ParseStringAsVector4);
	REGISTER_CELSHADE_KEY_VALUE(celshade_midpoint, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(celshade_softness, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(celshade_shadow_color, UTIL_ParseStringAsColor3);
	REGISTER_CELSHADE_KEY_VALUE(celshade_head_offset, UTIL_ParseStringAsVector3);
	REGISTER_CELSHADE_KEY_VALUE(celshade_lightdir_adjust, UTIL_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(outline_size, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(outline_dark, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_power, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_smooth, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_smooth2, UTIL_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(rimlight_color, UTIL_ParseStringAsColor3);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_power, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_smooth, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_smooth2, UTIL_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(rimdark_color, UTIL_ParseStringAsColor3);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_exp, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_intensity, UTIL_ParseStringAsVector3);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_noise, UTIL_ParseStringAsVector4);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_exp2, UTIL_ParseStringAsVector1);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_intensity2, UTIL_ParseStringAsVector3);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_noise2, UTIL_ParseStringAsVector4);
	REGISTER_CELSHADE_KEY_VALUE(hair_specular_smooth, UTIL_ParseStringAsVector2);
	REGISTER_CELSHADE_KEY_VALUE(hair_shadow_offset, UTIL_ParseStringAsVector2);

#undef REGISTER_CELSHADE_KEY_VALUE
}

void R_StudioLoadExternalFile_LowerBody(bspentity_t* ent, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData)
{
#define REGISTER_LOWERBODY_KEY_VALUE(name, parser) if (1)\
	{\
		auto name = ValueForKey(ent, #name);\
		if (name && name[0])\
		{\
			vec4_t values = { 0 };\
			if (parser(name, values))\
			{\
				pRenderData->LowerBodyControl.name.SetOverrideValues(values);\
				pRenderData->LowerBodyControl.name.SetOverride(true);\
			}\
			else\
			{\
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"" #name  "\" in entity \"studio_lowerbody_control\"\n");\
			}\
		}\
	}

	REGISTER_LOWERBODY_KEY_VALUE(model_origin, UTIL_ParseStringAsVector3);
	REGISTER_LOWERBODY_KEY_VALUE(model_scale, UTIL_ParseStringAsVector1);
}

void R_StudioLoadExternalFile(model_t* mod, studiohdr_t* studiohdr, CStudioModelRenderData* pRenderData)
{
	gEngfuncs.Con_DPrintf("R_StudioLoadExternalFile: for \"%s\".\n", mod->name);

	std::string fullPath = mod->name;

	RemoveFileExtension(fullPath);

	fullPath += "_external.txt";

	auto pFile = (const char*)gEngfuncs.COM_LoadFile(fullPath.c_str(), 5, NULL);

	if (pFile)
	{
		std::vector<bspentity_t*> vEntities;

		R_ParseBSPEntities(pFile, vEntities);

		for (auto ent : vEntities)
		{
			auto classname = ent->classname;

			if (!classname)
				continue;

			if (!strcmp(classname, "studio_texture"))
			{
				R_StudioLoadExternalFile_Texture(ent, studiohdr, pRenderData);
			}
			else if (!strcmp(classname, "studio_efx"))
			{
				R_StudioLoadExternalFile_Efx(ent, studiohdr, pRenderData);
			}
			else if (!strcmp(classname, "studio_celshade_control"))
			{
				R_StudioLoadExternalFile_Celshade(ent, studiohdr, pRenderData);
			}
			else if (!strcmp(classname, "studio_lowerbody_control"))
			{
				R_StudioLoadExternalFile_LowerBody(ent, studiohdr, pRenderData);
			}
			else if (!strcmp(classname, "studio_bone"))
			{
				R_StudioLoadExternalFile_Bone(ent, studiohdr, pRenderData);
			}
		}

		for (auto ent : vEntities)
		{
			delete ent;
		}

		gEngfuncs.COM_FreeFile((void*)pFile);
	}
}

void R_StudioStartFrame(void)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		auto playerindex = i;

		g_PlayerInfoStorage[playerindex].Filled = false;
	}

	R_StudioClearAllBoneCaches();
}

void R_StudioEndFrame(void)
{

}

void UpdatePlayerPitch(cl_entity_t *ent, float a2)
{
	//Sniber NMSL
	return;
}