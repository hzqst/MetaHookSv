#pragma once

#define MAX_SHADOW_MANAGERS 32
#define MAX_SHADOW_LIGHTS 256
#define MAX_SHADOW_TEXSIZE gl_max_texture_size

typedef struct sdlight_s
{
	cl_entity_t *followent;
	vec3_t origin;
	vec3_t angles;
	float radius;
	float fard;
	vec3_t vforward;
	vec3_t vright;
	vec3_t vup;
	int free;
	struct sdlight_s *next;
}shadowlight_t;

typedef struct
{
	char affectmodel[64];
	vec3_t angles;
	float radius;
	float fard;
	float scale;
	int texsize;
}shadow_manager_t;

typedef struct
{
	int program;
	int texoffset_high;
	int texoffset_medium;
	int texoffset_low;
	int depthmap_high;
	int depthmap_medium;
	int depthmap_low;
	int alpha;
}shadow_program_t;

//renderer 
extern qboolean drawshadowmap;
extern qboolean drawshadowscene;
extern vec3_t shadow_light_mins;
extern vec3_t shadow_light_maxs;

extern int shadow_depthmap_high;
extern int shadow_depthmap_high_texsize;

extern int shadow_depthmap_medium;
extern int shadow_depthmap_medium_texsize;

extern int shadow_depthmap_low;
extern int shadow_depthmap_low_texsize;

//shader
extern SHADER_DEFINE(shadow);

//cvar
extern cvar_t *r_shadow;
extern cvar_t *r_shadow_debug;
extern cvar_t *r_shadow_angle_p;
extern cvar_t *r_shadow_angle_y;
extern cvar_t *r_shadow_angle_r;
extern cvar_t *r_shadow_high_texsize;
extern cvar_t *r_shadow_high_distance;
extern cvar_t *r_shadow_medium_texsize;
extern cvar_t *r_shadow_medium_distance;
extern cvar_t *r_shadow_low_texsize;
extern cvar_t *r_shadow_low_distance;

void R_RenderShadowMap(void);
void R_InitShadow(void);
void R_ClearShadow(void);
void R_RecursiveWorldNodeShadow(mnode_t *node);
void R_RenderShadowScenes(void);
qboolean R_ShouldCastShadow(cl_entity_t *ent);