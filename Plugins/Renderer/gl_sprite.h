#pragma once

#include "gl_model.h"

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

#define SPRITE_REPLACE_TEXTURE		0
#define SPRITE_MAX_TEXTURE			1

class CSpriteModelRenderMaterial
{
public:
	std::string basetexture;
	CGameModelRenderTexture textures[SPRITE_MAX_TEXTURE];
};

class CSpriteModelRenderData
{
public:
	CSpriteModelRenderData(model_t* mod) : model(mod)
	{

	}

	int flags{};
	model_t* model{};
	std::unordered_map<uint32_t, std::shared_ptr<CSpriteModelRenderMaterial>> mSpriteMaterials;
};

void R_UseSpriteProgram(program_state_t state, sprite_program_t *progOutput);
void R_UseTriAPIProgram(program_state_t state, triapi_program_t* progOutput);
void R_InitSprite(void);
void R_ShutdownSprite(void);
void R_LoadSpriteProgramStates(void);
void R_SaveSpriteProgramStates(void);
void R_LoadTriAPIProgramStates(void);
void R_SaveTriAPIProgramStates(void);
void R_SpriteLoadExternalFile(model_t* mod, msprite_t* pSprite, CSpriteModelRenderData* pSpriteRenderData);
std::shared_ptr<CSpriteModelRenderData> R_GetSpriteRenderDataFromModel(model_t* mod);
std::shared_ptr<CSpriteModelRenderData> R_CreateSpriteRenderData(model_t* mod);
void R_FreeSpriteRenderData(model_t* mod);
void R_FreeSpriteRenderData(const std::shared_ptr<CSpriteModelRenderData>& pRenderData);
std::shared_ptr<CSpriteModelRenderMaterial> R_SpriteCreateMaterial(CSpriteModelRenderData* pRenderData, mspriteframe_t* pSpriteFrame);
std::shared_ptr<CSpriteModelRenderMaterial> R_SpriteGetMaterial(CSpriteModelRenderData* pRenderData, mspriteframe_t* pSpriteFrame);

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