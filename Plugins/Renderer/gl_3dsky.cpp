#include "gl_local.h"

qboolean draw3dsky;
vec3_t _3dsky_view;
float _3dsky_mvmatrix[16];

r_3dsky_parm_t r_3dsky_parm;

cvar_t *r_3dsky;
cvar_t *r_3dsky_debug;

void R_Clear3DSky(void)
{
	draw3dsky = false;
	r_3dsky_parm.enable = false;
	r_3dsky_parm.scale = 16;
	VectorClear(r_3dsky_parm.camera);
	VectorClear(r_3dsky_parm.center);
	VectorClear(r_3dsky_parm.mins);
	VectorClear(r_3dsky_parm.maxs);
}

void R_Init3DSky(void)
{
	R_Clear3DSky();

	r_3dsky = gEngfuncs.pfnRegisterVariable("r_3dsky", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_3dsky_debug = gEngfuncs.pfnRegisterVariable("r_3dsky_debug", "0", FCVAR_CLIENTDLL);
}

void R_SetupGL_3DSky(void)
{
	//projection
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();

	if(r_params.movevars)
		MYgluPerspective(yfov, screenaspect, 4, r_params.movevars->zmax);
	else
		MYgluPerspective(yfov, screenaspect, 4, 4096);

	//modelview
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglRotatef(-90, 1, 0, 0);
	qglRotatef(90, 0, 0, 1);
	qglRotatef(-r_refdef->viewangles[2], 1, 0, 0);
	qglRotatef(-r_refdef->viewangles[0], 0, 1, 0);
	qglRotatef(-r_refdef->viewangles[1], 0, 0, 1);

	qglScalef(r_3dsky_parm.scale, r_3dsky_parm.scale, r_3dsky_parm.scale);

	qglTranslatef(-r_refdef->vieworg[0], -r_refdef->vieworg[1], -r_refdef->vieworg[2]);

	qglGetFloatv(GL_MODELVIEW_MATRIX, _3dsky_mvmatrix);

	//render states
	qglCullFace(GL_FRONT);

	if (gl_cull->value)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
}

void R_ViewOriginFor3DSky(float *org)
{
	vec3_t r_sub;
	VectorSubtract(r_refdef->vieworg, r_3dsky_parm.center, r_sub);
	VectorMA(r_3dsky_parm.camera, 1 / r_3dsky_parm.scale, r_sub, org);

	//(org - center) / scale + camera
}

void R_Render3DSky(void)
{
	draw3dsky = true;

	R_PushRefDef();

	VectorCopy(_3dsky_view, r_refdef->vieworg);

	R_UpdateRefDef();

	++(*r_framecount);
	*r_oldviewleaf = *r_viewleaf;
	*r_viewleaf = Mod_PointInLeaf(r_origin, r_worldmodel);

	R_SetFrustum();
	R_SetupGL_3DSky();

	R_MarkLeaves();

	qglViewport(0, 0, glwidth, glheight);

	R_ClearSkyBox();

	R_PopRefDef();

	draw3dsky = false;
}

void R_Draw3DSkyEntities(void)
{
	int i, j, numvisedicts;

	numvisedicts = *cl_numvisedicts;

	(*numTransObjs) = 0;

	for (i = 0; i < numvisedicts; i++)
	{
		(*currententity) = cl_visedicts[i];

		if( (*currententity)->curstate.entityType != ET_3DSKYENTITY )//if( !((*currententity)->curstate.effects & EF_3DSKY) )
			continue;

		if ((*currententity)->curstate.rendermode != kRenderNormal)
		{
			R_AddTEntity(*currententity);
			continue;
		}

		switch ((*currententity)->model->type)
		{
			case mod_brush:
			{
				R_DrawBrushModel(*currententity);
				break;
			}

			case mod_studio:
			{
				if ((*currententity)->player)
				{
					(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS, IEngineStudio.GetPlayerState((*currententity)->index) );
				}
				else
				{
					if ((*currententity)->curstate.movetype == MOVETYPE_FOLLOW)
					{
						for (j = 0; j < numvisedicts; j++)
						{
							if (cl_visedicts[j]->index == (*currententity)->curstate.aiment)
							{
								*currententity = cl_visedicts[j];

								if ((*currententity)->player)
								{
									(*gpStudioInterface)->StudioDrawPlayer(0, IEngineStudio.GetPlayerState((*currententity)->index));
								}
								else
								{
									(*gpStudioInterface)->StudioDrawModel(0);
								}

								*currententity = cl_visedicts[i];
								break;
							}
						}
					}

					(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS);
				}
				break;
			}

			default:
			{
				break;
			}
		}
	}

	*r_blend = 1.0;

	for (i = 0; i < numvisedicts; i++)
	{
		*currententity = cl_visedicts[i];

		if ((*currententity)->curstate.rendermode != kRenderNormal)
		{
			continue;
		}

		switch ((*currententity)->model->type)
		{
			case mod_sprite:
			{
				if ((*currententity)->curstate.body)
				{
					float *pAttachment = R_GetAttachmentPoint((*currententity)->curstate.skin, (*currententity)->curstate.body);
					VectorCopy(pAttachment, r_entorigin);
				}
				else
				{
					VectorCopy((*currententity)->origin, r_entorigin);
				}

				R_DrawSpriteModel(*currententity);
				break;
			}
		}
	}

	//draw trans objects
	//R_SortTEntities();

	for (i = 0; i < (*numTransObjs); i++)
	{
		(*currententity) = (*transObjects)[i].pEnt;

		if( (*currententity)->curstate.entityType != ET_3DSKYENTITY )//if( !((*currententity)->curstate.effects & EF_3DSKY) )
			continue;

		qglDisable(GL_FOG);
		
		*r_blend = gRefFuncs.CL_FxBlend(*currententity);

		if (*r_blend <= 0)
			continue;

		*r_blend = (*r_blend) / 255.0;

		if ((*currententity)->curstate.rendermode == kRenderGlow && (*currententity)->model->type != mod_sprite)
			gEngfuncs.Con_DPrintf("Non-sprite set to glow!\n");

		switch ((*currententity)->model->type)
		{
			case mod_brush:
			{
				if (g_bUserFogOn && *g_bUserFogOn)
				{
					if ((*currententity)->curstate.rendermode != kRenderGlow && (*currententity)->curstate.rendermode != kRenderTransAdd)
						qglEnable(GL_FOG);
				}

				R_DrawBrushModel(*currententity);
				break;
			}

			case mod_sprite:
			{
				if ((*currententity)->curstate.body)
				{
					float *pAttachment = R_GetAttachmentPoint((*currententity)->curstate.skin, (*currententity)->curstate.body);
					VectorCopy(pAttachment, r_entorigin);
				}
				else
				{
					VectorCopy((*currententity)->origin, r_entorigin);
				}

				if ((*currententity)->curstate.rendermode == kRenderGlow)
					(*r_blend) *= gRefFuncs.GlowBlend(*currententity);

				if ((*r_blend) != 0)
					R_DrawSpriteModel(*currententity);

				break;
			}

			case mod_studio:
			{
				R_Setup3DSkyModel();
				if ((*currententity)->player)
				{
					(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS, IEngineStudio.GetPlayerState((*currententity)->index));
				}
				else
				{
					if ((*currententity)->curstate.movetype == MOVETYPE_FOLLOW)
					{
						for (j = 0; j < numvisedicts; j++)
						{
							if ((*transObjects)[j].pEnt->index == (*currententity)->curstate.aiment)
							{
								*currententity = (*transObjects)[j].pEnt;

								if ((*currententity)->player)
								{
									(*gpStudioInterface)->StudioDrawPlayer(0, IEngineStudio.GetPlayerState((*currententity)->index));
								}
								else
								{
									(*gpStudioInterface)->StudioDrawModel(0);
								}

								*currententity = (*transObjects)[i].pEnt;
								break;
							}
						}
					}

					(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS);
				}
				R_Finish3DSkyModel();
				break;
			}

			default:
			{
				break;
			}
		}
	}
	(*numTransObjs) = 0;
}

void R_Add3DSkyEntity(cl_entity_t *ent)
{
	//ent->curstate.effects |= EF_3DSKY;
	ent->curstate.entityType = ET_3DSKYENTITY;
}

void R_Setup3DSkyModel(void)
{
	//if((*currententity)->curstate.effects & EF_3DSKY)
	if((*currententity)->curstate.entityType == ET_3DSKYENTITY)
	{
		if(!drawreflect && !drawrefract)
		{
			qglMatrixMode(GL_MODELVIEW);
			qglPushMatrix();
			qglLoadMatrixf(_3dsky_mvmatrix);

			R_PushRefDef();
			VectorCopy(_3dsky_view, r_refdef->vieworg);
			VectorCopy(_3dsky_view, r_origin);

			draw3dsky = true;
		}
	}
}

void R_Finish3DSkyModel(void)
{
	if((*currententity)->curstate.entityType == ET_3DSKYENTITY)//if((*currententity)->curstate.effects & EF_3DSKY)
	{
		if(!drawreflect && !drawrefract)
		{
			qglMatrixMode(GL_MODELVIEW);
			qglPopMatrix();

			R_PopRefDef();
			VectorCopy(r_refdef->vieworg, r_origin);

			draw3dsky = false;
		}
	}
}