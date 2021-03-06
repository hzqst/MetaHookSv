#pragma once

extern int r_light_env_color[4];
extern qboolean r_light_env_color_exists;
extern vec3_t r_light_env_angles;
extern qboolean r_light_env_angles_exists;
extern cvar_t *r_light_dynamic;
extern cvar_t *r_light_debug;

extern bool drawgbuffer;

typedef struct
{
	int program;
	int diffuseTex;
	int lightmapTex;
	int lightmapTexArray;
	int detailTex;
	int speed;
	int entitymatrix;
}gbuffer_program_t;

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
	int additiveTex;
	int depthTex;
}dlight_final_program_t, dlight_final2_program_t;

void R_InitLight(void);
void R_ShutdownLight(void);
void R_BeginRenderGBuffer(void);
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
#define GBUFFER_SCROLL_ENABLED			64
#define GBUFFER_ROTATE_ENABLED			128