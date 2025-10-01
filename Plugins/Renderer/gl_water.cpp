#include "gl_local.h"

//renderer
vec3_t g_CurrentCameraView;

//cvar
cvar_t* r_water = NULL;

CWaterReflectCache* g_CurrentReflectCache = NULL;
CWaterReflectCache g_WaterReflectCaches[MAX_REFLECT_WATERS];

int g_iNumWaterReflectCaches = 0;

std::vector<cl_entity_t*> g_VisibleWaterEntity;
std::vector<std::shared_ptr<CWaterSurfaceModel>> g_VisibleWaterSurfaceModels;

std::unordered_map<program_state_t, water_program_t> g_WaterProgramTable;

std::vector<std::shared_ptr<CEnvWaterControl>> g_EnvWaterControls;

//std::vector<cubemap_t> r_cubemaps;

CWaterSurfaceModel::~CWaterSurfaceModel()
{
	if (ripplemap)
	{
		GL_DeleteTexture(ripplemap);
		ripplemap = 0;
	}

	if (hABO)
	{
		GL_DeleteVAO(hABO);
		hABO = 0;
	}

	if (ripple_data)
	{
		free(ripple_data);
		ripple_data = NULL;
	}
	if (ripple_image)
	{
		free(ripple_image);
		ripple_image = NULL;
	}
	if (ripple_spots[0])
	{
		free(ripple_spots[0]);
		ripple_spots[0] = NULL;
	}
	if (ripple_spots[1])
	{
		free(ripple_spots[1]);
		ripple_spots[1] = NULL;
	}
}

void R_UseWaterProgram(program_state_t state, water_program_t* progOutput)
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

		if (state & WATER_LINEAR_FOG_SHIFT_ENABLED)
			defs << "#define LINEAR_FOG_SHIFT_ENABLED\n";

		if ((state & WATER_OIT_BLEND_ENABLED) && g_bUseOITBlend)
			defs << "#define OIT_BLEND_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFile("renderer\\shader\\water_shader.vert.glsl", "renderer\\shader\\water_shader.frag.glsl", def.c_str(), def.c_str());
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
		Sys_Error("R_UseWaterProgram: Failed to load program!");
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
	{ WATER_LINEAR_FOG_SHIFT_ENABLED		, "WATER_LINEAR_FOG_SHIFT_ENABLED"	 },
};

void R_SaveWaterProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto& p : g_WaterProgramTable)
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

void R_FreeWaterVBO(CWaterSurfaceModel* WaterVBO)
{

}

void R_FreeWaterReflectCache(CWaterReflectCache* ReflectCache)
{
	if (ReflectCache->reflect_texture)
	{
		GL_DeleteTexture(ReflectCache->reflect_texture);
		ReflectCache->reflect_texture = 0;
	}

	if (ReflectCache->reflect_depth_texture)
	{
		GL_DeleteTexture(ReflectCache->reflect_depth_texture);
		ReflectCache->reflect_depth_texture = 0;
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
	ReflectCache->reflectmap_ready = false;
}

void R_FreeWaterReflectCaches(void)
{
	for (int i = 0; i < _ARRAYSIZE(g_WaterReflectCaches); ++i)
	{
		R_FreeWaterReflectCache(&g_WaterReflectCaches[i]);
	}

	g_iNumWaterReflectCaches = 0;
}

void R_ClearWaterReflectCaches(void)
{
	for (int i = 0; i < _ARRAYSIZE(g_WaterReflectCaches); ++i)
	{
		g_WaterReflectCaches[i].used = false;
		g_WaterReflectCaches[i].reflectmap_ready = false;
	}

	g_iNumWaterReflectCaches = 0;
}

CWaterReflectCache* R_FindReflectCache(int level, vec3_t normal, float planedist)
{
	for (int i = 0; i < g_iNumWaterReflectCaches; ++i)
	{
		if (g_WaterReflectCaches[i].reflect_texture &&
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

CWaterReflectCache* R_FindEmptyReflectCache()
{
	for (int i = 0; i < _ARRAYSIZE(g_WaterReflectCaches); ++i)
	{
		if (!g_WaterReflectCaches[i].used || !g_WaterReflectCaches[i].reflect_texture)
		{
			return &g_WaterReflectCaches[i];
		}
	}

	return NULL;
}

CWaterReflectCache* R_PrepareReflectCache(cl_entity_t* ent, CWaterSurfaceModel* pWaterModel)
{
	vec3_t vert{};
	vec3_t normal{};

	//Calculate real vert and normal based on ent->origin and ent->angles

	R_RotateForEntity(ent, r_entity_matrix);

	VectorTransform(pWaterModel->vert, r_entity_matrix, vert);
	VectorRotate(pWaterModel->normal, r_entity_matrix, normal);

	//almost vertical?
	if (fabs(normal[2]) < 0.3)
		return nullptr;

	float planedist = DotProduct(normal, vert);

	auto ReflectCache = R_FindReflectCache(pWaterModel->level, normal, planedist);

	if (!ReflectCache)
	{
		ReflectCache = R_FindEmptyReflectCache();

		if (ReflectCache)
		{
			int texwidth = glwidth * 0.5f;
			int texheight = glheight * 0.5f;

			if (ReflectCache->refract_texture && ReflectCache->texwidth != texwidth)
			{
				GL_DeleteTexture(ReflectCache->refract_texture);
				GL_DeleteTexture(ReflectCache->refract_depth_texture);

				ReflectCache->refract_texture = 0;
				ReflectCache->refract_depth_texture = 0;
			}

			if (!ReflectCache->refract_texture)
			{
				ReflectCache->refract_texture = GL_GenTextureColorFormat(texwidth, texheight, GL_RGB16F, true, nullptr, true);
				ReflectCache->refract_depth_texture = GL_GenDepthStencilTexture(texwidth, texheight, true);
			}

			if (ReflectCache->reflect_texture && ReflectCache->texwidth != texwidth)
			{
				GL_DeleteTexture(ReflectCache->reflect_texture);
				GL_DeleteTexture(ReflectCache->reflect_depth_texture);

				ReflectCache->reflect_texture = 0;
				ReflectCache->reflect_depth_texture = 0;
			}

			if (!ReflectCache->reflect_texture)
			{
				ReflectCache->reflect_texture = GL_GenTextureColorFormat(texwidth, texheight, GL_RGB16F, true, nullptr, true);
				ReflectCache->reflect_depth_texture = GL_GenDepthStencilTexture(texwidth, texheight, true);
			}

			ReflectCache->texwidth = texwidth;
			ReflectCache->texheight = texheight;

			VectorCopy(normal, ReflectCache->normal);
			ReflectCache->planedist = planedist;

			ReflectCache->color.r = pWaterModel->color.r;
			ReflectCache->color.g = pWaterModel->color.g;
			ReflectCache->color.b = pWaterModel->color.b;
			ReflectCache->color.a = pWaterModel->color.a;

			ReflectCache->level = pWaterModel->level;

			ReflectCache->used = true;
			ReflectCache->reflectmap_ready = false;

			int index = ReflectCache - g_WaterReflectCaches;

			if (index + 1 > g_iNumWaterReflectCaches)
				g_iNumWaterReflectCaches = index + 1;
		}
		else
		{
			return nullptr;
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
}

bool R_IsAboveWater(CWaterSurfaceModel* pWaterModel)
{
	if (pWaterModel->normal[2] > 0)
	{
		float org[4] = { (*r_refdef.vieworg)[0], (*r_refdef.vieworg)[1], (*r_refdef.vieworg)[2], 1 };
		float equation[4] = { pWaterModel->normal[0], pWaterModel->normal[1], pWaterModel->normal[2], -pWaterModel->planedist };
		return DotProduct4(org, equation) > 0;
	}

	return false;
}

bool R_IsAboveWater(CWaterReflectCache* pWaterReflectCache)
{
	if (pWaterReflectCache->normal[2] > 0)
	{
		float org[4] = { (*r_refdef.vieworg)[0], (*r_refdef.vieworg)[1], (*r_refdef.vieworg)[2], 1 };
		float equation[4] = { pWaterReflectCache->normal[0], pWaterReflectCache->normal[1], pWaterReflectCache->normal[2], -pWaterReflectCache->planedist };
		return DotProduct4(org, equation) > 0;
	}

	return false;
}

std::shared_ptr<CEnvWaterControl> R_FindWaterControl(msurface_t* surf)
{
	for (const auto &pWaterControl : g_EnvWaterControls)
	{
		if (pWaterControl->basetexture[0] == '*')
		{
			return pWaterControl;
		}
		else if (pWaterControl->wildcard.length() > 0)
		{
			if (!strncmp(pWaterControl->wildcard.c_str(), surf->texinfo->texture->name, pWaterControl->wildcard.length()))
			{
				return pWaterControl;
			}
		}
		else
		{
			if (!strcmp(pWaterControl->basetexture.c_str(), surf->texinfo->texture->name))
			{
				return pWaterControl;
			}
		}
	}

	return nullptr;
}

void R_UpdateRippleTexture(CWaterSurfaceModel* pWaterModel, int framecount)
{
	if (R_IsRenderingWaterView())
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

	if (framecount == pWaterModel->ripple_framecount)
		return;

	auto curtime = (uint64_t)(gEngfuncs.GetClientTime() * pWaterModel->speedrate);
	auto prevtime = pWaterModel->ripple_time;

	if (prevtime + (1ull << PROCEDURAL_SPEED_BITS) > curtime)
		return;

	pWaterModel->ripple_framecount = framecount;
	pWaterModel->ripple_time = curtime;
	pWaterModel->ripple_shift++;

	const int parity = pWaterModel->ripple_shift & 1;
	short* pBuffer0 = pWaterModel->ripple_spots[parity];
	short* pBuffer1 = pWaterModel->ripple_spots[parity ^ 1];

	unsigned int* pSrcBuf = (unsigned int*)pWaterModel->ripple_image;
	unsigned int* pDstBuf = (unsigned int*)pWaterModel->ripple_data;

	int bufWide = pWaterModel->ripple_width;
	int bufTall = pWaterModel->ripple_height;

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
	if (pWaterModel->ripple_width > 64) procFrame *= (pWaterModel->ripple_width >> 6);
	int skipDrips = procFrame & 7;
	if (pWaterModel->ripple_shift < 16)
		skipDrips = 0;

	if (!skipDrips)
	{
		int randBase = procFrame & 0xFF;
		int randTexBase = pDstBuf[0] & 0xFFFF;
		int rand1 = m_pPermutation[(randTexBase + (randBase++) % 0xFF) & (RANDOM_BYTES_SIZE - 1)] << 1;
		int rand2 = m_pPermutation[(randTexBase + (randBase++)) & (RANDOM_BYTES_SIZE - 1)] << 1;
		short dripsize = 96 + (m_pPermutation[(randBase % 0xFF)] >> 2);

		int x = rand1 & (bufWide - 1);
		int y = rand2 & (bufTall - 1);
		int xl = (x - 1);
		if (xl < 0) xl += bufWide;
		int xr = (x + 1) & (bufWide - 1);
		int yl = (y - 1);
		if (yl < 0) yl += bufTall;
		int yr = (y + 1) & (bufTall - 1);

		pBuffer1[yl * bufWide + xl] += dripsize;
		pBuffer1[yl * bufWide + xr] += dripsize;
		pBuffer1[yl * bufWide + x] += dripsize;
		pBuffer1[y * bufWide + xl] += dripsize;
		pBuffer1[y * bufWide + x] += dripsize;
		pBuffer1[y * bufWide + xr] += dripsize;
		pBuffer1[yr * bufWide + x] += dripsize;
		pBuffer1[yr * bufWide + xr] += dripsize;
		pBuffer1[yr * bufWide + xl] += dripsize;
	}

	glBindTexture(GL_TEXTURE_2D, pWaterModel->ripplemap);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pWaterModel->ripple_width, pWaterModel->ripple_height, GL_RGBA, GL_UNSIGNED_BYTE, pWaterModel->ripple_data);
	glBindTexture(GL_TEXTURE_2D, (*currenttexture));
}

std::shared_ptr<CWaterSurfaceModel> R_FindFlatWaterSurfaceModel(model_t* mod, msurface_t* surf, int direction, CWorldSurfaceWorldModel* pWorldModel, CWorldSurfaceLeaf* pLeaf)
{
	auto surfIndex = R_GetWorldSurfaceIndex(mod, surf);

	if (surfIndex == -1)
	{
		Sys_Error("R_FindFlatWaterSurfaceModel: invalid surfIndex!");
		return nullptr;
	}

	auto brushface = &pWorldModel->m_vFaceBuffer[surfIndex];

	vec3_t normal;
	VectorCopy(brushface->normal, normal);

	if (direction)
	{
		VectorInverse(normal);
	}

	for (size_t i = 0; i < pLeaf->m_vWaterSurfaceModels.size(); ++i)
	{
		auto pWaterModel = pLeaf->m_vWaterSurfaceModels[i];

		if (surf->texinfo->texture == pWaterModel->texture &&
			surf->plane == pWaterModel->plane &&
			VectorDistance(normal, pWaterModel->normal) < 0.1f)//make sure it's same direction
		{
			return pWaterModel;
		}
	}

	return nullptr;
}

std::shared_ptr<CWaterSurfaceModel> R_GetWaterSurfaceModel(model_t* mod, msurface_t* surf, int direction, CWorldSurfaceWorldModel* pWorldModel, CWorldSurfaceLeaf* pLeaf)
{
	auto worldmodel = pWorldModel->m_model;
	auto surfIndex = R_GetWorldSurfaceIndex(worldmodel, surf);

	if (surfIndex == -1)
	{
		Sys_Error("R_GetWaterSurfaceModel: invalid surfIndex!");
		return nullptr;
	}

	auto brushface = &pWorldModel->m_vFaceBuffer[surfIndex];

	std::shared_ptr<CWaterSurfaceModel> pWaterModel = R_FindFlatWaterSurfaceModel(mod, surf, direction, pWorldModel, pLeaf);

	if (!pWaterModel)
	{
		pWaterModel = std::make_shared<CWaterSurfaceModel>();
		pWaterModel->texture = surf->texinfo->texture;

		VectorCopy(brushface->normal, pWaterModel->normal);
		VectorCopy(surf->polys->verts[0], pWaterModel->vert);

		if (direction)
		{
			VectorInverse(pWaterModel->normal);
		}

		pWaterModel->planedist = DotProduct(pWaterModel->normal, pWaterModel->vert);

		pWaterModel->plane = surf->plane;

		auto pSourcePalette = surf->texinfo->texture->pPal;
		pWaterModel->color.r = pSourcePalette[9];
		pWaterModel->color.g = pSourcePalette[10];
		pWaterModel->color.b = pSourcePalette[11];
		pWaterModel->color.a = 255;

		//Default
		pWaterModel->normalmap = 0;
		pWaterModel->ripplemap = 0;
		pWaterModel->fresnelfactor[0] = 0;
		pWaterModel->fresnelfactor[1] = 0;
		pWaterModel->fresnelfactor[2] = 0;
		pWaterModel->fresnelfactor[3] = 0;
		pWaterModel->depthfactor[0] = 0;
		pWaterModel->depthfactor[1] = 0;
		pWaterModel->depthfactor[2] = 0;
		pWaterModel->normfactor = 0;
		pWaterModel->minheight = 0;
		pWaterModel->maxtrans = 1;
		pWaterModel->speedrate = 1;
		pWaterModel->level = WATER_LEVEL_LEGACY;

		pWaterModel->m_vDrawAttribBuffer.clear();

		auto pWaterControl = R_FindWaterControl(surf);

		if (pWaterControl)
		{
			pWaterModel->minheight = pWaterControl->minheight;

			if (pWaterControl->level >= WATER_LEVEL_REFLECT_SKYBOX && pWaterControl->level <= WATER_LEVEL_REFLECT_ENTITY)
			{
				gl_loadtexture_result_t loadResult;
				if (R_LoadTextureFromFile(pWaterControl->normalmap.c_str(), pWaterControl->normalmap.c_str(), GLT_WORLD, true, &loadResult))
				{
					pWaterModel->normalmap = loadResult.gltexturenum;
				}
				else
				{
					gEngfuncs.Con_Printf("R_GetWaterSurfaceModel: Failed to load %s.\n", pWaterControl->normalmap.c_str());
				}
			}

			if (pWaterControl->level == WATER_LEVEL_LEGACY_RIPPLE)
			{
				glBindTexture(GL_TEXTURE_2D, surf->texinfo->texture->gl_texturenum);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &pWaterModel->ripple_width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &pWaterModel->ripple_height);

				auto imageSize = pWaterModel->ripple_width * pWaterModel->ripple_height;

				pWaterModel->ripple_data = (unsigned int*)malloc(imageSize * sizeof(unsigned int));
				memset(pWaterModel->ripple_data, 0, imageSize * sizeof(unsigned int));

				pWaterModel->ripple_image = (unsigned int*)malloc(imageSize * sizeof(unsigned int));
				memset(pWaterModel->ripple_image, 0, imageSize * sizeof(unsigned long));

				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pWaterModel->ripple_image);

				char identifier[64] = { 0 };
				snprintf(identifier, sizeof(identifier), "%s_ripple", surf->texinfo->texture->name);

				pWaterModel->ripplemap = R_LoadRGBA8TextureFromMemory(identifier, pWaterModel->ripple_image, pWaterModel->ripple_width, pWaterModel->ripple_height, GLT_WORLD, true);

				pWaterModel->ripple_spots[0] = (short*)malloc(imageSize * sizeof(short));
				memset(pWaterModel->ripple_spots[0], 0, imageSize * sizeof(short));

				pWaterModel->ripple_spots[1] = (short*)malloc(imageSize * sizeof(short));
				memset(pWaterModel->ripple_spots[1], 0, imageSize * sizeof(short));

				pWaterModel->ripple_shift = 0;
				pWaterModel->ripple_framecount = 0;

				glBindTexture(GL_TEXTURE_2D, (*currenttexture));
			}

			pWaterModel->fresnelfactor[0] = pWaterControl->fresnelfactor[0];
			pWaterModel->fresnelfactor[1] = pWaterControl->fresnelfactor[1];
			pWaterModel->fresnelfactor[2] = pWaterControl->fresnelfactor[2];
			pWaterModel->fresnelfactor[3] = pWaterControl->fresnelfactor[3];
			pWaterModel->depthfactor[0] = pWaterControl->depthfactor[0];
			pWaterModel->depthfactor[1] = pWaterControl->depthfactor[1];
			pWaterModel->depthfactor[2] = pWaterControl->depthfactor[2];
			pWaterModel->normfactor = pWaterControl->normfactor;
			pWaterModel->minheight = pWaterControl->minheight;
			pWaterModel->maxtrans = pWaterControl->maxtrans;
			pWaterModel->speedrate = pWaterControl->speedrate;
			pWaterModel->level = pWaterControl->level;
		}

		pLeaf->m_vWaterSurfaceModels.emplace_back(pWaterModel);
	}

	if (direction)
	{
		CDrawIndexAttrib drawAttrib;
		drawAttrib.FirstIndexLocation = brushface->reverse_start_index;
		drawAttrib.NumIndices = brushface->reverse_index_count;
		drawAttrib.FirstInstanceLocation = brushface->instance_index;
		drawAttrib.NumInstances = brushface->instance_count;

		pWaterModel->m_vDrawAttribBuffer.emplace_back(drawAttrib);
	}
	else
	{
		CDrawIndexAttrib drawAttrib;
		drawAttrib.FirstIndexLocation = brushface->start_index;
		drawAttrib.NumIndices = brushface->index_count;
		drawAttrib.FirstInstanceLocation = brushface->instance_index;
		drawAttrib.NumInstances = brushface->instance_count;

		pWaterModel->m_vDrawAttribBuffer.emplace_back(drawAttrib);
	}

	pWaterModel->drawCount = (uint32_t)pWaterModel->m_vDrawAttribBuffer.size();
	pWaterModel->polyCount += brushface->poly_count;

	return pWaterModel;
}

void R_RenderWaterRefractView(CWaterReflectCache* ReflectCache)
{
	GL_BeginDebugGroup("R_RenderWaterRefractView");

	int texwidth = glwidth * 0.5f;
	int texheight = glheight * 0.5f;

	r_draw_refractview = true;
	g_CurrentReflectCache = ReflectCache;

	GL_BindFrameBufferWithTextures(&s_WaterSurfaceFBO, ReflectCache->refract_texture, 0, ReflectCache->refract_depth_texture, ReflectCache->texwidth, ReflectCache->texheight);
	GL_SetCurrentSceneFBO(&s_WaterSurfaceFBO);

	vec4_t vecClearColor = { ReflectCache->color.r / 255.0f, ReflectCache->color.g / 255.0f, ReflectCache->color.b / 255.0f, 0 };

	GammaToLinear(vecClearColor);

	GL_ClearColorDepthStencil(vecClearColor, 1, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

	R_PushRefDef();

	VectorCopy((*r_refdef.vieworg), g_CurrentCameraView);

	auto saved_cl_waterlevel = *cl_waterlevel;
	(*cl_waterlevel) = 0;

	auto old_draw_classify = r_draw_classify;

	if (g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_SKYBOX)
	{
		r_draw_classify = (
			DRAW_CLASSIFY_SKYBOX | 
			DRAW_CLASSIFY_LIGHTMAP
			);
	}
	else if (g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_WORLD)
	{
		r_draw_classify = (
			DRAW_CLASSIFY_SKYBOX | 
			DRAW_CLASSIFY_WORLD | 
			DRAW_CLASSIFY_DECAL | 
			DRAW_CLASSIFY_LIGHTMAP
			);
	}
	else if (g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_ENTITY)
	{
		r_draw_classify = (
			DRAW_CLASSIFY_SKYBOX |
			DRAW_CLASSIFY_WORLD | 
			DRAW_CLASSIFY_OPAQUE_ENTITIES | 
			DRAW_CLASSIFY_TRANS_ENTITIES |
			DRAW_CLASSIFY_PARTICLES | 
			DRAW_CLASSIFY_LOCAL_PLAYER |
			DRAW_CLASSIFY_DECAL |
			DRAW_CLASSIFY_LIGHTMAP
			);
	}

	R_RenderScene();

	r_draw_classify = old_draw_classify;

	(*cl_waterlevel) = saved_cl_waterlevel;

	R_PopRefDef();

	r_draw_refractview = false;
	g_CurrentReflectCache = NULL;

	GL_SetCurrentSceneFBO(NULL);

	ReflectCache->refractmap_ready = true;

	GL_EndDebugGroup();
}

void R_RenderWaterReflectView(CWaterReflectCache* ReflectCache)
{
	GL_BeginDebugGroup("R_RenderWaterReflectView");

	r_draw_reflectview = true;
	g_CurrentReflectCache = ReflectCache;

	GL_BindFrameBufferWithTextures(&s_WaterSurfaceFBO, ReflectCache->reflect_texture, 0, ReflectCache->reflect_depth_texture, ReflectCache->texwidth, ReflectCache->texheight);
	GL_SetCurrentSceneFBO(&s_WaterSurfaceFBO);

	vec4_t vecClearColor = { ReflectCache->color.r / 255.0f, ReflectCache->color.g / 255.0f, ReflectCache->color.b / 255.0f, 0 };

	GammaToLinear(vecClearColor);

	GL_ClearColorDepthStencil(vecClearColor, 1, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

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

	auto old_draw_classify = r_draw_classify;

	if (g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_SKYBOX)
	{
		r_draw_classify &= ~DRAW_CLASSIFY_WATER;
		r_draw_classify &= ~DRAW_CLASSIFY_WORLD;
		r_draw_classify &= ~DRAW_CLASSIFY_OPAQUE_ENTITIES;
		r_draw_classify &= ~DRAW_CLASSIFY_TRANS_ENTITIES;
		r_draw_classify &= ~DRAW_CLASSIFY_PARTICLES;
	}
	else if (g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_WORLD)
	{
		r_draw_classify &= ~DRAW_CLASSIFY_WATER;
		r_draw_classify &= ~DRAW_CLASSIFY_OPAQUE_ENTITIES;
		r_draw_classify &= ~DRAW_CLASSIFY_TRANS_ENTITIES;
		r_draw_classify &= ~DRAW_CLASSIFY_PARTICLES;
	}
	else if (g_CurrentReflectCache->level == WATER_LEVEL_REFLECT_SKYBOX)
	{
		r_draw_classify &= ~DRAW_CLASSIFY_WATER;
	}
	
	R_RenderScene();

	r_draw_classify = old_draw_classify;

	*cl_waterlevel = saved_cl_waterlevel;

	R_PopRefDef();

	r_draw_reflectview = false;
	g_CurrentReflectCache = NULL;

	GL_SetCurrentSceneFBO(NULL);

	ReflectCache->reflectmap_ready = true;

	GL_EndDebugGroup();
}

void R_RenderWaterPass_CollectWorldWater(const std::shared_ptr<CWaterSurfaceModel>& pWaterModel)
{
	vec3_t origin = { 0, 0, 0 };

	VectorSubtract((*r_refdef.vieworg), origin, modelorg);

	auto pplane = pWaterModel->plane;

	auto dot = DotProduct(modelorg, pWaterModel->normal) - pWaterModel->planedist;

	if (dot > 0)
	{
		g_VisibleWaterEntity.emplace_back(gEngfuncs.GetEntityByIndex(0));
		g_VisibleWaterSurfaceModels.emplace_back(pWaterModel);

		if ((*cl_waterlevel) >= 2)
		{
			auto pSourcePalette = pWaterModel->texture->pPal;

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

void R_RenderWaterPass_CollectEntityWater(cl_entity_t* e)
{
	vec3_t entity_mins, entity_maxs;

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

	auto pModel = R_GetWorldSurfaceModel(e->model);

	auto pLeaf = pModel->GetLeafByIndex(0);

	if (pLeaf)
	{
		for (size_t i = 0; i < pLeaf->m_vWaterSurfaceModels.size(); ++i)
		{
			const auto& pWaterModel = pLeaf->m_vWaterSurfaceModels[i];

			auto pplane = pWaterModel->plane;

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

			auto dot = DotProduct(modelorg, pWaterModel->normal) - pWaterModel->planedist;

			if (dot > 0)
			{
				g_VisibleWaterEntity.emplace_back(e);
				g_VisibleWaterSurfaceModels.emplace_back(pWaterModel);

				if ((*cl_waterlevel) >= 2)
				{
					auto pSourcePalette = pWaterModel->texture->pPal;

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
}

void R_RenderWaterPass(void)
{
	if (R_IsRenderingWaterView())
		return;

	GL_BeginDebugGroup("R_RenderWaterPass");

	g_VisibleWaterSurfaceModels.clear();
	g_VisibleWaterEntity.clear();
	R_ClearWaterReflectCaches();

	//Mainly for updating frustrum
	R_UpdateRefDef();

	mleaf_t* viewleaf = NULL;

	if (r_refdef_SvEngine && r_refdef_SvEngine->useCamera)
	{
		viewleaf = Mod_PointInLeaf(r_refdef_SvEngine->r_camera_origin, (*cl_worldmodel));
	}
	else
	{
		viewleaf = Mod_PointInLeaf(r_origin, (*cl_worldmodel));
	}

	R_SetupGL();
	R_SetFrustum();

	const auto& pModel = R_GetWorldSurfaceModel((*cl_worldmodel));

	int leafIndex = R_GetWorldLeafIndex((*cl_worldmodel), viewleaf);

	const auto& pLeaf = pModel->GetLeafByIndex(leafIndex);

	if (pLeaf)
	{
		for (size_t i = 0; i < pLeaf->m_vWaterSurfaceModels.size(); ++i)
		{
			const auto& pWaterModel = pLeaf->m_vWaterSurfaceModels[i];

			R_RenderWaterPass_CollectWorldWater(pWaterModel);
		}
	}

	for (int i = 0; i < (*cl_numvisedicts); ++i)
	{
		auto e = cl_visedicts[i];

		if (e->model && e->model->type == mod_brush)
		{
			R_RenderWaterPass_CollectEntityWater(e);
		}
	}

	for (size_t i = 0; i < g_VisibleWaterSurfaceModels.size(); ++i)
	{
		auto pWaterModel = g_VisibleWaterSurfaceModels[i];

		auto ent = g_VisibleWaterEntity[i];

		CWaterReflectCache* pReflectCache = NULL;

		if (pWaterModel->level >= WATER_LEVEL_REFLECT_SKYBOX && pWaterModel->level <= WATER_LEVEL_REFLECT_ENTITY && r_water->value > 0)
		{
			pReflectCache = R_PrepareReflectCache(ent, pWaterModel.get());
		}
		else if (pWaterModel->level == WATER_LEVEL_LEGACY_RIPPLE)
		{
			R_UpdateRippleTexture(pWaterModel.get(), (*r_framecount));
		}

		auto pEntityComponent = R_GetEntityComponentContainer(ent, true);

		if (pEntityComponent)
		{
			pEntityComponent->RenderWaterModels.emplace_back(pWaterModel);
			pEntityComponent->ReflectCaches.emplace_back(pReflectCache);
		}
	}

	for (int i = 0; i < g_iNumWaterReflectCaches; ++i)
	{
		if (g_WaterReflectCaches[i].used)
		{
			if (g_WaterReflectCaches[i].normal[2] > 0)
			{
				R_RenderWaterReflectView(&g_WaterReflectCaches[i]);
			}

			R_RenderWaterRefractView(&g_WaterReflectCaches[i]);
		}
	}

	GL_EndDebugGroup();
}

void R_DrawWaterSurfaceModelBegin(CWorldSurfaceLeaf* pLeaf, CWaterSurfaceModel* pWaterModel)
{
	auto pModel = pLeaf->m_pModel.lock();

	auto pWorldModel = pModel->m_pWorldModel.lock();
	
	GL_BindVAO(pWorldModel->hVAO);
	GL_BindABO(pWaterModel->hABO);
}

void R_DrawWaterSurfaceModelEnd()
{
	GL_BindABO(0);
	GL_BindVAO(0);
}

void R_DrawWaterSurfaceModelReflective(
	CWorldSurfaceModel* pModel,
	CWorldSurfaceLeaf* pLeaf,
	CWaterSurfaceModel* pWaterModel,
	CWaterReflectCache* pReflectCache,
	cl_entity_t* ent,
	bool bIsAboveWater,
	float color[4])
{

	GL_BeginDebugGroup("R_DrawWaterSurfaceModelReflective");

	R_SetRenderMode(ent);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	program_state_t WaterProgramState = 0;

	if (bIsAboveWater)
		WaterProgramState |= WATER_DEPTH_ENABLED;

	if ((*currententity)->curstate.rendermode == kRenderTransTexture || (*currententity)->curstate.rendermode == kRenderTransAdd)
	{
		WaterProgramState |= WATER_REFRACT_ENABLED;
		WaterProgramState |= WATER_ALPHA_BLEND_ENABLED;

		if (color[3] > pWaterModel->maxtrans)
			color[3] = pWaterModel->maxtrans;
	}

	if (!bIsAboveWater)
	{
		WaterProgramState |= WATER_UNDERWATER_ENABLED;
	}
	else
	{
		if (!R_IsRenderingGBuffer())
		{
			if ((WaterProgramState & WATER_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((WaterProgramState & WATER_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
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

				if (!R_IsRenderingGammaBlending() && (int)r_linear_fog_shift->value > 0)
				{
					WaterProgramState |= WATER_LINEAR_FOG_SHIFT_ENABLED;
				}
			}
		}
	}

	if (R_IsRenderingGBuffer())
	{
		WaterProgramState |= WATER_GBUFFER_ENABLED;
	}

	if (R_IsRenderingGammaBlending())
	{
		WaterProgramState |= WATER_GAMMA_BLEND_ENABLED;
	}

	if (r_draw_oitblend && (WaterProgramState & (WATER_ALPHA_BLEND_ENABLED | WATER_ADDITIVE_BLEND_ENABLED)))
	{
		WaterProgramState |= WATER_OIT_BLEND_ENABLED;
	}

	R_DrawWaterSurfaceModelBegin(pLeaf, pWaterModel);

	water_program_t prog = { 0 };
	R_UseWaterProgram(WaterProgramState, &prog);

	if (prog.u_watercolor != -1)
	{
		glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);
	}
	if (prog.u_depthfactor != -1)
	{
		glUniform3f(prog.u_depthfactor, pWaterModel->depthfactor[0], pWaterModel->depthfactor[1], pWaterModel->depthfactor[2]);
	}
	if (prog.u_fresnelfactor != -1)
	{
		glUniform4f(prog.u_fresnelfactor, pWaterModel->fresnelfactor[0], pWaterModel->fresnelfactor[1], pWaterModel->fresnelfactor[2], pWaterModel->fresnelfactor[3]);
	}
	if (prog.u_normfactor != -1)
	{
		glUniform1f(prog.u_normfactor, pWaterModel->normfactor);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_BindTextureUnit(WATER_BIND_BASE_TEXTURE, GL_TEXTURE_2D, pWaterModel->texture->gl_texturenum);

	GL_BindTextureUnit(WATER_BIND_NORMAL_TEXTURE, GL_TEXTURE_2D, pWaterModel->normalmap);

	if (pReflectCache && pReflectCache->refractmap_ready)
	{
		GL_BindTextureUnit(WATER_BIND_REFRACT_TEXTURE, GL_TEXTURE_2D, pReflectCache->refract_texture);
		GL_BindTextureUnit(WATER_BIND_REFRACT_DEPTH_TEXTURE, GL_TEXTURE_2D, pReflectCache->refract_depth_texture);
	}

	if (pReflectCache && pReflectCache->reflectmap_ready)
	{
		GL_BindTextureUnit(WATER_BIND_REFLECT_TEXTURE, GL_TEXTURE_2D, pReflectCache->reflect_texture);
		GL_BindTextureUnit(WATER_BIND_REFLECT_DEPTH_TEXTURE, GL_TEXTURE_2D, pReflectCache->reflect_depth_texture);
	}

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(0), pWaterModel->drawCount, 0);

	(*c_brush_polys) += pWaterModel->polyCount;

	GL_BindTextureUnit(WATER_BIND_NORMAL_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(WATER_BIND_BASE_TEXTURE, GL_TEXTURE_2D, 0);

	if (pReflectCache && pReflectCache->reflectmap_ready)
	{
		GL_BindTextureUnit(WATER_BIND_REFLECT_TEXTURE, GL_TEXTURE_2D, 0);
		GL_BindTextureUnit(WATER_BIND_REFLECT_DEPTH_TEXTURE, GL_TEXTURE_2D, 0);
	}

	if (pReflectCache && pReflectCache->refractmap_ready)
	{
		GL_BindTextureUnit(WATER_BIND_REFRACT_TEXTURE, GL_TEXTURE_2D, 0);
		GL_BindTextureUnit(WATER_BIND_REFRACT_DEPTH_TEXTURE, GL_TEXTURE_2D, 0);
	}

	glDisable(GL_BLEND);

	GL_UseProgram(0);

	R_DrawWaterSurfaceModelEnd();

	GL_EndDebugGroup();
}

void R_DrawWaterSurfaceModelRipple(
	CWorldSurfaceModel* pModel,
	CWorldSurfaceLeaf* pLeaf,
	CWaterSurfaceModel* pWaterModel,
	CWaterReflectCache* pReflectCache,
	cl_entity_t* ent,
	bool bIsAboveWater,
	float color[4])
{
	GL_BeginDebugGroup("R_DrawWaterSurfaceModelRipple");

	R_SetRenderMode(ent);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	program_state_t WaterProgramState = WATER_LEGACY_ENABLED;

	if (!bIsAboveWater)
	{

	}
	else
	{
		if (!R_IsRenderingGBuffer())
		{
			if ((WaterProgramState & WATER_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((WaterProgramState & WATER_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
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

				if (!R_IsRenderingGammaBlending() && (int)r_linear_fog_shift->value > 0)
				{
					WaterProgramState |= WATER_LINEAR_FOG_SHIFT_ENABLED;
				}
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

	if (R_IsRenderingGammaBlending())
	{
		WaterProgramState |= WATER_GAMMA_BLEND_ENABLED;
	}

	if (r_draw_oitblend && (WaterProgramState & (WATER_ALPHA_BLEND_ENABLED | WATER_ADDITIVE_BLEND_ENABLED)))
	{
		WaterProgramState |= WATER_OIT_BLEND_ENABLED;
	}

	R_DrawWaterSurfaceModelBegin(pLeaf, pWaterModel);

	//TODO: blend state?

	water_program_t prog = { 0 };
	R_UseWaterProgram(WaterProgramState, &prog);

	if (prog.u_watercolor != -1)
		glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);

	if (prog.u_scale != -1)
		glUniform1f(prog.u_scale, 0);

	if (prog.u_speed != -1)
		glUniform1f(prog.u_speed, 0);

	GL_BindTextureUnit(WATER_BIND_BASE_TEXTURE, GL_TEXTURE_2D, pWaterModel->ripplemap);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(0), pWaterModel->drawCount, 0);

	(*c_brush_polys) += pWaterModel->polyCount;

	GL_BindTextureUnit(WATER_BIND_BASE_TEXTURE, GL_TEXTURE_2D, 0);

	GL_UseProgram(0);

	//TODO: blend state?

	R_DrawWaterSurfaceModelEnd();

	GL_EndDebugGroup();
}

void R_DrawWaterSurfaceModelLegacy(
	CWorldSurfaceModel* pModel,
	CWorldSurfaceLeaf* pLeaf,
	CWaterSurfaceModel* pWaterModel,
	CWaterReflectCache* pReflectCache,
	cl_entity_t* ent,
	bool bIsAboveWater,
	float color[4])
{
	GL_BeginDebugGroup("R_DrawWaterSurfaceModelLegacy");

	R_SetRenderMode(ent);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

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
			if ((WaterProgramState & WATER_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((WaterProgramState & WATER_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else if (R_IsRenderingFog())
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

				if (!R_IsRenderingGammaBlending() && (int)r_linear_fog_shift->value > 0)
				{
					WaterProgramState |= WATER_LINEAR_FOG_SHIFT_ENABLED;
				}
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

	if (R_IsRenderingGammaBlending())
	{
		WaterProgramState |= WATER_GAMMA_BLEND_ENABLED;
	}

	if (r_draw_oitblend && (WaterProgramState & (WATER_ALPHA_BLEND_ENABLED | WATER_ADDITIVE_BLEND_ENABLED)))
	{
		WaterProgramState |= WATER_OIT_BLEND_ENABLED;
	}

	R_DrawWaterSurfaceModelBegin(pLeaf, pWaterModel);

	//TODO: blend state?

	water_program_t prog = { 0 };
	R_UseWaterProgram(WaterProgramState, &prog);

	if (prog.u_watercolor != -1)
		glUniform4f(prog.u_watercolor, color[0], color[1], color[2], color[3]);

	if (prog.u_scale != -1)
		glUniform1f(prog.u_scale, scale);

	if (prog.u_speed != -1)
		glUniform1f(prog.u_speed, pWaterModel->speedrate);

	GL_BindTextureUnit(WATER_BIND_BASE_TEXTURE, GL_TEXTURE_2D, pWaterModel->texture->gl_texturenum);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(0), pWaterModel->drawCount, 0);

	(*c_brush_polys) += pWaterModel->polyCount;

	GL_BindTextureUnit(WATER_BIND_BASE_TEXTURE, GL_TEXTURE_2D, 0);

	GL_UseProgram(0);

	//TODO: blend state?

	R_DrawWaterSurfaceModelEnd();

	GL_EndDebugGroup();
}

void R_DrawWaterSurfaceModel(
	CWorldSurfaceModel* pModel,
	CWorldSurfaceLeaf* pLeaf, 
	CWaterSurfaceModel* pWaterModel,
	CWaterReflectCache* pReflectCache, 
	cl_entity_t* ent)
{
	if (!pWaterModel->drawCount)
		return;

	bool bIsAboveWater = (pWaterModel->normal[2] > 0) && R_IsAboveWater(pWaterModel) ? true : false;

	float color[4];
	color[0] = pWaterModel->color.r / 255.0f;
	color[1] = pWaterModel->color.g / 255.0f;
	color[2] = pWaterModel->color.b / 255.0f;
	color[3] = 1;

	if ((*currententity)->curstate.rendermode == kRenderTransTexture)
		color[3] = (*r_blend);

	if (pWaterModel->level >= WATER_LEVEL_REFLECT_SKYBOX && pWaterModel->level <= WATER_LEVEL_REFLECT_ENTITY && (int)r_water->value > 0)
	{
		R_DrawWaterSurfaceModelReflective(
			pModel,
			pLeaf,
			pWaterModel,
			pReflectCache,
			ent,
			bIsAboveWater,
			color
		);
	}
	else if (pWaterModel->level == WATER_LEVEL_LEGACY_RIPPLE && (int)r_water->value > 0)
	{
		R_DrawWaterSurfaceModelRipple(
			pModel,
			pLeaf,
			pWaterModel,
			pReflectCache,
			ent,
			bIsAboveWater,
			color
		);
	}
	else
	{
		R_DrawWaterSurfaceModelLegacy(
			pModel,
			pLeaf,
			pWaterModel,
			pReflectCache,
			ent,
			bIsAboveWater,
			color
		);
	}
}

void R_DrawWaters(CWorldSurfaceModel* pModel, CWorldSurfaceLeaf* pLeaf, cl_entity_t* ent)
{
	if (!(r_draw_classify & DRAW_CLASSIFY_WATER))
		return;

	if (!pLeaf->m_vWaterSurfaceModels.size())
		return;

	auto pEntityComponentContainer = R_GetEntityComponentContainer(ent, false);

	if (!pEntityComponentContainer)
		return;

	GL_BeginDebugGroup("R_DrawWaters");

	for (size_t i = 0; i < pEntityComponentContainer->RenderWaterModels.size(); ++i)
	{
		const auto& pWaterModel = pEntityComponentContainer->RenderWaterModels[i];
		auto ReflectCache = pEntityComponentContainer->ReflectCaches[i];

		R_DrawWaterSurfaceModel(
			pModel,
			pLeaf, 
			pWaterModel.get(),
			ReflectCache,
			ent);
	}

	GL_EndDebugGroup();

}