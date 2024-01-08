#include <vgui/ISurface.h>
#include <vgui/Cursor.h>
#include "Surface2.h"

//BaseUISurface from hw.dll
extern vgui::ISurface *g_pSurface;
extern vgui::ISurface_HL25 *g_pSurface_HL25;

using namespace vgui;

static CSurface2 s_Surface2;

vgui::CSurface2 *g_pVGuiSurface = &s_Surface2;

CSurface2::CSurface2()
{
	m_flAlphaMultiplier = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down app
//-----------------------------------------------------------------------------
void CSurface2::Shutdown(void)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->Shutdown();

	return g_pSurface->Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Handles windows message pump
//-----------------------------------------------------------------------------
void CSurface2::RunFrame(void)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->RunFrame();

	return g_pSurface->RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPANEL CSurface2::GetEmbeddedPanel(void)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->GetEmbeddedPanel();

	return g_pSurface->GetEmbeddedPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the panel for use
// Input  : *embeddedPanel - Main panel that becomes the top of the hierarchy
//-----------------------------------------------------------------------------
void CSurface2::SetEmbeddedPanel(VPANEL pPanel)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->SetEmbeddedPanel(pPanel);

	return g_pSurface->SetEmbeddedPanel(pPanel);
}

void CSurface2::PushMakeCurrent(VPANEL panel, bool useInsets)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->PushMakeCurrent(panel, useInsets);

	return g_pSurface->PushMakeCurrent(panel, useInsets);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::PopMakeCurrent(VPANEL panel)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->PopMakeCurrent(panel);

	return g_pSurface->PopMakeCurrent(panel);
}

void CSurface2::DrawSetColor(int r, int g, int b, int a)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->DrawSetColor(r, g, b, a);

	return g_pSurface->DrawSetColor(r, g, b, a * m_flAlphaMultiplier);
}

void CSurface2::DrawSetColor(Color col)
{
	col[3] *= m_flAlphaMultiplier;

	if (g_pSurface_HL25)
		return g_pSurface_HL25->DrawSetColor(col);

	return g_pSurface->DrawSetColor(col);
}

void CSurface2::DrawFilledRect(int x0, int y0, int x1, int y1)
{
	if (g_pSurface_HL25)
		return g_pSurface_HL25->DrawFilledRect(x0, y0, x1, y1);

	return g_pSurface->DrawFilledRect(x0, y0, x1, y1);
}

void CSurface2::DrawFilledRectArray(IntRect *pRects, int numRects)
{
	for (int i = 0; i < numRects; ++i)
	{
		DrawFilledRect(pRects[i].x0, pRects[i].y0, pRects[i].x1, pRects[i].y1);
	}
}

void CSurface2::DrawOutlinedRect(int x0, int y0, int x1, int y1)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawOutlinedRect(x0, y0, x1, y1);
	}

	return g_pSurface->DrawOutlinedRect(x0, y0, x1, y1);
}

void CSurface2::DrawLine(int x0, int y0, int x1, int y1)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawLine(x0, y0, x1, y1);
	}

	return g_pSurface->DrawLine(x0, y0, x1, y1);
}

void CSurface2::DrawPolyLine(int *px, int *py, int numPoints)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawPolyLine(px, py, numPoints);
	}

	return g_pSurface->DrawPolyLine(px, py, numPoints);
}

void CSurface2::DrawSetTextFont(HFont font)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawSetTextFont(font);
	}

	return g_pSurface->DrawSetTextFont(font);
}

void CSurface2::DrawSetTextColor(int r, int g, int b, int a)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawSetTextColor(r, g, b, a * m_flAlphaMultiplier);
	}

	return g_pSurface->DrawSetTextColor(r, g, b, a * m_flAlphaMultiplier);
}

void CSurface2::DrawSetTextColor(Color col)
{
	col[3] *= m_flAlphaMultiplier;

	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawSetTextColor(col);
	}

	return g_pSurface->DrawSetTextColor(col);
}

void CSurface2::DrawSetTextPos(int x, int y)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawSetTextPos(x, y);
	}

	return g_pSurface->DrawSetTextPos(x, y);
}

void CSurface2::DrawGetTextPos(int &x, int &y)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawGetTextPos(x, y);
	}

	return g_pSurface->DrawGetTextPos(x, y);
}

void CSurface2::DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType)
{
	if (g_pSurface_HL25)
	{
		//TODO:...? drawType?
		return g_pSurface_HL25->DrawPrintText(text, textLen);
	}

	g_pSurface->DrawPrintText(text, textLen);
}

//-----------------------------------------------------------------------------
// Purpose: draws single unicode character at the current position with the
//			current font & color
//-----------------------------------------------------------------------------
void CSurface2::DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType)
{
	if (g_pSurface_HL25)
	{
		//TODO:...? drawType?
		return g_pSurface_HL25->DrawUnicodeChar(wch);
	}

	return g_pSurface->DrawUnicodeChar(wch);
}

//-----------------------------------------------------------------------------
// Purpose: does nothing, since we don't need this optimization in win32
//-----------------------------------------------------------------------------
void CSurface2::DrawFlushText(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawFlushText();
	}

	return g_pSurface->DrawFlushText();
}

IHTML *CSurface2::CreateHTMLWindow(IHTMLEvents *events, VPANEL context)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->CreateHTMLWindow(events, context);
	}

	return g_pSurface->CreateHTMLWindow(events, context);
}

void CSurface2::PaintHTMLWindow(IHTML *htmlwin)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->PaintHTMLWindow(htmlwin);
	}

	return g_pSurface->PaintHTMLWindow(htmlwin);
}

void CSurface2::DeleteHTMLWindow(IHTML *htmlwin)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DeleteHTMLWindow(htmlwin);
	}

	g_pSurface->DeleteHTMLWindow(htmlwin);
}

int CSurface2::DrawGetTextureId(const char *filename)
{
	if (g_pSurface_HL25)
	{
		return -1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//			*filename - 
//			maxlen - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSurface2::DrawGetTextureFile(int id, char *filename, int maxlen)
{
	filename[0] = 0;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Maps a texture file to an id, and makes it the current drawing texture
//			tries to load as a .tga first, and if not found as a .bmp
//-----------------------------------------------------------------------------
void CSurface2::DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawSetTextureFile(id, filename, hardwareFilter, forceReload);
	}

	return g_pSurface->DrawSetTextureFile(id, filename, hardwareFilter, forceReload);
}

//-----------------------------------------------------------------------------
// Purpose: maps a texture from memory to an id, and uploads it into the engine
//-----------------------------------------------------------------------------
void CSurface2::DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawSetTextureRGBA(id, rgba, wide, tall, hardwareFilter, forceReload);
	}

	return g_pSurface->DrawSetTextureRGBA(id, rgba, wide, tall, hardwareFilter, forceReload);
}

//-----------------------------------------------------------------------------
// Purpose: sets the current active texture
//-----------------------------------------------------------------------------
void CSurface2::DrawSetTexture(int id)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawSetTexture(id);
	}

	return g_pSurface->DrawSetTexture(id);
}

//-----------------------------------------------------------------------------
// Purpose: Called by vgui to get texture dimensions
//-----------------------------------------------------------------------------
void CSurface2::DrawGetTextureSize(int id, int &wide, int &tall)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawGetTextureSize(id, wide, tall);
	}

	return g_pSurface->DrawGetTextureSize(id, wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::DrawTexturedRect(int x0, int y0, int x1, int y1)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->DrawTexturedRect(x0, y0, x1, y1);
	}

	return g_pSurface->DrawTexturedRect(x0, y0, x1, y1);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the texture id has a valid texture bound to it
//-----------------------------------------------------------------------------
bool CSurface2::IsTextureIDValid(int id)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->IsTextureIDValid(id);
	}

	return g_pSurface->IsTextureIDValid(id);
}

//-----------------------------------------------------------------------------
// Purpose: allocates a new texture id
//-----------------------------------------------------------------------------
int CSurface2::CreateNewTextureID(bool procedural)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->CreateNewTextureID(procedural);
	}

	return g_pSurface->CreateNewTextureID(procedural);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::GetScreenSize(int &wide, int &tall)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetScreenSize(wide, tall);
	}
	return g_pSurface->GetScreenSize(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: 
// HWND_TOPMOST - Places the window above all non-topmost windows. 
// The window maintains its topmost position even when it is deactivated.
// HWND_NOTOPMOST - Places the window above all non-topmost windows (that is, behind 
// all topmost windows). This flag has no effect if the window is already a non-topmost window.
//-----------------------------------------------------------------------------
void CSurface2::SetAsTopMost(VPANEL panel, bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetAsTopMost(panel, state);
	}
	return g_pSurface->SetAsTopMost(panel, state);
}

//-----------------------------------------------------------------------------
// Purpose: brings the current surface to the foreground
//-----------------------------------------------------------------------------
void CSurface2::BringToFront(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->BringToFront(panel);
	}
	return g_pSurface->BringToFront(panel);
}

//-----------------------------------------------------------------------------
// Purpose: puts the thread that created the specified window into the foreground 
//          and activates the window.
//-----------------------------------------------------------------------------
void CSurface2::SetForegroundWindow(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetForegroundWindow(panel);
	}
	return g_pSurface->SetForegroundWindow(panel);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::SetPanelVisible(VPANEL panel, bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetPanelVisible(panel, state);
	}
	return g_pSurface->SetPanelVisible(panel, state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::SetMinimized(VPANEL panel, bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetMinimized(panel, state);
	}
	return g_pSurface->SetMinimized(panel, state);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the window is minimized
//-----------------------------------------------------------------------------
bool CSurface2::IsMinimized(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->IsMinimized(panel);
	}
	return g_pSurface->IsMinimized(panel);
}

//-----------------------------------------------------------------------------
// Purpose: Flashes the window icon in the taskbar, to get the users attention
// Input  : flashCount - number of times to flash the window
//-----------------------------------------------------------------------------
void CSurface2::FlashWindow(VPANEL panel, bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->FlashWindow(panel, state);
	}
	return g_pSurface->FlashWindow(panel, state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::SetTitle(VPANEL panel, const wchar_t *title)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetTitle(panel, title);
	}
	return g_pSurface->SetTitle(panel, title);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::SetAsToolBar(VPANEL panel, bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetAsToolBar(panel, state);
	}
	return g_pSurface->SetAsToolBar(panel, state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon, bool disabled, bool mouseInput, bool kbInput)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->CreatePopup(panel, minimised, showTaskbarIcon, disabled, mouseInput, kbInput);
	}
	return g_pSurface->CreatePopup(panel, minimised, showTaskbarIcon, disabled, mouseInput, kbInput);
}

//-----------------------------------------------------------------------------
// Purpose: Called after a Paint to display the new buffer
//-----------------------------------------------------------------------------
void CSurface2::SwapBuffers(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SwapBuffers(panel);
	}
	return g_pSurface->SwapBuffers(panel);
}

//-----------------------------------------------------------------------------
// Purpose: Forces the window to be redrawn
//-----------------------------------------------------------------------------
void CSurface2::Invalidate(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->Invalidate(panel);
	}
	return g_pSurface->Invalidate(panel);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the current cursor
//-----------------------------------------------------------------------------
void CSurface2::SetCursor(HCursor cursor)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetCursor(cursor);
	}
	return g_pSurface->SetCursor(cursor);
}

bool CSurface2::IsCursorVisible(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->IsCursorVisible();
	}
	return g_pSurface->IsCursorVisible();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame to change to window state if necessary
//-----------------------------------------------------------------------------
void CSurface2::ApplyChanges(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->ApplyChanges();
	}
	return g_pSurface->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the cursor is over this surface
//			Uses the windows call to do this, instead of doing it procedurally
//-----------------------------------------------------------------------------
bool CSurface2::IsWithin(int x, int y)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->IsWithin(x, y);
	}
	return g_pSurface->IsWithin(x, y);
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if any of the windows have focus
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSurface2::HasFocus(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->HasFocus();
	}
	return g_pSurface->HasFocus();
}

//-----------------------------------------------------------------------------
// Purpose: cap bits
//-----------------------------------------------------------------------------
bool CSurface2::SupportsFeature(SurfaceFeature_e feature)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SupportsFeature(feature);
	}
	return g_pSurface->SupportsFeature(feature);
}

// FIXME: write these functions!
void CSurface2::RestrictPaintToSinglePanel(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->RestrictPaintToSinglePanel(panel);
	}
	return g_pSurface->RestrictPaintToSinglePanel(panel);
}

void CSurface2::SetModalPanel(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetModalPanel(panel);
	}
	return g_pSurface->SetModalPanel(panel);
}

VPANEL CSurface2::GetModalPanel(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetModalPanel();
	}
	return g_pSurface->GetModalPanel();
}

void CSurface2::UnlockCursor(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->UnlockCursor();
	}
	return g_pSurface->UnlockCursor();
}

void CSurface2::LockCursor(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->LockCursor();
	}
	return g_pSurface->LockCursor();
}

void CSurface2::SetTranslateExtendedKeys(bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetTranslateExtendedKeys(state);
	}
	return g_pSurface->SetTranslateExtendedKeys(state);
}

VPANEL CSurface2::GetTopmostPopup(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetTopmostPopup();
	}
	return g_pSurface->GetTopmostPopup();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::SetTopLevelFocus(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetTopLevelFocus(panel);
	}
	return g_pSurface->SetTopLevelFocus(panel);
}

//-----------------------------------------------------------------------------
// Purpose: creates a new empty font
//-----------------------------------------------------------------------------
HFont CSurface2::CreateFont(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->CreateFont();
	}
	return g_pSurface->CreateFont();
}

//-----------------------------------------------------------------------------
// Purpose: adds glyphs to a font created by CreateFont()
//-----------------------------------------------------------------------------
bool CSurface2::AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->AddGlyphSetToFont(font, windowsFontName, tall, weight, blur, scanlines, flags, 0x0, 0xFFFF);
	}
	return g_pSurface->AddGlyphSetToFont(font, windowsFontName, tall, weight, blur, scanlines, flags, 0x0, 0xFFFF);
}

//-----------------------------------------------------------------------------
// Purpose: adds a custom font file (only supports true type font files (.ttf) for now)
//-----------------------------------------------------------------------------
bool CSurface2::AddCustomFontFile(const char *fontFileName)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->AddCustomFontFile(fontFileName);
	}
	return g_pSurface->AddCustomFontFile(fontFileName);
}

//-----------------------------------------------------------------------------
// Purpose: returns the max height of a font
//-----------------------------------------------------------------------------
int CSurface2::GetFontTall(HFont font)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetFontTall(font);
	}
	return g_pSurface->GetFontTall(font);
}

//-----------------------------------------------------------------------------
// Purpose: returns the max height of a font
//-----------------------------------------------------------------------------
int CSurface2::GetFontAscent(HFont font, wchar_t wch)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetFontAscent(font, wch);
	}
	return g_pSurface->GetFontAscent(font, wch);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSurface2::IsFontAdditive(HFont font)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->IsFontAdditive(font);
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns the abc widths of a single character
//-----------------------------------------------------------------------------
void CSurface2::GetCharABCwide(HFont font, int ch, int &a, int &b, int &c)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetCharABCwide(font, ch, a, b, c);
	}
	return g_pSurface->GetCharABCwide(font, ch, a, b, c);
}

//-----------------------------------------------------------------------------
// Purpose: returns the pixel width of a single character
//-----------------------------------------------------------------------------
int CSurface2::GetCharacterWidth(HFont font, int ch)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetCharacterWidth(font, ch);
	}
	return g_pSurface->GetCharacterWidth(font, ch);
}

//-----------------------------------------------------------------------------
// Purpose: returns the area of a text string, including newlines
//-----------------------------------------------------------------------------
void CSurface2::GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetTextSize(font, text, wide, tall);
	}
	return g_pSurface->GetTextSize(font, text, wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPANEL CSurface2::GetNotifyPanel(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetNotifyPanel();
	}
	return g_pSurface->GetNotifyPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetNotifyIcon(context, icon, panelToReceiveMessages, text);
	}
	return g_pSurface->SetNotifyIcon(context, icon, panelToReceiveMessages, text);
}

//-----------------------------------------------------------------------------
// Purpose: Plays a sound
// Input  : *fileName - name of the wav file
//-----------------------------------------------------------------------------
void CSurface2::PlaySound(const char *fileName)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->PlaySound(fileName);
	}
	return g_pSurface->PlaySound(fileName);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSurface2::GetPopupCount(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetPopupCount();
	}
	return g_pSurface->GetPopupCount();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPANEL CSurface2::GetPopup(int index)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetPopup(index);
	}
	return g_pSurface->GetPopup(index);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSurface2::ShouldPaintChildPanel(VPANEL childPanel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->ShouldPaintChildPanel(childPanel);
	}
	return g_pSurface->ShouldPaintChildPanel(childPanel);
}

//-----------------------------------------------------------------------------
// Purpose: Applies any changes to the panel into the underline wnidow
//-----------------------------------------------------------------------------
bool CSurface2::RecreateContext(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->RecreateContext(panel);
	}
	return g_pSurface->RecreateContext(panel);
}

//-----------------------------------------------------------------------------
// Purpose: Called when a panel is created
//-----------------------------------------------------------------------------
void CSurface2::AddPanel(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->AddPanel(panel);
	}
	return g_pSurface->AddPanel(panel);
}

//-----------------------------------------------------------------------------
// Purpose: Called when a panel gets deleted
//-----------------------------------------------------------------------------
void CSurface2::ReleasePanel(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->ReleasePanel(panel);
	}
	return g_pSurface->ReleasePanel(panel);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::MovePopupToFront(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->MovePopupToFront(panel);
	}
	return g_pSurface->MovePopupToFront(panel);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::MovePopupToBack(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->MovePopupToBack(panel);
	}
	return g_pSurface->MovePopupToBack(panel);
}

//-----------------------------------------------------------------------------
// Purpose: Walks through the panel tree calling Solve() on them all, in order
//-----------------------------------------------------------------------------
void CSurface2::SolveTraverse(VPANEL panel, bool forceApplySchemeSettings)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SolveTraverse(panel, forceApplySchemeSettings);
	}
	return g_pSurface->SolveTraverse(panel, forceApplySchemeSettings);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::PaintTraverse(VPANEL panel)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->PaintTraverse(panel);
	}
	return g_pSurface->PaintTraverse(panel);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface2::EnableMouseCapture(VPANEL panel, bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->EnableMouseCapture(panel, state);
	}
	return g_pSurface->EnableMouseCapture(panel, state);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the bounds of the usable workspace area
//-----------------------------------------------------------------------------
void CSurface2::GetWorkspaceBounds(int &x, int &y, int &wide, int &tall)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetWorkspaceBounds(x, y, wide, tall);
	}
	return g_pSurface->GetWorkspaceBounds(x, y, wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: gets the absolute coordinates of the screen (in screen space)
//-----------------------------------------------------------------------------
void CSurface2::GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetAbsoluteWindowBounds(x, y, wide, tall);
	}
	return g_pSurface->GetAbsoluteWindowBounds(x, y, wide, tall);
}

void CSurface2::GetProportionalBase(int &width, int &height)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetProportionalBase(width, height);
	}
	g_pSurface->GetProportionalBase(width, height);
}

void CSurface2::GetHDProportionalBase(int& width, int& height)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->GetHDProportionalBase(width, height);
	}

	//Fallback
	return g_pSurface->GetProportionalBase(width, height);
}
//-----------------------------------------------------------------------------
// Purpose: Checks to see if the mouse should be visible or not
//-----------------------------------------------------------------------------
void CSurface2::CalculateMouseVisible(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->CalculateMouseVisible();
	}
	return g_pSurface->CalculateMouseVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSurface2::NeedKBInput(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->NeedKBInput();
	}
	return g_pSurface->NeedKBInput();
}

bool CSurface2::HasCursorPosFunctions(void)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->HasCursorPosFunctions();
	}
	return g_pSurface->HasCursorPosFunctions();
}

void CSurface2::SurfaceGetCursorPos(int &x, int &y)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SurfaceGetCursorPos(x, y);
	}
	return g_pSurface->SurfaceGetCursorPos(x, y);
}

void CSurface2::SurfaceSetCursorPos(int x, int y)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SurfaceSetCursorPos(x, y);
	}
	return g_pSurface->SurfaceSetCursorPos(x, y);
}

// Source specific interfaces
void CSurface2::DrawTexturedLine(const Vertex_t &a, const Vertex_t &b)
{
}

void CSurface2::DrawOutlinedCircle(int x, int y, int radius, int segments)
{
}

void CSurface2::DrawTexturedPolyLine(const Vertex_t *p, int n)
{
}

void CSurface2::DrawTexturedSubRect(int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1)
{
}

void CSurface2::DrawTexturedPolygon(int n, Vertex_t *pVertices)
{
}

const wchar_t *CSurface2::GetTitle(VPANEL panel)
{
	return L"";
}

bool CSurface2::IsCursorLocked(void) const
{
	return false;
}

void CSurface2::SetWorkspaceInsets(int left, int top, int right, int bottom)
{
}

// Lower level char drawing code, call DrawGet then pass in info to DrawRender
bool CSurface2::DrawGetUnicodeCharRenderInfo(wchar_t ch, CharRenderInfo &info)
{
	info.valid = false;
	return false;
}

void CSurface2::DrawRenderCharFromInfo(const CharRenderInfo &info)
{
}

void CSurface2::DrawSetAlphaMultiplier(float alpha)
{
	m_flAlphaMultiplier = alpha;
}

float CSurface2::DrawGetAlphaMultiplier(void)
{
	return m_flAlphaMultiplier;
}

void CSurface2::SetAllowHTMLJavaScript(bool state)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->SetAllowHTMLJavaScript(state);
	}
	return g_pSurface->SetAllowHTMLJavaScript(state);
}

// screen size changing
void CSurface2::OnScreenSizeChanged(int nOldWidth, int nOldHeight)
{
}

// We don't support this for non material system surfaces (we could)
HCursor CSurface2::CreateCursorFromFile(const char *curOrAniFile, const char *pPathID)
{
	return dc_arrow;
}

void *CSurface2::DrawGetTextureMatInfoFactory(int id)
{
	return NULL;
}

void CSurface2::PaintTraverseEx(VPANEL panel, bool paintPopups)
{
	if (g_pSurface_HL25)
	{
		return g_pSurface_HL25->PaintTraverse(panel);
	}
	return g_pSurface->PaintTraverse(panel);
}

float CSurface2::GetZPos(void) const
{
	return 0.0f;
}

void CSurface2::SetPanelForInput(VPANEL vpanel)
{
}

void CSurface2::DrawFilledRectFade(int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal)
{
}

void CSurface2::DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, int imageFormat)
{
}

void CSurface2::DrawSetTextScale(float sx, float sy)
{
}

bool CSurface2::SetBitmapFontGlyphSet(HFont font, const char *windowsFontName, float scalex, float scaley, int flags)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Pre-compiled bitmap font support for game engine - not implemented for GDI
//-----------------------------------------------------------------------------
bool CSurface2::AddBitmapFontFile(const char *fontFileName)
{
	return false;
}

void CSurface2::SetBitmapFontName(const char *pName, const char *pFontFilename)
{
}

const char *CSurface2::GetBitmapFontName(const char *fontFileName)
{
	return NULL;
}

void CSurface2::ClearTemporaryFontCache(void)
{
}

IImage *CSurface2::GetIconImageForFullPath(const char *pFullPath)
{
	return NULL;
}

void CSurface2::DrawUnicodeString(const wchar_t *pwString, FontDrawType_t drawType)
{
	if (!pwString)
		return;

	while (1)
	{
		auto ch = (*pwString); 
		if (!ch)
			break;
		DrawUnicodeChar(ch);

		pwString++;
	}
}

void CSurface2::PrecacheFontCharacters(HFont font, wchar_t *pCharacters)
{

}

const char *CSurface2::GetResolutionKey(void) const
{
	return NULL;
}