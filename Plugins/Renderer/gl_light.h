#pragma once

extern int r_light_env_color[4];
extern qboolean r_light_env_color_exists;
extern vec3_t r_light_env_angles;
extern qboolean r_light_env_angles_exists;
extern cvar_t *r_light_dynamic;
extern cvar_t *r_light_debug;
extern cvar_t *r_light_darkness;

extern bool drawgbuffer;

typedef struct deferred_light_s
{
	deferred_light_s()
	{
		type = 0;
		memset(origin, 0, sizeof(origin));
		memset(color, 0, sizeof(color));
		fade = 0;
	}
	deferred_light_s(int t, float *o, float *c, float f)
	{
		type = t;
		memcpy(origin, o, sizeof(origin));
		memcpy(color, c, sizeof(color));
		fade = f;
	}

	int type;
	vec3_t origin;
	float color[4];
	float fade;
}deferred_light_t;

typedef struct
{
	int program;
	int diffuseTex;
	int lightmapTex;
	int lightmapTexArray;
	int detailTex;
}gbuffer_program_t;

typedef struct
{
	int program;
	int positionTex;
	int normalTex;
	int stencilTex;
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
	int diffuseTex;
	int lightmapTex;
	int additiveTex;
	int depthTex;
	int clipInfo;
}dlight_program_t;

void R_InitLight(void);
void R_ShutdownLight(void);
bool R_BeginRenderGBuffer(void);
void R_EndRenderGBuffer(void);
void R_SetGBufferMask(int mask);
void R_UseGBufferProgram(int state);
void R_UseGBufferProgram(int state, gbuffer_program_t *progOutput);

#define GBUFFER_MASK_DIFFUSE		1
#define GBUFFER_MASK_LIGHTMAP		2
#define GBUFFER_MASK_WORLD			4
#define GBUFFER_MASK_NORMAL			8
#define GBUFFER_MASK_ADDITIVE		16

#define GBUFFER_MASK_ALL (GBUFFER_MASK_DIFFUSE | GBUFFER_MASK_LIGHTMAP | GBUFFER_MASK_WORLD | GBUFFER_MASK_NORMAL | GBUFFER_MASK_ADDITIVE)

#define GBUFFER_DIFFUSE_ENABLED			1
#define GBUFFER_LIGHTMAP_ENABLED		2
#define GBUFFER_DETAILTEXTURE_ENABLED	4
#define GBUFFER_LIGHTMAP_ARRAY_ENABLED	8
#define GBUFFER_TRANSPARENT_ENABLED		16
#define GBUFFER_ADDITIVE_ENABLED		32

#define DLIGHT_LIGHT_PASS				2
#define DLIGHT_LIGHT_PASS_SPOT			4
#define DLIGHT_LIGHT_PASS_POINT			8
#define DLIGHT_LIGHT_PASS_VOLUME		0x10
#define DLIGHT_FINAL_PASS				0x20
#define DLIGHT_LINEAR_FOG_ENABLED		0x40

#define DLIGHT_POINT					0
#define DLIGHT_SPOT						1