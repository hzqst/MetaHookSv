#include <metahook.h>
#include "BaseUI.h"
#include <VGUI\IScheme.h>
#include <VGUI\ILocalize.h>
#include <VGUI\ISurface.h>
#include <VGUI\IInput.h>
#include "FontTextureCache.h"
#include <IEngineSurface.h>
#include "vgui_internal.h"
#include "IKeyValuesSystem.h"
#include "exportfuncs.h"
#include "engfuncs.h"

namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

void (__fastcall *m_pfnCBaseUI_Initialize)(void *pthis, int, CreateInterfaceFn *factories, int count);
void (__fastcall *m_pfnCBaseUI_Start)(void *pthis, int, struct cl_enginefuncs_s *engineFuncs, int interfaceVersion);
void (__fastcall *m_pfnCBaseUI_Shutdown)(void *pthis, int);
int (__fastcall *m_pfnCBaseUI_Key_Event)(void *pthis, int, int down, int keynum, const char *pszCurrentBinding);
void (__fastcall *m_pfnCBaseUI_CallEngineSurfaceProc)(void *pthis, int, void *hwnd, unsigned int msg, unsigned int wparam, long lparam);
void (__fastcall *m_pfnCBaseUI_Paint)(void *pthis, int, int x, int y, int right, int bottom);
void (__fastcall *m_pfnCBaseUI_HideGameUI)(void *pthis, int);
void (__fastcall *m_pfnCBaseUI_ActivateGameUI)(void *pthis, int);
bool (__fastcall *m_pfnCBaseUI_IsGameUIVisible)(void *pthis, int);
void (__fastcall *m_pfnCBaseUI_HideConsole)(void *pthis, int);
void (__fastcall *m_pfnCBaseUI_ShowConsole)(void *pthis, int);

class CBaseUI : public IBaseUI
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count);
	virtual void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion);
	virtual void Shutdown(void);
	virtual int Key_Event(int down, int keynum, const char *pszCurrentBinding);
	virtual void CallEngineSurfaceProc(void *hwnd, unsigned int msg, unsigned int wparam, long lparam);
	virtual void Paint(int x, int y, int right, int bottom);
	virtual void HideGameUI(void);
	virtual void ActivateGameUI(void);
	virtual bool IsGameUIVisible(void);
	virtual void HideConsole(void);
	virtual void ShowConsole(void);
};

static CBaseUI s_BaseUI;

IBaseUI *baseuifuncs;
IGameUIFuncs *gameuifuncs;

extern vgui::ISurface *g_pSurface;
extern vgui::ISchemeManager *g_pScheme;
extern IKeyValuesSystem *g_pKeyValuesSystem;
extern IEngineSurface *staticSurface;

static BOOL s_LoadingClientFactory = false;

void CBaseUI::Initialize(CreateInterfaceFn *factories, int count)
{
	//Patch ClientFactory
	if(!g_IsClientVGUI2 && *gCapFuncs.pfnClientFactory == NULL)
	{
		*gCapFuncs.pfnClientFactory = NewClientFactory;
		s_LoadingClientFactory = true;
	}

	m_pfnCBaseUI_Initialize(this, 0, factories, count);

	s_LoadingClientFactory = false;

	HINTERFACEMODULE hVGUI2 = (HINTERFACEMODULE)GetModuleHandle("vgui2.dll");
	if(hVGUI2)
	{
		CreateInterfaceFn fnVGUI2CreateInterface = Sys_GetFactory(hVGUI2);
		g_pScheme = (vgui::ISchemeManager *)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, NULL);
		g_pKeyValuesSystem = (IKeyValuesSystem *)fnVGUI2CreateInterface(KEYVALUESSYSTEM_INTERFACE_VERSION, NULL);
	}

	g_pSurface = (vgui::ISurface *)factories[0](VGUI_SURFACE_INTERFACE_VERSION, NULL);
	staticSurface = (IEngineSurface *)factories[0](ENGINE_SURFACE_VERSION, NULL);

	KeyValuesSystem_InstallHook();
	Surface_InstallHook();
	Scheme_InstallHook();
	GameUI_InstallHook();
}

void CBaseUI::Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion)
{
	m_pfnCBaseUI_Start(this, 0, engineFuncs, interfaceVersion);
}

void CBaseUI::Shutdown(void)
{
	m_pfnCBaseUI_Shutdown(this, 0);
}

int CBaseUI::Key_Event(int down, int keynum, const char *pszCurrentBinding)
{
	return m_pfnCBaseUI_Key_Event(this, 0, down, keynum, pszCurrentBinding);
}

void CBaseUI::CallEngineSurfaceProc(void *hwnd, unsigned int msg, unsigned int wparam, long lparam)
{
	m_pfnCBaseUI_CallEngineSurfaceProc(this, 0, hwnd, msg, wparam, lparam);
}

void CBaseUI::Paint(int x, int y, int right, int bottom)
{
	m_pfnCBaseUI_Paint(this, 0, x, y, right, bottom);
}

void CBaseUI::HideGameUI(void)
{
	m_pfnCBaseUI_HideGameUI(this, 0);
}

void CBaseUI::ActivateGameUI(void)
{
	m_pfnCBaseUI_ActivateGameUI(this, 0);
}

bool CBaseUI::IsGameUIVisible(void)
{
	return m_pfnCBaseUI_IsGameUIVisible(this, 0);
}

void CBaseUI::HideConsole(void)
{
	m_pfnCBaseUI_HideConsole(this, 0);
}

void CBaseUI::ShowConsole(void)
{
	m_pfnCBaseUI_ShowConsole(this, 0);
}

void BaseUI_InstallHook(void)
{
	CreateInterfaceFn fnCreateInterface = g_pMetaHookAPI->GetEngineFactory();
	baseuifuncs = (IBaseUI *)fnCreateInterface(BASEUI_INTERFACE_VERSION, NULL);
	gameuifuncs = (IGameUIFuncs *)fnCreateInterface(VENGINE_GAMEUIFUNCS_VERSION, NULL);

	//Search CBaseUI::Initialize for ClientFactory
	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CLIENTFACTORY_SIG_SVENGINE "\x83\xC4\x0C\x83\x3D"
		DWORD *vft = *(DWORD **)baseuifuncs;

		DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)vft[1], 0x200, CLIENTFACTORY_SIG_SVENGINE, Sig_Length(CLIENTFACTORY_SIG_SVENGINE));
		Sig_AddrNotFound(ClientFactory);
		gCapFuncs.pfnClientFactory = (void *(**)(void))*(DWORD *)(addr + 5);

		DWORD *pVFTable = *(DWORD **)&s_BaseUI;

		g_pMetaHookAPI->VFTHook(baseuifuncs, 0, 1, (void *)pVFTable[1], (void **)&m_pfnCBaseUI_Initialize);
	}
	else
	{
#define CLIENTFACTORY_SIG "\xCC\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74"
		DWORD *vft = *(DWORD **)baseuifuncs;
		DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)vft[1], 0x200, CLIENTFACTORY_SIG, Sig_Length(CLIENTFACTORY_SIG));
		Sig_AddrNotFound(ClientFactory);
		gCapFuncs.pfnClientFactory = (void *(**)(void))*(DWORD *)(addr + 2);

		DWORD *pVFTable = *(DWORD **)&s_BaseUI;

		g_pMetaHookAPI->VFTHook(baseuifuncs, 0, 1, (void *)pVFTable[1], (void **)&m_pfnCBaseUI_Initialize);
	}
}