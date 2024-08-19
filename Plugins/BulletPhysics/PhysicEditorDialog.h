#pragma once

#include <vgui_controls/Button.h>
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

	CRigidBodyPage(vgui::Panel* parent, const char* name, int entindex, int modelindex, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicConfig);
	
private:

	MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);
	MESSAGE_FUNC_INT(OnRefreshRigidBody, "RefreshRigidBody", configId);
	MESSAGE_FUNC(OnRefreshRigidBodies, "RefreshRigidBodies");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	void LoadRigidBodyAsListPanelItem(const CClientRigidBodyConfig* pRigidBodyConfig);
	void ReloadAllRigidBodiesIntoListPanelItem();
	void OnCreateNewRigidBody();
	void OnEditRigidBody(const char* command);
	void OnDeleteRigidBody(const char* command);
	void OpenRigidBodyEditor(int configId);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	CRigidBodyListPanel* m_pRigidBodyListPanel{};
	vgui::Button* m_pCreateNewRigidBody{};

	int m_iInspectEntityIndex{};
	int m_iEngineModelIndex{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicConfig;
};

class CCollisionShapeEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CCollisionShapeEditDialog, vgui::Frame);

	CCollisionShapeEditDialog(vgui::Panel* parent, const char* name, int entindex, int modelindex, const std::shared_ptr<CClientCollisionShapeConfig>& pCollisionShapeConfig);
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

	vgui::Label* m_pObjPathLabel{};
	vgui::TextEntry* m_pObjPath{};

	int m_iInspectEntityIndex{};
	int m_iEngineModelIndex{};
	std::shared_ptr<CClientCollisionShapeConfig> m_pCollisionShapeConfig;
};

class CRigidBodyEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CRigidBodyEditDialog, vgui::Frame);

	CRigidBodyEditDialog(vgui::Panel* parent, const char* name, int entindex, int modelindex, const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig);
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

	int m_iInspectEntityIndex{};
	int m_iEngineModelIndex{};
	std::shared_ptr<CClientRigidBodyConfig> m_pRigidBodyConfig;
};

class CPhysicEditorDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicEditorDialog, vgui::Frame);

	CPhysicEditorDialog(vgui::Panel *parent, const char* name, int entindex, int modelindex, const std::shared_ptr<CClientPhysicObjectConfig> &pPhysicConfig);
	~CPhysicEditorDialog();

private:

	typedef vgui::Frame BaseClass;

	vgui::PropertySheet* m_pTabPanel{};
	CRigidBodyPage* m_pRigidBodyPage{};

	int m_iInspectEntityIndex{};
	int m_iEngineModelIndex{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicConfig;
};