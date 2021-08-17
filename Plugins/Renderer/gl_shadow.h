#pragma once

extern int shadow_texture_depth;
extern int shadow_texture_color;
extern int shadow_texture_size;

extern float shadow_projmatrix[3][16];
extern float shadow_mvmatrix[3][16];

extern int shadow_numvisedicts[3];

//cvar
extern cvar_t *r_shadow;
extern cvar_t *r_shadow_debug;
extern cvar_t *r_shadow_distfade;
extern cvar_t *r_shadow_lumfade;
extern cvar_t *r_shadow_angles;
extern cvar_t *r_shadow_color;
extern cvar_t *r_shadow_intensity;
extern cvar_t *r_shadow_high_distance;
extern cvar_t *r_shadow_high_scale;
extern cvar_t *r_shadow_medium_distance;
extern cvar_t *r_shadow_medium_scale;
extern cvar_t *r_shadow_low_distance;
extern cvar_t *r_shadow_low_scale;

bool R_ShouldCastShadow(cl_entity_t *ent);
bool R_ShouldRenderShadowScene(int level);
void R_RenderShadowMap(void);
void R_InitShadow(void);
void R_FreeShadow(void);