#include "PhysicEditorDialog.h"

CRigidBodyListPanel::CRigidBodyListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{

}

CPhysicEditorDialog::CPhysicEditorDialog(vgui::Panel* parent, const char *name) : vgui::Frame(parent, name)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_PhysicEditor", false);

	//m_pRigidBodyList = new CRigidBodyListPanel(this, "RigidBodyList");

	//m_pRigidBodyList->AddColumnHeader(0, "Name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(16), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	//m_pRigidBodyList->AddColumnHeader(1, "Mass", "#BulletPhysics_Mass", vgui::scheme()->GetProportionalScaledValue(16), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);

	//vgui::ivgui()->AddTickSignal(GetVPanel());
	m_pTabPanel = new vgui::PropertySheet(this, "PhysicTabs");
	m_pTabPanel->SetTabWidth(72);

	LoadControlSettings("bulletphysics/PhysicEditorDialog.res", "GAME");
	InvalidateLayout();
}

CPhysicEditorDialog::~CPhysicEditorDialog()
{

}