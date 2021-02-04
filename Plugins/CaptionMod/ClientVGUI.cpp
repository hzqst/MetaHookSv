#include <metahook.h>
#include <VGUI/VGUI.h>
#include <IClientVGUI.h>
#include "engfuncs.h"
#include "plugins.h"
#include "Viewport.h"

namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

static void (__fastcall *m_pfnCClientVGUI_Initialize)(void *pthis, int,CreateInterfaceFn *factories, int count) = NULL;
static void (__fastcall *m_pfnCClientVGUI_Start)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_SetParent)(void *pthis, int, vgui::VPANEL parent) = NULL;
static bool (__fastcall *m_pfnCClientVGUI_UseVGUI1)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideScoreBoard)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideAllVGUIMenu)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_ActivateClientUI)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideClientUI)(void *pthis, int) = NULL;

class CClientVGUI : public IClientVGUI
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count);
	virtual void Start(void);
	virtual void SetParent(vgui::VPANEL parent);
	virtual bool UseVGUI1(void);
	virtual void HideScoreBoard(void);
	virtual void HideAllVGUIMenu(void);
	virtual void ActivateClientUI(void);
	virtual void HideClientUI(void);
};

static CClientVGUI s_ClientVGUI;

IClientVGUI *g_pClientVGUI = NULL;

void Sys_ErrorEx(const char *fmt, ...);

void CClientVGUI::Initialize(CreateInterfaceFn *factories, int count)
{
	//MessageBoxA(NULL, "CClientVGUI::Initialize", "", 0);

	m_pfnCClientVGUI_Initialize(this, 0, factories, count);

	vgui::VGui_InitInterfacesList("CaptionMod", factories, count);

	vgui::scheme()->LoadSchemeFromFile( "captionmod/CaptionScheme.res", "CaptionScheme" );

	if(!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/dictionary_%language%.txt"))
		Sys_ErrorEx("Failed to load captionmod/dictionary_%%language%%.txt");
}

void CClientVGUI::Start(void)
{
	m_pfnCClientVGUI_Start(this, 0);

	g_pViewPort = new CViewport();
	g_pViewPort->Start();
}

void CClientVGUI::SetParent(vgui::VPANEL parent)
{
	m_pfnCClientVGUI_SetParent(this, 0, parent);

	g_pViewPort->SetParent(parent);
}

bool CClientVGUI::UseVGUI1(void)
{
	return m_pfnCClientVGUI_UseVGUI1(this, 0);
}

void CClientVGUI::HideScoreBoard(void)
{
	m_pfnCClientVGUI_HideScoreBoard(this, 0);
}

void CClientVGUI::HideAllVGUIMenu(void)
{
	m_pfnCClientVGUI_HideAllVGUIMenu(this, 0);
}

void CClientVGUI::ActivateClientUI(void)
{
	m_pfnCClientVGUI_ActivateClientUI(this, 0);

	g_pViewPort->ActivateClientUI();
}

void CClientVGUI::HideClientUI(void)
{
	m_pfnCClientVGUI_HideClientUI(this, 0);

	g_pViewPort->HideClientUI();
}

FARPROC WINAPI NewGetProcAddress(HMODULE hModule, LPCSTR lpProcName);

void ClientVGUI_InstallHook(void)
{
	CreateInterfaceFn ClientVGUICreateInterface = NULL;
	if(g_hClientDll)
		ClientVGUICreateInterface = (CreateInterfaceFn)gCapFuncs.GetProcAddress(g_hClientDll, CREATEINTERFACE_PROCNAME);
	if(!ClientVGUICreateInterface && gExportfuncs.ClientFactory)
		ClientVGUICreateInterface = (CreateInterfaceFn)gExportfuncs.ClientFactory();

	g_pClientVGUI = (IClientVGUI *)ClientVGUICreateInterface(CLIENTVGUI_INTERFACE_VERSION, NULL);

	if(g_pClientVGUI)
	{
		DWORD *pVFTable = *(DWORD **)&s_ClientVGUI;

		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  1, (void *)pVFTable[1], (void *&)m_pfnCClientVGUI_Initialize);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  2, (void *)pVFTable[2], (void *&)m_pfnCClientVGUI_Start);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  3, (void *)pVFTable[3], (void *&)m_pfnCClientVGUI_SetParent);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  7, (void *)pVFTable[7], (void *&)m_pfnCClientVGUI_ActivateClientUI);
		g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0,  8, (void *)pVFTable[8], (void *&)m_pfnCClientVGUI_HideClientUI);

		g_IsClientVGUI2 = true;			
	}
	else
	{
		auto pfnGetProcAddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");
		gCapFuncs.hk_GetProcAddress = g_pMetaHookAPI->InlineHook(pfnGetProcAddress, NewGetProcAddress, (void *&)gCapFuncs.GetProcAddress);
	}
}

//implement the ClientVGUI interface for those mod with no ClientVGUI implemented

class NewClientVGUI : public IClientVGUI
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count);
	virtual void Start(void);
	virtual void SetParent(vgui::VPANEL parent);
	virtual bool UseVGUI1(void);
	virtual void HideScoreBoard(void);
	virtual void HideAllVGUIMenu(void);
	virtual void ActivateClientUI(void);
	virtual void HideClientUI(void);
};

void NewClientVGUI::Initialize(CreateInterfaceFn *factories, int count)
{
	//MessageBoxA(NULL, "NewClientVGUI::Initialize", "", 0);

	vgui::VGui_InitInterfacesList("CaptionMod", factories, count);

	vgui::scheme()->LoadSchemeFromFile( "captionmod/CaptionScheme.res", "CaptionScheme" );

	if(!vgui::localize()->AddFile(g_pFullFileSystem, "captionmod/dictionary_%language%.txt"))
		Sys_ErrorEx("Failed to load captionmod/dictionary_%%language%%.txt");
}

void NewClientVGUI::Start(void)
{
	g_pViewPort = new CViewport();
	g_pViewPort->Start();
}

void NewClientVGUI::SetParent(vgui::VPANEL parent)
{
	g_pViewPort->SetParent(parent);
}

bool NewClientVGUI::UseVGUI1(void)
{
	return true;
}

void NewClientVGUI::HideScoreBoard(void)
{
}

void NewClientVGUI::HideAllVGUIMenu(void)
{
}

void NewClientVGUI::ActivateClientUI(void)
{
	g_pViewPort->ActivateClientUI();
}

void NewClientVGUI::HideClientUI(void)
{
	g_pViewPort->HideClientUI();
}

EXPOSE_SINGLE_INTERFACE(NewClientVGUI, IClientVGUI, CLIENTVGUI_INTERFACE_VERSION);