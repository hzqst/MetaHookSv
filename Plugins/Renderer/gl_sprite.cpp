#include "gl_local.h"
#include <sstream>
#include <algorithm>

static std::vector<std::shared_ptr<CSpriteModelRenderData>> g_SpriteRenderDataCache;

std::unordered_map<program_state_t, sprite_program_t> g_SpriteProgramTable;
std::unordered_map<program_state_t, triapi_program_t> g_TriAPIProgramTable;

int *particletexture = NULL;
particle_t **active_particles = NULL;
word **host_basepal = NULL;

void R_UseSpriteProgram(program_state_t state, sprite_program_t *progOutput)
{
	sprite_program_t prog = { 0 };

	auto itor = g_SpriteProgramTable.find(state);
	if (itor == g_SpriteProgramTable.end())
	{
		std::stringstream defs;

		if(state & SPRITE_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if ((state & SPRITE_OIT_BLEND_ENABLED) && g_bUseOITBlend)
			defs << "#define OIT_BLEND_ENABLED\n";

		if (state & SPRITE_GAMMA_BLEND_ENABLED)
			defs << "#define GAMMA_BLEND_ENABLED\n";

		if (state & SPRITE_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & SPRITE_ADDITIVE_BLEND_ENABLED)
			defs << "#define ADDITIVE_BLEND_ENABLED\n";

		if (state & SPRITE_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & SPRITE_EXP_FOG_ENABLED)
			defs << "#define EXP_FOG_ENABLED\n";

		if (state & SPRITE_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

		if (state & SPRITE_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		if (state & SPRITE_LERP_ENABLED)
			defs << "#define LERP_ENABLED\n";

		if (state & SPRITE_LINEAR_FOG_SHIFT_ENABLED)
			defs << "#define LINEAR_FOG_SHIFT_ENABLED\n";

		if (state & SPRITE_ALPHA_TEST_ENABLED)
			defs << "#define ALPHA_TEST_ENABLED\n";

		//...

		if (state & SPRITE_PARALLEL_UPRIGHT_ENABLED)
			defs << "#define PARALLEL_UPRIGHT_ENABLED\n";

		if (state & SPRITE_FACING_UPRIGHT_ENABLED)
			defs << "#define FACING_UPRIGHT_ENABLED\n";

		if (state & SPRITE_PARALLEL_ORIENTED_ENABLED)
			defs << "#define PARALLEL_ORIENTED_ENABLED\n";

		if (state & SPRITE_PARALLEL_ENABLED)
			defs << "#define PARALLEL_ENABLED\n";

		if (state & SPRITE_ORIENTED_ENABLED)
			defs << "#define ORIENTED_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFile("renderer\\shader\\sprite_shader.vert.glsl", "renderer\\shader\\sprite_shader.frag.glsl", def.c_str(), def.c_str());
		if (prog.program)
		{
			SHADER_UNIFORM(prog, in_up_down_left_right, "in_up_down_left_right");
			SHADER_UNIFORM(prog, in_color, "in_color");
			SHADER_UNIFORM(prog, in_origin, "in_origin");
			SHADER_UNIFORM(prog, in_angles, "in_angles");
			SHADER_UNIFORM(prog, in_scale, "in_scale");
			SHADER_UNIFORM(prog, in_lerp, "in_lerp");
		}

		g_SpriteProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_Error("R_UseSpriteProgram: Failed to load program!");
	}
}

const program_state_mapping_t s_SpriteProgramStateName[] = {
{ SPRITE_GBUFFER_ENABLED			  ,"SPRITE_GBUFFER_ENABLED"				},
{ SPRITE_OIT_BLEND_ENABLED			  ,"SPRITE_OIT_BLEND_ENABLED"			},
{ SPRITE_GAMMA_BLEND_ENABLED		  ,"SPRITE_GAMMA_BLEND_ENABLED"			},
{ SPRITE_ALPHA_BLEND_ENABLED		  ,"SPRITE_ALPHA_BLEND_ENABLED"			},
{ SPRITE_ADDITIVE_BLEND_ENABLED		  ,"SPRITE_ADDITIVE_BLEND_ENABLED"		},
{ SPRITE_LINEAR_FOG_ENABLED			  ,"SPRITE_LINEAR_FOG_ENABLED"			},
{ SPRITE_EXP_FOG_ENABLED			  ,"SPRITE_EXP_FOG_ENABLED"				},
{ SPRITE_EXP2_FOG_ENABLED			  ,"SPRITE_EXP2_FOG_ENABLED"			},
{ SPRITE_CLIP_ENABLED				  ,"SPRITE_CLIP_ENABLED"				},
{ SPRITE_LERP_ENABLED				  ,"SPRITE_LERP_ENABLED"				},
{ SPRITE_LINEAR_FOG_SHIFT_ENABLED		,"SPRITE_LINEAR_FOG_SHIFT_ENABLED"	},
{ SPRITE_ALPHA_TEST_ENABLED				,"SPRITE_ALPHA_TEST_ENABLED"		},

{ SPRITE_PARALLEL_UPRIGHT_ENABLED	  ,"SPRITE_PARALLEL_UPRIGHT_ENABLED"	},
{ SPRITE_FACING_UPRIGHT_ENABLED		  ,"SPRITE_FACING_UPRIGHT_ENABLED"		},
{ SPRITE_PARALLEL_ORIENTED_ENABLED	  ,"SPRITE_PARALLEL_ORIENTED_ENABLED"	},
{ SPRITE_PARALLEL_ENABLED			  ,"SPRITE_PARALLEL_ENABLED"			},
{ SPRITE_ORIENTED_ENABLED			  ,"SPRITE_ORIENTED_ENABLED"			},
};

void R_SaveSpriteProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto &p : g_SpriteProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/sprite_cache.txt", states, s_SpriteProgramStateName, _ARRAYSIZE(s_SpriteProgramStateName));
}

void R_LoadSpriteProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/sprite_cache.txt", s_SpriteProgramStateName, _ARRAYSIZE(s_SpriteProgramStateName), [](program_state_t state) {

		R_UseSpriteProgram(state, NULL);

	});
}

void R_UseTriAPIProgram(program_state_t state, triapi_program_t* progOutput)
{
	triapi_program_t prog = { 0 };

	auto itor = g_TriAPIProgramTable.find(state);
	if (itor == g_TriAPIProgramTable.end())
	{
		std::stringstream defs;

		if (state & SPRITE_OIT_BLEND_ENABLED)
			defs << "#define OIT_BLEND_ENABLED\n";

		if (state & SPRITE_GAMMA_BLEND_ENABLED)
			defs << "#define GAMMA_BLEND_ENABLED\n";

		if (state & SPRITE_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & SPRITE_ADDITIVE_BLEND_ENABLED)
			defs << "#define ADDITIVE_BLEND_ENABLED\n";

		if (state & SPRITE_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & SPRITE_EXP_FOG_ENABLED)
			defs << "#define EXP_FOG_ENABLED\n";

		if (state & SPRITE_EXP2_FOG_ENABLED)
			defs << "#define EXP2_FOG_ENABLED\n";

		if (state & SPRITE_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		if (state & SPRITE_LERP_ENABLED)
			defs << "#define LERP_ENABLED\n";

		if (state & SPRITE_LINEAR_FOG_SHIFT_ENABLED)
			defs << "#define LINEAR_FOG_SHIFT_ENABLED\n";

		if (state & SPRITE_ALPHA_TEST_ENABLED)
			defs << "#define ALPHA_TEST_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFile("renderer\\shader\\triapi_shader.vert.glsl", "renderer\\shader\\triapi_shader.frag.glsl", def.c_str(), def.c_str());

		if (prog.program)
		{

		}

		g_TriAPIProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_Error("R_UseTriAPIProgram: Failed to load program!");
	}
}

void R_SaveTriAPIProgramStates(void)
{
	std::vector<program_state_t> states;

	for (auto& p : g_TriAPIProgramTable)
	{
		states.emplace_back(p.first);
	}

	R_SaveProgramStatesCaches("renderer/shader/triapi_cache.txt", states, s_SpriteProgramStateName, _ARRAYSIZE(s_SpriteProgramStateName));
}

void R_LoadTriAPIProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/triapi_cache.txt", s_SpriteProgramStateName, _ARRAYSIZE(s_SpriteProgramStateName), [](program_state_t state) {

		R_UseTriAPIProgram(state, NULL);

	});
}

void R_InitSprite(void)
{

}

void R_ShutdownSprite(void)
{

}

void R_SpriteColor(colorVec *pColor, cl_entity_t *pEntity, int alpha)
{
	int a;

	if (pEntity->curstate.rendermode == kRenderGlow || pEntity->curstate.rendermode == kRenderTransAdd)
	{
		a = math_clamp(alpha, 0, 255);//some entities > 255 wtf?
	}
	else
	{
		a = 256;
	}

	if (pEntity->curstate.rendercolor.r != 0 || pEntity->curstate.rendercolor.g != 0 || pEntity->curstate.rendercolor.b != 0)
	{
		pColor->r = (pEntity->curstate.rendercolor.r * a) >> 8;
		pColor->g = (pEntity->curstate.rendercolor.g * a) >> 8;
		pColor->b = (pEntity->curstate.rendercolor.b * a) >> 8;
	}
	else
	{
		pColor->r = (255 * a) >> 8;
		pColor->g = (255 * a) >> 8;
		pColor->b = (255 * a) >> 8;
	}
}

int R_GetSpriteFrameIndex(msprite_t* pSprite, mspriteframe_t *pSpriteFrame)
{
	if (!pSprite)
	{
		gEngfuncs.Con_DPrintf("R_GetSpriteFrameIndex: pSprite is NULL!\n");
		return NULL;
	}

	if (!pSprite->numframes)
	{
		gEngfuncs.Con_DPrintf("R_GetSpriteFrameIndex: pSprite has no frames!!!\n");
		return NULL;
	}

	for (int i = 0; i < pSprite->numframes; ++i)
	{
		if (pSprite->frames[i].type == SPR_SINGLE && pSprite->frames[i].frameptr == pSpriteFrame)
		{
			return i;
		}
	}

	return -1;
}

mspriteframe_t* R_GetSpriteFrame(msprite_t* pSprite, int frame)
{
	mspriteframe_t* pspriteframe;

	if (!pSprite)
	{
		gEngfuncs.Con_DPrintf("R_GetSpriteFrame: pSprite is NULL!\n");
		return NULL;
	}

	if (!pSprite->numframes)
	{
		gEngfuncs.Con_DPrintf("R_GetSpriteFrame: pSprite has no frames!!!\n");
		return NULL;
	}

	if ((frame >= pSprite->numframes) || (frame < 0))
	{
		gEngfuncs.Con_DPrintf("R_GetSpriteFrame: no such frame %d\n", frame);
		frame = 0;
	}

	if (pSprite->frames[frame].type == SPR_SINGLE)
	{
		//TODO: fake our own mspriteframe_t

		pspriteframe = pSprite->frames[frame].frameptr;
	}
	else
	{
		pspriteframe = NULL;
	}

	return pspriteframe;
}

//Credits to https://github.com/FWGS/xashxt-fwgs/blob/dee61d6cf0a8f681c322e863f8df1d5e6f22443e/client/render/r_sprite.cpp#L126
//And https://github.com/FWGS/xash3d-fwgs/blob/33da68b013fd9a2c683316758d751308d1a98109/ref/gl/gl_sprite.c#L462
void R_GetSpriteFrameInterpolant(cl_entity_t* ent, msprite_t* pSprite, int* curFrameIndex, mspriteframe_t** curframe, int* oldFrameIndex, mspriteframe_t** oldframe, float *lerp)
{
	auto frame = (int)ent->curstate.frame;
	auto lerpFrac = 1.0f;

	// misc info
	auto fDoInterp = (ent->curstate.effects & EF_NOINTERP) ? false : true;

	if (frame < 0)
	{
		frame = 0;
	}
	else if (frame >= pSprite->numframes)
	{
		frame = pSprite->numframes - 1;
	}

	if (pSprite->frames[frame].type == SPR_SINGLE)
	{
		if (fDoInterp)
		{
			if (ent->latched.prevseqblending[0] >= pSprite->numframes || pSprite->frames[ent->latched.prevseqblending[0]].type != SPR_SINGLE)
			{
				// this can be happens when rendering switched between single and angled frames
				// or change model on replace delta-entity
				ent->latched.prevseqblending[0] = ent->latched.prevseqblending[1] = frame;
				ent->latched.sequencetime = (*cl_time);
				lerpFrac = 1.0f;
			}

			if (ent->latched.sequencetime < (*cl_time))
			{
				if (frame != ent->latched.prevseqblending[1])
				{
					ent->latched.prevseqblending[0] = ent->latched.prevseqblending[1];
					ent->latched.prevseqblending[1] = frame;
					ent->latched.sequencetime = (*cl_time);
					lerpFrac = 0.0f;
				}
				else
				{
					lerpFrac = ((*cl_time) - ent->latched.sequencetime) * 11.0f;
				}
			}
			else
			{
				ent->latched.prevseqblending[0] = ent->latched.prevseqblending[1] = frame;
				ent->latched.sequencetime = (*cl_time);
				lerpFrac = 0.0f;
			}
		}
		else
		{
			ent->latched.prevseqblending[0] = ent->latched.prevseqblending[1] = frame;
			lerpFrac = 1.0f;
		}

		if (ent->latched.prevseqblending[0] >= pSprite->numframes)
		{
			// reset interpolation on change model
			ent->latched.prevseqblending[0] = ent->latched.prevseqblending[1] = frame;
			ent->latched.sequencetime = (*cl_time);
			lerpFrac = 0.0f;
		}

		// get the interpolated frames
		if (oldFrameIndex) (*oldFrameIndex) = ent->latched.prevseqblending[0];
		if (oldframe) (*oldframe) = R_GetSpriteFrame(pSprite, ent->latched.prevseqblending[0]);

		if (curFrameIndex) (*curFrameIndex) = frame;
		if (curframe) (*curframe) = R_GetSpriteFrame(pSprite, frame);
	}

	*lerp = lerpFrac;
}

bool R_SpriteAllowLerping(cl_entity_t* ent, msprite_t* pSprite)
{
	if (!r_sprite_lerping->value)
		return false;

	if (pSprite->numframes <= 1)
		return false;

	if (pSprite->texFormat != SPR_ADDITIVE && pSprite->texFormat != SPR_INDEXALPHA)
		return false;

	if (ent->curstate.rendermode == kRenderNormal || ent->curstate.rendermode == kRenderTransAlpha)
		return false;

	return true;
}

std::shared_ptr<CSpriteModelRenderMaterial> R_SpriteCreateMaterial(CSpriteModelRenderData* pRenderData, int frameIndex)
{
	if (!pRenderData->vSpriteMaterials[frameIndex]) {
		pRenderData->vSpriteMaterials[frameIndex] = std::make_shared<CSpriteModelRenderMaterial>();
	}

	return pRenderData->vSpriteMaterials[frameIndex];
}

std::shared_ptr<CSpriteModelRenderMaterial> R_SpriteGetMaterial(CSpriteModelRenderData* pRenderData, int frameIndex)
{
	return pRenderData->vSpriteMaterials[frameIndex];
}

void R_DrawSpriteModelBindFrameTexture(CSpriteModelRenderData* pRenderData, int frameIndex, mspriteframe_t* frame, int bindSlot)
{
	auto pRenderMaterial = R_SpriteGetMaterial(pRenderData, frameIndex);

	if (pRenderMaterial && pRenderMaterial->textures[SPRITE_REPLACE_TEXTURE].gltexturenum)
	{
		GL_BindTextureUnit(bindSlot, GL_TEXTURE_2D, pRenderMaterial->textures[SPRITE_REPLACE_TEXTURE].gltexturenum);
	}
	else
	{
		GL_BindTextureUnit(bindSlot, GL_TEXTURE_2D, frame->gl_texturenum);
	}
}

void R_DrawSpriteModelInterpFrames(cl_entity_t* ent, CSpriteModelRenderData *pRenderData, msprite_t* pSprite, int frameIndex, mspriteframe_t* frame, int oldFrameIndex, mspriteframe_t* oldframe, float lerp)
{
	GL_BeginDebugGroupFormat("R_DrawSpriteModelInterpFrames - %s", ent->model ? ent->model->name : "<empty>");

	program_state_t SpriteProgramState = 0;

	auto scale = ent->curstate.scale;

	if (scale <= 0)
		scale = 1;

	if (ent->curstate.rendermode == kRenderNormal)
		(*r_blend) = 1.0;

	colorVec color = { 0 };
	R_SpriteColor(&color, ent, (*r_blend) * 255);

	float u_color[4] = { 0 };

	if (!gl_spriteblend->value && ent->curstate.rendermode == kRenderNormal)
	{
		glDisable(GL_BLEND);

		u_color[0] = color.r / 255.0f;
		u_color[1] = color.g / 255.0f;
		u_color[2] = color.b / 255.0f;
		u_color[3] = 1;
	}
	else
	{
		switch (ent->curstate.rendermode)
		{
		case kRenderTransColor:
		{
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			SpriteProgramState |= SPRITE_ALPHA_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = (*r_blend);
			break;
		}

		case kRenderTransAdd:
		{
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			R_SetGBufferBlend(GL_ONE, GL_ONE);

			SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = 1;
			break;
		}

		case kRenderGlow:
		{
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			R_SetGBufferBlend(GL_ONE, GL_ONE);

			SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = 1;
			break;
		}

		case kRenderTransAlpha:
		{
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			SpriteProgramState |= SPRITE_ALPHA_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = (*r_blend);
			
			break;
		}

		default:
		{
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			SpriteProgramState |= SPRITE_ALPHA_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = (*r_blend);
			break;
		}
		}
	}

	SpriteProgramState |= SPRITE_ALPHA_TEST_ENABLED;

	if (R_IsRenderingWaterView())
	{
		SpriteProgramState |= SPRITE_CLIP_ENABLED;
	}

	if (R_IsRenderingGBuffer())
	{
		SpriteProgramState |= SPRITE_GBUFFER_ENABLED;
	}

	if (!R_IsRenderingGBuffer())
	{
		if ((SpriteProgramState & SPRITE_ADDITIVE_BLEND_ENABLED) && (int)r_fog_trans->value <= 1)
		{

		}
		else if ((SpriteProgramState & SPRITE_ALPHA_BLEND_ENABLED) && (int)r_fog_trans->value <= 0)
		{

		}
		else if (R_IsRenderingFog())
		{
			if (r_fog_mode == GL_LINEAR)
			{
				SpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
			}
			else if (r_fog_mode == GL_EXP)
			{
				SpriteProgramState |= SPRITE_EXP_FOG_ENABLED;
			}
			else if (r_fog_mode == GL_EXP2)
			{
				SpriteProgramState |= SPRITE_EXP2_FOG_ENABLED;
			}

			if (!R_IsRenderingGammaBlending() && r_linear_fog_shift->value > 0)
			{
				SpriteProgramState |= SPRITE_LINEAR_FOG_SHIFT_ENABLED;
			}
		}
	}

	if (R_IsRenderingGammaBlending())
	{
		SpriteProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
	}

	if (r_draw_oitblend && (SpriteProgramState & (SPRITE_ALPHA_BLEND_ENABLED | SPRITE_ADDITIVE_BLEND_ENABLED)))
	{
		SpriteProgramState |= SPRITE_OIT_BLEND_ENABLED;
	}

	int type = pSprite->type;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		if (ent->curstate.effects & EF_SPRITE_CUSTOM_VP)
		{
			type = math_clamp(ent->curstate.sequence, SPR_VP_PARALLEL_UPRIGHT, SPR_VP_PARALLEL_ORIENTED);
		}
	}

	if (ent->angles[2] != 0 && type == SPR_VP_PARALLEL)
	{
		type = SPR_VP_PARALLEL_ORIENTED;
	}

	switch (type)
	{
	case SPR_FACING_UPRIGHT:
	{
		SpriteProgramState |= SPRITE_FACING_UPRIGHT_ENABLED;
		break;
	}

	case SPR_VP_PARALLEL:
	{
		SpriteProgramState |= SPRITE_PARALLEL_ENABLED;
		break;
	}

	case SPR_VP_PARALLEL_UPRIGHT:
	{
		SpriteProgramState |= SPRITE_PARALLEL_UPRIGHT_ENABLED;
		break;
	}

	case SPR_ORIENTED:
	{
		SpriteProgramState |= SPRITE_ORIENTED_ENABLED;
		break;
	}

	case SPR_VP_PARALLEL_ORIENTED:
	{
		SpriteProgramState |= SPRITE_PARALLEL_ORIENTED_ENABLED;
		break;
	}
	}

	if (frame != oldframe)
	{
		SpriteProgramState |= SPRITE_LERP_ENABLED;
	}

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	if (r_draw_opaque)
	{
		int iStencilRef = 0;

		if ((pRenderData->flags & FMODEL_NOBLOOM))
		{
			iStencilRef |= STENCIL_MASK_NO_BLOOM;
		}

		//has stencil write-in
		GL_BeginStencilWrite(iStencilRef, STENCIL_MASK_ALL);
	}
	else
	{
		int iStencilRef = 0;

		if ((pRenderData->flags & FMODEL_NOBLOOM))
		{
			iStencilRef |= STENCIL_MASK_NO_BLOOM;
		}

		//has stencil write-in
		GL_BeginStencilWrite(iStencilRef, STENCIL_MASK_NO_BLOOM);
	}

	sprite_program_t prog = { 0 };
	R_UseSpriteProgram(SpriteProgramState, &prog);

	if (prog.in_up_down_left_right != -1)
		glUniform4f(prog.in_up_down_left_right, frame->up, frame->down, frame->left, frame->right);

	if (prog.in_origin != -1)
		glUniform4f(prog.in_color, u_color[0], u_color[1], u_color[2], u_color[3]);

	if (prog.in_origin != -1)
		glUniform3f(prog.in_origin, r_entorigin[0], r_entorigin[1], r_entorigin[2]);

	if (prog.in_angles != -1)
		glUniform3f(prog.in_angles, ent->angles[0], ent->angles[1], ent->angles[2]);
	
	if(prog.in_scale != -1)
		glUniform1f(prog.in_scale, scale);

	if (prog.in_lerp != -1)
		glUniform1f(prog.in_lerp, math_clamp(lerp, 0.0f, 1.0f));

	const uint32_t indices [] = {0,1,2,2,3,0};

	GL_BindVAO(r_empty_vao);

	R_DrawSpriteModelBindFrameTexture(pRenderData, frameIndex, frame, 0);

	if (SpriteProgramState & SPRITE_LERP_ENABLED)
	{
		R_DrawSpriteModelBindFrameTexture(pRenderData, oldFrameIndex, oldframe, 1);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

		GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);
	}
	else
	{
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
	}

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_BindVAO(0);

	GL_UseProgram(0);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	GL_EndStencil();

	GL_EndDebugGroup();
}

void R_DrawSpriteModel(cl_entity_t *ent)
{
	auto pSprite = (msprite_t *)ent->model->cache.data;

	if (!pSprite)
		return;

	auto pSpriteRenderData = R_GetSpriteRenderDataFromModel(ent->model);

	if (!pSpriteRenderData)
		return;

	float lerp = 0;

	int frameIndex = 0;
	mspriteframe_t* frame = NULL;

	int oldFrameIndex = 0;
	mspriteframe_t* oldframe = NULL;

	if (R_SpriteAllowLerping(ent, pSprite))
	{
		R_GetSpriteFrameInterpolant(ent, pSprite, &frameIndex, &frame, &oldFrameIndex, &oldframe, &lerp);
	}
	else
	{
		oldFrameIndex = frameIndex = (int)ent->curstate.frame;
		oldframe = frame = R_GetSpriteFrame(pSprite, frameIndex);
	}

	if (!frame)
	{
		gEngfuncs.Con_DPrintf("R_DrawSpriteModel: Couldn't get sprite frame for %s\n", ent->model);
		return;
	}

	R_DrawSpriteModelInterpFrames(ent, pSpriteRenderData.get(), pSprite, frameIndex, frame, oldFrameIndex, oldframe, lerp);
}

void R_SpriteLoadExternalFile_Efx(bspentity_t* ent, msprite_t* pSprite, CSpriteModelRenderData* pRenderData)
{
	auto flags_string = ValueForKey(ent, "flags");

#define REGISTER_EFX_FLAGS_KEY_VALUE(name) if (flags_string && !strcmp(flags_string, #name))\
	{\
		pRenderData->flags |= name; \
	}\
	if (flags_string && !strcmp(flags_string, "-" #name))\
	{\
		pRenderData->flags &= ~name; \
	}

	REGISTER_EFX_FLAGS_KEY_VALUE(FMODEL_NOBLOOM);

#undef REGISTER_EFX_FLAGS_KEY_VALUE
}

void R_SpriteLoadExternalFile_FrameTextureLoad(bspentity_t* ent, msprite_t* pSprite, CSpriteModelRenderData* pRenderData, int frameIndex, mspriteframe_t* pSpriteFrame, const char* textureValue, const char* scaleValue, int spriteTextureType)
{
	if (textureValue && textureValue[0])
	{
		auto pSpriteMaterial = R_SpriteCreateMaterial(pRenderData, frameIndex);

		if (pSpriteMaterial)
		{
			bool bLoaded = false;
			gl_loadtexture_result_t loadResult;
			std::string texturePath;

			bool bHasMipmaps = (*gSpriteMipMap) ? true : false;

			//Texture name starts with "models\\" or "models/"
			if (!bLoaded &&
				!strnicmp(textureValue, "sprites", sizeof("sprites") - 1) &&
				(textureValue[sizeof("sprites") - 1] == '\\' || textureValue[sizeof("sprites") - 1] == '/'))
			{
				texturePath = textureValue;
				if (!V_GetFileExtension(textureValue))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_SPRITE, bHasMipmaps, &loadResult);
			}

			if (!bLoaded)
			{
				texturePath = "gfx/";
				texturePath += textureValue;

				if (!V_GetFileExtension(textureValue))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_SPRITE, bHasMipmaps, &loadResult);
			}

			if (!bLoaded)
			{
				texturePath = "renderer/texture/";
				texturePath += textureValue;

				if (!V_GetFileExtension(textureValue))
					texturePath += ".tga";

				bLoaded = R_LoadTextureFromFile(texturePath.c_str(), texturePath.c_str(), GLT_SPRITE, bHasMipmaps, &loadResult);
			}

			if (!bLoaded)
			{
				gEngfuncs.Con_Printf("R_SpriteLoadExternalFile_TextureLoad: Failed to load external texture \"%s\".\n", textureValue);
				return;
			}

			pSpriteMaterial->textures[spriteTextureType].gltexturenum = loadResult.gltexturenum;
			pSpriteMaterial->textures[spriteTextureType].numframes = loadResult.numframes;
			pSpriteMaterial->textures[spriteTextureType].framerate = GetFrameRateFromFrameDuration(loadResult.frameduration);
			pSpriteMaterial->textures[spriteTextureType].width = loadResult.width;
			pSpriteMaterial->textures[spriteTextureType].height = loadResult.height;
			pSpriteMaterial->textures[spriteTextureType].scaleX = 1;
			pSpriteMaterial->textures[spriteTextureType].scaleY = 1;

			if (scaleValue && scaleValue[0])
			{
				float scales[2] = { 0 };

				if (2 == sscanf(scaleValue, "%f %f", &scales[0], &scales[1]))
				{
					if (scales[0] > 0 || scales[0] < 0)
						pSpriteMaterial->textures[spriteTextureType].scaleX = scales[0];

					if (scales[1] > 0 || scales[1] < 0)
						pSpriteMaterial->textures[spriteTextureType].scaleY = scales[1];
				}
				else if (1 == sscanf(scaleValue, "%f", &scales[0]))
				{
					if (scales[0] > 0 || scales[0] < 0)
					{
						pSpriteMaterial->textures[spriteTextureType].scaleX = scales[0];
						pSpriteMaterial->textures[spriteTextureType].scaleY = scales[0];
					}
				}
			}
		}
	}
}

void R_SpriteLoadExternalFile_FrameTexture(bspentity_t* ent, msprite_t* pSprite, CSpriteModelRenderData* pRenderData)
{
	auto frame = ValueForKey(ent, "frame");

	if (!frame)
	{
		gEngfuncs.Con_Printf("R_SpriteLoadExternalFile_FrameTexture: Failed to parse \"frame\" from \"sprite_texture\"\n");
		return;
	}

	int frameIndex = atoi(frame);

	auto pSpriteFrame = R_GetSpriteFrame(pSprite, frameIndex);

	if (!pSpriteFrame)
	{
		gEngfuncs.Con_Printf("R_SpriteLoadExternalFile_FrameTexture: no such frame \"%s\".\n", frame);
		return;
	}

	auto replacetexture = ValueForKey(ent, "replacetexture");
	auto replacescale = ValueForKey(ent, "replacescale");

	R_SpriteLoadExternalFile_FrameTextureLoad(ent, pSprite, pRenderData, frameIndex, pSpriteFrame, replacetexture, replacescale, SPRITE_REPLACE_TEXTURE);
}

void R_SpriteLoadExternalFile(model_t* mod, msprite_t* pSprite, CSpriteModelRenderData* pRenderData)
{
	std::string fullPath = mod->name;

	RemoveFileExtension(fullPath);

	fullPath += "_external.txt";

	auto pFile = (const char*)gEngfuncs.COM_LoadFile(fullPath.c_str(), 5, NULL);

	if (pFile)
	{
		std::vector<bspentity_t*> vEntities;

		R_ParseBSPEntities(pFile, vEntities);

		for (auto ent : vEntities)
		{
			auto classname = ent->classname;

			if (!classname)
				continue;

			if (!strcmp(classname, "sprite_efx"))
			{
				R_SpriteLoadExternalFile_Efx(ent, pSprite, pRenderData);
			}
			else if (!strcmp(classname, "sprite_frame_texture"))
			{
				R_SpriteLoadExternalFile_FrameTexture(ent, pSprite, pRenderData);
			}
		}

		for (auto ent : vEntities)
		{
			delete ent;
		}

		gEngfuncs.COM_FreeFile((void*)pFile);
	}
}

std::shared_ptr<CSpriteModelRenderData> R_GetSpriteRenderDataFromModel(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex >= (int)g_SpriteRenderDataCache.size())
		return nullptr;

	return g_SpriteRenderDataCache[modelindex];
}

void R_AllocSlotForSpriteRenderData(model_t* mod, msprite_t* pSprite, const std::shared_ptr<CSpriteModelRenderData>& pRenderData)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex >= (int)g_SpriteRenderDataCache.size())
	{
		g_SpriteRenderDataCache.resize(modelindex + 1);
	}

	g_SpriteRenderDataCache[modelindex] = pRenderData;
}

void R_FreeSpriteRenderData(model_t* mod)
{
	auto modelindex = EngineGetModelIndex(mod);

	if (modelindex >= 0 && modelindex < (int)g_SpriteRenderDataCache.size())
	{
		auto pRenderData = g_SpriteRenderDataCache[modelindex];

		if (pRenderData)
		{
			gEngfuncs.Con_DPrintf("R_FreeSpriteRenderData: modelindex[%d] modname[%s]!\n", EngineGetModelIndex(mod), mod->name);

			g_SpriteRenderDataCache[modelindex].reset();
		}
	}
}

void R_FreeSpriteRenderData(const std::shared_ptr<CSpriteModelRenderData>& pRenderData)
{
	auto mod = pRenderData->model;

	R_FreeSpriteRenderData(mod);
}

std::shared_ptr<CSpriteModelRenderData> R_CreateSpriteRenderData(model_t *mod)
{
	auto pSprite = (msprite_t*)mod->cache.data;

	auto pRenderData = R_GetSpriteRenderDataFromModel(mod);

	if (pRenderData)
	{
		gEngfuncs.Con_DPrintf("R_CreateSpriteRenderData: Found modelindex[%d] modname[%s].\n", EngineGetModelIndex(mod), mod->name);

		pRenderData->vSpriteMaterials.clear();
		pRenderData->vSpriteMaterials.resize(pSprite->numframes);
	
		R_SpriteLoadExternalFile(mod, pSprite, pRenderData.get());

		return pRenderData;
	}

	gEngfuncs.Con_DPrintf("R_CreateSpriteRenderData: Create modelindex[%d] modname[%s].\n", EngineGetModelIndex(mod), mod->name);

	pRenderData = std::make_shared<CSpriteModelRenderData>(mod);

	pRenderData->vSpriteMaterials.resize(pSprite->numframes);

	R_AllocSlotForSpriteRenderData(mod, pSprite, pRenderData);

	R_SpriteLoadExternalFile(mod, pSprite, pRenderData.get());

	return pRenderData;
}