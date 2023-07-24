#include "gl_local.h"

colorVec *gWaterColor = NULL;
cshift_t *cshift_water = NULL;
int *gSkyTexNumber = NULL;
int *r_loading_skybox = NULL;

cvar_t* r_detailskytextures = NULL;

//TODO water fog?
#if 0
void EmitWaterPolys(msurface_t *fa, int direction)
{
	if (r_draw_reflectview)
		return;


}
#endif


void R_DrawSkyBox(void)
{
	if (CL_IsDevOverviewMode())
		return;

	if (r_draw_shadowcaster)
		return;

	if (!r_wsurf.vSkyboxTextureId[0])
		return;

	glDisable(GL_BLEND);
	glDepthMask(0);

	program_state_t WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_SKYBOX_ENABLED;

	if (bUseBindless && gl_bindless->value)
	{
		WSurfProgramState |= WSURF_BINDLESS_ENABLED;
	}

	if (*filterMode != 0)
	{
		WSurfProgramState |= WSURF_COLOR_FILTER_ENABLED;
	}

	if (r_draw_reflectview)
	{
		WSurfProgramState |= WSURF_CLIP_WATER_ENABLED;
	}
	else if (g_bPortalClipPlaneEnabled[0])
	{
		WSurfProgramState |= WSURF_CLIP_ENABLED;
	}

	if (r_wsurf_sky_fog->value)
	{
		if (!drawgbuffer && r_fog_mode == GL_LINEAR)
		{
			WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
		}
		else if (!drawgbuffer && r_fog_mode == GL_EXP)
		{
			WSurfProgramState |= WSURF_EXP_FOG_ENABLED;
		}
		else if (!drawgbuffer && r_fog_mode == GL_EXP2)
		{
			WSurfProgramState |= WSURF_EXP2_FOG_ENABLED;
		}
	}

	if (drawgbuffer)
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	if (WSurfProgramState & WSURF_BINDLESS_ENABLED)
	{
		if (r_detailskytextures->value && r_wsurf.vSkyboxTextureId[6])
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_SKYBOX_SSBO, r_wsurf.hDetailSkyboxSSBO);
		}
		else
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_SKYBOX_SSBO, r_wsurf.hSkyboxSSBO);
		}

		glDrawArrays(GL_QUADS, 0, 4 * 6);
	}
	else
	{
		if (r_detailskytextures->value && r_wsurf.vSkyboxTextureId[6])
		{
			for (int i = 0; i < 6; ++i)
			{
				GL_Bind(r_wsurf.vSkyboxTextureId[6 + i]);
				glDrawArrays(GL_QUADS, 4 * i, 4);
			}
		}
		else
		{
			for (int i = 0; i < 6; ++i)
			{
				GL_Bind(r_wsurf.vSkyboxTextureId[i]);
				glDrawArrays(GL_QUADS, 4 * i, 4);
			}
		}
	}

	GL_UseProgram(0);

	glDepthMask(1);
}