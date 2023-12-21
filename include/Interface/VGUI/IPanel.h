#ifndef IPANEL_H
#define IPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "interface.h"

#ifdef SendMessage
#undef SendMessage
#endif

#ifdef GetClassName
#undef GetClassName
#endif

class KeyValues;

namespace vgui
{

class SurfacePlat;
class IClientPanel;

class Panel;

class IPanel : public IBaseInterface
{
public:
	virtual void Init(VPANEL vguiPanel, IClientPanel *panel) = 0;
	virtual void SetPos(VPANEL vguiPanel, int x, int y) = 0;
	virtual void GetPos(VPANEL vguiPanel, int &x, int &y) = 0;
	virtual void SetSize(VPANEL vguiPanel, int wide, int tall) = 0;
	virtual void GetSize(VPANEL vguiPanel, int &wide, int &tall) = 0;
	virtual void SetMinimumSize(VPANEL vguiPanel, int wide, int tall) = 0;
	virtual void GetMinimumSize(VPANEL vguiPanel, int &wide, int &tall) = 0;
	virtual void SetZPos(VPANEL vguiPanel, int z) = 0;
	virtual int GetZPos(VPANEL vguiPanel) = 0;
	virtual void GetAbsPos(VPANEL vguiPanel, int &x, int &y) = 0;
	virtual void GetClipRect(VPANEL vguiPanel, int &x0, int &y0, int &x1, int &y1) = 0;
	virtual void SetInset(VPANEL vguiPanel, int left, int top, int right, int bottom) = 0;
	virtual void GetInset(VPANEL vguiPanel, int &left, int &top, int &right, int &bottom) = 0;
	virtual void SetVisible(VPANEL vguiPanel, bool state) = 0;
	virtual bool IsVisible(VPANEL vguiPanel) = 0;
	virtual void SetParent(VPANEL vguiPanel, VPANEL newParent) = 0;
	virtual int GetChildCount(VPANEL vguiPanel) = 0;
	virtual VPANEL GetChild(VPANEL vguiPanel, int index) = 0;
	virtual VPANEL GetParent(VPANEL vguiPanel) = 0;
	virtual void MoveToFront(VPANEL vguiPanel) = 0;
	virtual void MoveToBack(VPANEL vguiPanel) = 0;
	virtual bool HasParent(VPANEL vguiPanel, VPANEL potentialParent) = 0;
	virtual bool IsPopup(VPANEL vguiPanel) = 0;
	virtual void SetPopup(VPANEL vguiPanel, bool state) = 0;
	virtual bool Render_GetPopupVisible(VPANEL vguiPanel) = 0;
	virtual void Render_SetPopupVisible(VPANEL vguiPanel, bool state) = 0;
	virtual HScheme GetScheme(VPANEL vguiPanel) = 0;
	virtual bool IsProportional(VPANEL vguiPanel) = 0;
	virtual bool IsAutoDeleteSet(VPANEL vguiPanel) = 0;
	virtual void DeletePanel(VPANEL vguiPanel) = 0;
	virtual void SetKeyBoardInputEnabled(VPANEL vguiPanel, bool state) = 0;
	virtual void SetMouseInputEnabled(VPANEL vguiPanel, bool state) = 0;
	virtual bool IsKeyBoardInputEnabled(VPANEL vguiPanel) = 0;
	virtual bool IsMouseInputEnabled(VPANEL vguiPanel) = 0;
	virtual void Solve(VPANEL vguiPanel) = 0;
	virtual const char *GetName(VPANEL vguiPanel) = 0;
	virtual const char *GetClassName(VPANEL vguiPanel) = 0;
	virtual void SendMessage(VPANEL vguiPanel, KeyValues *params, VPANEL ifromPanel) = 0;
	virtual void Think(VPANEL vguiPanel) = 0;
	virtual void PerformApplySchemeSettings(VPANEL vguiPanel) = 0;
	virtual void PaintTraverse(VPANEL vguiPanel, bool forceRepaint, bool allowForce = true) = 0;
	virtual void Repaint(VPANEL vguiPanel) = 0;
	virtual VPANEL IsWithinTraverse(VPANEL vguiPanel, int x, int y, bool traversePopups) = 0;
	virtual void OnChildAdded(VPANEL vguiPanel, VPANEL child) = 0;
	virtual void OnSizeChanged(VPANEL vguiPanel, int newWide, int newTall) = 0;
	virtual void InternalFocusChanged(VPANEL vguiPanel, bool lost) = 0;
	virtual bool RequestInfo(VPANEL vguiPanel, KeyValues *outputData) = 0;
	virtual void RequestFocus(VPANEL vguiPanel, int direction = 0) = 0;
	virtual bool RequestFocusPrev(VPANEL vguiPanel, VPANEL existingPanel) = 0;
	virtual bool RequestFocusNext(VPANEL vguiPanel, VPANEL existingPanel) = 0;
	virtual VPANEL GetCurrentKeyFocus(VPANEL vguiPanel) = 0;
	virtual int GetTabPosition(VPANEL vguiPanel) = 0;
	virtual SurfacePlat *Plat(VPANEL vguiPanel) = 0;
	virtual void SetPlat(VPANEL vguiPanel, SurfacePlat *Plat) = 0;
	virtual Panel *GetPanel(VPANEL vguiPanel, const char *destinationModule) = 0;
	virtual bool IsEnabled(VPANEL vguiPanel) = 0;
	virtual void SetEnabled(VPANEL vguiPanel, bool state) = 0;
	virtual IClientPanel *Client(VPANEL vguiPanel) = 0;
	virtual const char *GetModuleName(VPANEL vguiPanel) = 0;
};
}

#define VGUI_PANEL_INTERFACE_VERSION "VGUI_Panel007"

#endif