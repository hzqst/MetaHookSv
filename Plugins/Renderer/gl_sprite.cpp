#include "gl_local.h"
#include <sstream>
#include <algorithm>

std::unordered_map<program_state_t, sprite_program_t> g_SpriteProgramTable;
std::unordered_map<program_state_t, legacysprite_program_t> g_LegacySpriteProgramTable;

int r_sprite_drawcall = 0;
int r_sprite_polys = 0;

int *particletexture = NULL;
particle_t **active_particles = NULL;
word **host_basepal = NULL;

cvar_t *r_sprite_lerping = NULL;

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

		prog.program = R_CompileShaderFileEx("renderer\\shader\\sprite_shader.vert.glsl", "renderer\\shader\\sprite_shader.frag.glsl", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			
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
		g_pMetaHookAPI->SysError("R_UseSpriteProgram: Failed to load program!");
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

void R_UseLegacySpriteProgram(program_state_t state, legacysprite_program_t *progOutput)
{
	legacysprite_program_t prog = { 0 };

	auto itor = g_LegacySpriteProgramTable.find(state);
	if (itor == g_LegacySpriteProgramTable.end())
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

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\legacysprite_shader.vert.glsl", "renderer\\shader\\legacysprite_shader.frag.glsl", def.c_str(), def.c_str(), NULL);

		if (prog.program)
		{

		}

		g_LegacySpriteProgramTable[state] = prog;
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
		g_pMetaHookAPI->SysError("R_UseLegacySpriteProgram: Failed to load program!");
	}
}

void R_SaveLegacySpriteProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto &p : g_LegacySpriteProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/legacysprite_cache.txt", states, s_SpriteProgramStateName, _ARRAYSIZE(s_SpriteProgramStateName));
}

void R_LoadLegacySpriteProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/legacysprite_cache.txt", s_SpriteProgramStateName, _ARRAYSIZE(s_SpriteProgramStateName), [](program_state_t state) {

		R_UseLegacySpriteProgram(state, NULL);

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
void R_GetSpriteFrameInterpolant(cl_entity_t* ent, msprite_t* pSprite, mspriteframe_t** oldframe, mspriteframe_t** curframe, float *lerp)
{
#if 0
	float	framerate = 10.0f;
	float	lerpFrac = 0.0f, frame;
	float	frametime = (1.0f / framerate);
	int	i, j, iframe, oldf, newf;

	bool fDoInterp = (ent->curstate.effects & EF_NOINTERP) ? false : true;

	if (!fDoInterp || ent->curstate.framerate < 0.0f || pSprite->numframes <= 1)
	{
		*oldframe = *curframe = R_GetSpriteFrame(pSprite, ent->curstate.frame);
		*lerp = lerpFrac;
		return;
	}
	frame = fmax(0.0f, ent->curstate.frame);
	iframe = (int)ent->curstate.frame;

	if (ent->curstate.framerate > 0.0f)
	{
		frametime = (1.0f / ent->curstate.framerate);
		framerate = ent->curstate.framerate;
	}

	if (iframe < 0)
	{
		iframe = 0;
	}
	else if (iframe >= pSprite->numframes)
	{
		iframe = pSprite->numframes - 1;
	}

	oldf = (int)floor(frame - 0.5);
	newf = (int)ceil(frame - 0.5);

	oldf = oldf % (pSprite->numframes - 1);
	newf = newf % (pSprite->numframes + 1);

	if (pSprite->frames[iframe].type == SPR_SINGLE)
	{
		// frame was changed
		if (newf != ent->latched.prevframe)
		{
			ent->latched.prevanimtime = (*cl_time) + frametime;
			ent->latched.prevframe = newf;
			lerpFrac = 0.0f; // reset lerp
		}

		if (ent->latched.prevanimtime != 0.0f && ent->latched.prevanimtime >= (*cl_time))
			lerpFrac = (ent->latched.prevanimtime - (*cl_time)) * framerate;

		// compute lerp factor
		lerpFrac = (int)(10000 * lerpFrac) / 10000.0f;
		lerpFrac = clamp(1.0f - lerpFrac, 0.0f, 1.0f);

		// get the interpolated frames
		*oldframe = R_GetSpriteFrame(pSprite, oldf);
		*curframe = R_GetSpriteFrame(pSprite, newf);
	}

	*lerp = lerpFrac;
#endif

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
		if (oldframe) *oldframe = R_GetSpriteFrame(pSprite, ent->latched.prevseqblending[0]);
		if (curframe) *curframe = R_GetSpriteFrame(pSprite, frame);
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

void R_DrawSpriteModelInterpFrames(cl_entity_t* ent, msprite_t* pSprite, mspriteframe_t* frame, mspriteframe_t* oldframe, float lerp)
{
	auto pSpriteVBOData = (sprite_vbo_t *)pSprite->cachespot;

	auto scale = ent->curstate.scale;

	if (scale <= 0)
		scale = 1;

	if (ent->curstate.rendermode == kRenderNormal)
		(*r_blend) = 1.0;

	colorVec color = { 0 };
	R_SpriteColor(&color, ent, (*r_blend) * 255);

	float u_color[4] = { 0 };

	program_state_t SpriteProgramState = 0;

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

	glEnable(GL_ALPHA_TEST);

	if (r_draw_reflectview)
	{
		SpriteProgramState |= SPRITE_CLIP_ENABLED;
	}

	if (!R_IsRenderingGBuffer() && R_IsRenderingFog())
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
	}

	if (R_IsRenderingGBuffer())
	{
		SpriteProgramState |= SPRITE_GBUFFER_ENABLED;
	}

	if (r_draw_gammablend)
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

	bool bNoBloom = false;

	if (pSpriteVBOData && (pSpriteVBOData->flags & EF_NOBLOOM))
	{
		bNoBloom = true;
	}

	if (r_draw_opaque)
	{
		int iStencilRef = STENCIL_MASK_WORLD;
		
		if (bNoBloom)
			iStencilRef |= STENCIL_MASK_NO_BLOOM;

		//has stencil write-in
		GL_BeginStencilWrite(iStencilRef, STENCIL_MASK_ALL);
	}
	else
	{
		int iStencilRef = 0;

		if (bNoBloom)
			iStencilRef |= STENCIL_MASK_NO_BLOOM;

		//has stencil write-in
		GL_BeginStencilWrite(iStencilRef, STENCIL_MASK_NO_BLOOM);
	}

	sprite_program_t prog = { 0 };
	R_UseSpriteProgram(SpriteProgramState, &prog);

	glUniform2i(0, frame->width, frame->height);
	glUniform4f(1, frame->up, frame->down, frame->left, frame->right);
	glUniform4f(2, u_color[0], u_color[1], u_color[2], u_color[3]);
	glUniform3f(3, r_entorigin[0], r_entorigin[1], r_entorigin[2]);
	glUniform3f(4, ent->angles[0], ent->angles[1], ent->angles[2]);
	glUniform1f(5, scale);
	glUniform1f(6, math_clamp(lerp, 0.0f, 1.0f));

	GL_Bind(frame->gl_texturenum);

	if (SpriteProgramState & SPRITE_LERP_ENABLED)
	{
		GL_EnableMultitexture();
		GL_Bind(oldframe->gl_texturenum);

		glDrawArrays(GL_QUADS, 0, 4);

		GL_Bind(0);
		GL_DisableMultitexture();
	}
	else
	{
		glDrawArrays(GL_QUADS, 0, 4);
	}

	r_sprite_drawcall++;
	r_sprite_polys++;

	GL_UseProgram(0);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	GL_EndStencil();
}

void R_DrawSpriteModel(cl_entity_t *ent)
{
	if (R_IsRenderingShadowView())
		return;

	auto pSprite = (msprite_t *)ent->model->cache.data;

	float lerp = 0;
	mspriteframe_t* frame = NULL;
	mspriteframe_t* oldframe = NULL;

	if (R_SpriteAllowLerping(ent, pSprite))
	{
		R_GetSpriteFrameInterpolant(ent, pSprite, &oldframe, &frame, &lerp);
	}
	else
	{
		oldframe = frame = R_GetSpriteFrame(pSprite, ent->curstate.frame);
	}

	if (!frame)
	{
		gEngfuncs.Con_DPrintf("R_DrawSpriteModel: Couldn't get sprite frame for %s\n", ent->model);
		return;
	}

	R_DrawSpriteModelInterpFrames(ent, pSprite, frame, oldframe, lerp);
}

void R_SpriteTextureAddReferences(model_t* mod, msprite_t* pSprite, std::set<int>& textures)
{
	for (int i = 0; i < pSprite->numframes; i++)
	{
		auto pSpriteFrame = R_GetSpriteFrame(pSprite, i);

		if (pSpriteFrame)
		{
			textures.emplace(pSpriteFrame->gl_texturenum);
		}
	}
}

void R_SpriteLoadExternalFile_Efx(bspentity_t* ent, msprite_t* pSprite, sprite_vbo_t* pSpriteVBOData)
{
	auto flags_string = ValueForKey(ent, "flags");

#define REGISTER_EFX_FLAGS_KEY_VALUE(name) if (flags_string && !strcmp(flags_string, #name))\
	{\
		pSpriteVBOData->flags |= name; \
	}\
	if (flags_string && !strcmp(flags_string, "-" #name))\
	{\
		pSpriteVBOData->flags &= ~name; \
	}

	REGISTER_EFX_FLAGS_KEY_VALUE(EF_NOBLOOM);

#undef REGISTER_EFX_FLAGS_KEY_VALUE
}

void R_SpriteLoadExternalFile(model_t* mod, msprite_t* pSprite, sprite_vbo_t* pSpriteVBOData)
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
				R_SpriteLoadExternalFile_Efx(ent, pSprite, pSpriteVBOData);
			}
		}

		for (auto ent : vEntities)
		{
			delete ent;
		}

		gEngfuncs.COM_FreeFile((void*)pFile);
	}
}