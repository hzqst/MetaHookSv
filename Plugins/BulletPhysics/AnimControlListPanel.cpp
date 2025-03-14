#include "AnimControlListPanel.h"

CAnimControlListPanel::CAnimControlListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{
	AddColumnHeader(0, "index", "#BulletPhysics_Index", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	AddColumnHeader(1, "configId", "#BulletPhysics_ConfigId", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	AddColumnHeader(2, "sequence", "#BulletPhysics_Sequence", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(3, "gaitsequence", "#BulletPhysics_GaitSequence", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(4, "animframe", "#BulletPhysics_AnimFrame", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(5, "activityType", "#BulletPhysics_ActivityType", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(6, "flags", "#BulletPhysics_Flags", vgui::scheme()->GetProportionalScaledValue(160), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	SetSortColumn(0);
	SetMultiselectEnabled(false);
}
