#pragma once

#include "gl_draw.h"
#include <vector>
#include <unordered_map>

typedef struct
{
	int program;
	int diffuseTex;

	int bonematrix;

	int v_lambert;
	int v_brightness;
	int v_lightgamma;
	int r_ambientlight;
	int r_shadelight;
	int r_blend;
	int r_g1;
	int r_g3;
	int r_plightvec;
	int r_colormix;
	//chrome
	int r_origin;
	int r_vright;
	int r_scale;
	//shadow caster
	int entityPos;
	//attribute
	int attr_bone;
}studio_program_t;

typedef struct studio_vbo_vertex_s
{
	studio_vbo_vertex_s(float *a, float *b, float *c, int d, int e)
	{
		memcpy(pos, a, sizeof(vec3_t));
		memcpy(normal, b, sizeof(vec3_t));
		memcpy(texcoord, c, sizeof(vec2_t));
		vertbone = d;
		normbone = e;
	}
	studio_vbo_vertex_s()
	{

	}
	vec3_t	pos;
	vec3_t	normal;
	vec2_t	texcoord;
	int		vertbone;
	int		normbone;
}studio_vbo_vertex_t;

typedef struct studio_vbo_trilist_s
{
	studio_vbo_trilist_s()
	{
		start_vertex = 0;
		num_vertex = 0;
		draw_type = 0;
	}
	studio_vbo_trilist_s(int a, int b, int c) : start_vertex(a), num_vertex(b), draw_type(c)
	{

	}
	int start_vertex;
	int num_vertex;
	int draw_type;
}studio_vbo_trilist_t;

typedef struct studio_vbo_mesh_s
{
	studio_vbo_mesh_s()
	{
		iTriStripStartIndex = -1;
		iTriStripVertexCount = 0;
		iTriFanStartIndex = -1;
		iTriFanVertexCount = 0;
	}

	std::vector<studio_vbo_trilist_t> vTri;
	std::vector<unsigned int> vTriStrip;
	std::vector<unsigned int> vTriFan;
	int iTriStripStartIndex;
	int iTriStripVertexCount;
	int iTriFanStartIndex;
	int iTriFanVertexCount;
}studio_vbo_mesh_t;

typedef struct studio_vbo_submodel_s
{
	studio_vbo_submodel_s()
	{
		vMesh = NULL;
		iNumMesh = 0;
	}
	studio_vbo_mesh_t *vMesh;
	int iNumMesh;
}studio_vbo_submodel_t;

typedef struct studio_vbo_s
{
	studio_vbo_s()
	{
		hDataBuffer = 0;
		hIndexBuffer = 0;
		iStartIndices = 0;
	}

	GLuint				hDataBuffer;
	GLuint				hIndexBuffer;
	std::unordered_map<mstudiomodel_t *, studio_vbo_submodel_t *> vSubmodel;
	std::vector<studio_vbo_vertex_t> vVertex;
	std::vector<unsigned int> vIndices;
	int					iStartIndices;
}studio_vbo_t;

typedef struct studio_bone_s
{
	studio_bone_s()
	{
		
	}

	int framecount;
	int numbones;
	float cached_bonetransform[MAXSTUDIOBONES][3][4];
	float cached_lighttransform[MAXSTUDIOBONES][3][4];
}studio_bone_t;

//engine
extern mstudiomodel_t **psubmodel;
extern mstudiobodyparts_t **pbodypart;
extern studiohdr_t **pstudiohdr;
extern model_t **r_model;
extern float *r_blend;
extern auxvert_t **pauxverts;
extern float **pvlightvalues;
extern auxvert_t(*auxverts)[MAXSTUDIOVERTS];
extern vec3_t(*lightvalues)[MAXSTUDIOVERTS];
extern float (*pbonetransform)[MAXSTUDIOBONES][3][4];
extern float (*plighttransform)[MAXSTUDIOBONES][3][4];
extern float(*rotationmatrix)[3][4];
extern int (*g_NormalIndex)[MAXSTUDIOVERTS];
extern int (*chromeage)[MAXSTUDIOBONES];
extern int(*chrome)[MAXSTUDIOVERTS][2];
extern cl_entity_t *cl_viewent;
extern int *g_ForcedFaceFlags;
extern int *lightgammatable;
extern float *g_ChromeOrigin;
extern int *r_ambientlight;
extern float *r_shadelight;
extern vec3_t *r_blightvec;
extern float *r_plightvec;
extern float *r_colormix;
extern model_t *cl_sprite_white;
extern model_t *cl_shellchrome;

//renderer
extern int r_studio_drawcall;
extern int r_studio_polys;
extern int r_studio_framecount;

//void R_StudioClearBoneCache(void);
void R_StudioClearVBOCache(void);
void R_ShutdownStudio(void);
void R_InitStudio(void);
bool R_StudioRestoreBones(void);
void R_StudioSaveBones(void);
void R_GLStudioDrawPoints(void);

void studioapi_SetupRenderer(int rendermode);
void studioapi_RestoreRenderer(void);
void studioapi_StudioDynamicLight(cl_entity_t *ent, alight_t *plight);
void studioapi_SetupModel(int bodypart, void **ppbodypart, void **ppsubmodel);

extern engine_studio_api_t IEngineStudio;
extern r_studio_interface_t **gpStudioInterface;

extern cvar_t *r_studio_vbo;

#define STUDIO_GBUFFER_ENABLED			0x10000
#define STUDIO_TRANSPARENT_ENABLED		0x20000
#define STUDIO_TRANSADDITIVE_ENABLED	0x40000
#define STUDIO_LINEAR_FOG_ENABLED		0x80000
#define STUDIO_SHADOW_CASTER_ENABLED	0x100000
#define STUDIO_LEGACY_BONE_ENABLED		0x200000