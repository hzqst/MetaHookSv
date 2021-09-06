#include <metahook.h>
#include <cvardef.h>
#include <IGameUI.h>
#include <vgui/VGUI.h>
#include <vgui/IPanel.h>

#include "vgui_controls/Button.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/QueryBox.h"
#include "vgui_controls/MessageMap.h"
#include "CvarSlider.h"
#include "CvarToggleCheckButton.h"

#include "plugins.h"

void Sys_ErrorEx(const char *fmt, ...);

namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

IGameUI *g_pGameUI = NULL;

void (__fastcall *g_pfnCGameUI_Initialize)(void *pthis, int edx, CreateInterfaceFn *factories, int count) = 0;
void (__fastcall *g_pfnCGameUI_Start)(void *pthis, int edx, struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, void *system) = 0;
void (__fastcall *g_pfnCGameUI_Shutdown)(void *pthis, int edx) = 0;
int (__fastcall *g_pfnCGameUI_ActivateGameUI)(void *pthis, int edx) = 0;
int (__fastcall *g_pfnCGameUI_ActivateDemoUI)(void *pthis, int edx) = 0;
int (__fastcall *g_pfnCGameUI_HasExclusiveInput)(void *pthis, int edx) = 0;
void (__fastcall *g_pfnCGameUI_RunFrame)(void *pthis, int edx) = 0;
void (__fastcall *g_pfnCGameUI_ConnectToServer)(void *pthis, int edx, const char *game, int IP, int port) = 0;
void (__fastcall *g_pfnCGameUI_DisconnectFromServer)(void *pthis, int edx) = 0;
void (__fastcall *g_pfnCGameUI_HideGameUI)(void *pthis, int edx) = 0;
bool (__fastcall *g_pfnCGameUI_IsGameUIActive)(void *pthis, int edx) = 0;
void (__fastcall *g_pfnCGameUI_LoadingStarted)(void *pthis, int edx, const char *resourceType, const char *resourceName) = 0;
void (__fastcall *g_pfnCGameUI_LoadingFinished)(void *pthis, int edx, const char *resourceType, const char *resourceName) = 0;
void (__fastcall *g_pfnCGameUI_StartProgressBar)(void *pthis, int edx, const char *progressType, int progressSteps) = 0;
int (__fastcall *g_pfnCGameUI_ContinueProgressBar)(void *pthis, int edx, int progressPoint, float progressFraction) = 0;
void (__fastcall *g_pfnCGameUI_StopProgressBar)(void *pthis, int edx, bool bError, const char *failureReason, const char *extendedReason) = 0;
int (__fastcall *g_pfnCGameUI_SetProgressBarStatusText)(void *pthis, int edx, const char *statusText) = 0;
void (__fastcall *g_pfnCGameUI_SetSecondaryProgressBar)(void *pthis, int edx, float progress) = 0;
void (__fastcall *g_pfnCGameUI_SetSecondaryProgressBarText)(void *pthis, int edx, const char *statusText) = 0;


void *(__fastcall*g_pfnCOptionsDialog_ctor)(void *pthis, int a2, void *parent) = NULL;
void *(__fastcall*g_pfnCOptionsSubVideo_ctor)(void *pthis, int a2, void *parent) = NULL;
void (__fastcall *g_pfnCOptionsSubVideo_ApplyVidSettings)(void *pthis, int, bool bForceRestart) = NULL;

class CGameUI : public IGameUI
{
public:
	void Initialize(CreateInterfaceFn *factories, int count);
	void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, void *system);
	void Shutdown(void);
	int ActivateGameUI(void);
	int ActivateDemoUI(void);
	int HasExclusiveInput(void);
	void RunFrame(void);
	void ConnectToServer(const char *game, int IP, int port);
	void DisconnectFromServer(void);
	void HideGameUI(void);
	bool IsGameUIActive(void);
	void LoadingStarted(const char *resourceType, const char *resourceName);
	void LoadingFinished(const char *resourceType, const char *resourceName);
	void StartProgressBar(const char *progressType, int progressSteps);
	int ContinueProgressBar(int progressPoint, float progressFraction);
	void StopProgressBar(bool bError, const char *failureReason, const char *extendedReason = NULL);
	int SetProgressBarStatusText(const char *statusText);
	void SetSecondaryProgressBar(float progress);
	void SetSecondaryProgressBarText(const char *statusText);
};

CGameUI s_GameUI;


void CGameUI::Initialize(CreateInterfaceFn *factories, int count)
{
	g_pfnCGameUI_Initialize(this, 0, factories, count);
}

void CGameUI::Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, void *system)
{
	if (!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/gameui_%language%.txt"))
	{
		if (!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/gameui_english.txt"))
		{
			Sys_ErrorEx("Failed to load captionmod/gameui_english.txt");
		}
	}

	return g_pfnCGameUI_Start(this, 0, engineFuncs, interfaceVersion, system);
}

void CGameUI::Shutdown(void)
{
	return g_pfnCGameUI_Shutdown(this, 0);
}

int CGameUI::ActivateGameUI(void)
{
	return g_pfnCGameUI_ActivateGameUI(this, 0);
}

int CGameUI::ActivateDemoUI(void)
{
	return g_pfnCGameUI_ActivateDemoUI(this, 0);
}

int CGameUI::HasExclusiveInput(void)
{
	return g_pfnCGameUI_HasExclusiveInput(this, 0);
}

void CGameUI::RunFrame(void)
{
	return g_pfnCGameUI_RunFrame(this, 0);
}

void CGameUI::ConnectToServer(const char *game, int IP, int port)
{
	return g_pfnCGameUI_ConnectToServer(this, 0, game, IP, port);
}

void CGameUI::DisconnectFromServer(void)
{
	return g_pfnCGameUI_DisconnectFromServer(this, 0);
}

void CGameUI::HideGameUI(void)
{
	return g_pfnCGameUI_HideGameUI(this, 0);
}

bool CGameUI::IsGameUIActive(void)
{
	return g_pfnCGameUI_IsGameUIActive(this, 0);
}

void CGameUI::LoadingStarted(const char *resourceType, const char *resourceName)
{
	return g_pfnCGameUI_LoadingStarted(this, 0, resourceType, resourceName);
}

void CGameUI::LoadingFinished(const char *resourceType, const char *resourceName)
{
	return g_pfnCGameUI_LoadingFinished(this, 0, resourceType, resourceName);
}

void CGameUI::StartProgressBar(const char *progressType, int progressSteps)
{
	return g_pfnCGameUI_StartProgressBar(this, 0, progressType, progressSteps);
}

int CGameUI::ContinueProgressBar(int progressPoint, float progressFraction)
{
	return g_pfnCGameUI_ContinueProgressBar(this, 0, progressPoint, progressFraction);
}

void CGameUI::StopProgressBar(bool bError, const char *failureReason, const char *extendedReason)
{
	return g_pfnCGameUI_StopProgressBar(this, 0, bError, failureReason, extendedReason);
}

int CGameUI::SetProgressBarStatusText(const char *statusText)
{
	return g_pfnCGameUI_SetProgressBarStatusText(this, 0, statusText);
}

void CGameUI::SetSecondaryProgressBar(float progress)
{
	return g_pfnCGameUI_SetSecondaryProgressBar(this, 0, progress);
}

void CGameUI::SetSecondaryProgressBarText(const char *statusText)
{
	return g_pfnCGameUI_SetSecondaryProgressBarText(this, 0, statusText);
}

class COptionsSubVideoAdvancedDlg : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(COptionsSubVideoAdvancedDlg, vgui::Frame);

public:
	COptionsSubVideoAdvancedDlg(vgui::Panel *parent) : BaseClass(parent, "OptionsSubVideoAdvancedDlg")
	{
		SetTitle("#GameUI_VideoAdvanced_Title", true);
		SetSize(600, 400);
		SetSizeable(false);
		SetDeleteSelfOnClose(true);

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

	virtual void Activate(void)
	{
		OnResetData();
		BaseClass::Activate();

		vgui::input()->SetAppModalSurface(GetVPanel());
	}

	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel)
	{
	}

	MESSAGE_FUNC(OnGameUIHidden, "GameUIHidden")
	{
		Close();
	}

	MESSAGE_FUNC(OK_Confirmed, "OK_Confirmed")
	{
		Close();
	}

	void ApplyChangesToConVar(const char *pConVarName, int value)
	{
		char szCmd[1024];
		Q_snprintf(szCmd, sizeof(szCmd), "%s %d\n", pConVarName, value);
		gEngfuncs.pfnClientCmd(szCmd);
	}

	virtual void ApplyChanges(void)
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

	virtual void OnResetData(void)
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

	virtual void OnCommand(const char *command)
	{
		if (!stricmp(command, "OK"))
		{
			ApplyChanges();
			Close();
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	bool RequiresRestart(void)
	{
		return false;
	}

private:
	bool m_bUseChanges;
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

class CAdvancedButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE(CAdvancedButton, vgui::Button);

public:
	CAdvancedButton(vgui::Panel *parent, const char *panelName, const char *text) : BaseClass(parent, panelName, text)
	{
	
	}
	~CAdvancedButton(void)
	{

	}

private:
	MESSAGE_FUNC(OnOpenAdvanced, "OpenAdvanced");

private:

	vgui::DHANDLE<class COptionsSubVideoAdvancedDlg> m_hOptionsSubVideoAdvancedDlg;
};


void CAdvancedButton::OnOpenAdvanced(void)
{
	if (!m_hOptionsSubVideoAdvancedDlg.Get())
		m_hOptionsSubVideoAdvancedDlg = new COptionsSubVideoAdvancedDlg(this);

	m_hOptionsSubVideoAdvancedDlg->Activate();
}

void * __fastcall COptionsSubVideo_ctor(vgui::Panel *pthis, int a2, vgui::Panel *parent)
{
	auto result = g_pfnCOptionsSubVideo_ctor(pthis, a2, parent);

	auto m_pVideoAdvanced = new CAdvancedButton(pthis, "Advanced", "#GameUI_AdvancedEllipsis");
	m_pVideoAdvanced->SetPos(248, 160);
	m_pVideoAdvanced->SetSize(120, 24);

	m_pVideoAdvanced->AddActionSignalTarget(m_pVideoAdvanced->GetVPanel());
	m_pVideoAdvanced->SetCommand("OpenAdvanced");
	
	return result;
}

void __fastcall COptionsSubVideo_ApplyVidSettings(vgui::Panel *pthis, int a2, bool bForceRestart)
{
	g_pfnCOptionsSubVideo_ApplyVidSettings(pthis, a2, false);
}

void GameUI_InstallHook(void)
{
	auto hGameUI = GetModuleHandleA("GameUI.dll");
	CreateInterfaceFn GameUICreateInterface = Sys_GetFactory((HINTERFACEMODULE)hGameUI);
	g_pGameUI = (IGameUI *)GameUICreateInterface(GAMEUI_INTERFACE_VERSION, 0);

	DWORD *pVFTable = *(DWORD **)&s_GameUI;

	//g_pMetaHookAPI->VFTHook(g_pGameUI, 0,  1, (void *)pVFTable[1], (void **)&g_pfnCGameUI_Initialize);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0,  2, (void *)pVFTable[2], (void **)&g_pfnCGameUI_Start);

	if (1)
	{
		const char sigs1[] = "#GameUI_Options";
		auto GameUI_Options_String = g_pMetaHookAPI->SearchPattern(hGameUI, g_pMetaHookAPI->GetModuleSize(hGameUI), sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(GameUI_Options_String);
		char pattern[] = "\x6A\x01\x68\x2A\x2A\x2A\x2A\x8B\xCE";
		*(DWORD *)(pattern + 3) = (DWORD)GameUI_Options_String;
		auto GameUI_Options_Call = g_pMetaHookAPI->SearchPattern(hGameUI, g_pMetaHookAPI->GetModuleSize(hGameUI), pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(GameUI_Options_Call);

		g_pfnCOptionsDialog_ctor = (decltype(g_pfnCOptionsDialog_ctor))g_pMetaHookAPI->ReverseSearchFunctionBegin(GameUI_Options_Call, 0x300);
		Sig_VarNotFound(g_pfnCOptionsDialog_ctor);
	}

	if (1)
	{
		const char sigs1[] = "#GameUI_Video";
		auto GameUI_Video_String = g_pMetaHookAPI->SearchPattern(hGameUI, g_pMetaHookAPI->GetModuleSize(hGameUI), sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(GameUI_Video_String);
		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x2A\x2A\x33\xC0\x68\x2A\x2A\x2A\x2A";
		*(DWORD *)(pattern + 10) = (DWORD)GameUI_Video_String;
		auto GameUI_Video_Call = g_pMetaHookAPI->SearchPattern(g_pfnCOptionsDialog_ctor, 0x300, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(GameUI_Video_Call);

		g_pfnCOptionsSubVideo_ctor = (decltype(g_pfnCOptionsSubVideo_ctor))GetCallAddress(GameUI_Video_Call);
		Sig_VarNotFound(g_pfnCOptionsSubVideo_ctor);
	}

	if (1)
	{
		const char sigs1[] = "_setvideomode";
		auto SetVideoMode_String = g_pMetaHookAPI->SearchPattern(hGameUI, g_pMetaHookAPI->GetModuleSize(hGameUI), sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(SetVideoMode_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x50\xE8";
		*(DWORD *)(pattern + 1) = (DWORD)SetVideoMode_String;
		auto SetVideoMode_StringPush = g_pMetaHookAPI->SearchPattern(hGameUI, g_pMetaHookAPI->GetModuleSize(hGameUI), pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(SetVideoMode_StringPush);

		g_pfnCOptionsSubVideo_ApplyVidSettings = (decltype(g_pfnCOptionsSubVideo_ApplyVidSettings))g_pMetaHookAPI->ReverseSearchFunctionBegin(SetVideoMode_StringPush, 0x300);
		Sig_VarNotFound(g_pfnCOptionsSubVideo_ApplyVidSettings);
	}

	g_pMetaHookAPI->InlineHook(g_pfnCOptionsSubVideo_ctor, COptionsSubVideo_ctor, (void **)&g_pfnCOptionsSubVideo_ctor);
	g_pMetaHookAPI->InlineHook(g_pfnCOptionsSubVideo_ApplyVidSettings, COptionsSubVideo_ApplyVidSettings, (void **)&g_pfnCOptionsSubVideo_ApplyVidSettings);
}
