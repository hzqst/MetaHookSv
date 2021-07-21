#include "gl_local.h"
#include <sstream>

//renderer
vec3_t water_view;

//water
r_water_t waters[MAX_WATERS];
r_water_t *curwater;
r_water_t *waters_free;
r_water_t *waters_active;

//shader

int water_normalmap = 0;
GLuint refractmap = 0;
GLuint depthrefrmap = 0;
qboolean refractmap_ready = 0;

SHADER_DEFINE(drawdepth);

int saved_cl_waterlevel;

//cvar
cvar_t *r_water = NULL;
cvar_t *r_water_debug = NULL;
cvar_t *r_water_fresnelfactor = NULL;
cvar_t *r_water_depthfactor1 = NULL;
cvar_t *r_water_depthfactor2 = NULL;
cvar_t *r_water_normfactor = NULL;
cvar_t *r_water_minheight = NULL;
cvar_t *r_water_maxalpha = NULL;

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

		if (state & WATER_DEPTH_ENABLED)
			defs << "#define DEPTH_ENABLED\n";

		if (state & WATER_REFRACT_ENABLED)
			defs << "#define REFRACT_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\water_shader.vsh", NULL, "renderer\\shader\\water_shader.fsh", def.c_str(), NULL, def.c_str());
		if (prog.program)
		{
			SHADER_UNIFORM(prog, watercolor, "watercolor");
			SHADER_UNIFORM(prog, eyepos, "eyepos");
			SHADER_UNIFORM(prog, entitymatrix, "entitymatrix");
			SHADER_UNIFORM(prog, worldmatrix, "worldmatrix");
			SHADER_UNIFORM(prog, time, "time");
			SHADER_UNIFORM(prog, fresnelfactor, "fresnelfactor");
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
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, true, (float *)r_rotate_entity_matrix);
			else
				qglUniformMatrix4fvARB(prog.entitymatrix, 1, false, (float *)r_identity_matrix);
		}
		if (prog.worldmatrix != -1)
		{
			if (g_SvEngine_DrawPortalView)
			{
				float mvmatrix[16];
				qglGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
				qglUniformMatrix4fvARB(prog.worldmatrix, 1, false, (float *)mvmatrix);
			}
			else
			{
				qglUniformMatrix4fvARB(prog.worldmatrix, 1, true, (float *)r_world_matrix);
			}
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
	if (refractmap)
	{
		GL_DeleteTexture(refractmap);
		refractmap = 0;
	}
	if (depthrefrmap)
	{
		GL_DeleteTexture(depthrefrmap);
		depthrefrmap = 0;
	}

	for (int i = 0; i < MAX_WATERS; ++i)
	{
		if (waters[i].depthreflmap)
		{
			GL_DeleteTexture(waters[i].depthreflmap);
			waters[i].depthreflmap = 0;
		}
		if (waters[i].reflectmap)
		{
			GL_DeleteTexture(waters[i].reflectmap);
			waters[i].reflectmap = 0;
		}
		waters[i].ready = false;
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
	r_water_fresnelfactor = gEngfuncs.pfnRegisterVariable("r_water_fresnelfactor", "0.4", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_depthfactor1 = gEngfuncs.pfnRegisterVariable("r_water_depthfactor1", "0.02", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_depthfactor2 = gEngfuncs.pfnRegisterVariable("r_water_depthfactor2", "0.01", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_normfactor = gEngfuncs.pfnRegisterVariable("r_water_normfactor", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_minheight = gEngfuncs.pfnRegisterVariable("r_water_minheight", "7.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_maxalpha = gEngfuncs.pfnRegisterVariable("r_water_maxalpha", "0.75", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	curwater = NULL;

	R_ClearWater();
}

bool R_ShouldReflect(r_water_t *w) 
{
	return r_refdef->vieworg[2] > w->vecs[2];
}

r_water_t *R_GetActiveWater(cl_entity_t *ent, vec3_t p, vec3_t n, colorVec *color)
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

	//Load if normalmap not exists.
	if (!water_normalmap)
	{
		water_normalmap = R_LoadTextureEx("renderer\\texture\\water_normalmap.tga", "water_normalmap", NULL, NULL, GLT_SYSTEM, true, true);
	}

	//Upload color textures and depth textures.
	if (!refractmap)
	{
		refractmap = GL_GenTextureRGBA8(glwidth, glheight);
	}

	if (!depthrefrmap)
	{
		depthrefrmap = GL_GenDepthTexture(glwidth, glheight);
	}

	if (!w->reflectmap)
	{
		w->reflectmap = GL_GenTextureRGBA8(glwidth, glheight);
	}

	if (!w->depthreflmap)
	{
		w->depthreflmap = GL_GenDepthTexture(glwidth, glheight);
	}

	w->ready = false;

	VectorCopy(p, w->vecs);
	VectorCopy(n, w->norm);
	w->ent = ent;
	w->org[0] = (ent->curstate.mins[0] + ent->curstate.maxs[0]) / 2;
	w->org[1] = (ent->curstate.mins[1] + ent->curstate.maxs[1]) / 2;
	w->org[2] = (ent->curstate.mins[2] + ent->curstate.maxs[2]) / 2;
	memcpy(&w->color, color, sizeof(*color));
	w->free = false;
	w->framecount = (*r_framecount);

	return w;
}

void R_RenderReflectView(r_water_t *w)
{
	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, w->reflectmap, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, w->depthreflmap, 0);
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	s_WaterFBO.s_hBackBufferTex = w->reflectmap;
	s_WaterFBO.s_hBackBufferDepthTex = w->depthreflmap;

	qglClearColor(w->color.r / 255.0f, w->color.g / 255.0f, w->color.b / 255.0f, 1);
	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglDepthMask(GL_TRUE);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglStencilMask(0);

	R_PushRefDef();

	VectorCopy(r_refdef->vieworg, water_view);

	float vForward[3], vRight[3], vUp[3];
	gEngfuncs.pfnAngleVectors(r_refdef->viewangles, vForward, vRight, vUp);

	float flDist = fabs(w->vecs[2] - r_refdef->vieworg[2]);
	VectorMA(r_refdef->vieworg, -2 * flDist, w->norm, r_refdef->vieworg);

	float neg_norm[3];
	VectorCopy(w->norm, neg_norm);
	VectorInverse(neg_norm);

	float flDist2 = DotProduct(vForward, neg_norm);
	VectorMA(vForward, -2 * flDist2, neg_norm, vForward);

	r_refdef->viewangles[0] = -asin(vForward[2]) / M_PI * 180;
	r_refdef->viewangles[1] = atan2(vForward[1], vForward[0]) / M_PI * 180;
	r_refdef->viewangles[2] = -r_refdef->viewangles[2];

	r_draw_pass = r_draw_reflect;

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

	R_PopRefDef();

	r_draw_pass = r_draw_normal;

	w->ready = true;
}

#if 0
void R_RenderRefractView(r_water_t *w)
{
	if (refractmap_ready)
		return;

	qglBindFramebufferEXT(GL_FRAMEBUFFER, s_WaterFBO.s_hBackBufferFBO);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractmap, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthrefrmap, 0);
	qglDrawBuffer(GL_COLOR_ATTACHMENT0);

	s_WaterFBO.s_hBackBufferTex = refractmap;
	s_WaterFBO.s_hBackBufferDepthTex = depthrefrmap;

	qglClearColor(w->color.r / 255.0f, w->color.g / 255.0f, w->color.b / 255.0f, 1);
	qglStencilMask(0xFF);
	qglClearStencil(0);
	qglDepthMask(GL_TRUE);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglStencilMask(0);

	R_PushRefDef();

	VectorCopy(r_refdef->vieworg, water_view);

	r_draw_pass = r_draw_refract;

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

	R_PopRefDef();

	r_draw_pass = r_draw_normal;

end:
	w->ready = true;
	refractmap_ready = true;
}
#endif

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
	refractmap_ready = false;

	for(r_water_t *w = waters_active; w; w = w->next)
	{
		curwater = w;

		if(R_ShouldReflect(w))
		{
			R_RenderReflectView(w);
		}

		//R_RenderRefractView(w);

		curwater = NULL;
	}
}