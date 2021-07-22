#pragma once

typedef struct
{
	int program;
	int texoffset_high;
	int texoffset_medium;
	int texoffset_low;
	int texture_high;
	int texture_medium;
	int texture_low;
	int entitymatrix;
	int alpha;
	int fadefactor;
}shadow_program_t;

typedef struct
{
	int program;
	//shadow caster
	int entitypos;
}cshadow_program_t;

//renderer 
extern vec3_t shadow_light_mins;
extern vec3_t shadow_light_maxs;

extern int shadow_texture_depth;
extern int shadow_texture_high;
extern int shadow_texture_medium;
extern int shadow_texture_low;
extern int shadow_texture_size;

extern int shadow_numvisedicts_high;
extern int shadow_numvisedicts_medium;
extern int shadow_numvisedicts_low;

//cvar
extern cvar_t *r_shadow;
extern cvar_t *r_shadow_debug;
extern cvar_t *r_shadow_alpha;
extern cvar_t *r_shadow_fade_start;
extern cvar_t *r_shadow_fade_end;
extern cvar_t *r_shadow_angle_p;
extern cvar_t *r_shadow_angle_y;
extern cvar_t *r_shadow_angle_r;
extern cvar_t *r_shadow_high_distance;
extern cvar_t *r_shadow_medium_distance;
extern cvar_t *r_shadow_low_distance;
extern cvar_t *r_shadow_map_override;

void R_RenderShadowMap(void);
void R_InitShadow(void);
void R_FreeShadow(void);
void R_RecursiveWorldNodeShadow(mnode_t *node);
void R_RenderShadowScenes(void);
qboolean R_ShouldCastShadow(cl_entity_t *ent);
void R_UseShadowProgram(int state, shadow_program_t *progOutput);
void R_UseCastShadowProgram(int state, cshadow_program_t *progOutput);

#define SHADOW_HIGH_ENABLED			1
#define SHADOW_MEDIUM_ENABLED		2
#define SHADOW_LOW_ENABLED			4