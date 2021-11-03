#include <metahook.h>
#include <VGUI/ISurface.h>
#include "FontTextureCache.h"
#include <IEngineSurface.h>
#include "Color.h"
#include "vgui_internal.h"
#include "plugins.h"
#include "tier1/utlrbtree.h"
#include "tier1/UtlDict.h"
#include "privatefuncs.h"
#include "Cursor.h"

extern IEngineSurface *staticSurface;

extern vgui::ISurface *g_pSurface;

using namespace vgui;

class CSurfaceProxy : public ISurface
{
public:
	virtual void Shutdown(void);
	virtual void RunFrame(void);
	virtual VPANEL GetEmbeddedPanel(void);
	virtual void SetEmbeddedPanel(VPANEL pPanel);
	virtual void PushMakeCurrent(VPANEL panel, bool useInsets);
	virtual void PopMakeCurrent(VPANEL panel);
	virtual void DrawSetColor(int r, int g, int b, int a);
	virtual void DrawSetColor(Color col);
	virtual void DrawFilledRect(int x0, int y0, int x1, int y1);
	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1);
	virtual void DrawLine(int x0, int y0, int x1, int y1);
	virtual void DrawPolyLine(int *px, int *py, int numPoints);
	virtual void DrawSetTextFont(HFont font);
	virtual void DrawSetTextColor(int r, int g, int b, int a);
	virtual void DrawSetTextColor(Color col);
	virtual void DrawSetTextPos(int x, int y);
	virtual void DrawGetTextPos(int &x, int &y);
	virtual void DrawPrintText(const wchar_t *text, int textLen);
	virtual void DrawUnicodeChar(wchar_t wch);
	virtual void DrawUnicodeCharAdd(wchar_t wch);
	virtual void DrawFlushText(void);
	virtual IHTML *CreateHTMLWindow(IHTMLEvents *events, VPANEL context);
	virtual void PaintHTMLWindow(IHTML *htmlwin);
	virtual void DeleteHTMLWindow(IHTML *htmlwin);
	virtual void DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload);
	virtual void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload);
	virtual void DrawSetTexture(int id);
	virtual void DrawGetTextureSize(int id, int &wide, int &tall);
	virtual void DrawTexturedRect(int x0, int y0, int x1, int y1);
	virtual bool IsTextureIDValid(int id);
	virtual int CreateNewTextureID(bool procedural = false);
	virtual void GetScreenSize(int &wide, int &tall);
	virtual void SetAsTopMost(VPANEL panel, bool state);
	virtual void BringToFront(VPANEL panel);
	virtual void SetForegroundWindow(VPANEL panel);
	virtual void SetPanelVisible(VPANEL panel, bool state);
	virtual void SetMinimized(VPANEL panel, bool state);
	virtual bool IsMinimized(VPANEL panel);
	virtual void FlashWindow(VPANEL panel, bool state);
	virtual void SetTitle(VPANEL panel, const wchar_t *title);
	virtual void SetAsToolBar(VPANEL panel, bool state);
	virtual void CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true, bool kbInput = true);
	virtual void SwapBuffers(VPANEL panel);
	virtual void Invalidate(VPANEL panel);
	virtual void SetCursor(HCursor cursor);
	virtual bool IsCursorVisible(void);
	virtual void ApplyChanges(void);
	virtual bool IsWithin(int x, int y);
	virtual bool HasFocus(void);
	virtual bool SupportsFeature(SurfaceFeature_e feature);
	virtual void RestrictPaintToSinglePanel(VPANEL panel);
	virtual void SetModalPanel(VPANEL panel);
	virtual VPANEL GetModalPanel(void);
	virtual void UnlockCursor(void);
	virtual void LockCursor(void);
	virtual void SetTranslateExtendedKeys(bool state);
	virtual VPANEL GetTopmostPopup(void);
	virtual void SetTopLevelFocus(VPANEL panel);
	virtual HFont CreateFont(void);
	virtual bool AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange);
	virtual bool AddCustomFontFile(const char *fontFileName);
	virtual int GetFontTall(HFont font);
	virtual void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c);
	virtual int GetCharacterWidth(HFont font, int ch);
	virtual void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall);
	virtual VPANEL GetNotifyPanel(void);
	virtual void SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text);
	virtual void PlaySound(const char *fileName);
	virtual int GetPopupCount(void);
	virtual VPANEL GetPopup(int index);
	virtual bool ShouldPaintChildPanel(VPANEL childPanel);
	virtual bool RecreateContext(VPANEL panel);
	virtual void AddPanel(VPANEL panel);
	virtual void ReleasePanel(VPANEL panel);
	virtual void MovePopupToFront(VPANEL panel);
	virtual void MovePopupToBack(VPANEL panel);
	virtual void SolveTraverse(VPANEL panel, bool forceApplySchemeSettings = false);
	virtual void PaintTraverse(VPANEL panel);
	virtual void EnableMouseCapture(VPANEL panel, bool state);
	virtual void GetWorkspaceBounds(int &x, int &y, int &wide, int &tall);
	virtual void GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall);
	virtual void GetProportionalBase(int &width, int &height);
	virtual void CalculateMouseVisible(void);
	virtual bool NeedKBInput(void);
	virtual bool HasCursorPosFunctions(void);
	virtual void SurfaceGetCursorPos(int &x, int &y);
	virtual void SurfaceSetCursorPos(int x, int y);
	virtual void DrawTexturedPolygon(int *p, int n);
	virtual int GetFontAscent(HFont font, wchar_t wch);
	virtual void SetAllowHTMLJavaScript(bool state);
	virtual void SetLanguage(const char *pchLang);
	virtual const char *GetLanguage(void);
	virtual void DeleteTextureByID(int id);
	virtual void DrawUpdateRegionTextureBGRA(int nTextureID, int x, int y, const unsigned char *pchData, int wide, int tall);
	virtual void DrawSetTextureBGRA(int id, const unsigned char *rgba, int wide, int tall);
	virtual void CreateBrowser(VPANEL panel, IHTMLResponses *pBrowser, bool bPopupWindow, const char *pchUserAgentIdentifier);
	virtual void RemoveBrowser(VPANEL panel, IHTMLResponses *pBrowser);
	virtual IHTMLChromeController *AccessChromeHTMLController(void);
	virtual void setFullscreenMode(int wide, int tall, int bpp);
	virtual void setWindowedMode(void);
	virtual void SetAsTopMost(bool state);
	virtual void SetAsToolBar(bool state);
	virtual void PanelRequestFocus(VPANEL panel);
	virtual void EnableMouseCapture(bool state);
	virtual void DrawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1);
	virtual void SetNotifyIcon(Image *image, VPANEL panelToReceiveMessages, const char *text);
	virtual bool SetWatchForComputerUse(bool state);
	virtual double GetTimeSinceLastUse(void);
	virtual bool VGUI2MouseControl(void);
	virtual void SetVGUI2MouseControl(bool state);

public:
	void DrawSetAlphaMultiplier(float alpha);
	float DrawGetAlphaMultiplier(void);
};

HFont g_hCurrentFont;
int g_iCurrentTextR, g_iCurrentTextG, g_iCurrentTextB, g_iCurrentTextA;

static CUtlVector<CUtlSymbol> m_CustomFontFileNames;

void (__fastcall *m_pfnDrawSetTextureFile)(void *pthis, int, int id, const char *filename, int hardwareFilter, bool forceReload);
void (__fastcall *m_pfnDrawTexturedRect)(void *pthis, int, int x1, int y1, int x2, int y2);
void (__fastcall *m_pfnSurface_Shutdown)(void *pthis, int);
void (__fastcall *m_pfnDrawSetTextFont)(void *pthis, int, HFont font);
void (__fastcall *m_pfnDrawUnicodeChar)(void *pthis, int, wchar_t wch);
void (__fastcall *m_pfnDrawUnicodeCharAdd)(void *pthis, int, wchar_t wch);
bool (__fastcall *m_pfnSupportsFeature)(void *pthis, int, ISurface::SurfaceFeature_e feature);
bool (__fastcall *m_pfnAddGlyphSetToFont)(void *pthis, int, HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange);
void (__fastcall *m_pfnAddCustomFontFile)(void *pthis, int, const char *fontFileName);
int (__fastcall *m_pfnGetFontTall)(void *pthis, int, HFont font);
void (__fastcall *m_pfnGetCharABCwide)(void *pthis, int, HFont font, int ch, int &a, int &b, int &c);
int (__fastcall *m_pfnGetCharacterWidth)(void *pthis, int, HFont font, int ch);
void (__fastcall *m_pfnGetTextSize)(void *pthis, int, HFont font, const wchar_t *text, int &wide, int &tall);
int (__fastcall *m_pfnGetFontAscent)(void *pthis, int, HFont font, wchar_t wch);
HFont (__fastcall *m_pfnCreateFont)(void *pthis, int);
void (__fastcall *m_pfnDrawSetTextColor)(void *pthis, int, int r, int g, int b, int a);
void (__fastcall *m_pfnDrawSetTextColor2)(void *pthis, int, Color col);
void (__fastcall *m_pfnSetAllowHTMLJavaScript)(void *pthis, int, bool state);
void (__fastcall *m_pfnSetLanguage)(void *pthis, int, const char *pchLang);
const char *(__fastcall *m_pfnGetLanguage)(void *pthis, int);
void (__fastcall *m_pfnDeleteTextureByID)(void *pthis, int, int id);
void (__fastcall *m_pfnDrawUpdateRegionTextureBGRA)(void *pthis, int, int nTextureID, int x, int y, const unsigned char *pchData, int wide, int tall);
void (__fastcall *m_pfnDrawSetTextureBGRA)(void *pthis, int, int id, const unsigned char *rgba, int wide, int tall);
void (__fastcall *m_pfnCreateBrowser)(void *pthis, int, VPANEL panel, IHTMLResponses *pBrowser, bool bPopupWindow, const char *pchUserAgentIdentifier);
void (__fastcall *m_pfnRemoveBrowser)(void *pthis, int, VPANEL panel, IHTMLResponses *pBrowser);
IHTMLChromeController *(__fastcall *m_pfnAccessChromeHTMLController)(void *pthis, int);
void (__fastcall *m_pfnsetFullscreenMode)(void *pthis, int, int wide, int tall, int bpp);
void (__fastcall *m_pfnsetWindowedMode)(void *pthis, int);
void (__fastcall *m_pfnSetAsTopMost)(void *pthis, int, bool state);
void (__fastcall *m_pfnSetAsToolBar)(void *pthis, int, bool state);
void (__fastcall *m_pfnPanelRequestFocus)(void *pthis, int, VPANEL panel);
void (__fastcall *m_pfnEnableMouseCapture)(void *pthis, int, bool state);
void (__fastcall *m_pfnDrawPrintChar)(void *pthis, int, int x, int y, int wide, int tall, float s0, float t0, float s1, float t1);
void (__fastcall *m_pfnSetNotifyIcon)(void *pthis, int, Image *image, VPANEL panelToReceiveMessages, const char *text);
bool (__fastcall *m_pfnSetWatchForComputerUse)(void *pthis, int, bool state);
double (__fastcall *m_pfnGetTimeSinceLastUse)(void *pthis, int);
bool (__fastcall *m_pfnVGUI2MouseControl)(void *pthis, int);
void (__fastcall *m_pfnSetVGUI2MouseControl)(void *pthis, int, bool state);
void (__fastcall *m_pfnSurfaceSetCursorPos)(void *pthis, int, int x, int y);
void (__fastcall *m_pfnSetCursor)(void *pthis, int, HCursor cursor);

CSurfaceProxy g_SurfaceProxy;

void CSurfaceProxy::Shutdown(void)
{
	m_pfnSurface_Shutdown(this, 0);

	for (int i = 0; i < m_CustomFontFileNames.Count(); i++)
 	{
		int nRetries = 0;

		while (::RemoveFontResource(m_CustomFontFileNames[i].String()) && (nRetries < 10))
		{
			nRetries++;
			Msg("Removed font resource %s on attempt %d.\n", m_CustomFontFileNames[i].String(), nRetries);
		}
 	}

	m_CustomFontFileNames.RemoveAll();

	FontManager().ClearAllFonts();
}

void CSurfaceProxy::RunFrame(void)
{
	g_pSurface->RunFrame();
}

VPANEL CSurfaceProxy::GetEmbeddedPanel(void)
{
	return g_pSurface->GetEmbeddedPanel();
}

void CSurfaceProxy::SetEmbeddedPanel(VPANEL pPanel)
{
	g_pSurface->SetEmbeddedPanel(pPanel);
}

void CSurfaceProxy::PushMakeCurrent(VPANEL panel, bool useInsets)
{
	g_pSurface->PushMakeCurrent(panel, useInsets);
}

void CSurfaceProxy::PopMakeCurrent(VPANEL panel)
{
	g_pSurface->PopMakeCurrent(panel);
}

void CSurfaceProxy::DrawSetColor(int r, int g, int b, int a)
{
	g_pSurface->DrawSetColor(r, g, b, a);
}

void CSurfaceProxy::DrawSetColor(Color col)
{
	g_pSurface->DrawSetColor(col);
}

void CSurfaceProxy::DrawFilledRect(int x0, int y0, int x1, int y1)
{
	g_pSurface->DrawFilledRect(x0, y0, x1, y1);
}

void CSurfaceProxy::DrawOutlinedRect(int x0, int y0, int x1, int y1)
{
	g_pSurface->DrawOutlinedRect(x0, y0, x1, y1);
}

void CSurfaceProxy::DrawLine(int x0, int y0, int x1, int y1)
{
	g_pSurface->DrawLine(x0, y0, x1, y1);
}

void CSurfaceProxy::DrawPolyLine(int *px, int *py, int numPoints)
{
	g_pSurface->DrawPolyLine(px, py, numPoints);
}

void CSurfaceProxy::DrawSetTextFont(HFont font)
{
	g_hCurrentFont = font;
	m_pfnDrawSetTextFont(this, 0, font);
}

void CSurfaceProxy::DrawSetTextColor(int r, int g, int b, int a)
{
	g_iCurrentTextR = r;
	g_iCurrentTextG = g;
	g_iCurrentTextB = b;
	g_iCurrentTextA = a;

	m_pfnDrawSetTextColor(this, 0, r, g, b, a);
}

void CSurfaceProxy::DrawSetTextColor(Color col)
{
	int r = col.r();
	int g = col.g();
	int b = col.b();
	int a = col.a();

	g_iCurrentTextR = r;
	g_iCurrentTextG = g;
	g_iCurrentTextB = b;
	g_iCurrentTextA = a;

	m_pfnDrawSetTextColor2(this, 0, col);
}

void CSurfaceProxy::DrawSetTextPos(int x, int y)
{
	g_pSurface->DrawSetTextPos(x, y);
}

void CSurfaceProxy::DrawGetTextPos(int &x, int &y)
{
	g_pSurface->DrawGetTextPos(x, y);
}

void CSurfaceProxy::DrawPrintText(const wchar_t *text, int textLen)
{
	g_pSurface->DrawPrintText(text, textLen);
}

void CSurfaceProxy::DrawUnicodeChar(wchar_t wch)
{
	if (g_hCurrentFont == INVALID_FONT)
		return;

	int x, y;
	g_pSurface->DrawGetTextPos(x, y);

	int a, b, c;
	FontManager().GetCharABCwide(g_hCurrentFont, wch, a, b, c);

	int rgbaWide, rgbaTall;
	rgbaTall = GetFontTall(g_hCurrentFont);

	if (FontManager().GetFontUnderlined(g_hCurrentFont))
	{
		rgbaWide = c + b + a;
	}
	else
	{
		x += a;
		rgbaWide = b;
	}

	int textureID;
	float *texCoords = NULL;

	if (!g_FontTextureCache.GetTextureForChar(g_hCurrentFont, wch, &textureID, &texCoords))
		return;

	g_pSurface->DrawSetTexture(textureID);

	if(FontManager().GetFontOutlined(g_hCurrentFont))
	{
		int OutlineColor = (g_iCurrentTextR <= 10 && g_iCurrentTextG <= 10 && g_iCurrentTextB <= 10) ? 255 : 0;
		m_pfnDrawSetTextColor(this, 0, OutlineColor, OutlineColor, OutlineColor, g_iCurrentTextA);
		for(int i = -1;i <= 1; i += 1)
		{
			for(int j = -1;j <= 1; j += 1)
			{
				if(i != 0 && j != 0)
					staticSurface->drawPrintChar(x+i, y+j, rgbaWide, rgbaTall, texCoords[0], texCoords[1], texCoords[2], texCoords[3]);
			}
		}
	}

	m_pfnDrawSetTextColor(this, 0, g_iCurrentTextR, g_iCurrentTextG, g_iCurrentTextB, g_iCurrentTextA);
	staticSurface->drawPrintChar(x, y, rgbaWide, rgbaTall, texCoords[0], texCoords[1], texCoords[2], texCoords[3]);
	m_pfnDrawSetTextColor(this, 0, g_iCurrentTextR, g_iCurrentTextG, g_iCurrentTextB, g_iCurrentTextA);
	g_pSurface->DrawSetTextPos(x + b + c, y);
}

void CSurfaceProxy::DrawUnicodeCharAdd(wchar_t wch)
{
	if (g_hCurrentFont == INVALID_FONT)
		return;

	int x, y;
	g_pSurface->DrawGetTextPos(x, y);

	int a, b, c;
	FontManager().GetCharABCwide(g_hCurrentFont, wch, a, b, c);

	int rgbaWide, rgbaTall;
	rgbaTall = GetFontTall(g_hCurrentFont);

	if (FontManager().GetFontUnderlined(g_hCurrentFont))
	{
		rgbaWide = c + b + a;
	}
	else
	{
		x += a;
		rgbaWide = b;
	}

	int textureID;
	float *texCoords = NULL;

	if (!g_FontTextureCache.GetTextureForChar(g_hCurrentFont, wch, &textureID, &texCoords))
		return;

	g_pSurface->DrawSetTexture(textureID);

	if(FontManager().GetFontOutlined(g_hCurrentFont))
	{
		int OutlineColor = (g_iCurrentTextR <= 10 && g_iCurrentTextG <= 10 && g_iCurrentTextB <= 10) ? 255 : 0;
		m_pfnDrawSetTextColor(this, 0, OutlineColor, OutlineColor, OutlineColor, g_iCurrentTextA);
		for(int i = -1;i <= 1; i ++)
		{
			for(int j = -1;j <= 1; j ++)
			{
				if(i != 0 && j != 0)
					staticSurface->drawPrintCharAdd(x+i, y+j, rgbaWide, rgbaTall, texCoords[0], texCoords[1], texCoords[2], texCoords[3]);
			}
		}
	}

	m_pfnDrawSetTextColor(this, 0, g_iCurrentTextR, g_iCurrentTextG, g_iCurrentTextB, g_iCurrentTextA);
	staticSurface->drawPrintCharAdd(x, y, rgbaWide, rgbaTall, texCoords[0], texCoords[1], texCoords[2], texCoords[3]);
	m_pfnDrawSetTextColor(this, 0, g_iCurrentTextR, g_iCurrentTextG, g_iCurrentTextB, g_iCurrentTextA);
	g_pSurface->DrawSetTextPos(x + b + c, y);
}

void CSurfaceProxy::DrawFlushText(void)
{
	g_pSurface->DrawFlushText();
}

IHTML *CSurfaceProxy::CreateHTMLWindow(IHTMLEvents *events, VPANEL context)
{
	return g_pSurface->CreateHTMLWindow(events, context);
}

void CSurfaceProxy::PaintHTMLWindow(IHTML *htmlwin)
{
	g_pSurface->PaintHTMLWindow(htmlwin);
}

void CSurfaceProxy::DeleteHTMLWindow(IHTML *htmlwin)
{
	g_pSurface->DeleteHTMLWindow(htmlwin);
}

void CSurfaceProxy::DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload)
{
	g_pSurface->DrawSetTextureFile(id, filename, hardwareFilter, forceReload);
}

void CSurfaceProxy::DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload)
{
	g_pSurface->DrawSetTextureRGBA(id, rgba, wide, tall, hardwareFilter, forceReload);
}

void CSurfaceProxy::DrawSetTexture(int id)
{
	g_pSurface->DrawSetTexture(id);
}

void CSurfaceProxy::DrawGetTextureSize(int id, int &wide, int &tall)
{
	g_pSurface->DrawGetTextureSize(id, wide, tall);
}

void CSurfaceProxy::DrawTexturedRect(int x0, int y0, int x1, int y1)
{
	g_pSurface->DrawTexturedRect(x0, y0, x1, y1);
}

bool CSurfaceProxy::IsTextureIDValid(int id)
{
	return g_pSurface->IsTextureIDValid(id);
}

int CSurfaceProxy::CreateNewTextureID(bool procedural)
{
	return g_pSurface->CreateNewTextureID(procedural);
}

void CSurfaceProxy::GetScreenSize(int &wide, int &tall)
{
	g_pSurface->GetScreenSize(wide, tall);
}

void CSurfaceProxy::SetAsTopMost(VPANEL panel, bool state)
{
	g_pSurface->SetAsTopMost(panel, state);
}

void CSurfaceProxy::BringToFront(VPANEL panel)
{
	g_pSurface->BringToFront(panel);
}

void CSurfaceProxy::SetForegroundWindow(VPANEL panel)
{
	g_pSurface->SetForegroundWindow(panel);
}

void CSurfaceProxy::SetPanelVisible(VPANEL panel, bool state)
{
	g_pSurface->SetPanelVisible(panel, state);
}

void CSurfaceProxy::SetMinimized(VPANEL panel, bool state)
{
	g_pSurface->SetMinimized(panel, state);
}

bool CSurfaceProxy::IsMinimized(VPANEL panel)
{
	return g_pSurface->IsMinimized(panel);
}

void CSurfaceProxy::FlashWindow(VPANEL panel, bool state)
{
	g_pSurface->FlashWindow(panel, state);
}

void CSurfaceProxy::SetTitle(VPANEL panel, const wchar_t *title)
{
	g_pSurface->SetTitle(panel, title);
}

void CSurfaceProxy::SetAsToolBar(VPANEL panel, bool state)
{
	g_pSurface->SetAsToolBar(panel, state);
}

void CSurfaceProxy::CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon, bool disabled, bool mouseInput, bool kbInput)
{
	g_pSurface->CreatePopup(panel, minimised, showTaskbarIcon, disabled, mouseInput, kbInput);
}

void CSurfaceProxy::SwapBuffers(VPANEL panel)
{
	g_pSurface->SwapBuffers(panel);
}

void CSurfaceProxy::Invalidate(VPANEL panel)
{
	g_pSurface->Invalidate(panel);
}

void CSurfaceProxy::SetCursor(HCursor cursor)
{
	m_pfnSetCursor(this, 0, cursor);
}

bool CSurfaceProxy::IsCursorVisible(void)
{
	return g_pSurface->IsCursorVisible();
}

void CSurfaceProxy::ApplyChanges(void)
{
	g_pSurface->ApplyChanges();
}

bool CSurfaceProxy::IsWithin(int x, int y)
{
	return g_pSurface->IsWithin(x, y);
}

bool CSurfaceProxy::HasFocus(void)
{
	return g_pSurface->HasFocus();
}

bool CSurfaceProxy::SupportsFeature(vgui::ISurface::SurfaceFeature_e feature)
{
	switch (feature)
	{
		case OUTLINE_FONTS:
		{
			if (IsX360())
				return false;

			return true;
		}
	}

	return m_pfnSupportsFeature(this, 0, feature);
}

void CSurfaceProxy::RestrictPaintToSinglePanel(VPANEL panel)
{
	g_pSurface->RestrictPaintToSinglePanel(panel);
}

void CSurfaceProxy::SetModalPanel(VPANEL panel)
{
	g_pSurface->SetModalPanel(panel);
}

VPANEL CSurfaceProxy::GetModalPanel(void)
{
	return g_pSurface->GetModalPanel();
}

void CSurfaceProxy::UnlockCursor(void)
{
	g_pSurface->UnlockCursor();
}

void CSurfaceProxy::LockCursor(void)
{
	g_pSurface->LockCursor();
}

void CSurfaceProxy::SetTranslateExtendedKeys(bool state)
{
	g_pSurface->SetTranslateExtendedKeys(state);
}

VPANEL CSurfaceProxy::GetTopmostPopup(void)
{
	return g_pSurface->GetTopmostPopup();
}

void CSurfaceProxy::SetTopLevelFocus(VPANEL panel)
{
	g_pSurface->SetTopLevelFocus(panel);
}

HFont CSurfaceProxy::CreateFont(void)
{
	return FontManager().CreateFont();
}

bool CSurfaceProxy::AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange)
{
	return FontManager().AddGlyphSetToFont(font, windowsFontName, tall, weight, blur, scanlines, flags, lowRange, highRange);
}

bool CSurfaceProxy::AddCustomFontFile(const char *fontFileName)
{
	char fullPath[MAX_PATH];

	if (!g_pFullFileSystem->GetLocalPath(fontFileName, fullPath, sizeof(fullPath)))
	{
		Msg("Couldn't find custom font file '%s'\n", fontFileName);
		return false;
	}

	Q_strlower(fullPath);
	CUtlSymbol sym(fullPath);
	int i;

	for (i = 0; i < m_CustomFontFileNames.Count(); i++)
	{
		if (m_CustomFontFileNames[i] == sym)
			break;
	}

	if (!m_CustomFontFileNames.IsValidIndex(i))
	{
	 	m_CustomFontFileNames.AddToTail(fullPath);

		if (IsPC())
		{
			g_pFullFileSystem->GetLocalCopy(fullPath);
		}
	}

	return FontManager().AddCustomFontFile(fullPath);
}

int CSurfaceProxy::GetFontTall(HFont font)
{
	return FontManager().GetFontTall(font);
}

void CSurfaceProxy::GetCharABCwide(HFont font, int ch, int &a, int &b, int &c)
{
	return FontManager().GetCharABCwide(font, ch, a, b, c);
}

int CSurfaceProxy::GetCharacterWidth(HFont font, int ch)
{
	return FontManager().GetCharacterWidth(font, ch);
}

void CSurfaceProxy::GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall)
{
	return FontManager().GetTextSize(font, text, wide, tall);
}

VPANEL CSurfaceProxy::GetNotifyPanel(void)
{
	return g_pSurface->GetNotifyPanel();
}

void CSurfaceProxy::SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text)
{
	g_pSurface->SetNotifyIcon(context, icon, panelToReceiveMessages, text);
}

void CSurfaceProxy::PlaySound(const char *fileName)
{
	g_pSurface->PlaySound(fileName);
}

int CSurfaceProxy::GetPopupCount(void)
{
	return g_pSurface->GetPopupCount();
}

VPANEL CSurfaceProxy::GetPopup(int index)
{
	return g_pSurface->GetPopup(index);
}

bool CSurfaceProxy::ShouldPaintChildPanel(VPANEL childPanel)
{
	return g_pSurface->ShouldPaintChildPanel(childPanel);
}

bool CSurfaceProxy::RecreateContext(VPANEL panel)
{
	return g_pSurface->RecreateContext(panel);
}

void CSurfaceProxy::AddPanel(VPANEL panel)
{
	g_pSurface->AddPanel(panel);
}

void CSurfaceProxy::ReleasePanel(VPANEL panel)
{
	g_pSurface->ReleasePanel(panel);
}

void CSurfaceProxy::MovePopupToFront(VPANEL panel)
{
	g_pSurface->MovePopupToFront(panel);
}

void CSurfaceProxy::MovePopupToBack(VPANEL panel)
{
	g_pSurface->MovePopupToBack(panel);
}

void CSurfaceProxy::SolveTraverse(VPANEL panel, bool forceApplySchemeSettings)
{
	g_pSurface->SolveTraverse(panel, forceApplySchemeSettings);
}

void CSurfaceProxy::PaintTraverse(VPANEL panel)
{
	g_pSurface->PaintTraverse(panel);
}

void CSurfaceProxy::EnableMouseCapture(VPANEL panel, bool state)
{
	g_pSurface->EnableMouseCapture(panel, state);
}

void CSurfaceProxy::GetWorkspaceBounds(int &x, int &y, int &wide, int &tall)
{
	g_pSurface->GetWorkspaceBounds(x, y, wide, tall);
}

void CSurfaceProxy::GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall)
{
	g_pSurface->GetAbsoluteWindowBounds(x, y, wide, tall);
}

void CSurfaceProxy::GetProportionalBase(int &width, int &height)
{
	g_pSurface->GetProportionalBase(width, height);
}

void CSurfaceProxy::CalculateMouseVisible(void)
{
	g_pSurface->CalculateMouseVisible();
}

bool CSurfaceProxy::NeedKBInput(void)
{
	return g_pSurface->NeedKBInput();
}

bool CSurfaceProxy::HasCursorPosFunctions(void)
{
	return g_pSurface->HasCursorPosFunctions();
}

void CSurfaceProxy::SurfaceGetCursorPos(int &x, int &y)
{
	g_pSurface->SurfaceGetCursorPos(x, y);
}

void CSurfaceProxy::SurfaceSetCursorPos(int x, int y)
{
	return m_pfnSurfaceSetCursorPos(this, 0, x, y);
}

void CSurfaceProxy::DrawTexturedPolygon(int *p, int n)
{
	g_pSurface->DrawTexturedPolygon(p, n);
}

int CSurfaceProxy::GetFontAscent(HFont font, wchar_t wch)
{
	return FontManager().GetFontAscent(font, wch);
}

void CSurfaceProxy::SetAllowHTMLJavaScript(bool state)
{
	m_pfnSetAllowHTMLJavaScript(this, 0, state);
}

void CSurfaceProxy::SetLanguage(const char *pchLang)
{
	m_pfnSetLanguage(this, 0, pchLang);
}

const char *CSurfaceProxy::GetLanguage(void)
{
	return m_pfnGetLanguage(this, 0);
}

void CSurfaceProxy::DeleteTextureByID(int id)
{
	m_pfnDeleteTextureByID(this, 0, id);
}

void CSurfaceProxy::DrawUpdateRegionTextureBGRA(int nTextureID, int x, int y, const unsigned char *pchData, int wide, int tall)
{
	m_pfnDrawUpdateRegionTextureBGRA(this, 0, nTextureID, x, y, pchData, wide, tall);
}

void CSurfaceProxy::DrawSetTextureBGRA(int id, const unsigned char *rgba, int wide, int tall)
{
	m_pfnDrawSetTextureBGRA(this, 0, id, rgba, wide, tall);
}

void CSurfaceProxy::CreateBrowser(VPANEL panel, IHTMLResponses *pBrowser, bool bPopupWindow, const char *pchUserAgentIdentifier)
{
	m_pfnCreateBrowser(this, 0, panel, pBrowser, bPopupWindow, pchUserAgentIdentifier);
}

void CSurfaceProxy::RemoveBrowser(VPANEL panel, IHTMLResponses *pBrowser)
{
	m_pfnRemoveBrowser(this, 0, panel, pBrowser);
}

IHTMLChromeController *CSurfaceProxy::AccessChromeHTMLController(void)
{
	return m_pfnAccessChromeHTMLController(this, 0);
}

void CSurfaceProxy::setFullscreenMode(int wide, int tall, int bpp)
{
	m_pfnsetFullscreenMode(this, 0, wide, tall, bpp);
}

void CSurfaceProxy::setWindowedMode(void)
{
	m_pfnsetWindowedMode(this, 0);
}

void CSurfaceProxy::SetAsTopMost(bool state)
{
	m_pfnSetAsTopMost(this, 0, state);
}

void CSurfaceProxy::SetAsToolBar(bool state)
{
	m_pfnSetAsToolBar(this, 0, state);
}

void CSurfaceProxy::PanelRequestFocus(VPANEL panel)
{
	m_pfnPanelRequestFocus(this, 0, panel);
}

void CSurfaceProxy::EnableMouseCapture(bool state)
{
	m_pfnEnableMouseCapture(this, 0, state);
}

void CSurfaceProxy::DrawPrintChar(int x, int y, int wide, int tall, float s0, float t0, float s1, float t1)
{
	m_pfnDrawPrintChar(this, 0, x, y, wide, tall, s0, s1, s1, t1);
}

void CSurfaceProxy::SetNotifyIcon(Image *image, VPANEL panelToReceiveMessages, const char *text)
{
	m_pfnSetNotifyIcon(this, 0, image, panelToReceiveMessages, text);
}

bool CSurfaceProxy::SetWatchForComputerUse(bool state)
{
	return m_pfnSetWatchForComputerUse(this, 0, state);
}

double CSurfaceProxy::GetTimeSinceLastUse(void)
{
	return m_pfnGetTimeSinceLastUse(this, 0);
}

bool CSurfaceProxy::VGUI2MouseControl(void)
{
	return m_pfnVGUI2MouseControl(this, 0);
}

void CSurfaceProxy::SetVGUI2MouseControl(bool state)
{
	m_pfnSetVGUI2MouseControl(this, 0, state);
}

#define CWIN32FONT_GETCHARABCWIDTHS_SIG "\x55\x8B\xEC\x83\xEC\x70\x53\x56\x8B\xF1\x8D\x45\xD0\x57\x8D\x4D\xE4\x50\x8B\x45\x08\x8D\x55\xD4\x51\x52\x50\x8B\xCE"

void (__fastcall *m_pfnCWin32Font_GetCharRGBA)(void *pthis, int, int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char *rgba);
hook_t *g_hCWin32Font_GetCharRGBA;

void Surface_InstallHook(void)
{
	DWORD *pVFTable = *(DWORD **)&g_SurfaceProxy;

	m_pfnCWin32Font_GetCharRGBA = (void (__fastcall *)(void *, int, int, int, int, int, int, unsigned char *))
		g_pMetaHookAPI->SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, CWIN32FONT_GETCHARABCWIDTHS_SIG, sizeof(CWIN32FONT_GETCHARABCWIDTHS_SIG) - 1);
	if (m_pfnCWin32Font_GetCharRGBA)
	{
		g_hCWin32Font_GetCharRGBA = g_pMetaHookAPI->InlineHook(m_pfnCWin32Font_GetCharRGBA, CWin32Font_GetCharRGBA, (void **)&m_pfnCWin32Font_GetCharRGBA);
	}

	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 1, (void *)pVFTable[1],   (void **)&m_pfnSurface_Shutdown);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 13, (void *)pVFTable[13], (void **)&m_pfnDrawSetTextFont);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 15, (void *)pVFTable[15], (void **)&m_pfnDrawSetTextColor);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 14, (void *)pVFTable[14], (void **)&m_pfnDrawSetTextColor2);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 19, (void *)pVFTable[19], (void **)&m_pfnDrawUnicodeChar);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 20, (void *)pVFTable[20], (void **)&m_pfnDrawUnicodeCharAdd);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 50, (void *)pVFTable[50], (void **)&m_pfnSupportsFeature);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 59, (void *)pVFTable[59], (void **)&m_pfnCreateFont);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 60, (void *)pVFTable[60], (void **)&m_pfnAddGlyphSetToFont);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 61, (void *)pVFTable[61], (void **)&m_pfnAddCustomFontFile);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 62, (void *)pVFTable[62], (void **)&m_pfnGetFontTall);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 63, (void *)pVFTable[63], (void **)&m_pfnGetCharABCwide);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 64, (void *)pVFTable[64], (void **)&m_pfnGetCharacterWidth);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 65, (void *)pVFTable[65], (void **)&m_pfnGetTextSize);
	g_pMetaHookAPI->VFTHook(g_pSurface, 0, 89, (void *)pVFTable[89], (void **)&m_pfnGetFontAscent);
}