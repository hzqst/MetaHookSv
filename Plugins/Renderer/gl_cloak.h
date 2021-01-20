#pragma once

typedef struct
{
	int program;
	int refract;
	int eyepos;
	int cloakfactor;
	int refractamount;	
}cloak_program_t;

extern SHADER_DEFINE(cloak);

extern int cloak_texture;
extern cvar_t *r_cloak_debug;

typedef struct
{
	int program;
	int refractmap;
	int normalmap;
	int packedfactor;
}conc_program_t;

void R_InitCloak(void);
void R_RenderCloakTexture(void);
void R_BeginRenderConc(float flBlurFactor, float flRefractFactor);
int R_GetCloakTexture(void);