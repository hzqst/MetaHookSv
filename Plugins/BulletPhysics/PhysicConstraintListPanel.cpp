#include "PhysicConstraintListPanel.h"

CPhysicConstraintListPanel::CPhysicConstraintListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{
	AddColumnHeader(0, "index", "#BulletPhysics_Index", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	AddColumnHeader(1, "configId", "#BulletPhysics_ConfigId", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	AddColumnHeader(2, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(180), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(3, "type", "#BulletPhysics_Type", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(4, "rigidbodyA", "#BulletPhysics_RigidBodyA", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(5, "rigidbodyB", "#BulletPhysics_RigidBodyB", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(6, "flags", "#BulletPhysics_Flags", vgui::scheme()->GetProportionalScaledValue(180), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	SetSortColumn(0);
	SetMultiselectEnabled(false);
}
