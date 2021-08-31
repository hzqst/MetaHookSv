#include "gl_local.h"
#include <sstream>
#include <algorithm>

std::unordered_map<int, sprite_program_t> g_SpriteProgramTable;
std::unordered_map<int, legacysprite_program_t> g_LegacySpriteProgramTable;

int r_sprite_drawcall = 0;
int r_sprite_polys = 0;

int *particletexture = NULL;
particle_t **active_particles = NULL;
word **host_basepal = NULL;

void R_UseSpriteProgram(int state, sprite_program_t *progOutput)
{
	sprite_program_t prog = { 0 };

	auto itor = g_SpriteProgramTable.find(state);
	if (itor == g_SpriteProgramTable.end())
	{
		std::stringstream defs;

		if(state & SPRITE_BINDLESS_ENABLED)
			defs << "#define BINDLESS_ENABLED\n";

		if(state & SPRITE_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if(state & SPRITE_OIT_ALPHA_BLEND_ENABLED)
			defs << "#define OIT_ALPHA_BLEND_ENABLED\n";

		if (state & SPRITE_OIT_ADDITIVE_BLEND_ENABLED)
			defs << "#define OIT_ADDITIVE_BLEND_ENABLED\n";

		if (state & SPRITE_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & SPRITE_ADDITIVE_BLEND_ENABLED)
			defs << "#define ADDITIVE_BLEND_ENABLED\n";

		if (state & SPRITE_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & SPRITE_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

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

		prog.program = R_CompileShaderFileEx("renderer\\shader\\sprite_shader.vsh", "renderer\\shader\\sprite_shader.fsh", def.c_str(), def.c_str(), NULL);
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
		Sys_ErrorEx("R_UseSpriteProgram: Failed to load program!");
	}
}

void R_UseLegacySpriteProgram(int state, legacysprite_program_t *progOutput)
{
	legacysprite_program_t prog = { 0 };

	auto itor = g_LegacySpriteProgramTable.find(state);
	if (itor == g_LegacySpriteProgramTable.end())
	{
		std::stringstream defs;

		if (state & SPRITE_OIT_ALPHA_BLEND_ENABLED)
			defs << "#define OIT_ALPHA_BLEND_ENABLED\n";

		if (state & SPRITE_OIT_ADDITIVE_BLEND_ENABLED)
			defs << "#define OIT_ADDITIVE_BLEND_ENABLED\n";

		if (state & SPRITE_ALPHA_BLEND_ENABLED)
			defs << "#define ALPHA_BLEND_ENABLED\n";

		if (state & SPRITE_ADDITIVE_BLEND_ENABLED)
			defs << "#define ADDITIVE_BLEND_ENABLED\n";

		if (state & SPRITE_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & SPRITE_CLIP_ENABLED)
			defs << "#define CLIP_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\legacysprite_shader.vsh", "renderer\\shader\\legacysprite_shader.fsh", def.c_str(), def.c_str(), NULL);
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
		Sys_ErrorEx("R_UseLegacySpriteProgram: Failed to load program!");
	}
}

void R_SpriteColor(colorVec *pColor, cl_entity_t *pEntity, int alpha)
{
	int a;

	if (pEntity->curstate.rendermode == kRenderGlow || pEntity->curstate.rendermode == kRenderTransAdd)
		a = clamp(alpha, 0, 255);//some entities > 255 wtf?
	else
		a = 256;

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

mspriteframe_t *R_GetSpriteFrame(msprite_t *pSprite, int frame)
{
	mspriteframe_t *pspriteframe;

	if (!pSprite)
	{
		gEngfuncs.Con_DPrintf("Sprite:  no pSprite!!!\n");
		return NULL;
	}

	if (!pSprite->numframes)
	{
		gEngfuncs.Con_DPrintf("Sprite:  pSprite has no frames!!!\n");
		return NULL;
	}

	if ((frame >= pSprite->numframes) || (frame < 0))
	{
		gEngfuncs.Con_DPrintf("Sprite: no such frame %d\n", frame);
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

void R_DrawSpriteModel(cl_entity_t *ent)
{
	auto psprite = (msprite_t *)ent->model->cache.data;
	auto frame = R_GetSpriteFrame(psprite, ent->curstate.frame);
	if (!frame)
	{
		gEngfuncs.Con_DPrintf("R_DrawSpriteModel: couldn't get sprite frame for %s\n", ent->model);
		return;
	}

	auto scale = ent->curstate.scale;

	if (scale <= 0)
		scale = 1;

	if (ent->curstate.rendermode == kRenderNormal)
		(*r_blend) = 1.0;

	colorVec color;
	R_SpriteColor(&color, ent, (*r_blend) * 255);

	float u_color[4];

	int SpriteProgramState = 0;

	if (r_draw_opaque)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	if (!gl_spriteblend->value && ent->curstate.rendermode == kRenderNormal)
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
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
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ALPHA);

			glDepthMask(0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if (r_draw_oitblend)
				SpriteProgramState |= SPRITE_OIT_ALPHA_BLEND_ENABLED;
			else
				SpriteProgramState |= SPRITE_ALPHA_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = (*r_blend);
			break;
		}

		case kRenderTransAdd:
		{
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glDepthMask(0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			R_SetGBufferBlend(GL_ONE, GL_ONE);

			if (r_draw_oitblend)
				SpriteProgramState |= SPRITE_OIT_ADDITIVE_BLEND_ENABLED;
			else
				SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = 1;
			break;
		}

		case kRenderGlow:
		{
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glDisable(GL_DEPTH_TEST);
			glDepthMask(0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			R_SetGBufferBlend(GL_ONE, GL_ONE);

			if (r_draw_oitblend)
				SpriteProgramState |= SPRITE_OIT_ADDITIVE_BLEND_ENABLED;
			else
				SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = 1;
			break;
		}

		case kRenderTransAlpha:
		{
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			
			glDepthMask(0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if (r_draw_oitblend)
				SpriteProgramState |= SPRITE_OIT_ALPHA_BLEND_ENABLED;
			else
				SpriteProgramState |= SPRITE_ALPHA_BLEND_ENABLED;

			u_color[0] = color.r / 255.0f;
			u_color[1] = color.g / 255.0f;
			u_color[2] = color.b / 255.0f;
			u_color[3] = (*r_blend);
			break;
		}

		default:
		{
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glDepthMask(0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			R_SetGBufferBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if (r_draw_oitblend)
				SpriteProgramState |= SPRITE_OIT_ALPHA_BLEND_ENABLED;
			else
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

	if (r_draw_pass == r_draw_reflect && curwater)
	{
		SpriteProgramState |= SPRITE_CLIP_ENABLED;
	}

	if (!drawgbuffer && r_fog_mode == GL_LINEAR)
	{
		SpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
	}

	if (drawgbuffer)
	{
		SpriteProgramState |= SPRITE_GBUFFER_ENABLED;
		R_SetGBufferMask(GBUFFER_MASK_ALL);
	}

	int type = psprite->type;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		if (ent->curstate.effects & EF_FIBERCAMERA)
		{
			type = clamp(ent->curstate.sequence, SPR_VP_PARALLEL_UPRIGHT, SPR_VP_PARALLEL_ORIENTED);
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

	sprite_program_t prog = { 0 };
	R_UseSpriteProgram(SpriteProgramState, &prog);

	glUniform2i(0, frame->width, frame->height);
	glUniform4f(1, frame->up, frame->down, frame->left, frame->right);
	glUniform4f(2, u_color[0], u_color[1], u_color[2], u_color[3]);
	glUniform3f(3, r_entorigin[0], r_entorigin[1], r_entorigin[2]);
	glUniform3f(4, ent->angles[0], ent->angles[1], ent->angles[2]);
	glUniform1f(5, scale);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GL_Bind(frame->gl_texturenum);
	glDrawArrays(GL_QUADS, 0, 4);

	r_sprite_drawcall++;
	r_sprite_polys++;

	GL_UseProgram(0);

	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);

	glDisable(GL_STENCIL_TEST);

	if (ent->curstate.rendermode != kRenderNormal || gl_spriteblend->value)
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
}