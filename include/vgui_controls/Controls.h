//===== Copyright ?1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef CONTROLS_H
#define CONTROLS_H

#ifdef _WIN32
#pragma once
#endif

#include <interface.h>
#include <vstdlib/IKeyValuesSystem.h>
#include <tier0/dbg.h>
#include <tier1/strtools.h>

#include <filesystem.h>

#include <vgui/VGUI.h>
#include <vgui/IInput.h>
#include <vgui/IInputInternal.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGUI.h>
#include <vgui/IPanel.h>
#include <vgui/ILocalize.h>
#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>

#ifdef VGUI_USE_SURFACE2
#include <ISurface2.h>
#endif

#ifdef VGUI_USE_SCHEME2
#include "Scheme2.h"
#endif

extern IFileSystem *g_pFullFileSystem;

extern vgui::IInput *g_pVGuiInput;
extern vgui::CSchemeManager * g_pVGuiSchemeManager;
extern vgui::ISurface2 *g_pVGuiSurface;
extern vgui::ISystem *g_pVGuiSystem;
extern vgui::IVGui *g_pVGui;
extern vgui::IPanel *g_pVGuiPanel;
extern vgui::ILocalize *g_pVGuiLocalize;

namespace vgui
{

// handles the initialization of the vgui interfaces
// interfaces (listed below) are first attempted to be loaded from primaryProvider, then secondaryProvider
// moduleName should be the name of the module that this instance of the vgui_controls has been compiled into
bool VGui_InitInterfacesList( const char *moduleName, CreateInterfaceFn *factoryList, int numFactories );

void SetOverrideControlsModuleName(const char* pszNewName);

const char* GetOverrideControlsModuleName();

// returns the name of the module as specified above
const char *GetControlsModuleName();

class IPanel;
class IInput;
class ISchemeManager;
class ISurface;
class ISystem;
class IVGui;

//-----------------------------------------------------------------------------
// Backward compat interfaces, use the interfaces grabbed in tier3
// set of accessor functions to vgui interfaces
// the appropriate header file for each is listed above the item
//-----------------------------------------------------------------------------

// #include <vgui/IInput.h>
inline vgui::IInput *input()
{
	return g_pVGuiInput;
}

inline vgui::IInputInternal *inputinternal()
{
	return (vgui::IInputInternal *)g_pVGuiInput;
}

// #include <vgui/IScheme.h>
inline vgui::CSchemeManager *scheme()
{
	return g_pVGuiSchemeManager;
}

// #include <vgui/ISurface2.h>
inline vgui::ISurface2 *surface()
{
	return g_pVGuiSurface;
}

// #include <vgui/ISystem.h>
inline vgui::ISystem *system()
{
	return g_pVGuiSystem;
}

// #include <vgui/IVGui.h>
inline vgui::IVGui *ivgui()
{
	return g_pVGui;
}

// #include <vgui/IPanel.h>
inline vgui::IPanel *ipanel()
{
	return g_pVGuiPanel;
}

inline vgui::ILocalize *localize()
{
	return g_pVGuiLocalize;
}

// predeclare all the vgui control class names
class AnalogBar;
class AnimatingImagePanel;
class AnimationController;
class BuildModeDialog;
class Button;
class CheckButton;
class CheckButtonList;
class ComboBox;
class DirectorySelectDialog;
class Divider;
class EditablePanel;
class FileOpenDialog;
class Frame;
class GraphPanel;
class HTML;
class ImagePanel;
class Label;
class ListPanel;
class ListViewPanel;
class Menu;
class MenuBar;
class MenuButton;
class MenuItem;
class MessageBox;
class Panel;
class PanelListPanel;
class ProgressBar;
class ProgressBox;
class PropertyDialog;
class PropertyPage;
class PropertySheet;
class QueryBox;
class RadioButton;
class RichText;
class ScrollBar;
class ScrollBarSlider;
class SectionedListPanel;
class Slider;
class Splitter;
class TextEntry;
class ToggleButton;
class Tooltip;
class TreeView;
class CTreeViewListControl;
class URLLabel;
class WizardPanel;
class WizardSubPanel;

// vgui controls helper classes
class BuildGroup;
class FocusNavGroup;
class IBorder;
class IImage;
class Image;
class ImageList;
class TextImage;

} // namespace vgui

// hotkeys disabled until we work out exactly how we want to do them
#define VGUI_HOTKEYS_ENABLED
// #define VGUI_DRAW_HOTKEYS_ENABLED

#define USING_BUILD_FACTORY( className )				\
	extern className *g_##className##LinkerHack;		\
	className *g_##className##PullInModule = g_##className##LinkerHack;

#define USING_BUILD_FACTORY_ALIAS( className, factoryName )				\
	extern className *g_##factoryName##LinkerHack;		\
	className *g_##factoryName##PullInModule = g_##factoryName##LinkerHack;

#endif // CONTROLS_H
