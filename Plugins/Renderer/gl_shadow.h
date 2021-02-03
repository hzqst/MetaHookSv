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
	float scale;	
	int texsize;

	int depthmap;
	float projmatrix[16];
	float mvmatrix[16];
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
	int texoffset;
	int depthmap;
	int entorigin;
	int radius;
	int fard;
}shadow_program_t;

//renderer 
extern qboolean drawshadow;
extern qboolean drawshadowscene;
extern vec3_t shadow_light_mins;
extern vec3_t shadow_light_maxs;

//shadow
extern shadowlight_t *cursdlight;
extern shadowlight_t sdlights[MAX_SHADOW_LIGHTS];
extern shadowlight_t *sdlights_active;
extern shadowlight_t *sdlights_free;

//shadow manager
extern shadow_manager_t sdmanagers[MAX_SHADOW_MANAGERS];
extern shadow_manager_t sdmanager_player;
extern int numsdmanagers;

//shader
extern SHADER_DEFINE(shadow);
extern vec3_t shadow_light_mins;
extern vec3_t shadow_light_maxs;

//cvar
extern cvar_t *r_shadow;
extern cvar_t *r_shadow_debug;
extern cvar_t *r_shadow_angle_p;
extern cvar_t *r_shadow_angle_y;
extern cvar_t *r_shadow_angle_r;
extern cvar_t *r_shadow_radius;
extern cvar_t *r_shadow_fardist;
extern cvar_t *r_shadow_scale;
extern cvar_t *r_shadow_texsize;
	   
void R_RenderShadowMaps(void);
void R_InitShadow(void);
void R_ClearShadow(void);
void R_RecursiveWorldNodeShadow(mnode_t *node);
void R_RenderAllShadowScenes(void);
void R_AddEntityShadow(cl_entity_t *ent, const char *model);
qboolean R_ShouldCastShadow(cl_entity_t *ent);

//Shadow Light
shadowlight_t *R_FindShadowLight(cl_entity_t *entity);
void R_CreateShadowLight(cl_entity_t *entity, vec3_t ang, float radius, float fard, float scale, int texscale);