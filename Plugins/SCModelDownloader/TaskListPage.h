#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

class CTaskListPanel;

class CTaskListPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CTaskListPage, vgui::PropertyPage);

	CTaskListPage(vgui::Panel* parent, const char* name);

private:

	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC(OnRefreshTaskList, "RefreshTaskList");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CTaskListPanel* m_pTaskListPanel{};
};