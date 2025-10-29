#include <metahook.h>
#include <capstone.h>
#include <IEngineSurface.h>
#include "plugins.h"
#include "privatefuncs.h"

#include <cstdlib>

extern IEngineSurface *staticSurface;
extern IEngineSurface_HL25 *staticSurface_HL25;

static void (*m_pfnEngineSurface_pushMakeCurrent)(int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace) = NULL;
static void (*m_pfnEngineSurface_popMakeCurrent)(void) = NULL;

bool* g_bScissor = NULL;
RECT* g_ScissorRect = NULL;

void** (*pmainwindow) = nullptr;

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

void EngineSurface_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void EngineSurface_InstallHooks(void);
void EngineSurface_UninstallHooks(void);

//Non-HL25

void CEngineSurfaceProxy::pushMakeCurrent(int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace)
{
	return m_pfnEngineSurface_pushMakeCurrent(insets, absExtents, clipRect, translateToScreenSpace);
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

void EngineSurface_FillAddress_pushMakeCurrent(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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
}

void EngineSurface_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto engineFactory = g_pMetaHookAPI->GetEngineFactory();

	if (engineFactory)
	{
#define ENGINE_SURFACE_VERSION "EngineSurface007"
		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			staticSurface_HL25 = (decltype(staticSurface_HL25))engineFactory(ENGINE_SURFACE_VERSION, NULL);

			auto engineSurface_vftable = *(PVOID**)staticSurface_HL25;

			gPrivateFuncs.index_enginesurface_pushMakeCurrent = 1;

			gPrivateFuncs.enginesurface_pushMakeCurrent = (decltype(gPrivateFuncs.enginesurface_pushMakeCurrent))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_pushMakeCurrent, DllInfo, RealDllInfo, RealDllInfo);

		}
		else
		{
			staticSurface = (decltype(staticSurface))engineFactory(ENGINE_SURFACE_VERSION, NULL);

			auto engineSurface_vftable = *(PVOID**)staticSurface;

			gPrivateFuncs.index_enginesurface_pushMakeCurrent = 1;

			gPrivateFuncs.enginesurface_pushMakeCurrent = (decltype(gPrivateFuncs.enginesurface_pushMakeCurrent))GetVFunctionFromVFTable(engineSurface_vftable, gPrivateFuncs.index_enginesurface_pushMakeCurrent, DllInfo, RealDllInfo, RealDllInfo);
		}

		EngineSurface_FillAddress_pushMakeCurrent(DllInfo, RealDllInfo);
	}
}

void EngineSurface_InstallHooks(void)
{

}

void EngineSurface_UninstallHooks(void)
{

}