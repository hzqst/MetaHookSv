#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

#include "PhysicGUICommon.h"

class CPhysicDebugGUI : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CPhysicDebugGUI, vgui::Frame);

public:

	CPhysicDebugGUI(vgui::Panel* parent);
	~CPhysicDebugGUI();

	void NewMap(void);
	void Init();
	void UpdateInspectStuffs();

	bool HasFocus() override;

protected:

	MESSAGE_FUNC_UINT64(OnCreatePhysicObject, "CreatePhysicObject", physicObjectId);
	MESSAGE_FUNC_UINT64(OnEditPhysicObject, "EditPhysicObject", physicObjectId);
	MESSAGE_FUNC_INT_UINT64(OnEditRigidBodyEx, "EditRigidBodyEx", configId, physicObjectId);
	MESSAGE_FUNC_INT_UINT64(OnMoveRigidBodyEx, "MoveRigidBodyEx", configId, physicObjectId);
	MESSAGE_FUNC_INT_UINT64(OnRotateRigidBodyEx, "RotateRigidBodyEx", configId, physicObjectId);
	MESSAGE_FUNC_INT_UINT64(OnDeleteRigidBodyEx, "DeleteRigidBodyEx", configId, physicObjectId);

	void Reset();
	void OnThink() override;
	void PerformLayout(void) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	void OnCommand(const char* command) override;
	void OnMousePressed(vgui::MouseCode code) override;
	void OnMouseDoublePressed(vgui::MouseCode code) override;
	void OnKeyCodeTyped(vgui::KeyCode code) override;

	void OpenEditPhysicObjectDialog(uint64 physicObjectId);
	void OpenEditRigidBodyDialog(int configId, uint64 physicObjectId);

	bool UpdateInspectedClientEntity(bool bSelected);
	bool UpdateInspectedPhysicObject(bool bSelected);
	bool UpdateInspectedRigidBody(bool bSelected);
	void UpdateInspectMode(PhysicInspectMode mode);
	void UpdateEditMode(PhysicEditMode mode);

	void ShowInspectContentLabel(const wchar_t* wszText);
	void HideInspectContentLabel();

	void ShowInspectContentLabel2(const wchar_t* wszText);
	void HideInspectContentLabel2();

protected:

	vgui::Button* m_pClose{};
	vgui::Panel* m_pTopBar{};
	vgui::Panel* m_pBottomBarBlank{};
	vgui::Label* m_pInspectContentLabel{};
	vgui::Label* m_pInspectContentLabel2{};
	vgui::Label* m_pInspectModeLabel{};

	PhysicInspectMode m_InspectMode{ PhysicInspectMode::Entity };
	PhysicEditMode m_EditMode{ PhysicEditMode::None };
};