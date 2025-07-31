#include "gl_local.h"
#include "pm_defs.h"
#include <sstream>

cvar_t * r_deferred_lighting = NULL;
cvar_t *r_light_debug = NULL;

MapConVar* r_flashlight_enable = NULL;
MapConVar *r_flashlight_ambient = NULL;
MapConVar *r_flashlight_diffuse = NULL;
MapConVar *r_flashlight_specular = NULL;
MapConVar *r_flashlight_specularpow = NULL;
MapConVar *r_flashlight_attachment = NULL;
MapConVar *r_flashlight_distance = NULL;
MapConVar* r_flashlight_min_distance = NULL;
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

std::vector<light_dynamic_t> g_DynamicLights;

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

		prog.program = R_CompileShaderFileEx("renderer\\shader\\fullscreentriangle.vert.glsl", "renderer\\shader\\dfinal_shader.frag.glsl", def.c_str(), def.c_str(), NULL);
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
		g_pMetaHookAPI->SysError("R_UseDFinalProgram: Failed to load program!");
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

		prog.program = R_CompileShaderFileEx("renderer\\shader\\dlight_shader.vert.glsl", "renderer\\shader\\dlight_shader.frag.glsl", def.c_str(), def.c_str(), NULL);
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

const program_state_mapping_t s_DLightProgramStateName[] = {
{ DLIGHT_SPOT_ENABLED				,"DLIGHT_SPOT_ENABLED"	 },
{ DLIGHT_POINT_ENABLED				,"DLIGHT_POINT_ENABLED"	 },
{ DLIGHT_VOLUME_ENABLED				,"DLIGHT_VOLUME_ENABLED" },
{ DLIGHT_CONE_TEXTURE_ENABLED		,"DLIGHT_CONE_TEXTURE_ENABLED" },
{ DLIGHT_SHADOW_TEXTURE_ENABLED		,"DLIGHT_SHADOW_TEXTURE_ENABLED" },
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
	r_deferred_lighting = gEngfuncs.pfnRegisterVariable("r_deferred_lighting", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_debug = gEngfuncs.pfnRegisterVariable("r_light_debug", "0", FCVAR_CLIENTDLL);

	r_dynlight_ambient = R_RegisterMapCvar("r_dynlight_ambient", "0.2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_diffuse = R_RegisterMapCvar("r_dynlight_diffuse", "0.4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_specular = R_RegisterMapCvar("r_dynlight_specular", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_dynlight_specularpow = R_RegisterMapCvar("r_dynlight_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_flashlight_enable = R_RegisterMapCvar("r_flashlight_enable", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_ambient = R_RegisterMapCvar("r_flashlight_ambient", "0.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_diffuse = R_RegisterMapCvar("r_flashlight_diffuse", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_specular = R_RegisterMapCvar("r_flashlight_specular", "2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_specularpow = R_RegisterMapCvar("r_flashlight_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_attachment = R_RegisterMapCvar("r_flashlight_attachment", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_flashlight_distance = R_RegisterMapCvar("r_flashlight_distance", "2000", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_min_distance = R_RegisterMapCvar("r_flashlight_min_distance", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
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
	GL_UploadDataToVBOStaticDraw(r_sphere_vbo, sphereVertices.size() * sizeof(float), sphereVertices.data());

	r_sphere_ebo = GL_GenBuffer();
	GL_UploadDataToVBOStaticDraw(r_sphere_ebo, sphereIndices.size() * sizeof(int), sphereIndices.data());

	r_sphere_vao = GL_GenVAO();

	GL_BindStatesForVAO(r_sphere_vao, r_sphere_vbo, r_sphere_ebo,
	[]() {
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	}, 
	[]() {
		glDisableVertexAttribArray(0);
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
	},
	[]() {
		glDisableVertexAttribArray(0);
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

bool R_BeginRenderGBuffer(void)
{
	if (!R_CanRenderGBuffer())
		return false;

	r_draw_gbuffer = true;
	gbuffer_mask = -1;

	GL_BindFrameBuffer(&s_GBufferFBO);

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	R_SetGBufferBlend(GL_ONE, GL_ZERO);

	vec4_t vecClearColor = { 0, 0, 0, 0 };

	GL_ClearColorDepthStencil(vecClearColor, 1, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

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

void R_IterateDynamicLights(fnPointLightCallback pointlight_callback, fnSpotLightCallback spotlight_callback, void *context)
{
	for (size_t i = 0; i < g_DynamicLights.size(); i++)
	{
		auto &dynlight = g_DynamicLights[i];

		if (dynlight.type == DLIGHT_POINT)
		{
			float radius = math_clamp(dynlight.distance, 0, 999999);

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

				PointLightCallbackArgs args;
				args.radius = radius;
				VectorCopy(dynlight.origin, args.origin);
				VectorCopy(dynlight.color, args.color);

				//GammaToLinear(args.color);

				args.ambient = dynlight.ambient;
				args.diffuse = dynlight.diffuse;
				args.specular = dynlight.specular;
				args.specularpow = dynlight.specularpow;
				args.shadowtex = &dynlight.shadowtex;
				args.bVolume = true;

				pointlight_callback(&args, context);
			}
			else
			{
				PointLightCallbackArgs args;
				args.radius = radius;
				VectorCopy(dynlight.origin, args.origin);
				VectorCopy(dynlight.color, args.color);

				//GammaToLinear(args.color);

				args.ambient = dynlight.ambient;
				args.diffuse = dynlight.diffuse;
				args.specular = dynlight.specular;
				args.specularpow = dynlight.specularpow;
				args.shadowtex = &dynlight.shadowtex;
				args.bVolume = false;

				pointlight_callback(&args, context);
			}
		}
	}

	int max_dlight = EngineGetMaxDLights();
	
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

			auto ent = R_GetDLightBindingEntity(dl);

			if (!ent)
				continue;

			bool bIsFromLocalPlayer = (ent == gEngfuncs.GetLocalPlayer()) ? true : false;

			vec3_t org = { 0 };
			vec3_t end = { 0 };

			float max_distance = r_flashlight_distance->GetValue();
			float min_distance = r_flashlight_min_distance->GetValue();

			if (bIsFromLocalPlayer && R_IsRenderingFirstPersonView())
			{
				VectorCopy((*r_refdef.viewangles), dlight_angle);
				gEngfuncs.pfnAngleVectors(dlight_angle, dlight_vforward, dlight_vright, dlight_vup);

				bool bUsingAttachment = false;

				if (cl_viewent && cl_viewent->model && r_flashlight_attachment->GetValue() > 0)
				{
					int attachmentIndex = (int)(r_flashlight_attachment->GetValue());

					attachmentIndex = math_clamp(attachmentIndex, 1, 4) - 1;

					if (cl_viewent->model)
					{
						auto pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(cl_viewent->model);

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
				VectorMA(org, max_distance, dlight_vforward, end);
			}
			else
			{
				VectorCopy(ent->angles, dlight_angle);
				dlight_angle[0] = dlight_angle[0] * -3.0f;

				gEngfuncs.pfnAngleVectors(dlight_angle, dlight_vforward, dlight_vright, dlight_vup);

				VectorCopy(ent->origin, org);
				VectorMA(org, 8, dlight_vup, org);
				VectorMA(org, 10, dlight_vright, org);

				VectorCopy(org, dlight_origin);
				VectorMA(org, max_distance, dlight_vforward, end);
			}
#if 1//Don't do such thing for spotlight
			struct pmtrace_s trace {};

			//auto iLocalPlayerPhysEntIndex = EngineFindPhysEntIndexByEntity(gEngfuncs.GetLocalPlayer());

			if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
			{
				// Trace a line outward, don't use hitboxes (too slow)
				pmove_10152->usehull = 2;
				trace = pmove_10152->PM_PlayerTrace(dlight_origin, end, PM_GLASS_IGNORE, -1);

				float distance = trace.fraction * max_distance;

				if (trace.startsolid || distance < min_distance)
					continue;
			}
			else
			{
				// Trace a line outward, don't use hitboxes (too slow)
				pmove->usehull = 2;
				trace = pmove->PM_PlayerTrace(dlight_origin, end, PM_GLASS_IGNORE, -1);

				float distance = trace.fraction * max_distance;

				if (trace.startsolid || distance < min_distance)
					continue;
			}
#endif
			float coneCosAngle = r_flashlight_cone_cosine->GetValue();
			float coneAngle = acosf(coneCosAngle);
			float coneSinAngle = sqrt(1 - coneCosAngle * coneCosAngle);
			float coneTanAngle = tanf(coneAngle);
			float radius = max_distance * coneTanAngle;
			
			float ambient = r_flashlight_ambient->GetValue();
			float diffuse = r_flashlight_diffuse->GetValue();
			float specular = r_flashlight_specular->GetValue();
			float specularpow = r_flashlight_specularpow->GetValue();

			vec3_t color;
			color[0] = (float)dl->color.r / 255.0f;
			color[1] = (float)dl->color.g / 255.0f;
			color[2] = (float)dl->color.b / 255.0f;

			if (!Util_IsOriginInCone((*r_refdef.vieworg), dlight_origin, dlight_vforward, coneCosAngle, max_distance))
			{
				SpotLightCallbackArgs args;
				args.distance = max_distance;
				args.radius = radius;
				args.coneAngle = coneAngle;
				args.coneCosAngle = coneCosAngle;
				args.coneSinAngle = coneSinAngle;
				args.coneTanAngle = coneTanAngle;
				VectorCopy(dlight_origin, args.origin);
				VectorCopy(dlight_angle, args.angle);
				VectorCopy(dlight_vforward, args.vforward);
				VectorCopy(dlight_vright, args.vright);
				VectorCopy(dlight_vup, args.vup);
				VectorCopy(color, args.color);

				//GammaToLinear(args.color);

				args.ambient = ambient;
				args.diffuse = diffuse;
				args.specular = specular;
				args.specularpow = specularpow;
				args.shadowtex = &cl_dlight_shadow_textures[i];
				args.bVolume = true;
				args.bIsFromLocalPlayer = bIsFromLocalPlayer;

				spotlight_callback(&args, context);
			}
			else
			{
				SpotLightCallbackArgs args;
				args.distance = max_distance;
				args.radius = radius;
				args.coneAngle = coneAngle;
				args.coneCosAngle = coneCosAngle;
				args.coneSinAngle = coneSinAngle;
				args.coneTanAngle = coneTanAngle;
				VectorCopy(dlight_origin, args.origin);
				VectorCopy(dlight_angle, args.angle);
				VectorCopy(dlight_vforward, args.vforward);
				VectorCopy(dlight_vright, args.vright);
				VectorCopy(dlight_vup, args.vup);
				VectorCopy(color, args.color);

				//GammaToLinear(args.color);

				args.ambient = ambient;
				args.diffuse = diffuse;
				args.specular = specular;
				args.specularpow = specularpow;
				args.shadowtex = &cl_dlight_shadow_textures[i];
				args.bVolume = false;
				args.bIsFromLocalPlayer = bIsFromLocalPlayer;

				spotlight_callback(&args, context);
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

			float radius = math_clamp(dl->radius, 0, 999999);

			vec3_t distToLight;
			VectorSubtract((*r_refdef.vieworg), dl->origin, distToLight);

			if (VectorLength(distToLight) > radius + 32)
			{
				vec3_t mins, maxs;
				for (int j = 0; j < 3; j++)
				{
					mins[j] = dl->origin[j] - radius;
					maxs[j] = dl->origin[j] + radius;
				}

				if (R_CullBox(mins, maxs))
					continue;

				PointLightCallbackArgs args;
				args.radius = radius;
				VectorCopy(dl->origin, args.origin);
				VectorCopy(color, args.color);

				//GammaToLinear(args.color);

				args.ambient = ambient;
				args.diffuse = diffuse;
				args.specular = specular;
				args.specularpow = specularpow;
				args.shadowtex = &cl_dlight_shadow_textures[i];
				args.bVolume = true;

				pointlight_callback(&args, context);
			}
			else
			{
				PointLightCallbackArgs args;
				args.radius = radius;
				VectorCopy(dl->origin, args.origin);
				VectorCopy(color, args.color);

				//GammaToLinear(args.color);

				args.ambient = ambient;
				args.diffuse = diffuse;
				args.specular = specular;
				args.specularpow = specularpow;
				args.shadowtex = &cl_dlight_shadow_textures[i];
				args.bVolume = false;

				pointlight_callback(&args, context);
			}
		}
	}
}

void R_LightShadingPass(void)
{
	//Disable depth write and re-enable later after light pass.
	glDepthMask(GL_FALSE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	//Texture unit 0 = GBuffer diffuse array
	GL_Bind(s_GBufferFBO.s_hBackBufferTex);

	//Texture unit 1 = GBuffer lightmap array
	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferTex2);

	//Texture unit 2 = GBuffer worldnorm array
	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_WORLDNORM_TEXTURE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex3);

	//Texture unit 3 = GBuffer specular array
	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_SPECULAR_TEXTURE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex4);

	//Texture unit 4 = Depth texture
	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_DEPTH_TEXTURE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

	//Texture unit 5 = Stencil texture
	if (s_GBufferFBO.s_hBackBufferStencilView)
	{
		glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_STENCIL_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferStencilView);
	}

	//Texture unit 6 = Flashlight cone texture
	if (r_flashlight_cone_texture)
	{
		glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_CONE_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, r_flashlight_cone_texture);
	}

	const auto PointLightCallback = [](PointLightCallbackArgs *args, void *context)
	{
		if (args->bVolume)
		{
			GL_BindVAO(r_sphere_vao);

			glPushMatrix();
			glLoadIdentity();
			glTranslatef(args->origin[0], args->origin[1], args->origin[2]);
			glScalef(args->radius, args->radius, args->radius);

			float modelmatrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
			glPopMatrix();

			program_state_t DLightProgramState = DLIGHT_POINT_ENABLED | DLIGHT_VOLUME_ENABLED;

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			glUniformMatrix4fv(prog.u_modelmatrix, 1, false, modelmatrix);
			glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			glUniform1f(prog.u_lightradius, args->radius);
			glUniform1f(prog.u_lightambient, args->ambient);
			glUniform1f(prog.u_lightdiffuse, args->diffuse);
			glUniform1f(prog.u_lightspecular, args->specular);
			glUniform1f(prog.u_lightspecularpow, args->specularpow);

			glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

			GL_BindVAO(0);
		}
		else
		{
			GL_BeginFullScreenQuad(false);

			program_state_t DLightProgramState = DLIGHT_POINT_ENABLED;

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);
			glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			glUniform1f(prog.u_lightradius, args->radius);
			glUniform1f(prog.u_lightambient, args->ambient);
			glUniform1f(prog.u_lightdiffuse, args->diffuse);
			glUniform1f(prog.u_lightspecular, args->specular);
			glUniform1f(prog.u_lightspecularpow, args->specularpow);

			glDrawArrays(GL_QUADS, 0, 4);

			GL_EndFullScreenQuad();
		}
	};

	const auto SpotLightCallback = [](SpotLightCallbackArgs *args, void *context)
	{
		if (args->bVolume)
		{
			GL_BindVAO(r_cone_vao);

			glPushMatrix();
			glLoadIdentity();
			glTranslatef(args->origin[0], args->origin[1], args->origin[2]);
			glRotatef(args->angle[1], 0, 0, 1);
			glRotatef(args->angle[0], 0, 1, 0);
			glRotatef(args->angle[2], 1, 0, 0);
			glScalef(args->distance, args->radius, args->radius);

			float modelmatrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
			glPopMatrix();

			program_state_t DLightProgramState = DLIGHT_SPOT_ENABLED | DLIGHT_VOLUME_ENABLED;

			if (r_flashlight_cone_texture)
			{
				DLightProgramState |= DLIGHT_CONE_TEXTURE_ENABLED;
			}

			if (args->shadowtex->depth_stencil && args->shadowtex->ready)
			{
				DLightProgramState |= DLIGHT_SHADOW_TEXTURE_ENABLED;

				glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_SHADOWMAP_TEXTURE);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, args->shadowtex->depth_stencil);
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			glUniformMatrix4fv(prog.u_modelmatrix, 1, false, modelmatrix);
			glUniform3f(prog.u_lightdir, args->vforward[0], args->vforward[1], args->vforward[2]);
			glUniform3f(prog.u_lightright, args->vright[0], args->vright[1], args->vright[2]);
			glUniform3f(prog.u_lightup, args->vup[0], args->vup[1], args->vup[2]);
			glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			glUniform2f(prog.u_lightcone, args->coneCosAngle, args->coneSinAngle);
			glUniform1f(prog.u_lightradius, args->distance);
			glUniform1f(prog.u_lightambient, args->ambient);
			glUniform1f(prog.u_lightdiffuse, args->diffuse);
			glUniform1f(prog.u_lightspecular, args->specular);
			glUniform1f(prog.u_lightspecularpow, args->specularpow);

			if (prog.u_shadowtexel != -1 && args->shadowtex->size > 0)
			{
				glUniform2f(prog.u_shadowtexel, args->shadowtex->size, 1.0f / (float)args->shadowtex->size);
			}

			if (prog.u_shadowmatrix != -1)
			{
				glUniformMatrix4fv(prog.u_shadowmatrix, 1, false, args->shadowtex->matrix);
			}

			glDrawArrays(GL_TRIANGLES, 0, X_SEGMENTS * 6);

			if (args->shadowtex->depth_stencil && args->shadowtex->ready)
			{
				glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_SHADOWMAP_TEXTURE);
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
			}

			GL_BindVAO(0);

			if (DLightProgramState & DLIGHT_SHADOW_TEXTURE_ENABLED)
			{
				glDisable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		else
		{
			GL_BeginFullScreenQuad(false);

			program_state_t DLightProgramState = DLIGHT_SPOT_ENABLED;

			if (r_flashlight_cone_texture)
			{
				DLightProgramState |= DLIGHT_CONE_TEXTURE_ENABLED;
			}

			if (args->shadowtex->depth_stencil && args->shadowtex->ready)
			{
				DLightProgramState |= DLIGHT_SHADOW_TEXTURE_ENABLED;

				glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_SHADOWMAP_TEXTURE);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, args->shadowtex->depth_stencil);
			}

			dlight_program_t prog = { 0 };
			R_UseDLightProgram(DLightProgramState, &prog);

			glUniform3f(prog.u_lightdir, args->vforward[0], args->vforward[1], args->vforward[2]);
			glUniform3f(prog.u_lightright, args->vright[0], args->vright[1], args->vright[2]);
			glUniform3f(prog.u_lightup, args->vup[0], args->vup[1], args->vup[2]);
			glUniform3f(prog.u_lightpos, args->origin[0], args->origin[1], args->origin[2]);
			glUniform3f(prog.u_lightcolor, args->color[0], args->color[1], args->color[2]);
			glUniform2f(prog.u_lightcone, args->coneCosAngle, args->coneSinAngle);
			glUniform1f(prog.u_lightradius, args->distance);
			glUniform1f(prog.u_lightambient, args->ambient);
			glUniform1f(prog.u_lightdiffuse, args->diffuse);
			glUniform1f(prog.u_lightspecular, args->specular);
			glUniform1f(prog.u_lightspecularpow, args->specularpow);

			if (prog.u_shadowtexel != -1 && args->shadowtex->size > 0)
			{
				glUniform2f(prog.u_shadowtexel, args->shadowtex->size, 1.0f / (float)args->shadowtex->size);
			}

			if (prog.u_shadowmatrix != -1)
			{
				glUniformMatrix4fv(prog.u_shadowmatrix, 1, false, args->shadowtex->matrix);
			}

			glDrawArrays(GL_QUADS, 0, 4);

			if (args->shadowtex->depth_stencil && args->shadowtex->ready)
			{
				glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_SHADOWMAP_TEXTURE);
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
			}

			GL_EndFullScreenQuad();

			if (DLightProgramState & DLIGHT_SHADOW_TEXTURE_ENABLED)
			{
				glDisable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

	};

	R_IterateDynamicLights(PointLightCallback, SpotLightCallback, NULL);

	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_CONE_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_STENCIL_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_DEPTH_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_SPECULAR_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_WORLDNORM_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0 + DSHADE_BIND_LIGHTMAP_TEXTURE);
	GL_Bind(0);

	GL_DisableMultitexture();
	GL_Bind(0);
}

/*
	Purpose : final shading pass that blits colors from s_GBufferFBO to dst, also blits depth and stencil.
*/

void R_FinalShadingPass(FBO_Container_t *dst)
{
	//Re-enable depth write
	glDepthMask(GL_TRUE);

	//Write GBuffer depth and stencil buffer into main framebuffer
	GL_BlitFrameBufferToFrameBufferDepthStencil(&s_GBufferFBO, dst);

	GL_BindFrameBuffer(dst);

	//Only draw color0 channel
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_DIFFUSE);

	//No blend for final shading pass
	glDisable(GL_BLEND);

	GL_BeginFullScreenQuad(false);

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

	if (r_linear_fog_shift->value > 0)
	{
		FinalProgramState |= DFINAL_LINEAR_FOG_SHIFT_ENABLED;
	}

	R_UseDFinalProgram(FinalProgramState, NULL);

	//Texture unit 0 = GBuffer texture array
	GL_Bind(s_GBufferFBO.s_hBackBufferTex);

	//Texture unit 1 = GBuffer lightmap array
	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferTex2);

	//Texture unit 2 = GBuffer worldnorm array
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_WORLDNORM_TEXTURE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex3);

	//Texture unit 3 = GBuffer specular array
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_SPECULAR_TEXTURE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex4);

	//Texture unit 4 = Depth texture
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_DEPTH_TEXTURE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

	//Texture unit 5 = Stencil texture
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_STENCIL_TEXTURE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferStencilView);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Disable texture unit 5 (stencil)
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_STENCIL_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Disable texture unit 4 (depth)
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_DEPTH_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Disable texture unit 3 (specular)
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_SPECULAR_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Disable texture unit 2 (worldnorm)
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_WORLDNORM_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Disable texture unit 1 (lightmap)
	glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_LIGHTMAP_TEXTURE);
	GL_Bind(0);

	GL_DisableMultitexture();
	GL_Bind(0);

	GL_UseProgram(0);

	GL_EndFullScreenQuad();
}

void R_EndRenderGBuffer(FBO_Container_t *dst)
{
	R_LinearizeDepth(&s_GBufferFBO, &s_DepthLinearFBO);

	if (R_IsAmbientOcclusionEnabled())
	{
		R_AmbientOcclusion(&s_DepthLinearFBO, &s_GBufferFBO);
	}

	//Write to GBuffer->lightmap only whatsoever
	GL_BindFrameBuffer(&s_GBufferFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_INDEX_LIGHTMAP);

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
		GL_BeginFullScreenQuad(false);

		//No blend for final shading pass
		glDisable(GL_BLEND);

		program_state_t FinalProgramState = 0;

		//Setup final program
		R_UseDFinalProgram(FinalProgramState, NULL);

		//Texture unit 0 = GBuffer
		GL_Bind(s_GBufferFBO.s_hBackBufferTex);

		//Texture unit 1 = GBuffer lightmap array
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_LIGHTMAP_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex2);
		
		//Texture unit 2 = GBuffer worldnorm array
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_WORLDNORM_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex3);

		//Texture unit 4 = GBuffer specular array
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_SPECULAR_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferTex4);

		//Texture unit 4 = GBuffer depth texture
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_DEPTH_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, s_GBufferFBO.s_hBackBufferDepthTex);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		//Disable texture unit 4 (GBuffer depth)
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_DEPTH_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Disable texture unit 3 (GBuffer specular)
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_SPECULAR_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Disable texture unit 2 (GBuffer worldnorm)
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_WORLDNORM_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Disable texture unit 1 (GBuffer lightmap)
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_LIGHTMAP_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		
		//Switch back to texture unit 0 (GBuffer diffuse)
		glActiveTexture(GL_TEXTURE0 + DFINAL_BIND_DIFFUSE_TEXTURE);

		GL_UseProgram(0);

		GL_EndFullScreenQuad();
	}
}