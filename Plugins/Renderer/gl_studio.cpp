#include "gl_local.h"
#include "triangleapi.h"
#include <sstream>

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
int r_studio_framecount;

cvar_t *r_studio_vbo = NULL;

studio_vbo_t *R_StudioPrepareVBOSubmodel(studiohdr_t *studiohdr, mstudiomodel_t *submodel)
{
	studio_vbo_t *VBOData = NULL;
	if (submodel->groupindex > 0 && submodel->groupindex - 1 < g_StudioVBOCache.size())
	{
		VBOData = (studio_vbo_t *)g_StudioVBOCache[submodel->groupindex - 1];
		if (VBOData)
		{
			return VBOData;
		}
	}

	VBOData = new studio_vbo_t;
	VBOData->studiohdr = studiohdr;
	g_StudioVBOCache.emplace_back(VBOData);
	submodel->groupindex = g_StudioVBOCache.size();

	std::vector<studio_vbo_vertex_t> vVertex;
	std::vector<unsigned int> vIndices;
	
	auto pstudioverts = (vec3_t *)((byte *)studiohdr + submodel->vertindex);
	auto pstudionorms = (vec3_t *)((byte *)studiohdr + submodel->normindex);
	auto pvertbone = ((byte *)studiohdr + submodel->vertinfoindex);
	auto pnormbone = ((byte *)studiohdr + submodel->norminfoindex);

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
					iNumTri++;

					prv0 = prv1;
					prv1 = prv2;
					prv2 = iNumVertex;

					vVertex.emplace_back(pstudioverts[ptricmds[0]], pstudionorms[ptricmds[1]], (float)ptricmds[2], (float)ptricmds[3], (int)pvertbone[ptricmds[0]], (int)pnormbone[ptricmds[1]]);
					iNumVertex++;
				}
			}
		}

		VBOData->vMesh.emplace_back(VBOMesh);
	}

	qglGenBuffersARB(1, &VBOData->hDataBuffer);
	qglGenBuffersARB(1, &VBOData->hIndexBuffer);

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, VBOData->hDataBuffer);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, vVertex.size() * sizeof(studio_vbo_vertex_t), vVertex.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, VBOData->hIndexBuffer);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vIndices.size() * sizeof(unsigned int), vIndices.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

	return VBOData;
}

void R_StudioPrepareVBO(studiohdr_t *studiohdr)
{
	for (int i = 0; i < studiohdr->numbodyparts; i++)
	{
		auto pbodypart = (mstudiobodyparts_t *)((byte *)studiohdr + studiohdr->bodypartindex) + i;

		if (pbodypart->modelindex && pbodypart->nummodels)
		{
			for (int j = 0; j < pbodypart->nummodels; ++j)
			{
				auto psubmodel = (mstudiomodel_t *)((byte *)studiohdr + pbodypart->modelindex) + j;

				R_StudioPrepareVBOSubmodel(studiohdr, psubmodel);
			}
		}
	}
}

void R_StudioReloadVBOCache(void)
{
	for (size_t i = 0; i < g_StudioVBOCache.size(); ++i)
	{
		if (g_StudioVBOCache[i])
		{
			auto VBOData = g_StudioVBOCache[i];

			if (VBOData->hDataBuffer)
			{
				qglDeleteBuffersARB(1, &VBOData->hDataBuffer);
			}
			if (VBOData->hIndexBuffer)
			{
				qglDeleteBuffersARB(1, &VBOData->hIndexBuffer);
			}

			delete g_StudioVBOCache[i];

			g_StudioVBOCache[i] = NULL;
		}
	}

#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2
#define NL_CLIENT 3
	for (int i = 0; i < *mod_numknown; ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_studio && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);
				if (studiohdr)
				{
					R_StudioPrepareVBO(studiohdr);
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

		if (state & STUDIO_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if (state & STUDIO_TRANSPARENT_ENABLED)
			defs << "#define TRANSPARENT_ENABLED\n";

		if (state & STUDIO_TRANSADDITIVE_ENABLED)
			defs << "#define TRANSADDITIVE_ENABLED\n";

		if (state & STUDIO_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & STUDIO_SHADOW_CASTER_ENABLED)
			defs << "#define SHADOW_CASTER_ENABLED\n";

		if (state & STUDIO_LEGACY_BONE_ENABLED)
			defs << "#define LEGACY_BONE_ENABLED\n";

		if (state & STUDIO_GLOW_SHELL_ENABLED)
			defs << "#define GLOW_SHELL_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\studio_shader.vsh", "renderer\\shader\\studio_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, bonematrix, "bonematrix");

			SHADER_UNIFORM(prog, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(prog, v_lambert, "v_lambert");
			SHADER_UNIFORM(prog, v_brightness, "v_brightness");
			SHADER_UNIFORM(prog, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(prog, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(prog, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(prog, r_blend, "r_blend");
			SHADER_UNIFORM(prog, r_g1, "r_g1");
			SHADER_UNIFORM(prog, r_g3, "r_g3");
			SHADER_UNIFORM(prog, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(prog, r_colormix, "r_colormix");
			SHADER_UNIFORM(prog, r_origin, "r_origin");
			SHADER_UNIFORM(prog, r_vright, "r_vright");
			SHADER_UNIFORM(prog, r_scale, "r_scale");
			SHADER_UNIFORM(prog, r_uvscale, "r_uvscale");
			SHADER_UNIFORM(prog, entityPos, "entityPos");
			SHADER_UNIFORM(prog, viewpos, "viewpos");

			SHADER_ATTRIB(prog, attr_bone, "attr_bone");
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

		if (prog.diffuseTex != -1)
			qglUniform1iARB(prog.diffuseTex, 0);

		if (prog.bonematrix != -1)
			qglUniformMatrix3x4fvARB(prog.bonematrix, 128, 0, (float *)(*pbonetransform));

		if (prog.r_blend != -1)
			qglUniform1fARB(prog.r_blend, (*r_blend));

		if (prog.r_g1 != -1 || prog.r_g3 != -1)
		{
			float r_g = 1.0f / v_gamma->value;

			float r_g3;
			if (v_brightness->value <= 0.0f)
				r_g3 = 0.125f;
			else if (v_brightness->value > 1.0f)
				r_g3 = 0.05f;
			else
				r_g3 = 0.125f - (v_brightness->value * v_brightness->value) * 0.075f;

			if (prog.r_g1 != -1)
				qglUniform1fARB(prog.r_g1, r_g);

			if(prog.r_g3 != -1)
				qglUniform1fARB(prog.r_g3, r_g3);
		}

		if (prog.r_ambientlight != -1)
			qglUniform1fARB(prog.r_ambientlight, (float)(*r_ambientlight));

		if (prog.r_shadelight != -1)
			qglUniform1fARB(prog.r_shadelight, (*r_shadelight));

		if (prog.v_brightness != -1)
			qglUniform1fARB(prog.v_brightness, v_brightness->value);

		if (prog.v_lightgamma != -1)
			qglUniform1fARB(prog.v_lightgamma, v_lightgamma->value);

		if (prog.v_lambert != -1)
			qglUniform1fARB(prog.v_lambert, v_lambert->value);

		if (prog.r_plightvec != -1)
			qglUniform3fARB(prog.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);

		if (prog.r_colormix != -1)
			qglUniform3fARB(prog.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);

		if (prog.r_origin != -1)
			qglUniform3fARB(prog.r_origin, g_ChromeOrigin[0], g_ChromeOrigin[1], g_ChromeOrigin[2]);

		if (prog.r_vright != -1)
			qglUniform3fARB(prog.r_vright, vright[0], vright[1], vright[2]);

		if (prog.r_scale != -1)
			qglUniform1fARB(prog.r_scale, ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME) ? (*currententity)->curstate.renderamt * 0.05f : 0);

		if (prog.entityPos != -1)
			qglUniform3fARB(prog.entityPos, (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3]);

		if (prog.viewpos != -1)
			qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseStudioProgram: Failed to load program!");
	}
}

void R_ShutdownStudio(void)
{
	g_StudioProgramTable.clear();
}

void R_InitStudio(void)
{
	r_studio_vbo = gEngfuncs.pfnRegisterVariable("r_studio_vbo", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
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

void BuildGlowShellVerts(vec3_t *pstudioverts, auxvert_t *paux)
{
	int					i;
	byte				*pvertbone;
	vec3_t				*pstudionorms;
	auxvert_t			*av;
	float				vscale;

	pvertbone = ((byte *)(*pstudiohdr) + (*psubmodel)->vertinfoindex);
	pstudionorms = (vec3_t *)((byte *)(*pstudiohdr) + (*psubmodel)->normindex);

	vscale = (*currententity)->curstate.renderamt * 0.05;

	for (i = 0; i < (*pstudiohdr)->numbones; i++)
	{
		(*chromeage)[i] = 0;
	}

	for (i = 0; i < (*psubmodel)->numverts; i++)
	{
		vec3_t vert;
		VectorMA(pstudioverts[i], vscale, pstudionorms[(*g_NormalIndex)[i]], vert);

		av = &paux[i];
		R_StudioTransformAuxVert(av, pvertbone[i], vert);
	}

	g_ChromeOrigin[0] = cos(r_glowshellfreq->value * (*cl_time)) * 4000.0;
	g_ChromeOrigin[1] = sin(r_glowshellfreq->value * (*cl_time)) * 4000.0;
	g_ChromeOrigin[2] = cos(r_glowshellfreq->value * (*cl_time) * 0.33) * 4000.0;

	qglColor4ub((*currententity)->curstate.rendercolor.r, (*currententity)->curstate.rendercolor.g, (*currententity)->curstate.rendercolor.b, 255);
}

void R_LightStrength(int bone, float *vert, float light[3][4])
{
	if (gRefFuncs.R_LightStrength)
		return gRefFuncs.R_LightStrength(bone, vert, light);
}

void R_StudioChrome(float *pchrome, int bone, vec3_t normal)
{
	float n;

	if ((*chromeage)[bone] != (*r_smodels_total))
	{
		// calculate vectors from the viewer to the bone. This roughly adjusts for position
		vec3_t chromeupvec;		// chrome t vector in world reference frame
		vec3_t chromerightvec;	// chrome s vector in world reference frame
		vec3_t tmp;				// vector pointing at bone in world reference frame
		VectorScale(g_ChromeOrigin, -1, tmp);
		tmp[0] += (*pbonetransform)[bone][0][3];
		tmp[1] += (*pbonetransform)[bone][1][3];
		tmp[2] += (*pbonetransform)[bone][2][3];
		//bonepos - chromeorg
		VectorNormalize(tmp);
		CrossProduct(tmp, vright, chromeupvec);
		VectorNormalize(chromeupvec);
		CrossProduct(tmp, chromeupvec, chromerightvec);
		VectorNormalize(chromerightvec);

		VectorIRotate(chromeupvec, (*pbonetransform)[bone], r_chromeup[bone]);
		VectorIRotate(chromerightvec, (*pbonetransform)[bone], r_chromeright[bone]);

		(*chromeage)[bone] = (*r_smodels_total);
	}
	// calc s coord
	n = DotProduct(normal, r_chromeright[bone]);
	pchrome[0] = (n + 1.0) * 1024; // FIX: make this a float

	// calc t coord
	n = DotProduct(normal, r_chromeup[bone]);
	pchrome[1] = (n + 1.0) * 1024; // FIX: make this a float
}

void R_StudioLighting(float *lv, int bone, int flags, vec3_t normal)
{
	if (gRefFuncs.R_StudioLighting)
		return gRefFuncs.R_StudioLighting(lv, bone, flags, normal);

	float 	illum;
	float	lightcos;

	illum = (*r_ambientlight);

	if (flags & STUDIO_NF_FLATSHADE)
	{
		illum += (*r_shadelight) * 0.8;
	}
	else
	{
		float r;

		if (bone != -1)
		{
			lightcos = DotProduct(normal, r_blightvec[bone]);
		}
		else
		{
			lightcos = DotProduct(normal, r_plightvec); // -1 colinear, 1 opposite
		}

		if (lightcos > 1.0)
			lightcos = 1;

		r = v_lambert->value;
		if (r < 1.0)
		{
			lightcos = (r - lightcos) / (r + 1.0f); 		// do modified hemispherical lighting
			if (lightcos > 0.0)
			{
				illum += (*r_shadelight) * lightcos;
			}
		}
		else
		{
			illum += (*r_shadelight);
			lightcos = (lightcos + (r - 1.0)) / r; 		// do modified hemispherical lighting
			if (lightcos > 0.0)
			{
				illum -= (*r_shadelight) * lightcos;
			}
		}

		if (illum <= 0)
			illum = 0;
	}

	if (illum > 255)
		illum = 255;

	*lv = lightgammatable[(int)(illum * 4)] / 1023.0;	// Light from 0 to 1.0
}

void studioapi_SetupModel(int bodypart, void **ppbodypart, void **ppsubmodel)
{
	gRefFuncs.studioapi_SetupModel(bodypart, ppbodypart, ppsubmodel);
}

void R_GLStudioDrawPoints(void)
{
	int i, j, k;
	byte *pvertbone;
	byte *pnormbone;
	vec3_t *pstudioverts;
	vec3_t *pstudionorms;
	studiohdr_t *ptexturehdr;
	mstudiotexture_t *ptexture;
	mstudiomesh_t *pmesh;
	auxvert_t *av;
	float *lv;
	vec3_t fl;
	float lv_tmp;
	short *pskinref;
	int flags;

	int stencilState = 1;

	if (r_draw_pass == r_draw_shadow_caster)
	{
		//the fxxking StudioRenderFinal which will enable GL_BLEND and mess everything up.
		qglDisable(GL_BLEND);
	}
	else if (r_draw_nontransparent)
	{
		qglEnable(GL_STENCIL_TEST);
		qglStencilMask(0xFF);
		qglStencilFunc(GL_ALWAYS, stencilState, 0xFF);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	auto engine_pauxverts = (*pauxverts);
	auto engine_pvlightvalues = (*pvlightvalues);
	auto engine_pstudiohdr = (*pstudiohdr);
	auto engine_psubmodel = (*psubmodel);

	pvertbone = ((byte *)engine_pstudiohdr + engine_psubmodel->vertinfoindex);
	pnormbone = ((byte *)engine_pstudiohdr + engine_psubmodel->norminfoindex);
	ptexturehdr = R_LoadTextures(*r_model);
	ptexture = (mstudiotexture_t *)((byte *)ptexturehdr + ptexturehdr->textureindex);

	pmesh = (mstudiomesh_t *)((byte *)engine_pstudiohdr + engine_psubmodel->meshindex);

	pstudioverts = (vec3_t *)((byte *)engine_pstudiohdr + engine_psubmodel->vertindex);
	pstudionorms = (vec3_t *)((byte *)engine_pstudiohdr + engine_psubmodel->normindex);

	pskinref = (short *)((byte *)ptexturehdr + ptexturehdr->skinindex);

	int iFlippedVModel = 0;

	studio_vbo_t *VBOData = NULL;

	if (r_studio_vbo->value)
	{
		VBOData = R_StudioPrepareVBOSubmodel(engine_pstudiohdr, engine_psubmodel);
	}

	if ((*currententity)->curstate.skin != 0 && (*currententity)->curstate.skin < ptexturehdr->numskinfamilies)
		pskinref += ((*currententity)->curstate.skin * ptexturehdr->numskinref);

	if (engine_pstudiohdr->numbones > MAXSTUDIOBONES)
	{
		Sys_ErrorEx("R_GLStudioDrawPoints: %s numbones (%d) > MAXSTUDIOBONES (%d)", engine_pstudiohdr->name, engine_pstudiohdr->numbones, MAXSTUDIOBONES);
	}

	//Setup light, chrome...
	if (VBOData)
	{
		if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
		{
			g_ChromeOrigin[0] = cos(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
			g_ChromeOrigin[1] = sin(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
			g_ChromeOrigin[2] = cos(r_glowshellfreq->value * (*cl_time) * 0.33f) * 4000.0f;

			r_colormix[0] = (float)(*currententity)->curstate.rendercolor.r / 255.0f;
			r_colormix[1] = (float)(*currententity)->curstate.rendercolor.g / 255.0f;
			r_colormix[2] = (float)(*currententity)->curstate.rendercolor.b / 255.0f;
		}
	}
	else
	{
		if (engine_psubmodel->numverts > MAXSTUDIOVERTS)
		{
			Sys_ErrorEx("R_GLStudioDrawPoints: %s -> %s numverts (%d) > MAXSTUDIOVERTS (%d)", engine_pstudiohdr->name, engine_psubmodel->name, engine_psubmodel->numverts, MAXSTUDIOVERTS);
		}

		if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
		{
			BuildNormalIndexTable();
			BuildGlowShellVerts(pstudioverts, engine_pauxverts);
		}
		else
		{
			for (i = 0; i < engine_psubmodel->numverts; i++)
			{
				av = &(engine_pauxverts[i]);
				R_StudioTransformAuxVert(av, pvertbone[i], pstudioverts[i]);
			}
		}

		for (i = 0; i < engine_psubmodel->numverts; i++)
		{
			R_LightStrength(pvertbone[i], pstudioverts[i], lightpos[i]);
		}

		lv = engine_pvlightvalues;

		for (j = 0; j < engine_psubmodel->nummesh; j++)
		{
			flags = ptexture[pskinref[pmesh[j].skinref]].flags | (*g_ForcedFaceFlags);

			if (r_fullbright->value >= 2)
				flags |= STUDIO_NF_FULLBRIGHT;

			if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				for (k = 0; k < pmesh[j].numnorms; k++, lv += 3, pstudionorms++, pnormbone++)
				{
					lv[0] = *r_blend;
					lv[1] = *r_blend;
					lv[2] = *r_blend;

					if (flags & STUDIO_NF_CHROME)
					{
						int m = (int)((char *)lv - (char *)engine_pvlightvalues) / 12;
						R_StudioChrome(r_chrome[m], *pnormbone, *pstudionorms);
					}
				}
			}
			else
			{
				for (k = 0; k < pmesh[j].numnorms; k++, lv += 3, pstudionorms++, pnormbone++)
				{
					gRefFuncs.R_StudioLighting(&lv_tmp, *pnormbone, flags, *pstudionorms);
					//R_StudioLighting(&lv_tmp, *pnormbone, flags, *pstudionorms);

					if (flags & STUDIO_NF_CHROME)
					{
						int m = (int)((char *)lv - (char *)engine_pvlightvalues) / 12;
						R_StudioChrome(r_chrome[m], *pnormbone, *pstudionorms);
					}

					lv[0] = lv_tmp * r_colormix[0];
					lv[1] = lv_tmp * r_colormix[1];
					lv[2] = lv_tmp * r_colormix[2];
				}
			}
		}
	}

	qglCullFace(GL_FRONT);

	if (R_IsFlippedViewModel())
	{
		qglDisable(GL_CULL_FACE);
		iFlippedVModel = 1;
	}

	pstudionorms = (vec3_t *)((byte *)engine_pstudiohdr + engine_psubmodel->normindex);
	pnormbone = ((byte *)engine_pstudiohdr + engine_psubmodel->norminfoindex);

	if (VBOData)
	{
		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, VBOData->hDataBuffer);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, VBOData->hIndexBuffer);
		qglVertexPointer(3, GL_FLOAT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, pos));
		qglNormalPointer(GL_FLOAT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, normal));

		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, texcoord));

		for (j = 0; j < engine_psubmodel->nummesh; j++)
		{
			auto &VBOMesh = VBOData->vMesh[j];

			pmesh = (mstudiomesh_t *)((byte *)engine_pstudiohdr + engine_psubmodel->meshindex) + j;

			r_studio_polys += pmesh->numtris;

			flags = ptexture[pskinref[pmesh->skinref]].flags | (*g_ForcedFaceFlags);

			if (r_fullbright->value >= 2)
			{
				flags = flags & 0xFC;
			}

			int GBufferMask = GBUFFER_MASK_ALL;
			int StudioProgramState = flags;

			if (r_draw_pass == r_draw_shadow_caster)
			{
				StudioProgramState |= STUDIO_SHADOW_CASTER_ENABLED;
			}
			else if (r_draw_nontransparent)
			{
				if (flags & STUDIO_NF_FLATSHADE)
				{
					if (stencilState != 2)
					{
						stencilState = 2;
						qglStencilFunc(GL_ALWAYS, stencilState, 0xFF);
					}
				}
				else
				{
					if (stencilState != 1)
					{
						stencilState = 1;
						qglStencilFunc(GL_ALWAYS, stencilState, 0xFF);
					}
				}
			}

			if (r_draw_pass == r_draw_shadow_caster)
			{

			}
			else if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);

				GBufferMask = GBUFFER_MASK_ADDITIVE;
				StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE;
			}
			else if (flags & STUDIO_NF_MASKED)
			{
				qglEnable(GL_ALPHA_TEST);
				qglAlphaFunc(GL_GREATER, 0.5);
				qglDepthMask(GL_TRUE);

				//StudioProgramState |= STUDIO_NF_MASKED;
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);

				GBufferMask = GBUFFER_MASK_ADDITIVE;
				StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE;
			}
			else if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglShadeModel(GL_SMOOTH);

				GBufferMask = GBUFFER_MASK_ADDITIVE;
				StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE | STUDIO_TRANSADDITIVE_ENABLED;
			}

			if (drawgbuffer)
			{
				StudioProgramState |= STUDIO_GBUFFER_ENABLED;
			}

			if (r_fog_mode == GL_LINEAR)
			{
				StudioProgramState |= STUDIO_LINEAR_FOG_ENABLED;
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
			R_SetGBufferMask(GBufferMask);

			if (prog.r_uvscale != -1)
			{
				qglUniform2fARB(prog.r_uvscale, s, t);
			}
			
			if (prog.attr_bone != -1)
			{
				qglEnableVertexAttribArray(prog.attr_bone);
				qglVertexAttribIPointer(prog.attr_bone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
			}

			if (VBOMesh.iIndiceCount)
			{
				qglDrawElements(GL_TRIANGLES, VBOMesh.iIndiceCount, GL_UNSIGNED_INT, BUFFER_OFFSET(VBOMesh.iStartIndex));
				
				++r_studio_drawcall;
			}

			if (r_draw_pass == r_draw_shadow_caster)
			{
				
			}
			else if (flags & STUDIO_NF_MASKED)
			{
				qglAlphaFunc(GL_NOTEQUAL, 0);
				qglDisable(GL_ALPHA_TEST);
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglDisable(GL_BLEND);
				qglDepthMask(1);
				qglShadeModel(GL_FLAT);
			}
			else if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				qglDisable(GL_BLEND);
				qglShadeModel(GL_FLAT);
			}

			if (prog.attr_bone != -1)
			{
				qglDisableVertexAttribArray(prog.attr_bone);
			}
		}

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		qglDisableClientState(GL_VERTEX_ARRAY);
		qglDisableClientState(GL_NORMAL_ARRAY);
		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
	else
	{
		for (j = 0; j < engine_psubmodel->nummesh; j++)
		{
			pmesh = (mstudiomesh_t *)((byte *)engine_pstudiohdr + engine_psubmodel->meshindex) + j;

			auto ptricmds = (short *)((byte *)engine_pstudiohdr + pmesh->triindex);

			r_studio_polys += pmesh->numtris;

			flags = ptexture[pskinref[pmesh->skinref]].flags | (*g_ForcedFaceFlags);

			if (r_fullbright->value >= 2)
			{
				flags = flags & 0xFC;
			}

			int GBufferMask = GBUFFER_MASK_ALL;
			int StudioProgramState = STUDIO_LEGACY_BONE_ENABLED;

			if (r_draw_pass == r_draw_shadow_caster)
			{
				StudioProgramState |= STUDIO_SHADOW_CASTER_ENABLED;
			}
			else if (r_draw_nontransparent)
			{
				if (flags & STUDIO_NF_FLATSHADE)
				{
					if (stencilState != 2)
					{
						stencilState = 2;
						qglStencilFunc(GL_ALWAYS, stencilState, 0xFF);
					}
				}
				else
				{
					if (stencilState != 1)
					{
						stencilState = 1;
						qglStencilFunc(GL_ALWAYS, stencilState, 0xFF);
					}
				}
			}

			if (r_draw_pass == r_draw_shadow_caster)
			{

			}
			else if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);

				GBufferMask = GBUFFER_MASK_ADDITIVE;
				StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE;
			}
			else if (flags & STUDIO_NF_MASKED)
			{
				qglEnable(GL_ALPHA_TEST);
				qglAlphaFunc(GL_GREATER, 0.5);
				qglDepthMask(GL_TRUE);

				//StudioProgramState |= STUDIO_NF_MASKED;
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);

				GBufferMask = GBUFFER_MASK_ADDITIVE;
				StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE;
			}
			else if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglShadeModel(GL_SMOOTH);

				GBufferMask = GBUFFER_MASK_ADDITIVE;
				StudioProgramState |= STUDIO_TRANSPARENT_ENABLED | STUDIO_NF_ADDITIVE | STUDIO_TRANSADDITIVE_ENABLED;
			}

			if (drawgbuffer)
			{
				StudioProgramState |= STUDIO_GBUFFER_ENABLED;
			}

			if (r_fog_mode == GL_LINEAR)
			{
				StudioProgramState |= STUDIO_LINEAR_FOG_ENABLED;
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

			studio_program_t prog = { 0 };

			R_UseStudioProgram(StudioProgramState, &prog);
			R_SetGBufferMask(GBufferMask);

			if (flags & STUDIO_NF_CHROME)//chrome start
			{
				if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)//force chrome, the fucking glowshell
				{
					s *= 1.0f / 32.0f;
					t *= 1.0f / 32.0f;

					while (i = *(ptricmds++))
					{
						if (i < 0)
						{
							qglBegin(GL_TRIANGLE_FAN);
							i = -i;
						}
						else
						{
							qglBegin(GL_TRIANGLE_STRIP);
						}

						for (; i > 0; i--, ptricmds += 4)
						{
							int normalIndex = (*g_NormalIndex)[ptricmds[0]];

							qglTexCoord2f(r_chrome[normalIndex][0] * s, r_chrome[normalIndex][1] * t);

							VectorRotate(pstudionorms[ptricmds[1]], (*pbonetransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[1]]);

							qglNormal3fv(r_studionormal[ptricmds[1]]);

							vec4_t col;
							col[0] = (float)(*currententity)->curstate.rendercolor.r / 255.0f;
							col[1] = (float)(*currententity)->curstate.rendercolor.g / 255.0f;
							col[2] = (float)(*currententity)->curstate.rendercolor.b / 255.0f;
							col[3] = 1.0f;
							qglColor4fv(col);

							av = &engine_pauxverts[ptricmds[0]];
							qglVertex3fv(av->fv);
						}

						qglEnd();

						r_studio_drawcall++;
					}
				}
				else
				{//standard chrome, not glowshell
					s *= 1.0f / 2048.0f;
					t *= 1.0f / 2048.0f;
					s *= (float)ptexture[pskinref[pmesh->skinref]].width;
					t *= (float)ptexture[pskinref[pmesh->skinref]].height;

					while (i = *(ptricmds++))
					{
						if (i < 0)
						{
							qglBegin(GL_TRIANGLE_FAN);
							i = -i;
						}
						else
						{
							qglBegin(GL_TRIANGLE_STRIP);
						}

						for (; i > 0; i--, ptricmds += 4)
						{
							qglTexCoord2f(r_chrome[ptricmds[1]][0] * s, r_chrome[ptricmds[1]][1] * t);

							lv = &engine_pvlightvalues[ptricmds[1] * 3];

							vec3_t vNormal;
							VectorCopy(pstudionorms[ptricmds[1]], vNormal);

							if (iFlippedVModel == 1)
								VectorInverse(vNormal);

							gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], vNormal, lv, fl);

							VectorRotate(pstudionorms[ptricmds[1]], (*pbonetransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[1]]);

							qglNormal3fv(r_studionormal[ptricmds[1]]);
							qglColor4f(fl[0], fl[1], fl[2], *r_blend);

							av = &engine_pauxverts[ptricmds[0]];
							qglVertex3fv(av->fv);
						}

						qglEnd();

						r_studio_drawcall++;
					}
				}//no force chrome end

			}//chrome end
			else
			{//normal render

				while (i = *(ptricmds++))
				{
					if (i < 0)
					{
						qglBegin(GL_TRIANGLE_FAN);
						i = -i;
					}
					else
					{
						qglBegin(GL_TRIANGLE_STRIP);
					}

					for (; i > 0; i--, ptricmds += 4)
					{
						vec2_t st = { ptricmds[2] * s, ptricmds[3] * t };
						qglTexCoord2fv(st);

						lv = &(engine_pvlightvalues[ptricmds[1] * 3]);

						vec3_t vNormal;
						VectorCopy(pstudionorms[ptricmds[1]], vNormal);

						if (iFlippedVModel)
							VectorInverse(vNormal);

						gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], vNormal, lv, fl);

						VectorRotate(pstudionorms[ptricmds[1]], (*pbonetransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[1]]);

						qglNormal3fv(r_studionormal[ptricmds[1]]);

						if (flags & STUDIO_NF_FULLBRIGHT)
						{
							qglColor4f(1, 1, 1, *r_blend);
						}
						else
						{
							qglColor4f(fl[0], fl[1], fl[2], *r_blend);
						}

						av = &(engine_pauxverts[ptricmds[0]]);
						qglVertex3fv(av->fv);
					}

					qglEnd();

					r_studio_drawcall++;
				}

			}//normal draw end

			if (r_draw_pass == r_draw_shadow_caster)
			{

			}
			else if (flags & STUDIO_NF_MASKED)
			{
				qglAlphaFunc(GL_NOTEQUAL, 0);
				qglDisable(GL_ALPHA_TEST);
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglDisable(GL_BLEND);
				qglDepthMask(1);
				qglShadeModel(GL_FLAT);
			}
			else if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				qglDisable(GL_BLEND);
				qglShadeModel(GL_FLAT);
			}

		}//mesh draw end

	}//non-VBO way

	qglEnable(GL_CULL_FACE);

	if (r_draw_nontransparent)
	{
		qglStencilMask(0);
		qglDisable(GL_STENCIL_TEST);
	}

	GL_UseProgram(0);
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
	qglDepthMask(1);
	gRefFuncs.studioapi_RestoreRenderer();
}