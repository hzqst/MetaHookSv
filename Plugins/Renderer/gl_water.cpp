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
{ WATER_EXP2_FOG_ENABLED				, "WATER_EXP2_FOG_ENABLED"			 },
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
			GL_DeleteTexture(VBOCache->depthreflmap);

		if (VBOCache->reflectmap)
			GL_DeleteTexture(VBOCache->reflectmap);

		if (VBOCache->ripplemap)
			GL_DeleteTexture(VBOCache->ripplemap);

		if (VBOCache->hEBO)
			GL_DeleteBuffer(VBOCache->hEBO);

		if (VBOCache->ripple_data)
			free(VBOCache->ripple_data);
		if (VBOCache->ripple_image)
			free(VBOCache->ripple_image);
		if (VBOCache->ripple_spots[0])
			free(VBOCache->ripple_spots[0]);
		if (VBOCache->ripple_spots[1])
			free(VBOCache->ripple_spots[1]);

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

void R_UpdateRippleTexture(water_vbo_t *VBOCache, int framecount)
{
	if (r_draw_pass != r_draw_normal)
		return;

#define RANDOM_BYTES_SIZE 256
#define PROCEDURAL_SPEED_BITS	5
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

	if (VBOCache->ripple_framecount + (1 << PROCEDURAL_SPEED_BITS) > framecount)
		return;

	VBOCache->ripple_framecount = framecount;
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

	int procFrame = (framecount >> PROCEDURAL_SPEED_BITS);
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

void GL_UploadRGBA8(byte *data, int width, int height, qboolean mipmap, qboolean ansio, int wrap);

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

			VBOCache->hEBO = GL_GenBuffer();

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

				if (waterControl->level == WATER_LEVEL_LEGACY_RIPPLE)
				{
					glBindTexture(GL_TEXTURE_2D, surf->texinfo->texture->gl_texturenum);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &VBOCache->ripple_width);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &VBOCache->ripple_height);

					auto imageSize = VBOCache->ripple_width * VBOCache->ripple_height;

					VBOCache->ripple_data = (unsigned int*)malloc(imageSize * sizeof(unsigned int));
					memset(VBOCache->ripple_data, 0, imageSize * sizeof(unsigned int));

					VBOCache->ripple_image = (unsigned int*)malloc(imageSize * sizeof(unsigned int));
					memset(VBOCache->ripple_image, 0, imageSize * sizeof(unsigned long));

					glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, VBOCache->ripple_image);
					
					VBOCache->ripplemap = GL_GenTextureRGBA8(VBOCache->ripple_width, VBOCache->ripple_height);
					
					//Upload original image data using GL_UploadRGBA8, otherwise this texture will not work.

					glBindTexture(GL_TEXTURE_2D, VBOCache->ripplemap);

					int filter_min = *gl_filter_min;
					int filter_max = *gl_filter_max; 
					
					*gl_filter_min = GL_NEAREST;
					*gl_filter_max = GL_NEAREST;

					GL_UploadRGBA8((byte *)VBOCache->ripple_image, VBOCache->ripple_width, VBOCache->ripple_height, true, true, GL_REPEAT);

					*gl_filter_min = filter_min;
					*gl_filter_max = filter_max;

					VBOCache->ripple_spots[0] = (short *)malloc(imageSize * sizeof(short));
					memset(VBOCache->ripple_spots[0], 0, imageSize * sizeof(short));

					VBOCache->ripple_spots[1] = (short *)malloc(imageSize * sizeof(short));
					memset(VBOCache->ripple_spots[1], 0, imageSize * sizeof(short));

					VBOCache->ripple_shift = 0;
					VBOCache->ripple_framecount = 0;

					glBindTexture(GL_TEXTURE_2D, *currenttexture);
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

	if(VBOCache->level == WATER_LEVEL_LEGACY_RIPPLE)
		R_UpdateRippleTexture(VBOCache, (*r_framecount));

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