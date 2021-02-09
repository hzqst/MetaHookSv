#include "gl_local.h"
#include "cJSON.h"
//#include <sselib.h>

//engine
mstudiomodel_t **psubmodel;
studiohdr_t **pstudiohdr;
model_t **r_model;
float *r_blend;
float (*pbonetransform)[MAXSTUDIOBONES][3][4];
float (*plighttransform)[MAXSTUDIOBONES][3][4];
int (*g_NormalIndex)[MAXSTUDIOVERTS];
int (*chromeage)[MAXSTUDIOBONES];
cl_entity_t *cl_viewent;
int *g_ForcedFaceFlags;
int (*lightgammatable)[1024];

//renderer
vec3_t r_studionormal[MAXSTUDIOVERTS];
vec3_t r_studiotangent[MAXSTUDIOVERTS];
int chrome[MAXSTUDIOVERTS][2];
float lightpos[MAXSTUDIOVERTS][3][4];
auxvert_t auxverts[MAXSTUDIOVERTS];
vec3_t lightvalues[MAXSTUDIOVERTS];
auxvert_t *pauxverts;
float *pvlightvalues;
vec3_t r_colormix;
vec3_t r_blightvec[MAXSTUDIOBONES];
vec3_t r_plightvec;
int r_ambientlight;
float r_shadelight;

SHADER_DEFINE(studio);

cvar_t *gl_studionormal;
cvar_t *r_pplightambient;
cvar_t *r_pplightdiffuse;
cvar_t *r_pplightspecular;
cvar_t *r_pplightshiness;

studio_texarray_mgr_t g_TexArray;
studio_texarray_mgr_t g_LocalTexArray;

int Q_stricmp_slash(const char *s1, const char *s2);
void VectorIRotate(const vec3_t in1, const float in2[3][4], vec3_t out);
void VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out);

SHADER_DEFINE(invuln);

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
			R_LoadStudioTextures_Normal(tex, "normal", &pTex->normal);
		}//texture load end			
	}//model load end		
	cJSON_Delete(root);
	gEngfuncs.COM_FreeFile((void *) pFile );
}

void R_InitStudio(void)
{
	if(gl_shader_support)
	{
		char *studio_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\studio_shader.vsh", 5, 0);
		char *studio_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\studio_shader.fsh", 5, 0);

		if(studio_vscode && studio_fscode)
		{
			studio.program = R_CompileShader(studio_vscode, studio_fscode, "studio_shader.vsh", "studio_shader.fsh");
			if(studio.program)
			{
				SHADER_UNIFORM(studio, lightpos, "lightpos");
				SHADER_UNIFORM(studio, eyepos, "eyepos");
				SHADER_UNIFORM(studio, basemap, "basemap");
				SHADER_UNIFORM(studio, normalmap, "normalmap");
				SHADER_UNIFORM(studio, ambient, "ambient");
				SHADER_UNIFORM(studio, diffuse, "diffuse");
				SHADER_UNIFORM(studio, specular, "specular");
				SHADER_UNIFORM(studio, shiness, "shiness");
				SHADER_ATTRIB(studio, tangent, "tangent");
				SHADER_ATTRIB(studio, binormal, "binormal");
			}
		}
		gEngfuncs.COM_FreeFile((void *) studio_vscode);
		gEngfuncs.COM_FreeFile((void *) studio_fscode);

		char *invuln_vscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\invuln_shader.vsh", 5, 0);
		char *invuln_fscode = (char *)gEngfuncs.COM_LoadFile("resource\\shader\\invuln_shader.fsh", 5, 0);
		if(invuln_vscode && invuln_fscode)
		{
			invuln.program = R_CompileShader(invuln_vscode, invuln_fscode, "invuln_shader.vsh", "invuln_shader.fsh");
			if(invuln.program)
			{
				SHADER_UNIFORM(invuln, basemap, "basemap");
				SHADER_UNIFORM(invuln, normalmap, "normalmap");
				SHADER_UNIFORM(invuln, time, "time");
			}
		}
		gEngfuncs.COM_FreeFile((void *) invuln_vscode);
		gEngfuncs.COM_FreeFile((void *) invuln_fscode);
	}

	gl_studionormal = gEngfuncs.pfnRegisterVariable("gl_studionormal", "0", FCVAR_CLIENTDLL);
	r_pplightambient = gEngfuncs.pfnRegisterVariable("r_pplightambient", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_pplightdiffuse = gEngfuncs.pfnRegisterVariable("r_pplightdiffuse", "0.3", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_pplightspecular = gEngfuncs.pfnRegisterVariable("r_pplightspecular", "0.1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_pplightshiness = gEngfuncs.pfnRegisterVariable("r_pplightshiness", "2.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
}

//Engine Studio

void R_StudioLighting(float *lv, int bone, int flags, vec3_t normal)
{
	static float v_lambert1 = 1.4953241;

	float illum;
	float lightcos;

	illum = r_ambientlight;

	if (flags & STUDIO_NF_FULLBRIGHT)
	{
		illum = 255;
	}
	else if (flags & STUDIO_NF_FLATSHADE)
	{
		illum += r_shadelight * 0.8;
	}
	else
	{
		float r;
#ifndef SSE
		if (bone != -1)
			lightcos = DotProduct(normal, r_blightvec[bone]);
		else
			lightcos = DotProduct(normal, r_plightvec);
#else
		if (bone != -1)
			DotProductSSE(&lightcos, normal, r_blightvec[bone]);
		else
			DotProductSSE(&lightcos, normal, r_plightvec);
#endif
		if (lightcos > 1.0)
			lightcos = 1;

		illum += r_shadelight;

		r = v_lambert1;
		lightcos = (lightcos + (r - 1.0)) / r;

		if (lightcos > 0)
		{
			illum -= r_shadelight * lightcos;
		}

		if (illum <= 0)
			illum = 0;
	}

	if (illum > 255)
		illum = 255;

	*lv = (*lightgammatable)[(int)(illum * 4)] / 1023.0;
}

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

void R_GLStudioDrawPointsEx(void)
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
	qboolean iFlippedVModel;
	qboolean has_extra_texture;
	qboolean use_extra_texture;
	int replace_texture;
	int normal_texture;
	qboolean iNoBaseTexture;
	studio_texarray_t *pTexArray;

	pvertbone = ((byte *)(*pstudiohdr) + (*psubmodel)->vertinfoindex);
	pnormbone = ((byte *)(*pstudiohdr) + (*psubmodel)->norminfoindex);
	ptexturehdr = gRefFuncs.R_LoadTextures(*r_model);
	ptexture = (mstudiotexture_t *)((byte *)ptexturehdr + ptexturehdr->textureindex);

	pmesh = (mstudiomesh_t *)((byte *)(*pstudiohdr) + (*psubmodel)->meshindex);

	pstudioverts = (vec3_t *)((byte *)(*pstudiohdr) + (*psubmodel)->vertindex);
	pstudionorms = (vec3_t *)((byte *)(*pstudiohdr) + (*psubmodel)->normindex);

	pskinref = (short *)((byte *)ptexturehdr + ptexturehdr->skinindex);

	iFlippedVModel = false;
	iNoBaseTexture = false;

	if ((*currententity)->curstate.skin != 0 && (*currententity)->curstate.skin < ptexturehdr->numskinfamilies)
		pskinref += ((*currententity)->curstate.skin * ptexturehdr->numskinref);

	if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
	{
		gRefFuncs.BuildNormalIndexTable();
		gRefFuncs.BuildGlowShellVerts(pstudioverts, pauxverts);
	}
	else
	{
		for (i = 0; i < (*psubmodel)->numverts; i++)
		{
			av = &(pauxverts[i]);
			R_StudioTransformAuxVert(av, pvertbone[i], pstudioverts[i]);
		}
	}

	if((*currententity)->curstate.renderfx != kRenderFxShadow)
	{
		for (i = 0; i < (*psubmodel)->numverts; i++)
		{
			gRefFuncs.R_LightStrength(pvertbone[i], pstudioverts[i], lightpos[i]);
		}
		lv = pvlightvalues;
	}

	for (j = 0; j < (*psubmodel)->nummesh; j++)
	{
		flags = ptexture[pskinref[pmesh[j].skinref]].flags | (*g_ForcedFaceFlags);

		if (r_fullbright->value >= 2)
			flags |= STUDIO_NF_FULLBRIGHT;

		if((*currententity)->curstate.renderfx == kRenderFxFireLayer)
		{
			flags &= ~STUDIO_NF_CHROME;
		}
		else if((*currententity)->curstate.renderfx == kRenderFxInvulnLayer)
		{
			flags |= STUDIO_NF_CHROME;
		}
		else if((*currententity)->curstate.renderfx == kRenderFxCloak)
		{
			flags &= ~STUDIO_NF_CHROME;
		}

		if((*currententity)->curstate.renderfx != kRenderFxShadow)
		{
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
						gRefFuncs.R_StudioChrome(chrome[m], *pnormbone, *pstudionorms);
					}
				}
			}
			else
			{
				for (k = 0; k < pmesh[j].numnorms; k++, lv += 3, pstudionorms++, pnormbone++)
				{
					R_StudioLighting(&lv_tmp, *pnormbone, flags, *pstudionorms);

					if (flags & STUDIO_NF_CHROME)
					{
						int m = (int)((char *)lv - (char *)pvlightvalues) / 12;
						gRefFuncs.R_StudioChrome(chrome[m], *pnormbone, *pstudionorms);
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

	if((*currententity)->curstate.renderfx == kRenderFxShadow || (*currententity)->curstate.renderfx == kRenderFxCloak || (*currententity)->curstate.renderfx == kRenderFxGlowShell || (*currententity)->curstate.renderfx == kRenderFxFireLayer || (*currententity)->curstate.renderfx == kRenderFxInvulnLayer )
	{
		iNoBaseTexture = true;
	}

	if(!iNoBaseTexture)
	{
		has_extra_texture = false;
		for(i = 0; i < g_LocalTexArray.iNumTexArray; ++i)
		{
			if(!Q_stricmp_slash(g_LocalTexArray.pTexArray[i].modelname, (*r_model)->name))
			{
				has_extra_texture = true;
				pTexArray = &g_LocalTexArray.pTexArray[i];
				break;
			}
		}
		if(!has_extra_texture)
		{
			for(i = 0; i < g_TexArray.iNumTexArray; ++i)
			{
				if(!Q_stricmp_slash(g_TexArray.pTexArray[i].modelname, (*r_model)->name))
				{
					has_extra_texture = true;
					pTexArray = &g_TexArray.pTexArray[i];
					break;
				}
			}
		}
	}

	for (j = 0; j < (*psubmodel)->nummesh; j++) 
	{
		float s, t;
		short *ptricmds;

		pmesh = (mstudiomesh_t *)((byte *)(*pstudiohdr) + (*psubmodel)->meshindex) + j;
		ptricmds = (short *)((byte *)(*pstudiohdr) + pmesh->triindex);

		//c_alias_polys += pmesh->numtris;

		flags = ptexture[pskinref[pmesh->skinref]].flags | (*g_ForcedFaceFlags);

		if (r_fullbright->value >= 2)
			flags |= STUDIO_NF_FULLBRIGHT;

		if((*currententity)->curstate.renderfx == kRenderFxCloak)
		{
			if ((flags & STUDIO_NF_MASKED) || (flags & STUDIO_NF_ADDITIVE))
			{
				continue;
			}
			qglDepthMask(0);
			qglEnable(GL_BLEND);
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			qglEnable(GL_POLYGON_OFFSET_FILL);
			qglPolygonOffset(-1, -gl_polyoffset->value);
			flags &= ~STUDIO_NF_CHROME;
		}
		else if((*currententity)->curstate.renderfx == kRenderFxFireLayer)
		{
			if ((flags & STUDIO_NF_MASKED) || (flags & STUDIO_NF_ADDITIVE))
			{
				continue;
			}
			qglDepthMask(0);
			qglEnable(GL_BLEND);
			qglBlendFunc(GL_SRC_COLOR, GL_ONE);
			qglEnable(GL_POLYGON_OFFSET_FILL);
			qglPolygonOffset(-1, -gl_polyoffset->value);
			flags &= ~STUDIO_NF_CHROME;
		}
		else if((*currententity)->curstate.renderfx == kRenderFxInvulnLayer)
		{
			if ((flags & STUDIO_NF_MASKED) || (flags & STUDIO_NF_ADDITIVE))
			{
				continue;
			}
			qglDepthMask(0);
			qglEnable(GL_BLEND);
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			qglEnable(GL_POLYGON_OFFSET_FILL);
			qglPolygonOffset(-1, -gl_polyoffset->value);
			flags |= STUDIO_NF_CHROME;
		}
		else if((*currententity)->curstate.renderfx != kRenderFxShadow)
		{
			if (flags & STUDIO_NF_MASKED)
			{
				qglEnable(GL_ALPHA_TEST);
				qglAlphaFunc(GL_GREATER, 0.5);
				qglDepthMask(1);
			}

			if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglBlendFunc(GL_ONE, GL_ONE);
				qglEnable(GL_BLEND);
				qglDepthMask(0);
				qglShadeModel(GL_SMOOTH);
			}
		}

		if((*currententity)->curstate.renderfx == kRenderFxShadow)
		{
			while (i = *(ptricmds++))
			{
				if (i < 0)
				{
					qglBegin(GL_TRIANGLE_FAN);
					i = -i;
				}
				else
				{
					qglBegin(GL_TRIANGLE_STRIP);
				}

				for ( ; i > 0; i--, ptricmds += 4)
				{
					av = &(pauxverts[ptricmds[0]]);
					qglVertex3fv(av->fv);
				}

				qglEnd();
			}
		}
		else if((*currententity)->curstate.renderfx == kRenderFxCloak)
		{
			GL_DisableMultitexture();
			GL_Bind(cloak_texture);
			qglUseProgramObjectARB(cloak.program);
			qglUniform1iARB(cloak.refract, 0);
			qglUniform3fARB(cloak.eyepos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);			
			qglUniform1fARB(cloak.cloakfactor, clamp((255 - (*currententity)->curstate.renderamt) / 255.0, 0, 1));
			qglUniform1fARB(cloak.refractamount, 2.0f);

			while (i = *(ptricmds++))
			{
				if (i < 0)
				{
					qglBegin(GL_TRIANGLE_FAN);
					i = -i;
				}
				else
				{
					qglBegin(GL_TRIANGLE_STRIP);
				}

				for ( ; i > 0; i--, ptricmds += 4)
				{
					short *ptricmds2;
					if(i <= 1)
						ptricmds2 = ptricmds-4;
					else
						ptricmds2 = ptricmds+4;

					//VectorSubtract((pauxverts[ptricmds2[0]]).fv, (pauxverts[ptricmds[0]]).fv, r_studiotangent[ptricmds[0]]);
#ifndef SSE
					//VectorNormalize(r_studiotangent[ptricmds[0]]);
					VectorRotate(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#else
					//VectorNormalizeSSE(r_studiotangent[ptricmds[0]]);
					VectorRotateSSE(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#endif
					//if (iFlippedVModel)
					//	VectorInverse(r_studionormal[ptricmds[0]]);

					//qglVertexAttrib3fv(studio_attrib.tangent, r_studiotangent[ptricmds[0]]);
					qglNormal3fv(r_studionormal[ptricmds[0]]);

					av = &(pauxverts[ptricmds[0]]);
					qglVertex3fv(av->fv);
				}

				qglEnd();
			}

			qglUseProgramObjectARB(0);

			if(gl_studionormal->value == 1)
			{
				qglDisable(GL_TEXTURE_2D);		
				ptricmds = (short *)((byte *)(*pstudiohdr) + pmesh->triindex);
				while (i = *(ptricmds++))
				{
					if (i < 0)
					{
						i = -i;
					}

					for ( ; i > 0; i--, ptricmds += 4)
					{
						av = &(pauxverts[ptricmds[0]]);
						qglColor4f(0,1,0,1);
						qglBegin(GL_LINES);
						qglVertex3f(av->fv[0], av->fv[1], av->fv[2]);
						qglVertex3f(av->fv[0]+r_studionormal[ptricmds[0]][0], av->fv[1]+r_studionormal[ptricmds[0]][1], av->fv[2]+r_studionormal[ptricmds[0]][2]);
						qglEnd();
						qglColor4f(0,0,1,1);
						qglBegin(GL_LINES);
						qglVertex3f(av->fv[0], av->fv[1], av->fv[2]);
						qglVertex3f(av->fv[0]+r_studiotangent[ptricmds[0]][0], av->fv[1]+r_studiotangent[ptricmds[0]][1], av->fv[2]+r_studiotangent[ptricmds[0]][2]);
						qglEnd();
					}
				}
				qglEnable(GL_TEXTURE_2D);
			}
		}
		else if (flags & STUDIO_NF_CHROME)//chrome start
		{
			s = (1 / (float)ptexture[pskinref[pmesh->skinref]].width) / 1024;
			t = (1 / (float)ptexture[pskinref[pmesh->skinref]].height) / 1024;

			normal_texture = 0;

			if(!iNoBaseTexture)
			{
				gRefFuncs.R_StudioSetupSkin(ptexturehdr, pskinref[pmesh->skinref]);
			}
			else
			{
				if( (*currententity)->curstate.renderfx == kRenderFxInvulnLayer )
				{
					//normal_texture = water_normalmap;

					GL_EnableMultitexture();
					qglEnable(GL_TEXTURE_2D);
					GL_Bind(normal_texture);

					qglUseProgramObjectARB(invuln.program);

					qglUniform1iARB(invuln.basemap, 0);
					qglUniform1iARB(invuln.normalmap, 1);
					qglUniform1fARB(invuln.time, (*cl_time) * 0.3f);
				}
			}

			if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)
			{//force chrome
				while (i = *(ptricmds++))
				{
					if (i < 0)
					{
						qglBegin(GL_TRIANGLE_FAN);
						i = -i;
					}
					else
					{
						qglBegin(GL_TRIANGLE_STRIP);
					}

					for ( ; i > 0; i--, ptricmds += 4)
					{
						int normalIndex;

						normalIndex = (*g_NormalIndex)[ptricmds[0]];
						qglTexCoord2f(chrome[normalIndex][0] * s, chrome[normalIndex][1] * t);

						av = &pauxverts[ptricmds[0]];
						qglVertex3f(av->fv[0], av->fv[1], av->fv[2]);
					}

					qglEnd();
				}
			}
			else
			{//no force chrome
				while (i = *(ptricmds++))
				{
					if (i < 0)
					{
						qglBegin(GL_TRIANGLE_FAN);
						i = -i;
					}
					else
					{
						qglBegin(GL_TRIANGLE_STRIP);
					}

					for ( ; i > 0; i--, ptricmds += 4)
					{
						if(normal_texture)
							qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, chrome[ptricmds[1]][0] * s, chrome[ptricmds[1]][1] * t);
						else
							qglTexCoord2f(chrome[ptricmds[1]][0] * s, chrome[ptricmds[1]][1] * t);

						lv = &pvlightvalues[ptricmds[1] * 3];
						gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], pstudionorms[ptricmds[1]], lv, fl);
						qglColor4f(fl[0], fl[1], fl[2], *r_blend);

						av = &pauxverts[ptricmds[0]];
						qglVertex3f(av->fv[0], av->fv[1], av->fv[2]);
					}

					qglEnd();
				}
			}//no force chrome end

			if(normal_texture)
			{
				qglEnable(GL_BLEND);
				qglUseProgramObjectARB(0);
				GL_DisableMultitexture();
			}

		}//chrome end
		else
		{//normal render

			s = 1.0 / (float)ptexture[pskinref[pmesh->skinref]].width;
			t = 1.0 / (float)ptexture[pskinref[pmesh->skinref]].height;

			GL_SelectTexture(GL_TEXTURE0_ARB);
			qglEnable(GL_TEXTURE_2D);

			use_extra_texture = false;
			replace_texture = 0;
			normal_texture = 0;

			if(!iNoBaseTexture)
			{
				if(has_extra_texture)
				{
					for(k = 0; k < pTexArray->numtextures; ++k)
					{
						if(!stricmp(pTexArray->textures[k].base.name, ptexture[pskinref[pmesh->skinref]].name))
						{
							use_extra_texture = true;
							if(pTexArray->textures[k].replace.index)
								replace_texture = pTexArray->textures[k].replace.index;
							if(pTexArray->textures[k].normal.index)
								normal_texture = pTexArray->textures[k].normal.index;
							break;
						}
					}
				}
				if(use_extra_texture)
				{
					if(replace_texture)
						GL_Bind(replace_texture);
					else
						gRefFuncs.R_StudioSetupSkin(ptexturehdr, pskinref[pmesh->skinref]);
					if(normal_texture)
					{
						GL_EnableMultitexture();
						qglEnable(GL_TEXTURE_2D);
						GL_Bind(normal_texture);

						qglUseProgramObjectARB(studio.program);

						vec3_t vlightpos;
						VectorMA((*currententity)->origin, -10000, r_plightvec, vlightpos);
						qglUniform3fARB(studio.lightpos, vlightpos[0], vlightpos[1], vlightpos[2]);
						qglUniform3fARB(studio.eyepos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);
						qglUniform1iARB(studio.basemap, 0);
						qglUniform1iARB(studio.normalmap, 1);
						float ambient, diffuse, specular;
						ambient = max(r_pplightambient->value, 0.0);
						diffuse = max(r_pplightdiffuse->value, 0.0);
						specular = max(r_pplightspecular->value, 0.0);
						qglUniform4fARB(studio.ambient, r_colormix[0]*ambient, r_colormix[1]*ambient, r_colormix[2]*ambient, 1);
						qglUniform4fARB(studio.diffuse, r_colormix[0]*diffuse, r_colormix[1]*diffuse, r_colormix[2]*diffuse, 1);
						qglUniform4fARB(studio.specular, specular, specular, specular, 1);
						qglUniform1fARB(studio.shiness, max(r_pplightshiness->value, 0.0));
						qglDisable(GL_BLEND);
					}
				}
				else
				{
					gRefFuncs.R_StudioSetupSkin(ptexturehdr, pskinref[pmesh->skinref]);
				}
			}

			while (i = *(ptricmds++))
			{
				if (i < 0)
				{
					qglBegin(GL_TRIANGLE_FAN);
					i = -i;
				}
				else
				{
					qglBegin(GL_TRIANGLE_STRIP);
				}

				for ( ; i > 0; i--, ptricmds += 4)
				{
					if(normal_texture)
						qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, ptricmds[2] * s, ptricmds[3] * t);
					else
						qglTexCoord2f(ptricmds[2] * s, ptricmds[3] * t);

					lv = &(pvlightvalues[ptricmds[1] * 3]);

					vec3_t vNormal;
					VectorCopy(pstudionorms[ptricmds[1]], vNormal);

					if (iFlippedVModel)
						VectorInverse(vNormal);

					gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], vNormal, lv, fl);

					if(normal_texture)
					{
						short *ptricmds2;
						if(i <= 1)
							ptricmds2 = ptricmds-4;
						else
							ptricmds2 = ptricmds+4;

						VectorSubtract((pauxverts[ptricmds2[0]]).fv, (pauxverts[ptricmds[0]]).fv, r_studiotangent[ptricmds[0]]);
#ifndef SSE
						VectorNormalize(r_studiotangent[ptricmds[0]]);
						VectorRotate(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#else
						VectorNormalizeSSE(r_studiotangent[ptricmds[0]]);
						VectorRotateSSE(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#endif
						//if (iFlippedVModel)
						//	VectorInverse(r_studionormal[ptricmds[0]]);

						qglVertexAttrib3fv(studio.tangent, r_studiotangent[ptricmds[0]]);
						qglNormal3fv(r_studionormal[ptricmds[0]]);
					}

					qglColor4f(fl[0], fl[1], fl[2], *r_blend);

					av = &(pauxverts[ptricmds[0]]);
					qglVertex3fv(av->fv);
				}

				qglEnd();
			}
			if(normal_texture)
			{
				qglEnable(GL_BLEND);
				qglUseProgramObjectARB(0);
				GL_DisableMultitexture();

				if(gl_studionormal->value == 1)
				{
					qglDepthMask(0);
					qglDisable(GL_TEXTURE_2D);		
					ptricmds = (short *)((byte *)(*pstudiohdr) + pmesh->triindex);
					while (i = *(ptricmds++))
					{
						if (i < 0)
						{
							i = -i;
						}
				
						for ( ; i > 0; i--, ptricmds += 4)
						{
							av = &(pauxverts[ptricmds[0]]);
							qglColor4f(0,1,0,1);
							qglBegin(GL_LINES);
							qglVertex3f(av->fv[0], av->fv[1], av->fv[2]);
							qglVertex3f(av->fv[0]+r_studionormal[ptricmds[0]][0], av->fv[1]+r_studionormal[ptricmds[0]][1], av->fv[2]+r_studionormal[ptricmds[0]][2]);
							qglEnd();
							qglColor4f(0,0,1,1);
							qglBegin(GL_LINES);
							qglVertex3f(av->fv[0], av->fv[1], av->fv[2]);
							qglVertex3f(av->fv[0]+r_studiotangent[ptricmds[0]][0], av->fv[1]+r_studiotangent[ptricmds[0]][1], av->fv[2]+r_studiotangent[ptricmds[0]][2]);
							qglEnd();
						}
					}
					qglDepthMask(1);
					qglEnable(GL_TEXTURE_2D);
				}//gl_studionormal draw end
			}//finish normalmap
		}//normal draw end

		//restore render state
		if((*currententity)->curstate.renderfx == kRenderFxCloak)
		{
			qglDepthMask(1);
			qglDisable(GL_BLEND);
			qglDisable(GL_POLYGON_OFFSET_FILL);
		}
		else if((*currententity)->curstate.renderfx == kRenderFxFireLayer)
		{
			qglDepthMask(1);
			qglDisable(GL_BLEND);
			qglDisable(GL_POLYGON_OFFSET_FILL);
		}
		else if((*currententity)->curstate.renderfx == kRenderFxInvulnLayer)
		{
			qglDepthMask(1);
			qglDisable(GL_BLEND);
			qglDisable(GL_POLYGON_OFFSET_FILL);
		}
		else if((*currententity)->curstate.renderfx != kRenderFxShadow)
		{
			if (flags & STUDIO_NF_MASKED)
			{
				qglAlphaFunc(GL_NOTEQUAL, 0);
				qglDisable(GL_ALPHA_TEST);
			}

			if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
			{
				qglDisable(GL_BLEND);
				qglDepthMask(1);
				qglShadeModel(GL_FLAT);
			}
		}

	}//mesh draw end

	qglEnable(GL_CULL_FACE);
}

void R_StudioRenderFinal(void)
{
	pauxverts = &auxverts[0];
	pvlightvalues = (float *)&lightvalues[0];

	gRefFuncs.R_StudioRenderFinal();
}

//StudioAPI

void studioapi_StudioDrawPoints(void)
{
	R_GLStudioDrawPointsEx();
}

void studioapi_StudioSetupLighting(alight_t *plighting)
{
	r_ambientlight = plighting->ambientlight;
	r_shadelight = plighting->shadelight;

	VectorCopy(plighting->plightvec, r_plightvec);
	for (int i = 0; i < (*pstudiohdr)->numbones; i++)
		VectorIRotate(plighting->plightvec, (*plighttransform)[i], r_blightvec[i]);

	VectorCopy(plighting->color, r_colormix);
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