//========= Copyright ?1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PANEL_LEGACY_H
#define PANEL_LEGACY_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/Dar.h>
#include <vgui/MessageMap.h>
#include <vgui/IClientPanel.h>
#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <vgui/MouseCode.h>
#include <vgui_controls/Controls.h>
#include <Color.h>

// undefine windows function macros that overlap 
#ifdef PostMessage
#undef PostMessage
#endif

#ifdef SetCursor
#undef SetCursor
#endif

namespace vgui
{

class Panel_Legacy : public IClientPanel
{
	DECLARE_CLASS_SIMPLE_NOBASE( Panel_Legacy );

public:
	//-----------------------------------------------------------------------------
	// CONSTRUCTORS
	// these functions deal with the creation of the Panel
	// the Panel automatically gets a handle to a vgui-internal panel, the ipanel(), upon construction
	// vgui interfaces deal only with ipanel(), not Panel directly
	Panel_Legacy();
	Panel_Legacy(Panel_Legacy *parent);
	Panel_Legacy(Panel_Legacy *parent, const char *panelName);
	Panel_Legacy(Panel_Legacy *parent, const char *panelName, HScheme scheme);

	virtual ~Panel_Legacy();

	// returns pointer to Panel's vgui VPanel interface handle
	virtual VPANEL GetVPanel() { return _vpanel; }

	//-----------------------------------------------------------------------------
	// PANEL METHODS
	// these functions all manipulate panels
	// they cannot be derived from
	void SetName(const char *panelName);  // sets the name of the panel - used as an identifier
	const char *GetName();		// returns the name of this panel... never NULL
	const char *GetClassName(); // returns the class name of the panel (eg. Panel, Label, Button, etc.)

	void MakeReadyForUse(); // fully construct this panel so its ready for use right now (i.e fonts loaded, colors set, default label text set, ...)

	// panel position & size
	// all units are in pixels
	void SetPos(int x,int y);		// sets position of panel, in local space (ie. relative to parent's position)
	void GetPos(int &x,int &y);		// gets local position of panel
	void SetSize(int wide,int tall);	// sets size of panel
	void GetSize(int &wide, int &tall);	// gets size of panel
	void SetBounds(int x, int y, int wide, int tall);		// combination of SetPos/SetSize
	void GetBounds(int &x, int &y, int &wide, int &tall);	// combination of GetPos/GetSize
	int  GetWide();	// returns width of panel
	void SetWide(int wide);	// sets width of panel
	int  GetTall();	// returns height of panel
	void SetTall(int tall);	// sets height of panel
	void SetMinimumSize(int wide,int tall);		// sets the minimum size the panel can go
	void GetMinimumSize(int& wide,int& tall);	// gets the minimum size
	bool IsBuildModeEditable();	  // editable in the buildModeDialog?
	void SetBuildModeEditable(bool state);  // set buildModeDialog editable
	bool IsBuildModeDeletable();  // deletable in the buildModeDialog?
	void SetBuildModeDeletable(bool state);	// set buildModeDialog deletable
	bool IsBuildModeActive();	// true if we're currently in edit mode
	void SetZPos(int z);	// sets Z ordering - lower numbers are always behind higher z's

	// panel visibility
	// invisible panels and their children do not drawn, updated, or receive input messages
	virtual void SetVisible(bool state);
	virtual bool IsVisible();

	// painting
	virtual VPANEL IsWithinTraverse(int x, int y, bool traversePopups);	// recursive; returns a pointer to the panel at those coordinates
	MESSAGE_FUNC( Repaint, "Repaint" );							// marks the panel as needing to be repainted
	virtual void PostMessage(VPANEL target, KeyValues *message, float delaySeconds = 0.0f);

	bool IsWithin(int x, int y); //in screen space
	void LocalToScreen(int &x, int &y);
	void ScreenToLocal(int &x, int &y);
	void ParentLocalToScreen(int &x, int &y);
	void MakePopup(bool showTaskbarIcon = true,bool disabled = false);		// turns the panel into a popup window (ie. can draw outside of it's parents space)
	virtual void OnMove();

	// panel hierarchy
	virtual Panel *GetParent();
	virtual VPANEL GetVParent();
	virtual void SetParent(Panel *newParent);
	virtual void SetParent(VPANEL newParent);
	virtual bool HasParent(VPANEL potentialParent);
	
	int GetChildCount();
	Panel *GetChild(int index);
	Panel *FindChildByName(const char *childName, bool recurseDown = false);
	Panel *FindSiblingByName(const char *siblingName);
	void CallParentFunction(KeyValues *message);

	virtual void SetAutoDelete(bool state);		// if set to true, panel automatically frees itself when parent is deleted
	virtual bool IsAutoDeleteSet();
	virtual void DeletePanel();				// simply does a  { delete this; }

	// messaging
	virtual void AddActionSignalTarget(Panel *messageTarget);
	virtual void AddActionSignalTarget(VPANEL messageTarget);
	virtual void RemoveActionSignalTarget(Panel *oldTarget);
	virtual void PostActionSignal(KeyValues *message);			// sends a message to the current actionSignalTarget(s)
	virtual bool RequestInfoFromChild(const char *childName, KeyValues *outputData);
	virtual void PostMessageToChild(const char *childName, KeyValues *messsage);
	virtual void PostMessage(Panel *target, KeyValues *message, float delaySeconds = 0.0f);
	virtual bool RequestInfo(KeyValues *outputData);				// returns true if output is successfully written.  You should always chain back to the base class if info request is not handled
	virtual bool SetInfo(KeyValues *inputData);						// sets a specified value in the control - inverse of the above

	// drawing state
	virtual void   SetEnabled(bool state);
	virtual bool   IsEnabled();
	virtual bool   IsPopup();	// has a parent, but is in it's own space
	virtual void   GetClipRect(int &x0, int &y0, int &x1, int &y1);
	virtual void   MoveToFront();

	// pin positions for auto-layout
	enum PinCorner_e 
	{
		PIN_TOPLEFT = 0,
		PIN_TOPRIGHT,
		PIN_BOTTOMLEFT,
		PIN_BOTTOMRIGHT,
	};
	// specifies the corner the panel is to be pinned to if a dialog is resized
	void SetPinCorner(PinCorner_e pinCorner);
	PinCorner_e GetPinCorner();

	// specifies the auto-resize directions for the panel
	enum AutoResize_e
	{
		AUTORESIZE_NO = 0,
		AUTORESIZE_RIGHT,
		AUTORESIZE_DOWN,
		AUTORESIZE_DOWNANDRIGHT,
	};
	void SetAutoResize(AutoResize_e resizeDir);
	AutoResize_e GetAutoResize();

	// colors
	virtual void SetBgColor(Color color);
	virtual void SetFgColor(Color color);
	virtual Color GetBgColor();
	virtual Color GetFgColor();

	virtual void SetCursor(HCursor cursor);
	virtual HCursor GetCursor();
	virtual void RequestFocus(int direction = 0);
	virtual bool HasFocus();
	virtual void InvalidateLayout(bool layoutNow = false, bool reloadScheme = false);
	virtual bool RequestFocusPrev(VPANEL panel = 0);
	virtual bool RequestFocusNext(VPANEL panel = 0);
	// tab positioning
	virtual void   SetTabPosition(int position);
	virtual int    GetTabPosition();
	// border
	virtual void SetBorder(IBorder *border);
	virtual IBorder *GetBorder();
	virtual void SetPaintBorderEnabled(bool state);
	virtual void SetPaintBackgroundEnabled(bool state);
	virtual void SetPaintEnabled(bool state);
	virtual void SetPostChildPaintEnabled(bool state);
	virtual void GetInset(int &left, int &top, int &right, int &bottom);
	virtual void GetPaintSize(int &wide, int &tall);
	virtual void SetBuildGroup(BuildGroup *buildGroup);
	virtual bool IsBuildGroupEnabled();
	virtual bool IsCursorNone();
	virtual bool IsCursorOver();		// returns true if the cursor is currently over the panel
	virtual void MarkForDeletion();		// object will free it's memory next tick
	virtual bool IsLayoutInvalid();		// does this object require a perform layout?
	virtual Panel *HasHotkey(wchar_t key);			// returns the panel that has this hotkey
	virtual bool IsOpaque();

	// scheme access functions
	virtual HScheme GetScheme();
	virtual void SetScheme(const char *tag);
	virtual void SetScheme(HScheme scheme);
	virtual Color GetSchemeColor(const char *keyName,IScheme *pScheme);
	virtual Color GetSchemeColor(const char *keyName, Color defaultColor,IScheme *pScheme);

	// called when scheme settings need to be applied; called the first time before the panel is painted
	virtual void ApplySchemeSettings(IScheme *pScheme);

	// interface to build settings
	// takes a group of settings and applies them to the control
	virtual void ApplySettings(KeyValues *inResourceData);

	// records the settings into the resource data
	virtual void GetSettings(KeyValues *outResourceData);

	// gets a description of the resource for use in the UI
	// format: <type><whitespace | punctuation><keyname><whitespace| punctuation><type><whitespace | punctuation><keyname>...
	// unknown types as just displayed as strings in the UI (for future UI expansion)
	virtual const char *GetDescription();

	// returns the name of the module that this instance of panel was compiled into
	virtual const char *GetModuleName();

	// user configuration settings
	// this is used for any control details the user wants saved between sessions
	// eg. dialog positions, last directory opened, list column width
	virtual void ApplyUserConfigSettings(KeyValues *userConfig);

	// returns user config settings for this control
	virtual void GetUserConfigSettings(KeyValues *userConfig);

	// optimization, return true if this control has any user config settings
	virtual bool HasUserConfigSettings();

	// message handlers
	// override to get access to the message
	virtual void OnMessage(const KeyValues *params, VPANEL fromPanel);	// called when panel receives message; must chain back
	virtual void OnThink();							// called every frame before painting, but only if panel is visible
	MESSAGE_FUNC_CHARPTR( OnCommand, "Command", command );	// called when a panel receives a command
	MESSAGE_FUNC( OnMouseCaptureLost, "MouseCaptureLost" );	// called after the panel loses mouse capture
	MESSAGE_FUNC( OnSetFocus, "SetFocus" );			// called after the panel receives the keyboard focus
	MESSAGE_FUNC( OnKillFocus, "KillFocus" );		// called after the panel loses the keyboard focus
	MESSAGE_FUNC( OnDelete, "Delete" );				// called to delete the panel; Panel::OnDelete() does simply { delete this; }
	virtual void OnChildAdded(VPANEL child);		// called when a child has been added to this panel
	virtual void OnSizeChanged(int newWide, int newTall);	// called after the size of a panel has been changed
	
	// called every frame if ivgui()->AddTickSignal() is called
	MESSAGE_FUNC( OnTick, "Tick" );

	// input messages
	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnMouseWheeled(int delta);

	// base implementation forwards Key messages to the Panel's parent 
	// - override to 'swallow' the input
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void OnKeyCodeReleased(KeyCode code);
	virtual void OnKeyFocusTicked(); // every window gets key ticked events

	// forwards mouse messages to the panel's parent
	MESSAGE_FUNC( OnMouseFocusTicked, "OnMouseFocusTicked" );

	// message handlers that don't go through the message pump
	virtual void PaintBackground();
	virtual void Paint();
	virtual void PaintBorder();
	virtual void PaintBuildOverlay();		// the extra drawing for when in build mode
	virtual void PostChildPaint();
	virtual void PerformLayout();

	// this enables message mapping for this class - requires matching IMPLEMENT_PANELDESC() in the .cpp file
	DECLARE_PANELMAP();

	virtual VPANEL GetCurrentKeyFocus();

	// returns a pointer to the tooltip object associated with the panel
	// creates a new one if none yet exists
	Tooltip *GetTooltip();

	virtual bool IsProportional() { return m_bProportional; }
	virtual void SetProportional(bool state);

	// input interest
	virtual void SetMouseInputEnabled( bool state );
	virtual void SetKeyBoardInputEnabled( bool state );
	virtual bool IsMouseInputEnabled();
	virtual bool IsKeyBoardInputEnabled();

protected:
	MESSAGE_FUNC_ENUM_ENUM( OnRequestFocus, "OnRequestFocus", VPANEL, subFocus, VPANEL, defaultPanel);
	virtual void *QueryInterface(EInterfaceID id);

private:
	// used to get the Panel * for users with only IClientPanel
	virtual Panel *GetPanel() { return this; }

	// private methods
	void Think();
	void PerformApplySchemeSettings();

	void InternalPerformLayout();
	void InternalSetCursor();

	MESSAGE_FUNC_INT_INT( InternalCursorMoved, "CursorMoved", xpos, ypos );
	MESSAGE_FUNC( InternalCursorEntered, "CursorEntered" );
	MESSAGE_FUNC( InternalCursorExited, "CursorExited" );
	
	MESSAGE_FUNC_INT( InternalMousePressed, "MousePressed", code );
	MESSAGE_FUNC_INT( InternalMouseDoublePressed, "MouseDoublePressed", code );
	// Triple presses are synthesized
	MESSAGE_FUNC_INT( InternalMouseReleased, "MouseReleased", code );
	MESSAGE_FUNC_INT( InternalMouseWheeled, "MouseWheeled", delta );
	MESSAGE_FUNC_INT( InternalKeyCodePressed, "KeyCodePressed", code );
	MESSAGE_FUNC_INT( InternalKeyCodeTyped, "KeyCodeTyped", code );
	MESSAGE_FUNC_INT( InternalKeyTyped, "KeyTyped", unichar );
	MESSAGE_FUNC_INT( InternalKeyCodeReleased, "KeyCodeReleased", code );

	MESSAGE_FUNC( InternalKeyFocusTicked, "KeyFocusTicked" );
	MESSAGE_FUNC( InternalMouseFocusTicked, "MouseFocusTicked" );

	MESSAGE_FUNC( InternalInvalidateLayout, "Invalidate" );

	MESSAGE_FUNC( InternalMove, "Move" );
	virtual void InternalFocusChanged(bool lost);	// called when the focus gets changed
	virtual void PaintTraverse(bool Repaint, bool allowForce = true);

	void Init(int x,int y,int wide,int tall);
	void PreparePanelMap(PanelMap_t *panelMap);

	// data
	VPANEL _vpanel;	// handle to a vgui panel
	HCursor _cursor;
	bool _markedForDeletion;
	IBorder	*_border;
	bool _needsRepaint;
	BuildGroup *_buildGroup;
	Color _fgColor;		// foreground color
	Color _bgColor;		// background color
	bool _paintBorderEnabled;
	bool _paintBackgroundEnabled;
	bool _paintEnabled;
	bool _postChildPaintEnabled;
	char *_panelName;		// string name of the panel - only unique within the current context
	bool _needsLayout;
	bool _needsSchemeUpdate;
	bool _autoDelete;
	int	_tabPosition;		// the panel's place in the tab ordering
	Dar<HPanel> _actionSignalTargetDar;	// the panel to direct notify messages to ("Command", "TextChanged", etc.)

	PinCorner_e _pinCorner;	// the corner of the dialog this panel is pinned to
	AutoResize_e _autoResizeDirection; // the directions in which the panel will auto-resize to

	unsigned int m_iScheme; // handle to the scheme to use

	enum
	{
		BUILDMODE_EDITABLE					= 0x01,
		BUILDMODE_DELETABLE					= 0x02,
		BUILDMODE_SAVE_XPOS_RIGHTALIGNED	= 0x04,
		BUILDMODE_SAVE_XPOS_CENTERALIGNED	= 0x08,
		BUILDMODE_SAVE_YPOS_BOTTOMALIGNED	= 0x10,
		BUILDMODE_SAVE_YPOS_CENTERALIGNED	= 0x20,
	};
	unsigned char _buildModeFlags; // flags that control how the build mode dialog handles this panel
	bool m_bProportional;
	bool m_bInPerformLayout;
	Tooltip *m_pTooltips;

	// friend class Panel;
	friend class BuildGroup;
	friend class BuildModeDialog;
	friend class PHandle;

	// obselete, remove soon
	void OnOldMessage(KeyValues *params, VPANEL ifromPanel);
};

} // namespace vgui


#endif // PANEL_H
