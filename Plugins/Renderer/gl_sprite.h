#pragma once

#include <set>

typedef struct sprite_program_s
{
	int program;
	int width_height;
	int up_down_left_right;
	int in_color;
	int in_origin;
	int in_angles;
	int in_scale;
	int in_lerp;
}sprite_program_t;

typedef struct triapi_program_s
{
	int program;
}triapi_program_t;

extern int *particletexture;
extern particle_t **active_particles;
extern word **host_basepal;

extern cvar_t* r_sprite_lerping;

typedef struct sprite_vbo_s
{
	sprite_vbo_s()
	{
		flags = 0;
	}
	int flags;
}sprite_vbo_t;

void R_UseSpriteProgram(program_state_t state, sprite_program_t *progOutput);
void R_UseTriAPIProgram(program_state_t state, triapi_program_t* progOutput);
void R_InitSprite(void);
void R_ShutdownSprite(void);
void R_LoadSpriteProgramStates(void);
void R_SaveSpriteProgramStates(void);
void R_LoadTriAPIProgramStates(void);
void R_SaveTriAPIProgramStates(void);
void R_SpriteTextureAddReferences(model_t* mod, msprite_t* pSprite, std::set<int>& textures);
void R_SpriteLoadExternalFile(model_t* mod, msprite_t* pSprite, sprite_vbo_t* pSpriteVBOData);

#define SPRITE_GBUFFER_ENABLED				0x2ull
#define SPRITE_OIT_BLEND_ENABLED			0x4ull
#define SPRITE_GAMMA_BLEND_ENABLED			0x8ull
#define SPRITE_ALPHA_BLEND_ENABLED			0x10ull
#define SPRITE_ADDITIVE_BLEND_ENABLED		0x20ull
#define SPRITE_LINEAR_FOG_ENABLED			0x40ull
#define SPRITE_EXP_FOG_ENABLED				0x80ull
#define SPRITE_EXP2_FOG_ENABLED				0x100ull
#define SPRITE_CLIP_ENABLED					0x200ull
#define SPRITE_LERP_ENABLED					0x400ull
#define SPRITE_LINEAR_FOG_SHIFT_ENABLED		0x800ull
#define SPRITE_ALPHA_TEST_ENABLED			0x1000ull

#define SPRITE_PARALLEL_UPRIGHT_ENABLED		0x2000ull
#define SPRITE_FACING_UPRIGHT_ENABLED		0x4000ull
#define SPRITE_PARALLEL_ORIENTED_ENABLED	0x8000ull
#define SPRITE_PARALLEL_ENABLED				0x10000ull
#define SPRITE_ORIENTED_ENABLED				0x20000ull