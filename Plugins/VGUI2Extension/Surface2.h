#ifndef SURFACE2_H
#define SURFACE2_H

#include "VGUI.h"
#include "IHTML.h"
#include "utldict.h"
#include <mathlib/vector2d.h>
#include <Color.h>
#include <ISurface2.h>

namespace vgui
{

class CSurface2 : public ISurface2
{
public:
	CSurface2();

	// call to Shutdown surface; surface can no longer be used after this is called
	void Shutdown( void );

	// frame
	void RunFrame( void );

	// hierarchy root
	VPANEL GetEmbeddedPanel( void );
	void SetEmbeddedPanel( VPANEL pPanel );

	// drawing context
	void PushMakeCurrent( VPANEL panel, bool useInsets );
	void PopMakeCurrent( VPANEL panel );

	// rendering functions
	void DrawSetColor( int r, int g, int b, int a );
	void DrawSetColor( Color col );

	void DrawFilledRect( int x0, int y0, int x1, int y1 );
	void DrawFilledRectArray( IntRect *pRects, int numRects );
	void DrawOutlinedRect( int x0, int y0, int x1, int y1 );

	void DrawLine( int x0, int y0, int x1, int y1 );
	void DrawPolyLine( int *px, int *py, int numPoints );

	void DrawSetTextFont( HFont font );
	void DrawSetTextColor( int r, int g, int b, int a );
	void DrawSetTextColor( Color col );
	void DrawSetTextPos( int x, int y );
	void DrawGetTextPos( int &x, int &y );
	void DrawPrintText( const wchar_t *text, int textLen, FontDrawType_t drawType = FONT_DRAW_DEFAULT );
	void DrawUnicodeChar( wchar_t wch, FontDrawType_t drawType = FONT_DRAW_DEFAULT );

	void DrawFlushText( void );		// flushes any buffered text (for rendering optimizations)
	IHTML *CreateHTMLWindow( IHTMLEvents *events, VPANEL context );
	void PaintHTMLWindow( IHTML *htmlwin );
	void DeleteHTMLWindow( IHTML *htmlwin );

	int DrawGetTextureId( const char *filename );
	bool DrawGetTextureFile( int id, char *filename, int maxlen );
	void DrawSetTextureFile( int id, const char *filename, int hardwareFilter, bool forceReload );
	void DrawSetTextureRGBA( int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload );
	void DrawSetTexture( int id );
	void DrawGetTextureSize( int id, int &wide, int &tall );

	void DrawTexturedRect( int x0, int y0, int x1, int y1 );

	//Added in HL25
	void DrawTexturedRectAdd(int x0, int y0, int x1, int y1);

	bool IsTextureIDValid( int id );

	int CreateNewTextureID( bool procedural = false );

	void GetScreenSize( int &wide, int &tall );
	void SetAsTopMost( VPANEL panel, bool state );
	void BringToFront( VPANEL panel );
	void SetForegroundWindow( VPANEL panel );
	void SetPanelVisible( VPANEL panel, bool state );
	void SetMinimized( VPANEL panel, bool state );
	bool IsMinimized( VPANEL panel );
	void FlashWindow( VPANEL panel, bool state );
	void SetTitle( VPANEL panel, const wchar_t *title );
	void SetAsToolBar( VPANEL panel, bool state );		// removes the window's task bar entry (for context menu's, etc.)

	// windows stuff
	void CreatePopup( VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true, bool kbInput = true );
	void SwapBuffers( VPANEL panel );
	void Invalidate( VPANEL panel );
	void SetCursor( HCursor cursor );
	bool IsCursorVisible( void );
	void ApplyChanges( void );
	bool IsWithin( int x, int y );
	bool HasFocus( void );

	bool SupportsFeature( SurfaceFeature_e feature );

	// restricts what gets drawn to one panel and it's children
	// currently only works in the game
	void RestrictPaintToSinglePanel( VPANEL panel );

	// these two functions obselete, use IInput::SetAppModalSurface() instead
	void SetModalPanel( VPANEL panel );
	VPANEL GetModalPanel( void );

	void UnlockCursor( void );
	void LockCursor( void );
	void SetTranslateExtendedKeys( bool state );
	VPANEL GetTopmostPopup( void );

	// engine-only focus handling (replacing WM_FOCUS windows handling)
	void SetTopLevelFocus( VPANEL panel );

	// fonts
	// creates an empty handle to a vgui font.  windows fonts can be add to this via SetFontGlyphSet().
	HFont CreateFont( void );

	// adds to the font
	enum EFontFlags
	{
		FONTFLAG_NONE,
		FONTFLAG_ITALIC			= 0x001,
		FONTFLAG_UNDERLINE		= 0x002,
		FONTFLAG_STRIKEOUT		= 0x004,
		FONTFLAG_SYMBOL			= 0x008,
		FONTFLAG_ANTIALIAS		= 0x010,
		FONTFLAG_GAUSSIANBLUR	= 0x020,
		FONTFLAG_ROTARY			= 0x040,
		FONTFLAG_DROPSHADOW		= 0x080,
		FONTFLAG_ADDITIVE		= 0x100,
		FONTFLAG_OUTLINE		= 0x200,
		FONTFLAG_CUSTOM			= 0x400,
		FONTFLAG_OUTLINE2		= 0x800,
	};

	bool AddGlyphSetToFont( HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags );

	// adds a custom font file (only supports true type font files (.ttf) for now)
	bool AddCustomFontFile( const char *fontFileName );

	// returns the details about the font
	int GetFontTall( HFont font );
	int GetFontAscent( HFont font, wchar_t wch );
	bool IsFontAdditive( HFont font );
	void GetCharABCwide( HFont font, int ch, int &a, int &b, int &c );
	int GetCharacterWidth( HFont font, int ch );
	void GetTextSize( HFont font, const wchar_t *text, int &wide, int &tall );

	// notify icons?!?
	VPANEL GetNotifyPanel( void );
	void SetNotifyIcon( VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text );

	// plays a sound
	void PlaySound( const char *fileName );

	//!! these functions should not be accessed directly, but only through other vgui items
	//!! need to move these to seperate interface
	int GetPopupCount( void );
	VPANEL GetPopup( int index );
	bool ShouldPaintChildPanel( VPANEL childPanel );
	bool RecreateContext( VPANEL panel );
	void AddPanel( VPANEL panel );
	void ReleasePanel( VPANEL panel );
	void MovePopupToFront( VPANEL panel );
	void MovePopupToBack( VPANEL panel );

	void SolveTraverse( VPANEL panel, bool forceApplySchemeSettings = false );
	void PaintTraverse( VPANEL panel );

	void EnableMouseCapture( VPANEL panel, bool state );

	// returns the size of the workspace
	void GetWorkspaceBounds( int &x, int &y, int &wide, int &tall );

	// gets the absolute coordinates of the screen (in windows space)
	void GetAbsoluteWindowBounds( int &x, int &y, int &wide, int &tall );

	// gets the base resolution used in proportional mode
	void GetProportionalBase( int &width, int &height );

	//Added in HL25
	void GetHDProportionalBase(int& width, int& height);
	void SetProportionalBase(int width, int height);
	void SetHDProportionalBase(int width, int height);

	void CalculateMouseVisible( void );
	bool NeedKBInput( void );

	bool HasCursorPosFunctions( void );
	void SurfaceGetCursorPos( int &x, int &y );
	void SurfaceSetCursorPos( int x, int y );


	// SRC only functions!!!
	void DrawTexturedLine( const Vertex_t &a, const Vertex_t &b );
	void DrawOutlinedCircle( int x, int y, int radius, int segments );
	void DrawTexturedPolyLine( const Vertex_t *p, int n );		// (Note: this connects the first and last points).
	void DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 );
	void DrawTexturedPolygon( int n, Vertex_t *pVertices );
	const wchar_t *GetTitle( VPANEL panel );
	bool IsCursorLocked( void ) const;
	void SetWorkspaceInsets( int left, int top, int right, int bottom );

	// Lower level char drawing code, call DrawGet then pass in info to DrawRender
	bool DrawGetUnicodeCharRenderInfo( wchar_t ch, CharRenderInfo &info );
	void DrawRenderCharFromInfo( const CharRenderInfo &info );

	// global alpha setting functions
	// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
	void DrawSetAlphaMultiplier( float alpha );
	float DrawGetAlphaMultiplier( void );

	// web browser
	void SetAllowHTMLJavaScript( bool state );
	
	// video mode changing
	void OnScreenSizeChanged( int nOldWidth, int nOldHeight );

	HCursor CreateCursorFromFile( const char *curOrAniFile, const char *pPathID );

	// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
	void *DrawGetTextureMatInfoFactory( int id );

	void PaintTraverseEx( VPANEL panel, bool paintPopups = false );

	float GetZPos( void ) const;

	// From the Xbox
	void SetPanelForInput( VPANEL vpanel );
	void DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal );
	void DrawSetTextureRGBAEx( int id, const unsigned char *rgba, int wide, int tall, int imageFormat );
	void DrawSetTextScale( float sx, float sy );
	bool SetBitmapFontGlyphSet( HFont font, const char *windowsFontName, float scalex, float scaley, int flags );
	// adds a bitmap font file
	bool AddBitmapFontFile( const char *fontFileName );
	// sets a symbol for the bitmap font
	void SetBitmapFontName( const char *pName, const char *pFontFilename );
	// gets the bitmap font filename
	const char *GetBitmapFontName( const char *pName );
	void ClearTemporaryFontCache( void );

	IImage *GetIconImageForFullPath( const char *pFullPath );
	void DrawUnicodeString( const wchar_t *pwString, FontDrawType_t drawType );
	void PrecacheFontCharacters( HFont font, wchar_t *pCharacters );
	// Console-only.  Get the string to use for the current video mode for layout files.
	const char *GetResolutionKey( void ) const;

	void DeleteTextureByID(int textureId) override;

	bool IsIMEComposing() const override;
	double GetImeComposingTime() const override;
	double GetAbsoluteTime() const override;
private:
	float m_flAlphaMultiplier;
	CUtlDict< vgui::IImage*, unsigned short >	m_FileTypeImages;
};

}

#endif