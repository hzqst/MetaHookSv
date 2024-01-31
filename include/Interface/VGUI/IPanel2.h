//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IPANEL2_H
#define IPANEL2_H

#ifdef _WIN32
#pragma once
#endif

#include "VGUI.h"
#include <interface.h>
#include <IPanel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Source engine based IPanel interface
//-----------------------------------------------------------------------------

class IPanel2 : public IPanel
{
public:
	virtual bool IsTopmostPopup(VPANEL vguiPanel) = 0;
	virtual void SetTopmostPopup(VPANEL vguiPanel, bool state) = 0;
	virtual bool IsFullyVisible(VPANEL vguiPanel) = 0;
};

#define VGUI_PANEL2_INTERFACE_VERSION "VGUI_Panel2_007"

} // namespace vgui

#endif // IPANEL2_H
