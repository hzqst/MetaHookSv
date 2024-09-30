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

class CPhysicObjectConfigPage;
class CPhysicRigidBodyPage;
class CPhysicConstraintPage;
class CPhysicBehaviorPage;
class CAnimControlPage;

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
	CPhysicObjectConfigPage* m_pPhysicObjectConfigPage{};
	CPhysicRigidBodyPage* m_pPhysicRigidBodyPage{};
	CPhysicConstraintPage* m_pPhysicConstraintPage{};
	CPhysicBehaviorPage* m_pPhysicBehaviorPage{};
	CAnimControlPage* m_pAnimControlPage{};

	uint64 m_physicObjectId{};
	std::shared_ptr<CClientPhysicObjectConfig> m_pPhysicObjectConfig;
};