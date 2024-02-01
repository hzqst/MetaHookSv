#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <IClientVGUI.h>
#include "plugins.h"
#include "privatefuncs.h"
#include "exportfuncs.h"

#include "VGUI2ExtensionInternal.h"
namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

extern vgui::ISurface* g_pSurface;
extern vgui::ISurface_HL25* g_pSurface_HL25;

bool g_IsNativeClientVGUI2 = false;
IClientVGUI* g_pClientVGUI = NULL;

static void (__fastcall *m_pfnCClientVGUI_Initialize)(void *pthis, int,CreateInterfaceFn *factories, int count) = NULL;
static void (__fastcall *m_pfnCClientVGUI_Start)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_SetParent)(void *pthis, int, vgui::VPANEL parent) = NULL;
static bool (__fastcall *m_pfnCClientVGUI_UseVGUI1)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideScoreBoard)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideAllVGUIMenu)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_ActivateClientUI)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideClientUI)(void *pthis, int) = NULL;

class CClientVGUIProxy : public IClientVGUI
{
public:
	void Initialize(CreateInterfaceFn *factories, int count) override;
	void Start(void) override;
	void SetParent(vgui::VPANEL parent) override;
	bool UseVGUI1(void) override;
	void HideScoreBoard(void) override;
	void HideAllVGUIMenu(void) override;
	void ActivateClientUI(void) override;
	void HideClientUI(void) override;
	void Unknown(void) override;
	void Shutdown(void) override;
};

static CClientVGUIProxy s_ClientVGUIProxy;

void CClientVGUIProxy::Initialize(CreateInterfaceFn *factories, int count)
{
	m_pfnCClientVGUI_Initialize(this, 0, factories, count);

	if (!vgui::VGui_InitInterfacesList("VGUI2Extension", factories, count))
	{
		Sys_Error("Failed to VGui_InitInterfacesList");
		return;
	}

	VGUI2ExtensionInternal()->ClientVGUI_Initialize(factories, count);
}

void CClientVGUIProxy::Start(void)
{
	m_pfnCClientVGUI_Start(this, 0);

	VGUI2ExtensionInternal()->ClientVGUI_Start();
}

void CClientVGUIProxy::SetParent(vgui::VPANEL parent)
{
	m_pfnCClientVGUI_SetParent(this, 0, parent);

	VGUI2ExtensionInternal()->ClientVGUI_SetParent(parent);
}

bool CClientVGUIProxy::UseVGUI1(void)
{
	bool fake_ret = false;
	bool real_ret = false;
	bool ret = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = m_pfnCClientVGUI_UseVGUI1(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
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

void CClientVGUIProxy::HideScoreBoard(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_HideScoreBoard(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);
	}
}

void CClientVGUIProxy::HideAllVGUIMenu(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_HideAllVGUIMenu(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);
	}
}

void CClientVGUIProxy::ActivateClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_ActivateClientUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);
	}
}

void CClientVGUIProxy::HideClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_HideClientUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);
	}
}

void CClientVGUIProxy::Unknown(void)
{

}

void CClientVGUIProxy::Shutdown(void)
{

}

//Implement the ClientVGUI interface for those mod with no ClientVGUI implemented

class NewClientVGUI : public IClientVGUI
{
public:
	void Initialize(CreateInterfaceFn *factories, int count) override;
	void Start(void) override;
	void SetParent(vgui::VPANEL parent) override;
	bool UseVGUI1(void) override;
	void HideScoreBoard(void) override;
	void HideAllVGUIMenu(void) override;
	void ActivateClientUI(void) override;
	void HideClientUI(void) override;
	void Unknown(void) override;
	void Shutdown(void) override;
};

void NewClientVGUI::Initialize(CreateInterfaceFn *factories, int count)
{
	if (!vgui::VGui_InitInterfacesList("VGUI2Extension", factories, count))
	{
		Sys_Error("Failed to VGui_InitInterfacesList");
		return;
	}

	VGUI2ExtensionInternal()->ClientVGUI_Initialize(factories, count);
}

void NewClientVGUI::Start(void)
{
	VGUI2ExtensionInternal()->ClientVGUI_Start();

	//TODO: Need to fix for HL25?
	if (g_pSurface)
	{
		//Fix a bug that VGUI1 mouse disappear
		auto pSurface4 = (DWORD)g_pSurface + 4;
		*(PUCHAR)(pSurface4 + 0x4B) = 0;
	}
}

void NewClientVGUI::SetParent(vgui::VPANEL parent)
{
	VGUI2ExtensionInternal()->ClientVGUI_SetParent(parent);
}

bool NewClientVGUI::UseVGUI1(void)
{
	bool fake_ret = false;
	bool real_ret = false;
	bool ret = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = true;
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
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

void NewClientVGUI::HideScoreBoard(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);
	}
}

void NewClientVGUI::HideAllVGUIMenu(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);
	}
}

void NewClientVGUI::ActivateClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);
	}
}

void NewClientVGUI::HideClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{

	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);
	}
}

void NewClientVGUI::Unknown(void)
{

}

void NewClientVGUI::Shutdown(void)
{
	
}

EXPOSE_SINGLE_INTERFACE(NewClientVGUI, IClientVGUI, CLIENTVGUI_INTERFACE_VERSION);

/*
	Purpose : Install hooks for ClientVGUI interface
*/

void ClientVGUI_InstallHooks(cl_exportfuncs_t* pExportFunc)
{
	CreateInterfaceFn ClientVGUICreateInterface = NULL;

	if (g_hClientDll)
	{
		ClientVGUICreateInterface = (CreateInterfaceFn)Sys_GetFactory((HINTERFACEMODULE)g_hClientDll);
	}

	if (!ClientVGUICreateInterface && gExportfuncs.ClientFactory)
	{
		ClientVGUICreateInterface = (CreateInterfaceFn)gExportfuncs.ClientFactory();
	}

	if (ClientVGUICreateInterface)
	{
		g_pClientVGUI = (IClientVGUI *)ClientVGUICreateInterface(CLIENTVGUI_INTERFACE_VERSION, NULL);

		if (g_pClientVGUI)
		{
			PVOID* pVFTable = *(PVOID**)&s_ClientVGUIProxy;

			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 1, (void*)pVFTable[1], (void**)&m_pfnCClientVGUI_Initialize);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 2, (void*)pVFTable[2], (void**)&m_pfnCClientVGUI_Start);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 3, (void*)pVFTable[3], (void**)&m_pfnCClientVGUI_SetParent);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 4, (void*)pVFTable[4], (void**)&m_pfnCClientVGUI_UseVGUI1);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 5, (void*)pVFTable[5], (void**)&m_pfnCClientVGUI_HideScoreBoard);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 6, (void*)pVFTable[6], (void**)&m_pfnCClientVGUI_HideAllVGUIMenu);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 7, (void*)pVFTable[7], (void**)&m_pfnCClientVGUI_ActivateClientUI);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 8, (void*)pVFTable[8], (void**)&m_pfnCClientVGUI_HideClientUI);

			g_IsNativeClientVGUI2 = true;
		}
	}

	if (!g_IsNativeClientVGUI2)
	{
		pExportFunc->ClientFactory = NewClientFactory;
	}
}

PVOID VGUIClient001_CreateInterface(HINTERFACEMODULE hModule)
{
	if (hModule == (HINTERFACEMODULE)g_hClientDll && !g_IsNativeClientVGUI2)
	{
		return NewCreateInterface;
	}

	return Sys_GetFactory(hModule);
}
