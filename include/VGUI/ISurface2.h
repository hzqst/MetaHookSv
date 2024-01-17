#ifndef ISURFACE2_H
#define ISURFACE2_H

#include <interface.h>
#include "VGUI.h"
#include "IHTML.h"
#include <mathlib/vector2d.h>
#include <Color.h>

namespace vgui
{

class IImage;

// SRC only defines

struct Vertex_t
{
	Vertex_t() {}
	Vertex_t( const Vector2D &pos, const Vector2D &coord = Vector2D( 0, 0 ) )
	{
		m_Position = pos;
		m_TexCoord = coord;
	}
	void Init( const Vector2D &pos, const Vector2D &coord = Vector2D( 0, 0 ) )
	{
		m_Position = pos;
		m_TexCoord = coord;
	}
	
	Vector2D	m_Position;
	Vector2D	m_TexCoord;
};

enum FontDrawType_t
{
	// Use the "additive" value from the scheme file
	FONT_DRAW_DEFAULT = 0,

	// Overrides
	FONT_DRAW_NONADDITIVE,
	FONT_DRAW_ADDITIVE,

	FONT_DRAW_TYPE_COUNT = 2,
};

// Refactor these two
struct CharRenderInfo
{
	// In:
	FontDrawType_t	drawType;
	wchar_t			ch;

	// Out
	bool			valid;

	// In/Out (true by default)
	bool			shouldclip;
	// Text pos
	int				x, y;
	// Top left and bottom right
	Vertex_t		verts[ 2 ];
	int				textureId;
	int				abcA;
	int				abcB;
	int				abcC;
	int				fontTall;
	HFont			currentFont;
};

struct IntRect
{
	int x0;
	int y0;
	int x1;
	int y1;
};


#ifndef VGUI2_SURFACE_FEATURE
#define VGUI2_SURFACE_FEATURE
// returns true if the surface supports minimize & maximize capabilities
enum SurfaceFeature_e
{
	ANTIALIASED_FONTS = 1,
	DROPSHADOW_FONTS = 2,
	ESCAPE_KEY = 3,
	OPENING_NEW_HTML_WINDOWS = 4,
	FRAME_MINIMIZE_MAXIMIZE = 5,
	DIRECT_HWND_RENDER = 6,
	OUTLINE_FONTS = 7,
};

#endif

//Simple wrapper around original g_pSurface and g_pSurface_HL25

class ISurface2 : public IBaseInterface
{
public:

	// adds to the font
	enum EFontFlags
	{
		FONTFLAG_NONE,
		FONTFLAG_ITALIC = 0x001,
		FONTFLAG_UNDERLINE = 0x002,
		FONTFLAG_STRIKEOUT = 0x004,
		FONTFLAG_SYMBOL = 0x008,
		FONTFLAG_ANTIALIAS = 0x010,
		FONTFLAG_GAUSSIANBLUR = 0x020,
		FONTFLAG_ROTARY = 0x040,
		FONTFLAG_DROPSHADOW = 0x080,
		FONTFLAG_ADDITIVE = 0x100,
		FONTFLAG_OUTLINE = 0x200,
		FONTFLAG_CUSTOM = 0x400,
		FONTFLAG_OUTLINE2 = 0x800,
	};

public:
	virtual void Shutdown( void ) = 0;
	virtual void RunFrame( void );
	virtual VPANEL GetEmbeddedPanel( void );
	virtual void SetEmbeddedPanel( VPANEL pPanel );
	virtual void PushMakeCurrent( VPANEL panel, bool useInsets );
	virtual void PopMakeCurrent( VPANEL panel );
	virtual void DrawSetColor( int r, int g, int b, int a );
	virtual void DrawSetColor( Color col );
	virtual void DrawFilledRect( int x0, int y0, int x1, int y1 );
	virtual void DrawFilledRectArray( IntRect *pRects, int numRects );
	virtual void DrawOutlinedRect( int x0, int y0, int x1, int y1 );
	virtual void DrawLine( int x0, int y0, int x1, int y1 );
	virtual void DrawPolyLine( int *px, int *py, int numPoints );
	virtual void DrawSetTextFont( HFont font );
	virtual void DrawSetTextColor( int r, int g, int b, int a );
	virtual void DrawSetTextColor( Color col );
	virtual void DrawSetTextPos( int x, int y );
	virtual void DrawGetTextPos( int &x, int &y );
	virtual void DrawPrintText( const wchar_t *text, int textLen, FontDrawType_t drawType = FONT_DRAW_DEFAULT );
	virtual void DrawUnicodeChar( wchar_t wch, FontDrawType_t drawType = FONT_DRAW_DEFAULT );
	virtual void DrawFlushText( void );
	virtual IHTML *CreateHTMLWindow( IHTMLEvents *events, VPANEL context );
	virtual void PaintHTMLWindow( IHTML *htmlwin );
	virtual void DeleteHTMLWindow( IHTML *htmlwin );
	virtual int DrawGetTextureId( const char *filename );
	virtual bool DrawGetTextureFile( int id, char *filename, int maxlen );
	virtual void DrawSetTextureFile( int id, const char *filename, int hardwareFilter, bool forceReload );
	virtual void DrawSetTextureRGBA( int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload );
	virtual void DrawSetTexture( int id );
	virtual void DrawGetTextureSize( int id, int &wide, int &tall );
	virtual void DrawTexturedRect( int x0, int y0, int x1, int y1 );
	virtual void DrawTexturedRectAdd(int x0, int y0, int x1, int y1);
	virtual bool IsTextureIDValid( int id );
	virtual int CreateNewTextureID( bool procedural = false );
	virtual void GetScreenSize( int &wide, int &tall );
	virtual void SetAsTopMost( VPANEL panel, bool state );
	virtual void BringToFront( VPANEL panel );
	virtual void SetForegroundWindow( VPANEL panel );
	virtual void SetPanelVisible( VPANEL panel, bool state );
	virtual void SetMinimized( VPANEL panel, bool state );
	virtual bool IsMinimized( VPANEL panel );
	virtual void FlashWindow( VPANEL panel, bool state );
	virtual void SetTitle( VPANEL panel, const wchar_t *title );
	virtual void SetAsToolBar( VPANEL panel, bool state );
	virtual void CreatePopup( VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true, bool kbInput = true );
	virtual void SwapBuffers( VPANEL panel );
	virtual void Invalidate( VPANEL panel );
	virtual void SetCursor( HCursor cursor );
	virtual bool IsCursorVisible( void );
	virtual void ApplyChanges( void );
	virtual bool IsWithin( int x, int y );
	virtual bool HasFocus( void );
	virtual bool SupportsFeature( SurfaceFeature_e feature );
	virtual void RestrictPaintToSinglePanel( VPANEL panel );
	virtual void SetModalPanel( VPANEL panel );
	virtual VPANEL GetModalPanel( void );
	virtual void UnlockCursor( void );
	virtual void LockCursor( void );
	virtual void SetTranslateExtendedKeys( bool state );
	virtual VPANEL GetTopmostPopup( void );
	virtual void SetTopLevelFocus( VPANEL panel );
	virtual HFont CreateFont( void );
	virtual bool AddGlyphSetToFont( HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags );
	virtual bool AddCustomFontFile( const char *fontFileName );
	virtual int GetFontTall( HFont font );
	virtual int GetFontAscent( HFont font, wchar_t wch );
	virtual bool IsFontAdditive( HFont font );
	virtual void GetCharABCwide( HFont font, int ch, int &a, int &b, int &c );
	virtual int GetCharacterWidth( HFont font, int ch );
	virtual void GetTextSize( HFont font, const wchar_t *text, int &wide, int &tall );
	virtual VPANEL GetNotifyPanel( void );
	virtual void SetNotifyIcon( VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text );
	virtual void PlaySound( const char *fileName );
	virtual int GetPopupCount( void );
	virtual VPANEL GetPopup( int index );
	virtual bool ShouldPaintChildPanel( VPANEL childPanel );
	virtual bool RecreateContext( VPANEL panel );
	virtual void AddPanel( VPANEL panel );
	virtual void ReleasePanel( VPANEL panel );
	virtual void MovePopupToFront( VPANEL panel );
	virtual void MovePopupToBack( VPANEL panel );
	virtual void SolveTraverse( VPANEL panel, bool forceApplySchemeSettings = false );
	virtual void PaintTraverse( VPANEL panel );
	virtual void EnableMouseCapture( VPANEL panel, bool state );
	virtual void GetWorkspaceBounds( int &x, int &y, int &wide, int &tall );
	virtual void GetAbsoluteWindowBounds( int &x, int &y, int &wide, int &tall );
	virtual void GetProportionalBase( int &width, int &height );
	virtual void GetHDProportionalBase(int& width, int& height);
	virtual void SetProportionalBase(int width, int height);
	virtual void SetHDProportionalBase(int width, int height);
	virtual void CalculateMouseVisible( void );
	virtual bool NeedKBInput( void );
	virtual bool HasCursorPosFunctions( void );
	virtual void SurfaceGetCursorPos( int &x, int &y );
	virtual void SurfaceSetCursorPos( int x, int y );
	virtual void DrawTexturedLine( const Vertex_t &a, const Vertex_t &b );
	virtual void DrawOutlinedCircle( int x, int y, int radius, int segments );
	virtual void DrawTexturedPolyLine( const Vertex_t *p, int n );
	virtual void DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 );
	virtual void DrawTexturedPolygon( int n, Vertex_t *pVertices );
	virtual const wchar_t *GetTitle( VPANEL panel );
	virtual bool IsCursorLocked( void ) const;
	virtual void SetWorkspaceInsets( int left, int top, int right, int bottom );
	virtual bool DrawGetUnicodeCharRenderInfo( wchar_t ch, CharRenderInfo &info );
	virtual void DrawRenderCharFromInfo( const CharRenderInfo &info );
	virtual void DrawSetAlphaMultiplier( float alpha );
	virtual float DrawGetAlphaMultiplier( void );
	virtual void SetAllowHTMLJavaScript( bool state );
	virtual void OnScreenSizeChanged( int nOldWidth, int nOldHeight );
	virtual HCursor CreateCursorFromFile( const char *curOrAniFile, const char *pPathID );
	virtual void *DrawGetTextureMatInfoFactory( int id );
	virtual void PaintTraverseEx( VPANEL panel, bool paintPopups = false );
	virtual float GetZPos( void ) const;
	virtual void SetPanelForInput( VPANEL vpanel );
	virtual void DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal );
	virtual void DrawSetTextureRGBAEx( int id, const unsigned char *rgba, int wide, int tall, int imageFormat );
	virtual void DrawSetTextScale( float sx, float sy );
	virtual bool SetBitmapFontGlyphSet( HFont font, const char *windowsFontName, float scalex, float scaley, int flags );
	virtual bool AddBitmapFontFile( const char *fontFileName );
	virtual void SetBitmapFontName( const char *pName, const char *pFontFilename );
	virtual const char *GetBitmapFontName( const char *pName );
	virtual void ClearTemporaryFontCache( void );
	virtual IImage *GetIconImageForFullPath( const char *pFullPath );
	virtual void DrawUnicodeString( const wchar_t *pwString, FontDrawType_t drawType );
	virtual void PrecacheFontCharacters( HFont font, wchar_t *pCharacters );
	virtual const char *GetResolutionKey( void ) const;
	virtual void DeleteTextureByID(int textureId);
};

}

#define VGUI_SURFACE2_INTERFACE_VERSION "VGUI_Surface2_001"

#endif