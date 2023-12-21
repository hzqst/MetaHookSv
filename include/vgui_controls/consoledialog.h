#ifndef CONSOLEDIALOG_H
#define CONSOLEDIALOG_H

#ifdef _WIN32
#pragma once
#endif

#include <tier1/utlvector.h>
#include <Color.h>

#include "EditablePanel.h"
#include "Frame.h"

namespace vgui
{

class CHistoryItem
{
public:
	CHistoryItem( void );
	CHistoryItem( const char *text, const char *extra = NULL );
	CHistoryItem( const CHistoryItem& src );
	~CHistoryItem( void );

	const char *GetText() const;
	const char *GetExtra() const;
	void SetText( const char *text, const char *extra );
	bool HasExtra() { return m_bHasExtra; }

private:
	char		*m_text;
	char		*m_extraText;
	bool		m_bHasExtra;
};

class CConsolePanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CConsolePanel, vgui::EditablePanel );

public:
	CConsolePanel( Panel *pParent, const char *pName, bool bStatusVersion = false );
	~CConsolePanel();

	virtual void ColorPrint( const Color& clr, const char *pMessage );
	virtual void Print( const char *pMessage );
	virtual void DPrint( const char *pMessage );

	void Clear();

	void DumpConsoleTextToFile();

	void Hide();

	bool TextEntryHasFocus() const;
	void TextEntryRequestFocus();

private:
	enum
	{
		MAX_HISTORY_ITEMS = 100,
	};

	class CompletionItem
	{
	public:
		CompletionItem( void );
		CompletionItem( const CompletionItem& src );
		CompletionItem& CompletionItem::operator =( const CompletionItem& src );
		~CompletionItem( void );
		const char *GetItemText( void );
		const char *GetCommand( void ) const;
		const char *GetName() const;

		bool			m_bIsCommand;
		CHistoryItem	*m_pText;
	};

protected:
	void OnAutoComplete(bool reverse);
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	void RebuildCompletionList(const char *partialText);
	void UpdateCompletionListPosition();
	MESSAGE_FUNC( CloseCompletionList, "CloseCompletionList" );
	MESSAGE_FUNC_CHARPTR( OnMenuItemSelected, "CompletionCommand", command );
	void ClearCompletionList();
	void AddToHistory( const char *commandText, const char *extraText );

	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char *command);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void OnThink();

	vgui::RichText *m_pHistory;
	vgui::TextEntry *m_pEntry;
	vgui::Button *m_pSubmit;
	vgui::Menu *m_pCompletionList;
	Color m_PrintColor;
	Color m_DPrintColor;

	int m_iNextCompletion;
	char m_szPartialText[256];
	char m_szPreviousPartialText[256];
	bool m_bAutoCompleteMode;
	bool m_bWasBackspacing;
	bool m_bStatusVersion;

	CUtlVector< CompletionItem * > m_CompletionList;
	CUtlVector< CHistoryItem >	m_CommandHistory;

	friend class CConsoleDialog;
};


class CConsoleDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CConsoleDialog, vgui::Frame );

public:
	CConsoleDialog( vgui::Panel *pParent, const char *pName, bool bStatusVersion );

	virtual void OnScreenSizeChanged( int iOldWide, int iOldTall );
	virtual void Close();
	virtual void PerformLayout();

	MESSAGE_FUNC( Activate, "Activate" );
	MESSAGE_FUNC_CHARPTR( OnCommandSubmitted, "CommandSubmitted", command );

	void Hide();

	void Print( const char *msg );
	void DPrint( const char *msg );
	void ColorPrint( const Color& clr, const char *msg );
	void Clear();
	void DumpConsoleTextToFile();

protected:
	CConsolePanel *m_pConsolePanel;
};

}

#endif