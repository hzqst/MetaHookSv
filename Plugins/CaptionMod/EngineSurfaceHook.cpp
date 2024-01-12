#include <metahook.h>
#include <capstone.h>
#include <glew.h>
#include <cstdlib>
#include "EngineSurfaceHook.h"
#include "plugins.h"
#include "privatefuncs.h"

extern IEngineSurface *staticSurface;
extern IEngineSurface_HL25 *staticSurface_HL25;

static void (*m_pfnEngineSurface_pushMakeCurrent)(int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace) = NULL;
static void (*m_pfnEngineSurface_popMakeCurrent)(void) = NULL;

bool* g_bScissor = NULL;
RECT* g_ScissorRect = NULL;
PVOID** g_SDL2_mainwindow = NULL;

//Non-HL25

void SDL_GetWindowSize(void* window, int* w, int* h);

void CEngineSurfaceProxy::pushMakeCurrent(int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace)
{
	//return m_pfnEngineSurface_pushMakeCurrent(insets, absExtents, clipRect, translateToScreenSpace);

	POINT pnt = { 0 };
	RECT rect = {0};

	pnt.x = 0;
	pnt.y = 0;

	int iVideoWidth = 0, iVideoHeight = 0;
	bool bIsWindowed = false;
	g_pMetaHookAPI->GetVideoMode(&iVideoWidth, &iVideoHeight, NULL, &bIsWindowed);

	if (translateToScreenSpace && bIsWindowed)
	{
		auto mainwindow = (*g_SDL2_mainwindow);
		gPrivateFuncs.SDL_GetWindowPosition((*mainwindow), (int*)&pnt.x, (int*)&pnt.y);
	}
	else
	{
		pnt.x = 0;
		pnt.y = 0;
	}

	if (bIsWindowed)
	{
		auto mainwindow = (*g_SDL2_mainwindow);
		gPrivateFuncs.SDL_GetWindowSize((*mainwindow), (int*)&rect.right, (int*)&rect.bottom);
	}
	else
	{
		rect.right = iVideoWidth;
		rect.bottom = iVideoHeight;
	}

	int xTranslate = pnt.x;
	int yTranslate = pnt.y;

	int wide = rect.right;
	int tall = rect.bottom;

	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, wide, tall, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	int x0, y0, x1, y1;

	x0 = insets[0];
	y0 = insets[1];

	//Inner offset
	glTranslatef(x0, y0, 0);

	x1 = absExtents[0] - xTranslate;
	y1 = absExtents[1] - yTranslate;

	//Outer offset
	glTranslatef(x1, y1, 0);

	int surfaceAbsExtents[4];
	surfaceAbsExtents[0] = absExtents[0] - xTranslate;
	surfaceAbsExtents[1] = absExtents[1] - yTranslate;
	surfaceAbsExtents[2] = absExtents[2] - xTranslate;
	surfaceAbsExtents[3] = absExtents[3] - yTranslate;

	(*g_bScissor) = true;
	(*g_ScissorRect).left = clipRect[0] - xTranslate - (insets[0] + surfaceAbsExtents[0]);
	(*g_ScissorRect).top = clipRect[1] - yTranslate - (insets[1] + surfaceAbsExtents[1]);
	(*g_ScissorRect).right = clipRect[2] - xTranslate - (insets[0] + surfaceAbsExtents[0]);
	(*g_ScissorRect).bottom = clipRect[3] - yTranslate - (insets[1] + surfaceAbsExtents[1]);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void CEngineSurfaceProxy::popMakeCurrent(void)
{
	m_pfnEngineSurface_popMakeCurrent();
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

void CEngineSurfaceProxy::drawTexturedPolygon(int* p, int n)
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
	m_pfnEngineSurface_pushMakeCurrent(p1, p2, r, useInsets);
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

void EngineSurface_FillAddress(void)
{
#if 0
	PVOID* OriginalVFTable = (g_iEngineType == ENGINE_GOLDSRC_HL25) ? *(PVOID**)staticSurface_HL25 : *(PVOID**)staticSurface;

	auto pushMakeCurrent = OriginalVFTable[1];

	typedef struct
	{
		ULONG_PTR mainwindow_candidate;
		int mainwindow_candidate_reg;
		int mainwindow_candidate_InstCount;
		int g_bScissor_InstCount;
		ULONG_PTR candidate[4];
		int candidate_count;
	}pushMakeCurrent_ctx;

	pushMakeCurrent_ctx ctx = { 0 };

	g_pMetaHookAPI->DisasmRanges(pushMakeCurrent, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (pushMakeCurrent_ctx*)context;

		if (!g_SDL2_mainwindow &&
			instCount < 35 &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			pinst->detail->x86.operands[1].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
		{
			ctx->mainwindow_candidate = (decltype(ctx->mainwindow_candidate))pinst->detail->x86.operands[1].mem.disp;
			ctx->mainwindow_candidate_reg = pinst->detail->x86.operands[0].reg;
			ctx->mainwindow_candidate_InstCount = instCount;
		}

		if (!g_SDL2_mainwindow && 
			instCount < 40 && ctx->mainwindow_candidate &&
			instCount > ctx->mainwindow_candidate_InstCount &&
			instCount < ctx->mainwindow_candidate_InstCount + 6 &&			
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == ctx->mainwindow_candidate_reg)
		{
			g_SDL2_mainwindow = (decltype(g_SDL2_mainwindow))ctx->mainwindow_candidate;
		}

		if (!g_SDL2_mainwindow && 
			instCount < 40 && ctx->mainwindow_candidate &&
			instCount > ctx->mainwindow_candidate_InstCount && 
			instCount < ctx->mainwindow_candidate_InstCount + 6 &&			
			pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == ctx->mainwindow_candidate_reg)
		{
			g_SDL2_mainwindow = (decltype(g_SDL2_mainwindow))ctx->mainwindow_candidate;
		}

		if (!g_bScissor && pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 1 &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[0].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
		{
			g_bScissor = (decltype(g_bScissor))pinst->detail->x86.operands[0].mem.disp;
			ctx->g_bScissor_InstCount = instCount;
		}

		if (ctx->g_bScissor_InstCount > 0 && instCount > ctx->g_bScissor_InstCount && ctx->candidate_count < 4)
		{
			if(pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
		g_ScissorRect = (decltype(g_ScissorRect))ctx.candidate[0];
	}

	Sig_VarNotFound(g_SDL2_mainwindow);
	Sig_VarNotFound(g_bScissor);
	Sig_VarNotFound(g_ScissorRect);
#endif
}

void EngineSurface_InstallHooks(void)
{
#if 0
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		PVOID* ProxyVFTable = *(PVOID**)&g_EngineSurfaceProxy_HL25;

		//g_pMetaHookAPI->VFTHook(staticSurface_HL25, 0, 1, (void*)ProxyVFTable[1], (void**)&m_pfnEngineSurface_pushMakeCurrent);
		//g_pMetaHookAPI->VFTHook(staticSurface_HL25, 0, 2, (void*)ProxyVFTable[2], (void**)&m_pfnEngineSurface_popMakeCurrent);
	}
	else
	{
		PVOID* ProxyVFTable = *(PVOID**)&g_EngineSurfaceProxy;

		//g_pMetaHookAPI->VFTHook(staticSurface, 0, 1, (void*)ProxyVFTable[1], (void**)&m_pfnEngineSurface_pushMakeCurrent);
		//g_pMetaHookAPI->VFTHook(staticSurface, 0, 2, (void*)ProxyVFTable[2], (void**)&m_pfnEngineSurface_popMakeCurrent);
	}
#endif
}

void EngineSurface_UninstallHooks(void)
{

}