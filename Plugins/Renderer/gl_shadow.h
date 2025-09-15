#pragma once

typedef struct shadow_texture_s
{
	GLuint depth_stencil;
	size_t size;
	mat4 matrix;
	float distance;
	float cone_angle;
	bool ready;
}shadow_texture_t;

extern shadow_texture_t cl_dlight_shadow_textures[MAX_DLIGHTS_SVENGINE];

extern shadow_texture_t *current_shadow_texture;

//cvar
extern cvar_t *r_shadow;

extern MapConVar *r_shadow_distfade;
extern MapConVar *r_shadow_lumfade;
extern MapConVar *r_shadow_angles;
extern MapConVar *r_shadow_color;
extern MapConVar *r_shadow_intensity;
extern MapConVar *r_shadow_high_distance;
extern MapConVar *r_shadow_high_scale;
extern MapConVar *r_shadow_medium_distance;
extern MapConVar *r_shadow_medium_scale;
extern MapConVar *r_shadow_low_distance;
extern MapConVar *r_shadow_low_scale;

bool R_ShouldCastShadow(cl_entity_t *ent);
bool R_ShouldRenderShadow(void);
void R_RenderShadowMap(void); 
void R_InitShadow(void);
void R_ShutdownShadow(void);
void R_AllocShadowTexture(shadow_texture_t *shadowtex, int size, bool bUseColorArrayAsDepth);
void R_FreeShadowTexture(shadow_texture_t *shadowtex);