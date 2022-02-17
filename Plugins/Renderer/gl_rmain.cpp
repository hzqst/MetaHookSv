#include "gl_local.h"
#include "pm_defs.h"
#include <sstream>

ref_funcs_t gRefFuncs;

refdef_t r_refdef;
ref_params_t r_params;
refdef_GoldSrc_t *r_refdef_GoldSrc = NULL;
refdef_SvEngine_t *r_refdef_SvEngine = NULL;
float *scrfov = NULL;
float r_xfov;
float r_yfov;
float r_screenaspect;

float gldepthmin, gldepthmax;

cl_entity_t *r_worldentity = NULL;
model_t *r_worldmodel = NULL;
model_t *r_playermodel = NULL;
RECT *window_rect;

float *videowindowaspect;
float *windowvideoaspect;

int *cl_numvisedicts;
cl_entity_t **cl_visedicts;
cl_entity_t **currententity;
int *numTransObjs;
int *maxTransObjs;
transObjRef **transObjects;
mleaf_t **r_viewleaf;
mleaf_t **r_oldviewleaf;

GLint r_viewport[4];

vec_t *vup;
vec_t *vpn;
vec_t *vright;
vec_t *r_origin;
vec_t *modelorg;
vec_t *r_entorigin;
float *r_world_matrix;
float *r_projection_matrix;
float *gWorldToScreen;
float *gScreenToWorld;
overviewInfo_t *gDevOverview;
mplane_t *frustum;

int *g_bUserFogOn;
float *g_UserFogColor;
float *g_UserFogDensity;
float *g_UserFogStart;
float *g_UserFogEnd;

int *r_framecount;
int *r_visframecount;

void *cl_frames;
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
int(*rtable)[20][20];

model_t *mod_known = NULL;
int *mod_numknown = NULL;

int gl_max_ubo_size = 0;
int gl_max_texture_size = 0;
float gl_max_ansio = 0;

int *gl_msaa_fbo = 0;
int *gl_backbuffer_fbo = 0;
int *gl_mtexable = 0;
qboolean *mtexenabled = 0;

bool g_SvEngine_DrawPortalView = 0;

float r_identity_matrix[4][4] = {
	{1.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
};

float r_entity_matrix[4][4];
float r_entity_color[4];

bool r_draw_shadowcaster = false;
bool r_draw_opaque = false;
bool r_draw_oitblend = false;
bool r_draw_legacysprite = false;

int r_draw_pass = 0;

int glx = 0;
int gly = 0;
int glwidth = 0;
int glheight = 0;

FBO_Container_t s_GBufferFBO;
FBO_Container_t s_BackBufferFBO;
FBO_Container_t s_BackBufferFBO2;
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

bool bNoStretchAspect = false;
bool bUseBindless = true;
bool bUseOITBlend = false;
bool bVerticalFov = false;

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

cvar_t *r_adjust_fov = NULL;
cvar_t *r_vertical_fov = NULL;
cvar_t *gl_profile = NULL;
cvar_t *dev_overview_color = NULL;

int R_GetDrawPass(void)
{
	return r_draw_pass;
}

qboolean R_CullBox(vec3_t mins, vec3_t maxs)
{
	if(r_draw_shadowcaster)
		return false;

	if ((*currententity)->model && (*currententity)->model->type == mod_studio && (*currententity)->curstate.scale != 1.0f)
	{
		if ((*currententity)->curstate.scale > 8.0f)
			return false;
	}

	return gRefFuncs.R_CullBox(mins, maxs);
}

void R_RotateForEntity(float *origin, cl_entity_t *e)
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

	float entity_matrix[4][4];
	memcpy(entity_matrix, r_identity_matrix, sizeof(r_identity_matrix));
	Matrix4x4_CreateFromEntity(entity_matrix, angles, modelpos, 1);
	Matrix4x4_Transpose(r_entity_matrix, entity_matrix);
}

float GlowBlend(cl_entity_t *entity)
{
	return gRefFuncs.GlowBlend(entity);
}

int CL_FxBlend(cl_entity_t *entity)
{
	//Hack for R_DrawSpriteModel
	//if (entity->model && entity->model->type == mod_sprite && entity->curstate.rendermode == kRenderNormal)
	//{
	//	return 255;
	//}

	return gRefFuncs.CL_FxBlend(entity);
}

const int		ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
const int		ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
const int		ramp3[8] = { 0x6d, 0x6b, 6, 5, 4, 3 };

#define SPARK_COLORCOUNT	9
const int		gSparkRamp[SPARK_COLORCOUNT] = { 0xfe, 0xfd, 0xfc, 0x6f, 0x6e, 0x6d, 0x6c, 0x67, 0x60 };

const color24 gTracerColors[] =
{
	{ 255, 255, 255 },		// White
	{ 255, 0, 0 },			// Red
	{ 0, 255, 0 },			// Green
	{ 0, 0, 255 },			// Blue
	{ 0, 0, 0 },			// Tracer default, filled in from cvars, etc.
	{ 255, 167, 17 },		// Yellow-orange sparks
	{ 255, 130, 90 },		// Yellowish streaks (garg)
	{ 55, 60, 144 },		// Blue egon streak
	{ 255, 130, 90 },		// More Yellowish streaks (garg)
	{ 255, 140, 90 },		// More Yellowish streaks (garg)
	{ 200, 130, 90 },		// More red streaks (garg)
	{ 255, 120, 70 },		// Darker red streaks (garg)
};

void R_DrawParticlesNew(void)
{
	vec3_t			up, right;
	float			scale;

	GL_Bind((*particletexture));
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int LegacySpriteProgramState = 0;

	if (r_draw_oitblend)
	{
		LegacySpriteProgramState |= SPRITE_OIT_ALPHA_BLEND_ENABLED;
	}

	if (!drawgbuffer && r_fog_mode == GL_LINEAR)
	{
		LegacySpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
	}
	else if (!drawgbuffer && r_fog_mode == GL_EXP)
	{
		LegacySpriteProgramState |= SPRITE_EXP_FOG_ENABLED;
	}
	else if (!drawgbuffer && r_fog_mode == GL_EXP2)
	{
		LegacySpriteProgramState |= SPRITE_EXP2_FOG_ENABLED;
	}

	if (r_draw_pass == r_draw_reflect && curwater)
	{
		LegacySpriteProgramState |= SPRITE_CLIP_ENABLED;
	}

	R_UseLegacySpriteProgram(LegacySpriteProgramState, NULL);

	glBegin(GL_TRIANGLES);

	VectorScale(vup, 1.5, up);
	VectorScale(vright, 1.5, right);

	float frametime = (*cl_time) - (*cl_oldtime);
	float time3 = frametime * 15;
	float time2 = frametime * 10; // 15;
	float time1 = frametime * 5;
	float grav = frametime * (r_params.movevars ? r_params.movevars->gravity : 800) * 0.05;
	float dvel = 4 * frametime;

	gRefFuncs.R_FreeDeadParticles(&(*active_particles));

	for (auto p = (*active_particles); p; p = p->next)
	{
		if (p->type != pt_blob)
		{
			// hack a scale up to keep particles from disapearing
			scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
				+ (p->org[2] - r_origin[2])*vpn[2];
			if (scale < 20)
				scale = 1;
			else
				scale = 1 + scale * 0.004;

			auto pb = &(*host_basepal)[(int)p->color * 4];

			byte rgba[4];
			rgba[0] = pb[2];
			rgba[1] = pb[1];
			rgba[2] = pb[0];
			rgba[3] = 255;

			glColor3ubv(rgba);
			glTexCoord2f(0, 0);
			glVertex3fv(p->org);
			glTexCoord2f(1, 0);
			glVertex3f(p->org[0] + up[0] * scale, p->org[1] + up[1] * scale, p->org[2] + up[2] * scale);
			glTexCoord2f(0, 1);
			glVertex3f(p->org[0] + right[0] * scale, p->org[1] + right[1] * scale, p->org[2] + right[2] * scale);
		}

		if (p->type != pt_clientcustom)
		{
			p->org[0] += p->vel[0] * frametime;
			p->org[1] += p->vel[1] * frametime;
			p->org[2] += p->vel[2] * frametime;
		}

		switch (p->type)
		{
		case pt_static:
			break;

		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
			{
				p->color = ramp3[(int)p->ramp];
				p->packedColor = 0;
			}
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >= 8)
				p->die = -1;
			else
			{
				p->color = ramp1[(int)p->ramp];
				p->packedColor = 0;
			}
			for (int i = 0; i < 3; i++)
				p->vel[i] += p->vel[i] * dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >= 8)
				p->die = -1;
			else
			{
				p->color = ramp2[(int)p->ramp];
				p->packedColor = 0;
			}
			for (int i = 0; i < 3; i++)
				p->vel[i] -= p->vel[i] * frametime;
			p->vel[2] -= grav;
			break;

		case pt_blob:
		case pt_blob2:
			p->ramp += time2;
			if (p->ramp >= SPARK_COLORCOUNT)
				p->ramp = 0;

			p->color = gSparkRamp[(int)p->ramp];
			p->packedColor = 0;

			for (int i = 0; i < 3; i++)
				p->vel[i] += p->vel[i] * frametime * 0.5;
			p->vel[2] -= grav * 5;
			p->type = gEngfuncs.pfnRandomLong(0, 3) ? pt_blob : pt_blob2;
			break;

		case pt_grav:
			p->vel[2] -= grav * 20;
			break;

		case pt_slowgrav:
			p->vel[2] -= grav;
			break;

		case pt_vox_grav:
			p->vel[2] -= grav * 8;
			break;

		case pt_vox_slowgrav:
			p->vel[2] -= grav * 4;
			break;

		case pt_clientcustom:

			if (p->callback)
				p->callback(p, frametime);

			break;
		}
	}

	glEnd();

	gRefFuncs.R_TracerDraw();
	gRefFuncs.R_BeamDrawList();

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
}

void R_DrawParticles(void)
{
	return;
}

void triapi_Color4f(float x, float y, float z, float w)
{
	//glColor4f(x, y, z, w);
	gRefFuncs.triapi_Color4f(x, y, z, w);
}

void triapi_RenderMode(int mode)
{
	gRefFuncs.triapi_RenderMode(mode);

	switch (mode)
	{
	case kRenderNormal:
	{
		
		break;
	}

	case kRenderTransAdd:
	{
		R_SetGBufferBlend(GL_ONE, GL_ONE);
		if (r_draw_legacysprite)
		{
			int LegacySpriteProgramState = r_draw_oitblend ? SPRITE_OIT_ADDITIVE_BLEND_ENABLED : SPRITE_ADDITIVE_BLEND_ENABLED;

			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				LegacySpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
			}
			else if (!drawgbuffer && r_fog_mode == GL_EXP)
			{
				LegacySpriteProgramState |= SPRITE_EXP_FOG_ENABLED;
			}
			else if (!drawgbuffer && r_fog_mode == GL_EXP2)
			{
				LegacySpriteProgramState |= SPRITE_EXP2_FOG_ENABLED;
			}

			if (r_draw_pass == r_draw_reflect && curwater)
			{
				LegacySpriteProgramState |= SPRITE_CLIP_ENABLED;
			}

			R_UseLegacySpriteProgram(LegacySpriteProgramState, NULL);
		}
		break;
	}

	case kRenderTransAlpha:
	case kRenderTransColor:
	case kRenderTransTexture:
	{
		R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (r_draw_legacysprite)
		{
			int LegacySpriteProgramState = r_draw_oitblend ? SPRITE_OIT_ALPHA_BLEND_ENABLED : SPRITE_ALPHA_BLEND_ENABLED;

			if (r_draw_pass == r_draw_reflect && curwater)
			{
				LegacySpriteProgramState |= SPRITE_CLIP_ENABLED;
			}

			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				LegacySpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
			}
			else if (!drawgbuffer && r_fog_mode == GL_EXP)
			{
				LegacySpriteProgramState |= SPRITE_EXP_FOG_ENABLED;
			}
			else if (!drawgbuffer && r_fog_mode == GL_EXP2)
			{
				LegacySpriteProgramState |= SPRITE_EXP2_FOG_ENABLED;
			}

			R_UseLegacySpriteProgram(LegacySpriteProgramState, NULL);
		}
		break;
	}
	}
}

void R_DrawTEntitiesOnList(int onlyClientDraw)
{
	if (!r_drawentities->value)
		return;

	if (onlyClientDraw)
		return;

	static glprofile_t profile_DrawTEntitiesOnList;
	GL_BeginProfile(&profile_DrawTEntitiesOnList, "R_DrawTEntitiesOnList");

	if (bUseOITBlend)
	{
		glColorMask(0, 0, 0, 0);

		R_ClearOITBuffer();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		r_draw_oitblend = true;

		r_draw_legacysprite = true;

		gRefFuncs.R_DrawTEntitiesOnList(onlyClientDraw);
		
		R_DrawParticlesNew();

		r_draw_legacysprite = false;

		r_draw_oitblend = false;

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		glColorMask(1, 1, 1, 1);

		R_BlendOITBuffer();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	else
	{
		r_draw_legacysprite = true;

		gRefFuncs.R_DrawTEntitiesOnList(onlyClientDraw);

		R_DrawParticlesNew();

		r_draw_legacysprite = false;
	}

	GL_UseProgram(0);

	GL_EndProfile(&profile_DrawTEntitiesOnList);
}

void R_AddTEntity(cl_entity_t *ent)
{
	if (!ent->model)
		return;

	//Brush models with kRenderTransAlpha are forced to be opaque, except renderamt = 0 
	if (ent->model->type == mod_brush && ent->curstate.rendermode == kRenderTransAlpha)
	{
		if (ent->curstate.renderamt == 0)
			return;

		(*currententity) = ent;
		R_DrawCurrentEntity(false);

		return;
	}
	
	if (bUseOITBlend)
	{
		if ((*numTransObjs) >= (*maxTransObjs))
		{
			g_pMetaHookAPI->SysError("R_AddTEntity: Too many objects");
			return;
		}

		(*transObjects)[(*numTransObjs)].pEnt = ent;
		(*transObjects)[(*numTransObjs)].distance = 0;
		(*numTransObjs)++;
	}
	else
	{
		float dist;
		vec3_t v;

		if ((*numTransObjs) >= (*maxTransObjs))
		{
			gEngfuncs.Con_Printf("R_AddTEntity: Too many objects");
			return;
		}

		if (!ent->model || ent->model->type != mod_brush || ent->curstate.rendermode != kRenderTransAlpha)
		{
			VectorAdd(ent->model->mins, ent->model->maxs, v);
			VectorScale(v, 0.5, v);
			VectorAdd(v, ent->origin, v);
			VectorSubtract(r_origin, v, v);
			dist = DotProduct(v, v);
		}
		else
		{
			dist = 1000000000;
		}

		int i;

		for (i = (*numTransObjs); i > 0; i--)
		{
			if ((*transObjects)[i - 1].distance >= dist)
				break;

			(*transObjects)[i].pEnt = (*transObjects)[i - 1].pEnt;
			(*transObjects)[i].distance = (*transObjects)[i - 1].distance;
		}

		(*transObjects)[i].pEnt = ent;
		(*transObjects)[i].distance = dist;
		(*numTransObjs)++;
	}
}

entity_state_t *R_GetPlayerState(int index)
{
	return ((entity_state_t *)((char *)cl_frames + size_of_frame * ((*cl_parsecount) & 63) + sizeof(entity_state_t) * index));
}

void R_DrawCurrentEntity(bool bTransparent)
{
	if (bTransparent)
	{
		glDisable(GL_FOG);

		(*r_blend) = CL_FxBlend((*currententity));

		if ((*r_blend) <= 0)
			return;

		(*r_blend) = (*r_blend) / 255.0;

		if ((*currententity)->curstate.rendermode == kRenderGlow && (*currententity)->model->type != mod_sprite)
			gEngfuncs.Con_DPrintf("Non-sprite set to glow!\n");
	}

	switch ((*currententity)->model->type)
	{
	case mod_sprite:
	{
		if ((*currententity)->curstate.body)
		{
			float *pAttachment;

			pAttachment = R_GetAttachmentPoint((*currententity)->curstate.skin, (*currententity)->curstate.body);
			VectorCopy(pAttachment, r_entorigin);
		}
		else
		{
			VectorCopy((*currententity)->origin, r_entorigin);
		}

		if (bTransparent)
		{
			if ((*currententity)->curstate.rendermode == kRenderGlow)
				(*r_blend) *= GlowBlend((*currententity));
		}
		else
		{
			(*r_blend) = 1;
		}

		if ((*r_blend) != 0)
			R_DrawSpriteModel((*currententity));

		break;
	}
	case mod_brush:
	{
		if (bTransparent)
		{
			if ((*g_bUserFogOn))
			{
				if ((*currententity)->curstate.rendermode != kRenderGlow && (*currententity)->curstate.rendermode != kRenderTransAdd)
					glEnable(GL_FOG);
			}
		}

		R_DrawBrushModel((*currententity));
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
				bool bFound = false;

				for (int j = 0; j < (*cl_numvisedicts); j++)
				{
					if (cl_visedicts[j]->index == (*currententity)->curstate.aiment)
					{
						auto save_currententity = (*currententity);
						(*currententity) = cl_visedicts[j];

						if ((*currententity)->player)
						{
							(*gpStudioInterface)->StudioDrawPlayer(0, R_GetPlayerState((*currententity)->index));
						}
						else
						{
							(*gpStudioInterface)->StudioDrawModel(0);
						}

						(*currententity) = save_currententity;

						bFound = true;

						break;
					}
				}

				if (!bFound)
					break;
			}

			(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS);
		}

		break;
	}
	}
}

void R_SetRenderMode(cl_entity_t *pEntity)
{
	switch (pEntity->curstate.rendermode)
	{
	case kRenderNormal:
	{
		r_entity_color[0] = 1;
		r_entity_color[1] = 1;
		r_entity_color[2] = 1;
		r_entity_color[3] = 1;
		glDisable(GL_BLEND);
		break;
	}

	case kRenderTransColor:
	{
		r_entity_color[0] = (*currententity)->curstate.rendercolor.r / 255.0;
		r_entity_color[1] = (*currententity)->curstate.rendercolor.g / 255.0;
		r_entity_color[2] = (*currententity)->curstate.rendercolor.b / 255.0;
		r_entity_color[3] = (*r_blend);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}

	case kRenderTransAdd:
	{
		r_entity_color[0] = (*r_blend);
		r_entity_color[1] = (*r_blend);
		r_entity_color[2] = (*r_blend);
		r_entity_color[3] = 1;

		glBlendFunc(GL_ONE, GL_ONE);
		glDepthMask(0);
		glEnable(GL_BLEND);
		R_SetGBufferBlend(GL_ONE, GL_ONE);
		break;
	}

	case kRenderTransAlpha:
	{
		r_entity_color[0] = 1;
		r_entity_color[1] = 1;
		r_entity_color[2] = 1;
		r_entity_color[3] = 1;

		glEnable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
		glAlphaFunc(GL_GREATER, gl_alphamin->value);
		
		break;
	}

	default:
	{
		r_entity_color[0] = 1;
		r_entity_color[1] = 1;
		r_entity_color[2] = 1;
		r_entity_color[3] = (*r_blend);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(0);
		glEnable(GL_BLEND);
		R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}
	}
}

void R_DrawViewModel(void)
{
	float lightvec[3];

	lightvec[0] = -1;
	lightvec[1] = 0;
	lightvec[2] = 0;

	(*currententity) = cl_viewent;

	if (!r_drawviewmodel->value ||
		gExportfuncs.CL_IsThirdPerson() ||
		chase_active->value ||
		(*envmap) ||
		!r_drawentities->value ||
		cl_stats[0] <= 0 ||
		!(*currententity)->model ||
		r_params.viewentity > r_params.maxclients)
	{
		auto c = R_LightPoint((*currententity)->origin);
		(*cl_light_level) = (c.r + c.g + c.b) / 3;
		return;
	}

	glDepthRange(0, 0.3);

	switch ((*currententity)->model->type)
	{
	case mod_studio:
	{
		if (!(*cl_weaponstarttime))
			(*cl_weaponstarttime) = (*cl_time);

		hud_player_info_t hudPlayerInfo;
		gEngfuncs.pfnGetPlayerInfo(r_params.playernum + 1, &hudPlayerInfo);

		(*currententity)->curstate.frame = 0;
		(*currententity)->curstate.framerate = 1;
		(*currententity)->curstate.sequence = (*cl_weaponsequence);
		(*currententity)->curstate.animtime = (*cl_weaponstarttime);
		(*currententity)->curstate.colormap = ((hudPlayerInfo.topcolor) % 0xFFFF) | ((hudPlayerInfo.bottomcolor << 8) % 0xFFFF);

		auto c = R_LightPoint((*currententity)->origin);

		if (r_shadows)
		{
			auto oldShadows = r_shadows->value;
			r_shadows->value = 0;
			(*cl_light_level) = (c.r + c.g + c.b) / 3;
			(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER);
			r_shadows->value = oldShadows;
		}
		else
		{
			(*cl_light_level) = (c.r + c.g + c.b) / 3;
			(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER);
		}
		break;
	}

	case mod_brush:
	{
		R_DrawBrushModel((*currententity));
		break;
	}
	}

	glDepthRange(0, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void R_PolyBlend(void)
{
	gRefFuncs.R_PolyBlend();
}

void S_ExtraUpdate(void)
{
	gRefFuncs.S_ExtraUpdate();
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

void MYgluPerspectiveV(double fovx, double aspect, double zNear, double zFar)
{
	auto right = tan(fovx * (M_PI / 360.0)) * zNear;
	auto top = right * aspect;
	glFrustum(-right, right, -top, top, zNear, zFar);

	r_znear = zNear;
	r_zfar = zFar;
	r_ortho = false;

	vec3_t farplane;
	VectorMA((*r_refdef.vieworg), zNear, vpn, farplane);

	VectorMA(farplane, -right, vright, r_frustum_origin[0]);
	VectorMA(r_frustum_origin[0], -top, vup, r_frustum_origin[0]);
	VectorSubtract(r_frustum_origin[0], (*r_refdef.vieworg), r_frustum_vec[0]);
	VectorNormalize(r_frustum_vec[0]);

	VectorMA(farplane, -right, vright, r_frustum_origin[1]);
	VectorMA(r_frustum_origin[1], top, vup, r_frustum_origin[1]);
	VectorSubtract(r_frustum_origin[1], (*r_refdef.vieworg), r_frustum_vec[1]);
	VectorNormalize(r_frustum_vec[1]);

	VectorMA(farplane, right, vright, r_frustum_origin[2]);
	VectorMA(r_frustum_origin[2], top, vup, r_frustum_origin[2]);
	VectorSubtract(r_frustum_origin[2], (*r_refdef.vieworg), r_frustum_vec[2]);
	VectorNormalize(r_frustum_vec[2]);

	VectorMA(farplane, right, vright, r_frustum_origin[3]);
	VectorMA(r_frustum_origin[3], -top, vup, r_frustum_origin[3]);
	VectorSubtract(r_frustum_origin[3], (*r_refdef.vieworg), r_frustum_vec[3]);
	VectorNormalize(r_frustum_vec[3]);
 }

void MYgluPerspectiveH(double fovy, double aspect, double zNear, double zFar)
{
	auto top = tan(fovy * (M_PI / 360.0)) * zNear;
	auto right = top * aspect;
	glFrustum(-right, right, -top, top, zNear, zFar);

	r_znear = zNear;
	r_zfar = zFar;
	r_ortho = false;

	vec3_t farplane;
	VectorMA((*r_refdef.vieworg), zNear, vpn, farplane);

	VectorMA(farplane, -right, vright, r_frustum_origin[0]);
	VectorMA(r_frustum_origin[0], -top, vup, r_frustum_origin[0]);
	VectorSubtract(r_frustum_origin[0], (*r_refdef.vieworg), r_frustum_vec[0]);
	VectorNormalize(r_frustum_vec[0]);

	VectorMA(farplane, -right, vright, r_frustum_origin[1]);
	VectorMA(r_frustum_origin[1], top, vup, r_frustum_origin[1]);
	VectorSubtract(r_frustum_origin[1], (*r_refdef.vieworg), r_frustum_vec[1]);
	VectorNormalize(r_frustum_vec[1]);

	VectorMA(farplane, right, vright, r_frustum_origin[2]);
	VectorMA(r_frustum_origin[2], top, vup, r_frustum_origin[2]);
	VectorSubtract(r_frustum_origin[2], (*r_refdef.vieworg), r_frustum_vec[2]);
	VectorNormalize(r_frustum_vec[2]);

	VectorMA(farplane, right, vright, r_frustum_origin[3]);
	VectorMA(r_frustum_origin[3], -top, vup, r_frustum_origin[3]);
	VectorSubtract(r_frustum_origin[3], (*r_refdef.vieworg), r_frustum_vec[3]);
	VectorNormalize(r_frustum_vec[3]);
}

void GL_ClearFBO(FBO_Container_t *s)
{
	s->s_hBackBufferFBO = 0;
	s->s_hBackBufferCB = 0;
	s->s_hBackBufferDB = 0;
	s->s_hBackBufferTex = 0;
	s->s_hBackBufferTex2 = 0;
	s->s_hBackBufferDepthTex = 0;
	s->iWidth = s->iHeight = s->iTextureColorFormat = 0;
}

void GL_FreeFBO(FBO_Container_t *s)
{
	if (s->s_hBackBufferFBO)
		glDeleteFramebuffersEXT(1, &s->s_hBackBufferFBO);

	if (s->s_hBackBufferCB)
		glDeleteRenderbuffersEXT(1, &s->s_hBackBufferCB);

	if (s->s_hBackBufferDB)
		glDeleteRenderbuffersEXT(1, &s->s_hBackBufferDB);

	if (s->s_hBackBufferTex)
		glDeleteTextures(1, &s->s_hBackBufferTex);

	if (s->s_hBackBufferTex2)
		glDeleteTextures(1, &s->s_hBackBufferTex2);

	if (s->s_hBackBufferDepthTex)
		glDeleteTextures(1, &s->s_hBackBufferDepthTex);

	if (s->s_hBackBufferStencilView)
		glDeleteTextures(1, &s->s_hBackBufferStencilView);

	GL_ClearFBO(s);
}

void GL_GenerateFrameBuffers(void)
{
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

	glEnable(GL_TEXTURE_2D);

	s_BackBufferFBO.iWidth = glwidth;
	s_BackBufferFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_BackBufferFBO);
	GL_FrameBufferColorTexture(&s_BackBufferFBO, GL_RGBA16F);
	GL_FrameBufferDepthTexture(&s_BackBufferFBO, GL_DEPTH24_STENCIL8);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BackBufferFBO);
		g_pMetaHookAPI->SysError("Failed to initialize backbuffer framebuffer!\n");
	}

	s_BackBufferFBO2.iWidth = glwidth;
	s_BackBufferFBO2.iHeight = glheight;
	GL_GenFrameBuffer(&s_BackBufferFBO2);
	GL_FrameBufferColorTexture(&s_BackBufferFBO2, GL_RGBA16F);
	GL_FrameBufferDepthTexture(&s_BackBufferFBO2, GL_DEPTH24_STENCIL8);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BackBufferFBO2);
		g_pMetaHookAPI->SysError("Failed to initialize backbuffer2 framebuffer!\n");
	}

	s_GBufferFBO.iWidth = glwidth;
	s_GBufferFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_GBufferFBO);
	GL_FrameBufferColorTextureDeferred(&s_GBufferFBO, GL_RGB16F);
	GL_FrameBufferDepthTexture(&s_GBufferFBO, GL_DEPTH24_STENCIL8);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_GBufferFBO);
		g_pMetaHookAPI->SysError("Failed to initialize GBuffer framebuffer.\n");
	}

	s_DepthLinearFBO.iWidth = glwidth;
	s_DepthLinearFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_DepthLinearFBO);
	GL_FrameBufferColorTexture(&s_DepthLinearFBO, GL_R32F);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_DepthLinearFBO);
		g_pMetaHookAPI->SysError("Failed to initialize DepthLinear framebuffer!\n");
	}

	s_HBAOCalcFBO.iWidth = glwidth;
	s_HBAOCalcFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_HBAOCalcFBO);
	GL_FrameBufferColorTextureHBAO(&s_HBAOCalcFBO);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_HBAOCalcFBO);
		g_pMetaHookAPI->SysError("Failed to initialize HBAOCalc framebuffer.\n");
	}

	s_ShadowFBO.iWidth = glwidth;
	s_ShadowFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_ShadowFBO);

	s_WaterFBO.iWidth = glwidth;
	s_WaterFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_WaterFBO);
	GL_FrameBufferColorTexture(&s_WaterFBO, GL_RGB16F);
	GL_FrameBufferDepthTexture(&s_WaterFBO, GL_DEPTH24_STENCIL8);

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
		GL_FrameBufferColorTexture(&s_DownSampleFBO[i], GL_RGB16F);

		if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_DownSampleFBO[i]);
			g_pMetaHookAPI->SysError("Failed to initialize DownSample #%d framebuffer.\n", i);
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
		GL_FrameBufferColorTexture(&s_LuminFBO[i], GL_R32F);

		if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_LuminFBO[i]);
			g_pMetaHookAPI->SysError("Failed to initialize Luminance #%d framebuffer.\n", i);
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
		GL_FrameBufferColorTexture(&s_Lumin1x1FBO[i], GL_R32F);

		if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_FreeFBO(&s_Lumin1x1FBO[i]);
			g_pMetaHookAPI->SysError("Failed to initialize Luminance1x1 #%d framebuffer.\n", i);
		}
	}

	//Bright Pass FBO
	s_BrightPassFBO.iWidth = (glwidth >> DOWNSAMPLE_BUFFERS);
	s_BrightPassFBO.iHeight = (glheight >> DOWNSAMPLE_BUFFERS);
	GL_GenFrameBuffer(&s_BrightPassFBO);
	GL_FrameBufferColorTexture(&s_BrightPassFBO, GL_RGB16F);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BrightPassFBO);
		g_pMetaHookAPI->SysError("Failed to initialize BrightPass framebuffer.\n");
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
			GL_FrameBufferColorTexture(&s_BlurPassFBO[i][j], GL_RGB16F);

			if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				GL_FreeFBO(&s_BlurPassFBO[i][j]);
				g_pMetaHookAPI->SysError("Failed to initialize Blur #%d framebuffer.\n", i);
			}
		}
		downW >>= 1;
		downH >>= 1;
	}

	s_BrightAccumFBO.iWidth = glwidth >> DOWNSAMPLE_BUFFERS;
	s_BrightAccumFBO.iHeight = glheight >> DOWNSAMPLE_BUFFERS;
	GL_GenFrameBuffer(&s_BrightAccumFBO);
	GL_FrameBufferColorTexture(&s_BrightAccumFBO, GL_RGB16F);
	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BrightAccumFBO);
		g_pMetaHookAPI->SysError("Failed to initialize BrightAccumulate #%d framebuffer.\n");
	}

	s_ToneMapFBO.iWidth = glwidth;
	s_ToneMapFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_ToneMapFBO);
	GL_FrameBufferColorTexture(&s_ToneMapFBO, GL_RGB8);
	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_ToneMapFBO);
		gEngfuncs.Con_Printf("Failed to initialize ToneMapping #%d framebuffer.\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GL_Init(void)
{
	auto err = glewInit();
	if (GLEW_OK != err)
	{
		g_pMetaHookAPI->SysError("glewInit failed, %s", glewGetErrorString(err));
		return;
	}

	if (!(*gl_mtexable))
	{
		g_pMetaHookAPI->SysError("Multitexture extension must be enabled!\nPlease remove \"-nomtex\" from launch parameters and try again.");
		return;
	}

	if (!glewIsSupported("GL_ARB_shader_objects"))
	{
		g_pMetaHookAPI->SysError("Missing OpenGL extension GL_ARB_shader_objects!\n");
		return;
	}

	if (!glewIsSupported("GL_EXT_framebuffer_object"))
	{
		g_pMetaHookAPI->SysError("Missing OpenGL extension GL_EXT_framebuffer_object!\n");
		return;
	}

	if (!glewIsSupported("GL_EXT_framebuffer_blit"))
	{
		g_pMetaHookAPI->SysError("Missing OpenGL extension GL_EXT_framebuffer_blit!\n");
		return;
	}

	if (!glewIsSupported("GL_ARB_texture_float"))
	{
		g_pMetaHookAPI->SysError("Missing OpenGL extension GL_ARB_texture_float!\n");
		return;
	}

	if (!glewIsSupported("GL_EXT_multi_draw_arrays"))
	{
		g_pMetaHookAPI->SysError("Missing OpenGL extension GL_EXT_multi_draw_arrays!\n");
		return;
	}

	gl_max_texture_size = 128;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);

	gl_max_ansio = 1;
	if (glewIsSupported("GL_EXT_texture_filter_anisotropic"))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max_ansio);
	}

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &gl_max_ubo_size);

	bNoStretchAspect = (gEngfuncs.CheckParm("-stretchaspect", NULL) == 0);

	if(gEngfuncs.CheckParm("-nobindless", NULL))
		bUseBindless = false;

	if (bUseBindless && !glewIsSupported("GL_NV_bindless_texture") && !glewIsSupported("GL_ARB_bindless_texture"))
		bUseBindless = false;
	
	if (gEngfuncs.CheckParm("-oitblend", NULL))
		bUseOITBlend = true;

	if (bUseOITBlend && !glewIsSupported("GL_ARB_shader_image_load_store"))
		bUseOITBlend = false;

	if (bUseOITBlend && !glewIsSupported("GL_ARB_fragment_shader_interlock"))
		bUseOITBlend = false;

	GL_GenerateFrameBuffers();
	GL_InitShaders();
}

void GL_Shutdown(void)
{
	GL_FreeShaders();
	GL_FreeProfiles();

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

	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
}

void R_PreRenderView(int a1)
{
	g_SvEngine_DrawPortalView = a1 ? true : false;

	//Capture previous fog settings

	r_fog_mode = 0;

	if (glIsEnabled(GL_FOG))
	{
		glGetIntegerv(GL_FOG_MODE, &r_fog_mode);

		if (r_fog_mode == GL_LINEAR)
		{
			glGetFloatv(GL_FOG_START, &r_fog_control[0]);
			glGetFloatv(GL_FOG_END, &r_fog_control[1]);
			glGetFloatv(GL_FOG_COLOR, r_fog_color);
		}
		else if (r_fog_mode == GL_EXP)
		{
			glGetFloatv(GL_FOG_START, &r_fog_control[0]);
			glGetFloatv(GL_FOG_END, &r_fog_control[1]);
			glGetFloatv(GL_FOG_DENSITY, &r_fog_control[2]);
			glGetFloatv(GL_FOG_COLOR, r_fog_color);
		}
		else if (r_fog_mode == GL_EXP2)
		{
			glGetFloatv(GL_FOG_START, &r_fog_control[0]);
			glGetFloatv(GL_FOG_END, &r_fog_control[1]);
			glGetFloatv(GL_FOG_DENSITY, &r_fog_control[2]);
			glGetFloatv(GL_FOG_COLOR, r_fog_color);
		}
	}

	if (!(*r_refdef.onlyClientDraws))
	{
		shadow_numvisedicts[0] = 0;
		shadow_numvisedicts[1] = 0;
		shadow_numvisedicts[2] = 0;

		if (r_shadow && r_shadow->value && !g_SvEngine_DrawPortalView && r_draw_pass == r_draw_normal)
		{
			R_RenderShadowMap();
		}

		if (r_water && r_water->value)
		{
			R_RenderWaterView();
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s_BackBufferFBO.s_hBackBufferTex, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, s_BackBufferFBO.s_hBackBufferDepthTex, 0);
}

void R_PostRenderView()
{
	R_DoFXAA();

	if (r_hdr->value && !r_draw_pass && !g_SvEngine_DrawPortalView && !CL_IsDevOverviewMode())
	{
		R_HDR();
	}
	else
	{
		R_GammaCorrection();
	}

	GL_DisableMultitexture();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);

	glBindFramebuffer(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	g_SvEngine_DrawPortalView = false;	
}

void R_PreDrawViewModel(void)
{
	(*currententity) = cl_viewent;

	if (!r_drawviewmodel->value)
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

		if (!gExportfuncs.CL_IsThirdPerson() && !chase_active->value && !(*envmap) && cl_stats[0] > 0)
		{
			auto ent = gEngfuncs.GetEntityByIndex((*currententity)->index);
			for (int i = 0; i < 4; i++)
			{
				VectorCopy(ent->origin, (*currententity)->attachment[i]);
			}

			(*gpStudioInterface)->StudioDrawModel(STUDIO_EVENTS);
		}
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
		r_sprite_drawcall = 0;
		r_sprite_polys = 0;
	}

	if (r_norefresh->value)
		return;

	if (!r_worldmodel)
	{
		g_pMetaHookAPI->SysError("R_RenderView: NULL worldmodel");
	}

	double time1;

	if (!a1 && r_speeds->value)
	{
		time1 = gEngfuncs.GetAbsoluteTime();
	}

	R_ForceCVars(r_params.maxclients > 1);

	R_PreRenderView(a1);

	float clearColor[3];

	if (CL_IsDevOverviewMode())
	{
		R_ParseVectorCvar(dev_overview_color, clearColor);
	}
	else
	{
		R_ParseVectorCvar(gl_clearcolor, clearColor);
	}

	glClearColor(clearColor[0], clearColor[1], clearColor[2], 0);

	glStencilMask(0xFF);
	glClearStencil(0);
	glDepthMask(GL_TRUE);

	if (!gl_clear->value || a1)
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	else
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glStencilMask(0);

	glDepthFunc(GL_LEQUAL);
	glDepthRange(0, 1);

	if (!(*r_refdef.onlyClientDraws))
		R_PreDrawViewModel();

	R_RenderScene();

	if (!(*r_refdef.onlyClientDraws))
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

		gEngfuncs.Con_Printf("%3ifps %3i ms, %d brushpolys, %d brushdraw, %d studiopolys, %d studiodraw, %d spritepolys, %d spritedraw\n",
			(int)(framerate + 0.5), (int)((time2 - time1) * 1000), 
			r_wsurf_polys, r_wsurf_drawcall,
			r_studio_polys, r_studio_drawcall,
			r_sprite_polys, r_sprite_drawcall
		);

		*c_alias_polys = r_studio_polys;
		*c_brush_polys = r_wsurf_polys;
	}
}

void R_RenderView(void)
{
	R_RenderView_SvEngine(0);
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
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBlitFramebuffer(0, 0, glwidth, glheight, dstX, dstY, dstX2, dstY2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	//VID_FlipScreen for us.
	gRefFuncs.GL_EndRendering();

	if (gl_backbuffer_fbo)
	{
		*gl_backbuffer_fbo = save_backbuffer_fbo;
	}
}

void DLL_SetModKey(void *pinfo, char *pkey, char *pvalue)
{
	gRefFuncs.DLL_SetModKey(pinfo, pkey, pvalue);

	if (!strcmp(pkey, "vertical_fov"))
	{
		bVerticalFov = atoi(pvalue) ? true : false;
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
	//r_novis->flags &= ~FCVAR_SPONLY;

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

	dev_overview_color = gEngfuncs.pfnRegisterVariable("dev_overview_color", "0 255 0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

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
	//gl_max_size->value = gl_max_texture_size;

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

	r_vertical_fov = gEngfuncs.pfnRegisterVariable("r_vertical_fov", bVerticalFov ? "1" : "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_adjust_fov = gEngfuncs.pfnRegisterVariable("r_adjust_fov", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	gl_profile = gEngfuncs.pfnRegisterVariable("gl_profile", "0", FCVAR_CLIENTDLL );

	gEngfuncs.pfnAddCommand("saveprogstate", R_SaveProgramStates_f);
	gEngfuncs.pfnAddCommand("loadprogstate", R_LoadProgramStates_f);
}

void R_Init(void)
{
	R_InitCvars();

	R_InitWater();
	R_InitStudio();
	R_InitShadow();
	R_InitWSurf();
	R_InitLight();
	R_InitSprite();
	R_InitPostProcess();

	R_LoadProgramStates_f();
}

void R_Shutdown(void)
{
	R_ShutdownWater();
	R_ShutdownStudio();
	R_ShutdownShadow();
	R_ShutdownWSurf();
	R_ShutdownLight();
	R_ShutdownSprite();
	R_ShutdownPostProcess();

	R_FreeMapCvars();
}

void R_ForceCVars(qboolean mp)
{
	if (r_draw_pass)
		return;

	gRefFuncs.R_ForceCVars(mp);
}

void R_NewMap(void)
{
	R_GenerateSceneUBO();

	gRefFuncs.R_NewMap();

	r_worldentity = gEngfuncs.GetEntityByIndex(0);
	r_worldmodel = r_worldentity->model;
	r_playermodel = NULL;

	memset(&r_params, 0, sizeof(r_params));

	R_NewMapWater();
	R_NewMapWSurf();

	R_StudioReloadVBOCache();
}

mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model)
{
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

double V_CalcFovV(float fov, float width, float height)
{
	if (fov < 1.0 || fov > 179.0)
		fov = 90.0;
	return atan2(width / (height / tan(fov * (1.0 / 360.0) * M_PI)), 1.0) * 360.0 * (1 / M_PI);
}

double V_CalcFovH(float fov, float width, float height)
{
	if (fov < 1.0 || fov > 179.0)
		fov = 90.0;
	return atan2(height / (width / tan(fov * (1.0 / 360.0) * M_PI)), 1.0) * 360.0 * (1 / M_PI);
}

float V_CalcFov(float *fov_x, float width, float height)
{
	float	x, half_fov_y;

	if (*fov_x < 1.0f || *fov_x > 179.0f)
		*fov_x = 90.0f; // default value

	x = width / tan((*fov_x) * (M_PI / 360) * 0.5f);
	half_fov_y = atan(height / x);

	return (half_fov_y * 360 / M_PI) * 2;
}

void V_AdjustFov(float *fov_x, float *fov_y, float width, float height)
{
	float x, y;

	if (fabs(width * 3 - 4 * height) < 1 || fabs(width * 4 - height * 5) < 1)
	{
		// 4:3 or 5:4 ratio
		return;
	}

	y = V_CalcFov(fov_x, 640, 480);
	x = *fov_x;

	*fov_x = V_CalcFov(&y, height, width);

	if (*fov_x < x)
		*fov_x = x;
	else
		*fov_y = y;
}

void R_SetFrustum(void)
{
	float yfov, xfov;
	if (r_vertical_fov->value)
	{
		yfov = (*scrfov);
		xfov = V_CalcFovV((*scrfov), glwidth, glheight);
	}
	else
	{
		yfov = V_CalcFovH((*scrfov), glwidth, glheight);
		xfov = (*scrfov);

		if (r_adjust_fov->value)
		{
			V_AdjustFov(&xfov, &yfov, glwidth, glheight);
		}
	}

	RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90.0 - xfov * 0.5));
	RotatePointAroundVector(frustum[1].normal, vup, vpn, 90.0 - xfov * 0.5);
	RotatePointAroundVector(frustum[2].normal, vright, vpn, 90.0 - yfov * 0.5);
	RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90.0 - yfov * 0.5));

	for (int i = 0; i < 4; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane(&frustum[i]);
	}
}

bool CL_IsDevOverviewMode(void)
{
	return gRefFuncs.CL_IsDevOverviewMode();
}

void CL_SetDevOverView(void *a1)
{
	return gRefFuncs.CL_SetDevOverView(a1);
}

void MYgluPerspective2(double xfov, double yfov, double zNear, double zFar)
{
	auto yMax = zNear * tan(yfov * M_PI / 360.0f);
	auto yMin = -yMax;

	auto xMax = zNear * tan(xfov * M_PI / 360.0f);
	auto xMin = -xMax;

	glFrustum(xMin, xMax, yMin, yMax, zNear, zFar);
}

void R_SetupGL(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	auto v0 = (*r_refdef.vrect).x;
	auto v1 = glheight - (*r_refdef.vrect).y;
	auto v2 = (*r_refdef.vrect).x + (*r_refdef.vrect).width;
	auto v3 = glheight - (*r_refdef.vrect).height - (*r_refdef.vrect).y;
	if ((*r_refdef.vrect).x > 0)
		v0 = (*r_refdef.vrect).x - 1;
	if (v2 < glwidth)
		++v2;
	if (v3 < 0)
		--v3;
	if (v1 < glheight)
		++v1;
	auto v4 = v2 - v0;
	auto v5 = v1 - v3;

	if ((*envmap))
	{
		v3 = 0;
		v0 = 0;
		glheight = gl_envmapsize->value;
		glwidth = gl_envmapsize->value;
	}

	glViewport(v0 + glx, v3 + gly, v4, v5);

	r_viewport[0] = v0 + glx;
	r_viewport[1] = v3 + gly;
	r_viewport[2] = v4;
	r_viewport[3] = v5;

	if (r_vertical_fov->value)
	{
		auto height = (double)(*r_refdef.vrect).height;
		auto width = (double)(*r_refdef.vrect).width;
		auto aspect = height / width;

		auto fov = (*scrfov);
		if (fov < 1.0 || fov > 179.0)
			fov = 90.0;

		r_yfov = fov;
		r_xfov = atan2(width / (height / tan(fov * (1.0 / 360.0) * M_PI)), 1.0) * 360.0 * (1.0 / M_PI);

		if ((*r_refdef.onlyClientDraws))
		{
			MYgluPerspectiveV(r_xfov, aspect, 4.0, 16000.0);
		}
		else if (CL_IsDevOverviewMode())
		{
			glOrtho(
				-(4096.0 / (gDevOverview->zoom * aspect)),
				(4096.0 / (gDevOverview->zoom * aspect)),
				-(4096.0 / gDevOverview->zoom),
				(4096.0 / gDevOverview->zoom),
				16000.0 - gDevOverview->z_min,
				16000.0 - gDevOverview->z_max);

			r_znear = 16000.0 - gDevOverview->z_min;
			r_zfar = 16000.0 - gDevOverview->z_max;
			r_ortho = true;
		}
		else
		{
			MYgluPerspectiveV(r_xfov, aspect, 4.0, (r_params.movevars ? r_params.movevars->zmax : 4096));
		}
	}
	else
	{
		auto width = (double)(*r_refdef.vrect).width;
		auto height = (double)(*r_refdef.vrect).height;
		auto aspect = width / height;
		auto fov = (*scrfov);
		if (fov < 1.0 || fov > 179.0)
			fov = 90.0;
		r_xfov = fov;
		r_yfov = atan2(height / (width / tan(fov * (1.0 / 360.0) * M_PI)), 1.0) * 360.0 * (1.0 / M_PI);

		if ((*r_refdef.onlyClientDraws))
		{
			if (r_adjust_fov->value)
			{
				V_AdjustFov(&r_xfov, &r_yfov, width, height);
				MYgluPerspectiveH(r_yfov, aspect, 4.0, 16000.0);
			}
			else
			{
				MYgluPerspectiveH(r_yfov, aspect, 4.0, 16000.0);
			}
		}
		else if (CL_IsDevOverviewMode())
		{
			glOrtho(
				-(4096.0 / gDevOverview->zoom),
				(4096.0 / gDevOverview->zoom),
				-(4096.0 / (gDevOverview->zoom * aspect)),
				(4096.0 / (gDevOverview->zoom * aspect)),
				16000.0 - gDevOverview->z_min,
				16000.0 - gDevOverview->z_max);

			r_znear = 16000.0 - gDevOverview->z_min;
			r_zfar = 16000.0 - gDevOverview->z_max;
			r_ortho = true;
		}
		else
		{
			if (r_adjust_fov->value)
			{
				V_AdjustFov(&r_xfov, &r_yfov, width, height);
				MYgluPerspectiveH(r_yfov, aspect, 4.0, (r_params.movevars ? r_params.movevars->zmax : 4096));
			}
			else
			{
				MYgluPerspectiveH(r_yfov, aspect, 4.0, (r_params.movevars ? r_params.movevars->zmax : 4096));
			}
		}
	}
	glCullFace(GL_FRONT);
	glGetFloatv(GL_PROJECTION_MATRIX, r_projection_matrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(-90, 1, 0, 0);
	glRotatef(90, 0, 0, 1);
	glRotatef(-(*r_refdef.viewangles)[2], 1, 0, 0);
	glRotatef(-(*r_refdef.viewangles)[0], 0, 1, 0);
	glRotatef(-(*r_refdef.viewangles)[1], 0, 0, 1);
	glTranslatef(-(*r_refdef.vieworg)[0], -(*r_refdef.vieworg)[1], -(*r_refdef.vieworg)[2]);

	glGetFloatv(GL_MODELVIEW_MATRIX, r_world_matrix);
	if (!gl_cull->value)
		glDisable(GL_CULL_FACE);
	else
		glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);

	for (int i = 0; i < 16; i += 4)
	{
		for (int j = 0; j < 4; j++)
		{
			gWorldToScreen[i + j] = 0;

			for (int k = 0; k < 4; k++)
				gWorldToScreen[i + j] += r_world_matrix[i + k] * r_projection_matrix[k * 4 + j];
		}
	}

	InvertMatrix(gWorldToScreen, gScreenToWorld);
}

void R_CheckVariables(void)
{
	gRefFuncs.R_CheckVariables();
}

void R_AnimateLight(void)
{
	gRefFuncs.R_AnimateLight();
}

void R_SetupFrame(void)
{
	//No need to force cvars since we've already done this in R_RenderView
	//R_ForceCVars(gEngfuncs.GetMaxClients() > 1);

	R_CheckVariables();
	R_AnimateLight();

	++(*r_framecount);

	VectorCopy((*r_refdef.vieworg), r_origin);

	gEngfuncs.pfnAngleVectors((*r_refdef.viewangles), vpn, vright, vup);

	(*r_oldviewleaf) = (*r_viewleaf);

	if (r_draw_pass == r_draw_reflect)
	{
		(*r_viewleaf) = Mod_PointInLeaf(water_view, r_worldmodel);
	}
	else if (r_refdef_SvEngine && r_refdef_SvEngine->useCamera)
	{
		(*r_viewleaf) = Mod_PointInLeaf(r_refdef_SvEngine->r_camera_origin, r_worldmodel);
	}
	else
	{
		(*r_viewleaf) = Mod_PointInLeaf(r_origin, r_worldmodel);
	}

	if ((*cl_waterlevel) > 2 && !(*r_refdef.onlyClientDraws))
	{
		r_fog_color[0] = cshift_water->destcolor[0] * 0.00392156862745098;
		r_fog_color[1] = cshift_water->destcolor[1] * 0.00392156862745098;
		r_fog_color[2] = cshift_water->destcolor[2] * 0.00392156862745098;
		r_fog_color[3] = 1.0;

		r_fog_control[0] = 0;
		r_fog_control[1] = (1536 - 4 * cshift_water->percent);
		r_fog_control[2] = 0;

		r_fog_mode = GL_LINEAR;

		glFogi(GL_FOG_MODE, r_fog_mode);
		glFogfv(GL_FOG_COLOR, r_fog_color);
		glFogf(GL_FOG_START, r_fog_control[0]);
		glFogf(GL_FOG_END, r_fog_control[1]);
		glEnable(GL_FOG);
	}
}

void R_MarkLeaves(void)
{
	gRefFuncs.R_MarkLeaves();
}

void R_DrawEntitiesOnList(void)
{
	if (!r_drawentities->value)
		return;

	for (int i = 0; i < (*cl_numvisedicts); ++i)
	{
		(*currententity) = cl_visedicts[i];

		if ((*currententity)->curstate.rendermode != kRenderNormal)
		{
			R_AddTEntity((*currententity));
			continue;
		}

		if ((*currententity)->curstate.rendermode == kRenderNormal &&
			(*currententity)->model &&
			(*currententity)->model->type == mod_sprite &&
			gl_spriteblend->value)
		{
			R_AddTEntity((*currententity));
			continue;
		}

		if ((*currententity)->model && 
			(*currententity)->model->type != mod_sprite)
		{
			R_DrawCurrentEntity(false);
		}
	}
}

void R_RenderFinalFog(void)
{
	memcpy(r_fog_color, g_UserFogColor, sizeof(vec4_t));

	r_fog_control[0] = (*g_UserFogStart);
	r_fog_control[1] = (*g_UserFogEnd);
	r_fog_control[2] = (*g_UserFogDensity);

	r_fog_mode = GL_EXP2;

	scene_ubo_t SceneUBO;
	memcpy(SceneUBO.fogColor, r_fog_color, sizeof(vec4_t));
	SceneUBO.fogStart = r_fog_control[0];
	SceneUBO.fogEnd = r_fog_control[1];
	SceneUBO.fogDensity = r_fog_control[2];

	glNamedBufferSubData(r_wsurf.hSceneUBO, offsetof(scene_ubo_t, fogColor), offsetof(scene_ubo_t, time) - offsetof(scene_ubo_t, fogColor), &SceneUBO.fogColor);

	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, r_fog_mode);
	glFogf(GL_FOG_DENSITY, r_fog_control[2]);
	glHint(GL_FOG_HINT, GL_NICEST);
	glFogfv(GL_FOG_COLOR, r_fog_color);
	glFogf(GL_FOG_START, r_fog_control[0]);
	glFogf(GL_FOG_END, r_fog_control[1]);
}

void AllowFog(int allowed)
{
	static GLboolean isFogEnabled;

	if (allowed)
	{
		if (isFogEnabled)
			glEnable(GL_FOG);
	}
	else
	{
		isFogEnabled = glIsEnabled(GL_FOG);

		if (isFogEnabled)
			glDisable(GL_FOG);
	}
}

void R_RenderScene(void)
{
	//return gRefFuncs.R_RenderScene();

	static glprofile_t profile_RenderScene;
	GL_BeginProfile(&profile_RenderScene, "R_RenderScene");

	if (CL_IsDevOverviewMode())
		CL_SetDevOverView(R_GetRefDef());

	R_SetupFrame();
	R_SetFrustum();
	R_SetupGL();
	R_MarkLeaves();

	if (!(*r_refdef.onlyClientDraws))
	{
		R_DrawWorld();

		S_ExtraUpdate();

		R_DrawEntitiesOnList();
	}

	if ((*g_bUserFogOn))
		R_RenderFinalFog();

	AllowFog(false);
	HUD_DrawNormalTriangles();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	AllowFog(true);

	R_DrawTEntitiesOnList((*r_refdef.onlyClientDraws));

	if (((*cl_waterlevel) > 2 && (*r_refdef.onlyClientDraws)) || !(*g_bUserFogOn))
		glDisable(GL_FOG);

	S_ExtraUpdate();

	GL_EndProfile(&profile_RenderScene);
}

int EngineGetMaxKnownModel(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return 16384;

	return 1024;
}

int EngineGetModelIndex(model_t *mod)
{
	int index = (mod - (model_t *)(mod_known));

	if (index >= 0 && index < *mod_numknown)
		return index;

	return -1;
}

model_t *EngineGetModelByIndex(int index)
{
	auto pmod_known = (model_t *)(mod_known);

	if (index >= 0 && index < *mod_numknown)
		return &pmod_known[index];

	return NULL;
}

void Mod_LoadStudioModel(model_t *mod, void *buffer)
{
	gRefFuncs.Mod_LoadStudioModel(mod, buffer);

	studiohdr_t *studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);
	if (studiohdr)
	{
		if (studiohdr->soundtable)
		{
			//gEngfuncs.Con_DPrintf("wtf");
		}
		studiohdr->soundtable = 0;

		R_StudioLoadExternalFile(mod, studiohdr);

		R_PrepareStudioVBO(studiohdr);
	}
}

void R_LoadSkyName_SvEngine(const char *name)
{
	for (int i = 0; i < 6; ++i)
	{
		if (r_wsurf.vSkyboxTextureHandles[i])
		{
			glMakeTextureHandleNonResidentARB(r_wsurf.vSkyboxTextureHandles[i]);
			r_wsurf.vSkyboxTextureHandles[i] = 0;
		}

		if (gSkyTexNumber[i])
		{
			GL_DeleteTexture(gSkyTexNumber[i]);
			gSkyTexNumber[i] = 0;
		}
	}

	gRefFuncs.R_LoadSkyName_SvEngine(name);

	if (bUseBindless)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			for (int i = 0; i < 6; ++i)
			{
				if (!r_wsurf.vSkyboxTextureHandles[i] && gSkyTexNumber[i])
				{
					auto handle = glGetTextureHandleARB(gSkyTexNumber[i]);
					glMakeTextureHandleResidentARB(handle);
					r_wsurf.vSkyboxTextureHandles[i] = handle;
				}
			}
		}
		else
		{
			const int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };
			for (int i = 0; i < 6; ++i)
			{
				if (!r_wsurf.vSkyboxTextureHandles[i] && gSkyTexNumber[skytexorder[i]])
				{
					auto handle = glGetTextureHandleARB(gSkyTexNumber[skytexorder[i]]);
					glMakeTextureHandleResidentARB(handle);
					r_wsurf.vSkyboxTextureHandles[i] = handle;
				}
			}
		}
		glNamedBufferSubData(r_wsurf.hSkyboxSSBO, 0, sizeof(r_wsurf.vSkyboxTextureHandles), r_wsurf.vSkyboxTextureHandles);
	}
}

void R_LoadSkys(void)
{
	for (int i = 0; i < 6; ++i)
	{
		if (r_wsurf.vSkyboxTextureHandles[i])
		{
			glMakeTextureHandleNonResidentARB(r_wsurf.vSkyboxTextureHandles[i]);
			r_wsurf.vSkyboxTextureHandles[i] = 0;
		}
		if (gSkyTexNumber[i])
		{
			GL_DeleteTexture(gSkyTexNumber[i]);
			gSkyTexNumber[i] = 0;
		}
	}
	
	gRefFuncs.R_LoadSkys();

	if (bUseBindless)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			for (int i = 0; i < 6; ++i)
			{
				if (!r_wsurf.vSkyboxTextureHandles[i] && gSkyTexNumber[i])
				{
					auto handle = glGetTextureHandleARB(gSkyTexNumber[i]);
					glMakeTextureHandleResidentARB(handle);
					r_wsurf.vSkyboxTextureHandles[i] = handle;
				}
			}
		}
		else
		{
			const int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };
			for (int i = 0; i < 6; ++i)
			{
				if (!r_wsurf.vSkyboxTextureHandles[i] && gSkyTexNumber[skytexorder[i]])
				{
					auto handle = glGetTextureHandleARB(gSkyTexNumber[skytexorder[i]]);
					glMakeTextureHandleResidentARB(handle);
					r_wsurf.vSkyboxTextureHandles[i] = handle;
				}
			}
		}
		glNamedBufferSubData(r_wsurf.hSkyboxSSBO, 0, sizeof(r_wsurf.vSkyboxTextureHandles), r_wsurf.vSkyboxTextureHandles);
	}
}

#if 0

void R_BuildCubemap_Snapshot(cubemap_t *cubemap, int index)
{
	char name[64];
	COM_FileBase(r_worldmodel->name, name);

	if (!g_pFileSystem->IsDirectory("gfx/cubemap"))
		g_pFileSystem->CreateDirHierarchy("gfx/cubemap");

	char path[64];
	snprintf(path, sizeof(path) - 1, "gfx/cubemap/%s", name);
	path[sizeof(path) - 1] = 0;

	if (!g_pFileSystem->IsDirectory(path))
		g_pFileSystem->CreateDirHierarchy(path);

	char filepath[1024];
	snprintf(filepath, sizeof(filepath) - 1, "gfx/cubemap/%s/%s_%d.%s", name, cubemap->name.c_str(), index, cubemap->extension.c_str());
	filepath[sizeof(filepath) - 1] = 0;

	byte *pBuf = (byte *)malloc(cubemap->size * cubemap->size * 3);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, cubemap->size, cubemap->size, GL_RGB, GL_UNSIGNED_BYTE, pBuf);

	if (TRUE == SaveImageGeneric(filepath, cubemap->size, cubemap->size, pBuf))
	{
		gEngfuncs.Con_Printf("Cubemap %s saved.\n", filepath);
	}

	free(pBuf);
}

void R_BuildCubemap(cubemap_t *cubemap)
{
	gEngfuncs.Con_Printf("Building cubemap \"%s\" , cubemap size = %d\n", cubemap->name.c_str(), cubemap->size);

	vrect_t saveVrect;
	memcpy(&saveVrect, &(*r_refdef.vrect), sizeof(vrect_t));

	vec3_t viewangles_array[6] = {
		{0, 0, 0},
		{0, 180, 0},
		{0, 90, 0},
		{0, 270, 0},
		{-90, 270, 0},
		{90, 0, 0},
	};

	(*r_refdef.vrect).x = 0;
	(*r_refdef.vrect).y = 0;
	(*r_refdef.vrect).width = cubemap->size;
	(*r_refdef.vrect).height = cubemap->size;

	(*envmap) = true;

	float saved_gl_envmapsize = gl_envmapsize->value;

	gl_envmapsize->value = cubemap->size;

	R_PushRefDef();

	GL_PushFrameBuffer();

	for (int i = 0; i < 6; ++i)
	{
		VectorCopy(cubemap->origin, (*r_refdef).vieworg);
		VectorCopy(viewangles_array[i], (*r_refdef).viewangles);

		GL_BeginRendering(&glx, &gly, &glwidth, &glheight);

		if (g_iEngineType == ENGINE_SVENGINE)
			R_RenderView_SvEngine(0);
		else
			R_RenderView();

		R_BuildCubemap_Snapshot(cubemap, i);

		GL_EndRendering();
	}

	(*envmap) = false;

	gl_envmapsize->value = saved_gl_envmapsize;

	GL_PopFrameBuffer();

	R_PopRefDef();

	memcpy(&(*r_refdef.vrect), &saveVrect, sizeof(vrect_t));
}

void R_BuildCubemaps_f(void)
{
	if (!r_worldmodel || !r_worldmodel->name[0])
	{
		gEngfuncs.Con_Printf("Cannot build cubemap, no map loaded!\n");
		return;
	}

	gEngfuncs.Con_Printf("Building %d cubemap(s)...\n", r_cubemaps.size());

	for (size_t i = 0; i < r_cubemaps.size(); ++i)
		R_BuildCubemap(&r_cubemaps[i]);
}

cubemap_t *R_FindCubemap(float *origin)
{
	if (*envmap)
		return NULL;

	float max_dist = 99999;
	cubemap_t *cubemap = NULL;

	for (size_t i = 0; i < r_cubemaps.size(); ++i)
	{
		float dir[3];
		VectorSubtract(origin, r_cubemaps[i].origin, dir);
		float dist = VectorLength(dir);
		if (dist < max_dist && dist < r_cubemaps[i].radius)
		{
			cubemap = &r_cubemaps[i];
			max_dist = dist;
		}
	}

	return cubemap;
}

void R_LoadCubemap(cubemap_t *cubemap)
{
	char name[64];
	COM_FileBase(r_worldmodel->name, name);

	char filepath[1024];
	char identifier[256];

	for (int i = 0; i < 6; ++i)
	{
		snprintf(filepath, sizeof(filepath) - 1, "gfx/cubemap/%s/%s_%d.%s", name, cubemap->name.c_str(), i, cubemap->extension.c_str());
		filepath[sizeof(filepath) - 1] = 0;

		snprintf(identifier, sizeof(identifier) - 1, "cubemap_%s", cubemap->name.c_str());
		identifier[sizeof(identifier) - 1] = 0;

		gl_loadtexture_cubemap = i + 1;

		cubemap->cubetex = R_LoadTextureEx(filepath, identifier, NULL, NULL, GLT_WORLD, tre, true);
	}

	gl_loadtexture_cubemap = 0;
}

#endif

void R_SaveProgramStates_f(void)
{
	R_SaveWSurfProgramStates();
	R_SaveWaterProgramStates();
	R_SaveDLightProgramStates();
	R_SaveDFinalProgramStates();
	R_SaveStudioProgramStates();
	R_SaveSpriteProgramStates();
	R_SaveLegacySpriteProgramStates();

	gEngfuncs.Con_Printf("Program states loaded.\n");
}

void R_LoadProgramStates_f(void)
{
	R_LoadWSurfProgramStates();
	R_LoadWaterProgramStates();
	R_LoadDLightProgramStates();
	R_LoadDFinalProgramStates();
	R_LoadStudioProgramStates();
	R_LoadSpriteProgramStates();
	R_LoadLegacySpriteProgramStates();

	gEngfuncs.Con_Printf("Program states saved.\n");
}

void GammaToLinear(float *color)
{
	color[0] = pow(color[0], v_gamma->value);
	color[1] = pow(color[1], v_gamma->value);
	color[2] = pow(color[2], v_gamma->value);
}