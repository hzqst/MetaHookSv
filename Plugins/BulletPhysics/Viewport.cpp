#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/IEngineVGui.h>
#include <vgui/IGameUIFuncs.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <IVGUI2Extension.h>
#include <IDpiManager.h>
#include "Viewport.h"
#include "message.h"
#include "plugins.h"
#include "privatehook.h"
#include "exportfuncs.h"
#include "util.h"

#include "PhysicDebugGUI.h"

#include "ClientEntityManager.h"
#include "ClientPhysicManager.h"

using namespace vgui;

CViewport *g_pViewPort = NULL;

extern IGameUIFuncs* gameuifuncs;

CViewport::CViewport() : BaseClass(NULL, "BulletPhysicsViewport")
{
	int swide, stall;
	surface()->GetScreenSize(swide, stall);

	MakePopup(false, true);

	SetScheme2("ClientScheme");
	SetBounds(0, 0, swide, stall);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(true);
}

CViewport::~CViewport(void)
{
	if (m_pPhysicDebugViewGUI)
	{
		delete m_pPhysicDebugViewGUI;
		m_pPhysicDebugViewGUI = nullptr;
	}
}

void CViewport::Start(void)
{
	m_pPhysicDebugViewGUI = new CPhysicDebugGUI(NULL);

	SetVisible(false);
}

void CViewport::SetParent(VPANEL vPanel)
{
	BaseClass::SetParent(vPanel);

	m_pPhysicDebugViewGUI->SetParent(this);

	if (g_iEngineType != ENGINE_GOLDSRC_HL25 && DpiManager()->IsHighDpiSupportEnabled())
	{
		SetProportional(true);
	}
}

void CViewport::Think(void)
{
	
}

void CViewport::VidInit(void)
{

}

void CViewport::Init(void)
{
	m_pPhysicDebugViewGUI->Init();
}

void CViewport::NewMap(void)
{
	m_pPhysicDebugViewGUI->NewMap();
}

void CViewport::OpenPhysicDebugGUI()
{
	m_pPhysicDebugViewGUI->Activate();
}

void CViewport::ClosePhysicDebugGUI()
{
	m_pPhysicDebugViewGUI->Close();
}

void CViewport::SwitchPhysicDebugGUI()
{
	if(m_pPhysicDebugViewGUI->IsVisible())
		m_pPhysicDebugViewGUI->Close();
	else
		m_pPhysicDebugViewGUI->Activate();
}

bool CViewport::PhysicDebugGUIHasFocus()
{
	if (m_pPhysicDebugViewGUI->IsVisible() && m_pPhysicDebugViewGUI->HasFocus())
	{
		return true;
	}

	return false;
}

void CViewport::ActivateClientUI(void)
{
	SetVisible(true);

	if (m_hOldFocus)
	{
		vgui::input()->SetAppModalSurface(m_hOldFocus);
		vgui::input()->SetMouseFocus(m_hOldFocus);
		m_hOldFocus = NULL;
	}
}

void CViewport::HideClientUI(void)
{
	m_hOldFocus = vgui::input()->GetAppModalSurface();

	SetVisible(false);
}

void CViewport::UpdateInspectStuffs()
{
	m_pPhysicDebugViewGUI->UpdateInspectStuffs();
}