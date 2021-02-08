#include "gl_local.h"
#include "cJSON.h"

r_worldsurf_t r_wsurf;

cvar_t *r_wsurf_replace;
cvar_t *r_wsurf_sky;
cvar_t *r_wsurf_decal;

void R_ClearExtraTextures(void)
{
	r_wsurf.ExtraTextures.pTextures = NULL;
	r_wsurf.ExtraTextures.iNumTextures = 0;
	r_wsurf.LocalExtraTextures.pTextures = NULL;
	r_wsurf.LocalExtraTextures.iNumTextures = 0;
}

void R_ClearMapTextures(void)
{
	memset(r_wsurf.MapTextures, 0, sizeof(r_wsurf.MapTextures));
}

void R_ClearDecalTextures(void)
{
	memset(r_wsurf.DecalTextures, 0, sizeof(r_wsurf.DecalTextures));
	r_wsurf.iNumDecalTextures = 0;
}

void R_ClearSkyTextures(void)
{
	for(int i = 0; i < 6; ++i)
		r_wsurf.iSkyTextures[i] = 0;
}

/*void R_FreeDecalTextures(void)
{
	for(int i = 0; i < r_wsurf.iNumDecalTextures; ++i)
	{
		texture_t *t = &r_wsurf.DecalTextures[i];
		if(t->gl_texturenum && t->alternate_anims)
		{
			texture_t *link_decal = t->anim_next;
			if(link_decal)
			{
				link_decal->anim_next = NULL;
			}
			gltexture_t *glt = (gltexture_t *)t->alternate_anims;
			GL_FreeTexture(glt);
		}
	}
	R_ClearDecalTextures();
}*/

void R_InitDetailTextures(void)
{
	if(gl_mtexable > 2)
	{
		qglActiveTextureARB(TEXTURE2_SGIS);
		qglEnable(GL_TEXTURE_2D);
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		qglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		qglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2.0);
		qglDisable(GL_TEXTURE_2D);
		qglActiveTextureARB(TEXTURE0_SGIS);
	}
}

void R_FreeVertexBuffer(void)
{
	if (r_wsurf.hVBO)
	{
		qglDeleteBuffersARB(1, &r_wsurf.hVBO);
		r_wsurf.hVBO = 0;
	}
	if (r_wsurf.pVertexBuffer)
	{
		delete[] r_wsurf.pVertexBuffer;
		r_wsurf.pVertexBuffer = NULL;
	}
	if (r_wsurf.pFaceBuffer)
	{
		delete[] r_wsurf.pFaceBuffer;
		r_wsurf.pFaceBuffer = NULL;
	}
}

void R_GenerateVertexBuffer(void)
{
	brushvertex_t pVertexes[3];
	glpoly_t *poly;
	msurface_t *surf;
	float *v;
	int i, j;	

	int iNumFaces = 0;
	int iCurFace = 0;
	int iNumVerts = 0;
	int iCurVert = 0;

	surf = r_worldmodel->surfaces;

	for(i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		if (!(surf[i].flags & (SURF_DRAWSKY | SURF_UNDERWATER)))
		{
			for (poly = surf[i].polys; poly; poly = poly->next)
				iNumVerts += 3 + (poly->numverts-3)*3;

			iNumFaces++;
		}
	}

	//alloc vertex buffer
	r_wsurf.pVertexBuffer = new brushvertex_t[iNumVerts];
	memset(r_wsurf.pVertexBuffer, 0, sizeof(brushvertex_t)*iNumVerts);
	r_wsurf.iNumVerts = iNumVerts;

	//alloc face buffer
	r_wsurf.pFaceBuffer = new brushface_t[iNumFaces];
	memset(r_wsurf.pFaceBuffer, 0, sizeof(brushface_t)*iNumFaces);
	r_wsurf.iNumFaces = iNumFaces;

	for(i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		if ((surf[i].flags & (SURF_DRAWSKY | SURF_UNDERWATER)))
			continue;

		poly = surf[i].polys;

		poly->flags = iCurFace;
		brushface_t *face = &r_wsurf.pFaceBuffer[iCurFace];
		VectorCopy(surf[i].texinfo->vecs[0], face->s_tangent);
		VectorCopy(surf[i].texinfo->vecs[1], face->t_tangent);
		VectorNormalize(face->s_tangent);
		VectorNormalize(face->t_tangent);
		VectorCopy(surf[i].plane->normal, face->normal);
		face->index = i;

		if (surf[i].flags & SURF_PLANEBACK)
			VectorInverse(face->normal);

		// Link up with map textures
		face->maptex = NULL;
		for (j = 0; j < r_worldmodel->numtextures; j++)
		{
			if(!r_wsurf.MapTextures[j].basetex)
				continue;

			if(r_wsurf.MapTextures[j].basetex == surf[i].texinfo->texture)
			{
				face->maptex = &r_wsurf.MapTextures[j];
				break;
			}
		}

		face->start_vertex = iCurVert;
		for (poly = surf[i].polys; poly; poly = poly->next)
		{
			v = poly->verts[0];

			for(j = 0; j < 3; j++, v += VERTEXSIZE)
			{
				pVertexes[j].pos[0] = v[0];
				pVertexes[j].pos[1] = v[1];
				pVertexes[j].pos[2] = v[2];
				pVertexes[j].texcoord[0] = v[3];
				pVertexes[j].texcoord[1] = v[4];
				pVertexes[j].lightmaptexcoord[0] = v[5];
				pVertexes[j].lightmaptexcoord[1] = v[6];
				pVertexes[j].normal[0] = face->normal[0];
				pVertexes[j].normal[1] = face->normal[1];
				pVertexes[j].normal[2] = face->normal[2];				
				if(face->maptex && face->maptex->detailtex)
				{
					pVertexes[j].detailtexcoord[0] = v[3]*face->maptex->detailscale[0];
					pVertexes[j].detailtexcoord[1] = v[4]*face->maptex->detailscale[1];
				}
			}
			memcpy(&r_wsurf.pVertexBuffer[iCurVert], &pVertexes[0], sizeof(brushvertex_t)); iCurVert++;
			memcpy(&r_wsurf.pVertexBuffer[iCurVert], &pVertexes[1], sizeof(brushvertex_t)); iCurVert++;
			memcpy(&r_wsurf.pVertexBuffer[iCurVert], &pVertexes[2], sizeof(brushvertex_t)); iCurVert++;

			for(j = 0; j < (poly->numverts-3); j++, v += VERTEXSIZE)
			{
				memcpy(&pVertexes[1], &pVertexes[2], sizeof(brushvertex_t));

				pVertexes[2].pos[0] = v[0];
				pVertexes[2].pos[1] = v[1];
				pVertexes[2].pos[2] = v[2];
				pVertexes[2].texcoord[0] = v[3];
				pVertexes[2].texcoord[1] = v[4];
				pVertexes[2].lightmaptexcoord[0] = v[5];
				pVertexes[2].lightmaptexcoord[1] = v[6];
				pVertexes[2].normal[0] = face->normal[0];
				pVertexes[2].normal[1] = face->normal[1];
				pVertexes[2].normal[2] = face->normal[2];
				if(face->maptex && face->maptex->detailtex)
				{
					pVertexes[2].detailtexcoord[0] = v[3]*face->maptex->detailscale[0];
					pVertexes[2].detailtexcoord[1] = v[4]*face->maptex->detailscale[1];
				}
				memcpy(&r_wsurf.pVertexBuffer[iCurVert], &pVertexes[0], sizeof(brushvertex_t)); iCurVert++;
				memcpy(&r_wsurf.pVertexBuffer[iCurVert], &pVertexes[1], sizeof(brushvertex_t)); iCurVert++;
				memcpy(&r_wsurf.pVertexBuffer[iCurVert], &pVertexes[2], sizeof(brushvertex_t)); iCurVert++;
			}
		}

		face->num_vertexes = iCurVert - face->start_vertex;
		iCurFace++;
	}

	qglGenBuffersARB( 1, &r_wsurf.hVBO );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO );
	qglBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(brushvertex_t)*iNumVerts, r_wsurf.pVertexBuffer, GL_STATIC_DRAW_ARB );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
}

char *strtolower(char *str)
{
	char *temp;

	for ( temp = str; *temp; temp++ ) 
		*temp = tolower( *temp );

	return str;
}

void R_LoadSkyTextures(cJSON *tex)
{
	cJSON *replace = cJSON_GetObjectItem(tex, "replace");
	if(replace)
	{
		//Load Sky Replace Textures
		char *skytexname[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
		for(int i = 0;i < 6; ++i)
		{
			cJSON *obj = cJSON_GetObjectItem(replace, skytexname[i]);
			if(obj && obj->valuestring)
			{
				int texid = R_LoadTextureEx(obj->valuestring, obj->valuestring, NULL, NULL, GLT_WORLD, false, true);
				if(texid)
				{
					r_wsurf.iSkyTextures[i] = texid;
				}
			}
		}
	}
}

void R_LoadExtraTextureFile(qboolean loadmap)
{
	int i;
	char szFileName[256];
	char *pFile;

	if(!loadmap)
	{
		sprintf(szFileName, "resource/extra_textures.txt");
	}
	else
	{
		strcpy( szFileName, gEngfuncs.pfnGetLevelName() );
		if ( !strlen(szFileName) )
		{
			gEngfuncs.Con_Printf("R_LoadExtraTextureFile couldn't GetLevelName.\n");
			return;
		}
		szFileName[strlen(szFileName)-4] = 0;
		strcat(szFileName, "_extra.txt");
	}

	pFile = (char *)gEngfuncs.COM_LoadFile(szFileName, 5, NULL);
	if (!pFile)
	{
		gEngfuncs.Con_Printf("R_LoadExtraTextureFile couldn't open %s.\n", szFileName);
		return;
	}

	cJSON *pRoot = cJSON_Parse(pFile);
	if (!pRoot)
	{
		gEngfuncs.Con_Printf("R_LoadExtraTextureFile couldn't parse %s.\n", szFileName);
		return;
	}

	extratexture_mgr_t *pExtraMgr;

	if(!loadmap)
		pExtraMgr = &r_wsurf.ExtraTextures;
	else
		pExtraMgr = &r_wsurf.LocalExtraTextures;

	//free
	pExtraMgr->iNumTextures = 0;
	if(pExtraMgr->pTextures)
	{
		delete [] pExtraMgr->pTextures;
		pExtraMgr->pTextures = NULL;
	}

	int numtextures = cJSON_GetArraySize(pRoot);
	if(numtextures)
	{
		pExtraMgr->iNumTextures = 0;
		pExtraMgr->pTextures = new extratexture_t[numtextures];
		memset(pExtraMgr->pTextures, 0, sizeof(extratexture_t) * numtextures);
		for(i = 0; i < numtextures; ++i)
		{
			cJSON *tex = cJSON_GetArrayItem(pRoot, i);
			if(tex)
			{
				cJSON *base = cJSON_GetObjectItem(tex, "base");
				if(!base || !base->valuestring)
					continue;

				strtolower(base->valuestring);

				if(loadmap && !strcmp(base->valuestring, "sky"))
				{
					R_LoadSkyTextures(tex);
					continue;
				}
				int num = pExtraMgr->iNumTextures;
				pExtraMgr->iNumTextures ++;
				strncpy(pExtraMgr->pTextures[num].basetex, base->valuestring, 31);
				pExtraMgr->pTextures[num].basetex[31] = 0;
				pExtraMgr->pTextures[num].detailtex[0] = 0;
				pExtraMgr->pTextures[num].replacetex[0] = 0;
				pExtraMgr->pTextures[num].normaltex[0] = 0;
				pExtraMgr->pTextures[num].detailscale[0] = 1;
				pExtraMgr->pTextures[num].detailscale[1] = 1;
				pExtraMgr->pTextures[num].replacescale[0] = 1;
				pExtraMgr->pTextures[num].replacescale[1] = 1;
				pExtraMgr->pTextures[num].loaded = false;
				cJSON *detail = cJSON_GetObjectItem(tex, "detail");
				if(detail && detail->valuestring)
				{
					strncpy(pExtraMgr->pTextures[num].detailtex, detail->valuestring, 63);
					pExtraMgr->pTextures[num].detailtex[63] = 0;
				}
				cJSON *detailscale = cJSON_GetObjectItem(tex, "detailscale");
				if(detailscale && detailscale->valuestring)
				{
					sscanf(detailscale->valuestring, "%f %f", &(pExtraMgr->pTextures[num].detailscale[0]), &(pExtraMgr->pTextures[num].detailscale[1]));
				}

				cJSON *replace = cJSON_GetObjectItem(tex, "replace");
				if(replace && replace->valuestring)
				{
					strncpy(pExtraMgr->pTextures[num].replacetex, replace->valuestring, 63);
					pExtraMgr->pTextures[num].replacetex[63] = 0;
				}
				cJSON *replacescale = cJSON_GetObjectItem(tex, "replacescale");
				if(replacescale && replacescale->valuestring)
				{
					sscanf(replacescale->valuestring, "%f %f", &(pExtraMgr->pTextures[num].replacescale[0]), &(pExtraMgr->pTextures[num].replacescale[1]));
				}
				cJSON *normal = cJSON_GetObjectItem(tex, "normal");
				if(normal && normal->valuestring)
				{
					strncpy(pExtraMgr->pTextures[num].normaltex, normal->valuestring, 63);
					pExtraMgr->pTextures[num].normaltex[63] = 0;
				}
			}
		}
	}

	cJSON_Delete(pRoot);
	gEngfuncs.COM_FreeFile( pFile );
}

void R_LoadExtraTextures(qboolean loadmap)
{
	int i, j;
	texture_t *t;
	extratexture_t *pExTex;
	extratexture_mgr_t *pExTexMgr;
	maptexture_t *pMapTex;

	if(!r_worldmodel->numtextures)
		return;

	if(loadmap)
		pExTexMgr = &r_wsurf.LocalExtraTextures;
	else
		pExTexMgr = &r_wsurf.ExtraTextures;

	//no texture need to load
	if(!pExTexMgr->iNumTextures)
		return;

	for (i = 0; i < r_worldmodel->numtextures; i++)
	{
		t = r_worldmodel->textures[i];
		if (!t)
			continue;

		pMapTex = &r_wsurf.MapTextures[i];

		if(pMapTex->loaded)
			continue;

		for(j = 0; j < pExTexMgr->iNumTextures; ++j)
		{
			pExTex = &pExTexMgr->pTextures[j];
			//if(pExTex->loaded)
			//	continue;
			if(stricmp(t->name, pExTex->basetex))
				continue;

			//pExTex->loaded = true;

			pMapTex->basetex = t;
			pMapTex->detailtex = 0;
			pMapTex->replacetex = 0;
			pMapTex->normaltex = 0;
			pMapTex->detailscale[0] = 1;
			pMapTex->detailscale[1] = 1;
			pMapTex->replacescale[0] = 1;
			pMapTex->replacescale[1] = 1;
			pMapTex->loaded = true;

			if(pExTex->detailtex[0])
			{
				pMapTex->detailtex = R_LoadTextureEx(pExTex->detailtex, pExTex->detailtex, NULL, NULL, GLT_WORLD, true, true);
				pMapTex->detailscale[0] = (fabs(pExTex->detailscale[0]) < COLINEAR_EPSILON) ? 1 : 1 / pExTex->detailscale[0];
				pMapTex->detailscale[1] = (fabs(pExTex->detailscale[1]) < COLINEAR_EPSILON) ? 1 : 1 / pExTex->detailscale[1];
			}
			if(pExTex->replacetex[0])
			{
				pMapTex->replacetex = R_LoadTextureEx(pExTex->replacetex, pExTex->replacetex, NULL, NULL, GLT_WORLD, true, true);
				pMapTex->replacescale[0] = (fabs(pExTex->replacescale[0]) < COLINEAR_EPSILON) ? 1 : 1 / pExTex->replacescale[0];
				pMapTex->replacescale[1] = (fabs(pExTex->replacescale[1]) < COLINEAR_EPSILON) ? 1 : 1 / pExTex->replacescale[1];
			}
			if(pExTex->normaltex[0])
			{
				pMapTex->normaltex = R_LoadTextureEx(pExTex->normaltex, pExTex->normaltex, NULL, NULL, GLT_WORLD, true, true);
			}				
		}
	}
}

void R_LinkDecalTexture(texture_t *t)
{
	int i, n;
	texture_t *t2;
	gltexture_t *glt;
	extratexture_mgr_t *pExtraMgr;
	extratexture_t *pExtraTex;

	for(n = 0; n < 2; ++n)
	{
		if(n == 0)
			pExtraMgr = &r_wsurf.LocalExtraTextures;
		else
			pExtraMgr = &r_wsurf.ExtraTextures;
		for(i = 0; i < pExtraMgr->iNumTextures; ++i)
		{
			pExtraTex = &pExtraMgr->pTextures[i];
			if(pExtraTex->loaded)
				continue;
			if(!stricmp(t->name, pExtraTex->basetex) && pExtraTex->replacetex[0])
			{
				t2 = &r_wsurf.DecalTextures[r_wsurf.iNumDecalTextures];
				r_wsurf.iNumDecalTextures ++;
				
				t2->gl_texturenum = R_LoadTextureEx(pExtraTex->replacetex, pExtraTex->replacetex, NULL, NULL, GLT_DECAL, false, true);
				if(t2->gl_texturenum)
				{
					glt = R_GetCurrentGLTexture();
					if(pExtraTex->replacescale[0] != 1)
						t2->width = glt->width * pExtraTex->replacescale[0];
					else
						t2->width = glt->width;
					if(pExtraTex->replacescale[1] != 1)
						t2->height = glt->height * pExtraTex->replacescale[1];
					else
						t2->height = glt->height;

					strcpy(t2->name, t->name);
					t->anim_next = t2;
					t2->anim_next = t;
					t2->alternate_anims = (texture_t *)glt;
					pExtraTex->loaded = true;
				}
				return;
			}
		}
	}
}

void R_InitWSurf(void)
{
	r_wsurf.hVBO = 0;
	r_wsurf.pVertexBuffer = NULL;
	r_wsurf.pFaceBuffer = NULL;
	r_wsurf.iNumBSPEntities = 0;

	R_ClearExtraTextures();
	R_ClearMapTextures();
	R_ClearSkyTextures();
	R_ClearDecalTextures();
	R_ClearBSPEntities();

	r_wsurf_replace = gEngfuncs.pfnRegisterVariable("r_wsurf_replace", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_sky = gEngfuncs.pfnRegisterVariable("r_wsurf_sky", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_decal = gEngfuncs.pfnRegisterVariable("r_wsurf_decal", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}

void R_VidInitWSurf(void)
{
	water_normalmap = water_normalmap_default;

	//we don't need to free extra or decal textures cuz they are freed by engine when level changes.

	R_ClearMapTextures();
	R_ClearSkyTextures();
	R_ClearBSPEntities();

	//Load local extra textures into array
	R_LoadStudioTextures(true);
	R_LoadExtraTextureFile(true);

	//Rebuild MapTextures from both local and global array
	R_LoadExtraTextures(true);
	R_LoadExtraTextures(false);

	R_FreeVertexBuffer();
	R_GenerateVertexBuffer();

	//parse entities data from bsp's entity lump
	//R_ParseBSPEntities();
	//R_LoadBSPEntities();
}

void R_DrawPolyFromArray(glpoly_t *p)
{
	qglDrawArrays(GL_TRIANGLES, r_wsurf.pFaceBuffer[p->flags].start_vertex, r_wsurf.pFaceBuffer[p->flags].num_vertexes );
}

float ScrollOffset(msurface_t *psurface, cl_entity_t *pEntity)
{
	float speed, sOffset;

	speed = (pEntity->curstate.rendercolor.b + (pEntity->curstate.rendercolor.g << 8)) / 16.0;

	if (pEntity->curstate.rendercolor.r == 0)
		speed = -speed;

	sOffset = (1.0 / psurface->texinfo->texture->width) * speed * (*cl_time);

	if (sOffset < 0)
		sOffset = fmod(sOffset, -1.0f);
	else
		sOffset = fmod(sOffset, 1.0f);

	return sOffset;
}

inline void R_BeginVertexArrayNoTexture(void)
{
	qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
}

void R_BeginVertexArrayTexture(qboolean detail)
{
	qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));

	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));

	if(detail)
	{
		qglClientActiveTextureARB(GL_TEXTURE2_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
	}
}

inline void R_DrawVertexArray(brushface_t *pFace)
{
	qglDrawArrays(GL_POLYGON, pFace->start_vertex, pFace->num_vertexes);
}

void R_DrawPolyWireFrame(brushface_t *pFace, void(*pfnDrawFunc)(brushface_t *face))
{
	if (gl_wireframe->value)
	{
		GL_DisableMultitexture();
		qglDisable(GL_TEXTURE_2D);
		qglColor3f(1, 1, 1);
		qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		qglLineWidth(1);
		if (gl_wireframe->value == 2)
			qglDisable(GL_DEPTH_TEST);

		pfnDrawFunc(pFace);

		qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		qglEnable(GL_TEXTURE_2D);
		if (gl_wireframe->value == 2)
			qglEnable(GL_DEPTH_TEST);					
	}
}

inline void R_EndVertexArrayTexture(qboolean detail)
{
	if(detail)
	{
		qglClientActiveTextureARB(GL_TEXTURE2_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

inline void R_EndDetailTexture(void)
{
	GL_SelectTexture(TEXTURE2_SGIS);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
	qglDisable(GL_TEXTURE_2D);
	GL_SelectTexture(TEXTURE1_SGIS);
}

void R_DrawScrollingPoly(brushface_t *pFace, float sOffset, qboolean detail)
{
	int i;

	brushvertex_t *pVert = &r_wsurf.pVertexBuffer[pFace->start_vertex];

	qglBegin( GL_POLYGON );
	for(i = 0; i < pFace->num_vertexes; i++, pVert++)
	{
		qglMultiTexCoord2fARB(TEXTURE0_SGIS, pVert->texcoord[0] + sOffset, pVert->texcoord[1]);
		qglMultiTexCoord2fARB(TEXTURE1_SGIS, pVert->lightmaptexcoord[0], pVert->lightmaptexcoord[1]);
		if(detail)
			qglMultiTexCoord2fARB(TEXTURE2_SGIS, pVert->detailtexcoord[0] + sOffset, pVert->detailtexcoord[1]);
		qglVertex3fv(pVert->pos);			
	}
	qglEnd();
}

void R_DrawGLPoly(brushface_t *pFace)
{
	int i;
	brushvertex_t *pVert = &r_wsurf.pVertexBuffer[pFace->start_vertex];
	qglBegin( GL_POLYGON );
	for(i = 0; i < pFace->num_vertexes; i++, pVert++)
	{
		qglVertex3fv(pVert->pos);			
	}
	qglEnd();
}

void R_DrawSequentialPoly(msurface_t *s, int face)
{
	if ((*currententity)->curstate.rendermode == kRenderNormal)
	{
		if (!(s->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER)) && gl_mtexable)
		{
			if (!drawreflect && !drawrefract && !drawshadowmap)
			{
			}

			return;
		}
	}

	return gRefFuncs.R_DrawSequentialPoly(s, face);

	glpoly_t *p;
	int lightmapnum;
	texture_t *t;
	glRect_t *theRect;
	qboolean detail;
	qboolean replace;
	qboolean replacescaled;
	brushface_t *bface;

	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		p = s->polys;
		bface = &r_wsurf.pFaceBuffer[p->flags];

		GL_DisableMultitexture();
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable(GL_BLEND);
		qglDisable(GL_TEXTURE_2D);

		qglEnableClientState(GL_VERTEX_ARRAY);
		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO );
		R_BeginVertexArrayNoTexture();

		R_DrawVertexArray(bface);

		R_DrawPolyWireFrame(bface, R_DrawVertexArray);

		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
		qglDisableClientState(GL_VERTEX_ARRAY);

		qglEnable(GL_TEXTURE_2D);
		GL_EnableMultitexture();
		return;
	}

	if (s->flags & SURF_DRAWTURB)
	{
		GL_DisableMultitexture();
		GL_Bind(s->texinfo->texture->gl_texturenum);
		EmitWaterPolys(s, face);
		return;
	}

	if (!(s->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER)))
	{
		R_RenderDynamicLightmaps(s);

		if (gl_mtexable)
		{
			p = s->polys;
			bface = &r_wsurf.pFaceBuffer[p->flags];
			t = gRefFuncs.R_TextureAnimation(s);

			detail = false;
			replace = false;
			replacescaled = false;

			GL_SelectTexture(TEXTURE0_SGIS);
			if(bface->maptex && bface->maptex->replacetex && r_wsurf_replace->value > 0)
			{
				GL_Bind(bface->maptex->replacetex);

				if(bface->maptex->replacescale[0] != 1 || bface->maptex->replacescale[1] != 1)
				{
					replacescaled = true;
					qglMatrixMode(GL_TEXTURE);
					qglLoadIdentity();
					qglScalef(bface->maptex->replacescale[0], bface->maptex->replacescale[1], 1.0f);
					qglMatrixMode(GL_MODELVIEW);
				}
				replace = true;
			}
			else
			{
				GL_Bind(t->gl_texturenum);
			}
			qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			GL_EnableMultitexture();
			lightmapnum = s->lightmaptexturenum;
			GL_Bind( lightmap_textures[lightmapnum] );
			qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			if(!replace && bface->maptex && bface->maptex->detailtex && r_detailtextures->value > 0)
			{
				GL_SelectTexture(TEXTURE2_SGIS);
				qglEnable(GL_TEXTURE_2D);
				GL_Bind(bface->maptex->detailtex);
				qglMatrixMode(GL_TEXTURE);
				qglLoadIdentity();
				qglScalef(bface->maptex->detailscale[0], bface->maptex->detailscale[1], 1.0f);
				qglMatrixMode(GL_MODELVIEW);
				detail = true;
			}

			if (lightmap_modified[lightmapnum])
			{
				lightmap_modified[lightmapnum] = 0;
				theRect = &lightmap_rectchange[lightmapnum];
				qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps_new + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
				theRect->l = BLOCK_WIDTH;
				theRect->t = BLOCK_HEIGHT;
				theRect->h = 0;
				theRect->w = 0;
			}

			if (s->flags & SURF_DRAWTILED)
			{
				float sOffset = ScrollOffset(s, *currententity);

				R_DrawScrollingPoly(bface, sOffset, detail);

				if(detail)
					R_EndDetailTexture();

				R_DrawPolyWireFrame(bface, R_DrawGLPoly);
			}
			else
			{
				qglEnableClientState(GL_VERTEX_ARRAY);
				qglBindBufferARB( GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO );
				R_BeginVertexArrayTexture(detail);

				R_DrawVertexArray(bface);

				if(detail)
					R_EndDetailTexture();
				R_EndVertexArrayTexture(detail);

				R_DrawPolyWireFrame(bface, R_DrawVertexArray);

				qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
				qglDisableClientState(GL_VERTEX_ARRAY);
			}

			if (!gl_texsort->value && s->pdecals)
			{
				gDecalSurfs[(*gDecalSurfCount)] = s;
				(*gDecalSurfCount)++;

				if ((*gDecalSurfCount) > MAX_DECALSURFS)
					Sys_ErrorEx("Too many decal surfaces!\n");

				if ((*currententity)->curstate.rendermode != kRenderTransColor)
					gRefFuncs.R_DrawDecals(true);
			}

			if(replacescaled)
			{
				GL_SelectTexture(TEXTURE0_SGIS);
				qglMatrixMode(GL_TEXTURE);
				qglLoadIdentity();
				qglMatrixMode(GL_MODELVIEW);
				GL_SelectTexture(TEXTURE1_SGIS);
			}
			return;
		}
		return;
	}
}

char *ValueForKey(bspentity_t *ent, char *key)
{
   for (epair_t  *pEPair = ent->epairs; pEPair; pEPair = pEPair->next)
   {
      if (!strcmp(pEPair->key, key) )
         return pEPair->value;
   }
   return NULL;
}

void R_ClearBSPEntities(void)
{
	for(int i = 0; i < r_wsurf.iNumBSPEntities; i++)
	{
		epair_t *pPair = r_wsurf.pBSPEntities[i].epairs;
		while(pPair)
		{
			epair_t *pFree = pPair;
			pPair = pFree->next;

			delete [] pFree->key;
			delete [] pFree->value;
			delete pFree;
		}
		r_wsurf.pBSPEntities[i].epairs = NULL;
	}
	
	r_wsurf.iNumBSPEntities = 0;
}

//From trinity renderer
void R_ParseBSPEntities(void)
{
	return;

	char *pEntData = r_worldmodel->entities;

	if(!pEntData)
		return;

	int iEntDataSize = strlen(pEntData);

	char *pCurText = pEntData;
	while(pCurText && pCurText - pEntData < iEntDataSize)
	{
		if(r_wsurf.iNumBSPEntities == 4096)
			break;

		while(1)
		{
			if(pCurText[0] == '{')
				break;
			
			if(pCurText - pEntData >= iEntDataSize)
				break;

			pCurText++;
		}

		if(pCurText - pEntData >= iEntDataSize)
			break;

		bspentity_t *pEntity = &r_wsurf.pBSPEntities[r_wsurf.iNumBSPEntities];
		r_wsurf.iNumBSPEntities++;

		while(1)
		{
			// skip to next token
			while(1)
			{
				if(pCurText[0] == '}')
					break;

				if(pCurText[0] == '"')
				{
					pCurText++;
					break;
				}

				pCurText++;
			}

			// end of ent
			if(pCurText[0] == '}')
				break;

			epair_t *pEPair = new epair_t;
			memset(pEPair, 0, sizeof(epair_t));

			if(pEntity->epairs)
				pEPair->next = pEntity->epairs;
				
			pEntity->epairs = pEPair;

			int iLength = 0;
			char *pTemp = pCurText;
			while(1)
			{
				if(pTemp[0] == '"')
					break;
				
				if(pCurText[0] == '}')
				{
					Sys_ErrorEx("R_ParseBSPEntities: failed to parse entity data, bad \"}\" excceeded.\n");
					return;
				}

				iLength++;
				pTemp++;
			}

			pEPair->key = new char[iLength+1];
			pEPair->key[iLength] = NULL; // terminator

			memcpy(pEPair->key, pCurText, sizeof(char)*iLength);
			pCurText += iLength+1;

			// skip to next token
			while(1)
			{
				if(pCurText[0] == '}')
				{
					Sys_ErrorEx("R_ParseBSPEntities: failed to parse entity data, bad \"}\" excceeded.\n");
					return;
				}

				if(pCurText[0] == '"')
				{
					pCurText++;
					break;
				}

				pCurText++;
			}

			iLength = 0;
			pTemp = pCurText;
			while(1)
			{
				if(pCurText[0] == '}')
				{
					Sys_ErrorEx("R_ParseBSPEntities: failed to parse entity data, bad \"}\" excceeded.\n");
					return;
				}

				if(pTemp[0] == '"')
					break;
				
				iLength++;
				pTemp++;
			}

			pEPair->value = new char[iLength+1];
			strncpy(pEPair->value, pCurText, sizeof(char)*iLength);
			pEPair->value[iLength] = NULL;
			pCurText += iLength+1;
		}
	}
}

void R_LoadBSPEntities(void)
{
	for(int i = 0; i < r_wsurf.iNumBSPEntities; i++)
	{
		bspentity_t *ent = &r_wsurf.pBSPEntities[i];
		char *classname = ValueForKey(ent, "classname");

		if(!classname)
			continue;

		if(!strcmp(classname, "sky_box"))
		{
			char *model = ValueForKey(ent, "model");
			if (model && model[0] == '*')
			{
				model_t *mod = IEngineStudio.Mod_ForName(model, false);
				if(mod)
				{
					VectorCopy(mod->mins, r_3dsky_parm.mins);
					VectorCopy(mod->maxs, r_3dsky_parm.maxs);
				}
			}
		}
		if(!strcmp(classname, "sky_center"))
		{
			char *origin = ValueForKey(ent, "origin");
			if (origin)
			{
				sscanf(origin, "%f %f %f", &r_3dsky_parm.center[0], &r_3dsky_parm.center[1], &r_3dsky_parm.center[2]);
			}			
		}
		if(!strcmp(classname, "sky_camera"))
		{
			char *origin = ValueForKey(ent, "origin");
			if (origin)
			{
				sscanf(origin, "%f %f %f", &r_3dsky_parm.camera[0], &r_3dsky_parm.camera[1], &r_3dsky_parm.camera[2]);
			}
			r_3dsky_parm.enable = true;
		}
	}//end for
}