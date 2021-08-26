#pragma once

typedef struct sprite_program_s
{
	int program;
}sprite_program_t;

typedef struct legacysprite_program_s
{
	int program;
}legacysprite_program_t;

#pragma pack(push, 16)

typedef struct spriteframe_ssbo_t
{
	ivec4 type_width_height_texturenum;
	vec4 up_down_left_right;

	GLuint64 texturehandle[2];
}spriteframe_ssbo_t;

#pragma pack(pop)

#pragma pack(push, 16)

typedef struct spriteentry_ssbo_s
{
	vec4 color;
	vec4 origin;//origin[3] = scale
	vec4 angles;
	ivec4 frameindex;
}spriteentry_ssbo_t;

#pragma pack(pop)

extern int g_iNumSpriteEntries[kRenderTransAdd + 1];

extern int r_sprite_drawcall;
extern int r_sprite_polys;

extern int *particletexture;
extern particle_t **active_particles;
extern word **host_basepal;

void R_UseSpriteProgram(int state, sprite_program_t *progOutput);
void R_UseLegacySpriteProgram(int state, legacysprite_program_t *progOutput);
//void R_DrawSpriteEntris(int kRenderMode);
void R_BlitOITBlendBuffer(void);
void R_ReloadSpriteFrameCache(void);

#define SPRITE_BINDLESS_ENABLED				1
#define SPRITE_GBUFFER_ENABLED				2
#define SPRITE_OIT_ALPHA_BLEND_ENABLED		4
#define SPRITE_OIT_ADDITIVE_BLEND_ENABLED	8
#define SPRITE_PARALLEL_UPRIGHT_ENABLED		0x10
#define SPRITE_FACING_UPRIGHT_ENABLED		0x20
#define SPRITE_PARALLEL_ORIENTED_ENABLED	0x40
#define SPRITE_PARALLEL_ENABLED				0x80
#define SPRITE_ORIENTED_ENABLED				0x100

#define MAX_SPRITE_FRAMES 4096
#define MAX_SPRITE_ENTRIES 512