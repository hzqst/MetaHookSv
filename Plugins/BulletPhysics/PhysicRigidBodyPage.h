#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

#include "ClientPhysicConfig.h"

class CPhysicRigidBodyListPanel;

class CPhysicRigidBodyPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicRigidBodyPage, vgui::PropertyPage);

	CPhysicRigidBodyPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

private:

	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);
	MESSAGE_FUNC_INT(OnRefreshRigidBody, "RefreshRigidBody", configId);
	MESSAGE_FUNC_INT(OnEditRigidBody, "EditRigidBody", configId);
	MESSAGE_FUNC_INT(OnCloneRigidBody, "CloneRigidBody", configId);
	MESSAGE_FUNC_INT(OnDeleteRigidBody, "DeleteRigidBody", configId);
	MESSAGE_FUNC_INT(OnShiftUpRigidBody, "ShiftUpRigidBody", configId);
	MESSAGE_FUNC_INT(OnShiftDownRigidBody, "ShiftDownRigidBody", configId);
	MESSAGE_FUNC(OnRefreshRigidBodies, "RefreshRigidBodies");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	void LoadRigidBodyAsListPanelItem(const CClientRigidBodyConfig* pRigidBodyConfig);
	void ReloadAllRigidBodiesIntoListPanelItem();
	void OnOpenRigidBodyEditor(int configId);
	void OnCreateRigidBody();
	void SelectRigidBodyItem(int configId);
	void DeleteRigidBodyItem(int configId);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CPhysicRigidBodyListPanel* m_pPhysicRigidBodyListPanel{};
	vgui::Button* m_pShiftUpRigidBody{};
	vgui::Button* m_pShiftDownRigidBody{};
	vgui::Button* m_pCreateRigidBody{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};