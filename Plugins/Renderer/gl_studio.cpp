#include "gl_local.h"
#include "triangleapi.h"
#include "cJSON.h"
//#include <sselib.h>

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
int (*lightgammatable)[1024];
float *g_ChromeOrigin;
int *r_smodels_total;
int *r_ambientlight;
float *r_shadelight;
vec3_t(*r_blightvec)[MAXSTUDIOBONES];
vec3_t *r_plightvec;
vec3_t *r_colormix;

//renderer
vec3_t r_studionormal[MAXSTUDIOVERTS];
float lightpos[MAXSTUDIOVERTS][3][4];
auxvert_t auxverts[MAXSTUDIOVERTS];
vec3_t lightvalues[MAXSTUDIOVERTS];
auxvert_t *pauxverts;
float *pvlightvalues;

SHADER_DEFINE(studio);

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
		}//texture load end			
	}//model load end		
	cJSON_Delete(root);
	gEngfuncs.COM_FreeFile((void *) pFile );
}

void R_InitStudio(void)
{
	
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
	qboolean iFlippedVModel;
	qboolean has_extra_texture;
	qboolean use_extra_texture;
	int replace_texture;
	qboolean iNoBaseTexture;
	studio_texarray_t *pTexArray;

	pvertbone = ((byte *)(*pstudiohdr) + (*psubmodel)->vertinfoindex);
	pnormbone = ((byte *)(*pstudiohdr) + (*psubmodel)->norminfoindex);
	ptexturehdr = R_LoadTextures(*r_model);
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

	if((*currententity)->curstate.renderfx != kRenderFxShadow)
	{
		for (i = 0; i < (*psubmodel)->numverts; i++)
		{
			R_LightStrength(pvertbone[i], pstudioverts[i], lightpos[i]);
		}
		lv = pvlightvalues;
	}

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

				if (flags & STUDIO_NF_CHROME)
				{
					int m = (int)((char *)lv - (char *)pvlightvalues) / 12;
					gRefFuncs.R_StudioChrome((*chrome)[m], *pnormbone, *pstudionorms);
				}

				lv[0] = lv_tmp * (*r_colormix)[0];
				lv[1] = lv_tmp * (*r_colormix)[1];
				lv[2] = lv_tmp * (*r_colormix)[2];
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
		{
			flags = flags & 0xFC;
		}

		if ((*currententity)->curstate.renderfx == kRenderFxGlowShell)
		{
			qglBlendFunc(GL_ONE, GL_ONE);
			qglEnable(GL_BLEND);
			qglDepthMask(GL_FALSE);
			qglShadeModel(GL_SMOOTH);
		}

		if (flags & STUDIO_NF_MASKED)
		{
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GREATER, 0.5);
			qglDepthMask(GL_TRUE);
		}

		if ((flags & STUDIO_NF_ADDITIVE) && (*currententity)->curstate.rendermode == kRenderNormal)
		{
			qglBlendFunc(GL_ONE, GL_ONE);
			qglEnable(GL_BLEND);
			qglDepthMask(GL_FALSE);
			qglShadeModel(GL_SMOOTH);
		}

		R_SetGBufferRenderState(1);

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

		if (flags & STUDIO_NF_CHROME)//chrome start
		{
			//GL_SelectTexture(GL_TEXTURE0_ARB);
			//qglEnable(GL_TEXTURE_2D);

			if ((*g_ForcedFaceFlags) & STUDIO_NF_CHROME)//force chrome
			{
				s /= 256.0f;
				t /= 256.0f;

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
						int normalIndex = (*g_NormalIndex)[ptricmds[0]];
						qglTexCoord2f((*chrome)[normalIndex][0] * s, (*chrome)[normalIndex][1] * t);

#ifndef SSE
						VectorRotate(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#else
						VectorRotateSSE(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#endif

						qglNormal3fv(r_studionormal[ptricmds[0]]);

						av = &pauxverts[ptricmds[0]];
						qglVertex3fv(av->fv);
					}

					qglEnd();
				}
			}
			else
			{//no force chrome
				s = 1.0f / 2048.0f;
				t = 1.0f / 2048.0f;

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
						qglTexCoord2f((*chrome)[ptricmds[1]][0] * s, (*chrome)[ptricmds[1]][1] * t);

						lv = &pvlightvalues[ptricmds[1] * 3];

						vec3_t vNormal;
						VectorCopy(pstudionorms[ptricmds[1]], vNormal);

						if (iFlippedVModel == 1)
						{
							VectorScale(vNormal, -1, vNormal);
						}

						gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], vNormal, lv, fl);

#ifndef SSE
						VectorRotate(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#else
						VectorRotateSSE(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#endif

						qglNormal3fv(r_studionormal[ptricmds[0]]);

						qglColor4f(fl[0], fl[1], fl[2], *r_blend);

						av = &pauxverts[ptricmds[0]];
						qglVertex3fv(av->fv);
					}

					qglEnd();
				}
			}//no force chrome end

			//restore gbuffer state
			R_SetGBufferRenderState(2);

		}//chrome end
		else
		{//normal render

			s = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].width;
			t = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].height;

			use_extra_texture = false;
			replace_texture = 0;

			R_SetGBufferRenderState(1);

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
							break;
						}
					}
				}
				if(use_extra_texture)
				{
					if (replace_texture)
					{
						GL_SelectTexture(GL_TEXTURE0_ARB);
						qglEnable(GL_TEXTURE_2D);
						GL_Bind(replace_texture);
					}
					else
					{
						gRefFuncs.R_StudioSetupSkin(ptexturehdr, pskinref[pmesh->skinref]);
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
					qglTexCoord2f(ptricmds[2] * s, ptricmds[3] * t);

					lv = &(pvlightvalues[ptricmds[1] * 3]);

					vec3_t vNormal;
					VectorCopy(pstudionorms[ptricmds[1]], vNormal);

					if (iFlippedVModel)
						VectorInverse(vNormal);

					gRefFuncs.R_LightLambert(lightpos[ptricmds[0]], vNormal, lv, fl);

#ifndef SSE
					VectorRotate(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#else
					VectorRotateSSE(pstudionorms[ptricmds[1]], (*plighttransform)[pnormbone[ptricmds[1]]], r_studionormal[ptricmds[0]]);
#endif

					qglNormal3fv(r_studionormal[ptricmds[0]]);

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
				}

				qglEnd();
			}

			R_SetGBufferRenderState(2);

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