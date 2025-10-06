#pragma once

#ifndef GL_WATER_H
#define GL_WATER_H

#include <vector>
#include <string>

#define MAX_WATERS 16

#define MAX_REFLECT_WATERS 4

#define WATER_LEVEL_LEGACY						0
#define WATER_LEVEL_REFLECT_SKYBOX				1
#define WATER_LEVEL_REFLECT_WORLD				1
#define WATER_LEVEL_REFLECT_ENTITY				2
#define WATER_LEVEL_REFLECT_SSR					3
#define WATER_LEVEL_LEGACY_RIPPLE				4
#define WATER_LEVEL_MAX							5

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

extern std::vector<std::shared_ptr<CEnvWaterControl>> g_EnvWaterControls;

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

class CWaterReflectCache
{
public:
	GLuint refract_texture{};
	GLuint refract_depth_texture{};
	GLuint reflect_texture{};
	GLuint reflect_depth_texture{};
	GLsizei texwidth{};
	GLsizei texheight{};
	vec3_t normal{};
	float planedist{};
	colorVec color{};
	int level{};
	bool used{};
	bool refractmap_ready{};
	bool reflectmap_ready{};
};

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
	std::vector<CDrawIndexAttrib> m_vDrawAttribBuffer{};
};

//water
extern CWaterReflectCache *g_CurrentReflectCache;
extern CWaterReflectCache g_WaterReflectCaches[MAX_REFLECT_WATERS];
//shader

//cvar
extern cvar_t *r_water;

extern colorVec *gWaterColor;
extern cshift_t *cshift_water;

bool R_IsAboveWater(CWaterSurfaceModel *pWaterModel);
bool R_IsAboveWater(CWaterReflectCache* pWaterReflectCache);
void R_InitWater(void);
void R_ShutdownWater(void);
void R_RenderWaterPass(void);
void R_UseWaterProgram(program_state_t state, water_program_t *progOutput);
void R_SaveWaterProgramStates(void);
void R_LoadWaterProgramStates(void);

class CWorldSurfaceModel;
class CWorldSurfaceLeaf;
void R_DrawWaters(CWorldSurfaceModel* pModel, CWorldSurfaceLeaf* pLeaf, cl_entity_t* ent);

#endif // GL_WATER_H