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

typedef enum
{
	ACT_RESET,
	ACT_IDLE,
	ACT_GUARD,
	ACT_WALK,
	ACT_RUN,
	ACT_FLY,
	ACT_SWIM,
	ACT_HOP,
	ACT_LEAP,
	ACT_FALL,
	ACT_LAND,
	ACT_STRAFE_LEFT,
	ACT_STRAFE_RIGHT,
	ACT_ROLL_LEFT,
	ACT_ROLL_RIGHT,
	ACT_TURN_LEFT,
	ACT_TURN_RIGHT,
	ACT_CROUCH,
	ACT_CROUCHIDLE,
	ACT_STAND,
	ACT_USE,
	ACT_SIGNAL1,
	ACT_SIGNAL2,
	ACT_SIGNAL3,
	ACT_TWITCH,
	ACT_COWER,
	ACT_SMALL_FLINCH,
	ACT_BIG_FLINCH,
	ACT_RANGE_ATTACK1,
	ACT_RANGE_ATTACK2,
	ACT_MELEE_ATTACK1,
	ACT_MELEE_ATTACK2,
	ACT_RELOAD,
	ACT_ARM,
	ACT_DISARM,
	ACT_EAT,
	ACT_DIESIMPLE,
	ACT_DIEBACKWARD,
	ACT_DIEFORWARD,
	ACT_DIEVIOLENT,
	ACT_BARNACLE_HIT,
	ACT_BARNACLE_PULL,
	ACT_BARNACLE_CHOMP,
	ACT_BARNACLE_CHEW,
	ACT_SLEEP,
	ACT_INSPECT_FLOOR,
	ACT_INSPECT_WALL,
	ACT_IDLE_ANGRY,
	ACT_WALK_HURT,
	ACT_RUN_HURT,
	ACT_HOVER,
	ACT_GLIDE,
	ACT_FLY_LEFT,
	ACT_FLY_RIGHT,
	ACT_DETECT_SCENT,
	ACT_SNIFF,
	ACT_BITE,
	ACT_THREAT_DISPLAY,
	ACT_FEAR_DISPLAY,
	ACT_EXCITED,
	ACT_SPECIAL_ATTACK1,
	ACT_SPECIAL_ATTACK2,
	ACT_COMBAT_IDLE,
	ACT_WALK_SCARED,
	ACT_RUN_SCARED,
	ACT_VICTORY_DANCE,
	ACT_DIE_HEADSHOT,
	ACT_DIE_CHESTSHOT,
	ACT_DIE_GUTSHOT,
	ACT_DIE_BACKSHOT,
	ACT_FLINCH_HEAD,
	ACT_FLINCH_CHEST,
	ACT_FLINCH_STOMACH,
	ACT_FLINCH_LEFTARM,
	ACT_FLINCH_RIGHTARM,
	ACT_FLINCH_LEFTLEG,
	ACT_FLINCH_RIGHTLEG,
	ACT_FLINCH_SMALL,
	ACT_FLINCH_LARGE,
	ACT_HOLDBOMB
}activity_e;

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

void R_AllocShadowTexture(shadow_texture_t *shadowtex, int size, bool bUseDepthArray)
{
	shadowtex->size = size;

	vec4_t depthBorderColor = { 1, 1, 1, 1 };
	
	shadowtex->depth = GL_GenShadowTexture(shadowtex->size, shadowtex->size, depthBorderColor);

	if (bUseDepthArray)
	{
		vec4_t borderColor = { -99999, -99999, -99999, 1};
		shadowtex->depth_array = GL_GenTextureArrayColorFormat(shadowtex->size, shadowtex->size, 3, GL_RGBA16F, false, borderColor);
	}
}

void R_FreeShadowTexture(shadow_texture_t *shadowtex)
{
	if (shadowtex->color)
	{
		GL_DeleteTexture(shadowtex->color);
		shadowtex->color = 0;
	}

	if (shadowtex->depth_array)
	{
		GL_DeleteTexture(shadowtex->depth_array);
		shadowtex->depth_array = 0;
	}

	if (shadowtex->depth)
	{
		GL_DeleteTexture(shadowtex->depth);
		shadowtex->depth = 0;
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
	if (r_draw_shadowcaster)
		return false;

	if (r_draw_reflectview)
		return false;

	if (R_IsRenderingPortal())
		return false;

	if (gRefFuncs.CL_IsDevOverviewMode())
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
		if (ent->model == r_playermodel)
			return true;

		//BulletPhysics ragdoll corpse
		if (ent->curstate.iuser4 == PhyCorpseFlag)
			return true;

		if (ent->index == 0)
			return false;

		if (ent->player && StudioGetSequenceActivityType(ent->model, &ent->curstate) == 1)
			return true;

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

void R_RenderShadowScene(void)
{
	GL_BeginProfile(&Profile_RenderShadowScene);

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
		GL_BindFrameBuffer(&s_ShadowFBO);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GEQUAL);

		glDepthMask(GL_TRUE);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		for (int i = 0; i < 3; ++i)
		{
			if (!shadow_numvisedicts[i])
				continue;

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, r_shadow_texture.depth_array, 0, i);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, r_shadow_texture.depth, 0);

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
			GL_ClearColorDepthStencil(vecClearColor, 0, STENCIL_MASK_SKY, STENCIL_MASK_ALL);

			if (glNamedBufferSubData)
			{
				glNamedBufferSubData(r_wsurf.hSceneUBO, offsetof(scene_ubo_t, viewMatrix), sizeof(mat4), shadow_mvmatrix[i]);
				glNamedBufferSubData(r_wsurf.hSceneUBO, offsetof(scene_ubo_t, projMatrix), sizeof(mat4), shadow_projmatrix[i]);
			}
			else
			{
				glBindBuffer(GL_UNIFORM_BUFFER, r_wsurf.hSceneUBO);
				glBufferSubData(GL_UNIFORM_BUFFER, offsetof(scene_ubo_t, viewMatrix), sizeof(mat4), shadow_mvmatrix[i]);
				glBufferSubData(GL_UNIFORM_BUFFER, offsetof(scene_ubo_t, projMatrix), sizeof(mat4), shadow_projmatrix[i]);
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}

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
	}

	GL_EndProfile(&Profile_RenderShadowScene);
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

					if (!shadowtex->depth)
					{
						R_AllocShadowTexture(shadowtex, 1024, false);
					}

					//if (!shadowtex->color && shadowtex->size)
					//{
					//	shadowtex->color = GL_GenTextureRGBA8(shadowtex->size, shadowtex->size);
					//}

					shadowtex->distance = distance;
					shadowtex->cone_angle = coneAngle;
					current_shadow_texture = shadowtex;

					GL_BindFrameBufferWithTextures(&s_ShadowFBO, 0, 0, shadowtex->depth, shadowtex->size, shadowtex->size);
					glDrawBuffer(GL_NONE);
#if 1
					glDisable(GL_BLEND);
					glDisable(GL_ALPHA_TEST);
					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LEQUAL);
					glDepthMask(GL_TRUE);

					glEnable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset(10, 10);

					GL_ClearDepthStencil(1.0f, STENCIL_MASK_SKY, STENCIL_MASK_ALL);

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

					R_PushRefDef();

					(*r_refdef.vieworg)[0] = origin[0];
					(*r_refdef.vieworg)[1] = origin[1];
					(*r_refdef.vieworg)[2] = origin[2];

					(*r_refdef.viewangles)[0] = angle[0];
					(*r_refdef.viewangles)[1] = angle[1];
					(*r_refdef.viewangles)[2] = angle[2];

					if (gEngfuncs.GetLocalPlayer()->model)
					{
						auto save_localplayer_model = gEngfuncs.GetLocalPlayer()->model;

						//This stops local player from being rendered
						gEngfuncs.GetLocalPlayer()->model = NULL;

						//R_RenderScene();

						gEngfuncs.GetLocalPlayer()->model = save_localplayer_model;
					}
					else
					{
						//R_RenderScene();
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
#endif
					r_draw_shadowcaster = false;

				}
			});

	}
}

void R_RenderShadowMap(void)
{
	shadow_numvisedicts[0] = 0;
	shadow_numvisedicts[1] = 0;
	shadow_numvisedicts[2] = 0;

	for (int i = 0; i < _ARRAYSIZE(cl_dlight_shadow_textures); ++i)
	{
		cl_dlight_shadow_textures[i].ready = false;
	}

	if (!r_shadow->value)
		return;

	R_RenderShadowScene();
	R_RenderShadowDynamicLights();
}