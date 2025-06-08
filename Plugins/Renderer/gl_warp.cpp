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

	program_state_t WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_SKYBOX_ENABLED;

	if ((*filterMode) != 0)
	{
		WSurfProgramState |= WSURF_COLOR_FILTER_ENABLED;
	}

	if (R_IsRenderingReflectView())
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	if (r_wsurf_sky_fog->value)
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

	if (r_draw_opaque)
	{
		GL_BeginStencilWrite(STENCIL_MASK_WORLD | STENCIL_MASK_NO_SHADOW, STENCIL_MASK_ALL);
	}
	else
	{
		GL_BeginStencilWrite(STENCIL_MASK_NO_SHADOW, STENCIL_MASK_NO_SHADOW);
	}

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	if (r_detailskytextures->value && g_WorldSurfaceRenderer.vSkyboxTextureId[6])
	{
		for (int i = 0; i < 6; ++i)
		{
			GL_Bind(g_WorldSurfaceRenderer.vSkyboxTextureId[6 + i]);
			glDrawArrays(GL_QUADS, 4 * i, 4);
		}
	}
	else
	{
		for (int i = 0; i < 6; ++i)
		{
			GL_Bind(g_WorldSurfaceRenderer.vSkyboxTextureId[i]);
			glDrawArrays(GL_QUADS, 4 * i, 4);
		}
	}

	GL_UseProgram(0);

	GL_EndStencil();

	glDepthMask(GL_TRUE);
}