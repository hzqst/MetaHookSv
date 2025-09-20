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

PVOID** g_SDL2_mainwindow = NULL;

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

void EngineSurface_FillAddress(void);
void EngineSurface_InstallHooks(void);
void EngineSurface_UninstallHooks(void);

//Non-HL25

void SDL_GetWindowSize(void* window, int* w, int* h);

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

void EngineSurface_FillAddress(void)
{

}

void EngineSurface_InstallHooks(void)
{

}

void EngineSurface_UninstallHooks(void)
{

}