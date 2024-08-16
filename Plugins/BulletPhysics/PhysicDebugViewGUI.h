#pragma once

#include <vgui_controls/EditablePanel.h>

namespace vgui
{
	class Label;
};

class CPhysicDebugViewGUI : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CPhysicDebugViewGUI, vgui::EditablePanel);

public:
	CPhysicDebugViewGUI(vgui::Panel* parent);
	~CPhysicDebugViewGUI();

	void SetInspectEntityLabelText(const wchar_t* wszText);
	void ShowInspectEntityLabel(bool bVisible); 
	void SetInspectPhysicComponentLabelText(const wchar_t* wszText);
	void ShowInspectPhysicComponentLabel(bool bVisible);

public:
	const char* GetName(void) override { return "PhysicDebugViewGUI"; } 

protected:
	void PerformLayout(void) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

protected:
	vgui::Panel* m_pTopBar;
	vgui::Panel* m_pBottomBarBlank;
	vgui::Label* m_pInspectEntityLabel{};
	vgui::Label* m_pInspectPhysicComponentLabel{};
};