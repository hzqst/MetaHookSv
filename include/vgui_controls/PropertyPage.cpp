//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <vgui/ISurface.h>

#include <tier1/KeyValues.h>

#include "PropertyPage.h"
#include "Controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PropertyPage::PropertyPage(Panel *parent, const char *panelName, bool paintBorder) : EditablePanel(parent, panelName)
{
	_paintRaised = paintBorder;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PropertyPage::~PropertyPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when page is loaded.  Data should be reloaded from document into controls.
//-----------------------------------------------------------------------------
void PropertyPage::OnResetData()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when the OK / Apply button is pressed.  Changed data should be written into document.
//-----------------------------------------------------------------------------
void PropertyPage::OnApplyChanges()
{
}

//-----------------------------------------------------------------------------
// Purpose: Designed to be overriden
//-----------------------------------------------------------------------------
void PropertyPage::OnPageShow()
{
}

//-----------------------------------------------------------------------------
// Purpose: Designed to be overriden
//-----------------------------------------------------------------------------
void PropertyPage::OnPageHide()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pageTab - 
//-----------------------------------------------------------------------------
void PropertyPage::OnPageTabActivated(Panel *pageTab)
{
	_pageTab = pageTab;
}

void PropertyPage::PaintBorder()
{
	IBorder* border = GetBorder();

	// setup border break
	if (_paintRaised == true && border && _pageTab.Get())
	{
		int px, py, pwide, ptall;
		_pageTab->GetBounds(px, py, pwide, ptall);

		int wide, tall;
		GetSize(wide, tall);
		border->Paint(0, 0, wide, tall, IBorder::SIDE_TOP, px + 1, px + pwide - 1);
	}
	else
	{
		// Paint the border
		BaseClass::PaintBorder();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertyPage::OnKeyCodeTyped(KeyCode code)
{
	switch (code)
	{
        // left and right only get propogated to parents if our tab has focus
	case KEY_RIGHT:
		{
            if (_pageTab != NULL && _pageTab->HasFocus())
                BaseClass::OnKeyCodeTyped(code);
			break;
		}
	case KEY_LEFT:
		{
            if (_pageTab != NULL && _pageTab->HasFocus())
                BaseClass::OnKeyCodeTyped(code);
			break;
		}
	default:
		BaseClass::OnKeyCodeTyped(code);
		break;
	}
}

void PropertyPage::ApplySchemeSettings(IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	if (_paintRaised)
	{
		SetBorder(pScheme->GetBorder("ButtonBorder"));
	}
}

void PropertyPage::SetVisible(bool state)
{
    if (IsVisible() && !state)
    {
        // if we're going away and we have a current button, get rid of it
        if (GetFocusNavGroup().GetCurrentDefaultButton())
        {
            GetFocusNavGroup().SetCurrentDefaultButton(NULL);
        }
    }

    BaseClass::SetVisible(state);
}

