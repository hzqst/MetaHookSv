#include "gl_local.h"

colorVec *gWaterColor = NULL;
cshift_t *cshift_water = NULL;
int *gSkyTexNumber = NULL;
int *r_loading_skybox = NULL;

//TODO water fog?
#if 0
void EmitWaterPolys(msurface_t *fa, int direction)
{
	if (r_draw_reflectview)
		return;

	auto pSourcePalette = fa->texinfo->texture->pPal;
	gWaterColor->r = pSourcePalette[9];
	gWaterColor->g = pSourcePalette[10];
	gWaterColor->b = pSourcePalette[11];
	cshift_water->destcolor[0] = pSourcePalette[9];
	cshift_water->destcolor[1] = pSourcePalette[10];
	cshift_water->destcolor[2] = pSourcePalette[11];
	cshift_water->percent = pSourcePalette[12];

	if (gWaterColor->r == 0 && gWaterColor->g == 0 && gWaterColor->b == 0)
	{
		gWaterColor->r = pSourcePalette[0];
		gWaterColor->g = pSourcePalette[1];
		gWaterColor->b = pSourcePalette[2];
	}
}
#endif


void R_DrawSkyBox(void)
{
	if (CL_IsDevOverviewMode())
		return;

	if (r_draw_shadowcaster)
		return;

	if (!gSkyTexNumber[0])
		return;

	static glprofile_t profile_DrawSkyBox;
	GL_BeginProfile(&profile_DrawSkyBox, "R_DrawSkyBox");

	glDisable(GL_BLEND);
	glDepthMask(0);

	program_state_t WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_SKYBOX_ENABLED;

	if (bUseBindless)
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

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (bUseBindless)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_SKYBOX_SSBO, r_wsurf.hSkyboxSSBO);
	}

	if (WSurfProgramState & WSURF_BINDLESS_ENABLED)
	{
		glDrawArrays(GL_QUADS, 0, 4 * 6);
	}
	else
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			for (int i = 0; i < 6; ++i)
			{
				GL_Bind(gSkyTexNumber[i]);
				glDrawArrays(GL_QUADS, 4 * i, 4);
			}
		}
		else
		{
			const int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };
			for (int i = 0; i < 6; ++i)
			{
				GL_Bind(gSkyTexNumber[skytexorder[i]]);
				glDrawArrays(GL_QUADS, 4 * i, 4);
			}
		}
	}

	GL_UseProgram(0);

	glDepthMask(1);

	GL_EndProfile(&profile_DrawSkyBox);
}