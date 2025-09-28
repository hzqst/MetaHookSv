#pragma once

typedef struct shadow_texture_s
{
	GLuint depth_stencil{};

	size_t size{};

	uint32_t viewport[4]{};

	mat4 worldmatrix;
	mat4 projmatrix;
	mat4 shadowmatrix;

	bool ready{};
	bool csm{};
	bool isStatic{};
}shadow_texture_t;

extern shadow_texture_t cl_dlight_shadow_textures[MAX_DLIGHTS_SVENGINE];

extern shadow_texture_t *current_shadow_texture;

//cvar
extern cvar_t *r_shadow;

bool R_ShouldCastShadow(cl_entity_t *ent);
bool R_ShouldRenderShadow(void);
void R_RenderShadowMap(void); 
void R_InitShadow(void);
void R_ShutdownShadow(void);
void R_AllocShadowTexture(shadow_texture_t *shadowtex, int size);
void R_FreeShadowTexture(shadow_texture_t *shadowtex);