#pragma once

#include "gl_cvar.h"
#include "gl_draw.h"
#include <vector>
#include <unordered_map>

typedef struct
{
	int program;
	//celshade
	int r_celshade_midpoint;
	int r_celshade_softness;
	int r_celshade_shadow_color;
	int r_rimlight_power;
	int r_rimlight_smooth;
	int r_rimlight_smooth2;
	int r_rimlight_color;
	int r_rimdark_power;
	int r_rimdark_smooth;
	int r_rimdark_smooth2;
	int r_rimdark_color;
	int r_outline_dark;
	int r_hair_specular_exp;
	int r_hair_specular_noise;
	int r_hair_specular_intensity;
	int r_hair_specular_exp2;
	int r_hair_specular_noise2;
	int r_hair_specular_intensity2;
	int r_hair_specular_smooth;
	int r_hair_shadow_offset;
	int r_uvscale;
	//shadow caster
	int entityPos;
}studio_program_t;

typedef struct studio_vbo_vertex_s
{
	studio_vbo_vertex_s(float *a, float *b, float s, float t, int d, int e)
	{
		memcpy(pos, a, sizeof(vec3_t));
		memcpy(normal, b, sizeof(vec3_t));
		texcoord[0] = s;
		texcoord[1] = t;
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
		mesh = NULL;
		iStartIndex = -1;
		iIndiceCount = 0;
		iPolyCount = 0;
	}

	mstudiomesh_t *mesh;
	int iStartIndex;
	int iIndiceCount;
	int iPolyCount;
}studio_vbo_mesh_t;

typedef struct studio_vbo_submodel_s
{
	studio_vbo_submodel_s()
	{
		submodel = NULL;
	}
	mstudiomodel_t *submodel;
	std::vector<studio_vbo_mesh_t> vMesh;
}studio_vbo_submodel_t;

typedef struct studio_celshade_control_s
{
	StudioConVar celshade_midpoint;
	StudioConVar celshade_softness;
	StudioConVar celshade_shadow_color;
	StudioConVar outline_size;
	StudioConVar outline_dark;
	StudioConVar rimlight_power;
	StudioConVar rimlight_smooth;
	StudioConVar rimlight_smooth2;
	StudioConVar rimlight_color;
	StudioConVar rimdark_power;
	StudioConVar rimdark_smooth;
	StudioConVar rimdark_smooth2;
	StudioConVar rimdark_color;
	StudioConVar hair_specular_exp;
	StudioConVar hair_specular_intensity;
	StudioConVar hair_specular_noise;
	StudioConVar hair_specular_exp2;
	StudioConVar hair_specular_intensity2;
	StudioConVar hair_specular_noise2;
	StudioConVar hair_specular_smooth;
	StudioConVar hair_shadow_offset;
	StudioConVar hair_shadow_intensity;
}studio_celshade_control_t;

typedef struct studio_vbo_s
{
	studio_vbo_s()
	{
		studiohdr = NULL;
		hVBO = 0;
		hEBO = 0;
		hStudioUBO = 0;
		bExternalFileLoaded = false;
	}
	studiohdr_t	*		studiohdr;
	GLuint				hVBO;
	GLuint				hEBO;
	GLuint				hStudioUBO;
	std::vector<studio_vbo_submodel_t *> vSubmodel;
	studio_celshade_control_t celshade_control;
	bool bExternalFileLoaded;
}studio_vbo_t;

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
extern byte *texgammatable;
extern float *g_ChromeOrigin;
extern int *r_ambientlight;
extern float *r_shadelight;
extern vec3_t *r_blightvec;
extern float *r_plightvec;
extern float *r_colormix;
extern int *r_smodels_total;
extern int *r_amodels_drawn;

extern model_t *cl_sprite_white;
extern model_t *cl_shellchrome;

//renderer
extern int r_studio_drawcall;
extern int r_studio_polys;

studio_vbo_t *R_PrepareStudioVBO(studiohdr_t *studiohdr);
void R_StudioLoadExternalFile(model_t *mod, studiohdr_t *studiohdr, studio_vbo_t *VBOData);
void R_StudioReloadVBOCache(void);
void R_ShutdownStudio(void);
void R_InitStudio(void);
void R_SaveStudioProgramStates(void);
void R_LoadStudioProgramStates(void);
void R_GLStudioDrawPoints(void);
studiohdr_t *R_LoadTextures(model_t *psubm);
void studioapi_RestoreRenderer(void);
void studioapi_StudioDynamicLight(cl_entity_t *ent, alight_t *plight);
qboolean studioapi_StudioCheckBBox(void);
void __fastcall GameStudioRenderer_StudioRenderModel(void *pthis, int);
void __fastcall GameStudioRenderer_StudioRenderFinal(void *pthis, int);
void  R_StudioRenderModel(void);
void  R_StudioRenderFinal(void);

extern engine_studio_api_t IEngineStudio;
extern r_studio_interface_t **gpStudioInterface;

#define STUDIO_GBUFFER_ENABLED					0x10000
#define STUDIO_TRANSPARENT_ENABLED				0x20000
#define STUDIO_TRANSADDITIVE_ENABLED			0x40000
#define STUDIO_LINEAR_FOG_ENABLED				0x80000
#define STUDIO_EXP_FOG_ENABLED					0x100000
#define STUDIO_EXP2_FOG_ENABLED					0x200000
#define STUDIO_SHADOW_CASTER_ENABLED			0x400000
#define STUDIO_LEGACY_BONE_ENABLED				0x800000
#define STUDIO_GLOW_SHELL_ENABLED				0x1000000
#define STUDIO_OUTLINE_ENABLED					0x2000000
#define STUDIO_HAIR_SHADOW_ENABLED				0x4000000
#define STUDIO_CLIP_ENABLED						0x8000000
#define STUDIO_BINDLESS_ENABLED					0x10000000
#define STUDIO_OIT_ALPHA_BLEND_ENABLED			0x20000000
#define STUDIO_OIT_ADDITIVE_BLEND_ENABLED		0x40000000