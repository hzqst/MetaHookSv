#pragma once

#include <vector>
#include <string>

#define MAX_WATERS 64

typedef struct cubemap_s
{
	std::string name;
	std::string extension;
	int cubetex;
	int size;
	float radius;
	vec3_t origin;
}cubemap_t;

typedef struct water_control_s
{
	bool enabled;
	std::string basetexture;
	std::string wildcard;
	std::string normalmap;
	float fresnelfactor;
	float depthfactor[2];
	float normfactor;
	float minheight;
	float maxtrans;
	int level;
}water_control_t;

extern std::vector<water_control_t> r_water_controls;

//extern std::vector<cubemap_t> r_cubemaps;

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

typedef struct
{
	int program;
	int colormap;
}drawcolor_program_t;

extern GLuint refractmap;
extern GLuint depthrefrmap;
extern qboolean refractmap_ready;

typedef struct r_water_s
{
	GLuint normalmap;
	GLuint reflectmap;
	GLuint depthreflmap;
	float fresnelfactor;
	float depthfactor[2];
	float normfactor;
	float minheight;
	float maxtrans;
	int level;
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

extern SHADER_DEFINE(drawdepth);
extern SHADER_DEFINE(drawcolor);

//cvar
extern cvar_t *r_water;
extern cvar_t *r_water_debug;

typedef struct
{
	int percent;
	int destcolor[3];
}cshift_t;

extern colorVec *gWaterColor;
extern cshift_t *cshift_water;

bool R_IsAboveWater(float *v);

r_water_t *R_GetActiveWater(cl_entity_t *ent, const char *texname, vec3_t p, vec3_t n, colorVec *color, bool bAboveWater);
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

#define WATER_LEVEL_REFLECT_SKYBOX		0
#define WATER_LEVEL_REFLECT_WORLD		1
#define WATER_LEVEL_REFLECT_ENTITY		2
#define WATER_LEVEL_REFLECT_SSR			3