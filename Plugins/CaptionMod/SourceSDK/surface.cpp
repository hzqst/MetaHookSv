#include <vgui/ISurface.h>
#include <vgui/ISurface2.h>
#include <vgui/Cursor.h>

extern vgui::ISurface *g_pSurface;

using namespace vgui;

static CSurface s_Surface;

vgui::CSurface *g_pVGuiSurface = &s_Surface;

CSurface::CSurface()
{
	m_flAlphaMultiplier = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down app
//-----------------------------------------------------------------------------
void CSurface::Shutdown( void )
{
	return g_pSurface->Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Handles windows message pump
//-----------------------------------------------------------------------------
void CSurface::RunFrame( void )
{
	return g_pSurface->RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPANEL CSurface::GetEmbeddedPanel( void )
{
	return g_pSurface->GetEmbeddedPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the panel for use
// Input  : *embeddedPanel - Main panel that becomes the top of the hierarchy
//-----------------------------------------------------------------------------
void CSurface::SetEmbeddedPanel( VPANEL pPanel )
{
	return g_pSurface->SetEmbeddedPanel( pPanel );
}

void CSurface::PushMakeCurrent( VPANEL panel, bool useInsets )
{
	return g_pSurface->PushMakeCurrent( panel, useInsets );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::PopMakeCurrent( VPANEL panel )
{
	return g_pSurface->PopMakeCurrent( panel );
}

void CSurface::DrawSetColor( int r, int g, int b, int a )
{
	return g_pSurface->DrawSetColor( r, g, b, a*m_flAlphaMultiplier );
}

void CSurface::DrawSetColor( Color col )
{
	col[3] *= m_flAlphaMultiplier;
	return g_pSurface->DrawSetColor( col );
}

void CSurface::DrawFilledRect( int x0, int y0, int x1, int y1 )
{
	return g_pSurface->DrawFilledRect( x0, y0, x1, y1 );
}

void CSurface::DrawFilledRectArray( IntRect *pRects, int numRects )
{
	for (int i = 0; i < numRects; ++i)
	{
		DrawFilledRect( pRects[i].x0, pRects[i].y0, pRects[i].x1, pRects[i].y1 );
	}
}

void CSurface::DrawOutlinedRect( int x0, int y0, int x1, int y1 )
{
	return g_pSurface->DrawOutlinedRect( x0, y0, x1, y1 );
}

void CSurface::DrawLine( int x0, int y0, int x1, int y1 )
{
	return g_pSurface->DrawLine( x0, y0, x1, y1 );
}

void CSurface::DrawPolyLine( int *px, int *py, int numPoints )
{
	return g_pSurface->DrawPolyLine( px, py, numPoints );
}

void CSurface::DrawSetTextFont( HFont font )
{
	return g_pSurface->DrawSetTextFont( font );
}

void CSurface::DrawSetTextColor( int r, int g, int b, int a )
{
	return g_pSurface->DrawSetTextColor( r, g, b, a*m_flAlphaMultiplier );
}

void CSurface::DrawSetTextColor( Color col )
{
	col[3] *= m_flAlphaMultiplier;
	return g_pSurface->DrawSetTextColor( col );
}

void CSurface::DrawSetTextPos( int x, int y )
{
	return g_pSurface->DrawSetTextPos( x, y );
}

void CSurface::DrawGetTextPos( int &x, int &y )
{
	return g_pSurface->DrawGetTextPos( x, y );
}

void CSurface::DrawPrintText( const wchar_t *text, int textLen, FontDrawType_t drawType )
{
	g_pSurface->DrawPrintText( text, textLen );
}

//-----------------------------------------------------------------------------
// Purpose: draws single unicode character at the current position with the
//			current font & color
//-----------------------------------------------------------------------------
void CSurface::DrawUnicodeChar( wchar_t wch, FontDrawType_t drawType )
{
	return g_pSurface->DrawUnicodeChar( wch );
}

//-----------------------------------------------------------------------------
// Purpose: does nothing, since we don't need this optimization in win32
//-----------------------------------------------------------------------------
void CSurface::DrawFlushText( void )
{
	return g_pSurface->DrawFlushText();
}

IHTML *CSurface::CreateHTMLWindow( IHTMLEvents *events, VPANEL context )
{
	return g_pSurface->CreateHTMLWindow( events, context );
}

void CSurface::PaintHTMLWindow( IHTML *htmlwin )
{
	return g_pSurface->PaintHTMLWindow( htmlwin );
}

void CSurface::DeleteHTMLWindow( IHTML *htmlwin )
{
	g_pSurface->DeleteHTMLWindow( htmlwin );
}

int CSurface::DrawGetTextureId( const char *filename )
{
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//			*filename - 
//			maxlen - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSurface::DrawGetTextureFile( int id, char *filename, int maxlen )
{
	filename[0] = 0;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Maps a texture file to an id, and makes it the current drawing texture
//			tries to load as a .tga first, and if not found as a .bmp
//-----------------------------------------------------------------------------
void CSurface::DrawSetTextureFile( int id, const char *filename, int hardwareFilter, bool forceReload )
{
	return g_pSurface->DrawSetTextureFile( id, filename, hardwareFilter, forceReload );
}

//-----------------------------------------------------------------------------
// Purpose: maps a texture from memory to an id, and uploads it into the engine
//-----------------------------------------------------------------------------
void CSurface::DrawSetTextureRGBA( int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload )
{
	return g_pSurface->DrawSetTextureRGBA( id, rgba, wide, tall, hardwareFilter, forceReload );
}

//-----------------------------------------------------------------------------
// Purpose: sets the current active texture
//-----------------------------------------------------------------------------
void CSurface::DrawSetTexture( int id )
{
	return g_pSurface->DrawSetTexture( id );
}

//-----------------------------------------------------------------------------
// Purpose: Called by vgui to get texture dimensions
//-----------------------------------------------------------------------------
void CSurface::DrawGetTextureSize( int id, int &wide, int &tall )
{
	return g_pSurface->DrawGetTextureSize( id, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::DrawTexturedRect( int x0, int y0, int x1, int y1 )
{
	return g_pSurface->DrawTexturedRect( x0, y0, x1, y1 );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the texture id has a valid texture bound to it
//-----------------------------------------------------------------------------
bool CSurface::IsTextureIDValid( int id )
{
	return g_pSurface->IsTextureIDValid( id );
}

//-----------------------------------------------------------------------------
// Purpose: allocates a new texture id
//-----------------------------------------------------------------------------
int CSurface::CreateNewTextureID( bool procedural )
{
	return g_pSurface->CreateNewTextureID( procedural );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::GetScreenSize( int &wide, int &tall )
{
	return g_pSurface->GetScreenSize( wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: 
// HWND_TOPMOST - Places the window above all non-topmost windows. 
// The window maintains its topmost position even when it is deactivated.
// HWND_NOTOPMOST - Places the window above all non-topmost windows (that is, behind 
// all topmost windows). This flag has no effect if the window is already a non-topmost window.
//-----------------------------------------------------------------------------
void CSurface::SetAsTopMost( VPANEL panel, bool state )
{
	return g_pSurface->SetAsTopMost( panel, state );
}

//-----------------------------------------------------------------------------
// Purpose: brings the current surface to the foreground
//-----------------------------------------------------------------------------
void CSurface::BringToFront( VPANEL panel )
{
	return g_pSurface->BringToFront( panel );
}

//-----------------------------------------------------------------------------
// Purpose: puts the thread that created the specified window into the foreground 
//          and activates the window.
//-----------------------------------------------------------------------------
void CSurface::SetForegroundWindow( VPANEL panel )
{
	return g_pSurface->SetForegroundWindow( panel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::SetPanelVisible( VPANEL panel, bool state )
{
	return g_pSurface->SetPanelVisible( panel, state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::SetMinimized( VPANEL panel, bool state )
{
	return g_pSurface->SetMinimized( panel, state );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the window is minimized
//-----------------------------------------------------------------------------
bool CSurface::IsMinimized( VPANEL panel )
{
	return g_pSurface->IsMinimized( panel );
}

//-----------------------------------------------------------------------------
// Purpose: Flashes the window icon in the taskbar, to get the users attention
// Input  : flashCount - number of times to flash the window
//-----------------------------------------------------------------------------
void CSurface::FlashWindow( VPANEL panel, bool state )
{
	return g_pSurface->FlashWindow( panel, state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::SetTitle( VPANEL panel, const wchar_t *title )
{
	return g_pSurface->SetTitle( panel, title );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::SetAsToolBar( VPANEL panel, bool state )
{
	return g_pSurface->SetAsToolBar( panel, state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::CreatePopup( VPANEL panel, bool minimised, bool showTaskbarIcon, bool disabled, bool mouseInput, bool kbInput )
{
	return g_pSurface->CreatePopup( panel, minimised, showTaskbarIcon, disabled, mouseInput, kbInput );
}

//-----------------------------------------------------------------------------
// Purpose: Called after a Paint to display the new buffer
//-----------------------------------------------------------------------------
void CSurface::SwapBuffers( VPANEL panel )
{
	return g_pSurface->SwapBuffers( panel );
}

//-----------------------------------------------------------------------------
// Purpose: Forces the window to be redrawn
//-----------------------------------------------------------------------------
void CSurface::Invalidate( VPANEL panel )
{
	return g_pSurface->Invalidate( panel );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the current cursor
//-----------------------------------------------------------------------------
void CSurface::SetCursor( HCursor cursor )
{
	return g_pSurface->SetCursor( cursor );
}

bool CSurface::IsCursorVisible( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame to change to window state if necessary
//-----------------------------------------------------------------------------
void CSurface::ApplyChanges( void )
{
	return g_pSurface->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the cursor is over this surface
//			Uses the windows call to do this, instead of doing it procedurally
//-----------------------------------------------------------------------------
bool CSurface::IsWithin( int x, int y )
{
	return g_pSurface->IsWithin( x, y );
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if any of the windows have focus
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSurface::HasFocus( void )
{
	return g_pSurface->HasFocus();
}

//-----------------------------------------------------------------------------
// Purpose: cap bits
//-----------------------------------------------------------------------------
bool CSurface::SupportsFeature( SurfaceFeature_e feature )
{
	return g_pSurface->SupportsFeature( (ISurface::SurfaceFeature_e)feature );
}

// FIXME: write these functions!
void CSurface::RestrictPaintToSinglePanel( VPANEL panel )
{
	return g_pSurface->RestrictPaintToSinglePanel( panel );
}

void CSurface::SetModalPanel( VPANEL panel )
{
	return g_pSurface->SetModalPanel( panel );
}

VPANEL CSurface::GetModalPanel( void )
{
	return g_pSurface->GetModalPanel();
}

void CSurface::UnlockCursor( void )
{
	return g_pSurface->UnlockCursor();
}

void CSurface::LockCursor( void )
{
	return g_pSurface->LockCursor();
}

void CSurface::SetTranslateExtendedKeys( bool state )
{
	return g_pSurface->SetTranslateExtendedKeys( state );
}

VPANEL CSurface::GetTopmostPopup( void )
{
	return g_pSurface->GetTopmostPopup();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::SetTopLevelFocus( VPANEL panel )
{
	return g_pSurface->SetTopLevelFocus( panel );
}

//-----------------------------------------------------------------------------
// Purpose: creates a new empty font
//-----------------------------------------------------------------------------
HFont CSurface::CreateFont( void )
{
	return g_pSurface->CreateFont();
}

//-----------------------------------------------------------------------------
// Purpose: adds glyphs to a font created by CreateFont()
//-----------------------------------------------------------------------------
bool CSurface::AddGlyphSetToFont( HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags )
{
	return g_pSurface->AddGlyphSetToFont( font, windowsFontName, tall, weight, blur, scanlines, flags, 0x0, 0xFFFF );
}

//-----------------------------------------------------------------------------
// Purpose: adds a custom font file (only supports true type font files (.ttf) for now)
//-----------------------------------------------------------------------------
bool CSurface::AddCustomFontFile( const char *fontFileName )
{
	return g_pSurface->AddCustomFontFile( fontFileName );
}

//-----------------------------------------------------------------------------
// Purpose: returns the max height of a font
//-----------------------------------------------------------------------------
int CSurface::GetFontTall( HFont font )
{
	return g_pSurface->GetFontTall( font );
}

//-----------------------------------------------------------------------------
// Purpose: returns the max height of a font
//-----------------------------------------------------------------------------
int CSurface::GetFontAscent( HFont font, wchar_t wch )
{
	return g_pSurface->GetFontAscent( font, wch );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSurface::IsFontAdditive( HFont font )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns the abc widths of a single character
//-----------------------------------------------------------------------------
void CSurface::GetCharABCwide( HFont font, int ch, int &a, int &b, int &c )
{
	return g_pSurface->GetCharABCwide( font, ch, a, b, c );
}

//-----------------------------------------------------------------------------
// Purpose: returns the pixel width of a single character
//-----------------------------------------------------------------------------
int CSurface::GetCharacterWidth( HFont font, int ch )
{
	return g_pSurface->GetCharacterWidth( font, ch );
}

//-----------------------------------------------------------------------------
// Purpose: returns the area of a text string, including newlines
//-----------------------------------------------------------------------------
void CSurface::GetTextSize( HFont font, const wchar_t *text, int &wide, int &tall )
{
	return g_pSurface->GetTextSize( font, text, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPANEL CSurface::GetNotifyPanel( void )
{
	return g_pSurface->GetNotifyPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::SetNotifyIcon( VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text )
{
	return g_pSurface->SetNotifyIcon( context, icon, panelToReceiveMessages, text );
}

//-----------------------------------------------------------------------------
// Purpose: Plays a sound
// Input  : *fileName - name of the wav file
//-----------------------------------------------------------------------------
void CSurface::PlaySound( const char *fileName )
{
	return g_pSurface->PlaySound( fileName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSurface::GetPopupCount( void )
{
	return g_pSurface->GetPopupCount();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPANEL CSurface::GetPopup( int index )
{
	return g_pSurface->GetPopup( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSurface::ShouldPaintChildPanel( VPANEL childPanel )
{
	return g_pSurface->ShouldPaintChildPanel( childPanel );
}

//-----------------------------------------------------------------------------
// Purpose: Applies any changes to the panel into the underline wnidow
//-----------------------------------------------------------------------------
bool CSurface::RecreateContext( VPANEL panel )
{
	return g_pSurface->RecreateContext( panel );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a panel is created
//-----------------------------------------------------------------------------
void CSurface::AddPanel( VPANEL panel )
{
	return g_pSurface->AddPanel( panel );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a panel gets deleted
//-----------------------------------------------------------------------------
void CSurface::ReleasePanel( VPANEL panel )
{
	return g_pSurface->ReleasePanel( panel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::MovePopupToFront( VPANEL panel )
{
	return g_pSurface->MovePopupToFront( panel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::MovePopupToBack( VPANEL panel )
{
	return g_pSurface->MovePopupToBack( panel );
}

//-----------------------------------------------------------------------------
// Purpose: Walks through the panel tree calling Solve() on them all, in order
//-----------------------------------------------------------------------------
void CSurface::SolveTraverse( VPANEL panel, bool forceApplySchemeSettings )
{
	return g_pSurface->SolveTraverse( panel, forceApplySchemeSettings );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::PaintTraverse( VPANEL panel )
{
	return g_pSurface->PaintTraverse( panel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSurface::EnableMouseCapture( VPANEL panel, bool state )
{
	return g_pSurface->EnableMouseCapture( panel, state );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the bounds of the usable workspace area
//-----------------------------------------------------------------------------
void CSurface::GetWorkspaceBounds( int &x, int &y, int &wide, int &tall )
{
	return g_pSurface->GetWorkspaceBounds( x, y, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: gets the absolute coordinates of the screen (in screen space)
//-----------------------------------------------------------------------------
void CSurface::GetAbsoluteWindowBounds( int &x, int &y, int &wide, int &tall )
{
	return g_pSurface->GetAbsoluteWindowBounds( x, y, wide, tall );
}

void CSurface::GetProportionalBase( int &width, int &height )
{
	return g_pSurface->GetProportionalBase( width, height );
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the mouse should be visible or not
//-----------------------------------------------------------------------------
void CSurface::CalculateMouseVisible( void )
{
	return g_pSurface->CalculateMouseVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSurface::NeedKBInput( void )
{
	return g_pSurface->NeedKBInput();
}

bool CSurface::HasCursorPosFunctions( void )
{
	return g_pSurface->HasCursorPosFunctions();
}

void CSurface::SurfaceGetCursorPos( int &x, int &y )
{
	return g_pSurface->SurfaceGetCursorPos( x, y );
}

void CSurface::SurfaceSetCursorPos( int x, int y )
{
	return g_pSurface->SurfaceSetCursorPos( x, y );
}

// SRC specific interfaces
void CSurface::DrawTexturedLine( const Vertex_t &a, const Vertex_t &b )
{
}

void CSurface::DrawOutlinedCircle( int x, int y, int radius, int segments )
{
}

void CSurface::DrawTexturedPolyLine( const Vertex_t *p, int n )
{
}

void CSurface::DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 )
{
}

void CSurface::DrawTexturedPolygon( int n, Vertex_t *pVertices )
{
}

const wchar_t *CSurface::GetTitle( VPANEL panel )
{
	return L"";
}

bool CSurface::IsCursorLocked( void ) const
{
	return false;
}

void CSurface::SetWorkspaceInsets( int left, int top, int right, int bottom )
{
}

// Lower level char drawing code, call DrawGet then pass in info to DrawRender
bool CSurface::DrawGetUnicodeCharRenderInfo( wchar_t ch, CharRenderInfo &info )
{
	info.valid = false;
	return false;
}

void CSurface::DrawRenderCharFromInfo( const CharRenderInfo &info )
{
}

void CSurface::DrawSetAlphaMultiplier( float alpha )
{
	m_flAlphaMultiplier = alpha;
}

float CSurface::DrawGetAlphaMultiplier( void )
{
	return m_flAlphaMultiplier;
}

void CSurface::SetAllowHTMLJavaScript( bool state )
{
	return g_pSurface->SetAllowHTMLJavaScript( state );
}

// screen size changing
void CSurface::OnScreenSizeChanged( int nOldWidth, int nOldHeight )
{
}

// We don't support this for non material system surfaces (we could)
HCursor CSurface::CreateCursorFromFile( const char *curOrAniFile, const char *pPathID )
{
	return dc_arrow;
}

IVguiMatInfo *CSurface::DrawGetTextureMatInfoFactory( int id )
{
	return NULL;
}

void CSurface::PaintTraverseEx( VPANEL panel, bool paintPopups )
{
	return g_pSurface->PaintTraverse( panel );
}

float CSurface::GetZPos( void ) const
{
	return 0.0f;
}

void CSurface::SetPanelForInput( VPANEL vpanel )
{
}

void CSurface::DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal )
{
}

void CSurface::DrawSetTextureRGBAEx( int id, const unsigned char *rgba, int wide, int tall, int imageFormat )
{
}

void CSurface::DrawSetTextScale( float sx, float sy )
{
}

bool CSurface::SetBitmapFontGlyphSet( HFont font, const char *windowsFontName, float scalex, float scaley, int flags )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Pre-compiled bitmap font support for game engine - not implemented for GDI
//-----------------------------------------------------------------------------
bool CSurface::AddBitmapFontFile( const char *fontFileName )
{
	return false;
}

void CSurface::SetBitmapFontName( const char *pName, const char *pFontFilename )
{
}

const char *CSurface::GetBitmapFontName( const char *fontFileName )
{
	return NULL;
}

void CSurface::ClearTemporaryFontCache( void )
{
}

IImage *CSurface::GetIconImageForFullPath( const char *pFullPath )
{
	return NULL;
}

void CSurface::DrawUnicodeString( const wchar_t *pwString, int drawType )
{
	if (!pwString)
		return;

	while ( wchar_t ch = *pwString++ )
	{
		g_pSurface->DrawUnicodeChar( ch );
	}
}

void CSurface::PrecacheFontCharacters( HFont font, wchar_t *pCharacters )
{
}

const char *CSurface::GetResolutionKey( void ) const
{
	return NULL;
}