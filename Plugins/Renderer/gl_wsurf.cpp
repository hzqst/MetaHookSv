#include "gl_local.h"
#include "cJSON.h"
#include <sstream>

//TODO: detail texture

r_worldsurf_t r_wsurf;

cvar_t *r_wsurf_replace;
cvar_t *r_wsurf_vbo;

int r_wsurf_drawcall = 0;
int r_wsurf_polys = 0;

std::unordered_map<int, wsurf_program_t> g_WSurfProgramTable;

void R_UseWSurfProgram(int state, wsurf_program_t *progOutput)
{
	if (drawgbuffer)
		return;

	wsurf_program_t prog = { 0 };

	auto itor = g_WSurfProgramTable.find(state);
	if (itor == g_WSurfProgramTable.end())
	{
		std::stringstream defs;

		if (state & WSURF_DIFFUSE_ENABLED)
			defs << "#define DIFFUSE_ENABLED\n";

		if (state & WSURF_LIGHTMAP_ENABLED)
			defs << "#define LIGHTMAP_ENABLED\n";

		if (state & WSURF_DETAILTEXTURE_ENABLED)
			defs << "#define DETAILTEXTURE_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("resource\\shader\\wsurf_shader.vsh", NULL, "resource\\shader\\wsurf_shader.fsh", def.c_str(), NULL, def.c_str());
		if (prog.program)
		{
			SHADER_UNIFORM(prog, diffuseTex, "diffuseTex");
			SHADER_UNIFORM(prog, lightmapTexArray, "lightmapTexArray");
			SHADER_UNIFORM(prog, detailTex, "detailTex");
			SHADER_UNIFORM(prog, speed, "speed");
		}

		g_WSurfProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		qglUseProgramObjectARB(prog.program);

		if (prog.diffuseTex != -1)
			qglUniform1iARB(prog.diffuseTex, 0);
		if (prog.lightmapTexArray != -1)
			qglUniform1iARB(prog.lightmapTexArray, 1);
		if (prog.detailTex != -1)
			qglUniform1iARB(prog.detailTex, 2);

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseWSurfProgram: Failed to load program!");
	}
}

void R_FreeVertexBuffer(void)
{
	if (r_wsurf.hEBO)
	{
		qglDeleteBuffersARB(1, &r_wsurf.hEBO);
		r_wsurf.hEBO = 0;
	}

	if (r_wsurf.hVBO)
	{
		qglDeleteBuffersARB(1, &r_wsurf.hVBO);
		r_wsurf.hVBO = 0;
	}

	if (r_wsurf.vVertexBuffer)
	{
		delete[] r_wsurf.vVertexBuffer;
		r_wsurf.vVertexBuffer = NULL;
	}
	r_wsurf.iNumVerts = 0;

	if (r_wsurf.vFaceBuffer)
	{
		delete[] r_wsurf.vFaceBuffer;
		r_wsurf.vFaceBuffer = NULL;
	}
	r_wsurf.iNumFaces = 0;

	r_wsurf.vTextureChainStatic.clear();
	r_wsurf.vTextureChainScroll.clear();

	r_wsurf.vIndicesBuffer.clear();

	r_wsurf.iNumLightmapTextures = 0;
	GL_DeleteTexture(r_wsurf.iLightmapTextureArray);
	r_wsurf.iLightmapTextureArray = 0;
}

void R_RecursiveWorldNodeVertexBuffer(mnode_t *node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents < 0)
		return;

	R_RecursiveWorldNodeVertexBuffer(node->children[0]);

	auto c = node->numsurfaces;

	if (c)
	{
		auto surf = r_worldmodel->surfaces + node->firstsurface;

		for (; c; c--, surf++)
		{
			surf->texturechain = surf->texinfo->texture->texturechain;
			surf->texinfo->texture->texturechain = surf;
		}
	}

	R_RecursiveWorldNodeVertexBuffer(node->children[1]);
}

void R_DrawSequentialPolyVertexBuffer(msurface_t *s, brushtexchain_t *texchain)
{
	if (s->flags & SURF_DRAWSKY)
		return;

	auto p = s->polys;
	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	if (s->flags & SURF_DRAWTURB)
	{

	}
	else if (s->flags & SURF_UNDERWATER)
	{

	}
	else if (s->flags & SURF_DRAWTILED)
	{
		if (texchain->iScroll)
		{
			for (int i = 0; i < brushface->num_vertexes; ++i)
			{
				r_wsurf.vIndicesBuffer.emplace_back(brushface->start_vertex + i);
				texchain->iVertexCount++;
			}
			r_wsurf.vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
			texchain->iVertexCount++;
			texchain->iFaceCount++;
		}
	}
	else
	{
		if (!texchain->iScroll)
		{
			for (int i = 0; i < brushface->num_vertexes; ++i)
			{
				r_wsurf.vIndicesBuffer.emplace_back(brushface->start_vertex + i);
				texchain->iVertexCount++;
			}
			r_wsurf.vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
			texchain->iVertexCount++;
			texchain->iFaceCount++;
		}
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
		if ((surf[i].flags & (SURF_DRAWSKY | SURF_UNDERWATER)))
			continue;

		for (poly = surf[i].polys; poly; poly = poly->next)
			iNumVerts += 3 + (poly->numverts-3)*3;

		iNumFaces++;
	}

	r_wsurf.iNumLightmapTextures = 0;

	r_wsurf.vVertexBuffer = new brushvertex_t[iNumVerts];
	r_wsurf.iNumVerts = iNumVerts;

	r_wsurf.vFaceBuffer = new brushface_t[iNumFaces];
	r_wsurf.iNumFaces = iNumFaces;

	for(i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		if ((surf[i].flags & (SURF_DRAWSKY | SURF_UNDERWATER)))
			continue;

		poly = surf[i].polys;

		poly->flags = iCurFace;

		brushface_t *face = &r_wsurf.vFaceBuffer[iCurFace];
		VectorCopy(surf[i].texinfo->vecs[0], face->s_tangent);
		VectorCopy(surf[i].texinfo->vecs[1], face->t_tangent);
		VectorNormalize(face->s_tangent);
		VectorNormalize(face->t_tangent);
		VectorCopy(surf[i].plane->normal, face->normal);
		face->index = i;

		if (surf[i].flags & SURF_PLANEBACK)
			VectorInverse(face->normal);

		if (surf[i].lightmaptexturenum + 1 > r_wsurf.iNumLightmapTextures)
			r_wsurf.iNumLightmapTextures = surf[i].lightmaptexturenum + 1;

		auto t = surf[i].texinfo ? surf[i].texinfo->texture : NULL;

		float detailScale[2] = { 1 };
		if (t)
		{
			if (R_BeginDetailTexture(t->gl_texturenum))
			{
				detailScale[0] = r_detail_texcoord[0];
				detailScale[1] = r_detail_texcoord[1];
				R_EndDetailTexture();
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

				if (t)
					pVertexes[j].texcoord[2] = 1.0f / t->width;
				else
					pVertexes[j].texcoord[2] = 0;

				pVertexes[j].lightmaptexcoord[0] = v[5];
				pVertexes[j].lightmaptexcoord[1] = v[6];
				pVertexes[j].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
				pVertexes[j].detailtexcoord[0] = v[3] * detailScale[0];
				pVertexes[j].detailtexcoord[1] = v[4] * detailScale[1];
				pVertexes[j].normal[0] = face->normal[0];
				pVertexes[j].normal[1] = face->normal[1];
				pVertexes[j].normal[2] = face->normal[2];
			}
			memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[0], sizeof(brushvertex_t)); iCurVert++;
			memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[1], sizeof(brushvertex_t)); iCurVert++;
			memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[2], sizeof(brushvertex_t)); iCurVert++;

			for(j = 0; j < (poly->numverts-3); j++, v += VERTEXSIZE)
			{
				memcpy(&pVertexes[1], &pVertexes[2], sizeof(brushvertex_t));

				pVertexes[2].pos[0] = v[0];
				pVertexes[2].pos[1] = v[1];
				pVertexes[2].pos[2] = v[2];
				pVertexes[2].texcoord[0] = v[3];
				pVertexes[2].texcoord[1] = v[4];

				if (t)
					pVertexes[2].texcoord[2] = 1.0f / t->width;
				else
					pVertexes[2].texcoord[2] = 0;

				pVertexes[2].lightmaptexcoord[0] = v[5];
				pVertexes[2].lightmaptexcoord[1] = v[6];
				pVertexes[2].lightmaptexcoord[2] = surf[i].lightmaptexturenum;
				pVertexes[2].detailtexcoord[0] = v[3] * detailScale[0];
				pVertexes[2].detailtexcoord[1] = v[4] * detailScale[1];
				pVertexes[2].normal[0] = face->normal[0];
				pVertexes[2].normal[1] = face->normal[1];
				pVertexes[2].normal[2] = face->normal[2];
				memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[0], sizeof(brushvertex_t)); iCurVert++;
				memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[1], sizeof(brushvertex_t)); iCurVert++;
				memcpy(&r_wsurf.vVertexBuffer[iCurVert], &pVertexes[2], sizeof(brushvertex_t)); iCurVert++;
			}
		}

		face->num_vertexes = iCurVert - face->start_vertex;
		iCurFace++;
	}

	qglGenBuffersARB( 1, &r_wsurf.hVBO );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO );
	qglBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(brushvertex_t) * r_wsurf.iNumVerts, r_wsurf.vVertexBuffer, GL_STATIC_DRAW_ARB );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	R_RecursiveWorldNodeVertexBuffer(r_worldmodel->nodes);

	for (i = 0; i < r_worldmodel->numtextures; i++)
	{
		auto t = r_worldmodel->textures[i];

		if (!t)
			continue;

		auto s = t->texturechain;

		if (s)
		{
			brushtexchain_t texchain;

			texchain.pTexture = t;
			texchain.iVertexCount = 0;
			texchain.iFaceCount = 0;
			texchain.iStartIndex = r_wsurf.vIndicesBuffer.size();
			texchain.iScroll = 0;

			if (i == *skytexturenum)
			{
				continue;
			}
			else
			{
				if ((s->flags & SURF_DRAWTURB) && r_wateralpha->value != 1.0)
					continue;

				for (; s; s = s->texturechain)
				{
					R_DrawSequentialPolyVertexBuffer(s, &texchain);
				}
			}

			if (texchain.iVertexCount > 0)
				r_wsurf.vTextureChainStatic.emplace_back(texchain);
		}
		
		s = t->texturechain;
		if (s)
		{
			brushtexchain_t texchain;

			texchain.pTexture = t;
			texchain.iVertexCount = 0;
			texchain.iFaceCount = 0;
			texchain.iStartIndex = r_wsurf.vIndicesBuffer.size();
			texchain.iScroll = 1;

			if (i == *skytexturenum)
			{
				continue;
			}
			else
			{
				if ((s->flags & SURF_DRAWTURB) && r_wateralpha->value != 1.0)
					continue;

				for (; s; s = s->texturechain)
				{
					R_DrawSequentialPolyVertexBuffer(s, &texchain);
				}
			}

			if (texchain.iVertexCount > 0)
				r_wsurf.vTextureChainScroll.emplace_back(texchain);
		}

		t->texturechain = NULL;
	}

	r_wsurf.vTextureChainStatic.shrink_to_fit();
	r_wsurf.vTextureChainScroll.shrink_to_fit();
	r_wsurf.vIndicesBuffer.shrink_to_fit();

	qglGenBuffersARB(1, &r_wsurf.hEBO);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_wsurf.hEBO);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(unsigned int)*r_wsurf.vIndicesBuffer.size(), r_wsurf.vIndicesBuffer.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

	r_wsurf.iLightmapTextureArray = GL_GenTexture();
	qglEnable(GL_TEXTURE_2D_ARRAY);
	qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, BLOCK_WIDTH, BLOCK_HEIGHT, r_wsurf.iNumLightmapTextures, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	for (int i = 0; i < r_wsurf.iNumLightmapTextures; ++i)
	{
		qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, BLOCK_WIDTH, BLOCK_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + 0x10000 * i);
	}
	qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	qglDisable(GL_TEXTURE_2D_ARRAY);
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

		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
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
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_wsurf.hEBO);
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
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_wsurf.hEBO);
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
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
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
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_wsurf.hEBO);
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
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_NO_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
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
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_wsurf.hEBO);
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
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_NO_TEXTURE)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
			qglClientActiveTextureARB(GL_TEXTURE2_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		}

		else if (r_wsurf.iVBOState == VBOSTATE_OFF)
		{
			qglClientActiveTextureARB(GL_TEXTURE0_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
			qglClientActiveTextureARB(GL_TEXTURE1_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
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

void R_InitWSurf(void)
{
	r_wsurf.hVBO = 0;
	r_wsurf.hEBO = 0;
	r_wsurf.iVBOState = 0;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bDetailTexture = false;
	r_wsurf.iNumBSPEntities = 0;
	r_wsurf.iNumLightmapTextures = 0;
	r_wsurf.iLightmapTextureArray = 0;
	r_wsurf.vVertexBuffer = 0;
	r_wsurf.iNumVerts = 0;
	r_wsurf.vFaceBuffer = 0;
	r_wsurf.iNumFaces = 0;

	R_ClearBSPEntities();

	r_wsurf_vbo = gEngfuncs.pfnRegisterVariable("r_wsurf_vbo", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}


void R_ShutdownWSurf(void)
{
	g_WSurfProgramTable.clear();
}

void R_VidInitWSurf(void)
{
	//we don't need to free extra or decal textures cuz they are freed by engine when level changes.

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

		R_UseGBufferProgram(GBUFFER_TRANSPARENT_ENABLED);
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

qboolean R_BeginDetailTexture(int texId)
{
	(*r_detail_texid) = -1;
	r_detail_texcoord[0] = -1;
	r_detail_texcoord[1] = -1;

	if (gRefFuncs.R_BeginDetailTexture(texId))
	{
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		qglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
		qglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0);
		qglMatrixMode(GL_TEXTURE);
		qglLoadIdentity();
		qglMatrixMode(GL_MODELVIEW);
		return true;
	}

	return false;
}

void R_EndDetailTexture(void)
{
	if (!r_wsurf.bDetailTexture)
		return;

	r_wsurf.bDetailTexture = false;

	qglActiveTextureARB(TEXTURE2_SGIS);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
	qglDisable(GL_TEXTURE_2D);

	if((*mtexenabled))
		qglActiveTextureARB(TEXTURE1_SGIS);
	else
		qglActiveTextureARB(TEXTURE0_SGIS);
}

void DrawGLScrollingVertex(brushface_t *brushface, float sOffset)
{
	brushvertex_t *vert = &r_wsurf.vVertexBuffer[brushface->start_vertex];

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

	r_wsurf_polys ++;
	r_wsurf_drawcall ++;
}

void DrawGLVertex(brushface_t *brushface)
{
	return DrawGLScrollingVertex(brushface, 0);
}

void DrawGLPoly(glpoly_t *p)
{
	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	DrawGLVertex(brushface);

	R_DrawWireFrame(brushface, DrawGLVertex);
}

void DrawGLPoly(msurface_t *fa)
{
	auto p = fa->polys;
	return DrawGLPoly(p);
}

void DrawGLPolyScroll(msurface_t *fa, cl_entity_t *pEntity)
{
	auto p = fa->polys;	
	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	auto sOffset = ScrollOffset(fa, pEntity);

	DrawGLScrollingVertex(brushface, sOffset);

	R_DrawWireFrame(brushface, DrawGLVertex);
}

void DrawGLPolySolid(msurface_t *fa)
{
	auto p = fa->polys;
	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	qglColor4f(
		(*currententity)->curstate.rendercolor.r / 255.0,
		(*currententity)->curstate.rendercolor.g / 255.0,
		(*currententity)->curstate.rendercolor.b / 255.0,
		(*r_blend));

	DrawGLVertex(brushface);

	R_DrawWireFrame(brushface, DrawGLVertex);
}

void R_DrawDecals(qboolean bMultitexture)
{
	//Force using multitexture
	R_UseGBufferProgram(GBUFFER_DIFFUSE_ENABLED | GBUFFER_LIGHTMAP_ENABLED);
	R_SetGBufferMask(GBUFFER_MASK_DIFFUSE);
	gRefFuncs.R_DrawDecals(1);

	r_wsurf_polys ++;
	r_wsurf_drawcall ++;

	//Restore if not enabled
	if (!bMultitexture)
		GL_DisableMultitexture();
}

void R_DrawSequentialPoly(msurface_t *s, int face)
{
	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		GL_DisableMultitexture();

		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable(GL_BLEND);

		qglDisable(GL_TEXTURE_2D);

		R_UseGBufferProgram(0);
		R_SetGBufferMask(GBUFFER_MASK_ALL);

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

	R_RenderDynamicLightmaps(s);

	if (((*currententity)->curstate.rendermode == kRenderTransAlpha || (*currententity)->curstate.rendermode == kRenderNormal))
	{
		auto p = s->polys;
		auto brushface = &r_wsurf.vFaceBuffer[p->flags];
		auto t = gRefFuncs.R_TextureAnimation(s);

		GL_SelectTexture(TEXTURE0_SGIS);
		GL_Bind(t->gl_texturenum);
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		if ((*currententity)->curstate.rendermode == kRenderTransColor)
			qglDisable(GL_TEXTURE_2D);

		auto lightmapnum = s->lightmaptexturenum;

		GL_EnableMultitexture();
		GL_Bind(lightmap_textures[lightmapnum]);
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		r_wsurf.bLightmapTexture = true;

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

		r_wsurf.bDetailTexture = R_BeginDetailTexture(t->gl_texturenum);

		int GBufferProgramState = GBUFFER_DIFFUSE_ENABLED | GBUFFER_LIGHTMAP_ENABLED;

		if (r_wsurf.bDetailTexture)
		{
			GBufferProgramState |= GBUFFER_DETAILTEXTURE_ENABLED;
		}

		if (r_rotate_entity)
		{
			GBufferProgramState |= GBUFFER_ROTATE_ENABLED;
		}

		R_UseGBufferProgram(GBufferProgramState);

		R_SetGBufferMask(GBUFFER_MASK_ALL);

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

		if (s->pdecals )
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
		//No Lightmap for entity other than kRenderNormal and kRenderTransAlpha

		auto p = s->polys;
		auto brushface = &r_wsurf.vFaceBuffer[p->flags];
		auto t = gRefFuncs.R_TextureAnimation(s); 

		GL_DisableMultitexture();
		GL_Bind(t->gl_texturenum);

		r_wsurf.bDetailTexture = R_BeginDetailTexture(t->gl_texturenum);

		int GBufferProgramState = GBUFFER_DIFFUSE_ENABLED;

		if (r_wsurf.bDetailTexture)
		{
			GBufferProgramState |= GBUFFER_DETAILTEXTURE_ENABLED;
		}

		if (r_rotate_entity)
		{
			GBufferProgramState |= GBUFFER_ROTATE_ENABLED;
		}

		R_UseGBufferProgram(GBufferProgramState);

		R_SetGBufferMask(GBUFFER_MASK_ALL);

		if (s->flags & SURF_DRAWTILED)
		{
			DrawGLPolyScroll(s, (*currententity));
		}
		else
		{
			DrawGLPoly(s);
		}

		R_EndDetailTexture();

		if (s->pdecals)
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
	else
	{
		gEngfuncs.Con_Printf("R_ParseBSPEntity: missing classname, try next section.");
		while (1)
		{
			data = gEngfuncs.COM_ParseFile(data, com_token);
			if (!data)
			{
				break;
			}
			if (com_token[0] == '}')
			{
				break;
			}
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
	}//end for
}

void R_DrawSequentialPolyVBO(msurface_t *s)
{
	R_RenderDynamicLightmaps(s);

	auto lightmapnum = s->lightmaptexturenum;

	if (lightmap_modified[lightmapnum])
	{
		lightmap_modified[lightmapnum] = 0;
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			glRect_SvEngine_t *theRect = (glRect_SvEngine_t *)((char *)lightmap_rectchange + sizeof(glRect_SvEngine_t) * lightmapnum);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
			qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
		else
		{
			glRect_t *theRect = (glRect_t *)((char *)lightmap_rectchange + sizeof(glRect_t) * lightmapnum);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
			qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
	}

	if (s->pdecals)
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
}

void R_RecursiveWorldNodeVBO(mnode_t *node)
{
	int c, side;
	mplane_t *plane;
	msurface_t *surf;
	float dot;

	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != (*r_visframecount))
		return;

	if (R_CullBox(node->minmaxs, node->minmaxs + 3))
		return;

	if (node->contents < 0)
	{
		auto pleaf = (mleaf_t *)node;

		auto mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = (*r_framecount);
				mark++;
			} while (--c);
		}

		//if (pleaf->efrags)
		//	R_StoreEfrags(&pleaf->efrags);

		return;
	}

	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
	{
		dot = r_refdef->vieworg[0] - plane->dist;
		break;
	}

	case PLANE_Y:
	{
		dot = r_refdef->vieworg[1] - plane->dist;
		break;
	}

	case PLANE_Z:
	{
		dot = r_refdef->vieworg[2] - plane->dist;
		break;
	}

	default:
	{
		dot = DotProduct(r_refdef->vieworg, plane->normal) - plane->dist;
		break;
	}
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

	R_RecursiveWorldNodeVBO(node->children[side]);

	c = node->numsurfaces;

	if (c)
	{
		surf = r_worldmodel->surfaces + node->firstsurface;

		if (dot < 0 - BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;

		for (; c; c--, surf++)
		{
			if (surf->visframe != (*r_framecount))
				continue;

			if (!(surf->flags & SURF_UNDERWATER) && ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
				continue;

			if (surf->flags & SURF_DRAWSKY)
			{
				surf->texturechain = (*skychain);
				(*skychain) = surf;
			}
			else if (surf->flags & SURF_DRAWTURB)
			{
				surf->texturechain = (*waterchain);
				(*waterchain) = surf;
			}
			else
			{
				R_DrawSequentialPolyVBO(surf);
			}
		}
	}

	R_RecursiveWorldNodeVBO(node->children[!side]);
}

void R_DrawWorld(void)
{
	R_BeginRenderGBuffer();

	if (gl_wireframe->value)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(1.0f, 1.0f);
		(*r_polygon_offset) = 1.0;
	}

	cl_entity_t tempent = { 0 };
	VectorCopy(r_refdef->vieworg, modelorg);
	tempent.model = r_worldmodel;
	tempent.curstate.rendercolor.r = cshift_water->destcolor[0];
	tempent.curstate.rendercolor.g = cshift_water->destcolor[1];
	tempent.curstate.rendercolor.b = cshift_water->destcolor[2];

	(*currententity) = &tempent;
	*currenttexture = -1;

	qglColor3f(1.0f, 1.0f, 1.0f);
	memset(lightmap_polys, 0, sizeof(glpoly_t *) * 1024);
	R_ClearSkyBox();

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	qglEnable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);
	qglStencilFunc(GL_ALWAYS, 0, 0xFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	if (r_wsurf_vbo->value)
	{
		GL_DisableMultitexture();

		qglActiveTextureARB(TEXTURE1_SGIS);
		qglEnable(GL_TEXTURE_2D_ARRAY);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);

		qglActiveTextureARB(TEXTURE0_SGIS);
		qglEnable(GL_TEXTURE_2D);

		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		R_SetVBOState(VBOSTATE_LIGHTMAP_TEXTURE);

		//prepare shader for non-GBuffer mode?
		for (size_t i = 0; i < r_wsurf.vTextureChainStatic.size(); ++i)
		{
			auto &texchain = r_wsurf.vTextureChainStatic[i];

			qglBindTexture(GL_TEXTURE_2D, texchain.pTexture->gl_texturenum);

			r_wsurf.bDetailTexture = R_BeginDetailTexture(texchain.pTexture->gl_texturenum);

			if(r_wsurf.bDetailTexture)
				R_SetVBOState(VBOSTATE_DETAIL_TEXTURE);
			else
				R_SetVBOState(VBOSTATE_LIGHTMAP_TEXTURE);

			if (!drawgbuffer)
			{
				wsurf_program_t wprog = { 0 };

				int WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_LIGHTMAP_ENABLED;
				if (r_wsurf.bDetailTexture)
					WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;

				R_UseWSurfProgram(WSurfProgramState, &wprog);
				if (wprog.program)
				{
					qglUniform1fARB(wprog.speed, 0);
				}
			}
			else
			{
				R_SetGBufferMask(GBUFFER_MASK_ALL);

				int GSurfProgramState = GBUFFER_DIFFUSE_ENABLED | GBUFFER_LIGHTMAP_ENABLED | GBUFFER_LIGHTMAP_ARRAY_ENABLED;
				if (r_wsurf.bDetailTexture)
					GSurfProgramState |= GBUFFER_DETAILTEXTURE_ENABLED;

				gbuffer_program_t gprog = { 0 };
				R_UseGBufferProgram(GSurfProgramState, &gprog);
			}

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			R_EndDetailTexture();

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}

		//Use scrolling shader
		float speed = 0;
		if (r_wsurf.vTextureChainScroll.size())
		{
			speed = ((*currententity)->curstate.rendercolor.b + ((*currententity)->curstate.rendercolor.g << 8)) / 16.0;
			if ((*currententity)->curstate.rendercolor.r == 0)
				speed = -speed;
			speed *= (*cl_time);
		}

		for (size_t i = 0; i < r_wsurf.vTextureChainScroll.size(); ++i)
		{
			auto &texchain = r_wsurf.vTextureChainScroll[i];

			qglBindTexture(GL_TEXTURE_2D, texchain.pTexture->gl_texturenum);

			r_wsurf.bDetailTexture = R_BeginDetailTexture(texchain.pTexture->gl_texturenum);

			if (r_wsurf.bDetailTexture)
				R_SetVBOState(VBOSTATE_DETAIL_TEXTURE);
			else
				R_SetVBOState(VBOSTATE_LIGHTMAP_TEXTURE);

			if (!drawgbuffer)
			{
				wsurf_program_t wprog2 = { 0 };

				int WSurfProgramState = WSURF_DIFFUSE_ENABLED | WSURF_LIGHTMAP_ENABLED;
				if (r_wsurf.bDetailTexture)
					WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;

				R_UseWSurfProgram(WSurfProgramState, &wprog2);
				if (wprog2.program)
				{
					qglUniform1fARB(wprog2.speed, speed);
				}
			}
			else
			{
				gbuffer_program_t gprog2 = { 0 };

				int GSurfProgramState = GBUFFER_DIFFUSE_ENABLED | GBUFFER_LIGHTMAP_ENABLED | GBUFFER_LIGHTMAP_ARRAY_ENABLED | GBUFFER_SCROLL_ENABLED;
				if (r_wsurf.bDetailTexture)
					GSurfProgramState |= GBUFFER_DETAILTEXTURE_ENABLED;

				R_UseGBufferProgram(GSurfProgramState, &gprog2);
				if (gprog2.program)
				{
					qglUniform1fARB(gprog2.speed, speed);
				}
			}

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			R_EndDetailTexture();

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}

		qglUseProgramObjectARB(0);

		R_SetVBOState(VBOSTATE_OFF);

		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		qglActiveTextureARB(TEXTURE1_SGIS);
		qglDisable(GL_TEXTURE_2D_ARRAY);
		qglActiveTextureARB(TEXTURE0_SGIS);

		(*gDecalSurfCount) = 0;
		R_RecursiveWorldNodeVBO(r_worldmodel->nodes);
		(*gDecalSurfCount) = 0;
	}
	else
	{
		(*gDecalSurfCount) = 0;
		R_RecursiveWorldNode(r_worldmodel->nodes);
		(*gDecalSurfCount) = 0;
	}

	(*currententity) = gEngfuncs.GetEntityByIndex(0);

	GL_DisableMultitexture();

	if ((*skychain))
	{
		R_DrawSkyChain((*skychain));
		(*skychain) = 0;
	}

	if ((*waterchain))
	{
		for (auto s = (*waterchain); s; s = s->texturechain)
		{
			qglColor4ub(255, 255, 255, 255);
			GL_Bind(s->texinfo->texture->gl_texturenum);
			EmitWaterPolys(s, 0);
		}
		(*waterchain) = 0;
	}

	if (gl_wireframe->value)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
		(*r_polygon_offset) = 0.0;
	}

	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);
}