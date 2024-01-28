#include <metahook.h>
#include "CvarTextEntry.h"
#include <vgui/IVGui.h>
#include "IGameUIFuncs.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

static const int MAX_CVAR_TEXT = 64;

CCvarTextEntry::CCvarTextEntry( Panel *parent, const char *panelName, char const *cvarname )
 : TextEntry( parent, panelName)
{
	auto CvarNameLength = strlen(cvarname);
	m_pszCvarName = (cvarname) ? (char *)malloc(CvarNameLength + 1) : NULL;
	if (m_pszCvarName)
	{
		memcpy(m_pszCvarName, cvarname, CvarNameLength);
		m_pszCvarName[CvarNameLength] = 0;
	}

	m_pszStartValue[0] = 0;

	if ( m_pszCvarName )
	{
		Reset();
	}

	AddActionSignalTarget( this );
}

CCvarTextEntry::~CCvarTextEntry()
{
	if ( m_pszCvarName )
	{
		free( m_pszCvarName );
	}
}

void CCvarTextEntry::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	if (GetMaximumCharCount() < 0 || GetMaximumCharCount() > MAX_CVAR_TEXT)
	{
		SetMaximumCharCount(MAX_CVAR_TEXT - 1);
	}
}

void CCvarTextEntry::ApplyChanges( bool immediate )
{
	if ( !m_pszCvarName )
		return;

	char szText[ MAX_CVAR_TEXT ];
	GetText( szText, MAX_CVAR_TEXT );

	if ( !szText[ 0 ] )
		return;

	if ( immediate )
	{
		gEngfuncs.Cvar_Set( m_pszCvarName, szText );
	}
	else
	{
		char szCommand[ 256 ];
		sprintf( szCommand, "%s \"%s\"\n", m_pszCvarName, szText );
		gEngfuncs.pfnClientCmd( szCommand );
	}

	strcpy( m_pszStartValue, szText );
}

void CCvarTextEntry::Reset()
{
	char *value = gEngfuncs.pfnGetCvarString( m_pszCvarName );
	if ( value && value[ 0 ] )
	{
		SetText( value );
		strncpy( m_pszStartValue, value, sizeof(m_pszStartValue) - 1 );
		m_pszStartValue[sizeof(m_pszStartValue) - 1] = 0;
	}
}

bool CCvarTextEntry::HasBeenModified()
{
	char szText[ MAX_CVAR_TEXT ];
	GetText( szText, MAX_CVAR_TEXT );

	return stricmp( szText, m_pszStartValue );
}


void CCvarTextEntry::OnTextChanged()
{
	if ( !m_pszCvarName )
		return;
	
	if (HasBeenModified())
	{
		PostActionSignal(new KeyValues("ControlModified"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
vgui::MessageMapItem_t CCvarTextEntry::m_MessageMap[] =
{
	MAP_MESSAGE( CCvarTextEntry, "TextChanged", OnTextChanged ),	// custom message
};

IMPLEMENT_PANELMAP( CCvarTextEntry, BaseClass );