#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

#include "SCModelDatabase.h"

class CTaskListPanel;

class CTaskListPage : public vgui::PropertyPage, public IEnumSCModelQueryHandler, public ISCModelQueryStateChangeHandler
{
public:
	DECLARE_CLASS_SIMPLE(CTaskListPage, vgui::PropertyPage);

	CTaskListPage(vgui::Panel* parent, const char* name);
	~CTaskListPage();

private:

	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC(OnRefreshTaskList, "RefreshTaskList");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	void OnEnumQuery(ISCModelQuery* pQuery) override;
	void OnQueryStateChanged(ISCModelQuery* pQuery, SCModelQueryState newState) override;

	void AddQueryItem(ISCModelQuery* pQuery);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CTaskListPanel* m_pTaskListPanel{};
};