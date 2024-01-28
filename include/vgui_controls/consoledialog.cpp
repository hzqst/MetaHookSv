#include "consoledialog.h"

#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/IVGUI.h>
#include <vgui/ISurface.h>
#include "Surface2.h"
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>

#include <tier1/KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/RichText.h>

#include <filesystem.h>

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

#include <tier0/memdbgon.h>

using namespace vgui;

class CNonFocusableMenu : public Menu
{
	DECLARE_CLASS_SIMPLE( CNonFocusableMenu, Menu );

public:
	CNonFocusableMenu( Panel *parent, const char *panelName )
		: BaseClass( parent, panelName ),
		m_pFocus( 0 )
	{
	}

	void SetFocusPanel( Panel *panel )
	{
		m_pFocus = panel;
	}

	VPANEL GetCurrentKeyFocus()
	{
		if ( !m_pFocus )
			return GetVPanel();

		return m_pFocus->GetVPanel();
	}

private:
	Panel		*m_pFocus;
};

class TabCatchingTextEntry : public TextEntry
{
public:
	TabCatchingTextEntry(Panel *parent, const char *name, VPANEL comp) : TextEntry(parent, name), m_pCompletionList( comp )
	{
		SetAllowNonAsciiCharacters( true );
		SetDragEnabled( true );
	}

	virtual void OnKeyCodeTyped(KeyCode code)
	{
		if (code == KEY_TAB)
		{
			GetParent()->OnKeyCodeTyped(code);
		}
		else if ( code == KEY_ENTER )
		{
			PostMessage2( GetParent()->GetVPanel(), new KeyValues( "Command", "command", "submit" ) );
		}
		else
		{
			TextEntry::OnKeyCodeTyped(code);
		}
	}

	virtual void OnKillFocus()
	{
		if ( input()->GetFocus() != m_pCompletionList )
		{
			PostMessage1(GetParent(), new KeyValues("CloseCompletionList"));
		}
	}

private:
	VPANEL m_pCompletionList;
};

CHistoryItem::CHistoryItem( void )
{
	m_text = NULL;
	m_extraText = NULL;
	m_bHasExtra = false;
}

CHistoryItem::CHistoryItem( const char *text, const char *extra )
{
	Assert( text );
	m_text = NULL;
	m_extraText = NULL;
	m_bHasExtra = false;
	SetText( text , extra );
}

CHistoryItem::CHistoryItem( const CHistoryItem& src )
{
	m_text = NULL;
	m_extraText = NULL;
	m_bHasExtra = false;
	SetText( src.GetText(), src.GetExtra() );
}

CHistoryItem::~CHistoryItem( void )
{
	delete[] m_text;
	delete[] m_extraText;
	m_text = NULL;
}

const char *CHistoryItem::GetText() const
{
	if ( m_text )
	{
		return m_text;
	}
	else
	{
		return "";
	}
}

const char *CHistoryItem::GetExtra() const
{
	if ( m_extraText )
	{
		return m_extraText;
	}
	else
	{
		return NULL;
	}
}

void CHistoryItem::SetText( const char *text, const char *extra )
{
	delete[] m_text;
	int len = strlen( text ) + 1;

	m_text = new char[ len ];
	Q_memset( m_text, 0x0, len );
	Q_strncpy( m_text, text, len );

	if ( extra )
	{
		m_bHasExtra = true;
		delete[] m_extraText;
		int elen = strlen( extra ) + 1;
		m_extraText = new char[ elen ];
		Q_memset( m_extraText, 0x0, elen);
		Q_strncpy( m_extraText, extra, elen );
	}
	else
	{
		m_bHasExtra = false;
	}
}

CConsolePanel::CompletionItem::CompletionItem( void )
{
	m_bIsCommand = true;
	m_pText = NULL;
}

CConsolePanel::CompletionItem::CompletionItem( const CompletionItem& src )
{
	m_pText = NULL;
}

CConsolePanel::CompletionItem& CConsolePanel::CompletionItem::operator =( const CompletionItem& src )
{
	return *this;
}

CConsolePanel::CompletionItem::~CompletionItem( void )
{
	if ( m_pText )
	{
		delete m_pText;
		m_pText = NULL;
	}
}

const char *CConsolePanel::CompletionItem::GetName() const
{
	return "";
}

const char *CConsolePanel::CompletionItem::GetItemText( void )
{
	static char text[256];
	text[0] = 0;
	if ( m_pText )
	{
		if ( m_pText->HasExtra() )
		{
			Q_snprintf( text, sizeof( text ), "%s %s", m_pText->GetText(), m_pText->GetExtra() );
		}
		else
		{
			Q_strncpy( text, m_pText->GetText(), sizeof( text ) );
		}
	}
	return text;
}	

const char *CConsolePanel::CompletionItem::GetCommand( void ) const
{
	static char text[256];
	text[0] = 0;
	if ( m_pText )
	{
		Q_strncpy( text, m_pText->GetText(), sizeof( text ) );
	}
	return text;
}

CConsolePanel::CConsolePanel( vgui::Panel *pParent, const char *pName, bool bStatusVersion ) : 
	BaseClass( pParent, pName ), m_bStatusVersion( bStatusVersion )
{
	SetKeyBoardInputEnabled( true );

	if ( !m_bStatusVersion )
	{
		SetMinimumSize(100,100);
	}

	m_pHistory = new RichText(this, "ConsoleHistory");
	m_pHistory->SetAllowKeyBindingChainToParent( false );
	SETUP_PANEL( m_pHistory );
	m_pHistory->SetVerticalScrollbar( !m_bStatusVersion );
	if ( m_bStatusVersion )
	{
		m_pHistory->SetDrawOffsets( 3, 3 );
	}
	m_pHistory->GotoTextEnd();

	m_pSubmit = new Button(this, "ConsoleSubmit", "#GameUI_Submit");
	m_pSubmit->SetCommand("submit");
	m_pSubmit->SetVisible( !m_bStatusVersion );

	CNonFocusableMenu *pCompletionList = new CNonFocusableMenu( this, "CompletionList" );
	m_pCompletionList = pCompletionList;
	m_pCompletionList->SetVisible(false);

	m_pEntry = new TabCatchingTextEntry(this, "ConsoleEntry", m_pCompletionList->GetVPanel() );
	m_pEntry->AddActionSignalTarget(this);
	m_pEntry->SendNewLine(true);
	pCompletionList->SetFocusPanel( m_pEntry );

	m_PrintColor = Color(216, 222, 211, 255);
	m_DPrintColor = Color(196, 181, 80, 255);

	m_pEntry->SetTabPosition(1);

	m_bAutoCompleteMode = false;
	m_szPartialText[0] = 0;
	m_szPreviousPartialText[0]=0;
}

CConsolePanel::~CConsolePanel()
{
	ClearCompletionList();
	m_CommandHistory.Purge();
}

void CConsolePanel::OnThink()
{
	BaseClass::OnThink();

	if ( !IsVisible() )
		return;

	if ( !m_pCompletionList->IsVisible() )
		return;

	UpdateCompletionListPosition();
}

void CConsolePanel::Clear()
{
	m_pHistory->SetText("");
	m_pHistory->GotoTextEnd();
}

void CConsolePanel::ColorPrint( const Color& clr, const char *msg )
{
	if ( m_bStatusVersion )
	{
		Clear();
	}

	m_pHistory->InsertColorChange( clr );
	m_pHistory->InsertString( msg );
}

void CConsolePanel::Print(const char *msg)
{
	ColorPrint( m_PrintColor, msg );
}

void CConsolePanel::DPrint( const char *msg )
{
	ColorPrint( m_DPrintColor, msg );
}


void CConsolePanel::ClearCompletionList()
{
	int c = m_CompletionList.Count();
	int i;
	for ( i = c - 1; i >= 0; i-- )
	{
		delete m_CompletionList[ i ];
	}
	m_CompletionList.Purge();
}

void CConsolePanel::RebuildCompletionList(const char *text)
{
	ClearCompletionList();
}

void CConsolePanel::OnAutoComplete(bool reverse)
{
	if (!m_bAutoCompleteMode)
	{
		m_iNextCompletion = 0;
		m_bAutoCompleteMode = true;
	}

	if (reverse)
	{
		m_iNextCompletion -= 2;
		if (m_iNextCompletion < 0)
		{
			m_iNextCompletion = m_CompletionList.Size() - 1;
		}
	}

	if (!m_CompletionList.IsValidIndex(m_iNextCompletion))
	{
		m_iNextCompletion = 0;
	}

	if (!m_CompletionList.IsValidIndex(m_iNextCompletion))
		return;

	char completedText[256];
	CompletionItem *item = m_CompletionList[m_iNextCompletion];
	Assert( item );

	Q_strncpy(completedText, item->GetItemText(), sizeof(completedText) - 2 );

	if ( !Q_strstr( completedText, " " ) )
	{
		Q_strncat(completedText, " ", sizeof(completedText), COPY_ALL_CHARACTERS );
	}

	m_pEntry->SetText(completedText);
	m_pEntry->SelectNone();
	m_pEntry->GotoTextEnd();

	m_iNextCompletion++;
}

void CConsolePanel::OnTextChanged(Panel *panel)
{
	if (panel != m_pEntry)
		return;

	Q_strncpy( m_szPreviousPartialText, m_szPartialText, sizeof( m_szPreviousPartialText ) );

	m_pEntry->GetText(m_szPartialText, sizeof(m_szPartialText));

	int len = Q_strlen(m_szPartialText);

	bool hitTilde = ( m_szPartialText[len - 1] == '~' || m_szPartialText[len - 1] == '`' ) ? true : false;

	bool altKeyDown = ( vgui::input()->IsKeyDown( KEY_LALT ) || vgui::input()->IsKeyDown( KEY_RALT ) ) ? true : false;
	bool ctrlKeyDown = ( vgui::input()->IsKeyDown( KEY_LCONTROL ) || vgui::input()->IsKeyDown( KEY_RCONTROL ) ) ? true : false;

	if ( ( len > 0 ) && hitTilde )
	{
		m_szPartialText[ len - 1 ] = L'\0';

		if( !altKeyDown && !ctrlKeyDown )
		{
			m_pEntry->SetText( "" );

			PostMessage1( this, new KeyValues( "Close" ) );
			PostActionSignal( new KeyValues( "ClosedByHittingTilde" ) );
		}
		else
		{
			m_pEntry->SetText( m_szPartialText );
		}
		return;
	}

	m_bAutoCompleteMode = false;

	RebuildCompletionList(m_szPartialText);

	if ( m_CompletionList.Count() < 1 )
	{
		m_pCompletionList->SetVisible(false);
	}
	else
	{
		m_pCompletionList->SetVisible(true);
		m_pCompletionList->DeleteAllItems();
		const int MAX_MENU_ITEMS = 10;

		for (int i = 0; i < m_CompletionList.Count() && i < MAX_MENU_ITEMS; i++)
		{
			char text[256];
			text[0] = 0;
			if (i == MAX_MENU_ITEMS - 1)
			{
				Q_strncpy(text, "...", sizeof( text ) );
			}
			else
			{
				Assert( m_CompletionList[i] );
				Q_strncpy(text, m_CompletionList[i]->GetItemText(), sizeof( text ) );
			}
			KeyValues *kv = new KeyValues("CompletionCommand");
			kv->SetString("command",text);
			m_pCompletionList->AddMenuItem(text, kv, this);
		}

		UpdateCompletionListPosition();
	}
	
	RequestFocus();
	m_pEntry->RequestFocus();

}

void CConsolePanel::OnCommand(const char *command)
{
	if ( !Q_stricmp( command, "Submit" ) )
	{
		char szCommand[256];
		m_pEntry->GetText(szCommand, sizeof(szCommand));
		PostActionSignal( new KeyValues( "CommandSubmitted", "command", szCommand ) );

		Print("] ");
		Print(szCommand);
		Print("\n");

		m_pEntry->SetText("");

		OnTextChanged(m_pEntry);

		m_pHistory->GotoTextEnd();

		char *extra = strchr(szCommand, ' ');
		if ( extra )
		{
			*extra = '\0';
			extra++;
		}

		if ( Q_strlen( szCommand ) > 0 )
		{
			AddToHistory( szCommand, extra );
			// Execute the command in here
		}
		m_pCompletionList->SetVisible(false);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

bool CConsolePanel::TextEntryHasFocus() const
{
	return ( input()->GetFocus() == m_pEntry->GetVPanel() );
}

void CConsolePanel::TextEntryRequestFocus()
{
	m_pEntry->RequestFocus();
}

void CConsolePanel::OnKeyCodeTyped(KeyCode code)
{
	BaseClass::OnKeyCodeTyped(code);

	if ( TextEntryHasFocus() )
	{
		if (code == KEY_TAB)
		{
			bool reverse = false;
			if (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT))
			{
				reverse = true;
			}

			OnAutoComplete(reverse);
			m_pEntry->RequestFocus();
		}
		else if (code == KEY_DOWN)
		{
			OnAutoComplete(false);
		//	UpdateCompletionListPosition();
		//	m_pCompletionList->SetVisible(true);

			m_pEntry->RequestFocus();
		}
		else if (code == KEY_UP)
		{
			OnAutoComplete(true);
			m_pEntry->RequestFocus();
		}
	}
}

void CConsolePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	GetFocusNavGroup().SetDefaultButton(m_pSubmit);

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	m_pEntry->SetBorder(pScheme->GetBorder("DepressedButtonBorder"));
	m_pHistory->SetBorder(pScheme->GetBorder("DepressedButtonBorder"));

	int wide, tall;
	GetSize(wide, tall);

	if ( !m_bStatusVersion )
	{
		const int inset = 8;
		const int entryHeight = 24;
		const int topHeight = 4;
		const int entryInset = 4;
		const int submitWide = 64;
		const int submitInset = 7;

		m_pHistory->SetPos(inset, inset + topHeight); 
		m_pHistory->SetSize(wide - (inset * 2), tall - (entryInset * 2 + inset * 2 + topHeight + entryHeight));
		m_pHistory->InvalidateLayout();

		int nSubmitXPos = wide - ( inset + submitWide + submitInset );
		m_pSubmit->SetPos( nSubmitXPos, tall - (entryInset * 2 + entryHeight));
		m_pSubmit->SetSize( submitWide, entryHeight);
		 
		m_pEntry->SetPos( inset, tall - (entryInset * 2 + entryHeight) );
		m_pEntry->SetSize( nSubmitXPos - entryInset - 2 * inset, entryHeight);
	}
	else
	{
		const int inset = 2;

		int entryWidth = wide / 2;
		if ( wide > 400 )
		{
			entryWidth = 200;
		}

		m_pEntry->SetBounds( inset, inset, entryWidth, tall - 2 * inset );

		m_pHistory->SetBounds( inset + entryWidth + inset, inset, ( wide - entryWidth ) - inset, tall - 2 * inset );
	}

	UpdateCompletionListPosition();
}

void CConsolePanel::UpdateCompletionListPosition()
{
	int ex, ey;
	m_pEntry->GetPos(ex, ey);

	if ( !m_bStatusVersion )
	{
		ey += m_pEntry->GetTall();
	}
	else
	{
		int menuwide, menutall;
		m_pCompletionList->GetSize( menuwide, menutall );
		ey -= ( menutall + 4 );
	}

	LocalToScreen( ex, ey );
	m_pCompletionList->SetPos( ex, ey );

	if ( m_pCompletionList->IsVisible() )
	{
		m_pEntry->RequestFocus();
		MoveToFront();
		m_pCompletionList->MoveToFront();
	}
}

void CConsolePanel::CloseCompletionList()
{
	m_pCompletionList->SetVisible(false);
}

void CConsolePanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_PrintColor = GetFgColor();
	m_DPrintColor = GetSchemeColor("BrightControlText", pScheme);
//	m_pHistory->SetFont( pScheme->GetFont( "ConsoleText", IsProportional() ) );
//	m_pCompletionList->SetFont( pScheme->GetFont( "DefaultSmall", IsProportional() ) );
	InvalidateLayout();
}

void CConsolePanel::OnMenuItemSelected(const char *command)
{
	if ( strstr( command, "..." ) )
	{
		m_pCompletionList->SetVisible( true );
	}
	else
	{
		m_pEntry->SetText(command);
		m_pEntry->GotoTextEnd();
		m_pEntry->InsertChar(' ');
		m_pEntry->GotoTextEnd();
	}
}

void CConsolePanel::Hide()
{
	OnClose();
	m_iNextCompletion = 0;
	RebuildCompletionList("");
}

void CConsolePanel::AddToHistory( const char *commandText, const char *extraText )
{
	while ( m_CommandHistory.Count() >= MAX_HISTORY_ITEMS )
	{
		m_CommandHistory.Remove( 0 );
	}

	char *command = static_cast<char *>( _alloca( (strlen( commandText ) + 1 ) * sizeof( char ) ));
	if ( command )
	{
		memset( command, 0x0, strlen( commandText ) + 1 );
		strncpy( command, commandText, strlen( commandText ));
		if ( command[ strlen( command ) -1 ] == ' ' )
		{
			 command[ strlen( command ) -1 ] = '\0';
		}
	}

	char *extra = NULL;

	if ( extraText )
	{
		extra = static_cast<char *>( malloc( (strlen( extraText ) + 1 ) * sizeof( char ) ));
		if ( extra )
		{
			memset( extra, 0x0, strlen( extraText ) + 1 );
			strncpy( extra, extraText, strlen( extraText ));
			
			int i = strlen( extra ) - 1; 
			while ( i >= 0 && 
				extra[ i ] == ' ' )
			{
				extra[ i ] = '\0';
				i--;
			}
		}
	}

	CHistoryItem *item = NULL;
	for ( int i = m_CommandHistory.Count() - 1; i >= 0; i-- )
	{
		item = &m_CommandHistory[ i ];
		if ( !item )
			continue;

		if ( stricmp( item->GetText(), command ) )
			continue;

		if ( extra || item->GetExtra() )
		{
			if ( !extra || !item->GetExtra() )
				continue;

			if ( stricmp( item->GetExtra(), extra ) )	
				continue;
		}
		m_CommandHistory.Remove( i );
	}

	item = &m_CommandHistory[ m_CommandHistory.AddToTail() ];
	Assert( item );
	item->SetText( command, extra );

	m_iNextCompletion = 0;
	RebuildCompletionList( m_szPartialText );

	free( extra );
}

void CConsolePanel::DumpConsoleTextToFile()
{
	const int CONDUMP_FILES_MAX_NUM = 1000;

	FileHandle_t handle;
	bool found = false;
	char szfile[ 512 ];

	for ( int i = 0 ; i < CONDUMP_FILES_MAX_NUM ; ++i )
	{
		_snprintf( szfile, sizeof(szfile), "condump%03d.txt", i );
		if ( !g_pFullFileSystem->FileExists(szfile) )
		{
			found = true;
			break;
		}
	}

	if ( !found )
	{
		Print( "Can't condump! Too many existing condump output files in the gamedir!\n" );
		return;
	}

	handle = g_pFullFileSystem->Open( szfile, "wb" );
	if ( handle != FILESYSTEM_INVALID_HANDLE )
	{
		int pos = 0;
		while (1)
		{
			wchar_t buf[512];
			m_pHistory->GetText(pos, buf, sizeof(buf));
			pos += sizeof(buf) / sizeof(wchar_t);

			if (buf[0] == 0)
				break;

			char ansi[512];
			g_pVGuiLocalize->ConvertUnicodeToANSI(buf, ansi, sizeof(ansi));

			int len = strlen(ansi);
			for (int i = 0; i < len; i++)
			{
				if (ansi[i] == '\n')
				{
					char ret = '\r';
					g_pFullFileSystem->Write( &ret, 1, handle );
				}

				g_pFullFileSystem->Write( ansi + i, 1, handle );
			}
		}

		g_pFullFileSystem->Close( handle );

		Print( "console dumped to " );
		Print( szfile );
		Print( "\n" );
	}
	else
	{
		Print( "Unable to condump to " );
		Print( szfile );
		Print( "\n" );
	}
}

CConsoleDialog::CConsoleDialog( vgui::Panel *pParent, const char *pName, bool bStatusVersion ) : 
	BaseClass( pParent, pName )
{
	SetVisible( false );
	SetTitle( "#GameUI_Console", true );
	m_pConsolePanel = new CConsolePanel( this, "ConsolePage", bStatusVersion );
}

void CConsoleDialog::OnScreenSizeChanged( int iOldWide, int iOldTall )
{
	BaseClass::OnScreenSizeChanged( iOldWide, iOldTall );

	int sx, sy;
	surface()->GetScreenSize( sx, sy );
									 
	int w, h;
	GetSize( w, h );
	if ( w > sx || h > sy  )
	{
		if ( w > sx )
		{
			w = sx;
		}
		if ( h > sy )
		{
			h = sy;
		}

		SetSize( w, h );
	}
}

void CConsoleDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, w, h;
	GetClientArea( x, y, w, h );
	m_pConsolePanel->SetBounds( x, y, w, h );
}

void CConsoleDialog::Activate()
{
	BaseClass::Activate();
	m_pConsolePanel->m_pEntry->RequestFocus();
}

void CConsoleDialog::Hide()
{
	OnClose();
	m_pConsolePanel->Hide();
}

void CConsoleDialog::Close()
{
	Hide();
}

void CConsoleDialog::OnCommandSubmitted( const char *pCommand )
{
	PostActionSignal( new KeyValues( "CommandSubmitted", "command", pCommand ) );
}

void CConsoleDialog::Print( const char *pMessage )
{
	m_pConsolePanel->Print( pMessage );
}

void CConsoleDialog::DPrint( const char *pMessage )
{
	m_pConsolePanel->DPrint( pMessage );
}

void CConsoleDialog::ColorPrint( const Color& clr, const char *msg )
{
	m_pConsolePanel->ColorPrint( clr, msg );
}

void CConsoleDialog::Clear()
{
	m_pConsolePanel->Clear( );
}

void CConsoleDialog::DumpConsoleTextToFile()
{
	m_pConsolePanel->DumpConsoleTextToFile( );
}