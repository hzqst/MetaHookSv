#pragma once

#include <stdint.h>
#include <memory>

class vgui1_IColor
{
public:
	virtual void init() = 0;
	virtual void setColor(int r, int g, int b, int a) = 0;
	virtual void setColor(int sc) = 0;
	virtual void getColor(int& r, int& g, int& b, int& a) = 0;
	virtual void getColor(int* sc) = 0;
};

class vgui1_Color
{
public:
	void** vftable;
	unsigned char color[4];
	int  schemeColor;
public:
	vgui1_Color()
	{
		vftable = nullptr;
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
		color[3] = 0;
		schemeColor = 0;
	}
};

class vgui1_IFont
{
public:
	virtual void init(const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol) = 0;
	virtual void getCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba) = 0;
	virtual void getCharABCwide(int ch, int& a, int& b, int& c) = 0;
	virtual void getTextSize(const char* text, int& wide, int& tall) = 0;
	virtual int  getTall() = 0;
	virtual int  getId() = 0;
};

class vgui1_ITextImage
{
public:
	virtual void setPos(int x, int y) = 0;
	virtual void getPos(int& x, int& y) = 0;//+4
	virtual void getSize(int& wide, int& tall) = 0;//+8
	virtual void setColor(vgui1_Color& color) = 0;
	virtual void getColor(vgui1_Color* color) = 0;//+16
	virtual void setSize(int wide, int tall) = 0;
	virtual void drawSetColor(int sc) = 0;
	virtual void drawSetColor(int r, int g, int b, int a) = 0;
	virtual void drawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawSetTextFont(int sf) = 0;
	virtual void drawSetTextFont(vgui1_IFont* font) = 0;
	virtual void drawSetTextColor(int* sc) = 0;
	virtual void drawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void drawSetTextPos(int x, int y) = 0;
	virtual void drawPrintText(const char* str, int strlen) = 0;
	virtual void drawPrintText(int x, int y, const char* str, int strlen) = 0;
	virtual void drawPrintChar(char ch) = 0;
	virtual void drawPrintChar(int x, int y, char ch) = 0;
	virtual void drawSetTextureRGBA(int id, const char* rgba, int wide, int tall) = 0;
	virtual void drawSetTexture(int id) = 0;
	virtual void drawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void paint(void* panel) = 0;
	virtual void doPaint(void* panel) = 0;
	virtual void  init(int textBufferLen, const char* text) = 0;
	virtual void  getTextSize(int& wide, int& tall) = 0;
	virtual void  getTextSizeWrapped(int& wide, int& tall) = 0;
	virtual vgui1_IFont* getFont() = 0;//+108
	virtual void  setText(int textBufferLen, const char* text) = 0;
	virtual void  setText(const char* text) = 0;
	virtual void  setFont(int schemeFont) = 0;
	virtual void  setFont(vgui1_IFont* font) = 0;
};

class vgui1_TextImage
{
public:
	void** vftable;
	char padding[0x20];
	char* text;

public:
	vgui1_TextImage()
	{
		vftable = NULL;
		memset(padding, 0, sizeof(padding));
		text = NULL;
	}
};

//non-SDL engine

class vgui1_SurfaceBaseLegacy
{
public:
	virtual void* getPanel() = 0;
	virtual void   requestSwap() = 0;
	virtual void   resetModeInfo() = 0;
	virtual int    getModeInfoCount() = 0;
	virtual bool   getModeInfo(int mode, int& wide, int& tall, int& bpp) = 0;
	virtual void* getApp() = 0;
	virtual void   setEmulatedCursorVisible(bool state) = 0;
	virtual void   setEmulatedCursorPos(int x, int y) = 0;
	virtual void setTitle(const char* title) = 0;
	virtual bool setFullscreenMode(int wide, int tall, int bpp) = 0;
	virtual void setWindowedMode() = 0;
	virtual void setAsTopMost(bool state) = 0;
	virtual void createPopup(void* embeddedPanel) = 0;
	virtual bool hasFocus() = 0;
	virtual bool isWithin(int x, int y) = 0;
	virtual int  createNewTextureID(void) = 0;
	virtual void addModeInfo(int wide, int tall, int bpp) = 0;
	virtual void drawSetColor(int r, int g, int b, int a) = 0;
	virtual void drawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawSetTextFont(void* font) = 0;
	virtual void drawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void drawSetTextPos(int x, int y) = 0;
	virtual void drawPrintText(const char* text, int textLen) = 0;
	virtual void drawSetTextureRGBA(int id, const char* rgba, int wide, int tall) = 0;
	virtual void drawSetTexture(int id) = 0;
	virtual void drawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void invalidate(void* panel) = 0;
	virtual void enableMouseCapture(bool state) = 0;
	virtual void setCursor(void* cursor) = 0;
	virtual void swapBuffers() = 0;
	virtual void pushMakeCurrent(void* panel, bool useInsets) = 0;
	virtual void popMakeCurrent(void* panel) = 0;
	virtual void applyChanges() = 0;
};

class vgui1_EngineSurfaceWrapLegacy : public vgui1_SurfaceBaseLegacy
{
public:
	virtual void WndProcHandler(void *hwnd, unsigned int msg, unsigned int wparam, long lparam) = 0;
	virtual void lockCursor() = 0;
	virtual void unlockCursor() = 0;
	virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
	virtual void drawPolyLine(int* px, int* py, int n) = 0;
	virtual void drawTexturedPolygon(void* pVertices, int n) = 0;
	virtual void drawSetTextureBGRA(int id, const char* rgba, int wide, int tall, int hardwareFilter, int hasAlphaChannel) = 0;
	virtual void drawUpdateRegionTextureBGRA(int nTextureID, int x, int y, const unsigned char* pchData, int wide, int tall) = 0;
	virtual void drawGetTextPos(int& x, int& y) = 0;
	virtual void drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) = 0;
	virtual void drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) = 0;
};

//SDL-capable engine

class vgui1_SurfaceBase
{
public:
	virtual void* getPanel() = 0;
	virtual void   requestSwap() = 0;
	virtual void   resetModeInfo() = 0;
	virtual int    getModeInfoCount() = 0;
	virtual bool   getModeInfo(int mode, int& wide, int& tall, int& bpp) = 0;
	virtual void* getApp() = 0;
	virtual void   setEmulatedCursorVisible(bool state) = 0;
	virtual void   setEmulatedCursorPos(int x, int y) = 0;
	virtual void setTitle(const char* title) = 0;
	virtual bool setFullscreenMode(int wide, int tall, int bpp) = 0;
	virtual void setWindowedMode() = 0;
	virtual void setAsTopMost(bool state) = 0;
	virtual void createPopup(void* embeddedPanel) = 0;
	virtual bool hasFocus() = 0;
	virtual bool isWithin(int x, int y) = 0;
	virtual int  createNewTextureID(void) = 0;
	virtual void GetMousePos(int& x, int& y) = 0;
	virtual void addModeInfo(int wide, int tall, int bpp) = 0;
	virtual void drawSetColor(int r, int g, int b, int a) = 0;
	virtual void drawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void drawSetTextFont(void* font) = 0;
	virtual void drawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void drawSetTextPos(int x, int y) = 0;
	virtual void drawPrintText(const char* text, int textLen) = 0;
	virtual void drawSetTextureRGBA(int id, const char* rgba, int wide, int tall) = 0;
	virtual void drawSetTexture(int id) = 0;
	virtual void drawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void invalidate(void* panel) = 0;
	virtual void enableMouseCapture(bool state) = 0;
	virtual void setCursor(void* cursor) = 0;
	virtual void swapBuffers() = 0;
	virtual void pushMakeCurrent(void* panel, bool useInsets) = 0;
	virtual void popMakeCurrent(void* panel) = 0;
	virtual void applyChanges() = 0;
};

class vgui1_EngineSurfaceWrap : public vgui1_SurfaceBase
{
public:
	virtual void AppHandler(void* event, void* userData) = 0;
	virtual void lockCursor() = 0;
	virtual void unlockCursor() = 0;
	virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
	virtual void drawPolyLine(int* px, int* py, int n) = 0;
	virtual void drawTexturedPolygon(void* pVertices, int n) = 0;
	virtual void drawSetTextureBGRA(int id, const char* rgba, int wide, int tall, int hardwareFilter, int hasAlphaChannel) = 0;
	virtual void drawUpdateRegionTextureBGRA(int nTextureID, int x, int y, const unsigned char* pchData, int wide, int tall) = 0;
	virtual void drawGetTextPos(int& x, int& y) = 0;
	virtual void drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) = 0;
	virtual void drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) = 0;
};

class vgui1_App
{
public:
	virtual void    start() = 0;
	virtual void    stop() = 0;
	virtual void    externalTick() = 0;
	virtual bool    wasMousePressed(int code, void* panel) = 0;
	virtual bool    wasMouseDoublePressed(int code, void* panel) = 0;
	virtual bool    isMouseDown(int code, void* panel) = 0;
	virtual bool    wasMouseReleased(int code, void* panel) = 0;
	virtual bool    wasKeyPressed(int code, void* panel) = 0;
	virtual bool    isKeyDown(int code, void* panel) = 0;
	virtual bool    wasKeyTyped(int code, void* panel) = 0;
	virtual bool    wasKeyReleased(int code, void* panel) = 0;
	virtual void    addTickSignal(void* s) = 0;
	virtual void    setCursorPos(int x, int y) = 0;
	virtual void    getCursorPos(int& x, int& y) = 0;
	virtual void    setMouseCapture(void* panel) = 0;
	virtual void    setMouseArena(int x0, int y0, int x1, int y1, bool enabled) = 0;
	virtual void    setMouseArena(void* panel) = 0;
	virtual void    requestFocus(void* panel) = 0;
	virtual void* getFocus() = 0;
	virtual void    repaintAll() = 0;
	virtual void    setScheme(void* scheme) = 0;
	virtual void* getScheme() = 0;
	virtual void    enableBuildMode() = 0;
	virtual long    getTimeMillis() = 0;
	virtual char    getKeyCodeChar(int code, bool shifted) = 0;
	virtual void    getKeyCodeText(int code, char* buf, int buflen) = 0;
	virtual int     getClipboardTextCount() = 0;
	virtual void    setClipboardText(const char* text, int textLen) = 0;
	virtual int     getClipboardText(int offset, char* buf, int bufLen) = 0;
	virtual void    reset() = 0;
	virtual void    internalSetMouseArena(int x0, int y0, int x1, int y1, bool enabled) = 0;
	virtual bool    setRegistryString(const char* key, const char* value) = 0;
	virtual bool    getRegistryString(const char* key, char* value, int valueLen) = 0;
	virtual bool    setRegistryInteger(const char* key, int value) = 0;
	virtual bool    getRegistryInteger(const char* key, int& value) = 0;
	virtual void    setCursorOveride(void* cursor) = 0;
	virtual void* getCursorOveride() = 0;
	virtual void    setMinimumTickMillisInterval(int interval) = 0;
public: //bullshit public stuff
	virtual void main(int argc, char* argv[]) = 0;
	virtual void run() = 0;
	virtual void internalCursorMoved(int x, int y, void* surfaceBase) = 0; //expects input in surface space
	virtual void internalMousePressed(int code, void* surfaceBase) = 0;
	virtual void internalMouseDoublePressed(int code, void* surfaceBase) = 0;
	virtual void internalMouseReleased(int code, void* surfaceBase) = 0;
	virtual void internalMouseWheeled(int delta, void* surfaceBase) = 0;
	virtual void internalKeyPressed(int code, void* surfaceBase) = 0;
	virtual void internalKeyTyped(int code, void* surfaceBase) = 0;
	virtual void internalKeyReleased(int code, void* surfaceBase) = 0;
private:
	virtual void init() = 0;
	virtual void updateMouseFocus(int x, int y, void* surfaceBase) = 0;
	virtual void setMouseFocus(void* newMouseFocus) = 0;
protected:
	virtual void surfaceBaseCreated(void* surfaceBase) = 0;
	virtual void surfaceBaseDeleted(void* surfaceBase) = 0;
	virtual void platTick() = 0;
	virtual void internalTick() = 0;
};

class vgui1_Scheme
{
public:
	enum SchemeColor
	{
		sc_user = 0,
		sc_black,
		sc_white,
		sc_primary1,
		sc_primary2,
		sc_primary3,
		sc_secondary1,
		sc_secondary2,
		sc_secondary3,
		sc_last,
	};
	enum SchemeFont
	{
		sf_user = 0,
		sf_primary1,
		sf_primary2,
		sf_primary3,
		sf_secondary1,
		sf_last,
	};
	enum SchemeCursor
	{
		scu_user = 0,
		scu_none,
		scu_arrow,
		scu_ibeam,
		scu_hourglass,
		scu_crosshair,
		scu_up,
		scu_sizenwse,
		scu_sizenesw,
		scu_sizewe,
		scu_sizens,
		scu_sizeall,
		scu_no,
		scu_hand,
		scu_last,
	};
public:
	virtual void    setColor(SchemeColor sc, int r, int g, int b, int a);
	virtual void    getColor(SchemeColor sc, int& r, int& g, int& b, int& a);
	virtual void    setFont(SchemeFont sf, void* font);
	virtual void* getFont(SchemeFont sf);
	virtual void    setCursor(SchemeCursor sc, void* cursor);
	virtual void* getCursor(SchemeCursor sc);
};

class vgui1_Cursor
{
public:
	enum DefaultCursor
	{
		dc_user,
		dc_none,
		dc_arrow,
		dc_ibeam,
		dc_hourglass,
		dc_crosshair,
		dc_up,
		dc_sizenwse,
		dc_sizenesw,
		dc_sizewe,
		dc_sizens,
		dc_sizeall,
		dc_no,
		dc_hand,
		dc_last,
	};
public:
	virtual void getHotspot(int& x, int& y);
private:
	virtual void privateInit(void* bitmap, int hotspotX, int hotspotY);
public:
	virtual void* getBitmap();
	virtual DefaultCursor getDefaultCursor();
};