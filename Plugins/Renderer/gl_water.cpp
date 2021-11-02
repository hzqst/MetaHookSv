#include "gl_local.h"
#include <sstream>

//GLuint64 refractmap_handle = 0;
//GLuint64 depthrefrmap_handle = 0;
bool refractmap_ready = false;

vec3_t water_view;

water_vbo_t *curwater;

//cvar
cvar_t *r_water = NULL;
cvar_t *r_water_forcetrans = NULL;
cvar_t *r_water_debug = NULL;

std::vector<water_vbo_t *> g_WaterVBOCache;
water_vbo_t *g_RenderWaterVBOCache[512];
int g_iNumRenderWaterVBOCache = 0;

std::unordered_map<int, water_program_t> g_WaterProgramTable;

std::vector<water_control_t> r_water_controls;

//std::vector<cubemap_t> r_cubemaps;

void R_UseWaterProgram(int state, water_program_t *progOutput)
{
	water_program_t prog = { 0 };

	auto itor = g_WaterProgramTable.find(state);
	if (itor == g_WaterProgramTable.end())
	{
		std::stringstream defs;

		if (state & WATER_LEGACY_ENABLED)
			defs << "#define LEGACY_ENABLED\n";

		if (state & WATER_UNDERWATER_ENABLED)
			defs << "#define UNDERWATER_ENABLED\n";

		if (state & WATER_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if (state & WATER_DEPTH_ENABLED)
			defs << "#define DEPTH_ENABLED\n";

		if (state & WATER_REFRACT_ENABLED)
			defs << "#define REFRACT_ENABLED\n";

		if (state & WATER_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & WATER_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

		if (state & WATER_BINDLESS_ENABLED)
			defs << "#define BINDLESS_ENABLED\n";

		if (state & WATER_OIT_ALPHA_BLEND_ENABLED)
			defs << "#define OIT_ALPHA_BLEND_ENABLED\n";

		if (state & WATER_OIT_ADDITIVE_BLEND_ENABLED)
			defs << "#define OIT_ADDITIVE_BLEND_ENABLED\n";

		if (glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define UINT64_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\water_shader.vsh", "renderer\\shader\\water_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, u_watercolor, "u_watercolor");
			SHADER_UNIFORM(prog, u_depthfactor, "u_depthfactor");
			SHADER_UNIFORM(prog, u_fresnelfactor, "u_fresnelfactor");
			SHADER_UNIFORM(prog, u_normfactor, "u_normfactor");
			SHADER_UNIFORM(prog, u_scale, "u_scale");
		}

		g_WaterProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		g_pMetaHookAPI->SysError("R_UseWaterProgram: Failed to load program!");
	}
}

const program_state_name_t s_WaterProgramStateName[] = {
{ WATER_LEGACY_ENABLED					, "WATER_LEGACY_ENABLED"			 },
{ WATER_UNDERWATER_ENABLED				, "WATER_UNDERWATER_ENABLED"		 },
{ WATER_GBUFFER_ENABLED					, "WATER_GBUFFER_ENABLED"			 },
{ WATER_DEPTH_ENABLED					, "WATER_DEPTH_ENABLED"				 },
{ WATER_REFRACT_ENABLED					, "WATER_REFRACT_ENABLED"			 },
{ WATER_LINEAR_FOG_ENABLED				, "WATER_LINEAR_FOG_ENABLED"		 },
{ WATER_EXP2_FOG_ENABLED				, "WATER_EXP2_FOG_ENABLED"		 },
{ WATER_BINDLESS_ENABLED				, "WATER_BINDLESS_ENABLED"			 },
{ WATER_OIT_ALPHA_BLEND_ENABLED			, "WATER_OIT_ALPHA_BLEND_ENABLED"	 },
{ WATER_OIT_ADDITIVE_BLEND_ENABLED		, "WATER_OIT_ADDITIVE_BLEND_ENABLED" },
};

void R_SaveWaterProgramStates(void)
{
	std::stringstream ss;
	for (auto &p : g_WaterProgramTable)
	{
		if (p.first == 0)
		{
			ss << "NONE";
		}
		else
		{
			for (int i = 0; i < _ARRAYSIZE(s_WaterProgramStateName); ++i)
			{
				if (p.first & s_WaterProgramStateName[i].state)
				{
					ss << s_WaterProgramStateName[i].name << " ";
				}
			}
		}
		ss << "\n";
	}

	auto FileHandle = g_pFileSystem->Open("renderer/shader/water_cache.txt", "wt");
	if (FileHandle)
	{
		auto str = ss.str();
		g_pFileSystem->Write(str.data(), str.length(), FileHandle);
		g_pFileSystem->Close(FileHandle);
	}
}

void R_LoadWaterProgramStates(void)
{
	auto FileHandle = g_pFileSystem->Open("renderer/shader/water_cache.txt", "rt");
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
						for (int i = 0; i < _ARRAYSIZE(s_WaterProgramStateName); ++i)
						{
							if (!strcmp(token, s_WaterProgramStateName[i].name))
							{
								if (ProgramState == -1)
									ProgramState = 0;
								ProgramState |= s_WaterProgramStateName[i].state;
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
				R_UseWaterProgram(ProgramState, NULL);
		}
		g_pFileSystem->Close(FileHandle);
	}

	GL_UseProgram(0);
}

void R_ShutdownWater(void)
{
	g_WaterProgramTable.clear();
}

void R_InitWater(void)
{
	r_water = gEngfuncs.pfnRegisterVariable("r_water", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_forcetrans = gEngfuncs.pfnRegisterVariable("r_water_forcetrans", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_debug = gEngfuncs.pfnRegisterVariable("r_water_debug", "0", FCVAR_CLIENTDLL);
}

void R_NewMapWater(void)
{
	for (size_t i = 0; i < g_WaterVBOCache.size(); ++i)
	{
		auto VBOCache = g_WaterVBOCache[i];
		if (VBOCache->depthreflmap)
		{
			GL_DeleteTexture(VBOCache->depthreflmap);
		}
		if (VBOCache->reflectmap)
		{
			GL_DeleteTexture(VBOCache->reflectmap);
		}
		if (VBOCache->hEBO)
		{
			GL_DeleteBuffer(VBOCache->hEBO);
		}
		/*if (VBOCache->hTextureSSBO)
		{
			GL_DeleteBuffer(VBOCache->hTextureSSBO);
		}*/
		delete VBOCache;
	}

	g_WaterVBOCache.clear();
}

bool R_IsAboveWater(water_vbo_t *water)
{
	float org[4] = { (*r_refdef.vieworg)[0], (*r_refdef.vieworg)[1], (*r_refdef.vieworg)[2], 1 };
	float equation[4] = { water->normal[0], water->normal[1], water->normal[2], -water->plane };
	return DotProduct4(org, equation) > 0;
}

water_vbo_t *R_FindFlatWaterVBO(cl_entity_t *ent, msurface_t *surf, int direction)
{
	auto poly = surf->polys;

	auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

	vec3_t normal;
	VectorCopy(brushface->normal, normal);

	if (direction)
		VectorInverse(normal);

	for (size_t i = 0; i < g_WaterVBOCache.size(); ++i)
	{
		auto cache = g_WaterVBOCache[i];
		if ((cache->ent == ent || cache->ent->curstate.rendermode == ent->curstate.rendermode) &&
			surf->texinfo->texture == cache->texture &&
			cache->normal[2] == normal[2])
		{
			bool bSkip = false;
			for (int j = 0; j < poly->numverts; ++j)
			{
				auto plane = DotProduct(poly->verts[i], normal);
				if (fabs(cache->plane - plane) > 0.1f)
				{
					bSkip = true;
					break;
				}
			}

			if(!bSkip)
				return cache;
		}
	}

	return NULL;
}

water_control_t *R_FindWaterControl(cl_entity_t *ent, msurface_t *surf)
{
	water_control_t *pControl = NULL;

	for (size_t i = 0; i < r_water_controls.size(); ++i)
	{
		auto &control = r_water_controls[i];

		if (control.basetexture[0] == '*')
		{
			pControl = &control;
			break;
		}
		else if (control.wildcard.length() > 0)
		{
			if (!strncmp(control.wildcard.c_str(), surf->texinfo->texture->name, control.wildcard.length()))
			{
				pControl = &control;
				break;
			}
		}
		else
		{
			if (!strcmp(control.basetexture.c_str(), surf->texinfo->texture->name))
			{
				pControl = &control;
				break;
			}
		}
	}

	if (pControl && ent != r_worldentity && ent->model->maxs[2] - ent->model->mins[2] < pControl->minheight)
	{
		return NULL;
	}

	return pControl;
}

water_vbo_t *R_PrepareWaterVBO(cl_entity_t *ent, msurface_t *surf, int direction)
{
	water_vbo_t *VBOCache = NULL;

	if (!surf->lightmaptexturenum)
	{
		//Find flat WaterVBOCache with same texture and same height

		auto poly = surf->polys;

		auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

		VBOCache = R_FindFlatWaterVBO(ent, surf, direction);

		if (!VBOCache)
		{
			VBOCache = new water_vbo_t;
			VBOCache->index = g_WaterVBOCache.size();
			VBOCache->ent = ent;
			VBOCache->texture = surf->texinfo->texture;

			VBOCache->depthreflmap = GL_GenDepthStencilTexture(glwidth, glheight);
			VBOCache->reflectmap = GL_GenTextureRGBA8(glwidth, glheight);

			/*if (bUseBindless)
			{
				auto handle = glGetTextureHandleARB(VBOCache->texture->gl_texturenum);
				glMakeTextureHandleResidentARB(handle);

				VBOCache->basetexture_handle = handle;
			}

			if (bUseBindless)
			{
				auto handle = glGetTextureHandleARB(VBOCache->reflectmap);
				glMakeTextureHandleResidentARB(handle);

				VBOCache->reflectmap_handle = handle;
			}

			if (bUseBindless && !refractmap_handle)
			{
				auto handle = glGetTextureHandleARB(s_WaterFBO.s_hBackBufferTex);
				glMakeTextureHandleResidentARB(handle);

				refractmap_handle = handle;
			}

			if (bUseBindless && !depthrefrmap_handle)
			{
				auto handle = glGetTextureHandleARB(s_WaterFBO.s_hBackBufferDepthTex);
				glMakeTextureHandleResidentARB(handle);

				depthrefrmap_handle = handle;
			}*/

			VBOCache->hEBO = GL_GenBuffer();
			//VBOCache->hTextureSSBO = GL_GenBuffer();

			VectorCopy(poly->verts[0], VBOCache->vert);
			VectorCopy(brushface->normal, VBOCache->normal);
			
			if (direction)
			{
				VectorInverse(VBOCache->normal);
			}

			VBOCache->plane = DotProduct(VBOCache->normal, VBOCache->vert);

			auto pSourcePalette = surf->texinfo->texture->pPal;
			VBOCache->color.r = pSourcePalette[9];
			VBOCache->color.g = pSourcePalette[10];
			VBOCache->color.b = pSourcePalette[11];
			VBOCache->color.a = 255;

			//Default
			VBOCache->normalmap = 0;
			VBOCache->fresnelfactor[0] = 0;
			VBOCache->fresnelfactor[1] = 0;
			VBOCache->fresnelfactor[2] = 0;
			VBOCache->depthfactor[0] = 0;
			VBOCache->depthfactor[1] = 0;
			VBOCache->depthfactor[2] = 0;
			VBOCache->normfactor = 0;
			VBOCache->minheight = 0;
			VBOCache->maxtrans = 1;
			VBOCache->level = WATER_LEVEL_LEGACY;

			auto waterControl = R_FindWaterControl(ent, surf);
			if (waterControl)
			{
				if (waterControl->level >= WATER_LEVEL_REFLECT_SKYBOX && waterControl->level <= WATER_LEVEL_REFLECT_ENTITY)
				{
					int found_normalmap = GL_FindTexture(waterControl->normalmap.c_str(), GLT_WORLD, NULL, NULL);

					//Disable mimap for normal texture
					VBOCache->normalmap = found_normalmap ? found_normalmap :
						R_LoadTextureEx(waterControl->normalmap.c_str(), waterControl->normalmap.c_str(), NULL, NULL, GLT_WORLD, false, true);
				}

				if (VBOCache->normalmap)
				{
					/*if (bUseBindless)
					{
						auto handle = glGetTextureHandleARB(VBOCache->normalmap);
						glMakeTextureHandleResidentARB(handle);

						VBOCache->normalmap_handle = handle;
					}*/
				}

				VBOCache->fresnelfactor[0] = waterControl->fresnelfactor[0];
				VBOCache->fresnelfactor[1] = waterControl->fresnelfactor[1];
				VBOCache->fresnelfactor[2] = waterControl->fresnelfactor[2];
				VBOCache->depthfactor[0] = waterControl->depthfactor[0];
				VBOCache->depthfactor[1] = waterControl->depthfactor[1];
				VBOCache->depthfactor[2] = waterControl->depthfactor[2];
				VBOCache->normfactor = waterControl->normfactor;
				VBOCache->minheight = waterControl->minheight;
				VBOCache->maxtrans = waterControl->maxtrans;
				VBOCache->level = waterControl->level;
			}
			/*
			GLuint64 ssbo[5] = {
				VBOCache->basetexture_handle,
				VBOCache->normalmap_handle,
				VBOCache->reflectmap_handle,
				refractmap_handle,
				depthrefrmap_handle
			};

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, VBOCache->hTextureSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo), &ssbo, GL_STATIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			*/

			g_WaterVBOCache.emplace_back(VBOCache);
		}

		surf->lightmaptexturenum = VBOCache->index + 1;

		for (int j = 0; j < brushface->num_polys; ++j)
		{
			for (int k = 0; k < brushface->num_vertexes[j]; ++k)
			{
				VBOCache->vIndicesBuffer.emplace_back(brushface->start_vertex[j] + k);
			}
			VBOCache->vIndicesBuffer.emplace_back((GLuint)0xFFFFFFFF);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOCache->hEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * VBOCache->vIndicesBuffer.size(), VBOCache->vIndicesBuffer.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		VBOCache->iPolyCount += brushface->num_polys;
	}
	else
	{
		VBOCache = g_WaterVBOCache[surf->lightmaptexturenum - 1];
	}

	return VBOCache;
}

void R_RenderReflectView(water_vbo_t *w)
{
	curwater = w;
	r_draw_pass = r_draw_reflect;

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, w->reflectmap, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, w->depthreflmap, 0);

	glClearColor(w->color.r / 255.0f, w->color.g / 255.0f, w->color.b / 255.0f, 1);
	glStencilMask(0xFF);
	glClearStencil(0);
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glStencilMask(0);

	R_PushRefDef();

	VectorCopy((*r_refdef.vieworg), water_view);

	float vForward[3], vRight[3], vUp[3];
	gEngfuncs.pfnAngleVectors((*r_refdef.viewangles), vForward, vRight, vUp);

	float flDist = fabs(w->vert[2] - (*r_refdef.vieworg)[2]);
	VectorMA((*r_refdef.vieworg), -2 * flDist, w->normal, (*r_refdef.vieworg));

	float neg_norm[3];
	VectorCopy(w->normal, neg_norm);
	VectorInverse(neg_norm);

	float flDist2 = DotProduct(vForward, neg_norm);
	VectorMA(vForward, -2 * flDist2, neg_norm, vForward);

	(*r_refdef.viewangles)[0] = -asin(vForward[2]) / M_PI * 180;
	(*r_refdef.viewangles)[1] = atan2(vForward[1], vForward[0]) / M_PI * 180;
	(*r_refdef.viewangles)[2] = -(*r_refdef.viewangles)[2];

	auto saved_cl_waterlevel = *cl_waterlevel;
	*cl_waterlevel = 0;

	auto saved_r_wsurf_sky_occlusion = r_wsurf_sky_occlusion->value;
	r_wsurf_sky_occlusion->value = 0;

	auto saved_r_drawentities = r_drawentities->value;
	if (curwater->level == WATER_LEVEL_REFLECT_ENTITY)
	{
		r_drawentities->value = 1;
	}
	else
	{
		r_drawentities->value = 0;
	}

	gRefFuncs.R_RenderScene();

	r_wsurf_sky_occlusion->value = saved_r_wsurf_sky_occlusion;
	r_drawentities->value = saved_r_drawentities;
	*cl_waterlevel = saved_cl_waterlevel;

	R_PopRefDef();

	r_draw_pass = r_draw_normal;
	curwater = NULL;
}

void R_RenderWaterView(void)
{
	refractmap_ready = false;

	if (g_WaterVBOCache.size())
	{
		static glprofile_t profile_RenderWaterView;
		GL_BeginProfile(&profile_RenderWaterView, "R_RenderWaterView");

		glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		for (size_t i = 0; i < g_WaterVBOCache.size(); ++i)
		{
			if (g_WaterVBOCache[i]->normal[2] > 0 &&
				R_IsAboveWater(g_WaterVBOCache[i]) && 
				g_WaterVBOCache[i]->framecount >= (*r_framecount) - 10)
			{
				R_RenderReflectView(g_WaterVBOCache[i]);
			}
		}

		GL_EndProfile(&profile_RenderWaterView);
	}
}