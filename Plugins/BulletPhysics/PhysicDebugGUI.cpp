#include "PhysicDebugGUI.h"
#include <vgui_controls/TextImage.h>

#include "PhysicEditorDialog.h"

#include "ClientEntityManager.h"
#include "Viewport.h"
#include "exportfuncs.h"


CPhysicDebugGUI::CPhysicDebugGUI(vgui::Panel* parent) : Frame(parent, "PhysicDebugGUI")
{
	SetVisible(false);
	SetProportional(true);

	SetScheme2("ClientScheme");
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	m_pTopBar = new vgui::Panel(this, "TopBar");
	m_pBottomBarBlank = new vgui::Panel(this, "BottomBarBlank");

	m_pInspectEntityLabel = new vgui::Label(this, "InspectEntityLabel", "");
	m_pInspectEntityLabel->SetVisible(false);

	m_pInspectPhysicComponentLabel = new vgui::Label(this, "InspectPhysicComponentLabel", "");
	m_pInspectPhysicComponentLabel->SetVisible(false);

	m_pClose = new vgui::Button(this, "Close", L"#GameUI_Close", this, "Close");

	m_pContextMenu_CreatePhysicObject = new vgui::Menu(this, "contextmenu");
	m_pContextMenu_CreatePhysicObject->AddMenuItem("#BulletPhysics_CreateRagdollObject", new KeyValues("Command", "command", "CreateRagdollObject"), this);
	m_pContextMenu_CreatePhysicObject->AddMenuItem("#BulletPhysics_CreateStaticObject", new KeyValues("Command", "command", "CreateStaticObject"), this);
	m_pContextMenu_CreatePhysicObject->AddMenuItem("#BulletPhysics_CreateDynamicObject", new KeyValues("Command", "command", "CreateDynamicObject"), this);

	//m_pContextMenu->AddMenuItem("#BulletPhysics_EditRigidBody", new KeyValues("Command", "command", "EditRigidBody"), this);

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	LoadControlSettings("bulletphysics/PhysicDebugGUI.res", "GAME");
	InvalidateLayout();
}

CPhysicDebugGUI::~CPhysicDebugGUI()
{
	m_pContextMenu_CreatePhysicObject->MarkForDeletion();
}

void CPhysicDebugGUI::OnMousePressed(vgui::MouseCode code)
{
	if (code == vgui::MOUSE_RIGHT)
	{
		if (g_pViewPort->GetInspectEntityIndex() > 0 && g_pViewPort->GetInspectEntityModelIndex() > 0 && g_pViewPort->GetInspectPhysicComponentId() == 0)
		{
			auto model = EngineGetModelByIndex(g_pViewPort->GetInspectEntityModelIndex());

			if (model && model->type == mod_studio)
			{
				vgui::Menu::PlaceContextMenu(this, m_pContextMenu_CreatePhysicObject);
			}

		}
		return;
	}

	BaseClass::OnMousePressed(code);
}

void CPhysicDebugGUI::SetInspectEntityLabelText(const wchar_t* wszText)
{
	m_pInspectEntityLabel->SetText(wszText);
}

void CPhysicDebugGUI::ShowInspectEntityLabel(bool bVisible)
{
	m_pInspectEntityLabel->SetVisible(bVisible);
}

void CPhysicDebugGUI::SetInspectPhysicComponentLabelText(const wchar_t* wszText)
{
	m_pInspectPhysicComponentLabel->SetText(wszText);
}

void CPhysicDebugGUI::ShowInspectPhysicComponentLabel(bool bVisible)
{
	m_pInspectPhysicComponentLabel->SetVisible(bVisible);
}

void CPhysicDebugGUI::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	m_pBottomBarBlank->SetVisible(true);
	m_pTopBar->SetVisible(true);

	BaseClass::ApplySchemeSettings(pScheme);

	m_pTopBar->SetBgColor(Color(0, 0, 0, 196));
	m_pBottomBarBlank->SetBgColor(Color(0, 0, 0, 196));

	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBorderEnabled(false);

	SetBorder(NULL);

	m_pInspectEntityLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));
	m_pInspectPhysicComponentLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));

	m_pInspectEntityLabel->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectPhysicComponentLabel->SetFgColor(Color(255, 255, 255, 255));

	m_pInspectEntityLabel->SetPaintBackgroundEnabled(false);
	m_pInspectPhysicComponentLabel->SetPaintBackgroundEnabled(false);
}

void CPhysicDebugGUI::PerformLayout(void)
{
	int screenWidth, screenHeight;
	vgui::surface()->GetScreenSize(screenWidth, screenHeight);
	SetBounds(0, 0, screenWidth, screenHeight);

	int w, h, x, y;
	m_pBottomBarBlank->GetPos(x, y);
	m_pBottomBarBlank->SetSize(screenWidth, screenHeight - y);

	m_pTopBar->GetSize(w, h);
	m_pTopBar->SetSize(screenWidth, h);
}

void CPhysicDebugGUI::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		
	}
	else if (!stricmp(command, "EditRigidBody"))
	{
		BaseClass::OnCommand(command);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}