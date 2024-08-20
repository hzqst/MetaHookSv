#include "PhysicDebugGUI.h"
#include <vgui_controls/TextImage.h>

#include "PhysicEditorDialog.h"

#include "CounterStrike.h"

#include "ClientEntityManager.h"
#include "ClientPhysicManager.h"

#include "privatehook.h"
#include "exportfuncs.h"
#include "util.h"

#include <format>

CPhysicDebugGUI::CPhysicDebugGUI(vgui::Panel* parent) : Frame(parent, "PhysicDebugGUI")
{
	SetVisible(false);
	SetProportional(true);

	SetScheme2("ClientScheme");
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	m_pTopBar = new vgui::Panel(this, "TopBar");
	m_pBottomBarBlank = new vgui::Panel(this, "BottomBarBlank");

	m_pInspectContentLabel = new vgui::Label(this, "InspectContentLabel", "");
	m_pInspectContentLabel2 = new vgui::Label(this, "InspectContentLabel2", "");
	m_pInspectModeLabel = new vgui::Label(this, "InspectModeLabel", "");

	m_pClose = new vgui::Button(this, "Close", L"#GameUI_Close", this, "Close");

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	LoadControlSettings("bulletphysics/PhysicDebugGUI.res", "GAME");
}

CPhysicDebugGUI::~CPhysicDebugGUI()
{

}

void CPhysicDebugGUI::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_F1)
	{
		UpdateInspectMode(PhysicInspectMode::Entity);
		return;
	}

	if (code == vgui::KEY_F2)
	{
		UpdateInspectMode(PhysicInspectMode::PhysicObject);
		return;
	}

	if (code == vgui::KEY_F3)
	{
		UpdateInspectMode(PhysicInspectMode::RigidBody);
		return;
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicDebugGUI::Init()
{
	Reset();
}

void CPhysicDebugGUI::NewMap()
{
	Reset();
}

void CPhysicDebugGUI::UpdateInspectStuffs()
{
	ClientEntityManager()->InspectEntityByIndex(0);
	ClientPhysicManager()->InspectPhysicObject(0);
	ClientPhysicManager()->InspectPhysicComponent(0);

	if (AllowCheats() && HasFocus())
	{
		int mouseX{}, mouseY{};
		gEngfuncs.GetMousePosition(&mouseX, &mouseY);

		SCREENINFO_s scr{};
		scr.iSize = sizeof(SCREENINFO_s);

		if (gEngfuncs.pfnGetScreenInfo(&scr) && scr.iWidth > 0 && scr.iHeight > 0)
		{
			vec3_t vecForward, vecRight, vecUp, vecTarget, vecScreen;

			vecScreen[0] = UNPROJECT_X(mouseX, scr.iWidth);
			vecScreen[1] = UNPROJECT_Y(mouseY, scr.iHeight);
			vecScreen[2] = 1;

			gEngfuncs.pTriAPI->ScreenToWorld(vecScreen, vecTarget);

			VectorSubtract(vecTarget, r_vieworg, vecForward);
			VectorNormalize(vecForward);
			VectorMA(r_vieworg, 4096, vecForward, vecTarget);

			if (m_InspectMode == PhysicInspectMode::Entity)
			{
				auto trace = gEngfuncs.PM_TraceLine(r_vieworg, vecTarget, PM_TRACELINE_PHYSENTSONLY, 2, -1);

				if (trace->fraction != 1.0 && trace->ent)
				{
					auto physent = gEngfuncs.pEventAPI->EV_GetPhysent(trace->ent);

					if (physent)
					{
						ClientEntityManager()->InspectEntityByIndex(physent->info);
					}
				}
			}
			else
			{
				CPhysicTraceLineHitResult hitResult;
				ClientPhysicManager()->TraceLine(r_vieworg, vecTarget, hitResult);

				if (hitResult.m_bHasHit && hitResult.m_iHitPhysicComponentIndex > 1)//1 == world
				{
					if (m_InspectMode == PhysicInspectMode::PhysicObject)
					{
						auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(hitResult.m_iHitEntityIndex);

						if (pPhysicObject)
						{
							auto model = pPhysicObject->GetModel();
							auto modelindex = EngineGetModelIndex(model);

							auto pPhysicObjectConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

							if (pPhysicObjectConfig && (pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
							{
								uint64 physicObjectId = PACK_PHYSIC_OBJECT_ID(hitResult.m_iHitEntityIndex, modelindex);

								ClientPhysicManager()->InspectPhysicObject(physicObjectId);
							}
						}
					}
					else
					{
						ClientPhysicManager()->InspectPhysicComponent(hitResult.m_iHitPhysicComponentIndex);
					}
				}
			}
		}
	}
}

void CPhysicDebugGUI::Reset()
{
	UpdateInspectMode(PhysicInspectMode::PhysicObject);
	UpdateEditMode(PhysicEditMode::None);

	SetVisible(false);
}

void CPhysicDebugGUI::OnThink()
{
	BaseClass::OnThink();

	HideInspectContentLabel();
	HideInspectContentLabel2();

	switch (m_InspectMode)
	{
	case PhysicInspectMode::Entity:
	{
		UpdateInspectClientEntity();
		break;
	}
	case PhysicInspectMode::PhysicObject:
	{
		UpdateInspectPhysicObject();
		break;
	}
	case PhysicInspectMode::RigidBody:
	{
		UpdateInspectRigidBody();
		break;
	}
	}
}

void CPhysicDebugGUI::UpdateInspectMode(PhysicInspectMode mode)
{
	m_InspectMode = mode;

	if (m_InspectMode == PhysicInspectMode::Entity)
	{
		m_pInspectModeLabel->SetText("#BulletPhysics_InspectMode_Entity");
	}
	else if (m_InspectMode == PhysicInspectMode::PhysicObject)
	{
		m_pInspectModeLabel->SetText("#BulletPhysics_InspectMode_PhysicObject");
	}
	else if (m_InspectMode == PhysicInspectMode::RigidBody)
	{
		m_pInspectModeLabel->SetText("#BulletPhysics_InspectMode_RigidBody");
	}
}

void CPhysicDebugGUI::UpdateEditMode(PhysicEditMode mode)
{
	m_EditMode = mode;
}

void CPhysicDebugGUI::OnMousePressed(vgui::MouseCode code)
{
	if (code == vgui::MOUSE_RIGHT)
	{
		if (m_InspectMode == PhysicInspectMode::Entity)
		{
			int entindex = ClientEntityManager()->GetInspectEntityIndex();
			int modelindex = ClientEntityManager()->GetInspectEntityModelIndex();

			if (entindex > 0 && modelindex > 0)
			{
				auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);
				auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

				if (!pPhysicObject && !pPhysicConfig)
				{
					auto model = EngineGetModelByIndex(modelindex);

					if (model && model->type == mod_studio)
					{
						auto menu = new vgui::Menu(this, "contextmenu");

						menu->SetAutoDelete(true);

						auto kv = new KeyValues("CreatePhysicObject");
						kv->SetUint64("physicObjectId", PACK_PHYSIC_OBJECT_ID(entindex, modelindex));

						menu->AddMenuItem("#BulletPhysics_CreatePhysicObject", kv, this);

						vgui::Menu::PlaceContextMenu(this, menu);
						return;
					}
				}
			}
		}
		else if (m_InspectMode == PhysicInspectMode::PhysicObject)
		{
			uint64 physicObjectId = ClientPhysicManager()->GetInspectingPhysicObjectId();

			if (physicObjectId)
			{
				auto menu = new vgui::Menu(this, "contextmenu");

				menu->SetAutoDelete(true);

				auto kv = new KeyValues("EditPhysicObject");
				kv->SetUint64("physicObjectId", physicObjectId);

				menu->AddMenuItem("#BulletPhysics_EditPhysicObject", kv, this);

				vgui::Menu::PlaceContextMenu(this, menu);
				return;
			}
		}
		else if (m_InspectMode == PhysicInspectMode::RigidBody)
		{
			int physicComponentId = ClientPhysicManager()->GetInspectingPhysicComponentId();

			if (physicComponentId > 0)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent && pPhysicComponent->IsRigidBody())
				{
					int entindex = pPhysicComponent->GetOwnerEntityIndex();

					auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);

					if (pPhysicObject)
					{
						int modelindex = EngineGetModelIndex(pPhysicObject->GetModel());

						auto physicObjectId = PACK_PHYSIC_OBJECT_ID(entindex, modelindex);

						auto menu = new vgui::Menu(this, "contextmenu");

						menu->SetAutoDelete(true);

						wchar_t szName[64] = { 0 };
						wchar_t szBuf[256] = { 0 };

						vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), szName, sizeof(szName));

						auto kv = new KeyValues("EditRigidBodyEx");
						kv->SetInt("configId", physicComponentId);
						kv->SetUint64("physicObjectId", physicObjectId);
						vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditRigidBody"), 1, szName);
						menu->AddMenuItem("EditRigidBodyEx", szBuf, kv, this);

						kv = new KeyValues("MoveRigidBodyEx");
						kv->SetInt("configId", physicComponentId);
						kv->SetUint64("physicObjectId", physicObjectId);
						vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_MoveRigidBody"), 1, szName);
						menu->AddMenuItem("MoveRigidBodyEx", szBuf, kv, this);

						kv = new KeyValues("RotateRigidBodyEx");
						kv->SetInt("configId", physicComponentId);
						kv->SetUint64("physicObjectId", physicObjectId);
						vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_RotateRigidBody"), 1, szName);
						menu->AddMenuItem("RotateRigidBodyEx", szBuf, kv, this);

						kv = new KeyValues("DeleteRigidBodyEx");
						kv->SetInt("configId", physicComponentId);
						kv->SetUint64("physicObjectId", physicObjectId);
						vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteRigidBody"), 1, szName);
						menu->AddMenuItem("DeleteRigidBodyEx", szBuf, kv, this);

						vgui::Menu::PlaceContextMenu(this, menu);
					}
				}
			}
		}
	}

	BaseClass::OnMousePressed(code);
}

void CPhysicDebugGUI::OnMouseDoublePressed(vgui::MouseCode code)
{
	if (code == vgui::MOUSE_LEFT)
	{
		if (m_InspectMode == PhysicInspectMode::PhysicObject)
		{
			uint64 physicObjectId = ClientPhysicManager()->GetInspectingPhysicObjectId();

			if (physicObjectId)
			{
				OpenEditPhysicObjectDialog(physicObjectId);
				return;
			}
		}
		else if (m_InspectMode == PhysicInspectMode::RigidBody)
		{
			int physicComponentId = ClientPhysicManager()->GetInspectingPhysicComponentId();

			if (physicComponentId)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent && pPhysicComponent->IsRigidBody())
				{
					int entindex = pPhysicComponent->GetOwnerEntityIndex();

					auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);

					if (pPhysicObject)
					{
						int modelindex = EngineGetModelIndex(pPhysicObject->GetModel());

						auto physicObjectId = PACK_PHYSIC_OBJECT_ID(entindex, modelindex);

						OpenEditRigidBodyDialog(pPhysicComponent->GetPhysicConfigId(), physicObjectId);
						return;
					}
				}
			}
		}
	}

	BaseClass::OnMouseDoublePressed(code);
}

void CPhysicDebugGUI::UpdateInspectClientEntity()
{
	int entindex = ClientEntityManager()->GetInspectEntityIndex();
	int modelindex = ClientEntityManager()->GetInspectEntityModelIndex();
	int playerindex = 0;

	auto ent = ClientEntityManager()->GetEntityByIndex(entindex);
	auto model = EngineGetModelByIndex(modelindex);

	if (entindex > 0 && ent && model)
	{
		wchar_t wszModelName[64] = { 0 };

		vgui::localize()->ConvertANSIToUnicode(model->name, wszModelName, sizeof(wszModelName));

		auto str = std::format(L"{0} (#{1}): {2}", vgui::localize()->Find("#BulletPhysics_Entity"), entindex, wszModelName);

		ShowInspectContentLabel(str.c_str());

		auto curstate = &ent->curstate;

		if (ClientEntityManager()->IsEntityDeadPlayer(ent))
		{
			playerindex = ent->curstate.renderamt;
			curstate = R_GetPlayerState(playerindex);
		}
		else if (ClientEntityManager()->IsEntityPlayer(ent))
		{
			playerindex = ent->curstate.number;
			curstate = R_GetPlayerState(playerindex);
		}

		auto str2 = std::format(L"{0}: {1}, {2}: {3}, {4}: {5}", 
			vgui::localize()->Find("#BulletPhysics_Sequence"), curstate->sequence, 
			vgui::localize()->Find("#BulletPhysics_GaitSequence"), curstate->gaitsequence,
			vgui::localize()->Find("#BulletPhysics_Frame"), curstate->frame);

		ShowInspectContentLabel2(str2.c_str());
	}
}

void CPhysicDebugGUI::UpdateInspectPhysicObject()
{
	uint64 physicObjectId = ClientPhysicManager()->GetInspectingPhysicObjectId();

	if (physicObjectId)
	{
		auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

		if (pPhysicObject)
		{
			wchar_t wszModelName[64] = { 0 };

			auto entindex = UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(physicObjectId);

			vgui::localize()->ConvertANSIToUnicode(pPhysicObject->GetModel()->name, wszModelName, sizeof(wszModelName));

			auto str = std::format(L"{0} (#{1}): {2}", vgui::localize()->Find("#BulletPhysics_PhysicObject"), entindex, wszModelName);

			ShowInspectContentLabel(str.c_str());
		}
	}
}

void CPhysicDebugGUI::UpdateInspectRigidBody()
{
	int physicComponentId = ClientPhysicManager()->GetInspectingPhysicComponentId();

	if (physicComponentId)
	{
		auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

		if (pPhysicComponent && pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			wchar_t wszName[64] = { 0 };

			vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), wszName, sizeof(wszName));

			auto str = std::format(L"{0} (#{1}): {2}", vgui::localize()->Find("#BulletPhysics_RigidBody"), pPhysicComponent->GetPhysicComponentId(), wszName);

			ShowInspectContentLabel(str.c_str());

			auto str2 = std::format(L"{0}: {1}", vgui::localize()->Find("#BulletPhysics_Mass"), pRigidBody->GetMass());

			ShowInspectContentLabel2(str2.c_str());

		}
	}
}

void CPhysicDebugGUI::ShowInspectContentLabel(const wchar_t* wszText)
{
	m_pInspectContentLabel->SetText(wszText);
	m_pInspectContentLabel->SetVisible(true);
}

void CPhysicDebugGUI::HideInspectContentLabel()
{
	m_pInspectContentLabel->SetVisible(false);
}

void CPhysicDebugGUI::ShowInspectContentLabel2(const wchar_t* wszText)
{
	m_pInspectContentLabel2->SetText(wszText);
	m_pInspectContentLabel2->SetVisible(true);
}

void CPhysicDebugGUI::HideInspectContentLabel2()
{
	m_pInspectContentLabel2->SetVisible(false);
}

bool CPhysicDebugGUI::HasFocus()
{
	if (BaseClass::HasFocus())
	{
		return true;
	}
	if (m_pClose->HasFocus())
	{
		return true;
	}
	if (m_pTopBar->HasFocus())
	{
		return true;
	}
	if (m_pBottomBarBlank->HasFocus())
	{
		return true;
	}

	return false;
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

	m_pInspectContentLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));
	m_pInspectContentLabel2->SetFont(pScheme->GetFont("EngineFont", IsProportional()));
	m_pInspectModeLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));

	m_pInspectContentLabel->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectContentLabel2->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectModeLabel->SetFgColor(Color(255, 255, 255, 255));

	m_pInspectContentLabel->SetPaintBackgroundEnabled(false);
	m_pInspectContentLabel2->SetPaintBackgroundEnabled(false);
	m_pInspectModeLabel->SetPaintBackgroundEnabled(false);
}

void CPhysicDebugGUI::PerformLayout(void)
{
	BaseClass::PerformLayout();

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
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicDebugGUI::OnCreatePhysicObject(uint64 physicObjectId)
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (pPhysicConfig)
		return;

	//TODO
}

void CPhysicDebugGUI::OnEditPhysicObject(uint64 physicObjectId)
{
	OpenEditPhysicObjectDialog(physicObjectId);
}

void CPhysicDebugGUI::OpenEditPhysicObjectDialog(uint64 physicObjectId)
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (!pPhysicConfig)
		return;

	if (!(pPhysicConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto dialog = new CPhysicEditorDialog(this, "PhysicEditorDialog", physicObjectId, pPhysicConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
}

void CPhysicDebugGUI::OpenEditRigidBodyDialog(int configId, uint64 physicObjectId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto dialog = new CRigidBodyEditDialog(this, "RigidBodyEditDialog", physicObjectId, pRigidBodyConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
}

void CPhysicDebugGUI::OnEditRigidBodyEx(int configId, uint64 physicObjectId)
{
	OpenEditRigidBodyDialog(configId, physicObjectId);
}

void CPhysicDebugGUI::OnMoveRigidBodyEx(int configId, uint64 physicObjectId)
{

}

void CPhysicDebugGUI::OnRotateRigidBodyEx(int configId, uint64 physicObjectId)
{

}

void CPhysicDebugGUI::OnDeleteRigidBodyEx(int configId, uint64 physicObjectId)
{

}