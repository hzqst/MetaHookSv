#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <VGUI_controls/Frame.h>
#include <IVGUI2Extension.h>
#include "plugins.h"
#include "Viewport.h"

namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

class CVGUI2Extension_ClientVGUICallbacks : public IVGUI2Extension_ClientVGUICallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		if (!vgui::VGui_InitInterfacesList("BulletPhysics", factories, count))
		{
			Sys_Error("Failed to VGui_InitInterfacesList");
			return;
		}

		if (!vgui::localize()->AddFile(g_pFullFileSystem, "bulletphysics/bulletphysics_%language%.txt"))
		{
			if (!vgui::localize()->AddFile(g_pFullFileSystem, "bulletphysics/bulletphysics_english.txt"))
			{
				Sys_Error("Failed to load \"bulletphysics/bulletphysics_english.txt\"");
			}
		}
	}

	void Shutdown() override
	{
		if (g_pViewPort)
		{
			delete g_pViewPort;
			g_pViewPort = NULL;
		}
	}

	void Start() override
	{
		g_pViewPort = new CViewport();
		g_pViewPort->Start();
	}

	void SetParent(vgui::VPANEL parent) override
	{
		g_pViewPort->SetParent(parent);
	}

	void UseVGUI1(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideScoreBoard(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideAllVGUIMenu(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ActivateClientUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		g_pViewPort->ActivateClientUI();
	}

	void HideClientUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		g_pViewPort->HideClientUI();
	}
};

static CVGUI2Extension_ClientVGUICallbacks s_ClientVGUICallbacks;

void ClientVGUI_InstallHooks(void)
{
	VGUI2Extension()->RegisterClientVGUICallbacks(&s_ClientVGUICallbacks);
}

void ClientVGUI_UninstallHooks(void)
{
	VGUI2Extension()->UnregisterClientVGUICallbacks(&s_ClientVGUICallbacks);
}