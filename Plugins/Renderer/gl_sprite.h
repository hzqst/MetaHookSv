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

void R_UseSpriteProgram(int state, sprite_program_t *progOutput);
void R_UseLegacySpriteProgram(int state, legacysprite_program_t *progOutput);
void R_InitSprite(void);
void R_ShutdownSprite(void);
void R_SaveLegacySpriteProgramStates(void);
void R_LoadLegacySpriteProgramStates(void);
void R_LoadSpriteProgramStates(void);
void R_SaveSpriteProgramStates(void);

#define SPRITE_BINDLESS_ENABLED				1
#define SPRITE_GBUFFER_ENABLED				2
#define SPRITE_OIT_ALPHA_BLEND_ENABLED		4
#define SPRITE_OIT_ADDITIVE_BLEND_ENABLED	8
#define SPRITE_ALPHA_BLEND_ENABLED			0x10
#define SPRITE_ADDITIVE_BLEND_ENABLED		0x20
#define SPRITE_LINEAR_FOG_ENABLED			0x40
#define SPRITE_EXP_FOG_ENABLED				0x80
#define SPRITE_EXP2_FOG_ENABLED				0x100
#define SPRITE_CLIP_ENABLED					0x200

#define SPRITE_PARALLEL_UPRIGHT_ENABLED		0x1000
#define SPRITE_FACING_UPRIGHT_ENABLED		0x2000
#define SPRITE_PARALLEL_ORIENTED_ENABLED	0x4000
#define SPRITE_PARALLEL_ENABLED				0x8000
#define SPRITE_ORIENTED_ENABLED				0x10000

#define MAX_SPRITE_FRAMES 4096
#define MAX_SPRITE_ENTRIES 512