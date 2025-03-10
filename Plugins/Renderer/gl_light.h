#pragma once

typedef struct light_dynamic_s
{
	int type;
	vec3_t origin;
	float color[3];
	float distance;
	float ambient;
	float diffuse;
	float specular;
	float specularpow;
	shadow_texture_t shadowtex;
}light_dynamic_t;

extern std::vector<light_dynamic_t> g_DynamicLights;

extern cvar_t *r_light_dynamic;
extern cvar_t *r_light_debug;
extern MapConVar *r_flashlight_ambient;
extern MapConVar *r_flashlight_diffuse;
extern MapConVar *r_flashlight_specular;
extern MapConVar *r_flashlight_specularpow;
extern MapConVar *r_flashlight_attachment;
extern MapConVar *r_flashlight_distance;
extern MapConVar *r_flashlight_cone_cosine;
extern GLuint r_flashlight_cone_texture;
extern std::string r_flashlight_cone_texture_name;

extern MapConVar *r_dynlight_ambient;
extern MapConVar *r_dynlight_diffuse;
extern MapConVar *r_dynlight_specular;
extern MapConVar *r_dynlight_specularpow;
extern MapConVar *r_dynlight_radius_scale;

extern cvar_t *r_ssr;
extern MapConVar *r_ssr_ray_step;
extern MapConVar *r_ssr_iter_count;
extern MapConVar *r_ssr_distance_bias;
extern MapConVar *r_ssr_exponential_step;
extern MapConVar *r_ssr_adaptive_step;
extern MapConVar *r_ssr_binary_search;
extern MapConVar *r_ssr_fade;

extern bool r_draw_gbuffer;

typedef void(*fnPointLightCallback)(float radius, vec3_t origin, vec3_t color, float ambient, float diffuse, float specular, float specularpow, shadow_texture_t *shadowtex, bool bVolume);
typedef void(*fnSpotLightCallback)(float distance, float radius,
	float coneAngle, float coneCosAngle, float coneSinAngle, float coneTanAngle,
	vec3_t origin, vec3_t angle, vec3_t vforward, vec3_t vright, vec3_t vup,
	vec3_t color, float ambient, float diffuse, float specular, float specularpow, shadow_texture_t *shadowtex, bool bVolume, bool bIsFromLocalPlayer);

void R_IterateDynamicLights(fnPointLightCallback pointlight_callback, fnSpotLightCallback spotlight_callback);

typedef struct
{
	int program;
	int u_lightdir;
	int u_lightright;
	int u_lightup;
	int u_lightpos;
	int u_lightcolor;
	int u_lightcone;
	int u_lightradius;
	int u_lightambient;
	int u_lightdiffuse;
	int u_lightspecular;
	int u_lightspecularpow;
	int u_shadowtexel;
	int u_shadowmatrix;
	int u_modelmatrix;
}dlight_program_t;

typedef struct
{
	int program;
	int u_ssrRayStep;
	int u_ssrIterCount;
	int u_ssrDistanceBias;
	int u_ssrFade;
}dfinal_program_t;

void R_LoadLightResources(void);
void R_InitLight(void);
void R_ShutdownLight(void);
bool R_BeginRenderGBuffer(void);
void R_EndRenderGBuffer(FBO_Container_t* dst);
void R_SetGBufferMask(int mask);
void R_SetGBufferBlend(int blendsrc, int blenddst);
void R_SaveDLightProgramStates(void);
void R_SaveDFinalProgramStates(void);
void R_LoadDLightProgramStates(void);
void R_LoadDFinalProgramStates(void);
void R_BlitGBufferToFrameBuffer(FBO_Container_t* fbo, bool color, bool depth, bool stencil);

#define GBUFFER_INDEX_DIFFUSE		0
#define GBUFFER_INDEX_LIGHTMAP		1
#define GBUFFER_INDEX_WORLDNORM		2
#define GBUFFER_INDEX_SPECULAR		3
#define GBUFFER_INDEX_MAX			4

#define GBUFFER_MASK_DIFFUSE		(1<<GBUFFER_INDEX_DIFFUSE)
#define GBUFFER_MASK_LIGHTMAP		(1<<GBUFFER_INDEX_LIGHTMAP)
#define GBUFFER_MASK_WORLDNORM		(1<<GBUFFER_INDEX_WORLDNORM)
#define GBUFFER_MASK_SPECULAR		(1<<GBUFFER_INDEX_SPECULAR)

#define GBUFFER_MASK_ALL			(GBUFFER_MASK_DIFFUSE | GBUFFER_MASK_LIGHTMAP | GBUFFER_MASK_WORLDNORM | GBUFFER_MASK_SPECULAR)

#define DLIGHT_SPOT_ENABLED						0x1ull
#define DLIGHT_POINT_ENABLED					0x2ull
#define DLIGHT_VOLUME_ENABLED					0x4ull
#define DLIGHT_CONE_TEXTURE_ENABLED				0x8ull
#define DLIGHT_SHADOW_TEXTURE_ENABLED			0x10ull

#define DFINAL_LINEAR_FOG_ENABLED				0x1ull
#define DFINAL_EXP_FOG_ENABLED					0x2ull
#define DFINAL_EXP2_FOG_ENABLED					0x4ull
#define DFINAL_SKY_FOG_ENABLED					0x8ull
#define DFINAL_SSR_ENABLED						0x10ull
#define DFINAL_SSR_ADAPTIVE_STEP_ENABLED		0x20ull
#define DFINAL_SSR_EXPONENTIAL_STEP_ENABLED		0x40ull
#define DFINAL_SSR_BINARY_SEARCH_ENABLED		0x80ull

#define DLIGHT_POINT					0
#define DLIGHT_SPOT						1