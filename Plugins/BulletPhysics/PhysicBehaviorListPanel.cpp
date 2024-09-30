#include "PhysicBehaviorListPanel.h"

CPhysicBehaviorListPanel::CPhysicBehaviorListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{
	AddColumnHeader(0, "index", "#BulletPhysics_Index", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	AddColumnHeader(1, "configId", "#BulletPhysics_ConfigId", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	AddColumnHeader(2, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(200), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(3, "type", "#BulletPhysics_Type", vgui::scheme()->GetProportionalScaledValue(100), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(4, "rigidbody", "#BulletPhysics_RigidBody", vgui::scheme()->GetProportionalScaledValue(100), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(5, "constraint", "#BulletPhysics_Constraint", vgui::scheme()->GetProportionalScaledValue(160), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(6, "flags", "#BulletPhysics_Flags", vgui::scheme()->GetProportionalScaledValue(180), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	SetSortColumn(0);
	SetMultiselectEnabled(false);
}
