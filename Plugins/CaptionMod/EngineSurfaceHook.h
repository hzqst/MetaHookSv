#include <IEngineSurface.h>

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