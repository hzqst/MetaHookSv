#include "gl_local.h"
#include <sstream>

//renderer
vec3_t g_CurrentCameraView;

//cvar
cvar_t *r_water = NULL;
cvar_t *r_water_forcetrans = NULL;
cvar_t *r_water_debug = NULL;

water_reflect_cache_t *g_CurrentReflectCache = NULL;
water_reflect_cache_t g_WaterReflectCaches[MAX_REFLECT_WATERS];
int g_iNumWaterReflectCaches = 0;

std::vector<cl_entity_t *> g_VisibleWaterEntity;
std::vector<water_vbo_t *> g_VisibleWaterVBO;

int g_VisWaterIndices[MAX_VISEDICTS] = { 0 };

std::unordered_map<program_state_t, water_program_t> g_WaterProgramTable;

std::vector<water_control_t> r_water_controls;

//std::vector<cubemap_t> r_cubemaps;

void R_UseWaterProgram(program_state_t state, water_program_t *progOutput)
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

		if (state & WATER_EXP_FOG_ENABLED)
			defs << "#define EXP_FOG_ENABLED\n";

		if (state & WATER_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

		if (state & WATER_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & WATER_ADDITIVE_BLEND_ENABLED)
			defs << "#define ADDITIVE_BLEND_ENABLED\n";

		if (state & WATER_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & WATER_GAMMA_BLEND_ENABLED)
			defs << "#define GAMMA_BLEND_ENABLED\n";

		if ((state & WATER_OIT_BLEND_ENABLED) && bUseOITBlend)
			defs << "#define OIT_BLEND_ENABLED\n";

		if (glewIsSupported("GL_NV_bindless_texture"))
			defs << "#define NV_BINDLESS_ENABLED\n";

		else if (glewIsSupported("GL_ARB_gpu_shader_int64"))
			defs << "#define INT64_BINDLESS_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\water_shader.vsh", "renderer\\shader\\water_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, u_watercolor, "u_watercolor");
			SHADER_UNIFORM(prog, u_depthfactor, "u_depthfactor");
			SHADER_UNIFORM(prog, u_fresnelfactor, "u_fresnelfactor");
			SHADER_UNIFORM(prog, u_normfactor, "u_normfactor");
			SHADER_UNIFORM(prog, u_scale, "u_scale");
			SHADER_UNIFORM(prog, u_speed, "u_speed");
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

const program_state_mapping_t s_WaterProgramStateName[] = {
{ WATER_LEGACY_ENABLED					, "WATER_LEGACY_ENABLED"			 },
{ WATER_UNDERWATER_ENABLED				, "WATER_UNDERWATER_ENABLED"		 },
{ WATER_GBUFFER_ENABLED					, "WATER_GBUFFER_ENABLED"			 },
{ WATER_DEPTH_ENABLED					, "WATER_DEPTH_ENABLED"				 },
{ WATER_REFRACT_ENABLED					, "WATER_REFRACT_ENABLED"			 },
{ WATER_LINEAR_FOG_ENABLED				, "WATER_LINEAR_FOG_ENABLED"		 },
{ WATER_EXP_FOG_ENABLED					, "WATER_EXP_FOG_ENABLED"			 },
{ WATER_EXP2_FOG_ENABLED				, "WATER_EXP2_FOG_ENABLED"			 },
{ WATER_ALPHA_BLEND_ENABLED				, "WATER_ALPHA_BLEND_ENABLED"		 },
{ WATER_ADDITIVE_BLEND_ENABLED			, "WATER_ADDITIVE_BLEND_ENABLED"	 },
{ WATER_OIT_BLEND_ENABLED				, "WATER_OIT_BLEND_ENABLED"			 },
{ WATER_GAMMA_BLEND_ENABLED				, "WATER_GAMMA_BLEND_ENABLED"		 },
};

void R_SaveWaterProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto &p : g_WaterProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/water_cache.txt", states, s_WaterProgramStateName, _ARRAYSIZE(s_WaterProgramStateName));
}

void R_LoadWaterProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/water_cache.txt", s_WaterProgramStateName, _ARRAYSIZE(s_WaterProgramStateName), [](program_state_t state) {

		R_UseWaterProgram(state, NULL);

	});
}

void R_FreeWaterVBO(water_vbo_t *WaterVBO)
{
	if (WaterVBO->ripplemap)
	{
		GL_DeleteTexture(WaterVBO->ripplemap);
		WaterVBO->ripplemap = 0;
	}

	if (WaterVBO->hVAO)
	{
		GL_DeleteVAO(WaterVBO->hVAO);
		WaterVBO->hVAO = 0;
	}

	if (WaterVBO->hEBO)
	{
		GL_DeleteBuffer(WaterVBO->hEBO);
		WaterVBO->hEBO = 0;
	}

	if (WaterVBO->ripple_data)
	{
		free(WaterVBO->ripple_data);
		WaterVBO->ripple_data = NULL;
	}
	if (WaterVBO->ripple_image)
	{
		free(WaterVBO->ripple_image);
		WaterVBO->ripple_image = NULL;
	}
	if (WaterVBO->ripple_spots[0])
	{
		free(WaterVBO->ripple_spots[0]);
		WaterVBO->ripple_spots[0] = NULL;
	}
	if (WaterVBO->ripple_spots[1])
	{
		free(WaterVBO->ripple_spots[1]);
		WaterVBO->ripple_spots[1] = NULL;
	}
}

void R_FreeWaterReflectCache(water_reflect_cache_t *ReflectCache)
{
	if (ReflectCache->reflectmap)
	{
		GL_DeleteTexture(ReflectCache->reflectmap);
		ReflectCache->reflectmap = 0;
	}
	if (ReflectCache->depthreflmap)
	{
		GL_DeleteTexture(ReflectCache->depthreflmap);
		ReflectCache->depthreflmap = 0;
	}

	ReflectCache->normal[0] = 0;
	ReflectCache->normal[1] = 0;
	ReflectCache->normal[2] = 0;
	ReflectCache->planedist = 0;
	ReflectCache->color.r = 0;
	ReflectCache->color.g = 0;
	ReflectCache->color.b = 0;
	ReflectCache->color.a = 0;
	ReflectCache->level = 0;
	ReflectCache->used = false;
	ReflectCache->refractmap_ready = false;
}

void R_ClearWaterReflectCaches(void)
{
	for (int i = 0; i < _ARRAYSIZE(g_WaterReflectCaches); ++i)
	{
		g_WaterReflectCaches[i].used = false;
		g_WaterReflectCaches[i].refractmap_ready = false;
	}

	g_iNumWaterReflectCaches = 0;
}

void R_FreeWaterReflectCaches(void)
{
	for (int i = 0; i < _ARRAYSIZE(g_WaterReflectCaches); ++i)
	{
		R_FreeWaterReflectCache(&g_WaterReflectCaches[i]);
	}

	g_iNumWaterReflectCaches = 0;
}

water_reflect_cache_t *R_FindReflectCache(int level, vec3_t normal, float planedist)
{
	for (int i = 0; i < g_iNumWaterReflectCaches; ++i)
	{
		if (g_WaterReflectCaches[i].reflectmap &&
			g_WaterReflectCaches[i].used &&

			g_WaterReflectCaches[i].level == level &&
			VectorDistance(g_WaterReflectCaches[i].normal, normal) < 0.1f &&
			fabs(g_WaterReflectCaches[i].planedist - planedist) < 0.1)
		{
			return &g_WaterReflectCaches[i];
		}
	}

	return NULL;
}

water_reflect_cache_t *R_FindEmptyReflectCache()
{
	for (int i = 0; i < _ARRAYSIZE(g_WaterReflectCaches); ++i)
	{
		if (!g_WaterReflectCaches[i].used || !g_WaterReflectCaches[i].reflectmap)
		{
			return &g_WaterReflectCaches[i];
		}
	}

	return NULL;
}

water_reflect_cache_t *R_PrepareReflectCache(cl_entity_t *ent, water_vbo_t *WaterVBO)
{
	vec3_t vert;

	R_RotateForEntity(ent);

	VectorTransform(WaterVBO->vert, r_entity_matrix, vert);

	float planedist = DotProduct(WaterVBO->normal, vert);

	auto ReflectCache = R_FindReflectCache(WaterVBO->level, WaterVBO->normal, planedist);
	if (!ReflectCache)
	{
		ReflectCache = R_FindEmptyReflectCache();
		if (ReflectCache)
		{
			int texwidth = glwidth;
			int texheight = glheight;

			if (ReflectCache->refractmap && ReflectCache->texwidth != texwidth)
			{
				GL_DeleteTexture(ReflectCache->refractmap);
				GL_DeleteTexture(ReflectCache->depthrefrmap);

				ReflectCache->refractmap = 0;
				ReflectCache->depthrefrmap = 0;
			}

			if (ReflectCache->reflectmap && ReflectCache->texwidth != texwidth)
			{
				GL_DeleteTexture(ReflectCache->reflectmap);
				GL_DeleteTexture(ReflectCache->depthreflmap);

				ReflectCache->reflectmap = 0;
				ReflectCache->depthreflmap = 0;
			}

			if (!ReflectCache->refractmap)
			{
				ReflectCache->refractmap = GL_GenTextureColorFormat(texwidth, texheight, GL_RGB16F, true, NULL);
				ReflectCache->depthrefrmap = GL_GenDepthStencilTexture(texwidth, texheight);
			}

			if (!ReflectCache->reflectmap)
			{
				ReflectCache->reflectmap = GL_GenTextureColorFormat(texwidth, texheight, GL_RGB16F, true, NULL);
				ReflectCache->depthreflmap = GL_GenDepthStencilTexture(texwidth, texheight);
			}

			ReflectCache->texwidth = texwidth;
			ReflectCache->texheight = texheight;

			VectorCopy(WaterVBO->normal, ReflectCache->normal);
			ReflectCache->planedist = planedist;

			ReflectCache->color.r = WaterVBO->color.r;
			ReflectCache->color.g = WaterVBO->color.g;
			ReflectCache->color.b = WaterVBO->color.b;
			ReflectCache->color.a = WaterVBO->color.a;

			ReflectCache->level = WaterVBO->level;

			ReflectCache->used = true;
			ReflectCache->refractmap_ready = false;

			int index = ReflectCache - g_WaterReflectCaches;

			if (index + 1 > g_iNumWaterReflectCaches)
				g_iNumWaterReflectCaches = index + 1;
		}
		else
		{
			//Really?
			//g_pMetaHookAPI->SysError("R_PrepareReflectCache: no empty reflect cache!");
			return NULL;			
		}
	}

	return ReflectCache;
}

void R_ShutdownWater(void)
{
	R_FreeWaterReflectCaches();

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

}

bool R_IsAboveWater(water_vbo_t *water)
{
	float org[4] = { (*r_refdef.vieworg)[0], (*r_refdef.vieworg)[1], (*r_refdef.vieworg)[2], 1 };
	float equation[4] = { water->normal[0], water->normal[1], water->normal[2], -water->planedist };
	return DotProduct4(org, equation) > 0;
}

water_vbo_t *R_FindFlatWaterVBO(msurface_t *surf, int direction, wsurf_vbo_leaf_t *leaf)
{
	auto poly = surf->polys;
	auto surfIndex = R_GetWorldSurfaceIndex(surf);
	auto brushface = &r_wsurf.vFaceBuffer[surfIndex];

	vec3_t normal;
	VectorCopy(brushface->normal, normal);

	if (direction)
	{
		VectorInverse(normal);
	}

	for (size_t i = 0; i < leaf->vWaterVBO.size(); ++i)
	{
		auto WaterVBO = leaf->vWaterVBO[i];

		if (surf->texinfo->texture == WaterVBO->texture &&
			surf->plane == WaterVBO->plane && 
			VectorDistance(normal, WaterVBO->normal) < 0.1f)//make sure it's same direction
		{
			return WaterVBO;
		}
	}

	return NULL;
}

water_control_t *R_FindWaterControl(msurface_t *surf)
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

	return pControl;
}

void R_UpdateRippleTexture(water_vbo_t *VBOCache, int framecount)
{
	if (r_draw_reflectview)
		return;

#define RANDOM_BYTES_SIZE 256
#define PROCEDURAL_SPEED_BITS 5ull

	static byte m_pPermutation[RANDOM_BYTES_SIZE];

	static bool init = false;
	if (!init)
	{
		for (int i = 0; i < RANDOM_BYTES_SIZE; i++)
			m_pPermutation[i] = i & 0xff;

		for (int i = 0; i < RANDOM_BYTES_SIZE; ++i) {
			int	swap;
			do { swap = gEngfuncs.pfnRandomLong(0, RANDOM_BYTES_SIZE - 1); } while (swap == i);
			m_pPermutation[i] ^= m_pPermutation[swap] ^= m_pPermutation[i] ^= m_pPermutation[swap];
		}

		init = true;
	}

	if (framecount == VBOCache->ripple_framecount)
		return;

	auto curtime = (uint64_t)(gEngfuncs.GetClientTime() * VBOCache->speedrate);
	auto prevtime = VBOCache->ripple_time;

	if (prevtime + (1ull << PROCEDURAL_SPEED_BITS) > curtime)
		return;

	VBOCache->ripple_framecount = framecount;
	VBOCache->ripple_time = curtime;
	VBOCache->ripple_shift ++;

	const int parity = VBOCache->ripple_shift & 1;
	short *pBuffer0 = VBOCache->ripple_spots[parity];
	short *pBuffer1 = VBOCache->ripple_spots[parity ^ 1];

	unsigned int *pSrcBuf = (unsigned int *)VBOCache->ripple_image;
	unsigned int *pDstBuf = (unsigned int *)VBOCache->ripple_data;

	int bufWide = VBOCache->ripple_width;
	int bufTall = VBOCache->ripple_height;

	for (int j = 0; j < bufTall; ++j) {
		int p2 = (!j) ? (bufTall - 1) : (j - 1);
		int p3 = (j + 1) & (bufTall - 1);

		for (int i = 0; i < bufWide; ++i) {
			int p0 = (!i) ? (bufWide - 1) : (i - 1);
			int p1 = (i + 1) & (bufWide - 1);

			/* update buffers */
			*pBuffer0 = ((pBuffer1[p0 + j * bufWide] + pBuffer1[i + p3 * bufWide] +
				pBuffer1[p1 + j * bufWide] + pBuffer1[i + p2 * bufWide]) >> 1) - (*pBuffer0);
			*pBuffer0 -= (*pBuffer0) >> 6;
			pBuffer0++;

			/* update texture */
			int gradX = (pBuffer1[p0 + j * bufWide] - pBuffer1[p1 + j * bufWide]) >> 4;
			int gradY = (pBuffer1[i + p2 * bufWide] - pBuffer1[i + p3 * bufWide]) >> 4;

			int ts = (i + gradX) & (bufWide - 1);
			if (ts < 0) ts += bufWide;
			int tt = (j + gradY) & (bufTall - 1);
			if (tt < 0) tt += bufTall;

			*pDstBuf = pSrcBuf[tt * bufWide + ts];
			pDstBuf++;
		}
	}

	auto procFrame = (curtime >> PROCEDURAL_SPEED_BITS);
	if (VBOCache->ripple_width > 64) procFrame *= (VBOCache->ripple_width >> 6);
	int skipDrips = procFrame & 7;
	if (VBOCache->ripple_shift < 16)
		skipDrips = 0;

	if (!skipDrips)
	{
		int randBase = procFrame & 0xff;
		int randTexBase = pDstBuf[0] & 0xffff;
		int rand1 = m_pPermutation[(randTexBase + (randBase++))&(RANDOM_BYTES_SIZE - 1)] << 1;
		int rand2 = m_pPermutation[(randTexBase + (randBase++))&(RANDOM_BYTES_SIZE - 1)] << 1;
		short dripsize = 96 + (m_pPermutation[randBase] >> 2);

		int x = rand1 & (bufWide - 1);
		int y = rand2 & (bufTall - 1);
		int xl = (x - 1);
		if (xl < 0) xl += bufWide;
		int xr = (x + 1) & (bufWide - 1);
		int yl = (y - 1);
		if (yl < 0) yl += bufTall;
		int yr = (y + 1) & (bufTall - 1);

		pBuffer1[yl*bufWide + xl] += dripsize;
		pBuffer1[yl*bufWide + xr] += dripsize;
		pBuffer1[yl*bufWide + x] += dripsize;
		pBuffer1[y*bufWide + xl] += dripsize;
		pBuffer1[y*bufWide + x] += dripsize;
		pBuffer1[y*bufWide + xr] += dripsize;
		pBuffer1[yr*bufWide + x] += dripsize;
		pBuffer1[yr*bufWide + xr] += dripsize;
		pBuffer1[yr*bufWide + xl] += dripsize;
	}

	glBindTexture(GL_TEXTURE_2D, VBOCache->ripplemap);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VBOCache->ripple_width, VBOCache->ripple_height, GL_RGBA, GL_UNSIGNED_BYTE, VBOCache->ripple_data);
	glBindTexture(GL_TEXTURE_2D, *currenttexture);
}

water_vbo_t *R_CreateWaterVBO(msurface_t *surf, int direction, wsurf_vbo_leaf_t *leaf)
{
	auto poly = surf->polys;
	auto surfIndex = R_GetWorldSurfaceIndex(surf);
	auto brushface = &r_wsurf.vFaceBuffer[surfIndex];

	auto WaterVBO = R_FindFlatWaterVBO(surf, direction, leaf);

	if (!WaterVBO)
	{
		WaterVBO = new water_vbo_t;
		WaterVBO->texture = surf->texinfo->texture;

		VectorCopy(brushface->normal, WaterVBO->normal);
		VectorCopy(surf->polys->verts[0], WaterVBO->vert);

		if (direction)
		{
			VectorInverse(WaterVBO->normal);
		}

		WaterVBO->planedist = DotProduct(WaterVBO->normal, WaterVBO->vert);

		WaterVBO->plane = surf->plane;

		auto pSourcePalette = surf->texinfo->texture->pPal;
		WaterVBO->color.r = pSourcePalette[9];
		WaterVBO->color.g = pSourcePalette[10];
		WaterVBO->color.b = pSourcePalette[11];
		WaterVBO->color.a = 255;

		//Default
		WaterVBO->normalmap = 0;
		WaterVBO->ripplemap = 0;
		WaterVBO->fresnelfactor[0] = 0;
		WaterVBO->fresnelfactor[1] = 0;
		WaterVBO->fresnelfactor[2] = 0;
		WaterVBO->fresnelfactor[3] = 0;
		WaterVBO->depthfactor[0] = 0;
		WaterVBO->depthfactor[1] = 0;
		WaterVBO->depthfactor[2] = 0;
		WaterVBO->normfactor = 0;
		WaterVBO->minheight = 0;
		WaterVBO->maxtrans = 1;
		WaterVBO->speedrate = 1;
		WaterVBO->level = WATER_LEVEL_LEGACY;

		WaterVBO->vIndicesBuffer = new std::vector<GLuint>();

		auto waterControl = R_FindWaterControl(surf);
		if (waterControl)
		{
			WaterVBO->minheight = waterControl->minheight;

			if (waterControl->level >= WATER_LEVEL_REFLECT_SKYBOX && waterControl->level <= WATER_LEVEL_REFLECT_ENTITY)
			{
				//TODO: disable mimap for normal texture ?
				gl_loadtexture_result_t loadResult;
				if (R_LoadTextureFromFile(waterControl->normalmap.c_str(), waterControl->normalmap.c_str(), GLT_WORLD, true, &loadResult))
				{
					WaterVBO->normalmap = loadResult.gltexturenum;
				}
				else
				{
					gEngfuncs.Con_Printf("R_CreateWaterVBO: Failed to load %s.\n", waterControl->normalmap.c_str());
				}
			}

			if (waterControl->level == WATER_LEVEL_LEGACY_RIPPLE)
			{
				glBindTexture(GL_TEXTURE_2D, surf->texinfo->texture->gl_texturenum);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &WaterVBO->ripple_width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &WaterVBO->ripple_height);

				auto imageSize = WaterVBO->ripple_width * WaterVBO->ripple_height;

				WaterVBO->ripple_data = (unsigned int*)malloc(imageSize * sizeof(unsigned int));
				memset(WaterVBO->ripple_data, 0, imageSize * sizeof(unsigned int));

				WaterVBO->ripple_image = (unsigned int*)malloc(imageSize * sizeof(unsigned int));
				memset(WaterVBO->ripple_image, 0, imageSize * sizeof(unsigned long));

				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, WaterVBO->ripple_image);

				char identifier[64] = { 0 };
				snprintf(identifier, sizeof(identifier), "%s_ripple", surf->texinfo->texture->name);

				WaterVBO->ripplemap = R_LoadRGBA8TextureFromMemory(identifier, WaterVBO->ripple_image, WaterVBO->ripple_width, WaterVBO->ripple_height, GLT_WORLD, true);

				WaterVBO->ripple_spots[0] = (short *)malloc(imageSize * sizeof(short));
				memset(WaterVBO->ripple_spots[0], 0, imageSize * sizeof(short));

				WaterVBO->ripple_spots[1] = (short *)malloc(imageSize * sizeof(short));
				memset(WaterVBO->ripple_spots[1], 0, imageSize * sizeof(short));

				WaterVBO->ripple_shift = 0;
				WaterVBO->ripple_framecount = 0;

				glBindTexture(GL_TEXTURE_2D, *currenttexture);
			}

			WaterVBO->fresnelfactor[0] = waterControl->fresnelfactor[0];
			WaterVBO->fresnelfactor[1] = waterControl->fresnelfactor[1];
			WaterVBO->fresnelfactor[2] = waterControl->fresnelfactor[2];
			WaterVBO->fresnelfactor[3] = waterControl->fresnelfactor[3];
			WaterVBO->depthfactor[0] = waterControl->depthfactor[0];
			WaterVBO->depthfactor[1] = waterControl->depthfactor[1];
			WaterVBO->depthfactor[2] = waterControl->depthfactor[2];
			WaterVBO->normfactor = waterControl->normfactor;
			WaterVBO->minheight = waterControl->minheight;
			WaterVBO->maxtrans = waterControl->maxtrans;
			WaterVBO->speedrate = waterControl->speedrate;
			WaterVBO->level = waterControl->level;
		}
	}

	for (int j = 0; j < brushface->num_polys; ++j)
	{
		if (direction)
		{
			for (int k = brushface->num_vertexes[j] - 1; k >= 0; --k)
			{
				WaterVBO->vIndicesBuffer->emplace_back(brushface->start_vertex[j] + k);
			}
		}
		else
		{
			for (int k = 0; k < brushface->num_vertexes[j]; ++k)
			{
				WaterVBO->vIndicesBuffer->emplace_back(brushface->start_vertex[j] + k);
			}
		}
		WaterVBO->vIndicesBuffer->emplace_back((GLuint)0xFFFFFFFF);
	}

	WaterVBO->iPolyCount += brushface->num_polys;

	return WaterVBO;
}

void R_RenderReflectView(water_reflect_cache_t *ReflectCache)
{
	r_draw_reflectview = true;
	g_CurrentReflectCache = ReflectCache;

	GL_BindFrameBufferWithTextures(&s_BackBufferFBO, ReflectCache->reflectmap, 0, ReflectCache->depthreflmap, ReflectCache->texwidth, ReflectCache->texheight);
	GL_SetCurrentSceneFBO(&s_BackBufferFBO);

	vec4_t vecClearColor = { ReflectCache->color.r / 255.0f, ReflectCache->color.g / 255.0f, ReflectCache->color.b / 255.0f, 0 };
	GL_ClearColorDepthStencil(vecClearColor, 1, STENCIL_MASK_SKY, STENCIL_MASK_ALL);

	R_PushRefDef();

	VectorCopy((*r_refdef.vieworg), g_CurrentCameraView);

	float vForward[3], vRight[3], vUp[3];
	gEngfuncs.pfnAngleVectors((*r_refdef.viewangles), vForward, vRight, vUp);

	float viewplane = DotProduct(ReflectCache->normal, (*r_refdef.vieworg));

	float viewdistance = fabs(ReflectCache->planedist - viewplane);
	VectorMA((*r_refdef.vieworg), -2 * viewdistance, ReflectCache->normal, (*r_refdef.vieworg));

	float neg_norm[3];
	VectorCopy(ReflectCache->normal, neg_norm);
	VectorInverse(neg_norm);

	float flDist2 = DotProduct(vForward, neg_norm);
	VectorMA(vForward, -2 * flDist2, neg_norm, vForward);

	(*r_refdef.viewangles)[0] = -asin(vForward[2]) / M_PI * 180;
	(*r_refdef.viewangles)[1] = atan2(vForward[1], vForward[0]) / M_PI * 180;
	(*r_refdef.viewangles)[2] = -(*r_refdef.viewangles)[2];

	auto saved_cl_waterlevel = *cl_waterlevel;
	*cl_waterlevel = 0;
	
	auto saved_r_drawentities = r_drawentities->value;

	if (g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_ENTITY)
	{
		r_drawentities->value = 1;
	}
	else
	{
		r_drawentities->value = 0;
	}

	R_RenderScene();

	r_drawentities->value = saved_r_drawentities;
	*cl_waterlevel = saved_cl_waterlevel;

	R_PopRefDef();

	r_draw_reflectview = false;
	g_CurrentReflectCache = NULL;

	GL_SetCurrentSceneFBO(NULL);
}

void R_RenderWaterPass(void)
{
	if (r_draw_reflectview)
		return;

	GL_BeginProfile(&Profile_RenderWaterPass);

	g_VisibleWaterVBO.clear();
	g_VisibleWaterEntity.clear();
	R_ClearWaterReflectCaches();

	mleaf_t *viewleaf = NULL;

	if (r_refdef_SvEngine && r_refdef_SvEngine->useCamera)
	{
		viewleaf = Mod_PointInLeaf(r_refdef_SvEngine->r_camera_origin, r_worldmodel);
	}
	else
	{
		viewleaf = Mod_PointInLeaf(r_origin, r_worldmodel);
	}

	int leafindex = (viewleaf - r_worldmodel->leafs);

	auto modvbo = R_PrepareWSurfVBO(r_worldmodel);

	if (leafindex >= 0 && leafindex < (int)modvbo->vLeaves.size())
	{
		auto vboleaf = modvbo->vLeaves[leafindex];

		//TODO Frustum Culling for world water?
		for (size_t i = 0; i < vboleaf->vWaterVBO.size(); ++i)
		{
			auto WaterVBO = vboleaf->vWaterVBO[i];

			g_VisibleWaterEntity.emplace_back(r_worldentity);
			g_VisibleWaterVBO.emplace_back(WaterVBO);
		}
	}

	for (int i = 0; i < (*cl_numvisedicts); ++i)
	{
		vec3_t entity_mins, entity_maxs;

		auto e = cl_visedicts[i];

		bool rotated = false;

		if (e->angles[0] || e->angles[1] || e->angles[2])
		{
			rotated = true;

			for (int i = 0; i < 3; i++)
			{
				entity_mins[i] = e->origin[i] - e->model->radius;
				entity_maxs[i] = e->origin[i] + e->model->radius;
			}
		}
		else
		{
			rotated = false;

			VectorAdd(e->origin, e->model->mins, entity_mins);
			VectorAdd(e->origin, e->model->maxs, entity_maxs);
		}

		if (R_CullBox(entity_mins, entity_maxs))
			continue;

		VectorSubtract((*r_refdef.vieworg) , e->origin, modelorg);

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

		modvbo = R_PrepareWSurfVBO(e->model);

		auto vboleaf = modvbo->vLeaves[0];

		if (vboleaf->vWaterVBO.empty())
			continue;

		for (size_t i = 0; i < vboleaf->vWaterVBO.size(); ++i)
		{
			auto WaterVBO = vboleaf->vWaterVBO[i];

			auto pplane = WaterVBO->plane;

			if (g_iEngineType == ENGINE_SVENGINE)
			{
				if (entity_mins[2] >= pplane->dist)
					continue;
			}
			else
			{
				if (entity_mins[2] + 1.0f >= pplane->dist)
					continue;
			}

			auto dot = DotProduct(modelorg, WaterVBO->normal) - WaterVBO->planedist;

			if (dot > 0)
			{
				g_VisibleWaterEntity.emplace_back(e);
				g_VisibleWaterVBO.emplace_back(WaterVBO);

				if ((*cl_waterlevel) >= 2)
				{
					auto pSourcePalette = WaterVBO->texture->pPal;

					gWaterColor->r = pSourcePalette[9];
					gWaterColor->g = pSourcePalette[10];
					gWaterColor->b = pSourcePalette[11];
					cshift_water->destcolor[0] = pSourcePalette[9];
					cshift_water->destcolor[1] = pSourcePalette[10];
					cshift_water->destcolor[2] = pSourcePalette[11];
					cshift_water->percent = pSourcePalette[12];

					if (gWaterColor->r == 0 && gWaterColor->g == 0 && gWaterColor->b == 0)
					{
						gWaterColor->r = pSourcePalette[0];
						gWaterColor->g = pSourcePalette[1];
						gWaterColor->b = pSourcePalette[2];
					}
				}
			}
		}
	}

	for (size_t i = 0; i < g_VisibleWaterVBO.size(); ++i)
	{
		auto WaterVBO = g_VisibleWaterVBO[i];

		auto e = g_VisibleWaterEntity[i];

		water_reflect_cache_t *ReflectCache = NULL;

		if (WaterVBO->level >= WATER_LEVEL_REFLECT_SKYBOX && WaterVBO->level <= WATER_LEVEL_REFLECT_ENTITY && r_water->value > 0)
		{
			ReflectCache = R_PrepareReflectCache(e, WaterVBO);
		}
		else if (WaterVBO->level == WATER_LEVEL_LEGACY_RIPPLE)
		{
			R_UpdateRippleTexture(WaterVBO, (*r_framecount));
		}

		auto comp = R_GetEntityComponent(e, true);
		if (comp)
		{
			comp->WaterVBOs.emplace_back(WaterVBO);
			comp->ReflectCaches.emplace_back(ReflectCache);
		}
	}

	if (g_iNumWaterReflectCaches > 0)
	{
		for (int i = 0; i < g_iNumWaterReflectCaches; ++i)
		{
			if (g_WaterReflectCaches[i].used)
			{
				R_RenderReflectView(&g_WaterReflectCaches[i]);
			}
		}
	}

	GL_EndProfile(&Profile_RenderWaterPass);
}

void R_DrawWaterVBOBegin(water_vbo_t* WaterVBO)
{
	GL_BindVAO(WaterVBO->hVAO);

	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
}

void R_DrawWaterVBOEnd()
{
	glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	GL_BindVAO(0);
}

void R_DrawWaterVBO(water_vbo_t *WaterVBO, water_reflect_cache_t *ReflectCache, cl_entity_t *ent)
{
	R_DrawWaterVBOBegin(WaterVBO);

	if (r_draw_opaque)
	{
		GL_BeginStencilWrite(STENCIL_MASK_WATER, STENCIL_MASK_ALL);
	}

	R_SetRenderMode(ent);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	bool bIsAboveWater = (WaterVBO->normal[2] > 0) && R_IsAboveWater(WaterVBO) ? true : false;

	float color[4];
	color[0] = WaterVBO->color.r / 255.0f;
	color[1] = WaterVBO->color.g / 255.0f;
	color[2] = WaterVBO->color.b / 255.0f;
	color[3] = 1;

	if ((*currententity)->curstate.rendermode == kRenderTransTexture)
		color[3] = (*r_blend);

	if (WaterVBO->level >= WATER_LEVEL_REFLECT_SKYBOX && WaterVBO->level <= WATER_LEVEL_REFLECT_ENTITY && ReflectCache)
	{
		if (!ReflectCache->refractmap_ready)
		{
			if (r_draw_gbuffer)
			{
				R_DrawWaterVBOEnd();

				//Purpose : Blit color and depth of s_GBuffers into ReflectCache->refractmap and ReflectCache->depthrefrmap
				GL_BindFrameBufferWithTextures(&s_BackBufferFBO2, ReflectCache->refractmap, 0, ReflectCache->depthrefrmap, ReflectCache->texwidth, ReflectCache->texheight);
				
				//The output is in linear space
				R_BlitGBufferToFrameBuffer(&s_BackBufferFBO2, true, true, true);

				//Restore BackBufferFBO2 to it's original states.
				GL_BindFrameBufferWithTextures(&s_BackBufferFBO2, s_BackBufferFBO2.s_hBackBufferTex, 0, s_BackBufferFBO2.s_hBackBufferDepthTex, glwidth, glheight);

				//Restore previous framebuffer
				GL_BindFrameBuffer(&s_GBufferFBO);

				//Restore Legacy OpenGL matrix that manipulated by R_BlitGBufferToFrameBuffer
				R_LoadLegacyOpenGLMatrixForWorld();

				R_DrawWaterVBOBegin(WaterVBO);
			}
			else
			{
				R_DrawWaterVBOEnd();

				//Purpose : Blit color and depth of SceneFBO into ReflectCache->refractmap and ReflectCache->depthrefrmap

				GL_BindFrameBufferWithTextures(&s_BackBufferFBO2, ReflectCache->refractmap, 0, ReflectCache->depthrefrmap, ReflectCache->texwidth, ReflectCache->texheight);
				
				if (r_draw_gammablend)
				{
					//The SceneFBO is in gamma space
					GL_BlitFrameBufferToFrameBufferDepthStencil(GL_GetCurrentSceneFBO(), &s_BackBufferFBO2);
					//Convert back to linear space
					R_GammaUncorrection(GL_GetCurrentSceneFBO(), &s_BackBufferFBO2);
				}
				else
				{
					//The SceneFBO is in linear space
					GL_BlitFrameBufferToFrameBufferColorDepthStencil(GL_GetCurrentSceneFBO(), &s_BackBufferFBO2);
				}

				//Restore BackBufferFBO2 to it's original states.
				GL_BindFrameBufferWithTextures(&s_BackBufferFBO2, s_BackBufferFBO2.s_hBackBufferTex, 0, s_BackBufferFBO2.s_hBackBufferDepthTex, glwidth, glheight);

				//Restore previous framebuffer
				GL_BindFrameBuffer(GL_GetCurrentSceneFBO());

				//Restore Legacy OpenGL matrix that manipulated by R_GammaUncorrection
				R_LoadLegacyOpenGLMatrixForWorld();

				R_DrawWaterVBOBegin(WaterVBO);
			}

			ReflectCache->refractmap_ready = true;
		}

		program_state_t WaterProgramState = 0;

		if (bIsAboveWater)
			WaterProgramState |= WATER_DEPTH_ENABLED;

		if (r_water_forcetrans->value)
		{
			WaterProgramState |= WATER_REFRACT_ENABLED;
			WaterProgramState |= WATER_ALPHA_BLEND_ENABLED;

			if (color[3] > WaterVBO->maxtrans)
				color[3] = WaterVBO->maxtrans;
		}
		else
		{
			if ((*currententity)->curstate.rendermode == kRenderTransTexture || (*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				WaterProgramState |= WATER_REFRACT_ENABLED;
				WaterProgramState |= WATER_ALPHA_BLEND_ENABLED;

				if (color[3] > WaterVBO->maxtrans)
					color[3] = WaterVBO->maxtrans;
			}
		}

		if (!bIsAboveWater)
		{
			WaterProgramState |= WATER_UNDERWATER_ENABLED;
		}
		else
		{
			if (!R_IsRenderingGBuffer())
			{
				if (r_fog_mode == GL_LINEAR)
				{
					WaterProgramState |= WATER_LINEAR_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP)
				{
					WaterProgramState |= WATER_EXP_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP2)
				{
					WaterProgramState |= WATER_EXP2_FOG_ENABLED;
				}
			}
		}

		if (R_IsRenderingGBuffer())
		{
			WaterProgramState |= WATER_GBUFFER_ENABLED;
		}

		if (r_draw_gammablend)
		{
			WaterProgramState |= WATER_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (WaterProgramState & (WATER_ALPHA_BLEND_ENABLED | WATER_ADDITIVE_BLEND_ENABLED)))
		{
			WaterProgramState |= WATER_OIT_BLEND_ENABLED;
		}

		water_program_t prog = { 0 };
		R_UseWaterProgram(WaterProgramState, &prog);

		if (prog.u_watercolor != -1)
		{
			glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);
		}
		if (prog.u_depthfactor != -1)
		{
			glUniform3f(prog.u_depthfactor, WaterVBO->depthfactor[0], WaterVBO->depthfactor[1], WaterVBO->depthfactor[2]);
		}
		if (prog.u_fresnelfactor != -1)
		{
			glUniform4f(prog.u_fresnelfactor, WaterVBO->fresnelfactor[0], WaterVBO->fresnelfactor[1], WaterVBO->fresnelfactor[2], WaterVBO->fresnelfactor[3]);
		}
		if (prog.u_normfactor != -1)
		{
			glUniform1f(prog.u_normfactor, WaterVBO->normfactor);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, WaterVBO->normalmap);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ReflectCache->reflectmap);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ReflectCache->refractmap);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, ReflectCache->depthrefrmap);

		glDrawElements(GL_POLYGON, WaterVBO->iIndicesCount, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

		r_wsurf_drawcall++;
		r_wsurf_polys += WaterVBO->iPolyCount;

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(*oldtarget);

		glDisable(GL_BLEND);
	}
	else if (WaterVBO->level == WATER_LEVEL_LEGACY_RIPPLE && r_water->value > 0)
	{
		program_state_t WaterProgramState = WATER_LEGACY_ENABLED;

		if (!bIsAboveWater)
		{

		}
		else
		{
			if (!R_IsRenderingGBuffer())
			{
				if (r_fog_mode == GL_LINEAR)
				{
					WaterProgramState |= WATER_LINEAR_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP)
				{
					WaterProgramState |= WATER_EXP_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP2)
				{
					WaterProgramState |= WATER_EXP2_FOG_ENABLED;
				}
			}
		}

		if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			WaterProgramState |= WATER_ADDITIVE_BLEND_ENABLED;
		else if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
			WaterProgramState |= WATER_ALPHA_BLEND_ENABLED;

		if (R_IsRenderingGBuffer())
		{
			WaterProgramState |= WATER_GBUFFER_ENABLED;
		}

		if (r_draw_gammablend)
		{
			WaterProgramState |= WATER_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (WaterProgramState & (WATER_ALPHA_BLEND_ENABLED | WATER_ADDITIVE_BLEND_ENABLED)))
		{
			WaterProgramState |= WATER_OIT_BLEND_ENABLED;
		}

		water_program_t prog = { 0 };
		R_UseWaterProgram(WaterProgramState, &prog);

		if (prog.u_watercolor != -1)
			glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);

		if (prog.u_scale != -1)
			glUniform1f(prog.u_scale, 0);

		if (prog.u_speed != -1)
			glUniform1f(prog.u_speed, 0);

		GL_Bind(WaterVBO->ripplemap);

		glDrawElements(GL_POLYGON, WaterVBO->iIndicesCount, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

		r_wsurf_drawcall++;
		r_wsurf_polys += WaterVBO->iPolyCount;
	}
	else
	{
		float scale;

		if (bIsAboveWater)
			scale = (*currententity)->curstate.scale;
		else
			scale = -(*currententity)->curstate.scale;

		program_state_t WaterProgramState = WATER_LEGACY_ENABLED;

		if (!bIsAboveWater)
		{

		}
		else
		{
			if (!R_IsRenderingGBuffer())
			{
				if (r_fog_mode == GL_LINEAR)
				{
					WaterProgramState |= WATER_LINEAR_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP)
				{
					WaterProgramState |= WATER_EXP_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP2)
				{
					WaterProgramState |= WATER_EXP2_FOG_ENABLED;
				}
			}
		}

		if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			WaterProgramState |= WATER_ADDITIVE_BLEND_ENABLED;
		else if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
			WaterProgramState |= WATER_ALPHA_BLEND_ENABLED;

		if (R_IsRenderingGBuffer())
		{
			WaterProgramState |= WATER_GBUFFER_ENABLED;
		}

		if (r_draw_gammablend)
		{
			WaterProgramState |= WATER_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (WaterProgramState & (WATER_ALPHA_BLEND_ENABLED | WATER_ADDITIVE_BLEND_ENABLED)))
		{
			WaterProgramState |= WATER_OIT_BLEND_ENABLED;
		}

		water_program_t prog = { 0 };
		R_UseWaterProgram(WaterProgramState, &prog);

		if (prog.u_watercolor != -1)
			glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);

		if (prog.u_scale != -1)
			glUniform1f(prog.u_scale, scale);

		if (prog.u_speed != -1)
			glUniform1f(prog.u_speed, WaterVBO->speedrate);

		GL_Bind(WaterVBO->texture->gl_texturenum);

		glDrawElements(GL_POLYGON, WaterVBO->iIndicesCount, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

		r_wsurf_drawcall++;
		r_wsurf_polys += WaterVBO->iPolyCount;
	}

	GL_UseProgram(0);

	if (r_draw_opaque)
	{
		GL_EndStencil();
	}

	R_DrawWaterVBOEnd();
}

void R_DrawWaters(wsurf_vbo_leaf_t *vboleaf, cl_entity_t *ent)
{
	if (r_draw_reflectview)
		return;

	if (r_draw_shadowcaster)
		return;

	if (!vboleaf->vWaterVBO.size())
		return;

	auto EntityComponent = R_GetEntityComponent(ent, false);

	if (!EntityComponent)
		return;

	for (size_t i = 0; i < EntityComponent->WaterVBOs.size(); ++i)
	{
		R_DrawWaterVBO(EntityComponent->WaterVBOs[i], EntityComponent->ReflectCaches[i], ent);
	}
}