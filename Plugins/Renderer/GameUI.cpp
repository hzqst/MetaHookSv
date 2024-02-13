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

class COptionsSubVideoAdvancedDlg : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(COptionsSubVideoAdvancedDlg, vgui::PropertyPage);

public:
	COptionsSubVideoAdvancedDlg(vgui::Panel *parent) : BaseClass(parent, "OptionsSubVideoAdvancedDlg")
	{
		m_pAnisotropicFiltering = new vgui::ComboBox(this, "AnisotropicFiltering", 5, false);
		m_pAnisotropicFiltering->AddItem("1X", NULL);
		m_pAnisotropicFiltering->AddItem("2X", NULL);
		m_pAnisotropicFiltering->AddItem("4X", NULL);
		m_pAnisotropicFiltering->AddItem("8X", NULL);
		m_pAnisotropicFiltering->AddItem("16X", NULL);

		m_pDetailTexture = new CCvarToggleCheckButton(this, "DetailTexture", "#GameUI_DetailTextures", "r_detailtextures");
		m_pWaterShader = new CCvarToggleCheckButton(this, "WaterShader", "#GameUI_WaterShader", "r_water");
		m_pDynamicShadow = new CCvarToggleCheckButton(this, "DynamicShadow", "#GameUI_DynamicShadow", "r_shadow");
		m_pAmbientOcclusion = new CCvarToggleCheckButton(this, "AmbientOcclusion", "#GameUI_AmbientOcclusion", "r_ssao");
		m_pDynamicLights = new CCvarToggleCheckButton(this, "DynamicLights", "#GameUI_DynamicLights", "r_light_dynamic");
		m_pScreenSpaceReflection = new CCvarToggleCheckButton(this, "ScreenSpaceReflection", "#GameUI_ScreenSpaceReflection", "r_ssr");
		m_pCelShade = new CCvarToggleCheckButton(this, "CelShade", "#GameUI_CelShade", "r_studio_celshade");
		m_pAntiAliasing = new CCvarToggleCheckButton(this, "AntiAliasing", "#GameUI_AntiAliasing", "r_fxaa");
		m_pSkyOcclusion = new CCvarToggleCheckButton(this, "SkyOcclusion", "#GameUI_SkyOcclusion", "r_wsurf_sky_occlusion");
		m_pZPrepass = new CCvarToggleCheckButton(this, "ZPrepass", "#GameUI_ZPrepass", "r_wsurf_zprepass");
		m_pHDR = new CCvarToggleCheckButton(this, "HDR", "#GameUI_HDR", "r_hdr");

		m_pHDRExposure = new CCvarSlider(this, "HDRExposure", "#GameUI_HDRExposure", 0.1f, 2.0f, "r_hdr_exposure", false);
		m_pHDRDarkness = new CCvarSlider(this, "HDRDarkness", "#GameUI_HDRDarkness", 0.1f, 2.0f, "r_hdr_darkness", false);
		m_pBloomIntensity = new CCvarSlider(this, "BloomIntensity", "#GameUI_BloomIntensity", 0.0f, 1.0f, "r_hdr_blurwidth", false);
		m_pShadowIntensity = new CCvarSlider(this, "ShadowIntensity", "#GameUI_ShadowIntensity", 0.0f, 1.0f, "r_shadow_intensity", false);

		m_pTexGamma = new CCvarSlider(this, "TexGamma", "#GameUI_TexGamma", 1.8f, 3.0f, "texgamma", false);
		m_pLightGamma = new CCvarSlider(this, "LightGamma", "#GameUI_LightGamma", 1.8f, 3.0f, "lightgamma", false);

		LoadControlSettings("captionmod/OptionsSubVideoAdvancedDlg.res");
	}

	void ApplyChangesToConVar(const char *pConVarName, int value)
	{
		char szCmd[256] = {0};
		Q_snprintf(szCmd, sizeof(szCmd) - 1, "%s %d\n", pConVarName, value);
		gEngfuncs.pfnClientCmd(szCmd);
	}

	void ApplyChanges(void)
	{
		int activateItem = m_pAnisotropicFiltering->GetActiveItem();

		switch (activateItem)
		{
		case 0: ApplyChangesToConVar("gl_ansio", 1); break;
		case 1: ApplyChangesToConVar("gl_ansio", 2); break;
		case 2: ApplyChangesToConVar("gl_ansio", 4); break;
		case 3: ApplyChangesToConVar("gl_ansio", 8); break;
		case 4: ApplyChangesToConVar("gl_ansio", 16); break;
		}

		ApplyChangesToConVar(m_pDetailTexture->GetCvarName(), m_pDetailTexture->IsSelected());
		ApplyChangesToConVar(m_pWaterShader->GetCvarName(), m_pWaterShader->IsSelected());
		ApplyChangesToConVar(m_pDynamicShadow->GetCvarName(), m_pDynamicShadow->IsSelected());
		ApplyChangesToConVar(m_pAmbientOcclusion->GetCvarName(), m_pAmbientOcclusion->IsSelected());
		ApplyChangesToConVar(m_pScreenSpaceReflection->GetCvarName(), m_pScreenSpaceReflection->IsSelected());
		ApplyChangesToConVar(m_pCelShade->GetCvarName(), m_pCelShade->IsSelected());
		ApplyChangesToConVar(m_pAntiAliasing->GetCvarName(), m_pAntiAliasing->IsSelected());
		ApplyChangesToConVar(m_pSkyOcclusion->GetCvarName(), m_pSkyOcclusion->IsSelected());
		ApplyChangesToConVar(m_pZPrepass->GetCvarName(), m_pZPrepass->IsSelected());
		ApplyChangesToConVar(m_pHDR->GetCvarName(), m_pHDR->IsSelected());

		m_pHDRExposure->ApplyChanges();
		m_pHDRDarkness->ApplyChanges();
		m_pBloomIntensity->ApplyChanges();
		m_pShadowIntensity->ApplyChanges();
		m_pTexGamma->ApplyChanges();
		m_pLightGamma->ApplyChanges();
	}

	void OnApplyChanges(void) override
	{
		ApplyChanges();
	}

	void OnResetData(void) override
	{
		m_pDetailTexture->Reset();
		m_pWaterShader->Reset();
		m_pDynamicShadow->Reset();
		m_pDynamicLights->Reset();
		m_pAmbientOcclusion->Reset();
		m_pScreenSpaceReflection->Reset();
		m_pCelShade->Reset();
		m_pAntiAliasing->Reset();
		m_pSkyOcclusion->Reset();
		m_pZPrepass->Reset();
		m_pHDR->Reset();

		m_pHDRExposure->Reset();
		m_pHDRDarkness->Reset();
		m_pBloomIntensity->Reset();
		m_pShadowIntensity->Reset();
		m_pTexGamma->Reset();
		m_pLightGamma->Reset();

		auto gl_ansio = gEngfuncs.pfnGetCvarPointer("gl_ansio");

		if (gl_ansio)
		{
			int ansio = gl_ansio->value;

			if (ansio >= 16)
				m_pAnisotropicFiltering->ActivateItem(4);
			else if (ansio >= 8)
				m_pAnisotropicFiltering->ActivateItem(3);
			else if (ansio >= 4)
				m_pAnisotropicFiltering->ActivateItem(2);
			else if (ansio >= 2)
				m_pAnisotropicFiltering->ActivateItem(1);
			else
				m_pAnisotropicFiltering->ActivateItem(0);
		}
		else
		{
			m_pAnisotropicFiltering->SetEnabled(false);
		}
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
	vgui::ComboBox *m_pAnisotropicFiltering;
	CCvarToggleCheckButton *m_pDetailTexture;
	CCvarToggleCheckButton *m_pWaterShader;
	CCvarToggleCheckButton *m_pDynamicShadow;
	CCvarToggleCheckButton *m_pAmbientOcclusion;
	CCvarToggleCheckButton *m_pDynamicLights;
	CCvarToggleCheckButton *m_pScreenSpaceReflection;
	CCvarToggleCheckButton *m_pCelShade;
	CCvarToggleCheckButton *m_pAntiAliasing;
	CCvarToggleCheckButton *m_pSkyOcclusion;
	CCvarToggleCheckButton *m_pZPrepass;
	CCvarToggleCheckButton *m_pHDR;

	CCvarSlider *m_pHDRExposure;
	CCvarSlider *m_pHDRDarkness;
	CCvarSlider *m_pBloomIntensity;
	CCvarSlider *m_pShadowIntensity;
	CCvarSlider *m_pTexGamma;
	CCvarSlider *m_pLightGamma;
};

void COptionsSubVideoAdvancedDlg::OnDataChanged()
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
		return "Renderer";
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		if (!vgui::VGui_InitInterfacesList("Renderer", factories, count))
		{
			Sys_Error("Failed to VGui_InitInterfacesList");
			return;
		}
	}

	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		if (g_pFileSystem)
		{
			if (!vgui::localize()->AddFile(g_pFileSystem, "renderer/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile(g_pFileSystem, "renderer/gameui_english.txt"))
				{
					Sys_Error("Failed to load renderer/gameui_english.txt");
				}
			}
		}
		else if (g_pFileSystem_HL25)
		{
			if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "renderer/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "renderer/gameui_english.txt"))
				{
					Sys_Error("Failed to load renderer/gameui_english.txt");
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
		CallbackContext->AddPage(new COptionsSubVideoAdvancedDlg((vgui::Panel*)CallbackContext->GetDialog()), "#GameUI_Renderer_Tab");
	}

	void COptionsSubVideo_ApplyVidSettings(void*& pPanel, bool& bForceRestart, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}
};

static CVGUI2Extension_GameUIOptionDialogCallbacks s_GameUIOptionDialogCallbacks;

/*
=================================================================================================================
KeyValues Callbacks
=================================================================================================================
*/

class CVGUI2Extension_KeyValuesCallbacks : public IVGUI2Extension_KeyValuesCallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void KeyValues_LoadFromFile(void*& pthis, IFileSystem*& pFileSystem, const char*& resourceName, const char*& pathId, const char* sourceModule, VGUI2Extension_CallbackContext* CallbackContext)
	{
		
	}
};

static CVGUI2Extension_KeyValuesCallbacks s_KeyValuesCallbacks;

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
	VGUI2Extension()->RegisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
	//VGUI2Extension()->RegisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
}

void GameUI_UninstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->UnregisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->UnregisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
	//VGUI2Extension()->UnregisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
}