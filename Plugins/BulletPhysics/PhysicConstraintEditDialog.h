#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>

#include "ClientPhysicConfig.h"

class CPhysicFactorListPanel;

class CPhysicConstraintEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicConstraintEditDialog, vgui::Frame);

	CPhysicConstraintEditDialog(vgui::Panel* parent, const char* name,
		uint64 physicObjectId,
		const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
		const std::shared_ptr<CClientConstraintConfig>& pConstraintConfig);
	~CPhysicConstraintEditDialog();

	void Activate(void) override;

private:
	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);
	MESSAGE_FUNC_PARAMS(OnModifyFactor, "ModifyFactor", kv);

	void OnCommand(const char* command) override;
	void OnKeyCodeTyped(vgui::KeyCode code) override;

	void LoadAvailableTypesIntoControl(vgui::ComboBox* pComboBox);
	void LoadAvailableRotOrdersIntoControl(vgui::ComboBox* pComboBox);
	void LoadAvailableRigidBodiesIntoControl(vgui::ComboBox* pComboBox);
	void LoadAvailableFactorsIntoControl(int type);
	void DeleteFactorListPanelItem(int factorIdx);
	void LoadFactorAsListPanelItem(int factorIdx, const char* token, float value, float defaultValue);
	void LoadFactorAsListPanelItemEx(int factorIdx, const char* name, const char* value, float defaultValue);
	void LoadTypeIntoControl(int type);
	void LoadRotOrderIntoControl(int rotOrder);
	void LoadRigidBodyIntoControl(vgui::ComboBox* pComboBox, const std::string& rigidBodyName);
	void LoadConfigIntoControls();
	void SaveTypeFromControl(vgui::ComboBox* pComboBox);
	void SaveRotOrderFromControl(vgui::ComboBox* pComboBox);
	void SaveFactorsFromControl(vgui::ListPanel* pListPanel);
	void SaveRigidBodyFromControl(vgui::ComboBox* pComboBox, std::string& rigidBodyName);
	void SaveConfigFromControls();
	int GetCurrentSelectedConstraintType();
	void UpdateControlStates();

	typedef vgui::Frame BaseClass;

	vgui::TextEntry* m_pName{};
	vgui::TextEntry* m_pDebugDrawLevel{};

	vgui::ComboBox* m_pType{};

	vgui::ComboBox* m_pRigidBodyA{};
	vgui::ComboBox* m_pRigidBodyB{};

	vgui::TextEntry* m_pOriginAX{};
	vgui::TextEntry* m_pOriginAY{};
	vgui::TextEntry* m_pOriginAZ{};
	vgui::TextEntry* m_pAnglesAX{};
	vgui::TextEntry* m_pAnglesAY{};
	vgui::TextEntry* m_pAnglesAZ{};

	vgui::TextEntry* m_pOriginBX{};
	vgui::TextEntry* m_pOriginBY{};
	vgui::TextEntry* m_pOriginBZ{};
	vgui::TextEntry* m_pAnglesBX{};
	vgui::TextEntry* m_pAnglesBY{};
	vgui::TextEntry* m_pAnglesBZ{};

	vgui::TextEntry* m_pForwardX{};
	vgui::TextEntry* m_pForwardY{};
	vgui::TextEntry* m_pForwardZ{};

	vgui::CheckButton* m_pDisableCollision{};
	vgui::CheckButton* m_pUseGlobalJointFromA{};
	vgui::CheckButton* m_pUseLookAtOther{};
	vgui::CheckButton* m_pUseGlobalJointOriginFromOther{};
	vgui::CheckButton* m_pUseRigidBodyDistanceAsLinearLimit{};
	vgui::CheckButton* m_pUseLinearReferenceFrameA{};

	vgui::ComboBox* m_pRotOrder{};
	vgui::TextEntry* m_pMaxTolerantLinearError{};

	vgui::CheckButton* m_pBarnacle{};
	vgui::CheckButton* m_pGargantua{};
	vgui::CheckButton* m_pDeactiveOnNormalActivity{};
	vgui::CheckButton* m_pDeactiveOnDeathActivity{};
	vgui::CheckButton* m_pDeactiveOnCaughtByBarnacleActivity{};
	vgui::CheckButton* m_pDeactiveOnBarnacleCatchingActivity{};
	vgui::CheckButton* m_pDontResetPoseOnErrorCorrection{};

	CPhysicFactorListPanel* m_pPhysicFactorListPanel{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
	std::shared_ptr<CClientConstraintConfig> m_pConstraintConfig;
};