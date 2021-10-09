#include "gl_local.h"
#include <sstream>

//renderer

int shadow_texture_depth = 0;
int shadow_texture_color = 0;
int shadow_texture_size = 0;

float shadow_projmatrix[3][16];
float shadow_mvmatrix[3][16];

cl_entity_t *shadow_visedicts[3][512] = { 0 };
int shadow_numvisedicts[3] = {0};

//cvar
cvar_t *r_shadow = NULL;
cvar_t *r_shadow_debug = NULL;
MapConVar *r_shadow_distfade = NULL;
MapConVar *r_shadow_lumfade = NULL;
MapConVar *r_shadow_angles = NULL;
MapConVar *r_shadow_color = NULL;
MapConVar *r_shadow_intensity = NULL;
MapConVar *r_shadow_high_distance = NULL;
MapConVar *r_shadow_high_scale = NULL;
MapConVar *r_shadow_medium_distance = NULL;
MapConVar *r_shadow_medium_scale = NULL;
MapConVar *r_shadow_low_distance = NULL;
MapConVar *r_shadow_low_scale = NULL;

void R_ShutdownShadow(void)
{
	if (shadow_texture_depth)
	{
		GL_DeleteTexture(shadow_texture_depth);
		shadow_texture_depth = 0;
	}
	if (shadow_texture_color)
	{
		GL_DeleteTexture(shadow_texture_color);
		shadow_texture_color = 0;
	}
}

void R_InitShadow(void)
{
	shadow_texture_size = min(gl_max_texture_size, 4096);
	shadow_texture_depth = GL_GenDepthTexture(shadow_texture_size, shadow_texture_size);
	shadow_texture_color = GL_GenTextureArrayColorFormat(shadow_texture_size, shadow_texture_size, 3, GL_RGBA16F);

	r_shadow = gEngfuncs.pfnRegisterVariable("r_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_debug = gEngfuncs.pfnRegisterVariable("r_shadow_debug", "0",  FCVAR_CLIENTDLL);
	r_shadow_distfade = R_RegisterMapCvar("r_shadow_distfade", "64 128", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 2);
	r_shadow_lumfade = R_RegisterMapCvar("r_shadow_lumfade", "80 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 2, MapConVar_Color255);
	r_shadow_angles = R_RegisterMapCvar("r_shadow_angles", "90 0 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 3);
	r_shadow_color = R_RegisterMapCvar("r_shadow_color", "0 0 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 3, MapConVar_Color255);
	r_shadow_intensity = R_RegisterMapCvar("r_shadow_intensity", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_distance = R_RegisterMapCvar("r_shadow_high_distance", "400", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_scale = R_RegisterMapCvar("r_shadow_high_scale", "4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_distance = R_RegisterMapCvar("r_shadow_medium_distance", "800", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_scale = R_RegisterMapCvar("r_shadow_medium_scale", "2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_distance = R_RegisterMapCvar("r_shadow_low_distance", "1200", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_scale = R_RegisterMapCvar("r_shadow_low_scale", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

bool R_ShouldRenderShadowScene(int level)
{
	if(r_draw_pass)
		return false;

	if (gRefFuncs.CL_IsDevOverviewMode())
		return false;

	if (!shadow_numvisedicts[0] && !shadow_numvisedicts[1] && !shadow_numvisedicts[2])
		return false;

	return r_shadow->value >= level;
}

bool R_ShouldCastShadow(cl_entity_t *ent)
{
	if(!ent)
		return false;

	if(!ent->model)
		return false;

	if (ent->curstate.rendermode != kRenderNormal)
		return false;

	if (ent->model->type == mod_studio)
	{
		if (ent->curstate.effects & EF_NODRAW)
			return false;

		//player model always render shadow
		if (ent->model == r_playermodel)
			return true;

		if (ent->index == 0)
			return false;

		if (ent->curstate.movetype == MOVETYPE_NONE)
			return false;

		return true;
	}

	return false;
}

void R_RenderShadowMap(void)
{
	vec3_t shadow_angles = { r_shadow_angles->GetValues()[0], r_shadow_angles->GetValues()[1] , r_shadow_angles->GetValues()[2] };

	float max_distance[3] = { r_shadow_high_distance->GetValue(), r_shadow_medium_distance->GetValue(), r_shadow_low_distance->GetValue() };

	float shadow_scales[3] = { r_shadow_high_scale->GetValue(), r_shadow_medium_scale->GetValue(), r_shadow_low_scale->GetValue() };

	for (int j = 0; j < *cl_numvisedicts; ++j)
	{
		if (R_ShouldCastShadow(cl_visedicts[j]))
		{
			vec3_t vec;
			VectorSubtract(cl_visedicts[j]->origin, (*r_refdef.vieworg), vec);
			float distance = VectorLength(vec);

			if (distance < max_distance[0])
			{
				if (shadow_numvisedicts[0] < 512)
				{
					shadow_visedicts[0][shadow_numvisedicts[0]] = cl_visedicts[j];
					shadow_numvisedicts[0]++;
				}
			}
			else if (distance < max_distance[1])
			{
				if (shadow_numvisedicts[1] < 512)
				{
					shadow_visedicts[1][shadow_numvisedicts[1]] = cl_visedicts[j];
					shadow_numvisedicts[1]++;
				}
			}
			else if (distance < max_distance[2])
			{
				if (shadow_numvisedicts[2] < 512)
				{
					shadow_visedicts[2][shadow_numvisedicts[2]] = cl_visedicts[j];
					shadow_numvisedicts[2]++;
				}
			}
		}
	}

	int total_numvisedicts = shadow_numvisedicts[0] + shadow_numvisedicts[1] + shadow_numvisedicts[2];

	if (total_numvisedicts)
	{
		static glprofile_t profile_RenderShadowMap;
		GL_BeginProfile(&profile_RenderShadowMap, "R_RenderShadowMap");

		glBindFramebuffer(GL_FRAMEBUFFER, s_ShadowFBO.s_hBackBufferFBO);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GEQUAL);

		glDepthMask(1);
		glColorMask(1, 1, 1, 1);

		for (int i = 0; i < 3; ++i)
		{
			if (!shadow_numvisedicts[i])
				continue;

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadow_texture_color, 0, i);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_texture_depth, 0);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			float texsize = (float)shadow_texture_size / shadow_scales[i];
			glOrtho(-texsize / 2, texsize / 2, -texsize / 2, texsize / 2, -4096, 4096);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glRotatef(-90, 1, 0, 0);
			glRotatef(90, 0, 0, 1);
			glRotatef(-shadow_angles[2], 1, 0, 0);
			glRotatef(-shadow_angles[0], 0, 1, 0);
			glRotatef(-shadow_angles[1], 0, 0, 1);
			glTranslatef(-(*r_refdef.vieworg)[0], -(*r_refdef.vieworg)[1], -(*r_refdef.vieworg)[2]);

			glGetFloatv(GL_PROJECTION_MATRIX, shadow_projmatrix[i]);
			glGetFloatv(GL_MODELVIEW_MATRIX, shadow_mvmatrix[i]);

			glViewport(0, 0, shadow_texture_size, shadow_texture_size);

			glClearDepth(0);
			glClearColor(-99999, -99999, -99999, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glNamedBufferSubData(r_wsurf.hSceneUBO, offsetof(scene_ubo_t, viewMatrix), sizeof(mat4), shadow_mvmatrix[i]);
			glNamedBufferSubData(r_wsurf.hSceneUBO, offsetof(scene_ubo_t, projMatrix), sizeof(mat4), shadow_projmatrix[i]);

			cl_entity_t *backup_curentity = (*currententity);

			for (int j = 0; j < shadow_numvisedicts[i]; ++j)
			{
				(*currententity) = shadow_visedicts[i][j];

				r_draw_shadowcaster = true;

				R_DrawCurrentEntity(false);

				r_draw_shadowcaster = false;
			}

			(*currententity) = backup_curentity;
		}

		glClearDepth(1);
		glDepthFunc(GL_LEQUAL);

		GL_EndProfile(&profile_RenderShadowMap);
	}
}