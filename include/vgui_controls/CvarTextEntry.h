//========= Copyright ?1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CVARTEXTENTRY_H
#define CVARTEXTENTRY_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/TextEntry.h>

class CCvarTextEntry : public vgui::TextEntry
{
public:
	CCvarTextEntry( vgui::Panel *parent, const char *panelName, char const *cvarname );
	~CCvarTextEntry();

	void			OnTextChanged();
	void			ApplyChanges(  bool immediate = false );
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
    void            Reset();
    bool            HasBeenModified();

	DECLARE_PANELMAP();

private:
	typedef vgui::TextEntry BaseClass;

	char			*m_pszCvarName;
	char			m_pszStartValue[64];
};

#endif // CVARTEXTENTRY_H
