//===== Copyright ?1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef VGUI_IINPUT2_H
#define VGUI_IINPUT2_H

#ifdef _WIN32
#pragma once
#endif

#include <interface.h>

#include "vgui/VGUI.h"
#include "vgui/MouseCode.h"
#include "vgui/KeyCode.h"
#include <vgui/IInputInternal.h>
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


struct LanguageItem
{
	wchar_t		shortname[4];
	wchar_t		menuname[128];
	int			handleValue;
	bool		active; // true if this is the active language
};

struct ConversionModeItem
{
	wchar_t		menuname[128];
	int			handleValue;
	bool		active; // true if this is the active conversion mode
};

struct SentenceModeItem
{
	wchar_t		menuname[128];
	int			handleValue;
	bool		active; // true if this is the active sentence mode
};

class IInput2 : public IInputInternal
{
public:
	virtual void SetIMEWindow( void *hwnd ) = 0;
	virtual void *GetIMEWindow() = 0;
	virtual void OnChangeIME( bool forward ) = 0;
	virtual int  GetCurrentIMEHandle() = 0;
	virtual int  GetEnglishIMEHandle() = 0;
	virtual void GetIMELanguageName( wchar_t *buf, int unicodeBufferSizeInBytes ) = 0;
	virtual void GetIMELanguageShortCode( wchar_t *buf, int unicodeBufferSizeInBytes ) = 0;
	virtual int	 GetIMELanguageList( LanguageItem *dest, int destcount ) = 0;
	virtual int	 GetIMEConversionModes( ConversionModeItem *dest, int destcount ) = 0;
	virtual int	 GetIMESentenceModes( SentenceModeItem *dest, int destcount ) = 0;
	virtual void OnChangeIMEByHandle( int handleValue ) = 0;
	virtual void OnChangeIMEConversionModeByHandle( int handleValue ) = 0;
	virtual void OnChangeIMESentenceModeByHandle( int handleValue ) = 0;
	virtual void OnInputLanguageChanged() = 0;
	virtual void OnIMEStartComposition() = 0;
	virtual void OnIMECompositionWin32( long flags ) = 0;
	virtual void OnIMEEndComposition() = 0;
	virtual void OnIMEShowCandidates() = 0;
	virtual void OnIMEChangeCandidates() = 0;
	virtual void OnIMECloseCandidates() = 0;
	virtual void OnIMERecomputeModes() = 0;
	virtual int  GetCandidateListCount() = 0;
	virtual void GetCandidate( int num, wchar_t *dest, int destSizeBytes ) = 0;
	virtual int  GetCandidateListSelectedItem() = 0;
	virtual int  GetCandidateListPageSize() = 0;
	virtual int  GetCandidateListPageStart() = 0;
	virtual void SetCandidateWindowPos( int x, int y ) = 0;
	virtual bool GetShouldInvertCompositionString() = 0;
	virtual bool CandidateListStartsAtOne() = 0;
	virtual void SetCandidateListPageStart( int start ) = 0;
	virtual void GetCompositionString(wchar_t *dest, int destSizeBytes) = 0;
	virtual void OnIMESelectCandidate(int num) = 0;
	virtual bool PostKeyMessage(KeyValues *message) = 0;
	virtual void DestroyCandidateList(void) = 0;
	virtual void CreateNewCandidateListWin32(void) = 0;
	virtual void InternalShowCandidateWindow(void) = 0;
	virtual void InternalHideCandidateWindow(void) = 0;
	virtual void InternalUpdateCandidateWindow(void) = 0;
	virtual void InternalSetCompositionString(const wchar_t *compstr) = 0;
	virtual void OnKeyCodeUnhandled(int keyCode) = 0;
	virtual bool ShouldModalSubTreeReceiveMessages() const = 0;
	virtual VPANEL GetModalSubTree(void) = 0;
	virtual bool IsIMEComposing() const = 0;
	virtual double GetImeComposingTime() const = 0;
	virtual void OnIMECompositionSDL(const char* text, int start, int length) = 0;
	virtual void OnIMECandidateSDL(const char* const* candidates, int num_candidates, int selected_candidate) = 0;
};

#define VGUI_INPUT2_INTERFACE_VERSION "VGUI_Input2_005"

} // namespace vgui

#endif // VGUI_IINPUT2_H
