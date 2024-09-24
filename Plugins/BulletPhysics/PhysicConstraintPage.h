#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

#include "ClientPhysicConfig.h"

class CPhysicConstraintListPanel;

class CPhysicConstraintPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicConstraintPage, vgui::PropertyPage);

	CPhysicConstraintPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

private:

	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);
	MESSAGE_FUNC_INT(OnRefreshConstraint, "RefreshConstraint", configId);
	MESSAGE_FUNC_INT(OnEditConstraint, "EditConstraint", configId);
	MESSAGE_FUNC_INT(OnCloneConstraint, "CloneConstraint", configId);
	MESSAGE_FUNC_INT(OnDeleteConstraint, "DeleteConstraint", configId);
	MESSAGE_FUNC_INT(OnShiftUpConstraint, "ShiftUpConstraint", configId);
	MESSAGE_FUNC_INT(OnShiftDownConstraint, "ShiftDownConstraint", configId);
	MESSAGE_FUNC(OnRefreshConstraints, "RefreshConstraints");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	void LoadConstraintAsListPanelItem(const CClientConstraintConfig* pConstraintConfig);
	void ReloadAllConstraintsIntoListPanelItem();
	void OnOpenConstraintEditor(int configId);
	void OnCreateConstraint();
	void SelectConstraintItem(int configId);
	void DeleteConstraintItem(int configId);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CPhysicConstraintListPanel* m_pPhysicConstraintListPanel{};
	vgui::Button* m_pShiftUpConstraint{};
	vgui::Button* m_pShiftDownConstraint{};
	vgui::Button* m_pCreateConstraint{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};