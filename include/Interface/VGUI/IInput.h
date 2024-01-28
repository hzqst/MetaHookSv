//===== Copyright ?1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef VGUI_IINPUT_H
#define VGUI_IINPUT_H

#ifdef _WIN32
#pragma once
#endif

#include <interface.h>

#include "vgui/VGUI.h"
#include "vgui/MouseCode.h"
#include "vgui/KeyCode.h"
#include <KeyValues.h>

namespace vgui
{
typedef struct InputContext_s InputContext_t;
class VPanel;
class Cursor;
typedef unsigned long HCursor;
typedef int HInputContext;
#define VGUI_GCS_COMPREADSTR                 0x0001
#define VGUI_GCS_COMPREADATTR                0x0002
#define VGUI_GCS_COMPREADCLAUSE              0x0004
#define VGUI_GCS_COMPSTR                     0x0008
#define VGUI_GCS_COMPATTR                    0x0010
#define VGUI_GCS_COMPCLAUSE                  0x0020
#define VGUI_GCS_CURSORPOS                   0x0080
#define VGUI_GCS_DELTASTART                  0x0100
#define VGUI_GCS_RESULTREADSTR               0x0200
#define VGUI_GCS_RESULTREADCLAUSE            0x0400
#define VGUI_GCS_RESULTSTR                   0x0800
#define VGUI_GCS_RESULTCLAUSE                0x1000
// style bit flags for WM_IME_COMPOSITION
#define VGUI_CS_INSERTCHAR                   0x2000
#define VGUI_CS_NOMOVECARET                  0x4000

class IInput : public IBaseInterface
{
public:
	virtual void SetMouseFocus(VPANEL newMouseFocus) = 0;
	virtual void SetMouseCapture(VPANEL panel) = 0;

	// returns the string name of a scan code
	virtual void GetKeyCodeText(KeyCode code, char *buf, int buflen) = 0;

	// focus
	virtual VPANEL GetFocus(void) = 0;
	virtual VPANEL GetMouseOver(void) = 0;		// returns the panel the mouse is currently over, ignoring mouse capture

	// mouse state
	virtual void SetCursorPos(int x, int y) = 0;
	virtual void GetCursorPos(int &x, int &y) = 0;
	virtual bool WasMousePressed(MouseCode code) = 0;
	virtual bool WasMouseDoublePressed(MouseCode code) = 0;
	virtual bool IsMouseDown(MouseCode code) = 0;

	// cursor override
	virtual void SetCursorOveride(HCursor cursor) = 0;
	virtual HCursor GetCursorOveride(void) = 0;

	// key state
	virtual bool WasMouseReleased(MouseCode code) = 0;
	virtual bool WasKeyPressed(KeyCode code) = 0;
	virtual bool IsKeyDown(KeyCode code) = 0;
	virtual bool WasKeyTyped(KeyCode code) = 0;
	virtual bool WasKeyReleased(KeyCode code) = 0;

	virtual VPANEL GetAppModalSurface(void) = 0;
	// set the modal dialog panel.
	// all events will go only to this panel and its children.
	virtual void SetAppModalSurface(VPANEL panel) = 0;
	// release the modal dialog panel
	// do this when your modal dialog finishes.
	virtual void ReleaseAppModalSurface(void) = 0;

	virtual void GetCursorPosition(int &x, int &y) = 0;

	void SetIMEWindow( void *hwnd );
	void *GetIMEWindow();

	void OnChangeIME( bool forward );
	int  GetCurrentIMEHandle();
	int  GetEnglishIMEHandle();

	// Returns the Language Bar label (Chinese, Korean, Japanese, Russion, Thai, etc.)
	void GetIMELanguageName( wchar_t *buf, int unicodeBufferSizeInBytes );
	// Returns the short code for the language (EN, CH, KO, JP, RU, TH, etc. ).
	void GetIMELanguageShortCode( wchar_t *buf, int unicodeBufferSizeInBytes );

	struct LanguageItem
	{
		wchar_t		shortname[ 4 ];
		wchar_t		menuname[ 128 ];
		int			handleValue;
		bool		active; // true if this is the active language
	};

	struct ConversionModeItem
	{
		wchar_t		menuname[ 128 ];
		int			handleValue;
		bool		active; // true if this is the active conversion mode
	};

	struct SentenceModeItem
	{
		wchar_t		menuname[ 128 ];
		int			handleValue;
		bool		active; // true if this is the active sentence mode
	};

	// Call with NULL dest to get item count
	int	 GetIMELanguageList( LanguageItem *dest, int destcount );
	int	 GetIMEConversionModes( ConversionModeItem *dest, int destcount );
	int	 GetIMESentenceModes( SentenceModeItem *dest, int destcount );

	void OnChangeIMEByHandle( int handleValue );
	void OnChangeIMEConversionModeByHandle( int handleValue );
	void OnChangeIMESentenceModeByHandle( int handleValue );

	void OnInputLanguageChanged();
	void OnIMEStartComposition();
	void OnIMEComposition( int flags );
	void OnIMEEndComposition();

	void OnIMEShowCandidates();
	void OnIMEChangeCandidates();
	void OnIMECloseCandidates();
	void OnIMERecomputeModes();

	int  GetCandidateListCount();
	void GetCandidate( int num, wchar_t *dest, int destSizeBytes );
	int  GetCandidateListSelectedItem();
	int  GetCandidateListPageSize();
	int  GetCandidateListPageStart();

	//NOTE:  We render our own candidate lists most of the time...
	void SetCandidateWindowPos( int x, int y );

	bool GetShouldInvertCompositionString();
	bool CandidateListStartsAtOne();

	void SetCandidateListPageStart( int start );

	void GetCompositionString(wchar_t *dest, int destSizeBytes);
	void OnIMESelectCandidate(int num);
	bool PostKeyMessage(KeyValues *message);
	void DestroyCandidateList(void);
	void CreateNewCandidateList(void);
	void InternalShowCandidateWindow(void);
	void InternalHideCandidateWindow(void);
	void InternalUpdateCandidateWindow(void);
	void InternalSetCompositionString(const wchar_t *compstr);
	void OnKeyCodeUnhandled(int keyCode);
	bool ShouldModalSubTreeReceiveMessages() const;
	VPANEL GetModalSubTree(void);
private:

};

#define VGUI_INPUT_INTERFACE_VERSION "VGUI_Input004"

} // namespace vgui

#endif // VGUI_IINPUT_H
