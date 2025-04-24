#include "TaskListPanel.h"

CTaskListPanel::CTaskListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{
	AddColumnHeader(0, "taskId", "#GameUI_SCModelDownloader_TaskIndex", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	AddColumnHeader(1, "name", "#GameUI_SCModelDownloader_TaskName", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(2, "identifier", "#GameUI_SCModelDownloader_TaskIdentifier", vgui::scheme()->GetProportionalScaledValue(140), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(3, "state", "#GameUI_SCModelDownloader_TaskState", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_FIXEDSIZE);
	AddColumnHeader(4, "url", "#GameUI_SCModelDownloader_TaskURL", vgui::scheme()->GetProportionalScaledValue(160), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	SetSortColumn(0);
	SetMultiselectEnabled(false);
}
