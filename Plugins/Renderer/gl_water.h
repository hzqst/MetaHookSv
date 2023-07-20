#pragma once

#include <vector>
#include <string>

#define MAX_WATERS 16

#define MAX_REFLECT_WATERS 16

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
	int level;
	std::string basetexture;
	std::string wildcard;
	std::string normalmap;
	float fresnelfactor[4];
	float depthfactor[3];
	float normfactor;
	float minheight;
	float maxtrans;
	float speedrate;
}water_control_t;

extern std::vector<water_control_t> r_water_controls;

//extern std::vector<cubemap_t> r_cubemaps;

typedef struct
{
	int program;
	int u_watercolor;
	int u_depthfactor;
	int u_fresnelfactor;
	int u_normfactor;
	int u_scale;
	int u_speed;
}water_program_t;

typedef struct
{
	int program;
	int depthmap;
}drawdepth_program_t;

typedef struct water_reflect_cache_s
{
	water_reflect_cache_s()
	{
		refractmap = 0;
		depthrefrmap = 0;
		reflectmap = 0;
		depthreflmap = 0;
		texwidth = 0;
		texheight = 0;
		normal[0] = 0;
		normal[1] = 0;
		normal[2] = 0;
		planedist = 0;
		color.r = 0;
		color.g = 0;
		color.b = 0;
		color.a = 0;
		level = 0;
		used = false;
	}
	GLuint refractmap;
	GLuint depthrefrmap;
	GLuint reflectmap;
	GLuint depthreflmap;
	GLsizei texwidth;
	GLsizei texheight;
	vec3_t normal;
	float planedist;
	colorVec color;
	int level;
	bool used;
	bool refractmap_ready;
}water_reflect_cache_t;

typedef struct water_vbo_s
{
	water_vbo_s()
	{
		hEBO = 0;
		hVAO = 0;
		texture = NULL;

		normalmap = 0;
		ripplemap = 0;

		ripple_data = NULL;
		ripple_image = NULL;
		ripple_spots[0] = NULL;
		ripple_spots[1] = NULL;
		ripple_shift = 0;
		ripple_width = 0;
		ripple_height = 0;
		ripple_framecount = 0;
		ripple_time = 0;

		fresnelfactor[0] = 0;
		fresnelfactor[1] = 0;
		fresnelfactor[2] = 0;
		fresnelfactor[3] = 0;
		depthfactor[0] = 0;
		depthfactor[1] = 0;
		depthfactor[2] = 0;
		normfactor = 0;
		minheight = 0;
		maxtrans = 0;
		speedrate = 0;
		level = 0;
		planedist = 0;
		plane = 0;
		iPolyCount = 0;
		iIndicesCount = 0;
		vIndicesBuffer = NULL;
	}
	GLuint hEBO;
	GLuint hVAO;

	texture_t *texture;

	GLuint normalmap;
	GLuint ripplemap;

	void *ripple_data;
	void *ripple_image;
	short *ripple_spots[2];
	int ripple_shift;
	int ripple_width;
	int ripple_height;
	int ripple_framecount;
	uint64_t ripple_time;

	float fresnelfactor[4];
	float depthfactor[3];
	float normfactor;
	float minheight;
	float maxtrans;
	float speedrate;
	int level;
	float planedist;
	vec3_t vert;
	vec3_t normal;
	mplane_t *plane;
	colorVec color;
	int iPolyCount;
	int iIndicesCount;
	std::vector<GLuint> *vIndicesBuffer;
}water_vbo_t;

//renderer
extern vec3_t g_CurrentCameraView;

//water
extern water_reflect_cache_t *g_CurrentReflectCache;
extern water_reflect_cache_t g_WaterReflectCaches[MAX_REFLECT_WATERS];
extern int g_iNumWaterReflectCaches;
//shader

//cvar
extern cvar_t *r_water;
extern cvar_t *r_water_forcetrans;
extern cvar_t *r_water_debug;

typedef struct
{
	int percent;
	int destcolor[3];
}cshift_t;

extern colorVec *gWaterColor;
extern cshift_t *cshift_water;

bool R_IsAboveWater(water_vbo_t *water);
void R_InitWater(void);
void R_ShutdownWater(void);
void R_RenderWaterPass(void);
void R_NewMapWater(void);
void R_UseWaterProgram(program_state_t state, water_program_t *progOutput);
void R_SaveWaterProgramStates(void);
void R_LoadWaterProgramStates(void);

#define WATER_LEGACY_ENABLED				0x1ull
#define WATER_UNDERWATER_ENABLED			0x2ull
#define WATER_GBUFFER_ENABLED				0x4ull
#define WATER_DEPTH_ENABLED					0x8ull
#define WATER_REFRACT_ENABLED				0x10ull
#define WATER_LINEAR_FOG_ENABLED			0x20ull
#define WATER_EXP_FOG_ENABLED				0x40ull
#define WATER_EXP2_FOG_ENABLED				0x80ull
#define WATER_OIT_ALPHA_BLEND_ENABLED		0x100ull
#define WATER_OIT_ADDITIVE_BLEND_ENABLED	0x200ull

#define WATER_LEVEL_LEGACY						0
#define WATER_LEVEL_REFLECT_SKYBOX				1
#define WATER_LEVEL_REFLECT_WORLD				2
#define WATER_LEVEL_REFLECT_ENTITY				3
#define WATER_LEVEL_REFLECT_SSR					4
#define WATER_LEVEL_LEGACY_RIPPLE				5
#define WATER_LEVEL_MAX							6