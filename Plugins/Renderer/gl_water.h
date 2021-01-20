#pragma once

#define MAX_WATERS 32

typedef struct
{
	int program;
	int waterfogcolor;
	int eyepos;
	int eyedir;
	int zmax;
	int time;
	int fresnel;
	int abovewater;
	int normalmap;
	int refractmap;
	int reflectmap;	
	int depthrefrmap;
}water_program_t;

typedef struct
{
	int program;
	int depthmap;
}drawdepth_program_t;

typedef struct r_water_s
{
	GLuint refractmap;
	GLuint reflectmap;
	GLuint depthrefrmap;
	vec3_t vecs;
	float distances;
	cl_entity_t *ent;
	vec3_t org;
	struct r_water_s *next;
	int is3dsky;
}r_water_t;

//renderer
extern qboolean drawreflect;
extern qboolean drawrefract;
extern mplane_t custom_frustum[4];
extern int water_update_counter;
extern int water_texture_size;
//water
extern r_water_t *curwater;
extern r_water_t *waters_free;
extern r_water_t *waters_active;

//shader
extern SHADER_DEFINE(water);
extern int water_normalmap;
extern int water_normalmap_default;

extern SHADER_DEFINE(drawdepth);

//water fog
extern int *g_bUserFogOn;
extern int save_userfogon;
extern int waterfog_on;

typedef struct
{
	qboolean fog;
	vec4_t color;
	float start;
	float end;
	float density;
	float fresnel;
	qboolean active;
}water_parm_t;

extern water_parm_t water_parm;

//cvar
extern cvar_t *r_water;
extern cvar_t *r_water_debug;
extern cvar_t *r_water_fresnel;

void R_AddWater(cl_entity_t *ent, vec3_t p);
void R_InitWater(void);
void R_ClearWater(void);
void R_SetupReflect(void);
void R_FinishReflect(void);
void R_SetupRefract(void);
void R_FinishRefract(void);
void R_UpdateWater(void);
void R_SetupClip(qboolean isdrawworld);
void R_SetWaterParm(water_parm_t *parm);
void R_SetCustomFrustum(void);