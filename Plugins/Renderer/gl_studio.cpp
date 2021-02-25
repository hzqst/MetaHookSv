#include "gl_local.h"
#include "triangleapi.h"
#include "cJSON.h"
#define SSE
#include <sselib.h>

std::unordered_map<studiohdr_t *, studio_vbo_t *> g_StudioVBOTable;

//engine
model_t *cl_sprite_white;
model_t *cl_shellchrome;
mstudiomodel_t **psubmodel;
studiohdr_t **pstudiohdr;
model_t **r_model;
float *r_blend;
float (*pbonetransform)[MAXSTUDIOBONES][3][4];
float (*plighttransform)[MAXSTUDIOBONES][3][4];
int (*g_NormalIndex)[MAXSTUDIOVERTS];
int(*chrome)[MAXSTUDIOVERTS][2];
int (*chromeage)[MAXSTUDIOBONES];
cl_entity_t *cl_viewent;
int *g_ForcedFaceFlags;
int *lightgammatable;
float *g_ChromeOrigin;
int *r_smodels_total;
int *r_ambientlight;
float *r_shadelight;
vec3_t *r_blightvec;
float *r_plightvec;
float *r_colormix;

//renderer
vec3_t r_studionormal[MAXSTUDIOVERTS];
float lightpos[MAXSTUDIOVERTS][3][4];
auxvert_t auxverts[MAXSTUDIOVERTS];
vec3_t lightvalues[MAXSTUDIOVERTS];
auxvert_t *pauxverts;
float *pvlightvalues;

SHADER_DEFINE(studio);
SHADER_DEFINE(studiogbuffer);

SHADER_DEFINE(studio_flatshade);
SHADER_DEFINE(studiogbuffer_flatshade);

SHADER_DEFINE(studio_fullbright);
SHADER_DEFINE(studiogbuffer_fullbright);

SHADER_DEFINE(studio_chrome);
SHADER_DEFINE(studiogbuffer_chrome);

studio_texarray_mgr_t g_TexArray;
studio_texarray_mgr_t g_LocalTexArray;

cvar_t *r_studio_vbo = NULL;

int Q_stricmp_slash(const char *s1, const char *s2);
void VectorIRotate(const vec3_t in1, const float in2[3][4], vec3_t out);
void VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out);

void R_StudioClearVBOCache(void)
{
	for (auto &itor = g_StudioVBOTable.begin(); itor != g_StudioVBOTable.end();++itor)
	{
		if (itor->second->hDataBuffer)
		{
			qglDeleteBuffersARB(1, &itor->second->hDataBuffer);
		}
		if (itor->second->hIndexBuffer)
		{
			qglDeleteBuffersARB(1, &itor->second->hIndexBuffer);
		}

		for (auto &submodel : itor->second->vSubmodel)
		{
			delete[]submodel.second->vMesh;
		}

		delete itor->second;
	}
	g_StudioVBOTable.clear();
}

void R_UnloadTextureArray(studio_texarray_t *texarray)
{
	int i;
	if(!texarray->numtextures)
		return;
	for(i = 0; i < texarray->numtextures; ++i)
	{
		studio_texture_t *st = &texarray->textures[i];
		if(st->replace.glt)
			GL_FreeTexture(st->replace.glt);
		if(st->normal.glt)
			GL_FreeTexture(st->normal.glt);
	}
	delete [] texarray->textures;
	texarray->textures = NULL;
	texarray->numtextures = 0;
	texarray->modelname[0] = 0;
}

void R_UnloadTextureArrayMgr(studio_texarray_mgr_t *pTexArrayMgr)
{
	int i;
	for(i = 0;i < pTexArrayMgr->iNumTexArray; ++i)
	{
		R_UnloadTextureArray(&pTexArrayMgr->pTexArray[i]);
	}
	delete [] pTexArrayMgr->pTexArray;
	pTexArrayMgr->pTexArray = NULL;
	pTexArrayMgr->iNumTexArray = 0;
}

void R_LoadStudioTextures_Normal(cJSON *tex, const char *objname, studio_texentry_t *pTexEntry)
{
	int texid;

	cJSON *obj = cJSON_GetObjectItem(tex, objname);
	if(!obj || !obj->valuestring)
		return;

	texid = R_LoadTextureEx(obj->valuestring, obj->valuestring, NULL, NULL, GLT_SYSTEM, false, false);
	if (!texid)
		return;
	strncpy(pTexEntry->name, obj->valuestring, 63);
	pTexEntry->name[63] = 0;
	pTexEntry->glt = R_GetCurrentGLTexture();
	pTexEntry->w = pTexEntry->glt->width;
	pTexEntry->h = pTexEntry->glt->height;
	pTexEntry->index = texid;
}

void R_LoadStudioTextures(qboolean loadmap)
{
	int i, j;
	char szFileName[256];
	char *pFile;
	studio_texarray_mgr_t *pTexArrayMgr;
	studio_texarray_t *pTexArray;
	studio_texture_t *pTex;

	if(!loadmap)
	{
		memset(&g_TexArray, 0, sizeof(g_TexArray));
		memset(&g_LocalTexArray, 0, sizeof(g_LocalTexArray));
	}
	else
	{
		R_UnloadTextureArrayMgr(&g_LocalTexArray);
	}

	if(!loadmap)
	{
		sprintf(szFileName, "resource/studio_textures.txt");
	}
	else
	{
		strcpy( szFileName, gEngfuncs.pfnGetLevelName() );
		if ( !strlen(szFileName) )
		{
			gEngfuncs.Con_Printf("R_LoadStudioTextures failed to GetLevelName.\n");
			return;
		}
		szFileName[strlen(szFileName)-4] = 0;
		strcat(szFileName, "_studio.txt");
	}

	pFile = (char *)gEngfuncs.COM_LoadFile(szFileName, 5, NULL);
	if (!pFile)
	{
		gEngfuncs.Con_Printf("R_LoadStudioTextures failed to load %s.\n", szFileName);
		return;
	}

	cJSON *root = cJSON_Parse(pFile);
	if (!root)
	{
		gEngfuncs.Con_Printf("R_LoadStudioTextures failed to parse %s.\n", szFileName);
		return;
	}
	//texture array load
	int nummodels = cJSON_GetArraySize(root);
	if(nummodels < 1)
		return;

	if(!loadmap)
		pTexArrayMgr = &g_TexArray;
	else
		pTexArrayMgr = &g_LocalTexArray;

	//alloc new texarrays
	pTexArrayMgr->iNumTexArray = 0;
	pTexArrayMgr->pTexArray = new studio_texarray_t[nummodels];
	memset(pTexArrayMgr->pTexArray, 0, sizeof(studio_texarray_t)*nummodels);

	for(i = 0; i < nummodels; ++i)
	{
		cJSON *mod = cJSON_GetArrayItem(root, i);
		if(!mod)
			continue;
		//parse modelname
		cJSON *modelname = cJSON_GetObjectItem(mod, "modelname");
		if(!modelname || !modelname->valuestring)
		{
			gEngfuncs.Con_Printf("R_LoadStudioTextures parse %s with error, model #%d has no \"model\" component.\n", szFileName, i);
			continue;
		}

		//parse textures
		cJSON *textures = cJSON_GetObjectItem(mod, "textures");
		if(!textures)
		{
			gEngfuncs.Con_Printf("R_LoadStudioTextures parse %s with error, model #%d has no \"textures\" component.\n", szFileName, i);
			continue;
		}

		//create texture array
		int numtextures = cJSON_GetArraySize(textures);
		if(numtextures < 1)
			continue;

		pTexArray = &pTexArrayMgr->pTexArray[pTexArrayMgr->iNumTexArray];
		pTexArrayMgr->iNumTexArray ++;

		//init texarray
		pTexArray->textures = new studio_texture_t[numtextures];
		memset(pTexArray->textures, 0, sizeof(studio_texture_t)*numtextures);
		pTexArray->numtextures = 0;
		strncpy(pTexArray->modelname, modelname->valuestring, 63);
		pTexArray->modelname[63] = 0;

		for(j = 0; j < numtextures; ++j)
		{
			pTex = &pTexArray->textures[pTexArray->numtextures];
			cJSON *tex = cJSON_GetArrayItem(textures, j);
			if(!tex)
				continue;
			cJSON *base = cJSON_GetObjectItem(tex, "base");
			if(!base || !base->valuestring)
			{
				gEngfuncs.Con_Printf("R_LoadStudioTextures parse %s with error, model #%d's texture #%d has no \"base\" component.\n", szFileName, i, j);
				continue;
			}
			//increment
			pTexArray->numtextures ++;
			//load base texture
			strncpy(pTex->base.name, base->valuestring, 63);
			pTex->base.name[63] = 0;
			pTex->base.index = 0;
			R_LoadStudioTextures_Normal(tex, "replace", &pTex->replace);
		}//texture load end			
	}//model load end		
	cJSON_Delete(root);
	gEngfuncs.COM_FreeFile((void *) pFile );
}

void R_InitStudio(void)
{
	if (gl_shader_support)
	{
		studio.program = R_CompileShaderFile("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh");
		if (studio.program)
		{
			SHADER_UNIFORM(studio, bonematrix, "bonematrix");

			SHADER_UNIFORM(studio, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studio, v_lambert, "v_lambert");
			SHADER_UNIFORM(studio, v_brightness, "v_brightness");
			SHADER_UNIFORM(studio, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studio, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studio, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studio, r_blend, "r_blend");
			SHADER_UNIFORM(studio, r_g1, "r_g1");
			SHADER_UNIFORM(studio, r_g3, "r_g3");
			SHADER_UNIFORM(studio, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studio, r_colormix, "r_colormix");

			SHADER_ATTRIB(studio, attrbone, "attrbone");
		}

		studiogbuffer.program = R_CompileShaderFileEx("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh",
			"#define GBUFFER_ENABLED", NULL, "#define GBUFFER_ENABLED");
		if (studiogbuffer.program)
		{
			SHADER_UNIFORM(studiogbuffer, bonematrix, "bonematrix");

			SHADER_UNIFORM(studiogbuffer, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studiogbuffer, v_lambert, "v_lambert");
			SHADER_UNIFORM(studiogbuffer, v_brightness, "v_brightness");
			SHADER_UNIFORM(studiogbuffer, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studiogbuffer, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studiogbuffer, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studiogbuffer, r_blend, "r_blend");
			SHADER_UNIFORM(studiogbuffer, r_g1, "r_g1");
			SHADER_UNIFORM(studiogbuffer, r_g3, "r_g3");
			SHADER_UNIFORM(studiogbuffer, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studiogbuffer, r_colormix, "r_colormix");

			SHADER_ATTRIB(studiogbuffer, attrbone, "attrbone");
		}

		//FlatShade
		studio_flatshade.program = R_CompileShaderFileEx("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh",
			"#define STUDIO_FLATSHADE", NULL, "#define STUDIO_FLATSHADE");
		if (studio_flatshade.program)
		{
			SHADER_UNIFORM(studio_flatshade, bonematrix, "bonematrix");

			SHADER_UNIFORM(studio_flatshade, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studio_flatshade, v_lambert, "v_lambert");
			SHADER_UNIFORM(studio_flatshade, v_brightness, "v_brightness");
			SHADER_UNIFORM(studio_flatshade, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studio_flatshade, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studio_flatshade, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studio_flatshade, r_blend, "r_blend");
			SHADER_UNIFORM(studio_flatshade, r_g1, "r_g1");
			SHADER_UNIFORM(studio_flatshade, r_g3, "r_g3");
			SHADER_UNIFORM(studio_flatshade, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studio_flatshade, r_colormix, "r_colormix");

			SHADER_ATTRIB(studio_flatshade, attrbone, "attrbone");
		}

		studiogbuffer_flatshade.program = R_CompileShaderFileEx("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh",
			"#define GBUFFER_ENABLED\n#define STUDIO_FLATSHADE", NULL, "#define GBUFFER_ENABLED\n#define STUDIO_FLATSHADE");
		if (studiogbuffer_flatshade.program)
		{
			SHADER_UNIFORM(studiogbuffer_flatshade, bonematrix, "bonematrix");

			SHADER_UNIFORM(studiogbuffer_flatshade, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studiogbuffer_flatshade, v_lambert, "v_lambert");
			SHADER_UNIFORM(studiogbuffer_flatshade, v_brightness, "v_brightness");
			SHADER_UNIFORM(studiogbuffer_flatshade, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studiogbuffer_flatshade, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studiogbuffer_flatshade, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studiogbuffer_flatshade, r_blend, "r_blend");
			SHADER_UNIFORM(studiogbuffer_flatshade, r_g1, "r_g1");
			SHADER_UNIFORM(studiogbuffer_flatshade, r_g3, "r_g3");
			SHADER_UNIFORM(studiogbuffer_flatshade, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studiogbuffer_flatshade, r_colormix, "r_colormix");

			SHADER_ATTRIB(studiogbuffer_flatshade, attrbone, "attrbone");
		}

		//FullBright
		studio_fullbright.program = R_CompileShaderFileEx("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh",
			"#define STUDIO_FULLBRIGHT", NULL, "#define STUDIO_FULLBRIGHT");
		if (studio_fullbright.program)
		{
			SHADER_UNIFORM(studio_fullbright, bonematrix, "bonematrix");

			SHADER_UNIFORM(studio_fullbright, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studio_fullbright, v_lambert, "v_lambert");
			SHADER_UNIFORM(studio_fullbright, v_brightness, "v_brightness");
			SHADER_UNIFORM(studio_fullbright, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studio_fullbright, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studio_fullbright, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studio_fullbright, r_blend, "r_blend");
			SHADER_UNIFORM(studio_fullbright, r_g1, "r_g1");
			SHADER_UNIFORM(studio_fullbright, r_g3, "r_g3");
			SHADER_UNIFORM(studio_fullbright, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studio_fullbright, r_colormix, "r_colormix");

			SHADER_ATTRIB(studio_fullbright, attrbone, "attrbone");
		}

		studiogbuffer_fullbright.program = R_CompileShaderFileEx("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh",
			"#define GBUFFER_ENABLED\n#define STUDIO_FULLBRIGHT", NULL, "#define GBUFFER_ENABLED\n#define STUDIO_FULLBRIGHT");
		if (studiogbuffer_fullbright.program)
		{
			SHADER_UNIFORM(studiogbuffer_fullbright, bonematrix, "bonematrix");

			SHADER_UNIFORM(studiogbuffer_fullbright, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studiogbuffer_fullbright, v_lambert, "v_lambert");
			SHADER_UNIFORM(studiogbuffer_fullbright, v_brightness, "v_brightness");
			SHADER_UNIFORM(studiogbuffer_fullbright, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studiogbuffer_fullbright, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studiogbuffer_fullbright, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studiogbuffer_fullbright, r_blend, "r_blend");
			SHADER_UNIFORM(studiogbuffer_fullbright, r_g1, "r_g1");
			SHADER_UNIFORM(studiogbuffer_fullbright, r_g3, "r_g3");
			SHADER_UNIFORM(studiogbuffer_fullbright, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studiogbuffer_fullbright, r_colormix, "r_colormix");

			SHADER_ATTRIB(studiogbuffer_fullbright, attrbone, "attrbone");
		}

		studio_chrome.program = R_CompileShaderFileEx("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh",
			"#define STUDIO_CHROME", NULL, "#define STUDIO_CHROME");
		if (studio_chrome.program)
		{
			SHADER_UNIFORM(studio_chrome, bonematrix, "bonematrix");

			SHADER_UNIFORM(studio_chrome, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studio_chrome, v_lambert, "v_lambert");
			SHADER_UNIFORM(studio_chrome, v_brightness, "v_brightness");
			SHADER_UNIFORM(studio_chrome, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studio_chrome, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studio_chrome, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studio_chrome, r_blend, "r_blend");
			SHADER_UNIFORM(studio_chrome, r_g1, "r_g1");
			SHADER_UNIFORM(studio_chrome, r_g3, "r_g3");
			SHADER_UNIFORM(studio_chrome, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studio_chrome, r_colormix, "r_colormix");
			SHADER_UNIFORM(studio_chrome, r_origin, "r_origin");
			SHADER_UNIFORM(studio_chrome, r_vright, "r_vright");
			SHADER_UNIFORM(studio_chrome, r_scale, "r_scale");

			SHADER_ATTRIB(studio_chrome, attrbone, "attrbone");
		}

		studiogbuffer_chrome.program = R_CompileShaderFileEx("resource\\shader\\studio_shader.vsh", NULL, "resource\\shader\\studio_shader.fsh",
			"#define GBUFFER_ENABLED\n#define STUDIO_CHROME", NULL, "#define GBUFFER_ENABLED\n#define STUDIO_CHROME");
		if (studiogbuffer_chrome.program)
		{
			SHADER_UNIFORM(studiogbuffer_chrome, bonematrix, "bonematrix");

			SHADER_UNIFORM(studiogbuffer_chrome, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(studiogbuffer_chrome, v_lambert, "v_lambert");
			SHADER_UNIFORM(studiogbuffer_chrome, v_brightness, "v_brightness");
			SHADER_UNIFORM(studiogbuffer_chrome, v_lightgamma, "v_lightgamma");
			SHADER_UNIFORM(studiogbuffer_chrome, r_ambientlight, "r_ambientlight");
			SHADER_UNIFORM(studiogbuffer_chrome, r_shadelight, "r_shadelight");
			SHADER_UNIFORM(studiogbuffer_chrome, r_blend, "r_blend");
			SHADER_UNIFORM(studiogbuffer_chrome, r_g1, "r_g1");
			SHADER_UNIFORM(studiogbuffer_chrome, r_g3, "r_g3");
			SHADER_UNIFORM(studiogbuffer_chrome, r_plightvec, "r_plightvec");
			SHADER_UNIFORM(studiogbuffer_chrome, r_colormix, "r_colormix");
			SHADER_UNIFORM(studiogbuffer_chrome, r_origin, "r_origin");
			SHADER_UNIFORM(studiogbuffer_chrome, r_vright, "r_vright");
			SHADER_UNIFORM(studiogbuffer_chrome, r_scale, "r_scale");

			SHADER_ATTRIB(studiogbuffer_chrome, attrbone, "attrbone");
		}
	}

	r_studio_vbo = gEngfuncs.pfnRegisterVariable("r_studio_vbo", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

//Engine Studio

inline void R_StudioTransformAuxVert(auxvert_t *av, int bone, vec3_t vert)
{
#ifndef SSE
	VectorTransform(vert, (*pbonetransform)[bone], av->fv);
#else
	VectorTransformSSE(vert, (*pbonetransform)[bone], av->fv);
#endif
}

inline qboolean R_IsFlippedViewModel(void)
{
	if (cl_righthand && cl_righthand->value > 0)
	{
		if (cl_viewent == (*currententity))
			return true;
	}

	return false;
}

void BuildNormalIndexTable(void)
{
	if (gRefFuncs.BuildNormalIndexTable)
		return gRefFuncs.BuildNormalIndexTable();

	int					j;
	int					i;
	mstudiomesh_t		*pmesh;

	for (i = 0; i < (*psubmodel)->numverts; i++)
	{
		(*g_NormalIndex)[i] = -1;
	}

	for (j = 0; j < (*psubmodel)->nummesh; j++)
	{
		short		*ptricmds;

		pmesh = (mstudiomesh_t *)((byte *)(*pstudiohdr) + (*psubmodel)->meshindex) + j;
		ptricmds = (short *)((byte *)(*pstudiohdr) + pmesh->triindex);

		while (i = *(ptricmds++))
		{
			if (i < 0)
			{
				i = -i;
			}

			for (; i > 0; i--, ptricmds += 4)
			{
				if ((*g_NormalIndex)[ptricmds[0]] < 0)
					(*g_NormalIndex)[ptricmds[0]] = ptricmds[1];
			}
		}
	}
}

studiohdr_t *R_LoadTextures(model_t *psubm)
{
	if (gRefFuncs.R_LoadTextures)
		return gRefFuncs.R_LoadTextures(psubm);

	model_t *texmodel;

	if ((*pstudiohdr)->textureindex == 0)
	{
		studiohdr_t *ptexturehdr;
		char modelname[256];

		strncpy(modelname, psubm->name, sizeof(modelname) - 2);
		modelname[sizeof(modelname) - 2] = 0;

		strcpy(&modelname[strlen(modelname) - 4], "T.mdl");
		texmodel = IEngineStudio.Mod_ForName(modelname, true);
		psubm->texinfo = (mtexinfo_t *)texmodel;

		ptexturehdr = (studiohdr_t *)texmodel->cache.data;
		strncpy(ptexturehdr->name, modelname, sizeof(ptexturehdr->name) - 1);
		ptexturehdr->name[sizeof(ptexturehdr->name) - 1] = 0;

		return ptexturehdr;
	}

	return (*pstudiohdr);
}

void BuildGlowShellVerts(vec3_t *pstudioverts, auxvert_t *paux)
{
	if (gRefFuncs.BuildGlowShellVerts)
		return gRefFuncs.BuildGlowShellVerts(pstudioverts, paux);

	int					i;
	byte				*pvertbone;
	vec3_t				*pstudionorms;
	auxvert_t			*av;
	float				vscale;

	pvertbone = ((byte *)(*pstudiohdr) + (*psubmodel)->vertinfoindex);
	pstudionorms = (vec3_t *)((byte *)(*pstudiohdr) + (*psubmodel)->normindex);

	vscale = (*currententity)->curstate.renderamt * 0.05;

	for (i = 0; i < (*pstudiohdr)->numbones; i++)
	{
		(*chromeage)[i] = 0;
	}

	for (i = 0; i < (*psubmodel)->numverts; i++)
	{
		vec3_t vert;
		VectorMA(pstudioverts[i], vscale, pstudionorms[(*g_NormalIndex)[i]], vert);

		av = &paux[i];
		R_StudioTransformAuxVert(av, pvertbone[i], vert);
	}

	g_ChromeOrigin[0] = cos(r_glowshellfreq->value * (*cl_time)) * 4000.0;
	g_ChromeOrigin[1] = sin(r_glowshellfreq->value * (*cl_time)) * 4000.0;
	g_ChromeOrigin[2] = cos(r_glowshellfreq->value * (*cl_time) * 0.33) * 4000.0;

	qglColor4ub((*currententity)->curstate.rendercolor.r, (*currententity)->curstate.rendercolor.g, (*currententity)->curstate.rendercolor.b, 255);
}

void R_LightStrength(int bone, float *vert, float light[3][4])
{
	if (gRefFuncs.R_LightStrength)
		return gRefFuncs.R_LightStrength(bone, vert, light);
}

void R_StudioLighting(float *lv, int bone, int flags, vec3_t normal)
{
	if (gRefFuncs.R_StudioLighting)
		return gRefFuncs.R_StudioLighting(lv, bone, flags, normal);

	float 	illum;
	float	lightcos;

	illum = (*r_ambientlight);

	if (flags & STUDIO_NF_FLATSHADE)
	{
		illum += (*r_shadelight) * 0.8;
	}
	else
	{
		float r;

		if (bone != -1)
		{
			lightcos = DotProduct(normal, r_blightvec[bone]);
		}
		else
		{
			lightcos = DotProduct(normal, r_plightvec); // -1 colinear, 1 opposite
		}

		if (lightcos > 1.0)
			lightcos = 1;

		r = v_lambert->value;
		if (r < 1.0)
		{
			lightcos = (r - lightcos) / (r + 1.0f); 		// do modified hemispherical lighting
			if (lightcos > 0.0)
			{
				illum += (*r_shadelight) * lightcos;
			}
		}
		else
		{
			illum += (*r_shadelight);
			lightcos = (lightcos + (r - 1.0)) / r; 		// do modified hemispherical lighting
			if (lightcos > 0.0)
			{
				illum -= (*r_shadelight) * lightcos;
			}
		}

		if (illum <= 0)
			illum = 0;
	}

	if (illum > 255)
		illum = 255;

	*lv = lightgammatable[(int)(illum * 4)] / 1023.0;	// Light from 0 to 1.0
}

void studioapi_SetupModel(int bodypart, void **ppbodypart, void **ppsubmodel)
{
	gRefFuncs.studioapi_SetupModel(bodypart, ppbodypart, ppsubmodel);
}

void R_GLStudioDrawPoints(void)
{
	int i, j, k;
	byte *pvertbone;
	byte *pnormbone;
	vec3_t *pstudioverts;
	vec3_t *pstudionorms;
	studiohdr_t *ptexturehdr;
	mstudiotexture_t *ptexture;
	mstudiomesh_t *pmesh;
	auxvert_t *av;
	float *lv;
	vec3_t fl;
	float lv_tmp;
	short *pskinref;
	int flags;

	pvertbone = ((byte *)(*pstudiohdr) + (*psubmodel)->vertinfoindex);
	pnormbone = ((byte *)(*pstudiohdr) + (*psubmodel)->norminfoindex);
	ptexturehdr = R_LoadTextures(*r_model);
	ptexture = (mstudiotexture_t *)((byte *)ptexturehdr + ptexturehdr->textureindex);

	pmesh = (mstudiomesh_t *)((byte *)(*pstudiohdr) + (*psubmodel)->meshindex);

	pstudioverts = (vec3_t *)((byte *)(*pstudiohdr) + (*psubmodel)->vertindex);
	pstudionorms = (vec3_t *)((byte *)(*pstudiohdr) + (*psubmodel)->normindex);

	pskinref = (short *)((byte *)ptexturehdr + ptexturehdr->skinindex);

	int iFlippedVModel = 0;

	int iInitVBO = 0;

	studio_vbo_t *VBOData = NULL;

	studio_vbo_submodel_t *VBOSubmodel = NULL;

	if (r_studio_vbo->value)
	{
		auto itor = g_StudioVBOTable.find((*pstudiohdr));
		if (itor != g_StudioVBOTable.end())
		{
			VBOData = itor->second;
			auto itor2 = VBOData->vSubmodel.find((*psubmodel));
			if (itor2 != VBOData->vSubmodel.end())
			{
				VBOSubmodel = itor2->second;
			}
			else
			{
				VBOSubmodel = new studio_vbo_submodel_t;
				VBOData->vSubmodel[(*psubmodel)] = VBOSubmodel;
				iInitVBO = 2;
			}
		}
		else
		{
			VBOData = new studio_vbo_t;
			VBOSubmodel = new studio_vbo_submodel_t;
			VBOData->vSubmodel[(*psubmodel)] = VBOSubmodel;

			g_StudioVBOTable[(*pstudiohdr)] = VBOData;
			iInitVBO = 3;
		}
	}

	if (iInitVBO & 2)
	{
		VBOSubmodel->iNumMesh = (*psubmodel)->nummesh;
		VBOSubmodel->vMesh = new studio_vbo_mesh_t[VBOSubmodel->iNumMesh];
	}

	if ((*currententity)->curstate.skin != 0 && (*currententity)->curstate.skin < ptexturehdr->numskinfamilies)
		pskinref += ((*currententity)->curstate.skin * ptexturehdr->numskinref);

	//Setup light, chrome...
	if (!iInitVBO && VBOSubmodel && r_studio_vbo->value)
	{
		if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
		{
			g_ChromeOrigin[0] = cos(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
			g_ChromeOrigin[1] = sin(r_glowshellfreq->value * (*cl_time)) * 4000.0f;
			g_ChromeOrigin[2] = cos(r_glowshellfreq->value * (*cl_time) * 0.33f) * 4000.0f;

			r_colormix[0] = (float)(*currententity)->curstate.rendercolor.r / 255.0f;
			r_colormix[1] = (float)(*currententity)->curstate.rendercolor.g / 255.0f;
			r_colormix[2] = (float)(*currententity)->curstate.rendercolor.b / 255.0f;
		}
	}
	else
	{
		if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
		{
			BuildNormalIndexTable();
			BuildGlowShellVerts(pstudioverts, pauxverts);
		}
		else
		{
			for (i = 0; i < (*psubmodel)->numverts; i++)
			{
				av = &(pauxverts[i]);
				R_StudioTransformAuxVert(av, pvertbone[i], pstudioverts[i]);
			}
		}

		for (i = 0; i < (*psubmodel)->numverts; i++)
		{
			R_LightStrength(pvertbone[i], pstudioverts[i], lightpos[i]);
		}

		lv = pvlightvalues;

		for (j = 0; j < (*psubmodel)->nummesh; j++)
		{
			flags = ptexture[pskinref[pmesh[j].skinref]].flags | (*g_ForcedFaceFlags);

			if (r_fullbright->value >= 2)
				flags |= STUDIO_NF_FULLBRIGHT;

			if ((*currententity)->curstate.rendermode == kRenderTransAdd)
			{
				for (k = 0; k < pmesh[j].numnorms; k++, lv += 3, pstudionorms++, pnormbone++)
				{
					lv[0] = *r_blend;
					lv[1] = *r_blend;
					lv[2] = *r_blend;

					if (flags & STUDIO_NF_CHROME)
					{
						int m = (int)((char *)lv - (char *)pvlightvalues) / 12;
						gRefFuncs.R_StudioChrome((*chrome)[m], *pnormbone, *pstudionorms);
					}
				}
			}
			else
			{
				for (k = 0; k < pmesh[j].numnorms; k++, lv += 3, pstudionorms++, pnormbone++)
				{
					gRefFuncs.R_StudioLighting(&lv_tmp, *pnormbone, flags, *pstudionorms);
					//R_StudioLighting(&lv_tmp, *pnormbone, flags, *pstudionorms);

					if (flags & STUDIO_NF_CHROME)
					{
						int m = (int)((char *)lv - (char *)pvlightvalues) / 12;
						gRefFuncs.R_StudioChrome((*chrome)[m], *pnormbone, *pstudionorms);
					}

					lv[0] = lv_tmp * r_colormix[0];
					lv[1] = lv_tmp * r_colormix[1];
					lv[2] = lv_tmp * r_colormix[2];
				}
			}
		}
	}

	qglCullFace(GL_FRONT);

	if (R_IsFlippedViewModel())
	{
		qglDisable(GL_CULL_FACE);
		iFlippedVModel = 1;
	}

	pstudionorms = (vec3_t *)((byte *)(*pstudiohdr) + (*psubmodel)->normindex);
	pnormbone = ((byte *)(*pstudiohdr) + (*psubmodel)->norminfoindex);

	if (!iInitVBO && VBOSubmodel && r_studio_vbo->value && studio.program)
	{
		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, VBOData->hDataBuffer);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, VBOData->hIndexBuffer);
		qglVertexPointer(3, GL_FLOAT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, pos));
		qglNormalPointer(GL_FLOAT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, normal));

		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, texcoord));

		float r_g = 1.0f / v_gamma->value;

		float r_g3;
		if (v_brightness->value <= 0.0f)
			r_g3 = 0.125f;
		else if (v_brightness->value > 1.0f)
			r_g3 = 0.05f;
		else
			r_g3 = 0.125f - (v_brightness->value * v_brightness->value) * 0.075f;

		for (j = 0; j < (*psubmodel)->nummesh; j++)
		{
			auto &VBOMesh = VBOSubmodel->vMesh[j];

			pmesh = (mstudiomesh_t *)((byte *)(*pstudiohdr) + (*psubmodel)->meshindex) + j;

			(*c_alias_polys) += pmesh->numtris;

			flags = ptexture[pskinref[pmesh->skinref]].flags | (*g_ForcedFaceFlags);

			if (r_fullbright->value >= 2)
			{
				flags = flags & 0xFC;
			}

			bool bTransparent = false;
			if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);

				bTransparent = true;
			}
			else if (flags & STUDIO_NF_MASKED)
			{
				qglEnable(GL_ALPHA_TEST);
				qglAlphaFunc(GL_GREATER, 0.5);
				qglDepthMask(GL_TRUE);
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);
			}

			//Transparent object?

			//GLboolean writemask;
			//qglGetBooleanv(GL_DEPTH_WRITEMASK, &writemask);

			if (bTransparent)
			{
				R_SetGBufferRenderState(GBUFFER_STATE_TRANSPARENT_DIFFUSE);
				R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);
			}
			else
			{
				R_SetGBufferRenderState(GBUFFER_STATE_DIFFUSE);
				R_SetGBufferMask(GBUFFER_MASK_ALL);
			}

			if (r_fullbright->value >= 2)
			{
				gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);
			}
			else
			{
				gRefFuncs.R_StudioSetupSkin(ptexturehdr, pskinref[pmesh->skinref]);
			}

			int attrbone = -1;

			if (drawgbuffer)
			{
				if (flags & STUDIO_NF_CHROME)
				{
					qglUseProgramObjectARB(studiogbuffer_chrome.program);
					qglUniformMatrix3x4fvARB(studiogbuffer_chrome.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniform1iARB(studiogbuffer_chrome.diffuseTex, 0);
					qglUniform1fARB(studiogbuffer_chrome.r_blend, (*r_blend));
					qglUniform1fARB(studiogbuffer_chrome.r_g1, r_g);
					qglUniform1fARB(studiogbuffer_chrome.r_g3, r_g3);
					qglUniform1fARB(studiogbuffer_chrome.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studiogbuffer_chrome.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studiogbuffer_chrome.v_brightness, v_brightness->value);
					qglUniform1fARB(studiogbuffer_chrome.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studiogbuffer_chrome.v_lambert, v_lambert->value);
					qglUniform3fARB(studiogbuffer_chrome.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studiogbuffer_chrome.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);
					qglUniform3fARB(studiogbuffer_chrome.r_origin, g_ChromeOrigin[0], g_ChromeOrigin[1], g_ChromeOrigin[2]);
					qglUniform3fARB(studiogbuffer_chrome.r_vright, vright[0], vright[1], vright[2]);
					if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
					{
						qglUniform1fARB(studiogbuffer_chrome.r_scale, (*currententity)->curstate.renderamt * 0.05f);
					}
					else
					{
						qglUniform1fARB(studiogbuffer_chrome.r_scale, 0);
					}
					qglVertexAttribIPointer(studiogbuffer_chrome.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studiogbuffer_chrome.attrbone);
					attrbone = studiogbuffer_chrome.attrbone;
				}
				else if (flags & STUDIO_NF_FULLBRIGHT)
				{
					qglUseProgramObjectARB(studiogbuffer_fullbright.program);
					qglUniformMatrix3x4fvARB(studiogbuffer_fullbright.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniform1iARB(studiogbuffer_fullbright.diffuseTex, 0);
					qglUniform1fARB(studiogbuffer_fullbright.r_blend, (*r_blend));
					qglUniform1fARB(studiogbuffer_fullbright.r_g1, r_g);
					qglUniform1fARB(studiogbuffer_fullbright.r_g3, r_g3);
					qglUniform1fARB(studiogbuffer_fullbright.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studiogbuffer_fullbright.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studiogbuffer_fullbright.v_brightness, v_brightness->value);
					qglUniform1fARB(studiogbuffer_fullbright.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studiogbuffer_fullbright.v_lambert, v_lambert->value);
					qglUniform3fARB(studiogbuffer_fullbright.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studiogbuffer_fullbright.r_colormix, 1, 1, 1);
					qglVertexAttribIPointer(studiogbuffer_fullbright.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studiogbuffer_fullbright.attrbone);
					attrbone = studiogbuffer_fullbright.attrbone;
				}
				else if (flags & STUDIO_NF_FLATSHADE)
				{
					qglUseProgramObjectARB(studiogbuffer_flatshade.program);
					qglUniformMatrix3x4fvARB(studiogbuffer_flatshade.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniform1iARB(studiogbuffer_flatshade.diffuseTex, 0);
					qglUniform1fARB(studiogbuffer_flatshade.r_blend, (*r_blend));
					qglUniform1fARB(studiogbuffer_flatshade.r_g1, r_g);
					qglUniform1fARB(studiogbuffer_flatshade.r_g3, r_g3);
					qglUniform1fARB(studiogbuffer_flatshade.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studiogbuffer_flatshade.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studiogbuffer_flatshade.v_brightness, v_brightness->value);
					qglUniform1fARB(studiogbuffer_flatshade.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studiogbuffer_flatshade.v_lambert, v_lambert->value);
					qglUniform3fARB(studiogbuffer_flatshade.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studiogbuffer_flatshade.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);
					qglVertexAttribIPointer(studiogbuffer_flatshade.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studiogbuffer_flatshade.attrbone);
					attrbone = studiogbuffer_flatshade.attrbone;
				}
				else
				{
					qglUseProgramObjectARB(studiogbuffer.program);
					qglUniformMatrix3x4fvARB(studiogbuffer.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniformMatrix3x4fvARB(studiogbuffer.lightmatrix, 128, 0, (float *)(*plighttransform));
					qglUniform1iARB(studiogbuffer.diffuseTex, 0);
					qglUniform1fARB(studiogbuffer.r_blend, (*r_blend));
					qglUniform1fARB(studiogbuffer.r_g1, r_g);
					qglUniform1fARB(studiogbuffer.r_g3, r_g3);
					qglUniform1fARB(studiogbuffer.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studiogbuffer.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studiogbuffer.v_brightness, v_brightness->value);
					qglUniform1fARB(studiogbuffer.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studiogbuffer.v_lambert, v_lambert->value);
					qglUniform3fARB(studiogbuffer.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studiogbuffer.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);
					qglVertexAttribIPointer(studiogbuffer.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studiogbuffer.attrbone);
					attrbone = studiogbuffer.attrbone;
				}
			}
			else
			{
				if (flags & STUDIO_NF_CHROME)
				{
					qglUseProgramObjectARB(studio_chrome.program);
					qglUniformMatrix3x4fvARB(studio_chrome.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniform1iARB(studio_chrome.diffuseTex, 0);
					qglUniform1fARB(studio_chrome.r_blend, (*r_blend));
					qglUniform1fARB(studio_chrome.r_g1, r_g);
					qglUniform1fARB(studio_chrome.r_g3, r_g3);
					qglUniform1fARB(studio_chrome.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studio_chrome.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studio_chrome.v_brightness, v_brightness->value);
					qglUniform1fARB(studio_chrome.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studio_chrome.v_lambert, v_lambert->value);
					qglUniform3fARB(studio_chrome.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studio_chrome.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);
					qglUniform3fARB(studio_chrome.r_origin, g_ChromeOrigin[0], g_ChromeOrigin[1], g_ChromeOrigin[2]);
					qglUniform3fARB(studio_chrome.r_vright, vright[0], vright[1], vright[2]);
					if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
					{
						qglUniform1fARB(studio_chrome.r_scale, (*currententity)->curstate.renderamt * 0.05f);
					}
					else
					{
						qglUniform1fARB(studio_chrome.r_scale, 0);
					}
					qglVertexAttribIPointer(studio_chrome.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studio_chrome.attrbone);
					attrbone = studio_chrome.attrbone;
				}
				else if (flags & STUDIO_NF_FULLBRIGHT)
				{
					qglUseProgramObjectARB(studio_fullbright.program);
					qglUniformMatrix3x4fvARB(studio_fullbright.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniform1iARB(studio_fullbright.diffuseTex, 0);
					qglUniform1fARB(studio_fullbright.r_blend, (*r_blend));
					qglUniform1fARB(studio_fullbright.r_g1, r_g);
					qglUniform1fARB(studio_fullbright.r_g3, r_g3);
					qglUniform1fARB(studio_fullbright.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studio_fullbright.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studio_fullbright.v_brightness, v_brightness->value);
					qglUniform1fARB(studio_fullbright.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studio_fullbright.v_lambert, v_lambert->value);
					qglUniform3fARB(studio_fullbright.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studio_fullbright.r_colormix, 1, 1, 1);
					qglVertexAttribIPointer(studio_fullbright.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studio_fullbright.attrbone);
					attrbone = studio_fullbright.attrbone;
				}
				else if (flags & STUDIO_NF_FLATSHADE)
				{
					qglUseProgramObjectARB(studio_flatshade.program);
					qglUniformMatrix3x4fvARB(studio_flatshade.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniform1iARB(studio_flatshade.diffuseTex, 0);
					qglUniform1fARB(studio_flatshade.r_blend, (*r_blend));
					qglUniform1fARB(studio_flatshade.r_g1, r_g);
					qglUniform1fARB(studio_flatshade.r_g3, r_g3);
					qglUniform1fARB(studio_flatshade.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studio_flatshade.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studio_flatshade.v_brightness, v_brightness->value);
					qglUniform1fARB(studio_flatshade.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studio_flatshade.v_lambert, v_lambert->value);
					qglUniform3fARB(studio_flatshade.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studio_flatshade.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);
					qglVertexAttribIPointer(studio_flatshade.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studio_flatshade.attrbone);
					attrbone = studio_flatshade.attrbone;
				}
				else
				{
					qglUseProgramObjectARB(studio.program);
					qglUniformMatrix3x4fvARB(studio.bonematrix, 128, 0, (float *)(*pbonetransform));
					qglUniform1iARB(studio.diffuseTex, 0);
					qglUniform1fARB(studio.r_blend, (*r_blend));
					qglUniform1fARB(studio.r_g1, r_g);
					qglUniform1fARB(studio.r_g3, r_g3);
					qglUniform1fARB(studio.r_ambientlight, (float)(*r_ambientlight));
					qglUniform1fARB(studio.r_shadelight, (*r_shadelight));
					qglUniform1fARB(studio.v_brightness, v_brightness->value);
					qglUniform1fARB(studio.v_lightgamma, v_lightgamma->value);
					qglUniform1fARB(studio.v_lambert, v_lambert->value);
					qglUniform3fARB(studio.r_plightvec, r_plightvec[0], r_plightvec[1], r_plightvec[2]);
					qglUniform3fARB(studio.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);
					if (flags & STUDIO_NF_FULLBRIGHT)
					{
						qglUniform3fARB(studio.r_colormix, 1, 1, 1);
					}
					else
					{
						qglUniform3fARB(studio.r_colormix, r_colormix[0], r_colormix[1], r_colormix[2]);
					}
					qglVertexAttribIPointer(studio.attrbone, 2, GL_INT, sizeof(studio_vbo_vertex_t), OFFSET(studio_vbo_vertex_t, vertbone));
					qglEnableVertexAttribArray(studio.attrbone);
					attrbone = studio.attrbone;
				}
			}

#define BUFFER_OFFSET(i) ((unsigned int *)NULL + (i))

			if (VBOMesh.iTriStripVertexCount)
				qglDrawElements(GL_TRIANGLE_STRIP, VBOMesh.iTriStripVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(VBOMesh.iTriStripStartIndex));

			if (VBOMesh.iTriFanVertexCount)
				qglDrawElements(GL_TRIANGLE_FAN, VBOMesh.iTriFanVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(VBOMesh.iTriFanStartIndex));

			if (flags & STUDIO_NF_MASKED)
			{
				qglAlphaFunc(GL_NOTEQUAL, 0);
				qglDisable(GL_ALPHA_TEST);
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglDisable(GL_BLEND);
				qglDepthMask(1);
				qglShadeModel(GL_FLAT);
			}

			if(attrbone != -1)
				qglDisableVertexAttribArray(attrbone);
		}

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		qglDisableClientState(GL_VERTEX_ARRAY);
		qglDisableClientState(GL_NORMAL_ARRAY);
		qglUseProgramObjectARB(0);
		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
	else
	{
		for (j = 0; j < (*psubmodel)->nummesh; j++)
		{
			pmesh = (mstudiomesh_t *)((byte *)(*pstudiohdr) + (*psubmodel)->meshindex) + j;

			auto ptricmds = (short *)((byte *)(*pstudiohdr) + pmesh->triindex);

			(*c_alias_polys) += pmesh->numtris;

			flags = ptexture[pskinref[pmesh->skinref]].flags | (*g_ForcedFaceFlags);

			if (r_fullbright->value >= 2)
			{
				flags = flags & 0xFC;
			}

			bool bTransparent = false;

			if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);

				bTransparent = true;
			}
			else if (flags & STUDIO_NF_MASKED)
			{
				qglEnable(GL_ALPHA_TEST);
				qglAlphaFunc(GL_GREATER, 0.5);
				qglDepthMask(GL_TRUE);
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(GL_FALSE);
				qglShadeModel(GL_SMOOTH);
			}

			//Transparent object?

			//GLboolean writemask;
			//qglGetBooleanv(GL_DEPTH_WRITEMASK, &writemask);

			if (bTransparent)
			{
				R_SetGBufferRenderState(GBUFFER_STATE_TRANSPARENT_DIFFUSE);
				R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);
			}
			else
			{
				R_SetGBufferRenderState(GBUFFER_STATE_DIFFUSE);
				R_SetGBufferMask(GBUFFER_MASK_ALL);
			}

			float s, t;
			//setup texture and texcoord
			if (r_fullbright->value >= 2)
			{
				gEngfuncs.pTriAPI->SpriteTexture(cl_sprite_white, 0);
				s = 1.0f / 256.0f;
				t = 1.0f / 256.0f;
			}
			else
			{
				s = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].width;
				t = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].height;

				gRefFuncs.R_StudioSetupSkin(ptexturehdr, pskinref[pmesh->skinref]);
			}

			int iStartDrawVertex;
			int iNumDrawVertex;
			int iCurrentDrawType;

			studio_vbo_mesh_t *VBOMesh = NULL;

			if (iInitVBO & 2)
				VBOMesh = &VBOSubmodel->vMesh[j];

			if (flags & STUDIO_NF_CHROME)//chrome start
			{
				if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)//force chrome, the fucking glowshell
				{
					s /= 32.0f;
					t /= 32.0f;

					while (i = *(ptricmds++))
					{
						if (i < 0)
						{
							qglBegin(GL_TRIANGLE_FAN);
							i = -i;

							if (iInitVBO & 2)
							{
								iCurrentDrawType = GL_TRIANGLE_FAN;
								iStartDrawVertex = VBOData->vVertex.size();
								iNumDrawVertex = 0;
							}
						}
						else
						{
							qglBegin(GL_TRIANGLE_STRIP);

							if (iInitVBO & 2)
							{
								iCurrentDrawType = GL_TRIANGLE_STRIP;
								iStartDrawVertex = VBOData->vVertex.size();
								iNumDrawVertex = 0;
							}
						}

						for (; i > 0; i--, ptricmds += 4)
						{
							int normalIndex = (*g_NormalIndex)[ptricmds[0]];

							qglTexCoord2f((*chrome)[normalIndex][0] * s, (*chrome)[normalIndex][1] * t);

							VectorRotate(pstudionorms[ptricmds[1]], (*pbonetransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[1]]);
							qglNormal3fv(r_studionormal[ptricmds[1]]);

							vec4_t col;
							col[0] = (float)(*currententity)->curstate.rendercolor.r / 255.0f;
							col[1] = (float)(*currententity)->curstate.rendercolor.g / 255.0f;
							col[2] = (float)(*currententity)->curstate.rendercolor.b / 255.0f;
							col[3] = 1.0f;
							qglColor4fv(col);

							av = &pauxverts[ptricmds[0]];
							qglVertex3fv(av->fv);

							if (iInitVBO & 2)
							{
								vec2_t stChrome = { (float)s, (float)t };
								VBOData->vVertex.emplace_back(pstudioverts[ptricmds[0]], pstudionorms[ptricmds[1]], stChrome, (int)pvertbone[ptricmds[0]], (int)pnormbone[ptricmds[1]]);
								iNumDrawVertex++;
							}
						}

						qglEnd();

						if (iInitVBO & 2)
						{
							VBOMesh->vTri.emplace_back(iStartDrawVertex, iNumDrawVertex, iCurrentDrawType);
						}
					}
				}
				else
				{//standard chrome, not glowshell
					s = 1.0f / 2048.0f;
					t = 1.0f / 2048.0f;

					while (i = *(ptricmds++))
					{
						if (i < 0)
						{
							qglBegin(GL_TRIANGLE_FAN);
							i = -i;

							if (iInitVBO & 2)
							{
								iCurrentDrawType = GL_TRIANGLE_FAN;
								iStartDrawVertex = VBOData->vVertex.size();
								iNumDrawVertex = 0;
							}
						}
						else
						{
							qglBegin(GL_TRIANGLE_STRIP);

							if (iInitVBO & 2)
							{
								iCurrentDrawType = GL_TRIANGLE_STRIP;
								iStartDrawVertex = VBOData->vVertex.size();
								iNumDrawVertex = 0;
							}
						}

						for (; i > 0; i--, ptricmds += 4)
						{
							qglTexCoord2f((*chrome)[ptricmds[1]][0] * s, (*chrome)[ptricmds[1]][1] * t);

							lv = &pvlightvalues[ptricmds[1] * 3];

							vec3_t vNormal;
							VectorCopy(pstudionorms[ptricmds[1]], vNormal);

							if (iFlippedVModel == 1)
								VectorInverse(vNormal);

							gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], vNormal, lv, fl);

							VectorRotate(pstudionorms[ptricmds[1]], (*pbonetransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[1]]);

							qglNormal3fv(r_studionormal[ptricmds[1]]);
							qglColor4f(fl[0], fl[1], fl[2], *r_blend);

							av = &pauxverts[ptricmds[0]];
							qglVertex3fv(av->fv);

							if (iInitVBO & 2)
							{
								vec2_t stChrome = { (float)s, (float)t };
								VBOData->vVertex.emplace_back(pstudioverts[ptricmds[0]], pstudionorms[ptricmds[1]], stChrome, (int)pvertbone[ptricmds[0]], (int)pnormbone[ptricmds[1]]);
								iNumDrawVertex++;
							}
						}

						qglEnd();

						if (iInitVBO & 2)
						{
							VBOMesh->vTri.emplace_back(iStartDrawVertex, iNumDrawVertex, iCurrentDrawType);
						}
					}
				}//no force chrome end

			}//chrome end
			else
			{//normal render

				while (i = *(ptricmds++))
				{
					if (i < 0)
					{
						qglBegin(GL_TRIANGLE_FAN);
						i = -i;

						if (iInitVBO & 2)
						{
							iCurrentDrawType = GL_TRIANGLE_FAN;
							iStartDrawVertex = VBOData->vVertex.size();
							iNumDrawVertex = 0;
						}
					}
					else
					{
						qglBegin(GL_TRIANGLE_STRIP);

						if (iInitVBO & 2)
						{
							iCurrentDrawType = GL_TRIANGLE_STRIP;
							iStartDrawVertex = VBOData->vVertex.size();
							iNumDrawVertex = 0;
						}
					}

					for (; i > 0; i--, ptricmds += 4)
					{
						vec2_t st = { ptricmds[2] * s, ptricmds[3] * t };
						qglTexCoord2fv(st);

						lv = &(pvlightvalues[ptricmds[1] * 3]);

						vec3_t vNormal;
						VectorCopy(pstudionorms[ptricmds[1]], vNormal);

						if (iFlippedVModel)
							VectorInverse(vNormal);

						gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], vNormal, lv, fl);

						VectorRotate(pstudionorms[ptricmds[1]], (*pbonetransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[1]]);

						qglNormal3fv(r_studionormal[ptricmds[1]]);

						if (flags & STUDIO_NF_FULLBRIGHT)
						{
							qglColor4f(1, 1, 1, *r_blend);
						}
						else
						{
							qglColor4f(fl[0], fl[1], fl[2], *r_blend);
						}

						av = &(pauxverts[ptricmds[0]]);
						qglVertex3fv(av->fv);

						if (iInitVBO & 2)
						{
							VBOData->vVertex.emplace_back(pstudioverts[ptricmds[0]], pstudionorms[ptricmds[1]], st, (int)pvertbone[ptricmds[0]], (int)pnormbone[ptricmds[1]]);
							iNumDrawVertex++;
						}
					}

					qglEnd();

					if (iInitVBO & 2)
					{
						VBOMesh->vTri.emplace_back(iStartDrawVertex, iNumDrawVertex, iCurrentDrawType);
					}
				}

			}//normal draw end

			if (flags & STUDIO_NF_MASKED)
			{
				qglAlphaFunc(GL_NOTEQUAL, 0);
				qglDisable(GL_ALPHA_TEST);
			}
			else if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglDisable(GL_BLEND);
				qglDepthMask(1);
				qglShadeModel(GL_FLAT);
			}

			if (iInitVBO & 2)
			{
				//Convert vTri into indices
				for (int t = 0; t < VBOMesh->vTri.size(); ++t)
				{
					auto &tri = VBOMesh->vTri[t];
					if (tri.draw_type == GL_TRIANGLE_STRIP)
					{
						for (int k = 0; k < tri.num_vertex; ++k)
						{
							VBOMesh->vTriStrip.emplace_back((unsigned int)(tri.start_vertex + k));
						}
						//Restart Primitives
						VBOMesh->vTriStrip.emplace_back((unsigned int)0xFFFFFFFF);
					}
					else if (tri.draw_type == GL_TRIANGLE_FAN)
					{
						for (int k = 0; k < tri.num_vertex; ++k)
						{
							VBOMesh->vTriFan.emplace_back((unsigned int)(tri.start_vertex + k));
						}
						//Restart Primitives
						VBOMesh->vTriFan.emplace_back((unsigned int)0xFFFFFFFF);
					}
					
				}

				//Push indices into indices buffer
				if (VBOMesh->vTriStrip.size() > 0)
				{
					VBOMesh->iTriStripStartIndex = VBOData->vIndices.size();
					for (size_t k = 0; k < VBOMesh->vTriStrip.size(); ++k)
					{
						VBOData->vIndices.emplace_back(VBOMesh->vTriStrip[k]);
						VBOMesh->iTriStripVertexCount++;
					}

					VBOMesh->vTriStrip.shrink_to_fit();
				}

				if (VBOMesh->vTriFan.size() > 0)
				{
					VBOMesh->iTriFanStartIndex = VBOData->vIndices.size();
					for (size_t k = 0; k < VBOMesh->vTriFan.size(); ++k)
					{
						VBOData->vIndices.emplace_back(VBOMesh->vTriFan[k]);
						VBOMesh->iTriFanVertexCount++;
					}

					VBOMesh->vTriFan.shrink_to_fit();
				}
			}

		}//mesh draw end

	}//non-VBO way

	qglEnable(GL_CULL_FACE);

	//upload all data and indices to GPU
	if (iInitVBO & 2)
	{
		if (!VBOData->hDataBuffer)
		{
			qglGenBuffersARB(1, &VBOData->hDataBuffer);
		}
		if (!VBOData->hIndexBuffer)
		{
			qglGenBuffersARB(1, &VBOData->hIndexBuffer);
		}

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, VBOData->hDataBuffer);
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, VBOData->vVertex.size() * sizeof(studio_vbo_vertex_t), VBOData->vVertex.data(), GL_STATIC_DRAW_ARB);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, VBOData->hIndexBuffer);
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, VBOData->vIndices.size() * sizeof(unsigned int), VBOData->vIndices.data(), GL_STATIC_DRAW_ARB);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

		VBOData->vVertex.shrink_to_fit();
		VBOData->vIndices.shrink_to_fit();
	}
}

void R_StudioRenderFinal(void)
{
	pauxverts = &auxverts[0];
	pvlightvalues = (float *)&lightvalues[0];

	gRefFuncs.R_StudioRenderFinal();
}

//StudioAPI

void studioapi_StudioDynamicLight(cl_entity_t *ent, alight_t *plight)
{
	if (!r_light_dynamic->value)
		return gRefFuncs.studioapi_StudioDynamicLight(ent, plight);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		float dies[256];

		dlight_t *dl = cl_dlights;
		for (int i = 0; i < 256; i++, dl++)
		{
			dies[i] = dl->die;
			dl->die = 0;
		}

		gRefFuncs.studioapi_StudioDynamicLight(ent, plight);

		dl = cl_dlights;
		for (int i = 0; i < 256; i++, dl++)
		{
			dl->die = dies[i];
		}
	}
	else
	{
		float dies[32];
		dlight_t *dl = cl_dlights;
		for (int i = 0; i < 32; i++, dl++)
		{
			dies[i] = dl->die;
			dl->die = 0;
		}

		gRefFuncs.studioapi_StudioDynamicLight(ent, plight);

		dl = cl_dlights;
		for (int i = 0; i < 32; i++, dl++)
		{
			dl->die = dies[i];
		}
	}
}

void studioapi_SetupRenderer(int rendermode)
{
	pauxverts = &auxverts[0];
	pvlightvalues = (float *)&lightvalues[0];

	gRefFuncs.studioapi_SetupRenderer(rendermode);
}

void studioapi_RestoreRenderer(void)
{
	qglDepthMask(1);
	gRefFuncs.studioapi_RestoreRenderer();
}