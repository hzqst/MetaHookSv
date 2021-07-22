#include "gl_local.h"
#include <sstream>

//renderer

vec3_t shadow_light_mins;
vec3_t shadow_light_maxs;

int shadow_texture_depth = 0;
int shadow_texture_high = 0;
int shadow_texture_medium = 0;
int shadow_texture_low = 0;
int shadow_texture_size = 0;

float shadow_projmatrix_high[16];
float shadow_mvmatrix_high[16];

float shadow_projmatrix_medium[16];
float shadow_mvmatrix_medium[16];

float shadow_projmatrix_low[16];
float shadow_mvmatrix_low[16];

cl_entity_t *shadow_visedicts_high[512];
cl_entity_t *shadow_visedicts_medium[512];
cl_entity_t *shadow_visedicts_low[512];

int shadow_numvisedicts_high = 0;
int shadow_numvisedicts_medium = 0;
int shadow_numvisedicts_low = 0;

//cvar
cvar_t *r_shadow = NULL;
cvar_t *r_shadow_debug = NULL;
cvar_t *r_shadow_alpha = NULL;
cvar_t *r_shadow_fade_start = NULL;
cvar_t *r_shadow_fade_end = NULL;
cvar_t *r_shadow_angle_p = NULL;
cvar_t *r_shadow_angle_y = NULL;
cvar_t *r_shadow_angle_r = NULL;
cvar_t *r_shadow_high_distance = NULL;
cvar_t *r_shadow_high_scale = NULL;
cvar_t *r_shadow_medium_distance = NULL;
cvar_t *r_shadow_medium_scale = NULL;
cvar_t *r_shadow_low_distance = NULL;
cvar_t *r_shadow_low_scale = NULL;
cvar_t *r_shadow_map_override = NULL;

std::unordered_map <int, shadow_program_t> g_ShadowProgramTable;

std::unordered_map <int, cshadow_program_t> g_CastShadowProgramTable;

void R_UseShadowProgram(int state, shadow_program_t *progOutput)
{
	shadow_program_t prog = { 0 };

	auto itor = g_ShadowProgramTable.find(state);
	if (itor == g_ShadowProgramTable.end())
	{
		std::stringstream defs;

		if (state & SHADOW_HIGH_ENABLED)
			defs << "#define HIGH_ENABLED\n";

		if (state & SHADOW_MEDIUM_ENABLED)
			defs << "#define MEDIUM_ENABLED\n";

		if (state & SHADOW_LOW_ENABLED)
			defs << "#define LOW_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\shadow_shader.vsh", NULL, "renderer\\shader\\shadow_shader.fsh", def.c_str(), NULL, def.c_str());
		SHADER_UNIFORM(prog, texoffset_high, "texoffset_high");
		SHADER_UNIFORM(prog, texoffset_medium, "texoffset_medium");
		SHADER_UNIFORM(prog, texoffset_low, "texoffset_low");
		SHADER_UNIFORM(prog, texture_high, "texture_high");
		SHADER_UNIFORM(prog, texture_medium, "texture_medium");
		SHADER_UNIFORM(prog, texture_low, "texture_low");
		SHADER_UNIFORM(prog, entitymatrix, "entitymatrix");
		SHADER_UNIFORM(prog, alpha, "alpha");
		SHADER_UNIFORM(prog, fadefactor, "fadefactor");

		g_ShadowProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		qglUseProgramObjectARB(prog.program);

		if (prog.texture_high != -1)
			qglUniform1iARB(prog.texture_high, 0);

		if (prog.texture_medium != -1)
			qglUniform1iARB(prog.texture_medium, 1);

		if (prog.texture_low != -1)
			qglUniform1iARB(prog.texture_low, 2);

		if (prog.texoffset_high != -1)
			qglUniform1fARB(prog.texoffset_high, 1.0f / shadow_texture_size);

		if (prog.texoffset_medium != -1)
			qglUniform1fARB(prog.texoffset_medium, 1.0f / shadow_texture_size);

		if (prog.texoffset_low != -1)
			qglUniform1fARB(prog.texoffset_low, 1.0f / shadow_texture_size);

		if (prog.alpha != -1)
			qglUniform1fARB(prog.alpha, r_shadow_alpha->value);

		if (prog.fadefactor != -1)
			qglUniform2fARB(prog.fadefactor, r_shadow_fade_start->value, r_shadow_fade_end->value - r_shadow_fade_start->value);

		if (prog.entitymatrix != -1)
		{
			if (r_rotate_entity)
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, true, (float *)r_rotate_entity_matrix);
			else
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, false, (float *)r_identity_matrix);
		}

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseShadowProgram: Failed to load program!");
	}
}

void R_UseCastShadowProgram(int state, cshadow_program_t *progOutput)
{
	cshadow_program_t prog = { 0 };

	auto itor = g_CastShadowProgramTable.find(state);
	if (itor == g_CastShadowProgramTable.end())
	{
		std::stringstream defs;

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\cshadow_shader.vsh", NULL, "renderer\\shader\\cshadow_shader.fsh", def.c_str(), NULL, def.c_str());

		SHADER_UNIFORM(prog, entitypos, "entitypos");

		g_CastShadowProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		qglUseProgramObjectARB(prog.program);

		if (prog.entitypos != -1)
			qglUniform3fARB(prog.entitypos, (*rotationmatrix)[0][3], (*rotationmatrix)[1][3], (*rotationmatrix)[2][3]);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseCastShadowProgram: Failed to load program!");
	}
}

void R_FreeShadow(void)
{
	if (shadow_texture_depth)
	{
		GL_DeleteTexture(shadow_texture_depth);
		shadow_texture_depth = 0;
	}
	if (shadow_texture_high)
	{
		GL_DeleteTexture(shadow_texture_high);
		shadow_texture_high = 0;
	}
	if (shadow_texture_medium)
	{
		GL_DeleteTexture(shadow_texture_medium);
		shadow_texture_medium = 0;
	}
	if (shadow_texture_low)
	{
		GL_DeleteTexture(shadow_texture_low);
		shadow_texture_low = NULL;
	}

	g_ShadowProgramTable.clear();
	g_CastShadowProgramTable.clear();
}

void R_InitShadow(void)
{
	shadow_texture_size = min(gl_max_texture_size, 4096);
	shadow_texture_depth = GL_GenShadowTexture(shadow_texture_size, shadow_texture_size);
	shadow_texture_high = GL_GenTextureColorFormat(shadow_texture_size, shadow_texture_size, gl_color_format);
	shadow_texture_medium = GL_GenTextureColorFormat(shadow_texture_size, shadow_texture_size, gl_color_format);
	shadow_texture_low = GL_GenTextureColorFormat(shadow_texture_size, shadow_texture_size, gl_color_format);

	r_shadow = gEngfuncs.pfnRegisterVariable("r_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_map_override = gEngfuncs.pfnRegisterVariable("r_shadow_map_override", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_debug = gEngfuncs.pfnRegisterVariable("r_shadow_debug", "0",  FCVAR_CLIENTDLL);
	r_shadow_alpha = gEngfuncs.pfnRegisterVariable("r_shadow_alpha", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_fade_start = gEngfuncs.pfnRegisterVariable("r_shadow_fade_start", "60", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_fade_end = gEngfuncs.pfnRegisterVariable("r_shadow_fade_end", "100", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_p = gEngfuncs.pfnRegisterVariable("r_shadow_angle_pitch", "90", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_y = gEngfuncs.pfnRegisterVariable("r_shadow_angle_yaw", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_r = gEngfuncs.pfnRegisterVariable("r_shadow_angle_roll", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_distance = gEngfuncs.pfnRegisterVariable("r_shadow_high_distance", "400", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_scale = gEngfuncs.pfnRegisterVariable("r_shadow_high_scale", "4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_distance = gEngfuncs.pfnRegisterVariable("r_shadow_medium_distance", "1024", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_scale = gEngfuncs.pfnRegisterVariable("r_shadow_medium_scale", "2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_distance = gEngfuncs.pfnRegisterVariable("r_shadow_low_distance", "4096", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_scale = gEngfuncs.pfnRegisterVariable("r_shadow_low_scale", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

#define PhyCorpseFlag1 (753951)
#define PhyCorpseFlag2 (152359)

qboolean R_ShouldCastShadow(cl_entity_t *ent)
{
	if(!ent)
		return false;

	if(!ent->model)
		return false;

	if (ent->curstate.rendermode != kRenderNormal)
		return false;

	if (ent->model->type == mod_studio)
	{
		if (ent->curstate.iuser3 == PhyCorpseFlag1 && ent->curstate.iuser4 == PhyCorpseFlag2)
		{
			return true;
		}
		if (ent->index == 0)
			return false;
		if (ent->curstate.movetype == MOVETYPE_NONE)
			return false;

		return true;
	}

	return false;
}

void R_RenderShadowMap(void)
{
	vec3_t sangles;

	if (r_light_env_angles_exists && r_shadow_map_override->value)
	{
		sangles[0] = r_light_env_angles[0];
		sangles[1] = r_light_env_angles[1];
		sangles[2] = r_light_env_angles[2];
	}
	else
	{
		sangles[0] = r_shadow_angle_p->value;
		sangles[1] = r_shadow_angle_y->value;
		sangles[2] = r_shadow_angle_r->value;
	}

	shadow_numvisedicts_high = 0;
	shadow_numvisedicts_medium = 0;
	shadow_numvisedicts_low = 0;

	for (int j = 0; j < *cl_numvisedicts; ++j)
	{
		if (R_ShouldCastShadow(cl_visedicts[j]))
		{
			vec3_t vec;
			VectorSubtract(cl_visedicts[j]->origin, r_refdef->vieworg, vec);
			float distance = VectorLength(vec);

			if (distance > r_shadow_low_distance->value)
				continue;

			if (distance < r_shadow_high_distance->value)
			{
				if (shadow_numvisedicts_high < 512)
				{
					shadow_visedicts_high[shadow_numvisedicts_high] = cl_visedicts[j];
					shadow_numvisedicts_high++;
				}
			}
			else if (distance < r_shadow_medium_distance->value)
			{
				if (shadow_numvisedicts_medium < 512)
				{
					shadow_visedicts_medium[shadow_numvisedicts_medium] = cl_visedicts[j];
					shadow_numvisedicts_medium++;
				}
			}
			else
			{
				if (shadow_numvisedicts_low < 512)
				{
					shadow_visedicts_low[shadow_numvisedicts_low] = cl_visedicts[j];
					shadow_numvisedicts_low++;
				}
			}
		}
	}

	int total_numvisedicts = shadow_numvisedicts_high + shadow_numvisedicts_medium + shadow_numvisedicts_low;

	if (!total_numvisedicts)
		return;

	int textureArray[3] = { shadow_texture_high, shadow_texture_medium, shadow_texture_low };
	float scaleArray[3] = { r_shadow_high_scale->value, r_shadow_medium_scale->value, r_shadow_low_scale->value };
	int numvisedictsArray[3] = { shadow_numvisedicts_high , shadow_numvisedicts_medium, shadow_numvisedicts_low };
	cl_entity_t **visedictsArray[3] = { shadow_visedicts_high , shadow_visedicts_medium, shadow_visedicts_low };
	float *projmatrixArray[3] = { shadow_projmatrix_high , shadow_projmatrix_medium, shadow_projmatrix_low };
	float *mvmatrixArray[3] = { shadow_mvmatrix_high , shadow_mvmatrix_medium, shadow_mvmatrix_low };

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_ShadowFBO.s_hBackBufferFBO);
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	r_draw_pass = r_draw_shadow_caster;

	for (int i = 0; i < 3; ++i)
	{
		if (!numvisedictsArray[i])
			continue;

		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureArray[i], 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_texture_depth, 0);

		qglMatrixMode(GL_PROJECTION);
		qglLoadIdentity();

		float texsize = (float)shadow_texture_size / scaleArray[i];
		qglOrtho(-texsize / 2, texsize / 2, -texsize / 2, texsize / 2, -4096, 4096);

		qglMatrixMode(GL_MODELVIEW);
		qglLoadIdentity();

		qglRotatef(-90, 1, 0, 0);
		qglRotatef(90, 0, 0, 1);
		qglRotatef(-sangles[2], 1, 0, 0);
		qglRotatef(-sangles[0], 0, 1, 0);
		qglRotatef(-sangles[1], 0, 0, 1);
		qglTranslatef(-r_refdef->vieworg[0], -r_refdef->vieworg[1], -r_refdef->vieworg[2]);

		qglGetFloatv(GL_PROJECTION_MATRIX, projmatrixArray[i]);
		qglGetFloatv(GL_MODELVIEW_MATRIX, mvmatrixArray[i]);

		qglViewport(0, 0, shadow_texture_size, shadow_texture_size);

		qglDisable(GL_BLEND);
		qglDisable(GL_ALPHA_TEST);
		qglDepthMask(1);
		qglColorMask(1, 1, 1, 1);
		qglClearColor(-99999, -99999, -99999, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cl_entity_t *backup_curentity = (*currententity);

		for (int j = 0; j < numvisedictsArray[i]; ++j)
		{
			(*currententity) = visedictsArray[i][j];
			R_DrawCurrentEntity();
		}

		(*currententity) = backup_curentity;

	}

	r_draw_pass = r_draw_normal;
}

int R_ShadowLightCullBox(vec3_t mins, vec3_t maxs)
{
	if (mins[0] > shadow_light_maxs[0]) 
		return TRUE;

	if (mins[1] > shadow_light_maxs[1]) 
		return TRUE;

	if (mins[2] > shadow_light_maxs[2]) 
		return TRUE;

	if (maxs[0] < shadow_light_mins[0]) 
		return TRUE;

	if (maxs[1] < shadow_light_mins[1]) 
		return TRUE;

	if (maxs[2] < shadow_light_mins[2]) 
		return TRUE;

	return FALSE;
}

void R_RecursiveWorldNodeShadow(mnode_t *node)
{
	int c, side;
	mplane_t *plane;
	msurface_t *surf;
	float dot;

	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != (*r_visframecount))
		return;

	if (R_ShadowLightCullBox(node->minmaxs, node->minmaxs + 3))
		return;

	if (node->contents < 0)
	{
		return;
	}

	plane = node->plane;

	switch (plane->type)
	{
		case PLANE_X:
		{
			dot = r_refdef->vieworg[0] - plane->dist;
			break;
		}

		case PLANE_Y:
		{
			dot = r_refdef->vieworg[1] - plane->dist;
			break;
		}

		case PLANE_Z:
		{
			dot = r_refdef->vieworg[2] - plane->dist;
			break;
		}

		default:
		{
			dot = DotProduct(r_refdef->vieworg, plane->normal) - plane->dist;
			break;
		}
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

	R_RecursiveWorldNodeShadow(node->children[side]);

	c = node->numsurfaces;

	if (c)
	{
		surf = r_worldmodel->surfaces + node->firstsurface;

		if (dot < 0 -BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;

		if (plane->type == PLANE_Z && side == SURF_PLANEBACK)
		{
			
		}
		else
		{
			for ( ; c; c--, surf++)
			{
				if (surf->visframe != (*r_framecount))
					continue;

				if (!(surf->flags & SURF_UNDERWATER) && ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
					continue;

				if (!(surf->flags & SURF_DRAWTURB) && !(surf->flags & SURF_DRAWSKY))
				{
					if((*currententity)->curstate.rendermode == kRenderNormal || (*currententity)->curstate.rendermode == kRenderTransAlpha)
						DrawGLPoly(surf);
				}
			}
		}
	}

	R_RecursiveWorldNodeShadow(node->children[!side]);
}

void R_DrawBrushModelShadow(cl_entity_t *e)
{
	int i;
	vec3_t mins, maxs;
	msurface_t *psurf;
	float dot;
	mplane_t *pplane;
	model_t *clmodel;
	qboolean rotated;

	if (e->curstate.rendermode != kRenderNormal)
		return;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;

		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(e->origin, clmodel->mins, mins);
		VectorAdd(e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;

	(*currententity) = e;

	VectorSubtract(r_refdef->vieworg, e->origin, modelorg);

	if (rotated)
	{
		vec3_t temp;
		vec3_t forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

	qglPushMatrix();

	R_RotateForEntity(e->origin, e);

	//TODO: reload matrix
	//R_UseShadowProgram(ShadowProgramState, NULL);

	if (r_wsurf_vbo->value)
	{
		auto modcache = R_PrepareWSurfVBO(clmodel);

		R_EnableWSurfVBO(modcache);

		R_DrawWSurfVBOSolid(modcache);

		R_EnableWSurfVBO(NULL);
	}
	else
	{
		for (i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				continue;
			}

			dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

			if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
			{
				DrawGLPoly(psurf);
			}
			else
			{
				if (psurf->flags & SURF_DRAWTURB)
				{
					DrawGLPoly(psurf);
				}
			}
		}
	}

	qglPopMatrix();
}

void R_DrawEntitiesOnListShadow(void)
{
	int i, numvisedicts;

	if (!r_drawentities->value)
		return;

	numvisedicts = *cl_numvisedicts;

	for (i = 0; i < numvisedicts; i++)
	{
		(*currententity) = cl_visedicts[i];

		switch ((*currententity)->model->type)
		{
		case mod_brush:
		{
			R_DrawBrushModelShadow(*currententity);
			break;
		}

		default:
		{
			break;
		}
		}
	}
}

GLfloat g_SaveTextureEnv[32];

void R_BeginShadowTexture(int unitId, int textureId, float *mvmatrix, float *projmatrix, float *invmvmatrix)
{
	if (unitId == TEXTURE0_SGIS)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
	}
	else if (unitId == TEXTURE1_SGIS)
	{
		GL_EnableMultitexture();
	}
	else
	{
		qglActiveTextureARB(unitId);
	}

	const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f };

	const GLfloat planeS[] = { 1.0, 0.0, 0.0, 0.0 };
	const GLfloat planeT[] = { 0.0, 1.0, 0.0, 0.0 };
	const GLfloat planeR[] = { 0.0, 0.0, 1.0, 0.0 };
	const GLfloat planeQ[] = { 0.0, 0.0, 0.0, 1.0 };

	qglGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &g_SaveTextureEnv[unitId - TEXTURE0_SGIS]);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglEnable(GL_TEXTURE_GEN_S);
	qglEnable(GL_TEXTURE_GEN_T);
	qglEnable(GL_TEXTURE_GEN_R);
	qglEnable(GL_TEXTURE_GEN_Q);
	qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	qglTexGenfv(GL_S, GL_EYE_PLANE, planeS);
	qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	qglTexGenfv(GL_T, GL_EYE_PLANE, planeT);
	qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	qglTexGenfv(GL_R, GL_EYE_PLANE, planeR);
	qglTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	qglTexGenfv(GL_Q, GL_EYE_PLANE, planeQ);

	if (unitId == TEXTURE0_SGIS || unitId == TEXTURE1_SGIS)
	{
		GL_Bind(textureId);
	}
	else
	{
		qglBindTexture(GL_TEXTURE_2D, textureId);
	}

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglLoadMatrixf(bias);
	qglMultMatrixf(projmatrix);
	qglMultMatrixf(mvmatrix);
	qglMultMatrixf(invmvmatrix);
}

void R_EndShadowTexture(int unitId)
{
	qglActiveTextureARB(unitId);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, g_SaveTextureEnv[unitId - TEXTURE0_SGIS]);
	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);
	qglDisable(GL_TEXTURE_GEN_R);
	qglDisable(GL_TEXTURE_GEN_Q);
	if (unitId > TEXTURE1_SGIS)
	{
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}
}

void R_RenderShadowScenes(void)
{
	if(!r_shadow->value)
		return;

	GL_PushDrawState();

	float mvmatrix[16];
	float invmvmatrix[16];

	//test
	if(g_SvEngine_DrawPortalView)
		qglGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
	else
		memcpy(mvmatrix, r_world_matrix, sizeof(mvmatrix));

	InvertMatrix(mvmatrix, invmvmatrix);

	if (gl_polyoffset && gl_polyoffset->value)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);

		if (gl_ztrick && gl_ztrick->value)
			qglPolygonOffset(1, gl_polyoffset->value);
		else
			qglPolygonOffset(-1, -gl_polyoffset->value);
	}

	qglDepthMask(GL_FALSE);
	qglEnable(GL_DEPTH_TEST);

	qglEnable(GL_BLEND);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int ShadowProgramState = 0;

	if (shadow_numvisedicts_high > 0)
	{
		R_BeginShadowTexture(TEXTURE0_SGIS, shadow_texture_high, shadow_mvmatrix_high, shadow_projmatrix_high, invmvmatrix);
		ShadowProgramState |= SHADOW_HIGH_ENABLED;
	}
	if (shadow_numvisedicts_medium > 0)
	{
		R_BeginShadowTexture(TEXTURE1_SGIS, shadow_texture_medium, shadow_mvmatrix_medium, shadow_projmatrix_medium, invmvmatrix);
		ShadowProgramState |= SHADOW_MEDIUM_ENABLED;
	}
	if (shadow_numvisedicts_low > 0)
	{
		R_BeginShadowTexture(TEXTURE2_SGIS, shadow_texture_low, shadow_mvmatrix_low, shadow_projmatrix_low, invmvmatrix);
		ShadowProgramState |= SHADOW_LOW_ENABLED;
	}

	qglMatrixMode(GL_MODELVIEW);

	shadow_light_mins[0] = r_refdef->vieworg[0] - r_shadow_low_distance->value;
	shadow_light_mins[1] = r_refdef->vieworg[1] - r_shadow_low_distance->value;
	shadow_light_mins[2] = r_refdef->vieworg[2] - r_shadow_low_distance->value;
	shadow_light_maxs[0] = r_refdef->vieworg[0] + r_shadow_low_distance->value;
	shadow_light_maxs[1] = r_refdef->vieworg[1] + r_shadow_low_distance->value;
	shadow_light_maxs[2] = r_refdef->vieworg[2] + r_shadow_low_distance->value;

	R_UseShadowProgram(ShadowProgramState, NULL);

	cl_entity_t *backup_curentity = (*currententity);
	(*currententity) = r_worldentity;

	if (r_wsurf_vbo->value)
	{
		auto modcache = R_PrepareWSurfVBO(r_worldmodel);

		R_EnableWSurfVBOSolid(modcache);

		R_DrawWSurfVBOSolid(modcache);

		R_EnableWSurfVBOSolid(NULL);
	}
	else
	{
		R_RecursiveWorldNodeShadow(r_worldmodel->nodes);
	}

	if (r_shadow->value >= 2)
		R_DrawEntitiesOnListShadow();

	(*currententity) = backup_curentity;

	qglUseProgramObjectARB(0);

	if(ShadowProgramState & SHADOW_LOW_ENABLED)
		R_EndShadowTexture(TEXTURE2_SGIS);

	if (ShadowProgramState & SHADOW_MEDIUM_ENABLED)
		R_EndShadowTexture(TEXTURE1_SGIS);

	if (ShadowProgramState & SHADOW_HIGH_ENABLED)
		R_EndShadowTexture(TEXTURE0_SGIS);

	qglActiveTextureARB(*oldtarget);

	GL_PopDrawState();
}