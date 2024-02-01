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

#include "Viewport.h"

class COptionsSubAudioAdvancedDlg : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(COptionsSubAudioAdvancedDlg, vgui::PropertyPage);

public:
	COptionsSubAudioAdvancedDlg(vgui::Panel *parent) : BaseClass(parent, "OptionsSubAudioAdvancedDlg")
	{
		m_pPrefixButton = new CCvarToggleCheckButton(this, "PrefixButton", "#GameUI_CaptionMod_Prefix", "cap_subtitle_prefix");
		m_pWaitPlayButton = new CCvarToggleCheckButton(this, "WaitPlayButton", "#GameUI_CaptionMod_WaitPlay", "cap_subtitle_waitplay");
		m_pAntiSpamButton = new CCvarToggleCheckButton(this, "AntiSpamButton", "#GameUI_CaptionMod_AntiSpam", "cap_subtitle_antispam");

		m_pFadeInEntry = new CCvarTextEntry(this, "FadeInEntry", "cap_subtitle_fadein");
		m_pFadeOutEntry = new CCvarTextEntry(this, "FadeOutEntry", "cap_subtitle_fadeout");
		m_pHoldTimeEntry = new CCvarTextEntry(this, "HoldTimeEntry", "cap_subtitle_holdtime");
		m_pHoldTimeScaleEntry = new CCvarTextEntry(this, "HoldTimeScaleEntry", "cap_subtitle_htimescale");
		m_pStartTimeScaleEntry = new CCvarTextEntry(this, "StartTimeScaleEntry", "cap_subtitle_stimescale");

		LoadControlSettings("captionmod/OptionsSubAudioAdvancedDlg.res");
	}

	void ApplyChangesToConVar(const char *pConVarName, int value)
	{
		char szCmd[256] = {0};
		Q_snprintf(szCmd, sizeof(256) - 1, "%s %d\n", pConVarName, value);
		gEngfuncs.pfnClientCmd(szCmd);
	}

	void ApplyChanges(void)
	{
		m_pPrefixButton->ApplyChanges();
		m_pWaitPlayButton->ApplyChanges();
		m_pAntiSpamButton->ApplyChanges();

		m_pFadeInEntry->ApplyChanges();
		m_pFadeOutEntry->ApplyChanges();
		m_pHoldTimeEntry->ApplyChanges();
		m_pHoldTimeScaleEntry->ApplyChanges();
		m_pStartTimeScaleEntry->ApplyChanges();
	}

	void OnApplyChanges() override
	{
		ApplyChanges();
	}

	void OnResetData(void) override
	{
		m_pPrefixButton->Reset();
		m_pWaitPlayButton->Reset();
		m_pAntiSpamButton->Reset();

		m_pFadeInEntry->Reset();
		m_pFadeOutEntry->Reset();
		m_pHoldTimeEntry->Reset();
		m_pHoldTimeScaleEntry->Reset();
		m_pStartTimeScaleEntry->Reset();
	}

	void OnCommand(const char *command) override
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

	MESSAGE_FUNC(OnDataChanged, "ControlModified");

private:

	CCvarToggleCheckButton*m_pPrefixButton;
	CCvarToggleCheckButton*m_pWaitPlayButton;
	CCvarToggleCheckButton*m_pAntiSpamButton;

	CCvarTextEntry *m_pFadeInEntry;
	CCvarTextEntry *m_pFadeOutEntry;
	CCvarTextEntry *m_pHoldTimeEntry;
	CCvarTextEntry *m_pHoldTimeScaleEntry;
	CCvarTextEntry *m_pStartTimeScaleEntry;
};

void COptionsSubAudioAdvancedDlg::OnDataChanged()
{
	GetParentWithModuleName("GameUI")->PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

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
		return "CaptionMod";
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		if (!vgui::VGui_InitInterfacesList("CaptionMod", factories, count))
		{
			Sys_Error("Failed to VGui_InitInterfacesList");
			return;
		}
	}

	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		if (g_pFileSystem)
		{
			if (!vgui::localize()->AddFile(g_pFileSystem, "captionmod/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile(g_pFileSystem, "captionmod/gameui_english.txt"))
				{
					Sys_Error("Failed to load captionmod/gameui_english.txt");
				}
			}
		}
		else if (g_pFileSystem_HL25)
		{
			if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "captionmod/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "captionmod/gameui_english.txt"))
				{
					Sys_Error("Failed to load captionmod/gameui_english.txt");
				}
			}
		}
	}

	void Shutdown(void) override
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
		if (!CallbackContext->IsPost)
		{
			g_pViewPort->ConnectToServer(game, IP, port);

			if (gEngfuncs.GetMaxClients() <= 1)
			{
				//This stop GameUI from sending "mp3 stop" on level transition
				game = "valve";
			}
		}
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
		CallbackContext->AddPage(new COptionsSubAudioAdvancedDlg((vgui::Panel*)CallbackContext->GetDialog()), "#GameUI_CaptionMod_Tab");
	}

	void COptionsSubVideo_ApplyVidSettings(void*& pPanel, bool& bForceRestart, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}
};

static CVGUI2Extension_GameUIOptionDialogCallbacks s_GameUIOptionDialogCallbacks;

/*
=================================================================================================================
GameUI KeyValues Callbacks
=================================================================================================================
*/

class CVGUI2Extension_GameUIKeyValuesCallbacks : public IVGUI2Extension_GameUIKeyValuesCallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void KeyValues_LoadFromFile(void*& pthis, IFileSystem*& pFileSystem, const char*& resourceName, const char*& pathId, const char *sourceModule, VGUI2Extension_CallbackContext* CallbackContext)
	{
#if 0
		if (CallbackContext->IsPost && !stricmp(resourceName, "resource/GameMenu.res"))
		{
			bool *pRealReturnValue = (bool*)CallbackContext->pRealReturnValue;

			if ((*pRealReturnValue) == true)
			{
				KeyValues* pKeyValues = (KeyValues*)pthis;

				auto name = pKeyValues->GetName();

				KeyValues* SectionQuit = NULL;
				for (auto p = pKeyValues->GetFirstSubKey(); p; p = p->GetNextKey())
				{
					auto command = p->GetString("command");
					if (!strcmp(command, "Quit"))
					{
						SectionQuit = p;
					}
				}
				if (SectionQuit)
				{
					auto NameSectionQuit = SectionQuit->GetName();
					int iNameSectionQuit = atoi(NameSectionQuit);
					if (iNameSectionQuit > 0)
					{
						/*
						
						 //Update this:
							"8"
							{
								"label" "#GameUI_GameMenu_Quit"
								"command" "Quit"
							}

						//To this:
							"8"
							{
								"label" "#GameUI_GameMenu_TestButton"
								"command" "TestButton"
							}
							"9"
							{
								"label" "#GameUI_GameMenu_Quit"
								"command" "Quit"
							}
						*/

						char szNewNameSectionQuit[32];
						snprintf(szNewNameSectionQuit, sizeof(szNewNameSectionQuit), "%d", iNameSectionQuit + 1);

						SectionQuit->SetName(szNewNameSectionQuit);

						char szNewNameTestButton[32];
						snprintf(szNewNameTestButton, sizeof(szNewNameTestButton), "%d", iNameSectionQuit);

						auto SectionTestButton = new KeyValues(szNewNameTestButton);

						SectionTestButton->SetString("label", "#GameUI_GameMenu_TestButton");
						SectionTestButton->SetString("command", "TestCommand");

						pKeyValues->AddSubKeyBefore(SectionTestButton, SectionQuit);

					}
				}
			}
		}
#endif
	}
};

static CVGUI2Extension_GameUIKeyValuesCallbacks s_GameUIKeyValuesCallbacks;

/*
=================================================================================================================
GameUI init & shutdown
=================================================================================================================
*/

void GameUI_InstallHooks(void)
{
	VGUI2Extension()->RegisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->RegisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
	VGUI2Extension()->RegisterGameUIKeyValuesCallbacks(&s_GameUIKeyValuesCallbacks);
}

void GameUI_UninstallHooks(void)
{
	VGUI2Extension()->UnregisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->UnregisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
	VGUI2Extension()->UnregisterGameUIKeyValuesCallbacks(&s_GameUIKeyValuesCallbacks);
}