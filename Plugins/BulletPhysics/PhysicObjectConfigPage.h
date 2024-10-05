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

#define DEFINE_CHECK_BUTTON(name) vgui::CheckButton* m_p##name{}
	DEFINE_CHECK_BUTTON(Barnacle);
	DEFINE_CHECK_BUTTON(Gargantua);
	DEFINE_CHECK_BUTTON(OverrideStudioCheckBBox);
	DEFINE_CHECK_BUTTON(VerifyBoneChunk);
	DEFINE_CHECK_BUTTON(VerifyModelFile);
#undef DEFINE_CHECK_BUTTON

	vgui::TextEntry* m_pCrc32BoneChunk{};
	vgui::TextEntry* m_pCrc32ModelFile{};
	vgui::TextEntry* m_pDebugDrawLevel{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};
