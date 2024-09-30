#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

#include "ClientPhysicConfig.h"

class CPhysicBehaviorListPanel;

class CPhysicBehaviorPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicBehaviorPage, vgui::PropertyPage);

	CPhysicBehaviorPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

private:

	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);
	MESSAGE_FUNC_INT(OnRefreshPhysicBehavior, "RefreshPhysicBehavior", configId);
	MESSAGE_FUNC_INT(OnEditPhysicBehavior, "EditPhysicBehavior", configId);
	MESSAGE_FUNC_INT(OnClonePhysicBehavior, "ClonePhysicBehavior", configId);
	MESSAGE_FUNC_INT(OnDeletePhysicBehavior, "DeletePhysicBehavior", configId);
	MESSAGE_FUNC_INT(OnShiftUpPhysicBehavior, "ShiftUpPhysicBehavior", configId);
	MESSAGE_FUNC_INT(OnShiftDownPhysicBehavior, "ShiftDownPhysicBehavior", configId);
	MESSAGE_FUNC(OnRefreshPhysicBehaviors, "RefreshPhysicBehaviors");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	void LoadPhysicBehaviorAsListPanelItem(const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig);
	void ReloadAllPhysicBehaviorsIntoListPanelItem();
	void OnOpenPhysicBehaviorEditor(int configId);
	void OnCreatePhysicBehavior();
	void SelectPhysicBehaviorItem(int configId);
	void DeletePhysicBehaviorItem(int configId);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CPhysicBehaviorListPanel* m_pPhysicBehaviorListPanel{};
	vgui::Button* m_pShiftUpPhysicBehavior{};
	vgui::Button* m_pShiftDownPhysicBehavior{};
	vgui::Button* m_pCreatePhysicBehavior{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};