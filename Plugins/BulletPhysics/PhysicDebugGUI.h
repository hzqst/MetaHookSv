#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/QueryBox.h>

enum class PhysicInspectMode
{
	Entity,
	PhysicObject,
	RigidBody,
	Constraint,
	Floater,
};

enum class PhysicEditMode
{
	None,
	Move,
	Rotate,
	Resize
};

class IPhysicComponent;

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
	void Activate() override;

protected:

	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);
	MESSAGE_FUNC_UINT64(OnCreateStaticObject, "CreateStaticObject", physicObjectId);
	MESSAGE_FUNC_UINT64(OnCreateDynamicObject, "CreateDynamicObject", physicObjectId);
	MESSAGE_FUNC_UINT64(OnCreateRagdollObject, "CreateRagdollObject", physicObjectId);

	MESSAGE_FUNC_PARAMS(OnEditPhysicObject, "EditPhysicObject", kv);

	MESSAGE_FUNC_UINT64(OnCreateRigidBody, "CreateRigidBody", physicObjectId);
	MESSAGE_FUNC_PARAMS(OnEditRigidBodyEx, "EditRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnMoveRigidBodyEx, "MoveRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnRotateRigidBodyEx, "RotateRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnResizeRigidBodyEx, "ResizeRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnCloneRigidBodyEx, "CloneRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnDeleteRigidBodyEx, "DeleteRigidBodyEx", kv);

	MESSAGE_FUNC_UINT64(OnCreateConstraint, "CreateConstraint", physicObjectId);
	MESSAGE_FUNC_PARAMS(OnEditConstraintEx, "EditConstraintEx", kv);
	MESSAGE_FUNC_PARAMS(OnMoveConstraintEx, "MoveConstraintEx", kv);
	MESSAGE_FUNC_PARAMS(OnRotateConstraintEx, "RotateConstraintEx", kv);
	MESSAGE_FUNC_PARAMS(OnResizeConstraintEx, "ResizeConstraintEx", kv);
	MESSAGE_FUNC_PARAMS(OnCloneConstraintEx, "CloneConstraintEx", kv);
	MESSAGE_FUNC_PARAMS(OnDeleteConstraintEx, "DeleteConstraintEx", kv);

	void OnThink() override;
	void PerformLayout(void) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	void OnCommand(const char* command) override;
	void OnMousePressed(vgui::MouseCode code) override;
	void OnMouseDoublePressed(vgui::MouseCode code) override;
	void OnKeyCodeTyped(vgui::KeyCode code) override;

	void Reset();

	void SaveOpenPrompt();
	void SaveConfirm();

	bool DeleteRigidBodyByComponent(IPhysicComponent* pPhysicComponent);
	bool DeleteRigidBodyByComponentId(int physicComponentId);
	bool DeleteConstraintByComponent(IPhysicComponent* pPhysicComponent);
	bool DeleteConstraintByComponentId(int physicComponentId);
	bool OpenEditPhysicObjectDialog(uint64 physicObjectId);
	bool OpenEditPhysicObjectDialogEx(uint64 physicObjectId, int physicObjectConfigId);
	bool OpenEditRigidBodyDialog(uint64 physicObjectId, int physicObjectConfigId, int rigidBodyConfigId);
	bool OpenEditConstraintDialog(uint64 physicObjectId, int physicObjectConfigId, int constraintConfigId);

	bool UpdateInspectedClientEntity(bool bSelected);
	bool UpdateInspectedPhysicObject(bool bSelected);
	bool UpdateInspectedRigidBody(bool bSelected);
	bool UpdateInspectedConstraint(bool bSelected);
	void UpdateInspectMode(PhysicInspectMode mode);
	void UpdateEditMode(PhysicEditMode mode);

	bool OpenInspectClientEntityMenu(bool bSelected);
	bool OpenInspectPhysicObjectMenu(bool bSelected);
	bool OpenInspectPhysicComponentMenu(bool bSelected);

	void ShowInspectContentLabel(const wchar_t* wszText);
	void HideInspectContentLabel();

	void ShowInspectContentLabel2(const wchar_t* wszText);
	void HideInspectContentLabel2();

	void ShowInspectContentLabel3(const wchar_t* wszText);
	void HideInspectContentLabel3();

	bool UpdateConfigOrigin(int physicComponentId, int axis, float value);
	bool UpdateConfigAngles(int physicComponentId, int axis, float value);
	bool UpdateConfigSize(int physicComponentId, int axis, float value);

	void LoadAvailableInspectModeIntoControls();
	void LoadAvailableEditModeIntoControls();
protected:

	vgui::Panel* m_pTopBar{};
	vgui::Panel* m_pBottomBarBlank{};

	vgui::Button* m_pReload{};
	vgui::Button* m_pSave{};
	vgui::ComboBox* m_pInspectMode{};

	vgui::Button* m_pClose{};

	vgui::Label* m_pInspectContentLabel{};
	vgui::Label* m_pInspectContentLabel2{};
	vgui::Label* m_pInspectContentLabel3{};
	vgui::Label* m_pInspectModeLabel{};
	vgui::Label* m_pEditModeLabel{};

	PhysicInspectMode m_InspectMode{ PhysicInspectMode::Entity };
	PhysicEditMode m_EditMode{ PhysicEditMode::None };
};