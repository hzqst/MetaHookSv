#include <metahook.h>
#include <cvardef.h>
#include <IGameUI.h>
#include <vgui/VGUI.h>
#include <vgui/IPanel.h>
#include <vgui_controls/Panel.h>

#include "plugins.h"
#include "privatefuncs.h"
#include "DpiManagerInternal.h"
#include "VGUI2ExtensionInternal.h"
#include <capstone.h>
#include <set>
#include <sstream>
#include <vector>

static hook_t* g_phook_GameUI_Panel_Init = NULL;
static hook_t* g_phook_CGameConsoleDialog_ctor = NULL;
static hook_t* g_phook_CCreateMultiplayerGameDialog_ctor = NULL;
static hook_t* g_phook_COptionsDialog_ctor = NULL;
static hook_t *g_phook_COptionsSubVideo_ApplyVidSettings = NULL;
static hook_t* g_phook_RichText_InsertChar = NULL;
static hook_t* g_phook_RichText_InsertStringW = NULL;
static hook_t* g_phook_RichText_OnThink = NULL;
static hook_t* g_phook_TextEntry_OnKeyCodeTyped = NULL;
static hook_t* g_phook_TextEntry_LayoutVerticalScrollBarSlider = NULL;
static hook_t* g_phook_TextEntry_GetStartDrawIndex = NULL;
static hook_t* g_phook_PropertySheet_HasHotkey = NULL;
static hook_t* g_phook_FocusNavGroup_GetCurrentFocus = NULL;

namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

static int g_iPatchingPanelTall = 0;
static bool g_bPatchingGetFontTall = false;

int GetPatchedGetFontTall(int fontTall)
{
	if (g_bPatchingGetFontTall)
	{
		const int DRAW_OFFSET_X = 3, DRAW_OFFSET_Y = 1;

		//The displayLines should be always >= 1 otherwise the legacy VGUI2 controls may randomly crash

		/*
			int displayLines = g_iPatchingPanelTall / (fontTall + DRAW_OFFSET_Y);
		*/

		if (g_iPatchingPanelTall < fontTall + DRAW_OFFSET_Y)
		{
			return g_iPatchingPanelTall - DRAW_OFFSET_Y;
		}
	}

	return fontTall;
}

void __fastcall RichText_InsertChar(void* pthis, int dummy, wchar_t ch)
{
	if (ch == L'\r')
		return;

	gPrivateFuncs.RichText_InsertChar(pthis, 0, ch);
}

void __fastcall RichText_InsertStringW(void* pthis, int dummy, wchar_t *ch)
{
	while (1)
	{
		std::wstringstream wss;

		while (1)
		{
			if ((*ch) == L'\r' || (*ch) == L'\0')
				break;

			wss << (*ch);

			ch++;
		}

		auto ws = wss.str();

		if (ws.size())
		{
			gPrivateFuncs.RichText_InsertStringW(pthis, 0, ws.c_str());
		}

		if ((*ch) == L'\r' || (*ch) == L'\0')
			break;
	}
}

void __fastcall RichText_OnThink(void* pthis, int dummy)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	gPrivateFuncs.RichText_OnThink(pthis, 0);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;
}

void __fastcall TextEntry_LayoutVerticalScrollBarSlider(void* pthis, int dummy)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	gPrivateFuncs.TextEntry_LayoutVerticalScrollBarSlider(pthis, 0);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;
}

void __fastcall TextEntry_OnKeyCodeTyped(void* pthis, int dummy, vgui::KeyCode code)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	gPrivateFuncs.TextEntry_OnKeyCodeTyped(pthis, 0, (int)code);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;
}

int __fastcall TextEntry_GetStartDrawIndex(void* pthis, int dummy, int& lineBreakIndexIndex)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	int result = gPrivateFuncs.TextEntry_GetStartDrawIndex(pthis, 0, lineBreakIndexIndex);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;

	return result;
}

void __fastcall GameUI_Panel_Init(vgui::Panel* pthis, int dummy, int x, int y, int w, int h)
{
	gPrivateFuncs.GameUI_Panel_Init(pthis, 0, x, y, w, h);

	if (g_iEngineType != ENGINE_GOLDSRC_HL25 && DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		PVOID* PanelVFTable = *(PVOID**)pthis;
		void(__fastcall * pfnSetProportional)(vgui::Panel * pthis, int dummy, bool state) = (decltype(pfnSetProportional))PanelVFTable[113];
		pfnSetProportional(pthis, 0, true);
	}
}

IGameUI *g_pGameUI = NULL;

static void (__fastcall *g_pfnCGameUI_Initialize)(void *pthis, int edx, CreateInterfaceFn *factories, int count) = 0;
static void (__fastcall *g_pfnCGameUI_Start)(void *pthis, int edx, struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, void *system) = 0;
static void (__fastcall *g_pfnCGameUI_Shutdown)(void *pthis, int edx) = 0;
static int (__fastcall *g_pfnCGameUI_ActivateGameUI)(void *pthis, int edx) = 0;
static int (__fastcall *g_pfnCGameUI_ActivateDemoUI)(void *pthis, int edx) = 0;
static int (__fastcall *g_pfnCGameUI_HasExclusiveInput)(void *pthis, int edx) = 0;
static void (__fastcall *g_pfnCGameUI_RunFrame)(void *pthis, int edx) = 0;
static void (__fastcall *g_pfnCGameUI_ConnectToServer)(void *pthis, int edx, const char *game, int IP, int port) = 0;
static void (__fastcall *g_pfnCGameUI_DisconnectFromServer)(void *pthis, int edx) = 0;
static void (__fastcall *g_pfnCGameUI_HideGameUI)(void *pthis, int edx) = 0;
static bool (__fastcall *g_pfnCGameUI_IsGameUIActive)(void *pthis, int edx) = 0;
static void (__fastcall *g_pfnCGameUI_LoadingStarted)(void *pthis, int edx, const char *resourceType, const char *resourceName) = 0;
static void (__fastcall *g_pfnCGameUI_LoadingFinished)(void *pthis, int edx, const char *resourceType, const char *resourceName) = 0;
static void (__fastcall *g_pfnCGameUI_StartProgressBar)(void *pthis, int edx, const char *progressType, int progressSteps) = 0;
static int (__fastcall *g_pfnCGameUI_ContinueProgressBar)(void *pthis, int edx, int progressPoint, float progressFraction) = 0;
static void (__fastcall *g_pfnCGameUI_StopProgressBar)(void *pthis, int edx, bool bError, const char *failureReason, const char *extendedReason) = 0;
static int (__fastcall *g_pfnCGameUI_SetProgressBarStatusText)(void *pthis, int edx, const char *statusText) = 0;
static void (__fastcall *g_pfnCGameUI_SetSecondaryProgressBar)(void *pthis, int edx, float progress) = 0;
static void (__fastcall *g_pfnCGameUI_SetSecondaryProgressBarText)(void *pthis, int edx, const char *statusText) = 0;

class CGameUIProxy : public IGameUI
{
public:
	void Initialize(CreateInterfaceFn *factories, int count) override;
	void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, void *system) override;
	void Shutdown(void) override;
	int ActivateGameUI(void) override;
	int ActivateDemoUI(void) override;
	int HasExclusiveInput(void) override;
	void RunFrame(void) override;
	void ConnectToServer(const char *game, int IP, int port) override;
	void DisconnectFromServer(void) override;
	void HideGameUI(void) override;
	bool IsGameUIActive(void) override;
	void LoadingStarted(const char *resourceType, const char *resourceName) override;
	void LoadingFinished(const char *resourceType, const char *resourceName) override;
	void StartProgressBar(const char *progressType, int progressSteps) override;
	int ContinueProgressBar(int progressPoint, float progressFraction) override;
	void StopProgressBar(bool bError, const char *failureReason, const char *extendedReason) override;
	int SetProgressBarStatusText(const char *statusText) override;
	void SetSecondaryProgressBar(float progress) override;
	void SetSecondaryProgressBarText(const char *statusText) override;
};

static CGameUIProxy s_GameUIProxy;

void CGameUIProxy::Initialize(CreateInterfaceFn *factories, int count)
{
	g_pfnCGameUI_Initialize(this, 0, factories, count);

	VGUI2ExtensionInternal()->GameUI_Initialize(factories, count);
}

void CGameUIProxy::Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, void *system)
{
	g_pfnCGameUI_Start(this, 0, engineFuncs, interfaceVersion, system);

	VGUI2ExtensionInternal()->GameUI_Start(engineFuncs, interfaceVersion, system);
}

void CGameUIProxy::Shutdown(void)
{
	VGUI2ExtensionInternal()->GameUI_Shutdown();

	return g_pfnCGameUI_Shutdown(this, 0);
}

int CGameUIProxy::ActivateGameUI(void)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_ActivateGameUI(&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_ActivateGameUI(this, 0);
	}

	CallbackContext.pRealReturnValue = &real_ret;
	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_ActivateGameUI(&CallbackContext);

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

int CGameUIProxy::ActivateDemoUI(void)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_ActivateDemoUI(&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_ActivateDemoUI(this, 0);
	}

	CallbackContext.pRealReturnValue = &real_ret;
	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_ActivateDemoUI(&CallbackContext);

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

int CGameUIProxy::HasExclusiveInput(void)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_HasExclusiveInput(&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_HasExclusiveInput(this, 0);
	}

	CallbackContext.pRealReturnValue = &real_ret;
	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_HasExclusiveInput(&CallbackContext);

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

void CGameUIProxy::RunFrame(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_RunFrame(&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_RunFrame(this, 0);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_RunFrame(&CallbackContext);
}

void CGameUIProxy::ConnectToServer(const char *game, int IP, int port)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_ConnectToServer(game, IP, port , &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_ConnectToServer(this, 0, game, IP, port);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_ConnectToServer(game, IP, port , &CallbackContext);

#if 0
	if (gEngfuncs.GetMaxClients() > 1)
	{
		return g_pfnCGameUI_ConnectToServer(this, 0, game, IP, port);
	}

	//This just stop GameUI from sending "mp3 stop" on level transition
	return g_pfnCGameUI_ConnectToServer(this, 0, "valve", IP, port);
#endif
}

void CGameUIProxy::DisconnectFromServer(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_DisconnectFromServer(&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_DisconnectFromServer(this, 0);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_DisconnectFromServer(&CallbackContext);
}

void CGameUIProxy::HideGameUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_HideGameUI(&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_HideGameUI(this, 0);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_HideGameUI(&CallbackContext);
}

bool CGameUIProxy::IsGameUIActive(void)
{
	bool fake_ret = 0;
	bool real_ret = 0;
	bool ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_IsGameUIActive(&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_IsGameUIActive(this, 0);
	}

	CallbackContext.pRealReturnValue = &real_ret;
	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_IsGameUIActive(&CallbackContext);

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

void CGameUIProxy::LoadingStarted(const char *resourceType, const char *resourceName)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_LoadingStarted(resourceType, resourceName, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_LoadingStarted(this, 0, resourceType, resourceName);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_LoadingStarted(resourceType, resourceName, &CallbackContext);
}

void CGameUIProxy::LoadingFinished(const char *resourceType, const char *resourceName)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_LoadingFinished(resourceType, resourceName, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_LoadingFinished(this, 0, resourceType, resourceName);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_LoadingFinished(resourceType, resourceName, &CallbackContext);
}

void CGameUIProxy::StartProgressBar(const char *progressType, int progressSteps)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_StartProgressBar(progressType, progressSteps, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_StartProgressBar(this, 0, progressType, progressSteps);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_StartProgressBar(progressType, progressSteps, &CallbackContext);
}

int CGameUIProxy::ContinueProgressBar(int progressPoint, float progressFraction)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_ContinueProgressBar(progressPoint, progressFraction, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_ContinueProgressBar(this, 0, progressPoint, progressFraction);
	}

	CallbackContext.pRealReturnValue = &real_ret;
	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_ContinueProgressBar(progressPoint, progressFraction, &CallbackContext);

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

void CGameUIProxy::StopProgressBar(bool bError, const char *failureReason, const char *extendedReason)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_StopProgressBar(bError, failureReason, extendedReason, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_StopProgressBar(this, 0, bError, failureReason, extendedReason);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_StopProgressBar(bError, failureReason, extendedReason, &CallbackContext);
}

int CGameUIProxy::SetProgressBarStatusText(const char *statusText)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_SetProgressBarStatusText(statusText, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_SetProgressBarStatusText(this, 0, statusText);
	}

	CallbackContext.pRealReturnValue = &real_ret;
	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_SetProgressBarStatusText(statusText, &CallbackContext);

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	{
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

void CGameUIProxy::SetSecondaryProgressBar(float progress)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBar(progress, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_SetSecondaryProgressBar(this, 0, progress);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBar(progress, &CallbackContext);
}

void CGameUIProxy::SetSecondaryProgressBarText(const char *statusText)
{
	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBarText(statusText, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_SetSecondaryProgressBarText(this, 0, statusText);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBarText(statusText, &CallbackContext);
}

void __fastcall COptionsSubVideo_ApplyVidSettings(vgui::Panel *pthis, int dummy, bool bForceRestart)
{
	void* _this = pthis;

	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart ,&CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.COptionsSubVideo_ApplyVidSettings(_this, dummy, bForceRestart);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);	
}

void __fastcall COptionsSubVideo_ApplyVidSettings_HL25(vgui::Panel *pthis, int dummy)
{
	void* _this = pthis;
	bool bForceRestart = false;

	VGUI2Extension_CallbackContext CallbackContext;

	VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.COptionsSubVideo_ApplyVidSettings_HL25(pthis, dummy);
	}

	CallbackContext.IsPost = true;

	VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);
}

void* __fastcall FocusNavGroup_GetCurrentFocus(void* pthis, int dummy)
{
	vgui::VPanelHandle* _currentFocus = (vgui::VPanelHandle*)((PUCHAR)pthis + 12);

	auto vpanel = _currentFocus->Get();

	if (vpanel)
	{
		auto pPanel = vgui::ipanel()->GetPanel(vpanel, "GameUI");

		if (!pPanel)
		{
			for (int i = 0; i < VGUI2ExtensionInternal()->GameUI_GetCallbackCount(); ++i)
			{
				pPanel = vgui::ipanel()->GetPanel(vpanel, VGUI2ExtensionInternal()->GameUI_GetControlModuleName(i));

				if (pPanel)
					break;
			}			
		}

		return pPanel;
	}

	return NULL;
}

void* __fastcall PropertySheet_HasHotkey(void *pthis, int dummy, wchar_t key)
{
	int offset_activePage = 144;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		offset_activePage = 148;
	}

	auto _activePage = *(vgui::Panel**)((PUCHAR)pthis + offset_activePage);

	if (!_activePage)
		return 0;

	int childCount = _activePage->GetChildCount();

	for (int i = 0; i < childCount; i++)
	{
		auto pChild = _activePage->GetChild(i);
#if 0
		if (!pChild)
		{
			pChild = _activePage->GetChildWithModuleName(i, "GameUI");
		}
		if (!pChild)
		{
			pChild = _activePage->GetChildWithModuleName(i, "CaptionMod");
		}
#endif
		if (pChild)
		{
			auto hot = pChild->HasHotkey(key);

			if (hot)
			{
				return (void *)hot;
			}
		}
	}

	return NULL;
}

class CGameUIOptionsDialogCtorCallbackContext : public IGameUIOptionsDialogCtorCallbackContext
{
public:
	CGameUIOptionsDialogCtorCallbackContext(vgui::Panel* pthis, vgui::Panel* _propertySheet)
	{
		m_pDialog = pthis;
		m_pPropertySheet = _propertySheet;
	}

	void InstallHook()
	{
		if (!gPrivateFuncs.FocusNavGroup_GetCurrentFocus)
		{
			PVOID* COptionsDialog_vftable = *(PVOID**)m_pDialog;
			void* (__fastcall * pfnGetFocusNavGroup)(vgui::Panel * pthis, int dummy) = (decltype(pfnGetFocusNavGroup))COptionsDialog_vftable[612 / 4];

			auto FocusNavGroup = pfnGetFocusNavGroup(m_pDialog, 0);
			PVOID* FocusNavGroup_vftable = *(PVOID**)FocusNavGroup;

			gPrivateFuncs.FocusNavGroup_GetCurrentFocus = (decltype(gPrivateFuncs.FocusNavGroup_GetCurrentFocus))FocusNavGroup_vftable[7];
			Install_InlineHook(FocusNavGroup_GetCurrentFocus);
		}

		if (!gPrivateFuncs.PropertySheet_HasHotkey)
		{
			PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

			gPrivateFuncs.PropertySheet_HasHotkey = (decltype(gPrivateFuncs.PropertySheet_HasHotkey))_propertySheet_vftable[73];
			Install_InlineHook(PropertySheet_HasHotkey);
		}
	}

	void* GetDialog() const override
	{
		return m_pDialog;
	}

	void* GetPropertySheet() const override
	{
		return m_pPropertySheet;
	}

	void AddPage(void* panel, const char* title) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnAddPage)(vgui::Panel * pthis, int dummy, vgui::Panel * panel, const char* title) =
			(decltype(pfnAddPage))_propertySheet_vftable[536 / 4];

		pfnAddPage(m_pPropertySheet, 0, (vgui::Panel*)panel, title);
	}

	vgui::Panel* m_pDialog;
	vgui::Panel* m_pPropertySheet;
};


void* __fastcall COptionsDialog_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.COptionsDialog_ctor(pthis, dummy, parent);

	int offset_propertySheet = 272;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		offset_propertySheet = 280;
	}

	vgui::Panel* _propertySheet = *(vgui::Panel**)((PUCHAR)pthis + offset_propertySheet);

	CGameUIOptionsDialogCtorCallbackContext CallbackContext(pthis, _propertySheet);

	CallbackContext.InstallHook();

	VGUI2ExtensionInternal()->GameUI_COptionsDialog_ctor(&CallbackContext);

	//Load res to make it proportional
	gPrivateFuncs.GameUI_LoadControlSettings(pthis, 0, "Resource/OptionsDialog.res", NULL);

	return result;
}

void* __fastcall CCreateMultiplayerGameDialog_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.CCreateMultiplayerGameDialog_ctor(pthis, dummy, parent);

	//Load res to make it proportional
	gPrivateFuncs.GameUI_LoadControlSettings(pthis, 0, "Resource/CreateMultiplayerGameDialog.res", NULL);

	return result;
}

void* __fastcall CGameConsoleDialog_ctor(vgui::Panel* pthis, int dummy)
{
	auto result = gPrivateFuncs.CGameConsoleDialog_ctor(pthis, dummy);

	//Load res to make it proportional
	gPrivateFuncs.GameUI_LoadControlSettings(pthis, 0, "Resource/GameConsoleDialog.res", NULL);

	return result;
}

#if 0
void* __fastcall QueryBox_ctor(vgui::Panel* pthis, int dummy, const char* title, const char* queryText, vgui::Panel* parent)
{
	

	//Load res to make it proportional
	if (!strcmp(queryText, "#GameUI_QuitConfirmationText"))
	{
		//gPrivateFuncs.GameUI_LoadControlSettings(pthis, 0, "Resource/QuitConfirmationBox.res", NULL);

		return gPrivateFuncs.QueryBox_ctor(pthis, dummy, title, "#GameUI_QuitConfirmationText\n\n", parent);
	}
	auto result = gPrivateFuncs.QueryBox_ctor(pthis, dummy, title, queryText, parent);

	return result;
}
#endif

void GameUI_FillAddress(HMODULE hGameUI)
{
	auto GameUIBase = g_pMetaHookAPI->GetModuleBase(hGameUI);
	if (!GameUIBase)
	{
		g_pMetaHookAPI->SysError("Failed to get image base of GameUI.dll");
		return;
	}

	ULONG GameUITextSize = 0;
	auto GameUITextBase = g_pMetaHookAPI->GetSectionByName(GameUIBase, ".text\0\0\0", &GameUITextSize);

	if (!GameUITextBase)
	{
		g_pMetaHookAPI->SysError("Failed to locate section \".text\" in GameUI.dll");
		return;
	}

	ULONG GameUIRdataSize = 0;
	auto GameUIRdataBase = g_pMetaHookAPI->GetSectionByName(GameUIBase, ".rdata\0\0", &GameUIRdataSize);

	if (!GameUIRdataBase)
	{
		g_pMetaHookAPI->SysError("Failed to locate section \".rdata\" in GameUI.dll");
		return;
	}

	ULONG GameUIDataSize = 0;
	auto GameUIDataBase = g_pMetaHookAPI->GetSectionByName(GameUIBase, ".data\0\0", &GameUIDataSize);

	if (!GameUIDataBase)
	{
		g_pMetaHookAPI->SysError("Failed to locate section \".data\" in GameUI.dll");
		return;
	}
#if 0
	if (1)
	{
		const char sigs1[] = "#GameUI_QuitConfirmationTitle";
		auto GameUI_QuitConfirmationTitle_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!GameUI_QuitConfirmationTitle_String)
			GameUI_QuitConfirmationTitle_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(GameUI_QuitConfirmationTitle_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 6) = (DWORD)GameUI_QuitConfirmationTitle_String;
		auto GameUI_QuitConfirmationTitle_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);

		Sig_VarNotFound(GameUI_QuitConfirmationTitle_PushString);

		g_pMetaHookAPI->DisasmRanges(GameUI_QuitConfirmationTitle_PushString, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;

			if (address[0] == 0xE8 && instCount <= 5)
			{
				gPrivateFuncs.QueryBox_ctor = (decltype(gPrivateFuncs.QueryBox_ctor))GetCallAddress(address);

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, NULL);

		Sig_FuncNotFound(QueryBox_ctor);
	}
#endif

	if (1)
	{
		const char sigs1[] = "CreateMultiplayerGameDialog\0";
		auto CreateMultiplayerGameDialog_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!CreateMultiplayerGameDialog_String)
			CreateMultiplayerGameDialog_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(CreateMultiplayerGameDialog_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 1) = (DWORD)CreateMultiplayerGameDialog_String;
		auto CreateMultiplayerGameDialog_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(CreateMultiplayerGameDialog_PushString);
		Sig_VarNotFound(CreateMultiplayerGameDialog_PushString);

		gPrivateFuncs.CCreateMultiplayerGameDialog_ctor = (decltype(gPrivateFuncs.CCreateMultiplayerGameDialog_ctor))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(CreateMultiplayerGameDialog_PushString, 0x120, [](PUCHAR Candidate) {

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
				return TRUE;

			//8B 44 24 04                                         mov     eax, [esp+arg_0]
			if (Candidate[0] == 0x8B &&
				Candidate[1] == 0x44 &&
				Candidate[2] == 0x24)
			{
				//.text:1001B472 68 CC 01 00 00                                      push    1CCh
				//text : 1001B477 68 5C 01 00 00                                     push    15Ch
				if (g_pMetaHookAPI->SearchPattern(Candidate, 0x30, "\x68\xCC\x01\x00\x00\x68\x5C\x01\x00\x00", sizeof("\x68\xCC\x01\x00\x00\x68\x5C\x01\x00\x00") - 1))
				{
					return TRUE;
				}
			}
			return FALSE;
			});

		Sig_FuncNotFound(CCreateMultiplayerGameDialog_ctor);
	}

	if (1)
	{
		const char sigs1[] = "#GameUI_Options";
		auto GameUI_Options_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!GameUI_Options_String)
			GameUI_Options_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(GameUI_Options_String);

		char pattern[] = "\x6A\x01\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 3) = (DWORD)GameUI_Options_String;
		auto GameUI_Options_Call = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(GameUI_Options_Call);

		gPrivateFuncs.COptionsDialog_ctor = (decltype(gPrivateFuncs.COptionsDialog_ctor))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(GameUI_Options_Call, 0x300, [](PUCHAR Candidate) {
			//.text : 10016CB0 55                                                  push    ebp
			//.text : 10016CB1 8B EC                                               mov     ebp, esp
			//.text : 10016CB3 6A FF
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x6A &&
				Candidate[4] == 0xFF)
				return TRUE;

			//8B 44 24 04                                         mov     eax, [esp+arg_0]
			if (Candidate[0] == 0x8B &&
				Candidate[1] == 0x44 &&
				Candidate[2] == 0x24)
			{
				//.text : 100377D2 68 96 01 00 00                                      push    196h
				//.text : 100377D7 68 00 02 00 00                                      push    200h
				if (g_pMetaHookAPI->SearchPattern(Candidate, 0x30, "\x68\x96\x01\x00\x00\x68\x00\x02\x00\x00", sizeof("\x68\x96\x01\x00\x00\x68\x00\x02\x00\x00") - 1))
				{
					return TRUE;
				}
			}
			return FALSE;
			});

		Sig_FuncNotFound(COptionsDialog_ctor);
	}

	if (1)
	{
		const char sigs1[] = "#GameUI_Video";
		auto GameUI_Video_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!GameUI_Video_String)
			GameUI_Video_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(GameUI_Video_String);

		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x2A\x2A\x33\xC0\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 10) = (DWORD)GameUI_Video_String;
		auto GameUI_Video_Call = g_pMetaHookAPI->SearchPattern(gPrivateFuncs.COptionsDialog_ctor, 0x300, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(GameUI_Video_Call);

		gPrivateFuncs.COptionsSubVideo_ctor = (decltype(gPrivateFuncs.COptionsSubVideo_ctor))GetCallAddress(GameUI_Video_Call);
		Sig_FuncNotFound(COptionsSubVideo_ctor);
	}

	if (1)
	{
		const char sigs1[] = "Resource\\OptionsSubVideo.res";
		auto OptionsSubVideo_res_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!OptionsSubVideo_res_String)
			OptionsSubVideo_res_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(OptionsSubVideo_res_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x8B";
		*(DWORD*)(pattern + 1) = (DWORD)OptionsSubVideo_res_String;
		auto OptionsSubVideo_res_PushString = g_pMetaHookAPI->SearchPattern(gPrivateFuncs.COptionsSubVideo_ctor, 0x800, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(OptionsSubVideo_res_PushString);

		g_pMetaHookAPI->DisasmRanges(OptionsSubVideo_res_PushString, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;

			if (address[0] == 0xE8 && instCount <= 8)
			{
				gPrivateFuncs.GameUI_LoadControlSettings = (decltype(gPrivateFuncs.GameUI_LoadControlSettings))GetCallAddress(address);

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, NULL);

		Sig_FuncNotFound(GameUI_LoadControlSettings);
	}

	if (1)
	{
		const char sigs1[] = "#GameUI_Audio";
		auto GameUI_Audio_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!GameUI_Audio_String)
			GameUI_Audio_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(GameUI_Audio_String);

		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x2A\x2A\x33\xC0\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 10) = (DWORD)GameUI_Audio_String;
		auto GameUI_Audio_Call = g_pMetaHookAPI->SearchPattern(gPrivateFuncs.COptionsDialog_ctor, 0x300, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(GameUI_Audio_Call);

		gPrivateFuncs.COptionsSubAudio_ctor = (decltype(gPrivateFuncs.COptionsSubAudio_ctor))GetCallAddress(GameUI_Audio_Call);
		Sig_FuncNotFound(COptionsSubAudio_ctor);
	}

	if (1)
	{
		const char sigs1[] = "_setvideomode";
		auto SetVideoMode_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!SetVideoMode_String)
			SetVideoMode_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(SetVideoMode_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)SetVideoMode_String;
		auto SetVideoMode_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(SetVideoMode_PushString);

		if (g_iEngineType != ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.COptionsSubVideo_ApplyVidSettings = (decltype(gPrivateFuncs.COptionsSubVideo_ApplyVidSettings))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(SetVideoMode_PushString, 0x300, [](PUCHAR Candidate) {
				//  .text : 1001D2C0 55                                                  push    ebp
				//	.text : 1001D2C1 8B EC                                               mov     ebp, esp
				//	.text : 1001D2C3 81 EC 0C 02 00 00                                   sub     esp, 20Ch
				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC &&
					Candidate[3] == 0x81 &&
					Candidate[4] == 0xEC)
					return TRUE;

				//.text:1003DDB0 81 EC 08 02 00 00                                   sub     esp, 208h
				if (Candidate[0] == 0x81 &&
					Candidate[1] == 0xEC &&
					Candidate[4] == 0x00 &&
					Candidate[5] == 0x00)
				{
					return TRUE;
				}

				return FALSE;
				});

			Sig_FuncNotFound(COptionsSubVideo_ApplyVidSettings);
		}
		else
		{
			gPrivateFuncs.COptionsSubVideo_ApplyVidSettings_HL25 = (decltype(gPrivateFuncs.COptionsSubVideo_ApplyVidSettings_HL25))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(SetVideoMode_PushString, 0x300, [](PUCHAR Candidate) {
				//  .text : 1001D2C0 55                                                  push    ebp
				//	.text : 1001D2C1 8B EC                                               mov     ebp, esp
				//	.text : 1001D2C3 81 EC 0C 02 00 00                                   sub     esp, 20Ch
				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC &&
					Candidate[3] == 0x81 &&
					Candidate[4] == 0xEC)
					return TRUE;

				//.text:1003DDB0 81 EC 08 02 00 00                                   sub     esp, 208h
				if (Candidate[0] == 0x81 &&
					Candidate[1] == 0xEC &&
					Candidate[4] == 0x00 &&
					Candidate[5] == 0x00)
				{
					return TRUE;
				}

				return FALSE;
				});
			Sig_FuncNotFound(COptionsSubVideo_ApplyVidSettings_HL25);
		}
	}

	if (1)
	{
		const char sigs1[] = "ConsoleHistory\0";
		auto ConsoleHistory_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!ConsoleHistory_String)
			ConsoleHistory_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(ConsoleHistory_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 1) = (DWORD)ConsoleHistory_String;
		auto ConsoleHistory_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(ConsoleHistory_PushString);

		typedef struct
		{
			PVOID GameUIRdataBase;
			ULONG GameUIRdataSize;

			PVOID GameUITextBase;
			ULONG GameUITextSize;

			PVOID* ConsoleHistory_vftable;

		}ConsoleHistorySearchContext;

		ConsoleHistorySearchContext ctx = { 0 };

		ctx.GameUIRdataBase = GameUIRdataBase;
		ctx.GameUIRdataSize = GameUIRdataSize;

		ctx.GameUITextBase = GameUITextBase;
		ctx.GameUITextSize = GameUITextSize;

		g_pMetaHookAPI->DisasmRanges(ConsoleHistory_PushString, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (ConsoleHistorySearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->GameUIRdataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->GameUIRdataBase + ctx->GameUIRdataSize))
			{
				auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;
				if (candidate[0] >= (PUCHAR)ctx->GameUITextBase && candidate[0] < (PUCHAR)ctx->GameUITextBase + ctx->GameUITextSize)
				{
					ctx->ConsoleHistory_vftable = candidate;
				}
			}

			if (ctx->ConsoleHistory_vftable)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(ctx.ConsoleHistory_vftable);

		gPrivateFuncs.RichText_OnThink = (decltype(gPrivateFuncs.RichText_OnThink))ctx.ConsoleHistory_vftable[0x158 / 4];
	}

	if (1)
	{
		const char sigs2[] = "Unable to condump to \0";
		auto UnableToCondump_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs2, sizeof(sigs2) - 1);
		if (!UnableToCondump_String)
			UnableToCondump_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs2, sizeof(sigs2) - 1);
		Sig_VarNotFound(UnableToCondump_String);

		char pattern2[] = "\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern2 + 1) = (DWORD)UnableToCondump_String;
		auto UnableToCondump_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern2, sizeof(pattern2) - 1);
		Sig_VarNotFound(UnableToCondump_PushString);

		typedef struct
		{

			bool bFound118h;//it's 128h for HL25
			bool bFound_RichText_Print;//it's 128h for HL25

		}UnableToCondumpSearchContext;

		UnableToCondumpSearchContext ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(UnableToCondump_PushString, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (UnableToCondumpSearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.disp >= 0x118 && pinst->detail->x86.operands[1].mem.disp <= 0x120)
			{
				ctx->bFound118h = true;
			}

			if (address[0] == 0xE8 && instCount <= 5)
			{
				if (ctx->bFound118h)
				{
					gPrivateFuncs.RichText_InsertStringA = (decltype(gPrivateFuncs.RichText_InsertStringA))GetCallAddress(address);
				}
				else
				{
					gPrivateFuncs.RichText_Print = (decltype(gPrivateFuncs.RichText_Print))GetCallAddress(address);
				}
				ctx->bFound_RichText_Print = true;

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(ctx.bFound_RichText_Print);
	}

	if (1)
	{
		PVOID RecursiveWalkBase = (gPrivateFuncs.RichText_Print) ? gPrivateFuncs.RichText_Print : gPrivateFuncs.RichText_InsertStringA;

		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;

			PVOID FunctionBeginCandidate;
			int FunctionBeginCandidateDepth;
			PVOID Found0xDCandidate;
			int Found0xDCandidateInstCount;
			bool Is0xDCandidatePatched;

			PVOID GameUIRdataBase;
			ULONG GameUIRdataSize;

			PVOID GameUITextBase;
			ULONG GameUITextSize;

		}RichText_PrintWalkContext;

		RichText_PrintWalkContext ctx = { 0 };

		ctx.GameUIRdataBase = GameUIRdataBase;
		ctx.GameUIRdataSize = GameUIRdataSize;

		ctx.GameUITextBase = GameUITextBase;
		ctx.GameUITextSize = GameUITextSize;

		ctx.base = RecursiveWalkBase;
		ctx.max_insts = 1000;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x1000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn*)inst;
					auto ctx = (RichText_PrintWalkContext*)context;

					if (ctx->code.size() > ctx->max_insts)
						return TRUE;

					if (ctx->code.find(address) != ctx->code.end())
						return TRUE;

					ctx->code.emplace(address);

					/*
					Engine: 3266
.text:1006BE4D 0F BE 45 08                                         movsx   eax, byte ptr [ebp+arg_0]
.text:1006BE51 83 F8 0D                                            cmp     eax, 0Dh
.text:1006BE54 75 02                                               jnz     short loc_1006BE58 label_work
.text:1006BE56 EB 67                                               jmp     short loc_1006BEBF label_exit
					*/

					/*
					Engine: 4554, 6153
.text:100573B0 8A 44 24 04                                         mov     al, byte ptr [esp+arg_0]
.text:100573B4 55                                                  push    ebp
.text:100573B5 3C 0D                                               cmp     al, 0Dh
.text:100573B7 8B E9                                               mov     ebp, ecx
.text:100573B9 0F 84 8C 00 00 00                                   jz      loc_1005744B label_exit
					*/

					/*
					Engine: SvEngine
.text:10047463 80 7D 08 0D                                         cmp     [ebp+arg_0], 0Dh

.text:1004746A 74 3C                                               jz      short loc_100474A8 label_exit
					*/

					/*
					Engine: 9920
.text:1005D664 0F B7 C1                                            movzx   eax, cx
.text:1005D667 89 45 08                                            mov     [ebp+arg_0], eax
.text:1005D66A 80 F9 0D                                            cmp     cl, 0Dh
.text:1005D66D 74 3B                                               jz      short loc_1005D6AA label_exit
					*/

					if (instCount == 1)
					{
						ctx->FunctionBeginCandidate = address;
						ctx->FunctionBeginCandidateDepth = depth;
					}

					if (!ctx->Found0xDCandidate &&
						instCount < 25 &&
						depth == ctx->FunctionBeginCandidateDepth &&
						pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						(pinst->detail->x86.operands[0].type == X86_OP_REG || pinst->detail->x86.operands[0].type == X86_OP_MEM) &&
						pinst->detail->x86.operands[1].imm == 0x0D)
					{
						ctx->Found0xDCandidate = address;
						ctx->Found0xDCandidateInstCount = instCount;

						typedef struct
						{
							bool IsFetchWord;
						}RichText_InsertCharContext;

						RichText_InsertCharContext ctx2 = { 0 };

						g_pMetaHookAPI->DisasmRanges(ctx->FunctionBeginCandidate, address - ctx->FunctionBeginCandidate, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
							{
								auto pinst = (cs_insn*)inst;
								auto ctx = (RichText_InsertCharContext*)context;
								//66 8B 06                                            mov     ax, [esi]
								//0F B7 07                                            movzx   eax, word ptr [edi]
								//66 8B 08                                            mov     cx, [eax]

								if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_MOVZX) &&
									pinst->detail->x86.op_count == 2 &&
									pinst->detail->x86.operands[0].type == X86_OP_REG &&
									pinst->detail->x86.operands[1].type == X86_OP_MEM &&
									(pinst->detail->x86.operands[0].size == 2 || pinst->detail->x86.operands[1].size == 2) &&
									pinst->detail->x86.operands[1].mem.base != 0 &&
									pinst->detail->x86.operands[1].mem.disp == 0 &&
									pinst->detail->x86.operands[1].mem.index == 0 &&
									pinst->detail->x86.operands[1].mem.scale == 1)
								{
									ctx->IsFetchWord = true;
								}

								if (address[0] == 0xCC)
									return TRUE;

								if (pinst->id == X86_INS_RET)
									return TRUE;

								return FALSE;

							}, 0, &ctx2);

						if (ctx2.IsFetchWord)
						{
							gPrivateFuncs.RichText_InsertStringW = (decltype(gPrivateFuncs.RichText_InsertStringW))ctx->FunctionBeginCandidate;
						}
						else
						{
							gPrivateFuncs.RichText_InsertChar = (decltype(gPrivateFuncs.RichText_InsertChar))ctx->FunctionBeginCandidate;
						}
					}

					if (!ctx->Is0xDCandidatePatched &&
						ctx->Found0xDCandidateInstCount > 0 &&
						instCount > ctx->Found0xDCandidateInstCount &&
						instCount < ctx->Found0xDCandidateInstCount + 5 &&
						(pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS) &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM)
					{
						if (pinst->id == X86_INS_JE)
						{
							g_pMetaHookAPI->WriteNOP(address, instLen);
							ctx->Is0xDCandidatePatched = true;
						}
						else if (pinst->id == X86_INS_JNE)
						{
							if (instLen == 2)
							{
								//jmp short
								g_pMetaHookAPI->WriteBYTE(address, 0xEB);
								ctx->Is0xDCandidatePatched = true;
							}
							else if (instLen == 5)
							{
								//jmp
								g_pMetaHookAPI->WriteBYTE(address, 0xE9);
								ctx->Is0xDCandidatePatched = true;
							}
						}
					}

					if (ctx->Is0xDCandidatePatched)
						return TRUE;

					if ((pinst->id == X86_INS_CALL || pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM)
					{
						PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
						if (imm >= (PUCHAR)ctx->GameUITextBase && imm < (PUCHAR)ctx->GameUITextBase + ctx->GameUITextSize)
						{
							auto foundbranch = ctx->branches.find(imm);
							if (foundbranch == ctx->branches.end())
							{
								ctx->branches.emplace(imm);
								if (depth + 1 < ctx->max_depth)
									ctx->walks.emplace_back(imm, 0x1000, depth + 1);
							}
						}

						if (pinst->id == X86_INS_JMP)
							return TRUE;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

				}, walk.depth, &ctx);
		}

		if (!ctx.Is0xDCandidatePatched)
		{
			g_pMetaHookAPI->SysError("Failed to patch GameUI!RichText_InsertChar.");
		}

		if (!gPrivateFuncs.RichText_InsertChar && !gPrivateFuncs.RichText_InsertStringW)
		{
			g_pMetaHookAPI->SysError("Failed to locate GameUI!RichText_InsertChar or RichText_InsertStringW.");
		}
	}

	if (1)
	{
		const char sigs1[] = "ConsoleEntry\0";
		auto ConsoleEntry_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!ConsoleEntry_String)
			ConsoleEntry_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(ConsoleEntry_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 1) = (DWORD)ConsoleEntry_String;
		auto ConsoleEntry_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(ConsoleEntry_PushString);

		typedef struct
		{
			PVOID GameUIRdataBase;
			ULONG GameUIRdataSize;

			PVOID GameUITextBase;
			ULONG GameUITextSize;

			PVOID* ConsoleEntry_vftable;

		}ConsoleEntrySearchContext;

		ConsoleEntrySearchContext ctx = { 0 };

		ctx.GameUIRdataBase = GameUIRdataBase;
		ctx.GameUIRdataSize = GameUIRdataSize;

		ctx.GameUITextBase = GameUITextBase;
		ctx.GameUITextSize = GameUITextSize;

		g_pMetaHookAPI->DisasmRanges(ConsoleEntry_PushString, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (ConsoleEntrySearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->GameUIRdataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->GameUIRdataBase + ctx->GameUIRdataSize))
			{
				auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;
				if (candidate[0] >= (PUCHAR)ctx->GameUITextBase && candidate[0] < (PUCHAR)ctx->GameUITextBase + ctx->GameUITextSize)
				{
					ctx->ConsoleEntry_vftable = candidate;
				}
			}

			if (ctx->ConsoleEntry_vftable)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(ctx.ConsoleEntry_vftable);

		gPrivateFuncs.TextEntry_OnKeyCodeTyped = (decltype(gPrivateFuncs.TextEntry_OnKeyCodeTyped))ctx.ConsoleEntry_vftable[0x194 / 4];
		//gPrivateFuncs.TextEntry_InsertChar = (decltype(gPrivateFuncs.TextEntry_InsertChar))ctx.ConsoleEntry_vftable[0x250 / 4];
		gPrivateFuncs.TextEntry_LayoutVerticalScrollBarSlider = (decltype(gPrivateFuncs.TextEntry_LayoutVerticalScrollBarSlider))ctx.ConsoleEntry_vftable[0x2C0 / 4];
		gPrivateFuncs.TextEntry_GetStartDrawIndex = (decltype(gPrivateFuncs.TextEntry_GetStartDrawIndex))ctx.ConsoleEntry_vftable[0x2F8 / 4];
	}
#if 0
	if (1)
	{
		PVOID Sheet_PushString = NULL;
		const char sigs1[] = "Sheet\0";
		auto PropertySheet_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
		if (!PropertySheet_String)
		{
			PropertySheet_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);

			if (PropertySheet_String)
			{
				PUCHAR SearchBegin = (PUCHAR)GameUIDataBase;
				PUCHAR SearchLimit = (PUCHAR)GameUIDataBase + GameUIDataSize;
				while (SearchBegin < SearchLimit)
				{
					PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, sigs1);
					if (pFound)
					{
						char pattern[] = "\x74\x2A\x68\x2A\x2A\x2A\x2A";
						*(DWORD*)(pattern + 3) = (DWORD)pFound;
						Sheet_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);
						if (Sheet_PushString)
						{
							break;
						}

						SearchBegin = pFound + Sig_Length(sigs1);
					}
					else
					{
						break;
					}
				}
			}
		}
		else
		{
			PUCHAR SearchBegin = (PUCHAR)GameUIRdataBase;
			PUCHAR SearchLimit = (PUCHAR)GameUIRdataBase + GameUIRdataSize;
			while (SearchBegin < SearchLimit)
			{
				PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, sigs1);
				if (pFound)
				{
					char pattern[] = "\x74\x2A\x68\x2A\x2A\x2A\x2A";
					*(DWORD*)(pattern + 3) = (DWORD)pFound;
					Sheet_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);
					if (Sheet_PushString)
					{
						break;
					}

					SearchBegin = pFound + Sig_Length(sigs1);
				}
				else
				{
					break;
				}
			}
		}
		Sig_VarNotFound(Sheet_PushString);

		g_pMetaHookAPI->DisasmRanges(Sheet_PushString, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;

			if (address[0] == 0xE8 && instCount <= 5)
			{
				gPrivateFuncs.Sheet_ctor = (decltype(gPrivateFuncs.Sheet_ctor))GetCallAddress(address);

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, NULL);

		Sig_FuncNotFound(Sheet_ctor);

		typedef struct
		{
			PVOID GameUIRdataBase;
			ULONG GameUIRdataSize;

			PVOID GameUITextBase;
			ULONG GameUITextSize;

			PVOID* Sheet_vftable;

		}SheetSearchContext;

		SheetSearchContext ctx = { 0 };

		ctx.GameUIRdataBase = GameUIRdataBase;
		ctx.GameUIRdataSize = GameUIRdataSize;

		ctx.GameUITextBase = GameUITextBase;
		ctx.GameUITextSize = GameUITextSize;

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.Sheet_ctor, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (SheetSearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->GameUIRdataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->GameUIRdataBase + ctx->GameUIRdataSize))
			{
				auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;
				if (candidate[0] >= (PUCHAR)ctx->GameUITextBase && candidate[0] < (PUCHAR)ctx->GameUITextBase + ctx->GameUITextSize)
				{
					ctx->Sheet_vftable = candidate;
				}
			}

			if (ctx->Sheet_vftable)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(ctx.Sheet_vftable);

		gPrivateFuncs.PropertySheet_HasHotkey = (decltype(gPrivateFuncs.PropertySheet_HasHotkey))ctx.Sheet_vftable[73];
	}
#endif

	gPrivateFuncs.GameUI_Panel_Init = (decltype(gPrivateFuncs.GameUI_Panel_Init))VGUI2_FindPanelInit(GameUITextBase, GameUITextSize);
	Sig_FuncNotFound(GameUI_Panel_Init);

	//gPrivateFuncs.PropertySheet_HasHotkey = (decltype(gPrivateFuncs.PropertySheet_HasHotkey))((PUCHAR)GameUIBase + 0x53060);
	//gPrivateFuncs.FocusNavGroup_GetCurrentFocus = (decltype(gPrivateFuncs.FocusNavGroup_GetCurrentFocus))((PUCHAR)GameUIBase + 0x4D640);
}

HMODULE GetGameUIModule();

void GameUI_InstallHooks(void)
{
	auto hGameUI = GetGameUIModule();

	if (!hGameUI)
	{
		g_pMetaHookAPI->SysError("Failed to get GameUI.dll ");
		return;
	}

	CreateInterfaceFn GameUICreateInterface = Sys_GetFactory((HINTERFACEMODULE)hGameUI);

	if (!GameUICreateInterface)
	{
		g_pMetaHookAPI->SysError("Failed to get interface factory from GameUI.dll");
		return;
	}

	g_pGameUI = (IGameUI*)GameUICreateInterface(GAMEUI_INTERFACE_VERSION, 0);

	if (!g_pGameUI)
	{
		g_pMetaHookAPI->SysError("Failed to get interface \"" GAMEUI_INTERFACE_VERSION "\" from GameUI.dll");
		return;
	}

	PVOID * ProxyVFTable = *(PVOID**)&s_GameUIProxy;

	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 1, ProxyVFTable[1], (void**)&g_pfnCGameUI_Initialize);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 2, ProxyVFTable[2], (void**)&g_pfnCGameUI_Start);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 3, ProxyVFTable[3], (void**)&g_pfnCGameUI_Shutdown);

	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 4, ProxyVFTable[4], (void**)&g_pfnCGameUI_ActivateGameUI);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 5, ProxyVFTable[5], (void**)&g_pfnCGameUI_ActivateDemoUI);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 6, ProxyVFTable[6], (void**)&g_pfnCGameUI_HasExclusiveInput);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 7, ProxyVFTable[7], (void**)&g_pfnCGameUI_RunFrame);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 8, ProxyVFTable[8], (void**)&g_pfnCGameUI_ConnectToServer);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 9, ProxyVFTable[9], (void**)&g_pfnCGameUI_DisconnectFromServer);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 10, ProxyVFTable[10], (void**)&g_pfnCGameUI_HideGameUI);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 11, ProxyVFTable[11], (void**)&g_pfnCGameUI_IsGameUIActive);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 12, ProxyVFTable[12], (void**)&g_pfnCGameUI_LoadingStarted);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 13, ProxyVFTable[13], (void**)&g_pfnCGameUI_LoadingFinished);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 14, ProxyVFTable[14], (void**)&g_pfnCGameUI_StartProgressBar);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 15, ProxyVFTable[15], (void**)&g_pfnCGameUI_ContinueProgressBar);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 16, ProxyVFTable[16], (void**)&g_pfnCGameUI_StopProgressBar);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 17, ProxyVFTable[17], (void**)&g_pfnCGameUI_SetProgressBarStatusText);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 18, ProxyVFTable[18], (void**)&g_pfnCGameUI_SetSecondaryProgressBar);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 19, ProxyVFTable[19], (void**)&g_pfnCGameUI_SetSecondaryProgressBarText);

	Install_InlineHook(GameUI_Panel_Init);
	Install_InlineHook(CGameConsoleDialog_ctor);
	Install_InlineHook(CCreateMultiplayerGameDialog_ctor);
	Install_InlineHook(COptionsDialog_ctor);
	Install_InlineHook(COptionsSubVideo_ApplyVidSettings);
	
	if(gPrivateFuncs.RichText_InsertChar)
		Install_InlineHook(RichText_InsertChar);

	if (gPrivateFuncs.RichText_InsertStringW)
		Install_InlineHook(RichText_InsertStringW);

	Install_InlineHook(RichText_OnThink);
	Install_InlineHook(TextEntry_OnKeyCodeTyped);
	Install_InlineHook(TextEntry_LayoutVerticalScrollBarSlider);
	Install_InlineHook(TextEntry_GetStartDrawIndex);
}

void GameUI_UninstallHooks(void)
{
	Uninstall_Hook(GameUI_Panel_Init);
	Uninstall_Hook(CGameConsoleDialog_ctor);
	Uninstall_Hook(CCreateMultiplayerGameDialog_ctor);
	Uninstall_Hook(COptionsDialog_ctor);
	Uninstall_Hook(COptionsSubVideo_ApplyVidSettings);
	Uninstall_Hook(RichText_InsertChar);
	Uninstall_Hook(RichText_InsertStringW);
	Uninstall_Hook(RichText_OnThink);
	Uninstall_Hook(TextEntry_OnKeyCodeTyped);
	Uninstall_Hook(TextEntry_LayoutVerticalScrollBarSlider);
	Uninstall_Hook(TextEntry_GetStartDrawIndex);
	Uninstall_Hook(PropertySheet_HasHotkey);
	Uninstall_Hook(FocusNavGroup_GetCurrentFocus);
}