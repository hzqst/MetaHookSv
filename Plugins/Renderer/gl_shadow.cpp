#include "gl_local.h"
#include <sstream>

//renderer

shadow_texture_t r_shadow_texture = {0};

shadow_texture_t cl_dlight_shadow_textures[256] = { 0 };

shadow_texture_t *current_shadow_texture = NULL;

float shadow_projmatrix[3][16] = { 0 };
float shadow_mvmatrix[3][16] = { 0 };

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

int StudioGetSequenceActivityType(model_t *mod, entity_state_t* entstate)
{
	/*if (g_bIsSvenCoop)
	{
		if (entstate->scale != 0 && entstate->scale != 1.0f)
			return 0;
	}*/

	if (mod->type != mod_studio)
		return 0;

	auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return 0;

	int sequence = entstate->sequence;
	if (sequence >= studiohdr->numseq)
		return 0;

	auto pseqdesc = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex) + sequence;

	if (
		pseqdesc->activity == ACT_DIESIMPLE ||
		pseqdesc->activity == ACT_DIEBACKWARD ||
		pseqdesc->activity == ACT_DIEFORWARD ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIE_HEADSHOT ||
		pseqdesc->activity == ACT_DIE_CHESTSHOT ||
		pseqdesc->activity == ACT_DIE_GUTSHOT ||
		pseqdesc->activity == ACT_DIE_BACKSHOT
		)
	{
		return 1;
	}

	if (
		pseqdesc->activity == ACT_BARNACLE_HIT ||
		pseqdesc->activity == ACT_BARNACLE_PULL ||
		pseqdesc->activity == ACT_BARNACLE_CHOMP ||
		pseqdesc->activity == ACT_BARNACLE_CHEW
		)
	{
		return 2;
	}

	return 0;
}

void R_AllocShadowTexture(shadow_texture_t *shadowtex, int size, bool bUseColorArrayAsDepth)
{
	shadowtex->size = size;

	vec4_t depthBorderColor = { 1, 1, 1, 1 };
	
	shadowtex->depth_stencil = GL_GenShadowTexture(shadowtex->size, shadowtex->size, depthBorderColor);

	if (bUseColorArrayAsDepth)
	{
		vec4_t borderColor = { -99999, -99999, -99999, 1};
		shadowtex->color_array_as_depth = GL_GenTextureArrayColorFormat(shadowtex->size, shadowtex->size, 3, GL_RGBA16F, false, borderColor);
	}
}

void R_FreeShadowTexture(shadow_texture_t *shadowtex)
{
	if (shadowtex->color)
	{
		GL_DeleteTexture(shadowtex->color);
		shadowtex->color = 0;
	}

	if (shadowtex->color_array_as_depth)
	{
		GL_DeleteTexture(shadowtex->color_array_as_depth);
		shadowtex->color_array_as_depth = 0;
	}

	if (shadowtex->depth_stencil)
	{
		GL_DeleteTexture(shadowtex->depth_stencil);
		shadowtex->depth_stencil = 0;
	}

	shadowtex->size = 0;
}

void R_InitShadow(void)
{
	R_AllocShadowTexture(&r_shadow_texture, 4096, true);

	r_shadow = gEngfuncs.pfnRegisterVariable("r_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_debug = gEngfuncs.pfnRegisterVariable("r_shadow_debug", "0",  FCVAR_CLIENTDLL);
	r_shadow_distfade = R_RegisterMapCvar("r_shadow_distfade", "64 128", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 2);
	r_shadow_lumfade = R_RegisterMapCvar("r_shadow_lumfade", "80 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 2, ConVar_Color255);
	r_shadow_angles = R_RegisterMapCvar("r_shadow_angles", "90 0 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 3);
	r_shadow_color = R_RegisterMapCvar("r_shadow_color", "0 0 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, 3, ConVar_Color255);
	r_shadow_intensity = R_RegisterMapCvar("r_shadow_intensity", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_distance = R_RegisterMapCvar("r_shadow_high_distance", "400", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_scale = R_RegisterMapCvar("r_shadow_high_scale", "4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_distance = R_RegisterMapCvar("r_shadow_medium_distance", "800", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_scale = R_RegisterMapCvar("r_shadow_medium_scale", "2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_distance = R_RegisterMapCvar("r_shadow_low_distance", "1200", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_scale = R_RegisterMapCvar("r_shadow_low_scale", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

void R_ShutdownShadow(void)
{
	R_FreeShadowTexture(&r_shadow_texture);

	for (int i = 0; i < _ARRAYSIZE(cl_dlight_shadow_textures); ++i)
	{
		R_FreeShadowTexture(&cl_dlight_shadow_textures[i]);
	}
}

bool R_ShouldRenderShadow(void)
{
	if (R_IsRenderingShadowView())
		return false;

	if (R_IsRenderingWaterView())
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (gPrivateFuncs.CL_IsDevOverviewMode())
		return false;

	return r_shadow->value ? true : false;
}

bool R_ShouldRenderShadowScene(void)
{
	if (!shadow_numvisedicts[0] && !shadow_numvisedicts[1] && !shadow_numvisedicts[2])
		return false;

	return R_ShouldRenderShadow();
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
		if (!strcmp(ent->model->name, "models/player.mdl"))
			return true;

		if (ent->player)
			return true;

		//BulletPhysics ragdoll corpse
		if (ent->curstate.iuser4 == PhyCorpseFlag)
			return true;

		if (ent->index == 0)
			return false;

		//if (ent->player && StudioGetSequenceActivityType(ent->model, &ent->curstate) == 1)
		//	return true;

		if (ent->curstate.movetype == MOVETYPE_NONE && ent->curstate.solid == SOLID_NOT)
			return false;

		//if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (ent->curstate.effects & EF_NOSHADOW)
				return false;
		}

		return true;
	}

	return false;
}

/*

	Purpose : Rendering textures for shadow mapping

*/
void R_RenderShadowScene(void)
{
	vec3_t shadow_angles = { r_shadow_angles->GetValues()[0], r_shadow_angles->GetValues()[1] , r_shadow_angles->GetValues()[2] };

	float max_distance[3] = { r_shadow_high_distance->GetValue(), r_shadow_medium_distance->GetValue(), r_shadow_low_distance->GetValue() };

	float shadow_scales[3] = { r_shadow_high_scale->GetValue(), r_shadow_medium_scale->GetValue(), r_shadow_low_scale->GetValue() };

	for (int j = 0; j < (*cl_numvisedicts); ++j)
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

	if (R_ShouldRenderShadowScene())
	{
		r_draw_shadowscene = true;

		GL_BindFrameBuffer(&s_ShadowFBO);

		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GEQUAL);

		glDepthMask(GL_TRUE);

		for (int i = 0; i < 3; ++i)
		{
			if (!shadow_numvisedicts[i])
				continue;

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, r_shadow_texture.color_array_as_depth, 0, i);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, r_shadow_texture.depth_stencil, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			float texsize = (float)r_shadow_texture.size / shadow_scales[i];
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

			glViewport(0, 0, r_shadow_texture.size, r_shadow_texture.size);

			vec4_t vecClearColor = { -99999, -99999, -99999, 1 };

			GL_ClearColorDepthStencil(vecClearColor, 0.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

			GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hSceneUBO, offsetof(scene_ubo_t, viewMatrix), sizeof(mat4), shadow_mvmatrix[i]);
			GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hSceneUBO, offsetof(scene_ubo_t, projMatrix), sizeof(mat4), shadow_projmatrix[i]);

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

		r_draw_shadowscene = false;
	}
}

void R_RenderShadowDynamicLights(void)
{
	if (!r_light_dynamic->value)
		return;

	if (R_ShouldRenderShadow())
	{
		R_IterateDynamicLights([](
			float radius, vec3_t origin, vec3_t color,
			float ambient, float diffuse, float specular, float specularpow,
			shadow_texture_t *shadowtex, bool bVolume)
		{
				shadowtex->ready = false;
		},
		[](
			float distance, float radius,
			float coneAngle, float coneCosAngle, float coneSinAngle, float coneTanAngle,
			vec3_t origin, vec3_t angle, vec3_t vforward, vec3_t vright, vec3_t vup,
			vec3_t color, float ambient, float diffuse, float specular, float specularpow, shadow_texture_t *shadowtex, bool bVolume, bool bIsFromLocalPlayer)
			{
				shadowtex->ready = false;

				if (bIsFromLocalPlayer)
				{
					r_draw_shadowcaster = true;

					if (!shadowtex->depth_stencil)
					{
						R_AllocShadowTexture(shadowtex, 1024, false);
					}

					//Just for test
					//if (!shadowtex->color && shadowtex->size)
					//{
					//	shadowtex->color = GL_GenTextureRGBA8(shadowtex->size, shadowtex->size);
					//}

					shadowtex->distance = distance;
					shadowtex->cone_angle = coneAngle;
					current_shadow_texture = shadowtex;

					GL_BindFrameBufferWithTextures(&s_ShadowFBO, shadowtex->color, 0, shadowtex->depth_stencil, shadowtex->size, shadowtex->size);
					glDrawBuffer(GL_NONE);

					glDisable(GL_BLEND);
					glDisable(GL_ALPHA_TEST);
					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LEQUAL);
					glDepthMask(GL_TRUE);

					glEnable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset(10, 10);

					GL_ClearDepthStencil(1.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

					R_PushRefDef();

					(*r_refdef.vieworg)[0] = origin[0];
					(*r_refdef.vieworg)[1] = origin[1];
					(*r_refdef.vieworg)[2] = origin[2];

					(*r_refdef.viewangles)[0] = angle[0];
					(*r_refdef.viewangles)[1] = angle[1];
					(*r_refdef.viewangles)[2] = angle[2];

					auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

					if (pLocalPlayer->model)
					{
						auto save_localplayer_model = pLocalPlayer->model;

						//This stops local player from being rendered
						pLocalPlayer->model = NULL;

						R_RenderScene();

						pLocalPlayer->model = save_localplayer_model;
					}
					else
					{
						R_RenderScene();
					}

					const float bias[16] = {
						0.5f, 0.0f, 0.0f, 0.0f,
						0.0f, 0.5f, 0.0f, 0.0f,
						0.0f, 0.0f, 0.5f, 0.0f,
						0.5f, 0.5f, 0.5f, 1.0f
					};

					glMatrixMode(GL_TEXTURE);
					glPushMatrix();
					glLoadIdentity();
					glLoadMatrixf(bias);
					glMultMatrixf(r_projection_matrix);
					glMultMatrixf(r_world_matrix);
					glGetFloatv(GL_TEXTURE_MATRIX, shadowtex->matrix);
					glPopMatrix();
					glMatrixMode(GL_MODELVIEW);

					glDisable(GL_POLYGON_OFFSET_FILL);
					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
					glDrawBuffer(GL_COLOR_ATTACHMENT0);

					R_PopRefDef();

					shadowtex->ready = true;

					r_draw_shadowcaster = false;

				}
			});

	}
}

/*

	Purpose : Clear shadow related vars which might be accessed later by deferred lighting pass.

*/

void R_RenderShadowMap_Start(void)
{
	shadow_numvisedicts[0] = 0;
	shadow_numvisedicts[1] = 0;
	shadow_numvisedicts[2] = 0;

	for (int i = 0; i < _ARRAYSIZE(cl_dlight_shadow_textures); ++i)
	{
		cl_dlight_shadow_textures[i].ready = false;
	}
}

/*

	Purpose : Rendering textures for shadow mapping

*/

void R_RenderShadowMap(void)
{
	R_RenderShadowMap_Start();

	if (!r_shadow->value)
		return;

	R_RenderShadowScene();
	R_RenderShadowDynamicLights();
}