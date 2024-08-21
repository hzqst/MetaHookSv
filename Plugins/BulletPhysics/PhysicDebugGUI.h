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
};

enum class PhysicEditMode
{
	None,
	Move,
	Rotate,
	Resize
};

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
	MESSAGE_FUNC_PARAMS(OnEditPhysicObject, "EditPhysicObject", kv);
	MESSAGE_FUNC_PARAMS(OnEditRigidBodyEx, "EditRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnMoveRigidBodyEx, "MoveRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnRotateRigidBodyEx, "RotateRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnResizeRigidBodyEx, "ResizeRigidBodyEx", kv);
	MESSAGE_FUNC_PARAMS(OnDeleteRigidBodyEx, "DeleteRigidBodyEx", kv);

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

	bool OpenEditPhysicObjectDialog(uint64 physicObjectId);
	bool OpenEditRigidBodyDialog(uint64 physicObjectId, int physicObjectConfigId, int rigidBodyConfigId);

	bool UpdateInspectedClientEntity(bool bSelected);
	bool UpdateInspectedPhysicObject(bool bSelected);
	bool UpdateInspectedRigidBody(bool bSelected);
	void UpdateInspectMode(PhysicInspectMode mode);
	void UpdateEditMode(PhysicEditMode mode);

	bool OpenInspectClientEntityMenu(bool bSelected);
	bool OpenInspectPhysicObjectMenu(bool bSelected);
	bool OpenInspectPhysicComponentMenu(bool bSelected);

	void ShowInspectContentLabel(const wchar_t* wszText);
	void HideInspectContentLabel();

	void ShowInspectContentLabel2(const wchar_t* wszText);
	void HideInspectContentLabel2();

	bool UpdateRigidBodyConfigOrigin(int physicComponentId, int axis, float value);
	bool UpdateRigidBodyConfigAngles(int physicComponentId, int axis, float value);
	bool UpdateRigidBodyConfigSize(int physicComponentId, int axis, float value);

protected:

	vgui::Panel* m_pTopBar{};
	vgui::Panel* m_pBottomBarBlank{};

	vgui::Button* m_pReload{};
	vgui::Button* m_pSave{};
	vgui::ComboBox* m_pInspectMode{};

	vgui::Button* m_pClose{};

	vgui::Label* m_pInspectContentLabel{};
	vgui::Label* m_pInspectContentLabel2{};
	vgui::Label* m_pInspectModeLabel{};
	vgui::Label* m_pEditModeLabel{};

	PhysicInspectMode m_InspectMode{ PhysicInspectMode::Entity };
	PhysicEditMode m_EditMode{ PhysicEditMode::None };
};