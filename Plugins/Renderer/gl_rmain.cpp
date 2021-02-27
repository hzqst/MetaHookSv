#include "gl_local.h"
#include "cJSON.h"
#include "pm_defs.h"

ref_funcs_t gRefFuncs;

refdef_t *r_refdef;

ref_params_t r_params;

int saved_c_alias_polys;
int saved_c_brush_polys;

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

mplane_t custom_frustum[4];
mplane_t *frustum;
mleaf_t **r_viewleaf, **r_oldviewleaf;
texture_t *r_notexture_mip;

int mirrortexturenum;
qboolean mirror;
mplane_t *mirror_plane;

float yfov;
float screenaspect;

vec_t *vup;
vec_t *vpn;
vec_t *vright;
vec_t *r_origin;
vec_t *modelorg;
vec_t *r_entorigin;

int *r_framecount;
int *r_visframecount;

frame_t *cl_frames;
int size_of_frame = sizeof(frame_t);
int *cl_parsecount;
int *cl_waterlevel;
double *cl_time;
double *cl_oldtime;

qboolean gl_framebuffer_object = false;
qboolean gl_shader_support = false;
qboolean gl_program_support = false;
qboolean gl_msaa_support = false;
qboolean gl_msaa_blit_support = false;
qboolean gl_csaa_support = false;
qboolean gl_float_buffer_support = false;
qboolean gl_s3tc_compression_support = false;

int gl_max_texture_size = 0;
float gl_max_ansio = 0;
float gl_force_ansio = 0;
int gl_msaa_samples = 0;

int *gl_msaa_fbo = 0;
int *gl_backbuffer_fbo = 0;
int *gl_mtexable = 0;
qboolean *mtexenabled = 0;

int glx = 0;
int gly = 0;
int glwidth = 0;
int glheight = 0;

FBO_Container_t s_MSAAFBO;
FBO_Container_t s_GBufferFBO;
FBO_Container_t s_BackBufferFBO;
FBO_Container_t s_DownSampleFBO[DOWNSAMPLE_BUFFERS];
FBO_Container_t s_LuminFBO[LUMIN_BUFFERS];
FBO_Container_t s_Lumin1x1FBO[LUMIN1x1_BUFFERS];
FBO_Container_t s_BrightPassFBO;
FBO_Container_t s_BlurPassFBO[BLUR_BUFFERS][2];
FBO_Container_t s_BrightAccumFBO;
FBO_Container_t s_ToneMapFBO;
FBO_Container_t s_DepthLinearFBO;
FBO_Container_t s_HBAOCalcFBO;
FBO_Container_t s_CloakFBO;
FBO_Container_t s_ShadowFBO;
FBO_Container_t s_WaterFBO;

qboolean bDoMSAAFBO = true;
qboolean bDoScaledFBO = true;
qboolean bDoDirectBlit = true;
qboolean bDoHDR = true;
qboolean bNoStretchAspect = false;

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

int R_GetDrawPass(void)
{
	if(drawreflect)
		return r_draw_reflect;
	if(drawrefract)
		return r_draw_refract;
	if(draw3dsky)
		return r_draw_3dsky;
	return r_draw_normal;
}

qboolean R_CullBox(vec3_t mins, vec3_t maxs)
{
	if(draw3dsky)
		return false;

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

	qglTranslatef(modelpos[0], modelpos[1], modelpos[2]);

	qglRotatef(angles[1], 0, 0, 1);
	qglRotatef(angles[0], 0, 1, 0);
	qglRotatef(angles[2], 1, 0, 0);
}

void R_DrawSpriteModel(cl_entity_t *entity)
{
	R_UseGBufferProgram(GBUFFER_DIFFUSE_ENABLED | GBUFFER_TRANSPARENT_ENABLED);
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);

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
		if (gl_clear->value)
			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
			qglClear(GL_COLOR_BUFFER_BIT);

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
		if (gl_clear->value)
			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
	float dist;
	vec3_t v;

	if ((*numTransObjs) >= (*maxTransObjs))
		gEngfuncs.Con_Printf("R_AddTEntity: Too many objects");

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

void R_DrawBrushModel(cl_entity_t *entity)
{
	gRefFuncs.R_DrawBrushModel(entity);
}

void R_DrawViewModel(void)
{
}

void R_PreDrawViewModel(void)
{
}

void R_PolyBlend(void)
{
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

	if ((drawreflect || drawrefract) && curwater)
	{
		qglViewport(0, 0, curwater->texwidth, curwater->texheight);

		R_EnableClip(true);
	}
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

	if (s->s_hBackBufferDepthTex)
		qglDeleteTextures(1, &s->s_hBackBufferDepthTex);

	GL_ClearFBO(s);
}

void R_GLGenFrameBuffer(FBO_Container_t *s)
{
	qglGenFramebuffersEXT(1, &s->s_hBackBufferFBO);
	qglBindFramebufferEXT(GL_FRAMEBUFFER, s->s_hBackBufferFBO);
}

void R_GLGenRenderBuffer(FBO_Container_t *s, qboolean depth)
{
	if(!depth)
	{
		qglGenRenderbuffersEXT(1, &s->s_hBackBufferCB);
		qglBindRenderbufferEXT(GL_RENDERBUFFER, s->s_hBackBufferCB);
	}
	else
	{
		qglGenRenderbuffersEXT(1, &s->s_hBackBufferDB);
		qglBindRenderbufferEXT(GL_RENDERBUFFER, s->s_hBackBufferDB);
	}
}

void R_GLRenderBufferStorage(FBO_Container_t *s, qboolean depth, GLuint iInternalFormat, qboolean multisample)
{
	if(multisample)
	{
		/*if(gl_csaa_support)
			qglRenderbufferStorageMultisampleCoverageNV(GL_RENDERBUFFER, gl_csaa_samples, gl_msaa_samples, iInternalFormat, s->iWidth, s->iHeight);
		else
			*/qglRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, gl_msaa_samples, iInternalFormat, s->iWidth, s->iHeight);
	}
	else
	{
		qglRenderbufferStorageEXT(GL_RENDERBUFFER, iInternalFormat, s->iWidth, s->iHeight);
	}
	qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER, (depth) ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, (depth) ? s->s_hBackBufferDB : s->s_hBackBufferCB);
}

void R_GLFrameBufferColorTexture(FBO_Container_t *s, GLuint iInternalFormat, qboolean multisample)
{
	int tex2D = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

	s->s_hBackBufferTex = GL_GenTexture();
	qglBindTexture(tex2D, s->s_hBackBufferTex);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(tex2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (multisample)
	{
		qglTexStorage2DMultisample(tex2D, gl_msaa_samples, iInternalFormat, s->iWidth, s->iHeight, GL_FALSE);
	}
	else
	{
		if (iInternalFormat == GL_R32F)
			qglTexStorage2D(tex2D, 1, iInternalFormat, s->iWidth, s->iHeight);
		else
			qglTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_RGBA,
			(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);
	}
	s->iTextureColorFormat = iInternalFormat;

	qglFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s->s_hBackBufferTex, 0);
	qglBindTexture(tex2D, 0);
}

void R_GLFrameBufferDepthTexture(FBO_Container_t *s, GLuint iInternalFormat, qboolean multisample)
{
	int tex2D = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

	s->s_hBackBufferDepthTex = GL_GenTexture();
	qglBindTexture(tex2D, s->s_hBackBufferDepthTex);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(tex2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (multisample)
	{
		qglTexStorage2DMultisample(tex2D, gl_msaa_samples, iInternalFormat, s->iWidth, s->iHeight, GL_FALSE);
	}
	else
	{
		qglTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_DEPTH_COMPONENT,
			(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);
	}

	qglFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	qglBindTexture(tex2D, 0);
}

void R_GLFrameBufferDepthStencilTexture(FBO_Container_t *s, qboolean multisample)
{
	int tex2D = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

	s->s_hBackBufferDepthTex = GL_GenTexture();
	qglBindTexture(tex2D, s->s_hBackBufferDepthTex);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(tex2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexImage2D(tex2D, 0, GL_DEPTH24_STENCIL8, s->iWidth, s->iHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);

	qglFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	qglBindTexture(tex2D, 0);
}

int R_GLGenColorTextureHBAO(int w, int h)
{
	GLint swizzle[4] = { GL_RED,GL_GREEN,GL_ZERO,GL_ZERO };

	int texId = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, texId);
	qglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_FLOAT, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);

	return texId;
}

void R_GLFrameBufferColorTextureHBAO(FBO_Container_t *s)
{
	GLint swizzle[4] = { GL_RED,GL_GREEN,GL_ZERO,GL_ZERO };

	s->s_hBackBufferTex = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex);
	qglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, s->iWidth, s->iHeight);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex2 = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex2);
	qglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, s->iWidth, s->iHeight);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->iTextureColorFormat = GL_RG16F;

	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s->s_hBackBufferTex, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s->s_hBackBufferTex2, 0);
}

void R_GLFrameBufferColorTextureDeferred(FBO_Container_t *s)
{
	s->s_hBackBufferTex = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, s->iWidth, s->iHeight, 0, GL_RGB, GL_FLOAT, NULL);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex2 = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex2);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, s->iWidth, s->iHeight, 0, GL_RGB, GL_FLOAT, NULL);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex3 = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex3);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, s->iWidth, s->iHeight, 0, GL_RGB, GL_FLOAT, NULL);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex4 = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex4);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, s->iWidth, s->iHeight, 0, GL_RGB, GL_FLOAT, NULL);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->iTextureColorFormat = GL_RGB32F;

	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s->s_hBackBufferTex, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s->s_hBackBufferTex2, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, s->s_hBackBufferTex3, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, s->s_hBackBufferTex4, 0);
}

void GL_GenerateFBO(void)
{
	if (!gl_framebuffer_object)
		return;

	bNoStretchAspect = (gEngfuncs.CheckParm("-stretchaspect", NULL) == 0);

	if (gEngfuncs.CheckParm("-nomsaa", NULL))
		bDoMSAAFBO = false;

	if (!gl_msaa_support)
		bDoMSAAFBO = false;

	if (!gl_msaa_blit_support)
		bDoMSAAFBO = false;

	if (gEngfuncs.CheckParm("-nofbo", NULL))
		bDoScaledFBO = false;

	if (gEngfuncs.CheckParm("-directblit", NULL))
		bDoDirectBlit = true;

	if (gEngfuncs.CheckParm("-nodirectblit", NULL))
		bDoDirectBlit = false;

	if(!gl_float_buffer_support)
		bDoHDR = false;

	if (!qglGenFramebuffersEXT || !qglBindFramebufferEXT || !qglBlitFramebufferEXT)
		bDoScaledFBO = false;

	GL_ClearFBO(&s_MSAAFBO);
	GL_ClearFBO(&s_GBufferFBO);
	GL_ClearFBO(&s_BackBufferFBO);
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
	GL_ClearFBO(&s_CloakFBO);

	if(!bDoScaledFBO)
		bDoMSAAFBO = false;

	qglEnable(GL_TEXTURE_2D);

	GLuint iColorInternalFormat = GL_RGBA8;

	if(bDoHDR)
	{
		iColorInternalFormat = GL_RGBA16F;

		const char *s_HDRColor;
		if(g_pInterface->CommandLine->CheckParm("-hdrcolor", &s_HDRColor))
		{
			if(s_HDRColor && s_HDRColor[0] >= '0' && s_HDRColor[0] <= '9')
			{
				int i_HDRColor = atoi(s_HDRColor);
				if(i_HDRColor == 8)
					iColorInternalFormat = GL_RGBA8;
				else if(i_HDRColor == 16)
					iColorInternalFormat = GL_RGBA16F;
				else if(i_HDRColor == 32)
					iColorInternalFormat = GL_RGBA32F;
			}
		}
	}

	if (bDoMSAAFBO)
	{
		const char *s_Samples;
		gl_msaa_samples = 4;
		if(g_pInterface->CommandLine->CheckParm("-msaa", &s_Samples))
		{
			if(s_Samples && s_Samples[0] >= '0' && s_Samples[0] <= '9')
			{
				int i_Samples = atoi(s_Samples);
				if(i_Samples == 4)
					gl_msaa_samples = 4;
				else if(i_Samples == 8)
					gl_msaa_samples = 8;
				else if(i_Samples == 16)
					gl_msaa_samples = 16;
			}
		}
		s_MSAAFBO.iWidth = glwidth;
		s_MSAAFBO.iHeight = glheight;
		R_GLGenFrameBuffer(&s_MSAAFBO);
		R_GLFrameBufferColorTexture(&s_MSAAFBO, iColorInternalFormat, true);
		R_GLFrameBufferDepthTexture(&s_MSAAFBO, GL_DEPTH_COMPONENT24, true);

		if (s_MSAAFBO.s_hBackBufferFBO)
			qglEnable(GL_MULTISAMPLE);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_MSAAFBO);
			bDoMSAAFBO = false;
			gEngfuncs.Con_Printf("Error initializing MSAA frame buffer\n");
		}
	}
	else
	{
		gEngfuncs.Con_Printf("MSAA backbuffer rendering disabled.\n");
	}

	if (bDoScaledFBO)
	{
		s_BackBufferFBO.iWidth = glwidth;
		s_BackBufferFBO.iHeight = glheight;
		R_GLGenFrameBuffer(&s_BackBufferFBO);
		R_GLFrameBufferColorTexture(&s_BackBufferFBO, iColorInternalFormat, false);
		R_GLFrameBufferDepthTexture(&s_BackBufferFBO, GL_DEPTH_COMPONENT24, false);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_BackBufferFBO);
			gEngfuncs.Con_Printf("FBO backbuffer rendering disabled due to create error.\n");
		}
	}
	else
	{
		gEngfuncs.Con_Printf("FBO backbuffer rendering enabled.\n");
	}

	if (!s_BackBufferFBO.s_hBackBufferTex)
	{
		s_BackBufferFBO.s_hBackBufferTex = GL_GenTextureColorFormat(s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight, iColorInternalFormat);
		s_BackBufferFBO.s_hBackBufferDepthTex = GL_GenDepthTexture(s_BackBufferFBO.iWidth, s_BackBufferFBO.iHeight);
		s_BackBufferFBO.iWidth = glwidth;
		s_BackBufferFBO.iHeight = glheight;
		s_BackBufferFBO.iTextureColorFormat = iColorInternalFormat;
	}

	if (bDoScaledFBO)
	{
		s_DepthLinearFBO.iWidth = glwidth;
		s_DepthLinearFBO.iHeight = glheight;
		R_GLGenFrameBuffer(&s_DepthLinearFBO);
		R_GLFrameBufferColorTexture(&s_DepthLinearFBO, GL_R32F, false);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_DepthLinearFBO);
			gEngfuncs.Con_Printf("DepthLinear FBO rendering disabled due to create error.\n");
		}
	}
	if (!s_DepthLinearFBO.s_hBackBufferTex)
	{
		s_DepthLinearFBO.s_hBackBufferTex = GL_GenTextureColorFormat(glwidth, glheight, GL_R32F);	
		s_DepthLinearFBO.iWidth = glwidth;
		s_DepthLinearFBO.iHeight = glheight;
		s_DepthLinearFBO.iTextureColorFormat = GL_R32F;
	}

	if (bDoScaledFBO)
	{
		s_HBAOCalcFBO.iWidth = glwidth;
		s_HBAOCalcFBO.iHeight = glheight;
		R_GLGenFrameBuffer(&s_HBAOCalcFBO);
		R_GLFrameBufferColorTextureHBAO(&s_HBAOCalcFBO);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_HBAOCalcFBO);
			gEngfuncs.Con_Printf("HBAOCalc FBO rendering disabled due to create error.\n");
		}
	}

	if (!s_HBAOCalcFBO.s_hBackBufferTex)
	{
		s_HBAOCalcFBO.s_hBackBufferTex = R_GLGenColorTextureHBAO(glwidth, glheight);
		s_HBAOCalcFBO.s_hBackBufferTex2 = R_GLGenColorTextureHBAO(glwidth, glheight);
		s_HBAOCalcFBO.iWidth = glwidth;
		s_HBAOCalcFBO.iHeight = glheight;
		s_HBAOCalcFBO.iTextureColorFormat = GL_RG16F;
	}

	s_ShadowFBO.iWidth = glwidth;
	s_ShadowFBO.iHeight = glheight;
	if (bDoScaledFBO)
	{
		R_GLGenFrameBuffer(&s_ShadowFBO);
	}

	s_WaterFBO.iWidth = glwidth;
	s_WaterFBO.iHeight = glheight;
	if (bDoScaledFBO)
	{
		R_GLGenFrameBuffer(&s_WaterFBO);
	}

	int downW, downH;
	//DownSample FBO 1->1/4->1/16
	if(bDoHDR)
	{
		downW = glwidth;
		downH = glheight;
		for(int i = 0; i < DOWNSAMPLE_BUFFERS && bDoHDR; ++i)
		{
			downW >>= 1;
			downH >>= 1;
			s_DownSampleFBO[i].iWidth = downW;
			s_DownSampleFBO[i].iHeight = downH;
			if (bDoScaledFBO)
			{
				//fbo
				R_GLGenFrameBuffer(&s_DownSampleFBO[i]);
				//color
				R_GLFrameBufferColorTexture(&s_DownSampleFBO[i], iColorInternalFormat, false);

				if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					GL_FreeFBO(&s_DownSampleFBO[i]);
					gEngfuncs.Con_Printf("DownSample FBO #%d rendering disabled due to create error.\n", i);
				}
			}
			
			if (!s_DownSampleFBO[i].s_hBackBufferTex)
			{
				s_DownSampleFBO[i].s_hBackBufferTex = GL_GenTextureColorFormat(downW, downH, iColorInternalFormat);
				s_DownSampleFBO[i].iTextureColorFormat = iColorInternalFormat;
			}
		}
	}

	if (bDoScaledFBO)
	{
		s_GBufferFBO.iWidth = glwidth;
		s_GBufferFBO.iHeight = glheight;
		R_GLGenFrameBuffer(&s_GBufferFBO);
		R_GLFrameBufferColorTextureDeferred(&s_GBufferFBO);
		R_GLFrameBufferDepthTexture(&s_GBufferFBO, GL_DEPTH_COMPONENT24, false);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_GBufferFBO);
			gEngfuncs.Con_Printf("GBuffer backbuffer rendering disabled due to create error.\n");
		}
	}

	//Luminance FBO
	if(bDoHDR)
	{
		downW = glwidth;
		downH = glheight;
		while ((downH >> 1) >= 256)
		{
			downW >>= 1;
			downH >>= 1;
		}

		//64x64 16x16 4x4
		for(int i = 0; i < LUMIN_BUFFERS; ++i)
		{
			s_LuminFBO[i].iWidth = downW;
			s_LuminFBO[i].iHeight = downH;
			if (bDoScaledFBO)
			{
				R_GLGenFrameBuffer(&s_LuminFBO[i]);
				R_GLFrameBufferColorTexture(&s_LuminFBO[i], GL_R32F, false);

				if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					GL_FreeFBO(&s_LuminFBO[i]);
					gEngfuncs.Con_Printf("Luminance FBO #%d rendering disabled due to create error.\n", i);
				}
			}

			if(!s_LuminFBO[i].s_hBackBufferTex)
			{
				s_LuminFBO[i].s_hBackBufferTex = GL_GenTextureColorFormat(downW, downH, GL_R32F);
				s_LuminFBO[i].iTextureColorFormat = GL_R32F;
			}

			downW >>= 2;
			downH >>= 2;
		}
	}

	//Luminance 1x1 FBO
	if(bDoHDR)
	{
		for(int i = 0; i < LUMIN1x1_BUFFERS; ++i)
		{
			s_Lumin1x1FBO[i].iWidth = 1;
			s_Lumin1x1FBO[i].iHeight = 1;
			if (bDoScaledFBO)
			{
				R_GLGenFrameBuffer(&s_Lumin1x1FBO[i]);
				R_GLFrameBufferColorTexture(&s_Lumin1x1FBO[i], GL_R32F, false);

				if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					GL_FreeFBO(&s_Lumin1x1FBO[i]);
					gEngfuncs.Con_Printf("Luminance FBO #%d rendering disabled due to create error.\n", i);
				}
			}

			if (!s_Lumin1x1FBO[i].s_hBackBufferTex)
			{
				s_Lumin1x1FBO[i].s_hBackBufferTex = GL_GenTextureColorFormat(1, 1, GL_R32F);
				s_Lumin1x1FBO[i].iTextureColorFormat = GL_R32F;
			}
		}
	}

	//Bright Pass FBO
	if(bDoHDR)
	{
		s_BrightPassFBO.iWidth = (glwidth >> DOWNSAMPLE_BUFFERS);
		s_BrightPassFBO.iHeight = (glheight >> DOWNSAMPLE_BUFFERS);
		if (bDoScaledFBO)
		{
			R_GLGenFrameBuffer(&s_BrightPassFBO);
			R_GLFrameBufferColorTexture(&s_BrightPassFBO, iColorInternalFormat, false);

			if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				GL_FreeFBO(&s_BrightPassFBO);
				gEngfuncs.Con_Printf("BrightPass FBO rendering disabled due to create error.\n");
			}
		}

		if (!s_BrightPassFBO.s_hBackBufferTex)
		{
			s_BrightPassFBO.s_hBackBufferTex = GL_GenTextureColorFormat(glwidth >> DOWNSAMPLE_BUFFERS, glheight >> DOWNSAMPLE_BUFFERS, iColorInternalFormat);
			s_BrightPassFBO.iTextureColorFormat = iColorInternalFormat;
		}
	}

	//Blur FBO
	if(bDoHDR)
	{
		downW = glwidth >> DOWNSAMPLE_BUFFERS;
		downH = glheight >> DOWNSAMPLE_BUFFERS;

		for(int i = 0; i < BLUR_BUFFERS; ++i)
		{
			for(int j = 0; j < 2; ++j)
			{
				s_BlurPassFBO[i][j].iWidth = downW;
				s_BlurPassFBO[i][j].iHeight = downH;

				if (bDoScaledFBO)
				{
					R_GLGenFrameBuffer(&s_BlurPassFBO[i][j]);
					R_GLFrameBufferColorTexture(&s_BlurPassFBO[i][j], iColorInternalFormat, false);

					if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
					{
						GL_FreeFBO(&s_BlurPassFBO[i][j]);
						gEngfuncs.Con_Printf("Blur FBO #%d rendering disabled due to create error.\n", i);
					}
				}

				if (!s_BlurPassFBO[i][j].s_hBackBufferTex)
				{
					s_BlurPassFBO[i][j].s_hBackBufferTex = GL_GenTextureColorFormat(downW, downH, iColorInternalFormat);
					s_BlurPassFBO[i][j].iTextureColorFormat = iColorInternalFormat;
				}
			}
			downW >>= 1;
			downH >>= 1;
		}
	}

	if(bDoHDR)
	{
		s_BrightAccumFBO.iWidth = glwidth >> DOWNSAMPLE_BUFFERS;
		s_BrightAccumFBO.iHeight = glheight >> DOWNSAMPLE_BUFFERS;

		if (bDoScaledFBO)
		{
			R_GLGenFrameBuffer(&s_BrightAccumFBO);
			R_GLFrameBufferColorTexture(&s_BrightAccumFBO, iColorInternalFormat, false);

			if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				GL_FreeFBO(&s_BrightAccumFBO);
				gEngfuncs.Con_Printf("Bright accumulate FBO #%d rendering disabled due to create error.\n");
			}
		}

		if (!s_BrightAccumFBO.s_hBackBufferTex)
		{
			s_BrightAccumFBO.s_hBackBufferTex = GL_GenTextureColorFormat(glwidth >> DOWNSAMPLE_BUFFERS, glheight >> DOWNSAMPLE_BUFFERS, iColorInternalFormat);
			s_BrightAccumFBO.iTextureColorFormat = iColorInternalFormat;
		}
	}

	if(bDoHDR)
	{
		s_ToneMapFBO.iWidth = glwidth;
		s_ToneMapFBO.iHeight = glheight;

		if (bDoScaledFBO)
		{
			R_GLGenFrameBuffer(&s_ToneMapFBO);
			R_GLFrameBufferColorTexture(&s_ToneMapFBO, GL_RGBA8, false);

			if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				GL_FreeFBO(&s_ToneMapFBO);
				gEngfuncs.Con_Printf("Tone mapping FBO #%d rendering disabled due to create error.\n");
			}
		}

		if (!s_ToneMapFBO.s_hBackBufferTex)
		{
			s_ToneMapFBO.s_hBackBufferTex = GL_GenTextureColorFormat(s_ToneMapFBO.iWidth, s_ToneMapFBO.iHeight, GL_RGBA8);
			s_ToneMapFBO.iTextureColorFormat = GL_RGBA8;
		}
	}

	if (bDoScaledFBO)
	{
		s_CloakFBO.iWidth = glwidth;
		s_CloakFBO.iHeight = glheight;

		//fbo
		R_GLGenFrameBuffer(&s_CloakFBO);
		//color
		R_GLFrameBufferColorTexture(&s_CloakFBO, GL_RGBA8, false);

		if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_CloakFBO);
			gEngfuncs.Con_Printf("Cloak FBO rendering disabled due to create error.\n");
		}
	}

	qglBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}

void GL_Init(void)
{
	QGL_Init();
	GL_GenerateFBO();
	GL_InitShaders();

	if (gl_mtexable && !(*gl_mtexable))
	{
		Sys_ErrorEx("Multitexture extension must be enabled! please remove -nomtex from launch parameters and try again.");
	}
}

void GL_Shutdown(void)
{
	GL_FreeShaders();

	GL_FreeFBO(&s_MSAAFBO);
	GL_FreeFBO(&s_BackBufferFBO);
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

	if (s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	}

	r_wsurf_drawcall = 0;
	r_studio_drawcall = 0;
	
	qglClearColor(0.0, 0.0, 0.0, 1.0);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void R_PreRenderView()
{
	if (!r_refdef->onlyClientDraws)
	{
		(*c_alias_polys) = 0;
		(*c_brush_polys) = 0;
		if (r_water && r_water->value && waters_active)
		{
			R_RenderWaterView();
		}
		if (r_shadow && r_shadow->value)
		{
			R_RenderShadowMap();
		}
		saved_c_alias_polys = (*c_alias_polys);
		saved_c_brush_polys = (*c_brush_polys);
	}

	if (s_BackBufferFBO.s_hBackBufferFBO)
	{
		if (s_MSAAFBO.s_hBackBufferFBO)
			qglBindFramebufferEXT(GL_FRAMEBUFFER, s_MSAAFBO.s_hBackBufferFBO);
		else
			qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	}	
}

void R_PostRenderView()
{
	if (s_BackBufferFBO.s_hBackBufferFBO)
	{
		if (s_MSAAFBO.s_hBackBufferFBO)
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
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	}
	else
	{
		R_DoFXAA();
		R_DoHDR();
	}

	if (r_speeds->value)
	{
		gEngfuncs.Con_Printf("%d wsurf drawcall, %d studio drawcall\n", r_wsurf_drawcall, r_studio_drawcall);
	}
}

void R_RenderView_SvEngine(int a1)
{
	if (a1 == 0)
	{
		R_PreRenderView();

		gRefFuncs.R_RenderView_SvEngine(a1);

		R_PostRenderView();
	}
	else
	{
		gRefFuncs.R_RenderView_SvEngine(a1);
	}
}

void R_RenderView(void)
{
	R_PreRenderView();

	gRefFuncs.R_RenderView();

	R_PostRenderView();
}

void R_RenderScene(void)
{
	gRefFuncs.R_RenderScene();

	if (!drawreflect && !drawrefract)
	{
		if (s_MSAAFBO.s_hBackBufferFBO)
		{
			for (int sampleIndex = 0; sampleIndex < max(1, gl_msaa_samples); sampleIndex++)
			{
				if (!R_DoSSAO(sampleIndex))
				{
					break;
				}
			}
		}
		else
		{
			R_DoSSAO(-1);
		}
	}

	if (!r_refdef->onlyClientDraws)
	{
		(*c_alias_polys) += saved_c_alias_polys;
		(*c_brush_polys) += saved_c_brush_polys;
	}
}

void GL_EndRendering(void)
{
	GLuint save_backbuffer_fbo = 0;
	if (gl_backbuffer_fbo)
	{
		save_backbuffer_fbo = *gl_backbuffer_fbo;
		*gl_backbuffer_fbo = 0;
	}

	if(s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglClearColor(0, 0, 0, 0);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		if ( bNoStretchAspect )
		{
			if ( windowAspect < videoAspect )
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

		if ( bDoDirectBlit )
		{
			qglBlitFramebufferEXT(0, 0, glwidth, glheight, dstX, dstY, dstX2, dstY2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
		else
		{
			qglDisable(GL_BLEND);
			qglDisable(GL_LIGHTING);
			qglDisable(GL_DEPTH_TEST);
			qglDisable(GL_ALPHA_TEST);
			qglDisable(GL_CULL_FACE);
			qglMatrixMode(GL_PROJECTION);
			qglPushMatrix();
			qglLoadIdentity();
			qglOrtho(0.0, windowW, windowH, 0.0, -1.0, 1.0);
			qglMatrixMode(GL_MODELVIEW);
			qglPushMatrix();
			qglLoadIdentity();
			qglViewport(0, 0, windowW, windowH);
			qglColor4f(1, 1, 1, 1);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferTex);

			qglBegin(GL_QUADS);
			qglTexCoord2f(0, 1);
			qglVertex3f(dstX, dstY, 0);
			qglTexCoord2f(1, 1);
			qglVertex3f(dstX2, dstY, 0);
			qglTexCoord2f(1, 0);
			qglVertex3f(dstX2, dstY2, 0);
			qglTexCoord2f(0, 0);
			qglVertex3f(dstX, dstY2, 0);
			qglEnd();

			qglMatrixMode(GL_PROJECTION);
			qglPopMatrix();
			qglMatrixMode(GL_MODELVIEW);
			qglPopMatrix();
		}
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);
	}

	//this will call VID_FlipScreen for us.
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

	const char *s_ansio;
	gl_force_ansio = 0;
	if(g_pInterface->CommandLine->CheckParm("-ansio", &s_ansio))
	{
		gl_force_ansio = gl_max_ansio;
		if(s_ansio && s_ansio[0] && s_ansio[0] >= '0' && s_ansio[0] <= '9')
		{
			float f_ansio = atof(s_ansio);
			gl_force_ansio = max(min(f_ansio, gl_max_ansio), 1);
		}
	}

	v_texgamma = gEngfuncs.pfnGetCvarPointer("texgamma");
	v_lightgamma = gEngfuncs.pfnGetCvarPointer("lightgamma");
	v_brightness = gEngfuncs.pfnGetCvarPointer("brightness");
	v_gamma = gEngfuncs.pfnGetCvarPointer("gamma");
	v_lambert = gEngfuncs.pfnGetCvarPointer("lambert");

	cl_righthand = gEngfuncs.pfnGetCvarPointer("cl_righthand");
}

void R_Init(void)
{
	R_InitCvars();
	R_InitWater();
	R_InitStudio();
	R_InitShadow();
	R_InitWSurf();
	R_InitGLHUD();
	R_Init3DSky();
	R_InitCloak();
	R_InitLight();

	Draw_Init();
}

void R_Shutdown(void)
{
	R_FreeShadow();
	R_FreeWater();
	R_ShutdownLight();
	R_ShutdownWSurf();
}

void R_ForceCVars(qboolean mp)
{
	if (drawreflect || drawrefract)
		return;

	gRefFuncs.R_ForceCVars(mp);
}

void R_NewMap(void)
{
	gRefFuncs.R_NewMap();

	r_worldentity = gEngfuncs.GetEntityByIndex(0);
	r_worldmodel = r_worldentity->model;

	memset(&r_params, 0, sizeof(r_params));

	R_ClearWater();
	R_Clear3DSky();
	R_VidInitWSurf();
}

mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model)
{
	if (drawreflect && model == r_worldmodel && 0 == VectorCompare(p, r_refdef->vieworg))
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