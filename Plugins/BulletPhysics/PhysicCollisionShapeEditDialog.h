#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>

#include "ClientPhysicConfig.h"

class CPhysicCollisionShapeEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicCollisionShapeEditDialog, vgui::Frame);

	CPhysicCollisionShapeEditDialog(vgui::Panel* parent, const char* name, uint64 physicObjectId,
		const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
		const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig,
		const std::shared_ptr<CClientCollisionShapeConfig>& pCollisionShapeConfig);
	~CPhysicCollisionShapeEditDialog();

	void Activate(void) override;

private:
	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);

	void OnCommand(const char* command) override;

	void LoadConfigIntoControls();
	void LoadAvailableShapesIntoControl(vgui::ComboBox *pComboBox);
	void LoadAvailableShapeDirectionsIntoControl(vgui::ComboBox* pComboBox);
	void LoadShapeIntoControl(vgui::ComboBox* pComboBox);
	void LoadShapeDirectionIntoControl(vgui::ComboBox* pComboBox);

	void SaveConfigFromControls();

	int GetCurrentSelectedShapeType();
	int GetCurrentSelectedShapeDirection();

	void UpdateControlStates();

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
