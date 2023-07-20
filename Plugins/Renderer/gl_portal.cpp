#include "gl_local.h"
#include <sstream>

//int g_LastPortalTextureId = 0;

std::unordered_map<program_state_t, portal_program_t> g_PortalProgramTable;

std::unordered_map<portal_vbo_hash_t, portal_vbo_t *, portal_vbo_hasher> g_PortalVBOCache;

void R_UsePortalProgram(program_state_t state, portal_program_t *progOutput)
{
	portal_program_t prog = { 0 };

	auto itor = g_PortalProgramTable.find(state);
	if (itor == g_PortalProgramTable.end())
	{
		std::stringstream defs;

		if (state & OVERLAY_TEXTURE_ENABLED)
			defs << "#define OVERLAY_TEXTURE_ENABLED\n";

		if (state & PORTAL_TEXCOORD_ENABLED)
			defs << "#define PORTAL_TEXCOORD_ENABLED\n";

		if (state & REVERSE_PORTAL_TEXCOORD_ENABLED)
			defs << "#define REVERSE_PORTAL_TEXCOORD_ENABLED\n";

		auto def = defs.str();

		prog.program = R_CompileShaderFileEx("renderer\\shader\\portal_shader.vsh", "renderer\\shader\\portal_shader.fsh", def.c_str(), def.c_str(), NULL);
		if (prog.program)
		{
			SHADER_UNIFORM(prog, u_entityMatrix, "u_entityMatrix");
		}

		g_PortalProgramTable[state] = prog;
	}
	else
	{
		prog = itor->second;
	}

	if (prog.program)
	{
		GL_UseProgram(prog.program);

		if (prog.u_entityMatrix)
		{
			glUniformMatrix4fv(prog.u_entityMatrix, 1, false, (const GLfloat *)r_entity_matrix);
		}

		if (progOutput)
			*progOutput = prog;
	}
	else
	{
		g_pMetaHookAPI->SysError("R_UsePortalProgram: Failed to load program!");
	}
}

const program_state_mapping_t s_PortalProgramStateName[] = {
{ OVERLAY_TEXTURE_ENABLED					, "OVERLAY_TEXTURE_ENABLED"			 },
{ PORTAL_TEXCOORD_ENABLED					, "PORTAL_TEXCOORD_ENABLED"			 },
{ REVERSE_PORTAL_TEXCOORD_ENABLED			, "REVERSE_PORTAL_TEXCOORD_ENABLED"	 },
};

void R_SavePortalProgramStates(void)
{
	std::vector<program_state_t> states;
	for (auto &p : g_PortalProgramTable)
	{
		states.emplace_back(p.first);
	}
	R_SaveProgramStatesCaches("renderer/shader/portal_cache.txt", states, s_PortalProgramStateName, _ARRAYSIZE(s_PortalProgramStateName));
}

void R_LoadPortalProgramStates(void)
{
	R_LoadProgramStateCaches("renderer/shader/portal_cache.txt", s_PortalProgramStateName, _ARRAYSIZE(s_PortalProgramStateName), [](program_state_t state) {

		R_UsePortalProgram(state, NULL);

	});
}

void R_NewMapPortal(void)
{
	for (auto &itor : g_PortalVBOCache)
	{
		auto VBOCache = itor.second;

		if (VBOCache->hVAO)
		{
			GL_DeleteVAO(VBOCache->hVAO);
			VBOCache->hVAO = 0;
		}

		if (VBOCache->hEBO)
		{
			GL_DeleteBuffer(VBOCache->hEBO);
			VBOCache->hEBO = 0;
		}

		delete VBOCache;
	}

	g_PortalVBOCache.clear();
}

void R_ShutdownPortal(void)
{
	g_PortalProgramTable.clear();
}

void R_InitPortal(void)
{
	
}

int ClientPortal_GetPortalMode(void *ClientPortal)
{
	return *(int *)((ULONG_PTR)ClientPortal + 0x28);
}

void __fastcall ClientPortalManager_ResetAll(void * pthis, int)
{
	portal_texture_t *ptextures = *(portal_texture_t **)((ULONG_PTR)pthis + 0x9C);

	if (ptextures->next != ptextures)
	{
		do
		{
			//glDeleteTextures(1, &ptextures->gl_texturenum2);
			ptextures->gl_texturenum2 = 0;
			ptextures = ptextures->next;
		} while (ptextures != *(portal_texture_t **)((ULONG_PTR)pthis + 0x9C));
	}

	gRefFuncs.ClientPortalManager_ResetAll(pthis, 0);
}

mtexinfo_t * __fastcall ClientPortalManager_GetOriginalSurfaceTexture(void * pthis, int dummy, msurface_t *surf)
{
	return gRefFuncs.ClientPortalManager_GetOriginalSurfaceTexture(pthis, dummy, surf);
}

portal_vbo_t *R_FindPortalVBO(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId)
{
	auto poly = surf->polys;

	//auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

	portal_vbo_hash_t hash(ClientPortal, poly->flags, textureId);
	auto itor = g_PortalVBOCache.find(hash);
	if (itor == g_PortalVBOCache.end())
	{
		return NULL;
	}

	return itor->second;
}

portal_vbo_t *R_PreparePortalVBO(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId)
{
	portal_vbo_t *VBOCache = R_FindPortalVBO(ClientPortalManager, ClientPortal, surf, textureId);

	if (!VBOCache)
	{
		VBOCache = new portal_vbo_t;
		VBOCache->texinfo = ClientPortalManager_GetOriginalSurfaceTexture(ClientPortalManager, 0, surf);
		
		auto poly = surf->polys;

		auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

		VBOCache->PolySet.emplace(poly->flags);

		for (int j = 0; j < brushface->num_polys; ++j)
		{
			for (int k = 0; k < brushface->num_vertexes[j]; ++k)
			{
				VBOCache->vIndicesBuffer.emplace_back(brushface->start_vertex[j] + k);
			}
			VBOCache->vIndicesBuffer.emplace_back((GLuint)0xFFFFFFFF);
		}

		VBOCache->hEBO = GL_GenBuffer();

		GL_UploadDataToEBO(VBOCache->hEBO, sizeof(GLuint) * VBOCache->vIndicesBuffer.size(), VBOCache->vIndicesBuffer.data());

		VBOCache->hVAO = GL_GenVAO();

		GL_BindStatesForVAO(VBOCache->hVAO, r_wsurf.hSceneVBO, VBOCache->hEBO,
		[]() {
			glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_POSITION);
			glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMAL);
			glEnableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
			glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_POSITION, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
			glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_NORMAL, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, normal));
			glVertexAttribPointer(VERTEX_ATTRIBUTE_INDEX_TEXCOORD, 3, GL_FLOAT, false, sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
		},
		[]() {
			glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_POSITION);
			glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_NORMAL);
			glDisableVertexAttribArray(VERTEX_ATTRIBUTE_INDEX_TEXCOORD);
		});

		VBOCache->iPolyCount += brushface->num_polys;

		portal_vbo_hash_t hash(ClientPortal, surf->texinfo->texture->name[0] == '{' ? surf->texinfo->texture->gl_texturenum : 0, textureId);

		g_PortalVBOCache[hash] = VBOCache;
	}
	else
	{
		auto poly = surf->polys;

		auto brushface = &r_wsurf.vFaceBuffer[poly->flags];

		auto polyItor = VBOCache->PolySet.find(poly->flags);
		if (polyItor == VBOCache->PolySet.end())
		{
			VBOCache->PolySet.emplace(poly->flags);

			for (int j = 0; j < brushface->num_polys; ++j)
			{
				for (int k = 0; k < brushface->num_vertexes[j]; ++k)
				{
					VBOCache->vIndicesBuffer.emplace_back(brushface->start_vertex[j] + k);
				}
				VBOCache->vIndicesBuffer.emplace_back((GLuint)0xFFFFFFFF);
			}

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOCache->hEBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * VBOCache->vIndicesBuffer.size(), VBOCache->vIndicesBuffer.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			VBOCache->iPolyCount += brushface->num_polys;
		}
	}

	return VBOCache;
}

void R_DrawPortal(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId, portal_vbo_t *VBOCache)
{
	auto ent = (cl_entity_t *)*(DWORD *)((ULONG_PTR)ClientPortal + 0x70);

	program_state_t programState = (ClientPortal_GetPortalMode(ClientPortal) == 0) ? REVERSE_PORTAL_TEXCOORD_ENABLED  : PORTAL_TEXCOORD_ENABLED;

	if (VBOCache->texinfo->texture->name[0] == '{')
	{
		programState |= OVERLAY_TEXTURE_ENABLED;
	}

	R_RotateForEntity(ent);

	portal_program_t prog = { 0 };
	R_UsePortalProgram(programState, &prog);

	GL_Bind(textureId);

	GL_EnableMultitexture();
	GL_Bind(VBOCache->texinfo->texture->gl_texturenum);

	glDrawElements(GL_POLYGON, VBOCache->vIndicesBuffer.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	GL_DisableMultitexture();

	GL_UseProgram(0);

	r_wsurf_drawcall++;
	r_wsurf_polys += VBOCache->iPolyCount;
}

void R_DrawMonitor(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId, portal_vbo_t *VBOCache)
{
	auto ent = (cl_entity_t *)*(DWORD *)((ULONG_PTR)ClientPortal + 0x70);

	program_state_t programState = 0;

	if (VBOCache->texinfo->texture->name[0] == '{')
	{
		programState |= OVERLAY_TEXTURE_ENABLED;
	}

	R_RotateForEntity(ent);

	portal_program_t prog = { 0 };
	R_UsePortalProgram(programState, &prog);

	GL_Bind(textureId);

	GL_EnableMultitexture();
	GL_Bind(VBOCache->texinfo->texture->gl_texturenum);

	glDrawElements(GL_POLYGON, VBOCache->vIndicesBuffer.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	GL_DisableMultitexture();

	GL_UseProgram(0);

	r_wsurf_drawcall++;
	r_wsurf_polys += VBOCache->iPolyCount;
}

void __fastcall ClientPortalManager_EnableClipPlane(void * pthis, int dummy, int index, vec3_t viewangles, vec3_t view, vec4_t plane)
{
	g_PortalClipPlane[index][0] = plane[0];
	g_PortalClipPlane[index][1] = plane[1];
	g_PortalClipPlane[index][2] = plane[2];
	g_PortalClipPlane[index][3] = plane[3];
	g_bPortalClipPlaneEnabled[index] = true;
}

void __fastcall ClientPortalManager_DrawPortalSurface(void *ClientPortalManager, int dummy, void *ClientPortal, msurface_t *surf, GLuint textureId)
{
	//gEngfuncs.Con_Printf("ClientPortalManager_DrawPortalSurface %d\n", textureId);

	//g_LastPortalTextureId = textureId;

	auto VBOCache = R_PreparePortalVBO(ClientPortalManager, ClientPortal, surf, textureId);

	GL_BindVAO(VBOCache->hVAO);

	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	if (ClientPortal_GetPortalMode(ClientPortal) == 1)
	{
		R_DrawMonitor(ClientPortalManager, ClientPortal, surf, textureId, VBOCache);
	}
	else
	{
		R_DrawPortal(ClientPortalManager, ClientPortal, surf, textureId, VBOCache);
	}

	glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	GL_BindVAO(0);
}