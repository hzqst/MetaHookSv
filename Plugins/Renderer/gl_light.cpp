#include "gl_local.h"
#include "pm_defs.h"
#include <sstream>

cvar_t * r_deferred_lighting = nullptr;

MapConVar* r_deferred_lightmap_pow = nullptr;
MapConVar* r_deferred_lightmap_scale = nullptr;

MapConVar* r_flashlight_enable = nullptr;
MapConVar *r_flashlight_ambient = nullptr;
MapConVar *r_flashlight_diffuse = nullptr;
MapConVar *r_flashlight_specular = nullptr;
MapConVar *r_flashlight_specularpow = nullptr;
MapConVar *r_flashlight_attachment = nullptr;
MapConVar *r_flashlight_distance = nullptr;
MapConVar* r_flashlight_min_distance = nullptr;
MapConVar * r_flashlight_cone_degree = nullptr;

MapConVar *r_dynlight_ambient = nullptr;
MapConVar *r_dynlight_diffuse = nullptr;
MapConVar *r_dynlight_specular = nullptr;
MapConVar *r_dynlight_specularpow = nullptr;

cvar_t *r_ssr = nullptr;
MapConVar *r_ssr_ray_step = nullptr;
MapConVar *r_ssr_iter_count = nullptr;
MapConVar *r_ssr_distance_bias = nullptr;
MapConVar *r_ssr_exponential_step= nullptr;
MapConVar *r_ssr_adaptive_step = nullptr;
MapConVar *r_ssr_binary_search = nullptr;
MapConVar *r_ssr_fade = nullptr;

bool r_draw_gbuffer = false;

int gbuffer_mask = -1;
GLuint gbuffer_attachments[GBUFFER_INDEX_MAX] = {0};
int gbuffer_attachment_count = 0;

GLuint r_sphere_vbo = 0;
GLuint r_sphere_ebo = 0;
GLuint r_sphere_vao = 0;
GLuint r_cone_vbo = 0;
GLuint r_cone_vao = 0;

GLuint r_flashlight_cone_texture = 0;
std::string r_flashlight_cone_texture_name;

/*
	Purpose: Map dynamic lights from cl_dlights to array of CDynamicLight
*/
std::vector<std::shared_ptr<CDynamicLight>> g_EngineDynamicLights;

/*
	Purpose: Stores "light_dynamic" from "[mapname]_entity.txt"
*/
std::vector<std::shared_ptr<CDynamicLight>> g_BSPDynamicLights;

/*
	Purpose: visible dynamic lights current frame
*/
std::vector<CVisibleDynamicLightEntry> g_VisibleDynamicLights;

std::unordered_map<program_state_t, dfinal_program_t> g_DFinalProgramTable;

std::unordered_map<program_state_t, dlight_program_t> g_DLightProgramTable;

void R_UseDFinalProgram(program_state_t state, dfinal_program_t *progOutput)
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

		if (state & DFINAL_LINEAR_FOG_SHIFT_ENABLED)
			defs << "#define LINEAR_FOG_SHIFT_ENABLED\n";

		auto def = defs.str();

		prog.program = GL_CompileShaderFile("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\dfinal_shader.frag.glsl", def.c_str(), def.c_str());
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
		{
			glUniform1f(prog.u_ssrRayStep, r_ssr_ray_step->GetValue());
		}

		if (prog.u_ssrIterCount != -1)
		{
			glUniform1i(prog.u_ssrIterCount, r_ssr_iter_count->GetValue());
		}

		if (prog.u_ssrDistanceBias != -1)
		{
			glUniform1f(prog.u_ssrDistanceBias, r_ssr_distance_bias->GetValue());
		}

		if (prog.u_ssrFade != -1)
		{
			glUniform2f(prog.u_ssrFade, r_ssr_fade->GetValues()[0], r_ssr_fade->GetValues()[1]);
		}

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_Error("R_UseDFinalProgram: Failed to load program!");
	}
}

const program_state_mapping_t s_DFinalProgramStateName[] = {
{ DFINAL_LINEAR_FOG_ENABLED				,"DFINAL_LINEAR_FOG_ENABLED"			},
{ DFINAL_EXP_FOG_ENABLED				,"DFINAL_EXP_FOG_ENABLED"				},
{ DFINAL_EXP2_FOG_ENABLED				,"DFINAL_EXP2_FOG_ENABLED"				},
{ DFINAL_SKY_FOG_ENABLED				,"DFINAL_SKY_FOG_ENABLED"				},
{ DFINAL_SSR_ENABLED					,"DFINAL_SSR_ENABLED"					},
{ DFINAL_SSR_ADAPTIVE_STEP_ENABLED		,"DFINAL_SSR_ADAPTIVE_STEP_ENABLED"		},
{ DFINAL_SSR_EXPONENTIAL_STEP_ENABLED	,"DFINAL_SSR_EXPONENTIAL_STEP_ENABLED"	},
{ DFINAL_SSR_BINARY_SEARCH_ENABLED		,"DFINAL_SSR_BINARY_SEARCH_ENABLED"		},
{ DFINAL_LINEAR_FOG_SHIFT_ENABLED		,"DFINAL_LINEAR_FOG_SHIFT_ENABLED"		},
};

void R_SaveDFinalProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto &p : g_DFinalProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/dfinal_cache.txt", states, s_DFinalProgramStateName, _ARRAYSIZE(s_DFinalProgramStateName));
}

void R_LoadDFinalProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/dfinal_cache.txt", s_DFinalProgramStateName, _ARRAYSIZE(s_DFinalProgramStateName), [](program_state_t state) {

		R_UseDFinalProgram(state, NULL);

	});
}

void R_UseDLightProgram(program_state_t state, dlight_program_t *progOutput)
{
	dlight_program_t prog = { 0 };

	auto itor = g_DLightProgramTable.find(state);
	if (itor == g_DLightProgramTable.end())
	{
		std::stringstream defs;

		// #define DLIGHT_SPOT_ENABLED								0x1ull
		// #define DLIGHT_POINT_ENABLED							0x2ull
		// #define DLIGHT_VOLUME_ENABLED							0x4ull
		// #define DLIGHT_CONE_TEXTURE_ENABLED						0x8ull
		// #define DLIGHT_DIRECTIONAL_ENABLED						0x10ull
		// #define DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED			0x20ull
		// #define DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED			0x40ull
		// #define DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED	0x80ull
		// #define DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED	0x100ull
		// #define DLIGHT_CSM_SHADOW_TEXTURE_ENABLED				0x200ull
		// #define DLIGHT_PCF_ENABLED								0x400ull

		if (state & DLIGHT_SPOT_ENABLED)
			defs << "#define SPOT_ENABLED\n";

		if (state & DLIGHT_POINT_ENABLED)
			defs << "#define POINT_ENABLED\n";

		if (state & DLIGHT_VOLUME_ENABLED)
			defs << "#define VOLUME_ENABLED\n";

		if (state & DLIGHT_CONE_TEXTURE_ENABLED)
			defs << "#define CONE_TEXTURE_ENABLED\n";

		if (state & DLIGHT_DIRECTIONAL_ENABLED)
			defs << "#define DIRECTIONAL_ENABLED\n";

		if (state & DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED)
			defs << "#define STATIC_SHADOW_TEXTURE_ENABLED\n";

		if (state & DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED)
			defs << "#define DYNAMIC_SHADOW_TEXTURE_ENABLED\n";

		if (state & DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
			defs << "#define STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED\n";

		if (state & DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
			defs << "#define DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED\n";

		if (state & DLIGHT_CSM_SHADOW_TEXTURE_ENABLED)
			defs << "#define CSM_SHADOW_TEXTURE_ENABLED\n";

		if (state & DLIGHT_PCF_ENABLED)
			defs << "#define PCF_ENABLED\n";

		auto def = defs.str();

		prog.program = GL_CompileShaderFile("renderer\\shader\\dlight_shader.vert.glsl", "renderer\\shader\\dlight_shader.frag.glsl", def.c_str(), def.c_str());
		if (prog.program)
		{
			// int program;
			// int u_lightdir;
			// int u_lightright;
			// int u_lightup;
			// int u_lightpos;
			// int u_lightcolor;
			// int u_lightcone;
			// int u_lightradius;
			// int u_lightambient;
			// int u_lightdiffuse;
			// int u_lightspecular;
			// int u_lightspecularpow;
			// int u_lightSize;
			// int u_modelmatrix;
			// int u_staticShadowTexel;
			// int u_staticShadowMatrix;
			// int u_dynamicShadowTexel;
			// int u_dynamicShadowMatrix;
			// int u_staticCubemapShadowTexel;
			// int u_dynamicCubemapShadowTexel;
			// int u_csmMatrices;
			// int u_csmDistances;
			// int u_csmTexel;

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
			SHADER_UNIFORM(prog, u_lightSize, "u_lightSize");
			SHADER_UNIFORM(prog, u_modelmatrix, "u_modelmatrix");
			SHADER_UNIFORM(prog, u_staticShadowTexel, "u_staticShadowTexel");
			SHADER_UNIFORM(prog, u_staticShadowMatrix, "u_staticShadowMatrix");
			SHADER_UNIFORM(prog, u_dynamicShadowTexel, "u_dynamicShadowTexel");
			SHADER_UNIFORM(prog, u_dynamicShadowMatrix, "u_dynamicShadowMatrix");
			SHADER_UNIFORM(prog, u_staticCubemapShadowTexel, "u_staticCubemapShadowTexel");
			SHADER_UNIFORM(prog, u_dynamicCubemapShadowTexel, "u_dynamicCubemapShadowTexel");
			SHADER_UNIFORM(prog, u_csmMatrices, "u_csmMatrices");
			SHADER_UNIFORM(prog, u_csmDistances, "u_csmDistances");
			SHADER_UNIFORM(prog, u_csmTexel, "u_csmTexel");
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
		Sys_Error("R_UseDLightProgram: Failed to load program!");
	}
}

// #define DLIGHT_SPOT_ENABLED								0x1ull
// #define DLIGHT_POINT_ENABLED								0x2ull
// #define DLIGHT_VOLUME_ENABLED							0x4ull
// #define DLIGHT_CONE_TEXTURE_ENABLED						0x8ull
// #define DLIGHT_DIRECTIONAL_ENABLED						0x10ull
// #define DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED				0x20ull
// #define DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED			0x40ull
// #define DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED		0x80ull
// #define DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED	0x100ull
// #define DLIGHT_CSM_SHADOW_TEXTURE_ENABLED				0x200ull
// #define DLIGHT_PCF_ENABLED								0x400ull
// #define DLIGHT_PCSS_ENABLED								0x800ull

const program_state_mapping_t s_DLightProgramStateName[] = {
{ DLIGHT_SPOT_ENABLED								,"DLIGHT_SPOT_ENABLED"	 },
{ DLIGHT_POINT_ENABLED								,"DLIGHT_POINT_ENABLED"	 },
{ DLIGHT_VOLUME_ENABLED								,"DLIGHT_VOLUME_ENABLED" },
{ DLIGHT_CONE_TEXTURE_ENABLED						,"DLIGHT_CONE_TEXTURE_ENABLED" },
{ DLIGHT_DIRECTIONAL_ENABLED						,"DLIGHT_DIRECTIONAL_ENABLED" },
{ DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED				,"DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED" },
{ DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED				,"DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED" },
{ DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED		,"DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED" },
{ DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED		,"DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED" },
{ DLIGHT_CSM_SHADOW_TEXTURE_ENABLED					,"DLIGHT_CSM_SHADOW_TEXTURE_ENABLED" },
{ DLIGHT_PCF_ENABLED								,"DLIGHT_PCF_ENABLED" },
};

void R_SaveDLightProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto &p : g_DLightProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/dlight_cache.txt", states, s_DLightProgramStateName, _ARRAYSIZE(s_DLightProgramStateName));
}

void R_LoadDLightProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/dlight_cache.txt", s_DLightProgramStateName, _ARRAYSIZE(s_DLightProgramStateName), [](program_state_t state) {

		R_UseDLightProgram(state, NULL);

	});
}

void R_ShutdownLight(void)
{
	g_BSPDynamicLights.clear();
	g_EngineDynamicLights.clear();
	g_VisibleDynamicLights.clear();

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

GLuint R_GetEmptyVAO()
{
	return r_empty_vao;
}

void R_InitLight(void)
{
	r_empty_vao = GL_GenVAO();

	r_deferred_lighting = gEngfuncs.pfnRegisterVariable("r_deferred_lighting", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_deferred_lightmap_pow = R_RegisterMapCvar("r_deferred_lightmap_pow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_deferred_lightmap_scale = R_RegisterMapCvar("r_deferred_lightmap_scale", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	/*
		Affects legacy dynamic lights
	*/
	r_dynlight_ambient = R_RegisterMapCvar("r_dynlight_ambient", "0.2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_diffuse = R_RegisterMapCvar("r_dynlight_diffuse", "0.4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_specular = R_RegisterMapCvar("r_dynlight_specular", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_specularpow = R_RegisterMapCvar("r_dynlight_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	/*
		Affects flashlight
	*/
	r_flashlight_enable = R_RegisterMapCvar("r_flashlight_enable", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_ambient = R_RegisterMapCvar("r_flashlight_ambient", "0.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_diffuse = R_RegisterMapCvar("r_flashlight_diffuse", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_specular = R_RegisterMapCvar("r_flashlight_specular", "2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_specularpow = R_RegisterMapCvar("r_flashlight_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_attachment = R_RegisterMapCvar("r_flashlight_attachment", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_distance = R_RegisterMapCvar("r_flashlight_distance", "2000", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_min_distance = R_RegisterMapCvar("r_flashlight_min_distance", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_cone_degree = R_RegisterMapCvar("r_flashlight_cone_degree", "50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

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
	GL_UploadDataToVBOStaticDraw(r_sphere_vbo, sphereVertices.size() * sizeof(float), sphereVertices.data());

	r_sphere_ebo = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(r_sphere_ebo, sphereIndices.size() * sizeof(int), sphereIndices.data());

	r_sphere_vao = GL_GenVAO();

	GL_BindStatesForVAO(r_sphere_vao, r_sphere_vbo, r_sphere_ebo,
	[]() {
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	});

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
	GL_UploadDataToVBOStaticDraw(r_cone_vbo, coneVertices.size() * sizeof(float), coneVertices.data());

	r_cone_vao = GL_GenVAO();

	GL_BindStatesForVAO(r_cone_vao, r_cone_vbo, 0,
	[]() {
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	});

	r_draw_gbuffer = false;
}

void R_LoadLightResources()
{
	if (!r_flashlight_cone_texture_name.empty())
	{
		gl_loadtexture_result_t loadResult;

		if (R_LoadTextureFromFile(r_flashlight_cone_texture_name.c_str(), r_flashlight_cone_texture_name.c_str(), GLT_WORLD, true, &loadResult))
		{
			r_flashlight_cone_texture = loadResult.gltexturenum;
		}
		else
		{
			gEngfuncs.Con_Printf("R_NewMapLight: Failed to load %s.\n", r_flashlight_cone_texture_name.c_str());
		}
	}
	else
	{
		r_flashlight_cone_texture = 0;
	}
}

cl_entity_t *R_GetDLightBindingEntity(dlight_t* dl)
{
	if (dl->key >= 1 && dl->key <= gEngfuncs.GetMaxClients())
	{
		auto ent = gEngfuncs.GetEntityByIndex(dl->key);

		if (ent && ent->player)
			return ent;
	}

	return nullptr;
}

/*
	Purpose: determine if dl is an engine flashlight.
*/
bool R_IsDLightFlashlight(dlight_t *dl)
{
	if (!r_flashlight_enable || r_flashlight_enable->GetValue() < 1)
		return false;

	if (!R_IsDeferredRenderingEnabled())
		return false;

	if (dl->key >= 1 && dl->key <= gEngfuncs.GetMaxClients())
	{
		auto ent = gEngfuncs.GetEntityByIndex(dl->key);

		if (ent && ent->player)
			return true;
	}

	if (g_bIsAoMDC)
	{
		if (dl->key == 0 && dl->decay == 100 && dl->radius == 110 && dl->minlight == 0 && dl->color.r == 48 && dl->color.g == 48 && dl->color.b == 48)
		{
			dl->key = gEngfuncs.GetLocalPlayer()->index;
		}
	}

	return false;
}

void R_SetGBufferBlend(int blendsrc, int blenddst)
{
	if (!R_IsRenderingGBuffer())
		return;

	for (int i = 0; i < gbuffer_attachment_count; ++i)
	{
		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_DIFFUSE)
			glBlendFunci(i, blendsrc, blenddst);

		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_LIGHTMAP)
			glBlendFunci(i, blendsrc, blenddst);

		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_WORLDNORM)
			glBlendFunci(i, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (gbuffer_attachments[i] == GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_SPECULAR)
			glBlendFunci(i, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void R_SetGBufferMask(int mask)
{
	if (!R_IsRenderingGBuffer())
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
	if ((int)r_deferred_lighting->value < 1)
		return false;

	if ((int)r_gamma_blend->value >= 1)
		return false;

	if ((*r_refdef.onlyClientDraws))
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	return true;
}

/*

	Purpose : Switch to s_GBufferFBO
*/

bool R_CanRenderGBuffer(void)
{
	if (!R_IsDeferredRenderingEnabled())
		return false;

	if (R_IsRenderingWaterView())
		return false;

	if (R_IsRenderingShadowView())
		return false;

	if (R_IsRenderingPortal())
		return false;

	return true;
}

void R_BeginRenderGBuffer(void)
{
	if (!R_CanRenderGBuffer())
		return;

	GL_BeginDebugGroup("R_BeginRenderGBuffer");

	r_draw_gbuffer = true;
	gbuffer_mask = -1;

	GL_BindFrameBuffer(&s_GBufferFBO);

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	R_SetGBufferBlend(GL_ONE, GL_ZERO);

	vec4_t vecClearColor = { 0, 0, 0, 0 };

	GL_ClearColorDepthStencil(vecClearColor, 1, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

	GL_EndDebugGroup();
}

bool Util_IsOriginInCone(const float *origin, const float *cone_origin, const float *cone_vforward, float cone_cosine, float cone_distance)
{
	float dir[3];
	VectorSubtract(origin, cone_origin, dir);

	float dist = VectorLength(dir);

	if (dist > cone_distance)
		return false;

	VectorNormalize(dir);

	auto dot = DotProduct(cone_vforward, dir);

	return dot > cone_cosine;
}

void R_AddVisibleDynamicLight(const std::shared_ptr<CDynamicLight>& dynamicLight)
{
	if (dynamicLight->type == DynamicLightType_Point)
	{
		float radius = dynamicLight->size;

		if (VectorDistance((*r_refdef.vieworg), dynamicLight->origin) > radius + 32)
		{
			//outside sphere ? do AABB check
			vec3_t mins, maxs;
			for (int j = 0; j < 3; j++)
			{
				mins[j] = dynamicLight->origin[j] - radius;
				maxs[j] = dynamicLight->origin[j] + radius;
			}

			if (R_CullBox(mins, maxs))
				return;

			g_VisibleDynamicLights.emplace_back(dynamicLight, true);
		}
		else
		{
			g_VisibleDynamicLights.emplace_back(dynamicLight, false);
		}
	}
	else if (dynamicLight->type == DynamicLightType_Spot)
	{
		float maxRadius = max(dynamicLight->size, dynamicLight->distance);

		//outside sphere ? do AABB check
		vec3_t mins, maxs;
		for (int j = 0; j < 3; j++)
		{
			mins[j] = dynamicLight->origin[j] - maxRadius;
			maxs[j] = dynamicLight->origin[j] + maxRadius;
		}

		if (R_CullBox(mins, maxs))
			return;

		vec3_t dlight_vforward;
		vec3_t dlight_vright;
		vec3_t dlight_vup;
		AngleVectors(dynamicLight->angles, dlight_vforward, dlight_vright, dlight_vup);

		vec3_t adjustedOrigin;
		VectorMA(dynamicLight->origin, -24.0f, dlight_vforward, adjustedOrigin);
		float adjustedDistance = dynamicLight->distance + 64.0f;

		float coneAngle = dynamicLight->coneAngle;
		float coneCosAngle = cosf(coneAngle);

		if (!Util_IsOriginInCone((*r_refdef.vieworg), adjustedOrigin, dlight_vforward, coneCosAngle, adjustedDistance))
		{
			g_VisibleDynamicLights.emplace_back(dynamicLight, true);
		}
		else
		{
			g_VisibleDynamicLights.emplace_back(dynamicLight, false);
		}
	}
	else if (dynamicLight->type == DynamicLightType_Directional)
	{
		//No vis check, always visible
		g_VisibleDynamicLights.emplace_back(dynamicLight, false);
	}
}

/*
	Purpose: iterate through all visible dynamic lights
*/
void R_IterateVisibleDynamicLights(
	fnPointLightCallback pointlightCallback, 
	fnSpotLightCallback spotlightCallback,
	fnDirectionalLightCallback directionalLightCallback,
	void *context)
{
	for (size_t i = 0; i < g_VisibleDynamicLights.size(); i++)
	{
		const auto &entry = g_VisibleDynamicLights[i];

		const auto& dynamicLight = entry.m_pDynamicLight;

		if (dynamicLight->type == DynamicLightType_Point)
		{
			PointLightCallbackArgs args{};

			args.radius = dynamicLight->size;
			VectorCopy(dynamicLight->origin, args.origin);

			VectorCopy(dynamicLight->color, args.color);
			args.ambient = dynamicLight->ambient;
			args.diffuse = dynamicLight->diffuse;
			args.specular = dynamicLight->specular;
			args.specularpow = dynamicLight->specularpow;

			if (dynamicLight->shadow > 0) {
				args.ppDynamicShadowTexture = &dynamicLight->pDynamicShadowTexture;
				args.ppStaticShadowTexture = &dynamicLight->pStaticShadowTexture;
				args.staticShadowSize = dynamicLight->static_shadow_size;
				args.dynamicShadowSize = dynamicLight->dynamic_shadow_size;
			}

			args.bVolume = entry.m_bVolume;

			pointlightCallback(&args, context);
		}
		else if (dynamicLight->type == DynamicLightType_Spot)
		{
			SpotLightCallbackArgs args{};

			args.radius = dynamicLight->size;
			args.distance = dynamicLight->distance;
			args.coneAngle = dynamicLight->coneAngle;
			VectorCopy(dynamicLight->origin, args.origin);
			VectorCopy(dynamicLight->angles, args.angles);

			VectorCopy(dynamicLight->color, args.color);
			args.ambient = dynamicLight->ambient;
			args.diffuse = dynamicLight->diffuse;
			args.specular = dynamicLight->specular;
			args.specularpow = dynamicLight->specularpow;

			if (dynamicLight->shadow > 0) {
				args.ppDynamicShadowTexture = &dynamicLight->pDynamicShadowTexture;
				args.ppStaticShadowTexture = &dynamicLight->pStaticShadowTexture;
				args.staticShadowSize = dynamicLight->static_shadow_size;
				args.dynamicShadowSize = dynamicLight->dynamic_shadow_size;
			}

			args.bVolume = entry.m_bVolume;
			args.bHideEntitySource = entry.m_bHideEntitySource;
			args.pHideEntity = entry.m_pHideEntity;

			spotlightCallback(&args, context);
		}
		else if (dynamicLight->type == DynamicLightType_Directional)
		{
			DirectionalLightCallbackArgs args{};
			VectorCopy(dynamicLight->origin, args.origin);
			VectorCopy(dynamicLight->angles, args.angles);
			args.size = dynamicLight->size;

			VectorCopy(dynamicLight->color, args.color);
			args.ambient = dynamicLight->ambient;
			args.diffuse = dynamicLight->diffuse;
			args.specular = dynamicLight->specular;
			args.specularpow = dynamicLight->specularpow;

			if (dynamicLight->shadow > 0) {
				args.ppStaticShadowTexture = &dynamicLight->pStaticShadowTexture;
				args.ppDynamicShadowTexture = &dynamicLight->pDynamicShadowTexture;
				args.dynamicShadowSize = dynamicLight->dynamic_shadow_size;
				args.staticShadowSize = dynamicLight->static_shadow_size;
				args.csmLambda = dynamicLight->csm_lambda;
				args.csmMargin = dynamicLight->csm_margin;
			}

			args.bVolume = entry.m_bVolume;

			directionalLightCallback(&args, context);
		}
	}
}

void R_LightShadingPass(void)
{
	GL_BeginDebugGroup("R_LightShadingPass");

	//Write to GBuffer->lightmap only whatsoever
	GL_BindFrameBuffer(&s_GBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_LIGHTMAP);

	//Disable depth write and re-enable later after light pass.
	glDepthMask(GL_FALSE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	//Texture unit 0 = GBuffer diffuse
	GL_BindTextureUnit(DSHADE_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex);

	//Texture unit 1 = GBuffer lightmap
	GL_BindTextureUnit(DSHADE_BIND_LIGHTMAP_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex2);

	//Texture unit 2 = GBuffer worldnorm
	GL_BindTextureUnit(DSHADE_BIND_WORLDNORM_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex3);

	//Texture unit 3 = GBuffer specular
	GL_BindTextureUnit(DSHADE_BIND_SPECULAR_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex4);

	//Texture unit 4 = Depth texture
	GL_BindTextureUnit(DSHADE_BIND_DEPTH_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

	//Texture unit 5 = Stencil texture
	if (s_GBufferFBO.s_hBackBufferStencilView)
	{
		GL_BindTextureUnit(DSHADE_BIND_STENCIL_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferStencilView);
	}

	//Texture unit 6 = Flashlight cone texture
	if (r_flashlight_cone_texture)
	{
		GL_BindTextureUnit(DSHADE_BIND_CONE_TEXTURE, GL_TEXTURE_2D, r_flashlight_cone_texture);
	}

	const auto PointLightCallback = [](PointLightCallbackArgs *args, void *context)
	{
		if (args->bVolume)
		{
			GL_BeginDebugGroup("R_LightShadingPass - DrawVolumePointLight");

			GL_BindVAO(r_sphere_vao);

			vec3_t angles = { 0, 0, 0 };
			vec3_t origin = { args->origin[0], args->origin[1], args->origin[2] };
			vec3_t scales = { args->radius, args->radius, args->radius };

			float modelmatrix[4][4];
			Matrix4x4_CreateFromEntityEx(modelmatrix, angles, origin, scales);

			float modelmatrix_T[4][4];
			Matrix4x4_Transpose(modelmatrix_T, modelmatrix);

			program_state_t DLightProgramState = DLIGHT_POINT_ENABLED | DLIGHT_VOLUME_ENABLED;

			std::shared_ptr<IShadowTexture> pStaticShadowTexture;
			std::shared_ptr<IShadowTexture> pDynamicShadowTexture;

			if (args->ppStaticShadowTexture)
				pStaticShadowTexture = (*args->ppStaticShadowTexture);

			if (pStaticShadowTexture && pStaticShadowTexture->IsReady() && pStaticShadowTexture->IsCubemap())
			{
				DLightProgramState |= DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED;
				DLightProgramState |= DLIGHT_PCF_ENABLED;
			}

			if (args->ppDynamicShadowTexture)
				pDynamicShadowTexture = (*args->ppDynamicShadowTexture);

			if (pDynamicShadowTexture && pDynamicShadowTexture->IsReady() && pDynamicShadowTexture->IsCubemap())
			{
				DLightProgramState |= DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED;
				DLightProgramState |= DLIGHT_PCF_ENABLED;
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			if (prog.u_modelmatrix != -1)
			{
				glUniformMatrix4fv(prog.u_modelmatrix, 1, false, (float *)modelmatrix_T);
			}
			if (prog.u_lightpos != -1)
			{
				glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			}
			if (prog.u_lightcolor != -1)
			{
				glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			}
			if (prog.u_lightradius != -1)
			{
				glUniform1f(prog.u_lightradius, args->radius);
			}
			if (prog.u_lightambient != -1)
			{
				glUniform1f(prog.u_lightambient, args->ambient);
			}
			if (prog.u_lightdiffuse != -1)
			{
				glUniform1f(prog.u_lightdiffuse, args->diffuse);
			}
			if (prog.u_lightspecular != -1)
			{
				glUniform1f(prog.u_lightspecular, args->specular);
			}
			if (prog.u_lightspecularpow != -1)
			{
				glUniform1f(prog.u_lightspecularpow, args->specularpow);
			}

			if ((DLightProgramState & DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pStaticShadowTexture)
			{
				if (prog.u_staticCubemapShadowTexel != -1)
				{
					glUniform2f(prog.u_staticCubemapShadowTexel, (float)pStaticShadowTexture->GetTextureSize(), 1.0f / (float)pStaticShadowTexture->GetTextureSize());
				}

				GL_BindTextureUnit(DSHADE_BIND_STATIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, pStaticShadowTexture->GetDepthTexture());
			}

			if ((DLightProgramState & DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				if (prog.u_dynamicCubemapShadowTexel != -1)
				{
					glUniform2f(prog.u_dynamicCubemapShadowTexel, (float)pDynamicShadowTexture->GetTextureSize(), 1.0f / (float)pDynamicShadowTexture->GetTextureSize());
				}

				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, pDynamicShadowTexture->GetDepthTexture());
			}

			glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

			if ((DLightProgramState & DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, 0);
			}

			if ((DLightProgramState & DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pStaticShadowTexture)
			{
				GL_BindTextureUnit(DSHADE_BIND_STATIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, 0);
			}

			GL_UseProgram(0);

			GL_BindVAO(0);

			GL_EndDebugGroup();
		}
		else
		{
			GL_BeginDebugGroup("R_LightShadingPass - DrawFullscreenPointLight");

			GL_Set2D();

			GL_BindVAO(r_empty_vao);

			program_state_t DLightProgramState = DLIGHT_POINT_ENABLED;

			std::shared_ptr<IShadowTexture> pStaticShadowTexture;
			std::shared_ptr<IShadowTexture> pDynamicShadowTexture;
			
			if (args->ppStaticShadowTexture)
				pStaticShadowTexture = (*args->ppStaticShadowTexture);

			if (pStaticShadowTexture && pStaticShadowTexture->IsReady() && pStaticShadowTexture->IsCubemap())
			{
				DLightProgramState |= DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED;
				DLightProgramState |= DLIGHT_PCF_ENABLED;
			}

			if (args->ppDynamicShadowTexture)
				pDynamicShadowTexture = (*args->ppDynamicShadowTexture);

			if (pDynamicShadowTexture && pDynamicShadowTexture->IsReady() && pDynamicShadowTexture->IsCubemap())
			{
				DLightProgramState |= DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED;
				DLightProgramState |= DLIGHT_PCF_ENABLED;
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);
			if (prog.u_lightpos != -1)
			{
				glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			}
			if (prog.u_lightcolor != -1)
			{
				glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			}
			if (prog.u_lightradius != -1)
			{
				glUniform1f(prog.u_lightradius, args->radius);
			}
			if (prog.u_lightambient != -1)
			{
				glUniform1f(prog.u_lightambient, args->ambient);
			}
			if (prog.u_lightdiffuse != -1)
			{
				glUniform1f(prog.u_lightdiffuse, args->diffuse);
			}
			if (prog.u_lightspecular != -1)
			{
				glUniform1f(prog.u_lightspecular, args->specular);
			}
			if (prog.u_lightspecularpow != -1)
			{
				glUniform1f(prog.u_lightspecularpow, args->specularpow);
			}

			if ((DLightProgramState & DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pStaticShadowTexture)
			{
				if (prog.u_staticCubemapShadowTexel != -1)
				{
					glUniform2f(prog.u_staticCubemapShadowTexel, (float)pStaticShadowTexture->GetTextureSize(), 1.0f / (float)pStaticShadowTexture->GetTextureSize());
				}

				GL_BindTextureUnit(DSHADE_BIND_STATIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, pStaticShadowTexture->GetDepthTexture());
			}

			if ((DLightProgramState & DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				if (prog.u_dynamicCubemapShadowTexel != -1)
				{
					glUniform2f(prog.u_dynamicCubemapShadowTexel, (float)pDynamicShadowTexture->GetTextureSize(), 1.0f / (float)pDynamicShadowTexture->GetTextureSize());
				}

				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, pDynamicShadowTexture->GetDepthTexture());
			}

			const uint32_t indices[] = { 0,1,2,2,3,0 };
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

			if ((DLightProgramState & DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, 0);
			}

			if ((DLightProgramState & DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) && pStaticShadowTexture)
			{
				GL_BindTextureUnit(DSHADE_BIND_STATIC_CUBEMAP_SHADOW_TEXTURE, GL_TEXTURE_CUBE_MAP, 0);
			}

			GL_UseProgram(0);

			GL_BindVAO(0);

			GL_Finish2D();

			GL_EndDebugGroup();
		}
	};

	const auto SpotLightCallback = [](SpotLightCallbackArgs *args, void *context)
	{
		vec3_t angles = { args->angles[0], args->angles[1], args->angles[2] };
		vec3_t origin = { args->origin[0], args->origin[1], args->origin[2] };
		vec3_t scales = { args->distance, args->radius, args->radius };

		vec3_t v_forward{}, v_right{}, v_up{};
		AngleVectors(angles, v_forward, v_right, v_up);

		if (args->bVolume)
		{
			GL_BeginDebugGroup("R_LightShadingPass - DrawConeSpotLight");

			GL_BindVAO(r_cone_vao);

			float modelmatrix[4][4];
			Matrix4x4_CreateFromEntityEx(modelmatrix, angles, origin, scales);

			float modelmatrix_T[4][4];
			Matrix4x4_Transpose(modelmatrix_T, modelmatrix);

			program_state_t DLightProgramState = DLIGHT_SPOT_ENABLED | DLIGHT_VOLUME_ENABLED;

			if (r_flashlight_cone_texture)
			{
				DLightProgramState |= DLIGHT_CONE_TEXTURE_ENABLED;
			}

			std::shared_ptr<IShadowTexture> pStaticShadowTexture;
			std::shared_ptr<IShadowTexture> pDynamicShadowTexture;

			if(args->ppDynamicShadowTexture)
				pDynamicShadowTexture = (*args->ppDynamicShadowTexture);

			if (pDynamicShadowTexture && pDynamicShadowTexture->IsReady())
			{
				DLightProgramState |= DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED;
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			if (prog.u_modelmatrix != -1)
			{
				glUniformMatrix4fv(prog.u_modelmatrix, 1, false, (float *)modelmatrix_T);
			}
			if (prog.u_lightdir != -1)
			{
				glUniform3f(prog.u_lightdir, v_forward[0], v_forward[1], v_forward[2]);
			}
			if (prog.u_lightright != -1)
			{
				glUniform3f(prog.u_lightright, v_right[0], v_right[1], v_right[2]);
			}
			if (prog.u_lightup != -1)
			{
				glUniform3f(prog.u_lightup, v_up[0], v_up[1], v_up[2]);
			}
			if (prog.u_lightpos != -1)
			{
				glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			}
			if (prog.u_lightcolor != -1)
			{
				glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			}
			if (prog.u_lightcone != -1)
			{
				float coneCosAngle = cosf(args->coneAngle);
				float coneSinAngle = sinf(args->coneAngle);
				glUniform2f(prog.u_lightcone, coneCosAngle, coneSinAngle);
			}
			if (prog.u_lightradius != -1)
			{
				glUniform1f(prog.u_lightradius, args->distance);
			}
			if (prog.u_lightambient != -1)
			{
				glUniform1f(prog.u_lightambient, args->ambient);
			}
			if (prog.u_lightdiffuse != -1)
			{
				glUniform1f(prog.u_lightdiffuse, args->diffuse);
			}
			if (prog.u_lightspecular != -1)
			{
				glUniform1f(prog.u_lightspecular, args->specular);
			}
			if (prog.u_lightspecularpow != -1)
			{
				glUniform1f(prog.u_lightspecularpow, args->specularpow);
			}

			if ((DLightProgramState & DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				if (prog.u_dynamicShadowTexel != -1)
				{
					glUniform2f(prog.u_dynamicShadowTexel, pDynamicShadowTexture->GetTextureSize(), 1.0f / (float)pDynamicShadowTexture->GetTextureSize());
				}

				if (prog.u_dynamicShadowMatrix != -1)
				{
					glUniformMatrix4fv(prog.u_dynamicShadowMatrix, 1, false, (float*)pDynamicShadowTexture->GetShadowMatrix(0));
				}

				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_SHADOW_TEXTURE, GL_TEXTURE_2D, pDynamicShadowTexture->GetDepthTexture());
			}

			glDrawArrays(GL_TRIANGLES, 0, X_SEGMENTS * 6);

			if ((DLightProgramState & DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_SHADOW_TEXTURE, GL_TEXTURE_2D, 0);
			}

			GL_UseProgram(0);

			GL_BindVAO(0);

			GL_EndDebugGroup();
		}
		else
		{
			GL_BeginDebugGroup("R_LightShadingPass - DrawFullscreenSpotLight");

			GL_Set2D();

			GL_BindVAO(r_empty_vao);

			program_state_t DLightProgramState = DLIGHT_SPOT_ENABLED;

			if (r_flashlight_cone_texture)
			{
				DLightProgramState |= DLIGHT_CONE_TEXTURE_ENABLED;
			}

			std::shared_ptr<IShadowTexture> pStaticShadowTexture;
			std::shared_ptr<IShadowTexture> pDynamicShadowTexture;

			if (args->ppDynamicShadowTexture)
				pDynamicShadowTexture = (*args->ppDynamicShadowTexture);

			if (pDynamicShadowTexture && pDynamicShadowTexture->IsReady())
			{
				DLightProgramState |= DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED;
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			if (prog.u_lightdir != -1)
			{
				glUniform3f(prog.u_lightdir, v_forward[0], v_forward[1], v_forward[2]);
			}
			if (prog.u_lightright != -1)
			{
				glUniform3f(prog.u_lightright, v_right[0], v_right[1], v_right[2]);
			}
			if (prog.u_lightup != -1)
			{
				glUniform3f(prog.u_lightup, v_up[0], v_up[1], v_up[2]);
			}
			if (prog.u_lightpos != -1)
			{
				glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			}
			if (prog.u_lightcolor != -1)
			{
				glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			}
			if (prog.u_lightcone != -1)
			{
				float coneCosAngle = cosf(args->coneAngle);
				float coneSinAngle = sinf(args->coneAngle);
				glUniform2f(prog.u_lightcone, coneCosAngle, coneSinAngle);
			}
			if (prog.u_lightradius != -1)
			{
				glUniform1f(prog.u_lightradius, args->distance);
			}
			if (prog.u_lightambient != -1)
			{
				glUniform1f(prog.u_lightambient, args->ambient);
			}
			if (prog.u_lightdiffuse != -1)
			{
				glUniform1f(prog.u_lightdiffuse, args->diffuse);
			}
			if (prog.u_lightspecular != -1)
			{
				glUniform1f(prog.u_lightspecular, args->specular);
			}
			if (prog.u_lightspecularpow != -1)
			{
				glUniform1f(prog.u_lightspecularpow, args->specularpow);
			}

			if ((DLightProgramState & DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				if (prog.u_dynamicShadowTexel != -1)
				{
					glUniform2f(prog.u_dynamicShadowTexel, pDynamicShadowTexture->GetTextureSize(), 1.0f / (float)pDynamicShadowTexture->GetTextureSize());
				}

				if (prog.u_dynamicShadowMatrix != -1)
				{
					glUniformMatrix4fv(prog.u_dynamicShadowMatrix, 1, false, (float*)pDynamicShadowTexture->GetShadowMatrix(0));
				}

				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_SHADOW_TEXTURE, GL_TEXTURE_2D, pDynamicShadowTexture->GetDepthTexture());
			}

			const uint32_t indices[] = { 0,1,2,2,3,0 };
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

			if ((DLightProgramState & DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
			{
				GL_BindTextureUnit(DSHADE_BIND_DYNAMIC_SHADOW_TEXTURE, GL_TEXTURE_2D, 0);
			}

			GL_BindVAO(0);

			GL_Finish2D();

			GL_EndDebugGroup();
		}
	};

	const auto DirectionalLightCallback = [](DirectionalLightCallbackArgs* args, void* context)
	{
		GL_BeginDebugGroup("R_LightShadingPass - DrawDirectionalLight");

		GL_Set2D();

		GL_BindVAO(r_empty_vao);

		program_state_t DLightProgramState = DLIGHT_DIRECTIONAL_ENABLED;

		std::shared_ptr<IShadowTexture> pStaticShadowTexture;
		std::shared_ptr<IShadowTexture> pDynamicShadowTexture;

		if (args->ppStaticShadowTexture)
			 pStaticShadowTexture = (*args->ppStaticShadowTexture);

		if (pStaticShadowTexture && pStaticShadowTexture->IsSingleLayer() && pStaticShadowTexture->IsReady())
		{
			DLightProgramState |= DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED;
		}

		if(args->ppDynamicShadowTexture)
			pDynamicShadowTexture = (*args->ppDynamicShadowTexture);

		if (pDynamicShadowTexture && pDynamicShadowTexture->IsCascaded() && pDynamicShadowTexture->IsReady())
		{
			DLightProgramState |= DLIGHT_CSM_SHADOW_TEXTURE_ENABLED;
		}

		vec3_t angles = { args->angles[0], args->angles[1], args->angles[2] };
		vec3_t origin = { args->origin[0], args->origin[1], args->origin[2] };

		vec3_t v_forward{}, v_right{}, v_up{};
		AngleVectors(angles, v_forward, v_right, v_up);

		dlight_program_t prog = { 0 };
		R_UseDLightProgram(DLightProgramState, &prog);

		if (prog.u_lightdir != -1)
		{
			glUniform3f(prog.u_lightdir, v_forward[0], v_forward[1], v_forward[2]);
		}
		if (prog.u_lightright != -1)
		{
			glUniform3f(prog.u_lightright, v_right[0], v_right[1], v_right[2]);
		}
		if (prog.u_lightup != -1)
		{
			glUniform3f(prog.u_lightup, v_up[0], v_up[1], v_up[2]);
		}
		if (prog.u_lightpos != -1)
		{
			glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
		}
		if (prog.u_lightcolor != -1)
		{
			glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
		}
		if (prog.u_lightambient != -1)
		{
			glUniform1f(prog.u_lightambient, args->ambient);
		}
		if (prog.u_lightdiffuse != -1)
		{
			glUniform1f(prog.u_lightdiffuse, args->diffuse);
		}
		if (prog.u_lightspecular != -1)
		{
			glUniform1f(prog.u_lightspecular, args->specular);
		}
		if (prog.u_lightspecularpow != -1)
		{
			glUniform1f(prog.u_lightspecularpow, args->specularpow);
		}

		if (prog.u_lightSize != -1)
		{
			glUniform1f(prog.u_lightSize, args->size);
		}

		if ((DLightProgramState & DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED) && pStaticShadowTexture)
		{
			if (prog.u_staticShadowTexel != -1)
			{
				glUniform2f(prog.u_staticShadowTexel, pStaticShadowTexture->GetTextureSize(), 1.0f / (float)pStaticShadowTexture->GetTextureSize());
			}

			if (prog.u_staticShadowMatrix != -1)
			{
				glUniformMatrix4fv(prog.u_staticShadowMatrix, 1, false, (float*)pStaticShadowTexture->GetShadowMatrix(0));
			}

			GL_BindTextureUnit(DSHADE_BIND_STATIC_SHADOW_TEXTURE, GL_TEXTURE_2D, pStaticShadowTexture->GetDepthTexture());
		}

		if ((DLightProgramState & DLIGHT_CSM_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
		{
			if (prog.u_csmMatrices != -1)
			{
				glUniformMatrix4fv(prog.u_csmMatrices, 4, GL_FALSE, (float*)pDynamicShadowTexture->GetShadowMatrix(0));
			}

			if (prog.u_csmDistances != -1)
			{
				glUniform4f(prog.u_csmDistances, 
					pDynamicShadowTexture->GetCSMDistance(0),
					pDynamicShadowTexture->GetCSMDistance(1),
					pDynamicShadowTexture->GetCSMDistance(2),
					pDynamicShadowTexture->GetCSMDistance(3));
			}

			if (prog.u_csmTexel != -1)
			{
				// CSM now uses texture array: 4096x4096x4, each layer is full 4096x4096
				glUniform2f(prog.u_csmTexel, pDynamicShadowTexture->GetTextureSize(), 1.0f / (float)pDynamicShadowTexture->GetTextureSize());
			}

			GL_BindTextureUnit(DSHADE_BIND_CSM_TEXTURE, GL_TEXTURE_2D_ARRAY, pDynamicShadowTexture->GetDepthTexture());
		}

		const uint32_t indices[] = { 0,1,2,2,3,0 };
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

		if ((DLightProgramState & DLIGHT_CSM_SHADOW_TEXTURE_ENABLED) && pDynamicShadowTexture)
		{
			GL_BindTextureUnit(DSHADE_BIND_CSM_TEXTURE, GL_TEXTURE_2D_ARRAY, 0);
		}

		if ((DLightProgramState & DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED) && pStaticShadowTexture)
		{
			GL_BindTextureUnit(DSHADE_BIND_STATIC_SHADOW_TEXTURE, GL_TEXTURE_2D, 0);
		}

		GL_UseProgram(0);

		GL_BindVAO(0);

		GL_Finish2D();

		GL_EndDebugGroup();
	};

	R_IterateVisibleDynamicLights(PointLightCallback, SpotLightCallback, DirectionalLightCallback, nullptr);

	glDisable(GL_BLEND);

	GL_BindTextureUnit(DSHADE_BIND_CONE_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(DSHADE_BIND_STENCIL_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(DSHADE_BIND_DEPTH_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(DSHADE_BIND_SPECULAR_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(DSHADE_BIND_WORLDNORM_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(DSHADE_BIND_LIGHTMAP_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(DSHADE_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, 0);

	GL_EndDebugGroup();
}

/*
	Purpose : final shading pass that blits colors from s_GBufferFBO to dst, also blits depth and stencil.
*/

void R_FinalShadingPass(FBO_Container_t *dst)
{
	//Write GBuffer depth and stencil buffer into dst framebuffer
	GL_BlitFrameBufferToFrameBufferDepthStencil(&s_GBufferFBO, dst);

	GL_BeginDebugGroupFormat("R_FinalShadingPass - write to %s", GL_GetFrameBufferName(dst));

	//Re-enable depth write
	glDepthMask(GL_TRUE);

	GL_BindFrameBuffer(dst);

	GL_Set2DEx(0, 0, dst->iWidth, dst->iHeight);

	//Only draw color0 channel
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + 0);

	//No blend for final shading pass
	glDisable(GL_BLEND);

	program_state_t FinalProgramState = 0;

	if (R_IsRenderingFog())
	{
		if (r_fog_mode == GL_LINEAR)
			FinalProgramState |= DFINAL_LINEAR_FOG_ENABLED;
		else if (r_fog_mode == GL_EXP)
			FinalProgramState |= DFINAL_EXP_FOG_ENABLED;
		else if (r_fog_mode == GL_EXP2)
			FinalProgramState |= DFINAL_EXP2_FOG_ENABLED;
	}

	if ((int)r_wsurf_sky_fog->value > 0)
	{
		FinalProgramState |= DFINAL_SKY_FOG_ENABLED;
	}

	if ((int)r_ssr->value > 0)
	{
		FinalProgramState |= DFINAL_SSR_ENABLED;

		if (r_ssr_adaptive_step->GetValue())
			FinalProgramState |= DFINAL_SSR_ADAPTIVE_STEP_ENABLED;

		if (r_ssr_exponential_step->GetValue())
			FinalProgramState |= DFINAL_SSR_EXPONENTIAL_STEP_ENABLED;

		if (r_ssr_binary_search->GetValue())
			FinalProgramState |= DFINAL_SSR_BINARY_SEARCH_ENABLED;
	}

	if (r_linear_fog_shift->value > 0)
	{
		FinalProgramState |= DFINAL_LINEAR_FOG_SHIFT_ENABLED;
	}

	R_UseDFinalProgram(FinalProgramState, NULL);

	GL_BindVAO(r_empty_vao);

	//Texture unit 0 = GBuffer texture array
	GL_BindTextureUnit(DFINAL_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex);

	//Texture unit 1 = GBuffer lightmap array
	GL_BindTextureUnit(DFINAL_BIND_LIGHTMAP_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex2);

	//Texture unit 2 = GBuffer worldnorm array
	GL_BindTextureUnit(DFINAL_BIND_WORLDNORM_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex3);

	//Texture unit 3 = GBuffer specular array
	GL_BindTextureUnit(DFINAL_BIND_SPECULAR_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex4);

	//Texture unit 4 = Depth texture
	GL_BindTextureUnit(DFINAL_BIND_DEPTH_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

	//Texture unit 5 = Stencil texture
	GL_BindTextureUnit(DFINAL_BIND_STENCIL_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferStencilView);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Texture unit 5 = Stencil texture
	GL_BindTextureUnit(DFINAL_BIND_STENCIL_TEXTURE, GL_TEXTURE_2D, 0);

	//Texture unit 4 = Depth texture
	GL_BindTextureUnit(DFINAL_BIND_DEPTH_TEXTURE, GL_TEXTURE_2D, 0);

	//Texture unit 3 = GBuffer specular array
	GL_BindTextureUnit(DFINAL_BIND_SPECULAR_TEXTURE, GL_TEXTURE_2D, 0);

	//Texture unit 2 = GBuffer worldnorm array
	GL_BindTextureUnit(DFINAL_BIND_WORLDNORM_TEXTURE, GL_TEXTURE_2D, 0);

	//Texture unit 1 = GBuffer lightmap array
	GL_BindTextureUnit(DFINAL_BIND_LIGHTMAP_TEXTURE, GL_TEXTURE_2D, 0);

	//Texture unit 0 = GBuffer texture array
	GL_BindTextureUnit(DFINAL_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	GL_Finish2D();

	GL_EndDebugGroup();
}

void R_EndRenderGBuffer(FBO_Container_t *dst)
{
	if (R_IsAmbientOcclusionEnabled())
	{
		R_LinearizeDepth(&s_GBufferFBO, &s_DepthLinearFBO);
		R_AmbientOcclusion(&s_DepthLinearFBO, &s_GBufferFBO);
	}

	R_LightShadingPass();

	R_FinalShadingPass(dst);

	r_draw_gbuffer = false;
	gbuffer_mask = -1;
}

void R_BlitGBufferToFrameBuffer(FBO_Container_t *dst, bool color, bool depth, bool stencil)
{
	if (depth && stencil)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
		glBlitFramebuffer(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	}
	else if (depth && !stencil)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
		glBlitFramebuffer(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
	else if (!depth && stencil)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->s_hBackBufferFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
		glBlitFramebuffer(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight, 0, 0, dst->iWidth, dst->iHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	}

	//Shading pass
	GL_BindFrameBuffer(dst);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (color)
	{
		//No blend for final shading pass
		glDisable(GL_BLEND);

		program_state_t FinalProgramState = 0;

		//Setup final program
		R_UseDFinalProgram(FinalProgramState, NULL);

		GL_BindVAO(r_empty_vao);

		//Texture unit 0 = GBuffer diffuse color
		GL_BindTextureUnit(DFINAL_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex);

		//Texture unit 1 = GBuffer lightmap array
		GL_BindTextureUnit(DFINAL_BIND_LIGHTMAP_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex2);
		
		//Texture unit 2 = GBuffer worldnorm array
		GL_BindTextureUnit(DFINAL_BIND_WORLDNORM_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex3);

		//Texture unit 4 = GBuffer specular array
		GL_BindTextureUnit(DFINAL_BIND_SPECULAR_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex4);

		//Texture unit 4 = GBuffer depth texture
		GL_BindTextureUnit(DFINAL_BIND_DEPTH_TEXTURE, GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		//Disable texture unit 4 (GBuffer depth)
		GL_BindTextureUnit(DFINAL_BIND_DEPTH_TEXTURE, GL_TEXTURE_2D, 0);

		//Disable texture unit 3 (GBuffer specular)
		GL_BindTextureUnit(DFINAL_BIND_SPECULAR_TEXTURE, GL_TEXTURE_2D, 0);

		//Disable texture unit 2 (GBuffer worldnorm)
		GL_BindTextureUnit(DFINAL_BIND_WORLDNORM_TEXTURE, GL_TEXTURE_2D, 0);

		//Disable texture unit 1 (GBuffer lightmap)
		GL_BindTextureUnit(DFINAL_BIND_LIGHTMAP_TEXTURE, GL_TEXTURE_2D, 0);
		
		//Switch back to texture unit 0 (GBuffer diffuse)
		GL_BindTextureUnit(DFINAL_BIND_DIFFUSE_TEXTURE, GL_TEXTURE_2D, 0);

		GL_BindVAO(0);

		GL_UseProgram(0);
	}
}

/*
	Purpose: Get CDynamicLight at given slot, allocate one if no dynamic light is available.
*/
std::shared_ptr<CDynamicLight> R_GetEngineDynamicLight(int index)
{
	if (g_EngineDynamicLights[index]) {
		return g_EngineDynamicLights[index];
	}

	std::shared_ptr<CDynamicLight> dynamicLight = std::make_shared<CDynamicLight>();

	g_EngineDynamicLights[index] = dynamicLight;

	return dynamicLight;
}

/*
	Purpose: Map cl_dlights array to g_EngineDynamicLights
*/
void R_ProcessEngineDynamicLights()
{
	int max_dlight = EngineGetMaxDLights();

	if (g_EngineDynamicLights.size() < max_dlight)
	{
		g_EngineDynamicLights.resize(max_dlight);
	}

	dlight_t* dl = cl_dlights;
	float curtime = (*cl_time);

	for (int i = 0; i < max_dlight; i++, dl++)
	{
		std::shared_ptr<CDynamicLight> dynamicLight = R_GetEngineDynamicLight(i);

		if (dl->die < curtime || !dl->radius)
		{
			dynamicLight->type = DynamicLightType_Unknown;
			continue;
		}

		if (R_IsDLightFlashlight(dl))
		{
			vec3_t dlight_origin;
			vec3_t dlight_angles;
			vec3_t dlight_vforward;
			vec3_t dlight_vright;
			vec3_t dlight_vup;

			auto ent = R_GetDLightBindingEntity(dl);

			if (!ent)
			{
				dynamicLight->type = DynamicLightType_Unknown;
				continue;
			}

			dynamicLight->source_entity_index = ent->index;

			bool bIsFromLocalPlayer = (ent == gEngfuncs.GetLocalPlayer()) ? true : false;

			vec3_t org = { 0 };
			vec3_t end = { 0 };

			float maxDistance = r_flashlight_distance->GetValue();
			float minDistance = r_flashlight_min_distance->GetValue();

			if (bIsFromLocalPlayer && R_IsRenderingFirstPersonView())
			{
				VectorCopy((*r_refdef.viewangles), dlight_angles);
				AngleVectors(dlight_angles, dlight_vforward, dlight_vright, dlight_vup);

				bool bUsingAttachment = false;

				if (cl_viewent && cl_viewent->model && r_flashlight_attachment->GetValue() > 0)
				{
					int attachmentIndex = (int)(r_flashlight_attachment->GetValue());

					attachmentIndex = math_clamp(attachmentIndex, 1, 4) - 1;

					if (cl_viewent->model)
					{
						auto pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(cl_viewent->model);

						if (pstudiohdr)
						{
							auto numattachments = pstudiohdr->numattachments;

							if (attachmentIndex < numattachments)
							{
								bUsingAttachment = true;

								VectorCopy(cl_viewent->attachment[attachmentIndex], org);
							}
						}
					}
				}

				if (!bUsingAttachment)
				{
					VectorCopy((*r_refdef.vieworg), org);
					VectorMA(org, 2, dlight_vup, org);
					if (cl_righthand && cl_righthand->value > 0)
					{
						VectorMA(org, 10, dlight_vright, org);
					}
					else
					{
						VectorMA(org, -10, dlight_vright, org);
					}
				}

				VectorCopy(org, dlight_origin);
				VectorMA(org, maxDistance, dlight_vforward, end);
			}
			else
			{
				VectorCopy(ent->angles, dlight_angles);
				dlight_angles[0] = dlight_angles[0] * -3.0f;

				AngleVectors(dlight_angles, dlight_vforward, dlight_vright, dlight_vup);

				VectorCopy(ent->origin, org);
				VectorMA(org, 8, dlight_vup, org);
				VectorMA(org, 10, dlight_vright, org);

				VectorCopy(org, dlight_origin);
				VectorMA(org, maxDistance, dlight_vforward, end);
			}
#if 1//Don't do such thing for spotlight
			struct pmtrace_s trace {};

			if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
			{
				// Trace a line outward, don't use hitboxes (too slow)
				pmove_10152->usehull = 2;
				trace = pmove_10152->PM_PlayerTrace(dlight_origin, end, PM_GLASS_IGNORE, -1);

				float distance = trace.fraction * maxDistance;

				if (trace.startsolid || distance < minDistance) {
					dynamicLight->type = DynamicLightType_Unknown;
					continue;
				}
			}
			else
			{
				// Trace a line outward, don't use hitboxes (too slow)
				pmove->usehull = 2;
				trace = pmove->PM_PlayerTrace(dlight_origin, end, PM_GLASS_IGNORE, -1);

				float distance = trace.fraction * maxDistance;

				if (trace.startsolid || distance < minDistance) {
					dynamicLight->type = DynamicLightType_Unknown;
					continue;
				}
			}
#endif
			float coneAngle = r_flashlight_cone_degree->GetValue() * M_PI / 360;
			float coneTanAngle = tanf(coneAngle);
			float radius = maxDistance * coneTanAngle;

			float ambient = r_flashlight_ambient->GetValue();
			float diffuse = r_flashlight_diffuse->GetValue();
			float specular = r_flashlight_specular->GetValue();
			float specularpow = r_flashlight_specularpow->GetValue();

			vec3_t dlight_color;
			dlight_color[0] = (float)dl->color.r / 255.0f;
			dlight_color[1] = (float)dl->color.g / 255.0f;
			dlight_color[2] = (float)dl->color.b / 255.0f;

			dynamicLight->type = DynamicLightType_Spot;

			dynamicLight->distance = maxDistance;
			dynamicLight->size = radius;
			dynamicLight->coneAngle = coneAngle;
			VectorCopy(dlight_origin, dynamicLight->origin);
			VectorCopy(dlight_angles, dynamicLight->angles);
			VectorCopy(dlight_color, dynamicLight->color);

			dynamicLight->ambient = ambient;
			dynamicLight->diffuse = diffuse;
			dynamicLight->specular = specular;
			dynamicLight->specularpow = specularpow;

			dynamicLight->static_shadow_size = 0;
			dynamicLight->dynamic_shadow_size = 256;
			dynamicLight->shadow = 1;
		}
		else
		{
			dynamicLight->type = DynamicLightType_Point;

			vec3_t dlight_color;
			dlight_color[0] = (float)dl->color.r / 255.0f;
			dlight_color[1] = (float)dl->color.g / 255.0f;
			dlight_color[2] = (float)dl->color.b / 255.0f;

			float ambient = r_dynlight_ambient->GetValue();
			float diffuse = r_dynlight_diffuse->GetValue();
			float specular = r_dynlight_specular->GetValue();
			float specularpow = r_dynlight_specularpow->GetValue();

			float radius = math_clamp(dl->radius, 0, 999999);

			vec3_t distToLight;
			VectorSubtract((*r_refdef.vieworg), dl->origin, distToLight);

			dynamicLight->size = radius;
			VectorCopy(dl->origin, dynamicLight->origin);
			VectorCopy(dlight_color, dynamicLight->color);

			dynamicLight->ambient = ambient;
			dynamicLight->diffuse = diffuse;
			dynamicLight->specular = specular;
			dynamicLight->specularpow = specularpow;

			dynamicLight->static_shadow_size = 0;
			dynamicLight->dynamic_shadow_size = 0;
			dynamicLight->shadow = 0;
		}
	}
}
