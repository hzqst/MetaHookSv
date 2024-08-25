#pragma once

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Frame.h>

#include "ClientPhysicConfig.h"

class CRigidBodyListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CRigidBodyListPanel, vgui::ListPanel);

	CRigidBodyListPanel(vgui::Panel *parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
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
	MESSAGE_FUNC(OnRefreshCollisionShape, "RefreshCollisionShape");
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

class CPhysicEditorDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicEditorDialog, vgui::Frame);

	CPhysicEditorDialog(vgui::Panel *parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig> &pPhysicObjectConfig);
	~CPhysicEditorDialog();

private:

	typedef vgui::Frame BaseClass;

	vgui::PropertySheet* m_pTabPanel{};
	CRigidBodyPage* m_pRigidBodyPage{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};