#pragma once

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Menu.h>

class CTaskListPage;
class CSCModelDownloaderSettingsPage;

class CSCModelDownloaderDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CSCModelDownloaderDialog, vgui::Frame);

	CSCModelDownloaderDialog(vgui::Panel *parent, const char* name);
	~CSCModelDownloaderDialog();

private:

	void OnCommand(const char* command) override;

	typedef vgui::Frame BaseClass;

	CTaskListPage* m_pTaskListPage{};
	CSCModelDownloaderSettingsPage* m_pSCModelDownloaderSettingsPage{};
	vgui::PropertySheet* m_pTabPanel{};
};