#include "gl_local.h"

//renderer
qboolean drawshadow;
qboolean drawshadowscene;
int shadow_update_counter;
vec3_t shadow_light_mins;
vec3_t shadow_light_maxs;

//shadow
shadowlight_t *cursdlight = NULL;
shadowlight_t sdlights[MAX_SHADOW_LIGHTS];
shadowlight_t *sdlights_active = NULL;
shadowlight_t *sdlights_free = NULL;

shadow_manager_t sdmanagers[MAX_SHADOW_MANAGERS];
shadow_manager_t sdmanager_player;
int numsdmanagers;

//shader
SHADER_DEFINE(shadow);

//cvar
cvar_t *r_shadow = NULL;
cvar_t *r_shadow_debug = NULL;

//bug fix
int g_flPlayerParseCount;

void R_ClearShadow(void)
{
	for(int i = 0; i < MAX_SHADOW_LIGHTS; ++i)
		sdlights[i].next = &sdlights[i+1];
	sdlights[MAX_SHADOW_LIGHTS-1].next = NULL;
	sdlights_free = &sdlights[0];
	sdlights_active = NULL;

	memset(sdmanagers, 0, sizeof(sdmanagers));
	memset(&sdmanager_player, 0, sizeof(sdmanager_player));
	numsdmanagers = 0;

	sdmanager_player.radius = 256;
	sdmanager_player.fard = 64;
	sdmanager_player.scale = 8;
	sdmanager_player.texsize = 512;
	sdmanager_player.angles[0] = 90;
	sdmanager_player.angles[1] = 0;
	sdmanager_player.angles[2] = 0;
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
				SHADER_UNIFORM(shadow, texoffset, "texoffset");
				SHADER_UNIFORM(shadow, depthmap, "depthmap");
				SHADER_UNIFORM(shadow, entorigin, "entorigin");
				SHADER_UNIFORM(shadow, radius, "radius");
				SHADER_UNIFORM(shadow, fard, "fard");
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
	r_shadow_debug = gEngfuncs.pfnRegisterVariable("r_shadow_debug", "0", FCVAR_CLIENTDLL);

	shadow_update_counter = 0;
	drawshadow = false;
	drawshadowscene = false;
	cursdlight = NULL;

	memset(sdlights, 0, sizeof(sdlights));
	R_ClearShadow();
}

void R_GLUploadShadowTexture(int texid, int w, int h)
{
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
}

GLuint R_GLGenShadowTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	R_GLUploadShadowTexture(texid, w, h);
	return texid;
}

void R_FreeDeadShadowLights(void)
{
	shadowlight_t *kill;
	shadowlight_t *p;

	for ( ;; )
	{
		kill = sdlights_active;

		if (kill && kill->free)
		{
			sdlights_active = kill->next;
			kill->next = sdlights_free;
			sdlights_free = kill;
			continue;
		}

		break;
	}

	for (p = sdlights_active; p; p = p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->free)
			{
				p->next = kill->next;
				kill->next = sdlights_free;
				sdlights_free = kill;
				continue;
			}
			break;
		}
	}
}

shadowlight_t *R_FindShadowLight(cl_entity_t *entity)
{
	for(shadowlight_t *sl = sdlights_active; sl; sl = sl->next)
	{
		if(sl->followent == entity)
			return sl;
	}
	return NULL;
}

void R_AddEntityShadow(cl_entity_t *ent, const char *model)
{
	if (!model)
		return;

	if (!R_ShouldCastShadow(ent))
		return;

	if(ent->player)
	{
		shadowlight_t *sl = R_FindShadowLight(ent);
		if(!sl)
		{
			shadow_manager_t *sm = R_FindPlayerShadowManager();
			if(sm)
			{
				R_CreateShadowLight(ent, sm->angles, sm->radius, sm->fard, sm->scale, sm->texsize);
			}
		}
		else
		{
			sl->free = false;
		}
	}
	else
	{
		shadowlight_t *sl = R_FindShadowLight(ent);
		if(!sl)
		{
			shadow_manager_t *sm = R_FindShadowManager(model);
			if(sm)
			{
				R_CreateShadowLight(ent, sm->angles, sm->radius, sm->fard, sm->scale, sm->texsize);
			}
			else
			{
				vec3_t ang = {100, 0, 0};
				R_CreateShadowLight(ent, ang, 256, 64, 8, 512);
			}
		}
		else
		{
			sl->free = false;
		}
	}
}

void R_CreateShadowLight(cl_entity_t *entity, vec3_t angles, float radius, float fard, float scale, int texsize)
{
	if(!sdlights_free)
	{
		gEngfuncs.Con_DPrintf("R_CreateShadowLight: Overflow %d shadow lights!\n", MAX_SHADOW_LIGHTS);
		return;
	}

	if(!entity)
	{
		gEngfuncs.Con_DPrintf("R_CreateShadowLight: Null Entity!\n");
		return;
	}

	cursdlight = sdlights_free;
	sdlights_free = cursdlight->next;

	cursdlight->next = sdlights_active;
	sdlights_active = cursdlight;

	cursdlight->followent = entity;
	VectorCopy(angles, cursdlight->angles);
	AngleVectors(angles, cursdlight->vforward, cursdlight->vright, cursdlight->vup);

	int scaled_texsize;
	for (scaled_texsize = 1; scaled_texsize < texsize; scaled_texsize <<= 1) {}
	if (gl_round_down->value > 0 && texsize < scaled_texsize && (gl_round_down->value == 1 || (scaled_texsize - texsize) > (scaled_texsize >> (int)gl_round_down->value)))
		scaled_texsize >>= 1;
	int max_size = max(128, gl_max_texture_size);
	if(!s_ShadowFBO.s_hBackBufferFBO && max_size > 512)//glCopyTexImage2D fix
		max_size = 512;
	scaled_texsize = max(min(scaled_texsize >> (int)gl_picmip->value, max_size), 64);

	cursdlight->radius = min(max(radius, 0), 1000);
	cursdlight->fard = min(max(fard, 0), cursdlight->radius);
	cursdlight->scale = min(max(scale, 1), 64);
	cursdlight->free = false;
	if(!cursdlight->depthmap)
	{
		cursdlight->texsize = scaled_texsize;
		cursdlight->depthmap = R_GLGenShadowTexture(cursdlight->texsize, cursdlight->texsize);
	}
	else
	{
		if(scaled_texsize != cursdlight->texsize)
		{
			cursdlight->texsize = scaled_texsize;
			R_GLUploadShadowTexture(cursdlight->depthmap, cursdlight->texsize, cursdlight->texsize);
		}
	}

	cursdlight = NULL;
}

#define CURRENT_DRAW_PLAYER_STATE ((entity_state_t *)( (char *)cl_frames + size_of_frame * parsecount + sizeof(entity_state_t) * (*currententity)->index) )

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
				(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RENDER, CURRENT_DRAW_PLAYER_STATE );
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

void R_RenderShadowMap(void)
{
	VectorCopy(cursdlight->followent->origin, cursdlight->origin);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	
	int scaled_texsize = cursdlight->texsize / cursdlight->scale;
	qglOrtho(-scaled_texsize, scaled_texsize, -scaled_texsize, scaled_texsize, -cursdlight->radius, cursdlight->radius);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglRotatef(-90, 1, 0, 0);
	qglRotatef(90, 0, 0, 1);
	qglRotatef(-cursdlight->angles[2], 1, 0, 0);
	qglRotatef(-cursdlight->angles[0], 0, 1, 0);
	qglRotatef(-cursdlight->angles[1], 0, 0, 1);
	qglTranslatef(-cursdlight->origin[0], -cursdlight->origin[1], -cursdlight->origin[2]);

	qglGetFloatv(GL_PROJECTION_MATRIX, cursdlight->projmatrix);
	qglGetFloatv(GL_MODELVIEW_MATRIX, cursdlight->mvmatrix);

	qglViewport(0, 0, cursdlight->texsize, cursdlight->texsize);

	if(s_ShadowFBO.s_hBackBufferFBO)
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, cursdlight->depthmap, 0);

	qglClear(GL_DEPTH_BUFFER_BIT);
	
	//render start

	cl_entity_t *curentity = (*currententity);
	(*currententity) = cursdlight->followent;
	//int iSaveRenderFx = (*currententity)->curstate.renderfx;
	//(*currententity)->curstate.renderfx = kRenderFxShadow;

	R_RenderCurrentEntity();

	//(*currententity)->curstate.renderfx = iSaveRenderFx;
	(*currententity) = curentity;

	if(!s_ShadowFBO.s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, cursdlight->depthmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, cursdlight->texsize, cursdlight->texsize, 0);
	}
}

qboolean R_ShouldCastShadow(cl_entity_t *ent)
{
	if(!ent)
		return false;

	if(!ent->model)
		return false;

	if (ent->curstate.effects & EF_NODRAW)
		return false;

	if (ent->curstate.rendermode != kRenderNormal)
		return false;

	if (ent->model->type == mod_studio)
	{
		return true;
	}
	else if (ent->model->type == mod_brush)
	{
		if (ent->curstate.movetype == MOVETYPE_NONE)
			return false;

		return true;
	}

	return false;
}

void R_RenderShadowMaps(void)
{
	if(s_ShadowFBO.s_hBackBufferFBO)
	{
		R_PushFrameBuffer();
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_ShadowFBO.s_hBackBufferFBO);
	}

	qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	qglPolygonOffset(1.0f, 4096.0f);
	qglEnable(GL_POLYGON_OFFSET_FILL);
	qglDisable(GL_CULL_FACE);
	qglEnable(GL_TEXTURE_2D);

	r_refdef->onlyClientDraws = true;
	drawshadow = true;

	for (shadowlight_t *sl = sdlights_active; sl; sl = sl->next)
	{
		if (!R_ShouldCastShadow(sl->followent))
		{
			sl->free = true;
			continue;
		}

		cursdlight = sl;

		R_RenderShadowMap();

		cursdlight = NULL;
	}

	r_refdef->onlyClientDraws = false;
	drawshadow = false;

	qglEnable(GL_CULL_FACE);
	qglDisable(GL_POLYGON_OFFSET_FILL);
	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	if(s_ShadowFBO.s_hBackBufferFBO)
	{
		R_PopFrameBuffer();
	}

	R_FreeDeadShadowLights();
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
			dot2 = cursdlight->origin[0] - plane->dist;
			break;
		}

		case PLANE_Y:
		{
			dot = r_refdef->vieworg[1] - plane->dist;
			dot2 = cursdlight->origin[1] - plane->dist;
			break;
		}

		case PLANE_Z:
		{
			dot = r_refdef->vieworg[2] - plane->dist;
			dot2 = cursdlight->origin[2] - plane->dist;
			break;
		}

		default:
		{
			dot = DotProduct(r_refdef->vieworg, plane->normal) - plane->dist;
			dot2 = DotProduct(cursdlight->origin, plane->normal) - plane->dist;
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
		{
			for ( ; c; c--, surf++)
			{
				if (surf->visframe != (*r_framecount))
					continue;

				if (!(surf->flags & SURF_UNDERWATER) && ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
					continue;

				if (dot2 > cursdlight->radius)
					continue;

				if (dot2 < 0)
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

void R_RenderAllShadowScenes(void)
{
	if(!r_shadow->value || !shadow.program)
		return;

	if(!sdlights_active)
		return;

	const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f, 
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f	};

	float mvmatrix[16];
	float invmvmatrix[16];	

	qglGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
	InvertMatrix(mvmatrix, invmvmatrix);

	GLfloat planeS[] = {1.0, 0.0, 0.0, 0.0};
	GLfloat planeT[] = {0.0, 1.0, 0.0, 0.0};
	GLfloat planeR[] = {0.0, 0.0, 1.0, 0.0};
	GLfloat planeQ[] = {0.0, 0.0, 0.0, 1.0};

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

	if (gl_polyoffset && gl_polyoffset->value)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);

		if (gl_ztrick && gl_ztrick->value)
			qglPolygonOffset(1, gl_polyoffset->value);
		else
			qglPolygonOffset(-1, -gl_polyoffset->value);
	}

	qglDepthMask(GL_FALSE);
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglColor4f(0.1, 0.1, 0.1, 0.5);

	qglUseProgramObjectARB(shadow.program);
	qglUniform1iARB(shadow.depthmap, 0);

	GL_DisableMultitexture();

	cl_entity_t *curentity = (*currententity);
	(*currententity) = r_worldentity;

	drawshadowscene = true;

	for(shadowlight_t *slight = sdlights_active; slight; slight = slight->next)
	{
		if(slight->free)
			continue;

		cursdlight = slight;

		qglUniform1fARB(shadow.texoffset, 1 / cursdlight->texsize);
		qglUniform1fARB(shadow.radius, cursdlight->radius);
		qglUniform1fARB(shadow.fard, cursdlight->fard);
		qglUniform3fvARB(shadow.entorigin, 1, cursdlight->origin);

		GL_SelectTexture(TEXTURE0_SGIS);
		qglBindTexture(GL_TEXTURE_2D, cursdlight->depthmap);

		qglMatrixMode(GL_TEXTURE);
		qglLoadIdentity();
		qglLoadMatrixf(bias);
		qglMultMatrixf(cursdlight->projmatrix);
		qglMultMatrixf(cursdlight->mvmatrix);
		qglMultMatrixf(invmvmatrix);
		qglMatrixMode(GL_MODELVIEW);

		shadow_light_mins[0] = cursdlight->origin[0] - cursdlight->radius;
		shadow_light_mins[1] = cursdlight->origin[1] - cursdlight->radius;
		shadow_light_mins[2] = cursdlight->origin[2] - cursdlight->radius;

		shadow_light_maxs[0] = cursdlight->origin[0] + cursdlight->radius;
		shadow_light_maxs[1] = cursdlight->origin[1] + cursdlight->radius;
		shadow_light_maxs[2] = cursdlight->origin[2] + cursdlight->radius;

		R_RecursiveWorldNodeShadow(r_worldmodel->nodes);

		cursdlight = NULL;
	}

	drawshadowscene = false;

	(*currententity) = curentity;

	qglUseProgramObjectARB(0);

	qglDepthMask(GL_TRUE);
	//qglDisable(GL_BLEND);

	if (gl_polyoffset->value)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	//Restore texture0
	GL_SelectTexture(TEXTURE0_SGIS);
	qglBindTexture(GL_TEXTURE_2D, 0);

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();

	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);
	qglDisable(GL_TEXTURE_GEN_R);
	qglDisable(GL_TEXTURE_GEN_Q);

	qglMatrixMode(GL_MODELVIEW);

	GL_EnableMultitexture();
}

shadow_manager_t *R_FindShadowManager(const char *affectmodel)
{
	for(int i = 0; i < numsdmanagers; ++i)
	{
		if(!stricmp(affectmodel, sdmanagers[i].affectmodel))
		{
			return &sdmanagers[i];
		}
	}
	return NULL;
}

shadow_manager_t *R_FindPlayerShadowManager(void)
{
	return &sdmanager_player;
}

void R_CreateShadowManager(char *affectmodel, vec3_t angles, float radius, float fard, float scale, int texsize)
{
	shadow_manager_t *sm;
	if(!stricmp(affectmodel, "player"))
	{
		sm = &sdmanager_player;
	}
	else
	{
		if(numsdmanagers >= MAX_SHADOW_MANAGERS)
		{
			gEngfuncs.Con_DPrintf("R_CreateShadowManager: Overflow %d shadow managers!\n", MAX_SHADOW_MANAGERS);
			return;
		}
		sm = &sdmanagers[numsdmanagers];
	}
	strcpy(sm->affectmodel, affectmodel);
	VectorCopy(angles, sm->angles);
	sm->radius = radius;
	sm->fard = fard;
	sm->scale = scale;
	sm->texsize = texsize;
	numsdmanagers ++;
}