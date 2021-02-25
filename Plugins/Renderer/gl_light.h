#pragma once

extern int r_light_env_color[4];
extern qboolean r_light_env_color_exists;
extern vec3_t r_light_env_angles;
extern qboolean r_light_env_angles_exists;
extern cvar_t *r_light_dynamic;
extern cvar_t *r_light_debug;
extern bool drawpolynocolor;
extern bool drawgbuffer;

typedef struct
{
	int program;
	int diffuseTex;
	int lightmapTex;
	int detailTex;
}gbuffer_color_program_t, 
gbuffer_diffuse_program_t, 
gbuffer_lightmap_program_t, 
gbuffer_detail_program_t, 
gbuffer_transparent_diffuse_program_t,
gbuffer_transparent_color_program_t,
gbuffer_lightmap_only_program_t;

typedef struct
{
	int program;
	int positionTex;
	int normalTex;
	int viewpos;
	int lightdir;
	int lightpos;
	int lightcolor;
	int lightcone;
	int lightradius;
	int lightambient;
	int lightdiffuse;
	int lightspecular;
	int lightspecularpow;
}dlight_spot_program_t;

typedef struct
{
	int program;
	int positionTex;
	int normalTex;
	int viewpos;
	int lightpos;
	int lightcolor;
	int lightradius;
	int lightambient;
	int lightdiffuse;
	int lightspecular;
	int lightspecularpow;
}dlight_point_program_t;

typedef struct
{
	int program;
	int diffuseTex;
	int lightmapTex;
	int depthTex;
}dlight_final_program_t;

void R_InitLight(void);
void R_BeginRenderGBuffer(void);
void R_EndRenderGBuffer(void);
void R_SetGBufferRenderState(int state);
void R_SetGBufferMask(int mask);

#define GBUFFER_MASK_DIFFUSE		1
#define GBUFFER_MASK_LIGHTMAP		2
#define GBUFFER_MASK_WORLD			4
#define GBUFFER_MASK_NORMAL			8

#define GBUFFER_MASK_ALL (GBUFFER_MASK_DIFFUSE | GBUFFER_MASK_LIGHTMAP | GBUFFER_MASK_WORLD | GBUFFER_MASK_NORMAL )

#define GBUFFER_STATE_COLOR			1
#define GBUFFER_STATE_DIFFUSE			2
#define GBUFFER_STATE_LIGHTMAP			3
#define GBUFFER_STATE_DETAIL			4
#define GBUFFER_STATE_TRANSPARENT_COLOR		5
#define GBUFFER_STATE_TRANSPARENT_DIFFUSE		6
#define GBUFFER_STATE_LIGHTMAP_ONLY		7