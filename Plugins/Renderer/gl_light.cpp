#include "gl_local.h"
#include <sstream>

shadow_control_t r_shadow_control; 

ssr_control_t r_ssr_control;

cvar_t *r_light_dynamic = NULL;
cvar_t *r_light_debug = NULL;

cvar_t *r_light_radius = NULL;
cvar_t *r_light_ambient = NULL;
cvar_t *r_light_diffuse = NULL;
cvar_t *r_light_specular = NULL;
cvar_t *r_light_specularpow = NULL;

cvar_t *r_flashlight_distance = NULL;
cvar_t *r_flashlight_cone = NULL;

cvar_t *r_ssr = NULL;
cvar_t *r_ssr_ray_step = NULL;
cvar_t *r_ssr_iter_count = NULL;
cvar_t *r_ssr_distance_bias = NULL;
cvar_t *r_ssr_exponential_step= NULL;
cvar_t *r_ssr_adaptive_step = NULL;
cvar_t *r_ssr_binary_search = NULL;
cvar_t *r_ssr_fade = NULL;

bool drawgbuffer = false;

int gbuffer_mask = -1;

GLuint r_sphere_vbo = 0;
GLuint r_sphere_ebo = 0;
GLuint r_cone_vbo = 0;

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

		if (state & DFINAL_SSR_ENABLED)
			defs << "#define SSR_ENABLED\n";

		if (state & DFINAL_SSR_ADAPTIVE_STEP_ENABLED)
			defs << "#define SSR_ADAPTIVE_STEP_ENABLED\n";

		if (state & DFINAL_SSR_EXPONENTIAL_STEP_ENABLED)
			defs << "#define SSR_EXPONENTIAL_STEP_ENABLED\n";

		if (state & DFINAL_SSR_BINARY_SEARCH_ENABLED)
			defs << "#define SSR_BINARY_SEARCH_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\dfinal_shader.vsh", "renderer\\shader\\dfinal_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, gbufferTex, "gbufferTex");
			SHADER_UNIFORM(prog, depthTex, "depthTex");
			SHADER_UNIFORM(prog, linearDepthTex, "linearDepthTex");
			SHADER_UNIFORM(prog, viewpos, "viewpos");
			SHADER_UNIFORM(prog, viewmatrix, "viewmatrix");
			SHADER_UNIFORM(prog, projmatrix, "projmatrix");
			SHADER_UNIFORM(prog, invprojmatrix, "invprojmatrix");
			SHADER_UNIFORM(prog, ssrRayStep, "ssrRayStep");
			SHADER_UNIFORM(prog, ssrIterCount, "ssrIterCount");
			SHADER_UNIFORM(prog, ssrDistanceBias, "ssrDistanceBias");
			SHADER_UNIFORM(prog, ssrFade, "ssrFade");
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

		if (prog.gbufferTex != -1)
			qglUniform1iARB(prog.gbufferTex, 0);
		if (prog.depthTex != -1)
			qglUniform1iARB(prog.depthTex, 1);

		if (prog.linearDepthTex != -1)
			qglUniform1iARB(prog.linearDepthTex, 2);
		if (prog.viewpos != -1)
			qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);

		if (prog.viewmatrix != -1)
			qglUniformMatrix4fvARB(prog.viewmatrix, 1, false, r_world_matrix);
		if (prog.projmatrix != -1)
			qglUniformMatrix4fvARB(prog.projmatrix, 1, false, r_projection_matrix);
		if (prog.invprojmatrix != -1)
			qglUniformMatrix4fvARB(prog.invprojmatrix, 1, false, r_proj_matrix_inv);

		if (prog.ssrRayStep != -1)
			qglUniform1fARB(prog.ssrRayStep, r_ssr_control.ray_step);

		if (prog.ssrIterCount != -1)
			qglUniform1iARB(prog.ssrIterCount, r_ssr_control.iter_count);

		if (prog.ssrDistanceBias != -1)
			qglUniform1fARB(prog.ssrDistanceBias, r_ssr_control.distance_bias);

		if (prog.ssrFade != -1)
			qglUniform2fARB(prog.ssrFade, r_ssr_control.fade[0], r_ssr_control.fade[1]);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseDFinalProgram: Failed to load program!");
	}
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

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\dlight_shader.vsh", "renderer\\shader\\dlight_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, gbufferTex, "gbufferTex");
			SHADER_UNIFORM(prog, stencilTex, "stencilTex");
			SHADER_UNIFORM(prog, depthTex, "depthTex");
			SHADER_UNIFORM(prog, viewpos, "viewpos");
			SHADER_UNIFORM(prog, lightdir, "lightdir");
			SHADER_UNIFORM(prog, lightpos, "lightpos");
			SHADER_UNIFORM(prog, lightcolor, "lightcolor");
			SHADER_UNIFORM(prog, lightcone, "lightcone");
			SHADER_UNIFORM(prog, lightradius, "lightradius");
			SHADER_UNIFORM(prog, lightambient, "lightambient");
			SHADER_UNIFORM(prog, lightdiffuse, "lightdiffuse");
			SHADER_UNIFORM(prog, lightspecular, "lightspecular");
			SHADER_UNIFORM(prog, lightspecularpow, "lightspecularpow");
			SHADER_UNIFORM(prog, modelmatrix, "modelmatrix");
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

		if (prog.gbufferTex != -1)
			qglUniform1iARB(prog.gbufferTex, 0);
		if (prog.depthTex != -1)
			qglUniform1iARB(prog.depthTex, 1);
		if (prog.stencilTex != -1)
			qglUniform1iARB(prog.stencilTex, 2);
		if (prog.viewpos != -1)
			qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseDLightProgram: Failed to load program!");
	}
}

void R_ShutdownLight(void)
{
	g_DFinalProgramTable.clear();
	g_DLightProgramTable.clear();

	if(r_sphere_vbo)
		qglDeleteBuffersARB(1, &r_sphere_vbo);

	if (r_sphere_ebo)
		qglDeleteBuffersARB(1, &r_sphere_ebo);

	if (r_cone_vbo)
		qglDeleteBuffersARB(1, &r_cone_vbo);
}

void R_InitLight(void)
{
	r_light_dynamic = gEngfuncs.pfnRegisterVariable("r_light_dynamic", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_debug = gEngfuncs.pfnRegisterVariable("r_light_debug", "0", FCVAR_CLIENTDLL);

	r_light_radius = gEngfuncs.pfnRegisterVariable("r_light_radius", "300.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_ambient = gEngfuncs.pfnRegisterVariable("r_light_ambient", "0.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_diffuse = gEngfuncs.pfnRegisterVariable("r_light_diffuse", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_specular = gEngfuncs.pfnRegisterVariable("r_light_specular", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_light_specularpow = gEngfuncs.pfnRegisterVariable("r_light_specularpow", "10", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_ssr = gEngfuncs.pfnRegisterVariable("r_ssr", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_ray_step = gEngfuncs.pfnRegisterVariable("r_ssr_ray_step", "5.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_iter_count = gEngfuncs.pfnRegisterVariable("r_ssr_iter_count", "64", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_distance_bias = gEngfuncs.pfnRegisterVariable("r_ssr_distance_bias", "0.2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_adaptive_step = gEngfuncs.pfnRegisterVariable("r_ssr_adaptive_step", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_exponential_step = gEngfuncs.pfnRegisterVariable("r_ssr_exponential_step", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_binary_search = gEngfuncs.pfnRegisterVariable("r_ssr_binary_search", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_ssr_fade = gEngfuncs.pfnRegisterVariable("r_ssr_fade", "0.8 1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	r_flashlight_distance = gEngfuncs.pfnRegisterVariable("r_flashlight_distance", "2000", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_flashlight_cone = gEngfuncs.pfnRegisterVariable("r_flashlight_cone", "0.9", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

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

	qglGenBuffersARB(1, &r_sphere_vbo);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_sphere_vbo);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	qglGenBuffersARB(1, &r_sphere_ebo);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_sphere_ebo);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sphereIndices.size() * sizeof(int), sphereIndices.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

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

	qglGenBuffersARB(1, &r_cone_vbo);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_cone_vbo);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, coneVertices.size() * sizeof(float), coneVertices.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	drawgbuffer = false;
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

void R_SetGBufferMask(int mask)
{
	if (!drawgbuffer)
		return;

	if (gbuffer_mask == mask)
		return;

	gbuffer_mask = mask;

	GLuint attachments[10] = {0};
	int attachCount = 0;

	for (int i = 0; i < GBUFFER_INDEX_MAX; ++i)
	{
		if (mask & (1 << i))
		{
			attachments[attachCount] = GL_COLOR_ATTACHMENT0 + i;
			attachCount++;
		}
	}
	
	qglDrawBuffers(attachCount, attachments);
}

bool R_BeginRenderGBuffer(void)
{
	if (r_draw_pass)
		return false;

	if (!r_light_dynamic->value)
		return false;

	drawgbuffer = true;
	gbuffer_mask = -1;

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	qglClearColor(0, 0, 0, 1);
	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglDepthMask(GL_TRUE);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglStencilMask(0);

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

void R_EndRenderGBuffer(void)
{
	if (!drawgbuffer)
		return;

	drawgbuffer = false;
	gbuffer_mask = -1;

	GL_PushDrawState();

	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);

	//Write linearized depth

	GL_Begin2D();
	R_LinearizeDepth(&s_GBufferFBO);
	GL_End2D();

	//Write to GBuffer->lightmap only
	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
	qglDrawBuffer(GL_COLOR_ATTACHMENT1);

	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_CULL_FACE);
	qglDepthMask(0);

	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);

	//Texture unit 0 = GBuffer texture array

	GL_SelectTexture(TEXTURE0_SGIS);
	qglDisable(GL_TEXTURE_2D);
	qglEnable(GL_TEXTURE_2D_ARRAY);
	qglBindTexture(GL_TEXTURE_2D_ARRAY, s_GBufferFBO.s_hBackBufferTex);
	*currenttexture = -1;

	//Texture unit 1 = Stencil texture
	GL_EnableMultitexture();
	GL_Bind(s_GBufferFBO.s_hBackBufferStencilView);
	
	if (g_DynamicLights.size())
	{
		for (size_t i = 0; i < g_DynamicLights.size(); i++)
		{
			auto &dynlight = g_DynamicLights[i];

			if (dynlight.type == DLIGHT_POINT)
			{
				//Point Light

				float radius = dynlight.distance;

				vec3_t dist;
				VectorSubtract(r_refdef->vieworg, dynlight.origin, dist);

				if (VectorLength(dist) > radius + 32)
				{
					dlight_program_t prog = { 0 };
					R_UseDLightProgram(DLIGHT_POINT_ENABLED | DLIGHT_VOLUME_ENABLED, &prog);
					qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
					qglUniform3fARB(prog.lightpos, dynlight.origin[0], dynlight.origin[1], dynlight.origin[2]);
					qglUniform3fARB(prog.lightcolor, dynlight.color[0], dynlight.color[1], dynlight.color[2]);
					qglUniform1fARB(prog.lightradius, radius);
					qglUniform1fARB(prog.lightambient, dynlight.ambient);
					qglUniform1fARB(prog.lightdiffuse, dynlight.diffuse);
					qglUniform1fARB(prog.lightspecular,dynlight.specular);
					qglUniform1fARB(prog.lightspecularpow, dynlight.specularpow);

					qglEnableClientState(GL_VERTEX_ARRAY);
					qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_sphere_vbo);
					qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_sphere_ebo);
					qglVertexPointer(3, GL_FLOAT, sizeof(float[3]), 0);

					qglPushMatrix();
					qglLoadIdentity();
					qglTranslatef(dynlight.origin[0], dynlight.origin[1], dynlight.origin[2]);
					qglScalef(radius, radius, radius);

					float modelmatrix[16];
					qglGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
					qglPopMatrix();

					qglUniformMatrix4fvARB(prog.modelmatrix, 1, false, modelmatrix);

					qglDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

					qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
					qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
					qglDisableClientState(GL_VERTEX_ARRAY);
				}
				else
				{
					dlight_program_t prog = { 0 };
					R_UseDLightProgram(DLIGHT_POINT_ENABLED, &prog);
					qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
					qglUniform3fARB(prog.lightpos, dynlight.origin[0], dynlight.origin[1], dynlight.origin[2]);
					qglUniform3fARB(prog.lightcolor, dynlight.color[0], dynlight.color[1], dynlight.color[2]);
					qglUniform1fARB(prog.lightradius, radius);
					qglUniform1fARB(prog.lightambient, dynlight.ambient);
					qglUniform1fARB(prog.lightdiffuse, dynlight.diffuse);
					qglUniform1fARB(prog.lightspecular, dynlight.specular);
					qglUniform1fARB(prog.lightspecularpow, dynlight.specularpow);

					GL_Begin2D();

					R_DrawHUDQuadFrustum(glwidth, glheight);

					GL_End2D();
				}
			}
		}
	}

	int max_dlight;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		max_dlight = 256;
	}
	else
	{
		max_dlight = 32;
	}

	dlight_t *dl = cl_dlights;

	for (int i = 0; i < max_dlight; i++, dl++)
	{
		if (dl->die < (*cl_time) || !dl->radius)
			continue;

		if (R_IsDLightFlashlight(dl))
		{
			vec3_t dlight_origin;
			vec3_t dlight_angle;
			vec3_t dlight_vforward;
			vec3_t dlight_vright;
			vec3_t dlight_vup;

			//Spot Light
			auto ent = gEngfuncs.GetEntityByIndex(dl->key);

			vec3_t org;
			if (ent == gEngfuncs.GetLocalPlayer() && !gExportfuncs.CL_IsThirdPerson())
			{
				VectorCopy(r_refdef->viewangles, dlight_angle);
				gEngfuncs.pfnAngleVectors(dlight_angle, dlight_vforward, dlight_vright, dlight_vup);

				VectorCopy(r_refdef->vieworg, org);
				VectorMA(org, 2, dlight_vup, org);
				VectorMA(org, 10, dlight_vright, org);

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

			if (Util_IsOriginInCone(r_refdef->vieworg, dlight_origin, dlight_vforward, r_flashlight_cone->value, r_flashlight_distance->value))
			{
				dlight_program_t prog = { 0 };
				R_UseDLightProgram(DLIGHT_SPOT_ENABLED, &prog);
				qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
				qglUniform3fARB(prog.lightdir, dlight_vforward[0], dlight_vforward[1], dlight_vforward[2]);
				qglUniform3fARB(prog.lightpos, dlight_origin[0], dlight_origin[1], dlight_origin[2]);
				qglUniform3fARB(prog.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
				qglUniform1fARB(prog.lightcone, r_flashlight_cone->value);
				qglUniform1fARB(prog.lightradius, r_flashlight_distance->value);
				qglUniform1fARB(prog.lightambient, r_light_ambient->value);
				qglUniform1fARB(prog.lightdiffuse, r_light_diffuse->value);
				qglUniform1fARB(prog.lightspecular, r_light_specular->value);
				qglUniform1fARB(prog.lightspecularpow, r_light_specularpow->value);

				GL_Begin2D();

				R_DrawHUDQuadFrustum(glwidth, glheight);

				GL_End2D();
			}
			else
			{
				dlight_program_t prog = { 0 };
				R_UseDLightProgram(DLIGHT_SPOT_ENABLED | DLIGHT_VOLUME_ENABLED, &prog);
				qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
				qglUniform3fARB(prog.lightdir, dlight_vforward[0], dlight_vforward[1], dlight_vforward[2]);
				qglUniform3fARB(prog.lightpos, dlight_origin[0], dlight_origin[1], dlight_origin[2]);
				qglUniform3fARB(prog.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
				qglUniform1fARB(prog.lightcone, r_flashlight_cone->value);
				qglUniform1fARB(prog.lightradius, r_flashlight_distance->value);
				qglUniform1fARB(prog.lightambient, r_light_ambient->value);
				qglUniform1fARB(prog.lightdiffuse, r_light_diffuse->value);
				qglUniform1fARB(prog.lightspecular, r_light_specular->value);
				qglUniform1fARB(prog.lightspecularpow, r_light_specularpow->value);

				qglEnableClientState(GL_VERTEX_ARRAY);
				qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_cone_vbo);
				qglVertexPointer(3, GL_FLOAT, sizeof(float[3]), 0);

				float ang = acosf(r_flashlight_cone->value);
				float tan = tanf(ang);
				float radius = r_flashlight_distance->value * tan;

				qglPushMatrix();
				qglLoadIdentity();
				qglTranslatef(dlight_origin[0], dlight_origin[1], dlight_origin[2]);
				qglRotatef(dlight_angle[1], 0, 0, 1);
				qglRotatef(dlight_angle[0], 0, 1, 0);
				qglRotatef(dlight_angle[2], 1, 0, 0);
				qglScalef(r_flashlight_distance->value, radius, radius);

				float modelmatrix[16];
				qglGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
				qglPopMatrix();

				qglUniformMatrix4fvARB(prog.modelmatrix, 1, false, modelmatrix);

				qglDrawArrays(GL_TRIANGLES, 0, X_SEGMENTS * 6);

				qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
				qglDisableClientState(GL_VERTEX_ARRAY);
			}
		}
		else
		{
			vec3_t dist;
			VectorSubtract(r_refdef->vieworg, dl->origin, dist);

			if (VectorLength(dist) > dl->radius + 32)
			{
				vec3_t mins, maxs;
				for (int j = 0; j < 3; j++)
				{
					mins[j] = dl->origin[j] - dl->radius;
					maxs[j] = dl->origin[j] + dl->radius;
				}

				if (R_CullBox(mins, maxs))
					continue;

				dlight_program_t prog = { 0 };
				R_UseDLightProgram(DLIGHT_POINT_ENABLED | DLIGHT_VOLUME_ENABLED, &prog);
				qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
				qglUniform3fARB(prog.lightpos, dl->origin[0], dl->origin[1], dl->origin[2]);
				qglUniform3fARB(prog.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
				qglUniform1fARB(prog.lightradius, dl->radius);
				qglUniform1fARB(prog.lightambient, r_light_ambient->value);
				qglUniform1fARB(prog.lightdiffuse, r_light_diffuse->value);
				qglUniform1fARB(prog.lightspecular, r_light_specular->value);
				qglUniform1fARB(prog.lightspecularpow, r_light_specularpow->value);

				qglEnableClientState(GL_VERTEX_ARRAY);
				qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_sphere_vbo);
				qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_sphere_ebo);
				qglVertexPointer(3, GL_FLOAT, sizeof(float[3]), 0);

				qglPushMatrix();
				qglLoadIdentity();
				qglTranslatef(dl->origin[0], dl->origin[1], dl->origin[2]);
				qglScalef(dl->radius, dl->radius, dl->radius);

				float modelmatrix[16];
				qglGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
				qglPopMatrix();

				qglUniformMatrix4fvARB(prog.modelmatrix, 1, false, modelmatrix);

				qglDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);				

				qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
				qglDisableClientState(GL_VERTEX_ARRAY);
			}
			else
			{
				dlight_program_t prog = { 0 };
				R_UseDLightProgram(DLIGHT_POINT_ENABLED, &prog);
				qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
				qglUniform3fARB(prog.lightpos, dl->origin[0], dl->origin[1], dl->origin[2]);
				qglUniform3fARB(prog.lightcolor, (float)dl->color.r / 255.0f, (float)dl->color.g / 255.0f, (float)dl->color.b / 255.0f);
				qglUniform1fARB(prog.lightradius, dl->radius);
				qglUniform1fARB(prog.lightambient, r_light_ambient->value);
				qglUniform1fARB(prog.lightdiffuse, r_light_diffuse->value);
				qglUniform1fARB(prog.lightspecular, r_light_specular->value);
				qglUniform1fARB(prog.lightspecularpow, r_light_specularpow->value);

				GL_Begin2D();

				R_DrawHUDQuadFrustum(glwidth, glheight);

				GL_End2D();
			}
		}
	}

	GL_Begin2D();
	qglDisable(GL_BLEND);

	//Write GBuffer depth stencil into main framebuffer
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_GBufferFBO.s_hBackBufferFBO);
	qglBlitFramebufferEXT(0, 0, s_GBufferFBO.iWidth, s_GBufferFBO.iHeight,
		0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight,
		GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
		GL_NEAREST);

	//Shading pass
	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);	
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	int FinalProgramState = 0;

	if (r_fog_mode == GL_LINEAR)
		FinalProgramState |= DFINAL_LINEAR_FOG_ENABLED;

	if (r_ssr->value && r_ssr_control.enabled)
	{
		FinalProgramState |= DFINAL_SSR_ENABLED;

		if (r_ssr_control.adaptive_step)
			FinalProgramState |= DFINAL_SSR_ADAPTIVE_STEP_ENABLED;

		if (r_ssr_control.exponential_step)
			FinalProgramState |= DFINAL_SSR_EXPONENTIAL_STEP_ENABLED;

		if (r_ssr_control.binary_search)
			FinalProgramState |= DFINAL_SSR_BINARY_SEARCH_ENABLED;
	}

	//Setup final program
	R_UseDFinalProgram(FinalProgramState, NULL);

	//Texture unit 1 = (depth)
	GL_Bind(s_GBufferFBO.s_hBackBufferDepthTex);

	//Texture unit 2 = (linearized depth)
	qglActiveTextureARB(TEXTURE2_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, s_DepthLinearFBO.s_hBackBufferTex);

	R_DrawHUDQuadFrustum(glwidth, glheight);

	//Disable texture unit 2 (linearized depth)
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_2D);
	qglActiveTextureARB(TEXTURE1_SGIS);

	//Disable texture unit 1 (depth)
	GL_DisableMultitexture();

	//Disable texture unit 0 (GBuffer texture array)
	qglDisable(GL_TEXTURE_2D_ARRAY);
	qglEnable(GL_TEXTURE_2D);
	*currenttexture = -1;

	//Disable final program
	GL_UseProgram(0);

	//Restore 3D matrix
	GL_End2D();

	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);

	GL_PopDrawState();
}