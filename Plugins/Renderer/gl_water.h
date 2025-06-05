#pragma once

#include <vector>
#include <string>

#define MAX_WATERS 16

#define MAX_REFLECT_WATERS 16

#define WATER_LEVEL_LEGACY						0
#define WATER_LEVEL_REFLECT_SKYBOX				1
#define WATER_LEVEL_REFLECT_WORLD				2
#define WATER_LEVEL_REFLECT_ENTITY				3
#define WATER_LEVEL_REFLECT_SSR					4
#define WATER_LEVEL_LEGACY_RIPPLE				5
#define WATER_LEVEL_MAX							6

typedef struct cubemap_s
{
	std::string name;
	std::string extension;
	int cubetex;
	int size;
	float radius;
	vec3_t origin;
}cubemap_t;

class CEnvWaterControl
{
public:
	int level{ WATER_LEVEL_REFLECT_SKYBOX  };
	std::string basetexture;
	std::string wildcard;
	std::string normalmap;
	float fresnelfactor[4]{0};
	float depthfactor[3]{0};
	float normfactor{};
	float minheight{};
	float maxtrans{};
	float speedrate{ 1 };
};

extern std::vector<CEnvWaterControl*> g_EnvWaterControls;

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
	GLuint refract_texture{};
	GLuint refract_depth_texture{};
	GLuint reflect_texture{};
	GLuint reflect_depth_texture{};
	GLuint reflect_stencil_view_texture{};
	GLsizei texwidth{};
	GLsizei texheight{};
	vec3_t normal{};
	float planedist{};
	colorVec color{};
	int level{};
	bool used{};
	bool refractmap_ready{};
}water_reflect_cache_t;

class CWaterSurfaceModel
{
public:
	~CWaterSurfaceModel();

	GLuint hABO{};

	texture_t* texture{};

	GLuint normalmap{};
	GLuint ripplemap{};

	void* ripple_data{};
	void* ripple_image{};
	short* ripple_spots[2]{};
	int ripple_shift{};
	int ripple_width{};
	int ripple_height{};
	int ripple_framecount{};
	uint64_t ripple_time{};

	float fresnelfactor[4]{};
	float depthfactor[3]{};
	float normfactor{};
	float minheight{};
	float maxtrans{};
	float speedrate{};
	int level{};
	float planedist{};
	vec3_t vert{};
	vec3_t normal{};
	mplane_t *plane{};
	colorVec color{};
	uint32_t polyCount{};
	uint32_t drawCount{};
	std::vector<CDrawIndexAttrib> vDrawAttribBuffer{};
};

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

bool R_IsAboveWater(CWaterSurfaceModel *pWaterModel);
void R_InitWater(void);
void R_ShutdownWater(void);
void R_RenderWaterPass(void);
void R_UseWaterProgram(program_state_t state, water_program_t *progOutput);
void R_SaveWaterProgramStates(void);
void R_LoadWaterProgramStates(void);

class CWorldSurfaceModel;
class CWorldSurfaceLeaf;
void R_DrawWaters(CWorldSurfaceModel* pModel, CWorldSurfaceLeaf* pLeaf, cl_entity_t* ent);

#define WATER_LEGACY_ENABLED				0x1ull
#define WATER_UNDERWATER_ENABLED			0x2ull
#define WATER_GBUFFER_ENABLED				0x4ull
#define WATER_DEPTH_ENABLED					0x8ull
#define WATER_REFRACT_ENABLED				0x10ull
#define WATER_LINEAR_FOG_ENABLED			0x20ull
#define WATER_EXP_FOG_ENABLED				0x40ull
#define WATER_EXP2_FOG_ENABLED				0x80ull
#define WATER_ALPHA_BLEND_ENABLED			0x100ull
#define WATER_ADDITIVE_BLEND_ENABLED		0x200ull
#define WATER_OIT_BLEND_ENABLED				0x400ull
#define WATER_GAMMA_BLEND_ENABLED			0x800ull
/*
layout(binding = 0) uniform sampler2D baseTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D reflectTex;
layout(binding = 3) uniform sampler2D refractTex;
layout(binding = 4) uniform sampler2D depthTex;
*/
#define WATER_BIND_BASE_TEXTURE				0
#define WATER_BIND_NORMAL_TEXTURE			1
#define WATER_BIND_REFLECT_TEXTURE			2
#define WATER_BIND_REFLECT_STENCIL_TEXTURE	3
#define WATER_BIND_REFRACT_TEXTURE			4
#define WATER_BIND_REFRACT_DEPTH_TEXTURE	5

