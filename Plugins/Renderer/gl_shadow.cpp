#include "gl_local.h"

//renderer

vec3_t shadow_light_mins;
vec3_t shadow_light_maxs;

int shadow_depthmap_high = 0;
int shadow_depthmap_high_texsize = 0;

int shadow_depthmap_medium = 0;
int shadow_depthmap_medium_texsize = 0;

int shadow_depthmap_low = 0;
int shadow_depthmap_low_texsize = 0;

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

//shader
SHADER_DEFINE(shadow);

//cvar
cvar_t *r_shadow = NULL;
cvar_t *r_shadow_debug = NULL;
cvar_t *r_shadow_alpha = NULL;
cvar_t *r_shadow_angle_p = NULL;
cvar_t *r_shadow_angle_y = NULL;
cvar_t *r_shadow_angle_r = NULL;
cvar_t *r_shadow_high_texsize = NULL;
cvar_t *r_shadow_high_distance = NULL;
cvar_t *r_shadow_high_scale = NULL;
cvar_t *r_shadow_medium_texsize = NULL;
cvar_t *r_shadow_medium_distance = NULL;
cvar_t *r_shadow_medium_scale = NULL;
cvar_t *r_shadow_low_texsize = NULL;
cvar_t *r_shadow_low_distance = NULL;
cvar_t *r_shadow_low_scale = NULL;
cvar_t *r_shadow_map_override = NULL;

void R_FreeShadow(void)
{
	if (shadow_depthmap_high)
	{
		GL_DeleteTexture(shadow_depthmap_high);
		shadow_depthmap_high = NULL;
	}
	if (shadow_depthmap_medium)
	{
		GL_DeleteTexture(shadow_depthmap_medium);
		shadow_depthmap_medium = NULL;
	}
	if (shadow_depthmap_low)
	{
		GL_DeleteTexture(shadow_depthmap_low);
		shadow_depthmap_low = NULL;
	}
}

void R_InitShadow(void)
{
	if(gl_shader_support)
	{
		shadow.program = R_CompileShaderFile("resource\\shader\\shadow_shader.vsh", NULL, "resource\\shader\\shadow_shader.fsh");
		if (shadow.program)
		{
			SHADER_UNIFORM(shadow, texoffset_high, "texoffset_high");
			SHADER_UNIFORM(shadow, texoffset_medium, "texoffset_medium");
			SHADER_UNIFORM(shadow, texoffset_low, "texoffset_low");
			SHADER_UNIFORM(shadow, depthmap_high, "depthmap_high");
			SHADER_UNIFORM(shadow, depthmap_medium, "depthmap_medium");
			SHADER_UNIFORM(shadow, depthmap_low, "depthmap_low");
			SHADER_UNIFORM(shadow, numedicts_high, "numedicts_high");
			SHADER_UNIFORM(shadow, numedicts_medium, "numedicts_medium");
			SHADER_UNIFORM(shadow, numedicts_low, "numedicts_low");
			SHADER_UNIFORM(shadow, alpha, "alpha");
		}
	}

	r_shadow = gEngfuncs.pfnRegisterVariable("r_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_map_override = gEngfuncs.pfnRegisterVariable("r_shadow_map_override", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_debug = gEngfuncs.pfnRegisterVariable("r_shadow_debug", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_alpha = gEngfuncs.pfnRegisterVariable("r_shadow_alpha", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_p = gEngfuncs.pfnRegisterVariable("r_shadow_angle_pitch", "90", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_y = gEngfuncs.pfnRegisterVariable("r_shadow_angle_yaw", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_r = gEngfuncs.pfnRegisterVariable("r_shadow_angle_roll", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_texsize = gEngfuncs.pfnRegisterVariable("r_shadow_high_texsize", "2048", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_distance = gEngfuncs.pfnRegisterVariable("r_shadow_high_distance", "400", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_scale = gEngfuncs.pfnRegisterVariable("r_shadow_high_scale", "4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_texsize = gEngfuncs.pfnRegisterVariable("r_shadow_medium_texsize", "2048", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_distance = gEngfuncs.pfnRegisterVariable("r_shadow_medium_distance", "1024", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_scale = gEngfuncs.pfnRegisterVariable("r_shadow_medium_scale", "2", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_texsize = gEngfuncs.pfnRegisterVariable("r_shadow_low_texsize", "2048", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_distance = gEngfuncs.pfnRegisterVariable("r_shadow_low_distance", "4096", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_scale = gEngfuncs.pfnRegisterVariable("r_shadow_low_scale", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

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
		if (ent->index == 0)
			return false;
		if (ent->curstate.movetype == MOVETYPE_NONE)
			return false;

		return true;
	}

	return false;
}

int R_GetTextureSizePowerOfTwo(int texSize)
{
	int scaled_texsize;
	for (scaled_texsize = 1; scaled_texsize < texSize; scaled_texsize <<= 1) {}

	int max_size = max(128, gl_max_texture_size);
	if (!s_BackBufferFBO.s_hBackBufferFBO && max_size > 512)//glCopyTexImage2D fix
		max_size = 512;

	return scaled_texsize;
}

void R_RenderShadowMap(void)
{
	int highSize = R_GetTextureSizePowerOfTwo(r_shadow_high_texsize->value);

	if (!s_ShadowFBO.s_hBackBufferFBO)
		highSize = 512;//the fucking glCopyTexImage2D limit up to 512x512

	if (!shadow_depthmap_high)
	{
		shadow_depthmap_high_texsize = highSize;
		shadow_depthmap_high = GL_GenShadowTexture(highSize, highSize);
	}
	else if (shadow_depthmap_high_texsize != highSize)
	{
		shadow_depthmap_high_texsize = highSize;
		GL_UploadShadowTexture(shadow_depthmap_high, highSize, highSize);
	}

	int mediumSize = R_GetTextureSizePowerOfTwo(r_shadow_medium_texsize->value);

	if (!s_ShadowFBO.s_hBackBufferFBO)
		mediumSize = 512;//the fucking glCopyTexImage2D limit up to 512x512

	if (!shadow_depthmap_medium)
	{
		shadow_depthmap_medium_texsize = mediumSize;
		shadow_depthmap_medium = GL_GenShadowTexture(mediumSize, mediumSize);
	}
	else if (shadow_depthmap_medium_texsize != mediumSize)
	{
		shadow_depthmap_medium_texsize = mediumSize;
		GL_UploadShadowTexture(shadow_depthmap_medium, mediumSize, mediumSize);
	}

	int lowSize = R_GetTextureSizePowerOfTwo(r_shadow_low_texsize->value);

	if (!s_ShadowFBO.s_hBackBufferFBO)
		lowSize = 512;//the fucking glCopyTexImage2D limit up to 512x512

	if (!shadow_depthmap_low)
	{
		shadow_depthmap_low_texsize = lowSize;
		shadow_depthmap_low = GL_GenShadowTexture(lowSize, lowSize);
	}
	else if (shadow_depthmap_low_texsize != lowSize)
	{
		shadow_depthmap_low_texsize = lowSize;
		GL_UploadShadowTexture(shadow_depthmap_low, lowSize, lowSize);
	}

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

	if(s_ShadowFBO.s_hBackBufferFBO)
	{
		GL_PushFrameBuffer();
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_ShadowFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		qglDrawBuffer(GL_NONE);
	}

	int shadowmapArray[3] = { shadow_depthmap_high, shadow_depthmap_medium, shadow_depthmap_low };
	int texsizeArray[3] = { shadow_depthmap_high_texsize, shadow_depthmap_medium_texsize, shadow_depthmap_low_texsize };
	float scaleArray[3] = { r_shadow_high_scale->value, r_shadow_medium_scale->value, r_shadow_low_scale->value };

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

			if (distance > r_shadow_medium_distance->value)
			{
				if (shadow_numvisedicts_low < 512)
				{
					shadow_visedicts_low[shadow_numvisedicts_low] = cl_visedicts[j];
					shadow_numvisedicts_low++;
				}
			}
			else if (distance > r_shadow_high_distance->value)
			{
				if (shadow_numvisedicts_medium < 512)
				{
					shadow_visedicts_medium[shadow_numvisedicts_medium] = cl_visedicts[j];
					shadow_numvisedicts_medium++;
				}
			}
			else
			{
				if (shadow_numvisedicts_high < 512)
				{
					shadow_visedicts_high[shadow_numvisedicts_high] = cl_visedicts[j];
					shadow_numvisedicts_high++;
				}
			}
		}
	}

	int numvisedictsArray[3] = { shadow_numvisedicts_high , shadow_numvisedicts_medium, shadow_numvisedicts_low };
	cl_entity_t **visedictsArray[3] = { shadow_visedicts_high , shadow_visedicts_medium, shadow_visedicts_low };

	float *projmatrixArray[3] = { shadow_projmatrix_high , shadow_projmatrix_medium, shadow_projmatrix_low };
	float *mvmatrixArray[3] = { shadow_mvmatrix_high , shadow_mvmatrix_medium, shadow_mvmatrix_low };

	qglDisable(GL_CULL_FACE);

	for (int i = 0; i < 3; ++i)
	{
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowmapArray[i], 0);

		qglMatrixMode(GL_PROJECTION);
		qglLoadIdentity();

		float texsize = (float)texsizeArray[i] / scaleArray[i];
		qglOrtho(-texsize, texsize, -texsize, texsize, -4096, 4096);

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

		qglViewport(0, 0, texsizeArray[i], texsizeArray[i]);

		qglDepthMask(GL_TRUE);
		qglClear(GL_DEPTH_BUFFER_BIT);
		qglColorMask(0, 0, 0, 0);

		//render start

		cl_entity_t *backup_curentity = (*currententity);

		for (int j = 0; j < numvisedictsArray[i]; ++j)
		{
			(*currententity) = visedictsArray[i][j];
			R_DrawCurrentEntity();
		}

		(*currententity) = backup_curentity;

		if (!s_BackBufferFBO.s_hBackBufferFBO)
		{
			qglBindTexture(GL_TEXTURE_2D, shadowmapArray[i]);
			qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, texsizeArray[i], texsizeArray[i], 0);
		}

		qglColorMask(1, 1, 1, 1);
	}

	qglEnable(GL_CULL_FACE);

	if(s_ShadowFBO.s_hBackBufferFBO)
	{
		qglDrawBuffer(GL_COLOR_ATTACHMENT0);
		GL_PopFrameBuffer();
	}
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

	for (i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
	{
		pplane = psurf->plane;

		if (psurf->flags & SURF_DRAWTURB)
		{
			if (pplane->type != PLANE_Z && gl_watersides && !gl_watersides->value)
				continue;

			if (mins[2] + 1.0 >= pplane->dist)
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

		if ((*currententity)->curstate.rendermode != kRenderNormal)
		{
			continue;
		}

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

void R_RenderShadowScenes(void)
{
	if(!shadow.program || !r_shadow->value)
		return;

	if (!shadow_depthmap_high)
		return;

	if (!shadow_depthmap_medium)
		return;

	if (!shadow_depthmap_low)
		return;

	GL_PushDrawState();

	const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f, 
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f	};

	GLfloat planeS[] = { 1.0, 0.0, 0.0, 0.0 };
	GLfloat planeT[] = { 0.0, 1.0, 0.0, 0.0 };
	GLfloat planeR[] = { 0.0, 0.0, 1.0, 0.0 };
	GLfloat planeQ[] = { 0.0, 0.0, 0.0, 1.0 };

	float mvmatrix[16];
	float invmvmatrix[16];
	GLfloat texture1_env;
	GLfloat texture2_env;

	qglGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
	InvertMatrix(mvmatrix, invmvmatrix);

	qglUseProgramObjectARB(shadow.program);
	qglUniform1iARB(shadow.depthmap_high, 0);
	qglUniform1iARB(shadow.depthmap_medium, 1);
	qglUniform1iARB(shadow.depthmap_low, 2);

	qglUniform1iARB(shadow.numedicts_high, shadow_numvisedicts_high);
	qglUniform1iARB(shadow.numedicts_medium, shadow_numvisedicts_medium);
	qglUniform1iARB(shadow.numedicts_low, shadow_numvisedicts_low);

	qglUniform1fARB(shadow.texoffset_high, 1 / shadow_depthmap_high_texsize);
	qglUniform1fARB(shadow.texoffset_medium, 1 / shadow_depthmap_medium_texsize);
	qglUniform1fARB(shadow.texoffset_low, 1 / shadow_depthmap_low_texsize);

	qglUniform1fARB(shadow.alpha, r_shadow_alpha->value);

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
	//qglColor4f(0.1, 0.1, 0.1, 0.5);

	//setup texture 0
	GL_SelectTexture(TEXTURE0_SGIS);
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
	GL_Bind(shadow_depthmap_high);

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglLoadMatrixf(bias);
	qglMultMatrixf(shadow_projmatrix_high);
	qglMultMatrixf(shadow_mvmatrix_high);
	qglMultMatrixf(invmvmatrix);
	
	//setup texture1
	GL_EnableMultitexture();
	qglGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &texture1_env);
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
	GL_Bind(shadow_depthmap_medium);

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglLoadMatrixf(bias);
	qglMultMatrixf(shadow_projmatrix_medium);
	qglMultMatrixf(shadow_mvmatrix_medium);
	qglMultMatrixf(invmvmatrix);

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &texture2_env);
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
	qglBindTexture(GL_TEXTURE_2D, shadow_depthmap_low);

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglLoadMatrixf(bias);
	qglMultMatrixf(shadow_projmatrix_low);
	qglMultMatrixf(shadow_mvmatrix_low);
	qglMultMatrixf(invmvmatrix);
	
	qglMatrixMode(GL_MODELVIEW);

	shadow_light_mins[0] = r_refdef->vieworg[0] - r_shadow_low_distance->value;
	shadow_light_mins[1] = r_refdef->vieworg[1] - r_shadow_low_distance->value;
	shadow_light_mins[2] = r_refdef->vieworg[2] - r_shadow_low_distance->value;
	shadow_light_maxs[0] = r_refdef->vieworg[0] + r_shadow_low_distance->value;
	shadow_light_maxs[1] = r_refdef->vieworg[1] + r_shadow_low_distance->value;
	shadow_light_maxs[2] = r_refdef->vieworg[2] + r_shadow_low_distance->value;

	cl_entity_t *backup_curentity = (*currententity);
	(*currententity) = r_worldentity;

	if (r_wsurf_vbo->value)
	{
		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		R_SetVBOState(VBOSTATE_NO_TEXTURE);

		for (size_t i = 0; i < r_wsurf.vTextureChainStatic.size(); ++i)
		{
			auto &texchain = r_wsurf.vTextureChainStatic[i];

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}

		for (size_t i = 0; i < r_wsurf.vTextureChainScroll.size(); ++i)
		{
			auto &texchain = r_wsurf.vTextureChainScroll[i];

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}

		R_SetVBOState(VBOSTATE_OFF);

		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
	else
	{
		R_RecursiveWorldNodeShadow(r_worldmodel->nodes);
	}

	if (r_shadow->value >= 2)
		R_DrawEntitiesOnListShadow();

	(*currententity) = backup_curentity;

	qglUseProgramObjectARB(0);

	//Restore texture0

	//restore texture 2
	qglActiveTextureARB(TEXTURE2_SGIS);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texture2_env);
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);
	qglDisable(GL_TEXTURE_GEN_R);
	qglDisable(GL_TEXTURE_GEN_Q);
	qglDisable(GL_TEXTURE_2D);

	//restore texture 1
	qglActiveTextureARB(TEXTURE1_SGIS);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texture1_env);
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);
	qglDisable(GL_TEXTURE_GEN_R);
	qglDisable(GL_TEXTURE_GEN_Q);

	//restore texture 0
	GL_DisableMultitexture();
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);
	qglDisable(GL_TEXTURE_GEN_R);
	qglDisable(GL_TEXTURE_GEN_Q);

	GL_PopDrawState();
}