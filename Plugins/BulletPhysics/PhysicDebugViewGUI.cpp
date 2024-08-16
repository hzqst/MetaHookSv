#include "PhysicDebugViewGUI.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/TextImage.h>

CPhysicDebugViewGUI::CPhysicDebugViewGUI(vgui::Panel* parent) : EditablePanel(parent, "PhysicDebugViewGUI")
{
	SetVisible(false);
	SetProportional(true);
	SetCursor(vgui::dc_none);

	SetScheme2("ClientScheme");
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);

	m_pTopBar = new Panel(this, "TopBar");
	m_pBottomBarBlank = new Panel(this, "BottomBarBlank");

	m_pInspectEntityLabel = new vgui::Label(this, "InspectEntityLabel", "");
	m_pInspectEntityLabel->SetVisible(false);

	m_pInspectPhysicComponentLabel = new vgui::Label(this, "InspectPhysicComponentLabel", "");
	m_pInspectPhysicComponentLabel->SetVisible(false);
	
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	LoadControlSettings("bulletphysics/PhysicDebugViewGUI.res", "GAME");
	InvalidateLayout();
}

CPhysicDebugViewGUI::~CPhysicDebugViewGUI()
{

}

void CPhysicDebugViewGUI::SetInspectEntityLabelText(const wchar_t* wszText)
{
	m_pInspectEntityLabel->SetText(wszText);
}

void CPhysicDebugViewGUI::ShowInspectEntityLabel(bool bVisible)
{
	m_pInspectEntityLabel->SetVisible(bVisible);
}

void CPhysicDebugViewGUI::SetInspectPhysicComponentLabelText(const wchar_t* wszText)
{
	m_pInspectPhysicComponentLabel->SetText(wszText);
}

void CPhysicDebugViewGUI::ShowInspectPhysicComponentLabel(bool bVisible)
{
	m_pInspectPhysicComponentLabel->SetVisible(bVisible);
}

void CPhysicDebugViewGUI::ApplySchemeSettings(vgui::IScheme* pScheme)
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

void CPhysicDebugViewGUI::PerformLayout(void)
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