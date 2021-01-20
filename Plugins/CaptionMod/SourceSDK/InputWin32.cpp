#include <vgui/IInput.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *hwnd - 
//-----------------------------------------------------------------------------
void IInput::SetIMEWindow( void *hwnd )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void *IInput::GetIMEWindow()
{
	return NULL;
}

// Change keyboard layout type
void IInput::OnChangeIME( bool forward )
{
}

int IInput::GetCurrentIMEHandle()
{
	return 0;
}

int IInput::GetEnglishIMEHandle()
{
	return 0;
}

// Returns the Language Bar label (Chinese, Korean, Japanese, Russion, Thai, etc.)
void IInput::GetIMELanguageName( wchar_t *buf, int unicodeBufferSizeInBytes )
{
	buf[0] = 0;
}

// Returns the short code for the language (EN, CH, KO, JP, RU, TH, etc. ).
void IInput::GetIMELanguageShortCode( wchar_t *buf, int unicodeBufferSizeInBytes )
{
	buf[0] = 0;
}

// Call with NULL dest to get item count
int IInput::GetIMELanguageList( LanguageItem *dest, int destcount )
{
	return 0;
}

int IInput::GetIMEConversionModes( ConversionModeItem *dest, int destcount )
{
	return 0;
}

int IInput::GetIMESentenceModes( SentenceModeItem *dest, int destcount )
{
	return 0;
}

void IInput::OnChangeIMEByHandle( int handleValue )
{
}

void IInput::OnChangeIMEConversionModeByHandle( int handleValue )
{
}

void IInput::OnChangeIMESentenceModeByHandle( int handleValue )
{
}

void IInput::OnInputLanguageChanged()
{
}

void IInput::OnIMEStartComposition()
{
}

void IInput::OnIMEComposition( int flags )
{
}

void IInput::OnIMEEndComposition()
{
}

void IInput::OnIMEShowCandidates()
{
}

void IInput::OnIMEChangeCandidates()
{
}

void IInput::OnIMECloseCandidates()
{
}

void IInput::OnIMERecomputeModes()
{
}

int IInput::GetCandidateListCount()
{
	return 0;
}

void IInput::GetCandidate( int num, wchar_t *dest, int destSizeBytes )
{
	dest[0] = 0;
}

int IInput::GetCandidateListSelectedItem()
{
	return 0;
}

int IInput::GetCandidateListPageSize()
{
	return 0;
}

int IInput::GetCandidateListPageStart()
{
	return 0;
}

void IInput::SetCandidateWindowPos( int x, int y )
{
}

bool IInput::GetShouldInvertCompositionString()
{
	return false;
}

bool IInput::CandidateListStartsAtOne()
{
	return false;
}

void IInput::SetCandidateListPageStart( int start )
{
}

// Passes in a keycode which allows hitting other mouse buttons w/o cancelling capture mode
void IInput::SetMouseCaptureEx(VPANEL panel, MouseCode captureStartMouseCode )
{
}

void IInput::RegisterKeyCodeUnhandledListener( VPANEL panel )
{
}

void IInput::UnregisterKeyCodeUnhandledListener( VPANEL panel )
{
}

// Posts unhandled message to all interested panels
void IInput::OnKeyCodeUnhandled( int keyCode )
{
}

// Assumes subTree is a child panel of the root panel for the vgui contect
//  if restrictMessagesToSubTree is true, then mouse and kb messages are only routed to the subTree and it's children and mouse/kb focus
//   can only be on one of the subTree children, if a mouse click occurs outside of the subtree, and "UnhandledMouseClick" message is sent to unhandledMouseClickListener panel
//   if it's set
//  if restrictMessagesToSubTree is false, then mouse and kb messages are routed as normal except that they are not routed down into the subtree
//   however, if a mouse click occurs outside of the subtree, and "UnhandleMouseClick" message is sent to unhandledMouseClickListener panel
//   if it's set
void IInput::SetModalSubTree( VPANEL subTree, VPANEL unhandledMouseClickListener, bool restrictMessagesToSubTree)
{
}

void IInput::ReleaseModalSubTree()
{
}

VPANEL IInput::GetModalSubTree()
{
	return NULL;
}

// These toggle whether the modal subtree is exclusively receiving messages or conversely whether it's being excluded from receiving messages
void IInput::SetModalSubTreeReceiveMessages( bool state )
{
}

bool IInput::ShouldModalSubTreeReceiveMessages() const
{
	return false;
}

VPANEL IInput::GetMouseCapture()
{
	return NULL;
}