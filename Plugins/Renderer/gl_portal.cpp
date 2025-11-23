#include "gl_local.h"
#include <sstream>

void* g_pCurrentClientPortal = nullptr;
void* g_pClientPortalManager = nullptr;

std::unordered_map<program_state_t, portal_program_t> g_PortalProgramTable;

std::unordered_map<CWorldPortalModelHash, std::shared_ptr<CWorldPortalModel>, CWorldPortalModelHasher> g_PortalSurfaceModels;

std::unordered_map<CPortalTextureCacheHash, std::shared_ptr<CPortalTextureCache>, CPortalTextureCacheHasher> g_PortalTextureCaches;

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

		if (state & PORTAL_REVERSE_TEXCOORD_ENABLED)
			defs << "#define PORTAL_REVERSE_TEXCOORD_ENABLED\n";

		if (state & PORTAL_GAMMA_BLEND_ENABLED)
			defs << "#define GAMMA_BLEND_ENABLED\n";

		auto def = defs.str();

		CCompileShaderArgs args;
		args.vsfile = "renderer\\shader\\portal_shader.vert.glsl";
		args.fsfile = "renderer\\shader\\portal_shader.frag.glsl";
		args.vsdefine = def.c_str();
		args.fsdefine = def.c_str();

		prog.program = GL_CompileShaderFileEx(&args);

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
{ PORTAL_OVERLAY_TEXTURE_ENABLED			, "PORTAL_OVERLAY_TEXTURE_ENABLED"	 },
{ PORTAL_TEXCOORD_ENABLED					, "PORTAL_TEXCOORD_ENABLED"			 },
{ PORTAL_REVERSE_TEXCOORD_ENABLED			, "PORTAL_REVERSE_TEXCOORD_ENABLED"	 },
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

int ClientPortal_GetIndex(void* pClientPortal)
{
	auto vectorBase = *(void***)((ULONG_PTR)g_pClientPortalManager + 140);
	auto vectorEnd = *(void***)((ULONG_PTR)g_pClientPortalManager + 140);
	
	int index = 0;

	for (auto vectorItor = vectorBase; vectorItor < vectorEnd; ++vectorItor, index++)
	{
		auto clientPortal = (*vectorItor);

		if (clientPortal == pClientPortal)
			return index;
	}

	return -1;
}

bool ClientPortal_GetPortalTransform(void* pClientPortal, float *outOrigin, float *outAngles)
{
	if (g_dwEngineBuildnum >= 10000)//5.26
	{
		const auto origin = (const float*)((ULONG_PTR)pClientPortal + 0);
		const auto angles = (const float*)((ULONG_PTR)pClientPortal + 12);

		VectorCopy(origin, outOrigin);
		VectorCopy(angles, outAngles);
		return true;
	}
	if (g_dwEngineBuildnum >= 8948)//5.25
	{
		auto ent = *(cl_entity_t**)((ULONG_PTR)pClientPortal + 0x70);
		VectorCopy(ent->origin, outOrigin);
		VectorCopy(ent->angles, outAngles);
		return true;
	}

	return false;
}

int ClientPortal_GetPortalMode(void * pClientPortal)
{
	if (g_dwEngineBuildnum >= 10000 )//5.26
	{
		return *(int*)((ULONG_PTR)pClientPortal + 0x40);
	}
	if (g_dwEngineBuildnum >= 8948)//5.25
	{
		return *(int*)((ULONG_PTR)pClientPortal + 0x28);
	}

	return -1;
}

int ClientPortal_GetTextureId(void* pClientPortal)
{
	if (g_dwEngineBuildnum >= 10000)//5.26
	{
		return *(int*)((ULONG_PTR)pClientPortal + 204);
	}
	return -1;
}

int ClientPortal_GetTextureWidth(void* pClientPortal)
{
	if (g_dwEngineBuildnum >= 10000)//5.26
	{
		return *(int*)((ULONG_PTR)pClientPortal + 208);
	}
	return -1;
}

int ClientPortal_GetTextureHeight(void* pClientPortal)
{
	if (g_dwEngineBuildnum >= 10000)//5.26
	{
		return *(int*)((ULONG_PTR)pClientPortal + 212);
	}
	return -1;
}

void __fastcall ClientPortalManager_ResetAll(void * pthis, int)
{
#if 0
	portal_texture_t *ptextures = *(portal_texture_t **)((ULONG_PTR)pthis + 0x9C);

	if (ptextures->next != ptextures)
	{
		do
		{
			ptextures->gl_texturenum2 = 0;
			ptextures = ptextures->next;
		} while (ptextures != *(portal_texture_t **)((ULONG_PTR)pthis + 0x9C));
	}
#endif

	gPrivateFuncs.ClientPortalManager_ResetAll(pthis, 0);

	g_PortalTextureCaches.clear();
}

mtexinfo_t * __fastcall ClientPortalManager_GetOriginalSurfaceTexture(void * pthis, int dummy, msurface_t *surf)
{
	return gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture(pthis, dummy, surf);
}

std::shared_ptr<CWorldPortalModel> R_FindPortalSurfaceModel(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId)
{
	CWorldPortalModelHash hash(ClientPortal, surf->texinfo->texture->name[0] == '{' ? surf->texinfo->texture->gl_texturenum : 0, textureId);
	auto itor = g_PortalSurfaceModels.find(hash);
	if (itor == g_PortalSurfaceModels.end())
	{
		return NULL;
	}

	return itor->second;
}

std::shared_ptr<CWorldPortalModel> R_GetPortalSurfaceModel(void *ClientPortalManager, void * ClientPortal, msurface_t *surf, GLuint textureId)
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
		pPortalModel = std::make_shared<CWorldPortalModel>();

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

		CWorldPortalModelHash hash(ClientPortal, surf->texinfo->texture->name[0] == '{' ? surf->texinfo->texture->gl_texturenum : 0, textureId);

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
	program_state_t PortalProgramState = (ClientPortal_GetPortalMode(ClientPortal) == 0) ? PORTAL_REVERSE_TEXCOORD_ENABLED  : PORTAL_TEXCOORD_ENABLED;

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

	GL_BeginDebugGroup("R_DrawPortal");

	R_RotateForTransform(origin, angles, r_entity_matrix);

	R_DrawPortalSurfaceModelBegin(pPortalModel);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -gl_polyoffset->value);

	portal_program_t prog = { 0 };

	R_UsePortalProgram(PortalProgramState, &prog);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, textureId);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, pPortalModel->texinfo->texture->gl_texturenum);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void *)(0), pPortalModel->drawCount, 0);

	(*c_brush_polys) += pPortalModel->polyCount;

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_UseProgram(0);

	glDisable(GL_POLYGON_OFFSET_FILL);

	R_DrawPortalSurfaceModelEnd();

	GL_EndDebugGroup();
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

	GL_BeginDebugGroup("R_DrawMonitor");

	R_RotateForTransform(origin, angles, r_entity_matrix);

	R_DrawPortalSurfaceModelBegin(pPortalModel);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -gl_polyoffset->value);

	portal_program_t prog = { 0 };
	R_UsePortalProgram(PortalProgramState, &prog);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, textureId);

	GL_BindTextureUnit(1, GL_TEXTURE_2D, pPortalModel->texinfo->texture->gl_texturenum);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(0), pPortalModel->drawCount, 0);

	(*c_brush_polys) += pPortalModel->polyCount;

	GL_BindTextureUnit(1, GL_TEXTURE_2D, 0);

	GL_BindTextureUnit(0, GL_TEXTURE_2D, 0);

	GL_UseProgram(0);

	glDisable(GL_POLYGON_OFFSET_FILL);

	R_DrawPortalSurfaceModelEnd();

	GL_EndDebugGroup();
}

void ClientPortalManager_AngleVectors(const float* a1, float *a2, float* a3, float* a4)
{
	g_pCurrentClientPortal = (void*)((ULONG_PTR)a1 - 12);

	AngleVectors(a1, a2, a3, a4);
}

void __fastcall ClientPortalManager_RenderPortals(void* pthis, int dummy)
{
	g_pClientPortalManager = pthis;
	gPrivateFuncs.ClientPortalManager_RenderPortals(pthis, 0);
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
	GL_BeginDebugGroup("ClientPortalManager_DrawPortalSurface");

	auto pPortalModel = R_GetPortalSurfaceModel(ClientPortalManager, ClientPortal, surf, textureId);

	if (pPortalModel)
	{
		auto mode = ClientPortal_GetPortalMode(ClientPortal);

		if (mode != -1)
		{
			if (mode == 1)
			{
				R_DrawMonitor(ClientPortalManager, ClientPortal, surf, textureId, pPortalModel.get());
			}
			else
			{
				R_DrawPortal(ClientPortalManager, ClientPortal, surf, textureId, pPortalModel.get());
			}
		}
	}

	GL_EndDebugGroup();
}

std::shared_ptr<CPortalTextureCache> R_GetTextureCacheForPortalTexture(void *pClientPortal, int width, int height)
{
	CPortalTextureCacheHash hash(pClientPortal, width, height);

	auto it = g_PortalTextureCaches.find(hash);

	if (it != g_PortalTextureCaches.end())
		return it->second;

	auto pTextureCache = std::make_shared<CPortalTextureCache>();
	//pTextureCache->color = GL_GenTextureColorFormat(width, height, GL_RGBA8, true, nullptr, true);
	pTextureCache->depth_stencil = GL_GenDepthStencilTexture(width, height, true);
	pTextureCache->width = width;
	pTextureCache->height = height;

	g_PortalTextureCaches[hash] = pTextureCache;

	return pTextureCache;
}