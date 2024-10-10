#include "gl_local.h"
#include <sstream>

//int g_LastPortalTextureId = 0;

std::unordered_map<program_state_t, portal_program_t> g_PortalProgramTable;

std::unordered_map<portal_vbo_hash_t, CWorldPortalModel*, portal_vbo_hasher> g_PortalSurfaceModels;

CWorldPortalModel::~CWorldPortalModel()
{
	if (hVAO)
	{
		GL_DeleteVAO(hVAO);
	}

	if (hEBO)
	{
		GL_DeleteBuffer(hEBO);
	}
}

void R_UsePortalProgram(program_state_t state, portal_program_t *progOutput)
{
	portal_program_t prog = { 0 };

	auto itor = g_PortalProgramTable.find(state);
	if (itor == g_PortalProgramTable.end())
	{
		std::stringstream defs;

		if (state & PORTAL_OVERLAY_TEXTURE_ENABLED)
			defs << "#define PORTAL_OVERLAY_TEXTURE_ENABLED\n";

		if (state & PORTAL_TEXCOORD_ENABLED)
			defs << "#define PORTAL_TEXCOORD_ENABLED\n";

		if (state & REVERSE_PORTAL_TEXCOORD_ENABLED)
			defs << "#define REVERSE_PORTAL_TEXCOORD_ENABLED\n";

		if (state & PORTAL_GAMMA_BLEND_ENABLED)
			defs << "#define GAMMA_BLEND_ENABLED\n";

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
{ PORTAL_OVERLAY_TEXTURE_ENABLED					, "OVERLAY_TEXTURE_ENABLED"			 },
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

void R_NewMapPortal_Pre(void)
{
	for (auto& p : g_PortalSurfaceModels)
	{
		auto pPortalModel = p.second;

		delete pPortalModel;
	}

	g_PortalSurfaceModels.clear();
}

void R_NewMapPortal(void)
{

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

	gPrivateFuncs.ClientPortalManager_ResetAll(pthis, 0);
}

mtexinfo_t * __fastcall ClientPortalManager_GetOriginalSurfaceTexture(void * pthis, int dummy, msurface_t *surf)
{
	return gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture(pthis, dummy, surf);
}

CWorldPortalModel* R_FindPortalSurfaceModel(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId)
{
	portal_vbo_hash_t hash(ClientPortal, surf->texinfo->texture->name[0] == '{' ? surf->texinfo->texture->gl_texturenum : 0, textureId);
	auto itor = g_PortalSurfaceModels.find(hash);
	if (itor == g_PortalSurfaceModels.end())
	{
		return NULL;
	}

	return itor->second;
}

CWorldPortalModel* R_GetPortalSurfaceModel(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId)
{
	auto worldmodel = R_FindWorldModelBySurface(surf);

	if (!worldmodel)
	{
		Sys_Error("R_GetPortalSurfaceModel: Failed to find model by surface");
		return nullptr;
	}

	auto pWorldModel = R_GetWorldSurfaceWorldModel(worldmodel);

	if (!pWorldModel)
	{
		Sys_Error("R_GetPortalSurfaceModel: Failed to R_GetWorldSurfaceWorldModel!");
		return nullptr;
	}

	auto surfIndex = R_GetWorldSurfaceIndex(worldmodel, surf);

	if (surfIndex == -1)
	{
		Sys_Error("R_GetPortalSurfaceModel: invalid surfIndex!");
		return nullptr;
	}

	auto pBrushFace = &pWorldModel->vFaceBuffer[surfIndex];

	auto pPortalModel = R_FindPortalSurfaceModel(ClientPortalManager, ClientPortal, surf, textureId);

	if (!pPortalModel)
	{
		pPortalModel = new CWorldPortalModel;

		pPortalModel->texinfo = ClientPortalManager_GetOriginalSurfaceTexture(ClientPortalManager, 0, surf);
		
		pPortalModel->SurfaceSet.emplace(surfIndex);

		for (int j = 0; j < pBrushFace->num_polys; ++j)
		{
			for (int k = 0; k < pBrushFace->num_vertexes[j]; ++k)
			{
				pPortalModel->vIndicesBuffer.emplace_back(pBrushFace->start_vertex[j] + k);
			}
			pPortalModel->vIndicesBuffer.emplace_back((GLuint)0xFFFFFFFF);
		}

		pPortalModel->hEBO = GL_GenBuffer();

		GL_UploadDataToEBODynamicDraw(pPortalModel->hEBO, sizeof(GLuint) * pPortalModel->vIndicesBuffer.size(), pPortalModel->vIndicesBuffer.data());

		pPortalModel->hVAO = GL_GenVAO();

		GL_BindStatesForVAO(pPortalModel->hVAO, pWorldModel->hVBO, pPortalModel->hEBO,
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

		pPortalModel->iPolyCount += pBrushFace->num_polys;

		portal_vbo_hash_t hash(ClientPortal, surf->texinfo->texture->name[0] == '{' ? surf->texinfo->texture->gl_texturenum : 0, textureId);

		g_PortalSurfaceModels[hash] = pPortalModel;
	}
	else
	{
		auto itor = pPortalModel->SurfaceSet.find(surfIndex);

		if (itor == pPortalModel->SurfaceSet.end())
		{
			pPortalModel->SurfaceSet.emplace(surfIndex);

			for (int j = 0; j < pBrushFace->num_polys; ++j)
			{
				for (int k = 0; k < pBrushFace->num_vertexes[j]; ++k)
				{
					pPortalModel->vIndicesBuffer.emplace_back(pBrushFace->start_vertex[j] + k);
				}
				pPortalModel->vIndicesBuffer.emplace_back((GLuint)0xFFFFFFFF);
			}

			GL_UploadDataToEBODynamicDraw(pPortalModel->hEBO, sizeof(unsigned int) * pPortalModel->vIndicesBuffer.size(), pPortalModel->vIndicesBuffer.data());

			pPortalModel->iPolyCount += pBrushFace->num_polys;
		}
	}

	return pPortalModel;
}

void R_DrawPortal(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId, CWorldPortalModel* pPortalModel)
{
	auto ent = (cl_entity_t *)*(ULONG_PTR*)((ULONG_PTR)ClientPortal + 0x70);

	program_state_t programState = (ClientPortal_GetPortalMode(ClientPortal) == 0) ? REVERSE_PORTAL_TEXCOORD_ENABLED  : PORTAL_TEXCOORD_ENABLED;

	if (pPortalModel->texinfo->texture->name[0] == '{')
	{
		programState |= PORTAL_OVERLAY_TEXTURE_ENABLED;
	}

	if (r_draw_gammablend)
	{
		programState |= PORTAL_GAMMA_BLEND_ENABLED;
	}

	R_RotateForEntity(ent);

	portal_program_t prog = { 0 };

	R_UsePortalProgram(programState, &prog);

	GL_Bind(textureId);

	GL_EnableMultitexture();

	GL_Bind(pPortalModel->texinfo->texture->gl_texturenum);

	glDrawElements(GL_POLYGON, pPortalModel->vIndicesBuffer.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	GL_DisableMultitexture();

	GL_UseProgram(0);

	r_wsurf_drawcall++;
	r_wsurf_polys += pPortalModel->iPolyCount;
}

void R_DrawMonitor(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId, CWorldPortalModel* pPortalModel)
{
	auto ent = (cl_entity_t *)*(ULONG_PTR*)((ULONG_PTR)ClientPortal + 0x70);

	program_state_t programState = 0;

	if (pPortalModel->texinfo->texture->name[0] == '{')
	{
		programState |= PORTAL_OVERLAY_TEXTURE_ENABLED;
	}

	if (r_draw_gammablend)
	{
		programState |= PORTAL_GAMMA_BLEND_ENABLED;
	}

	R_RotateForEntity(ent);

	portal_program_t prog = { 0 };
	R_UsePortalProgram(programState, &prog);

	GL_Bind(textureId);

	GL_EnableMultitexture();

	GL_Bind(pPortalModel->texinfo->texture->gl_texturenum);

	glDrawElements(GL_POLYGON, pPortalModel->vIndicesBuffer.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	GL_DisableMultitexture();

	GL_UseProgram(0);

	r_wsurf_drawcall++;
	r_wsurf_polys += pPortalModel->iPolyCount;
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
	auto pPortalModel = R_GetPortalSurfaceModel(ClientPortalManager, ClientPortal, surf, textureId);

	if (pPortalModel)
	{
		GL_BindVAO(pPortalModel->hVAO);

		glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		if (ClientPortal_GetPortalMode(ClientPortal) == 1)
		{
			R_DrawMonitor(ClientPortalManager, ClientPortal, surf, textureId, pPortalModel);
		}
		else
		{
			R_DrawPortal(ClientPortalManager, ClientPortal, surf, textureId, pPortalModel);
		}

		glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

		GL_BindVAO(0);
	}
}