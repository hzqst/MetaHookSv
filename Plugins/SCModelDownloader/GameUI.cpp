#include <metahook.h>
#include <cvardef.h>
#include <IGameUI.h>
#include <vgui/VGUI.h>
#include <vgui/IPanel.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/MessageMap.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/CvarSlider.h>
#include <vgui_controls/CvarToggleCheckButton.h>
#include <vgui_controls/CvarTextEntry.h>

#include <IVGUI2Extension.h>

#include "plugins.h"

#include "SCModelDownloaderDialog.h"

#include "SCModelDatabase.h"

static vgui::DHANDLE<CSCModelDownloaderDialog> s_hSCModelDownloaderDialog;

/*
=================================================================================================================
GameUI Callbacks
=================================================================================================================
*/

class CVGUI2Extension_GameUICallbacks : public IVGUI2Extension_GameUICallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	const char* GetControlModuleName() const
	{
		return "SCModelDownloader";
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		if (!vgui::VGui_InitInterfacesList("SCModelDownloader", factories, count))
		{
			Sys_Error("Failed to VGui_InitInterfacesList");
			return;
		}
	}

	void PreStart(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		if (g_pFileSystem)
		{
			if (!vgui::localize()->AddFile(g_pFileSystem, "scmodeldownloader/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile(g_pFileSystem, "scmodeldownloader/gameui_english.txt"))
				{
					Sys_Error("Failed to load \"scmodeldownloader/gameui_english.txt\"");
				}
			}
		}
		else if (g_pFileSystem_HL25)
		{
			if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "scmodeldownloader/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "scmodeldownloader/gameui_english.txt"))
				{
					Sys_Error("Failed to load \"scmodeldownloader/gameui_english.txt\"");
				}
			}
		}
	}

	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		
	}

	void Shutdown(void) override
	{

	}

	void PostShutdown(void) override
	{

	}

	void ActivateGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ActivateDemoUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HasExclusiveInput(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void RunFrame(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ConnectToServer(const char*& game, int& IP, int& port, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void DisconnectFromServer(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void IsGameUIActive(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void LoadingStarted(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void LoadingFinished(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void StartProgressBar(const char*& progressType, int& progressSteps, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ContinueProgressBar(int& progressPoint, float& progressFraction, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void StopProgressBar(bool& bError, const char*& failureReason, const char*& extendedReason, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetProgressBarStatusText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetSecondaryProgressBar(float& progress, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetSecondaryProgressBarText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

};

static CVGUI2Extension_GameUICallbacks s_GameUICallbacks;

/*
=================================================================================================================
GameUI OptionDialog Callbacks
=================================================================================================================
*/

class CVGUI2Extension_GameUIOptionDialogCallbacks : public IVGUI2Extension_GameUIOptionDialogCallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void COptionsDialog_ctor(IGameUIOptionsDialogCtorCallbackContext* CallbackContext) override
	{
		
	}

	void COptionsSubVideo_ApplyVidSettings(void*& pPanel, bool& bForceRestart, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void COptionsDialogSubPage_ctor(IGameUIOptionsDialogSubPageCtorCallbackContext* CallbackContext) override
	{
		
	}

	void COptionsSubPage_OnApplyChanges(void*& pPanel, const char* name, VGUI2Extension_CallbackContext* CallbackContext) override
	{
	
	}
};

static CVGUI2Extension_GameUIOptionDialogCallbacks s_GameUIOptionDialogCallbacks;

/*
=================================================================================================================
GameUI KeyValues Callbacks
=================================================================================================================
*/

class CVGUI2Extension_KeyValuesCallbacks : public IVGUI2Extension_KeyValuesCallbacks
{
public:
	int GetAltitude() const override
	{
		return 1;
	}

	void KeyValues_LoadFromFile(void*& pthis, IFileSystem*& pFileSystem, const char*& resourceName, const char*& pathId, const char* sourceModule, VGUI2Extension_CallbackContext* CallbackContext)
	{
		if (CallbackContext->IsPost && !strcmp(resourceName, "resource/GameMenu.res")) {
			bool* pRealReturnValue = (bool*)CallbackContext->pRealReturnValue;
			if ((*pRealReturnValue) == true) {
				KeyValues* pKeyValues = (KeyValues*)pthis;
				auto name = pKeyValues->GetName();
				KeyValues* SectionQuit = nullptr;
				for (auto p = pKeyValues->GetFirstSubKey(); p; p = p->GetNextKey()) {
					auto command = p->GetString("command");
					if (!stricmp(command, "OpenOptionsDialog"))
						SectionQuit = p;
				}
				if (SectionQuit) {
					auto NameSectionQuit = SectionQuit->GetName();
					int iNameSectionQuit = atoi(NameSectionQuit);
					if (iNameSectionQuit > 0) {
						char szNewNameSectionQuit[32];
						snprintf(szNewNameSectionQuit, sizeof(szNewNameSectionQuit), "%d", iNameSectionQuit + 1);
						SectionQuit->SetName(szNewNameSectionQuit);
						char szNewNameTestButton[32];
						snprintf(szNewNameTestButton, sizeof(szNewNameTestButton), "%d", iNameSectionQuit);
						auto SectionTestButton = new KeyValues(szNewNameTestButton);
						SectionTestButton->SetString("label", "#GameUI_SCModelDownloader_SectionButton");
						SectionTestButton->SetString("command", "OpenSCModelDownloaderDialog");
						pKeyValues->AddSubKeyAfter(SectionTestButton, SectionQuit);
					}
				}
			}
		}
	}
};

static CVGUI2Extension_KeyValuesCallbacks s_KeyValuesCallbacks;

class CSCModelLocalPlayerModelChangeHandler : public ISCModelLocalPlayerModelChangeHandler
{
public:
	void OnLocalPlayerChangeModel(const char* previousModelName, const char* newModelName) override
	{
		const char* newerVersionModel = SCModelDatabase()->GetNewerVersionModel(newModelName);

		if (newerVersionModel)
		{
			if(SCModelDatabase()->IsModelSkipped(newerVersionModel))
				return;

			auto title = vgui::localize()->Find("#GameUI_SCModelDownloader_SwitchtoNewerVersionTitle");

			wchar_t szNewModelName[64] = { 0 };
			wchar_t szNewerVersionModelName[64] = { 0 };
			vgui::localize()->ConvertANSIToUnicode(newModelName, szNewModelName, sizeof(szNewModelName));
			vgui::localize()->ConvertANSIToUnicode(newerVersionModel, szNewerVersionModelName, sizeof(szNewerVersionModelName));

			wchar_t szBuf[256] = { 0 };
			if (SCModelDatabase()->IsAllRequiredFilesForModelAvailableCABI(newerVersionModel, false))
			{
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#GameUI_SCModelDownloader_SwitchtoNewerVersionContentNetwork"), 2, szNewModelName, szNewerVersionModelName);
			}
			else
			{
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#GameUI_SCModelDownloader_SwitchtoNewerVersionContent"), 2, szNewModelName, szNewerVersionModelName);
			}

			auto box = new vgui::QueryBox(title, szBuf, m_pBasePanel);

			box->SetOKButtonText("#GameUI_OK");

			char szOKCommandName[256]{};
			snprintf(szOKCommandName, sizeof(szOKCommandName), "SwitchtoNewerVersionModelConfirm_%s", newerVersionModel);
			box->SetOKCommand(new KeyValues("Command", "command", szOKCommandName));

			char szCancelCommandName[256]{};
			snprintf(szCancelCommandName, sizeof(szCancelCommandName), "SkipModel_%s", newerVersionModel);
			box->SetCancelCommand(new KeyValues("Command", "command", szCancelCommandName));
			//box->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
			box->AddActionSignalTarget(m_pBasePanel);
			box->DoModal();
		}
	}

	void OnSwitchtoNewerVersionModelConfirm(const char * newerVersionModel)
	{
		char cmd[256]{};
		snprintf(cmd, sizeof(cmd), "model %s\n", newerVersionModel);
		gEngfuncs.pfnClientCmd(cmd);

		SCModelDatabase()->QueryModel(newerVersionModel);
	}

	void OnSkipModel(const char * newerVersionModel)
	{
		SCModelDatabase()->AddSkippedModel(newerVersionModel);
	}

	vgui::Panel*m_pBasePanel{};

	void SetBasePanel(vgui::Panel* pBasePanel)
	{
		m_pBasePanel = pBasePanel;
	}
};

static CSCModelLocalPlayerModelChangeHandler s_LocalPlayerModelChangeHandler;

class CVGUI2Extension_TaskBarCallbacks : public IVGUI2Extension_GameUITaskBarCallbacks
{
	int GetAltitude() const override
	{
		return 0;
	}

	void CTaskBar_ctor(IGameUITaskBarCtorCallbackContext* CallbackContext) override
	{
		s_LocalPlayerModelChangeHandler.SetBasePanel((vgui::Panel *)CallbackContext->GetTaskBar());

		SCModelDatabase()->RegisterLocalPlayerChangeModelCallback(&s_LocalPlayerModelChangeHandler);
	}

	void CTaskBar_OnCommand(void*& pPanel, const char*& command, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		if (!strcmp(command, "OpenSCModelDownloaderDialog")) {

			if (!s_hSCModelDownloaderDialog)
			{
				s_hSCModelDownloaderDialog = new CSCModelDownloaderDialog((vgui::Panel*)pPanel, "SCModelDownloaderDialog");
			}

			if (s_hSCModelDownloaderDialog)
			{
				s_hSCModelDownloaderDialog->Activate();
			}
			return;
		}

		if (!strncmp(command, "SwitchtoNewerVersionModelConfirm_", sizeof("SwitchtoNewerVersionModelConfirm_") - 1)) {
			s_LocalPlayerModelChangeHandler.OnSwitchtoNewerVersionModelConfirm(command + sizeof("SwitchtoNewerVersionModelConfirm_") - 1);
			return;
		}

		if(!strncmp(command, "SkipModel_", sizeof("SkipModel_") - 1)) {
			s_LocalPlayerModelChangeHandler.OnSkipModel(command + sizeof("SkipModel_") - 1);
			return;
		}
	}
};

static CVGUI2Extension_TaskBarCallbacks s_TaskBarCallbacks;

#if 0
class CVGUI2Extension_BasePanelCallbacks : public IVGUI2Extension_GameUIBasePanelCallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}
	void CBasePanel_ctor(IGameUIBasePanelCtorCallbackContext* CallbackContext) override
	{
		s_pBasePanel = (decltype(s_pBasePanel))CallbackContext->GetBasePanel();
	}
	void CBasePanel_ApplySchemeSettings(void*& pPanel, void*& pScheme, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}
};

static CVGUI2Extension_BasePanelCallbacks s_BasePanelCallbacks;
#endif

/*
=================================================================================================================
GameUI init & shutdown
=================================================================================================================
*/

void GameUI_InstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->RegisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->RegisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
	VGUI2Extension()->RegisterGameUITaskBarCallbacks(&s_TaskBarCallbacks);
	//VGUI2Extension()->RegisterGameUIBasePanelCallbacks(&s_BasePanelCallbacks);
	VGUI2Extension()->RegisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
}

void GameUI_UninstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->UnregisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->UnregisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
	VGUI2Extension()->UnregisterGameUITaskBarCallbacks(&s_TaskBarCallbacks);
	//VGUI2Extension()->UnregisterGameUIBasePanelCallbacks(&s_BasePanelCallbacks);
	VGUI2Extension()->UnregisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
}