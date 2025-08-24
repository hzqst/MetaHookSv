#include "gl_local.h"

colorVec *gWaterColor = NULL;
cshift_t *cshift_water = NULL;
int *gSkyTexNumber = NULL;
int *r_loading_skybox = NULL;

/*
	Purpose : Draw skybox
*/

void R_DrawSkyBox(void)
{
	if (CL_IsDevOverviewMode())
		return;

	if (R_IsRenderingShadowView())
		return;

	if (!g_WorldSurfaceRenderer.vSkyboxTextureId[0])
		return;

	glDisable(GL_BLEND);
	glDepthMask(GL_FALSE);

	entity_ubo_t EntityUBO;

	Matrix4x4_Transpose(EntityUBO.entityMatrix, r_entity_matrix);
	memcpy(EntityUBO.color, r_entity_color, sizeof(vec4));
	EntityUBO.scrollSpeed = 0;

	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_POINT_ENTITY_UBO, g_WorldSurfaceRenderer.hEntityUBO);

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hEntityUBO, 0, sizeof(EntityUBO), &EntityUBO);

	program_state_t WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_SKYBOX_ENABLED;

	if ((*filterMode) != 0)
	{
		WSurfProgramState |= WSURF_COLOR_FILTER_ENABLED;
	}

	if (R_IsRenderingWaterView())
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	if ((int)r_wsurf_sky_fog->value > 0)
	{
		if (!R_IsRenderingGBuffer())
		{
			if (R_IsRenderingFog())
			{
				if (r_fog_mode == GL_LINEAR)
				{
					WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP)
				{
					WSurfProgramState |= WSURF_EXP_FOG_ENABLED;
				}
				else if (r_fog_mode == GL_EXP2)
				{
					WSurfProgramState |= WSURF_EXP2_FOG_ENABLED;
				}

				if (!R_IsRenderingGammaBlending() && r_linear_fog_shift->value > 0)
				{
					WSurfProgramState |= WSURF_LINEAR_FOG_SHIFT_ENABLED;
				}
			}
		}
	}

	if (R_IsRenderingGammaBlending())
	{
		WSurfProgramState |= WSURF_GAMMA_BLEND_ENABLED;
	}

	if (R_IsRenderingGBuffer())
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	if (r_detailskytextures->value && g_WorldSurfaceRenderer.vSkyboxTextureId[6])
	{
		for (int i = 0; i < 6; ++i)
		{
			GL_Bind(g_WorldSurfaceRenderer.vSkyboxTextureId[6 + i]);
			glDrawArrays(GL_TRIANGLES, 6 * i, 6);
		}
	}
	else
	{
		for (int i = 0; i < 6; ++i)
		{
			GL_Bind(g_WorldSurfaceRenderer.vSkyboxTextureId[i]);
			glDrawArrays(GL_TRIANGLES, 6 * i, 6);
		}
	}

	GL_UseProgram(0);

	glDepthMask(GL_TRUE);
}