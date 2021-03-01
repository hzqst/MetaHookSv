#include "gl_local.h"
#include <sstream>

//renderer
qboolean drawreflect;
qboolean drawrefract;
vec3_t water_view;

//water
r_water_t waters[MAX_WATERS];
r_water_t *curwater;
r_water_t *waters_free;
r_water_t *waters_active;

//shader

int water_normalmap = 0;

SHADER_DEFINE(drawdepth);

int save_userfogon;
int *g_bUserFogOn;

int saved_cl_waterlevel;

//cvar
cvar_t *r_water = NULL;
cvar_t *r_water_debug = NULL;
cvar_t *r_water_fresnel = NULL;
cvar_t *r_water_depthfactor = NULL;
cvar_t *r_water_normfactor = NULL;
cvar_t *r_water_novis = NULL;
cvar_t *r_water_texscale = NULL;
cvar_t *r_water_minheight = NULL;

std::unordered_map<int, water_program_t> g_WaterProgramTable;

void R_UseWaterProgram(int state, water_program_t *progOutput)
{
	water_program_t prog = { 0 };

	auto itor = g_WaterProgramTable.find(state);
	if (itor == g_WaterProgramTable.end())
	{
		std::stringstream defs;

		if (state & WATER_UNDERWATER_ENABLED)
			defs << "#define UNDERWATER_ENABLED\n";

		if (state & WATER_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("resource\\shader\\water_shader.vsh", NULL, "resource\\shader\\water_shader.fsh", def.c_str(), NULL, def.c_str());
		if (prog.program)
		{
			SHADER_UNIFORM(prog, waterfogcolor, "waterfogcolor");
			SHADER_UNIFORM(prog, eyepos, "eyepos");
			SHADER_UNIFORM(prog, entitymatrix, "entitymatrix");
			SHADER_UNIFORM(prog, zmax, "zmax");
			SHADER_UNIFORM(prog, time, "time");
			SHADER_UNIFORM(prog, fresnel, "fresnel");
			SHADER_UNIFORM(prog, depthfactor, "depthfactor");
			SHADER_UNIFORM(prog, normfactor, "normfactor");
			SHADER_UNIFORM(prog, normalmap, "normalmap");
			SHADER_UNIFORM(prog, refractmap, "refractmap");
			SHADER_UNIFORM(prog, reflectmap, "reflectmap");
			SHADER_UNIFORM(prog, depthrefrmap, "depthrefrmap");
		}

		g_WaterProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		qglUseProgramObjectARB(prog.program);

		if (prog.normalmap != -1)
			qglUniform1iARB(prog.normalmap, 0);
		if (prog.refractmap != -1)
			qglUniform1iARB(prog.refractmap, 1);
		if (prog.reflectmap != -1)
			qglUniform1iARB(prog.reflectmap, 2);
		if (prog.depthrefrmap != -1)
			qglUniform1iARB(prog.depthrefrmap, 3);
		if (prog.entitymatrix != -1)
		{
			if (r_rotate_entity)
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, false, r_rotate_entity_matrix);
			else
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, false, r_identity_matrix);
		}

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseWaterProgram: Failed to load program!");
	}
}

void R_FreeWater(void)
{
	for (int i = 0; i < MAX_WATERS; ++i)
	{
		if (waters[i].depthreflmap)
		{
			GL_DeleteTexture(waters[i].depthreflmap);
			waters[i].depthreflmap = 0;
		}
		if (waters[i].depthrefrmap)
		{
			GL_DeleteTexture(waters[i].depthrefrmap);
			waters[i].depthrefrmap = 0;
		}
		if (waters[i].reflectmap)
		{
			GL_DeleteTexture(waters[i].reflectmap);
			waters[i].reflectmap = 0;
		}
		if (waters[i].refractmap)
		{
			GL_DeleteTexture(waters[i].refractmap);
			waters[i].refractmap = 0;
		}
		waters[i].reflectmap_ready = false;
		waters[i].refractmap_ready = false;
	}

	g_WaterProgramTable.clear();
}

void R_ClearWater(void)
{
	for(int i = 0; i < MAX_WATERS - 1; ++i)
		waters[i].next = &waters[i+1];
	waters[MAX_WATERS-1].next = NULL;
	waters_free = &waters[0];
	waters_active = NULL;
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
"	vec4 vDepthColor = texture2D(depthmap, vBaseTexCoord);"
"	gl_FragColor = vec4(vec3(pow(vDepthColor.z, 50.0)), 1.0);"
"}";

void R_InitWater(void)
{
	if(gl_shader_support)
	{
		drawdepth.program = R_CompileShader(drawdepth_vscode, NULL, drawdepth_fscode, "drawdepth_shader.vsh", NULL, "drawdepth_shader.fsh");
		if (drawdepth.program)
		{
			SHADER_UNIFORM(drawdepth, depthmap, "depthmap");
		}
	}

	r_water = gEngfuncs.pfnRegisterVariable("r_water", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_debug = gEngfuncs.pfnRegisterVariable("r_water_debug", "0", FCVAR_CLIENTDLL);
	r_water_fresnel = gEngfuncs.pfnRegisterVariable("r_water_fresnel", "1.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_depthfactor = gEngfuncs.pfnRegisterVariable("r_water_depthfactor", "50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_normfactor = gEngfuncs.pfnRegisterVariable("r_water_normfactor", "1.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_novis = gEngfuncs.pfnRegisterVariable("r_water_novis", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_texscale = gEngfuncs.pfnRegisterVariable("r_water_texscale", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_minheight = gEngfuncs.pfnRegisterVariable("r_water_minheight", "7.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	curwater = NULL;
	drawreflect = false;
	drawrefract = false;

	R_ClearWater();
}

int R_ShouldReflect(void) 
{
	//The camera is above water?
	if(curwater->vecs[2] > r_refdef->vieworg[2] || *cl_waterlevel >= 3)
		return 0;

	return 1;
}

r_water_t *R_GetActiveWater(cl_entity_t *ent, vec3_t p, colorVec *color)
{
	r_water_t *w;

	for (w = waters_active; w; w = w->next)
	{
		if ((ent != r_worldentity && w->ent == ent) || (fabs(w->vecs[2] - p[2]) < 1.0f) )
		{
			//found one
			VectorCopy(p, w->vecs);
			w->free = false;
			w->framecount = (*r_framecount);
			return w;
		}
	}

	//No free water slot
	if (!waters_free)
	{
		gEngfuncs.Con_Printf("R_ActivateWater: MAX_WATER exceeded!");
		return NULL;
	}

	//Get one from free list
	w = waters_free;
	waters_free = w->next;

	//Link to active list
	w->next = waters_active;
	waters_active = w;

	int water_texture_width = glwidth;
	int water_texture_height = glheight;

	int desired_height = glheight * clamp(r_water_texscale->value, 0.1, 2.0);
	while ((water_texture_height >> 1) >= desired_height)
	{
		water_texture_width >>= 1;
		water_texture_height >>= 1;
	}

	//Load if normalmap not exists.
	if (!water_normalmap)
		water_normalmap = R_LoadTexture("resource\\tga\\water_normalmap.tga", "resource\\tga\\water_normalmap.tga", NULL, NULL, GLT_SYSTEM);

	//Upload color textures and depth textures.
	if (!w->reflectmap)
	{
		w->reflectmap = GL_GenTextureRGBA8(water_texture_width, water_texture_height);
	}
	else if (w->texwidth != water_texture_width)
	{
		GL_UploadTextureColorFormat(w->reflectmap, water_texture_width, water_texture_height, GL_RGBA8);
	}

	if (!w->refractmap)
	{
		w->refractmap = GL_GenTextureRGBA8(water_texture_width, water_texture_height);
	}
	else if (w->texwidth != water_texture_width)
	{
		GL_UploadTextureColorFormat(w->refractmap, water_texture_width, water_texture_height, GL_RGBA8);
	}

	if (!w->depthrefrmap)
	{
		w->depthrefrmap = GL_GenDepthTexture(water_texture_width, water_texture_height);
	}
	else if (w->texwidth != water_texture_width)
	{
		GL_UploadDepthTexture(w->depthrefrmap, water_texture_width, water_texture_height);
	}

	if (!w->depthreflmap)
	{
		w->depthreflmap = GL_GenDepthTexture(water_texture_width, water_texture_height);
	}
	else if (w->texwidth != water_texture_width)
	{
		GL_UploadDepthTexture(w->depthreflmap, water_texture_width, water_texture_height);
	}

	w->texwidth = water_texture_width;
	w->texheight = water_texture_height;

	w->reflectmap_ready = false;
	w->refractmap_ready = false;

	VectorCopy(p, w->vecs);
	w->ent = ent;
	w->org[0] = (ent->curstate.mins[0] + ent->curstate.maxs[0]) / 2;
	w->org[1] = (ent->curstate.mins[1] + ent->curstate.maxs[1]) / 2;
	w->org[2] = (ent->curstate.mins[2] + ent->curstate.maxs[2]) / 2;
	memcpy(&w->color, color, sizeof(*color));
	w->is3dsky = (draw3dsky) ? true : false;
	w->free = false;
	w->framecount = (*r_framecount);
	return w;
}

void R_EnableClip(qboolean isdrawworld)
{
	double clipPlane[] = {0, 0, 0, 0};

	if(drawreflect)
	{
		if(saved_cl_waterlevel > 2)
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
		return;

		if (saved_cl_waterlevel > 2)
		{
			clipPlane[2] = 1.0;
			clipPlane[3] = curwater->vecs[2]; 
		}
		else
		{
			clipPlane[2] = -1.0;
			clipPlane[3] = curwater->vecs[2];
		}
	}

	qglEnable(GL_CLIP_PLANE0);
	qglClipPlane(GL_CLIP_PLANE0, clipPlane);
}

void R_RenderReflectView(void)
{
	if (s_WaterFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curwater->reflectmap, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curwater->depthreflmap, 0);
		qglDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	qglClearColor(curwater->color.r / 255.0f, curwater->color.g / 255.0f, curwater->color.b / 255.0f, 1);
	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglStencilMask(0);

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

	drawreflect = true;

	saved_cl_waterlevel = *cl_waterlevel;
	*cl_waterlevel = 0;
	auto saved_r_drawentities = r_drawentities->value;
	if (r_water->value >= 2)
	{
		r_drawentities->value = 1;
	}
	else
	{
		r_drawentities->value = 0;
	}

	gRefFuncs.R_RenderScene();

	r_drawentities->value = saved_r_drawentities;
	*cl_waterlevel = saved_cl_waterlevel;

	qglDisable(GL_CLIP_PLANE0);

	if (!s_WaterFBO.s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, curwater->reflectmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, curwater->texwidth, curwater->texheight, 0);
	}

	R_PopRefDef();

	drawreflect = false;

	curwater->reflectmap_ready = true;
}

void R_RenderRefractView(void)
{
	if (s_WaterFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curwater->refractmap, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curwater->depthrefrmap, 0);
		qglDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	qglClearColor(curwater->color.r / 255.0f, curwater->color.g / 255.0f, curwater->color.b / 255.0f, 1);
	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglStencilMask(0);

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

	drawrefract = true;

	saved_cl_waterlevel = *cl_waterlevel;
	*cl_waterlevel = 0;
	auto saved_r_drawentities = r_drawentities->value;
	if (r_water->value >= 2)
	{
		r_drawentities->value = 1;
	}
	else
	{
		r_drawentities->value = 0;
	}

	gRefFuncs.R_RenderScene();
	
	r_drawentities->value = saved_r_drawentities;
	*cl_waterlevel = saved_cl_waterlevel;

	qglDisable(GL_CLIP_PLANE0);

	if (!s_WaterFBO.s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, curwater->refractmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, curwater->texwidth, curwater->texheight, 0);

		qglBindTexture(GL_TEXTURE_2D, curwater->depthrefrmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, curwater->texwidth, curwater->texheight, 0);
	}

	R_PopRefDef();

	drawrefract = false;

	curwater->refractmap_ready = true;
}

void R_FreeDeadWaters(void)
{
	r_water_t *kill;
	r_water_t *p;

	for (;; )
	{
		kill = waters_active;

		if (kill && kill->free)
		{
			waters_active = kill->next;
			kill->next = waters_free;
			waters_free = kill;
			continue;
		}

		break;
	}

	for (p = waters_active; p; p = p->next)
	{
		for (;; )
		{
			kill = p->next;
			if (kill && kill->free)
			{
				p->next = kill->next;
				kill->next = waters_free;
				waters_free = kill;
				continue;
			}
			break;
		}
	}
}

void R_RenderWaterView(void)
{
	GL_PushFrameBuffer();

	for(r_water_t *w = waters_active; w; w = w->next)
	{
		curwater = w;

		if(R_ShouldReflect())
		{
			R_RenderReflectView();
		}

		R_RenderRefractView();

		curwater = NULL;
	}

	GL_PopFrameBuffer();
}