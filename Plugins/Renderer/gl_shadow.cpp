#include "gl_local.h"

//renderer
qboolean drawshadowmap;
qboolean drawshadowscene;

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

void R_ClearShadow(void)
{

}

void R_InitShadow(void)
{
	if(gl_shader_support)
	{
		const char *shadow_vscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\shadow_shader.vsh", 5, 0);
		const char *shadow_fscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\shadow_shader.fsh", 5, 0);
		if(shadow_vscode && shadow_fscode)
		{
			shadow.program = R_CompileShader(shadow_vscode, shadow_fscode, "shadow_shader.vsh", "shadow_shader.fsh");
			if(shadow.program)
			{
				SHADER_UNIFORM(shadow, texoffset_high, "texoffset_high");
				SHADER_UNIFORM(shadow, texoffset_medium, "texoffset_medium");
				SHADER_UNIFORM(shadow, texoffset_low, "texoffset_low");
				SHADER_UNIFORM(shadow, depthmap_high, "depthmap_high");
				SHADER_UNIFORM(shadow, depthmap_medium, "depthmap_medium");
				SHADER_UNIFORM(shadow, depthmap_low, "depthmap_low");
				SHADER_UNIFORM(shadow, alpha, "alpha");
			}
		}

		if (!shadow_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\shadow_shader.vsh\" not found!");
		}
		if (!shadow_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\shadow_shader.fsh\" not found!");
		}
		gEngfuncs.COM_FreeFile((void *)shadow_vscode);
		gEngfuncs.COM_FreeFile((void *)shadow_fscode);
	}

	r_shadow = gEngfuncs.pfnRegisterVariable("r_shadow", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_debug = gEngfuncs.pfnRegisterVariable("r_shadow_debug", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_alpha = gEngfuncs.pfnRegisterVariable("r_shadow_alpha", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_p = gEngfuncs.pfnRegisterVariable("r_shadow_angle_pitch", "100", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_y = gEngfuncs.pfnRegisterVariable("r_shadow_angle_yaw", "30", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_angle_r = gEngfuncs.pfnRegisterVariable("r_shadow_angle_roll", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_texsize = gEngfuncs.pfnRegisterVariable("r_shadow_high_texsize", "2048", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_distance = gEngfuncs.pfnRegisterVariable("r_shadow_high_distance", "256", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_high_scale = gEngfuncs.pfnRegisterVariable("r_shadow_high_scale", "6", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_texsize = gEngfuncs.pfnRegisterVariable("r_shadow_medium_texsize", "2048", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_distance = gEngfuncs.pfnRegisterVariable("r_shadow_medium_distance", "512", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_medium_scale = gEngfuncs.pfnRegisterVariable("r_shadow_medium_scale", "3", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_texsize = gEngfuncs.pfnRegisterVariable("r_shadow_low_texsize", "1024", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_distance = gEngfuncs.pfnRegisterVariable("r_shadow_low_distance", "1024", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_shadow_low_scale = gEngfuncs.pfnRegisterVariable("r_shadow_low_scale", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	drawshadowmap = false;
	drawshadowscene = false;

	R_ClearShadow();
}

void R_RenderCurrentEntity(void)
{
	int parsecount = ((*cl_parsecount) % 63);
	switch ((*currententity)->model->type)
	{
		case mod_brush:
		{
			R_DrawBrushModel(*currententity);
			break;
		}
		case mod_studio:
		{
			if ((*currententity)->player)
			{
				(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER, R_GetCurrentDrawPlayerState(parsecount));
			}
			else
			{
				if ((*currententity)->curstate.movetype == MOVETYPE_FOLLOW)
				{
					break;
				}

				(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER);
			}

			break;
		}

		default:
		{
			break;
		}
	}
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
		return true;
	}
	else if (ent->model->type == mod_brush)
	{
		//if (ent->curstate.movetype == MOVETYPE_PUSH)
		//	return true;

		return false;
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
	if (!shadow_depthmap_high)
	{
		shadow_depthmap_high_texsize = highSize;
		shadow_depthmap_high = R_GLGenShadowTexture(highSize, highSize);
	}
	else if (shadow_depthmap_high_texsize != highSize)
	{
		shadow_depthmap_high_texsize = highSize;
		R_GLUploadShadowTexture(shadow_depthmap_high, highSize, highSize);
	}

	int mediumSize = R_GetTextureSizePowerOfTwo(r_shadow_medium_texsize->value);
	if (!shadow_depthmap_medium)
	{
		shadow_depthmap_medium_texsize = mediumSize;
		shadow_depthmap_medium = R_GLGenShadowTexture(mediumSize, mediumSize);
	}
	else if (shadow_depthmap_medium_texsize != mediumSize)
	{
		shadow_depthmap_medium_texsize = mediumSize;
		R_GLUploadShadowTexture(shadow_depthmap_medium, mediumSize, mediumSize);
	}

	int lowSize = R_GetTextureSizePowerOfTwo(r_shadow_low_texsize->value);
	if (!shadow_depthmap_low)
	{
		shadow_depthmap_low_texsize = lowSize;
		shadow_depthmap_low = R_GLGenShadowTexture(lowSize, lowSize);
	}
	else if (shadow_depthmap_low_texsize != lowSize)
	{
		shadow_depthmap_low_texsize = lowSize;
		R_GLUploadShadowTexture(shadow_depthmap_low, lowSize, lowSize);
	}

	vec3_t sangles;
	sangles[0] = r_shadow_angle_p->value;
	sangles[1] = r_shadow_angle_y->value;
	sangles[2] = r_shadow_angle_r->value;

	if(s_BackBufferFBO.s_hBackBufferFBO)
	{
		R_PushFrameBuffer();
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		qglDrawBuffer(GL_DEPTH_ATTACHMENT);
	}

	int shadowmapArray[3] = { shadow_depthmap_high, shadow_depthmap_medium, shadow_depthmap_low };
	int texsizeArray[3] = { shadow_depthmap_high_texsize, shadow_depthmap_medium_texsize, shadow_depthmap_low_texsize };
	float scaleArray[3] = { r_shadow_high_scale->value, r_shadow_medium_scale->value, r_shadow_low_scale->value };

	cl_entity_t *visedicts_high[512];
	cl_entity_t *visedicts_medium[512];
	cl_entity_t *visedicts_low[512];
	int numvisedicts_high = 0;
	int numvisedicts_medium = 0; 
	int numvisedicts_low = 0;

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
				if (numvisedicts_low < 512)
				{
					visedicts_low[numvisedicts_low] = cl_visedicts[j];
					numvisedicts_low++;
				}
			}
			else if (distance > r_shadow_high_distance->value)
			{
				if (numvisedicts_medium < 512)
				{
					visedicts_medium[numvisedicts_medium] = cl_visedicts[j];
					numvisedicts_medium++;
				}
			}
			else
			{
				if (numvisedicts_high < 512)
				{
					visedicts_high[numvisedicts_high] = cl_visedicts[j];
					numvisedicts_high++;
				}
			}
		}
	}

	int numvisedictsArray[3] = { numvisedicts_high , numvisedicts_medium, numvisedicts_low };
	cl_entity_t **visedictsArray[3] = { visedicts_high , visedicts_medium, visedicts_low };

	float *projmatrixArray[3] = { shadow_projmatrix_high , shadow_projmatrix_medium, shadow_projmatrix_low };
	float *mvmatrixArray[3] = { shadow_mvmatrix_high , shadow_mvmatrix_medium, shadow_mvmatrix_low };

	qglEnable(GL_POLYGON_OFFSET_FILL);
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

		qglClear(GL_DEPTH_BUFFER_BIT);

		qglColorMask(0, 0, 0, 0);

		drawshadowmap = true;

		//render start

		cl_entity_t *backup_curentity = (*currententity);

		for (int j = 0; j < numvisedictsArray[i]; ++j)
		{
			(*currententity) = visedictsArray[i][j];
			R_RenderCurrentEntity();
		}

		(*currententity) = backup_curentity;

		if (!s_BackBufferFBO.s_hBackBufferFBO)
		{
			qglBindTexture(GL_TEXTURE_2D, shadowmapArray[i]);
			qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, texsizeArray[i], texsizeArray[i], 0);
		}

		drawshadowmap = false;

		qglColorMask(1, 1, 1, 1);
	}

	qglEnable(GL_CULL_FACE);
	qglDisable(GL_POLYGON_OFFSET_FILL);

	if(s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferTex, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferDepthTex, 0);
		R_PopFrameBuffer();
	}
}

void R_DrawVertexArray(brushface_t *face);

void R_DrawSequentialPolyShadow(msurface_t *s)
{
	glpoly_t *p;

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO );
	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));

	p = s->polys;
	brushface_t *face = &r_wsurf.pFaceBuffer[p->flags];
	R_DrawVertexArray(face);

	qglDisableClientState(GL_VERTEX_ARRAY);
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
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
	msurface_t *surf, **mark;
	mleaf_t *pleaf;
	double dot, dot2;

	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != (*r_visframecount))
		return;

	if (R_ShadowLightCullBox(node->minmaxs, node->minmaxs + 3))
		return;

	if (node->contents < 0)
	{
		pleaf = (mleaf_t *)node;

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = (*r_framecount);
				mark++;
			}
			while (--c);
		}
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
					if((*currententity)->curstate.rendermode == kRenderNormal)
						R_DrawSequentialPolyShadow(surf);
				}
			}
		}
	}

	R_RecursiveWorldNodeShadow(node->children[!side]);
}

void R_RenderShadowScenes(void)
{
	if(!r_shadow || !r_shadow->value || !shadow.program)
		return;

	if (!shadow_depthmap_high)
		return;

	if (!shadow_depthmap_medium)
		return;

	if (!shadow_depthmap_low)
		return;

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

	qglGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
	InvertMatrix(mvmatrix, invmvmatrix);

	qglUseProgramObjectARB(shadow.program);
	qglUniform1iARB(shadow.depthmap_high, 0);
	qglUniform1iARB(shadow.depthmap_medium, 1);
	qglUniform1iARB(shadow.depthmap_low, 2);

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
	qglColor4f(0.1, 0.1, 0.1, 0.5);
	 
	//setup texture 0
	GL_SelectTexture(TEXTURE0_SGIS);
	qglEnable(GL_TEXTURE_2D);	
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
	qglBindTexture(GL_TEXTURE_2D, shadow_depthmap_high);

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglLoadMatrixf(bias);
	qglMultMatrixf(shadow_projmatrix_high);
	qglMultMatrixf(shadow_mvmatrix_high);
	qglMultMatrixf(invmvmatrix);
	
	//setup texture1
	GL_EnableMultitexture();
	qglEnable(GL_TEXTURE_2D);
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
	qglBindTexture(GL_TEXTURE_2D, shadow_depthmap_medium);

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglLoadMatrixf(bias);
	qglMultMatrixf(shadow_projmatrix_medium);
	qglMultMatrixf(shadow_mvmatrix_medium);
	qglMultMatrixf(invmvmatrix);

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglEnable(GL_TEXTURE_2D);
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

	drawshadowscene = true;

	R_RecursiveWorldNodeShadow(r_worldmodel->nodes);

	drawshadowscene = false;

	(*currententity) = backup_curentity;

	qglUseProgramObjectARB(0);

	//Restore texture0

	//restore texture 2
	qglActiveTextureARB(TEXTURE2_SGIS);

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();

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

	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);
	qglDisable(GL_TEXTURE_GEN_R);
	qglDisable(GL_TEXTURE_GEN_Q);
	GL_DisableMultitexture();

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();

	//restore texture 0
	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);
	qglDisable(GL_TEXTURE_GEN_R);
	qglDisable(GL_TEXTURE_GEN_Q);

	qglDepthMask(GL_TRUE);
	qglDisable(GL_DEPTH_TEST);

	if (gl_polyoffset->value)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	qglMatrixMode(GL_MODELVIEW);
}