#pragma once

#include <vector>
#include <string>

#define MAX_WATERS 256

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
	float fresnelfactor;
	float depthfactor[2];
	float normfactor;
	float minheight;
	float maxtrans;
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
}water_program_t;

typedef struct
{
	int program;
	int depthmap;
}drawdepth_program_t;

//extern GLuint refractmap;
//extern GLuint depthrefrmap;
extern bool refractmap_ready;

typedef struct water_vbo_s
{
	water_vbo_s()
	{
		hEBO = 0;
		hTextureSSBO = 0;

		index = -1;
		ent = NULL;
		texture = NULL;

		reflectmap = 0;
		depthreflmap = 0;
		normalmap = 0;

		reflectmap_handle = 0;
		normalmap_handle = 0;

		fresnelfactor = 0;
		depthfactor[0] = 0;
		depthfactor[1] = 0;
		normfactor = 0;
		minheight = 0;
		maxtrans = 0;
		level = 0;
		plane = 0;
		iPolyCount = 0;
		framecount = 0;
	}
	GLuint hEBO;
	GLuint hTextureSSBO;

	int index;
	cl_entity_t *ent;
	texture_t *texture;

	GLuint reflectmap;
	GLuint depthreflmap;
	GLuint normalmap;

	GLuint64 basetexture_handle;
	GLuint64 reflectmap_handle;
	GLuint64 normalmap_handle;

	float fresnelfactor;
	float depthfactor[2];
	float normfactor;
	float minheight;
	float maxtrans;
	int level;
	float plane;
	vec3_t vert;
	vec3_t normal;
	colorVec color;
	std::vector<GLuint> vIndicesBuffer;
	int iPolyCount;
	int framecount;
}water_vbo_t;

//renderer
extern vec3_t water_view;

//water
extern water_vbo_t *curwater;

extern std::vector<water_vbo_t *> g_WaterVBOCache;
extern water_vbo_t *g_RenderWaterVBOCache[512];
extern int g_iNumRenderWaterVBOCache;
//shader

extern SHADER_DEFINE(drawdepth);

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

bool R_IsAboveWater(water_vbo_t *water);

water_vbo_t *R_PrepareWaterVBO(cl_entity_t *ent, msurface_t *surf);
void R_InitWater(void);
void R_FreeWater(void);
void R_RenderWaterView(void);
void R_NewMapWater(void);
void R_UseWaterProgram(int state, water_program_t *progOutput);
void R_DrawWaters(void);

#define WATER_LEGACY_ENABLED				1
#define WATER_UNDERWATER_ENABLED			2
#define WATER_GBUFFER_ENABLED				4
#define WATER_DEPTH_ENABLED					8
#define WATER_REFRACT_ENABLED				0x10
#define WATER_LINEAR_FOG_ENABLED			0x20
#define WATER_BINDLESS_ENABLED				0x40
#define WATER_OIT_ALPHA_BLEND_ENABLED		0x80
#define WATER_OIT_ADDITIVE_BLEND_ENABLED	0x100

#define WATER_LEVEL_LEGACY				0
#define WATER_LEVEL_REFLECT_SKYBOX		1
#define WATER_LEVEL_REFLECT_WORLD		2
#define WATER_LEVEL_REFLECT_ENTITY		3
#define WATER_LEVEL_REFLECT_SSR			4