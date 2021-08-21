#include "gl_local.h"
#include <sstream>

GLuint refractmap = 0;
GLuint depthrefrmap = 0;
GLuint64 refractmap_handle = 0;
GLuint64 depthrefrmap_handle = 0;
bool refractmap_ready = false;

int saved_cl_waterlevel;

vec3_t water_view;

water_vbo_t *curwater;

//cvar
cvar_t *r_water = NULL;
cvar_t *r_water_debug = NULL;

std::vector<water_vbo_t *> g_WaterVBOCache;

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

		if (state & WATER_BINDLESS_ENABLED)
			defs << "#definen BINDLESS_ENABLED\n";

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
		Sys_ErrorEx("R_UseWaterProgram: Failed to load program!");
	}
}

void R_FreeWater(void)
{
	if (refractmap)
	{
		GL_DeleteTexture(refractmap);
		refractmap = 0;
		refractmap_handle = 0;
	}
	if (depthrefrmap)
	{
		GL_DeleteTexture(depthrefrmap);
		depthrefrmap = 0;
		depthrefrmap_handle = 0;
	}

	g_WaterProgramTable.clear();
}

void R_InitWater(void)
{
	r_water = gEngfuncs.pfnRegisterVariable("r_water", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_debug = gEngfuncs.pfnRegisterVariable("r_water_debug", "0", FCVAR_CLIENTDLL);

	if (!refractmap)
	{
		refractmap = GL_GenTextureRGBA8(glwidth, glheight);

		auto handle = glGetTextureHandleARB(refractmap);
		glMakeTextureHandleResidentARB(handle);

		refractmap_handle = handle;
	}

	if (!depthrefrmap)
	{
		depthrefrmap = GL_GenDepthTexture(glwidth, glheight);

		auto handle = glGetTextureHandleARB(depthrefrmap);
		glMakeTextureHandleResidentARB(handle);

		depthrefrmap_handle = handle;
	}
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
		if (VBOCache->hTextureSSBO)
		{
			GL_DeleteBuffer(VBOCache->hTextureSSBO);
		}
		delete VBOCache;
	}

	g_WaterVBOCache.clear();
}

bool R_IsAboveWater(float *v, float *n)
{
	float dir[3];
	VectorSubtract(r_refdef->vieworg, v, dir);
	return DotProduct(dir, n) > 0;
}

water_vbo_t *R_FindWaterVBOFlat(cl_entity_t *ent, msurface_t *surf)
{
	auto poly = surf->polys;

	auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

	for (size_t i = 0; i < g_WaterVBOCache.size(); ++i)
	{
		auto cache = g_WaterVBOCache[i];
		if (cache->ent == ent &&
			surf->texinfo->texture == cache->texture &&
			cache->normal[2] == brushface->normal[2])
		{
			auto plane = DotProduct(poly->verts[0], brushface->normal);

			bool bSkip = false;
			for (int j = 0; j < poly->numverts; ++j)
			{
				if (fabs(cache->plane - plane) < 0.1f)
				{
					bSkip = true;
					break;
				}
			}

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

water_vbo_t *R_PrepareWaterVBO(cl_entity_t *ent, msurface_t *surf)
{
	water_vbo_t *VBOCache = NULL;

	if (!surf->lightmaptexturenum)
	{
		//Find flat WaterVBOCache with same texture and same height

		auto poly = surf->polys;

		auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

		VBOCache = R_FindWaterVBOFlat(ent, surf);

		if (!VBOCache)
		{
			VBOCache = new water_vbo_t;
			VBOCache->index = g_WaterVBOCache.size();
			VBOCache->ent = ent;
			VBOCache->texture = surf->texinfo->texture;

			VBOCache->depthreflmap = GL_GenDepthTexture(glwidth, glheight);

			VBOCache->reflectmap = GL_GenTextureRGBA8(glwidth, glheight);

			auto handle = glGetTextureHandleARB(VBOCache->texture->gl_texturenum);
			glMakeTextureHandleResidentARB(handle);

			VBOCache->basetexture_handle = handle;

			handle = glGetTextureHandleARB(VBOCache->reflectmap);
			glMakeTextureHandleResidentARB(handle);

			VBOCache->reflectmap_handle = handle;

			VBOCache->hEBO = GL_GenBuffer();
			VBOCache->hTextureSSBO = GL_GenBuffer();

			VectorCopy(poly->verts[0], VBOCache->vert);
			VectorCopy(brushface->normal, VBOCache->normal);
			VBOCache->plane = DotProduct(VBOCache->normal, VBOCache->vert);

			auto pSourcePalette = surf->texinfo->texture->pPal;
			VBOCache->color.r = pSourcePalette[9];
			VBOCache->color.g = pSourcePalette[10];
			VBOCache->color.b = pSourcePalette[11];
			VBOCache->color.a = 255;

			//Default
			VBOCache->normalmap = 0;
			VBOCache->fresnelfactor = 0;
			VBOCache->depthfactor[0] = 0;
			VBOCache->depthfactor[1] = 0;
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
					auto handle = glGetTextureHandleARB(VBOCache->normalmap);
					glMakeTextureHandleResidentARB(handle);

					VBOCache->normalmap_handle = handle;
				}

				VBOCache->fresnelfactor = waterControl->fresnelfactor;
				VBOCache->depthfactor[0] = waterControl->depthfactor[0];
				VBOCache->depthfactor[1] = waterControl->depthfactor[1];
				VBOCache->normfactor = waterControl->normfactor;
				VBOCache->minheight = waterControl->minheight;
				VBOCache->maxtrans = waterControl->maxtrans;
				VBOCache->level = waterControl->level;
			}

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
	r_draw_pass = r_draw_reflect;

	s_WaterFBO.s_hBackBufferTex = w->reflectmap;
	s_WaterFBO.s_hBackBufferDepthTex = w->depthreflmap;

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_WaterFBO.s_hBackBufferDepthTex, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glClearColor(w->color.r / 255.0f, w->color.g / 255.0f, w->color.b / 255.0f, 1);
	glStencilMask(0xFF);
	glClearStencil(0);
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glStencilMask(0);

	R_PushRefDef();

	VectorCopy(r_refdef->vieworg, water_view);

	float vForward[3], vRight[3], vUp[3];
	gEngfuncs.pfnAngleVectors(r_refdef->viewangles, vForward, vRight, vUp);

	float flDist = fabs(w->vert[2] - r_refdef->vieworg[2]);
	VectorMA(r_refdef->vieworg, -2 * flDist, w->normal, r_refdef->vieworg);

	float neg_norm[3];
	VectorCopy(w->normal, neg_norm);
	VectorInverse(neg_norm);

	float flDist2 = DotProduct(vForward, neg_norm);
	VectorMA(vForward, -2 * flDist2, neg_norm, vForward);

	r_refdef->viewangles[0] = -asin(vForward[2]) / M_PI * 180;
	r_refdef->viewangles[1] = atan2(vForward[1], vForward[0]) / M_PI * 180;
	r_refdef->viewangles[2] = -r_refdef->viewangles[2];

	saved_cl_waterlevel = *cl_waterlevel;
	*cl_waterlevel = 0;

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

	glDisable(GL_CLIP_PLANE0);

	r_drawentities->value = saved_r_drawentities;
	*cl_waterlevel = saved_cl_waterlevel;

	R_PopRefDef();

	r_draw_pass = r_draw_normal;
}

void R_RenderWaterView(void)
{
	refractmap_ready = false;

	if (g_WaterVBOCache.size())
	{
		glBindFramebuffer(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);

		for (size_t i = 0; i < g_WaterVBOCache.size(); ++i)
		{
			curwater = g_WaterVBOCache[i];

			if (R_IsAboveWater(curwater->vert, curwater->normal))
			{
				R_RenderReflectView(curwater);
			}

			curwater = NULL;
		}
	}
}