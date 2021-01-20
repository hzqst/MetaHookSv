//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PCH_VGUI_CONTROLS_H
#define PCH_VGUI_CONTROLS_H

#ifdef _WIN32
#pragma once
#endif

// general includes
#include <ctype.h>
#include <stdlib.h>

#include <tier0/dbg.h>
#include <tier0/valve_off.h>
#include <tier1/KeyValues.h>
#include <tier0/valve_on.h>
#include <tier0/memdbgon.h>
#include <tier0/validator.h>

#include <filesystem.h>

// vgui includes
#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGUI.h>
#include <vgui/KeyCode.h>
#include <vgui/Cursor.h>
#include <vgui/MouseCode.h>

// vgui controls includes
#include "Controls.h"

#include "AnimatingImagePanel.h"
#include "AnimationController.h"
#include "BitmapImagePanel.h"
#include "BuildGroup.h"
#include "BuildModeDialog.h"
#include "Button.h"
#include "CheckButton.h"
#include "CheckButtonList.h"
#include "ComboBox.h"
#include "Controls.h"
#include "DialogManager.h"
#include "DirectorySelectDialog.h"
#include "Divider.h"
#include "EditablePanel.h"
#include "FileOpenDialog.h"
#include "FocusNavGroup.h"
#include "Frame.h"
#include "GraphPanel.h"
#include "HTML.h"
#include "Image.h"
#include "ImageList.h"
#include "ImagePanel.h"
#include "Label.h"
#include "ListPanel.h"
#include "ListViewPanel.h"
#include "Menu.h"
#include "MenuBar.h"
#include "MenuButton.h"
#include "MenuItem.h"
#include "MessageBox.h"
#include "Panel.h"
#ifndef HL1
#include "PanelAnimationVar.h"
#endif
#include "PanelListPanel.h"
#include "PHandle.h"
#include "ProgressBar.h"
#include "ProgressBox.h"
#include "PropertyDialog.h"
#include "PropertyPage.h"
#include "PropertySheet.h"
#include "QueryBox.h"
#include "RadioButton.h"
#include "RichText.h"
#include "ScrollBar.h"
#include "ScrollBarSlider.h"
#include "SectionedListPanel.h"
#include "Slider.h"
#ifndef HL1
#include "Splitter.h"
#endif
#include "TextEntry.h"
#include "TextImage.h"
#include "ToggleButton.h"
#include "Tooltip.h"
#ifndef HL1
#include "ToolWindow.h"
#endif
#include "TreeView.h"
#ifndef HL1
#include "TreeViewListControl.h"
#endif
#include "URLLabel.h"
#include "WizardPanel.h"
#include "WizardSubPanel.h"

#ifndef HL1
#include "KeyBoardEditorDialog.h"
#include "InputDialog.h"
#endif

#endif // PCH_VGUI_CONTROLS_H