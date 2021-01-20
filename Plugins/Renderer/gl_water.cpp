#include "gl_local.h"

//renderer
qboolean drawreflect;
qboolean drawrefract;
vec3_t water_view;
mplane_t custom_frustum[4];
int water_update_counter;
int water_texture_size = 0;

//water
r_water_t waters[MAX_WATERS];
r_water_t *curwater;
r_water_t *waters_free;
r_water_t *waters_active;

//shader
SHADER_DEFINE(water);

int water_normalmap;
int water_normalmap_default;

SHADER_DEFINE(drawdepth);

int save_userfogon;
int waterfog_on;
int *g_bUserFogOn;

water_parm_t water_parm;
water_parm_t default_water_parm = { true, {64.0f/255, 80.0f/255, 90.0f/255, 51.0f/255}, 100, 3000, 1, 1, false };

//cvar
cvar_t *r_water = NULL;
cvar_t *r_water_debug = NULL;
cvar_t *r_water_fresnel = NULL;

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);

void R_SetWaterParm(water_parm_t *parm)
{
	water_parm.fog = parm->fog ? 1 : 0;
	water_parm.start = max(parm->start, 0);
	water_parm.end = max(parm->end, 0);
	water_parm.density = clamp(parm->density, 0, 1);
	water_parm.fresnel = clamp(parm->fresnel, 0, 1);
	water_parm.color[0] = clamp(parm->color[0], 0, 1);
	water_parm.color[1] = clamp(parm->color[1], 0, 1);
	water_parm.color[2] = clamp(parm->color[2], 0, 1);
	water_parm.color[3] = clamp(parm->color[3], 0, 1);
	water_parm.active = parm->active;
}

void R_RenderWaterFog(void) 
{
	if(water_parm.fog)
	{
		/*save_userfogon = *g_bUserFogOn;
		*g_bUserFogOn = 1;
		waterfog_on = 1;
		qglEnable(GL_FOG);
		qglFogi(GL_FOG_MODE, GL_LINEAR);
		qglFogf(GL_FOG_DENSITY, water_parm.density);
		qglHint(GL_FOG_HINT, GL_NICEST);

		qglFogf(GL_FOG_START, water_parm.start);
		qglFogf(GL_FOG_END, (*cl_waterlevel > 2) ? water_parm.end / 4 : water_parm.end);
		qglFogfv(GL_FOG_COLOR, water_parm.color);

		qglFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);*/
	}
	else
	{
		waterfog_on = 0;
	}
}

void R_FinalWaterFog(void) 
{
	if(waterfog_on)
	{
		waterfog_on = 0;
		//*g_bUserFogOn = save_userfogon;
		//if(!save_userfogon)
		//	qglDisable(GL_FOG);
	}
}

void R_ClearWater(void)
{
	for(int i = 0; i < MAX_WATERS; ++i)
		waters[i].next = &waters[i+1];
	waters[MAX_WATERS-1].next = NULL;
	waters_free = &waters[0];
	waters_active = NULL;

	memcpy(&water_parm, &default_water_parm, sizeof(water_parm));
}

const char *drawdepth_vscode = 
"varying vec4 projpos;"
"void main(void)"
"{"
"	projpos = gl_ModelViewProjectionMatrix * gl_Vertex;"
"	gl_Position = ftransform();"
"}";

const char *drawdepth_fscode = 
"uniform sampler2D depthmap;"
"varying vec4 projpos;"
"void main(void)"
"{"
"	vec2 vBaseTexCoord = projpos.xy / projpos.w * 0.5 + 0.5;"
"	float flDepth = texture2D(depthmap, vBaseTexCoord);"
"	gl_FragColor = vec4(vec3(pow(flDepth, 50.0)), 1.0);"
"}";

void R_InitWater(void)
{
	if(gl_shader_support)
	{
		const char *water_vscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\water_shader.vsh", 5, 0);
		const char *water_fscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\water_shader.fsh", 5, 0);
		if(water_vscode && water_fscode)
		{
			water.program = R_CompileShader(water_vscode, water_fscode, "water_shader.vsh", "water_shader.fsh");
			if(water.program)
			{
				SHADER_UNIFORM(water, waterfogcolor, "waterfogcolor");
				SHADER_UNIFORM(water, eyepos, "eyepos");
				SHADER_UNIFORM(water, eyedir, "eyedir");
				SHADER_UNIFORM(water, zmax, "zmax");
				SHADER_UNIFORM(water, time, "time");
				SHADER_UNIFORM(water, fresnel, "fresnel");
				SHADER_UNIFORM(water, abovewater, "abovewater");
				
				SHADER_UNIFORM(water, normalmap, "normalmap");
				SHADER_UNIFORM(water, refractmap, "refractmap");
				SHADER_UNIFORM(water, reflectmap, "reflectmap");
				SHADER_UNIFORM(water, depthrefrmap, "depthrefrmap");
			}
		}
		gEngfuncs.COM_FreeFile((void *)water_vscode);
		gEngfuncs.COM_FreeFile((void *)water_fscode);

		if(drawdepth_vscode && drawdepth_fscode)
		{
			drawdepth.program = R_CompileShader(drawdepth_vscode, drawdepth_fscode, "drawdepth_shader.vsh", "drawdepth_shader.fsh");
			if(drawdepth.program)
			{
				SHADER_UNIFORM(drawdepth, depthmap, "depthmap");
			}
		}
	}

	water_normalmap_default = R_LoadTextureEx("resource\\tga\\water_normalmap.tga", "resource\\tga\\water_normalmap.tga", NULL, NULL, GLT_SYSTEM, false, false);
	water_normalmap = water_normalmap_default;

	if(!water_texture_size)//don't support FBO?
		water_texture_size = 512;

	r_water = gEngfuncs.pfnRegisterVariable("r_water", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_debug = gEngfuncs.pfnRegisterVariable("r_water_debug", "0", FCVAR_CLIENTDLL);
	r_water_fresnel = gEngfuncs.pfnRegisterVariable("r_water_fresnel", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	water_update_counter = 0;
	curwater = NULL;
	drawreflect = false;
	drawrefract = false;

	R_ClearWater();
}

int R_ShouldReflect(void) 
{
	if(curwater->vecs[2] > r_refdef->vieworg[2])
		return 0;

	for(r_water_t *w = waters_active; w; w = w->next)
	{
		if(w->ent && curwater->ent != w->ent)
		{
			if(fabs(w->vecs[2] - curwater->vecs[2]) < 1.0f)
				return 0;
		}
	}
	return 1;
}

void R_AddWater(cl_entity_t *ent, vec3_t p)
{
	r_water_t *w;

	curwater = NULL;

	for (w = waters_active; w; w = w->next)
	{
		if (w->ent == ent || fabs(w->vecs[2] - p[2]) < 1.0f)
		{
			//found one
			curwater = w;
			return;
		}
	}

	//no free water slot
	if (!waters_free)
		return;

	//not found, try to create
	curwater = waters_free;
	waters_free = curwater->next;

	curwater->next = waters_active;
	waters_active = curwater;

	if (!curwater->reflectmap)
		curwater->reflectmap = R_GLGenTexture(water_texture_size, water_texture_size);

	if (!curwater->refractmap)
		curwater->refractmap = R_GLGenTexture(water_texture_size, water_texture_size);

	if (!curwater->depthrefrmap)
		curwater->depthrefrmap = R_GLGenDepthTexture(water_texture_size, water_texture_size);

	GL_Bind(0);

	VectorCopy(p, curwater->vecs);
	curwater->ent = ent;
	curwater->org[0] = (ent->curstate.mins[0] + ent->curstate.maxs[0]) / 2;
	curwater->org[1] = (ent->curstate.mins[1] + ent->curstate.maxs[1]) / 2;
	curwater->org[2] = (ent->curstate.mins[2] + ent->curstate.maxs[2]) / 2;
	curwater->is3dsky = (draw3dsky) ? true : false;
	curwater = NULL;
}

void R_SetCustomFrustum(void)
{
	int i;

	if (scr_fov_value == 90) 
	{
		VectorAdd(vpn, vright, custom_frustum[0].normal);
		VectorSubtract(vpn, vright, custom_frustum[1].normal);

		VectorAdd(vpn, vup, custom_frustum[2].normal);
		VectorSubtract(vpn, vup, custom_frustum[3].normal);
	}
	else
	{
		RotatePointAroundVector(custom_frustum[0].normal, vup, vpn, -(90 - scr_fov_value / 2));
		RotatePointAroundVector(custom_frustum[1].normal, vup, vpn, 90 - scr_fov_value / 2);
		RotatePointAroundVector(custom_frustum[2].normal, vright, vpn, 90 - yfov / 2);
		RotatePointAroundVector(custom_frustum[3].normal, vright, vpn, -(90 - yfov / 2));
	}

	for (i = 0; i < 4; i++)
	{
		custom_frustum[i].type = PLANE_ANYZ;
		custom_frustum[i].dist = DotProduct(r_origin, custom_frustum[i].normal);
		custom_frustum[i].signbits = SignbitsForPlane(&custom_frustum[i]);
	}
}

void R_SetupClip(qboolean isdrawworld)
{
	double clipPlane[] = {0, 0, 0, 0};

	if(drawreflect)
	{
		if(water_view[2] < curwater->vecs[2])
		{
			clipPlane[2] = -1.0;
			clipPlane[3] = curwater->vecs[2];
		}
		else
		{
			clipPlane[2] = 1.0;
			clipPlane[3] = -curwater->vecs[2];
		}
	}
	if(drawrefract)
	{
		if (water_view[2] < curwater->vecs[2])
		{
			clipPlane[2] = 1.0;
			clipPlane[3] = -curwater->vecs[2];
		}
		else
		{
			clipPlane[2] = -1.0;
			clipPlane[3] = curwater->vecs[2];
		}
	}
	if(isdrawworld)
	{
		clipPlane[3] += 24;
	}
	qglEnable(GL_CLIP_PLANE0);
	qglClipPlane(GL_CLIP_PLANE0, clipPlane);
}

void R_SetupReflect(void)
{
	R_PushRefDef();

	if(curwater->is3dsky)
	{
		VectorCopy(_3dsky_view, water_view);
	}
	else
	{
		VectorCopy(r_refdef->vieworg, water_view);
	}

	VectorCopy(water_view, r_refdef->vieworg);

	r_refdef->vieworg[2] = (2 * curwater->vecs[2]) - r_refdef->vieworg[2];
	r_refdef->viewangles[0] = -r_refdef->viewangles[0];
	r_refdef->viewangles[2] = -r_refdef->viewangles[2];

	R_UpdateRefDef();

	++(*r_framecount);
	*r_oldviewleaf = *r_viewleaf;

	*r_viewleaf = Mod_PointInLeaf(water_view, r_worldmodel);

	float flNoVIS = r_novis->value;
	r_novis->value = 1;

	R_SetCustomFrustum();
	R_SetupGL();
	R_MarkLeaves();

	r_novis->value = flNoVIS;

	r_refdef->viewangles[2] = -r_refdef->viewangles[2];
	R_UpdateRefDef();

	qglViewport(0, 0, water_texture_size, water_texture_size);

	drawreflect = true;

	R_SetupClip(true);

	R_RenderWaterFog();

	if(s_WaterFBO.s_hBackBufferFBO)
	{
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curwater->reflectmap, 0);
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_WaterFBO.s_hBackBufferDB);
	}

	qglClearColor(water_parm.color[0], water_parm.color[1], water_parm.color[2], 1);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void R_FinishReflect(void)
{
	R_FinalWaterFog();

	qglDisable(GL_CLIP_PLANE0);

	if(!s_WaterFBO.s_hBackBufferFBO)
	{
		GL_Bind(curwater->reflectmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, water_texture_size, water_texture_size, 0);
	}

	R_PopRefDef();

	drawreflect = false;
}

void R_SetupRefract(void)
{
	R_PushRefDef();

	if(curwater->is3dsky)
	{
		VectorCopy(_3dsky_view, water_view);
	}
	else
	{
		VectorCopy(r_refdef->vieworg, water_view);
	}

	VectorCopy(water_view, r_refdef->vieworg);

	R_UpdateRefDef();

	++(*r_framecount);
	*r_oldviewleaf = *r_viewleaf;
	*r_viewleaf = Mod_PointInLeaf(water_view, r_worldmodel);

	float flNoVIS = r_novis->value;
	r_novis->value = 1;

	R_SetCustomFrustum();
	R_SetupGL();
	R_MarkLeaves();

	r_novis->value = flNoVIS;

	qglViewport(0, 0, water_texture_size, water_texture_size);

	drawrefract = true;

	R_SetupClip(true);

	R_RenderWaterFog();

	if(s_WaterFBO.s_hBackBufferFBO)
	{
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curwater->refractmap, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curwater->depthrefrmap, 0);
	}

	qglClearColor(water_parm.color[0], water_parm.color[1], water_parm.color[2], 1);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void R_FinishRefract(void)
{
	R_FinalWaterFog();

	qglDisable(GL_CLIP_PLANE0);

	if(!s_WaterFBO.s_hBackBufferFBO)
	{
		GL_Bind(curwater->refractmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, water_texture_size, water_texture_size, 0);

		GL_Bind(curwater->depthrefrmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, water_texture_size, water_texture_size, 0);
	}

	R_PopRefDef();

	drawrefract = false;
}

void R_UpdateWater(void)
{
	int currentframebuffer = 0;
	if(s_WaterFBO.s_hBackBufferFBO)
	{
		qglGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentframebuffer);
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
	}

	for(r_water_t *w = waters_active; w; w = w->next)
	{
		curwater = w;

		if(R_ShouldReflect())
		{
			R_SetupReflect();
			R_ClearSkyBox();
			R_DrawWorld();
			R_SetupClip(false);
			if(!curwater->is3dsky)
			{
				R_DrawEntitiesOnList();
				R_DrawTEntitiesOnList(0);
			}
			else
			{
				R_Draw3DSkyEntities();
			}
			R_FinishReflect();
		}

		R_SetupRefract();
		R_ClearSkyBox();
		R_DrawWorld();
		R_SetupClip(false);
		if(!curwater->is3dsky)
		{
			R_DrawEntitiesOnList();
			R_DrawTEntitiesOnList(0);
		}
		else
		{
			R_Draw3DSkyEntities();
		}
		R_FinishRefract();
		curwater = NULL;
	}

	if(s_WaterFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, currentframebuffer);
	}
}