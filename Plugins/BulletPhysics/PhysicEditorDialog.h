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

class CRigidBodyEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CRigidBodyEditDialog, vgui::Frame);

	CRigidBodyEditDialog(vgui::Panel* parent, const char* name, int entindex, int modelindex, const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig);
	~CRigidBodyEditDialog();

private:
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	void LoadBoneIntoControls();
	void LoadConfigIntoControls();

	typedef vgui::Frame BaseClass;

	vgui::TextEntry* m_pName{};
	vgui::ComboBox* m_pBone{};
	vgui::TextEntry* m_pOriginX{};
	vgui::TextEntry* m_pOriginY{};
	vgui::TextEntry* m_pOriginZ{};
	vgui::TextEntry* m_pAnglesX{};
	vgui::TextEntry* m_pAnglesY{};
	vgui::TextEntry* m_pAnglesZ{};
	vgui::TextEntry* m_pMass{};
	vgui::TextEntry* m_pDensity{};

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