#pragma once

typedef struct sprite_program_s
{
	int program;
}sprite_program_t;

typedef struct legacysprite_program_s
{
	int program;
}legacysprite_program_t;

extern int r_sprite_drawcall;
extern int r_sprite_polys;

extern int *particletexture;
extern particle_t **active_particles;
extern word **host_basepal;

extern cvar_t* r_sprite_lerping;

void R_UseSpriteProgram(program_state_t state, sprite_program_t *progOutput);
void R_UseLegacySpriteProgram(program_state_t state, legacysprite_program_t *progOutput);
void R_InitSprite(void);
void R_ShutdownSprite(void);
void R_SaveLegacySpriteProgramStates(void);
void R_LoadLegacySpriteProgramStates(void);
void R_LoadSpriteProgramStates(void);
void R_SaveSpriteProgramStates(void);

#define SPRITE_BINDLESS_ENABLED				0x1ull
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

#define SPRITE_PARALLEL_UPRIGHT_ENABLED		0x1000ull
#define SPRITE_FACING_UPRIGHT_ENABLED		0x2000ull
#define SPRITE_PARALLEL_ORIENTED_ENABLED	0x4000ull
#define SPRITE_PARALLEL_ENABLED				0x8000ull
#define SPRITE_ORIENTED_ENABLED				0x10000ull

#define MAX_SPRITE_FRAMES 4096
#define MAX_SPRITE_ENTRIES 512