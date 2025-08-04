#include "gl_local.h"
#include "gl_common.h"
#include "pm_defs.h"
#include "CounterStrike.h"
#include <event_api.h>

#include <intrin.h>
#include <sstream>
#include <set>
#include <SDL2/SDL_video.h>

#define HAS_VIEWMODEL_PASS 1

private_funcs_t gPrivateFuncs = { 0 };

refdef_t r_refdef = { 0 };
ref_params_t r_params = { 0 };
refdef_GoldSrc_t *r_refdef_GoldSrc = NULL;
refdef_SvEngine_t *r_refdef_SvEngine = NULL;

float *scrfov = NULL;
float r_xfov = 0;
float r_yfov = 0;
float r_xfov_viewmodel = 0;
float r_yfov_viewmodel = 0;
float r_xfov_currentpass = 0;
float r_yfov_currentpass = 0;
float r_screenaspect = 0;

bool r_fog_enabled = false;
int r_fog_mode = 0;
float r_fog_control[3] = { 0 };
float r_fog_color[4] = { 0 };

RECT *window_rect = NULL;

float * s_fXMouseAspectAdjustment = NULL;
float * s_fYMouseAspectAdjustment = NULL;

float s_fXMouseAspectAdjustment_Storage = 0;
float s_fYMouseAspectAdjustment_Storage = 0;

cl_entity_t* r_worldentity = NULL;
model_t** cl_worldmodel = NULL;

int *cl_numvisedicts = NULL;
cl_entity_t **cl_visedicts = NULL;
cl_entity_t **currententity = NULL;
int *numTransObjs = NULL;
int *maxTransObjs = NULL;
transObjRef **transObjects = NULL;
mleaf_t **r_viewleaf = NULL;
mleaf_t **r_oldviewleaf = NULL;

float r_viewport[4] = {0};

vec_t *vup = NULL;
vec_t *vpn = NULL;
vec_t *vright = NULL;
vec_t *r_origin = NULL;
vec_t *modelorg = NULL;
vec_t *r_entorigin = NULL;
float *r_world_matrix = NULL;
float *r_projection_matrix = NULL;
float *gWorldToScreen = NULL;
float *gScreenToWorld = NULL;
overviewInfo_t *gDevOverview = NULL;
mplane_t *frustum = NULL;

qboolean* vertical_fov_SvEngine = NULL;

vec_t* cl_simorg = NULL;

int *g_bUserFogOn = NULL;
float *g_UserFogColor = NULL;
float *g_UserFogDensity = NULL;
float *g_UserFogStart = NULL;
float *g_UserFogEnd = NULL;

/*
	r_visframecount is updated only when you encounter a new leaf
	while r_framecount is updated every new frame
*/

int *r_framecount = NULL;
int *r_visframecount = NULL;

int *cl_max_edicts = NULL;
cl_entity_t **cl_entities = NULL;

TEMPENTITY *gTempEnts = NULL;

int *cl_viewentity = NULL;
void *cl_frames = NULL;
int size_of_frame = sizeof(frame_t);
int *cl_parsecount = NULL;
int *cl_waterlevel = NULL;
double *cl_time = NULL;
double *cl_oldtime = NULL;
int *envmap = NULL;
int *cl_stats = NULL;
float *cl_weaponstarttime = NULL;
int *cl_weaponsequence = NULL;
int *cl_light_level = NULL;
int *c_alias_polys = NULL;
int *c_brush_polys = NULL;
int(*rtable)[20][20] = NULL;

model_t *mod_known = NULL;
int *mod_numknown = NULL;

char (*loadname)[64] = NULL;
model_t **loadmodel = NULL;

int gl_max_ubo_size = 0;
int gl_max_texture_size = 0;
float gl_max_ansio = 0;

int *gl_msaa_fbo = NULL;
int *gl_backbuffer_fbo = NULL;
int *gl_mtexable = NULL;
qboolean *mtexenabled = 0;

vec_t *r_soundOrigin = NULL;
vec_t *r_playerViewportAngles = NULL;

cactive_t *cls_state = NULL;
int *cls_signon = NULL;
qboolean *scr_drawloading = NULL;

movevars_t* pmovevars = NULL;
struct playermove_s* pmove = NULL;
struct playermove_10152_s* pmove_10152 = NULL;

int *filterMode = NULL;
float *filterColorRed = NULL;
float *filterColorGreen = NULL;
float *filterColorBlue = NULL;
float *filterBrightness = NULL;

bool* detTexSupported = NULL;

cache_system_t(*cache_head) = NULL;

texture_t** r_notexture_mip = NULL;

//Sven Co-op only
texture_t** r_missingtexture = NULL;

//Sven Co-op only
int* allow_cheats = NULL;

//Blob Engine only
int* allocated_textures = NULL;

//client dll

int *g_iUser1 = NULL;
int *g_iUser2 = NULL;

int* g_iWaterLevel = NULL;
bool *g_bRenderingPortals_SCClient = NULL;
int* g_ViewEntityIndex_SCClient = NULL;//Sniber NMSL

float* g_iFogColor_SCClient = NULL;
float* g_iStartDist_SCClient = NULL;
float* g_iEndDist_SCClient = NULL;

bool g_bPortalClipPlaneEnabled[6] = { false };

vec4_t g_PortalClipPlane[6] = {0};

bool g_bHasLowerBody = false;

float r_entity_matrix[4][4] = { 0 };
float r_entity_color[4] = {0};

//This is the very first pass for studiomodel mesh analysis
bool r_draw_analyzingstudio = false;

//This is when drawing a studiomodel has mesh with flag STUDIO_NF_ALPHA, STUDIO_NF_ADDITIVE or with renderfx=kRenderFxGlowShell in opaque pass, and this studiomodel need to be put into the transparent queue and draw again later.
bool r_draw_deferredtrans = false;

//This is to mark the studiomodel with flag STUDIO_NF_ALPHA
bool r_draw_hasalpha = false;

//This is to mark the studiomodel with flag STUDIO_NF_ADDITIVE
bool r_draw_hasadditive = false;

bool r_draw_hasface = false;

bool r_draw_hashair = false;

//This is to mark the studiomodel with outline
bool r_draw_hasoutline = false;

bool r_draw_shadowcaster = false;
bool r_draw_shadowscene = false;

bool r_draw_opaque = false;
bool r_draw_oitblend = false;
bool r_draw_gammablend = false;
bool r_draw_legacysprite = false;
bool r_draw_reflectview = false;
bool r_draw_refractview = false;
bool r_draw_portalview = false;

int r_renderview_pass = 0;

std::list<IDeferredFrameTask*> g_DeferredFrameTasks;

int glx = 0;
int gly = 0;
int glwidth = 0;
int glheight = 0;

FBO_Container_t s_FinalBufferFBO = { 0 };
FBO_Container_t s_BackBufferFBO = { 0 };
FBO_Container_t s_BackBufferFBO2 = { 0 };
FBO_Container_t s_BackBufferFBO3 = { 0 };
FBO_Container_t s_GBufferFBO = { 0 };
FBO_Container_t s_DownSampleFBO[DOWNSAMPLE_BUFFERS] = { 0 };
FBO_Container_t s_LuminFBO[LUMIN_BUFFERS] = { 0 };
FBO_Container_t s_Lumin1x1FBO[LUMIN1x1_BUFFERS] = { 0 };
FBO_Container_t s_BrightPassFBO = { 0 };
FBO_Container_t s_BlurPassFBO[BLUR_BUFFERS][2] = { 0 };
FBO_Container_t s_BrightAccumFBO = { 0 };
FBO_Container_t s_ToneMapFBO = { 0 };
FBO_Container_t s_DepthLinearFBO = { 0 };
FBO_Container_t s_HBAOCalcFBO = { 0 };
FBO_Container_t s_ShadowFBO = { 0 };
FBO_Container_t s_WaterSurfaceFBO = { 0 };

FBO_Container_t* g_CurrentSceneFBO = NULL;
FBO_Container_t *g_CurrentRenderingFBO = NULL;

bool g_bNoStretchAspect = true;
bool g_bUseOITBlend = false;
bool g_bUseLegacyTextureLoader = false;
bool g_bHasOfficialFBOSupport = false;
bool g_bHasOfficialGLTexAllocSupport = true;

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
cvar_t *r_cullsequencebox = NULL;

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
cvar_t *gl_overbright = NULL;
cvar_t *gl_envmapsize = NULL;
cvar_t *gl_flipmatrix = NULL;
cvar_t *gl_monolights = NULL;
cvar_t *gl_fog = NULL;
cvar_t *gl_wireframe = NULL;
cvar_t *gl_ansio = NULL;
cvar_t *developer = NULL;
cvar_t* sv_cheats = NULL;
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
cvar_t *spec_pip = NULL;

cvar_t *default_fov = NULL;
cvar_t *viewmodel_fov = NULL;

cvar_t *r_vertical_fov = NULL;
cvar_t* gl_widescreen_yfov = NULL;

cvar_t* cl_fixmodelinterpolationartifacts = NULL;

cvar_t *dev_overview_color = NULL;

cvar_t* r_gamma_blend = NULL;

cvar_t *r_linear_blend_shift = NULL;

cvar_t *r_linear_fog_shift = NULL;

cvar_t *r_linear_fog_shiftz = NULL;

cvar_t* r_fog_trans = NULL;

cvar_t* r_detailskytextures = NULL;

cvar_t* r_sprite_lerping = NULL;

cvar_t* r_drawlowerbody = NULL;

cvar_t* r_drawlowerbodyattachments = NULL;

cvar_t* r_drawlowerbodypitch = NULL;

cvar_t* r_leaf_lazy_load = NULL;

cvar_t* r_wsurf_parallax_scale = NULL;
cvar_t* r_wsurf_sky_fog = NULL;
cvar_t* r_wsurf_zprepass = NULL;

/*
	Purpose : Check if we can render fog
*/

bool R_CanRenderFog()
{
	if ((*r_refdef.onlyClientDraws))
		return false;

	if (CL_IsDevOverviewMode())
		return false;

	//if (R_IsRenderingWaterView())
	//	return false;

	return true;
}

/*
	Purpose : Check if we are rendering with fog
*/

bool R_IsRenderingFog()
{
	return r_fog_mode && r_fog_enabled;
}

/*
	Purpose : Check if we are rendering into GBuffer
*/

bool R_IsRenderingGBuffer()
{
	return GL_GetCurrentRenderingFBO() == &s_GBufferFBO;
}

/*
	Purpose : Check if we are rendering into a gamma space buffer
*/

bool R_IsRenderingGammaBlending()
{
	return r_draw_gammablend;
}

/*
	Purpose : Check if we are rendering Shadow Pass
*/

bool R_IsRenderingShadowView(void)
{
	return r_draw_shadowcaster;
}

/*
	Purpose : Check if we are rendering Water Pass
*/

bool R_IsRenderingWaterView(void)
{
	return r_draw_reflectview || r_draw_refractview;
}

bool R_IsRenderingReflectView(void)
{
	return r_draw_reflectview;
}

bool R_IsRenderingRefractView(void)
{
	return r_draw_refractview;
}

/*
	Purpose: Check if we are rendering viewmodel
*/

bool R_IsRenderingViewModel(void)
{
	return (*currententity) == cl_viewent;
}

bool R_IsRenderingFlippedViewModel(void)
{
	if (cl_righthand && cl_righthand->value > 0)
	{
		return R_IsRenderingViewModel();
	}

	return false;
}

/*
	Purpose : Check if we are rendering Portal Pass
*/

bool R_IsRenderingPortal(void)
{
	return g_bRenderingPortals_SCClient && (*g_bRenderingPortals_SCClient) == 1;
}

/*
	Purpose: Check if we are rendering lowerbody entity
*/

bool R_IsLowerBodyEntity(cl_entity_t *ent)
{
	return ent == gEngfuncs.GetLocalPlayer() && g_bHasLowerBody;
}

bool R_IsRenderingLowerBody(void)
{
	return R_IsLowerBodyEntity((*currententity));
}

bool R_IsRenderingClippedLowerBody(void)
{
	return R_IsLowerBodyEntity((*currententity)) && !R_IsRenderingShadowView() && !R_IsRenderingPortal();
}

bool R_IsRenderingFirstPersonView()
{
	return !gExportfuncs.CL_IsThirdPerson() && !chase_active->value && !(*envmap) && (*cl_viewentity) <= r_params.maxclients;
}

bool R_ShouldDrawViewModel()
{
	if (!r_drawviewmodel->value ||
		gExportfuncs.CL_IsThirdPerson() ||
		chase_active->value ||
		(*envmap) ||
		!r_drawentities->value ||
		cl_stats[0] <= 0 ||
		!cl_viewent->model ||
		(*cl_viewentity) > r_params.maxclients)
	{
		return false;
	}

	return true;
}


/*
	Purpose : Check if we are in SinglePlayer game
*/

qboolean Host_IsSinglePlayerGame()
{
	return gPrivateFuncs.Host_IsSinglePlayerGame();
}

bool AllowCheats()
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		return (*allow_cheats) != 0;
	}

	return (sv_cheats->value != 0) ? true : false;
}

/*
	Purpose : Check if the box specified by mins and maxs in world space can be culled by camera frustrum, return true on culled
*/

qboolean R_CullBox(vec3_t mins, vec3_t maxs)
{
	if (r_draw_shadowscene)
		return false;

	return gPrivateFuncs.R_CullBox(mins, maxs);
}

/*
	Purpose : Implement "cl_fixmodelinterpolationartifacts" for pre-HL25 engine
*/

void R_ResetLatched_Patched(cl_entity_t* ent, qboolean full_reset)
{
	if (cl_fixmodelinterpolationartifacts && cl_fixmodelinterpolationartifacts->value)
	{
		auto at = ent->curstate.animtime;

		gPrivateFuncs.R_ResetLatched(ent, full_reset);

		ent->latched.prevanimtime = ent->curstate.animtime = at;
	}
	else
	{
		gPrivateFuncs.R_ResetLatched(ent, full_reset);
	}
}

void R_RotateForTransform(const float *in_origin, const float* in_angles)
{
	int i;
	vec3_t angles;
	vec3_t modelpos;

	VectorCopy(in_origin, modelpos);
	VectorCopy(in_angles, angles);

	float entity_matrix[4][4];

	const float r_identity_matrix[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	memcpy(entity_matrix, r_identity_matrix, sizeof(r_identity_matrix));
	Matrix4x4_CreateFromEntity(entity_matrix, angles, modelpos, 1);
	Matrix4x4_Transpose(r_entity_matrix, entity_matrix);
}

/*
	Purpose : Setup world matrix for this entity
*/

void R_RotateForEntity(cl_entity_t *e)
{
	int i;
	vec3_t angles;
	vec3_t modelpos;

	VectorCopy(e->origin, modelpos);
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

	const float r_identity_matrix[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	memcpy(entity_matrix, r_identity_matrix, sizeof(r_identity_matrix));
	Matrix4x4_CreateFromEntity(entity_matrix, angles, modelpos, 1);
	Matrix4x4_Transpose(r_entity_matrix, entity_matrix);
}

float R_GlowBlend(cl_entity_t *entity)
{
	if (gPrivateFuncs.R_GlowBlend)
	{
		return gPrivateFuncs.R_GlowBlend(entity);
	}

	//pmove->PM_PlayerTrace might be NULL in the first frame because it's not initalized yet.
	if (pmove_10152 && pmove_10152->PM_PlayerTrace)
	{
		vec3_t tmp;
		float dist, brightness;
		pmtrace_t trace;

		VectorSubtract(r_entorigin, r_origin, tmp);
		dist = VectorLength(tmp);

		pmove_10152->usehull = 2;
		trace = pmove_10152->PM_PlayerTrace(r_origin, r_entorigin, r_traceglow->value ? PM_GLASS_IGNORE : (PM_GLASS_IGNORE | PM_STUDIO_IGNORE), -1);
		if ((1.0 - trace.fraction) * dist > 8)
			return 0;

		if (entity->curstate.renderfx == kRenderFxNoDissipation)
		{
			return (float)entity->curstate.renderamt * (1.0f / 255.0f);
		}

		// UNDONE: Tweak these magic numbers (19000 - falloff & 200 - sprite size)
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
	else if (pmove && pmove->PM_PlayerTrace)
	{
		vec3_t tmp;
		float dist, brightness;
		pmtrace_t trace;

		VectorSubtract(r_entorigin, r_origin, tmp);
		dist = VectorLength(tmp);

		pmove->usehull = 2;
		trace = pmove->PM_PlayerTrace(r_origin, r_entorigin, r_traceglow->value ? PM_GLASS_IGNORE : (PM_GLASS_IGNORE | PM_STUDIO_IGNORE), -1);
		if ((1.0 - trace.fraction) * dist > 8)
			return 0;

		if (entity->curstate.renderfx == kRenderFxNoDissipation)
		{
			return (float)entity->curstate.renderamt * (1.0f / 255.0f);
		}

		// UNDONE: Tweak these magic numbers (19000 - falloff & 200 - sprite size)
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

	return 0;
}

int CL_FxBlend(cl_entity_t *entity)
{
	//Hack for R_DrawSpriteModel
	if (entity->model && entity->model->type == mod_sprite && entity->curstate.rendermode == kRenderNormal && entity->curstate.renderamt == 0)
	{
		return 255;
	}

	auto EntityComponent = R_GetEntityComponentContainer((*currententity), false);

	if (EntityComponent && EntityComponent->DeferredStudioPasses.size())
	{
		return 255;
	}

	return gPrivateFuncs.CL_FxBlend(entity);
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

void R_DrawParticles(void)
{
	if (CL_IsDevOverviewMode())
		return;

	gPrivateFuncs.R_FreeDeadParticles(&(*active_particles));

	vec3_t			up, right;
	float			scale;

	GL_Bind((*particletexture));

#if 0
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	program_state_t LegacySpriteProgramState = 0;

	if (!R_IsRenderingGBuffer())
	{
		if (R_IsRenderingFog())
		{
			if (r_fog_mode == GL_LINEAR)
			{
				LegacySpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
			}
			else if (r_fog_mode == GL_EXP)
			{
				LegacySpriteProgramState |= SPRITE_EXP_FOG_ENABLED;
			}
			else if (r_fog_mode == GL_EXP2)
			{
				LegacySpriteProgramState |= SPRITE_EXP2_FOG_ENABLED;
			}

			if (!R_IsRenderingGammaBlending() && r_linear_fog_shift->value > 0)
			{
				LegacySpriteProgramState |= SPRITE_LINEAR_FOG_SHIFT_ENABLED;
			}
		}
	}

	if (R_IsRenderingWaterView())
	{
		LegacySpriteProgramState |= SPRITE_CLIP_ENABLED;
	}

	if (r_draw_oitblend)
	{
		LegacySpriteProgramState |= SPRITE_OIT_BLEND_ENABLED;
	}

	if (R_IsRenderingGammaBlending())
	{
		LegacySpriteProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
	}

	R_UseLegacySpriteProgram(LegacySpriteProgramState, NULL);
#endif

	gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);

	VectorScale(vup, 1.5, up);
	VectorScale(vright, 1.5, right);

	float frametime = (*cl_time) - (*cl_oldtime);
	float time3 = frametime * 15;
	float time2 = frametime * 10; // 15;
	float time1 = frametime * 5;
	float grav = frametime * (r_params.movevars ? r_params.movevars->gravity : 800) * 0.05;
	float dvel = 4 * frametime;

	for (auto p = (*active_particles); p; p = p->next)
	{
		if (p->type != pt_blob)
		{
			// hack a scale up to keep particles from disappearing
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

			gEngfuncs.pTriAPI->Color4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			gEngfuncs.pTriAPI->Vertex3fv(p->org);
			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			gEngfuncs.pTriAPI->Vertex3f(p->org[0] + up[0] * scale, p->org[1] + up[1] * scale, p->org[2] + up[2] * scale);
			gEngfuncs.pTriAPI->TexCoord2f(0, 1);
			gEngfuncs.pTriAPI->Vertex3f(p->org[0] + right[0] * scale, p->org[1] + right[1] * scale, p->org[2] + right[2] * scale);
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

	gEngfuncs.pTriAPI->End();

	gPrivateFuncs.R_TracerDraw();
	gPrivateFuncs.R_BeamDrawList();

	glDisable(GL_BLEND);
	//glDisable(GL_ALPHA_TEST);//TODO: do in shader
}

mbasenode_t* R_PVSNode(mbasenode_t* basenode, vec3_t emins, vec3_t emaxs)
{
	mplane_t* splitplane;
	int			sides;
	mbasenode_t* splitNode;

	//if (basenode->visframe != (*r_visframecount))
	//	return NULL;

	// add an efrag if the node is a leaf

	if (basenode->contents < 0)
	{
		if (basenode->contents == CONTENT_SOLID)
			return NULL;

		return basenode;
	}

	auto node = (mnode_t*)basenode;

	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE(emins, emaxs, splitplane);

	// recurse down the contacted sides
	if (sides & 1)
	{
		splitNode = R_PVSNode(node->children[0], emins, emaxs);

		if (splitNode)
			return splitNode;
	}

	if (sides & 2)
	{
		splitNode = R_PVSNode(node->children[1], emins, emaxs);

		if (splitNode)
			return splitNode;
	}

	return NULL;
}

mbasenode_t* PVSNode(mbasenode_t* basenode, vec3_t emins, vec3_t emaxs)
{
	return R_PVSNode(basenode, emins, emaxs);
}

class CTriAPICommand
{
public:
	int GLPrimitiveCode{};
	vec2_t TexCoord{};
	vec4_t DrawColor{};
	vec4_t GLColor{};
	std::vector<vertex3f_t> Positions{};
	std::vector<triapivertex_t> Vertices{};
	int RenderMode{ };
	int DrawRenderMode{ };
	GLuint hVBO{};
	GLuint hEBO{};
	GLuint hVAO{};
};

CTriAPICommand gTriAPICommand;

void triapi_Shutdown()
{
	if(gTriAPICommand.hVBO)
	{
		GL_DeleteBuffer(gTriAPICommand.hVBO);
		gTriAPICommand.hVBO = 0;
	}
	if(gTriAPICommand.hEBO)
	{
		GL_DeleteBuffer(gTriAPICommand.hEBO);
		gTriAPICommand.hEBO = 0;
	}
	if(gTriAPICommand.hVAO)
	{
		GL_DeleteVAO(gTriAPICommand.hVAO);
		gTriAPICommand.hVAO = 0;
	}
}

void triapi_RenderMode(int mode)
{
	gTriAPICommand.RenderMode = mode;
}

void triapi_Begin(int primitiveCode)
{
	if (!(primitiveCode >= TRI_TRIANGLES && primitiveCode <= TRI_QUAD_STRIP))
	{
		Sys_Error(__FUNCTION__": invalid primitive %d !", primitiveCode);
		return;
	}

	const int tri_GL_Modes[7] =
	{
		GL_TRIANGLES,
		GL_TRIANGLE_FAN,
		GL_QUADS,
		GL_POLYGON,
		GL_LINES,
		GL_TRIANGLE_STRIP,
		GL_QUAD_STRIP
	};

	gTriAPICommand.GLPrimitiveCode = tri_GL_Modes[primitiveCode];
	gTriAPICommand.DrawRenderMode = gTriAPICommand.RenderMode;
}

void triapi_EndClear()
{
	gTriAPICommand.Positions.clear();
	gTriAPICommand.Vertices.clear();
}

void triapi_End()
{
	std::vector<GLuint> Indices;	

	size_t n = gTriAPICommand.Vertices.size();
	
	// 如果没有顶点数据，直接返回
	if (n == 0)
	{
		triapi_EndClear();
		return;
	}

	if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLES)
	{
		// 三角形列表 - 直接使用索引
		if (n < 3) 
		{
			triapi_EndClear();
			return;
		}

		for (size_t i = 0; i < n; i += 3)
		{
			if (i + 2 < n)
			{
				Indices.push_back((GLuint)i);
				Indices.push_back((GLuint)i + 1);
				Indices.push_back((GLuint)i + 2);
			}
		}
	}
	else if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLE_FAN)
	{
		// 三角形扇形 - 转换为三角形列表索引
		if (n < 3) 
		{
			triapi_EndClear();
			return;
		}

		for (size_t i = 1; i < n - 1; ++i)
		{
			Indices.push_back(0);           // 扇形中心
			Indices.push_back((GLuint)i);
			Indices.push_back((GLuint)i + 1);
		}
	}
	else if (gTriAPICommand.GLPrimitiveCode == GL_QUADS)
	{
		// 四边形 - 转换为三角形列表索引
		if (n < 4) 
		{
			triapi_EndClear();
			return;
		}

		for (size_t i = 0; i < n; i += 4)
		{
			if (i + 3 < n)
			{
				// 将四边形分解为两个三角形 (0,1,2) 和 (2,3,0)
				// 第一个三角形
				Indices.push_back((GLuint)i + 0);
				Indices.push_back((GLuint)i + 1);
				Indices.push_back((GLuint)i + 2);
				
				// 第二个三角形
				Indices.push_back((GLuint)i + 2);
				Indices.push_back((GLuint)i + 3);
				Indices.push_back((GLuint)i + 0);
			}
		}
	}
	else if (gTriAPICommand.GLPrimitiveCode == GL_POLYGON)
	{
		if (n < 3) 
		{
			triapi_EndClear();
			return;
		}

		R_PolygonToTriangleList(gTriAPICommand.Positions, Indices);
	}
	else if (gTriAPICommand.GLPrimitiveCode == GL_LINES)
	{
		// 线段 - 直接使用线段索引
		for (size_t i = 0; i < n; i++)
		{
			Indices.push_back((GLuint)i);
		}
	}
	else if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLE_STRIP)
	{
		// 三角形带 - 转换为三角形列表索引
		if (n < 3) 
		{
			triapi_EndClear();
			return;
		}

		for (size_t i = 0; i < n - 2; ++i)
		{
			// 三角形带中每个三角形的顶点顺序需要交替
			if (i % 2 == 0)
			{
				// 偶数索引：正常顺序 (i, i+1, i+2)
				Indices.push_back((GLuint)i);
				Indices.push_back((GLuint)i + 1);
				Indices.push_back((GLuint)i + 2);
			}
			else
			{
				// 奇数索引：反向顺序 (i+1, i, i+2)
				Indices.push_back((GLuint)i + 1);
				Indices.push_back((GLuint)i);
				Indices.push_back((GLuint)i + 2);
			}
		}
	}
	else if (gTriAPICommand.GLPrimitiveCode == GL_QUAD_STRIP)
	{
		// 四边形带 - 转换为三角形列表索引
		if (n < 4) 
		{
			triapi_EndClear();
			return;
		}

		for (size_t i = 0; i + 3 < n; i += 2)
		{
			// 四边形带中每个四边形的四个顶点索引
			GLuint v0 = (GLuint)i;
			GLuint v1 = (GLuint)i + 1;
			GLuint v2 = (GLuint)i + 2;
			GLuint v3 = (GLuint)i + 3;

			// 将四边形分解为两个三角形 (v0,v1,v2) 和 (v2,v3,v0)
			// 第一个三角形
			Indices.push_back(v0);
			Indices.push_back(v1);
			Indices.push_back(v2);

			// 第二个三角形
			Indices.push_back(v2);
			Indices.push_back(v3);
			Indices.push_back(v0);
		}
	}

	// 如果没有生成索引，直接返回
	if (Indices.size() == 0)
	{
		triapi_EndClear();
		return;
	}

	if(!gTriAPICommand.hVBO){
		gTriAPICommand.hVBO = GL_GenBuffer();
	}
	
	size_t VBODataSize = sizeof(triapivertex_t) * gTriAPICommand.Vertices.size();
	GL_UploadDataToVBODynamicDraw(gTriAPICommand.hVBO, VBODataSize, nullptr);
	GL_UploadSubDataToVBODynamicDraw(gTriAPICommand.hVBO, 0, VBODataSize, gTriAPICommand.Vertices.data());

	if(!gTriAPICommand.hEBO){
		gTriAPICommand.hEBO = GL_GenBuffer();
	}
	
	size_t EBODataSize = sizeof(GLuint) * Indices.size();
	GL_UploadDataToEBODynamicDraw(gTriAPICommand.hEBO, EBODataSize, nullptr);
	GL_UploadSubDataToEBODynamicDraw(gTriAPICommand.hEBO, 0, EBODataSize, Indices.data());

	if(!gTriAPICommand.hVAO){
		gTriAPICommand.hVAO = GL_GenVAO();
		GL_BindStatesForVAO(gTriAPICommand.hVAO, gTriAPICommand.hVBO, gTriAPICommand.hEBO, 
			[]() {
				glEnableVertexAttribArray(TRIAPI_VA_POSITION);
				glEnableVertexAttribArray(TRIAPI_VA_TEXCOORD);
				glEnableVertexAttribArray(TRIAPI_VA_COLOR);
				glVertexAttribPointer(TRIAPI_VA_POSITION, 3, GL_FLOAT, false, sizeof(triapivertex_t), OFFSET(triapivertex_t, pos));
				glVertexAttribPointer(TRIAPI_VA_TEXCOORD, 2, GL_FLOAT, false, sizeof(triapivertex_t), OFFSET(triapivertex_t, texcoord));
				glVertexAttribPointer(TRIAPI_VA_COLOR, 4, GL_FLOAT, false, sizeof(triapivertex_t), OFFSET(triapivertex_t, color));
			}, 
			[]() {
				glDisableVertexAttribArray(TRIAPI_VA_POSITION);
				glDisableVertexAttribArray(TRIAPI_VA_TEXCOORD);
				glDisableVertexAttribArray(TRIAPI_VA_COLOR);
			});
	}

	GL_BindVAO(gTriAPICommand.hVAO);
	
	uint64_t ProgramState = 0;

	switch (gTriAPICommand.DrawRenderMode)
	{
	case kRenderNormal:
	{
		glDisable(GL_BLEND);

		if (!R_IsRenderingGBuffer())
		{
			if ((ProgramState & SPRITE_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((ProgramState & SPRITE_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
			{
				if (R_IsRenderingFog())
				{
					if (r_fog_mode == GL_LINEAR)
					{
						ProgramState |= SPRITE_LINEAR_FOG_ENABLED;
					}
					else if (r_fog_mode == GL_EXP)
					{
						ProgramState |= SPRITE_EXP_FOG_ENABLED;
					}
					else if (r_fog_mode == GL_EXP2)
					{
						ProgramState |= SPRITE_EXP2_FOG_ENABLED;
					}

					if (!R_IsRenderingGammaBlending() && r_linear_fog_shift->value > 0)
					{
						ProgramState |= SPRITE_LINEAR_FOG_SHIFT_ENABLED;
					}
				}
			}
		}

		if (R_IsRenderingWaterView())
		{
			ProgramState |= SPRITE_CLIP_ENABLED;
		}

		if (R_IsRenderingGammaBlending())
		{
			ProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (ProgramState & (SPRITE_ALPHA_BLEND_ENABLED | SPRITE_ADDITIVE_BLEND_ENABLED)))
		{
			ProgramState |= SPRITE_OIT_BLEND_ENABLED;
		}
		break;
	}

	case kRenderTransAdd:
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		R_SetGBufferBlend(GL_ONE, GL_ONE);

		ProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;

		if (!R_IsRenderingGBuffer())
		{
			if ((ProgramState & SPRITE_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((ProgramState & SPRITE_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
			{
				if (R_IsRenderingFog())
				{
					if (r_fog_mode == GL_LINEAR)
					{
						ProgramState |= SPRITE_LINEAR_FOG_ENABLED;
					}
					else if (r_fog_mode == GL_EXP)
					{
						ProgramState |= SPRITE_EXP_FOG_ENABLED;
					}
					else if (r_fog_mode == GL_EXP2)
					{
						ProgramState |= SPRITE_EXP2_FOG_ENABLED;
					}

					if (!R_IsRenderingGammaBlending() && r_linear_fog_shift->value > 0)
					{
						ProgramState |= SPRITE_LINEAR_FOG_SHIFT_ENABLED;
					}
				}
			}
		}

		if (R_IsRenderingWaterView())
		{
			ProgramState |= SPRITE_CLIP_ENABLED;
		}

		if (R_IsRenderingGammaBlending())
		{
			ProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (ProgramState & (SPRITE_ALPHA_BLEND_ENABLED | SPRITE_ADDITIVE_BLEND_ENABLED)))
		{
			ProgramState |= SPRITE_OIT_BLEND_ENABLED;
		}
		break;
	}

	case kRenderTransAlpha:
	case kRenderTransColor:
	case kRenderTransTexture:
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		ProgramState |= SPRITE_ALPHA_BLEND_ENABLED;

		if (R_IsRenderingWaterView())
		{
			ProgramState |= SPRITE_CLIP_ENABLED;
		}

		if (!R_IsRenderingGBuffer())
		{
			if ((ProgramState & SPRITE_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
			{

			}
			else if ((ProgramState & SPRITE_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
			{

			}
			else
			{
				if (R_IsRenderingFog())
				{
					if (r_fog_mode == GL_LINEAR)
					{
						ProgramState |= SPRITE_LINEAR_FOG_ENABLED;
					}
					else if (r_fog_mode == GL_EXP)
					{
						ProgramState |= SPRITE_EXP_FOG_ENABLED;
					}
					else if (r_fog_mode == GL_EXP2)
					{
						ProgramState |= SPRITE_EXP2_FOG_ENABLED;
					}

					if (!R_IsRenderingGammaBlending() && r_linear_fog_shift->value > 0)
					{
						ProgramState |= SPRITE_LINEAR_FOG_SHIFT_ENABLED;
					}
				}
			}
		}

		if (R_IsRenderingGammaBlending())
		{
			ProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
		}

		if (r_draw_oitblend && (ProgramState & (SPRITE_ALPHA_BLEND_ENABLED | SPRITE_ADDITIVE_BLEND_ENABLED)))
		{
			ProgramState |= SPRITE_OIT_BLEND_ENABLED;
		}
		break;
	}
	}

	triapi_program_t prog{};
	R_UseTriAPIProgram(ProgramState, &prog);

	// 根据图元类型选择正确的绘制模式
	if (gTriAPICommand.GLPrimitiveCode == GL_LINES)
	{
		glDrawElements(GL_LINES, Indices.size(), GL_UNSIGNED_INT, 0);
	}
	else 
	{
		glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
	}

	GL_UseProgram(0);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	GL_BindVAO(0);
	
	triapi_EndClear();
}

void triapi_Color4f(float r, float g, float b, float a)
{
	gTriAPICommand.GLColor[0] = r;
	gTriAPICommand.GLColor[1] = g;
	gTriAPICommand.GLColor[2] = b;
	gTriAPICommand.GLColor[3] = a;

	if (gTriAPICommand.RenderMode == kRenderTransAlpha)
	{
		gTriAPICommand.DrawColor[0] = r;
		gTriAPICommand.DrawColor[1] = g;
		gTriAPICommand.DrawColor[2] = b;
		gTriAPICommand.DrawColor[3] = a;
	}
	else
	{
		gTriAPICommand.DrawColor[0] = r * a;
		gTriAPICommand.DrawColor[1] = g * a;
		gTriAPICommand.DrawColor[2] = b * a;
		gTriAPICommand.DrawColor[3] = 1;
	}
}

void triapi_Color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	gTriAPICommand.GLColor[0] = r / 255.0;
	gTriAPICommand.GLColor[1] = g / 255.0;
	gTriAPICommand.GLColor[2] = b / 255.0;
	gTriAPICommand.GLColor[3] = a / 255.0;

	gTriAPICommand.DrawColor[0] = gTriAPICommand.GLColor[0];
	gTriAPICommand.DrawColor[1] = gTriAPICommand.GLColor[1];
	gTriAPICommand.DrawColor[2] = gTriAPICommand.GLColor[2];
	gTriAPICommand.DrawColor[3] = 1;
}

void triapi_Vertex3fv(float* v)
{
	vertex3f_t pos{};
	pos.v[0] = v[0];
	pos.v[1] = v[1];
	pos.v[2] = v[2];

	gTriAPICommand.Positions.emplace_back(pos);

	triapivertex_t vertex{};
	vertex.pos[0] = v[0];
	vertex.pos[1] = v[1];
	vertex.pos[2] = v[2];
	vertex.texcoord[0] = gTriAPICommand.TexCoord[0];
	vertex.texcoord[1] = gTriAPICommand.TexCoord[1];
	vertex.color[0] = gTriAPICommand.DrawColor[0];
	vertex.color[1] = gTriAPICommand.DrawColor[1];
	vertex.color[2] = gTriAPICommand.DrawColor[2];
	vertex.color[3] = gTriAPICommand.DrawColor[3];

	gTriAPICommand.Vertices.push_back(vertex);
}

void triapi_Vertex3f(float x, float y, float z)
{
	vertex3f_t pos{};
	pos.v[0] = x;
	pos.v[1] = y;
	pos.v[2] = z;

	gTriAPICommand.Positions.emplace_back(pos);

	triapivertex_t vertex{};
	vertex.pos[0] = x;
	vertex.pos[1] = y;
	vertex.pos[2] = z;
	vertex.texcoord[0] = gTriAPICommand.TexCoord[0];
	vertex.texcoord[1] = gTriAPICommand.TexCoord[1];
	vertex.color[0] = gTriAPICommand.DrawColor[0];
	vertex.color[1] = gTriAPICommand.DrawColor[1];
	vertex.color[2] = gTriAPICommand.DrawColor[2];
	vertex.color[3] = gTriAPICommand.DrawColor[3];
	
	gTriAPICommand.Vertices.push_back(vertex);
}

void triapi_TexCoord2f(float s, float t)
{
	gTriAPICommand.TexCoord[0] = s;
	gTriAPICommand.TexCoord[1] = t;
}

void triapi_Brightness(float brightness)
{
	gTriAPICommand.DrawColor[0] = gTriAPICommand.GLColor[0] * gTriAPICommand.GLColor[3] * brightness;
	gTriAPICommand.DrawColor[1] = gTriAPICommand.GLColor[1] * gTriAPICommand.GLColor[3] * brightness;
	gTriAPICommand.DrawColor[2] = gTriAPICommand.GLColor[2] * gTriAPICommand.GLColor[3] * brightness;
	gTriAPICommand.DrawColor[3] = 1;
}

void triapi_Color4fRendermode(float r, float g, float b, float a, int rendermode)
{
	if (gTriAPICommand.RenderMode == kRenderTransAlpha)
	{
		gTriAPICommand.GLColor[3] = a / 255;
	}

	if (gTriAPICommand.RenderMode == kRenderTransAlpha)
	{
		gTriAPICommand.DrawColor[0] = r;
		gTriAPICommand.DrawColor[1] = g;
		gTriAPICommand.DrawColor[2] = b;
		gTriAPICommand.DrawColor[3] = a;
	}
	else
	{
		gTriAPICommand.DrawColor[0] = r * a;
		gTriAPICommand.DrawColor[1] = g * a;
		gTriAPICommand.DrawColor[2] = b * a;
		gTriAPICommand.DrawColor[3] = 1;
	}
}

void triapi_GetMatrix(const int pname, float* matrix)
{
	if (pname == GL_MODELVIEW_MATRIX)
	{
		memcpy(matrix, r_world_matrix, sizeof(float[16]));
		return;
	}
	else if (pname == GL_PROJECTION_MATRIX)
	{
		memcpy(matrix, r_projection_matrix, sizeof(float[16]));
		return;
	}
	Sys_Error("triapi_GetMatrix: Invalid matrix type %d !", pname);
	//return gPrivateFuncs.triapi_GetMatrix(pname, matrix);
}

int triapi_BoxInPVS(float* mins, float* maxs)
{
	return R_PVSNode((*cl_worldmodel)->nodes, mins, maxs) != NULL;
}

void triapi_Fog(float* flFogColor, float flStart, float flEnd, BOOL bOn)
{
	gPrivateFuncs.triapi_Fog(flFogColor, flStart, flEnd, bOn);
}

void __stdcall SCClient_glBegin(int GLPrimitiveCode)
{
	gTriAPICommand.GLPrimitiveCode = GLPrimitiveCode;
	gTriAPICommand.DrawRenderMode = gTriAPICommand.RenderMode;
}

void __stdcall SCClient_glEnd()
{
	triapi_End();
}

void __stdcall SCClient_glColor4f(float r, float g, float b, float a)
{
	gTriAPICommand.DrawColor[0] = r;
	gTriAPICommand.DrawColor[1] = g;
	gTriAPICommand.DrawColor[2] = b;
	gTriAPICommand.DrawColor[3] = a;
}

void R_DrawTEntitiesOnList(int onlyClientDraw)
{
	if (onlyClientDraw)
		return;

	for (int i = 0; i < (*numTransObjs); i++)
	{
		(*currententity) = (*transObjects)[i].pEnt;

		R_DrawCurrentEntity(true);
	}
}

void ClientDLL_DrawTransparentTriangles(void)
{
	//Call ClientDLL_DrawTransparentTriangles() instead of HUD_DrawTransparentTriangles
	gPrivateFuncs.ClientDLL_DrawTransparentTriangles();

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void R_DrawTransEntities(int onlyClientDraw)
{
	if (R_IsRenderingShadowView())
		return;

	if (g_bUseOITBlend)
	{
		glColorMask(0, 0, 0, 0);

		R_ClearOITBuffer();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		r_draw_oitblend = true;

		r_draw_legacysprite = true;

		if (r_drawentities->value)
		{
			R_DrawTEntitiesOnList(onlyClientDraw);

			GL_DisableMultitexture();

			glEnable(GL_ALPHA_TEST);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			R_InhibitRenderingFog();

			ClientDLL_DrawTransparentTriangles();

			R_RestoreRenderingFog();

			(*numTransObjs) = 0;
			(*r_blend) = 1;
		}

		if (!onlyClientDraw)
		{
			GL_DisableMultitexture();
			R_DrawParticles();
		}

		r_draw_legacysprite = false;

		r_draw_oitblend = false;

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		glColorMask(1, 1, 1, 1);

		GL_BlitFrameBufferToFrameBufferColorOnly(GL_GetCurrentSceneFBO(), &s_BackBufferFBO2);
		R_BlendOITBuffer(&s_BackBufferFBO2, GL_GetCurrentSceneFBO());

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	else
	{
		r_draw_legacysprite = true;

		if (r_drawentities->value)
		{
			R_DrawTEntitiesOnList(onlyClientDraw);

			GL_DisableMultitexture();

			glEnable(GL_ALPHA_TEST);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			R_InhibitRenderingFog();

			ClientDLL_DrawTransparentTriangles();

			R_RestoreRenderingFog();

			(*numTransObjs) = 0;
			(*r_blend) = 1;
		}

		if (!onlyClientDraw)
		{
			GL_DisableMultitexture();
			R_DrawParticles();
		}

		r_draw_legacysprite = false;
	}

	GL_UseProgram(0);
}

void R_AddTEntity(cl_entity_t *ent)
{
	if (R_IsRenderingShadowView())
		return;

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

	if ((*numTransObjs) >= (*maxTransObjs))
	{
		g_pMetaHookAPI->SysError("R_AddTEntity: Too many objects");
		return;
	}

	if (g_bUseOITBlend)
	{
		(*transObjects)[(*numTransObjs)].pEnt = ent;
		(*transObjects)[(*numTransObjs)].distance = 0;
		(*numTransObjs)++;
	}
	else
	{
		float dist;
		vec3_t v;

		if (!ent->model || ent->model->type != mod_brush || ent->curstate.rendermode != kRenderTransAlpha)
		{
			VectorAdd(ent->model->mins, ent->model->maxs, v);
			VectorScale(v, 0.5f, v);
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
	if (!(index >= 0 && index <= MAX_CLIENTS))
	{
		Sys_Error("R_GetPlayerState: Invalid index %d !", index);
		return nullptr;
	}

	return ((entity_state_t *)((char *)cl_frames + size_of_frame * ((*cl_parsecount) & 63) + sizeof(entity_state_t) * index));
}

void R_DrawSpriteEntity(bool bTransparent)
{
	if ((*currententity)->curstate.body)
	{
		float* pAttachment = R_GetAttachmentPoint((*currententity)->curstate.skin, (*currententity)->curstate.body);
		VectorCopy(pAttachment, r_entorigin);
	}
	else
	{
		VectorCopy((*currententity)->origin, r_entorigin);
	}

	if (bTransparent)
	{
		if ((*currententity)->curstate.rendermode == kRenderGlow)
			(*r_blend) *= R_GlowBlend((*currententity));
	}
	else
	{
		(*r_blend) = 1;
	}

	if ((*r_blend) > 0)
	{
		R_DrawSpriteModel((*currententity));
	}
}

void R_DrawBrushEntity(bool bTransparent)
{
	if (bTransparent)
	{
		if ((*g_bUserFogOn))
		{
			if ((*currententity)->curstate.rendermode != kRenderGlow && (*currententity)->curstate.rendermode != kRenderTransAdd)
			{
				R_RestoreRenderingFog();
			}
		}
	}

	R_DrawBrushModel((*currententity));
}

void R_DrawStudioEntity(bool bTransparent)
{
	if ((*currententity)->player)
	{
		if (R_IsLowerBodyEntity((*currententity)))
		{
			if (R_IsRenderingPortal())
			{
				return;
			}

			(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER, R_GetPlayerState((*currententity)->index));
		}
		else
		{
			(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS, R_GetPlayerState((*currententity)->index));
		}
	}
	else
	{
		if ((*currententity)->curstate.movetype == MOVETYPE_FOLLOW)
		{
			auto aiment = gEngfuncs.GetEntityByIndex((*currententity)->curstate.aiment);

			//The aiment is invalid ?
			if (!aiment)
			{
				return;
			}

			//The aiment is invisible ?
			if (!EngineIsEntityInVisibleList(aiment))
			{
				return;
			}

			if (R_IsLowerBodyEntity(aiment) && (int)r_drawlowerbodyattachments->value < 1)
			{
				return;
			}

			if (aiment->model && aiment->model->type == mod_studio)
			{
				auto saved_currententity = (*currententity);

				(*currententity) = aiment;

				if ((*currententity)->player)
				{
					(*gpStudioInterface)->StudioDrawPlayer(0, R_GetPlayerState((*currententity)->index));
				}
				else
				{
					(*gpStudioInterface)->StudioDrawModel(0);
				}

				(*currententity) = saved_currententity;
			}

			if ((*currententity)->player)
			{
				(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS, R_GetPlayerState((*currententity)->index));
			}
			else
			{
				(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS);
			}

			return;
		}

		(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS);
	}
}

void R_DrawCurrentEntity(bool bTransparent)
{
	if (bTransparent)
	{
		(*r_blend) = CL_FxBlend((*currententity));

		if ((*r_blend) <= 0)
			return;

		//Why does GoldSrc use double not float?
		(*r_blend) = (*r_blend) / 255.0;

		if ((*currententity)->curstate.rendermode == kRenderGlow && (*currententity)->model->type != mod_sprite)
			gEngfuncs.Con_DPrintf("Non-sprite set to glow!\n");
	}

	switch ((*currententity)->model->type)
	{
	case mod_sprite:
	{
		R_DrawSpriteEntity(bTransparent);
		break;
	}
	case mod_brush:
	{
		R_DrawBrushEntity(bTransparent);
		break;
	}
	case mod_studio:
	{
		R_DrawStudioEntity(bTransparent);
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
	case kRenderGlow:
	{
		r_entity_color[0] = (*r_blend);
		r_entity_color[1] = (*r_blend);
		r_entity_color[2] = (*r_blend);
		r_entity_color[3] = 1;

		glBlendFunc(GL_ONE, GL_ONE);
		glDepthMask(GL_FALSE);
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

		glDisable(GL_BLEND);

		break;
	}

	default:
	{
		r_entity_color[0] = 1;
		r_entity_color[1] = 1;
		r_entity_color[2] = 1;
		r_entity_color[3] = (*r_blend);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		break;
	}
	}
}

void R_PolyBlend(void)
{
	gPrivateFuncs.R_PolyBlend();
}

void S_ExtraUpdate(void)
{
	gPrivateFuncs.S_ExtraUpdate();
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

void R_SetupPerspective(float fovx, float fovy, float zNear, float zFar)
{
	r_xfov_currentpass = fovx;
	r_yfov_currentpass = fovy;

	auto right = tan(fovx * (M_PI / 360.0)) * zNear;
	auto top = tan(fovy * (M_PI / 360.0)) * zNear;

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

void GL_FreeFrameBuffers(void)
{
	GL_FreeFBO(&s_FinalBufferFBO);
	GL_FreeFBO(&s_BackBufferFBO);
	GL_FreeFBO(&s_BackBufferFBO2);
	GL_FreeFBO(&s_BackBufferFBO3);
	GL_FreeFBO(&s_GBufferFBO);
	for (int i = 0; i < DOWNSAMPLE_BUFFERS; ++i)
		GL_FreeFBO(&s_DownSampleFBO[i]);
	for (int i = 0; i < LUMIN_BUFFERS; ++i)
		GL_FreeFBO(&s_LuminFBO[i]);
	for (int i = 0; i < LUMIN1x1_BUFFERS; ++i)
		GL_FreeFBO(&s_Lumin1x1FBO[i]);
	GL_FreeFBO(&s_BrightPassFBO);
	for (int i = 0; i < BLUR_BUFFERS; ++i)
	{
		GL_FreeFBO(&s_BlurPassFBO[i][0]);
		GL_FreeFBO(&s_BlurPassFBO[i][1]);
	}
	GL_FreeFBO(&s_BrightAccumFBO);
	GL_FreeFBO(&s_ToneMapFBO);
	GL_FreeFBO(&s_DepthLinearFBO);
	GL_FreeFBO(&s_HBAOCalcFBO);
	GL_FreeFBO(&s_ShadowFBO);
	GL_FreeFBO(&s_WaterSurfaceFBO);
}

void GL_GenerateFrameBuffers(void)
{
	GL_FreeFrameBuffers();

	s_FinalBufferFBO.iWidth = glwidth;
	s_FinalBufferFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_FinalBufferFBO);
	GL_FrameBufferColorTexture(&s_FinalBufferFBO, GL_RGBA8);
	GL_FrameBufferDepthTexture(&s_FinalBufferFBO, GL_DEPTH24_STENCIL8);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_FinalBufferFBO);
		g_pMetaHookAPI->SysError("Failed to initialize FinalBufferFBO!\n");
	}

	s_BackBufferFBO.iWidth = glwidth;
	s_BackBufferFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_BackBufferFBO);
	GL_FrameBufferColorTexture(&s_BackBufferFBO, GL_RGBA16F);
	GL_FrameBufferDepthTexture(&s_BackBufferFBO, GL_DEPTH24_STENCIL8);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BackBufferFBO);
		g_pMetaHookAPI->SysError("Failed to initialize BackBufferFBO!\n");
	}

	s_BackBufferFBO2.iWidth = glwidth;
	s_BackBufferFBO2.iHeight = glheight;
	GL_GenFrameBuffer(&s_BackBufferFBO2);
	GL_FrameBufferColorTexture(&s_BackBufferFBO2, GL_RGBA16F);
	GL_FrameBufferDepthTexture(&s_BackBufferFBO2, GL_DEPTH24_STENCIL8);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BackBufferFBO2);
		g_pMetaHookAPI->SysError("Failed to initialize BackBufferFBO2!\n");
	}

	s_BackBufferFBO3.iWidth = glwidth;
	s_BackBufferFBO3.iHeight = glheight;
	GL_GenFrameBuffer(&s_BackBufferFBO3);
	GL_FrameBufferColorTexture(&s_BackBufferFBO3, GL_RGBA8);
	GL_FrameBufferDepthTexture(&s_BackBufferFBO3, GL_DEPTH24_STENCIL8);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BackBufferFBO3);
		g_pMetaHookAPI->SysError("Failed to initialize BackBufferFBO3!\n");
	}

	s_GBufferFBO.iWidth = glwidth;
	s_GBufferFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_GBufferFBO);
	
	GL_FrameBufferColorTextureDeferred(&s_GBufferFBO, 
		GBUFFER_INTERNAL_FORMAT_DIFFUSE,
		GBUFFER_INTERNAL_FORMAT_LIGHTMAP,
		GBUFFER_INTERNAL_FORMAT_WORLDNORM,
		GBUFFER_INTERNAL_FORMAT_SPECULAR);

	GL_FrameBufferDepthTexture(&s_GBufferFBO, GL_DEPTH24_STENCIL8);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_GBufferFBO);
		g_pMetaHookAPI->SysError("Failed to initialize GBuffer framebuffer.\n");
	}

	s_DepthLinearFBO.iWidth = glwidth;
	s_DepthLinearFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_DepthLinearFBO);
	GL_FrameBufferColorTexture(&s_DepthLinearFBO, GL_R32F);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_DepthLinearFBO);
		g_pMetaHookAPI->SysError("Failed to initialize DepthLinear framebuffer!\n");
	}

	s_HBAOCalcFBO.iWidth = glwidth;
	s_HBAOCalcFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_HBAOCalcFBO);
	GL_FrameBufferColorTextureHBAO(&s_HBAOCalcFBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_HBAOCalcFBO);
		g_pMetaHookAPI->SysError("Failed to initialize HBAOCalc framebuffer.\n");
	}

	//Framebuffers that bind no texture
	GL_GenFrameBuffer(&s_ShadowFBO);

	//Framebuffers that bind no texture
	GL_GenFrameBuffer(&s_WaterSurfaceFBO);

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
		GL_GenFrameBuffer(&s_DownSampleFBO[i]);
		GL_FrameBufferColorTexture(&s_DownSampleFBO[i], GL_RGB16F);
		GL_FrameBufferDepthTexture(&s_DownSampleFBO[i], GL_DEPTH24_STENCIL8);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
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

		vec4_t clearColor = { 0, 0, 0, 0 };

		GL_ClearColor(clearColor);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
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

		vec4_t clearColor = { 0, 0, 0, 0 };

		GL_ClearColor(clearColor);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
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

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)

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

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
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
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_BrightAccumFBO);
		g_pMetaHookAPI->SysError("Failed to initialize BrightAccumulate #%d framebuffer.\n");
	}

	s_ToneMapFBO.iWidth = glwidth;
	s_ToneMapFBO.iHeight = glheight;
	GL_GenFrameBuffer(&s_ToneMapFBO);
	GL_FrameBufferColorTexture(&s_ToneMapFBO, GL_RGB8);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_FreeFBO(&s_ToneMapFBO);
		gEngfuncs.Con_Printf("Failed to initialize ToneMapping #%d framebuffer.\n");
	}

	GL_BindFrameBuffer(NULL);
}

void GLAPIENTRY GL_DebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (0 == strncmp(message, "API_ID_RECOMPILE_FRAGMENT_SHADER", sizeof("API_ID_RECOMPILE_FRAGMENT_SHADER") - 1))
		return;

	gEngfuncs.Con_DPrintf("GL_DebugOutputCallback: source:[%X], type:[%X], id:[%X], message:[%s]\n", source, type, id, message);
}

void GL_Init(void)
{
	gPrivateFuncs.GL_Init();

	//Just like what GL_SetMode does
	g_pMetaHookAPI->GetVideoMode(&glwidth, &glheight, NULL, NULL);

	auto err = glewInit();

	if (GLEW_OK != err)
	{
		Sys_Error("glewInit failed, %s", glewGetErrorString(err));
		return;
	}

	if (!(*gl_mtexable))
	{
		Sys_Error("Multitexture extension must be enabled!\nPlease remove \"-nomtex\" from launch parameters and try again.");
		return;
	}

	if (!GLEW_VERSION_4_3)
	{
		Sys_Error("OpenGL 4.3 is not supported!\n");
		return;
	}

	//No vanilla detail texture support
	(*detTexSupported) = false;

	if (gEngfuncs.CheckParm("-gl_debugoutput", NULL))
	{
		glDebugMessageCallback(GL_DebugOutputCallback, 0);
		glEnable(GL_DEBUG_OUTPUT);
	}

	gl_max_texture_size = 128;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);

	gl_max_ansio = 1;

	if (glewIsSupported("GL_EXT_texture_filter_anisotropic"))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &gl_max_ansio);
	}

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &gl_max_ubo_size);

	g_bNoStretchAspect = (gEngfuncs.CheckParm("-stretchaspect", NULL) == 0);

	if (gEngfuncs.CheckParm("-oitblend", NULL))
		g_bUseOITBlend = true;

	if (g_bUseOITBlend && !glewIsSupported("GL_ARB_shader_image_load_store"))
		g_bUseOITBlend = false;

	if (g_bUseOITBlend && !glewIsSupported("GL_ARB_fragment_shader_interlock"))
		g_bUseOITBlend = false;

	if (!g_bUseLegacyTextureLoader && gEngfuncs.CheckParm("-use_legacy_texloader", NULL))
		g_bUseLegacyTextureLoader = true;

	GL_GenerateFrameBuffers();
	GL_InitShaders();
}

void GL_Shutdown(void)
{
	GL_FreeShaders();
	GL_FreeFrameBuffers();
}

/*
	Purpose : Switch to s_FinalBufferFBO and clear it
*/

void GL_FlushFinalBuffer()
{
	GL_BindFrameBuffer(&s_FinalBufferFBO);
	
	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColorDepthStencil(vecClearColor, 1, STENCIL_MASK_NONE, STENCIL_MASK_ALL);
}

bool SCR_IsLoadingVisible()
{
	return scr_drawloading && (*scr_drawloading) == 1 ? true : false;
}

void R_GameFrameStart()
{
	g_bHasLowerBody = false;

	R_EntityComponents_StartFrame();
}

void R_RenderViewStart()
{

}

/*
	Purpose: Called once per frame, before running any render pass, but after physics and networking
*/

void R_RenderFrameStart()
{
	//Make sure r_framecount be advanced once per frame
	++(*r_framecount);

	R_PrepareDecals();
	R_ForceCVars(gEngfuncs.GetMaxClients() > 1);
	R_StudioStartFrame();
	R_CheckVariables();
	R_AnimateLight();
}

/*
	Called only once per frame, but before GL_EndRendering
*/

void R_RenderEndFrame()
{
	R_StudioEndFrame();
}

void GL_BeginRendering(int *x, int *y, int *width, int *height)
{
	gPrivateFuncs.GL_BeginRendering(x, y, width, height);

	R_RunDeferredFrameTasks();

	//Window resized?
#if 1
	if ((*width) != glwidth || (*height) != glheight)
	{
		glx = (*x);
		gly = (*y);
		glwidth = (*width);
		glheight = (*height);

		GL_GenerateFrameBuffers();
	}
	else
	{
		glx = (*x);
		gly = (*y);
		glwidth = (*width);
		glheight = (*height);
	}
#endif

	//No V_RenderView calls when level changes so don't GL_FlushFinalBuffer, this replicates vanilla engine's behavior
	if (SCR_IsLoadingVisible())
	{

	}
	else
	{
		GL_FlushFinalBuffer();
	}

	R_RenderFrameStart();

	r_renderview_pass = 0;
	*c_alias_polys = 0;
	*c_brush_polys = 0;
}

/*
	Purpose: 
		Called on beginning of RenderView
		This will switch from final framebuffer (RGBA8) to back framebuffer (RGBAF16)
*/

void R_PreRenderView()
{
	//Reset statistics

	r_wsurf_drawcall = 0;
	r_wsurf_polys = 0;
	r_studio_drawcall = 0;
	r_studio_polys = 0;
	r_sprite_drawcall = 0;
	r_sprite_polys = 0;

	//Always force GammaBlend to be disabled at very beginning.
	r_draw_gammablend = false;

	//Currently unused
	R_RenderViewStart();

	R_RenderShadowMap();

	R_RenderWaterPass();

	//Restore states because it might be corrupted by R_RenderWaterPass.
	if (R_IsGammaBlendEnabled())
	{
		GL_BindFrameBuffer(&s_BackBufferFBO3);
		GL_SetCurrentSceneFBO(&s_BackBufferFBO3);
		r_draw_gammablend = true;
	}
	else
	{
		GL_BindFrameBuffer(&s_BackBufferFBO);
		GL_SetCurrentSceneFBO(&s_BackBufferFBO);
		r_draw_gammablend = false;
	}

	vec4_t vecClearColor = {0};

	if (CL_IsDevOverviewMode())
	{
		UTIL_ParseCvarAsColor3(dev_overview_color, vecClearColor);
	}
	else
	{
		UTIL_ParseCvarAsColor3(gl_clearcolor, vecClearColor);
	}

	GammaToLinear(vecClearColor);

	GL_ClearColorDepthStencil(vecClearColor, 1, STENCIL_MASK_NONE, STENCIL_MASK_ALL);

	glDepthFunc(GL_LEQUAL);
	glDepthRange(0, 1);
}

void R_PostRenderView()
{
	if (R_IsHDREnabled())
	{
		if (R_IsRenderingGammaBlending())
		{
			R_GammaUncorrection(GL_GetCurrentSceneFBO(), &s_BackBufferFBO2);
		}
		else
		{
			GL_BlitFrameBufferToFrameBufferColorOnly(GL_GetCurrentSceneFBO(), &s_BackBufferFBO2);
		}

		R_HDR(&s_BackBufferFBO2, GL_GetCurrentSceneFBO(), &s_BackBufferFBO);

		if (GL_GetCurrentSceneFBO() != &s_BackBufferFBO)
		{
			GL_BlitFrameBufferToFrameBufferDepthStencil(GL_GetCurrentSceneFBO(), &s_BackBufferFBO);
		}
	}
	else
	{
		if (R_IsRenderingGammaBlending())
		{
			GL_BlitFrameBufferToFrameBufferColorDepthStencil(GL_GetCurrentSceneFBO(), &s_BackBufferFBO);
		}
		else
		{
			GL_BlitFrameBufferToFrameBufferColorOnly(GL_GetCurrentSceneFBO(), &s_BackBufferFBO2);
			R_GammaCorrection(&s_BackBufferFBO2, &s_BackBufferFBO);
		}
	}

	r_draw_gammablend = false;
	GL_BindFrameBuffer(&s_BackBufferFBO);
	GL_SetCurrentSceneFBO(&s_BackBufferFBO);

	if (R_IsUnderWaterEffectEnabled())
	{
		GL_BlitFrameBufferToFrameBufferColorOnly(&s_BackBufferFBO, &s_BackBufferFBO2);
		R_UnderWaterEffect(&s_BackBufferFBO2, &s_BackBufferFBO);
	}

	if (R_IsFXAAEnabled())
	{
		GL_BlitFrameBufferToFrameBufferColorOnly(&s_BackBufferFBO, &s_BackBufferFBO2);
		R_FXAA(&s_BackBufferFBO2, &s_BackBufferFBO);
	}

	//Restore OpenGL states to default
	GL_DisableMultitexture();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);
}

void R_PreDrawViewModel(void)
{
	(*currententity) = cl_viewent;

	if (!R_ShouldDrawViewModel())
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

void R_DrawViewModel(void)
{
	float lightvec[3];

	lightvec[0] = -1;
	lightvec[1] = 0;
	lightvec[2] = 0;

	(*currententity) = cl_viewent;

	if (!R_ShouldDrawViewModel())
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

	//Valve add this shit for what? idk
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void R_ClearPortalClipPlanes(void)
{
	for (int i = 0; i < 6; ++i)
	{
		g_bPortalClipPlaneEnabled[i] = false;
	}
	memset(g_PortalClipPlane, 0, sizeof(g_PortalClipPlane));
}

void R_RenderView_SvEngine(int viewIdx)
{
	//Clear texture id cache since SC client dll bind texture id 0 by glBindTexture directly and leave texture id caching system corrupted.
	(*currenttexture) = -1;

	//Clear s_FinalBuffer again since SC client dll may draw some portal views on it before the very first pass.
	if (R_IsRenderingPortal())
	{
		GL_FlushFinalBuffer();
	}
	else
	{
		//Clear s_FinalBuffer for the very first pass
		if (viewIdx == 0)
		{
			GL_FlushFinalBuffer();
		}
	}

	r_renderview_pass = viewIdx;

	double time1 = 0;

	if (r_speeds->value)
	{
		time1 = gEngfuncs.GetAbsoluteTime();
	}

	if (!r_norefresh->value)
	{
		if (!r_worldentity->model || !(*cl_worldmodel))
		{
			g_pMetaHookAPI->SysError("R_RenderView: NULL worldmodel");
		}

		R_PreRenderView();

		if (!(*r_refdef.onlyClientDraws))
		{
			//Allocate TEMPENT here
			R_PreDrawViewModel();
		}

		R_RenderScene();

		if (!(*r_refdef.onlyClientDraws))
		{
			R_SetupGLForViewModel();
			R_DrawViewModel();
		}

		R_PostRenderView();

		//This will switch to final framebuffer (RGBA8)
		//TODO: Why not using GL_BlitFrameBufferToFrameBufferColorOnly?		
		R_BlendFinalBuffer(&s_BackBufferFBO, &s_FinalBufferFBO);
		GL_SetCurrentSceneFBO(NULL);

		if (!(*r_refdef.onlyClientDraws))
			R_PolyBlend();

		S_ExtraUpdate();
	}
	else
	{
		GL_BindFrameBuffer(&s_FinalBufferFBO);
		GL_SetCurrentSceneFBO(NULL);
	}

	(*c_alias_polys) += r_studio_polys;
	(*c_brush_polys) += r_wsurf_polys;
	
	//Clear texture id cache since SC client dll bind texture id 0 but leave texture id cache non-zero
	(*currenttexture) = -1;

	R_ClearPortalClipPlanes();

	if (r_speeds->value)
	{
		float framerate = (*cl_time) - (*cl_oldtime);

		if (framerate > 0)
			framerate = 1.0 / framerate;

		auto time2 = gEngfuncs.GetAbsoluteTime();

		gEngfuncs.Con_Printf("%3ifps in %3i ms at viewpass #%d, with:\n  %d brushpolys,%d brushdraw.\n  %d studiopolys, %d studiodraw.\n  %d spritepolys, %d spritedraw.\n",
			(int)(framerate + 0.5), (int)((time2 - time1) * 1000),
			r_renderview_pass,
			r_wsurf_polys, r_wsurf_drawcall,
			r_studio_polys, r_studio_drawcall,
			r_sprite_polys, r_sprite_drawcall
		);
	}
}

void R_RenderView(void)
{
	//No arg(s) in GoldSrc
	r_renderview_pass ++;
	R_RenderView_SvEngine(r_renderview_pass);
}

void V_RenderView(void)
{
	gPrivateFuncs.V_RenderView();
}

float GetXMouseAspectRatioAdjustment(void)
{
	return (*s_fXMouseAspectAdjustment);
}

float GetYMouseAspectRatioAdjustment(void)
{
	return (*s_fYMouseAspectAdjustment);
}

void GL_EndRendering(void)
{
	R_RenderEndFrame();

	//Disable engine's framebuffer
	GLuint save_backbuffer_fbo = 0;

	if (gl_backbuffer_fbo)
	{
		save_backbuffer_fbo = *gl_backbuffer_fbo;
		*gl_backbuffer_fbo = 0;
	}

	int srcW = s_FinalBufferFBO.iWidth, srcH = s_FinalBufferFBO.iHeight;

	int dstX1 = 0;
	int dstY1 = 0;
	int dstX2 = window_rect->right - window_rect->left;
	int dstY2 = window_rect->bottom - window_rect->top;
	(*s_fXMouseAspectAdjustment) = (*s_fYMouseAspectAdjustment) = 1;

	float fSrcAspect = (float)srcW / (float)srcH;
	float fDstAspect = (float)dstX2 / (float)dstY2;

	if (g_bNoStretchAspect)
	{
		if (fSrcAspect > fDstAspect)
		{
			float fDesiredWidth = dstX2 * (1 / fSrcAspect);
			float fDiff = dstY2 - fDesiredWidth;
			dstY1 = fDiff / 2;
			dstY2 = dstY2 - dstY1;
			(*s_fYMouseAspectAdjustment) = fSrcAspect / fDstAspect;
		}
		else
		{
			float fDesiredHeight = dstY2 / (1 / fSrcAspect);
			float fDiff = dstX2 - fDesiredHeight;
			dstX1 = fDiff / 2;
			dstX2 = dstX2 - dstX1;
			(*s_fXMouseAspectAdjustment) = fDstAspect / fSrcAspect;
		}
	}

	//Blit to screen
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_FinalBufferFBO.s_hBackBufferFBO);

	vec4_t vecClearColor = { 0, 0, 0, 0 };
	GL_ClearColor(vecClearColor);

	glBlitFramebuffer(0, 0, srcW, srcH, dstX1, dstY1, dstX2, dstY2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	//Let engine call VID_FlipScreen for us.
	gPrivateFuncs.GL_EndRendering();

	if (gl_backbuffer_fbo)
	{
		*gl_backbuffer_fbo = save_backbuffer_fbo;
	}
}

#if 0
void DLL_SetModKey(void *pinfo, char *pkey, char *pvalue)
{
	gPrivateFuncs.DLL_SetModKey(pinfo, pkey, pvalue);

	if (!strcmp(pkey, "vertical_fov"))
	{
		bVerticalFov = atoi(pvalue) ? true : false;
	}
}
#endif

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

	r_detailtextures = gEngfuncs.pfnGetCvarPointer("r_detailtextures");

	r_cullsequencebox = gEngfuncs.pfnGetCvarPointer("r_cullsequencebox");

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

	sv_cheats = gEngfuncs.pfnGetCvarPointer("sv_cheats");

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

	viewmodel_fov = gEngfuncs.pfnGetCvarPointer("viewmodel_fov");
	if(!viewmodel_fov)
		viewmodel_fov = gEngfuncs.pfnRegisterVariable("viewmodel_fov", "90", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	default_fov = gEngfuncs.pfnGetCvarPointer("default_fov");

	r_vertical_fov = gEngfuncs.pfnRegisterVariable("r_vertical_fov", (vertical_fov_SvEngine  && (*vertical_fov_SvEngine)) ? "1" : "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	gl_widescreen_yfov = gEngfuncs.pfnGetCvarPointer("gl_widescreen_yfov");
	if(!gl_widescreen_yfov)
		gl_widescreen_yfov = gEngfuncs.pfnRegisterVariable("gl_widescreen_yfov", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	cl_fixmodelinterpolationartifacts = gEngfuncs.pfnGetCvarPointer("cl_fixmodelinterpolationartifacts");
	if (!cl_fixmodelinterpolationartifacts)
		cl_fixmodelinterpolationartifacts = gEngfuncs.pfnRegisterVariable("cl_fixmodelinterpolationartifacts", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	r_gamma_blend = gEngfuncs.pfnRegisterVariable("r_gamma_blend", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	/*
		r_linear_blend_shift 0: Don't shift color/alpha for transparent object at all in linear space.
		r_linear_blend_shift 1: Shift color/alpha for transparent object to how it looks like in vanilla engine. (Only works when r_gamma_blend off)
		r_linear_blend_shift can be ranged from 0.0 to 1.0 and the shifted result will interpolated between 0% to 100% of the shifted blend factor.
	*/
	r_linear_blend_shift = gEngfuncs.pfnRegisterVariable("r_linear_blend_shift", "0.8", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	/*
		r_linear_fog_shift 0: Shift fog intensity to how it looks like in vanilla engine, in linear space.
		r_linear_fog_shift 1: Don't shift fog intensity at all in linear space. (Only works when r_gamma_blend off)
	*/
	r_linear_fog_shift = gEngfuncs.pfnRegisterVariable("r_linear_fog_shift", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	/*
		r_linear_fog_shiftz : Shift the fog intensity to lower value to make it looks more like what it was in vanilla engine in linear space.
	*/
	r_linear_fog_shiftz = gEngfuncs.pfnRegisterVariable("r_linear_fog_shiftz", "0.8", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	/*
		r_fog_trans 0: Fog don't affect any transparent objects
		r_fog_trans 1: Fog affects alpha blending objects, but not additive blending objects
		r_fog_trans 2: Fog affects both alpha blending objects and additive blending objects
	*/
	r_fog_trans = gEngfuncs.pfnRegisterVariable("r_fog_trans", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	r_detailskytextures = gEngfuncs.pfnRegisterVariable("r_detailskytextures", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	r_sprite_lerping = gEngfuncs.pfnRegisterVariable("r_sprite_lerping", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	r_drawlowerbody = gEngfuncs.pfnRegisterVariable("r_drawlowerbody", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	r_drawlowerbodyattachments = gEngfuncs.pfnRegisterVariable("r_drawlowerbodyattachments", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	r_drawlowerbodypitch = gEngfuncs.pfnRegisterVariable("r_drawlowerbodypitch", "45", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	/*
	r_leaf_lazy_load 0: Load all GPU resouces into VRAM at once when loading map
	r_leaf_lazy_load 1: Load only necessary vertices and indices into VRAM when loading map, generate and load indirect draw command into VRAM in next few frames
	r_leaf_lazy_load 2: Load only necessary vertices and indices into VRAM when loading map, generate and load indirect draw command into VRAM when player enter leaf.
	*/
	r_leaf_lazy_load = gEngfuncs.pfnRegisterVariable("r_leaf_lazy_load", "2", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	gEngfuncs.pfnAddCommand("saveprogstate", R_SaveProgramStates_f);
	gEngfuncs.pfnAddCommand("loadprogstate", R_LoadProgramStates_f);
}

void R_Init(void)
{
	Mod_Init();

	R_InitCvars();

	R_InitWater();
	R_InitStudio();
	R_InitShadow();
	R_InitWSurf();
	R_InitLight();
	R_InitSprite();
	R_InitPostProcess();
	R_InitPortal();
	R_InitEntityComponents();

	R_LoadProgramStates_f();
}

void R_Shutdown(void)
{
	R_ClearDeferredFrameTasks();
	R_ShutdownWater();
	R_ShutdownStudio();
	R_ShutdownShadow();
	R_ShutdownWSurf();
	R_ShutdownLight();
	R_ShutdownSprite();
	R_ShutdownPostProcess();
	R_ShutdownPortal();
	R_ShutdownEntityComponents();
	triapi_Shutdown();

	R_FreeMapCvars();
}

void R_ForceCVars(qboolean mp)
{
	if (R_IsRenderingWaterView())
		return;

	if (gPrivateFuncs.R_ForceCVars)
		return gPrivateFuncs.R_ForceCVars(mp);

	//TODO implement this for 3266, inlined ?
}

void R_NewMap(void)
{
	memset(&r_params, 0, sizeof(r_params));

	R_ClearDeferredFrameTasksWithFlags(DEFERRED_FRAME_TASK_DESTROY_ON_CHANGE_LEVEL);
	R_GenerateSceneUBO();
	R_FreeWorldResources();
	R_FreePortalResouces();

	gPrivateFuncs.R_NewMap();

	R_LoadWorldResources();
	R_LoadLightResources();

	R_StudioFlushAllSkins();

	//Free GPU resources...
	R_FreeAllUnreferencedStudioRenderData();

	(*r_framecount) = 1;
	(*r_visframecount) = 1;
}

mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model)
{
	if (!model || !model->nodes)
		g_pMetaHookAPI->SysError("Mod_PointInLeaf: bad model");

	auto basenode = (mbasenode_t *)model->nodes;

	while (1)
	{
		if (basenode->contents < 0)
			return (mleaf_t *)basenode;

		auto node = (mnode_t*)basenode;

		auto plane = node->plane;
		auto d = DotProduct(p, plane->normal) - plane->dist;

		if (d > 0)
			basenode = node->children[0];
		else
			basenode = node->children[1];
	}

	return NULL;
	//return gPrivateFuncs.Mod_PointInLeaf(p, model);
}

float *R_GetAttachmentPoint(int entity, int attachment)
{
	auto pEntity = gEngfuncs.GetEntityByIndex(entity);

	if (attachment)
		return pEntity->attachment[attachment - 1];

	return pEntity->origin;
}

/*
	input: fov_y (aka vertical fov)
	output: fov_x (aka horizontal fov)
*/

double V_CalcFovV(float fov, float width, float height)
{
	if (fov < 1.0 || fov > 179.0)
		fov = 90.0;

	return atan2(width / (height / tan(fov * (1.0 / 360.0) * M_PI)), 1.0) * 360.0 * (1 / M_PI);
}

/*
	input: fov_x (aka horizontal fov)
	output: fov_y (aka vertical fov)
*/

double V_CalcFovH(float fov, float width, float height)
{
	if (fov < 1.0 || fov > 179.0)
		fov = 90.0;

	return atan2(height / (width / tan(fov * (1.0 / 360.0) * M_PI)), 1.0) * 360.0 * (1 / M_PI);
}

void V_AdjustFovV(float* fov_x, float* fov_y, float width, float height)
{
	float x, y;

	if (fabs(width * 3 - 4 * height) < 1)
	{
		// 4:3 ratio
		return;
	}

	if (fabs(width * 4 - 5 * height) < 1)
	{
		// 5:4 ratio
		return;
	}

	if (gl_widescreen_yfov->value == 1)
	{
		x = V_CalcFovV(*fov_y, 640, 480);
		y = *fov_y;
		*fov_y = V_CalcFovV(x, height, width);

		if (*fov_y < y)
			*fov_y = y;
		else
			*fov_x = x;
	}
	else if (gl_widescreen_yfov->value == 2)
	{
		//fov_y is the input fov, recalculate fov_x from 4:3 aspect ratio
		x = V_CalcFovV(*fov_y, 640, 480);
		y = *fov_y;

		*fov_x = x;
		*fov_y = y;
	}
}

void V_AdjustFovH(float *fov_x, float *fov_y, float width, float height)
{
	float x, y;

	if (fabs(width * 3 - 4 * height) < 1)
	{
		// 4:3 ratio
		return;
	}

	if (fabs(width * 4 - 5 * height) < 1)
	{
		// 5:4 ratio
		return;
	}

	if (gl_widescreen_yfov->value == 1)
	{
		y = V_CalcFovH(*fov_x, 640, 480);
		x = *fov_x;

		*fov_x = V_CalcFovH(y, height, width);

		if (*fov_x < x)
			*fov_x = x;
		else
			*fov_y = y;
	}
	else if (gl_widescreen_yfov->value == 2)
	{
		//fov_x is the input fov, recalculate fov_y from 4:3 aspect ratio
		y = V_CalcFovH(*fov_x, 640, 480);
		x = *fov_x;

		*fov_x = x;
		*fov_y = y;
	}
}

void R_SetFrustum(void)
{
	float xfov = r_xfov_currentpass;
	float yfov = r_xfov_currentpass;

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
	return gPrivateFuncs.CL_IsDevOverviewMode();
}

void CL_SetDevOverView(void *a1)
{
	return gPrivateFuncs.CL_SetDevOverView(a1);
}

void MYgluPerspective2(double xfov, double yfov, double zNear, double zFar)
{
	auto yMax = zNear * tan(yfov * M_PI / 360.0f);
	auto yMin = -yMax;

	auto xMax = zNear * tan(xfov * M_PI / 360.0f);
	auto xMin = -xMax;

	glFrustum(xMin, xMax, yMin, yMax, zNear, zFar);
}

float R_GetDefaultFOV()
{
	if (g_bIsCounterStrike)
	{
		return 90;
	}

	if (default_fov)
	{
		return default_fov->value;
	}

	return 90;
}

void R_AdjustScopeFOVForViewModel(float *fov)
{
	if (!g_bIsCounterStrike)
	{
		if (default_fov && fabs(viewmodel_fov->value - default_fov->value) > 1)
		{
			*fov = (*scrfov) * viewmodel_fov->value / default_fov->value;

			if (*fov < 15.0f)
				*fov = 15.0f;

			if (*fov < 1.0f || *fov > 179.0f)
				*fov = 90.0f;
		}
	}
	else
	{
		if (fabs(viewmodel_fov->value - 90.0f) > 1)
		{
			*fov = (*scrfov) * viewmodel_fov->value / 90.0f;

			if (*fov < 15.0f)
				*fov = 15.0f;

			if (*fov < 1.0f || *fov > 179.0f)
				*fov = 90.0f;
		}
	}
}

void R_LoadLegacyOpenGLMatrixForViewModel()
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(r_viewmodel_projection_matrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(r_world_matrix);
}

void R_LoadLegacyOpenGLMatrixForWorld()
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(r_projection_matrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(r_world_matrix);
}

void R_UploadProjMatrixForViewModel(void)
{
	camera_ubo_t CameraUBO;
	memcpy(CameraUBO.projMatrix, r_viewmodel_projection_matrix, sizeof(mat4));
	memcpy(CameraUBO.invProjMatrix, r_viewmodel_projection_matrix_inv, sizeof(mat4));

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, offsetof(camera_ubo_t, projMatrix), sizeof(mat4), &CameraUBO.projMatrix);
	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, offsetof(camera_ubo_t, invProjMatrix), sizeof(mat4), &CameraUBO.invProjMatrix);
}

void R_LoadProjMatrixForWorld(void)
{
	camera_ubo_t CameraUBO;
	memcpy(CameraUBO.projMatrix, r_projection_matrix, sizeof(mat4));
	memcpy(CameraUBO.invProjMatrix, r_projection_matrix_inv, sizeof(mat4));

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, offsetof(camera_ubo_t, projMatrix), sizeof(mat4), &CameraUBO.projMatrix);
	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, offsetof(camera_ubo_t, invProjMatrix), sizeof(mat4), &CameraUBO.invProjMatrix);
}

void R_SetupGLForViewModel(void)
{
	if (!CL_IsDevOverviewMode() && viewmodel_fov->value > 0)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	
		if (r_vertical_fov->value)
		{
			auto height = (double)(*r_refdef.vrect).height;
			auto width = (double)(*r_refdef.vrect).width;
			auto aspect = height / width;

			auto fov = viewmodel_fov->value;
			if (fov < 1.0 || fov > 179.0)
				fov = 90.0;

			R_AdjustScopeFOVForViewModel(&fov);

			r_yfov_viewmodel = fov;
			r_xfov_viewmodel = V_CalcFovV(fov, width, height);

			V_AdjustFovV(&r_xfov_viewmodel, &r_yfov_viewmodel, width, height);
			R_SetupPerspective(r_xfov_viewmodel, r_yfov_viewmodel, 4.0f, (r_params.movevars ? r_params.movevars->zmax : 4096));
		}
		else
		{
			auto width = (double)(*r_refdef.vrect).width;
			auto height = (double)(*r_refdef.vrect).height;
			auto aspect = width / height;

			auto fov = viewmodel_fov->value;
			if (fov < 1.0 || fov > 179.0)
				fov = 90.0;

			R_AdjustScopeFOVForViewModel(&fov);

			r_xfov_viewmodel = fov;
			r_yfov_viewmodel = V_CalcFovH(fov, width, height);

			V_AdjustFovH(&r_xfov_viewmodel, &r_yfov_viewmodel, width, height);
			R_SetupPerspective(r_xfov_viewmodel, r_yfov_viewmodel, 4.0f, (r_params.movevars ? r_params.movevars->zmax : 4096));
		}

		glGetFloatv(GL_PROJECTION_MATRIX, r_viewmodel_projection_matrix);
		glMatrixMode(GL_MODELVIEW);

		InvertMatrix(r_viewmodel_projection_matrix, r_viewmodel_projection_matrix_inv);

		R_UploadProjMatrixForViewModel();
	}
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

	if (R_IsRenderingShadowView())
	{
		auto CurrentSceneFBO = GL_GetCurrentSceneFBO();

		if (CurrentSceneFBO)
		{
			r_viewport[0] = 0;
			r_viewport[1] = 0;
			r_viewport[2] = CurrentSceneFBO->iWidth;
			r_viewport[3] = CurrentSceneFBO->iHeight;
		}
		else
		{
			r_viewport[0] = 0;
			r_viewport[1] = 0;
			r_viewport[2] = current_shadow_texture->size;
			r_viewport[3] = current_shadow_texture->size;
		}
	}
	else if (R_IsRenderingWaterView())
	{
		auto CurrentSceneFBO = GL_GetCurrentSceneFBO();

		if (CurrentSceneFBO)
		{
			r_viewport[0] = 0;
			r_viewport[1] = 0;
			r_viewport[2] = CurrentSceneFBO->iWidth;
			r_viewport[3] = CurrentSceneFBO->iHeight;
		}
		else
		{
			r_viewport[0] = 0;
			r_viewport[1] = 0;
			r_viewport[2] = glwidth;
			r_viewport[3] = glheight;
		}
	}
	else
	{
		r_viewport[0] = v0 + glx;
		r_viewport[1] = v3 + gly;
		r_viewport[2] = v4;
		r_viewport[3] = v5;
	}

	glViewport(r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);

	if (R_IsRenderingShadowView())
	{
		float cone_fov = current_shadow_texture->cone_angle * 2 * 360 / (M_PI * 2);
		R_SetupPerspective(cone_fov, cone_fov, 4.0f, current_shadow_texture->distance);
	}
	else if (r_vertical_fov->value)
	{
		auto height = (double)(*r_refdef.vrect).height;
		auto width = (double)(*r_refdef.vrect).width;
		auto aspect = height / width;

		auto fov = (*scrfov);

		if (fov < 1.0f || fov > 179.0f)
			fov = 90.0f;

		r_yfov = fov;
		r_xfov = V_CalcFovV(fov, width, height);

		if ((*r_refdef.onlyClientDraws))
		{
			V_AdjustFovV(&r_xfov, &r_yfov, width, height);
			R_SetupPerspective(r_xfov, r_yfov, 4.0f, 16000.0f);
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

			r_xfov_currentpass = 0;
			r_yfov_currentpass = 0;
		}
		else
		{
			V_AdjustFovV(&r_xfov, &r_yfov, width, height);
			R_SetupPerspective(r_xfov, r_yfov, 4.0f, (r_params.movevars ? r_params.movevars->zmax : 4096));
		}
	}
	else
	{
		auto width = (double)(*r_refdef.vrect).width;
		auto height = (double)(*r_refdef.vrect).height;
		auto aspect = width / height;
		auto fov = (*scrfov);

		if (fov < 1.0f || fov > 179.0f)
			fov = 90.0f;

		r_xfov = fov;
		r_yfov = V_CalcFovH(fov, width, height);

		if ((*r_refdef.onlyClientDraws))
		{
			V_AdjustFovH(&r_xfov, &r_yfov, width, height);
			R_SetupPerspective(r_xfov, r_yfov, 4.0f, 16000.0f);
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

			r_xfov_currentpass = 0;
			r_yfov_currentpass = 0;
		}
		else
		{
			V_AdjustFovH(&r_xfov, &r_yfov, width, height);
			R_SetupPerspective(r_xfov, r_yfov, 4.0f, (r_params.movevars ? r_params.movevars->zmax : 4096));
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

	InvertMatrix(r_world_matrix, r_world_matrix_inv);
	InvertMatrix(r_projection_matrix, r_projection_matrix_inv);
}

void R_CheckVariables(void)
{
	gPrivateFuncs.R_CheckVariables();
}

/*

R_AnimateLight basically fills d_lightstylevalue[0~255] from cl_lightstyle[0~255]

*/

void R_AnimateLight(void)
{
	gPrivateFuncs.R_AnimateLight();
}

void R_SetupFrame(void)
{
	//R_RenderScene might be called for multiple times in one frame. so move those to upper level.
	//R_ForceCVars(gEngfuncs.GetMaxClients() > 1);
	//R_CheckVariables();
	//R_AnimateLight();

	R_UpdateRefDef();
	//VectorCopy((*r_refdef.vieworg), r_origin);

	//gEngfuncs.pfnAngleVectors((*r_refdef.viewangles), vpn, vright, vup);

	(*r_oldviewleaf) = (*r_viewleaf);

	if (R_IsRenderingWaterView())
	{
		(*r_viewleaf) = Mod_PointInLeaf(g_CurrentCameraView, (*cl_worldmodel));
	}
	else if (r_refdef_SvEngine && r_refdef_SvEngine->useCamera)
	{
		(*r_viewleaf) = Mod_PointInLeaf(r_refdef_SvEngine->r_camera_origin, (*cl_worldmodel));
	}
	else
	{
		(*r_viewleaf) = Mod_PointInLeaf(r_origin, (*cl_worldmodel));
	}

	R_DisableRenderingFog();

	if (R_CanRenderFog())
	{
		if ((*cl_waterlevel) > 2)
		{
			R_RenderWaterFog();
		}
		else if (g_iStartDist_SCClient && g_iEndDist_SCClient && (*g_iStartDist_SCClient) >= 0 && (*g_iEndDist_SCClient) > 0)
		{
			R_RenderSvenFog();
		}
		else if ((*g_bUserFogOn))
		{
			R_RenderUserFog();
		}
	}
}

void R_MarkLeaves(void)
{
	byte *vis;

	if ((*r_oldviewleaf) == (*r_viewleaf) && !r_novis->value)
		return;

	(*r_visframecount)++;
	(*r_oldviewleaf) = (*r_viewleaf);

	if (r_novis->value)
	{
		vis = mod_novis;
	}
	else
	{
		vis = Mod_LeafPVS((*r_viewleaf), (*cl_worldmodel));
	}

	for (int i = 0; i < (*cl_worldmodel)->numleafs; i++)
	{
		if ((byte)(1 << (i & 7)) & vis[i >> 3])
		{
			auto basenode = (mbasenode_t *)&(*cl_worldmodel)->leafs[i + 1];

			do
			{
				if (basenode->visframe == (*r_visframecount))
					break;

				basenode->visframe = (*r_visframecount);
				basenode = basenode->parent;

			} while (basenode);
		}
	}
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

		if (r_draw_deferredtrans)
		{
			R_AddTEntity((*currententity));

			r_draw_deferredtrans = false;
		}
	}
}

void R_SetupFog(void)
{
	scene_ubo_t SceneUBO;

	memcpy(SceneUBO.fogColor, r_fog_color, sizeof(vec4_t));

	SceneUBO.fogStart = r_fog_control[0];
	SceneUBO.fogEnd = r_fog_control[1];
	SceneUBO.fogDensity = r_fog_control[2];

	GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hSceneUBO, offsetof(scene_ubo_t, fogColor), offsetof(scene_ubo_t, cl_time) - offsetof(scene_ubo_t, fogColor), &SceneUBO.fogColor);

	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, r_fog_mode);
	glFogf(GL_FOG_DENSITY, r_fog_control[2]);
	glHint(GL_FOG_HINT, GL_NICEST);
	glFogfv(GL_FOG_COLOR, r_fog_color);
	glFogf(GL_FOG_START, r_fog_control[0]);
	glFogf(GL_FOG_END, r_fog_control[1]);
}

void R_RenderWaterFog(void)
{
	r_fog_color[0] = cshift_water->destcolor[0] * 0.00392156862745098;
	r_fog_color[1] = cshift_water->destcolor[1] * 0.00392156862745098;
	r_fog_color[2] = cshift_water->destcolor[2] * 0.00392156862745098;
	r_fog_color[3] = 1.0;

	r_fog_control[0] = 0;
	r_fog_control[1] = (1536 - 4 * cshift_water->percent);
	r_fog_control[2] = 0;

	r_fog_mode = GL_LINEAR;
	r_fog_enabled = true;

	R_SetupFog();
}

void R_RenderSvenFog(void)
{
	r_fog_color[0] = g_iFogColor_SCClient[0] * 1.0f / 255.0f;
	r_fog_color[1] = g_iFogColor_SCClient[1] * 1.0f / 255.0f;
	r_fog_color[2] = g_iFogColor_SCClient[2] * 1.0f / 255.0f;
	r_fog_color[3] = 1.0;

	r_fog_control[0] = (*g_iStartDist_SCClient);
	r_fog_control[1] = (*g_iEndDist_SCClient);
	r_fog_control[2] = 0;

	r_fog_mode = GL_LINEAR;
	r_fog_enabled = true;

	R_SetupFog();
}

void R_RenderUserFog(void)
{
	memcpy(r_fog_color, g_UserFogColor, sizeof(vec4_t));

	r_fog_control[0] = (*g_UserFogStart);
	r_fog_control[1] = (*g_UserFogEnd);
	r_fog_control[2] = (*g_UserFogDensity);

	r_fog_mode = GL_EXP2;
	r_fog_enabled = true;

	R_SetupFog();
}

void R_DisableRenderingFog()
{
	r_fog_mode = 0;
	r_fog_enabled = false;

	glDisable(GL_FOG);
}

void R_InhibitRenderingFog()
{
	if (r_fog_mode)
	{
		r_fog_enabled = false;

		glDisable(GL_FOG);
	}
}

void R_RestoreRenderingFog()
{
	if (r_fog_mode)
	{
		r_fog_enabled = true;

		glEnable(GL_FOG);
	}
}

void R_EndRenderOpaque(void)
{
	r_draw_opaque = false;

	glDisable(GL_ALPHA_TEST);

	//Transfer everything from GBuffer into SceneFBO
	if (R_IsRenderingGBuffer())
	{
		R_EndRenderGBuffer(GL_GetCurrentSceneFBO());
	}

	//For backward compatibility, some Mods may use Legacy OpenGL 1.x Matrix
	R_LoadLegacyOpenGLMatrixForWorld();
}

void ClientDLL_DrawNormalTriangles(void)
{
	//Good news: Stencil write has been completely removed from portal code.

	GL_PushFrameBuffer();

	//glStencilMask(0xFF);

	//SC client dll should have enabled this but they don't
	glEnable(GL_POLYGON_OFFSET_FILL);

	r_draw_legacysprite = true;

	//Call ClientDLL_DrawNormalTriangles instead of HUD_DrawNormalTriangles
	gPrivateFuncs.ClientDLL_DrawNormalTriangles();

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	r_draw_legacysprite = false;

	//glStencilMask(0);

	glDisable(GL_POLYGON_OFFSET_FILL);

	//This should have been restored to 0 by SC client dll while drawing portal overlay but they don't, which breaks HUD/GUIs somehow.
	glAlphaFunc(GL_NOTEQUAL, 0);

	//Clear texture id cache since SC client dll bind texture id 0 but leave texture id cache non-zero
	(*currenttexture) = -1;

	//Restore current framebuffer just in case that Allow SC client dll changes it
	GL_PopFrameBuffer();
}

void R_RenderScene(void)
{
	if (CL_IsDevOverviewMode())
		CL_SetDevOverView(R_GetRefDef());

	R_SetupFrame();
	R_SetupGL();
	R_SetFrustum();
	R_MarkLeaves();

	R_PrepareDrawWorld();

	if (!(*r_refdef.onlyClientDraws))
	{
		R_DrawWorld();
		S_ExtraUpdate();
		R_DrawEntitiesOnList();
	}

	R_EndRenderOpaque();

	R_InhibitRenderingFog();
	ClientDLL_DrawNormalTriangles();
	R_RestoreRenderingFog();

	R_DrawTransEntities((*r_refdef.onlyClientDraws));

	if ((*cl_waterlevel) > 2 && (*r_refdef.onlyClientDraws))
	{
		R_InhibitRenderingFog();
	}
	else
	{
		if (!(*g_bUserFogOn))
		{
			R_InhibitRenderingFog();
		}
	}

	S_ExtraUpdate();

	R_DisableRenderingFog();
}

int EngineGetMaxGLTextures()
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		return MAX_GLTEXTURES_HL25;

	return MAX_GLTEXTURES;
}

int EngineGetNumKnownModel()
{
	return (*mod_numknown);
}

int EngineGetMaxKnownModel(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return MAX_KNOWN_MODELS_SVENGINE;

	return MAX_KNOWN_MODELS;
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

int EngineGetMaxDLights(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		return MAX_DLIGHTS_SVENGINE;
	}

	return MAX_DLIGHTS;
}

int EngineGetMaxELights(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		return MAX_ELIGHTS_SVENGINE;
	}

	return MAX_ELIGHTS;
}

int EngineGetMaxLightmapTextures(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return MAX_LIGHTMAPS_SVENGINE;

	return MAX_LIGHTMAPS;
}

int EngineGetMaxClientModels(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return MAX_MODELS_SVENGINE;

	return MAX_MODELS;
}

//const int skytexorder_svengine[6] = { 0, 1, 2, 3, 4, 5 };
//const int skytexorder_goldsrc[6] = { 0, 2, 1, 3, 4, 5 };

void R_FreeSkyboxTextures()
{
	for (int i = 0; i < 12; ++i)
	{
		if (g_WorldSurfaceRenderer.vSkyboxTextureId[i])
		{
			g_WorldSurfaceRenderer.vSkyboxTextureId[i] = 0;
		}
	}
}

bool R_LoadLegacySkyTextures(const char* name)
{
	const char* suf[6] = { "rt", "lf", "bk", "ft", "up", "dn" };

	for (int i = 0; i < 6; i++)
	{
		bool bLoaded = false;
		char fullPath[MAX_PATH];
		gl_loadtexture_result_t loadResult;

		if (!bLoaded)
		{
			snprintf(fullPath, sizeof(fullPath), "gfx/env/%s%s.tga", name, suf[i]);
			bLoaded = R_LoadTextureFromFile(fullPath, fullPath, GLT_WORLD, true, &loadResult);
		}

		if (!bLoaded)
		{
			snprintf(fullPath, sizeof(fullPath), "gfx/env/%s%s.bmp", name, suf[i]);
			bLoaded = R_LoadTextureFromFile(fullPath, fullPath, GLT_WORLD, true, &loadResult);
		}

		if (!bLoaded)
		{
			gEngfuncs.Con_DPrintf("R_LoadLegacySkyTextures: Failed to load %s\n", fullPath);

			return false;
		}

		g_WorldSurfaceRenderer.vSkyboxTextureId[0 + i] = loadResult.gltexturenum;
	}
	return true;
}

bool R_LoadDetailSkyTextures(const char* name)
{
	const char* suf[6] = { "rt", "lf", "bk", "ft", "up", "dn" };

	for (int i = 0; i < 6; i++)
	{
		bool bLoaded = false;
		char fullPath[MAX_PATH];
		gl_loadtexture_result_t loadResult;

		if (!bLoaded)
		{
			snprintf(fullPath, sizeof(fullPath), "gfx/env/%s%s.dds", name, suf[i]);
			bLoaded = R_LoadTextureFromFile(fullPath, fullPath, GLT_WORLD, true, &loadResult);
		}

		if (!bLoaded)
		{
			snprintf(fullPath, sizeof(fullPath), "renderer/texture/skybox/%s%s.dds", name, suf[i]);
			bLoaded = R_LoadTextureFromFile(fullPath, fullPath, GLT_WORLD, true, &loadResult);
		}

		if (!bLoaded)
		{
			gEngfuncs.Con_DPrintf("R_LoadDetailSkyTexture: Failed to load %s\n", fullPath);
			return false;
		}

		g_WorldSurfaceRenderer.vSkyboxTextureId[6 + i] = loadResult.gltexturenum;
	}

	return true;
}

void R_LoadSkyInternal(const char *name)
{
	if (R_LoadLegacySkyTextures(name))
	{
		R_LoadDetailSkyTextures(name);
	}
	else
	{
		if (strcmp(name, "desert") != 0)
		{
			if (R_LoadLegacySkyTextures("desert"))
			{
				R_LoadDetailSkyTextures("desert");
			}
		}
	}
}

void R_LoadSkyBox_SvEngine(const char *name)
{
	R_FreeSkyboxTextures();

	R_LoadSkyInternal(name);
}

void R_LoadSkys(void)
{
	R_FreeSkyboxTextures();

	R_LoadSkyInternal(pmovevars->skyName);
}

#if 0

void R_BuildCubemap_Snapshot(cubemap_t *cubemap, int index)
{
	char name[64];
	COM_FileBase((*cl_worldmodel)->name, name);

	if (!g_pFileSystem->IsDirectory("gfx/cubemap"))
		g_pFileSystem->CreateDirHierarchy("gfx/cubemap");

	char path[64];
	snprintf(path, sizeof(path), "gfx/cubemap/%s", name);
	path[sizeof(path) - 1] = 0;

	if (!g_pFileSystem->IsDirectory(path))
		g_pFileSystem->CreateDirHierarchy(path);

	char filepath[1024];
	snprintf(filepath, sizeof(filepath), "gfx/cubemap/%s/%s_%d.%s", name, cubemap->name.c_str(), index, cubemap->extension.c_str());
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

	vrect_GoldSrc_t saveVrect;
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
	if (!(*cl_worldmodel) || !(*cl_worldmodel)->name[0])
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
	COM_FileBase((*cl_worldmodel)->name, name);

	char filepath[1024];
	char identifier[256];

	for (int i = 0; i < 6; ++i)
	{
		snprintf(filepath, sizeof(filepath), "gfx/cubemap/%s/%s_%d.%s", name, cubemap->name.c_str(), i, cubemap->extension.c_str());
		snprintf(identifier, sizeof(identifier), "cubemap_%s", cubemap->name.c_str());

		gl_loadtexture_cubemap = i + 1;

		cubemap->cubetex = R_LoadTextureFromFile(filepath, identifier, NULL, NULL, GLT_WORLD, tre, true);
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
	R_SaveTriAPIProgramStates();
	R_SaveLegacySpriteProgramStates();
	R_SavePortalProgramStates();

	gEngfuncs.Con_Printf("R_SaveProgramStates_f: Program state caches saved.\n");
}

void R_LoadProgramStates_f(void)
{
	R_LoadWSurfProgramStates();
	R_LoadWaterProgramStates();
	R_LoadDLightProgramStates();
	R_LoadDFinalProgramStates();
	R_LoadStudioProgramStates();
	R_LoadSpriteProgramStates();
	R_LoadTriAPIProgramStates();
	R_LoadLegacySpriteProgramStates();
	R_LoadPortalProgramStates();
	GL_UseProgram(0);

	gEngfuncs.Con_Printf("R_LoadProgramStates_f: Program state caches loaded.\n");
}

void GammaToLinear(float *color)
{
	color[0] = pow(color[0], v_gamma->value);
	color[1] = pow(color[1], v_gamma->value);
	color[2] = pow(color[2], v_gamma->value);
}

int __cdecl SDL_GL_SetAttribute(int attr, int value)
{
	if (attr == SDL_GL_CONTEXT_MAJOR_VERSION)
	{
		return gPrivateFuncs.SDL_GL_SetAttribute(attr, 4);
	}
	if (attr == SDL_GL_CONTEXT_MINOR_VERSION)
	{
		return gPrivateFuncs.SDL_GL_SetAttribute(attr, 3);
	}
	if (attr == SDL_GL_CONTEXT_PROFILE_MASK)
	{
		return gPrivateFuncs.SDL_GL_SetAttribute(attr, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	}
	//Why the fuck 4,4,4 in GoldSrc and SvEngine????
	if (attr == SDL_GL_RED_SIZE || attr == SDL_GL_GREEN_SIZE || attr == SDL_GL_BLUE_SIZE)
	{
		return gPrivateFuncs.SDL_GL_SetAttribute(attr, 8);
	}
	return gPrivateFuncs.SDL_GL_SetAttribute(attr, value);
}

void CL_EmitPlayerFlashlight(int entindex)
{
	cl_entity_t* ent = gEngfuncs.GetEntityByIndex(entindex);

	if (!ent->player)
		return;

	dlight_t* dl = nullptr;

	// JAY: Flashlight effect.  Currently just a light that floats in from of 
	// the player a few feet. Using 3 lights is better looking, but SLOW.  
	// A conical light projection might look better and be more efficient.

	// BRJ: Modified the flashlight to use a true spotlight for model illumination
	// and uses Jay's old method of making a spherical light that only illuminates
	// the lightmaps at the intersection point.

	if (ent->curstate.effects & (EF_BRIGHTLIGHT | EF_DIMLIGHT) && (*cl_worldmodel))
	{
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight(entindex);

		if (ent->curstate.effects & EF_BRIGHTLIGHT)
		{
			dl->color.r = dl->color.g = dl->color.b = 250;
			dl->radius = 400;
			VectorCopy(ent->origin, dl->origin);
			dl->origin[2] += 16;
		}
		else
		{
			vec3_t		end;
			float		falloff;
			vec3_t		vecForward, vecRight, vecUp;

			vec3_t viewAngles;

			if (R_IsRenderingFirstPersonView())
			{
				VectorCopy(r_playerViewportAngles, viewAngles);
			}
			else
			{
				VectorCopy(ent->angles, viewAngles);
				viewAngles[0] = viewAngles[0] * -3.0f;
			}

			AngleVectors(viewAngles, vecForward, vecRight, vecUp);

			VectorCopy(ent->origin, dl->origin);

			float viewheight[3]{};

			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(viewheight);

			VectorAdd(ent->origin, viewheight, dl->origin);
			VectorMA(dl->origin, r_flashlight_distance->GetValue(), vecForward, end);

			struct pmtrace_s trace {};

			if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
			{
				// Trace a line outward, don't use hitboxes (too slow)
				pmove_10152->usehull = 2;
				trace = pmove_10152->PM_PlayerTrace(dl->origin, end, PM_NORMAL, -1);

				if (trace.ent > 0 && pmove_10152->physents[trace.ent].studiomodel)
				{
					VectorCopy(pmove_10152->physents[trace.ent].origin, dl->origin);
				}
				else
				{
					VectorCopy(trace.endpos, dl->origin);
				}
			}
			else
			{
				// Trace a line outward, don't use hitboxes (too slow)
				pmove->usehull = 2;
				trace = pmove->PM_PlayerTrace(dl->origin, end, PM_NORMAL, -1);

				if (trace.ent > 0 && pmove->physents[trace.ent].studiomodel)
				{
					VectorCopy(pmove->physents[trace.ent].origin, dl->origin);
				}
				else
				{
					VectorCopy(trace.endpos, dl->origin);
				}
			}

			falloff = trace.fraction * r_flashlight_distance->GetValue();

			if (falloff < 500)
				falloff = 1.0;
			else
				falloff = 500.0 / falloff;

			falloff *= falloff;

			dl->radius = 80;
			dl->color.r = dl->color.g = dl->color.b = 255 * falloff;
		}

		// Make it live for a bit
		dl->die = (*cl_time) + 0.2f;
	}
}

void R_EmitFlashlights()
{
	int max_dlight = EngineGetMaxDLights();

	dlight_t* dl = cl_dlights;
	float curtime = (*cl_time);

	if (gEngfuncs.GetMaxClients() <= 1)
	{
		//Do nothing for singleplayer

		return;
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//SvEngine done a good job here we don't need to setup our own flashlights.
	}
	else
	{
		for (int i = 0; i < max_dlight; i++, dl++)
		{
			if (dl->die < curtime || !dl->radius)
				continue;

			if (dl->key == 4 || dl->key == 1) {
				memset(dl, 0, sizeof(dlight_t));
			}
		}

		for (int i = 0; i < gEngfuncs.GetMaxClients(); ++i)
		{
			auto state = R_GetPlayerState(i + 1);

			if (state->messagenum != (*cl_parsecount))
				continue;

			if (!state->modelindex || (state->effects & EF_NODRAW))
				continue;

			auto entindex = state->number;
			auto ent = gEngfuncs.GetEntityByIndex(entindex);

			if (!ent)
				continue;

			if (ent->curstate.effects & EF_BRIGHTLIGHT)
			{
				dl = gEngfuncs.pEfxAPI->CL_AllocDlight(DLIGHT_KEY_PLAYER_BRIGHTLIGHT + entindex);
				if (dl)
				{
					VectorCopy(ent->origin, dl->origin);
					dl->origin[2] += 16;
					dl->color.r = dl->color.g = dl->color.b = 250;
					dl->radius = gEngfuncs.pfnRandomFloat(400, 431);
					dl->die = (*cl_time) + 0.001f;
				}
			}
			if (ent->curstate.effects & EF_DIMLIGHT)
			{
				CL_EmitPlayerFlashlight(entindex);
			}
		}
	}
}

bool StudioGetActivityType(model_t* mod, entity_state_t* entstate, StudioAnimActivityType* pStudioAnimActivityType, int* pAnimControlFlags)
{
	if (mod->type != mod_studio)
		return false;

	auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return false;

	int sequence = entstate->sequence;

	if (sequence < 0 || sequence >= studiohdr->numseq)
		return false;

	auto pseqdesc = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex) + sequence;

	if (
		pseqdesc->activity == ACT_DIESIMPLE ||
		pseqdesc->activity == ACT_DIEBACKWARD ||
		pseqdesc->activity == ACT_DIEFORWARD ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIE_HEADSHOT ||
		pseqdesc->activity == ACT_DIE_CHESTSHOT ||
		pseqdesc->activity == ACT_DIE_GUTSHOT ||
		pseqdesc->activity == ACT_DIE_BACKSHOT
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_Death;
		(*pAnimControlFlags) = AnimControlFlag_OverrideAllBones;
		return true;
	}

	if (
		pseqdesc->activity == ACT_BARNACLE_HIT ||
		pseqdesc->activity == ACT_BARNACLE_PULL ||
		pseqdesc->activity == ACT_BARNACLE_CHOMP ||
		pseqdesc->activity == ACT_BARNACLE_CHEW
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_CaughtByBarnacle;
		(*pAnimControlFlags) = AnimControlFlag_OverrideAllBones;
		return true;
	}

	if (
		pseqdesc->activity == ACT_EAT
		&& 0 == stricmp(mod->name, "models/barnacle.mdl")
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_BarnacleChewing;
		(*pAnimControlFlags) = 0;
		return true;
	}

	if (
		pseqdesc->activity == ACT_RANGE_ATTACK2
		&& 0 == stricmp(mod->name, "models/garg.mdl")
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_GargantuaBite;
		(*pAnimControlFlags) = 0;
		return true;
	}

	(*pStudioAnimActivityType) = StudioAnimActivityType_Idle;
	(*pAnimControlFlags) = 0;
	return true;
}

void R_CreateLowerBodyModel()
{
	if (r_drawlowerbody->value < 1)
		return;

	auto LocalPlayer = gEngfuncs.GetLocalPlayer();

	if (EngineIsEntityInVisibleList(LocalPlayer))
		return;

	auto state = R_GetPlayerState(LocalPlayer->index);

	if (!state->modelindex || (state->effects & EF_NODRAW))
		return;

	auto pitch = (*r_refdef.viewangles)[0] * -1;

	if (pitch > r_drawlowerbodypitch->value)
		return;

	memcpy(&LocalPlayer->curstate, state, sizeof(*state));
	VectorCopy(cl_simorg, LocalPlayer->origin);
	VectorCopy(cl_simorg, LocalPlayer->curstate.origin);

	//Just like the VoiceStatus icons.
	gEngfuncs.CL_CreateVisibleEntity(ET_NORMAL, LocalPlayer);

	g_bHasLowerBody = true;
}

void R_Reload_f(void)
{
	R_ClearBSPEntities();

	std::vector<bspentity_t*> vBSPEntities;

	R_ParseBSPEntities((*cl_worldmodel)->entities, vBSPEntities);
	R_LoadExternalEntities(vBSPEntities);
	R_LoadBSPEntities(vBSPEntities);

	for (auto ent : vBSPEntities)
	{
		delete ent;
	}

	gEngfuncs.Con_Printf("Map entities reloaded\n");
}

void R_DumpTextures_f(void)
{
	auto pgltextures = gltextures_get();

	for (int j = 0; j < (*numgltextures); ++j)
	{
		if (pgltextures[j].texnum)
		{
			gEngfuncs.Con_Printf("[%s] texid[%d], servercount[%d]\n", pgltextures[j].identifier, pgltextures[j].texnum, pgltextures[j].servercount);
		}
	}
}

void R_AddDeferredFrameTask(IDeferredFrameTask* pTask)
{
	g_DeferredFrameTasks.emplace_back(pTask);
}

void R_ClearDeferredFrameTasks()
{
	for (auto it = g_DeferredFrameTasks.begin(); it != g_DeferredFrameTasks.end();)
	{
		auto pTask = (*it);

		pTask->Destroy();

		it = g_DeferredFrameTasks.erase(it);
	}
}

void R_ClearDeferredFrameTasksWithFlags(int flags)
{
	for (auto it = g_DeferredFrameTasks.begin(); it != g_DeferredFrameTasks.end();)
	{
		auto pTask = (*it);

		if (pTask->GetFlags() & flags)
		{
			pTask->Destroy();

			it = g_DeferredFrameTasks.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void R_RunDeferredFrameTasks()
{
	for (auto it = g_DeferredFrameTasks.begin(); it != g_DeferredFrameTasks.end();)
	{
		auto pTask = (*it);

		if (pTask->Run())
		{
			pTask->Destroy();

			it = g_DeferredFrameTasks.erase(it);
		}
		else
		{
			// Task is not finished yet, we will run it again next frame
			++it;
		}
	}
}

void Mod_ClearStudioModel(void)
{
	for (int i = 0;i < EngineGetMaxKnownModel(); i++)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type != mod_alias && mod->needload != NL_CLIENT)
		{
			mod->needload = NL_UNREFERENCED;

			if (mod->type == mod_sprite)
				mod->cache.data = NULL;

			R_FreeStudioRenderData(mod);
		}
	}
}

void Host_ClearMemory(qboolean bQuite)
{


	gPrivateFuncs.Host_ClearMemory(bQuite);
}