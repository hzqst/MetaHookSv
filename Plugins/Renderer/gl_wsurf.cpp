#include "gl_local.h"
#include <sstream>
#include <algorithm>

r_worldsurf_t r_wsurf;

cvar_t *r_wsurf_vbo;
cvar_t *r_wsurf_parallax_scale;
cvar_t *r_wsurf_detail;
cvar_t *r_wsurf_sky_occlusion;

int r_fog_mode = 0;
float r_fog_control[2] = { 0 };
float r_fog_color[4] = {0};
float r_shadow_matrix[3][16];
float r_world_matrix_inv[16];
vec3_t r_frustum_origin[4];
vec3_t r_frustum_vec[4];
int r_wsurf_drawcall = 0;
int r_wsurf_polys = 0;

std::unordered_map <int, wsurf_program_t> g_WSurfProgramTable;

std::unordered_map<int, detail_texture_cache_t *> g_DetailTextureTable;

std::unordered_map<model_t *, wsurf_model_t *> g_WSurfModelCache;

void R_ClearWSurfModelCache(void)
{
	for (auto &itor = g_WSurfModelCache.begin(); itor != g_WSurfModelCache.end(); ++itor)
	{
		if (itor->second->hEBO)
		{
			qglDeleteBuffersARB(1, &itor->second->hEBO);
		}

		delete itor->second;
	}
	g_WSurfModelCache.clear();
}

void R_UseWSurfProgram(int state, wsurf_program_t *progOutput)
{
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

		if (state & WSURF_NORMALTEXTURE_ENABLED)
			defs << "#define NORMALTEXTURE_ENABLED\n";

		if (state & WSURF_PARALLAXTEXTURE_ENABLED)
			defs << "#define PARALLAXTEXTURE_ENABLED\n";

		if (state & WSURF_SPECULARTEXTURE_ENABLED)
			defs << "#define SPECULARTEXTURE_ENABLED\n";

		if (state & WSURF_CLIP_ABOVE_ENABLED)
			defs << "#define CLIP_ABOVE_ENABLED\n";

		if (state & WSURF_CLIP_UNDER_ENABLED)
			defs << "#define CLIP_UNDER_ENABLED\n";

		if (state & WSURF_LINEAR_FOG_ENABLED)
			defs << "#define LINEAR_FOG_ENABLED\n";

		if (state & WSURF_GBUFFER_ENABLED)
			defs << "#define GBUFFER_ENABLED\n";

		if (state & WSURF_TRANSPARENT_ENABLED)
			defs << "#define TRANSPARENT_ENABLED\n";

		if (state & WSURF_SHADOW_CASTER_ENABLED)
			defs << "#define SHADOW_CASTER_ENABLED\n";

		if (state & WSURF_SHADOWMAP_ENABLED)
		{
			defs << "#define SHADOWMAP_ENABLED\n";
			defs << "const float SHADOW_TEXTURE_OFFSET = 1.0 / " << shadow_texture_size << ".0; \n";
		}

		if (state & WSURF_SHADOWMAP_HIGH_ENABLED)
			defs << "#define SHADOWMAP_HIGH_ENABLED\n";

		if (state & WSURF_SHADOWMAP_MEDIUM_ENABLED)
			defs << "#define SHADOWMAP_MEDIUM_ENABLED\n";

		if (state & WSURF_SHADOWMAP_LOW_ENABLED)
			defs << "#define SHADOWMAP_LOW_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\wsurf_shader.vsh", NULL, "renderer\\shader\\wsurf_shader.fsh", def.c_str(), NULL, def.c_str());
		SHADER_UNIFORM(prog, diffuseTex, "diffuseTex");
		SHADER_UNIFORM(prog, lightmapTexArray, "lightmapTexArray");
		SHADER_UNIFORM(prog, shadowmapTexArray, "shadowmapTexArray");
		SHADER_UNIFORM(prog, detailTex, "detailTex");
		SHADER_UNIFORM(prog, normalTex, "normalTex");
		SHADER_UNIFORM(prog, parallaxTex, "parallaxTex");
		SHADER_UNIFORM(prog, specularTex, "specularTex");
		SHADER_UNIFORM(prog, speed, "speed");
		SHADER_UNIFORM(prog, entityMatrix, "entityMatrix");
		SHADER_UNIFORM(prog, shadowMatrix, "shadowMatrix");
		SHADER_UNIFORM(prog, shadowDirection, "shadowDirection");
		SHADER_UNIFORM(prog, shadowFade, "shadowFade");
		SHADER_UNIFORM(prog, shadowColor, "shadowColor");
		SHADER_UNIFORM(prog, clipPlane, "clipPlane");
		SHADER_UNIFORM(prog, clipInfo, "clipInfo");
		SHADER_UNIFORM(prog, viewpos, "viewpos");
		SHADER_UNIFORM(prog, parallaxScale, "parallaxScale");
		SHADER_ATTRIB(prog, s_tangent, "s_tangent");
		SHADER_ATTRIB(prog, t_tangent, "t_tangent");

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

		if (prog.normalTex != -1)
			qglUniform1iARB(prog.normalTex, 3);

		if (prog.parallaxTex != -1)
			qglUniform1iARB(prog.parallaxTex, 4);

		if (prog.specularTex != -1)
			qglUniform1iARB(prog.specularTex, 5);

		if (prog.shadowmapTexArray != -1)
			qglUniform1iARB(prog.shadowmapTexArray, 6);

		if (prog.shadowDirection != -1)
			qglUniform3fARB(prog.shadowDirection, r_shadow_control.vforward[0], r_shadow_control.vforward[1], r_shadow_control.vforward[2]);

		if (prog.shadowFade != -1)
			qglUniform3fARB(prog.shadowFade, r_shadow_control.fade[0], r_shadow_control.fade[1], r_shadow_control.minlum);

		if (prog.shadowColor != -1)
			qglUniform4fARB(prog.shadowColor, r_shadow_control.color[0], r_shadow_control.color[1], r_shadow_control.color[2], r_shadow_control.color[3]);

		if (prog.entityMatrix != -1)
		{
			if (r_rotate_entity)
				qglUniformMatrix4fvARB(prog.entityMatrix, 1, true, (float *)r_rotate_entity_matrix);
			else
				qglUniformMatrix4fvARB(prog.entityMatrix, 1, false, (float *)r_identity_matrix);
		}

		if (prog.clipPlane != -1)
			qglUniform1fARB(prog.clipPlane, curwater->vecs[2]);

		if (prog.viewpos != -1)
			qglUniform3fARB(prog.viewpos, r_refdef->vieworg[0], r_refdef->vieworg[1], r_refdef->vieworg[2]);

		if (prog.clipInfo != -1)
			qglUniform3fARB(prog.clipInfo, 4 * r_params.movevars->zmax, 4 - r_params.movevars->zmax, r_params.movevars->zmax);

		if (prog.parallaxScale != -1)
			qglUniform1fARB(prog.parallaxScale, r_wsurf_parallax_scale->value);

		if (prog.s_tangent != -1 && r_wsurf.pCurrentModel)
		{
			qglVertexAttribPointer(prog.s_tangent, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, s_tangent));
			qglEnableVertexAttribArray(prog.s_tangent);
		}

		if (prog.t_tangent != -1 && r_wsurf.pCurrentModel)
		{
			qglVertexAttribPointer(prog.t_tangent, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, t_tangent));
			qglEnableVertexAttribArray(prog.t_tangent);
		}

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		Sys_ErrorEx("R_UseWSurfProgram: Failed to load program!");
	}
}

void R_FreeLightmapArray(void)
{
	r_wsurf.iNumLightmapTextures = 0;
	if (r_wsurf.iLightmapTextureArray)
	{
		GL_DeleteTexture(r_wsurf.iLightmapTextureArray);
		r_wsurf.iLightmapTextureArray = 0;
	}
}

void R_FreeVertexBuffer(void)
{
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
}

void R_RecursiveWorldNodeGenerateTextureChain(mnode_t *node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents < 0)
		return;

	R_RecursiveWorldNodeGenerateTextureChain(node->children[0]);

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

	R_RecursiveWorldNodeGenerateTextureChain(node->children[1]);
}

void R_GenerateElementBufferIndices(msurface_t *s, brushtexchain_t *texchain, wsurf_model_t *modcache)
{
	auto p = s->polys;
	auto brushface = &r_wsurf.vFaceBuffer[p->flags];

	if (s->flags & SURF_DRAWSKY)
	{
		if (texchain->iType == TEXCHAIN_SKY)
		{
			for (int i = 0; i < brushface->num_vertexes; ++i)
			{
				modcache->vIndicesBuffer.emplace_back(brushface->start_vertex + i);
				texchain->iVertexCount++;
			}
			modcache->vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
			texchain->iVertexCount++;
			texchain->iFaceCount++;
		}
	}
	else if (s->flags & SURF_DRAWTURB)
	{

	}
	else if (s->flags & SURF_UNDERWATER)
	{

	}
	else if (s->flags & SURF_DRAWTILED)
	{
		if (texchain->iType == TEXCHAIN_SCROLL)
		{
			for (int i = 0; i < brushface->num_vertexes; ++i)
			{
				modcache->vIndicesBuffer.emplace_back(brushface->start_vertex + i);
				texchain->iVertexCount++;
			}
			modcache->vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
			texchain->iVertexCount++;
			texchain->iFaceCount++;
		}
	}
	else
	{

		if (texchain->iType == TEXCHAIN_STATIC)
		{
			for (int i = 0; i < brushface->num_vertexes; ++i)
			{
				modcache->vIndicesBuffer.emplace_back(brushface->start_vertex + i);
				texchain->iVertexCount++;
			}
			modcache->vIndicesBuffer.emplace_back((unsigned int)0xFFFFFFFF);
			texchain->iVertexCount++;
			texchain->iFaceCount++;
		}
	}
}

void R_GenerateElementBuffer(model_t *mod, wsurf_model_t *modcache)
{
	if (mod == r_worldmodel)
	{
		R_RecursiveWorldNodeGenerateTextureChain(mod->nodes);
	}
	else
	{
		auto psurf = &mod->surfaces[mod->firstmodelsurface];
		for (int i = 0; i < mod->nummodelsurfaces; i++, psurf++)
		{
			auto pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				continue;
			}
			else if (psurf->flags & SURF_DRAWSKY)
			{
				continue;
			}

			psurf->texturechain = psurf->texinfo->texture->texturechain;
			psurf->texinfo->texture->texturechain = psurf;
		}
	}

	for (int i = 0; i < mod->numtextures; i++)
	{
		auto t = mod->textures[i];

		if (!t)
			continue;

		if (!strcmp(t->name, "sky"))
		{
			auto s = t->texturechain;

			if (s)
			{
				brushtexchain_t texchain;

				texchain.pTexture = t;
				texchain.iVertexCount = 0;
				texchain.iFaceCount = 0;
				texchain.iStartIndex = modcache->vIndicesBuffer.size();
				texchain.iType = TEXCHAIN_SKY;

				for (; s; s = s->texturechain)
				{
					R_GenerateElementBufferIndices(s, &texchain, modcache);
				}

				if (texchain.iVertexCount > 0)
					modcache->TextureChainSky = texchain;
			}
		}
		else if (t->anim_total)
		{
			if (t->name[0] == '-')
			{
				//Construct texchain for random textures

				auto s = t->texturechain;

				if (s)
				{
					if (s->flags & SURF_DRAWSKY)
					{
						t->texturechain = NULL;
						continue;
					}
					else
					{
						if (s->flags & SURF_DRAWTURB)
						{
							t->texturechain = NULL;
							continue;
						}

						brushtexchain_t *texchainArray = new brushtexchain_t[t->anim_total];

						int numtexturechain = 0;
						for (msurface_t *s2 = s; s2; s2 = s2->texturechain)
						{
							numtexturechain++;
						}

						//rtable not initialized?
						if ((*rtable)[0][0] == 0)
						{
							gRefFuncs.R_TextureAnimation(s);
						}

						int *texchainMapper = new int[numtexturechain];
						msurface_t **texchainSurface = new msurface_t*[numtexturechain];

						{
							msurface_t *s2 = s;
							int k = 0;

							for (; s2; s2 = s2->texturechain, ++k)
							{
								texchainSurface[k] = s2;

								int mappingIndex = (*rtable)[(int)((s2->texturemins[0] + (t->width << 16)) / t->width) % 20][(int)((s2->texturemins[1] + (t->height << 16)) / t->height) % 20] % t->anim_total;

								texchainMapper[k] = mappingIndex;
							}
						}

						{
							texture_t *t2 = t;
							int k = 0;
							for (; k < t->anim_total && t2; t2 = t2->anim_next, ++k)
							{
								brushtexchain_t texchain;
								texchain.pTexture = t2;
								texchain.iVertexCount = 0;
								texchain.iFaceCount = 0;
								texchain.iStartIndex = modcache->vIndicesBuffer.size();
								texchain.iType = TEXCHAIN_STATIC;

								for (int n = 0; n < numtexturechain; ++n)
								{
									if (texchainMapper[n] == k)
										R_GenerateElementBufferIndices(texchainSurface[n], &texchain, modcache);
								}

								if (texchain.iVertexCount > 0)
									modcache->vTextureChainStatic.emplace_back(texchain);
							}
						}

						delete []texchainSurface;
						delete []texchainMapper;
						delete []texchainArray;
					}
				}
			}
			else if (t->name[0] == '+')
			{
				//Construct texchain for anim textures

				auto s = t->texturechain;

				if (s)
				{
					if (s->flags & SURF_DRAWSKY)
					{
						t->texturechain = NULL;
						continue;
					}
					else
					{
						if (s->flags & SURF_DRAWTURB)
						{
							t->texturechain = NULL;
							continue;
						}

						brushtexchain_t texchain;

						texchain.pTexture = t;
						texchain.iVertexCount = 0;
						texchain.iFaceCount = 0;
						texchain.iStartIndex = modcache->vIndicesBuffer.size();
						texchain.iType = TEXCHAIN_STATIC;

						for (; s; s = s->texturechain)
						{
							R_GenerateElementBufferIndices(s, &texchain, modcache);
						}

						if (texchain.iVertexCount > 0)
							modcache->vTextureChainAnim.emplace_back(texchain);
					}
				}
			}
		}
		else
		{
			//Construct texchain for static textures

			auto s = t->texturechain;

			if (s)
			{
				if (s->flags & SURF_DRAWSKY)
				{
					t->texturechain = NULL;
					continue;
				}
				else
				{
					if (s->flags & SURF_DRAWTURB)
					{
						t->texturechain = NULL;
						continue;
					}

					brushtexchain_t texchain;

					texchain.pTexture = t;
					texchain.iVertexCount = 0;
					texchain.iFaceCount = 0;
					texchain.iStartIndex = modcache->vIndicesBuffer.size();
					texchain.iType = TEXCHAIN_STATIC;

					for (; s; s = s->texturechain)
					{
						R_GenerateElementBufferIndices(s, &texchain, modcache);
					}

					if (texchain.iVertexCount > 0)
						modcache->vTextureChainStatic.emplace_back(texchain);
				}
			}

			//Construct texchain for scroll textures

			s = t->texturechain;
			if (s)
			{
				if (s->flags & SURF_DRAWSKY)
				{
					t->texturechain = NULL;
					continue;
				}

				if (s->flags & SURF_DRAWTURB)
				{
					t->texturechain = NULL;
					continue;
				}

				brushtexchain_t texchain;

				texchain.pTexture = t;
				texchain.iVertexCount = 0;
				texchain.iFaceCount = 0;
				texchain.iStartIndex = modcache->vIndicesBuffer.size();
				texchain.iType = TEXCHAIN_SCROLL;

				for (; s; s = s->texturechain)
				{
					R_GenerateElementBufferIndices(s, &texchain, modcache);
				}

				if (texchain.iVertexCount > 0)
					modcache->vTextureChainScroll.emplace_back(texchain);
			}
		}

		//End construction

		t->texturechain = NULL;
	}

	modcache->vTextureChainStatic.shrink_to_fit();
	modcache->vTextureChainScroll.shrink_to_fit();
	modcache->vTextureChainAnim.shrink_to_fit();
	modcache->vIndicesBuffer.shrink_to_fit();

	qglGenBuffersARB(1, &modcache->hEBO);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, modcache->hEBO);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(unsigned int) * modcache->vIndicesBuffer.size(), modcache->vIndicesBuffer.data(), GL_STATIC_DRAW_ARB);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
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
		if ((surf[i].flags & (/*SURF_DRAWSKY |*/ SURF_UNDERWATER)))
			continue;

		for (poly = surf[i].polys; poly; poly = poly->next)
			iNumVerts += 3 + (poly->numverts-3)*3;

		iNumFaces++;
	}

	r_wsurf.vVertexBuffer = new brushvertex_t[iNumVerts];
	r_wsurf.iNumVerts = iNumVerts;

	r_wsurf.vFaceBuffer = new brushface_t[iNumFaces];
	r_wsurf.iNumFaces = iNumFaces;

	for(i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		if ((surf[i].flags & (/*SURF_DRAWSKY | */SURF_UNDERWATER)))
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

		float detailScale[2] = { 1, 1 };
		float normalScale[2] = { 1, 1 };
		float parallaxScale[2] = { 1, 1 };
		float specularScale[2] = { 1, 1 };

		if (t)
		{
			auto pcache = R_FindDetailTextureCache(t->gl_texturenum);
			if (pcache)
			{
				if (pcache->tex[WSURF_DETAIL_TEXTURE].gltexturenum)
				{
					detailScale[0] = pcache->tex[WSURF_DETAIL_TEXTURE].scaleX;
					detailScale[1] = pcache->tex[WSURF_DETAIL_TEXTURE].scaleY;
				}
				if (pcache->tex[WSURF_NORMAL_TEXTURE].gltexturenum)
				{
					normalScale[0] = pcache->tex[WSURF_NORMAL_TEXTURE].scaleX;
					normalScale[1] = pcache->tex[WSURF_NORMAL_TEXTURE].scaleY;
				}
				if (pcache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum)
				{
					parallaxScale[0] = pcache->tex[WSURF_PARALLAX_TEXTURE].scaleX;
					parallaxScale[1] = pcache->tex[WSURF_PARALLAX_TEXTURE].scaleY;
				}
				if (pcache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum)
				{
					specularScale[0] = pcache->tex[WSURF_SPECULAR_TEXTURE].scaleX;
					specularScale[1] = pcache->tex[WSURF_SPECULAR_TEXTURE].scaleY;
				}
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
				pVertexes[j].detailtexcoord[0] = detailScale[0];
				pVertexes[j].detailtexcoord[1] = detailScale[1];
				pVertexes[j].normaltexcoord[0] = normalScale[0];
				pVertexes[j].normaltexcoord[1] = normalScale[1];
				pVertexes[j].parallaxtexcoord[0] = parallaxScale[0];
				pVertexes[j].parallaxtexcoord[1] = parallaxScale[1];
				pVertexes[j].speculartexcoord[0] = specularScale[0];
				pVertexes[j].speculartexcoord[1] = specularScale[1];
				pVertexes[j].normal[0] = face->normal[0];
				pVertexes[j].normal[1] = face->normal[1];
				pVertexes[j].normal[2] = face->normal[2];
				pVertexes[j].s_tangent[0] = face->s_tangent[0];
				pVertexes[j].s_tangent[1] = face->s_tangent[1];
				pVertexes[j].s_tangent[2] = face->s_tangent[2];
				pVertexes[j].t_tangent[0] = face->t_tangent[0];
				pVertexes[j].t_tangent[1] = face->t_tangent[1];
				pVertexes[j].t_tangent[2] = face->t_tangent[2];
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
				pVertexes[2].detailtexcoord[0] = detailScale[0];
				pVertexes[2].detailtexcoord[1] = detailScale[1];
				pVertexes[2].normaltexcoord[0] = normalScale[0];
				pVertexes[2].normaltexcoord[1] = normalScale[1];
				pVertexes[2].parallaxtexcoord[0] = parallaxScale[0];
				pVertexes[2].parallaxtexcoord[1] = parallaxScale[1];
				pVertexes[2].speculartexcoord[0] = specularScale[0];
				pVertexes[2].speculartexcoord[1] = specularScale[1];
				pVertexes[2].normal[0] = face->normal[0];
				pVertexes[2].normal[1] = face->normal[1];
				pVertexes[2].normal[2] = face->normal[2];
				pVertexes[2].s_tangent[0] = face->s_tangent[0];
				pVertexes[2].s_tangent[1] = face->s_tangent[1];
				pVertexes[2].s_tangent[2] = face->s_tangent[2];
				pVertexes[2].t_tangent[0] = face->t_tangent[0];
				pVertexes[2].t_tangent[1] = face->t_tangent[1];
				pVertexes[2].t_tangent[2] = face->t_tangent[2];
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
}

void R_GenerateLightmapArray(void)
{
	r_wsurf.iLightmapTextureArray = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, BLOCK_WIDTH, BLOCK_HEIGHT, r_wsurf.iNumLightmapTextures, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	for (int i = 0; i < r_wsurf.iNumLightmapTextures; ++i)
	{
		qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, BLOCK_WIDTH, BLOCK_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + 0x10000 * i);
	}
	qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

wsurf_model_t *R_PrepareWSurfVBO(model_t *mod)
{
	wsurf_model_t *modcache = NULL;

	auto itor = g_WSurfModelCache.find(mod);

	if (itor == g_WSurfModelCache.end())
	{
		modcache = new wsurf_model_t;

		R_GenerateElementBuffer(mod, modcache);

		modcache->pModel = mod;

		g_WSurfModelCache[mod] = modcache;
	}
	else
	{
		modcache = itor->second;
	}

	return modcache;
}

void R_EnableWSurfVBOSolid(wsurf_model_t *modcache)
{
	if (!r_wsurf_vbo->value)
		return;

	if (r_wsurf.pCurrentModel == modcache)
		return;

	r_wsurf.pCurrentModel = modcache;

	if (!modcache)
	{
		qglDisableVertexAttribArray(1);
		qglDisableVertexAttribArray(0);

		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		qglDisableClientState(GL_NORMAL_ARRAY);
		qglDisableClientState(GL_VERTEX_ARRAY);
	}
	else
	{
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, modcache->hEBO);
		qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
		qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));

		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
}

void R_EnableWSurfVBO(wsurf_model_t *modcache)
{
	if (!r_wsurf_vbo->value)
		return;

	if (r_wsurf.pCurrentModel == modcache)
		return;

	r_wsurf.pCurrentModel = modcache;

	if (!modcache)
	{
		qglDisableVertexAttribArray(1);
		qglDisableVertexAttribArray(0);

		qglDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		qglClientActiveTextureARB(GL_TEXTURE4_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE3_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE2_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		qglDisableClientState(GL_NORMAL_ARRAY);
		qglDisableClientState(GL_VERTEX_ARRAY);
	}
	else
	{
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, r_wsurf.hVBO);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, modcache->hEBO);
		qglNormalPointer(GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
		qglVertexPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));

		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(3, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
		qglClientActiveTextureARB(GL_TEXTURE2_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, detailtexcoord));
		qglClientActiveTextureARB(GL_TEXTURE3_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, normaltexcoord));
		qglClientActiveTextureARB(GL_TEXTURE4_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, parallaxtexcoord));
		qglClientActiveTextureARB(GL_TEXTURE5_ARB);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(brushvertex_t), OFFSET(brushvertex_t, speculartexcoord));

		qglEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
}

void R_DrawWSurfVBO(wsurf_model_t *modcache)
{
	//This only applies to world rendering
	if(modcache->pModel == r_worldmodel && r_wsurf_sky_occlusion->value)
	{
		//Sky surface uses stencil = 1
		qglStencilFunc(GL_ALWAYS, 1, 0xFF);

		qglColorMask(0, 0, 0, 0);

		auto &texchain = modcache->TextureChainSky;

		if (texchain.iVertexCount > 0)
		{
			GL_DisableMultitexture();

			GL_Bind(0);

			int WSurfProgramState = 0;

			if (r_draw_pass == r_draw_reflect && curwater)
			{
				WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
			}

			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
			}

			if (drawgbuffer)
			{
				WSurfProgramState |= WSURF_GBUFFER_ENABLED;
			}

			wsurf_program_t prog = { 0 };
			R_UseWSurfProgram(WSurfProgramState, &prog);

			if (prog.speed != -1)
				qglUniform1fARB(prog.speed, 0);

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}

		qglColorMask(1, 1, 1, 1);
	}

	//World alway use stencil = 0
	qglStencilFunc(GL_ALWAYS, 0, 0xFF);

	if (r_wsurf.bShadowmapTexture)
	{
		const GLfloat planeS[] = { 1.0, 0.0, 0.0, 0.0 };
		const GLfloat planeT[] = { 0.0, 1.0, 0.0, 0.0 };
		const GLfloat planeR[] = { 0.0, 0.0, 1.0, 0.0 };
		const GLfloat planeQ[] = { 0.0, 0.0, 0.0, 1.0 };

		qglActiveTextureARB(GL_TEXTURE6_ARB);

		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		qglEnable(GL_TEXTURE_GEN_S);
		qglEnable(GL_TEXTURE_GEN_T);
		qglEnable(GL_TEXTURE_GEN_R);
		qglEnable(GL_TEXTURE_GEN_Q);
		qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_S, GL_EYE_PLANE, planeS);
		qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_T, GL_EYE_PLANE, planeT);
		qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_R, GL_EYE_PLANE, planeR);
		qglTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_Q, GL_EYE_PLANE, planeQ);

		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_TEXTURE_2D_ARRAY);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, shadow_texture_color);

		qglActiveTextureARB(*oldtarget);
	}

	if (r_wsurf.bLightmapTexture)
	{
		GL_EnableMultitexture();
		qglDisable(GL_TEXTURE_2D);		
		qglEnable(GL_TEXTURE_2D_ARRAY);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
		*currenttexture = -1;
	}
	else
	{
		GL_DisableMultitexture();
	}

	if (r_wsurf.bDiffuseTexture)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglEnable(GL_TEXTURE_2D);
	}
	else
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglDisable(GL_TEXTURE_2D);
	}

	//Static texchain

	for (size_t i = 0; i < modcache->vTextureChainStatic.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainStatic[i];

		if (texchain.iVertexCount > 0)
		{
			GL_Bind(texchain.pTexture->gl_texturenum);

			R_BeginDetailTexture(texchain.pTexture->gl_texturenum);

			int WSurfProgramState = 0;

			if (r_wsurf.bDiffuseTexture)
			{
				WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
			}

			if (r_wsurf.bLightmapTexture)
			{
				WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
			}

			if (r_wsurf.bShadowmapTexture)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

				for (int i = 0; i < 3; ++i)
				{
					if (shadow_numvisedicts[i] > 0)
					{
						WSurfProgramState |= (WSURF_SHADOWMAP_HIGH_ENABLED << i);
					}
				}
			}

			if (r_wsurf.bDetailTexture)
			{
				WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
			}

			if (r_wsurf.bNormalTexture)
			{
				WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
			}

			if (r_wsurf.bParallaxTexture)
			{
				WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
			}

			if (r_wsurf.bSpecularTexture)
			{
				WSurfProgramState |= WSURF_SPECULARTEXTURE_ENABLED;
			}

			if (r_draw_pass == r_draw_reflect && curwater)
			{
				WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
			}

			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
			}

			if (drawgbuffer)
			{
				WSurfProgramState |= WSURF_GBUFFER_ENABLED;
			}

			if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
			{
				WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
			}

			if (r_draw_pass == r_draw_shadow_caster)
			{
				WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
			}

			wsurf_program_t prog = { 0 };
			R_UseWSurfProgram(WSurfProgramState, &prog);

			if (prog.speed != -1)
				qglUniform1fARB(prog.speed, 0);

			if(prog.shadowMatrix != -1)
				qglUniformMatrix4fvARB(prog.shadowMatrix, 3, false, (float *)r_shadow_matrix);

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			R_EndDetailTexture();

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}
	}

	//Animated texchain

	for (size_t i = 0; i < modcache->vTextureChainAnim.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainAnim[i];

		if (texchain.iVertexCount > 0)
		{
			auto base = texchain.pTexture;

			if ((*currententity)->curstate.effects & EF_SNIPERLASER)
			{
				int frame_count = 0;
				int total_frame = (*currententity)->curstate.frame;
				do
				{
					if (base->anim_next)
						base = base->anim_next;
					++frame_count;
				} while (frame_count < (*currententity)->curstate.frame);
			}
			else
			{
				if ((*currententity)->curstate.frame && base->alternate_anims)
					base = base->alternate_anims;

				if (!((*currententity)->curstate.effects & EF_NIGHTVISION))
				{
					int reletive = (int)((*cl_time) * 10.0f) % base->anim_total;

					int loop_count = 0;

					while (base->anim_min > reletive || base->anim_max <= reletive)
					{
						base = base->anim_next;

						if (!base)
							Sys_ErrorEx("R_TextureAnimation: broken cycle");

						if (++loop_count > 100)
							Sys_ErrorEx("R_TextureAnimation: infinite cycle");
					}
				}
			}

			GL_Bind(base->gl_texturenum);

			R_BeginDetailTexture(base->gl_texturenum);

			int WSurfProgramState = 0;

			if (r_wsurf.bDiffuseTexture)
			{
				WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
			}

			if (r_wsurf.bLightmapTexture)
			{
				WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
			}

			if (r_wsurf.bShadowmapTexture)
			{
				WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

				for (int i = 0; i < 3; ++i)
				{
					if (shadow_numvisedicts[i] > 0)
					{
						WSurfProgramState |= (WSURF_SHADOWMAP_HIGH_ENABLED << i);
					}
				}
			}

			if (r_wsurf.bDetailTexture)
			{
				WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
			}

			if (r_wsurf.bNormalTexture)
			{
				WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
			}

			if (r_wsurf.bParallaxTexture)
			{
				WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
			}

			if (r_wsurf.bSpecularTexture)
			{
				WSurfProgramState |= WSURF_SPECULARTEXTURE_ENABLED;
			}

			if (r_draw_pass == r_draw_reflect && curwater)
			{
				WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
			}

			if (!drawgbuffer && r_fog_mode == GL_LINEAR)
			{
				WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
			}

			if (drawgbuffer)
			{
				WSurfProgramState |= WSURF_GBUFFER_ENABLED;
			}

			if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
			{
				WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
			}

			if (r_draw_pass == r_draw_shadow_caster)
			{
				WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
			}

			wsurf_program_t prog = { 0 };
			R_UseWSurfProgram(WSurfProgramState, &prog);

			if (prog.speed != -1)
				qglUniform1fARB(prog.speed, 0);

			if (prog.shadowMatrix != -1)
				qglUniformMatrix4fvARB(prog.shadowMatrix, 3, false, (float *)r_shadow_matrix);

			qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

			R_EndDetailTexture();

			r_wsurf_drawcall++;
			r_wsurf_polys += texchain.iFaceCount;
		}
	}

	//Use scrolling shader
	if (modcache->vTextureChainScroll.size())
	{
		float scrollSpeed = ((*currententity)->curstate.rendercolor.b + ((*currententity)->curstate.rendercolor.g << 8)) / 16.0;
		if ((*currententity)->curstate.rendercolor.r == 0)
			scrollSpeed = -scrollSpeed;
		scrollSpeed *= (*cl_time);

		for (size_t i = 0; i < modcache->vTextureChainScroll.size(); ++i)
		{
			auto &texchain = modcache->vTextureChainScroll[i];

			if (texchain.iVertexCount > 0)
			{
				GL_Bind(texchain.pTexture->gl_texturenum);

				R_BeginDetailTexture(texchain.pTexture->gl_texturenum);

				wsurf_program_t wprog = { 0 };

				int WSurfProgramState = 0;

				if (r_wsurf.bDiffuseTexture)
				{
					WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
				}

				if (r_wsurf.bLightmapTexture)
				{
					WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
				}

				if (r_wsurf.bShadowmapTexture)
				{
					WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

					for (int i = 0; i < 3; ++i)
					{
						if (shadow_numvisedicts[i] > 0)
						{
							WSurfProgramState |= (WSURF_SHADOWMAP_HIGH_ENABLED << i);
						}
					}
				}

				if (r_wsurf.bDetailTexture)
				{
					WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
				}

				if (r_wsurf.bNormalTexture)
				{
					WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
				}

				if (r_wsurf.bParallaxTexture)
				{
					WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
				}

				if (r_wsurf.bSpecularTexture)
				{
					WSurfProgramState |= WSURF_SPECULARTEXTURE_ENABLED;
				}

				if (r_draw_pass == r_draw_reflect && curwater)
				{
					WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
				}

				if (!drawgbuffer && r_fog_mode == GL_LINEAR)
				{
					WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
				}

				if (drawgbuffer)
				{
					WSurfProgramState |= WSURF_GBUFFER_ENABLED;
				}

				if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
				{
					WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
				}

				if (r_draw_pass == r_draw_shadow_caster)
				{
					WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
				}

				wsurf_program_t prog = { 0 };
				R_UseWSurfProgram(WSurfProgramState, &prog);

				if (prog.speed != -1)
					qglUniform1fARB(prog.speed, scrollSpeed);

				if (prog.shadowMatrix != -1)
					qglUniformMatrix4fvARB(prog.shadowMatrix, 3, false, (float *)r_shadow_matrix);

				qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

				R_EndDetailTexture();

				r_wsurf_drawcall++;
				r_wsurf_polys += texchain.iFaceCount;
			}
		}
	}

	if (r_wsurf.bShadowmapTexture)
	{
		qglActiveTextureARB(GL_TEXTURE6_ARB);

		qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		qglDisable(GL_TEXTURE_2D_ARRAY);
		qglDisable(GL_TEXTURE_GEN_S);
		qglDisable(GL_TEXTURE_GEN_T);
		qglDisable(GL_TEXTURE_GEN_R);
		qglDisable(GL_TEXTURE_GEN_Q);

		qglActiveTextureARB(*oldtarget);
	}

	if (r_wsurf.bLightmapTexture)
	{
		qglActiveTextureARB(TEXTURE1_SGIS);

		qglDisable(GL_TEXTURE_2D_ARRAY);
		if (*mtexenabled)
			qglEnable(GL_TEXTURE_2D);
		else
			qglDisable(GL_TEXTURE_2D);

		GL_DisableMultitexture();
	}

	if (!r_wsurf.bDiffuseTexture)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglEnable(GL_TEXTURE_2D);
	}

	if (modcache->pModel == r_worldmodel)
	{
		(*gDecalSurfCount) = 0;
		R_RecursiveWorldNodeVBO(r_worldmodel->nodes);
		(*gDecalSurfCount) = 0;
	}

	//This only applies to world rendering, draw world again for correct depth
	if (modcache->pModel == r_worldmodel && r_wsurf_sky_occlusion->value)
	{
		qglDepthMask(GL_TRUE);
		qglClear(GL_DEPTH_BUFFER_BIT);

		qglStencilMask(0);

		qglColorMask(0, 0, 0, 0);

		GL_DisableMultitexture();

		GL_Bind(0);

		int WSurfProgramState = 0;

		if (r_draw_pass == r_draw_reflect && curwater)
		{
			WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
		}

		if (!drawgbuffer && r_fog_mode == GL_LINEAR)
		{
			WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
		}

		if (drawgbuffer)
		{
			WSurfProgramState |= WSURF_GBUFFER_ENABLED;
		}

		if (r_draw_pass == r_draw_shadow_caster)
		{
			WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
		}

		wsurf_program_t prog = { 0 };
		R_UseWSurfProgram(WSurfProgramState, &prog);

		if (prog.speed != -1)
			qglUniform1fARB(prog.speed, 0);

		R_DrawWSurfVBOSolid(modcache);

		qglStencilMask(0xFF);

		qglColorMask(1, 1, 1, 1);
	}

	qglUseProgramObjectARB(0);
}

void R_DrawWSurfVBOSolid(wsurf_model_t *modcache)
{
	for (size_t i = 0; i < modcache->vTextureChainStatic.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainStatic[i];

		qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.iFaceCount;
	}

	for (size_t i = 0; i < modcache->vTextureChainAnim.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainAnim[i];

		qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.iFaceCount;
	}

	for (size_t i = 0; i < modcache->vTextureChainScroll.size(); ++i)
	{
		auto &texchain = modcache->vTextureChainScroll[i];

		qglDrawElements(GL_POLYGON, texchain.iVertexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(texchain.iStartIndex));

		r_wsurf_drawcall++;
		r_wsurf_polys += texchain.iFaceCount;
	}
}

void R_Reload_f(void)
{
	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities);
	R_LoadExternalEntities();
	R_LoadBSPEntities();

	gEngfuncs.Con_Printf("Entities reloaded\n");
}

void R_InitWSurf(void)
{
	r_wsurf.hVBO = 0;
	r_wsurf.bLightmapTexture = false;
	r_wsurf.bDetailTexture = false;
	r_wsurf.bNormalTexture = false;
	r_wsurf.bParallaxTexture = false;
	r_wsurf.pDetailTextureCache = NULL;
	r_wsurf.pCurrentModel = NULL;
	r_wsurf.iNumBSPEntities = 0;
	r_wsurf.iNumLightmapTextures = 0;
	r_wsurf.iLightmapTextureArray = 0;
	r_wsurf.vVertexBuffer = 0;
	r_wsurf.iNumVerts = 0;
	r_wsurf.vFaceBuffer = 0;
	r_wsurf.iNumFaces = 0;

	R_ClearBSPEntities();

	r_wsurf_vbo = gEngfuncs.pfnRegisterVariable("r_wsurf_vbo", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_parallax_scale = gEngfuncs.pfnRegisterVariable("r_wsurf_parallax_scale", "0.03", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_detail = gEngfuncs.pfnRegisterVariable("r_wsurf_detail", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	r_wsurf_sky_occlusion = gEngfuncs.pfnRegisterVariable("r_wsurf_sky_occlusion", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}

void R_ShutdownWSurf(void)
{
	g_WSurfProgramTable.clear();

	R_FreeLightmapArray();
	R_FreeVertexBuffer();
	R_ClearWSurfModelCache();
	R_ClearBSPEntities();
}

void R_LoadDetailTextures(void)
{
	std::string name = gEngfuncs.pfnGetLevelName();
	name = name.substr(0, name.length() - 4);
	name += "_detail.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_Printf("R_LoadDetailTextures: No detail texture file %s\n", name.c_str());
		return;
	}

	char *ptext = pfile;
	while (1)
	{
		char temp[256];
		char basetexture[256];
		char detailtexture[256];
		char sz_xscale[64];
		char sz_yscale[64];

		ptext = gEngfuncs.COM_ParseFile(ptext, basetexture);

		if (!ptext)
			break;

		if (basetexture[0] == '{')
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, temp);
			strcat(basetexture, temp);

			if (!ptext)
				break;
		}

		ptext = gEngfuncs.COM_ParseFile(ptext, detailtexture);
		if (!ptext)
			break;

		ptext = gEngfuncs.COM_ParseFile(ptext, sz_xscale);

		if (!ptext)
			break;

		if (sz_xscale[0] == '{')
		{
			strcat(detailtexture, sz_xscale);

			ptext = gEngfuncs.COM_ParseFile(ptext, temp);
			if (!ptext)
				break;

			strcat(detailtexture, temp);

			ptext = gEngfuncs.COM_ParseFile(ptext, sz_xscale);
			if (!ptext)
				break;
		}

		ptext = gEngfuncs.COM_ParseFile(ptext, sz_yscale);
		if (!ptext)
			break;

		//Default: load as detail texture
		int texType = WSURF_DETAIL_TEXTURE;

		std::string base = basetexture;

		if (base.length() > (sizeof("_PARALLAX") - 1) && !strcmp(&base[base.length() - (sizeof("_PARALLAX") - 1)], "_PARALLAX"))
		{
			base = base.substr(0, base.length() - (sizeof("_PARALLAX") - 1));
			texType = WSURF_PARALLAX_TEXTURE;
		}
		else if (base.length() > (sizeof("_NORMAL") - 1) && !strcmp(&base[base.length() - (sizeof("_NORMAL") - 1)], "_NORMAL"))
		{
			base = base.substr(0, base.length() - (sizeof("_NORMAL") - 1));
			texType = WSURF_NORMAL_TEXTURE;
		}
		else if (base.length() > (sizeof("_REPLACE") - 1) && !strcmp(&base[base.length() - (sizeof("_REPLACE") - 1)], "_REPLACE"))
		{
			base = base.substr(0, base.length() - (sizeof("_REPLACE") - 1));
			texType = WSURF_REPLACE_TEXTURE;
		}
		else if (base.length() > (sizeof("_DETAIL") - 1) && !strcmp(&base[base.length() - (sizeof("_DETAIL") - 1)], "_DETAIL"))
		{
			base = base.substr(0, base.length() - (sizeof("_DETAIL") - 1));
			texType = WSURF_DETAIL_TEXTURE;
		}
		else if (base.length() > (sizeof("_SPECULAR") - 1) && !strcmp(&base[base.length() - (sizeof("_SPECULAR") - 1)], "_SPECULAR"))
		{
			base = base.substr(0, base.length() - (sizeof("_SPECULAR") - 1));
			texType = WSURF_SPECULAR_TEXTURE;
		}

		auto glt = GL_FindTexture(base.c_str(), GLT_WORLD, NULL, NULL);

		if (!glt)
		{
			gEngfuncs.Con_Printf("R_LoadDetailTextures: Missing basetexture %s\n", base.c_str());
			continue;
		}

		float i_xscale = atof(sz_xscale);
		float i_yscale = atof(sz_yscale);

		detail_texture_cache_t *cache = NULL;

		auto itor = std::find_if(g_DetailTextureTable.begin(), g_DetailTextureTable.end(),
			[&base](const std::pair<int, detail_texture_cache_t *>& pair) {return (pair.second->basetexture == base); });

		if (itor != g_DetailTextureTable.end())
		{
			cache = itor->second;
		}
		else
		{
			cache = new detail_texture_cache_t;
			cache->basetexture = base;
			g_DetailTextureTable[glt] = cache;
		}

		const char *textypeNames[] = {
			"WSURF_REPLACE_TEXTURE",
			"WSURF_DETAIL_TEXTURE",
			"WSURF_NORMAL_TEXTURE",
			"WSURF_PARALLAX_TEXTURE",
			"WSURF_SPECULAR_TEXTURE",
		};

		if (cache)
		{
			if (cache->tex[texType].gltexturenum)
			{
				gEngfuncs.Con_Printf("R_LoadDetailTextures: %s already exists for basetexture %s\n", textypeNames[texType], base.c_str());
				continue;
			}

			int width = 0, height = 0;

			std::string texturePath = "gfx/";
			texturePath += detailtexture;
			if (!V_GetFileExtension(detailtexture))
				texturePath += ".tga";

			int texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, true, true);
			if (!texId)
			{
				texturePath = "renderer/texture/";
				texturePath += detailtexture;
				if (!V_GetFileExtension(detailtexture))
					texturePath += ".tga";

				texId = R_LoadTextureEx(texturePath.c_str(), texturePath.c_str(), &width, &height, GLT_WORLD, true, true);
			}

			if (!texId)
			{
				gEngfuncs.Con_Printf("R_LoadDetailTextures: Failed to load %s as %s for basetexture %s\n", detailtexture, textypeNames[texType], base.c_str());
				continue;
			}

			cache->tex[texType].gltexturenum = texId;
			cache->tex[texType].width = width;
			cache->tex[texType].height = height;
			cache->tex[texType].scaleX = i_xscale;
			cache->tex[texType].scaleY = i_yscale;
		}
	}
	gEngfuncs.COM_FreeFile(pfile);
}

void R_VidInitWSurf(void)
{
	for (auto p : g_DetailTextureTable)
		delete p.second;

	g_DetailTextureTable.clear();
	R_LoadDetailTextures();

	R_FreeLightmapArray();
	R_FreeVertexBuffer();
	R_GenerateVertexBuffer();
	R_GenerateLightmapArray();
	R_ClearWSurfModelCache();
	R_ClearBSPEntities();
	R_ParseBSPEntities(r_worldmodel->entities);
	R_LoadExternalEntities();
	R_LoadBSPEntities();
}

void R_DrawWireFrame(brushface_t *brushface, void(*draw)(brushface_t *face))
{
	/*if (gl_wireframe->value)
	{
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
	}*/
}

detail_texture_cache_t *R_FindDetailTextureCache(int texId)
{
	auto itor = g_DetailTextureTable.find(texId);

	if (itor != g_DetailTextureTable.end())
	{
		auto cache = itor->second;

		if (cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum ||
			cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum ||
			cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum
			)
		{
			return cache;
		}
	}

	return NULL;
}

void R_BeginDetailTexture(int texId)
{
	if (!r_wsurf_detail->value)
		return;

	auto itor = g_DetailTextureTable.find(texId);

	if (itor != g_DetailTextureTable.end())
	{
		auto cache = itor->second;

		if (r_wsurf.bDiffuseTexture && cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum)
		{
			GL_Bind(cache->tex[WSURF_REPLACE_TEXTURE].gltexturenum);
		}

		if (cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum)
		{
			qglActiveTextureARB(TEXTURE2_SGIS);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_DETAIL_TEXTURE].gltexturenum);

			r_wsurf.bDetailTexture = true;
		}

		if (cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum)
		{
			qglActiveTextureARB(TEXTURE3_SGIS);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_NORMAL_TEXTURE].gltexturenum);

			r_wsurf.bNormalTexture = true;
		}

		if (cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum)
		{
			qglActiveTextureARB(GL_TEXTURE4_ARB);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_PARALLAX_TEXTURE].gltexturenum);

			r_wsurf.bParallaxTexture = true;
		}

		if (cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum)
		{
			qglActiveTextureARB(GL_TEXTURE5_ARB);
			qglEnable(GL_TEXTURE_2D);
			qglBindTexture(GL_TEXTURE_2D, cache->tex[WSURF_SPECULAR_TEXTURE].gltexturenum);

			r_wsurf.bSpecularTexture = true;
		}

		if (r_wsurf.bDetailTexture || r_wsurf.bNormalTexture || r_wsurf.bParallaxTexture || r_wsurf.bSpecularTexture)
		{
			r_wsurf.pDetailTextureCache = cache;
			return;
		}
	}
}

void R_EndDetailTexture(void)
{
	bool bRestore = false;

	if (r_wsurf.bDetailTexture)
	{
		r_wsurf.bDetailTexture = false;
		bRestore = true;

		qglActiveTextureARB(TEXTURE2_SGIS);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bNormalTexture)
	{
		r_wsurf.bNormalTexture = false;
		bRestore = true;

		qglActiveTextureARB(TEXTURE3_SGIS);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bParallaxTexture)
	{
		r_wsurf.bParallaxTexture = false;
		bRestore = true;

		qglActiveTextureARB(GL_TEXTURE4_ARB);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bSpecularTexture)
	{
		r_wsurf.bSpecularTexture = false;
		bRestore = true;

		qglActiveTextureARB(GL_TEXTURE5_ARB);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisable(GL_TEXTURE_2D);
	}

	if (bRestore)
	{
		qglActiveTextureARB(*oldtarget);
	}
}

void DrawGLVertex(brushface_t *brushface)
{
	brushvertex_t *vert = &r_wsurf.vVertexBuffer[brushface->start_vertex];

	qglBegin( GL_POLYGON );
	for(int i = 0; i < brushface->num_vertexes; i++, vert++)
	{
		if (r_wsurf.bDiffuseTexture)
			qglMultiTexCoord3fARB(TEXTURE0_SGIS, vert->texcoord[0], vert->texcoord[1], vert->texcoord[2]);

		if (r_wsurf.bLightmapTexture)
			qglMultiTexCoord3fARB(TEXTURE1_SGIS, vert->lightmaptexcoord[0], vert->lightmaptexcoord[1], vert->lightmaptexcoord[2]);

		if (r_wsurf.bDetailTexture)
			qglMultiTexCoord2fARB(TEXTURE2_SGIS, vert->detailtexcoord[0], vert->detailtexcoord[1]);

		if (r_wsurf.bNormalTexture)
			qglMultiTexCoord2fARB(TEXTURE3_SGIS, vert->normaltexcoord[0], vert->normaltexcoord[1]);

		if (r_wsurf.bParallaxTexture)
			qglMultiTexCoord2fARB(GL_TEXTURE4_ARB, vert->parallaxtexcoord[0], vert->parallaxtexcoord[1]);

		if (r_wsurf.bSpecularTexture)
			qglMultiTexCoord2fARB(GL_TEXTURE4_ARB, vert->speculartexcoord[0], vert->speculartexcoord[1]);

		if (r_wsurf.iS_Tangent != -1)
			qglVertexAttrib3fv(r_wsurf.iS_Tangent, vert->s_tangent);

		if (r_wsurf.iT_Tangent != -1)
			qglVertexAttrib3fv(r_wsurf.iT_Tangent, vert->t_tangent);

		qglNormal3fv(vert->normal);
		qglVertex3fv(vert->pos);
	}
	qglEnd();

	r_wsurf_polys ++;
	r_wsurf_drawcall ++;
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

void R_DrawSequentialPoly(msurface_t *s, int face)
{
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

	auto p = s->polys;
	auto t = gRefFuncs.R_TextureAnimation(s);

	float shadow_matrix[3][16];

	if (r_wsurf.bShadowmapTexture)
	{
		const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f };

		const GLfloat planeS[] = { 1.0, 0.0, 0.0, 0.0 };
		const GLfloat planeT[] = { 0.0, 1.0, 0.0, 0.0 };
		const GLfloat planeR[] = { 0.0, 0.0, 1.0, 0.0 };
		const GLfloat planeQ[] = { 0.0, 0.0, 0.0, 1.0 };

		qglActiveTextureARB(GL_TEXTURE6_ARB);

		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		qglEnable(GL_TEXTURE_GEN_S);
		qglEnable(GL_TEXTURE_GEN_T);
		qglEnable(GL_TEXTURE_GEN_R);
		qglEnable(GL_TEXTURE_GEN_Q);
		qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_S, GL_EYE_PLANE, planeS);
		qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_T, GL_EYE_PLANE, planeT);
		qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_R, GL_EYE_PLANE, planeR);
		qglTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		qglTexGenfv(GL_Q, GL_EYE_PLANE, planeQ);

		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_TEXTURE_2D_ARRAY);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, shadow_texture_color);

		qglMatrixMode(GL_TEXTURE);

		for (int i = 0; i < 3; ++i)
		{
			if (shadow_numvisedicts[i] > 0)
			{
				qglLoadIdentity();
				qglLoadMatrixf(bias);
				qglMultMatrixf(shadow_projmatrix[i]);
				qglMultMatrixf(shadow_mvmatrix[i]);
				qglMultMatrixf(r_world_matrix_inv);
				qglGetFloatv(GL_TEXTURE_MATRIX, shadow_matrix[i]);
			}
		}

		qglLoadIdentity();
		qglMatrixMode(GL_MODELVIEW);

		qglActiveTextureARB(*oldtarget);
	}

	if (r_wsurf.bLightmapTexture)
	{
		GL_EnableMultitexture();
		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_TEXTURE_2D_ARRAY);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
		*currenttexture = -1;

		int lightmapnum = s->lightmaptexturenum;

		if (lightmap_modified[lightmapnum])
		{
			lightmap_modified[lightmapnum] = 0;

			glRect_t *theRect = (glRect_t *)((char *)lightmap_rectchange + sizeof(glRect_t) * lightmapnum);
			qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
	}
	else
	{
		GL_DisableMultitexture();
	}

	if (r_wsurf.bDiffuseTexture)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		GL_Bind(t->gl_texturenum);
	}
	else
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglDisable(GL_TEXTURE_2D);
	}

	if (r_wsurf.bDiffuseTexture)
	{
		R_BeginDetailTexture(t->gl_texturenum);
	}

	int WSurfProgramState = 0;

	if (r_wsurf.bDiffuseTexture)
	{
		WSurfProgramState |= WSURF_DIFFUSE_ENABLED;
	}

	if (r_wsurf.bLightmapTexture)
	{
		WSurfProgramState |= WSURF_LIGHTMAP_ENABLED;
	}

	if (r_wsurf.bShadowmapTexture)
	{
		WSurfProgramState |= WSURF_SHADOWMAP_ENABLED;

		for (int i = 0; i < 3; ++i)
		{
			if (shadow_numvisedicts[i] > 0)
			{
				WSurfProgramState |= (WSURF_SHADOWMAP_HIGH_ENABLED << i);
			}
		}
	}

	if (r_wsurf.bDetailTexture)
	{
		WSurfProgramState |= WSURF_DETAILTEXTURE_ENABLED;
	}

	if (r_wsurf.bNormalTexture)
	{
		WSurfProgramState |= WSURF_NORMALTEXTURE_ENABLED;
	}

	if (r_wsurf.bParallaxTexture)
	{
		WSurfProgramState |= WSURF_PARALLAXTEXTURE_ENABLED;
	}

	if (r_wsurf.bSpecularTexture)
	{
		WSurfProgramState |= WSURF_SPECULARTEXTURE_ENABLED;
	}

	if (r_draw_pass == r_draw_reflect && curwater)
	{
		WSurfProgramState |= WSURF_CLIP_UNDER_ENABLED;
	}

	if (!drawgbuffer && r_fog_mode == GL_LINEAR)
	{
		WSurfProgramState |= WSURF_LINEAR_FOG_ENABLED;
	}

	if (drawgbuffer)
	{
		WSurfProgramState |= WSURF_GBUFFER_ENABLED;
	}

	if ((*currententity)->curstate.rendermode != kRenderNormal && (*currententity)->curstate.rendermode != kRenderTransAlpha)
	{
		WSurfProgramState |= WSURF_TRANSPARENT_ENABLED;
	}

	if (r_draw_pass == r_draw_shadow_caster)
	{
		WSurfProgramState |= WSURF_SHADOW_CASTER_ENABLED;
	}

	wsurf_program_t prog = { 0 };
	R_UseWSurfProgram(WSurfProgramState, &prog);

	float speed = 0;

	if (s->flags & SURF_DRAWTILED)
	{
		speed = ((*currententity)->curstate.rendercolor.b + ((*currententity)->curstate.rendercolor.g << 8)) / 16.0;

		if ((*currententity)->curstate.rendercolor.r == 0)
			speed = -speed;
	}

	if (prog.speed != -1)
	{
		qglUniform1fARB(prog.speed, speed);
	}

	r_wsurf.iS_Tangent = prog.s_tangent;
	r_wsurf.iT_Tangent = prog.t_tangent;

	R_SetGBufferMask(GBUFFER_MASK_ALL);

	DrawGLPoly(p);

	qglUseProgramObjectARB(0);

	R_EndDetailTexture();

	if (r_wsurf.bShadowmapTexture)
	{
		qglActiveTextureARB(GL_TEXTURE6_ARB);

		qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		qglDisable(GL_TEXTURE_2D_ARRAY);
		qglDisable(GL_TEXTURE_GEN_S);
		qglDisable(GL_TEXTURE_GEN_T);
		qglDisable(GL_TEXTURE_GEN_R);
		qglDisable(GL_TEXTURE_GEN_Q);

		qglActiveTextureARB(*oldtarget);
	}

	if (r_wsurf.bLightmapTexture)
	{
		qglActiveTextureARB(TEXTURE1_SGIS);

		qglDisable(GL_TEXTURE_2D_ARRAY);

		if(*mtexenabled)
			qglEnable(GL_TEXTURE_2D);
		else
			qglDisable(GL_TEXTURE_2D);

		GL_DisableMultitexture();
	}

	if (!r_wsurf.bDiffuseTexture)
	{
		GL_SelectTexture(TEXTURE0_SGIS);
		qglEnable(GL_TEXTURE_2D);
	}

	if (s->pdecals)
	{
		gDecalSurfs[(*gDecalSurfCount)] = s;
		(*gDecalSurfCount)++;

		if ((*gDecalSurfCount) > MAX_DECALSURFS)
			Sys_ErrorEx("Too many decal surfaces!\n");

		if (r_wsurf.bDiffuseTexture)
		{
			R_DrawDecals(r_wsurf.bLightmapTexture ? true : false);
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
	r_water_controls.clear();
	//r_cubemaps.clear();
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

void R_LoadExternalEntities(void)
{
	std::string name;
	
	name = gEngfuncs.pfnGetLevelName();
	name = name.substr(0, name.length() - 4);
	name += "_entity.txt";

	char *pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
	if (!pfile)
	{
		gEngfuncs.Con_Printf("R_LoadExternalEntities: No external entity file %s\n", name.c_str());

		name = "renderer/default_entity.txt";

		pfile = (char *)gEngfuncs.COM_LoadFile((char *)name.c_str(), 5, NULL);
		if (!pfile)
		{
			gEngfuncs.Con_Printf("R_LoadExternalEntities: No default external entity file %s\n", name.c_str());

			return;
		}
	}

	R_ParseBSPEntities(pfile);

	gEngfuncs.COM_FreeFile(pfile);
}

#if 0
void R_ParseBSPEntity_Env_Cubemap(bspentity_t *ent)
{
	float temp[4];

	cubemap_t cubemap;
	cubemap.cubetex = 0;
	cubemap.size = 0;
	cubemap.radius = 0;
	cubemap.origin[0] = 0;
	cubemap.origin[1] = 0;
	cubemap.origin[2] = 0;
	cubemap.extension = "tga";

	char *name_string = ValueForKey(ent, "name");
	if (name_string)
	{
		cubemap.name = name_string;
	}

	char *origin_string = ValueForKey(ent, "origin");
	if (origin_string)
	{
		if (sscanf(origin_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			cubemap.origin[0] = temp[0];
			cubemap.origin[1] = temp[1];
			cubemap.origin[2] = temp[2];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"origin\" in entity \"env_cubemap\"\n");
		}
	}

	char *cubemapsize_string = ValueForKey(ent, "cubemapsize");
	if (cubemapsize_string)
	{
		int size = 0;
		if (sscanf(cubemapsize_string, "%d", &size) == 1)
		{
			cubemap.size = size;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"cubemapsize\" in entity \"env_cubemap\"\n");
		}
	}

	char *radius_string = ValueForKey(ent, "radius");
	if (radius_string)
	{
		if (sscanf(radius_string, "%f", &temp[0]) == 1 && temp[0] > 0)
		{
			cubemap.radius = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"radius\" in entity \"env_cubemap\"\n");
		}
	}

	char *extension_string = ValueForKey(ent, "extension");
	if (extension_string)
	{
		cubemap.extension = extension_string;
	}

	if (cubemap.name.length() > 0 && cubemap.radius > 0)
	{
		R_LoadCubemap(&cubemap);

		r_cubemaps.emplace_back(cubemap);
	}
}
#endif

void R_ParseBSPEntity_Light_Dynamic(bspentity_t *ent)
{
	float temp[4];

	light_dynamic_t dynlight;
	dynlight.type = DLIGHT_POINT;
	VectorClear(dynlight.origin);
	VectorClear(dynlight.color);
	dynlight.distance = 0;
	dynlight.ambient = 0;
	dynlight.diffuse = 0;
	dynlight.specular = 0;
	dynlight.specularpow = 0;

	char *origin_string = ValueForKey(ent, "origin");
	if (origin_string)
	{
		if (sscanf(origin_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			dynlight.origin[0] = temp[0];
			dynlight.origin[1] = temp[1];
			dynlight.origin[2] = temp[2];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"origin\" in entity \"light_dynamic\"\n");
		}
	}

	char *color_string = ValueForKey(ent, "_light");
	if (color_string)
	{
		if (sscanf(color_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			dynlight.color[0] = clamp(temp[0], 0, 255) / 255.0f;
			dynlight.color[1] = clamp(temp[1], 0, 255) / 255.0f;
			dynlight.color[2] = clamp(temp[2], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_light\" in entity \"light_dynamic\"\n");
		}
	}

	char *distance_string = ValueForKey(ent, "_distance");
	if (distance_string)
	{
		if (sscanf(distance_string, "%f", &temp[0]) == 1)
		{
			dynlight.distance = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_distance\" in entity \"light_dynamic\"\n");
		}
	}

	char *ambient_string = ValueForKey(ent, "_ambient");
	if (ambient_string)
	{
		if (sscanf(ambient_string, "%f", &temp[0]) == 1)
		{
			dynlight.ambient = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_ambient\" in entity \"light_dynamic\"\n");
		}
	}

	char *diffuse_string = ValueForKey(ent, "_diffuse");
	if (diffuse_string)
	{
		if (sscanf(diffuse_string, "%f", &temp[0]) == 1)
		{
			dynlight.diffuse = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_diffuse\" in entity \"light_dynamic\"\n");
		}
	}

	char *specular_string = ValueForKey(ent, "_specular");
	if (specular_string)
	{
		if (sscanf(specular_string, "%f", &temp[0]) == 1)
		{
			dynlight.specular = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_specular\" in entity \"light_dynamic\"\n");
		}
	}

	char *specularpow_string = ValueForKey(ent, "_specularpow");
	if (specularpow_string)
	{
		if (sscanf(specularpow_string, "%f", &temp[0]) == 1)
		{
			dynlight.specularpow = temp[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"_specularpow\" in entity \"light_dynamic\"\n");
		}
	}

	g_DynamicLights.emplace_back(dynlight);
}

void R_ParseBSPEntity_Env_Water_Control(bspentity_t *ent)
{
	float temp[4];

	water_control_t control;
	control.enabled = true;
	control.fresnelfactor = 0;
	control.depthfactor[0] = 0;
	control.depthfactor[1] = 0;
	control.normfactor = 0;
	control.minheight = 0;
	control.maxtrans = 0;

	char *basetexture_string = ValueForKey(ent, "basetexture");
	if (basetexture_string)
	{
		control.basetexture = basetexture_string;
		if (control.basetexture[control.basetexture.length() - 1] == '*')
		{
			control.wildcard = control.basetexture.substr(0, control.basetexture.length() - 1);
		}
	}

	char *disablewater_string = ValueForKey(ent, "disablewater");
	if (disablewater_string)
	{
		if (atoi(disablewater_string) > 0)
		{
			control.enabled = false;
		}
	}

	char *normalmap_string = ValueForKey(ent, "normalmap");
	if (normalmap_string)
	{
		control.normalmap = normalmap_string;
	}

	char *fresnelfactor_string = ValueForKey(ent, "fresnelfactor");
	if (fresnelfactor_string)
	{
		if (sscanf(fresnelfactor_string, "%f", &temp[0]) == 1)
		{
			control.fresnelfactor = clamp(temp[0], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"fresnelfactor\" in entity \"env_water_control\"\n");
		}
	}

	char *normfactor_string = ValueForKey(ent, "normfactor");
	if (normfactor_string)
	{
		if (sscanf(normfactor_string, "%f", &temp[0]) == 1)
		{
			control.normfactor = clamp(temp[0], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"normfactor\" in entity \"env_water_control\"\n");
		}
	}

	char *depthfactor_string = ValueForKey(ent, "depthfactor");
	if (depthfactor_string)
	{
		if (sscanf(depthfactor_string, "%f %f", &temp[0], &temp[1]) == 2)
		{
			control.depthfactor[0] = clamp(temp[0], 0, 10);
			control.depthfactor[1] = clamp(temp[1], 0, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"depthfactor\" in entity \"env_water_control\"\n");
		}
	}

	char *minheight_string = ValueForKey(ent, "minheight");
	if (minheight_string)
	{
		if (sscanf(minheight_string, "%f", &temp[0]) == 1)
		{
			control.minheight = clamp(temp[0], 0, 10000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"minheight\" in entity \"env_water_control\"\n");
		}
	}

	char *maxtrans_string = ValueForKey(ent, "maxtrans");
	if (maxtrans_string)
	{
		if (sscanf(maxtrans_string, "%f", &temp[0]) == 1)
		{
			control.maxtrans = clamp(temp[0], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"maxtrans\" in entity \"env_water_control\"\n");
		}
	}

	if (control.basetexture.length())
	{
		r_water_controls.emplace_back(control);
	}
}

void R_ParseBSPEntity_Env_HDR_Control(bspentity_t *ent)
{
	float temp[4];

	char *adaptation_string = ValueForKey(ent, "adaptation");
	if (adaptation_string)
	{
		if (sscanf(adaptation_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.adaptation = clamp(temp[0], 0.1, 100);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"adaptation\" in entity \"env_hdr_control\"\n");
		}
	}
	char *blurwidth_string = ValueForKey(ent, "blurwidth");
	if (blurwidth_string)
	{
		if (sscanf(blurwidth_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.blurwidth = clamp(temp[0], 0, 1);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"blurwidth\" in entity \"env_hdr_control\"\n");
		}
	}
	char *darkness_string = ValueForKey(ent, "darkness");
	if (darkness_string)
	{
		if (sscanf(darkness_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.darkness = clamp(temp[0], 0.01, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"darkness\" in entity \"env_hdr_control\"\n");
		}
	}
	char *exposure_string = ValueForKey(ent, "exposure");
	if (exposure_string)
	{
		if (sscanf(exposure_string, "%f", &temp[0]) == 1)
		{
			r_hdr_control.exposure = clamp(temp[0], 0.01, 10);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"exposure\" in entity \"env_hdr_control\"\n");
		}
	}
	char *disablehdr_string = ValueForKey(ent, "disablehdr");
	if (disablehdr_string)
	{
		if (atoi(disablehdr_string) > 0)
		{
			r_hdr_control.enabled = false;
		}
	}
}

void R_ParseBSPEntity_Env_Shadow_Control(bspentity_t *ent)
{
	float temp[4];

	char *color_string = ValueForKey(ent, "color");
	if (color_string)
	{
		if (sscanf(color_string, "%f %f %f %f", &temp[0], &temp[1], &temp[2], &temp[3]) == 4)
		{
			r_shadow_control.color[0] = clamp(temp[0], 0, 255) / 255.0f;
			r_shadow_control.color[1] = clamp(temp[1], 0, 255) / 255.0f;
			r_shadow_control.color[2] = clamp(temp[2], 0, 255) / 255.0f;
			r_shadow_control.color[3] = clamp(temp[3], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"color\" in entity \"env_shadow_control\"\n");
		}
	}

	char *angles_string = ValueForKey(ent, "angles");
	if (angles_string)
	{
		if (sscanf(angles_string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
		{
			r_shadow_control.angles[0] = clamp(temp[0], -360, 360);
			r_shadow_control.angles[1] = clamp(temp[1], -360, 360);
			r_shadow_control.angles[2] = clamp(temp[2], -360, 360);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"angles\" in entity \"env_shadow_control\"\n");
		}
	}

	char *fade_string = ValueForKey(ent, "fade");
	if (fade_string)
	{
		if (sscanf(fade_string, "%f %f", &temp[0], &temp[1]) == 2)
		{
			r_shadow_control.fade[0] = clamp(temp[0], 0, 10000);
			r_shadow_control.fade[1] = clamp(temp[1], r_shadow_control.fade[0], 10000) - r_shadow_control.fade[0];
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"fade\" in entity \"env_shadow_control\"\n");
		}
	}

	char *minlum_string = ValueForKey(ent, "minlum");
	if (minlum_string)
	{
		if (sscanf(minlum_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.minlum = clamp(temp[0], 0, 255) / 255.0f;
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"minlum\" in entity \"env_shadow_control\"\n");
		}
	}

	char *disableallshadows_string = ValueForKey(ent, "disableallshadows");
	if (disableallshadows_string)
	{
		if (atoi(disableallshadows_string) > 0)
		{
			r_shadow_control.enabled = false;
		}
	}

	char *high_distance_string = ValueForKey(ent, "high_distance");
	if (high_distance_string)
	{
		if (sscanf(high_distance_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[0][0] = clamp(temp[0], 0, 100000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"high_distance\" in entity \"env_shadow_control\"\n");
		}
	}

	char *high_scale_string = ValueForKey(ent, "high_scale");
	if (high_scale_string)
	{
		if (sscanf(high_scale_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[0][1] = clamp(temp[0], 0.1, 8);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"high_scale\" in entity \"env_shadow_control\"\n");
		}
	}

	char *medium_distance_string = ValueForKey(ent, "medium_distance");
	if (medium_distance_string)
	{
		if (sscanf(medium_distance_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[1][0] = clamp(temp[0], 0, 100000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"medium_distance\" in entity \"env_shadow_control\"\n");
		}
	}

	char *medium_scale_string = ValueForKey(ent, "medium_scale");
	if (medium_scale_string)
	{
		if (sscanf(medium_scale_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[1][1] = clamp(temp[0], 0.1, 8);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"medium_scale\" in entity \"env_shadow_control\"\n");
		}
	}

	char *low_distance_string = ValueForKey(ent, "low_distance");
	if (low_distance_string)
	{
		if (sscanf(low_distance_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[2][0] = clamp(temp[0], 0, 100000);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"low_distance\" in entity \"env_shadow_control\"\n");
		}
	}

	char *low_scale_string = ValueForKey(ent, "low_scale");
	if (low_scale_string)
	{
		if (sscanf(low_scale_string, "%f", &temp[0]) == 1)
		{
			r_shadow_control.quality[2][1] = clamp(temp[0], 0.1, 8);
		}
		else
		{
			gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse \"low_scale\" in entity \"env_shadow_control\"\n");
		}
	}
}

void R_LoadBSPEntities(void)
{
	//Initialize shadow control, enabled by default

	r_shadow_control.enabled = true;

	float temp[4];
	if (sscanf(r_shadow_color->string, "%f %f %f %f", &temp[0], &temp[1], &temp[2], &temp[3]) == 4)
	{
		r_shadow_control.color[0] = clamp(temp[0], 0, 255) / 255.0f;
		r_shadow_control.color[1] = clamp(temp[1], 0, 255) / 255.0f;
		r_shadow_control.color[2] = clamp(temp[2], 0, 255) / 255.0f;
		r_shadow_control.color[3] = clamp(temp[3], 0, 255) / 255.0f;
	}
	else
	{
		r_shadow_control.color[0] = 0;
		r_shadow_control.color[1] = 0;
		r_shadow_control.color[2] = 0;
		r_shadow_control.color[3] = 0.5f;

		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse r_shadow_color\n");
	}

	if (sscanf(r_shadow_angles->string, "%f %f %f", &temp[0], &temp[1], &temp[2]) == 3)
	{
		r_shadow_control.angles[0] = clamp(temp[0], -360, 360);
		r_shadow_control.angles[1] = clamp(temp[1], -360, 360);
		r_shadow_control.angles[2] = clamp(temp[2], -360, 360);
	}
	else
	{
		r_shadow_control.angles[0] = 90;
		r_shadow_control.angles[1] = 0;
		r_shadow_control.angles[2] = 0;

		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse r_shadow_angles\n");
	}

	if (sscanf(r_shadow_fade->string, "%f %f", &temp[0], &temp[1]) == 2)
	{
		r_shadow_control.fade[0] = clamp(temp[0], 0, 10000);
		r_shadow_control.fade[1] = clamp(temp[1], r_shadow_control.fade[0], 10000) - r_shadow_control.fade[0];
	}
	else
	{
		r_shadow_control.fade[0] = 64;
		r_shadow_control.fade[1] = 128 - 64;

		gEngfuncs.Con_Printf("R_LoadBSPEntities: Failed to parse r_shadow_fade\n");
	}

	r_shadow_control.minlum = clamp(r_shadow_minlum->value, 0, 255) / 255.0f;

	r_shadow_control.quality[0][0] = clamp(r_shadow_high_distance->value, 0, 100000);
	r_shadow_control.quality[0][1] = clamp(r_shadow_high_scale->value, 0.1, 8);

	r_shadow_control.quality[1][0] = clamp(r_shadow_medium_distance->value, 0, 100000);
	r_shadow_control.quality[1][1] = clamp(r_shadow_medium_scale->value, 0.1, 8);

	r_shadow_control.quality[2][0] = clamp(r_shadow_low_distance->value, 0, 100000);
	r_shadow_control.quality[2][1] = clamp(r_shadow_low_scale->value, 0.1, 8);

	//Initialize hdr control, enabled by default

	r_hdr_control.enabled = true;

	r_hdr_control.adaptation = clamp(r_hdr_adaptation->value, 0.1, 100);
	r_hdr_control.blurwidth = clamp(r_hdr_blurwidth->value, 0, 1);
	r_hdr_control.darkness = clamp(r_hdr_darkness->value, 0.01, 10);
	r_hdr_control.exposure = clamp(r_hdr_exposure->value, 0.01, 10);

	//Initialize water control

	for(int i = 0; i < r_wsurf.iNumBSPEntities; i++)
	{
		bspentity_t *ent = &r_wsurf.pBSPEntities[i];

		char *classname = ent->classname;

		if(!classname)
			continue;
#if 0
		if (!strcmp(classname, "env_cubemap"))
		{
			R_ParseBSPEntity_Env_Cubemap(ent);
		}
#endif	
		else if (!strcmp(classname, "light_dynamic"))
		{
			R_ParseBSPEntity_Light_Dynamic(ent);
		}

		else if (!strcmp(classname, "env_water_control"))
		{
			R_ParseBSPEntity_Env_Water_Control(ent);
		}

		else if (!strcmp(classname, "env_hdr_control"))
		{
			R_ParseBSPEntity_Env_HDR_Control(ent);
		}

		else if (!strcmp(classname, "env_shadow_control"))
		{
			R_ParseBSPEntity_Env_Shadow_Control(ent);			
		}
	}//end for

	gEngfuncs.pfnAngleVectors(r_shadow_control.angles, r_shadow_control.vforward, r_shadow_control.vright, r_shadow_control.vup);
}

void R_DrawSequentialPolyVBO(msurface_t *s)
{
	R_RenderDynamicLightmaps(s);

	auto lightmapnum = s->lightmaptexturenum;

	if (lightmap_modified[lightmapnum])
	{
		lightmap_modified[lightmapnum] = 0;

		glRect_t *theRect = (glRect_t *)((char *)lightmap_rectchange + sizeof(glRect_t) * lightmapnum);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, r_wsurf.iLightmapTextureArray);
		qglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, theRect->t, lightmapnum, BLOCK_WIDTH, theRect->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, lightmaps + (lightmapnum * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * LIGHTMAP_BYTES);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		theRect->l = BLOCK_WIDTH;
		theRect->t = BLOCK_HEIGHT;
		theRect->h = 0;
		theRect->w = 0;
	}

	if (s->pdecals)
	{
		gDecalSurfs[(*gDecalSurfCount)] = s;
		(*gDecalSurfCount)++;

		if ((*gDecalSurfCount) > MAX_DECALSURFS)
			Sys_ErrorEx("Too many decal surfaces!\n");

		if (r_wsurf.bDiffuseTexture)
		{
			R_DrawDecals(r_wsurf.bLightmapTexture ? true : false);
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

void R_DrawBrushModel(cl_entity_t *e)
{
	int i;
	int k;
	vec3_t mins, maxs;
	float dot;
	mplane_t *pplane;
	model_t *clmodel;
	qboolean rotated;

	(*currententity) = e;
	(*currenttexture) = -1;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;

		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(e->origin, clmodel->mins, mins);
		VectorAdd(e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;

	qglColor3f(1, 1, 1);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 1024);
	}
	else
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 64);
	}

	VectorSubtract(r_refdef->vieworg, e->origin, modelorg);

	if (rotated)
	{
		vec3_t temp;
		vec3_t forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	if (!r_light_dynamic->value)
	{
		if (clmodel->firstmodelsurface != 0)
		{
			int max_dlights;

			if (g_iEngineType == ENGINE_SVENGINE)
			{
				max_dlights = 256;
			}
			else
			{
				max_dlights = 32;
			}

			for (k = 0; k < max_dlights; k++)
			{
				vec3_t saveOrigin;

				if ((cl_dlights[k].die < (*cl_time)) || (!cl_dlights[k].radius))
					continue;

				VectorCopy(cl_dlights[k].origin, saveOrigin);
				VectorSubtract(cl_dlights[k].origin, e->origin, cl_dlights[k].origin);

				//R_MarkLights(&cl_dlights[k], 1 << k, clmodel->nodes + clmodel->hulls[0].firstclipnode);
				VectorCopy(saveOrigin, cl_dlights[k].origin);
			}
		}
	}

	qglPushMatrix();
	R_RotateForEntity(e->origin, e);

	R_SetRenderMode(e);
	R_SetGBufferMask(GBUFFER_MASK_ALL);

	qglEnable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);
	qglStencilFunc(GL_ALWAYS, 0, 0xFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	if ((*currententity)->curstate.rendermode == kRenderTransColor)
	{
		r_wsurf.bDiffuseTexture = false;
		r_wsurf.bLightmapTexture = false;
	}
	else if ((*currententity)->curstate.rendermode == kRenderTransAlpha || (*currententity)->curstate.rendermode == kRenderNormal)
	{
		r_wsurf.bDiffuseTexture = true;
		r_wsurf.bLightmapTexture = true;
	}
	else
	{
		r_wsurf.bDiffuseTexture = true;
		r_wsurf.bLightmapTexture = false;
	}

	r_wsurf.bShadowmapTexture = false;

	if(R_ShouldRenderShadowScene(2))
		r_wsurf.bShadowmapTexture = true;

	if (r_wsurf_vbo->value)
	{
		auto modcache = R_PrepareWSurfVBO(clmodel);

		R_EnableWSurfVBO(modcache);

		R_DrawWSurfVBO(modcache);

		R_EnableWSurfVBO(NULL);

		auto psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
		for (i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				if (pplane->type != PLANE_Z)
					continue;

				if (mins[2] + 1.0 >= pplane->dist)
					continue;
			}

			dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

			if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
			{
				//fuck water
				if (psurf->flags & SURF_DRAWTURB)
				{
					R_SetRenderMode(e);
					R_DrawSequentialPoly(psurf, 0);
				}
				else
				{
					R_DrawSequentialPolyVBO(psurf);
				}
			}
			else
			{
				if (psurf->flags & SURF_DRAWTURB)
				{
					R_SetRenderMode(e);
					R_DrawSequentialPoly(psurf, 1);
				}
			}
		}

	}
	else
	{
		auto psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
		for (i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				if (pplane->type != PLANE_Z)
					continue;

				if (mins[2] + 1.0 >= pplane->dist)
					continue;
			}

			dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

			if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
			{
				R_SetRenderMode(e);
				R_DrawSequentialPoly(psurf, 0);
			}
			else
			{
				if (psurf->flags & SURF_DRAWTURB)
				{
					R_SetRenderMode(e);
					R_DrawSequentialPoly(psurf, 1);
				}
			}
		}
	}

	if ((*currententity)->curstate.rendermode != kRenderNormal)
	{
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		qglDisable(GL_BLEND);
	}

	qglPopMatrix();
	qglDepthMask(1);
	qglDisable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_NOTEQUAL, 0);
	qglDisable(GL_BLEND);

	qglStencilMask(0);
	qglEnable(GL_STENCIL_TEST);

	r_rotate_entity = false;
}

void R_DrawWorld(void)
{
	InvertMatrix(r_world_matrix, r_world_matrix_inv);
#if 0
	r_yfov = CalcFov((*r_xfov), r_refdef_vrect->width, r_refdef_vrect->height);

	float zNear = 4;
	float zFar = r_params.movevars->zmax;
	r_screenaspect = (float)r_refdef_vrect->width / r_refdef_vrect->height;

	float ymax = zNear * tan(r_yfov * M_PI / 360.0);
	float xmax = ymax * r_screenaspect;

	float yfar = ymax * zFar / zNear;
	float xfar = xmax * zFar / zNear;

	vec3_t farplane;
	VectorMA(r_refdef->vieworg, r_params.movevars->zmax, vpn, farplane);
	
	VectorMA(farplane, -xfar, vright, r_frustum_origin[0]);
	VectorMA(r_frustum_origin[0], -yfar, vup, r_frustum_origin[0]);
	VectorSubtract(r_frustum_origin[0], r_refdef->vieworg, r_frustum_vec[0]);

	VectorMA(farplane, -xfar, vright, r_frustum_origin[1]);
	VectorMA(r_frustum_origin[1], yfar, vup, r_frustum_origin[1]);
	VectorSubtract(r_frustum_origin[1], r_refdef->vieworg, r_frustum_vec[1]);

	VectorMA(farplane, xfar, vright, r_frustum_origin[2]);
	VectorMA(r_frustum_origin[2], yfar, vup, r_frustum_origin[2]);
	VectorSubtract(r_frustum_origin[2], r_refdef->vieworg, r_frustum_vec[2]);

	VectorMA(farplane, xfar, vright, r_frustum_origin[3]);
	VectorMA(r_frustum_origin[3], -yfar, vup, r_frustum_origin[3]);
	VectorSubtract(r_frustum_origin[3], r_refdef->vieworg, r_frustum_vec[3]);
#endif

	r_draw_nontransparent = true;

	R_BeginRenderGBuffer();

	VectorCopy(r_refdef->vieworg, modelorg);

	cl_entity_t tempent = { 0 };
	tempent.model = r_worldmodel;
	tempent.curstate.rendercolor.r = cshift_water->destcolor[0];
	tempent.curstate.rendercolor.g = cshift_water->destcolor[1];
	tempent.curstate.rendercolor.b = cshift_water->destcolor[2];

	(*currententity) = &tempent;
	*currenttexture = -1;

	qglColor3f(1.0f, 1.0f, 1.0f);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 1024);
	}
	else
	{
		memset(lightmap_polys, 0, sizeof(glpoly_t *) * 64);
	}

	GL_DisableMultitexture();
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//Capture previous fog settings from R_RenderScene
	if (qglIsEnabled(GL_FOG))
	{
		qglGetIntegerv(GL_FOG_MODE, &r_fog_mode);

		if (r_fog_mode == GL_LINEAR)
		{
			qglGetFloatv(GL_FOG_START, &r_fog_control[0]);
			qglGetFloatv(GL_FOG_END, &r_fog_control[1]);
			qglGetFloatv(GL_FOG_COLOR, r_fog_color);
		}
	}

	//Skybox uses stencil = 1

	qglEnable(GL_STENCIL_TEST);
	qglStencilMask(0xFF);
	qglStencilFunc(GL_ALWAYS, 1, 0xFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	r_wsurf.bDiffuseTexture = true;
	r_wsurf.bLightmapTexture = false;

	R_DrawSkyBox();

	//World uses stencil = 0

	qglStencilFunc(GL_ALWAYS, 0, 0xFF);

	r_wsurf.bDiffuseTexture = true;
	r_wsurf.bLightmapTexture = true;

	r_wsurf.bShadowmapTexture = false;

	if (R_ShouldRenderShadowScene(1))
	{
		r_wsurf.bShadowmapTexture = true;

		const float bias[16] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f };

		qglMatrixMode(GL_TEXTURE);
		qglPushMatrix();
		for (int i = 0; i < 3; ++i)
		{
			if (shadow_numvisedicts[i] > 0)
			{
				qglLoadIdentity();
				qglLoadMatrixf(bias);
				qglMultMatrixf(shadow_projmatrix[i]);
				qglMultMatrixf(shadow_mvmatrix[i]);
				qglMultMatrixf(r_world_matrix_inv);
				qglGetFloatv(GL_TEXTURE_MATRIX, r_shadow_matrix[i]);
			}
		}
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
	}

	if (r_wsurf_vbo->value)
	{
		auto modcache = R_PrepareWSurfVBO(r_worldmodel);

		R_EnableWSurfVBO(modcache);

		R_DrawWSurfVBO(modcache);

		R_EnableWSurfVBO(NULL);
	}
	else
	{
		(*gDecalSurfCount) = 0;
		R_RecursiveWorldNode(r_worldmodel->nodes);
		(*gDecalSurfCount) = 0;
	}

	(*currententity) = gEngfuncs.GetEntityByIndex(0);

	GL_DisableMultitexture();

	(*skychain) = 0;

	if ((*waterchain))
	{
		for (auto s = (*waterchain); s; s = s->texturechain)
		{
			qglColor4ub(255, 255, 255, 255);
			GL_Bind(s->texinfo->texture->gl_texturenum);

			//Water uses stencil = 1
			EmitWaterPolys(s, 0);
		}
		(*waterchain) = 0;
	}

	//No stencil write later
	qglStencilMask(0);
	qglDisable(GL_STENCIL_TEST);
}