#ifndef ICLIENTPANEL_H
#define ICLIENTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

#ifdef GetClassName
#undef GetClassName
#endif

class KeyValues;

namespace vgui
{

class Panel;
class SurfaceBase;

enum EInterfaceID
{
	ICLIENTPANEL_STANDARD_INTERFACE = 0,
};

class Panel;

class IClientPanel
{
public:
	virtual VPANEL GetVPanel(void) = 0;
	virtual void Think(void) = 0;
	virtual void PerformApplySchemeSettings(void) = 0;
	virtual void PaintTraverse(bool forceRepaint, bool allowForce) = 0;
	virtual void Repaint(void) = 0;
	virtual VPANEL IsWithinTraverse(int x, int y, bool traversePopups) = 0;
	virtual void GetInset(int &top, int &left, int &right, int &bottom) = 0;
	virtual void GetClipRect(int &x0, int &y0, int &x1, int &y1) = 0;
	virtual void OnChildAdded(VPANEL child) = 0;
	virtual void OnSizeChanged(int newWide, int newTall) = 0;
	virtual void InternalFocusChanged(bool lost) = 0;
	virtual bool RequestInfo(KeyValues *outputData) = 0;
	virtual void RequestFocus(int direction = 0) = 0;
	virtual bool RequestFocusPrev(VPANEL existingPanel) = 0;
	virtual bool RequestFocusNext(VPANEL existingPanel) = 0;
	virtual void OnMessage(const KeyValues *params, VPANEL ifromPanel) = 0;
	virtual VPANEL GetCurrentKeyFocus(void) = 0;
	virtual int GetTabPosition(void) = 0;
	virtual const char *GetName(void) = 0;
	virtual const char *GetClassName(void) = 0;
	virtual HScheme GetScheme(void) = 0;
	virtual bool IsProportional(void) = 0;
	virtual bool IsAutoDeleteSet(void) = 0;
	virtual void DeletePanel(void) = 0;
	virtual void *QueryInterface(EInterfaceID id) = 0;
	virtual Panel *GetPanel(void) = 0;
	virtual const char *GetModuleName(void) = 0;
};
}

#endif