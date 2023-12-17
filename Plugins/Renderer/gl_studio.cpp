#include "gl_local.h"
#include "triangleapi.h"
#include <sstream>
#include <algorithm>

#include "mathlib.h"

class studio_bone_handle
{
public:
	studio_bone_handle(int vboindex, int sequence, int gaitsequence, float frame, vec3_t origin, vec3_t angles)
	{
		m_vboindex = vboindex;
		m_sequence = sequence;
		m_gaitsequence = gaitsequence;
		m_frame = frame;
		VectorCopy(origin, m_origin);
		VectorCopy(angles, m_angles);
	}

	bool operator == (const studio_bone_handle& a) const
	{
		return
			m_vboindex == a.m_vboindex &&
			m_sequence == a.m_sequence &&
			m_gaitsequence == a.m_gaitsequence &&
			m_frame == a.m_frame &&
			VectorCompare(m_origin, a.m_origin) &&
			VectorCompare(m_angles, a.m_angles);
	}

	int m_vboindex;
	int m_sequence;
	int m_gaitsequence;
	float m_frame;
	vec3_t m_origin;
	vec3_t m_angles;
};

class studio_bone_hasher
{
public:
	std::size_t operator()(const studio_bone_handle& key) const
	{
		auto base = (std::size_t)(key.m_vboindex << 24);

		base += ((std::size_t)key.m_sequence << 16);
		base += ((std::size_t)key.m_gaitsequence << 8);
		base += (std::size_t)(key.m_frame * 128.0);
		base += (std::size_t)(key.m_origin[0] * 128.0);
		base += (std::size_t)(key.m_origin[1] * 128.0);
		base += (std::size_t)(key.m_origin[2] * 128.0);
		base += (std::size_t)(key.m_angles[0] * 128.0);
		base += (std::size_t)(key.m_angles[1] * 128.0);
		base += (std::size_t)(key.m_angles[2] * 128.0);

		return base;
	}
};

class studio_bone_cache
{
public:
	studio_bone_cache()
	{
		memset(m_bonetransform, 0, sizeof(m_bonetransform));
		memset(m_lighttransform, 0, sizeof(m_lighttransform));
		m_next = NULL;
	}
	studio_bone_cache(float* _bonetransform, float* _lighttransform)
	{
		memcpy(m_bonetransform, _bonetransform, sizeof(m_bonetransform));
		memcpy(m_lighttransform, _lighttransform, sizeof(m_lighttransform));
		m_next = NULL;
	}

	float m_bonetransform[MAXSTUDIOBONES][3][4];
	float m_lighttransform[MAXSTUDIOBONES][3][4];
	studio_bone_cache* m_next;
};

std::unordered_map<studio_bone_handle, studio_bone_cache*, studio_bone_hasher> g_StudioBoneCacheManager;

std::unordered_map<program_state_t, studio_program_t> g_StudioProgramTable;

std::vector<studio_vbo_t*> g_StudioVBOCache;

#define MAX_STUDIO_BONE_CACHES 1024

static studio_bone_cache g_StudioBoneCaches[MAX_STUDIO_BONE_CACHES];

static studio_bone_cache* g_pStudioBoneFreeCaches = NULL;

static studio_vbo_t* g_CurrentVBOCache = NULL;

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
int(*g_NormalIndex)[MAXSTUDIOVERTS] = NULL;
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
float* r_plightvec = NULL;
float* r_colormix = NULL;
void* tmp_palette = NULL;
int* r_smodels_total = NULL;
int* r_amodels_drawn = NULL;
dlight_t* (*locallight)[3] = NULL;
int* numlight = NULL;
int* r_topcolor = NULL;
int* r_bottomcolor = NULL;
player_model_t(*DM_PlayerState)[MAX_CLIENTS] = NULL;

//Stats

int r_studio_drawcall = 0;
int r_studio_polys = 0;

//Cvars

cvar_t* r_studio_celshade = NULL;
cvar_t* r_studio_celshade_midpoint = NULL;
cvar_t* r_studio_celshade_softness = NULL;
cvar_t* r_studio_celshade_shadow_color = NULL;

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

void R_StudioBoneCaches_StartFrame()
{
	for (int i = 0; i < MAX_STUDIO_BONE_CACHES - 1; i++)
		g_StudioBoneCaches[i].m_next = &g_StudioBoneCaches[i + 1];

	g_StudioBoneCaches[MAX_STUDIO_BONE_CACHES - 1].m_next = NULL;

	g_pStudioBoneFreeCaches = &g_StudioBoneCaches[0];

	g_StudioBoneCacheManager.clear();
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
					VBOMesh.mesh = pmesh;

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
					VBOMesh.mesh = pmesh;

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

studio_vbo_t* R_PrepareStudioVBO(studiohdr_t* studiohdr)
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

	VBOData = new studio_vbo_t;
	VBOData->studiohdr = studiohdr;

	g_StudioVBOCache.emplace_back(VBOData);

	studiohdr->soundtable = g_StudioVBOCache.size();

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
				vboSubmodel->submodel = submodel;

				R_PrepareStudioVBOSubmodel(studiohdr, submodel, vVertex, vIndices, vboSubmodel);

				VBOData->vSubmodels.emplace_back(vboSubmodel);

				submodel->groupindex = VBOData->vSubmodels.size();
			}
		}
	}

	VBOData->hVBO = GL_GenBuffer();
	GL_UploadDataToVBO(VBOData->hVBO, vVertex.size() * sizeof(studio_vbo_vertex_t), vVertex.data());

	VBOData->hEBO = GL_GenBuffer();
	GL_UploadDataToEBO(VBOData->hEBO, vIndices.size() * sizeof(GLuint), vIndices.data());

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

	VBOData->celshade_control.celshade_midpoint.Init(r_studio_celshade_midpoint, 1, 0);
	VBOData->celshade_control.celshade_softness.Init(r_studio_celshade_softness, 1, 0);
	VBOData->celshade_control.celshade_shadow_color.Init(r_studio_celshade_shadow_color, 3, ConVar_Color255);

	VBOData->celshade_control.outline_size.Init(r_studio_outline_size, 1, 0);
	VBOData->celshade_control.outline_dark.Init(r_studio_outline_dark, 1, 0);

	VBOData->celshade_control.rimlight_power.Init(r_studio_rimlight_power, 1, 0);
	VBOData->celshade_control.rimlight_smooth.Init(r_studio_rimlight_smooth, 1, 0);
	VBOData->celshade_control.rimlight_smooth2.Init(r_studio_rimlight_smooth2, 2, 0);
	VBOData->celshade_control.rimlight_color.Init(r_studio_rimlight_color, 3, ConVar_Color255);

	VBOData->celshade_control.rimdark_power.Init(r_studio_rimdark_power, 1, 0);
	VBOData->celshade_control.rimdark_smooth.Init(r_studio_rimdark_smooth, 1, 0);
	VBOData->celshade_control.rimdark_smooth2.Init(r_studio_rimdark_smooth2, 2, 0);
	VBOData->celshade_control.rimdark_color.Init(r_studio_rimdark_color, 3, ConVar_Color255);

	VBOData->celshade_control.hair_specular_exp.Init(r_studio_hair_specular_exp, 1, 0);
	VBOData->celshade_control.hair_specular_noise.Init(r_studio_hair_specular_noise, 4, 0);
	VBOData->celshade_control.hair_specular_intensity.Init(r_studio_hair_specular_intensity, 3, 0);
	VBOData->celshade_control.hair_specular_exp2.Init(r_studio_hair_specular_exp2, 1, 0);
	VBOData->celshade_control.hair_specular_noise2.Init(r_studio_hair_specular_noise2, 4, 0);
	VBOData->celshade_control.hair_specular_intensity2.Init(r_studio_hair_specular_intensity2, 3, 0);
	VBOData->celshade_control.hair_specular_smooth.Init(r_studio_hair_specular_smooth, 2, 0);
	VBOData->celshade_control.hair_shadow_offset.Init(r_studio_hair_shadow_offset, 2, 0);

	return VBOData;
}

void R_StudioReloadVBOCache(void)
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

			for (auto mat : VBOData->vMaterials)
			{
				for (int j = 0; j < STUDIO_MAX_TEXTURE; ++j)
				{
					if (mat->textures[j].gltexturenum)
					{
						GL_UnloadTextureEx(mat->textures[j].gltexturenum);
						mat->textures[j].gltexturenum = 0;
					}
				}
				delete mat;
			}

			delete VBOData;

			g_StudioVBOCache[i] = NULL;
		}
	}

	for (int i = 0; i < *mod_numknown; ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_studio && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				if (!strcmp(mod->name, "models/player.mdl"))
					r_playermodel = mod;

				auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
				if (studiohdr)
				{
					auto VBOData = R_PrepareStudioVBO(studiohdr);

					R_StudioLoadExternalFile(mod, studiohdr, VBOData);
				}
			}
		}
	}
}

void R_UseStudioProgram(program_state_t state, studio_program_t* progOutput)
{
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

		if (state & STUDIO_INVERT_NORMAL_ENABLED)
			defs << "#define INVERT_NORMAL_ENABLED\n";

		if (glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define NV_BINDLESS_ENABLED\n";

		else if (glewIsSupported("GL_ARB_gpu_shader_int64"))
			defs << "#define INT64_BINDLESS_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\studio_shader.vsh", "renderer\\shader\\studio_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, r_celshade_midpoint, "r_celshade_midpoint");
			SHADER_UNIFORM(prog, r_celshade_softness, "r_celshade_softness");
			SHADER_UNIFORM(prog, r_celshade_shadow_color, "r_celshade_shadow_color");
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
			SHADER_UNIFORM(prog, entityPos, "entityPos");
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

		if (prog.entityPos != -1)
		{
			glUniform3f(prog.entityPos, (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3]);
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
			if (g_CurrentVBOCache)
			{
				vec3_t color = { 0 };
				g_CurrentVBOCache->celshade_control.celshade_shadow_color.GetValues(color);
				glUniform3f(prog.r_celshade_shadow_color, color[0], color[1], color[2]);
			}
			else
			{
				vec3_t color = { 0 };
				R_ParseCvarAsColor3(r_studio_celshade_shadow_color, color);
				glUniform3f(prog.r_celshade_shadow_color, color[0], color[1], color[2]);
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
			if (g_CurrentVBOCache)
			{
				vec2_t values = { 0 };
				g_CurrentVBOCache->celshade_control.rimlight_smooth2.GetValues(values);
				glUniform2f(prog.r_rimlight_smooth2, values[0], values[1]);
			}
			else
			{
				vec2_t values = { 0 };
				R_ParseCvarAsVector2(r_studio_rimlight_color, values);
				glUniform2f(prog.r_rimlight_smooth2, values[0], values[1]);
			}
		}

		if (prog.r_rimlight_color != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec3_t color = { 0 };
				g_CurrentVBOCache->celshade_control.rimlight_color.GetValues(color);
				glUniform3f(prog.r_rimlight_color, color[0], color[1], color[2]);
			}
			else
			{
				vec3_t color = { 0 };
				R_ParseCvarAsColor3(r_studio_rimlight_color, color);
				glUniform3f(prog.r_rimlight_color, color[0], color[1], color[2]);
			}
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
			if (g_CurrentVBOCache)
			{
				vec2_t values = { 0 };
				g_CurrentVBOCache->celshade_control.rimdark_smooth2.GetValues(values);
				glUniform2f(prog.r_rimdark_smooth2, values[0], values[1]);
			}
			else
			{
				vec2_t values = { 0 };
				R_ParseCvarAsVector2(r_studio_rimdark_color, values);
				glUniform2f(prog.r_rimdark_smooth2, values[0], values[1]);
			}
		}

		if (prog.r_rimdark_color != -1)
		{
			if (g_CurrentVBOCache)
			{
				vec3_t color = { 0 };
				g_CurrentVBOCache->celshade_control.rimdark_color.GetValues(color);
				glUniform3f(prog.r_rimdark_color, color[0], color[1], color[2]);
			}
			else
			{
				vec3_t color = { 0 };
				R_ParseCvarAsColor3(r_studio_rimdark_color, color);
				glUniform3f(prog.r_rimdark_color, color[0], color[1], color[2]);
			}
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
{ STUDIO_INVERT_NORMAL_ENABLED			,"STUDIO_INVERT_NORMAL_ENABLED"		},

{ STUDIO_NF_FLATSHADE					,"STUDIO_NF_FLATSHADE"		},
{ STUDIO_NF_MASKED						,"STUDIO_NF_MASKED"			},
{ STUDIO_NF_CHROME						,"STUDIO_NF_CHROME"			},
{ STUDIO_NF_FULLBRIGHT					,"STUDIO_NF_FULLBRIGHT"		},
{ STUDIO_NF_ALPHA						,"STUDIO_NF_ALPHA"			},
{ STUDIO_NF_ADDITIVE					,"STUDIO_NF_ADDITIVE"		},
{ STUDIO_NF_CELSHADE					,"STUDIO_NF_CELSHADE"		},
{ STUDIO_NF_CELSHADE_FACE				,"STUDIO_NF_CELSHADE_FACE"	},
{ STUDIO_NF_CELSHADE_HAIR				,"STUDIO_NF_CELSHADE_HAIR"	},
{ STUDIO_NF_CELSHADE_HAIR_H				,"STUDIO_NF_CELSHADE_HAIR_H"	},
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

	R_StudioBoneCaches_StartFrame();
}

void R_InitStudio(void)
{
	r_studio_celshade = gEngfuncs.pfnRegisterVariable("r_studio_celshade", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_midpoint = gEngfuncs.pfnRegisterVariable("r_studio_celshade_midpoint", "-0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_softness = gEngfuncs.pfnRegisterVariable("r_studio_celshade_softness", "0.05", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_shadow_color = gEngfuncs.pfnRegisterVariable("r_studio_celshade_shadow_color", "160 150 150", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

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

	r_studio_legacy_dlight = gEngfuncs.pfnRegisterVariable("r_studio_legacy_dlight", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_legacy_elight = gEngfuncs.pfnRegisterVariable("r_studio_legacy_elight", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_legacy_elight_radius_scale = gEngfuncs.pfnRegisterVariable("r_studio_legacy_elight_radius_scale", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_bone_caches = gEngfuncs.pfnRegisterVariable("r_studio_bone_caches", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_external_textures = gEngfuncs.pfnRegisterVariable("r_studio_external_textures", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

inline qboolean R_IsFlippedViewModel(void)
{
	if (cl_righthand && cl_righthand->value > 0)
	{
		if (cl_viewent == (*currententity))
			return true;
	}

	return false;
}

/*

studiohdr_t *__cdecl R_LoadTextures(model_t *a1)
{
  studiohdr_t *result; // eax
  model_t *v2; // eax
  int len; // eax
  int mod; // eax
  model_t *mod2; // edi
  studiohdr_t *texhdr; // esi
  int v7; // [esp+0h] [ebp-10Ch]
  char modelname[260]; // [esp+4h] [ebp-108h]

  result = pstudiohdr;
  if ( !pstudiohdr->textureindex )
  {
	v2 = (model_t *)a1->texinfo;
	if ( !v2 || (result = (studiohdr_t *)v2->cache.data) == 0 )
	{
	  Q_strncpy(modelname, a1->name, 258u);
	  modelname[258] = 0;
	  len = strlen(modelname);
	  strcpy((char *)&v7 + len, "T.mdl");
	  mod = Mod_ForName((int)modelname, 1, 0);
	  mod2 = (model_t *)mod;
	  a1->texinfo = (void *)mod;
	  texhdr = *(studiohdr_t **)(mod + 388);
	  Q_strncpy(texhdr->name, modelname, 63u);
	  texhdr->name[63] = 0;
	  result = (studiohdr_t *)mod2->cache.data;
	}
  }
  return result;
}

*/

studiohdr_t* R_StudioGetTextures(model_t* psubm)
{
	if ((*pstudiohdr)->textureindex == 0)
	{
		if (psubm->texinfo)
		{
			auto texmodel = (model_t*)psubm->texinfo;

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

void R_StudioLoadTextures(model_t* psubm, studiohdr_t* studiohdr)
{
	if (studiohdr->textureindex == 0)
	{
		//This is actually 260 instead of 256
		char modelname[260];
		strncpy(modelname, psubm->name, sizeof(modelname) - 2);
		modelname[sizeof(modelname) - 2] = 0;

		strcpy(&modelname[strlen(modelname) - 4], "T.mdl");

		auto texmodel = IEngineStudio.Mod_ForName(modelname, true);
		psubm->texinfo = (mtexinfo_t*)texmodel;
		auto ptexturehdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(texmodel);
		strncpy(ptexturehdr->name, modelname, sizeof(ptexturehdr->name) - 1);
		ptexturehdr->name[sizeof(ptexturehdr->name) - 1] = 0;
	}
}

#if 0
size_t safe_strlen(const char* str, size_t maxChars)
{
	int		count;

	count = 0;
	while (str[count] && count < maxChars)
		count++;

	return count;
}

bool R_IsRemapSkin(const char* texture, int* low, int* mid, int* high)
{
	char	sz[32];
	char* p;
	int		len;
	char	ch;

	if (!strnicmp(texture, "Remap", 5))
	{
		len = safe_strlen(texture, 64);

		if (len == 18 || len == 22)
		{
			ch = texture[5];

			if (len != 18 || ch == 'c' || ch == 'C')
			{
				memset(sz, 0, sizeof(sz));
				strncpy(sz, &texture[7], 3);
				*low = atoi(sz);
				strncpy(sz, &texture[11], 3);
				*mid = atoi(sz);

				if (len == 22)
				{
					strncpy(sz, &texture[15], 3);
					*high = atoi(sz);
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

skin_t* R_StudioGetSkin(int keynum, int index)
{
	skin_t* pskin;

	if (index >= MAX_SKINS)
		index = 0;

	pskin = pDM_RemapSkin[keynum][index];

	if (!pskin || pskin->keynum != keynum)
	{
		pskin = &DM_RemapSkin[r_remapindex][index];
		r_remapindex = (r_remapindex + 1) % 64;
		pDM_RemapSkin[keynum][index] = pskin;
		pskin->keynum = keynum;
		pskin->topcolor = -1;
		pskin->bottomcolor = -1;
	}

	return pskin;
}

unsigned char* R_StudioReloadSkin(model_t* pModel, int index, skin_t* pskin)
{
	int					modelindex;
	cache_user_t* pCache;
	int* pData;
	mstudiotexture_t* ptexture;

	unsigned char* pbase;
	int					size;

	modelindex = pModel - mod_known;

	if (modelindex < 0 || modelindex >= MAX_KNOWN_MODELS)
		return NULL;

	pCache = &model_texture_cache[modelindex][index];

	if (Cache_Check(pCache))
	{
		pData = pCache->data;
	}
	else
	{
		pbase = gEngfuncs.COM_LoadFile(pModel->name, 5, NULL);
		ptexture = (mstudiotexture_t*)((byte*)pbase + ((studiohdr_t*)pbase)->textureindex) + index;
		size = ptexture->height * ptexture->width;
		Cache_Alloc(pCache, size + 768 + 8, pskin->name);
		pData = pCache->data;
		pData[0] = ptexture->width;
		pData[1] = ptexture->height;
		Q_memcpy(&pData[2], pbase + ptexture->index, size + 768);
		gEngfuncs.COM_FreeFile(pbase);
	}

	pskin->index = index;
	pskin->width = pData[0];
	pskin->height = pData[1];
	return (unsigned char*)&pData[2];
}
#endif

void R_StudioSetupVBOMaterial(const studio_vbo_t* VBOData, const studio_vbo_material_t* VBOMaterial, float* width, float* height)
{
	if (r_studio_external_textures->value > 0 && VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].gltexturenum)
	{
		GL_Bind(VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].gltexturenum);

		*width = VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].width * VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleX;
		*height = VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].height * VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleY;
	}
	else if (VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].gltexturenum)
	{
		GL_Bind(VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].gltexturenum);

		*width = VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].width * VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].scaleX;
		*height = VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].height * VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].scaleY;
	}
}

void R_StudioSetupSkinEx(const studio_vbo_t* VBOData, studiohdr_t* ptexturehdr, int index, float* width, float* height)
{
	if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
		return;

#if 0
	if ((*currententity)->index > 0)
	{
		int h = 223;
		int l = 160;
		int m = 191;

		if (!stricmp(ptexture[index].name, "DM_Base.bmp") || R_IsRemapSkin(ptexture[index].name, &l, &m, &h))
		{
			auto pskin = R_StudioGetSkin((*currententity)->index, index);

			if (pskin->model != (*r_model) || pskin->topcolor != (*r_topcolor) && pskin->bottomcolor != (*r_bottomcolor))
			{
				if (pskin->model)
					R_StudioFlushSkins((*currententity)->index);

				auto pData = R_StudioReloadSkin((*r_model), index, pskin);

				if (pData)
				{
					char name[MAX_PATH];
					snprintf(name, sizeof(name), "%s%d", ptexture[index].name, (*currententity)->index);
					name[MAX_PATH - 1] = 0;

					unsigned char* orig_palette = pData + (ptexture[index].height * ptexture[index].width);

					unsigned char tmp_palette[768];
					memcpy(tmp_palette, orig_palette, 768);

					pskin->model = (*r_model);
					pskin->bottomcolor = (*r_bottomcolor);
					pskin->topcolor = (*r_topcolor);

					PaletteHueReplace(tmp_palette, pskin->topcolor, l, m);

					if (h != 0)
						PaletteHueReplace(tmp_palette, pskin->bottomcolor, m, h);

					GL_UnloadTexture(name);

					pskin->gl_index = GL_LoadTexture(name, GLT_STUDIO, ptexture[index].width, ptexture[index].height, pData, false, (ptexture[index].flags & STUDIO_NF_MASKED) ? TEX_TYPE_ALPHA : TEX_TYPE_NONE, tmp_palette);
				}
			}

			if (pskin->gl_index != 0)
				GL_Bind(pskin->gl_index);
		}
	}

	GL_Bind(ptexture[index].index);
#endif

	gRefFuncs.R_StudioSetupSkin(ptexturehdr, index);

	if ((*currenttexture) & 0x80000000)
	{
		int materialIndex = (*currenttexture) & 0x7FFFFFFF;
		if (materialIndex >= 0 && materialIndex < (int)VBOData->vMaterials.size())
		{
			R_StudioSetupVBOMaterial(VBOData, VBOData->vMaterials[materialIndex], width, height);
		}
		else
		{
			GL_Bind(0);
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

	vec3_t entity_origin = { (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3] };
	memcpy(StudioUBO.entity_origin, entity_origin, sizeof(vec3_t));

	StudioUBO.r_numelight = 0;

	if (r_studio_legacy_elight->value > 0)
	{
		StudioUBO.r_numelight = *numlight;

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

			StudioUBO.r_elight_radius[i] = (*locallight)[i]->radius * clamp(r_studio_legacy_elight_radius_scale->value, 0.001f, 1000.0f);
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
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref,
	const int flags)
{
	auto pmesh = VBOMesh->mesh;

	program_state_t StudioProgramState = flags;

	if (r_draw_shadowcaster)
	{
		StudioProgramState |= STUDIO_SHADOW_CASTER_ENABLED;
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawGlowShell)
	{
		StudioProgramState |= (STUDIO_ADDITIVE_BLEND_ENABLED | STUDIO_GLOW_SHELL_ENABLED | STUDIO_NF_CHROME);

		if (StudioProgramState & STUDIO_NF_CELSHADE)
		{
			StudioProgramState &= ~STUDIO_NF_CELSHADE;
			StudioProgramState |= STUDIO_NF_FLATSHADE;
		}
	}
	else if ((*currententity)->curstate.renderfx == kRenderFxDrawOutline)
	{
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
				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, s_BackBufferFBO2.s_hBackBufferStencilView);
				glActiveTexture(GL_TEXTURE0);
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

	//Setup texture and texcoord
	float s, t;
	if (r_fullbright->value >= 2)
	{
		gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);

		s = 1.0f / 256.0f;
		t = 1.0f / 256.0f;
	}
	else
	{
		float width = ptexture[pskinref[pmesh->skinref]].width;
		float height = ptexture[pskinref[pmesh->skinref]].height;

		if (StudioProgramState & STUDIO_GLOW_SHELL_ENABLED)
		{
			gEngfuncs.pTriAPI->SpriteTexture(cl_shellchrome, 0);
		}
		else
		{
			if (ptexturehdr && pskinref)
			{
				R_StudioSetupSkinEx(VBOData, ptexturehdr, pskinref[pmesh->skinref], &width, &height);
			}
			else
			{
				gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);
			}
		}

		s = 1.0f / width;
		t = 1.0f / height;
	}

	if (StudioProgramState & STUDIO_NF_CHROME)
	{
		if (StudioProgramState & STUDIO_GLOW_SHELL_ENABLED)
		{
			s /= 32.0f;
			t /= 32.0f;
		}
		else
		{
			s = 1.0f / 2048.0f;
			t = 1.0f / 2048.0f;
		}
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
		glDisable(GL_CULL_FACE);

		StudioProgramState |= STUDIO_INVERT_NORMAL_ENABLED;
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

	studio_program_t prog = { 0 };

	R_UseStudioProgram(StudioProgramState, &prog);

	if (prog.r_uvscale != -1)
	{
		glUniform2f(prog.r_uvscale, s, t);
	}

	if (VBOMesh->iIndiceCount)
	{
		glDrawElements(GL_TRIANGLES, VBOMesh->iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(VBOMesh->iStartIndex));

		++r_studio_drawcall;
		r_studio_polys += VBOMesh->iPolyCount;
	}

	GL_UseProgram(0);

	//Restore states
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glEnable(GL_CULL_FACE);

	if (r_draw_opaque)
	{
		GL_EndStencil();
	}

	if (StudioProgramState & STUDIO_NF_CELSHADE_FACE)
	{
		//Texture unit 6 = Stencil texture
		if (s_GBufferFBO.s_hBackBufferStencilView)
		{
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0);
		}
	}
}

void R_StudioDrawVBOMesh(
	studio_vbo_t* VBOData,
	studio_vbo_submodel_t* VBOSubmodel,
	studio_vbo_mesh_t* VBOMesh,
	studiohdr_t* ptexturehdr,
	mstudiotexture_t* ptexture,
	short* pskinref)
{
	auto pmesh = VBOMesh->mesh;

	int flags = ptexture[pskinref[pmesh->skinref]].flags;

	//Lighting related flags are ignored when r_fullbright >= 2
	if (r_fullbright->value >= 2)
	{
		flags &= STUDIO_NF_FULLBRIGHT_ALLOWBITS;
	}
	else
	{
		flags &= STUDIO_NF_ALLOWBITS;
	}

	if ((*currententity)->curstate.renderfx == kRenderFxDrawGlowShell)
	{
		flags |= STUDIO_NF_CHROME;
	}

	//STUDIO_NF_ALPHA and STUDIO_NF_ADDITIVE is ignored when rendermode not equal to kRenderNormal
	if ((*currententity)->curstate.rendermode != kRenderNormal)
	{
		flags &= ~STUDIO_NF_ALPHA;
		flags &= ~STUDIO_NF_ADDITIVE;
	}

	if (!r_studio_celshade->value)
	{
		flags &= ~(STUDIO_NF_CELSHADE | STUDIO_NF_CELSHADE_FACE | STUDIO_NF_CELSHADE_HAIR | STUDIO_NF_CELSHADE_HAIR_H);
	}

	if (r_draw_analyzingstudio)
	{
		R_StudioDrawVBOMesh_AnalyzePass(VBOData,
			VBOSubmodel,
			VBOMesh,
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

		R_StudioDrawVBOMesh(VBOData, VBOSubmodel, VBOMesh, ptexturehdr, ptexture, pskinref);
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
	auto VBOData = R_PrepareStudioVBO(*pstudiohdr);

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
		return gRefFuncs.studioapi_StudioCheckBBox();
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

			gRefFuncs.studioapi_StudioDynamicLight(ent, plight);

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

			gRefFuncs.studioapi_StudioDynamicLight(ent, plight);

			dl = cl_dlights;
			for (int i = 0; i < 32; i++, dl++)
			{
				dl->die = dies[i];
			}
		}
	}
	else
	{
		gRefFuncs.studioapi_StudioDynamicLight(ent, plight);
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
			GL_BindFrameBuffer(&s_BackBufferFBO);
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
	gRefFuncs.R_StudioRenderFinal();
}

__forceinline void R_StudioRenderModel_originalcall_wrapper(void* pthis, int dummy)
{
	gRefFuncs.R_StudioRenderModel();
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
	StudioRenderFinal_Template(gRefFuncs.GameStudioRenderer_StudioRenderFinal, pthis, dummy);
}

void __fastcall GameStudioRenderer_StudioRenderModel(void* pthis, int dummy)
{
	StudioRenderModel_Template(gRefFuncs.GameStudioRenderer_StudioRenderModel, GameStudioRenderer_StudioRenderFinal, pthis, dummy);
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

	auto& itor = g_StudioBoneCacheManager.find(handle);

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

	auto& itor = g_StudioBoneCacheManager.find(handle);

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
	StudioSetupBones_Template(gRefFuncs.GameStudioRenderer_StudioSetupBones, pthis, dummy);
}

void __fastcall GameStudioRenderer_StudioMergeBones(void* pthis, int dummy, model_t* pSubModel)
{
	StudioMergeBones_Template(gRefFuncs.GameStudioRenderer_StudioMergeBones, pthis, dummy, pSubModel);
}

studio_vbo_material_t* R_StudioAllocVBOMaterial(studio_vbo_t* VBOData, mstudiotexture_t* texture)
{
	if (texture->index & 0x80000000)
	{
		auto materialIndex = (texture->index & 0x7FFFFFFF);
		if (materialIndex >= 0 && materialIndex < VBOData->vMaterials.size())
		{
			return VBOData->vMaterials[materialIndex];
		}
	}

	auto VBOMaterial = new studio_vbo_material_t();

	VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].gltexturenum = texture->index;
	VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].width = texture->width;
	VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].height = texture->height;
	VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].scaleX = 1;
	VBOMaterial->textures[STUDIO_DIFFUSE_TEXTURE].scaleY = 1;

	texture->index = (0x80000000 | (int)VBOData->vMaterials.size());

	VBOData->vMaterials.emplace_back(VBOMaterial);

	return VBOMaterial;
}

void R_StudioLoadExternalFile_Texture(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{
	char* basetexture_string = ValueForKey(ent, "basetexture");
	if (!basetexture_string)
	{
		gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"basetexture\" in entity \"studio_texture\"\n");
		return;
	}
	if (!studiohdr->textureindex)
	{
		gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Model %s has no texture\n", studiohdr->name);
		return;
	}
	char* flags_string = ValueForKey(ent, "flags");
	char* replacetexture_string = ValueForKey(ent, "replacetexture");

	for (int i = 0; i < studiohdr->numtextures; ++i)
	{
		auto ptexture = (mstudiotexture_t*)((byte*)studiohdr + studiohdr->textureindex) + i;

		bool bSelected = false;

		if (!strcmp(basetexture_string, "*"))
		{
			bSelected = true;
		}
		else if (!strcmp(ptexture->name, basetexture_string))
		{
			bSelected = true;
		}
		if (bSelected)
		{
			if (flags_string && !strcmp(flags_string, "STUDIO_NF_FLATSHADE"))
			{
				ptexture->flags |= STUDIO_NF_FLATSHADE;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_CHROME"))
			{
				ptexture->flags |= STUDIO_NF_CHROME;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_FULLBRIGHT"))
			{
				ptexture->flags |= STUDIO_NF_FULLBRIGHT;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_NOMIPS"))
			{
				ptexture->flags |= STUDIO_NF_NOMIPS;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_ALPHA"))
			{
				ptexture->flags |= STUDIO_NF_ALPHA;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_ADDITIVE"))
			{
				ptexture->flags |= STUDIO_NF_ADDITIVE;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_MASKED"))
			{
				ptexture->flags |= STUDIO_NF_MASKED;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_CELSHADE"))
			{
				ptexture->flags |= STUDIO_NF_CELSHADE;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_CELSHADE_FACE"))
			{
				ptexture->flags |= STUDIO_NF_CELSHADE;
				ptexture->flags |= STUDIO_NF_CELSHADE_FACE;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_CELSHADE_HAIR"))
			{
				ptexture->flags |= STUDIO_NF_CELSHADE;
				ptexture->flags |= STUDIO_NF_CELSHADE_HAIR;
			}
			else if (flags_string && !strcmp(flags_string, "STUDIO_NF_CELSHADE_HAIR_H"))
			{
				ptexture->flags |= STUDIO_NF_CELSHADE;
				ptexture->flags |= STUDIO_NF_CELSHADE_HAIR_H;
			}

			if (flags_string && !strcmp(flags_string, "-STUDIO_NF_FLATSHADE"))
			{
				ptexture->flags &= ~STUDIO_NF_FLATSHADE;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_CHROME"))
			{
				ptexture->flags &= ~STUDIO_NF_CHROME;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_FULLBRIGHT"))
			{
				ptexture->flags &= ~STUDIO_NF_FULLBRIGHT;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_NOMIPS"))
			{
				ptexture->flags &= ~STUDIO_NF_NOMIPS;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_ALPHA"))
			{
				ptexture->flags &= ~STUDIO_NF_ALPHA;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_ADDITIVE"))
			{
				ptexture->flags &= ~STUDIO_NF_ADDITIVE;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_MASKED"))
			{
				ptexture->flags &= ~STUDIO_NF_MASKED;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_CELSHADE"))
			{
				ptexture->flags &= ~STUDIO_NF_CELSHADE;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_CELSHADE_FACE"))
			{
				ptexture->flags &= ~STUDIO_NF_CELSHADE_FACE;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_CELSHADE_HAIR"))
			{
				ptexture->flags &= ~STUDIO_NF_CELSHADE_HAIR;
			}
			else if (flags_string && !strcmp(flags_string, "-STUDIO_NF_CELSHADE_HAIR_H"))
			{
				ptexture->flags &= ~STUDIO_NF_CELSHADE_HAIR_H;
			}

			if (replacetexture_string && replacetexture_string[0])
			{
				int width = 0;
				int height = 0;

				std::string texturePath = replacetexture_string;
				if (!V_GetFileExtension(replacetexture_string))
					texturePath += ".tga";

				int texId = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_STUDIO,
					(ptexture->flags & STUDIO_NF_NOMIPS) ? false : true, true, false);

				if (!texId)
				{
					texturePath = "gfx/";
					texturePath += replacetexture_string;
					if (!V_GetFileExtension(replacetexture_string))
						texturePath += ".tga";

					texId = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_STUDIO,
						(ptexture->flags & STUDIO_NF_NOMIPS) ? false : true, true, false);

					if (!texId)
					{
						texturePath = "renderer/texture/";
						texturePath += replacetexture_string;
						if (!V_GetFileExtension(replacetexture_string))
							texturePath += ".tga";

						texId = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_STUDIO,
							(ptexture->flags & STUDIO_NF_NOMIPS) ? false : true, true, true);
					}
				}

				if (texId)
				{
					auto VBOMaterial = R_StudioAllocVBOMaterial(VBOData, ptexture);

					VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].gltexturenum = texId;
					VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].width = width;
					VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].height = height;
					VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleX = 1;
					VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleY = 1;

					bool bSizeChanged = false;

					char* replacescale_string = ValueForKey(ent, "replacescale");
					if (replacescale_string)
					{
						float scales[2] = { 0 };

						if (2 == sscanf(replacescale_string, "%f %f", &scales[0], &scales[1]))
						{
							if (scales[0] > 0)
								VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleX = scales[0];

							if (scales[1] > 0)
								VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleY = scales[1];

							bSizeChanged = true;
						}
						else if (1 == sscanf(replacescale_string, "%f", &scales[0]))
						{
							if (scales[0] > 0)
							{
								VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleX = scales[0];
								VBOMaterial->textures[STUDIO_REPLACE_TEXTURE].scaleY = scales[0];
							}
							bSizeChanged = true;
						}
					}
				}
			}
		}
	}
}

void R_StudioLoadExternalFile_Efx(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{
	char* flags_string = ValueForKey(ent, "flags");
	if (flags_string && !strcmp(flags_string, "EF_ROCKET"))
	{
		studiohdr->flags |= EF_ROCKET;
	}
	if (flags_string && !strcmp(flags_string, "EF_GRENADE"))
	{
		studiohdr->flags |= EF_GRENADE;
	}
	if (flags_string && !strcmp(flags_string, "EF_GIB"))
	{
		studiohdr->flags |= EF_GIB;
	}
	if (flags_string && !strcmp(flags_string, "EF_ROTATE"))
	{
		studiohdr->flags |= EF_ROTATE;
	}
	if (flags_string && !strcmp(flags_string, "EF_TRACER"))
	{
		studiohdr->flags |= EF_TRACER;
	}
	if (flags_string && !strcmp(flags_string, "EF_ZOMGIB"))
	{
		studiohdr->flags |= EF_ZOMGIB;
	}
	if (flags_string && !strcmp(flags_string, "EF_TRACER2"))
	{
		studiohdr->flags |= EF_TRACER2;
	}
	if (flags_string && !strcmp(flags_string, "EF_TRACER3"))
	{
		studiohdr->flags |= EF_TRACER3;
	}
	if (flags_string && !strcmp(flags_string, "EF_NOSHADELIGHT"))
	{
		studiohdr->flags |= EF_NOSHADELIGHT;
	}
	if (flags_string && !strcmp(flags_string, "EF_HITBOXCOLLISIONS"))
	{
		studiohdr->flags |= EF_HITBOXCOLLISIONS;
	}
	if (flags_string && !strcmp(flags_string, "EF_FORCESKYLIGHT"))
	{
		studiohdr->flags |= EF_FORCESKYLIGHT;
	}
	if (flags_string && !strcmp(flags_string, "EF_OUTLINE"))
	{
		studiohdr->flags |= EF_OUTLINE;
	}
}

void R_StudioLoadExternalFile_Celshade(bspentity_t* ent, studiohdr_t* studiohdr, studio_vbo_t* VBOData)
{
	if (1)
	{
		char* celshade_midpoint = ValueForKey(ent, "celshade_midpoint");
		if (celshade_midpoint && celshade_midpoint[0])
		{
			if (R_ParseStringAsVector1(celshade_midpoint, VBOData->celshade_control.celshade_midpoint.m_override_value))
			{
				VBOData->celshade_control.celshade_midpoint.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"celshade_midpoint\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* celshade_softness = ValueForKey(ent, "celshade_softness");
		if (celshade_softness && celshade_softness[0])
		{
			if (R_ParseStringAsVector1(celshade_softness, VBOData->celshade_control.celshade_softness.m_override_value))
			{
				VBOData->celshade_control.celshade_softness.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"celshade_softness\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* celshade_shadow_color = ValueForKey(ent, "celshade_shadow_color");
		if (celshade_shadow_color && celshade_shadow_color[0])
		{
			if (R_ParseStringAsColor3(celshade_shadow_color, VBOData->celshade_control.celshade_shadow_color.m_override_value))
			{
				VBOData->celshade_control.celshade_shadow_color.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"celshade_shadow_color\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* outline_size = ValueForKey(ent, "outline_size");
		if (outline_size && outline_size[0])
		{
			if (R_ParseStringAsVector1(outline_size, VBOData->celshade_control.outline_size.m_override_value))
			{
				VBOData->celshade_control.outline_size.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"outline_size\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* outline_dark = ValueForKey(ent, "outline_dark");
		if (outline_dark && outline_dark[0])
		{
			if (R_ParseStringAsVector1(outline_dark, VBOData->celshade_control.outline_dark.m_override_value))
			{
				VBOData->celshade_control.outline_dark.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"outline_dark\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimlight_power = ValueForKey(ent, "rimlight_power");
		if (rimlight_power && rimlight_power[0])
		{
			if (R_ParseStringAsVector1(rimlight_power, VBOData->celshade_control.rimlight_power.m_override_value))
			{
				VBOData->celshade_control.rimlight_power.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimlight_power\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimlight_smooth2 = ValueForKey(ent, "rimlight_smooth2");
		if (rimlight_smooth2 && rimlight_smooth2[0])
		{
			if (R_ParseStringAsVector2(rimlight_smooth2, VBOData->celshade_control.rimlight_smooth2.m_override_value))
			{
				VBOData->celshade_control.rimlight_smooth2.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimlight_smooth2\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimlight_smooth = ValueForKey(ent, "rimlight_smooth");
		if (rimlight_smooth && rimlight_smooth[0])
		{
			if (R_ParseStringAsVector1(rimlight_smooth, VBOData->celshade_control.rimlight_smooth.m_override_value))
			{
				VBOData->celshade_control.rimlight_smooth.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimlight_smooth\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimlight_color = ValueForKey(ent, "rimlight_color");
		if (rimlight_color && rimlight_color[0])
		{
			if (R_ParseStringAsColor3(rimlight_color, VBOData->celshade_control.rimlight_color.m_override_value))
			{
				VBOData->celshade_control.rimlight_color.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimlight_color\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimdark_power = ValueForKey(ent, "rimdark_power");
		if (rimdark_power && rimdark_power[0])
		{
			if (R_ParseStringAsVector1(rimdark_power, VBOData->celshade_control.rimdark_power.m_override_value))
			{
				VBOData->celshade_control.rimdark_power.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimdark_power\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimdark_smooth2 = ValueForKey(ent, "rimdark_smooth2");
		if (rimdark_smooth2 && rimdark_smooth2[0])
		{
			if (R_ParseStringAsVector2(rimdark_smooth2, VBOData->celshade_control.rimdark_smooth2.m_override_value))
			{
				VBOData->celshade_control.rimdark_smooth2.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimdark_smooth2\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimdark_smooth = ValueForKey(ent, "rimdark_smooth");
		if (rimdark_smooth && rimdark_smooth[0])
		{
			if (R_ParseStringAsVector1(rimdark_smooth, VBOData->celshade_control.rimdark_smooth.m_override_value))
			{
				VBOData->celshade_control.rimdark_smooth.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimdark_smooth\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* rimdark_color = ValueForKey(ent, "rimdark_color");
		if (rimdark_color && rimdark_color[0])
		{
			if (R_ParseStringAsColor3(rimdark_color, VBOData->celshade_control.rimdark_color.m_override_value))
			{
				VBOData->celshade_control.rimdark_color.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"rimdark_color\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_specular_exp = ValueForKey(ent, "hair_specular_exp");
		if (hair_specular_exp && hair_specular_exp[0])
		{
			if (R_ParseStringAsVector1(hair_specular_exp, VBOData->celshade_control.hair_specular_exp.m_override_value))
			{
				VBOData->celshade_control.hair_specular_exp.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_specular_exp\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_specular_intensity = ValueForKey(ent, "hair_specular_intensity");
		if (hair_specular_intensity && hair_specular_intensity[0])
		{
			if (R_ParseStringAsVector3(hair_specular_intensity, VBOData->celshade_control.hair_specular_intensity.m_override_value))
			{
				VBOData->celshade_control.hair_specular_intensity.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_specular_intensity\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_specular_noise = ValueForKey(ent, "hair_specular_noise");
		if (hair_specular_noise && hair_specular_noise[0])
		{
			if (R_ParseStringAsVector3(hair_specular_noise, VBOData->celshade_control.hair_specular_noise.m_override_value))
			{
				VBOData->celshade_control.hair_specular_noise.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_specular_noise\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_specular_exp2 = ValueForKey(ent, "hair_specular_exp2");
		if (hair_specular_exp2 && hair_specular_exp2[0])
		{
			if (R_ParseStringAsVector1(hair_specular_exp2, VBOData->celshade_control.hair_specular_exp2.m_override_value))
			{
				VBOData->celshade_control.hair_specular_exp2.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_specular_exp2\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_specular_intensity2 = ValueForKey(ent, "hair_specular_intensity2");
		if (hair_specular_intensity2 && hair_specular_intensity2[0])
		{
			if (R_ParseStringAsVector3(hair_specular_intensity2, VBOData->celshade_control.hair_specular_intensity2.m_override_value))
			{
				VBOData->celshade_control.hair_specular_intensity2.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_specular_intensity2\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_specular_noise2 = ValueForKey(ent, "hair_specular_noise2");
		if (hair_specular_noise2 && hair_specular_noise2[0])
		{
			if (R_ParseStringAsVector3(hair_specular_noise2, VBOData->celshade_control.hair_specular_noise2.m_override_value))
			{
				VBOData->celshade_control.hair_specular_noise2.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_specular_noise2\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_specular_smooth = ValueForKey(ent, "hair_specular_smooth");
		if (hair_specular_smooth && hair_specular_smooth[0])
		{
			if (R_ParseStringAsVector2(hair_specular_smooth, VBOData->celshade_control.hair_specular_smooth.m_override_value))
			{
				VBOData->celshade_control.hair_specular_smooth.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_specular_smooth\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

	if (1)
	{
		char* hair_shadow_offset = ValueForKey(ent, "hair_shadow_offset");
		if (hair_shadow_offset && hair_shadow_offset[0])
		{
			if (R_ParseStringAsVector2(hair_shadow_offset, VBOData->celshade_control.hair_shadow_offset.m_override_value))
			{
				VBOData->celshade_control.hair_shadow_offset.m_is_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_StudioLoadExternalFile: Failed to parse \"hair_shadow_offset\" in entity \"studio_celshade_control\"\n");
			}
		}
	}

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

	std::string name = mod->name;
	name = name.substr(0, name.length() - 4);
	name += "_external.txt";

	char* pfile = (char*)gEngfuncs.COM_LoadFile((char*)name.c_str(), 5, NULL);
	if (!pfile)
	{
		//gEngfuncs.Con_DPrintf("R_StudioLoadExternalFile: No external file %s\n", name.c_str());
		return;
	}

	R_ParseBSPEntities(pfile, R_ParseBSPEntity_StudioAllocator);

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

	gEngfuncs.COM_FreeFile(pfile);
}