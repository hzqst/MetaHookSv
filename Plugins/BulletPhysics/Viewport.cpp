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

using namespace vgui;

CViewport *g_pViewPort = NULL;

extern IGameUIFuncs* gameuifuncs;

CViewport::CViewport() : BaseClass(NULL, "BulletPhysicsViewport")
{
	int swide, stall;
	surface()->GetScreenSize(swide, stall);

	MakePopup(false, true);

	SetScheme2("ClientScheme");//BulletPhysicsScheme ?
	SetBounds(0, 0, swide, stall);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(true);
}

CViewport::~CViewport(void)
{

}

void CViewport::Start(void)
{


	SetVisible(false);
}

void CViewport::SetParent(VPANEL vPanel)
{
	BaseClass::SetParent(vPanel);

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

}

void CViewport::ConnectToServer(const char* game, int IP, int port)
{


}

void CViewport::ActivateClientUI(void)
{
	SetVisible(true);
}

void CViewport::HideClientUI(void)
{
	SetVisible(false);
}

void CViewport::Paint(void)
{
	BaseClass::Paint();

}