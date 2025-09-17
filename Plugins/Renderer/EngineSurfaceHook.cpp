#include <metahook.h>
#include <capstone.h>
#include "gl_local.h"
#include "privatehook.h"
#include <IEngineSurface.h>
#include "plugins.h"

#include <cstdlib>

IEngineSurface *enginesurface = nullptr;
IEngineSurface_HL25 * enginesurface_HL25 = nullptr;

static std::unordered_map<int, EngineSurfaceTexture*> g_VGuiSurfaceTextures;
static EngineSurfaceTexture* staticTextureCurrent{};

EngineSurfaceVertexBuffer_t(*g_VertexBuffer)[MAXVERTEXBUFFERS] = nullptr;

int(*g_iVertexBufferEntriesUsed) = 0;

RECT* g_ScissorRect = nullptr;
bool* g_bScissor = nullptr;

static void(__fastcall* m_pfnEngineSurface_pushMakeCurrent)(void* pthis, int, int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace) = NULL;
static void(__fastcall* m_pfnEngineSurface_popMakeCurrent)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawFilledRect)(void* pthis, int, int x0, int y0, int x1, int y1) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawOutlinedRect)(void* pthis, int, int x0, int y0, int x1, int y1) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawLine)(void* pthis, int, int x0, int y0, int x1, int y1) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawPolyLine)(void* pthis, int, int* px, int* py, int numPoints) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawSetTextureRGBA)(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, qboolean hasAlphaChannel) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawSetTexture)(void* pthis, int, int textureId) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawTexturedRect)(void* pthis, int, int x0, int y0, int x1, int y1) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawTexturedRectAdd)(void* pthis, int, int x0, int y0, int x1, int y1) = NULL;
static int(__fastcall* m_pfnEngineSurface_createNewTextureID)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawPrintCharAdd)(void* pthis, int, int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawSetTextureFile)(void* pthis, int, int textureId, const char* filename, qboolean hardwareFilter, bool forceReload) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawGetTextureSize)(void* pthis, int, int textureId, int& wide, int& tall) = NULL;
static bool(__fastcall* m_pfnEngineSurface_isTextureIDValid)(void* pthis, int, int) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawSetSubTextureRGBA)(void* pthis, int, int textureID, int drawX, int drawY, const unsigned char* rgba, int subTextureWide, int subTextureTall) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawFlushText)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawSetTextureBGRA)(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, bool forceUpload) = NULL;
static void(__fastcall* m_pfnEngineSurface_drawUpdateRegionTextureBGRA)(void* pthis, int, int textureID, int x, int y, const unsigned char* pchData, int wide, int tall) = NULL;

void Engine_FillAddress_EngineSurface_drawFlushText(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto enginesurface_drawFlushText_VA = ConvertDllInfoSpace(gPrivateFuncs.enginesurface_drawFlushText, RealDllInfo, DllInfo);

	if (!enginesurface_drawFlushText_VA)
	{
		Sig_NotFound(enginesurface_drawFlushText);
	}
	typedef struct enginesurface_drawFlushText_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}enginesurface_drawFlushText_SearchContext;

	enginesurface_drawFlushText_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(enginesurface_drawFlushText_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (enginesurface_drawFlushText_SearchContext*)context;

		if (pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM
			)
		{
			g_iVertexBufferEntriesUsed = (decltype(g_iVertexBufferEntriesUsed))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (!g_VertexBuffer && pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
			)
		{
			g_VertexBuffer = (decltype(g_VertexBuffer))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_VarNotFound(g_VertexBuffer);
	Sig_VarNotFound(g_iVertexBufferEntriesUsed);
}

void Engine_FillAddress_EngineSurface_pushMakeCurrent(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto enginesurface_pushMakeCurrent_VA = ConvertDllInfoSpace(gPrivateFuncs.enginesurface_pushMakeCurrent, RealDllInfo, DllInfo);

	typedef struct pushMakeCurrent_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		ULONG_PTR mainwindow_candidate{};
		int mainwindow_candidate_reg{};
		int mainwindow_candidate_InstCount{};
		int g_bScissor_InstCount{};
		ULONG_PTR candidate[4]{};
		int candidate_count{};
	}pushMakeCurrent_SearchContext;

	pushMakeCurrent_SearchContext ctx = { DllInfo,RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(enginesurface_pushMakeCurrent_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (pushMakeCurrent_SearchContext*)context;

		if (!pmainwindow &&
			instCount < 35 &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			pinst->detail->x86.operands[1].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			ctx->mainwindow_candidate = (decltype(ctx->mainwindow_candidate))pinst->detail->x86.operands[1].mem.disp;
			ctx->mainwindow_candidate_reg = pinst->detail->x86.operands[0].reg;
			ctx->mainwindow_candidate_InstCount = instCount;
		}

		if (!pmainwindow &&
			instCount < 40 && ctx->mainwindow_candidate &&
			instCount > ctx->mainwindow_candidate_InstCount &&
			instCount < ctx->mainwindow_candidate_InstCount + 6 &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == ctx->mainwindow_candidate_reg)
		{
			pmainwindow = (decltype(pmainwindow))ConvertDllInfoSpace((PVOID)ctx->mainwindow_candidate, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (!pmainwindow &&
			instCount < 40 && ctx->mainwindow_candidate &&
			instCount > ctx->mainwindow_candidate_InstCount &&
			instCount < ctx->mainwindow_candidate_InstCount + 6 &&
			pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == ctx->mainwindow_candidate_reg)
		{
			pmainwindow = (decltype(pmainwindow))ConvertDllInfoSpace((PVOID)ctx->mainwindow_candidate, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (!g_bScissor && pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 1 &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[0].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			g_bScissor = (decltype(g_bScissor))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			ctx->g_bScissor_InstCount = instCount;
		}

		if (ctx->g_bScissor_InstCount > 0 && instCount > ctx->g_bScissor_InstCount && ctx->candidate_count < 4)
		{
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				ctx->candidate[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				ctx->candidate_count++;
			}
		}

		if (ctx->candidate_count >= 4 && g_bScissor)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	if (ctx.candidate_count >= 4)
	{
		std::qsort(ctx.candidate, ctx.candidate_count, sizeof(ctx.candidate[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
			});
		g_ScissorRect = (decltype(g_ScissorRect))ConvertDllInfoSpace((PVOID)ctx.candidate[0], DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(pmainwindow);
	Sig_VarNotFound(g_bScissor);
	Sig_VarNotFound(g_ScissorRect);
}

inline float InterpTCoord(float val, float mins, float maxs, float tMin, float tMax)
{
	float flPercent = (float)(val - mins) / (maxs - mins);
	return tMin + (tMax - tMin) * flPercent;
}

static bool ScissorRect_TCoords(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1, RECT* pOut, TCoordRect* pTCoords)
{
	RECT rcChar;

	rcChar.left = x0;
	rcChar.top = y0;
	rcChar.right = x1;
	rcChar.bottom = y1;

	if ((*g_bScissor))
	{
		if (!IntersectRect(pOut, g_ScissorRect, &rcChar))
			return false;

		if (pTCoords)
		{
			pTCoords->s0 = InterpTCoord(pOut->left, rcChar.left, rcChar.right, s0, s1);
			pTCoords->s1 = InterpTCoord(pOut->right, rcChar.left, rcChar.right, s0, s1);
			pTCoords->t0 = InterpTCoord(pOut->top, rcChar.top, rcChar.bottom, t0, t1);
			pTCoords->t1 = InterpTCoord(pOut->bottom, rcChar.top, rcChar.bottom, t0, t1);
		}
	}
	else
	{
		*pOut = rcChar;

		if (pTCoords)
		{
			pTCoords->s0 = s0;
			pTCoords->s1 = s1;
			pTCoords->t0 = t0;
			pTCoords->t1 = t1;
		}
	}

	return true;
}

static bool ScissorRect(int x0, int y0, int x1, int y1, RECT* pOut)
{
	return ScissorRect_TCoords(x0, y0, x1, y1, 0, 0, 0, 0, pOut, NULL);
}

static EngineSurfaceTexture* staticGetTextureById(int id)
{
	auto it = g_VGuiSurfaceTextures.find(id);

	if (it != g_VGuiSurfaceTextures.end())
	{
		return it->second;
	}

	return nullptr;
}

static EngineSurfaceTexture* staticAllocTextureForId(int id)
{
	auto it = g_VGuiSurfaceTextures.find(id);

	if (it != g_VGuiSurfaceTextures.end())
	{
		return it->second;
	}

	EngineSurfaceTexture* pNewEntry = new EngineSurfaceTexture;

	g_VGuiSurfaceTextures[id] = pNewEntry;

	return pNewEntry;
}

void staticFreeTextureId(int id)
{
	auto it = g_VGuiSurfaceTextures.find(id);
	if (it != g_VGuiSurfaceTextures.end())
	{
		EngineSurfaceTexture* pNewEntry = it->second;

		delete pNewEntry;

		g_VGuiSurfaceTextures.erase(it);
	}
}

void __fastcall enginesurface_pushMakeCurrent(void* pthis, int, int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace)
{
	int surfaceAbsExtents[4] = { 0 };
	int xTranslate = 0, yTranslate = 0;

	POINT pnt = { 0 };
	RECT rect = { 0 };

	if (translateToScreenSpace)
	{
		if (g_pMetaHookAPI->VideoModeIsWindowed())
		{
			gPrivateFuncs.SDL_GetWindowPosition(Sys_GetMainWindow(), (int*)&pnt.x, (int*)&pnt.y);
		}
		else
		{
			pnt.x = 0;
			pnt.y = 0;
		}
	}

	if (g_pMetaHookAPI->VideoModeIsWindowed())
	{
		gPrivateFuncs.SDL_GetWindowSize(Sys_GetMainWindow(), (int*)&rect.right, (int*)&rect.bottom);
	}
	else
	{
		g_pMetaHookAPI->GetVideoMode((int*)&rect.right, (int*)&rect.bottom, nullptr, nullptr);
	}

	xTranslate = pnt.x;
	yTranslate = pnt.y;

	int wide = rect.right;
	int tall = rect.bottom;

	int x0, y0, x1, y1;

	x0 = insets[0];
	y0 = insets[1];

	x1 = absExtents[0] - xTranslate;
	y1 = absExtents[1] - yTranslate;

	surfaceAbsExtents[0] = absExtents[0] - xTranslate;
	surfaceAbsExtents[1] = absExtents[1] - yTranslate;
	surfaceAbsExtents[2] = absExtents[2] - xTranslate;
	surfaceAbsExtents[3] = absExtents[3] - yTranslate;

	(*g_bScissor) = true;
	(*g_ScissorRect).left = clipRect[0] - xTranslate - (insets[0] + surfaceAbsExtents[0]);
	(*g_ScissorRect).top = clipRect[1] - yTranslate - (insets[1] + surfaceAbsExtents[1]);
	(*g_ScissorRect).right = clipRect[2] - xTranslate - (insets[0] + surfaceAbsExtents[0]);
	(*g_ScissorRect).bottom = clipRect[3] - yTranslate - (insets[1] + surfaceAbsExtents[1]);

	R_PushWorldMatrix();

	R_PushProjectionMatrix();

	R_SetupOrthoProjectionMatrix(0, wide, tall, 0, -1, 1, true);

	R_LoadIdentityForWorldMatrix();

	R_TranslateWorldMatrix(x0, y0, 0);

	R_TranslateWorldMatrix(x1, y1, 0);
}

void __fastcall enginesurface_popMakeCurrent(void* pthis, int)
{
	enginesurface_drawFlushText(pthis, 0);

	(*g_bScissor) = false;

	R_PopProjectionMatrix();

	R_PopWorldMatrix();
}

void __fastcall enginesurface_drawFilledRect(void* pthis, int, int x0, int y0, int x1, int y1)
{
	int (*_drawColor)[4] = (decltype(_drawColor))((ULONG_PTR)pthis + gPrivateFuncs.enginesurface_drawColor_offset);

	if ((*_drawColor)[3] == 255)
		return;

	RECT rcOut;

	if (!ScissorRect(x0, y0, x1, y1, &rcOut))
		return;

	filledrectvertex_t vertices[4];

	vertices[0].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[0].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[0].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[0].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[0].pos[0] = rcOut.left;
	vertices[0].pos[1] = rcOut.top;

	vertices[1].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[1].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[1].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[1].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[1].pos[0] = rcOut.right;
	vertices[1].pos[1] = rcOut.top;

	vertices[2].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[2].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[2].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[2].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[2].pos[0] = rcOut.right;
	vertices[2].pos[1] = rcOut.bottom;

	vertices[3].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[3].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[3].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[3].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[3].pos[0] = rcOut.left;
	vertices[3].pos[1] = rcOut.bottom;

	const uint32_t indices[] = { 0,1,2,2,3,0 };

	R_DrawFilledRect(vertices, _countof(vertices), indices, _countof(indices), DRAW_FILLED_RECT_ALPHA_BLEND_ENABLED, "drawFilledRect");
}

void __fastcall enginesurface_drawOutlinedRect(void* pthis, int, int x0, int y0, int x1, int y1)
{
	int (*_drawColor)[4] = (decltype(_drawColor))((ULONG_PTR)pthis + gPrivateFuncs.enginesurface_drawColor_offset);

	if ((*_drawColor)[3] == 255)
		return;

	enginesurface_drawFilledRect(pthis, 0, x0, y0, x1, y0 + 1);
	enginesurface_drawFilledRect(pthis, 0, x0, y1 - 1, x1, y1);
	enginesurface_drawFilledRect(pthis, 0, x0, y0 + 1, x0 + 1, y1 - 1);
	enginesurface_drawFilledRect(pthis, 0, x1 - 1, y0 + 1, x1, y1 - 1);
}

void __fastcall enginesurface_drawLine(void* pthis, int, int x0, int y0, int x1, int y1)
{
	int (*_drawColor)[4] = (decltype(_drawColor))((ULONG_PTR)pthis + gPrivateFuncs.enginesurface_drawColor_offset);

	if ((*_drawColor)[3] == 255)
		return;

	filledrectvertex_t vertices[2];

	vertices[0].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[0].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[0].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[0].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[0].pos[0] = x0;
	vertices[0].pos[1] = y0;

	vertices[1].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[1].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[1].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[1].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[1].pos[0] = x1;
	vertices[1].pos[1] = y1;

	const uint32_t indices[] = { 0,1 };

	R_DrawFilledRect(vertices, _countof(vertices), indices, _countof(indices), DRAW_FILLED_RECT_LINE_ENABLED, "drawFilledRect");
}

void __fastcall enginesurface_drawPolyLine(void* pthis, int, int* px, int* py, int numPoints)
{
	int (*_drawColor)[4] = (decltype(_drawColor))((ULONG_PTR)pthis + 4);

	if ((*_drawColor)[3] == 255)
		return;

	std::vector<filledrectvertex_t> vertices;
	std::vector<uint32_t> indices;

	for (int i = 0; i < numPoints; ++i)
	{
		filledrectvertex_t v;

		v.col[0] = (*_drawColor)[0] / 255.0f;
		v.col[1] = (*_drawColor)[1] / 255.0f;
		v.col[2] = (*_drawColor)[2] / 255.0f;
		v.col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);

		v.pos[0] = px[i];
		v.pos[1] = py[i];

		vertices.emplace_back(v);
	}

	for (uint32_t i = 0; i < (uint32_t)vertices.size() - 1; ++i)
	{
		indices.emplace_back(i);
		indices.emplace_back(i + 1);
	}

	R_DrawFilledRect(vertices.data(), vertices.size(), indices.data(), indices.size(), DRAW_FILLED_RECT_LINE_ENABLED, "drawFilledRect");
}

void __fastcall enginesurface_drawSetTextureRGBA(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, qboolean hasAlphaChannel)
{
	auto texture = staticGetTextureById(textureId);

	if (!texture)
		texture = staticAllocTextureForId(textureId);

	if (texture)
	{
		texture->_id = textureId;
		texture->_wide = wide;
		texture->_tall = tall;

		texture->_s0 = 0;
		texture->_t0 = 0;
		texture->_s1 = 1;
		texture->_t1 = 1;

		staticTextureCurrent = texture;
		GL_Bind(textureId);

		if (hardwareFilter)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, wide, tall, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

#if defined(_DEBUG)
		if (glObjectLabel)
		{
			char szObjectName[256]{};
			snprintf(szObjectName, sizeof(szObjectName), "drawSetTextureRGBA - %d", textureId);
			glObjectLabel(GL_TEXTURE, textureId, -1, szObjectName);
		}
#endif
	}
}

void __fastcall enginesurface_drawSetTexture(void* pthis, int, int textureId)
{
	if (textureId != (*currenttexture))
	{
		enginesurface_drawFlushText(pthis, 0);

		(*currenttexture) = textureId;
	}

	if (textureId == 4)
	{
		gEngfuncs.Con_DPrintf("enginesurface_drawSetTexture: %d\n", textureId);
	}

	staticTextureCurrent = staticGetTextureById(textureId);

	glBindTexture(GL_TEXTURE_2D, textureId);
}

void __fastcall enginesurface_drawTexturedRect(void* pthis, int, int x0, int y0, int x1, int y1)
{
	if (!staticTextureCurrent)
		return;

	int (*_drawColor)[4] = (decltype(_drawColor))((ULONG_PTR)pthis + gPrivateFuncs.enginesurface_drawColor_offset);

	if ((*_drawColor)[3] == 255)
		return;

	RECT rcOut;
	TCoordRect tRect;

	if (!ScissorRect_TCoords(x0, y0, x1, y1, staticTextureCurrent->_s0, staticTextureCurrent->_t0, staticTextureCurrent->_s1, staticTextureCurrent->_t1, &rcOut, &tRect))
		return;

	texturedrectvertex_t vertices[4];

	vertices[0].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[0].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[0].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[0].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[0].texcoord[0] = tRect.s0;
	vertices[0].texcoord[1] = tRect.t0;
	vertices[0].pos[0] = rcOut.left;
	vertices[0].pos[1] = rcOut.top;

	vertices[1].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[1].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[1].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[1].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[1].texcoord[0] = tRect.s1;
	vertices[1].texcoord[1] = tRect.t0;
	vertices[1].pos[0] = rcOut.right;
	vertices[1].pos[1] = rcOut.top;

	vertices[2].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[2].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[2].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[2].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[2].texcoord[0] = tRect.s1;
	vertices[2].texcoord[1] = tRect.t1;
	vertices[2].pos[0] = rcOut.right;
	vertices[2].pos[1] = rcOut.bottom;

	vertices[3].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[3].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[3].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[3].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[3].texcoord[0] = tRect.s0;
	vertices[3].texcoord[1] = tRect.t1;
	vertices[3].pos[0] = rcOut.left;
	vertices[3].pos[1] = rcOut.bottom;

	const uint32_t indices[] = { 0,1,2,2,3,0 };

	(*currenttexture) = staticTextureCurrent->_id;

#ifdef _DEBUG
	char szDebugName[256]{};
	snprintf(szDebugName, sizeof(szDebugName), "drawTexturedRect - %d", staticTextureCurrent->_id);
	R_DrawTexturedRect(staticTextureCurrent->_id, vertices, _countof(vertices), indices, _countof(indices), DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED, szDebugName);
#else
	R_DrawTexturedRect(staticTextureCurrent->_id, vertices, _countof(vertices), indices, _countof(indices), DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED, "drawTexturedRect");
#endif
}

void __fastcall enginesurface_drawTexturedRectAdd(void* pthis, int, int x0, int y0, int x1, int y1)
{
	if (!staticTextureCurrent)
		return;

	int (*_drawColor)[4] = (decltype(_drawColor))((ULONG_PTR)pthis + gPrivateFuncs.enginesurface_drawColor_offset);

	if ((*_drawColor)[3] == 255)
		return;

	RECT rcOut;
	TCoordRect tRect;

	if (!ScissorRect_TCoords(x0, y0, x1, y1, staticTextureCurrent->_s0, staticTextureCurrent->_t0, staticTextureCurrent->_s1, staticTextureCurrent->_t1, &rcOut, &tRect))
		return;

	texturedrectvertex_t vertices[4];

	vertices[0].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[0].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[0].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[0].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[0].texcoord[0] = tRect.s0;
	vertices[0].texcoord[1] = tRect.t0;
	vertices[0].pos[0] = rcOut.left;
	vertices[0].pos[1] = rcOut.top;

	vertices[1].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[1].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[1].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[1].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[1].texcoord[0] = tRect.s1;
	vertices[1].texcoord[1] = tRect.t0;
	vertices[1].pos[0] = rcOut.right;
	vertices[1].pos[1] = rcOut.top;

	vertices[2].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[2].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[2].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[2].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[2].texcoord[0] = tRect.s1;
	vertices[2].texcoord[1] = tRect.t1;
	vertices[2].pos[0] = rcOut.right;
	vertices[2].pos[1] = rcOut.bottom;

	vertices[3].col[0] = (*_drawColor)[0] / 255.0f;
	vertices[3].col[1] = (*_drawColor)[1] / 255.0f;
	vertices[3].col[2] = (*_drawColor)[2] / 255.0f;
	vertices[3].col[3] = 1.0f - ((*_drawColor)[3] / 255.0f);
	vertices[3].texcoord[0] = tRect.s0;
	vertices[3].texcoord[1] = tRect.t1;
	vertices[3].pos[0] = rcOut.left;
	vertices[3].pos[1] = rcOut.bottom;

	const uint32_t indices[] = { 0,1,2,2,3,0 };

	(*currenttexture) = staticTextureCurrent->_id;

	R_DrawTexturedRect(staticTextureCurrent->_id, vertices, _countof(vertices), indices, _countof(indices), DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED, "drawTexturedRectAdd");
}

void __fastcall enginesurface_drawPrintCharAdd(void* pthis, int, int x, int y, int wide, int tall, float s0, float t0, float s1, float t1)
{
	int (*_drawTextColor)[4] = (decltype(_drawTextColor))((ULONG_PTR)pthis + gPrivateFuncs.enginesurface_drawTextColor_offset);

	if ((*_drawTextColor)[3] == 255)
		return;

	RECT rcOut;
	TCoordRect tRect;

	if (!ScissorRect_TCoords(x, y, x + wide, y + tall, s0, t0, s1, t1, &rcOut, &tRect))
		return;

	texturedrectvertex_t vertices[4];

	vertices[0].col[0] = (*_drawTextColor)[0] / 255.0f;
	vertices[0].col[1] = (*_drawTextColor)[1] / 255.0f;
	vertices[0].col[2] = (*_drawTextColor)[2] / 255.0f;
	vertices[0].col[3] = 1.0f - ((*_drawTextColor)[3] / 255.0f);
	vertices[0].texcoord[0] = tRect.s0;
	vertices[0].texcoord[1] = tRect.t0;
	vertices[0].pos[0] = rcOut.left;
	vertices[0].pos[1] = rcOut.top;

	vertices[1].col[0] = (*_drawTextColor)[0] / 255.0f;
	vertices[1].col[1] = (*_drawTextColor)[1] / 255.0f;
	vertices[1].col[2] = (*_drawTextColor)[2] / 255.0f;
	vertices[1].col[3] = 1.0f - ((*_drawTextColor)[3] / 255.0f);
	vertices[1].texcoord[0] = tRect.s1;
	vertices[1].texcoord[1] = tRect.t0;
	vertices[1].pos[0] = rcOut.right;
	vertices[1].pos[1] = rcOut.top;

	vertices[2].col[0] = (*_drawTextColor)[0] / 255.0f;
	vertices[2].col[1] = (*_drawTextColor)[1] / 255.0f;
	vertices[2].col[2] = (*_drawTextColor)[2] / 255.0f;
	vertices[2].col[3] = 1.0f - ((*_drawTextColor)[3] / 255.0f);
	vertices[2].texcoord[0] = tRect.s1;
	vertices[2].texcoord[1] = tRect.t1;
	vertices[2].pos[0] = rcOut.right;
	vertices[2].pos[1] = rcOut.bottom;

	vertices[3].col[0] = (*_drawTextColor)[0] / 255.0f;
	vertices[3].col[1] = (*_drawTextColor)[1] / 255.0f;
	vertices[3].col[2] = (*_drawTextColor)[2] / 255.0f;
	vertices[3].col[3] = 1.0f - ((*_drawTextColor)[3] / 255.0f);
	vertices[3].texcoord[0] = tRect.s0;
	vertices[3].texcoord[1] = tRect.t1;
	vertices[3].pos[0] = rcOut.left;
	vertices[3].pos[1] = rcOut.bottom;

	const uint32_t indices[] = { 0,1,2,2,3,0 };

	(*currenttexture) = staticTextureCurrent->_id;

	R_DrawTexturedRect(staticTextureCurrent->_id, vertices, _countof(vertices), indices, _countof(indices), DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED, "drawPrintCharAdd");
}

void __fastcall enginesurface_drawSetTextureFile(void* pthis, int dummy, int textureId, const char* filename, qboolean hardwareFilter, bool forceReload)
{
	bool bLoaded = false;
	char filepath[1024]{};

	auto texture = staticGetTextureById(textureId);

	if (texture && !forceReload)
	{
		enginesurface_drawSetTexture(pthis, dummy, textureId);
		return;
	}

	gl_loadtexture_context_t loadContext;
	loadContext.wrap = GL_CLAMP_TO_EDGE;
	loadContext.filter = hardwareFilter ? GL_LINEAR : GL_NEAREST;
	loadContext.ignore_direction_LoadTGA = true;

	loadContext.callback = [pthis, textureId, hardwareFilter, filename](gl_loadtexture_context_t* ctx) {

		if (ctx->mipmaps.size() > 0)
		{
			if (ctx->compressed)
			{
				auto texture = staticGetTextureById(textureId);

				if (!texture)
					texture = staticAllocTextureForId(textureId);

				if (texture)
				{
					texture->_id = textureId;
					texture->_wide = ctx->width;
					texture->_tall = ctx->height;

					texture->_s0 = 0;
					texture->_t0 = 0;
					texture->_s1 = 1;
					texture->_t1 = 1;

					strncpy(texture->_name, filename, sizeof(texture->_name) - 1);
					texture->_name[sizeof(texture->_name) - 1] = 0;

					GL_Bind(textureId);
					GL_UploadCompressedTexture(ctx, GL_TEXTURE_2D);

#if defined(_DEBUG)
					if (glObjectLabel)
					{
						char szObjectName[256]{};
						snprintf(szObjectName, sizeof(szObjectName), "enginesurface - %s", filename);
						glObjectLabel(GL_TEXTURE, textureId, -1, szObjectName);
					}
#endif
				}

				return true;
			}
			else
			{
				auto texture = staticGetTextureById(textureId);

				if (!texture)
					texture = staticAllocTextureForId(textureId);

				if (texture)
				{
					texture->_id = textureId;
					texture->_wide = ctx->width;
					texture->_tall = ctx->height;

					texture->_s0 = 0;
					texture->_t0 = 0;
					texture->_s1 = 1;
					texture->_t1 = 1;

					strncpy(texture->_name, filename, sizeof(texture->_name) - 1);
					texture->_name[sizeof(texture->_name) - 1] = 0;

					GL_Bind(textureId);
					GL_UploadUncompressedTexture(ctx, GL_TEXTURE_2D);

#if defined(_DEBUG)
					if (glObjectLabel)
					{
						char szObjectName[256]{};
						snprintf(szObjectName, sizeof(szObjectName), "enginesurface - %s", filename);
						glObjectLabel(GL_TEXTURE, textureId, -1, szObjectName);
					}
#endif
				}

				return true;
			}
		}

		return false;
		};

	if (1)
	{
		snprintf(filepath, sizeof(filepath), "%s.dds", filename);

		if (g_iEngineType == ENGINE_SVENGINE && !bLoaded && LoadDDS(filepath, "UI", &loadContext))
		{
			bLoaded = true;
		}

		if (!bLoaded && LoadDDS(filepath, NULL, &loadContext))
		{
			bLoaded = true;
		}
	}

	if (!bLoaded)
	{
		snprintf(filepath, sizeof(filepath), "%s.tga", filename);

		if (g_iEngineType == ENGINE_SVENGINE && !bLoaded && LoadImageGenericFileIO(filepath, "UI", &loadContext))
		{
			bLoaded = true;
		}

		if (!bLoaded && LoadImageGenericFileIO(filepath, NULL, &loadContext))
		{
			bLoaded = true;
		}
	}

	if (!bLoaded)
	{
		snprintf(filepath, sizeof(filepath), "%s.bmp", filename);

		if (g_iEngineType == ENGINE_SVENGINE && !bLoaded && LoadImageGenericFileIO(filepath, "UI", &loadContext))
		{
			bLoaded = true;
		}

		if (!bLoaded && LoadImageGenericFileIO(filepath, NULL, &loadContext))
		{
			bLoaded = true;
		}
	}

	if (texture)
	{
		enginesurface_drawSetTexture(pthis, 0, textureId);
	}
}

void __fastcall enginesurface_drawGetTextureSize(void* pthis, int, int textureId, int& wide, int& tall)
{
	auto texture = staticGetTextureById(textureId);

	if (texture)
	{
		wide = texture->_wide;
		tall = texture->_tall;
	}
	else
	{
		wide = 0;
		tall = 0;
	}
}

bool __fastcall enginesurface_isTextureIDValid(void* pthis, int, int textureId)
{
	return (staticGetTextureById(textureId) != nullptr);
}

void __fastcall enginesurface_drawSetSubTextureRGBA(void* pthis, int, int textureID, int drawX, int drawY, const unsigned char* rgba, int subTextureWide, int subTextureTall)
{
	if (subTextureWide == 0 || subTextureTall == 0)
	{
		gEngfuncs.Con_DPrintf("drawSetSubTextureRGBA: invalid wide or tall.\n");
		return;
	}

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, drawX, drawY, subTextureWide, subTextureTall, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void __fastcall enginesurface_drawSetTextureBGRA(void* pthis, int, int textureId, const char* data, int wide, int tall, qboolean hardwareFilter, bool forceUpload)
{
	auto texture = staticGetTextureById(textureId);

	if (!texture)
		texture = staticAllocTextureForId(textureId);

	if (texture)
	{
		texture->_id = textureId;
		texture->_wide = wide;
		texture->_tall = tall;

		texture->_s0 = 0;
		texture->_t0 = 0;
		texture->_s1 = 1;
		texture->_t1 = 1;

		staticTextureCurrent = texture;
		GL_Bind(textureId);

		if (hardwareFilter)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, wide, tall, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);

#if defined(_DEBUG)
		if (glObjectLabel)
		{
			char szObjectName[256]{};
			snprintf(szObjectName, sizeof(szObjectName), "drawSetTextureBGRA - %d", textureId);
			glObjectLabel(GL_TEXTURE, textureId, -1, szObjectName);
		}
#endif
	}
}

void __fastcall enginesurface_drawUpdateRegionTextureBGRA(void* pthis, int, int textureID, int x, int y, const unsigned char* pchData, int wide, int tall)
{
	if (wide == 0 || tall == 0)
	{
		gEngfuncs.Con_DPrintf("drawUpdateRegionTextureBGRA: invalid wide or tall.\n");
		return;
	}

	auto texture = staticGetTextureById(textureID);

	if (texture)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->_wide);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, texture->_wide, tall, GL_BGRA, GL_UNSIGNED_BYTE, pchData + (y * texture->_wide * 4));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}


int __fastcall enginesurface_createNewTextureID(void* pthis, int dummy)
{
	// allocated_surface_texture = 5810; from BlobEngine
	return (int)GL_GenTexture();
}

void __fastcall enginesurface_drawFlushText(void* pthis, int dummy)
{
	int (*_drawTextColor)[4] = (decltype(_drawTextColor))((ULONG_PTR)pthis + gPrivateFuncs.enginesurface_drawTextColor_offset);

	if ((*g_iVertexBufferEntriesUsed) > 0)
	{
		std::vector<texturedrectvertex_t> vertices;
		std::vector<uint32_t> indices;

		vertices.reserve((*g_iVertexBufferEntriesUsed));

		for (int i = 0; i < (*g_iVertexBufferEntriesUsed); i++)
		{
			texturedrectvertex_t v;

			memcpy(v.texcoord, (*g_VertexBuffer)[i].texcoords, sizeof(vec2_t));
			memcpy(v.pos, (*g_VertexBuffer)[i].vertex, sizeof(vec2_t));
			v.col[0] = (*_drawTextColor)[0] / 255.0f;
			v.col[1] = (*_drawTextColor)[1] / 255.0f;
			v.col[2] = (*_drawTextColor)[2] / 255.0f;
			v.col[3] = 1.0f - ((*_drawTextColor)[3] / 255.0f);

			vertices.emplace_back(v);
		}

		const uint32_t baseIndices[] = { 0, 1, 2, 2, 3, 0 };

		for (int i = 0; i < (*g_iVertexBufferEntriesUsed); i += 4)
		{
			for (int j = 0; j < _countof(baseIndices); ++j)
			{
				indices.emplace_back(i + baseIndices[j]);
			}
		}

		R_DrawTexturedRect((*currenttexture), vertices.data(), vertices.size(), indices.data(), indices.size(), DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED, "drawFlushText");
	}

	(*g_iVertexBufferEntriesUsed) = 0;
}

class CEngineSurfaceProxy : public IEngineSurface
{
public:
	void pushMakeCurrent(int* p1, int* p2, int* r, bool useInsets) override;
	void popMakeCurrent(void) override;
	void drawFilledRect(int x0, int y0, int x1, int y1) override;
	void drawOutlinedRect(int x0, int y0, int x1, int y1) override;
	void drawLine(int x0, int y0, int x1, int y1) override;
	void drawPolyLine(int* px, int* py, int numPoints) override;
	void drawTexturedPolygon(vgui::VGuiVertex* p, int n) override;
	void drawSetTextureRGBA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceReload) override;
	void drawSetTexture(int id) override;
	void drawTexturedRect(int x0, int y0, int x1, int y1) override;
	int createNewTextureID(void) override;
	void drawSetColor(int r, int g, int b, int a) override;
	void drawSetTextColor(int r, int g, int b, int a) override;
	void drawSetTextPos(int x, int y) override;
	void drawGetTextPos(int& x, int& y) override;
	void drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override;
	void drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override;
	void drawSetTextureFile(int id, const char* filename, int hardwareFilter, bool forceReload) override;
	void drawGetTextureSize(int id, int& wide, int& tall) override;
	bool isTextureIDValid(int id) override;
	bool drawSetSubTextureRGBA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall) override;
	void drawFlushText(void) override;
	void resetViewPort(void) override;
	void drawSetTextureBGRA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceUpload) override;
	void drawUpdateRegionTextureBGRA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall) override;
};

extern CEngineSurfaceProxy g_EngineSurfaceProxy;

class CEngineSurfaceProxy_HL25 : public IEngineSurface_HL25
{
public:
	void pushMakeCurrent(int* p1, int* p2, int* r, bool useInsets) override;
	void popMakeCurrent(void) override;
	void drawFilledRect(int x0, int y0, int x1, int y1) override;
	void drawOutlinedRect(int x0, int y0, int x1, int y1) override;
	void drawLine(int x0, int y0, int x1, int y1) override;
	void drawPolyLine(int* px, int* py, int numPoints) override;
	void drawTexturedPolygon(int* p, int n) override;
	void drawSetTextureRGBA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceReload) override;
	void drawSetTexture(int id) override;
	void drawTexturedRect(int x0, int y0, int x1, int y1) override;
	void drawTexturedRectAdd(int x0, int y0, int x1, int y1) override;
	int createNewTextureID(void) override;
	void drawSetColor(int r, int g, int b, int a) override;
	void drawSetTextColor(int r, int g, int b, int a) override;
	void drawSetTextPos(int x, int y) override;
	void drawGetTextPos(int& x, int& y) override;
	void drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override;
	void drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override;
	void drawSetTextureFile(int id, const char* filename, int hardwareFilter, bool forceReload) override;
	void drawGetTextureSize(int id, int& wide, int& tall) override;
	bool isTextureIDValid(int id) override;
	bool drawSetSubTextureRGBA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall) override;
	void drawFlushText(void) override;
	void resetViewPort(void) override;
	void drawSetTextureBGRA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceUpload) override;
	void drawUpdateRegionTextureBGRA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall) override;
};

extern CEngineSurfaceProxy_HL25 g_EngineSurfaceProxy_HL25;

//Non-HL25

void CEngineSurfaceProxy::pushMakeCurrent(int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace)
{
	
}

void CEngineSurfaceProxy::popMakeCurrent(void)
{
	
}

void CEngineSurfaceProxy::drawFilledRect(int x0, int y0, int x1, int y1)
{

}

void CEngineSurfaceProxy::drawOutlinedRect(int x0, int y0, int x1, int y1)
{

}

void CEngineSurfaceProxy::drawLine(int x0, int y0, int x1, int y1)
{

}

void CEngineSurfaceProxy::drawPolyLine(int* px, int* py, int numPoints)
{

}

void CEngineSurfaceProxy::drawTexturedPolygon(vgui::VGuiVertex* p, int n)
{

}

void CEngineSurfaceProxy::drawSetTextureRGBA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceReload)
{

}

void CEngineSurfaceProxy::drawSetTexture(int id)
{

}

void CEngineSurfaceProxy::drawTexturedRect(int x0, int y0, int x1, int y1)
{

}

int CEngineSurfaceProxy::createNewTextureID(void)
{
	return 0;
}

void CEngineSurfaceProxy::drawSetColor(int r, int g, int b, int a)
{

}

void CEngineSurfaceProxy::drawSetTextColor(int r, int g, int b, int a)
{

}

void CEngineSurfaceProxy::drawSetTextPos(int x, int y)
{

}

void CEngineSurfaceProxy::drawGetTextPos(int& x, int& y)
{

}

void CEngineSurfaceProxy::drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1)
{

}

void CEngineSurfaceProxy::drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1)
{

}

void CEngineSurfaceProxy::drawSetTextureFile(int id, const char* filename, int hardwareFilter, bool forceReload)
{

}

void CEngineSurfaceProxy::drawGetTextureSize(int id, int& wide, int& tall)
{

}

bool CEngineSurfaceProxy::isTextureIDValid(int id)
{
	return false;
}

bool CEngineSurfaceProxy::drawSetSubTextureRGBA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall)
{
	return false;
}

void CEngineSurfaceProxy::drawFlushText(void)
{

}

void CEngineSurfaceProxy::resetViewPort(void)
{

}

void CEngineSurfaceProxy::drawSetTextureBGRA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceUpload)
{

}

void CEngineSurfaceProxy::drawUpdateRegionTextureBGRA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall)
{

}

static CEngineSurfaceProxy g_EngineSurfaceProxy;

//HL25

void CEngineSurfaceProxy_HL25::pushMakeCurrent(int* p1, int* p2, int* r, bool useInsets)
{
	
}

void CEngineSurfaceProxy_HL25::popMakeCurrent(void)
{

}

void CEngineSurfaceProxy_HL25::drawFilledRect(int x0, int y0, int x1, int y1)
{

}

void CEngineSurfaceProxy_HL25::drawOutlinedRect(int x0, int y0, int x1, int y1)
{

}

void CEngineSurfaceProxy_HL25::drawLine(int x0, int y0, int x1, int y1)
{

}

void CEngineSurfaceProxy_HL25::drawPolyLine(int* px, int* py, int numPoints)
{

}

void CEngineSurfaceProxy_HL25::drawTexturedPolygon(int* p, int n)
{

}

void CEngineSurfaceProxy_HL25::drawSetTextureRGBA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceReload)
{

}

void CEngineSurfaceProxy_HL25::drawSetTexture(int id)
{

}

void CEngineSurfaceProxy_HL25::drawTexturedRect(int x0, int y0, int x1, int y1)
{

}

void CEngineSurfaceProxy_HL25::drawTexturedRectAdd(int x0, int y0, int x1, int y1)
{

}

int CEngineSurfaceProxy_HL25::createNewTextureID(void)
{
	return 0;
}

void CEngineSurfaceProxy_HL25::drawSetColor(int r, int g, int b, int a)
{

}

void CEngineSurfaceProxy_HL25::drawSetTextColor(int r, int g, int b, int a)
{

}

void CEngineSurfaceProxy_HL25::drawSetTextPos(int x, int y)
{

}

void CEngineSurfaceProxy_HL25::drawGetTextPos(int& x, int& y)
{

}

void CEngineSurfaceProxy_HL25::drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1)
{

}

void CEngineSurfaceProxy_HL25::drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1)
{

}

void CEngineSurfaceProxy_HL25::drawSetTextureFile(int id, const char* filename, int hardwareFilter, bool forceReload)
{

}

void CEngineSurfaceProxy_HL25::drawGetTextureSize(int id, int& wide, int& tall)
{

}

bool CEngineSurfaceProxy_HL25::isTextureIDValid(int id)
{
	return false;
}

bool CEngineSurfaceProxy_HL25::drawSetSubTextureRGBA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall)
{
	return false;
}

void CEngineSurfaceProxy_HL25::drawFlushText(void)
{

}

void CEngineSurfaceProxy_HL25::resetViewPort(void)
{

}

void CEngineSurfaceProxy_HL25::drawSetTextureBGRA(int id, const unsigned char* rgba, int wide, int tall, int hardwareFilter, bool forceUpload)
{

}

void CEngineSurfaceProxy_HL25::drawUpdateRegionTextureBGRA(int textureID, int drawX, int drawY, unsigned const char* rgba, int subTextureWide, int subTextureTall)
{

}

static CEngineSurfaceProxy_HL25 g_EngineSurfaceProxy_HL25;

void EngineSurface_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto engineFactory = g_pMetaHookAPI->GetEngineFactory();

	if (engineFactory)
	{
#define ENGINE_SURFACE_VERSION "EngineSurface007"
		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			enginesurface_HL25 = (decltype(enginesurface_HL25))engineFactory(ENGINE_SURFACE_VERSION, NULL);

			auto engineSurface_vftable = *(PVOID**)enginesurface_HL25;

			gPrivateFuncs.index_enginesurface_pushMakeCurrent = 1;
			gPrivateFuncs.index_enginesurface_popMakeCurrent = 2;
			gPrivateFuncs.index_enginesurface_drawFilledRect = 3;
			gPrivateFuncs.index_enginesurface_drawOutlinedRect = 4;
			gPrivateFuncs.index_enginesurface_drawLine = 5;
			gPrivateFuncs.index_enginesurface_drawPolyLine = 6;
			gPrivateFuncs.index_enginesurface_drawTexturedPolygon = 7;
			gPrivateFuncs.index_enginesurface_drawSetTextureRGBA = 8;
			gPrivateFuncs.index_enginesurface_drawSetTexture = 9;
			gPrivateFuncs.index_enginesurface_drawTexturedRect = 10;
			gPrivateFuncs.index_enginesurface_drawTexturedRectAdd = 11;
			gPrivateFuncs.index_enginesurface_createNewTextureID = 12;
			gPrivateFuncs.index_enginesurface_drawPrintCharAdd = 18;
			gPrivateFuncs.index_enginesurface_drawSetTextureFile = 19;
			gPrivateFuncs.index_enginesurface_drawGetTextureSize = 20;
			gPrivateFuncs.index_enginesurface_isTextureIDValid = 21;
			gPrivateFuncs.index_enginesurface_drawSetSubTextureRGBA = 22;
			gPrivateFuncs.index_enginesurface_drawFlushText = 23;
			gPrivateFuncs.index_enginesurface_drawSetTextureBGRA = 25;
			gPrivateFuncs.index_enginesurface_drawUpdateRegionTextureBGRA = 26;


			gPrivateFuncs.enginesurface_pushMakeCurrent = (decltype(gPrivateFuncs.enginesurface_pushMakeCurrent))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_pushMakeCurrent, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_popMakeCurrent = (decltype(gPrivateFuncs.enginesurface_popMakeCurrent))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_popMakeCurrent, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawFilledRect = (decltype(gPrivateFuncs.enginesurface_drawFilledRect))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawFilledRect, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawOutlinedRect = (decltype(gPrivateFuncs.enginesurface_drawOutlinedRect))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawOutlinedRect, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawLine = (decltype(gPrivateFuncs.enginesurface_drawLine))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawLine, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawPolyLine = (decltype(gPrivateFuncs.enginesurface_drawPolyLine))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawPolyLine, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTextureRGBA = (decltype(gPrivateFuncs.enginesurface_drawSetTextureRGBA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTextureRGBA, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTexture = (decltype(gPrivateFuncs.enginesurface_drawSetTexture))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTexture, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawTexturedRect = (decltype(gPrivateFuncs.enginesurface_drawTexturedRect))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawTexturedRect, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_createNewTextureID = (decltype(gPrivateFuncs.enginesurface_createNewTextureID))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_createNewTextureID, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawPrintCharAdd = (decltype(gPrivateFuncs.enginesurface_drawPrintCharAdd))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawPrintCharAdd, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTextureFile = (decltype(gPrivateFuncs.enginesurface_drawSetTextureFile))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTextureFile, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawGetTextureSize = (decltype(gPrivateFuncs.enginesurface_drawGetTextureSize))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawGetTextureSize, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_isTextureIDValid = (decltype(gPrivateFuncs.enginesurface_isTextureIDValid))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_isTextureIDValid, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetSubTextureRGBA = (decltype(gPrivateFuncs.enginesurface_drawSetSubTextureRGBA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetSubTextureRGBA, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawFlushText = (decltype(gPrivateFuncs.enginesurface_drawFlushText))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawFlushText, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTextureBGRA = (decltype(gPrivateFuncs.enginesurface_drawSetTextureBGRA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTextureBGRA, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawUpdateRegionTextureBGRA = (decltype(gPrivateFuncs.enginesurface_drawUpdateRegionTextureBGRA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawUpdateRegionTextureBGRA, DllInfo, RealDllInfo, RealDllInfo);

		}
		else
		{
			enginesurface = (decltype(enginesurface))engineFactory(ENGINE_SURFACE_VERSION, NULL);

			auto engineSurface_vftable = *(PVOID**)engineSurface;

			gPrivateFuncs.index_enginesurface_pushMakeCurrent = 1;
			gPrivateFuncs.index_enginesurface_popMakeCurrent = 2;
			gPrivateFuncs.index_enginesurface_drawFilledRect = 3;
			gPrivateFuncs.index_enginesurface_drawOutlinedRect = 4;
			gPrivateFuncs.index_enginesurface_drawLine = 5;
			gPrivateFuncs.index_enginesurface_drawPolyLine = 6;
			gPrivateFuncs.index_enginesurface_drawTexturedPolygon = 7;
			gPrivateFuncs.index_enginesurface_drawSetTextureRGBA = 8;
			gPrivateFuncs.index_enginesurface_drawSetTexture = 9;
			gPrivateFuncs.index_enginesurface_drawTexturedRect = 10;
			gPrivateFuncs.index_enginesurface_createNewTextureID = 11;
			gPrivateFuncs.index_enginesurface_drawPrintCharAdd = 17;
			gPrivateFuncs.index_enginesurface_drawSetTextureFile = 18;
			gPrivateFuncs.index_enginesurface_drawGetTextureSize = 19;
			gPrivateFuncs.index_enginesurface_isTextureIDValid = 20;
			gPrivateFuncs.index_enginesurface_drawSetSubTextureRGBA = 21;
			gPrivateFuncs.index_enginesurface_drawFlushText = 22;
			gPrivateFuncs.index_enginesurface_drawSetTextureBGRA = 24;
			gPrivateFuncs.index_enginesurface_drawUpdateRegionTextureBGRA = 25;

			gPrivateFuncs.enginesurface_pushMakeCurrent = (decltype(gPrivateFuncs.enginesurface_pushMakeCurrent))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_pushMakeCurrent, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_popMakeCurrent = (decltype(gPrivateFuncs.enginesurface_popMakeCurrent))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_popMakeCurrent, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawFilledRect = (decltype(gPrivateFuncs.enginesurface_drawFilledRect))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawFilledRect, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawOutlinedRect = (decltype(gPrivateFuncs.enginesurface_drawOutlinedRect))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawOutlinedRect, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawLine = (decltype(gPrivateFuncs.enginesurface_drawLine))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawLine, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawPolyLine = (decltype(gPrivateFuncs.enginesurface_drawPolyLine))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawPolyLine, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTextureRGBA = (decltype(gPrivateFuncs.enginesurface_drawSetTextureRGBA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTextureRGBA, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTexture = (decltype(gPrivateFuncs.enginesurface_drawSetTexture))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTexture, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawTexturedRect = (decltype(gPrivateFuncs.enginesurface_drawTexturedRect))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawTexturedRect, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawTexturedRectAdd = (decltype(gPrivateFuncs.enginesurface_drawTexturedRectAdd))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawTexturedRectAdd, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_createNewTextureID = (decltype(gPrivateFuncs.enginesurface_createNewTextureID))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_createNewTextureID, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawPrintCharAdd = (decltype(gPrivateFuncs.enginesurface_drawPrintCharAdd))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawPrintCharAdd, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTextureFile = (decltype(gPrivateFuncs.enginesurface_drawSetTextureFile))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTextureFile, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawGetTextureSize = (decltype(gPrivateFuncs.enginesurface_drawGetTextureSize))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawGetTextureSize, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_isTextureIDValid = (decltype(gPrivateFuncs.enginesurface_isTextureIDValid))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_isTextureIDValid, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetSubTextureRGBA = (decltype(gPrivateFuncs.enginesurface_drawSetSubTextureRGBA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetSubTextureRGBA, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawFlushText = (decltype(gPrivateFuncs.enginesurface_drawFlushText))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawFlushText, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawSetTextureBGRA = (decltype(gPrivateFuncs.enginesurface_drawSetTextureBGRA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawSetTextureBGRA, DllInfo, RealDllInfo, RealDllInfo);
			gPrivateFuncs.enginesurface_drawUpdateRegionTextureBGRA = (decltype(gPrivateFuncs.enginesurface_drawUpdateRegionTextureBGRA))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_drawUpdateRegionTextureBGRA, DllInfo, RealDllInfo, RealDllInfo);

		}

		Engine_FillAddress_EngineSurface_pushMakeCurrent(DllInfo, RealDllInfo);
		Engine_FillAddress_EngineSurface_drawFlushText(DllInfo, RealDllInfo);

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.enginesurface_drawColor_offset = 4;
			gPrivateFuncs.enginesurface_drawTextColor_offset = 20;
		}
		else
		{
			gPrivateFuncs.enginesurface_drawColor_offset = 8;
			gPrivateFuncs.enginesurface_drawTextColor_offset = 24;
		}
	}
}

void EngineSurface_InstallHooks(void)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		PVOID* pVFTable = *(PVOID**)&enginesurface_HL25;

		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_pushMakeCurrent, (void*)enginesurface_pushMakeCurrent, (void**)&m_pfnEngineSurface_pushMakeCurrent);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_popMakeCurrent, (void*)enginesurface_popMakeCurrent, (void**)&m_pfnEngineSurface_popMakeCurrent);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawFilledRect, (void*)enginesurface_drawFilledRect, (void**)&m_pfnEngineSurface_drawFilledRect);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawOutlinedRect, (void*)enginesurface_drawOutlinedRect, (void**)&m_pfnEngineSurface_drawOutlinedRect);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawLine, (void*)enginesurface_drawLine, (void**)&m_pfnEngineSurface_drawLine);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawPolyLine, (void*)enginesurface_drawPolyLine, (void**)&m_pfnEngineSurface_drawPolyLine);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawSetTextureRGBA, (void*)enginesurface_drawSetTextureRGBA, (void**)&m_pfnEngineSurface_drawSetTextureRGBA);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawSetTexture, (void*)enginesurface_drawSetTexture, (void**)&m_pfnEngineSurface_drawSetTexture);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawTexturedRect, (void*)enginesurface_drawTexturedRect, (void**)&m_pfnEngineSurface_drawTexturedRect);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawTexturedRectAdd, (void*)enginesurface_drawTexturedRectAdd, (void**)&m_pfnEngineSurface_drawTexturedRectAdd);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_createNewTextureID, (void*)enginesurface_createNewTextureID, (void**)&m_pfnEngineSurface_createNewTextureID);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawPrintCharAdd, (void*)enginesurface_drawPrintCharAdd, (void**)&m_pfnEngineSurface_drawPrintCharAdd);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawSetTextureFile, (void*)enginesurface_drawSetTextureFile, (void**)&m_pfnEngineSurface_drawSetTextureFile);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawGetTextureSize, (void*)enginesurface_drawGetTextureSize, (void**)&m_pfnEngineSurface_drawGetTextureSize);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_isTextureIDValid, (void*)enginesurface_isTextureIDValid, (void**)&m_pfnEngineSurface_isTextureIDValid);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawSetSubTextureRGBA, (void*)enginesurface_drawSetSubTextureRGBA, (void**)&m_pfnEngineSurface_drawSetSubTextureRGBA);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawFlushText, (void*)enginesurface_drawFlushText, (void**)&m_pfnEngineSurface_drawFlushText);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawSetTextureBGRA, (void*)enginesurface_drawSetTextureBGRA, (void**)&m_pfnEngineSurface_drawSetTextureBGRA);
		g_pMetaHookAPI->VFTHook(enginesurface_HL25, 0, gPrivateFuncs.index_enginesurface_drawUpdateRegionTextureBGRA, (void*)enginesurface_drawUpdateRegionTextureBGRA, (void**)&m_pfnEngineSurface_drawUpdateRegionTextureBGRA);
	}
	else
	{
		PVOID* pVFTable = *(PVOID**)&enginesurface;

		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_pushMakeCurrent, (void*)enginesurface_pushMakeCurrent, (void**)&m_pfnEngineSurface_pushMakeCurrent);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_popMakeCurrent, (void*)enginesurface_popMakeCurrent, (void**)&m_pfnEngineSurface_popMakeCurrent);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawFilledRect, (void*)enginesurface_drawFilledRect, (void**)&m_pfnEngineSurface_drawFilledRect);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawOutlinedRect, (void*)enginesurface_drawOutlinedRect, (void**)&m_pfnEngineSurface_drawOutlinedRect);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawLine, (void*)enginesurface_drawLine, (void**)&m_pfnEngineSurface_drawLine);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawPolyLine, (void*)enginesurface_drawPolyLine, (void**)&m_pfnEngineSurface_drawPolyLine);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawSetTextureRGBA, (void*)enginesurface_drawSetTextureRGBA, (void**)&m_pfnEngineSurface_drawSetTextureRGBA);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawSetTexture, (void*)enginesurface_drawSetTexture, (void**)&m_pfnEngineSurface_drawSetTexture);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawTexturedRect, (void*)enginesurface_drawTexturedRect, (void**)&m_pfnEngineSurface_drawTexturedRect);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_createNewTextureID, (void*)enginesurface_createNewTextureID, (void**)&m_pfnEngineSurface_createNewTextureID);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawPrintCharAdd, (void*)enginesurface_drawPrintCharAdd, (void**)&m_pfnEngineSurface_drawPrintCharAdd);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawSetTextureFile, (void*)enginesurface_drawSetTextureFile, (void**)&m_pfnEngineSurface_drawSetTextureFile);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawGetTextureSize, (void*)enginesurface_drawGetTextureSize, (void**)&m_pfnEngineSurface_drawGetTextureSize);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_isTextureIDValid, (void*)enginesurface_isTextureIDValid, (void**)&m_pfnEngineSurface_isTextureIDValid);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawSetSubTextureRGBA, (void*)enginesurface_drawSetSubTextureRGBA, (void**)&m_pfnEngineSurface_drawSetSubTextureRGBA);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawFlushText, (void*)enginesurface_drawFlushText, (void**)&m_pfnEngineSurface_drawFlushText);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawSetTextureBGRA, (void*)enginesurface_drawSetTextureBGRA, (void**)&m_pfnEngineSurface_drawSetTextureBGRA);
		g_pMetaHookAPI->VFTHook(enginesurface, 0, gPrivateFuncs.index_enginesurface_drawUpdateRegionTextureBGRA, (void*)enginesurface_drawUpdateRegionTextureBGRA, (void**)&m_pfnEngineSurface_drawUpdateRegionTextureBGRA);
	}
}

void EngineSurface_UninstallHooks(void)
{

}