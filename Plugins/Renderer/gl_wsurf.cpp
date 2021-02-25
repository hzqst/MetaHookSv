#include "gl_local.h"
#include "cJSON.h"

r_worldsurf_t r_wsurf;

cvar_t *r_wsurf_replace;
cvar_t *r_wsurf_sky;
cvar_t *r_wsurf_decal;
cvar_t *r_wsurf_vbo;

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

void R_SetVBOState(int state)
{
	if(!r_wsurf_vbo->value)
		return;

	if (r_wsurf.iVBOState == state)
		return;

	switch (state)
	{
	case VBOSTATE_OFF:
	{
		if (r_wsurf.iVBOState == VBOSTATE_DETAIL_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_LIGHTMAP_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_DIFFUSE_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		else if (r_wsurf.iVBOState == VBOSTATE_NO_TEXTURE)
		{
			//do nothing
		}

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		qglDisableClientState(GL_NORMAL_ARRAY);
		qglDisableClientState(GL_VERTEX_ARRAY);

		break;
	}
	case VBOSTATE_NO_TEXTURE:
	{
		if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglEnableClientState(GL_NORMAL_ARRAY);
			qglEnableClientState(GL_VERTEX_ARRAY);
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
			qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
			qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
		}

		if (r_wsurf.iVBOState == VBOSTATE_DETAIL_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_LIGHTMAP_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_DIFFUSE_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_NO_TEXTURE)
		{
			//do nothing
		}

		else if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			//do nothing
		}

		break;
	}
	case VBOSTATE_DIFFUSE_TEXTURE:
	{
		if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglEnableClientState(GL_NORMAL_ARRAY);
			qglEnableClientState(GL_VERTEX_ARRAY);
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
			qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
			qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
		}

		if (r_wsurf.iVBOState == VBOSTATE_DETAIL_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_LIGHTMAP_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_DIFFUSE_TEXTURE)
		{
			//do nothing
		}

		else if (r_wsurf.iVBOState == VBOSTATE_NO_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		}

		break;
	}
	case VBOSTATE_LIGHTMAP_TEXTURE:
	{
		if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglEnableClientState(GL_NORMAL_ARRAY);
			qglEnableClientState(GL_VERTEX_ARRAY);
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
			qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
			qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
		}

		if (r_wsurf.iVBOState == VBOSTATE_DETAIL_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		else if (r_wsurf.iVBOState == VBOSTATE_LIGHTMAP_TEXTURE)
		{
			//do nothing
		}

		else if (r_wsurf.iVBOState == VBOSTATE_DIFFUSE_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_NO_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		}

		break;
	}
	case VBOSTATE_DETAIL_TEXTURE:
	{
		if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglEnableClientState(GL_NORMAL_ARRAY);
			qglEnableClientState(GL_VERTEX_ARRAY);
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
			qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
			qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
		}

		if (r_wsurf.iVBOState == VBOSTATE_DETAIL_TEXTURE)
		{
			//do nothing
		}

		else if (r_wsurf.iVBOState == VBOSTATE_LIGHTMAP_TEXTURE)
		{
			//do nothing
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_DIFFUSE_TEXTURE)
		{
			//do nothing
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_NO_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		}
		break;
	}
	}

	r_wsurf.iVBOState = state;
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

	//r_wsurf_replace = gEngfuncs.pfnRegisterVariable("r_wsurf_replace", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	//r_wsurf_sky = gEngfuncs.pfnRegisterVariable("r_wsurf_sky", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	//r_wsurf_decal = gEngfuncs.pfnRegisterVariable("r_wsurf_decal", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_vbo = gEngfuncs.pfnRegisterVariable("r_wsurf_vbo", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
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

	R_StudioClearVBOCache();
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

void R_DrawWireFrame(brushface_t *brushface, void(*draw)(brushface_t *face))
{
	if (gl_wireframe->value)
	{
		R_SetVBOState(VBOSTATE_NO_TEXTURE);

		R_SetGBufferRenderState(GBUFFER_STATE_TRANSPARENT_COLOR);
		R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);

		qglColor3f(1, 1, 1);
		qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		qglLineWidth(1);
		if (gl_wireframe->value == 2)
			qglDisable(GL_DEPTH_TEST);

		if ((*mtexenabled))
		{
			GL_DisableMultitexture();
			qglDisable(GL_TEXTURE_2D);

			draw(brushface);

			qglEnable(GL_TEXTURE_2D);
			GL_EnableMultitexture();
		}
		else
		{
			qglDisable(GL_TEXTURE_2D);

			draw(brushface);

			qglEnable(GL_TEXTURE_2D);
		}

		qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (gl_wireframe->value == 2)
			qglEnable(GL_DEPTH_TEST);

		R_SetVBOState(VBOSTATE_OFF);
	}
}

void R_BeginDetailTexture(msurface_t *fa)
{
	auto p = fa->polys;
	auto brushface = &r_wsurf.pFaceBuffer[p->flags];

	if (brushface->maptex && brushface->maptex->detailtex && r_detailtextures->value > 0)
	{
		qglActiveTextureARB(TEXTURE2_SGIS);
		qglEnable(GL_TEXTURE_2D);
		qglBindTexture(GL_TEXTURE_2D, brushface->maptex->detailtex);
		qglMatrixMode(GL_TEXTURE);
		qglLoadIdentity();
		qglScalef(brushface->maptex->detailscale[0], brushface->maptex->detailscale[1], 1.0f);
		qglMatrixMode(GL_MODELVIEW);

		r_wsurf.bDetailTexture = true;
	}
}

void R_EndDetailTexture(void)
{
	if (!r_wsurf.bDetailTexture)
		return;

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglDisable(GL_TEXTURE_2D);
	qglActiveTextureARB(TEXTURE1_SGIS);
}

void DrawGLVertexBufferObject(brushface_t *brushface)
{
	qglDrawArrays(GL_POLYGON, brushface->start_vertex, brushface->num_vertexes);
}

void DrawGLScrollingVertex(brushface_t *brushface, float sOffset)
{
	brushvertex_t *vert = &r_wsurf.pVertexBuffer[brushface->start_vertex];

	qglBegin( GL_POLYGON );
	for(int i = 0; i < brushface->num_vertexes; i++, vert++)
	{
		qglMultiTexCoord2fARB(TEXTURE0_SGIS, vert->texcoord[0] + sOffset, vert->texcoord[1]);

		if(r_wsurf.bLightmapTexture)
			qglMultiTexCoord2fARB(TEXTURE1_SGIS, vert->lightmaptexcoord[0], vert->lightmaptexcoord[1]);

		if(r_wsurf.bDetailTexture)
			qglMultiTexCoord2fARB(TEXTURE2_SGIS, vert->detailtexcoord[0] + sOffset, vert->detailtexcoord[1]);

		qglNormal3fv(vert->normal);
		qglVertex3fv(vert->pos);
	}
	qglEnd();
}

void DrawGLVertex(brushface_t *brushface)
{
	return DrawGLScrollingVertex(brushface, 0);
}

void DrawGLPoly(glpoly_t *p)
{
	auto brushface = &r_wsurf.pFaceBuffer[p->flags];

	if (r_wsurf_vbo->value)
	{
		if (r_wsurf.bDetailTexture)
		{
			R_SetVBOState(VBOSTATE_DETAIL_TEXTURE);
		}
		else if (r_wsurf.bLightmapTexture)
		{
			R_SetVBOState(VBOSTATE_LIGHTMAP_TEXTURE);
		}
		else
		{
			R_SetVBOState(VBOSTATE_DIFFUSE_TEXTURE);
		}

		DrawGLVertexBufferObject(brushface);

		R_DrawWireFrame(brushface, DrawGLVertexBufferObject);

		R_SetVBOState(VBOSTATE_OFF);
	}
	else
	{
		DrawGLVertex(brushface);

		R_DrawWireFrame(brushface, DrawGLVertex);
	}
}

void DrawGLPoly(msurface_t *fa)
{
	auto p = fa->polys;
	return DrawGLPoly(p);
}

void DrawGLPolyScroll(msurface_t *fa, cl_entity_t *pEntity)
{
	auto p = fa->polys;	
	auto brushface = &r_wsurf.pFaceBuffer[p->flags];

	auto sOffset = ScrollOffset(fa, pEntity);

	DrawGLScrollingVertex(brushface, sOffset);

	R_DrawWireFrame(brushface, DrawGLVertex);
}

void DrawGLPolySolid(msurface_t *fa)
{
	auto p = fa->polys;
	auto brushface = &r_wsurf.pFaceBuffer[p->flags];

	qglColor4f(
		(*currententity)->curstate.rendercolor.r / 255.0,
		(*currententity)->curstate.rendercolor.g / 255.0,
		(*currententity)->curstate.rendercolor.b / 255.0,
		(*r_blend));

	if (r_wsurf_vbo->value)
	{
		R_SetVBOState(VBOSTATE_NO_TEXTURE);

		DrawGLVertexBufferObject(brushface);

		R_DrawWireFrame(brushface, DrawGLVertexBufferObject);

		R_SetVBOState(VBOSTATE_OFF);
	}
	else
	{
		DrawGLVertex(brushface);

		R_DrawWireFrame(brushface, DrawGLVertex);
	}
}

// Generate lighting coordinates at each vertex for decal vertices v[] on surface psurf
void R_DecalVertsLight(float *v, msurface_t *psurf, int vertCount)
{
	int j;
	float s, t;

	for (j = 0; j < vertCount; j++, v += VERTEXSIZE)
	{
		s = DotProduct(v, psurf->texinfo->vecs[0]) + psurf->texinfo->vecs[0][3];
		s -= psurf->texturemins[0];
		s += psurf->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16;

		t = DotProduct(v, psurf->texinfo->vecs[1]) + psurf->texinfo->vecs[1][3];
		t -= psurf->texturemins[1];
		t += psurf->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16;

		v[5] = s;
		v[6] = t;
	}
}

// Quick and dirty sutherland Hodgman clipper
// Clip polygon to decal in texture space
// JAY: This code is lame, change it later.  It does way too much work per frame
// It can be made to recursively call the clipping code and only copy the vertex list once
int Inside(float *vert, int edge)
{
	switch (edge) {
	case 0:		// left
		if (vert[3] > 0.0)
			return 1;
		return 0;
	case 1:		// right
		if (vert[3] < 1.0)
			return 1;
		return 0;

	case 2:		// top
		if (vert[4] > 0.0)
			return 1;
		return 0;

	case 3:
		if (vert[4] < 1.0)
			return 1;
		return 0;
	}
	return 0;
}


void Intersect(float *one, float *two, int edge, float *out)
{
	float t;

	// t is the parameter of the line between one and two clipped to the edge
	// or the fraction of the clipped point between one & two
	// vert[3] is u
	// vert[4] is v
	// vert[0], vert[1], vert[2] is X, Y, Z
	if (edge < 2) {
		if (edge == 0) {	// left
			t = ((one[3] - 0) / (one[3] - two[3]));
			out[3] = 0;
		}
		else {				// right
			t = ((one[3] - 1) / (one[3] - two[3]));
			out[3] = 1;
		}
		out[4] = one[4] + (two[4] - one[4]) * t;
	}
	else {
		if (edge == 2) {	// top
			t = ((one[4] - 0) / (one[4] - two[4]));
			out[4] = 0;
		}
		else {				// bottom
			t = ((one[4] - 1) / (one[4] - two[4]));
			out[4] = 1;
		}
		out[3] = one[3] + (two[3] - one[3]) * t;
	}
	out[0] = one[0] + (two[0] - one[0]) * t;
	out[1] = one[1] + (two[1] - one[1]) * t;
	out[2] = one[2] + (two[2] - one[2]) * t;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *vert - 
//			vertCount - 
//			*out - 
//			outSize - 
//			edge - 
// Output : int
//-----------------------------------------------------------------------------
int SHClip(float *vert, int vertCount, float *out, int edge)
{
	int		j, outCount;
	float	*s, *p;

	outCount = 0;

	s = &vert[(vertCount - 1) * VERTEXSIZE];
	for (j = 0; j < vertCount; j++) {
		p = &vert[j * VERTEXSIZE];
		if (Inside(p, edge)) {
			if (Inside(s, edge)) {
				// Add a vertex and advance out to next vertex
				memcpy(out, p, sizeof(float)*VERTEXSIZE);
				outCount++;
				out += VERTEXSIZE;
			}
			else {
				Intersect(s, p, edge, out);
				out += VERTEXSIZE;
				outCount++;
				memcpy(out, p, sizeof(float)*VERTEXSIZE);
				outCount++;
				out += VERTEXSIZE;
			}
		}
		else {
			if (Inside(s, edge)) {
				Intersect(p, s, edge, out);
				out += VERTEXSIZE;
				outCount++;
			}
		}

		s = p;
	}

	return outCount;
}

#define MAX_DECALCLIPVERT		32
static float vert[MAX_DECALCLIPVERT][VERTEXSIZE];
static float outvert[MAX_DECALCLIPVERT][VERTEXSIZE];
//-----------------------------------------------------------------------------
// Generate clipped vertex list for decal pdecal projected onto polygon psurf
//-----------------------------------------------------------------------------
float *R_DecalVertsClip(
	float *poutVerts,
	decal_t *pdecal,
	msurface_t *psurf,
	texture_t *ptexture,
	int *pvertCount)
{
	float *v;
	float scalex, scaley;
	int j, outCount;

	scalex = (psurf->texinfo->texture->width * pdecal->scale) / (float)ptexture->width;
	scaley = (psurf->texinfo->texture->height * pdecal->scale) / (float)ptexture->width;

	if (poutVerts == NULL)
		poutVerts = (float *)&vert[0];

	v = psurf->polys->verts[0];

	for (j = 0; j < psurf->polys->numverts; j++, v += VERTEXSIZE)
	{
		VectorCopy(v, vert[j]);
		vert[j][3] = (v[3] - pdecal->dx) * scalex;
		vert[j][4] = (v[4] - pdecal->dy) * scaley;

		if (pdecal->flags & FDECAL_HFLIP)
			vert[j][3] = 1 - vert[j][3];

		if (pdecal->flags & FDECAL_VFLIP)
			vert[j][4] = 1 - vert[j][4];
	}

	// Clip the polygon to the decal texture space
	outCount = SHClip((float *)&vert[0], psurf->polys->numverts, (float *)&outvert[0], 0);
	outCount = SHClip((float *)&outvert[0], outCount, (float *)&vert[0], 1);
	outCount = SHClip((float *)&vert[0], outCount, (float *)&outvert[0], 2);
	outCount = SHClip((float *)&outvert[0], outCount, poutVerts, 3);

	if (outCount)
	{
		if (pdecal->flags & FDECAL_CLIPTEST)
		{
			pdecal->flags &= ~FDECAL_CLIPTEST;	// We're doing the test

			// If there are exactly 4 verts and they are all 0,1 tex coords, then we've got an unclipped decal
			// A more precise test would be to calculate the texture area and make sure it's one, but this
			// should work as well.
			if (outCount == 4)
			{
				int clipped = 0;
				float s, t;

				v = poutVerts;
				for (j = 0; j < outCount && !clipped; j++, v += VERTEXSIZE)
				{
					s = v[3];
					t = v[4];

					if ((s != 0.0 && s != 1.0) || (t != 0.0 && t != 1.0))
						clipped = 1;
				}

				// We didn't need to clip this decal, it's a quad covering the full texture space, optimize
				// subsequent frames.
				if (!clipped)
					pdecal->flags |= FDECAL_NOCLIP;
			}
		}
	}

	*pvertCount = outCount;
	return poutVerts;
}

static int R_DecalIndex(decal_t *pdecal)
{
	return (pdecal - gDecalPool);
}

static int R_DecalCacheIndex(int index)
{
	return index & (256 - 1);
}

static decalcache_t *R_DecalCacheSlot(int decalIndex)
{
	int				cacheIndex;

	cacheIndex = R_DecalCacheIndex(decalIndex);	// Find the cache slot

	return gDecalCache + cacheIndex;
}

static float *R_DecalVertsNoclip(decal_t *pdecal, msurface_t *psurf, texture_t *ptexture, qboolean bMultitexture)
{
	float			*vlist;
	decalcache_t	*pCache;
	int				decalIndex;
	int				outCount;

	decalIndex = R_DecalIndex(pdecal);
	pCache = R_DecalCacheSlot(decalIndex);

	// Is the decal cached?
	if (pCache->decalIndex == decalIndex)
	{
		return (float *)&pCache->decalVert[0];
	}

	pCache->decalIndex = decalIndex;

	vlist = pCache->decalVert[0];

	// Use the old code for now, and just cache them
	vlist = R_DecalVertsClip(vlist, pdecal, psurf, ptexture, &outCount);

	R_DecalVertsLight(vlist, psurf, 4);

	return vlist;
}

static void R_DecalPoly(float *v, texture_t *ptexture, msurface_t *psurf, int vertCount)
{
	auto p = psurf->polys;
	auto bface = &r_wsurf.pFaceBuffer[p->flags];

	R_SetGBufferRenderState(GBUFFER_STATE_DIFFUSE);
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);

	GL_Bind(ptexture->gl_texturenum);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglBegin(GL_POLYGON);
	for (int j = 0; j < vertCount; j++, v += VERTEXSIZE)
	{
		qglTexCoord2f(v[3], v[4]);
		qglVertex3fv(v);
	}
	qglEnd();
}

static void R_DecalMPoly(float *v, texture_t *ptexture, msurface_t *psurf, int vertCount)
{
	auto p = psurf->polys;
	auto bface = &r_wsurf.pFaceBuffer[p->flags];
	auto pVert = &r_wsurf.pVertexBuffer[bface->start_vertex];

	R_SetGBufferRenderState(GBUFFER_STATE_LIGHTMAP);
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);

	GL_SelectTexture(TEXTURE0_SGIS);
	GL_Bind(ptexture->gl_texturenum);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	GL_EnableMultitexture();
	GL_Bind(lightmap_textures[psurf->lightmaptexturenum]);
	qglBegin(GL_POLYGON);
	for (int j = 0; j < vertCount; j++, v += VERTEXSIZE)
	{
		qglMultiTexCoord2fARB(TEXTURE0_SGIS, v[3], v[4]);
		qglMultiTexCoord2fARB(TEXTURE1_SGIS, v[5], v[6]);
		qglNormal3fv(pVert->normal);
		qglVertex3fv(v);
	}
	qglEnd();
}

void R_DrawDecals(qboolean bMultitexture)
{
	if (drawpolynocolor)
		return;

	//Force using multitexture
	R_SetGBufferRenderState(GBUFFER_STATE_LIGHTMAP);
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);
	gRefFuncs.R_DrawDecals(1);

	if (!bMultitexture)
		GL_DisableMultitexture();
}

void R_RenderBrushPoly(msurface_t *s)
{
	(*c_brush_polys)++;

	if (s->flags & SURF_DRAWSKY)
		return;

	if (!drawpolynocolor)
		R_RenderDynamicLightmaps(s);

	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		qglDisable(GL_TEXTURE_2D);

		if ((*r_blend) < 1)
		{
			R_SetGBufferRenderState(GBUFFER_STATE_TRANSPARENT_COLOR);
			R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);
		}
		else
		{
			R_SetGBufferRenderState(GBUFFER_STATE_COLOR);
			R_SetGBufferMask(GBUFFER_MASK_ALL);
		}

		DrawGLPolySolid(s);

		qglEnable(GL_TEXTURE_2D);
	}
	else
	{
		auto t = gRefFuncs.R_TextureAnimation(s);
		
		if (!drawpolynocolor)
		{
			GL_SelectTexture(TEXTURE0_SGIS);
			GL_Bind(t->gl_texturenum);
			qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}

		if (s->flags & SURF_DRAWTURB)
		{
			EmitWaterPolys(s, 0);
			return;
		}

		if ((*currententity)->curstate.rendermode == kRenderTransAlpha || (*currententity)->curstate.rendermode == kRenderNormal)
		{
			auto lightmapnum = s->lightmaptexturenum;

			if (!drawpolynocolor)
			{
				GL_EnableMultitexture();
				GL_Bind(lightmap_textures[lightmapnum]);
				qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				r_wsurf.bLightmapTexture = true;
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

			R_BeginDetailTexture(s);
		}

		if (r_wsurf.bDetailTexture)
		{
			R_SetGBufferRenderState(GBUFFER_STATE_DETAIL);
			R_SetGBufferMask(GBUFFER_MASK_ALL);
		}
		else if(r_wsurf.bLightmapTexture)
		{
			R_SetGBufferRenderState(GBUFFER_STATE_LIGHTMAP);
			R_SetGBufferMask(GBUFFER_MASK_ALL);
		}
		else
		{
			R_SetGBufferRenderState(GBUFFER_STATE_DIFFUSE);
			R_SetGBufferMask(GBUFFER_MASK_ALL);
		}

		if (s->flags & SURF_UNDERWATER)
		{
			//todo
		}
		else if (s->flags & SURF_DRAWTILED)
		{
			DrawGLPolyScroll(s, (*currententity));
		}
		else
		{
			DrawGLPoly(s);
		}

		R_EndDetailTexture();

		GL_DisableMultitexture();

		r_wsurf.bLightmapTexture = false;

		if (s->pdecals)
		{
			gDecalSurfs[(*gDecalSurfCount)] = s;
			(*gDecalSurfCount)++;

			if ((*gDecalSurfCount) > MAX_DECALSURFS)
				Sys_ErrorEx("Too many decal surfaces!\n");
		}
	}
}

void R_DrawSequentialPoly(msurface_t *s, int face)
{
	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		GL_DisableMultitexture();

		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable(GL_BLEND);

		qglDisable(GL_TEXTURE_2D);

		if ((*r_blend) < 1)
		{
			R_SetGBufferRenderState(GBUFFER_STATE_TRANSPARENT_COLOR);
			R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);
		}
		else
		{
			R_SetGBufferRenderState(GBUFFER_STATE_COLOR);
			R_SetGBufferMask(GBUFFER_MASK_ALL);
		}

		DrawGLPolySolid(s);

		qglEnable(GL_TEXTURE_2D);

		GL_EnableMultitexture();
		return;
	}

	if ((s->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER)))
	{
		if (s->flags & SURF_DRAWTURB)
		{
			GL_DisableMultitexture();
			GL_Bind(s->texinfo->texture->gl_texturenum);
			EmitWaterPolys(s, face);
			return;
		}
		return;
	}

	if (!drawpolynocolor)
	{
		R_RenderDynamicLightmaps(s);
	}

	if (/*(*gl_mtexable) &&*/((*currententity)->curstate.rendermode == kRenderTransAlpha || (*currententity)->curstate.rendermode == kRenderNormal))
	{
		auto p = s->polys;
		auto brushface = &r_wsurf.pFaceBuffer[p->flags];
		auto t = gRefFuncs.R_TextureAnimation(s);

		if (!drawpolynocolor)
		{
			GL_SelectTexture(TEXTURE0_SGIS);
			GL_Bind(t->gl_texturenum);
			qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			if ((*currententity)->curstate.rendermode == kRenderTransColor)
				qglDisable(GL_TEXTURE_2D);
		}

		auto lightmapnum = s->lightmaptexturenum;

		if (!drawpolynocolor)
		{
			GL_EnableMultitexture();
			GL_Bind(lightmap_textures[lightmapnum]);
			qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			r_wsurf.bLightmapTexture = true;
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

		R_BeginDetailTexture(s);

		if (r_wsurf.bDetailTexture)
		{
			R_SetGBufferRenderState(GBUFFER_STATE_DETAIL);
			R_SetGBufferMask(GBUFFER_MASK_ALL);
		}
		else
		{
			R_SetGBufferRenderState(GBUFFER_STATE_LIGHTMAP);
			R_SetGBufferMask(GBUFFER_MASK_ALL);
		}

		if (s->flags & SURF_DRAWTILED)
		{
			DrawGLPolyScroll(s, (*currententity));
		}
		else
		{
			DrawGLPoly(s);
		}

		R_EndDetailTexture();

		r_wsurf.bLightmapTexture = false;

		if (!gl_texsort->value && s->pdecals && !drawpolynocolor)
		{
			gDecalSurfs[(*gDecalSurfCount)] = s;
			(*gDecalSurfCount)++;

			if ((*gDecalSurfCount) > MAX_DECALSURFS)
				Sys_ErrorEx("Too many decal surfaces!\n");

			if ((*currententity)->curstate.rendermode != kRenderTransColor)
			{
				R_DrawDecals(true);
			}
		}

		return;
	}
	else
	{
		auto p = s->polys;
		auto brushface = &r_wsurf.pFaceBuffer[p->flags];
		auto t = gRefFuncs.R_TextureAnimation(s); 

		if (!drawpolynocolor)
		{
			GL_DisableMultitexture();
			GL_Bind(t->gl_texturenum);
		}

		R_SetGBufferRenderState(GBUFFER_STATE_DIFFUSE);
		R_SetGBufferMask(GBUFFER_MASK_ALL);

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

			R_DrawDecals(false);
		}
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
	r_light_env_angles_exists = false;
	r_light_env_color_exists = false;
	VectorClear(r_light_env_angles);

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
				r_light_env_color_exists = true;
			}
			
			char *pitch = ValueForKey(ent, "pitch");
			if (pitch)
			{
				sscanf(pitch, "%f", &r_light_env_angles[0]);
				r_light_env_angles[0] += 180;
				r_light_env_angles[1] += 180;
				if (r_light_env_angles[0] > 360)
					r_light_env_angles[0] -= 360;
				if (r_light_env_angles[1] > 360)
					r_light_env_angles[1] -= 360;
				r_light_env_angles_exists = true;
			}

			char *angle = ValueForKey(ent, "angles");
			if (angle)
			{
				vec3_t ang;
				sscanf(angle, "%f %f %f", &ang[0], &ang[1], &ang[2]);
				if (ang[0] == 0 && ang[1] == 0 && ang[2] == 0)
				{

				}
				else
				{
					VectorCopy(ang, r_light_env_angles);
					r_light_env_angles[0] += 180;
					r_light_env_angles[1] += 180;
					if (r_light_env_angles[0] > 360)
						r_light_env_angles[0] -= 360;
					if (r_light_env_angles[1] > 360)
						r_light_env_angles[1] -= 360;
					r_light_env_angles_exists = true;
				}
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