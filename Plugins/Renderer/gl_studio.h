#pragma once

#include "gl_draw.h"
#include <vector>
#include <unordered_map>

#define STUDIO_RENDER 1
#define STUDIO_EVENTS 2

// lighting options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT	0x0004
#define STUDIO_NF_NOMIPS        0x0008
#define STUDIO_NF_ALPHA         0x0010
#define STUDIO_NF_ADDITIVE      0x0020
#define STUDIO_NF_MASKED        0x0040

typedef struct auxvert_s
{
	float	fv[3];
}auxvert_t;

typedef struct alight_s
{
	int ambientlight;
	int shadelight;
	vec3_t color;
	float *plightvec;
}alight_t;

typedef struct
{
	int program;
	int bonematrix;
	int lightmatrix;

	int diffuseTex;
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
	int r_origin;
	int r_vright;
	int r_scale;

	int attrbone;
}studio_chrome_program_t, studiogbuffer_chrome_program_t;

typedef struct
{
	int program;
	int bonematrix;
	int lightmatrix;

	int diffuseTex;
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

	int attrbone;
}studio_fullbright_program_t, studiogbuffer_fullbright_program_t;

typedef struct
{
	int program;
	int bonematrix;
	int lightmatrix;

	int diffuseTex;
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

	int attrbone;
}studio_flatshade_program_t, studiogbuffer_flatshade_program_t;

typedef struct
{
	int program;
	int bonematrix;
	int lightmatrix;

	int diffuseTex;
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

	int attrbone;
}studio_program_t, studiogbuffer_program_t;

typedef struct
{
	char name[64];
	int index;
	int w, h;
	gltexture_t *glt;
}studio_texentry_t;

typedef struct
{
	studio_texentry_t base;
	studio_texentry_t replace;
	studio_texentry_t normal;
	float ambient;
	float diffuse;
	float specular;
	float shiness;
}studio_texture_t;

typedef struct
{
	char modelname[64];
	int numtextures;
	studio_texture_t *textures;
}studio_texarray_t;

typedef struct
{
	studio_texarray_t *pTexArray;
	int iNumTexArray;
}studio_texarray_mgr_t;

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

//engine
extern mstudiomodel_t **psubmodel;
extern studiohdr_t **pstudiohdr;
extern model_t **r_model;
extern float *r_blend;
extern float (*pbonetransform)[MAXSTUDIOBONES][3][4];
extern float (*plighttransform)[MAXSTUDIOBONES][3][4];
extern int (*g_NormalIndex)[MAXSTUDIOVERTS];
extern int (*chromeage)[MAXSTUDIOBONES];
extern int(*chrome)[MAXSTUDIOVERTS][2];
extern cl_entity_t *cl_viewent;
extern int *g_ForcedFaceFlags;
extern int *lightgammatable;
extern float *g_ChromeOrigin;
extern int *r_smodels_total;
extern int *r_ambientlight;
extern float *r_shadelight;
extern vec3_t *r_blightvec;
extern float *r_plightvec;
extern float *r_colormix;
extern model_t *cl_sprite_white;
extern model_t *cl_shellchrome;
//renderer

void R_StudioClearVBOCache(void);
void R_LoadStudioTextures(qboolean loadmap);
void R_InitStudio(void);

void R_GLStudioDrawPoints(void);
void R_StudioRenderFinal(void);

void studioapi_SetupRenderer(int rendermode);
void studioapi_RestoreRenderer(void);
void studioapi_StudioDynamicLight(cl_entity_t *ent, alight_t *plight);
void studioapi_SetupModel(int bodypart, void **ppbodypart, void **ppsubmodel);

extern engine_studio_api_t IEngineStudio;
extern r_studio_interface_t **gpStudioInterface;

extern cvar_t *r_studio_vbo;

#define SPRITE_VERSION 2

#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2
#define NL_CLIENT 3