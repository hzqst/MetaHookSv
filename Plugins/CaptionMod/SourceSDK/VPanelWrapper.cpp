#include <metahook.h>
#include <assert.h>

#include "VPanel.h"
#include "vgui_internal.h"

#include <vgui_controls/Controls.h>

#include <vgui/IClientPanel.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>

using namespace vgui;

class VPanelWrapper : public vgui::IPanel
{
public:
	virtual void Init(VPANEL vguiPanel, IClientPanel *panel)
	{
		((VPanel *)vguiPanel)->Init(panel);
	}

	virtual void SetPos(VPANEL vguiPanel, int x, int y)
	{
		((VPanel *)vguiPanel)->SetPos(x, y);
	}

	virtual void GetPos(VPANEL vguiPanel, int &x, int &y)
	{
		((VPanel *)vguiPanel)->GetPos(x, y);
	}

	virtual void SetSize(VPANEL vguiPanel, int wide, int tall)
	{
		((VPanel *)vguiPanel)->SetSize(wide, tall);
	}

	virtual void GetSize(VPANEL vguiPanel, int &wide, int &tall)
	{
		((VPanel *)vguiPanel)->GetSize(wide, tall);
	}

	virtual void SetMinimumSize(VPANEL vguiPanel, int wide, int tall)
	{
		((VPanel *)vguiPanel)->SetMinimumSize(wide, tall);
	}

	virtual void GetMinimumSize(VPANEL vguiPanel, int &wide, int &tall)
	{
		((VPanel *)vguiPanel)->GetMinimumSize(wide, tall);
	}

	virtual void SetZPos(VPANEL vguiPanel, int z)
	{
		((VPanel *)vguiPanel)->SetZPos(z);
	}

	virtual int GetZPos(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetZPos();
	}

	virtual void GetAbsPos(VPANEL vguiPanel, int &x, int &y)
	{
		((VPanel *)vguiPanel)->GetAbsPos(x, y);
	}

	virtual void GetClipRect(VPANEL vguiPanel, int &x0, int &y0, int &x1, int &y1)
	{
		((VPanel *)vguiPanel)->GetClipRect(x0, y0, x1, y1);
	}

	virtual void SetInset(VPANEL vguiPanel, int left, int top, int right, int bottom)
	{
		((VPanel *)vguiPanel)->SetInset(left, top, right, bottom);
	}

	virtual void GetInset(VPANEL vguiPanel, int &left, int &top, int &right, int &bottom)
	{
		((VPanel *)vguiPanel)->GetInset(left, top, right, bottom);
	}

	virtual void SetVisible(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->SetVisible(state);
	}

	virtual bool IsVisible(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsVisible();
	}

	virtual void SetParent(VPANEL vguiPanel, VPANEL newParent)
	{
		((VPanel *)vguiPanel)->SetParent((VPanel *)newParent);
	}

	virtual int GetChildCount(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetChildCount();
	}

	virtual VPANEL GetChild(VPANEL vguiPanel, int index)
	{
		return (VPANEL)((VPanel *)vguiPanel)->GetChild(index);
	}

	virtual VPANEL GetParent(VPANEL vguiPanel)
	{
		return (VPANEL)((VPanel *)vguiPanel)->GetParent();
	}

	virtual void MoveToFront(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->MoveToFront();
	}

	virtual void MoveToBack(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->MoveToBack();
	}

	virtual bool HasParent(VPANEL vguiPanel, VPANEL potentialParent)
	{
		if (!vguiPanel)
			return false;

		return ((VPanel *)vguiPanel)->HasParent((VPanel *)potentialParent);
	}

	virtual bool IsPopup(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsPopup();
	}

	virtual void SetPopup(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->SetPopup(state);
	}

	virtual bool Render_GetPopupVisible(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Render_IsPopupPanelVisible();
	}

	virtual void Render_SetPopupVisible(VPANEL vguiPanel, bool state)
	{
		return ((VPanel *)vguiPanel)->Render_SetPopupVisible(state);
	}

	virtual HScheme GetScheme(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetScheme();
	}

	virtual bool IsProportional(VPANEL vguiPanel)
	{
		return Client(vguiPanel)->IsProportional();
	}

	virtual bool IsAutoDeleteSet(VPANEL vguiPanel)
	{
		return Client(vguiPanel)->IsAutoDeleteSet();
	}

	virtual void DeletePanel(VPANEL vguiPanel)
	{
		Client(vguiPanel)->DeletePanel();
	}

	virtual void SetKeyBoardInputEnabled(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->SetKeyBoardInputEnabled(state);
	}

	virtual void SetMouseInputEnabled(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->SetMouseInputEnabled(state);
	}

	virtual bool IsMouseInputEnabled(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsMouseInputEnabled();
	}

	virtual bool IsKeyBoardInputEnabled(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsKeyBoardInputEnabled();
	}

	virtual void Solve(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->Solve();
	}

	virtual const char *GetName(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetName();
	}

	virtual const char *GetClassName(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetClassName();
	}

	virtual void SendMessage(VPANEL vguiPanel, KeyValues *params, VPANEL ifrompanel)
	{
		((VPanel *)vguiPanel)->SendMessage(params, ifrompanel);
	}

	virtual void Think(VPANEL vguiPanel)
	{
		Client(vguiPanel)->Think();
	}

	virtual void PerformApplySchemeSettings(VPANEL vguiPanel)
	{
		Client(vguiPanel)->PerformApplySchemeSettings();
	}

	virtual void PaintTraverse(VPANEL vguiPanel, bool forceRepaint, bool allowForce)
	{
		Client(vguiPanel)->PaintTraverse(forceRepaint, allowForce);
	}

	virtual void Repaint(VPANEL vguiPanel)
	{
		Client(vguiPanel)->Repaint();
	}

	virtual VPANEL IsWithinTraverse(VPANEL vguiPanel, int x, int y, bool traversePopups)
	{
		return Client(vguiPanel)->IsWithinTraverse(x, y, traversePopups);
	}

	virtual void OnChildAdded(VPANEL vguiPanel, VPANEL child)
	{
		Client(vguiPanel)->OnChildAdded(child);
	}

	virtual void OnSizeChanged(VPANEL vguiPanel, int newWide, int newTall)
	{
		Client(vguiPanel)->OnSizeChanged(newWide, newTall);
	}

	virtual void InternalFocusChanged(VPANEL vguiPanel, bool lost)
	{
		Client(vguiPanel)->InternalFocusChanged(lost);
	}

	virtual bool RequestInfo(VPANEL vguiPanel, KeyValues *outputData)
	{
		return Client(vguiPanel)->RequestInfo(outputData);
	}

	virtual void RequestFocus(VPANEL vguiPanel, int direction = 0)
	{
		Client(vguiPanel)->RequestFocus(direction);
	}

	virtual bool RequestFocusPrev(VPANEL vguiPanel, VPANEL existingPanel)
	{
		return Client(vguiPanel)->RequestFocusPrev(existingPanel);
	}

	virtual bool RequestFocusNext(VPANEL vguiPanel, VPANEL existingPanel)
	{
		return Client(vguiPanel)->RequestFocusNext(existingPanel);
	}

	virtual VPANEL GetCurrentKeyFocus(VPANEL vguiPanel)
	{
		return Client(vguiPanel)->GetCurrentKeyFocus();
	}

	virtual int GetTabPosition(VPANEL vguiPanel)
	{
		return Client(vguiPanel)->GetTabPosition();
	}

	virtual SurfacePlat *Plat(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Plat();
	}

	virtual void SetPlat(VPANEL vguiPanel, SurfacePlat *Plat)
	{
		((VPanel *)vguiPanel)->SetPlat(Plat);
	}

	virtual Panel *GetPanel(VPANEL vguiPanel, const char *moduleName)
	{
		if (vguiPanel == surface()->GetEmbeddedPanel())
			return NULL;

		if (stricmp(GetModuleName(vguiPanel), moduleName))
			return NULL;

		return Client(vguiPanel)->GetPanel();
	}

	virtual bool IsEnabled(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsEnabled();
	}

	virtual void SetEnabled(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->SetEnabled(state);
	}

	virtual IClientPanel *Client(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Client();
	}

	virtual const char *GetModuleName(VPANEL vguiPanel)
	{
		return Client(vguiPanel)->GetModuleName();
	}

public:
	bool IsTopmostPopup(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsTopmostPopup();
	}

	void SetTopmostPopup(VPANEL vguiPanel, bool state)
	{
		return ((VPanel *)vguiPanel)->SetTopmostPopup(state);
	}

	bool IsFullyVisible(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsFullyVisible();
	}
};

VPanelWrapper g_Panel;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(VPanelWrapper, IPanel, VGUI_PANEL_INTERFACE_VERSION, g_Panel);

bool IPanel::IsTopmostPopup(VPANEL vguiPanel)
{
	return g_Panel.IsTopmostPopup(vguiPanel);
}

void IPanel::SetTopmostPopup(VPANEL vguiPanel, bool state)
{
	return g_Panel.SetTopmostPopup(vguiPanel, state);
}

bool IPanel::IsFullyVisible(VPANEL vguiPanel)
{
	return g_Panel.IsFullyVisible(vguiPanel);
}