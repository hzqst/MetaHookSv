#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

#include "ClientPhysicConfig.h"

class CPhysicActionListPanel;

class CPhysicActionPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicActionPage, vgui::PropertyPage);

	CPhysicActionPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

private:

	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);
	MESSAGE_FUNC_INT(OnRefreshPhysicAction, "RefreshPhysicAction", configId);
	MESSAGE_FUNC_INT(OnEditPhysicAction, "EditPhysicAction", configId);
	MESSAGE_FUNC_INT(OnClonePhysicAction, "ClonePhysicAction", configId);
	MESSAGE_FUNC_INT(OnDeletePhysicAction, "DeletePhysicAction", configId);
	MESSAGE_FUNC_INT(OnShiftUpPhysicAction, "ShiftUpPhysicAction", configId);
	MESSAGE_FUNC_INT(OnShiftDownPhysicAction, "ShiftDownPhysicAction", configId);
	MESSAGE_FUNC(OnRefreshPhysicActions, "RefreshPhysicActions");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	void LoadPhysicActionAsListPanelItem(const CClientPhysicActionConfig* pPhysicActionConfig);
	void ReloadAllPhysicActionsIntoListPanelItem();
	void OnOpenPhysicActionEditor(int configId);
	void OnCreatePhysicAction();
	void SelectPhysicActionItem(int configId);
	void DeletePhysicActionItem(int configId);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CPhysicActionListPanel* m_pPhysicActionListPanel{};
	vgui::Button* m_pShiftUpPhysicAction{};
	vgui::Button* m_pShiftDownPhysicAction{};
	vgui::Button* m_pCreatePhysicAction{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};