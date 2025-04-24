#include "SCModelDownloaderSettingsPage.h"

#include "exportfuncs.h"

#include <cvardef.h>

CSCModelDownloaderSettingsPage::CSCModelDownloaderSettingsPage(vgui::Panel* parent, const char* name) :
	BaseClass(parent, name)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

#define CREATE_CVAR_CHECK_BUTTON(name, cvar) m_p##name = new CCvarToggleCheckButton(this, #name, "#GameUI_SCModelDownloader_" #name, cvar)
	CREATE_CVAR_CHECK_BUTTON(AutoDownload, "scmodel_autodownload");
	CREATE_CVAR_CHECK_BUTTON(DownloadLatest, "scmodel_downloadlatest");
#undef CREATE_CVAR_CHECK_BUTTON

	m_pCDN = new vgui::ComboBox(this, "CDN", 2, false);
	m_pCDN->AddItem("none", NULL);
	m_pCDN->AddItem("jsdelivr", NULL);

	LoadControlSettings("scmodeldownloader/SCModelDownloaderSettingsPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CSCModelDownloaderSettingsPage::ApplyChangesToConVar(const char* pConVarName, int value)
{
	char szCmd[256] = { 0 };
	Q_snprintf(szCmd, sizeof(szCmd) - 1, "%s %d\n", pConVarName, value);
	gEngfuncs.pfnClientCmd(szCmd);
}

void CSCModelDownloaderSettingsPage::ApplyChanges(void)
{
	int activateItem = m_pCDN->GetActiveItem();

	switch (activateItem)
	{
	case 0: ApplyChangesToConVar("scmodel_cdn", 0); break;
	case 1: ApplyChangesToConVar("scmodel_cdn", 1); break;
	}

	ApplyChangesToConVar(m_pAutoDownload->GetCvarName(), m_pAutoDownload->IsSelected());
	ApplyChangesToConVar(m_pDownloadLatest->GetCvarName(), m_pDownloadLatest->IsSelected());
}

void CSCModelDownloaderSettingsPage::OnApplyChanges(void)
{
	ApplyChanges();
}

void CSCModelDownloaderSettingsPage::OnResetData(void)
{
	m_pAutoDownload->Reset();
	m_pDownloadLatest->Reset();

	auto scmodel_cdn = gEngfuncs.pfnGetCvarPointer("scmodel_cdn");

	if (scmodel_cdn)
	{
		int cdn = (int)scmodel_cdn->value;

		if (cdn == 1)
			m_pCDN->ActivateItem(1);
		else
			m_pCDN->ActivateItem(0);
	}
	else
	{
		m_pCDN->SetEnabled(false);
	}
}

void CSCModelDownloaderSettingsPage::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		ApplyChanges();
	}
	else if (!stricmp(command, "Apply"))
	{
		ApplyChanges();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CSCModelDownloaderSettingsPage::OnDataChanged()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
