#pragma once

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Menu.h>

#include "ClientPhysicConfig.h"

class CRigidBodyListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CRigidBodyListPanel, vgui::ListPanel);

	CRigidBodyListPanel(vgui::Panel *parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};

class CConstraintListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CConstraintListPanel, vgui::ListPanel);

	CConstraintListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};

class CPhysicActionListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicActionListPanel, vgui::ListPanel);

	CPhysicActionListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};

class CInlineTextEntryPanel;

class CFactorListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CFactorListPanel, vgui::ListPanel);

	CFactorListPanel(vgui::Panel* parent, const char* pName);
	~CFactorListPanel();

	void StartCaptureMode();
	void EndCaptureMode();
	bool IsCapturing(void) const;
	int GetCapturingItemId(void) const;
	int GetCapturingItemIndex(void) const;
	void OnMousePressed(vgui::MouseCode code) override;
private:

	typedef vgui::ListPanel BaseClass;
	bool m_bCaptureMode{};
	int m_iCaptureItemId{};
	int m_iCaptureItemIndex{};
	CInlineTextEntryPanel* m_pInlineTextEntryPanel{};
};

class CBaseObjectConfigPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CBaseObjectConfigPage, vgui::PropertyPage);

	CBaseObjectConfigPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

private:

	MESSAGE_FUNC(OnApplyChanges, "ApplyChanges");
	MESSAGE_FUNC(OnResetData, "ResetData");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;

	void LoadConfigIntoControls();
	void SaveConfigFromControls();

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	vgui::CheckButton* m_pStaticObject{};
	vgui::CheckButton* m_pDynamicObject{};
	vgui::CheckButton* m_pRagdollObject{};
	vgui::CheckButton* m_pFromBSP{};
	vgui::CheckButton* m_pFromConfig{};
	vgui::CheckButton* m_pBarnacle{};
	vgui::CheckButton* m_pGargantua{};
	vgui::TextEntry* m_pDebugDrawLevel{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};

class CRigidBodyPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CRigidBodyPage, vgui::PropertyPage);

	CRigidBodyPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);
	
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

	CRigidBodyListPanel* m_pRigidBodyListPanel{};
	vgui::Button* m_pShiftUpRigidBody{};
	vgui::Button* m_pShiftDownRigidBody{};
	vgui::Button* m_pCreateRigidBody{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};

class CConstraintPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CConstraintPage, vgui::PropertyPage);

	CConstraintPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

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

	CConstraintListPanel* m_pConstraintListPanel{};
	vgui::Button* m_pShiftUpConstraint{};
	vgui::Button* m_pShiftDownConstraint{};
	vgui::Button* m_pCreateConstraint{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};

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

class CCollisionShapeEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CCollisionShapeEditDialog, vgui::Frame);

	CCollisionShapeEditDialog(vgui::Panel* parent, const char* name, uint64 physicObjectId,
		const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
		const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig,
		const std::shared_ptr<CClientCollisionShapeConfig>& pCollisionShapeConfig);
	~CCollisionShapeEditDialog();

	void Activate(void) override;

private:
	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);

	void OnCommand(const char* command) override;

	void LoadAvailableShapesIntoControls();
	void LoadShapeIntoControls();

	void LoadAvailableShapeDirectionsIntoControls();
	void LoadShapeDirectionIntoControls();

	void LoadConfigIntoControls();
	void SaveConfigFromControls();
	void UpdateControlStates();

	int GetCurrentSelectedShapeType();
	int GetCurrentSelectedShapeDirection();

private:
	vgui::ComboBox* m_pShape{};

	vgui::Label* m_pDirectionLabel{};
	vgui::ComboBox* m_pDirection{};

	vgui::Label* m_pSizeLabel{};
	vgui::TextEntry* m_pSizeX{};
	vgui::TextEntry* m_pSizeY{};
	vgui::TextEntry* m_pSizeZ{};

	vgui::Label* m_pOriginLabel{};
	vgui::TextEntry* m_pOriginX{};
	vgui::TextEntry* m_pOriginY{};
	vgui::TextEntry* m_pOriginZ{};

	vgui::Label* m_pAnglesLabel{};
	vgui::TextEntry* m_pAnglesX{};
	vgui::TextEntry* m_pAnglesY{};
	vgui::TextEntry* m_pAnglesZ{};

	vgui::Label* m_pResourcePathLabel{};
	vgui::TextEntry* m_pResourcePath{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
	std::shared_ptr<CClientRigidBodyConfig> m_pRigidBodyConfig;
	std::shared_ptr<CClientCollisionShapeConfig> m_pCollisionShapeConfig;
};

class CRigidBodyEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CRigidBodyEditDialog, vgui::Frame);

	CRigidBodyEditDialog(vgui::Panel* parent, const char* name, 
		uint64 physicObjectId,
		const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig, 
		const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig);
	~CRigidBodyEditDialog();

	void Activate(void) override;

private:
	MESSAGE_FUNC_INT(OnRefreshCollisionShape, "RefreshCollisionShape", configId);
	MESSAGE_FUNC(OnResetData, "ResetData");
	void OnCommand(const char* command) override;

	void LoadAvailableShapesIntoControls();
	void LoadAvailableBonesIntoControls();
	void LoadShapeIntoControls(const CClientCollisionShapeConfigSharedPtr& collisionShape);
	void LoadBoneIntoControls(int boneindex);
	void LoadConfigIntoControls();
	void SaveConfigFromControls();
	void OnEditCollisionShape();
	int GetCurrentSelectedBoneIndex();

	typedef vgui::Frame BaseClass;

	vgui::TextEntry* m_pName{};
	vgui::TextEntry* m_pDebugDrawLevel{};
	vgui::ComboBox* m_pBone{};
	vgui::ComboBox* m_pShape{};
	vgui::TextEntry* m_pOriginX{};
	vgui::TextEntry* m_pOriginY{};
	vgui::TextEntry* m_pOriginZ{};
	vgui::TextEntry* m_pAnglesX{};
	vgui::TextEntry* m_pAnglesY{};
	vgui::TextEntry* m_pAnglesZ{};
	vgui::TextEntry* m_pMass{};
	vgui::TextEntry* m_pDensity{};
	vgui::TextEntry* m_pLinearFriction{};
	vgui::TextEntry* m_pRollingFriction{};
	vgui::TextEntry* m_pRestitution{};
	vgui::TextEntry* m_pCCDRadius{};
	vgui::TextEntry* m_pCCDThreshold{};
	vgui::TextEntry* m_pLinearSleepingThreshold{};
	vgui::TextEntry* m_pAngularSleepingThreshold{};
	vgui::CheckButton* m_pAlwaysDynamic{};
	vgui::CheckButton* m_pAlwaysKinematic{};
	vgui::CheckButton* m_pAlwaysStatic{};
	vgui::CheckButton* m_pNoCollisionToWorld{};
	vgui::CheckButton* m_pNoCollisionToStaticObject{};
	vgui::CheckButton* m_pNoCollisionToDynamicObject{};
	vgui::CheckButton* m_pNoCollisionToRagdollObject{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
	std::shared_ptr<CClientRigidBodyConfig> m_pRigidBodyConfig;
};

class CConstraintEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CConstraintEditDialog, vgui::Frame);

	CConstraintEditDialog(vgui::Panel* parent, const char* name,
		uint64 physicObjectId,
		const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
		const std::shared_ptr<CClientConstraintConfig>& pConstraintConfig);
	~CConstraintEditDialog();

	void Activate(void) override;

private:
	MESSAGE_FUNC(OnItemSelected, "ItemSelected");
	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);
	MESSAGE_FUNC_PARAMS(OnModifyFactor, "ModifyFactor", kv);

	void OnCommand(const char* command) override;
	void OnKeyCodeTyped(vgui::KeyCode code) override;

	void LoadAvailableTypesIntoControls();
	void LoadAvailableRotOrdersIntoControls();
	void LoadAvailableRigidBodiesIntoControls(vgui::ComboBox* pComboBox);
	void LoadAvailableFactorsIntoControls(int type);
	void DeleteFactorListPanelItem(int factorIdx);
	void LoadFactorAsListPanelItem(int factorIdx, const char* token, float value, float defaultValue);
	void LoadFactorAsListPanelItemEx(int factorIdx, const char* name, const char* value, float defaultValue);
	void LoadTypeIntoControl(int type);
	void LoadRotOrderIntoControl(int rotOrder);
	void LoadRigidBodyIntoControl(const std::string& rigidBodyName, vgui::ComboBox* pComboBox);
	void LoadConfigIntoControls();
	void SaveTypeFromControl();
	void SaveRotOrderFromControl();
	void SaveFactorFromControls();
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
	vgui::CheckButton* m_pDeactiveOnBarnacleActivity{};
	vgui::CheckButton* m_pDeactiveOnGargantuaActivity{};
	vgui::CheckButton* m_pDontResetPoseOnErrorCorrection{};

	CFactorListPanel *m_pFactorListPanel{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
	std::shared_ptr<CClientConstraintConfig> m_pConstraintConfig;
};

class CPhysicEditorDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicEditorDialog, vgui::Frame);

	CPhysicEditorDialog(vgui::Panel *parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig> &pPhysicObjectConfig);
	~CPhysicEditorDialog();

private:

	void OnCommand(const char* command) override;

	typedef vgui::Frame BaseClass;

	vgui::PropertySheet* m_pTabPanel{};
	CBaseObjectConfigPage* m_pBaseObjectConfigPage{};
	CRigidBodyPage* m_pRigidBodyPage{};
	CConstraintPage* m_pConstraintPage{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};