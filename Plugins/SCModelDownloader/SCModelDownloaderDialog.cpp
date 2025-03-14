#include "SCModelDownloaderDialog.h"
#include "TaskListPage.h"
#include "exportfuncs.h"

#include <format>

CSCModelDownloaderDialog::CSCModelDownloaderDialog(vgui::Panel* parent, const char *name) :
	BaseClass(parent, name)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#GameUI_SCModelDownloader_Title", false);

	m_pTaskListPage = new CTaskListPage(this, "TaskListPage");
	m_pTaskListPage->MakeReadyForUse();

	SetMinimumSize(640, 384);
	SetSize(640, 384);

	m_pTabPanel = new vgui::PropertySheet(this, "Tabs");
	m_pTabPanel->SetTabWidth(72);
	m_pTabPanel->AddPage(m_pTaskListPage, "#GameUI_SCModelDownloader_TaskListPage");

	m_pTabPanel->AddActionSignalTarget(this);

	LoadControlSettings("scmodeldownloader/SCModelDownloaderDialog.res", "GAME");

	m_pTabPanel->SetActivePage(m_pTaskListPage);

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CSCModelDownloaderDialog::~CSCModelDownloaderDialog()
{

}

void CSCModelDownloaderDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		m_pTabPanel->ApplyChanges();
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		m_pTabPanel->ApplyChanges();
		return;
	}

	BaseClass::OnCommand(command);
}