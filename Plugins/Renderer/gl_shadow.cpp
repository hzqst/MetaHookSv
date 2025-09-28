#include "gl_local.h"
#include <sstream>
#include <math.h>

//renderer

shadow_texture_t cl_dlight_shadow_textures[256] = { 0 };

shadow_texture_t *current_shadow_texture = NULL;

//cvar
cvar_t *r_shadow = NULL;

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

/*
	Purpose: allocate a depth stencil texture in "shadowtex->depth_stencil" with width/height of "size"
*/
void R_AllocShadowTexture(shadow_texture_t *shadowtex, int size, bool bUseColorArrayAsDepth)
{
	shadowtex->size = size;

	vec4_t depthBorderColor = { 1, 1, 1, 1 };
	
	shadowtex->depth_stencil = GL_GenShadowTexture(shadowtex->size, shadowtex->size, depthBorderColor, true);
}

void R_FreeShadowTexture(shadow_texture_t *shadowtex)
{
	if (shadowtex->depth_stencil)
	{
		gEngfuncs.Con_DPrintf("R_FreeShadowTexture: delete depth_stencil [%d].\n", shadowtex->depth_stencil);
		GL_DeleteTexture(shadowtex->depth_stencil);
		shadowtex->depth_stencil = 0;
	}

	shadowtex->size = 0;
}

void R_InitShadow(void)
{
	r_shadow = gEngfuncs.pfnRegisterVariable("r_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

void R_ShutdownShadow(void)
{
	for (int i = 0; i < _countof(cl_dlight_shadow_textures); ++i)
	{
		R_FreeShadowTexture(&cl_dlight_shadow_textures[i]);
	}
	//TODO: g_DynamicLights ?
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

		if (ent->curstate.movetype == MOVETYPE_NONE && ent->curstate.solid == SOLID_NOT)
			return false;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (ent->curstate.effects & EF_NOSHADOW)
				return false;
		}

		return true;
	}

	return false;
}

void R_SetupShadowMatrix(float out[4][4], const float worldMatrix[4][4], const float projMatrix[4][4])
{
	/*
	Counterpart of following matrix:
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
		glMultMatrixf(offsetMatrix); // CSM offset matrix
		glMultMatrixf(r_projection_matrix);
		glMultMatrixf(r_world_matrix);
		glGetFloatv(GL_TEXTURE_MATRIX, (float *)shadowmatrix);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	*/

	const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
	};

	// First multiply projection matrix with world matrix
	float projWorldMatrix[4][4];
	Matrix4x4_Multiply(projWorldMatrix, worldMatrix, projMatrix);

	// Then multiply bias matrix with the result
	Matrix4x4_Multiply(out, projWorldMatrix, (const float (*)[4])bias);
}

void R_RenderShadowmapForDynamicLights(void)
{
	if (!r_deferred_lighting->value)
		return;

	if (R_ShouldRenderShadow())
	{
		GL_BeginDebugGroup("R_RenderShadowmapForDynamicLights");

		const auto PointLightCallback = [](PointLightCallbackArgs *args, void *context)
		{
			if (args->shadowtex)
			{
				args->shadowtex->ready = false;
			}
		};

		const auto SpotLightCallback = [](SpotLightCallbackArgs *args, void *context)
		{
				if (args->shadowtex)
				{
					args->shadowtex->ready = false;

					if (args->bIsFromLocalPlayer)
					{
						r_draw_shadowcaster = true;

						if (!args->shadowtex->depth_stencil)
						{
							R_AllocShadowTexture(args->shadowtex, 1024, false);
						}

						current_shadow_texture = args->shadowtex;

						GL_BeginDebugGroup("R_RenderShadowDynamicLights - DrawSpotlightShadowPass");

						GL_BindFrameBufferWithTextures(&s_ShadowFBO, 0, 0, args->shadowtex->depth_stencil, args->shadowtex->size, args->shadowtex->size);

						current_shadow_texture->viewport[0] = 0;
						current_shadow_texture->viewport[1] = 0;
						current_shadow_texture->viewport[2] = args->shadowtex->size;
						current_shadow_texture->viewport[3] = args->shadowtex->size;

						glEnable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(10, 10);

						GL_ClearDepthStencil(1.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

						glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

						R_PushRefDef();

						(*r_refdef.vieworg)[0] = args->origin[0];
						(*r_refdef.vieworg)[1] = args->origin[1];
						(*r_refdef.vieworg)[2] = args->origin[2];

						(*r_refdef.viewangles)[0] = args->angle[0];
						(*r_refdef.viewangles)[1] = args->angle[1];
						(*r_refdef.viewangles)[2] = args->angle[2];

						R_LoadIdentityForWorldMatrix();
						R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));

						float cone_fov = args->coneAngle * 2 * 360 / (M_PI * 2);
						R_LoadIdentityForProjectionMatrix();
						R_SetupPerspective(cone_fov, cone_fov, gl_nearplane->value, args->distance);

						auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
						auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

						Matrix4x4_Copy(current_shadow_texture->worldmatrix, (*worldMatrix));
						Matrix4x4_Copy(current_shadow_texture->projmatrix, (*projMatrix));

						R_SetupShadowMatrix(current_shadow_texture->shadowmatrix, (*worldMatrix), (*projMatrix));

						auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

						if (pLocalPlayer->model && args->bIsFromLocalPlayer)
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

						glDisable(GL_POLYGON_OFFSET_FILL);
						glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

						R_PopRefDef();

						args->shadowtex->ready = true;

						r_draw_shadowcaster = false;

						GL_EndDebugGroup();
					}
				}
		};

		const auto DirectionalLightCallback = [](DirectionalLightCallbackArgs* args, void* context)
		{
			if (args->shadowtex)
			{
				args->shadowtex->ready = false;

				r_draw_shadowcaster = true;
				r_draw_shadowscene = true;

				// Allocate 4096x4096 CSM texture if not already allocated
				if (!args->shadowtex->depth_stencil)
				{
					R_AllocShadowTexture(args->shadowtex, CSM_RESOLUTION, false);
				}

				// Calculate cascade distances based on camera frustum
				// These could be configurable via cvars in the future
				float nearPlane = r_znear;  // Should match r_nearclip or similar, 4.0 by default
				float farPlane = r_zfar; // Should match r_farclip or similar, 8192.0 by default
				float cascadeDistances[4];

				// Use logarithmic distribution for cascades
				for (int i = 0; i < 4; ++i)
				{
					float ratio = (float)(i + 1) / 4.0f;
					cascadeDistances[i] = nearPlane * pow(farPlane / nearPlane, ratio);
				}

				// Copy to args for shader use
				for (int i = 0; i < 4; ++i)
				{
					args->csmDistances[i] = cascadeDistances[i];
				}

				current_shadow_texture = args->shadowtex;

				current_shadow_texture->viewport[0] = 0;
				current_shadow_texture->viewport[1] = 0;
				current_shadow_texture->viewport[2] = args->shadowtex->size;
				current_shadow_texture->viewport[3] = args->shadowtex->size;

				GL_BindFrameBufferWithTextures(&s_ShadowFBO, 0, 0, current_shadow_texture->depth_stencil, current_shadow_texture->size, current_shadow_texture->size);

				GL_ClearDepthStencil(1.0f, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

				GL_BeginDebugGroup("R_RenderShadowDynamicLights - DrawDirectionalLightCSM");

				R_PushRefDef();

				// Render each cascade
				for (int cascade = 0; cascade < CSM_LEVELS; ++cascade)
				{
					GL_BeginDebugGroupFormat("CSM DrawCascade %d", cascade);

					// Set up scissor test for this cascade region
					// CSM layout: [0,1]
					//             [2,3]
					int scissorX = (cascade % 2) * (CSM_RESOLUTION / 2);  // 0 or 2048
					int scissorY = (cascade / 2) * (CSM_RESOLUTION / 2);  // 0 or 2048
					int scissorWidth = (CSM_RESOLUTION / 2);
					int scissorHeight = (CSM_RESOLUTION / 2);

					glEnable(GL_SCISSOR_TEST);
					glScissor(scissorX, scissorY, scissorWidth, scissorHeight);

					// Update refdef for shadow rendering
					VectorCopy(args->origin, (*r_refdef.vieworg));
					VectorCopy(args->angle, (*r_refdef.viewangles));

					R_LoadIdentityForWorldMatrix();
					R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));

					// Set up orthographic projection for this cascade
					float orthoSize = args->size * (1.0f + cascade * 2.0f) / (1.0f + (4-1) * 2.0f); // Increase size for further cascades

					R_LoadIdentityForProjectionMatrix();
					R_SetupOrthoProjectionMatrix(-orthoSize / 2, orthoSize / 2, -orthoSize / 2, orthoSize / 2, 2048, -2048, true);

					auto worldMatrix = (float (*)[4][4])R_GetWorldMatrix();
					auto projMatrix = (float (*)[4][4])R_GetProjectionMatrix();

					float csmOffsetMatrix[4][4];
					Matrix4x4_CreateCSMOffset(csmOffsetMatrix, cascade);

					mat4 tempProjmatrix;
					Matrix4x4_Multiply(tempProjmatrix, (*projMatrix), csmOffsetMatrix);

					Matrix4x4_Copy(current_shadow_texture->projmatrix, tempProjmatrix);
					Matrix4x4_Copy(current_shadow_texture->worldmatrix, (*worldMatrix));

					R_SetupShadowMatrix(current_shadow_texture->shadowmatrix, current_shadow_texture->worldmatrix, current_shadow_texture->projmatrix);

					Matrix4x4_Copy(args->csmMatrices[cascade], current_shadow_texture->shadowmatrix);

					// Render scene from light's perspective
					R_RenderScene();

					// Disable scissor test after rendering this cascade
					glDisable(GL_SCISSOR_TEST);

					GL_EndDebugGroup();
				}

				R_PopRefDef();

				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

				args->shadowtex->ready = true;

				r_draw_shadowcaster = false;
				r_draw_shadowscene = false;

				GL_EndDebugGroup();
			}
		};

		R_IterateDynamicLights(PointLightCallback, SpotLightCallback, DirectionalLightCallback, nullptr);

		GL_EndDebugGroup();
	}
}

/*

	Purpose : Clear shadow related vars which might be accessed later by deferred lighting pass.

*/

void R_RenderShadowMap_Start(void)
{
	for (int i = 0; i < _countof(cl_dlight_shadow_textures); ++i)
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

	R_RenderShadowmapForDynamicLights();
}