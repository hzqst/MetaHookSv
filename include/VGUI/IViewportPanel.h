//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#ifndef VGUI_IVIEWPORTPANEL_H
#define VGUI_IVIEWPORTPANEL_H

#include <vgui/VGUI.h>

class KeyValues;

/**
*	A panel that is part of the client's viewport (HUD).
*/
class IViewportPanel
{
public:
	virtual ~IViewportPanel() { }

	/**
	*	@return identifer name
	*/
	virtual const char *GetName() = 0;

	/**
	*	Clears internal state, deactivates it
	*	Called during VidInit
	*/
	virtual void Reset() = 0;

	/**
	*	Activate VGUI Frame
	*/
	virtual void ShowPanel(bool state) = 0;

	// VGUI functions:

	/**
	*	@return VGUI panel handle
	*/
	virtual vgui::VPANEL GetVPanel() = 0;

	/**
	*	@return true if panel is visible
	*/
	virtual bool IsVisible() = 0;

	virtual void SetParent(vgui::VPANEL parent) = 0;
};

#endif
