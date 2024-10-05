#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

#include "ClientPhysicConfig.h"

class CAnimControlListPanel;

class CAnimControlPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CAnimControlPage, vgui::PropertyPage);

	CAnimControlPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientRagdollObjectConfig>& pRagdollObjectConfig);

private:

	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);
	MESSAGE_FUNC_INT(OnRefreshAnimControl, "RefreshAnimControl", configId);
	MESSAGE_FUNC_INT(OnEditAnimControl, "EditAnimControl", configId);
	MESSAGE_FUNC_INT(OnCloneAnimControl, "CloneAnimControl", configId);
	MESSAGE_FUNC_INT(OnDeleteAnimControl, "DeleteAnimControl", configId);
	MESSAGE_FUNC_INT(OnShiftUpAnimControl, "ShiftUpAnimControl", configId);
	MESSAGE_FUNC_INT(OnShiftDownAnimControl, "ShiftDownAnimControl", configId);
	MESSAGE_FUNC(OnRefreshAnimControls, "RefreshAnimControls");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	void LoadAnimControlAsListPanelItem(const CClientAnimControlConfig* pAnimControlConfig);
	void ReloadAllAnimControlsIntoListPanelItem();
	void OnOpenAnimControlEditor(int configId);
	void OnCreateAnimControl();
	void SelectAnimControlItem(int configId);
	void DeleteAnimControlItem(int configId);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CAnimControlListPanel* m_pAnimControlListPanel{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientRagdollObjectConfig> m_pRagdollObjectConfig;
};