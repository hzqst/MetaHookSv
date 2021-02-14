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
	//we don't need to free extra or decal textures cuz they are freed by engine when level changes.

	R_ClearMapTextures();
	R_ClearSkyTextures();
	R_ClearBSPEntities();

	//Load local extra textures into array
	//R_LoadStudioTextures(true);
	//R_LoadExtraTextureFile(true);

	//Rebuild MapTextures from both local and global array
	//R_LoadExtraTextures(true);
	//R_LoadExtraTextures(false);

	R_FreeVertexBuffer();
	R_GenerateVertexBuffer();

	//parse entities data from bsp's entity lump
	R_ParseBSPEntities(r_worldmodel->entities);
	R_LoadBSPEntities();
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

inline void R_BeginVertexArrayTexture(qboolean detail)
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

inline bool R_BeginDetailTexture(brushface_t *pFace)
{
	if (pFace->maptex && pFace->maptex->detailtex && r_detailtextures->value > 0)
	{
		qglActiveTextureARB(TEXTURE2_SGIS);
		qglEnable(GL_TEXTURE_2D);
		qglBindTexture(GL_TEXTURE_2D, pFace->maptex->detailtex);
		qglMatrixMode(GL_TEXTURE);
		qglLoadIdentity();
		qglScalef(pFace->maptex->detailscale[0], pFace->maptex->detailscale[1], 1.0f);
		qglMatrixMode(GL_MODELVIEW);

		return true;
	}

	return false;
}

inline void R_EndDetailTexture(void)
{
	qglActiveTextureARB(TEXTURE2_SGIS);
	qglDisable(GL_TEXTURE_2D);
	qglActiveTextureARB(TEXTURE1_SGIS);
}

void R_DrawScrollingPoly(brushface_t *pFace, float sOffset, qboolean detail)
{
	brushvertex_t *pVert = &r_wsurf.pVertexBuffer[pFace->start_vertex];

	qglBegin( GL_POLYGON );
	for(int i = 0; i < pFace->num_vertexes; i++, pVert++)
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
	brushvertex_t *pVert = &r_wsurf.pVertexBuffer[pFace->start_vertex];
	qglBegin( GL_POLYGON );
	for(int i = 0; i < pFace->num_vertexes; i++, pVert++)
	{
		qglVertex3fv(pVert->pos);			
	}
	qglEnd();
}

void DrawGLPoly(msurface_t *psurface)
{
	int i;
	float *v;
	glpoly_t *p;

	p = psurface->polys;

	qglBegin(GL_POLYGON);

	v = p->verts[0];

	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		qglTexCoord2f(v[3], v[4]);
		qglVertex3fv(v);
	}

	qglEnd();
}

void DrawGLPolyScroll(msurface_t *psurface, cl_entity_t *pEntity)
{
	int i;
	float *v, sOffset;
	glpoly_t *p;

	sOffset = ScrollOffset(psurface, pEntity);
	p = psurface->polys;

	qglBegin(GL_POLYGON);

	v = p->verts[0];

	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		qglTexCoord2f(v[3] + sOffset, v[4]);
		qglVertex3fv(v);
	}

	qglEnd();
}

void R_DrawSequentialPoly(msurface_t *s, int face)
{
	if (drawdlightsecond)
	{
		if ((*currententity)->curstate.rendermode == kRenderTransAlpha)
		{
			qglEnable(GL_BLEND);
			qglBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
		}
	}

	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		if (!drawpolynocolor && !drawdlightsecond)
		{
			auto p = s->polys;
			auto bface = &r_wsurf.pFaceBuffer[p->flags];

			GL_DisableMultitexture();
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			qglEnable(GL_BLEND);
			qglDisable(GL_TEXTURE_2D);

			qglEnableClientState(GL_VERTEX_ARRAY);
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
			R_BeginVertexArrayNoTexture();
			R_DrawVertexArray(bface);
			R_DrawPolyWireFrame(bface, R_DrawVertexArray);
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			qglDisableClientState(GL_VERTEX_ARRAY);

			qglEnable(GL_TEXTURE_2D);
			GL_EnableMultitexture();
		}
		return;
	}

	if (s->flags & SURF_DRAWTURB)
	{
		if (!drawdlightsecond && !drawpolynocolor)
		{
			GL_DisableMultitexture();
			GL_Bind(s->texinfo->texture->gl_texturenum);
			EmitWaterPolys(s, face);
		}
		return;
	}

	if (!(s->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER)))
	{
		if (!drawdlightsecond && !drawpolynocolor)
		{
			R_RenderDynamicLightmaps(s);
		}

		if (gl_mtexable && ((*currententity)->curstate.rendermode == kRenderTransAlpha || (*currententity)->curstate.rendermode == kRenderNormal))
		{
			auto p = s->polys;
			auto bface = &r_wsurf.pFaceBuffer[p->flags];
			auto t = gRefFuncs.R_TextureAnimation(s);

			if (!drawpolynocolor)
			{
				GL_SelectTexture(TEXTURE0_SGIS);
				GL_Bind(t->gl_texturenum);
				if (!drawdlightsecond)
				{
					qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				}
				else
				{
					qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				}

				if ((*currententity)->curstate.rendermode == kRenderTransColor)
					qglDisable(GL_TEXTURE_2D);
			}

			auto lightmapnum = s->lightmaptexturenum;

			if (!drawdlightsecond && !drawpolynocolor)
			{
				GL_EnableMultitexture();
				GL_Bind(lightmap_textures[lightmapnum]);
				qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}

			if (lightmap_modified[lightmapnum])
			{
				lightmap_modified[lightmapnum] = 0;
				if (g_iEngineType == ENGINE_SVENGINE)
				{
					glRect_SvEngine_t *theRect = (glRect_SvEngine_t *)((char *)lightmap_rectchange + sizeof(glRect_SvEngine_t) * lightmapnum);
					qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
					theRect->l = BLOCK_WIDTH;
					theRect->t = BLOCK_HEIGHT;
					theRect->h = 0;
					theRect->w = 0;
				}
				else
				{
					glRect_t *theRect = (glRect_t *)((char *)lightmap_rectchange + sizeof(glRect_t) * lightmapnum);
					qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
					theRect->l = BLOCK_WIDTH;
					theRect->t = BLOCK_HEIGHT;
					theRect->h = 0;
					theRect->w = 0;
				}
			}

			bool detail = drawpolynocolor ? false : R_BeginDetailTexture(bface);

			if (s->flags & SURF_DRAWTILED)
			{
				auto sOffset = ScrollOffset(s, *currententity);

				R_DrawScrollingPoly(bface, sOffset, detail);

				if(detail) R_EndDetailTexture();

				R_DrawPolyWireFrame(bface, R_DrawGLPoly);
			}
			else
			{
				qglEnableClientState(GL_VERTEX_ARRAY);
				qglBindBufferARB( GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO );

				if (drawpolynocolor)
				{
					R_BeginVertexArrayNoTexture();
					R_DrawVertexArray(bface);
				}
				else
				{
					R_BeginVertexArrayTexture(detail);
					R_DrawVertexArray(bface);
					if (detail) R_EndDetailTexture();
					R_EndVertexArrayTexture(detail);
				}
				
				if (!drawpolynocolor && !drawdlightsecond)
				{
					R_DrawPolyWireFrame(bface, R_DrawVertexArray);
				}

				qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
				qglDisableClientState(GL_VERTEX_ARRAY);
			}

			if (!gl_texsort->value && s->pdecals && !drawpolynocolor)
			{
				gDecalSurfs[(*gDecalSurfCount)] = s;
				(*gDecalSurfCount)++;

				if ((*gDecalSurfCount) > MAX_DECALSURFS)
					Sys_ErrorEx("Too many decal surfaces!\n");

				if ((*currententity)->curstate.rendermode != kRenderTransColor)
					gRefFuncs.R_DrawDecals(true);
			}

			return;
		}
		else
		{
			auto p = s->polys;
			auto t = gRefFuncs.R_TextureAnimation(s);

			if (!drawpolynocolor)
			{
				GL_DisableMultitexture();
				GL_Bind(t->gl_texturenum);
			}

			if (s->flags & SURF_DRAWTILED)
			{
				DrawGLPolyScroll(s, (*currententity));
			}
			else
			{
				DrawGLPoly(s);
			}

			if (!gl_texsort->value && s->pdecals && !drawpolynocolor)
			{
				gDecalSurfs[(*gDecalSurfCount)] = s;
				(*gDecalSurfCount)++;

				if ((*gDecalSurfCount) > MAX_DECALSURFS)
					Sys_ErrorEx("Too many decal surfaces!\n");

				gRefFuncs.R_DrawDecals(false);
			}

			if (gl_wireframe->value && !drawpolynocolor && !drawdlightsecond)
			{
				qglDisable(GL_TEXTURE_2D);
				qglColor3f(1, 1, 1);

				if (gl_wireframe->value == 2)
					qglDisable(GL_DEPTH_TEST);

				qglBegin(GL_LINE_LOOP);
				auto v = p->verts[0];
				for (int i = 0; i < p->numverts; i++, v += VERTEXSIZE)
				{
					qglVertex3fv(v);
				}
				qglEnd();
				qglEnable(GL_TEXTURE_2D);

				if (gl_wireframe->value == 2)
					qglEnable(GL_DEPTH_TEST);
			}

			if ((*currententity)->curstate.rendermode == kRenderNormal)
			{
				if (!drawpolynocolor && !drawdlightsecond)
				{
					GL_Bind(lightmap_textures[s->lightmaptexturenum]);
					qglEnable(GL_BLEND);
					qglBegin(GL_POLYGON);
					auto v = p->verts[0];
					for (int i = 0; i < p->numverts; i++, v += VERTEXSIZE)
					{
						qglTexCoord2f(v[5], v[6]);
						qglVertex3fv(v);
					}
					qglEnd();
					qglDisable(GL_BLEND);
				}
			}
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
		r_wsurf.pBSPEntities[i].classname = NULL;
		VectorClear(r_wsurf.pBSPEntities[i].origin);
	}
	
	r_wsurf.iNumBSPEntities = 0;
}

bspentity_t *current_parse_entity = NULL;
char com_token[4096];

bool R_ParseBSPEntityKeyValue(const char *classname, const char *keyname, const char *value)
{
	if (classname == NULL)
	{
		if (r_wsurf.iNumBSPEntities >= MAX_MAP_BSPENTITY)
			return false;

		current_parse_entity = &r_wsurf.pBSPEntities[r_wsurf.iNumBSPEntities];
		r_wsurf.iNumBSPEntities++;

		current_parse_entity->classname = NULL;
		current_parse_entity->epairs = NULL;
		VectorClear(current_parse_entity->origin);
	}

	if (current_parse_entity)
	{
		auto epairs = new epair_t;
		auto keynamelen = strlen(keyname);
		epairs->key = new char[keynamelen + 1];
		strncpy(epairs->key, keyname, keynamelen);
		epairs->key[keynamelen] = 0;

		auto valuelen = strlen(value);
		epairs->value = new char[valuelen + 1];
		strncpy(epairs->value, value, valuelen);
		epairs->value[valuelen] = 0;

		if (!strcmp(keyname, "origin"))
		{
			sscanf(value, "%f %f %f", &current_parse_entity->origin[0], &current_parse_entity->origin[1], &current_parse_entity->origin[2]);
		}

		if (!strcmp(keyname, "classname"))
		{
			current_parse_entity->classname = epairs->value;
		}

		epairs->next = current_parse_entity->epairs;
		current_parse_entity->epairs = epairs;

		return true;
	}

	return false;
}

bool R_ParseBSPEntityClassname(char *szInputStream, char *classname)
{
	char szKeyName[256];

	// key
	szInputStream = gEngfuncs.COM_ParseFile(szInputStream, com_token);
	while (szInputStream && com_token[0] != '}')
	{
		strncpy(szKeyName, com_token, sizeof(szKeyName) - 1);
		szKeyName[sizeof(szKeyName) - 1] = 0;

		szInputStream = gEngfuncs.COM_ParseFile(szInputStream, com_token);

		if (!strcmp(szKeyName, "classname"))
		{
			R_ParseBSPEntityKeyValue(NULL, szKeyName, com_token);

			strcpy(classname, com_token);

			return true;
		}

		if (!szInputStream)
		{
			break;
		}

		szInputStream = gEngfuncs.COM_ParseFile(szInputStream, com_token);
	}

	return false;
}

char *R_ParseBSPEntity(char *data)
{
	char keyname[256] = { 0 };
	char classname[256] = { 0 };

	if (R_ParseBSPEntityClassname(data, classname))
	{
		while (1)
		{
			data = gEngfuncs.COM_ParseFile(data, com_token);
			if (com_token[0] == '}')
			{
				break;
			}
			if (!data)
			{
				Sys_ErrorEx("R_ParseBSPEntity: EOF without closing brace");
			}

			strncpy(keyname, com_token, sizeof(keyname) - 1);
			keyname[sizeof(keyname) - 1] = 0;
			// Remove tail spaces
			for (int n = strlen(keyname) - 1; n >= 0 && keyname[n] == ' '; n--)
			{
				keyname[n] = 0;
			}

			data = gEngfuncs.COM_ParseFile(data, com_token);
			if (!data)
			{
				Sys_ErrorEx("R_ParseBSPEntity: EOF without closing brace");

			}
			if (com_token[0] == '}')
			{
				Sys_ErrorEx("R_ParseBSPEntity: closing brace without data");
			}

			if (!strcmp(classname, com_token))
			{
				continue;
			}

			R_ParseBSPEntityKeyValue(classname, keyname, com_token);
		}
	}

	current_parse_entity = NULL;

	return data;
}

void R_ParseBSPEntities(char *data)
{
	while (1)
	{
		data = gEngfuncs.COM_ParseFile(data, com_token);
		if (!data)
		{
			break;
		}
		if (com_token[0] != '{')
		{
			Sys_ErrorEx("R_ParseBSPEntities: found %s when expecting {", com_token);
			return;
		}
		data = R_ParseBSPEntity(data);
	}
}

void R_LoadBSPEntities(void)
{
	for(int i = 0; i < r_wsurf.iNumBSPEntities; i++)
	{
		bspentity_t *ent = &r_wsurf.pBSPEntities[i];

		char *classname = ent->classname;

		if(!classname)
			continue;

		if (!strcmp(classname, "light_environment"))
		{
			char *light = ValueForKey(ent, "_light");
			if (light)
			{
				sscanf(light, "%d %d %d %d", &r_light_env_color[0], &r_light_env_color[1], &r_light_env_color[2], &r_light_env_color[3]);

				r_light_env_enabled = true;
			}
			
			char *angle = ValueForKey(ent, "angles");
			if (angle)
			{
				sscanf(angle, "%f %f %f", &r_light_env_angles[0], &r_light_env_angles[1], &r_light_env_angles[2]);
				r_light_env_angles[0] += 180;
				r_light_env_angles[1] += 180;
			}
		}

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
			VectorCopy(ent->origin, r_3dsky_parm.center);
		}
		if(!strcmp(classname, "sky_camera"))
		{
			VectorCopy(ent->origin, r_3dsky_parm.camera);
			r_3dsky_parm.enable = true;
		}
	}//end for
}