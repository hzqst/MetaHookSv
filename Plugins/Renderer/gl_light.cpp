#include "gl_local.h"
#include <sstream>

cvar_t *r_light_dynamic = NULL;
cvar_t *r_light_debug = NULL;

MapConVar *r_flashlight_ambient = NULL;
MapConVar *r_flashlight_diffuse = NULL;
MapConVar *r_flashlight_specular = NULL;
MapConVar *r_flashlight_specularpow = NULL;
MapConVar *r_flashlight_attachment = NULL;
MapConVar *r_flashlight_distance = NULL;
MapConVar *r_flashlight_cone_cosine = NULL;

MapConVar *r_dynlight_ambient = NULL;
MapConVar *r_dynlight_diffuse = NULL;
MapConVar *r_dynlight_specular = NULL;
MapConVar *r_dynlight_specularpow = NULL;

cvar_t *r_ssr = NULL;
MapConVar *r_ssr_ray_step = NULL;
MapConVar *r_ssr_iter_count = NULL;
MapConVar *r_ssr_distance_bias = NULL;
MapConVar *r_ssr_exponential_step= NULL;
MapConVar *r_ssr_adaptive_step = NULL;
MapConVar *r_ssr_binary_search = NULL;
MapConVar *r_ssr_fade = NULL;

bool drawgbuffer = false;

int gbuffer_mask = -1;
GLuint gbuffer_attachments[GBUFFER_INDEX_MAX] = {0};
int gbuffer_attachment_count = 0;

GLuint r_sphere_vbo = 0;
GLuint r_sphere_ebo = 0;
GLuint r_cone_vbo = 0;

GLuint r_flashlight_cone_texture = 0;
std::string r_flashlight_cone_texture_name;

std::vector<light_dynamic_t> g_DynamicLights;

std::unordered_map<int, dfinal_program_t> g_DFinalProgramTable;

void R_UseDFinalProgram(int state, dfinal_program_t *progOutput)
{
	dfinal_program_t prog = { 0 };

	auto itor = g_DFinalProgramTable.find(state);
	if (itor == g_DFinalProgramTable.end())
	{
		std::stringstream defs;

		if (state & DFINAL_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & DFINAL_EXP_FOG_ENABLED)
			defs << "#define EXP_FOG_ENABLED\n";

		if (state & DFINAL_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

		if (state & DFINAL_SKY_FOG_ENABLED)
			defs << "#define SKY_FOG_ENABLED\n";

		if (state & DFINAL_SSR_ENABLED)
			defs << "#define SSR_ENABLED\n";

		if (state & DFINAL_SSR_ADAPTIVE_STEP_ENABLED)
			defs << "#define SSR_ADAPTIVE_STEP_ENABLED\n";

		if (state & DFINAL_SSR_EXPONENTIAL_STEP_ENABLED)
			defs << "#define SSR_EXPONENTIAL_STEP_ENABLED\n";

		if (state & DFINAL_SSR_BINARY_SEARCH_ENABLED)
			defs << "#define SSR_BINARY_SEARCH_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\dfinal_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, u_ssrRayStep, "u_ssrRayStep");
			SHADER_UNIFORM(prog, u_ssrIterCount, "u_ssrIterCount");
			SHADER_UNIFORM(prog, u_ssrDistanceBias, "u_ssrDistanceBias");
			SHADER_UNIFORM(prog, u_ssrFade, "u_ssrFade");
		}

		g_DFinalProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (prog.u_ssrRayStep != -1)
			glUniform1f(prog.u_ssrRayStep, r_ssr_ray_step->GetValue());

		if (prog.u_ssrIterCount != -1)
			glUniform1i(prog.u_ssrIterCount, r_ssr_iter_count->GetValue());

		if (prog.u_ssrDistanceBias != -1)
			glUniform1f(prog.u_ssrDistanceBias, r_ssr_distance_bias->GetValue());

		if (prog.u_ssrFade != -1)
			glUniform2f(prog.u_ssrFade, r_ssr_fade->GetValues()[0], r_ssr_fade->GetValues()[1]);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		g_pMetaHookAPI->SysError("R_UseDFinalProgram: Failed to load program!");
	}
}

const program_state_name_t s_DFinalProgramStateName[] = {
{ DFINAL_LINEAR_FOG_ENABLED				,"DFINAL_LINEAR_FOG_ENABLED"			},
{ DFINAL_EXP_FOG_ENABLED				,"DFINAL_EXP_FOG_ENABLED"				},
{ DFINAL_EXP2_FOG_ENABLED				,"DFINAL_EXP2_FOG_ENABLED"				},
{ DFINAL_SKY_FOG_ENABLED				,"DFINAL_SKY_FOG_ENABLED"				},
{ DFINAL_SSR_ENABLED					,"DFINAL_SSR_ENABLED"					},
{ DFINAL_SSR_ADAPTIVE_STEP_ENABLED		,"DFINAL_SSR_ADAPTIVE_STEP_ENABLED"		},
{ DFINAL_SSR_EXPONENTIAL_STEP_ENABLED	,"DFINAL_SSR_EXPONENTIAL_STEP_ENABLED"	},
{ DFINAL_SSR_BINARY_SEARCH_ENABLED		,"DFINAL_SSR_BINARY_SEARCH_ENABLED"		},
};

void R_SaveDFinalProgramStates(void)
{
	std::stringstream ss;
	for (auto &p : g_DFinalProgramTable)
	{
		if (p.first == 0)
		{
			ss << "NONE";
		}
		else
		{
			for (int i = 0; i < _ARRAYSIZE(s_DFinalProgramStateName); ++i)
			{
				if (p.first & s_DFinalProgramStateName[i].state)
				{
					ss << s_DFinalProgramStateName[i].name << " ";
				}
			}
		}
		ss << "\n";
	}

	auto FileHandle = g_pFileSystem->Open("renderer/shader/dfinal_cache.txt", "wt");
	if (FileHandle)
	{
		auto str = ss.str();
		g_pFileSystem->Write(str.data(), str.length(), FileHandle);
		g_pFileSystem->Close(FileHandle);
	}
}

void R_LoadDFinalProgramStates(void)
{
	auto FileHandle = g_pFileSystem->Open("renderer/shader/dfinal_cache.txt", "rt");
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
						for (int i = 0; i < _ARRAYSIZE(s_DFinalProgramStateName); ++i)
						{
							if (!strcmp(token, s_DFinalProgramStateName[i].name))
							{
								if (ProgramState == -1)
									ProgramState = 0;
								ProgramState |= s_DFinalProgramStateName[i].state;
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
				R_UseDFinalProgram(ProgramState, NULL);
		}
		g_pFileSystem->Close(FileHandle);
	}

	GL_UseProgram(0);
}

std::unordered_map<int, dlight_program_t> g_DLightProgramTable;

void R_UseDLightProgram(int state, dlight_program_t *progOutput)
{
	dlight_program_t prog = { 0 };

	auto itor = g_DLightProgramTable.find(state);
	if (itor == g_DLightProgramTable.end())
	{
		std::stringstream defs;

		if (state & DLIGHT_SPOT_ENABLED)
			defs << "#define SPOT_ENABLED\n";

		if (state & DLIGHT_POINT_ENABLED)
			defs << "#define POINT_ENABLED\n";

		if (state & DLIGHT_VOLUME_ENABLED)
			defs << "#define VOLUME_ENABLED\n";

		if (state & DLIGHT_CONE_TEXTURE_ENABLED)
			defs << "#define CONE_TEXTURE_ENABLED\n";

		if (state & DLIGHT_SHADOW_TEXTURE_ENABLED)
			defs << "#define SHADOW_TEXTURE_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\dlight_shader.vsh", "renderer\\shader\\dlight_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, u_lightdir, "u_lightdir");
			SHADER_UNIFORM(prog, u_lightright, "u_lightright");
			SHADER_UNIFORM(prog, u_lightup, "u_lightup");
			SHADER_UNIFORM(prog, u_lightpos, "u_lightpos");
			SHADER_UNIFORM(prog, u_lightcolor, "u_lightcolor");
			SHADER_UNIFORM(prog, u_lightcone, "u_lightcone");
			SHADER_UNIFORM(prog, u_lightradius, "u_lightradius");
			SHADER_UNIFORM(prog, u_lightambient, "u_lightambient");
			SHADER_UNIFORM(prog, u_lightdiffuse, "u_lightdiffuse");
			SHADER_UNIFORM(prog, u_lightspecular, "u_lightspecular");
			SHADER_UNIFORM(prog, u_lightspecularpow, "u_lightspecularpow");
			SHADER_UNIFORM(prog, u_shadowtexel, "u_shadowtexel");
			SHADER_UNIFORM(prog, u_shadowmatrix, "u_shadowmatrix");
			SHADER_UNIFORM(prog, u_modelmatrix, "u_modelmatrix");
		}

		g_DLightProgramTable[state] = prog;
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
		g_pMetaHookAPI->SysError("R_UseDLightProgram: Failed to load program!");
	}
}

const program_state_name_t s_DLightProgramStateName[] = {
{ DLIGHT_SPOT_ENABLED				,"DLIGHT_SPOT_ENABLED"	 },
{ DLIGHT_POINT_ENABLED				,"DLIGHT_POINT_ENABLED"	 },
{ DLIGHT_VOLUME_ENABLED				,"DLIGHT_VOLUME_ENABLED" },
{ DLIGHT_CONE_TEXTURE_ENABLED		,"DLIGHT_CONE_TEXTURE_ENABLED" },
{ DLIGHT_SHADOW_TEXTURE_ENABLED		,"DLIGHT_SHADOW_TEXTURE_ENABLED" },
};

void R_SaveDLightProgramStates(void)
{
	std::stringstream ss;
	for (auto &p : g_DLightProgramTable)
	{
		if (p.first == 0)
		{
			ss << "NONE";
		}
		else
		{
			for (int i = 0; i < _ARRAYSIZE(s_DLightProgramStateName); ++i)
			{
				if (p.first & s_DLightProgramStateName[i].state)
				{
					ss << s_DLightProgramStateName[i].name << " ";
				}
			}
		}
		ss << "\n";
	}

	auto FileHandle = g_pFileSystem->Open("renderer/shader/dlight_cache.txt", "wt");
	if (FileHandle)
	{
		auto str = ss.str();
		g_pFileSystem->Write(str.data(), str.length(), FileHandle);
		g_pFileSystem->Close(FileHandle);
	}
}

void R_LoadDLightProgramStates(void)
{
	auto FileHandle = g_pFileSystem->Open("renderer/shader/dlight_cache.txt", "rt");
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
						for (int i = 0; i < _ARRAYSIZE(s_DLightProgramStateName); ++i)
						{
							if (!strcmp(token, s_DLightProgramStateName[i].name))
							{
								if (ProgramState == -1)
									ProgramState = 0;
								ProgramState |= s_DLightProgramStateName[i].state;
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
				R_UseDLightProgram(ProgramState, NULL);
		}
		g_pFileSystem->Close(FileHandle);
	}

	GL_UseProgram(0);
}

void R_ShutdownLight(void)
{
	g_DFinalProgramTable.clear();
	g_DLightProgramTable.clear();

	if (r_sphere_vbo)
	{
		GL_DeleteBuffer(r_sphere_vbo);
		r_sphere_vbo = NULL;
	}

	if (r_sphere_ebo)
	{
		GL_DeleteBuffer(r_sphere_ebo);
		r_sphere_ebo = NULL;
	}

	if (r_cone_vbo)
	{
		GL_DeleteBuffer(r_cone_vbo);
		r_cone_vbo = NULL;
	}
}

void R_InitLight(void)
{
	r_light_dynamic = gEngfuncs.pfnRegisterVariable("r_light_dynamic", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_debug = gEngfuncs.pfnRegisterVariable("r_light_debug", "0", FCVAR_CLIENTDLL);

	r_dynlight_ambient = R_RegisterMapCvar("r_dynlight_ambient", "0.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_diffuse = R_RegisterMapCvar("r_dynlight_diffuse", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_specular = R_RegisterMapCvar("r_dynlight_specular", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_specularpow = R_RegisterMapCvar("r_dynlight_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_flashlight_ambient = R_RegisterMapCvar("r_flashlight_ambient", "0.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_diffuse = R_RegisterMapCvar("r_flashlight_diffuse", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_specular = R_RegisterMapCvar("r_flashlight_specular", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_specularpow = R_RegisterMapCvar("r_flashlight_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_attachment = R_RegisterMapCvar("r_flashlight_attachment", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_flashlight_distance = R_RegisterMapCvar("r_flashlight_distance", "2000", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_cone_cosine = R_RegisterMapCvar("r_flashlight_cone_cosine", "0.9", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_ssr = gEngfuncs.pfnRegisterVariable("r_ssr", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_ray_step = R_RegisterMapCvar("r_ssr_ray_step", "5.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_iter_count = R_RegisterMapCvar("r_ssr_iter_count", "64", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_distance_bias = R_RegisterMapCvar("r_ssr_distance_bias", "0.2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_adaptive_step = R_RegisterMapCvar("r_ssr_adaptive_step", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_exponential_step = R_RegisterMapCvar("r_ssr_exponential_step", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_binary_search = R_RegisterMapCvar("r_ssr_binary_search", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_fade = R_RegisterMapCvar("r_ssr_fade", "0.8 1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 2);

#define X_SEGMENTS 64
#define Y_SEGMENTS 64

	std::vector<float> sphereVertices;
	std::vector<int> sphereIndices;

	for (int y = 0; y <= Y_SEGMENTS; y++)
	{
		for (int x = 0; x <= X_SEGMENTS; x++)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
			float yPos = std::cos(ySegment * M_PI);
			float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
			sphereVertices.push_back(xPos);
			sphereVertices.push_back(yPos);
			sphereVertices.push_back(zPos);
		}
	}

	for (int i = 0; i < Y_SEGMENTS; i++)
	{
		for (int j = 0; j < X_SEGMENTS; j++)
		{
			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
			sphereIndices.push_back(i* (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j + 1);
		}
	}

	r_sphere_vbo = GL_GenBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, r_sphere_vbo);
	glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	r_sphere_ebo = GL_GenBuffer();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_sphere_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(int), sphereIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	std::vector<float> coneVertices;

	for (int x = 0; x < X_SEGMENTS; x++)
	{
		float xSegment = (float)x / (float)X_SEGMENTS;
		float xSegment2 = (float)(x + 1) / (float)X_SEGMENTS;

		//cone tri
		{
			coneVertices.push_back(0);
			coneVertices.push_back(0);
			coneVertices.push_back(0);

			float xPos2 = 1.0;
			float yPos2 = std::sin(xSegment2 * 2.0f * M_PI);
			float zPos2 = std::cos(xSegment2 * 2.0f * M_PI);
			coneVertices.push_back(xPos2);
			coneVertices.push_back(yPos2);
			coneVertices.push_back(zPos2);

			float xPos = 1.0;
			float yPos = std::sin(xSegment * 2.0f * M_PI);
			float zPos = std::cos(xSegment * 2.0f * M_PI);
			coneVertices.push_back(xPos);
			coneVertices.push_back(yPos);
			coneVertices.push_back(zPos);
		}

		//circle tri
		{
			coneVertices.push_back(1.0);
			coneVertices.push_back(0);
			coneVertices.push_back(0);

			float xPos = 1.0;
			float yPos = std::sin(xSegment * 2.0f * M_PI);
			float zPos = std::cos(xSegment * 2.0f * M_PI);
			coneVertices.push_back(xPos);
			coneVertices.push_back(yPos);
			coneVertices.push_back(zPos);

			float xPos2 = 1.0;
			float yPos2 = std::sin(xSegment2 * 2.0f * M_PI);
			float zPos2 = std::cos(xSegment2 * 2.0f * M_PI);
			coneVertices.push_back(xPos2);
			coneVertices.push_back(yPos2);
			coneVertices.push_back(zPos2);
		}
	}

	r_cone_vbo = GL_GenBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, r_cone_vbo);
	glBufferData(GL_ARRAY_BUFFER, coneVertices.size() * sizeof(float), coneVertices.data(), GL_STATIC_DRAW_ARB);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	drawgbuffer = false;
}

void R_NewMapLight(void)
{
	if (!r_flashlight_cone_texture_name.empty())
	{
		int found_texture = GL_FindTexture(r_flashlight_cone_texture_name.c_str(), GLT_WORLD, NULL, NULL);

		r_flashlight_cone_texture = found_texture ? found_texture : 
			R_LoadTextureEx(r_flashlight_cone_texture_name.c_str(), r_flashlight_cone_texture_name.c_str(), NULL, NULL, GLT_WORLD, true, true, true);
	}
	else
	{
		r_flashlight_cone_texture = 0;
	}
}

bool R_IsDLightFlashlight(dlight_t *dl)
{
	if (dl->key >= 1 && dl->key <= 32)
	{
		auto ent = gEngfuncs.GetEntityByIndex(dl->key);

		if (ent->curstate.effects & EF_DIMLIGHT)
		{
			return true;
		}
	}

	return false;
}

void R_SetGBufferBlend(int blendsrc, int blenddst)
{
	if (!drawgbuffer)
		return;

	for (int i = 0; i < gbuffer_attachment_count; ++i)
	{
		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_DIFFUSE)
			glBlendFunci(i, blendsrc, blenddst);

		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_LIGHTMAP)
			glBlendFunci(i, blendsrc, blenddst);

		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_WORLDNORM)
			glBlendFunci(i, GL_ONE, GL_ZERO);

		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_SPECULAR)
			glBlendFunci(i, GL_ONE, GL_ZERO);

		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_ADDITIVE)
			glBlendFunci(i, GL_ONE, GL_ONE);
	}
}

void R_SetGBufferMask(int mask)
{
	if (!drawgbuffer)
		return;

	if (gbuffer_mask == mask)
		return;

	gbuffer_mask = mask;

	gbuffer_attachment_count = 0;

	for (int i = 0; i < GBUFFER_INDEX_MAX; ++i)
	{
		if (mask & (1 << i))
		{
			gbuffer_attachments[gbuffer_attachment_count] = GL_COLOR_ATTACHMENT0 + i;
			gbuffer_attachment_count++;
		}
	}
	
	glDrawBuffers(gbuffer_attachment_count, gbuffer_attachments);
}

bool R_IsDeferredRenderingEnabled(void)
{
	if (!r_light_dynamic->value)
		return false;

	if (r_draw_shadowcaster)
		return false;

	if (r_draw_reflectview)
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	return true;
}

bool R_BeginRenderGBuffer(void)
{
	if (!R_IsDeferredRenderingEnabled())
		return false;

	drawgbuffer = true;
	gbuffer_mask = -1;

	glBindFramebuffer(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);

	R_SetGBufferMask(GBUFFER_MASK_ALL);
	R_SetGBufferBlend(GL_ONE, GL_ZERO);

	glClearColor(0, 0, 0, 0);
	glStencilMask(0xFF);
	glClearStencil(0);
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glStencilMask(0);

	return true;
}

bool Util_IsOriginInCone(float *org, float *cone_origin, float *cone_forward, float cone_cosine, float cone_distance)
{
	float dir[3];
	VectorSubtract(org, cone_origin, dir);

	float dist = VectorLength(dir);

	if (dist > cone_distance)
		return false;

	VectorNormalize(dir);

	auto dot = DotProduct(cone_forward, dir);

	return dot > cone_cosine;
}

void R_IterateDynamicLights(fnPointLightCallback pointlight_callback, fnSpotLightCallback spotlight_callback)
{
	for (size_t i = 0; i < g_DynamicLights.size(); i++)
	{
		auto &dynlight = g_DynamicLights[i];

		if (dynlight.type == DLIGHT_POINT)
		{
			float radius = dynlight.distance;

			vec3_t distToLight;
			VectorSubtract((*r_refdef.vieworg), dynlight.origin, distToLight);

			if (VectorLength(distToLight) > radius + 32)
			{
				vec3_t mins, maxs;
				for (int j = 0; j < 3; j++)
				{
					mins[j] = dynlight.origin[j] - radius;
					maxs[j] = dynlight.origin[j] + radius;
				}

				if (R_CullBox(mins, maxs))
					continue;

				if(pointlight_callback)
					pointlight_callback(radius, dynlight.origin, dynlight.color, dynlight.ambient, dynlight.diffuse, dynlight.specular, dynlight.specularpow, &dynlight.shadowtex, true);

			}
			else
			{
				if (pointlight_callback)
					pointlight_callback(radius, dynlight.origin, dynlight.color, dynlight.ambient, dynlight.diffuse, dynlight.specular, dynlight.specularpow, &dynlight.shadowtex, false);
			}
		}
	}

	int max_dlight = EngineGetMaxDLight();
	
	dlight_t *dl = cl_dlights;
	float curtime = (*cl_time);

	for (int i = 0; i < max_dlight; i++, dl++)
	{
		if (dl->die < curtime || !dl->radius)
			continue;

		if (R_IsDLightFlashlight(dl))
		{
			vec3_t dlight_origin;
			vec3_t dlight_angle;
			vec3_t dlight_vforward;
			vec3_t dlight_vright;
			vec3_t dlight_vup;

			auto ent = gEngfuncs.GetEntityByIndex(dl->key);

			bool bIsFromLocalPlayer = (ent == gEngfuncs.GetLocalPlayer()) ? true : false;

			vec3_t org;
			//first person mode
			if (bIsFromLocalPlayer && !gExportfuncs.CL_IsThirdPerson() && !chase_active->value && r_params.viewentity <= r_params.maxclients)
			{
				VectorCopy((*r_refdef.viewangles), dlight_angle);
				gEngfuncs.pfnAngleVectors(dlight_angle, dlight_vforward, dlight_vright, dlight_vup);

				if (cl_viewent && cl_viewent->model && r_flashlight_attachment->GetValue() > 0)
				{
					int attachmentIndex = (int)(r_flashlight_attachment->GetValue());
					VectorCopy(cl_viewent->attachment[clamp(attachmentIndex, 1, 4) - 1], org);
					VectorMA(org, -2, dlight_vup, org);
					VectorMA(org, -10, dlight_vforward, org);
				}
				else
				{
					VectorCopy((*r_refdef.vieworg), org);
					VectorMA(org, 2, dlight_vup, org);
					VectorMA(org, 10, dlight_vright, org);
				}

				VectorCopy(org, dlight_origin);
			}
			else
			{
				VectorCopy(ent->angles, dlight_angle);
				dlight_angle[0] = -dlight_angle[0];
				gEngfuncs.pfnAngleVectors(dlight_angle, dlight_vforward, dlight_vright, dlight_vup);

				VectorCopy(ent->origin, org);
				VectorMA(org, 8, dlight_vup, org);
				VectorMA(org, 10, dlight_vright, org);

				VectorCopy(org, dlight_origin);
			}

			float coneCosAngle = r_flashlight_cone_cosine->GetValue();
			float coneAngle = acosf(coneCosAngle);
			float coneSinAngle = sqrt(1 - coneCosAngle * coneCosAngle);
			float coneTanAngle = tanf(coneAngle);
			float distance = r_flashlight_distance->GetValue();
			float radius = distance * coneTanAngle;
			
			float ambient = r_flashlight_ambient->GetValue();
			float diffuse = r_flashlight_diffuse->GetValue();
			float specular = r_flashlight_specular->GetValue();
			float specularpow = r_flashlight_specularpow->GetValue();

			vec3_t color;
			color[0] = (float)dl->color.r / 255.0f;
			color[1] = (float)dl->color.g / 255.0f;
			color[2] = (float)dl->color.b / 255.0f;

			if (!Util_IsOriginInCone((*r_refdef.vieworg), dlight_origin, dlight_vforward, coneCosAngle, distance))
			{
				if(spotlight_callback)
					spotlight_callback(distance, radius,
					coneAngle, coneCosAngle, coneSinAngle, coneTanAngle,
					dlight_origin, dlight_angle, dlight_vforward, dlight_vright, dlight_vup,
					color, ambient, diffuse, specular, specularpow, &cl_dlight_shadow_textures[i], true, bIsFromLocalPlayer);
			}
			else
			{
				if (spotlight_callback)
					spotlight_callback(distance, radius,
					coneAngle, coneCosAngle, coneSinAngle, coneTanAngle,
					dlight_origin, dlight_angle, dlight_vforward, dlight_vright, dlight_vup,
					color, ambient, diffuse, specular, specularpow, &cl_dlight_shadow_textures[i], false, bIsFromLocalPlayer);
			}			
		}
		else
		{
			vec3_t color;
			color[0] = (float)dl->color.r / 255.0f;
			color[1] = (float)dl->color.g / 255.0f;
			color[2] = (float)dl->color.b / 255.0f;

			float ambient = r_dynlight_ambient->GetValue();
			float diffuse = r_dynlight_diffuse->GetValue();
			float specular = r_dynlight_specular->GetValue();
			float specularpow = r_dynlight_specularpow->GetValue();

			vec3_t distToLight;
			VectorSubtract((*r_refdef.vieworg), dl->origin, distToLight);

			if (VectorLength(distToLight) > dl->radius + 32)
			{
				vec3_t mins, maxs;
				for (int j = 0; j < 3; j++)
				{
					mins[j] = dl->origin[j] - dl->radius;
					maxs[j] = dl->origin[j] + dl->radius;
				}

				if (R_CullBox(mins, maxs))
					continue;

				if(pointlight_callback)
					pointlight_callback(dl->radius, dl->origin, color, ambient, diffuse, specular, specularpow, &cl_dlight_shadow_textures[i], true);
			}
			else
			{
				if (pointlight_callback)
					pointlight_callback(dl->radius, dl->origin, color, ambient, diffuse, specular, specularpow, &cl_dlight_shadow_textures[i], false);
			}
		}
	}
}

void R_EndRenderGBuffer(void)
{
	GL_BeginFullScreenQuad(false);

	R_LinearizeDepth(&s_GBufferFBO);

	if (R_IsSSAOEnabled())
	{
		R_AmbientOcclusion();
	}
	else
	{
		//Write to GBuffer->lightmap only
		glBindFramebuffer(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
	}

	GL_EndFullScreenQuad();

	static glprofile_t profile_EndRenderGBuffer;
	GL_BeginProfile(&profile_EndRenderGBuffer, "R_EndRenderGBuffer");

	//Disable depth write and re-enable later after light pass.
	glDepthMask(0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	//Texture unit 0 = GBuffer texture array

	GL_SelectTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, s_GBufferFBO.s_hBackBufferTex);
	*currenttexture = -1;

	//Texture unit 1 = Depth texture
	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferDepthTex);

	//Texture unit 2 = Stencil texture
	if (s_GBufferFBO.s_hBackBufferStencilView)
	{
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferStencilView);
	}

	//Texture unit 3 = Flashlight cone texture
	if (r_flashlight_cone_texture)
	{
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, r_flashlight_cone_texture);
	}

	R_IterateDynamicLights([](float radius, vec3_t origin, vec3_t color, float ambient, float diffuse, float specular, float specularpow, shadow_texture_t *shadowtex, bool bVolume)
	{
		if (bVolume)
		{
			glBindBuffer(GL_ARRAY_BUFFER, r_sphere_vbo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_sphere_ebo);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

			glPushMatrix();
			glLoadIdentity();
			glTranslatef(origin[0], origin[1], origin[2]);
			glScalef(radius, radius, radius);

			float modelmatrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
			glPopMatrix();

			int DLightProgramState = DLIGHT_POINT_ENABLED | DLIGHT_VOLUME_ENABLED;

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			glUniformMatrix4fv(prog.u_modelmatrix, 1, false, modelmatrix);
			glUniform3f(prog.u_lightpos, origin[0], origin[1], origin[2]);
			glUniform3f(prog.u_lightcolor, color[0], color[1], color[2]);
			glUniform1f(prog.u_lightradius, radius);
			glUniform1f(prog.u_lightambient, ambient);
			glUniform1f(prog.u_lightdiffuse, diffuse);
			glUniform1f(prog.u_lightspecular, specular);
			glUniform1f(prog.u_lightspecularpow, specularpow);

			glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

			glDisableVertexAttribArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		else
		{
			GL_BeginFullScreenQuad(false);

			int DLightProgramState = DLIGHT_POINT_ENABLED;

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);
			glUniform3f(prog.u_lightpos, origin[0], origin[1], origin[2]);
			glUniform3f(prog.u_lightcolor, color[0], color[1], color[2]);
			glUniform1f(prog.u_lightradius, radius);
			glUniform1f(prog.u_lightambient, ambient);
			glUniform1f(prog.u_lightdiffuse, diffuse);
			glUniform1f(prog.u_lightspecular, specular);
			glUniform1f(prog.u_lightspecularpow, specularpow);

			glDrawArrays(GL_QUADS, 0, 4);

			GL_EndFullScreenQuad();
		}
	}, 
	[](float distance, float radius,
		float coneAngle, float coneCosAngle, float coneSinAngle, float coneTanAngle,
		vec3_t origin, vec3_t angle, vec3_t vforward, vec3_t vright, vec3_t vup,
		vec3_t color, float ambient, float diffuse, float specular, float specularpow, shadow_texture_t *shadowtex, bool bVolume, bool bIsFromLocalPlayer)
	{

		if (bVolume)
		{
			glBindBuffer(GL_ARRAY_BUFFER, r_cone_vbo);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

			glPushMatrix();
			glLoadIdentity();
			glTranslatef(origin[0], origin[1], origin[2]);
			glRotatef(angle[1], 0, 0, 1);
			glRotatef(angle[0], 0, 1, 0);
			glRotatef(angle[2], 1, 0, 0);
			glScalef(distance, radius, radius);

			float modelmatrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
			glPopMatrix();

			int DLightProgramState = DLIGHT_SPOT_ENABLED | DLIGHT_VOLUME_ENABLED;

			if (r_flashlight_cone_texture)
			{
				DLightProgramState |= DLIGHT_CONE_TEXTURE_ENABLED;
			}

			if (shadowtex->depth && shadowtex->ready)
			{
				DLightProgramState |= DLIGHT_SHADOW_TEXTURE_ENABLED;

				glActiveTexture(GL_TEXTURE4);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, shadowtex->depth);
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			glUniformMatrix4fv(prog.u_modelmatrix, 1, false, modelmatrix);
			glUniform3f(prog.u_lightdir, vforward[0], vforward[1], vforward[2]);
			glUniform3f(prog.u_lightright, vright[0], vright[1], vright[2]);
			glUniform3f(prog.u_lightup, vup[0], vup[1], vup[2]);
			glUniform3f(prog.u_lightpos, origin[0], origin[1], origin[2]);
			glUniform3f(prog.u_lightcolor, color[0], color[1], color[2]);
			glUniform2f(prog.u_lightcone, coneCosAngle, coneSinAngle);
			glUniform1f(prog.u_lightradius, distance);
			glUniform1f(prog.u_lightambient, ambient);
			glUniform1f(prog.u_lightdiffuse, diffuse);
			glUniform1f(prog.u_lightspecular, specular);
			glUniform1f(prog.u_lightspecularpow, specularpow);

			if (prog.u_shadowtexel != -1 && shadowtex->size > 0)
			{
				glUniform2f(prog.u_shadowtexel, shadowtex->size, 1.0f / (float)shadowtex->size);
			}

			if (prog.u_shadowmatrix != -1)
			{
				glUniformMatrix4fv(prog.u_shadowmatrix, 1, false, shadowtex->matrix);
			}

			glDrawArrays(GL_TRIANGLES, 0, X_SEGMENTS * 6);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDisableVertexAttribArray(0);

			if (DLightProgramState & DLIGHT_SHADOW_TEXTURE_ENABLED)
			{
				glDisable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		else
		{
			GL_BeginFullScreenQuad(false);

			int DLightProgramState = DLIGHT_SPOT_ENABLED;

			if (r_flashlight_cone_texture)
			{
				DLightProgramState |= DLIGHT_CONE_TEXTURE_ENABLED;
			}

			if (shadowtex->depth && shadowtex->ready)
			{
				DLightProgramState |= DLIGHT_SHADOW_TEXTURE_ENABLED;

				glActiveTexture(GL_TEXTURE4);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, shadowtex->depth);
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			glUniform3f(prog.u_lightdir, vforward[0], vforward[1], vforward[2]);
			glUniform3f(prog.u_lightright, vright[0], vright[1], vright[2]);
			glUniform3f(prog.u_lightup, vup[0], vup[1], vup[2]);
			glUniform3f(prog.u_lightpos, origin[0], origin[1], origin[2]);
			glUniform3f(prog.u_lightcolor, color[0], color[1], color[2]);
			glUniform2f(prog.u_lightcone, coneCosAngle, coneSinAngle);
			glUniform1f(prog.u_lightradius, distance);
			glUniform1f(prog.u_lightambient, ambient);
			glUniform1f(prog.u_lightdiffuse, diffuse);
			glUniform1f(prog.u_lightspecular, specular);
			glUniform1f(prog.u_lightspecularpow, specularpow);

			if (prog.u_shadowtexel != -1 && shadowtex->size > 0)
			{
				glUniform2f(prog.u_shadowtexel, shadowtex->size, 1.0f / (float)shadowtex->size);
			}

			if (prog.u_shadowmatrix != -1)
			{
				glUniformMatrix4fv(prog.u_shadowmatrix, 1, false, shadowtex->matrix);
			}

			glDrawArrays(GL_QUADS, 0, 4);

			GL_EndFullScreenQuad();

			if (DLightProgramState & DLIGHT_SHADOW_TEXTURE_ENABLED)
			{
				glDisable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

	});

	//Re-enable depth write
	glDepthMask(1);

	//Write GBuffer depth and stencil buffer into main framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
	glBlitFramebuffer(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight,
		0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight,
		GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
		GL_NEAREST);

	//Shading pass
	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GL_BeginFullScreenQuad(false);

	//No blend for final shading pass
	glDisable(GL_BLEND);

	int FinalProgramState = 0;

	if (r_fog_mode == GL_LINEAR)
		FinalProgramState |= DFINAL_LINEAR_FOG_ENABLED;
	else if (r_fog_mode == GL_EXP)
		FinalProgramState |= DFINAL_EXP_FOG_ENABLED;
	else if (r_fog_mode == GL_EXP2)
		FinalProgramState |= DFINAL_EXP2_FOG_ENABLED;

	if (r_wsurf_sky_fog->value)
	{
		FinalProgramState |= DFINAL_SKY_FOG_ENABLED;
	}

	if (r_ssr->value)
	{
		FinalProgramState |= DFINAL_SSR_ENABLED;

		if (r_ssr_adaptive_step->GetValue())
			FinalProgramState |= DFINAL_SSR_ADAPTIVE_STEP_ENABLED;

		if (r_ssr_exponential_step->GetValue())
			FinalProgramState |= DFINAL_SSR_EXPONENTIAL_STEP_ENABLED;

		if (r_ssr_binary_search->GetValue())
			FinalProgramState |= DFINAL_SSR_BINARY_SEARCH_ENABLED;
	}

	//Setup final program
	R_UseDFinalProgram(FinalProgramState, NULL);

	//Texture unit 0 = GBuffer texture array, Texture unit 1 = depth, Texture unit 2 = stencil (optional), Texture unit 3 = linearized depth 
	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_DepthLinearFBO.s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Disable texture unit 3 (linearized depth)
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE2);

	//Disable texture unit 2 (stencil)
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);

	//Disable texture unit 1 (depth)
	GL_DisableMultitexture();

	//Disable texture unit 0 (GBuffer texture array)
	glDisable(GL_TEXTURE_2D_ARRAY);
	glEnable(GL_TEXTURE_2D);
	*currenttexture = -1;

	GL_UseProgram(0);

	GL_EndFullScreenQuad();

	drawgbuffer = false;
	gbuffer_mask = -1;

	GL_EndProfile(&profile_EndRenderGBuffer);
}

void R_BlitGBufferToFrameBuffer(FBO_Container_t *fbo)
{
	//Write GBuffer depth and stencil buffer into main framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo->s_hBackBufferFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
	glBlitFramebuffer(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight,
		0, 0, fbo->iWidth, fbo->iHeight,
		GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
		GL_NEAREST);

	//Shading pass
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->s_hBackBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GL_BeginFullScreenQuad(false);

	//No blend for final shading pass
	glDisable(GL_BLEND);

	int FinalProgramState = 0;

	//Setup final program
	R_UseDFinalProgram(FinalProgramState, NULL);

	//Texture unit 0 = (GBuffer texture array), Texture unit 1 = (depth), Texture unit 2 = (linearized depth)
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, s_GBufferFBO.s_hBackBufferTex);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_DepthLinearFBO.s_hBackBufferTex);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Disable texture unit 2 (linearized depth)
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Disable texture unit 1 (depth)
	glActiveTexture(GL_TEXTURE1);
	GL_DisableMultitexture();

	//Disable texture unit 0 (GBuffer texture array)
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D_ARRAY);
	glEnable(GL_TEXTURE_2D);
	*currenttexture = -1;

	GL_UseProgram(0);

	GL_EndFullScreenQuad();
}