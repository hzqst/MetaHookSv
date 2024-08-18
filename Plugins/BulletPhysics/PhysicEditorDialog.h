#pragma once

#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyDialog.h>

class CRigidBodyListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CRigidBodyListPanel, vgui::ListPanel);

public:
	CRigidBodyListPanel(vgui::Panel *parent, const char* pName);
};

class CPhysicEditorDialog : public vgui::Frame
{
public:
	CPhysicEditorDialog(vgui::Panel *parent, const char* name);
	~CPhysicEditorDialog();

private:
	typedef vgui::Frame BaseClass;

	vgui::PropertySheet* m_pTabPanel{};
	//CRigidBodyListPanel* m_pRigidBodyList{};
};