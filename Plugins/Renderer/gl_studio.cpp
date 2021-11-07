#include "gl_local.h"
#include "triangleapi.h"
#include <sstream>
#include <algorithm>

#include "mathlib.h"

std::unordered_map<int, studio_program_t> g_StudioProgramTable;

std::vector<studio_vbo_t *> g_StudioVBOCache;

//engine
model_t *cl_sprite_white;
model_t *cl_shellchrome;
mstudiomodel_t **psubmodel;
mstudiobodyparts_t **pbodypart;
studiohdr_t **pstudiohdr;
model_t **r_model;
float *r_blend;
auxvert_t **pauxverts;
float **pvlightvalues;
auxvert_t (*auxverts)[MAXSTUDIOVERTS];
vec3_t (*lightvalues)[MAXSTUDIOVERTS];
float (*pbonetransform)[MAXSTUDIOBONES][3][4];
float (*plighttransform)[MAXSTUDIOBONES][3][4];
float (*rotationmatrix)[3][4];
int (*g_NormalIndex)[MAXSTUDIOVERTS];
int(*chrome)[MAXSTUDIOVERTS][2];
int (*chromeage)[MAXSTUDIOBONES];
cl_entity_t *cl_viewent;
int *g_ForcedFaceFlags;
int *lightgammatable;
byte *texgammatable;
float *g_ChromeOrigin;
int *r_ambientlight;
float *r_shadelight;
vec3_t *r_blightvec;
float *r_plightvec;
float *r_colormix;
void *tmp_palette;
int *r_smodels_total;
int *r_amodels_drawn;

//renderer
float r_chrome[MAXSTUDIOVERTS][2];
vec3_t r_chromeup[MAXSTUDIOBONES];
vec3_t r_chromeright[MAXSTUDIOBONES];
vec3_t r_studionormal[MAXSTUDIOVERTS];
float lightpos[MAXSTUDIOVERTS][3][4];

int r_studio_drawcall;
int r_studio_polys;

cvar_t *r_studio_celshade = NULL;
cvar_t *r_studio_celshade_midpoint = NULL;
cvar_t *r_studio_celshade_softness = NULL;
cvar_t *r_studio_celshade_shadow_color = NULL;
cvar_t *r_studio_rimlight_power = NULL;
cvar_t *r_studio_rimlight_smooth = NULL;
cvar_t *r_studio_rimlight_color = NULL;
cvar_t *r_studio_rimdark_power = NULL;
cvar_t *r_studio_rimdark_smooth = NULL;
cvar_t *r_studio_rimdark_color = NULL;
cvar_t *r_studio_outline_size = NULL;
cvar_t *r_studio_outline_dark = NULL;

void R_PrepareStudioVBOSubmodel(
	studiohdr_t *studiohdr, mstudiomodel_t *submodel, 
	std::vector<studio_vbo_vertex_t> &vVertex,
	std::vector<unsigned int> &vIndices,
	studio_vbo_submodel_t *vboSubmodel)
{
	auto pstudioverts = (vec3_t *)((byte *)studiohdr + submodel->vertindex);
	auto pstudionorms = (vec3_t *)((byte *)studiohdr + submodel->normindex);
	auto pvertbone = ((byte *)studiohdr + submodel->vertinfoindex);
	auto pnormbone = ((byte *)studiohdr + submodel->norminfoindex);

	vboSubmodel->vMesh.reserve(submodel->nummesh);

	for (int k = 0; k < submodel->nummesh; k++)
	{
		auto pmesh = (mstudiomesh_t *)((byte *)studiohdr + submodel->meshindex) + k;

		auto ptricmds = (short *)((byte *)studiohdr + pmesh->triindex);

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

studio_vbo_t *R_PrepareStudioVBO(studiohdr_t *studiohdr)
{
	studio_vbo_t *VBOData = NULL;
	if (studiohdr->soundtable > 0 && studiohdr->soundtable - 1 < (int)g_StudioVBOCache.size())
	{
		VBOData = (studio_vbo_t *)g_StudioVBOCache[studiohdr->soundtable - 1];
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
		auto bodypart = (mstudiobodyparts_t *)((byte *)studiohdr + studiohdr->bodypartindex) + i;

		if (bodypart->modelindex && bodypart->nummodels)
		{
			for (int j = 0; j < bodypart->nummodels; ++j)
			{
				auto submodel = (mstudiomodel_t *)((byte *)studiohdr + bodypart->modelindex) + j;

				studio_vbo_submodel_t *vboSubmodel = new studio_vbo_submodel_t;
				vboSubmodel->submodel = submodel;

				R_PrepareStudioVBOSubmodel(studiohdr, submodel, vVertex, vIndices, vboSubmodel);

				VBOData->vSubmodel.emplace_back(vboSubmodel);

				submodel->groupindex = VBOData->vSubmodel.size();
			}
		}
	}

	VBOData->hVBO = GL_GenBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, VBOData->hVBO);
	glBufferData(GL_ARRAY_BUFFER, vVertex.size() * sizeof(studio_vbo_vertex_t), vVertex.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	VBOData->hEBO = GL_GenBuffer();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOData->hEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof(GLuint), vIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	VBOData->hStudioUBO = GL_GenBuffer();
	glBindBuffer(GL_UNIFORM_BUFFER, VBOData->hStudioUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(studio_ubo_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return VBOData;
}

void R_StudioReloadVBOCache(void)
{
	for (size_t i = 0; i < g_StudioVBOCache.size(); ++i)
	{
		if (g_StudioVBOCache[i])
		{
			auto VBOData = g_StudioVBOCache[i];

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

				auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);
				if (studiohdr)
				{
					R_PrepareStudioVBO(studiohdr);
				}
			}
		}
	}
}

void R_UseStudioProgram(int state, studio_program_t *progOutput)
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

		if (state & STUDIO_NF_ADDITIVE)
			defs << "#define STUDIO_NF_ADDITIVE\n";

		if (state & STUDIO_NF_MASKED)
			defs << "#define STUDIO_NF_MASKED\n";

		if (state & STUDIO_NF_CELSHADE)
			defs << "#define STUDIO_NF_CELSHADE\n";

		if (state & STUDIO_NF_CELSHADE_FACE)
			defs << "#define STUDIO_NF_CELSHADE_FACE\n";

		if (state & STUDIO_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if (state & STUDIO_TRANSPARENT_ENABLED)
			defs << "#define TRANSPARENT_ENABLED\n";

		if (state & STUDIO_TRANSADDITIVE_ENABLED)
			defs << "#define TRANSADDITIVE_ENABLED\n";

		if (state & STUDIO_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & STUDIO_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

		if (state & STUDIO_SHADOW_CASTER_ENABLED)
			defs << "#define SHADOW_CASTER_ENABLED\n";

		if (state & STUDIO_LEGACY_BONE_ENABLED)
			defs << "#define LEGACY_BONE_ENABLED\n";

		if (state & STUDIO_GLOW_SHELL_ENABLED)
			defs << "#define GLOW_SHELL_ENABLED\n";

		if (state & STUDIO_OUTLINE_ENABLED)
			defs << "#define OUTLINE_ENABLED\n";

		if (state & STUDIO_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		//if (state & STUDIO_BINDLESS_ENABLED)
		//	defs << "#define BINDLESS_ENABLED\n";

		if (state & STUDIO_OIT_ALPHA_BLEND_ENABLED)
			defs << "#define OIT_ALPHA_BLEND_ENABLED\n";

		if (state & STUDIO_OIT_ADDITIVE_BLEND_ENABLED)
			defs << "#define OIT_ADDITIVE_BLEND_ENABLED\n";

		if (glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define UINT64_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\studio_shader.vsh", "renderer\\shader\\studio_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, r_celshade_midpoint, "r_celshade_midpoint");
			SHADER_UNIFORM(prog, r_celshade_softness, "r_celshade_softness");
			SHADER_UNIFORM(prog, r_celshade_shadow_color, "r_celshade_shadow_color");
			SHADER_UNIFORM(prog, r_rimlight_power, "r_rimlight_power");
			SHADER_UNIFORM(prog, r_rimlight_smooth, "r_rimlight_smooth");
			SHADER_UNIFORM(prog, r_rimlight_color, "r_rimlight_color");
			SHADER_UNIFORM(prog, r_rimdark_power, "r_rimdark_power");
			SHADER_UNIFORM(prog, r_rimdark_smooth, "r_rimdark_smooth");
			SHADER_UNIFORM(prog, r_rimdark_color, "r_rimdark_color");
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
			glUniform3f(prog.entityPos, (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3]);

		if (prog.r_celshade_midpoint != -1)
			glUniform1f(prog.r_celshade_midpoint, r_studio_celshade_midpoint->value);

		if (prog.r_celshade_softness != -1)
			glUniform1f(prog.r_celshade_softness, r_studio_celshade_softness->value);

		if (prog.r_celshade_shadow_color != -1)
		{
			vec3_t color = { 0 };
			R_ParseVectorCvar(r_studio_celshade_shadow_color, color);
			glUniform3f(prog.r_celshade_shadow_color, color[0], color[1], color[2]);
		}

		if (prog.r_rimlight_power != -1)
		{
			glUniform1f(prog.r_rimlight_power, r_studio_rimlight_power->value);
		}
		if (prog.r_rimlight_smooth != -1)
		{
			glUniform1f(prog.r_rimlight_smooth, r_studio_rimlight_smooth->value);
		}
		if (prog.r_rimlight_color != -1)
		{
			vec3_t color = { 0 };
			R_ParseVectorCvar(r_studio_rimlight_color, color);
			glUniform3f(prog.r_rimlight_color, color[0], color[1], color[2]);
		}
		if (prog.r_rimdark_power != -1)
		{
			glUniform1f(prog.r_rimdark_power, r_studio_rimdark_power->value);
		}
		if (prog.r_rimdark_smooth != -1)
		{
			glUniform1f(prog.r_rimdark_smooth, r_studio_rimdark_smooth->value);
		}
		if (prog.r_rimdark_color != -1)
		{
			vec3_t color = { 0 };
			R_ParseVectorCvar(r_studio_rimdark_color, color);
			glUniform3f(prog.r_rimdark_color, color[0], color[1], color[2]);
		}
		if (prog.r_outline_dark != -1)
		{
			glUniform1f(prog.r_outline_dark, r_studio_outline_dark->value);
		}
		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		g_pMetaHookAPI->SysError("R_UseStudioProgram: Failed to load program!");
	}
}

const program_state_name_t s_StudioProgramStateName[] = {
{ STUDIO_GBUFFER_ENABLED				,"STUDIO_GBUFFER_ENABLED"					},
{ STUDIO_TRANSPARENT_ENABLED			,"STUDIO_TRANSPARENT_ENABLED"				},
{ STUDIO_TRANSADDITIVE_ENABLED			,"STUDIO_TRANSADDITIVE_ENABLED"				},
{ STUDIO_LINEAR_FOG_ENABLED				,"STUDIO_LINEAR_FOG_ENABLED"				},
{ STUDIO_EXP2_FOG_ENABLED				,"STUDIO_EXP2_FOG_ENABLED"					},
{ STUDIO_SHADOW_CASTER_ENABLED			,"STUDIO_SHADOW_CASTER_ENABLED"				},
{ STUDIO_LEGACY_BONE_ENABLED			,"STUDIO_LEGACY_BONE_ENABLED"				},
{ STUDIO_GLOW_SHELL_ENABLED				,"STUDIO_GLOW_SHELL_ENABLED"				},
{ STUDIO_OUTLINE_ENABLED				,"STUDIO_OUTLINE_ENABLED"					},
{ STUDIO_CLIP_ENABLED					,"STUDIO_CLIP_ENABLED"						},
{ STUDIO_BINDLESS_ENABLED				,"STUDIO_BINDLESS_ENABLED"					},
{ STUDIO_OIT_ALPHA_BLEND_ENABLED		,"STUDIO_OIT_ALPHA_BLEND_ENABLED"			},
{ STUDIO_OIT_ADDITIVE_BLEND_ENABLED		,"STUDIO_OIT_ADDITIVE_BLEND_ENABLED"		},

{ STUDIO_NF_FLATSHADE					,"STUDIO_NF_FLATSHADE"		},
{ STUDIO_NF_CHROME						,"STUDIO_NF_CHROME"			},
{ STUDIO_NF_FULLBRIGHT					,"STUDIO_NF_FULLBRIGHT"		},
{ STUDIO_NF_ADDITIVE					,"STUDIO_NF_ADDITIVE"		},
{ STUDIO_NF_CELSHADE					,"STUDIO_NF_CELSHADE"		},
{ STUDIO_NF_CELSHADE_FACE				,"STUDIO_NF_CELSHADE_FACE"	},
};

void R_SaveStudioProgramStates(void)
{
	std::stringstream ss;
	for (auto &p : g_StudioProgramTable)
	{
		if (p.first == 0)
		{
			ss << "NONE";
		}
		else
		{
			for (int i = 0; i < _ARRAYSIZE(s_StudioProgramStateName); ++i)
			{
				if (p.first & s_StudioProgramStateName[i].state)
				{
					ss << s_StudioProgramStateName[i].name << " ";
				}
			}
		}
		ss << "\n";
	}

	auto FileHandle = g_pFileSystem->Open("renderer/shader/studio_cache.txt", "wt");
	if (FileHandle)
	{
		auto str = ss.str();
		g_pFileSystem->Write(str.data(), str.length(), FileHandle);
		g_pFileSystem->Close(FileHandle);
	}
}

void R_LoadStudioProgramStates(void)
{
	auto FileHandle = g_pFileSystem->Open("renderer/shader/studio_cache.txt", "rt");
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
						for (int i = 0; i < _ARRAYSIZE(s_StudioProgramStateName); ++i)
						{
							if (!strcmp(token, s_StudioProgramStateName[i].name))
							{
								if (ProgramState == -1)
									ProgramState = 0;
								ProgramState |= s_StudioProgramStateName[i].state;
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

			if (ProgramState != -1)
				R_UseStudioProgram(ProgramState, NULL);
		}
		g_pFileSystem->Close(FileHandle);
	}

	GL_UseProgram(0);
}

void R_ShutdownStudio(void)
{
	g_StudioProgramTable.clear();
}

void R_InitStudio(void)
{
	r_studio_celshade = gEngfuncs.pfnRegisterVariable("r_studio_celshade", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_midpoint = gEngfuncs.pfnRegisterVariable("r_studio_celshade_midpoint", "-0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_softness = gEngfuncs.pfnRegisterVariable("r_studio_celshade_softness", "0.05", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_celshade_shadow_color = gEngfuncs.pfnRegisterVariable("r_studio_celshade_shadow_color", "220 210 210", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_outline_size = gEngfuncs.pfnRegisterVariable("r_studio_outline_size", "3.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_outline_dark = gEngfuncs.pfnRegisterVariable("r_studio_outline_dark", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_studio_rimlight_power = gEngfuncs.pfnRegisterVariable("r_studio_rimlight_power", "5.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimlight_smooth = gEngfuncs.pfnRegisterVariable("r_studio_rimlight_smooth", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimlight_color = gEngfuncs.pfnRegisterVariable("r_studio_rimlight_color", "40 40 40", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimdark_power = gEngfuncs.pfnRegisterVariable("r_studio_rimdark_power", "5.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimdark_smooth = gEngfuncs.pfnRegisterVariable("r_studio_rimdark_smooth", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_studio_rimdark_color = gEngfuncs.pfnRegisterVariable("r_studio_rimdark_color", "50 50 50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

inline void R_StudioTransformAuxVert(auxvert_t *av, int bone, vec3_t vert)
{
#ifndef SSE
	VectorTransform(vert, (*pbonetransform)[bone], av->fv);
#else
	VectorTransformSSE(vert, (*pbonetransform)[bone], av->fv);
#endif
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

void BuildNormalIndexTable(void)
{
	if (gRefFuncs.BuildNormalIndexTable)
		return gRefFuncs.BuildNormalIndexTable();

	int					j;
	int					i;
	mstudiomesh_t		*pmesh;

	for (i = 0; i < (*psubmodel)->numverts; i++)
	{
		(*g_NormalIndex)[i] = -1;
	}

	for (j = 0; j < (*psubmodel)->nummesh; j++)
	{
		short		*ptricmds;

		pmesh = (mstudiomesh_t *)((byte *)(*pstudiohdr) + (*psubmodel)->meshindex) + j;
		ptricmds = (short *)((byte *)(*pstudiohdr) + pmesh->triindex);

		while (i = *(ptricmds++))
		{
			if (i < 0)
			{
				i = -i;
			}

			for (; i > 0; i--, ptricmds += 4)
			{
				if ((*g_NormalIndex)[ptricmds[0]] < 0)
					(*g_NormalIndex)[ptricmds[0]] = ptricmds[1];
			}
		}
	}
}

studiohdr_t *R_LoadTextures(model_t *psubm)
{
	model_t *texmodel;

	if ((*pstudiohdr)->textureindex == 0)
	{
		studiohdr_t *ptexturehdr;
		char modelname[256];

		strncpy(modelname, psubm->name, sizeof(modelname) - 2);
		modelname[sizeof(modelname) - 2] = 0;

		strcpy(&modelname[strlen(modelname) - 4], "T.mdl");
		texmodel = IEngineStudio.Mod_ForName(modelname, true);
		psubm->texinfo = (mtexinfo_t *)texmodel;

		ptexturehdr = (studiohdr_t *)texmodel->cache.data;
		strncpy(ptexturehdr->name, modelname, sizeof(ptexturehdr->name) - 1);
		ptexturehdr->name[sizeof(ptexturehdr->name) - 1] = 0;

		return ptexturehdr;
	}

	return (*pstudiohdr);
}

void R_EnableStudioVBO(studio_vbo_t *VBOData)
{
	if (VBOData)
	{
		//setup ubo
		if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
		{
			g_ChromeOrigin[0] = cos(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
			g_ChromeOrigin[1] = sin(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
			g_ChromeOrigin[2] = cos(r_glowshellfreq->value * (*cl_time) * 0.33f) * 4000.0f;

			r_colormix[0] = (float)(*currententity)->curstate.rendercolor.r / 255.0f;
			r_colormix[1] = (float)(*currententity)->curstate.rendercolor.g / 255.0f;
			r_colormix[2] = (float)(*currententity)->curstate.rendercolor.b / 255.0f;
		}

		studio_ubo_t StudioUBO;

		StudioUBO.r_ambientlight = (float)(*r_ambientlight);
		StudioUBO.r_shadelight = (*r_shadelight);
		StudioUBO.r_blend = (*r_blend);

		StudioUBO.r_scale = ((*currententity)->curstate.renderfx == kRenderFxGlowShell ||
			(*currententity)->curstate.renderfx == kRenderFxOutline)
			? (*currententity)->curstate.renderamt * 0.05f : 0;

		memcpy(StudioUBO.r_colormix, r_colormix, sizeof(vec3_t));
		memcpy(StudioUBO.r_origin, g_ChromeOrigin, sizeof(vec3_t));
		memcpy(StudioUBO.r_plightvec, r_plightvec, sizeof(vec3_t));

		vec3_t entity_origin = { (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3] };
		memcpy(StudioUBO.entity_origin, entity_origin, sizeof(vec3_t));

		memcpy(StudioUBO.bonematrix, (*pbonetransform), sizeof(mat3x4) * 128);

		glNamedBufferSubData(VBOData->hStudioUBO, 0, sizeof(StudioUBO), &StudioUBO);

		//bind ubo

		glBindBuffer(GL_ARRAY_BUFFER, VBOData->hVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOData->hEBO);
		glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_STUDIO_UBO, VBOData->hStudioUBO);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);

		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, pos));
		glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, normal));
		glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, texcoord));
		glVertexAttribIPointer(3, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
	}
	else
	{
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void R_GLStudioDrawPoints(void)
{
	int stencilState = 1;

	if (r_draw_shadowcaster)
	{
		//the fxxking StudioRenderFinal which will enable GL_BLEND and mess everything up.
		glDisable(GL_BLEND);
	}
	else if (r_draw_opaque)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, stencilState, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	auto engine_pauxverts = (*pauxverts);
	auto engine_pvlightvalues = (*pvlightvalues);
	auto engine_pstudiohdr = (*pstudiohdr);
	auto engine_psubmodel = (*psubmodel);

	auto pvertbone = ((byte *)engine_pstudiohdr + engine_psubmodel->vertinfoindex);
	auto pnormbone = ((byte *)engine_pstudiohdr + engine_psubmodel->norminfoindex);
	auto ptexturehdr = R_LoadTextures(*r_model);
	auto ptexture = (mstudiotexture_t *)((byte *)ptexturehdr + ptexturehdr->textureindex);

	auto pmesh = (mstudiomesh_t *)((byte *)engine_pstudiohdr + engine_psubmodel->meshindex);

	auto pstudioverts = (vec3_t *)((byte *)engine_pstudiohdr + engine_psubmodel->vertindex);
	auto pstudionorms = (vec3_t *)((byte *)engine_pstudiohdr + engine_psubmodel->normindex);

	auto pskinref = (short *)((byte *)ptexturehdr + ptexturehdr->skinindex);

	int iFlippedVModel = 0;

	studio_vbo_t *VBOData = NULL;
	studio_vbo_submodel_t *VBOSubmodel = NULL;

	VBOData = R_PrepareStudioVBO(engine_pstudiohdr);

	R_EnableStudioVBO(VBOData);

	if (engine_psubmodel->groupindex < 1 || engine_psubmodel->groupindex >(int)VBOData->vSubmodel.size()) {
		g_pMetaHookAPI->SysError("R_StudioFindVBOCache: invalid index");
	}

	VBOSubmodel = VBOData->vSubmodel[engine_psubmodel->groupindex - 1];

	if ((*currententity)->curstate.skin != 0 && (*currententity)->curstate.skin < ptexturehdr->numskinfamilies)
		pskinref += ((*currententity)->curstate.skin * ptexturehdr->numskinref);

	if (engine_pstudiohdr->numbones > MAXSTUDIOBONES)
	{
		g_pMetaHookAPI->SysError("R_GLStudioDrawPoints: %s numbones (%d) > MAXSTUDIOBONES (%d)", engine_pstudiohdr->name, engine_pstudiohdr->numbones, MAXSTUDIOBONES);
	}

	glCullFace(GL_FRONT);

	if (R_IsFlippedViewModel())
	{
		glDisable(GL_CULL_FACE);
		iFlippedVModel = 1;
	}

	pstudionorms = (vec3_t *)((byte *)engine_pstudiohdr + engine_psubmodel->normindex);
	pnormbone = ((byte *)engine_pstudiohdr + engine_psubmodel->norminfoindex);

	for (size_t j = 0; j < VBOSubmodel->vMesh.size(); j++)
	{
		auto &VBOMesh = VBOSubmodel->vMesh[j];

		pmesh = VBOMesh.mesh;

		int flags = ptexture[pskinref[pmesh->skinref]].flags | (*g_ForcedFaceFlags);

		if (r_fullbright->value >= 2)
		{
			flags &= STUDIO_NF_FULLBRIGHT_ALLOWBITS;
		}
		else
		{
			flags &= STUDIO_NF_ALLOWBITS;
		}

		int GBufferMask = GBUFFER_MASK_ALL;
		int StudioProgramState = flags;

		//if (bUseBindless)
		//{
		//	StudioProgramState |= STUDIO_BINDLESS_ENABLED;
		//}

		if (r_draw_shadowcaster)
		{
			StudioProgramState |= STUDIO_SHADOW_CASTER_ENABLED;
		}
		else if (r_draw_opaque)
		{
			if (flags & STUDIO_NF_FLATSHADE)
			{
				if (stencilState != 2)
				{
					stencilState = 2;
					glStencilFunc(GL_ALWAYS, stencilState, 0xFF);
				}
			}
			else
			{
				if (stencilState != 1)
				{
					stencilState = 1;
					glStencilFunc(GL_ALWAYS, stencilState, 0xFF);
				}
			}
		}

		if (r_draw_shadowcaster)
		{

		}
		else if ((*currententity)->curstate.renderfx == kRenderFxOutline)
		{
			glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			StudioProgramState |= STUDIO_OUTLINE_ENABLED;
			StudioProgramState &= ~STUDIO_NF_CHROME;
			StudioProgramState &= ~STUDIO_NF_ADDITIVE;
			StudioProgramState &= ~STUDIO_NF_MASKED;
			StudioProgramState &= ~STUDIO_NF_CELSHADE;
			StudioProgramState &= ~STUDIO_NF_CELSHADE_FACE;
		}
		else if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
		{
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glShadeModel(GL_SMOOTH);

			GBufferMask = GBUFFER_MASK_ADDITIVE;
			StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE;
		}
		else if (flags & STUDIO_NF_MASKED)
		{
			//glEnable(GL_ALPHA_TEST);
			//glAlphaFunc(GL_GREATER, 0.5);
			glDepthMask(GL_TRUE);
		}
		else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
		{
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glShadeModel(GL_SMOOTH);

			GBufferMask = GBUFFER_MASK_ADDITIVE;
			StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE;
		}
		else if ((*currententity)->curstate.rendermode == kRenderTransAdd)
		{
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			glShadeModel(GL_SMOOTH);

			GBufferMask = GBUFFER_MASK_ADDITIVE;
			StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE | STUDIO_TRANSADDITIVE_ENABLED;
		}

		if (r_draw_pass == r_draw_reflect && curwater)
		{
			StudioProgramState |= STUDIO_CLIP_ENABLED;
		}

		if (!drawgbuffer && r_fog_mode == GL_LINEAR)
		{
			StudioProgramState |= STUDIO_LINEAR_FOG_ENABLED;
		}
		else if (!drawgbuffer && r_fog_mode == GL_EXP2)
		{
			StudioProgramState |= STUDIO_EXP2_FOG_ENABLED;
		}

		if (drawgbuffer)
		{
			StudioProgramState |= STUDIO_GBUFFER_ENABLED;
		}

		if (r_draw_oitblend)
		{
			if((*currententity)->curstate.rendermode == kRenderTransAdd)
				StudioProgramState |= STUDIO_OIT_ADDITIVE_BLEND_ENABLED;
			else
				StudioProgramState |= STUDIO_OIT_ALPHA_BLEND_ENABLED;
		}

		float s, t;
		//setup texture and texcoord
		if (r_fullbright->value >= 2)
		{
			gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);

			s = 1.0f / 256.0f;
			t = 1.0f / 256.0f;
		}
		else
		{
			s = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].width;
			t = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].height;

			gRefFuncs.R_StudioSetupSkin(ptexturehdr, pskinref[pmesh->skinref]);
		}

		if (flags & STUDIO_NF_CHROME)
		{
			if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
			{
				StudioProgramState |= STUDIO_GLOW_SHELL_ENABLED;
				s /= 32.0f;
				t /= 32.0f;
			}
			else
			{
				s = 1.0f / 2048.0f;
				t = 1.0f / 2048.0f;
			}
		}

		studio_program_t prog = { 0 };

		R_UseStudioProgram(StudioProgramState, &prog);

		if (prog.r_uvscale != -1)
		{
			glUniform2f(prog.r_uvscale, s, t);
		}

		R_SetGBufferMask(GBufferMask);

		if (VBOMesh.iIndiceCount)
		{
			glDrawElements(GL_TRIANGLES, VBOMesh.iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(VBOMesh.iStartIndex));
				
			++r_studio_drawcall;
			r_studio_polys += VBOMesh.iPolyCount;
		}

		if (r_draw_shadowcaster)
		{
				
		}
		else if ((*currententity)->curstate.renderfx == kRenderFxOutline)
		{
			glStencilFunc(GL_ALWAYS, stencilState, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		}
		else if (flags & STUDIO_NF_MASKED)
		{
			//glAlphaFunc(GL_NOTEQUAL, 0);
			//glDisable(GL_ALPHA_TEST);
		}
		else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
		{
			glDisable(GL_BLEND);
			glDepthMask(1);
			glShadeModel(GL_FLAT);
		}
		else if ((*currententity)->curstate.rendermode == kRenderTransAdd)
		{
			glDisable(GL_BLEND);
			glShadeModel(GL_FLAT);
		}
	}

	glEnable(GL_CULL_FACE);

	if (r_draw_opaque)
	{
		glDisable(GL_STENCIL_TEST);
	}

	GL_UseProgram(0);

	R_EnableStudioVBO(NULL);
}

//StudioAPI

void studioapi_StudioDynamicLight(cl_entity_t *ent, alight_t *plight)
{
	if (!r_light_dynamic->value)
		return gRefFuncs.studioapi_StudioDynamicLight(ent, plight);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		float dies[256];

		dlight_t *dl = cl_dlights;
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
		dlight_t *dl = cl_dlights;
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

void studioapi_RestoreRenderer(void)
{
	glDepthMask(1);
	gRefFuncs.studioapi_RestoreRenderer();
}

void R_StudioDrawBatch(void)
{
	void *vStartIndex[MAXSTUDIOMESHES];
	int vIndiceCount[MAXSTUDIOMESHES];
	int arrayCount = 0;

	auto VBOData = R_PrepareStudioVBO(*pstudiohdr);

	R_EnableStudioVBO(VBOData);

	for (int i = 0; i < (*pstudiohdr)->numbodyparts; i++)
	{
		void *temp_bodypart;
		void *temp_submodel;

		IEngineStudio.StudioSetupModel(i, &temp_bodypart, &temp_submodel);

		if ((*psubmodel)->groupindex < 1 || (*psubmodel)->groupindex >(int)VBOData->vSubmodel.size()) {
			g_pMetaHookAPI->SysError("R_StudioFindVBOCache: invalid index");
		}

		auto VBOSubmodel = VBOData->vSubmodel[(*psubmodel)->groupindex - 1];

		for (size_t j = 0; j < VBOSubmodel->vMesh.size(); ++j)
		{
			if (arrayCount == MAXSTUDIOMESHES)
			{
				g_pMetaHookAPI->SysError("R_StudioFindVBOCache: too many meshes.");
			}

			vStartIndex[arrayCount] = BUFFER_OFFSET(VBOSubmodel->vMesh[j].iStartIndex);
			vIndiceCount[arrayCount] = VBOSubmodel->vMesh[j].iIndiceCount;
			r_studio_polys += VBOSubmodel->vMesh[j].iIndiceCount;
			arrayCount++;
		}
	}

	int StudioProgramState = 0;
	int GBufferMask = GBUFFER_MASK_ALL;

	//if (bUseBindless)
	//{
	//	StudioProgramState |= STUDIO_BINDLESS_ENABLED;
	//}

	if (r_draw_shadowcaster)
	{
		glDisable(GL_BLEND);
		StudioProgramState |= STUDIO_SHADOW_CASTER_ENABLED;
	}

	if (drawgbuffer)
	{
		StudioProgramState |= STUDIO_GBUFFER_ENABLED;
	}

	glCullFace(GL_FRONT);

	studio_program_t prog = { 0 };

	R_UseStudioProgram(StudioProgramState, &prog);
	R_SetGBufferMask(GBufferMask);

	glMultiDrawElements(GL_TRIANGLES, vIndiceCount, GL_UNSIGNED_INT, (const void **)vStartIndex, arrayCount);
	r_studio_drawcall++;

	GL_UseProgram(0);

	R_EnableStudioVBO(NULL);
}

//Engine StudioRenderer

void R_StudioRenderFinal(void)
{
	if (r_draw_shadowcaster)
	{
		IEngineStudio.SetupRenderer((*currententity)->curstate.rendermode);
		IEngineStudio.GL_SetRenderMode((*currententity)->curstate.rendermode);

		R_StudioDrawBatch();

		IEngineStudio.RestoreRenderer();
	}
	else
	{
		gRefFuncs.R_StudioRenderFinal();
	}
}

void R_StudioRenderModel(void)
{
	if (r_draw_shadowcaster)
	{
		return gRefFuncs.R_StudioRenderModel();
	}

	if ((*pstudiohdr)->flags & EF_OUTLINE)
	{
		gRefFuncs.R_StudioRenderModel();

		int saved_renderfx = (*currententity)->curstate.renderfx;
		int savbed_renderamt = (*currententity)->curstate.renderamt;

		(*currententity)->curstate.renderfx = kRenderFxOutline;
		(*currententity)->curstate.renderamt = clamp(r_studio_outline_size->value, 0, 255);

		gRefFuncs.R_StudioRenderModel();

		(*currententity)->curstate.renderfx = saved_renderfx;
		(*currententity)->curstate.renderamt = savbed_renderamt;
		return;
	}

	return gRefFuncs.R_StudioRenderModel();
}

//Client StudioRenderer

void __fastcall GameStudioRenderer_StudioRenderFinal(void *pthis, int)
{
	if (r_draw_shadowcaster)
	{
		IEngineStudio.SetupRenderer((*currententity)->curstate.rendermode);
		IEngineStudio.GL_SetRenderMode((*currententity)->curstate.rendermode);

		R_StudioDrawBatch();

		IEngineStudio.RestoreRenderer();
	}
	else
	{
		gRefFuncs.GameStudioRenderer_StudioRenderFinal(pthis, 0);
	}
}

void __fastcall GameStudioRenderer_StudioRenderModel(void *pthis, int)
{
	if (r_draw_shadowcaster)
	{
		return gRefFuncs.GameStudioRenderer_StudioRenderModel(pthis, 0);
	}

	if ((*pstudiohdr)->flags & EF_OUTLINE)
	{
		gRefFuncs.GameStudioRenderer_StudioRenderModel(pthis, 0);

		int saved_renderfx = (*currententity)->curstate.renderfx;
		int savbed_renderamt = (*currententity)->curstate.renderamt;

		(*currententity)->curstate.renderfx = kRenderFxOutline;
		(*currententity)->curstate.renderamt = clamp(r_studio_outline_size->value, 0, 255);

		gRefFuncs.GameStudioRenderer_StudioRenderModel(pthis, 0);

		(*currententity)->curstate.renderfx = saved_renderfx;
		(*currententity)->curstate.renderamt = savbed_renderamt;
		return;
	}

	return gRefFuncs.GameStudioRenderer_StudioRenderModel(pthis, 0);
}

void R_StudioLoadExternalFile_Texture(bspentity_t *ent, studiohdr_t *studiohdr)
{
	char *basetexture_string = ValueForKey(ent, "basetexture");
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
	char *flags_string = ValueForKey(ent, "flags");
	char *replacetexture_string = ValueForKey(ent, "replacetexture");

	for (int i = 0; i < studiohdr->numtextures; ++i)
	{
		auto ptexture = (mstudiotexture_t *)((byte *)studiohdr + studiohdr->textureindex) + i;

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
				ptexture->flags |= STUDIO_NF_CELSHADE_FACE;
			}
			if (replacetexture_string && replacetexture_string[0])
			{
				int width = 0;
				int height = 0;
				std::string texturePath = "gfx/";
				texturePath += replacetexture_string;
				if (!V_GetFileExtension(replacetexture_string))
					texturePath += ".tga";

				int texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_STUDIO, 
					(ptexture->flags & STUDIO_NF_NOMIPS) ? false : true, true);
				if (!texId)
				{
					texturePath = "renderer/texture/";
					texturePath += replacetexture_string;
					if (!V_GetFileExtension(replacetexture_string))
						texturePath += ".tga";

					texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_STUDIO, 
						(ptexture->flags & STUDIO_NF_NOMIPS) ? false : true, true);
				}
				if (texId)
				{
					ptexture->index = texId;

					bool bSizeChanged = false;

					char *replacescale_string = ValueForKey(ent, "replacescale");
					if (replacescale_string)
					{
						float scales[2] = { 0 };
						if (2 == sscanf(replacescale_string, "%f %f", &scales[0], &scales[1]))
						{
							if (scales[0] > 0)
								ptexture->width = width * scales[0];

							if (scales[1] > 0)
								ptexture->height = height * scales[1];

							bSizeChanged = true;
						}
						else if (1 == sscanf(replacescale_string, "%f", &scales[0]))
						{
							if (scales[0] > 0)
							{
								ptexture->width = width * scales[0];
								ptexture->height = height * scales[0];
							}
							bSizeChanged = true;
						}
					}

					if (!bSizeChanged)
					{
						ptexture->width = width;
						ptexture->height = height;
					}
				}
			}
		}
	}
}

void R_StudioLoadExternalFile_Efx(bspentity_t *ent, studiohdr_t *studiohdr)
{
	char *flags_string = ValueForKey(ent, "flags");
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

static std::vector<bspentity_t> g_StudioBSPEntities;

bspentity_t *R_ParseBSPEntity_StudioAllocator(void)
{
	size_t len = g_StudioBSPEntities.size();

	g_StudioBSPEntities.resize(len + 1);

	return &g_StudioBSPEntities[len];
}

void R_StudioLoadExternalFile(model_t *mod, studiohdr_t *studiohdr)
{
	std::string name = mod->name;
	name = name.substr(0, name.length() - 4);
	name += "_external.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_DPrintf("R_StudioLoadExternalFile: No external file %s\n", name.c_str());
		return;
	}
	
	R_ParseBSPEntities(pfile, R_ParseBSPEntity_StudioAllocator);

	for (size_t i = 0; i < g_StudioBSPEntities.size(); ++i)
	{
		bspentity_t *ent = &g_StudioBSPEntities[i];

		char *classname = ent->classname;

		if (!classname)
			continue;

		if (!strcmp(classname, "studio_texture"))
		{
			R_StudioLoadExternalFile_Texture(ent, studiohdr);
		}
		else if (!strcmp(classname, "studio_efx"))
		{
			R_StudioLoadExternalFile_Efx(ent, studiohdr);
		}
	}

	for (size_t i = 0; i < g_StudioBSPEntities.size(); i++)
	{
		FreeBSPEntity(&g_StudioBSPEntities[i]);
	}

	g_StudioBSPEntities.clear();

	gEngfuncs.COM_FreeFile(pfile);
}