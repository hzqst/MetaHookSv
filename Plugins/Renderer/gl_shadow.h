#pragma once

typedef struct
{
	int program;
	int texoffset_high;
	int texoffset_medium;
	int texoffset_low;
	int texmatrix_high;
	int texmatrix_medium;
	int texmatrix_low;
	int texture_array;
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

extern int shadow_texture_depth;
extern int shadow_texture_color;
extern int shadow_texture_size;

extern float shadow_projmatrix[3][16];
extern float shadow_mvmatrix[3][16];

extern int shadow_numvisedicts[3];

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
qboolean R_ShouldCastShadow(cl_entity_t *ent);
void R_UseCastShadowProgram(int state, cshadow_program_t *progOutput);

#define SHADOW_HIGH_ENABLED			1
#define SHADOW_MEDIUM_ENABLED		2
#define SHADOW_LOW_ENABLED			4