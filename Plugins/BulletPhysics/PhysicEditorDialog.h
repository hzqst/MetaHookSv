#pragma once

#include <vgui_controls/PropertyDialog.h>

class CPhysicEditorInspectPage;

class CPhysicEditorDialog : public vgui::PropertyDialog
{
public:
	CPhysicEditorDialog(vgui::Panel *parent);
	~CPhysicEditorDialog();

protected:
	virtual void OnOK();
	virtual void OnClose();

private:
	typedef vgui::PropertyDialog BaseClass;

	CPhysicEditorInspectPage* m_pInspectPage{};
};