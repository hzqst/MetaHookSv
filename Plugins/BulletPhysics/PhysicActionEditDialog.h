#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>

#include "ClientPhysicConfig.h"

class CPhysicFactorListPanel;

class CPhysicActionEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicActionEditDialog, vgui::Frame);

	CPhysicActionEditDialog(vgui::Panel* parent, const char* name,
		uint64 physicObjectId,
		const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
		const std::shared_ptr<CClientPhysicActionConfig>& pPhysicActionConfig);
	~CPhysicActionEditDialog();

	void Activate(void) override;

private:
	MESSAGE_FUNC(OnResetData, "ResetData");

	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);
	MESSAGE_FUNC_PARAMS(OnModifyFactor, "ModifyFactor", kv);

	void OnCommand(const char* command) override;
	void OnKeyCodeTyped(vgui::KeyCode code) override;

	void LoadAvailableTypesIntoControl(vgui::ComboBox* pComboBox);
	void LoadAvailableRigidBodiesIntoControl(vgui::ComboBox* pComboBox);
	void LoadAvailableConstraintsIntoControl(vgui::ComboBox* pComboBox);
	void LoadConfigIntoControls();
	void LoadTypeIntoControl(vgui::ComboBox* pComboBox);
	void LoadRigidBodyIntoControl(vgui::ComboBox* pComboBox, const std::string& rigidBodyName);
	void LoadConstraintIntoControl(vgui::ComboBox* pComboBox, const std::string& constraintName);
	void DeleteFactorListPanelItem(int factorIdx);
	void LoadFactorAsListPanelItemEx(int factorIdx, const char* name, const char* value, float defaultValue);
	void LoadFactorAsListPanelItem(int factorIdx, const char* name, float value, float defaultValue);
	void LoadAvailableFactorsIntoControls(int type);
	void SaveConfigFromControls();
	void SaveTypeFromControl(vgui::ComboBox* pComboBox);
	void SaveRigidBodyFromControl(vgui::ComboBox* pComboBox, std::string& rigidBodyName);
	void SaveConstraintFromControl(vgui::ComboBox* pComboBox, std::string& constraintName);
	void SaveFactorsFromControl(vgui::ListPanel* pListPanel);;
	int GetCurrentSelectedActionTypeIndex();

	void UpdateControlStates();

	typedef vgui::Frame BaseClass;

	vgui::TextEntry* m_pName{};
	vgui::TextEntry* m_pDebugDrawLevel{};
	vgui::ComboBox* m_pType{};
	vgui::ComboBox* m_pRigidBody{};
	vgui::ComboBox* m_pConstraint{};

	vgui::TextEntry* m_pOriginX{};
	vgui::TextEntry* m_pOriginY{};
	vgui::TextEntry* m_pOriginZ{};
	vgui::TextEntry* m_pAnglesX{};
	vgui::TextEntry* m_pAnglesY{};
	vgui::TextEntry* m_pAnglesZ{};

	vgui::CheckButton* m_pBarnacle{};
	vgui::CheckButton* m_pGargantua{};

	CPhysicFactorListPanel* m_pPhysicFactorListPanel;

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
	std::shared_ptr<CClientPhysicActionConfig> m_pPhysicActionConfig;
};