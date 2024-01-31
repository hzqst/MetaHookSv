#include <metahook.h>
#include <assert.h>

#include "VPanel.h"
#include "vgui_internal.h"

#include <vgui_controls/Controls.h>

#include <vgui/IClientPanel.h>
#include <vgui/IPanel.h>
#include <vgui/IPanel2.h>
#include <vgui/ISurface.h>

using namespace vgui;

class VPanelWrapper : public vgui::IPanel2
{
public:
	void Init(VPANEL vguiPanel, IClientPanel* panel) override
	{
		((VPanel*)vguiPanel)->Init(panel);
	}

	void SetPos(VPANEL vguiPanel, int x, int y) override
	{
		((VPanel*)vguiPanel)->SetPos(x, y);
	}

	void GetPos(VPANEL vguiPanel, int& x, int& y) override
	{
		((VPanel*)vguiPanel)->GetPos(x, y);
	}

	void SetSize(VPANEL vguiPanel, int wide, int tall) override
	{
		((VPanel*)vguiPanel)->SetSize(wide, tall);
	}

	void GetSize(VPANEL vguiPanel, int& wide, int& tall) override
	{
		((VPanel*)vguiPanel)->GetSize(wide, tall);
	}

	void SetMinimumSize(VPANEL vguiPanel, int wide, int tall) override
	{
		((VPanel*)vguiPanel)->SetMinimumSize(wide, tall);
	}

	void GetMinimumSize(VPANEL vguiPanel, int& wide, int& tall) override
	{
		((VPanel*)vguiPanel)->GetMinimumSize(wide, tall);
	}

	void SetZPos(VPANEL vguiPanel, int z) override
	{
		((VPanel*)vguiPanel)->SetZPos(z);
	}

	int GetZPos(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->GetZPos();
	}

	void GetAbsPos(VPANEL vguiPanel, int& x, int& y) override
	{
		((VPanel*)vguiPanel)->GetAbsPos(x, y);
	}

	void GetClipRect(VPANEL vguiPanel, int& x0, int& y0, int& x1, int& y1) override
	{
		((VPanel*)vguiPanel)->GetClipRect(x0, y0, x1, y1);
	}

	void SetInset(VPANEL vguiPanel, int left, int top, int right, int bottom) override
	{
		((VPanel*)vguiPanel)->SetInset(left, top, right, bottom);
	}

	void GetInset(VPANEL vguiPanel, int& left, int& top, int& right, int& bottom) override
	{
		((VPanel*)vguiPanel)->GetInset(left, top, right, bottom);
	}

	void SetVisible(VPANEL vguiPanel, bool state) override
	{
		((VPanel*)vguiPanel)->SetVisible(state);
	}

	bool IsVisible(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->IsVisible();
	}

	void SetParent(VPANEL vguiPanel, VPANEL newParent) override
	{
		((VPanel*)vguiPanel)->SetParent((VPanel*)newParent);
	}

	int GetChildCount(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->GetChildCount();
	}

	VPANEL GetChild(VPANEL vguiPanel, int index) override
	{
		return (VPANEL)((VPanel*)vguiPanel)->GetChild(index);
	}

	VPANEL GetParent(VPANEL vguiPanel) override
	{
		return (VPANEL)((VPanel*)vguiPanel)->GetParent();
	}

	void MoveToFront(VPANEL vguiPanel) override
	{
		((VPanel*)vguiPanel)->MoveToFront();
	}

	void MoveToBack(VPANEL vguiPanel) override
	{
		((VPanel*)vguiPanel)->MoveToBack();
	}

	bool HasParent(VPANEL vguiPanel, VPANEL potentialParent) override
	{
		if (!vguiPanel)
			return false;

		return ((VPanel*)vguiPanel)->HasParent((VPanel*)potentialParent);
	}

	bool IsPopup(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->IsPopup();
	}

	void SetPopup(VPANEL vguiPanel, bool state) override
	{
		((VPanel*)vguiPanel)->SetPopup(state);
	}

	bool Render_GetPopupVisible(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->Render_IsPopupPanelVisible();
	}

	void Render_SetPopupVisible(VPANEL vguiPanel, bool state) override
	{
		return ((VPanel*)vguiPanel)->Render_SetPopupVisible(state);
	}

	HScheme GetScheme(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->GetScheme();
	}

	bool IsProportional(VPANEL vguiPanel) override
	{
		return Client(vguiPanel)->IsProportional();
	}

	bool IsAutoDeleteSet(VPANEL vguiPanel) override
	{
		return Client(vguiPanel)->IsAutoDeleteSet();
	}

	void DeletePanel(VPANEL vguiPanel) override
	{
		Client(vguiPanel)->DeletePanel();
	}

	void SetKeyBoardInputEnabled(VPANEL vguiPanel, bool state)override
	{
		((VPanel*)vguiPanel)->SetKeyBoardInputEnabled(state);
	}

	void SetMouseInputEnabled(VPANEL vguiPanel, bool state) override
	{
		((VPanel*)vguiPanel)->SetMouseInputEnabled(state);
	}

	bool IsMouseInputEnabled(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->IsMouseInputEnabled();
	}

	bool IsKeyBoardInputEnabled(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->IsKeyBoardInputEnabled();
	}

	void Solve(VPANEL vguiPanel) override
	{
		((VPanel*)vguiPanel)->Solve();
	}

	const char* GetName(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->GetName();
	}

	const char* GetClassName(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->GetClassName();
	}

	void SendMessage(VPANEL vguiPanel, KeyValues* params, VPANEL ifrompanel) override
	{
		((VPanel*)vguiPanel)->SendMessage(params, ifrompanel);
	}

	void Think(VPANEL vguiPanel) override
	{
		Client(vguiPanel)->Think();
	}

	void PerformApplySchemeSettings(VPANEL vguiPanel) override
	{
		Client(vguiPanel)->PerformApplySchemeSettings();
	}

	void PaintTraverse(VPANEL vguiPanel, bool forceRepaint, bool allowForce) override
	{
		Client(vguiPanel)->PaintTraverse(forceRepaint, allowForce);
	}

	void Repaint(VPANEL vguiPanel) override
	{
		Client(vguiPanel)->Repaint();
	}

	VPANEL IsWithinTraverse(VPANEL vguiPanel, int x, int y, bool traversePopups)override
	{
		return Client(vguiPanel)->IsWithinTraverse(x, y, traversePopups);
	}

	void OnChildAdded(VPANEL vguiPanel, VPANEL child)override
	{
		Client(vguiPanel)->OnChildAdded(child);
	}

	void OnSizeChanged(VPANEL vguiPanel, int newWide, int newTall)override
	{
		Client(vguiPanel)->OnSizeChanged(newWide, newTall);
	}

	void InternalFocusChanged(VPANEL vguiPanel, bool lost) override
	{
		Client(vguiPanel)->InternalFocusChanged(lost);
	}

	bool RequestInfo(VPANEL vguiPanel, KeyValues* outputData)override
	{
		return Client(vguiPanel)->RequestInfo(outputData);
	}

	void RequestFocus(VPANEL vguiPanel, int direction = 0)override
	{
		Client(vguiPanel)->RequestFocus(direction);
	}

	bool RequestFocusPrev(VPANEL vguiPanel, VPANEL existingPanel)override
	{
		return Client(vguiPanel)->RequestFocusPrev(existingPanel);
	}

	bool RequestFocusNext(VPANEL vguiPanel, VPANEL existingPanel)override
	{
		return Client(vguiPanel)->RequestFocusNext(existingPanel);
	}

	VPANEL GetCurrentKeyFocus(VPANEL vguiPanel)override
	{
		return Client(vguiPanel)->GetCurrentKeyFocus();
	}

	int GetTabPosition(VPANEL vguiPanel)override
	{
		return Client(vguiPanel)->GetTabPosition();
	}

	SurfacePlat* Plat(VPANEL vguiPanel)override
	{
		return ((VPanel*)vguiPanel)->Plat();
	}

	void SetPlat(VPANEL vguiPanel, SurfacePlat* Plat)override
	{
		((VPanel*)vguiPanel)->SetPlat(Plat);
	}

	Panel* GetPanel(VPANEL vguiPanel, const char* moduleName)override
	{
		if (vguiPanel == surface()->GetEmbeddedPanel())
			return NULL;

		if (stricmp(GetModuleName(vguiPanel), moduleName))
			return NULL;

		return Client(vguiPanel)->GetPanel();
	}

	bool IsEnabled(VPANEL vguiPanel)override
	{
		return ((VPanel*)vguiPanel)->IsEnabled();
	}

	void SetEnabled(VPANEL vguiPanel, bool state)override
	{
		((VPanel*)vguiPanel)->SetEnabled(state);
	}

	IClientPanel* Client(VPANEL vguiPanel)override
	{
		return ((VPanel*)vguiPanel)->Client();
	}

	const char* GetModuleName(VPANEL vguiPanel)override
	{
		return Client(vguiPanel)->GetModuleName();
	}

public:
	bool IsTopmostPopup(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->IsTopmostPopup();
	}

	void SetTopmostPopup(VPANEL vguiPanel, bool state) override
	{
		return ((VPanel*)vguiPanel)->SetTopmostPopup(state);
	}

	bool IsFullyVisible(VPANEL vguiPanel) override
	{
		return ((VPanel*)vguiPanel)->IsFullyVisible();
	}
};

static VPanelWrapper g_Panel;

//TODO: FIX ME!!!
vgui::IPanel2 *g_pVGuiPanel2 = NULL;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(VPanelWrapper, IPanel, VGUI_PANEL_INTERFACE_VERSION, g_Panel);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(VPanelWrapper, IPanel2, VGUI_PANEL2_INTERFACE_VERSION, g_Panel);
