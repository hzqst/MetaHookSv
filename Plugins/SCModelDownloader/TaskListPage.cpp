#include "TaskListPage.h"
#include "TaskListPanel.h"

#include "exportfuncs.h"

CTaskListPage::CTaskListPage(vgui::Panel* parent, const char* name) :
	BaseClass(parent, name)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pTaskListPanel = new CTaskListPanel(this, "TaskListPanel");

	LoadControlSettings("scmodeldownloader/TaskListPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CTaskListPage::OnResetData()
{
	
}

void CTaskListPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pTaskListPanel->SetFont(m_hFont);
}

void CTaskListPage::OnCommand(const char* command)
{
	//nothing...

	BaseClass::OnCommand(command);
}

void CTaskListPage::OnRefreshTaskList()
{
	
}