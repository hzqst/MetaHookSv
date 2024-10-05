#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>

#include "ClientPhysicConfig.h"

class CAnimControlEditDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CAnimControlEditDialog, vgui::Frame);

	CAnimControlEditDialog(vgui::Panel* parent, const char* name, uint64 physicObjectId,
		const std::shared_ptr<CClientRagdollObjectConfig>& pRagdollObjectConfig,
		const std::shared_ptr<CClientAnimControlConfig>& pAnimControlConfig);
	~CAnimControlEditDialog();

	void Activate(void) override;

private:
	MESSAGE_FUNC(OnResetData, "ResetData");

	void OnCommand(const char* command) override;

	void LoadAvailableSequencesIntoControl(vgui::ComboBox* pComboBox);
	void LoadAvailableActivityTypesIntoControl(vgui::ComboBox* pComboBox);
	
	void LoadSequenceIntoControl(vgui::ComboBox* pComboBox, int sequence);
	void LoadActivityTypeIntoControl(vgui::ComboBox* pComboBox);

	void LoadConfigIntoControls();

	void SaveConfigFromControls();

	int GetCurrentSelectedSequence(vgui::ComboBox* pComboBox);

	StudioAnimActivityType GetCurrentSelectedActivityType(vgui::ComboBox* pComboBox);

private:
	vgui::ComboBox* m_pSequence{};
	vgui::ComboBox* m_pGaitSequence{};
	vgui::ComboBox* m_pActivityType{};

	vgui::TextEntry* m_pAnimFrame{};

	vgui::TextEntry* m_pController_0{};
	vgui::TextEntry* m_pController_1{};
	vgui::TextEntry* m_pController_2{};
	vgui::TextEntry* m_pController_3{};

	vgui::TextEntry* m_pBlending_0{};
	vgui::TextEntry* m_pBlending_1{};
	vgui::TextEntry* m_pBlending_2{};
	vgui::TextEntry* m_pBlending_3{};

#define DEFINE_CHECK_BUTTON(name) vgui::CheckButton* m_p##name{}
	DEFINE_CHECK_BUTTON(OverrideAllBones);
	DEFINE_CHECK_BUTTON(OverrideController);
	DEFINE_CHECK_BUTTON(OverrideBlending);
#undef DEFINE_CHECK_BUTTON

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientRagdollObjectConfig> m_pRagdollObjectConfig;
	std::shared_ptr<CClientAnimControlConfig> m_pAnimControlConfig;
};
