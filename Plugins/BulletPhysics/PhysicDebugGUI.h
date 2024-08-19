#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

class CPhysicDebugGUI : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CPhysicDebugGUI, vgui::Frame);

public:
	CPhysicDebugGUI(vgui::Panel* parent);
	~CPhysicDebugGUI();

	void NewMap(void);

	bool HasFocus() override;

protected:

	void OnThink() override;
	void PerformLayout(void) override;
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	void OnCommand(const char* command) override;
	void OnMousePressed(vgui::MouseCode code) override;
	void OnMouseDoublePressed(vgui::MouseCode code) override;
#if 0
	void OnKeyCodeTyped(vgui::KeyCode code) override;
#endif
	void OnCreateRagdollObject(const char* command);
	void OnEditPhysicObject(const char* command);
	void OpenEditPhysicObjectDialog(int entindex, int modelindex);

	void UpdateInspectEntity(int entindex, int modelindex);
	void UpdateInspectPhysicComponent(int physicComponentId);

	void SetInspectEntityLabelText(const wchar_t* wszText);
	void ShowInspectEntityLabel(bool bVisible);

	void SetInspectPhysicComponentLabelText(const wchar_t* wszText);
	void ShowInspectPhysicComponentLabel(bool bVisible);

protected:
	vgui::Button* m_pClose{};
	vgui::Panel* m_pTopBar{};
	vgui::Panel* m_pBottomBarBlank{};
	vgui::Label* m_pInspectEntityLabel{};
	vgui::Label* m_pInspectPhysicComponentLabel{};

	int m_iCachedInspectEntityIndex{};
	int m_iCachedInspectModelIndex{};

	int m_iCachedInspectPhysicComponentId{};
};