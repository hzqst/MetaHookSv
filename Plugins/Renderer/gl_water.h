#pragma once

#define MAX_WATERS 32

typedef struct
{
	int program;
	int watercolor;
	int entitymatrix;
	int worldmatrix;
	int eyepos;
	int time;
	int fresnelfactor;
	int depthfactor;
	int normfactor;
	int normalmap;//0
	int refractmap;//1
	int reflectmap;//2
	int depthrefrmap;//3
}water_program_t;

typedef struct
{
	int program;
	int depthmap;
}drawdepth_program_t;

extern GLuint refractmap;
extern GLuint depthrefrmap;
extern qboolean refractmap_ready;

typedef struct r_water_s
{
	GLuint reflectmap;
	GLuint depthreflmap;

	vec3_t vecs;
	vec3_t norm;
	float distances;
	cl_entity_t *ent;
	vec3_t org;
	colorVec color;
	int free;
	int framecount;
	qboolean ready;
	struct r_water_s *next;
}r_water_t;

//renderer
extern vec3_t water_view;

//water
extern r_water_t *curwater;
extern r_water_t *waters_free;
extern r_water_t *waters_active;

//shader
extern int water_normalmap;

extern SHADER_DEFINE(drawdepth);

//cvar
extern cvar_t *r_water;
extern cvar_t *r_water_debug;
extern cvar_t *r_water_fresnelfactor;
extern cvar_t *r_water_depthfactor1;
extern cvar_t *r_water_depthfactor2;
extern cvar_t *r_water_normfactor;
extern cvar_t *r_water_minheight;
extern cvar_t *r_water_maxalpha;

typedef struct
{
	int percent;
	int destcolor[3];
}cshift_t;

extern colorVec *gWaterColor;
extern cshift_t *cshift_water;

r_water_t *R_GetActiveWater(cl_entity_t *ent, vec3_t p, vec3_t n, colorVec *color);
void R_InitWater(void);
void R_FreeWater(void);
void R_ClearWater(void);
void R_RenderWaterView(void);
void R_FreeDeadWaters(void);
void R_UseWaterProgram(int state, water_program_t *progOutput);

#define WATER_UNDERWATER_ENABLED		1
#define WATER_GBUFFER_ENABLED			2
#define WATER_DEPTH_ENABLED				4
#define WATER_REFRACT_ENABLED			8