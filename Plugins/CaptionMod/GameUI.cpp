#include <metahook.h>
#include <IGameUI.h>

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

void GameUI_InstallHook(void)
{
	CreateInterfaceFn GameUICreateInterface = Sys_GetFactory((HINTERFACEMODULE)GetModuleHandleA("GameUI.dll"));
	g_pGameUI = (IGameUI *)GameUICreateInterface(GAMEUI_INTERFACE_VERSION, 0);

	DWORD *pVFTable = *(DWORD **)&s_GameUI;

	g_pMetaHookAPI->VFTHook(g_pGameUI, 0,  1, (void *)pVFTable[1], (void *&)g_pfnCGameUI_Initialize);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0,  8, (void *)pVFTable[8], (void *&)g_pfnCGameUI_ConnectToServer);
	g_pMetaHookAPI->VFTHook(g_pGameUI, 0,  9, (void *)pVFTable[9], (void *&)g_pfnCGameUI_DisconnectFromServer);
}