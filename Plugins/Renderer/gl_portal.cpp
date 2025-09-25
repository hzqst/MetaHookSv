#include "gl_local.h"
#include <sstream>

std::unordered_map<program_state_t, portal_program_t> g_PortalProgramTable;

std::unordered_map<portal_vbo_hash_t, CWorldPortalModel*, portal_vbo_hasher> g_PortalSurfaceModels;

CWorldPortalModel::~CWorldPortalModel()
{
	if (hABO)
	{
		GL_DeleteVAO(hABO);
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

		prog.program = R_CompileShaderFileEx("renderer\\shader\\portal_shader.vert.glsl", "renderer\\shader\\portal_shader.frag.glsl", def.c_str(), def.c_str(), NULL);
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
			glUniformMatrix4fv(prog.u_entityMatrix, 1, true, (const GLfloat *)r_entity_matrix);
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

void R_FreePortalResouces(void)
{
	for (auto& p : g_PortalSurfaceModels)
	{
		auto pPortalModel = p.second;

		delete pPortalModel;
	}

	g_PortalSurfaceModels.clear();
}

void R_ShutdownPortal(void)
{
	g_PortalProgramTable.clear();

	R_FreePortalResouces();
}

void R_InitPortal(void)
{
	
}

bool ClientPortal_GetPortalTransform(void* ClientPortal, float *outOrigin, float *outAngles)
{
	if (g_dwEngineBuildnum >= 10000)//5.26
	{
		const auto origin = (const float*)((ULONG_PTR)ClientPortal + 0);
		const auto angles = (const float*)((ULONG_PTR)ClientPortal + 12);

		VectorCopy(origin, outOrigin);
		VectorCopy(angles, outAngles);
		return true;
	}
	if (g_dwEngineBuildnum >= 8948)//5.25
	{
		auto ent = *(cl_entity_t**)((ULONG_PTR)ClientPortal + 0x70);
		VectorCopy(ent->origin, outOrigin);
		VectorCopy(ent->angles, outAngles);
		return true;
	}

	return false;
}

int ClientPortal_GetPortalMode(void *ClientPortal)
{
	if (g_dwEngineBuildnum >= 10000 )//5.26
	{
		return *(int*)((ULONG_PTR)ClientPortal + 0x40);
	}
	if (g_dwEngineBuildnum >= 8948)//5.25
	{
		return *(int*)((ULONG_PTR)ClientPortal + 0x28);
	}

	return -1;
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

	auto pBrushFace = &pWorldModel->m_vFaceBuffer[surfIndex];

	auto pPortalModel = R_FindPortalSurfaceModel(ClientPortalManager, ClientPortal, surf, textureId);

	if (!pPortalModel)
	{
		pPortalModel = new CWorldPortalModel;

		pPortalModel->texinfo = ClientPortalManager_GetOriginalSurfaceTexture(ClientPortalManager, 0, surf);
		
		pPortalModel->SurfaceSet.emplace(surfIndex);

		CDrawIndexAttrib drawAttrib;
		drawAttrib.FirstIndexLocation = pBrushFace->start_index;
		drawAttrib.NumIndices = pBrushFace->index_count;
		drawAttrib.FirstInstanceLocation = pBrushFace->instance_index;
		drawAttrib.NumInstances = pBrushFace->instance_count;

		pPortalModel->vDrawAttribBuffer.emplace_back(drawAttrib);

		pPortalModel->hABO = GL_GenBuffer();

		GL_UploadDataToABODynamicDraw(pPortalModel->hABO, sizeof(CDrawIndexAttrib) * pPortalModel->vDrawAttribBuffer.size(), pPortalModel->vDrawAttribBuffer.data());

		pPortalModel->drawCount = (uint32_t)pPortalModel->vDrawAttribBuffer.size();

		pPortalModel->polyCount += pBrushFace->poly_count;

		pPortalModel->m_pWorldModel = pWorldModel;

		portal_vbo_hash_t hash(ClientPortal, surf->texinfo->texture->name[0] == '{' ? surf->texinfo->texture->gl_texturenum : 0, textureId);

		g_PortalSurfaceModels[hash] = pPortalModel;
	}
	else
	{
		auto itor = pPortalModel->SurfaceSet.find(surfIndex);

		if (itor == pPortalModel->SurfaceSet.end())
		{
			pPortalModel->SurfaceSet.emplace(surfIndex);

			CDrawIndexAttrib drawAttrib;
			drawAttrib.FirstIndexLocation = pBrushFace->start_index;
			drawAttrib.NumIndices = pBrushFace->index_count;
			drawAttrib.FirstInstanceLocation = pBrushFace->instance_index;
			drawAttrib.NumInstances = pBrushFace->instance_count;

			pPortalModel->vDrawAttribBuffer.emplace_back(drawAttrib);

			GL_UploadDataToABODynamicDraw(pPortalModel->hABO, sizeof(CDrawIndexAttrib) * pPortalModel->vDrawAttribBuffer.size(), pPortalModel->vDrawAttribBuffer.data());

			pPortalModel->drawCount = (uint32_t)pPortalModel->vDrawAttribBuffer.size();

			pPortalModel->polyCount += pBrushFace->poly_count;
		}
	}

	return pPortalModel;
}

void R_DrawPortalSurfaceModelBegin(CWorldPortalModel* pPortalModel)
{
	auto pWorldModel = pPortalModel->m_pWorldModel.lock();

	GL_BindVAO(pWorldModel->hVAO);
	GL_BindABO(pPortalModel->hABO);
}

void R_DrawPortalSurfaceModelEnd()
{
	GL_BindABO(0);
	GL_BindVAO(0);
}

void R_DrawPortal(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId, CWorldPortalModel* pPortalModel)
{
	program_state_t PortalProgramState = (ClientPortal_GetPortalMode(ClientPortal) == 0) ? REVERSE_PORTAL_TEXCOORD_ENABLED  : PORTAL_TEXCOORD_ENABLED;

	if (pPortalModel->texinfo->texture->name[0] == '{')
	{
		PortalProgramState |= PORTAL_OVERLAY_TEXTURE_ENABLED;
	}

	if (R_IsRenderingGammaBlending())
	{
		PortalProgramState |= PORTAL_GAMMA_BLEND_ENABLED;
	}

	vec3_t origin{};
	vec3_t angles{};

	if (!ClientPortal_GetPortalTransform(ClientPortal, origin, angles))
		return;

	R_RotateForTransform(origin, angles);

	R_DrawPortalSurfaceModelBegin(pPortalModel);

	portal_program_t prog = { 0 };

	R_UsePortalProgram(PortalProgramState, &prog);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, textureId);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, pPortalModel->texinfo->texture->gl_texturenum);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void *)(0), pPortalModel->drawCount, 0);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_UseProgram(0);

	R_DrawPortalSurfaceModelEnd();
}

void R_DrawMonitor(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId, CWorldPortalModel* pPortalModel)
{
	program_state_t PortalProgramState = 0;

	if (pPortalModel->texinfo->texture->name[0] == '{')
	{
		PortalProgramState |= PORTAL_OVERLAY_TEXTURE_ENABLED;
	}

	if (R_IsRenderingGammaBlending())
	{
		PortalProgramState |= PORTAL_GAMMA_BLEND_ENABLED;
	}

	vec3_t origin{};
	vec3_t angles{};

	if (!ClientPortal_GetPortalTransform(ClientPortal, origin, angles))
		return;

	R_RotateForTransform(origin, angles);

	R_DrawPortalSurfaceModelBegin(pPortalModel);

	portal_program_t prog = { 0 };
	R_UsePortalProgram(PortalProgramState, &prog);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, textureId);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, pPortalModel->texinfo->texture->gl_texturenum);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(0), pPortalModel->drawCount, 0);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_UseProgram(0);

	R_DrawPortalSurfaceModelEnd();
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
		auto mode = ClientPortal_GetPortalMode(ClientPortal);

		if (mode != -1)
		{
			if (mode == 1)
			{
				R_DrawMonitor(ClientPortalManager, ClientPortal, surf, textureId, pPortalModel);
			}
			else
			{
				R_DrawPortal(ClientPortalManager, ClientPortal, surf, textureId, pPortalModel);
			}
		}
	}
}