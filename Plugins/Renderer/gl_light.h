#pragma once

extern int r_light_env_color[4];
extern vec3_t r_light_env_angles;
extern qboolean r_light_env_enabled;
extern cvar_t *r_light_dynamic;
extern cvar_t *r_light_debug;
extern bool drawpolynocolor;
extern bool drawgbuffer;

typedef struct
{
	int program;
	int diffuseTex;
}gbuffer1_program_t;

typedef struct
{
	int program;
	int diffuseTex;
	int lightmapTex;
}gbuffer2_program_t;

typedef struct
{
	int program;
	int diffuseTex;
	int lightmapTex;
	int detailTex;
}gbuffer3_program_t;

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
void R_SetRenderGBufferDecal(void);
void R_SetRenderGBufferWater(void);
void R_SetRenderGBufferAll(void);