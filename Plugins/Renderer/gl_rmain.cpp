#include "gl_local.h"
#include "pm_defs.h"

ref_funcs_t gRefFuncs;

refdef_t *r_refdef;

ref_params_t r_params;

float gldepthmin, gldepthmax;

cl_entity_t *r_worldentity;
model_t *r_worldmodel;

RECT *window_rect;

float *videowindowaspect;
float *windowvideoaspect;
float videowindowaspect_old;
float windowvideoaspect_old;

int *cl_numvisedicts;
cl_entity_t **cl_visedicts;
cl_entity_t **currententity;
int *numTransObjs;
int *maxTransObjs;
transObjRef **transObjects;

float scr_fov_value;
GLint r_viewport[4];

mplane_t custom_frustum[4];
mplane_t *frustum;
mleaf_t **r_viewleaf, **r_oldviewleaf;

float yfov;

vec_t *vup;
vec_t *vpn;
vec_t *vright;
vec_t *r_origin;
vec_t *modelorg;
vec_t *r_entorigin;
float *r_world_matrix;
float *r_projection_matrix;

int *g_bUserFogOn;
float *g_UserFogColor;
float *g_UserFogDensity;
float *g_UserFogStart;
float *g_UserFogEnd;

int *r_framecount;
int *r_visframecount;

frame_t *cl_frames;
int size_of_frame = sizeof(frame_t);
int *cl_parsecount;
int *cl_waterlevel;
double *cl_time;
double *cl_oldtime;
int *envmap;
int *cl_stats;
float *cl_weaponstarttime;
int *cl_weaponsequence;
int *cl_light_level;
int *c_alias_polys;
int *c_brush_polys;

int gl_max_texture_size = 0;
float gl_max_ansio = 0;
GLuint gl_color_format = 0;
int gl_msaa_samples = 0;
cvar_t *r_msaa = NULL;

int *gl_msaa_fbo = 0;
int *gl_backbuffer_fbo = 0;
int *gl_mtexable = 0;
qboolean *mtexenabled = 0;
qboolean g_SvEngine_DrawPortalView = 0;

qboolean gl_framebuffer_object = false;
qboolean gl_shader_support = false;
qboolean gl_program_support = false;
qboolean gl_msaa_support = false;
qboolean gl_blit_support = false;
qboolean gl_float_buffer_support = false;
qboolean gl_s3tc_compression_support = false;

float r_identity_matrix[4][4] = {
	{1.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
};

float r_rotate_entity_matrix[4][4];

bool r_rotate_entity = false;

int r_draw_pass = 0;

int glx = 0;
int gly = 0;
int glwidth = 0;
int glheight = 0;

FBO_Container_t s_MSAAFBO;
FBO_Container_t s_GBufferFBO;
FBO_Container_t s_BackBufferFBO, s_BackBufferFBO2;
FBO_Container_t s_DownSampleFBO[DOWNSAMPLE_BUFFERS];
FBO_Container_t s_LuminFBO[LUMIN_BUFFERS];
FBO_Container_t s_Lumin1x1FBO[LUMIN1x1_BUFFERS];
FBO_Container_t s_BrightPassFBO;
FBO_Container_t s_BlurPassFBO[BLUR_BUFFERS][2];
FBO_Container_t s_BrightAccumFBO;
FBO_Container_t s_ToneMapFBO;
FBO_Container_t s_DepthLinearFBO;
FBO_Container_t s_HBAOCalcFBO;
FBO_Container_t s_ShadowFBO;
FBO_Container_t s_WaterFBO;

qboolean bNoStretchAspect = false;
qboolean bDoMSAA = true;

cvar_t *ati_subdiv = NULL;
cvar_t *ati_npatch = NULL;

cvar_t *r_bmodelinterp = NULL;
cvar_t *r_bmodelhighfrac = NULL;
cvar_t *r_norefresh = NULL;
cvar_t *r_drawentities = NULL;
cvar_t *r_drawviewmodel = NULL;
cvar_t *r_speeds = NULL;
cvar_t *r_fullbright = NULL;
cvar_t *r_decals = NULL;
cvar_t *r_lightmap = NULL;
cvar_t *r_shadows = NULL;
cvar_t *r_mirroralpha = NULL;
cvar_t *r_wateralpha = NULL;
cvar_t *r_dynamic = NULL;
cvar_t *r_novis = NULL;
cvar_t *r_mmx = NULL;
cvar_t *r_traceglow = NULL;
cvar_t *r_wadtextures = NULL;
cvar_t *r_glowshellfreq = NULL;
cvar_t *r_detailtextures = NULL;

cvar_t *gl_vsync = NULL;
cvar_t *gl_ztrick = NULL;
cvar_t *gl_finish = NULL;
cvar_t *gl_clear = NULL;
cvar_t *gl_clearcolor = NULL;
cvar_t *gl_cull = NULL;
cvar_t *gl_texsort = NULL;
cvar_t *gl_smoothmodels = NULL;
cvar_t *gl_affinemodels = NULL;
cvar_t *gl_flashblend = NULL;
cvar_t *gl_playermip = NULL;
cvar_t *gl_nocolors = NULL;
cvar_t *gl_keeptjunctions = NULL;
cvar_t *gl_reporttjunctions = NULL;
cvar_t *gl_wateramp = NULL;
cvar_t *gl_dither = NULL;
cvar_t *gl_spriteblend = NULL;
cvar_t *gl_polyoffset = NULL;
cvar_t *gl_lightholes = NULL;
cvar_t *gl_zmax = NULL;
cvar_t *gl_alphamin = NULL;
cvar_t *gl_overdraw = NULL;
cvar_t *gl_watersides = NULL;
cvar_t *gl_overbright = NULL;
cvar_t *gl_envmapsize = NULL;
cvar_t *gl_flipmatrix = NULL;
cvar_t *gl_monolights = NULL;
cvar_t *gl_fog = NULL;
cvar_t *gl_wireframe = NULL;
cvar_t *gl_ansio = NULL;
cvar_t *developer = NULL;
cvar_t *gl_round_down = NULL;
cvar_t *gl_picmip = NULL;
cvar_t *gl_max_size = NULL;

cvar_t *v_texgamma = NULL;
cvar_t *v_lightgamma = NULL;
cvar_t *v_brightness = NULL;
cvar_t *v_gamma = NULL;
cvar_t *v_lambert = NULL;

cvar_t *cl_righthand = NULL;
cvar_t *chase_active = NULL;

int R_GetDrawPass(void)
{
	return r_draw_pass;
}

qboolean R_CullBox(vec3_t mins, vec3_t maxs)
{
	if (r_draw_pass == r_draw_shadow)
		return false;

	if ((*currententity)->model && (*currententity)->model->type == mod_studio)
	{
		if ((*currententity)->curstate.scale > 8.0f)
			return false;
	}

	return gRefFuncs.R_CullBox(mins, maxs);
}

void R_RotateForEntity(vec_t *origin, cl_entity_t *e)
{
	int i;
	vec3_t angles;
	vec3_t modelpos;

	VectorCopy(origin, modelpos);
	VectorCopy(e->angles, angles);

	if (e->curstate.movetype != MOVETYPE_NONE)
	{
		float f;
		float d;

		if (e->curstate.animtime + 0.2f > (*cl_time) && e->curstate.animtime != e->latched.prevanimtime)
		{
			f = ((*cl_time) - e->curstate.animtime) / (e->curstate.animtime - e->latched.prevanimtime);
		}
		else
		{
			f = 0;
		}

		for (i = 0; i < 3; i++)
		{
			modelpos[i] -= (e->latched.prevorigin[i] - e->origin[i]) * f;
		}

		if (f != 0.0f && f < 1.5f)
		{
			f = 1.0f - f;

			for (i = 0; i < 3; i++)
			{
				d = e->latched.prevangles[i] - e->angles[i];

				if (d > 180.0)
					d -= 360.0;
				else if (d < -180.0)
					d += 360.0;

				angles[i] += d * f;
			}
		}
	}

	memcpy(r_rotate_entity_matrix, r_identity_matrix, sizeof(r_identity_matrix));
	Matrix4x4_CreateFromEntity(r_rotate_entity_matrix, angles, modelpos, 1);

	qglTranslatef(modelpos[0], modelpos[1], modelpos[2]);
	qglRotatef(angles[1], 0, 0, 1);
	qglRotatef(angles[0], 0, 1, 0);
	qglRotatef(angles[2], 1, 0, 0);

	r_rotate_entity = true;
}

void R_DrawSpriteModel(cl_entity_t *entity)
{
	if (drawgbuffer)
	{
		R_AddTEntity(entity);
		return;
	}

	gRefFuncs.R_DrawSpriteModel(entity);
}

void R_GetSpriteAxes(cl_entity_t *entity, int type, float *vforwrad, float *vright, float *vup)
{
	gRefFuncs.R_GetSpriteAxes(entity, type, vforwrad, vright, vup);
}

void R_SpriteColor(mcolor24_t *col, cl_entity_t *entity, int renderamt)
{
	gRefFuncs.R_SpriteColor(col, entity, renderamt);
}

float GlowBlend(cl_entity_t *entity)
{
	if(gRefFuncs.GlowBlend)
		return gRefFuncs.GlowBlend(entity);

	vec3_t tmp;
	float dist, brightness;

	VectorSubtract(r_entorigin, r_origin, tmp);
	dist = VectorLength(tmp);

	auto trace = gEngfuncs.PM_TraceLine(r_origin, r_entorigin, r_traceglow->value ? PM_GLASS_IGNORE : (PM_GLASS_IGNORE | PM_STUDIO_IGNORE), 2, -1);

	if ((1.0 - trace->fraction) * dist > 8)
		return 0;

	if (entity->curstate.renderfx == kRenderFxNoDissipation)
	{
		return (float)entity->curstate.renderamt * (1.0f / 255.0f);
	}

	brightness = 19000 / (dist * dist);

	if (brightness < 0.05)
	{
		brightness = 0.05;
	}
	else if (brightness > 1.0)
	{
		brightness = 1.0;
	}

	entity->curstate.scale = dist * (1.0 / 200.0);
	return brightness;
}

int CL_FxBlend(cl_entity_t *entity)
{
	return gRefFuncs.CL_FxBlend(entity);
}

void R_Clear(void)
{
	if (r_mirroralpha && r_mirroralpha->value != 1.0)
	{
		qglDepthMask(GL_TRUE);
		if (gl_clear->value)
			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		else
			qglClear(GL_DEPTH_BUFFER_BIT);

		gldepthmin = 0;
		gldepthmax = 0.5;
		qglDepthFunc(GL_LEQUAL);
	}
	else if (gl_ztrick && gl_ztrick->value)
	{
		static int trickframe;

		if (gl_clear->value)
			qglClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		trickframe++;

		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			qglDepthFunc(GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			qglDepthFunc(GL_GEQUAL);
		}
	}
	else
	{
		qglDepthMask(GL_TRUE);
		if (gl_clear->value)
			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		else
			qglClear(GL_DEPTH_BUFFER_BIT);

		gldepthmin = 0;
		gldepthmax = 1;
		qglDepthFunc(GL_LEQUAL);
	}

	qglDepthRange(gldepthmin, gldepthmax);
}

void R_AddTEntity(cl_entity_t *pEnt)
{
	if (pEnt->model && pEnt->model->type == mod_brush && pEnt->curstate.rendermode == kRenderTransAlpha)
	{
		cl_entity_t *backup_curentity = (*currententity);

		(*currententity) = pEnt;
		R_DrawCurrentEntity();

		(*currententity) = backup_curentity;
		return;
	}

	float dist;
	vec3_t v;

	if ((*numTransObjs) >= (*maxTransObjs))
	{
		gEngfuncs.Con_Printf("R_AddTEntity: Too many objects");
		return;
	}

	if (!pEnt->model || pEnt->model->type != mod_brush || pEnt->curstate.rendermode != kRenderTransAlpha)
	{
		VectorAdd(pEnt->model->mins, pEnt->model->maxs, v);
		VectorScale(v, 0.5, v);
		VectorAdd(v, pEnt->origin, v);
		VectorSubtract(r_origin, v, v);
		dist = DotProduct(v, v);
	}
	else
	{
		dist = 1000000000;
	}

	int i;

	for ( i = (*numTransObjs); i > 0; i-- )
	{
		if ( (*transObjects)[i - 1].distance >= dist )
			break;

		(*transObjects)[i].pEnt = (*transObjects)[i - 1].pEnt;
		(*transObjects)[i].distance = (*transObjects)[i - 1].distance;
	}

	(*transObjects)[i].pEnt = pEnt;
	(*transObjects)[i].distance = dist;
	(*numTransObjs)++;
}

entity_state_t *R_GetPlayerState(int index)
{
	return ((entity_state_t *)((char *)cl_frames + size_of_frame * ((*cl_parsecount) & 63) + sizeof(entity_state_t) * index));
}

void R_DrawCurrentEntity(void)
{
	switch ((*currententity)->model->type)
	{
	case mod_brush:
	{
		R_DrawBrushModel((*currententity));
		break;
	}
	case mod_studio:
	{
		if ((*currententity)->player)
		{
			(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER, R_GetPlayerState((*currententity)->index));
		}
		else
		{
			if ((*currententity)->curstate.movetype == MOVETYPE_FOLLOW)
			{
				for (int j = 0; j < (*cl_numvisedicts); j++)
				{
					if (cl_visedicts[j]->index == (*currententity)->curstate.aiment)
					{
						auto save_currententity = (*currententity);
						(*currententity) = cl_visedicts[j];

						if ((*currententity)->player)
						{
							(*gpStudioInterface)->StudioDrawPlayer(0, R_GetPlayerState(save_currententity->index));
						}
						else
						{
							(*gpStudioInterface)->StudioDrawModel(0);
						}

						(*currententity) = save_currententity;
						break;
					}
				}
				break;
			}

			(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER);
		}

		break;
	}

	default:
	{
		break;
	}
	}
}

void R_DrawEntitiesOnList(void)
{
	int i, j, numvisedicts;

	if (!r_drawentities->value)
		return;

	numvisedicts = *cl_numvisedicts;

	(*numTransObjs) = 0;

	for (i = 0; i < numvisedicts; i++)
	{
		(*currententity) = cl_visedicts[i];

		if ((*currententity)->curstate.rendermode != kRenderNormal)
		{
			//R_AddTEntity(*currententity);
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
					(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS, R_GetPlayerState((*currententity)->index));
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
									(*gpStudioInterface)->StudioDrawPlayer(0, R_GetPlayerState((*currententity)->index));
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
}

void R_DrawTEntitiesOnList(int onlyClientDraw)
{
	return gRefFuncs.R_DrawTEntitiesOnList(onlyClientDraw);

	int i, j, numvisedicts;

	if (!r_drawentities->value)
		return;

	numvisedicts = *cl_numvisedicts;

	if (!onlyClientDraw)
	{
		for (i = 0; i < (*numTransObjs); i++)
		{
			(*currententity) = (*transObjects)[i].pEnt;

			qglDisable(GL_FOG);

			*r_blend = CL_FxBlend(*currententity);

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
						(*r_blend) *= GlowBlend(*currententity);

					if ((*r_blend) != 0)
						R_DrawSpriteModel(*currententity);

					break;
				}

				case mod_studio:
				{
					if ( (*currententity)->curstate.renderamt == kRenderNormal )
						continue;

					if ((*currententity)->player)
					{
						(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS, R_GetPlayerState((*currententity)->index));
					}
					else
					{
						if ((*currententity)->curstate.movetype == MOVETYPE_FOLLOW)
						{
							for (j = 0; j < (*numTransObjs); j++)
							{
								if ((*transObjects)[j].pEnt->index == (*currententity)->curstate.aiment)
								{
									*currententity = (*transObjects)[j].pEnt;

									if ((*currententity)->player)
									{
										(*gpStudioInterface)->StudioDrawPlayer(0, R_GetPlayerState((*currententity)->index));
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

					break;
				}

				default:
				{
					break;
				}
			}
		}
	}

    GL_DisableMultitexture();

    qglEnable(GL_ALPHA_TEST);
    qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (g_bUserFogOn && *g_bUserFogOn)
		qglDisable(GL_FOG);

	HUD_DrawTransparentTriangles();

	if (g_bUserFogOn && *g_bUserFogOn)
		qglEnable(GL_FOG);

	(*numTransObjs) = 0;
	*r_blend = 1.0;
}

void R_SetRenderMode(cl_entity_t *pEntity)
{
	switch (pEntity->curstate.rendermode)
	{
	case kRenderNormal:
	{
		qglColor4f(1, 1, 1, 1);
		break;
	}

	case kRenderTransColor:
	{
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ALPHA);
		qglEnable(GL_BLEND);
		qglColor4f(
			(*currententity)->curstate.rendercolor.r / 255.0,
			(*currententity)->curstate.rendercolor.g / 255.0,
			(*currententity)->curstate.rendercolor.b / 255.0,
			(*r_blend));
		break;
	}

	case kRenderTransAdd:
	{
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		qglBlendFunc(GL_ONE, GL_ONE);
		qglColor4f(*r_blend, *r_blend, *r_blend, 1);
		qglDepthMask(0);
		qglEnable(GL_BLEND);
		break;
	}

	case kRenderTransAlpha:
	{
		qglEnable(GL_ALPHA_TEST);
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		qglColor4f(1, 1, 1, 1);
		qglDisable(GL_BLEND);
		qglAlphaFunc(GL_GREATER, gl_alphamin->value);
		break;
	}

	default:
	{
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglColor4f(1, 1, 1, *r_blend);
		qglDepthMask(0);
		qglEnable(GL_BLEND);
		break;
	}
	}
}

void R_DrawViewModel(void)
{
	float lightvec[3];
	colorVec c;
	float oldShadows;

	lightvec[0] = -1;
	lightvec[1] = 0;
	lightvec[2] = 0;

	(*currententity) = cl_viewent;

	if (!r_drawviewmodel->value)
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	if (gExportfuncs.CL_IsThirdPerson())
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	if (chase_active->value)
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	if ((*envmap))
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	if (!r_drawentities->value)
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	if (cl_stats[0] <= 0)
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	if (!(*currententity)->model)
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	if (r_params.viewentity > r_params.maxclients)
	{
		c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	qglDepthRange(0, 0.3f);

	switch ((*currententity)->model->type)
	{
	case mod_studio:
	{
		if (!(*cl_weaponstarttime))
			(*cl_weaponstarttime) = (*cl_time);

		int playernum = r_params.playernum;

		hud_player_info_t hudPlayerInfo;
		gEngfuncs.pfnGetPlayerInfo(r_params.playernum, &hudPlayerInfo);

		(*currententity)->curstate.frame = 0;
		(*currententity)->curstate.framerate = 1;
		(*currententity)->curstate.sequence = (*cl_weaponsequence);
		(*currententity)->curstate.animtime = (*cl_weaponstarttime);
		(*currententity)->curstate.colormap = ((hudPlayerInfo.topcolor) % 0xFFFF) | ((hudPlayerInfo.bottomcolor << 8) % 0xFFFF);

		c = R_LightPoint((*currententity)->origin);

		oldShadows = r_shadows->value;
		r_shadows->value = 0;
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER);
		r_shadows->value = oldShadows;
		break;
	}

	case mod_brush:
	{
		R_DrawBrushModel((*currententity));
		break;
	}
	}

	qglDepthRange(0, 1);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void R_PolyBlend(void)
{
	gRefFuncs.R_PolyBlend();
}

void S_ExtraUpdate(void)
{
	gRefFuncs.S_ExtraUpdate();
}

float CalcFov(float fov_x, float width, float height)
{
	float a;
	float x;

	if (fov_x < 1 || fov_x > 179)
		fov_x = 90;

	x = width / tan(fov_x / 360 * M_PI);

	a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}

void R_SetCustomFrustum(float *org, float *vpn2, float *vright2, float *vup2, float fov)
{
	//Seems does't work well with SvEngine
	if (fov == 90)
	{
		VectorAdd(vpn2, vright2, custom_frustum[0].normal);
		VectorSubtract(vpn2, vright2, custom_frustum[1].normal);

		VectorAdd(vpn2, vup2, custom_frustum[2].normal);
		VectorSubtract(vpn2, vup2, custom_frustum[3].normal);
	}
	else
	{
		float yfov = CalcFov(fov, glwidth, glheight);

		RotatePointAroundVector(custom_frustum[0].normal, vup2, vpn2, -(90 - fov / 2));
		RotatePointAroundVector(custom_frustum[1].normal, vup2, vpn2, 90 - fov / 2);
		RotatePointAroundVector(custom_frustum[2].normal, vright2, vpn2, 90 - yfov / 2);
		RotatePointAroundVector(custom_frustum[3].normal, vright2, vpn2, -(90 - yfov / 2));
	}

	for (int i = 0; i < 4; i++)
	{
		custom_frustum[i].type = PLANE_ANYZ;
		custom_frustum[i].dist = DotProduct(org, custom_frustum[i].normal);
		custom_frustum[i].signbits = SignbitsForPlane(&custom_frustum[i]);
	}
}

void R_SetFrustum(void)
{
	if(gRefFuncs.R_SetFrustum)
		return gRefFuncs.R_SetFrustum();

	//Seems does't work well with SvEngine
	if (scr_fov_value == 90)
	{
		VectorAdd(vpn, vright, frustum[0].normal);
		VectorSubtract(vpn, vright, frustum[1].normal);

		VectorAdd(vpn, vup, frustum[2].normal);
		VectorSubtract(vpn, vup, frustum[3].normal);
	}
	else
	{
		RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90 - scr_fov_value / 2));
		RotatePointAroundVector(frustum[1].normal, vup, vpn, 90 - scr_fov_value / 2);
		RotatePointAroundVector(frustum[2].normal, vright, vpn, 90 - yfov / 2);
		RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - yfov / 2));
	}

	for (int i = 0; i < 4; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane(&frustum[i]);
	}
}

int SignbitsForPlane(mplane_t *out)
{
	int bits, j;

	bits = 0;

	for (j = 0; j < 3; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1 << j;
	}

	return bits;
}

void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	qglFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void R_SetupGL(void)
{
	gRefFuncs.R_SetupGL();

	qglGetIntegerv(GL_VIEWPORT, r_viewport);
}

void R_CalcRefdef(struct ref_params_s *pparams)
{
	memcpy(&r_params, pparams, sizeof(struct ref_params_s));
}

void GL_ClearFBO(FBO_Container_t *s)
{
	s->s_hBackBufferFBO = 0;
	s->s_hBackBufferCB = 0;
	s->s_hBackBufferDB = 0;
	s->s_hBackBufferTex = 0;
	s->s_hBackBufferTex2 = 0;
	s->s_hBackBufferTex3 = 0;
	s->s_hBackBufferTex4 = 0;
	s->s_hBackBufferTex5 = 0;
	s->s_hBackBufferDepthTex = 0;
	s->iWidth = s->iHeight = s->iTextureColorFormat = 0;
}

void GL_FreeFBO(FBO_Container_t *s)
{
	if (s->s_hBackBufferFBO)
		qglDeleteFramebuffersEXT(1, &s->s_hBackBufferFBO);

	if (s->s_hBackBufferCB)
		qglDeleteRenderbuffersEXT(1, &s->s_hBackBufferCB);

	if (s->s_hBackBufferDB)
		qglDeleteRenderbuffersEXT(1, &s->s_hBackBufferDB);

	if (s->s_hBackBufferTex)
		qglDeleteTextures(1, &s->s_hBackBufferTex);

	if (s->s_hBackBufferTex2)
		qglDeleteTextures(1, &s->s_hBackBufferTex2);

	if (s->s_hBackBufferTex3)
		qglDeleteTextures(1, &s->s_hBackBufferTex3);

	if (s->s_hBackBufferTex4)
		qglDeleteTextures(1, &s->s_hBackBufferTex4);

	if (s->s_hBackBufferTex5)
		qglDeleteTextures(1, &s->s_hBackBufferTex5);

	if (s->s_hBackBufferDepthTex)
		qglDeleteTextures(1, &s->s_hBackBufferDepthTex);

	GL_ClearFBO(s);
}

bool GL_IsValidSampleCount(int msaa_samples)
{
	return (msaa_samples == 2 || msaa_samples == 4 || msaa_samples == 8 || msaa_samples == 16);
}

bool R_UseMSAA(void)
{
	return s_MSAAFBO.s_hBackBufferFBO && GL_IsValidSampleCount((int)r_msaa->value) && !g_SvEngine_DrawPortalView;
}

void GL_GenerateFBO(void)
{
	bNoStretchAspect = (gEngfuncs.CheckParm("-stretchaspect", NULL) == 0);

	GL_ClearFBO(&s_MSAAFBO);
	GL_ClearFBO(&s_GBufferFBO);
	GL_ClearFBO(&s_BackBufferFBO);
	GL_ClearFBO(&s_BackBufferFBO2);
	for(int i = 0; i < DOWNSAMPLE_BUFFERS; ++i)
		GL_ClearFBO(&s_DownSampleFBO[i]);
	for(int i = 0; i < LUMIN_BUFFERS; ++i)
		GL_ClearFBO(&s_LuminFBO[i]);
	for(int i = 0; i < LUMIN1x1_BUFFERS; ++i)
		GL_ClearFBO(&s_Lumin1x1FBO[i]);
	GL_ClearFBO(&s_BrightPassFBO);
	for(int i = 0; i < BLUR_BUFFERS; ++i)
	{
		GL_ClearFBO(&s_BlurPassFBO[i][0]);
		GL_ClearFBO(&s_BlurPassFBO[i][1]);
	}
	GL_ClearFBO(&s_BrightAccumFBO);
	GL_ClearFBO(&s_ToneMapFBO);
	GL_ClearFBO(&s_DepthLinearFBO);
	GL_ClearFBO(&s_HBAOCalcFBO);
	GL_ClearFBO(&s_WaterFBO);
	GL_ClearFBO(&s_ShadowFBO);

	if (!gl_msaa_support)
	{
		bDoMSAA = false;

		gEngfuncs.Con_Printf("MSAA disabled due to lack of GL_EXT_framebuffer_multisample.\n");
	}

	qglEnable(GL_TEXTURE_2D);

	gl_color_format = GL_RGBA32F;

	if (bDoMSAA)
	{
		gl_msaa_samples = 4;

		s_MSAAFBO.iWidth = glwidth;
		s_MSAAFBO.iHeight = glheight;
		GL_GenFrameBuffer(&s_MSAAFBO);
		GL_FrameBufferColorTexture(&s_MSAAFBO, gl_color_format, true);
		GL_FrameBufferDepthTexture(&s_MSAAFBO, GL_DEPTH24_STENCIL8, true);
		
		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_MSAAFBO);
			bDoMSAA = false;

			Sys_ErrorEx("Failed to initialize MSAA framebuffer!\n");
		}

		qglEnable(GL_MULTISAMPLE);
	}

	s_BackBufferFBO.iWidth = glwidth;
	s_BackBufferFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_BackBufferFBO);
	GL_FrameBufferColorTexture(&s_BackBufferFBO, gl_color_format, false);
	GL_FrameBufferDepthTexture(&s_BackBufferFBO, GL_DEPTH24_STENCIL8, false);

	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BackBufferFBO);
		Sys_ErrorEx("Failed to initialize backbuffer framebuffer!\n");
	}

	s_BackBufferFBO2.iWidth = glwidth;
	s_BackBufferFBO2.iHeight = glheight;
	GL_GenFrameBuffer(&s_BackBufferFBO2);
	GL_FrameBufferColorTexture(&s_BackBufferFBO2, gl_color_format, false);
	GL_FrameBufferDepthTexture(&s_BackBufferFBO2, GL_DEPTH24_STENCIL8, false);

	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BackBufferFBO2);
		Sys_ErrorEx("Failed to initialize backbuffer2 framebuffer!\n");
	}

	s_GBufferFBO.iWidth = glwidth;
	s_GBufferFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_GBufferFBO);
	GL_FrameBufferColorTextureDeferred(&s_GBufferFBO, gl_color_format);
	GL_FrameBufferDepthTexture(&s_GBufferFBO, GL_DEPTH24_STENCIL8, false);

	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_GBufferFBO);
		Sys_ErrorEx("Failed to initialize GBuffer framebuffer.\n");
	}

	s_DepthLinearFBO.iWidth = glwidth;
	s_DepthLinearFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_DepthLinearFBO);
	GL_FrameBufferColorTexture(&s_DepthLinearFBO, GL_R32F, false);

	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_DepthLinearFBO);
		Sys_ErrorEx("Failed to initialize DepthLinear framebuffer!\n");
	}

	s_HBAOCalcFBO.iWidth = glwidth;
	s_HBAOCalcFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_HBAOCalcFBO);
	GL_FrameBufferColorTextureHBAO(&s_HBAOCalcFBO);

	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_HBAOCalcFBO);
		Sys_ErrorEx("Failed to initialize HBAOCalc framebuffer.\n");
	}

	s_ShadowFBO.iWidth = glwidth;
	s_ShadowFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_ShadowFBO);

	s_WaterFBO.iWidth = glwidth;
	s_WaterFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_WaterFBO);

	//DownSample FBO 1->1/4->1/16
	int downW, downH;

	downW = glwidth;
	downH = glheight;
	for (int i = 0; i < DOWNSAMPLE_BUFFERS; ++i)
	{
		downW >>= 1;
		downH >>= 1;
		s_DownSampleFBO[i].iWidth = downW;
		s_DownSampleFBO[i].iHeight = downH;
		//fbo
		GL_GenFrameBuffer(&s_DownSampleFBO[i]);
		//color
		GL_FrameBufferColorTexture(&s_DownSampleFBO[i], gl_color_format, false);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_DownSampleFBO[i]);
			Sys_ErrorEx("Failed to initialize DownSample #%d framebuffer.\n", i);
		}
	}

	//Luminance FBO
	downW = glwidth;
	downH = glheight;
	while ((downH >> 1) >= 256)
	{
		downW >>= 1;
		downH >>= 1;
	}
	//64x64 16x16 4x4
	for (int i = 0; i < LUMIN_BUFFERS; ++i)
	{
		s_LuminFBO[i].iWidth = downW;
		s_LuminFBO[i].iHeight = downH;
		GL_GenFrameBuffer(&s_LuminFBO[i]);
		GL_FrameBufferColorTexture(&s_LuminFBO[i], GL_R32F, false);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_LuminFBO[i]);
			Sys_ErrorEx("Failed to initialize Luminance #%d framebuffer.\n", i);
		}

		downW >>= 2;
		downH >>= 2;
	}

	//Luminance 1x1 FBO
	for (int i = 0; i < LUMIN1x1_BUFFERS; ++i)
	{
		s_Lumin1x1FBO[i].iWidth = 1;
		s_Lumin1x1FBO[i].iHeight = 1;
		GL_GenFrameBuffer(&s_Lumin1x1FBO[i]);
		GL_FrameBufferColorTexture(&s_Lumin1x1FBO[i], GL_R32F, false);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_Lumin1x1FBO[i]);
			Sys_ErrorEx("Failed to initialize Luminance1x1 #%d framebuffer.\n", i);
		}
	}

	//Bright Pass FBO
	s_BrightPassFBO.iWidth = (glwidth >> DOWNSAMPLE_BUFFERS);
	s_BrightPassFBO.iHeight = (glheight >> DOWNSAMPLE_BUFFERS);
	GL_GenFrameBuffer(&s_BrightPassFBO);
	GL_FrameBufferColorTexture(&s_BrightPassFBO, gl_color_format, false);

	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BrightPassFBO);
		Sys_ErrorEx("Failed to initialize BrightPass framebuffer.\n");
	}

	//Blur FBO
	downW = glwidth >> DOWNSAMPLE_BUFFERS;
	downH = glheight >> DOWNSAMPLE_BUFFERS;

	for (int i = 0; i < BLUR_BUFFERS; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			s_BlurPassFBO[i][j].iWidth = downW;
			s_BlurPassFBO[i][j].iHeight = downH;

			GL_GenFrameBuffer(&s_BlurPassFBO[i][j]);
			GL_FrameBufferColorTexture(&s_BlurPassFBO[i][j], gl_color_format, false);

			if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				GL_FreeFBO(&s_BlurPassFBO[i][j]);
				Sys_ErrorEx("Failed to initialize Blur #%d framebuffer.\n", i);
			}
		}
		downW >>= 1;
		downH >>= 1;
	}

	s_BrightAccumFBO.iWidth = glwidth >> DOWNSAMPLE_BUFFERS;
	s_BrightAccumFBO.iHeight = glheight >> DOWNSAMPLE_BUFFERS;
	GL_GenFrameBuffer(&s_BrightAccumFBO);
	GL_FrameBufferColorTexture(&s_BrightAccumFBO, gl_color_format, false);
	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BrightAccumFBO);
		Sys_ErrorEx("Failed to initialize BrightAccumulate #%d framebuffer.\n");
	}

	s_ToneMapFBO.iWidth = glwidth;
	s_ToneMapFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_ToneMapFBO);
	GL_FrameBufferColorTexture(&s_ToneMapFBO, GL_RGBA8, false);
	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_ToneMapFBO);
		gEngfuncs.Con_Printf("Failed to initialize ToneMapping #%d framebuffer.\n");
	}

	qglBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}

void GL_Init(void)
{
	QGL_Init();

	if (!(*gl_mtexable))
	{
		Sys_ErrorEx("Multitexture extension must be enabled!\nPlease remove \"-nomtex\" from launch parameters and try again.");
		return;
	}

	if (!gl_shader_support)
	{
		Sys_ErrorEx("Missing OpenGL extension GL_ARB_shader_objects!\n");
		return;
	}

	if (!gl_framebuffer_object)
	{
		Sys_ErrorEx("Missing OpenGL extension GL_EXT_framebuffer_object!\n");
		return;
	}

	if (!gl_blit_support)
	{
		Sys_ErrorEx("Missing OpenGL extension GL_EXT_framebuffer_blit!\n");
		return;
	}

	if (!gl_float_buffer_support)
	{
		Sys_ErrorEx("Missing OpenGL extension GL_NV_float_buffer!\n");
		return;
	}

	GL_GenerateFBO();
	GL_InitShaders();
}

void GL_Shutdown(void)
{
	GL_FreeShaders();

	GL_FreeFBO(&s_MSAAFBO);
	GL_FreeFBO(&s_BackBufferFBO);
	GL_FreeFBO(&s_BackBufferFBO2);
	for (int i = 0; i < DOWNSAMPLE_BUFFERS; ++i)
		GL_FreeFBO(&s_DownSampleFBO[i]);
	for (int i = 0; i < LUMIN_BUFFERS; ++i)
		GL_FreeFBO(&s_LuminFBO[i]);
	for (int i = 0; i < LUMIN1x1_BUFFERS; ++i)
		GL_FreeFBO(&s_Lumin1x1FBO[i]);
	for (int i = 0; i < BLUR_BUFFERS; ++i)
	{
		GL_FreeFBO(&s_BlurPassFBO[i][0]);
		GL_FreeFBO(&s_BlurPassFBO[i][1]);
	}
	GL_FreeFBO(&s_ToneMapFBO);
	GL_FreeFBO(&s_DepthLinearFBO);
	GL_FreeFBO(&s_HBAOCalcFBO);
	GL_FreeFBO(&s_ShadowFBO);
	GL_FreeFBO(&s_WaterFBO);
}

void GL_BeginRendering(int *x, int *y, int *width, int *height)
{
	gRefFuncs.GL_BeginRendering(x, y, width, height);

	glx = *x;
	gly = *y;
	glwidth = *width; 
	glheight = *height;

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	qglClearColor(0.0, 0.0, 0.0, 1);
	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglDepthMask(GL_TRUE);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglStencilMask(0);

	//Re-Generate MSAA framebuffer
	if (bDoMSAA)
	{
		int msaa_samples = (int)r_msaa->value;
		if (GL_IsValidSampleCount(msaa_samples) && msaa_samples != gl_msaa_samples)
		{
			gl_msaa_samples = msaa_samples;

			GL_FreeFBO(&s_MSAAFBO);
			s_MSAAFBO.iWidth = glwidth;
			s_MSAAFBO.iHeight = glheight;
			GL_GenFrameBuffer(&s_MSAAFBO);
			GL_FrameBufferColorTexture(&s_MSAAFBO, gl_color_format, true);
			GL_FrameBufferDepthTexture(&s_MSAAFBO, GL_DEPTH24_STENCIL8, true);
		}
	}
}

void R_PreRenderView(int a1)
{
	g_SvEngine_DrawPortalView = a1;

	r_studio_framecount++;
	r_fog_mode = 0;

	if (!r_refdef->onlyClientDraws)
	{
		if (qglIsEnabled(GL_FOG))
		{
			qglGetIntegerv(GL_FOG_MODE, &r_fog_mode);

			if (r_fog_mode == GL_LINEAR)
			{
				qglGetFloatv(GL_FOG_START, &r_fog_control[0]);
				qglGetFloatv(GL_FOG_END, &r_fog_control[1]);
				qglGetFloatv(GL_FOG_COLOR, r_fog_color);
			}
		}

		if (r_water && r_water->value && waters_active)
		{
			R_RenderWaterView();
		}
		if (r_shadow && r_shadow->value && !g_SvEngine_DrawPortalView)
		{
			R_RenderShadowMap();
		}
	}

	if (R_UseMSAA())
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
	else
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
}

void R_PostRenderView()
{
	if (!r_draw_pass && !g_SvEngine_DrawPortalView)
	{
		R_FreeDeadWaters();
		for (r_water_t *water = waters_active; water; water = water->next)
		{
			water->free = true;
		}
	}

	if (R_UseMSAA())
	{
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
		qglBlitFramebufferEXT(0, 0, s_MSAAFBO.iWidth, s_MSAAFBO.iHeight, 0, 0, s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		R_DoHDR();
	}
	else
	{
		R_DoFXAA();

		R_DoHDR();
	}

	GL_DisableMultitexture();
	qglEnable(GL_TEXTURE_2D);
	qglColor4f(1, 1, 1, 1);

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	g_SvEngine_DrawPortalView = 0;	
}

void R_PreDrawViewModel(void)
{
	(*currententity) = cl_viewent;

	if (!r_drawviewmodel->value)
		return;

	if (gExportfuncs.CL_IsThirdPerson())
		return;

	if (chase_active->value)
		return;

	if ((*envmap))
		return;

	if (!r_drawentities->value)
		return;

	if (cl_stats[0] <= 0)
		return;

	if (!(*currententity)->model)
		return;

	if (r_params.viewentity > r_params.maxclients)
		return;

	switch ((*currententity)->model->type)
	{
	case mod_studio:
	{
		if (!(*cl_weaponstarttime))
			(*cl_weaponstarttime) = (*cl_time);

		(*currententity)->curstate.sequence = (*cl_weaponsequence);
		(*currententity)->curstate.frame = 0;
		(*currententity)->curstate.framerate = 1;
		(*currententity)->curstate.animtime = (*cl_weaponstarttime);

		auto ent = gEngfuncs.GetEntityByIndex((*currententity)->index);
		for (int i = 0; i < 4; i++)
		{
			VectorCopy(ent->origin, (*currententity)->attachment[i]);
		}

		(*gpStudioInterface)->StudioDrawModel(STUDIO_EVENTS);
		break;
	}
	}
}

void R_RenderView_SvEngine(int a1)
{
	if (!a1)
	{
		r_wsurf_drawcall = 0;
		r_wsurf_polys = 0;
		r_studio_drawcall = 0;
		r_studio_polys = 0;
	}

	if (r_norefresh->value)
		return;

	if (!r_worldmodel)
	{
		Sys_ErrorEx("R_RenderView: NULL worldmodel");
	}

	double time1;

	if (!a1 && r_speeds->value)
	{
		time1 = gEngfuncs.GetAbsoluteTime();
	}

	R_ForceCVars(r_params.maxclients > 1);

	R_PreRenderView(a1);

	float clearColor[3];
	R_ParseVectorCvar(gl_clearcolor, clearColor);

	qglClearColor(clearColor[0], clearColor[1], clearColor[2], 1);

	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglDepthMask(GL_TRUE);

	if (!gl_clear->value || a1)
		qglClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	else
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	qglStencilMask(0);

	qglDepthFunc(GL_LEQUAL);
	qglDepthRange(0, 1);

	if (!r_refdef->onlyClientDraws)
		R_PreDrawViewModel();

	R_RenderScene();

	if (!r_refdef->onlyClientDraws)
		R_DrawViewModel();

	R_PolyBlend();

	R_PostRenderView();

	S_ExtraUpdate();

	if (!a1 && r_speeds->value)
	{
		float framerate = (*cl_time) - (*cl_oldtime);

		if (framerate > 0)
			framerate = 1.0 / framerate;

		auto time2 = gEngfuncs.GetAbsoluteTime();

		gEngfuncs.Con_Printf("%3ifps %3i ms, %4i brushpolys, %4i brushdraw, %4i studiopolys, %4i studiodraw\n",
			(int)(framerate + 0.5), (int)((time2 - time1) * 1000), 
			r_wsurf_polys, r_wsurf_drawcall,
			r_studio_polys, r_studio_drawcall
		);

		*c_alias_polys = r_studio_polys;
		*c_brush_polys = r_wsurf_polys;
	}
}

void R_RenderView(void)
{
	R_PreRenderView(0);

	gRefFuncs.R_RenderView();

	R_PostRenderView();
}

void R_RenderScene(void)
{
	gRefFuncs.R_RenderScene();
}

void GL_EndRendering(void)
{
	//Disable engine's framebuffer
	GLuint save_backbuffer_fbo = 0;
	if (gl_backbuffer_fbo)
	{
		save_backbuffer_fbo = *gl_backbuffer_fbo;
		*gl_backbuffer_fbo = 0;
	}

	int windowW = glwidth;
	int windowH = glheight;

	windowW = window_rect->right - window_rect->left;
	windowH = window_rect->bottom - window_rect->top;

	int dstX = 0;
	int dstY = 0;
	int dstX2 = windowW;
	int dstY2 = windowH;

	*videowindowaspect = *windowvideoaspect = 1;

	float videoAspect = (float)glwidth / glheight;
	float windowAspect = (float)windowW / windowH;
	if (bNoStretchAspect)
	{
		if (windowAspect < videoAspect)
		{
			dstY = (windowH - 1.0 / videoAspect * windowW) / 2.0;
			dstY2 = windowH - dstY;
			*videowindowaspect = videoAspect / windowAspect;
		}
		else
		{
			dstX = (windowW - windowH * videoAspect) / 2.0;
			dstX2 = windowW - dstX;
			*windowvideoaspect = windowAspect / videoAspect;
		}
	}

	//Blit to screen
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	qglClearColor(0, 0, 0, 1);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglBlitFramebufferEXT(0, 0, glwidth, glheight, dstX, dstY, dstX2, dstY2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);

	//VID_FlipScreen for us.
	gRefFuncs.GL_EndRendering();

	if (gl_backbuffer_fbo)
	{
		*gl_backbuffer_fbo = save_backbuffer_fbo;
	}
}

void R_InitCvars(void)
{
	r_bmodelinterp = gEngfuncs.pfnGetCvarPointer("r_bmodelinterp");
	r_bmodelhighfrac = gEngfuncs.pfnGetCvarPointer("r_bmodelhighfrac");
	r_norefresh = gEngfuncs.pfnGetCvarPointer("r_norefresh");
	r_drawentities = gEngfuncs.pfnGetCvarPointer("r_drawentities");
	r_drawviewmodel = gEngfuncs.pfnGetCvarPointer("r_drawviewmodel");
	r_speeds = gEngfuncs.pfnGetCvarPointer("r_speeds");
	r_fullbright = gEngfuncs.pfnGetCvarPointer("r_fullbright");
	r_decals = gEngfuncs.pfnGetCvarPointer("r_decals");
	r_lightmap = gEngfuncs.pfnGetCvarPointer("r_lightmap");
	r_shadows = gEngfuncs.pfnGetCvarPointer("r_shadows");
	r_mirroralpha = gEngfuncs.pfnGetCvarPointer("r_mirroralpha");
	r_wateralpha = gEngfuncs.pfnGetCvarPointer("r_wateralpha");
	r_dynamic = gEngfuncs.pfnGetCvarPointer("r_dynamic");
	r_mmx = gEngfuncs.pfnGetCvarPointer("r_mmx");
	r_traceglow = gEngfuncs.pfnGetCvarPointer("r_traceglow");
	r_wadtextures = gEngfuncs.pfnGetCvarPointer("r_wadtextures");
	r_glowshellfreq = gEngfuncs.pfnGetCvarPointer("r_glowshellfreq");
	r_novis = gEngfuncs.pfnGetCvarPointer("r_novis");
	r_novis->flags &= ~FCVAR_SPONLY;

	r_detailtextures = gEngfuncs.pfnGetCvarPointer("r_detailtextures");

	gl_vsync = gEngfuncs.pfnGetCvarPointer("gl_vsync");

	if (!gl_vsync)
		gl_vsync = gEngfuncs.pfnRegisterVariable("gl_vsync", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	gl_ztrick = gEngfuncs.pfnGetCvarPointer("gl_ztrick");

	if (!gl_ztrick)
		gl_ztrick = gEngfuncs.pfnGetCvarPointer("gl_ztrick_old");

	gl_finish = gEngfuncs.pfnGetCvarPointer("gl_finish");
	gl_clear = gEngfuncs.pfnGetCvarPointer("gl_clear");
	gl_clearcolor = gEngfuncs.pfnGetCvarPointer("gl_clearcolor");
	if(!gl_clearcolor)
		gl_clearcolor = gEngfuncs.pfnRegisterVariable("gl_clearcolor", "0 0 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	gl_cull = gEngfuncs.pfnGetCvarPointer("gl_cull");
	gl_texsort = gEngfuncs.pfnGetCvarPointer("gl_texsort");

	gl_smoothmodels = gEngfuncs.pfnGetCvarPointer("gl_smoothmodels");
	gl_affinemodels = gEngfuncs.pfnGetCvarPointer("gl_affinemodels");
	gl_flashblend = gEngfuncs.pfnGetCvarPointer("gl_flashblend");
	gl_playermip = gEngfuncs.pfnGetCvarPointer("gl_playermip");
	gl_nocolors = gEngfuncs.pfnGetCvarPointer("gl_nocolors");
	gl_keeptjunctions = gEngfuncs.pfnGetCvarPointer("gl_keeptjunctions");
	gl_reporttjunctions = gEngfuncs.pfnGetCvarPointer("gl_reporttjunctions");
	gl_wateramp = gEngfuncs.pfnGetCvarPointer("gl_wateramp");
	gl_dither = gEngfuncs.pfnGetCvarPointer("gl_dither");
	gl_spriteblend = gEngfuncs.pfnGetCvarPointer("gl_spriteblend");
	gl_polyoffset = gEngfuncs.pfnGetCvarPointer("gl_polyoffset");
	gl_lightholes = gEngfuncs.pfnGetCvarPointer("gl_lightholes");
	gl_zmax = gEngfuncs.pfnGetCvarPointer("gl_zmax");
	gl_alphamin = gEngfuncs.pfnGetCvarPointer("gl_alphamin");
	gl_overdraw = gEngfuncs.pfnGetCvarPointer("gl_overdraw");
	gl_watersides = gEngfuncs.pfnGetCvarPointer("gl_watersides");
	gl_overbright = gEngfuncs.pfnGetCvarPointer("gl_overbright");
	gl_envmapsize = gEngfuncs.pfnGetCvarPointer("gl_envmapsize");
	gl_flipmatrix = gEngfuncs.pfnGetCvarPointer("gl_flipmatrix");
	gl_monolights = gEngfuncs.pfnGetCvarPointer("gl_monolights");
	gl_fog = gEngfuncs.pfnGetCvarPointer("gl_fog");

	gl_wireframe = gEngfuncs.pfnGetCvarPointer("gl_wireframe");
	gl_wireframe->flags &= ~FCVAR_SPONLY;

	gl_round_down = gEngfuncs.pfnGetCvarPointer("gl_round_down");
	gl_picmip = gEngfuncs.pfnGetCvarPointer("gl_picmip");
	gl_max_size = gEngfuncs.pfnGetCvarPointer("gl_max_size");
	gl_max_size->value = gl_max_texture_size;

	developer = gEngfuncs.pfnGetCvarPointer("developer");

	gEngfuncs.Cvar_SetValue("r_detailtextures", 1);

	gl_ansio = gEngfuncs.pfnGetCvarPointer("gl_ansio");
	if (!gl_ansio)
		gl_ansio = gEngfuncs.pfnRegisterVariable("gl_ansio", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	v_texgamma = gEngfuncs.pfnGetCvarPointer("texgamma");
	v_lightgamma = gEngfuncs.pfnGetCvarPointer("lightgamma");
	v_brightness = gEngfuncs.pfnGetCvarPointer("brightness");
	v_gamma = gEngfuncs.pfnGetCvarPointer("gamma");
	v_lambert = gEngfuncs.pfnGetCvarPointer("lambert");

	cl_righthand = gEngfuncs.pfnGetCvarPointer("cl_righthand");
	chase_active = gEngfuncs.pfnGetCvarPointer("chase_active");

	r_msaa = gEngfuncs.pfnRegisterVariable("r_msaa", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}

void R_Init(void)
{
	R_InitCvars();
	R_InitWater();
	R_InitStudio();
	R_InitShadow();
	R_InitWSurf();
	R_InitGLHUD();
	R_InitLight();

	Draw_Init();
}

void R_Shutdown(void)
{
	R_FreeShadow();
	R_FreeWater();
	R_ShutdownLight();
	R_ShutdownWSurf();
	R_ShutdownStudio();
}

void R_ForceCVars(qboolean mp)
{
	if (r_draw_pass)
		return;

	gRefFuncs.R_ForceCVars(mp);
}

void R_NewMap(void)
{
	gRefFuncs.R_NewMap();

	r_worldentity = gEngfuncs.GetEntityByIndex(0);
	r_worldmodel = r_worldentity->model;
	r_studio_framecount = 0;

	memset(&r_params, 0, sizeof(r_params));

	R_ClearWater();
	R_VidInitWSurf();

	R_StudioClearVBOCache();
	R_StudioClearBoneCache();
}

mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model)
{
	if (r_draw_pass == r_draw_reflect && model == r_worldmodel && VectorCompare(p, r_refdef->vieworg))
	{
		return gRefFuncs.Mod_PointInLeaf(water_view, model);
	}

	return gRefFuncs.Mod_PointInLeaf(p, model);
}

float *R_GetAttachmentPoint(int entity, int attachment)
{
	auto pEntity = gEngfuncs.GetEntityByIndex(entity);

	if (attachment)
		return pEntity->attachment[attachment - 1];

	return pEntity->origin;
}

qboolean R_ParseVectorCvar(cvar_t *a1, float *vec)
{
	double v2; // st7@2
	double v3; // st6@2
	double v4; // st5@2
	double v5; // st7@3
	double v6; // st4@7
	double v7; // st4@11
	double v8; // st7@13
	qboolean result; // eax@14
	float v10; // [sp+4h] [bp-10h]@1
	float v11; // [sp+8h] [bp-Ch]@1
	float v12; // [sp+Ch] [bp-8h]@1

	if (sscanf(a1->string, "%f %f %f", &v10, &v11, &v12) == 3)
	{
		vec[0] = v10;
		vec[1] = v11;
		vec[2] = v12;
		v2 = vec[0];
		v3 = 0.0;
		v4 = 255.0;
		if (v2 >= 0.0)
		{
			if (v2 <= 255.0)
			{
				v4 = vec[0];
				v5 = 255.0;
			}
			else
			{
				v5 = 255.0;
			}
		}
		else
		{
			v5 = 255.0;
			v4 = 0.0;
		}
		vec[0] = v4 * 0.0039215689;
		v6 = vec[1];
		if (v6 >= 0.0)
		{
			if (v6 > v5)
				v6 = v5;
		}
		else
		{
			v6 = 0.0;
		}
		vec[1] = v6 * 0.0039215689;
		v7 = vec[2];
		if (v7 < 0.0)
		{
			v8 = 0.0039215689;
		}
		else
		{
			if (v7 <= v5)
				v5 = vec[2];
			v3 = v5;
			v8 = 0.0039215689;
		}
		result = 1;
		vec[2] = v8 * v3;
	}
	else
	{
		result = 0;
	}
	return result;
}