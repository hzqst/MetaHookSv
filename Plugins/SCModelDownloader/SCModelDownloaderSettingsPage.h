#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>

#include <vgui_controls/CvarSlider.h>
#include <vgui_controls/CvarToggleCheckButton.h>
#include <vgui_controls/CvarTextEntry.h>

class CSCModelDownloaderSettingsPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CSCModelDownloaderSettingsPage, vgui::PropertyPage);

	CSCModelDownloaderSettingsPage(vgui::Panel* parent, const char* name);

private:

	void OnCommand(const char* command) override;
	void ApplyChanges(void);
	void ApplyChangesToConVar(const char* pConVarName, int value);
	void ForceUpdateDatabases();


	MESSAGE_FUNC(OnApplyChanges, "ApplyChanges");
	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC(OnDataChanged, "ControlModified");

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	vgui::ComboBox* m_pCDN{};

#define DEFINE_CVAR_CHECK_BUTTON(name) CCvarToggleCheckButton* m_p##name{}
	DEFINE_CVAR_CHECK_BUTTON(AutoDownload);
	DEFINE_CVAR_CHECK_BUTTON(DownloadLatest);
#undef DEFINE_CVAR_CHECK_BUTTON

	CCvarTextEntry* m_pMaxRetry{};
};
