#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextEntry.h>

#include "ClientPhysicConfig.h"

class CPhysicObjectConfigPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicObjectConfigPage, vgui::PropertyPage);

	CPhysicObjectConfigPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

private:

	MESSAGE_FUNC(OnApplyChanges, "ApplyChanges");
	MESSAGE_FUNC(OnResetData, "ResetData");

	void OnKeyCodeTyped(vgui::KeyCode code) override;
	void OnCommand(const char* command) override;

	void GenerateCrc32BoneChunk();
	void GenerateCrc32ModelFile();

	void LoadConfigIntoControls();
	void SaveConfigFromControls();

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	vgui::CheckButton* m_pBarnacle{};
	vgui::CheckButton* m_pGargantua{};
	vgui::CheckButton* m_pVerifyBoneChunk{};
	vgui::CheckButton* m_pVerifyModelFile{};
	vgui::TextEntry* m_pCrc32BoneChunk{};
	vgui::TextEntry* m_pCrc32ModelFile{};
	vgui::TextEntry* m_pDebugDrawLevel{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};
