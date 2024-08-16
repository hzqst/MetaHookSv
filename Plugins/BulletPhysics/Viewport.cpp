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

#include "PhysicEditorDialog.h"
#include "PhysicDebugViewGUI.h"

#include "CounterStrike.h"

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
	if (m_pPhysicEditorDialog)
	{
		delete m_pPhysicEditorDialog;
		m_pPhysicEditorDialog = nullptr;
	}
	if (m_pPhysicDebugViewGUI)
	{
		delete m_pPhysicDebugViewGUI;
		m_pPhysicDebugViewGUI = nullptr;
	}
}

void CViewport::Start(void)
{
	m_pPhysicDebugViewGUI = new CPhysicDebugViewGUI(this);

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
	if (IsDebugViewModeEnabled() && !m_pPhysicDebugViewGUI->IsVisible())
	{
		m_pPhysicDebugViewGUI->SetVisible(true);
	}
	else if (!IsDebugViewModeEnabled() && m_pPhysicDebugViewGUI->IsVisible())
	{
		m_pPhysicDebugViewGUI->SetVisible(false);
	}
}

void CViewport::VidInit(void)
{

}

void CViewport::Init(void)
{

}

void CViewport::NewMap(void)
{
	m_iCachedInspectEntity = 0;
	m_iCachedInspectModel = nullptr;
	m_iCachedInspectPhysicComponentId = 0;
}

void CViewport::UpdateInspectEntity(int entindex)
{
	model_t* model = nullptr;

	if (entindex > 0)
	{
		auto pClientEntity = ClientEntityManager()->GetEntityByIndex(entindex);

		if (pClientEntity && pClientEntity->model)
		{
			model = pClientEntity->model;
			
			if (ClientEntityManager()->IsEntityDeadPlayer(pClientEntity))
			{
				auto playerindex = pClientEntity->curstate.renderamt;
				auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

				if (g_bIsCounterStrike)
				{
					//Counter-Strike redirects playermodel in a pretty tricky way
					int modelindex = 0;
					model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
				}
			}
			else if (ClientEntityManager()->IsEntityPlayer(pClientEntity))
			{
				auto playerindex = pClientEntity->curstate.number;
				model = IEngineStudio.SetupPlayerModel(playerindex - 1);

				if (g_bIsCounterStrike)
				{
					//Counter-Strike redirects playermodel in a pretty tricky way
					int modelindex = 0;
					model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
				}
			}

		}
	}

	if (m_iCachedInspectEntity != entindex || m_iCachedInspectModel != model)
	{
		m_iCachedInspectEntity = entindex;
		m_iCachedInspectModel = model;

		if (entindex > 0 && model)
		{
			wchar_t wszIndex[32] = { 0 };
			wchar_t wszModel[64] = { 0 };
			wchar_t wszBuf[512] = { 0 };

			swprintf_s(wszIndex, L"%d", entindex);

			vgui::localize()->ConvertANSIToUnicode(model->name, wszModel, sizeof(wszModel));

			auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);

			if (pPhysicObject)
			{
				vgui::localize()->ConstructString(wszBuf, sizeof(wszBuf), vgui::localize()->Find("#BulletPhysics_InspectingPhysicObject"), 3, vgui::localize()->Find(pPhysicObject->GetTypeLocalizationTokenString()), wszIndex,  wszModel);
			}
			else
			{
				vgui::localize()->ConstructString(wszBuf, sizeof(wszBuf), vgui::localize()->Find("#BulletPhysics_InspectingEntity"), 2, wszIndex, wszModel);
			}

			m_pPhysicDebugViewGUI->SetInspectEntityLabelText(wszBuf);
			m_pPhysicDebugViewGUI->ShowInspectEntityLabel(true);
		}
		else
		{
			m_pPhysicDebugViewGUI->SetInspectEntityLabelText(L"");
			m_pPhysicDebugViewGUI->ShowInspectEntityLabel(false);
		}
	}
}

void CViewport::UpdateInspectPhysicComponent(int physicComponentId)
{
	if (m_iCachedInspectPhysicComponentId != physicComponentId)
	{
		m_iCachedInspectPhysicComponentId = physicComponentId;

		auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

		if (pPhysicComponent)
		{
			wchar_t wszIndex[32] = { 0 };
			wchar_t wszName[64] = { 0 };
			wchar_t wszBuf[512] = { 0 };

			swprintf_s(wszIndex, L"%d", physicComponentId);

			vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), wszName, sizeof(wszName));

			vgui::localize()->ConstructString(wszBuf, sizeof(wszBuf), vgui::localize()->Find("#BulletPhysics_InspectingPhysicComponent"), 3, vgui::localize()->Find(pPhysicComponent->GetTypeLocalizationTokenString()), wszIndex, wszName);

			m_pPhysicDebugViewGUI->SetInspectPhysicComponentLabelText(wszBuf);
			m_pPhysicDebugViewGUI->ShowInspectPhysicComponentLabel(true);
		}
		else
		{
			m_pPhysicDebugViewGUI->SetInspectPhysicComponentLabelText(L"");
			m_pPhysicDebugViewGUI->ShowInspectPhysicComponentLabel(false);
		}
	}
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