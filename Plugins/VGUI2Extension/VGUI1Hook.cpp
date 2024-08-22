#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/IMouseControl.h>
#include <vgui/Cursor.h>
#include <vgui.h>
#include <VGUI_controls/Controls.h>
#include "plugins.h"
#include "privatefuncs.h"
#include "exportfuncs.h"
#include "VGUI1_AIO.h"

bool ClientVGUI_UseVGUI1();
bool GameUI_HasExclusiveInput();
void __fastcall EngineSurfaceWrap_WndProcHandler(void* pthis, int, void* hwnd, unsigned int msg, unsigned int wparam, long lparam);
void __fastcall EngineSurfaceWrap_AppHandler(void* pthis, int, void* pevent, void *pdata);
void __fastcall EngineSurfaceWrap_setCursor(void* pthis, int, void *pCursor);
void __fastcall EngineSurfaceWrap_lockCursor(void* pthis, int);
void __fastcall EngineSurfaceWrap_unlockCursor(void* pthis, int);

HMODULE g_hVGui1 = NULL;

//VGUI1 EngineSurfaceWrapper
void* staticEngineSurface = NULL;

static hook_t* g_phook_vgui_TextImage_paint = NULL;
static hook_t* g_phook_EngineSurfaceWrap_WndProcHandler = NULL;
static hook_t* g_phook_EngineSurfaceWrap_AppHandler = NULL;
static hook_t* g_phook_EngineSurfaceWrap_setCursor = NULL;
static hook_t* g_phook_EngineSurfaceWrap_lockCursor = NULL;
static hook_t* g_phook_EngineSurfaceWrap_unlockCursor = NULL;

static decltype(EngineSurfaceWrap_WndProcHandler)* m_pfnEngineSurfaceWrap_WndProcHandler = NULL;
static decltype(EngineSurfaceWrap_AppHandler)* m_pfnEngineSurfaceWrap_AppHandler = NULL;
static decltype(EngineSurfaceWrap_setCursor)* m_pfnEngineSurfaceWrap_setCursor = NULL;
static decltype(EngineSurfaceWrap_lockCursor)* m_pfnEngineSurfaceWrap_lockCursor = NULL;
static decltype(EngineSurfaceWrap_unlockCursor)* m_pfnEngineSurfaceWrap_unlockCursor = NULL;

void __fastcall vgui_TextImage_paint(vgui1_TextImage* pthis, int, void* panel)
{
	vgui1_ITextImage* pthis2 = (vgui1_ITextImage*)pthis;

	int wide, tall;
	pthis2->getSize(wide, tall);
	if (pthis->text)
	{
		qboolean isNonANSI = false;

		auto p = pthis->text;
		while (*p)
		{
			if ((*p) < 0 || (*p) > 127) {
				isNonANSI = true;
				break;
			}
			p++;
		}

		if (isNonANSI)
		{
			vgui1_Color color;
			color.vftable = gPrivateFuncs.vftable_vgui1_Color;
			pthis2->getColor(&color);

			int r, g, b, a;
			vgui1_IColor* pColor = (vgui1_IColor*)&color;
			pColor->getColor(r, g, b, a);

			pthis2->drawSetTextColor(r, g, b, a);

			auto font = pthis2->getFont();
			int fontTall = font->getTall();

			vgui::HFont hFont = NULL;

			const char* fontName = "SmallScoreboardFont";

			if (fontTall >= 32)
				fontName = "LargeScoreboardFont";
			else if (fontTall >= 24)
				fontName = "MediumScoreboardFont";

			vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("CaptionScheme"));
			if (pScheme)
			{
				hFont = pScheme->GetFont(fontName, true);
			}
			else
			{
				pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetDefaultScheme());
				if (pScheme)
				{
					hFont = pScheme->GetFont(fontName, true);
				}
			}
			if (hFont)
			{
				vgui::surface()->DrawSetTexture(0);
				vgui::surface()->DrawSetTextColor(r, g, b, 255);
				vgui::surface()->DrawSetTextPos(0, 0);
				vgui::surface()->DrawSetTextFont(hFont);

				wchar_t wszText[256] = { 0 };
				vgui::localize()->ConvertANSIToUnicode(pthis->text, wszText, sizeof(wszText));

				vgui::surface()->DrawPrintText(wszText, wcslen(wszText));
				vgui::surface()->DrawFlushText();
				vgui::surface()->DrawSetTexture(0);
				return;
			}
		}
	}
	return gPrivateFuncs.vgui_TextImage_paint(pthis, 0, panel);
}

class vgui1_EngineSurfaceWrapLegacyProxy : public vgui1_EngineSurfaceWrapLegacy
{
public:
	void* getPanel() override { return nullptr; }
	void   requestSwap() override { }
	void   resetModeInfo() override { }
	int    getModeInfoCount() override { return 0; }
	bool   getModeInfo(int mode, int& wide, int& tall, int& bpp) override { return false; }
	void* getApp() override { return nullptr; }
	void   setEmulatedCursorVisible(bool state) override { }
	void   setEmulatedCursorPos(int x, int y) override { }
	void setTitle(const char* title) override { }
	bool setFullscreenMode(int wide, int tall, int bpp) override { return false; }
	void setWindowedMode() override { }
	void setAsTopMost(bool state) override { }
	void createPopup(void* embeddedPanel) override { }
	bool hasFocus() override { return false; }
	bool isWithin(int x, int y) override { return false; }
	int  createNewTextureID(void) override { return 0; }
	void addModeInfo(int wide, int tall, int bpp) override { }
	void drawSetColor(int r, int g, int b, int a) override { }
	void drawFilledRect(int x0, int y0, int x1, int y1) override { }
	void drawOutlinedRect(int x0, int y0, int x1, int y1) override { }
	void drawSetTextFont(void* font) override { }
	void drawSetTextColor(int r, int g, int b, int a) override { }
	void drawSetTextPos(int x, int y) override { }
	void drawPrintText(const char* text, int textLen) override { }
	void drawSetTextureRGBA(int id, const char* rgba, int wide, int tall) override { }
	void drawSetTexture(int id) override { }
	void drawTexturedRect(int x0, int y0, int x1, int y1) override { }
	void invalidate(void* panel) override { }
	void enableMouseCapture(bool state) override { }
	void setCursor(void* cursor) override {

		if (!ClientVGUI_UseVGUI1())
		{
			m_pfnEngineSurfaceWrap_setCursor(this, 0, cursor);
			return;
		}

		if (GameUI_HasExclusiveInput())
			return;

		if (g_pVGuiSurface2->IsCursorVisible())
			return;

		g_pVGuiSurface2->SetVGUI2MouseControl(false);
		m_pfnEngineSurfaceWrap_setCursor(this, 0, cursor);
		g_pVGuiSurface2->SetVGUI2MouseControl(true);
		return;
	}
	void swapBuffers() override { }
	void pushMakeCurrent(void* panel, bool useInsets) override { }
	void popMakeCurrent(void* panel) override { }
	void applyChanges() override { }
	void WndProcHandler(void* hwnd, unsigned int msg, unsigned int wparam, long lparam) override {
		//VGUI1 SDL event handler, TODO: non-SDL version?

		if (!ClientVGUI_UseVGUI1())
		{
			m_pfnEngineSurfaceWrap_WndProcHandler(this, 0, hwnd, msg, wparam, lparam);
			return;
		}

		if (GameUI_HasExclusiveInput())
			return;

		if (g_pVGuiSurface2->IsCursorVisible())
			return;

		m_pfnEngineSurfaceWrap_WndProcHandler(this, 0, hwnd, msg, wparam, lparam);
	}
	void lockCursor() override {
		//g_pVGuiSurface2->LockCursor();
	}
	void unlockCursor() override {
		//g_pVGuiSurface2->UnlockCursor();
	}
	void drawLine(int x1, int y1, int x2, int y2) override { }
	void drawPolyLine(int* px, int* py, int n) override { }
	void drawTexturedPolygon(void* pVertices, int n) override { }
	void drawSetTextureBGRA(int id, const char* rgba, int wide, int tall, int hardwareFilter, int hasAlphaChannel) override { }
	void drawUpdateRegionTextureBGRA(int nTextureID, int x, int y, const unsigned char* pchData, int wide, int tall) override { }
	void drawGetTextPos(int& x, int& y) override { }
	void drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override { }
	void drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override { }
};

class vgui1_EngineSurfaceWrapProxy : public vgui1_EngineSurfaceWrap
{
public:
	void* getPanel() override { return nullptr; }
	void   requestSwap() override { }
	void   resetModeInfo() override { }
	int    getModeInfoCount() override { return 0; }
	bool   getModeInfo(int mode, int& wide, int& tall, int& bpp) override { return false; }
	void* getApp() override { return nullptr; }
	void   setEmulatedCursorVisible(bool state) override { }
	void   setEmulatedCursorPos(int x, int y) override { }
	void setTitle(const char* title) override { }
	bool setFullscreenMode(int wide, int tall, int bpp) override { return false; }
	void setWindowedMode() override { }
	void setAsTopMost(bool state) override { }
	void createPopup(void* embeddedPanel) override { }
	bool hasFocus() override { return false; }
	bool isWithin(int x, int y) override { return false; }
	int  createNewTextureID(void) override { return 0; }
	void GetMousePos(int& x, int& y) override { }
	void addModeInfo(int wide, int tall, int bpp) override { }
	void drawSetColor(int r, int g, int b, int a) override { }
	void drawFilledRect(int x0, int y0, int x1, int y1) override { }
	void drawOutlinedRect(int x0, int y0, int x1, int y1) override { }
	void drawSetTextFont(void* font) override { }
	void drawSetTextColor(int r, int g, int b, int a) override { }
	void drawSetTextPos(int x, int y) override { }
	void drawPrintText(const char* text, int textLen) override { }
	void drawSetTextureRGBA(int id, const char* rgba, int wide, int tall) override { }
	void drawSetTexture(int id) override { }
	void drawTexturedRect(int x0, int y0, int x1, int y1) override { }
	void invalidate(void* panel) override { }
	void enableMouseCapture(bool state) override { }
	void setCursor(void* cursor) override {

		if (!ClientVGUI_UseVGUI1())
		{
			m_pfnEngineSurfaceWrap_setCursor(this, 0, cursor);
			return;
		}

		if (GameUI_HasExclusiveInput())
			return;

		if (g_pVGuiSurface2->IsCursorVisible())
			return;

		g_pVGuiSurface2->SetVGUI2MouseControl(false);
		m_pfnEngineSurfaceWrap_setCursor(this, 0, cursor);
		g_pVGuiSurface2->SetVGUI2MouseControl(true);
		return;
	}
	void swapBuffers() override { }
	void pushMakeCurrent(void* panel, bool useInsets) override { }
	void popMakeCurrent(void* panel) override { }
	void applyChanges() override { }
	void AppHandler(void* pevent, void* userData) override {
		//VGUI1 SDL event handler, TODO: non-SDL version?

		if (!ClientVGUI_UseVGUI1())
		{
			m_pfnEngineSurfaceWrap_AppHandler(this, 0, pevent, userData);
			return;
		}

		if (GameUI_HasExclusiveInput())
			return;

		if (g_pVGuiSurface2->IsCursorVisible())
			return;

		m_pfnEngineSurfaceWrap_AppHandler(this, 0, pevent, userData);

	}
	void lockCursor() override {
		//g_pVGuiSurface2->LockCursor();
	}
	void unlockCursor() override { 
		//g_pVGuiSurface2->UnlockCursor();
	}
	void drawLine(int x1, int y1, int x2, int y2) override { }
	void drawPolyLine(int* px, int* py, int n) override { }
	void drawTexturedPolygon(void* pVertices, int n) override { }
	void drawSetTextureBGRA(int id, const char* rgba, int wide, int tall, int hardwareFilter, int hasAlphaChannel) override { }
	void drawUpdateRegionTextureBGRA(int nTextureID, int x, int y, const unsigned char* pchData, int wide, int tall) override { }
	void drawGetTextPos(int& x, int& y) override { }
	void drawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override { }
	void drawPrintCharAdd(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1) override { }
};

static vgui1_EngineSurfaceWrapProxy s_EngineSurfaceWrapProxy;

static vgui1_EngineSurfaceWrapLegacyProxy s_EngineSurfaceWrapLegacyProxy;

void VGUI1_SetCursor(vgui1_Scheme::SchemeCursor SchemeCursor)
{
	auto pAppInstance = (vgui1_App *)gPrivateFuncs.vgui_App_getInstance();

	if (!pAppInstance)
		return;

	auto pScheme = (vgui1_Scheme *)pAppInstance->getScheme();

	auto pCursor = (vgui1_Cursor *)pScheme->getCursor(SchemeCursor);

	auto pStaticEngineSurface = *(vgui1_EngineSurfaceWrap**)staticEngineSurface;

	if (!pStaticEngineSurface)
		return;

	pStaticEngineSurface->setCursor(pCursor);
}

void VGUI1_LockCursor()
{
	auto pStaticEngineSurface = *(vgui1_EngineSurfaceWrap**)staticEngineSurface;

	if (!pStaticEngineSurface)
		return;

	pStaticEngineSurface->lockCursor();
}

void VGUI1_UnlockCursor()
{
	auto pStaticEngineSurface = *(vgui1_EngineSurfaceWrap**)staticEngineSurface;

	if (!pStaticEngineSurface)
		return;

	pStaticEngineSurface->unlockCursor();
}

void VGUI1_PostInstallHooks(void)
{
	if (!staticEngineSurface)
		return;

	if (gPrivateFuncs.SDL_GetWindowPosition)
	{
		auto pStaticEngineSurface = *(vgui1_EngineSurfaceWrap**)staticEngineSurface;

		if (!pStaticEngineSurface)
			return;

		PVOID* ProxyVFTable = *(PVOID**)&s_EngineSurfaceWrapProxy;

		g_phook_EngineSurfaceWrap_setCursor = g_pMetaHookAPI->VFTHook(pStaticEngineSurface, 0, 30, (void*)ProxyVFTable[30], (void**)&m_pfnEngineSurfaceWrap_setCursor);
		g_phook_EngineSurfaceWrap_AppHandler = g_pMetaHookAPI->VFTHook(pStaticEngineSurface, 0, 35, (void*)ProxyVFTable[35], (void**)&m_pfnEngineSurfaceWrap_AppHandler);
	}
	else
	{
		auto pStaticEngineSurfaceLegacy = *(vgui1_EngineSurfaceWrapLegacy**)staticEngineSurface;

		if (!pStaticEngineSurfaceLegacy)
			return;

		PVOID* ProxyVFTable = *(PVOID**)&s_EngineSurfaceWrapLegacyProxy;

		g_phook_EngineSurfaceWrap_setCursor = g_pMetaHookAPI->VFTHook(pStaticEngineSurfaceLegacy, 0, 29, (void*)ProxyVFTable[29], (void**)&m_pfnEngineSurfaceWrap_setCursor);
		g_phook_EngineSurfaceWrap_WndProcHandler = g_pMetaHookAPI->VFTHook(pStaticEngineSurfaceLegacy, 0, 34, (void*)ProxyVFTable[34], (void**)&m_pfnEngineSurfaceWrap_WndProcHandler);
	}
}

void VGUI1_InstallHooks(void)
{
	g_hVGui1 = GetModuleHandleA("vgui.dll");

	if (g_hVGui1)
	{
		gPrivateFuncs.vftable_vgui1_TextImage = (decltype(gPrivateFuncs.vftable_vgui1_TextImage))GetProcAddress(g_hVGui1, "??_7TextImage@vgui@@6B@");
		gPrivateFuncs.vftable_vgui1_Color = (decltype(gPrivateFuncs.vftable_vgui1_Color))GetProcAddress(g_hVGui1, "??_7Color@vgui@@6B@");
		gPrivateFuncs.vgui_App_getInstance = (decltype(gPrivateFuncs.vgui_App_getInstance))GetProcAddress(g_hVGui1, "?getInstance@App@vgui@@SAPAV12@XZ");
		gPrivateFuncs.vgui_TextImage_paint = (decltype(gPrivateFuncs.vgui_TextImage_paint))gPrivateFuncs.vftable_vgui1_TextImage[22];
	}

	Install_InlineHook(vgui_TextImage_paint);
}

void VGUI1_Shutdown(void)
{
	Uninstall_Hook(vgui_TextImage_paint);
	Uninstall_Hook(EngineSurfaceWrap_WndProcHandler);
	Uninstall_Hook(EngineSurfaceWrap_AppHandler);
	Uninstall_Hook(EngineSurfaceWrap_setCursor);
	//Uninstall_Hook(EngineSurfaceWrap_lockCursor);
	//Uninstall_Hook(EngineSurfaceWrap_unlockCursor);
	g_hVGui1 = NULL;
}
