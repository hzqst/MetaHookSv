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
#include <vgui/IBorder.h>
#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>

#ifdef VGUI_USE_SURFACE2
#include <vgui/ISurface2.h>
#endif

#ifdef VGUI_USE_SCHEME2
#include <vgui/IScheme2.h>
#endif

#ifdef VGUI_USE_INPUT2
#include <vgui/IInput2.h>
#endif

extern IFileSystem *g_pFullFileSystem;

#ifdef VGUI_USE_INPUT2
extern vgui::IInput2 *g_pVGuiInput2;
#else
extern vgui::IInput* g_pVGuiInput;
#endif

#ifdef VGUI_USE_SCHEME2
extern vgui::ISchemeManager2 * g_pVGuiSchemeManager2;
#else
extern vgui::ISchemeManager* g_pVGuiSchemeManager;
#endif

#ifdef VGUI_USE_SURFACE2
extern vgui::ISurface2 *g_pVGuiSurface2;
#else
extern vgui::ISurface* g_pVGuiSurface;
#endif

extern vgui::ISystem *g_pVGuiSystem;

extern vgui::IVGui *g_pVGui;
#ifdef VGUI_USE_PANEL2
extern vgui::IPanel2* g_pVGuiPanel2;
#else
extern vgui::IPanel *g_pVGuiPanel;
#endif

extern vgui::ILocalize *g_pVGuiLocalize;

namespace vgui
{

// handles the initialization of the vgui interfaces
// interfaces (listed below) are first attempted to be loaded from primaryProvider, then secondaryProvider
// moduleName should be the name of the module that this instance of the vgui_controls has been compiled into
bool VGui_InitInterfacesList( const char *moduleName, CreateInterfaceFn *factoryList, int numFactories );

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
#ifdef VGUI_USE_INPUT2
inline vgui::IInput2 *input()
{
	return g_pVGuiInput2;
}
inline vgui::IInputInternal* inputinternal()
{
	return (vgui::IInputInternal*)g_pVGuiInput2;
}
#else
inline vgui::IInput* input()
{
	return g_pVGuiInput;
}
inline vgui::IInputInternal* inputinternal()
{
	return (vgui::IInputInternal*)g_pVGuiInput;
}
#endif

// #include <vgui/IScheme.h>
#ifdef VGUI_USE_SCHEME2
inline vgui::ISchemeManager2 *scheme()
{
	return g_pVGuiSchemeManager2;
}
#else
inline vgui::ISchemeManager* scheme()
{
	return g_pVGuiSchemeManager;
}
#endif

// #include <vgui/ISurface2.h>
#ifdef VGUI_USE_SURFACE2
inline vgui::ISurface2 *surface()
{
	return g_pVGuiSurface2;
}
#else
inline vgui::ISurface* surface()
{
	return g_pVGuiSurface;
}
#endif

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
#ifdef VGUI_USE_PANEL2
inline vgui::IPanel2* ipanel2()
{
	return g_pVGuiPanel2;
}
#else
inline vgui::IPanel* ipanel()
{
	return g_pVGuiPanel;
}
#endif

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
