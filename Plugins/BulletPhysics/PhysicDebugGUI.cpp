#include "PhysicDebugGUI.h"

#include "PhysicEditorDialog.h"

#include "CounterStrike.h"

#include "ClientEntityManager.h"
#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

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

	m_pReload = new vgui::Button(this, "Reload", L"#BulletPhysics_Reload", this, "Reload");
	m_pSave = new vgui::Button(this, "Save", L"#BulletPhysics_Save", this, "SaveOpenPrompt");
	m_pInspectMode = new vgui::ComboBox(this, "InspectMode", 0, false);

	m_pClose = new vgui::Button(this, "Close", L"#GameUI_Close", this, "Close");

	m_pInspectContentLabel = new vgui::Label(this, "InspectContentLabel", "");
	m_pInspectContentLabel2 = new vgui::Label(this, "InspectContentLabel2", "");
	m_pInspectModeLabel = new vgui::Label(this, "InspectModeLabel", "");
	m_pEditModeLabel = new vgui::Label(this, "EditModeLabel", "");

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

	//Move
#define UPDATE_ORIGIN_STEP 0.1f
	if (m_EditMode == PhysicEditMode::Move && code == vgui::KEY_LEFT)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_ORIGIN_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigOrigin(physicComponentId, 0, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Move && code == vgui::KEY_RIGHT)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_ORIGIN_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigOrigin(physicComponentId, 0, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Move && code == vgui::KEY_UP)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_ORIGIN_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigOrigin(physicComponentId, 1, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Move && code == vgui::KEY_DOWN)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_ORIGIN_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigOrigin(physicComponentId, 1, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Move && code == vgui::KEY_PAGEUP)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_ORIGIN_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigOrigin(physicComponentId, 2, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Move && code == vgui::KEY_PAGEDOWN)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_ORIGIN_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if(UpdateRigidBodyConfigOrigin(physicComponentId, 2, step))
				return;
		}
	}
	//Rotate
#define UPDATE_ANGLES_STEP 0.1f
	else if (m_EditMode == PhysicEditMode::Rotate && code == vgui::KEY_LEFT)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_ANGLES_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigAngles(physicComponentId, 0, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Rotate && code == vgui::KEY_RIGHT)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_ANGLES_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigAngles(physicComponentId, 0, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Rotate && code == vgui::KEY_UP)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_ANGLES_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigAngles(physicComponentId, 1, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Rotate && code == vgui::KEY_DOWN)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_ANGLES_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigAngles(physicComponentId, 1, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Rotate && code == vgui::KEY_PAGEUP)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_ANGLES_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigAngles(physicComponentId, 2, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Rotate && code == vgui::KEY_PAGEDOWN)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_ANGLES_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigAngles(physicComponentId, 2, step))
				return;
		}
	}//Resize
#define UPDATE_RESIZE_STEP 0.1f
	else if (m_EditMode == PhysicEditMode::Resize && code == vgui::KEY_LEFT)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_RESIZE_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigSize(physicComponentId, 0, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Resize && code == vgui::KEY_RIGHT)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_RESIZE_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigSize(physicComponentId, 0, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Resize && code == vgui::KEY_UP)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_RESIZE_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigSize(physicComponentId, 1, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Resize && code == vgui::KEY_DOWN)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_RESIZE_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigSize(physicComponentId, 1, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Resize && code == vgui::KEY_PAGEUP)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = UPDATE_RESIZE_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigSize(physicComponentId, 2, step))
				return;
		}
	}
	else if (m_EditMode == PhysicEditMode::Resize && code == vgui::KEY_PAGEDOWN)
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId > 0)
		{
			float step = -UPDATE_RESIZE_STEP;

			if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
				step *= 10.0f;

			if (UpdateRigidBodyConfigSize(physicComponentId, 2, step))
				return;
		}
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
	ClientEntityManager()->SetInspectedEntityIndex(0);
	ClientPhysicManager()->SetInspectedPhysicObjectId(0);
	ClientPhysicManager()->SetInspectedPhysicComponentId(0);

	if (!IsVisible())
	{
		//TODO...
		//ClientEntityManager()->SetSelectedEntityIndex(0);
		ClientPhysicManager()->SetSelectedPhysicObjectId(0);
		ClientPhysicManager()->SetSelectedPhysicComponentId(0);
		return;
	}

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

			VectorSubtract(vecTarget, EngineGetRendererViewOrigin(), vecForward);
			VectorNormalize(vecForward);
			VectorMA(EngineGetRendererViewOrigin(), 8192, vecForward, vecTarget);

			if (m_InspectMode == PhysicInspectMode::Entity)
			{
				auto trace = gEngfuncs.PM_TraceLine(EngineGetRendererViewOrigin(), vecTarget, PM_TRACELINE_PHYSENTSONLY, 2, -1);

				if (trace->fraction != 1.0 && trace->ent)
				{
					auto physent = gEngfuncs.pEventAPI->EV_GetPhysent(trace->ent);

					if (physent)
					{
						ClientEntityManager()->SetInspectedEntityIndex(physent->info);
					}
				}
			}
			else
			{
				CPhysicTraceLineHitResult hitResult;
				ClientPhysicManager()->TraceLine(EngineGetRendererViewOrigin(), vecTarget, hitResult);

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

							if (pPhysicObjectConfig)
							{
								uint64 physicObjectId = PACK_PHYSIC_OBJECT_ID(hitResult.m_iHitEntityIndex, modelindex);

								ClientPhysicManager()->SetInspectedPhysicObjectId(physicObjectId);
							}
						}
					}
					else
					{
						ClientPhysicManager()->SetInspectedPhysicComponentId(hitResult.m_iHitPhysicComponentIndex);
					}
				}
			}
		}
	}
}

void CPhysicDebugGUI::Reset()
{
	UpdateInspectMode(PhysicInspectMode::Entity);
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
		UpdateInspectedClientEntity(false);
		break;
	}
	case PhysicInspectMode::PhysicObject:
	{
		if (false == UpdateInspectedPhysicObject(true));
			UpdateInspectedPhysicObject(false);
		break;
	}
	case PhysicInspectMode::RigidBody:
	{
		if (false == UpdateInspectedRigidBody(true))
			UpdateInspectedRigidBody(false);
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

	ClientPhysicManager()->SetSelectedPhysicObjectId(0);
	ClientPhysicManager()->SetSelectedPhysicComponentId(0);
}

void CPhysicDebugGUI::UpdateEditMode(PhysicEditMode mode)
{
	m_EditMode = mode;

	if (m_EditMode == PhysicEditMode::None)
	{
		m_pEditModeLabel->SetVisible(false);
	}
	else if (m_EditMode == PhysicEditMode::Move)
	{
		m_pEditModeLabel->SetText("#BulletPhysics_EditMode_Move");
		m_pEditModeLabel->SetVisible(true);
	}
	else if (m_EditMode == PhysicEditMode::Rotate)
	{
		m_pEditModeLabel->SetText("#BulletPhysics_EditMode_Rotate");
		m_pEditModeLabel->SetVisible(true);
	}
	else if (m_EditMode == PhysicEditMode::Resize)
	{
		m_pEditModeLabel->SetText("#BulletPhysics_EditMode_Resize");
		m_pEditModeLabel->SetVisible(true);
	}
}

void CPhysicDebugGUI::OnMousePressed(vgui::MouseCode code)
{
	if (code == vgui::MOUSE_LEFT)
	{
		if (m_InspectMode == PhysicInspectMode::Entity)
		{

			return;
		}
		else if (m_InspectMode == PhysicInspectMode::PhysicObject)
		{
			uint64 physicObjectId = ClientPhysicManager()->GetInspectedPhysicObjectId();

			if (physicObjectId)
			{
				if (ClientPhysicManager()->GetSelectedPhysicObjectId() != physicObjectId)
				{
					ClientPhysicManager()->SetSelectedPhysicObjectId(physicObjectId);
					UpdateEditMode(PhysicEditMode::None);
				}
				return;
			}

			ClientPhysicManager()->SetSelectedPhysicObjectId(0); 
			UpdateEditMode(PhysicEditMode::None);
			return;
		}
		else if (m_InspectMode == PhysicInspectMode::RigidBody)
		{
			int physicComponentId = ClientPhysicManager()->GetInspectedPhysicComponentId();

			if (physicComponentId > 0)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent && pPhysicComponent->IsRigidBody())
				{
					if (ClientPhysicManager()->GetSelectedPhysicComponentId() != physicComponentId)
					{
						ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
						UpdateEditMode(PhysicEditMode::None);
					}
					return;
				}
			}

			ClientPhysicManager()->SetSelectedPhysicComponentId(0);
			UpdateEditMode(PhysicEditMode::None);
			return;
		}
	}
	else if (code == vgui::MOUSE_RIGHT)
	{
		if (m_InspectMode == PhysicInspectMode::Entity)
		{
			if (OpenInspectClientEntityMenu(false))
			{
				return;
			}
		}
		else if (m_InspectMode == PhysicInspectMode::PhysicObject)
		{
			if (OpenInspectPhysicObjectMenu(true))
			{
				return;
			}
			else
			{
				if(OpenInspectPhysicObjectMenu(false))
					return;
			}
		}
		else if (m_InspectMode == PhysicInspectMode::RigidBody)
		{
			if (OpenInspectPhysicComponentMenu(true))
			{
				return;
			}
			else
			{
				if (OpenInspectPhysicComponentMenu(false))
					return;
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
			uint64 physicObjectId = ClientPhysicManager()->GetInspectedPhysicObjectId();

			if (physicObjectId)
			{
				OpenEditPhysicObjectDialog(physicObjectId);
				return;
			}
		}
		else if (m_InspectMode == PhysicInspectMode::RigidBody)
		{
			int physicComponentId = ClientPhysicManager()->GetInspectedPhysicComponentId();

			if (physicComponentId)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent && pPhysicComponent->IsRigidBody())
				{
					int entindex = pPhysicComponent->GetOwnerEntityIndex();

					auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);

					if (pPhysicObject)
					{
						if(OpenEditRigidBodyDialog(pPhysicObject->GetPhysicObjectId(), pPhysicObject->GetPhysicConfigId(), pPhysicComponent->GetPhysicConfigId()))
							return;
					}
				}
			}
		}
	}

	BaseClass::OnMouseDoublePressed(code);
}

bool CPhysicDebugGUI::UpdateInspectedClientEntity(bool bSelected)
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

		return true;
	}

	return false;
}

bool CPhysicDebugGUI::UpdateInspectedPhysicObject(bool bSelected)
{
	uint64 physicObjectId = bSelected ? ClientPhysicManager()->GetSelectedPhysicObjectId() : ClientPhysicManager()->GetInspectedPhysicObjectId();
	
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

			return true;
		}
	}

	return false;
}

bool CPhysicDebugGUI::UpdateInspectedRigidBody(bool bSelected)
{
	int physicComponentId = bSelected ? ClientPhysicManager()->GetSelectedPhysicComponentId() : ClientPhysicManager()->GetInspectedPhysicComponentId();

	if (physicComponentId)
	{
		auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

		if (pPhysicComponent && pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			wchar_t wszName[64] = { 0 };

			vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), wszName, sizeof(wszName));

			auto str = std::format(L"{0} (#{1}): {2} / {3}: {4}", vgui::localize()->Find("#BulletPhysics_RigidBody"), pPhysicComponent->GetPhysicComponentId(), wszName, vgui::localize()->Find("#BulletPhysics_Mass"), pRigidBody->GetMass());

			ShowInspectContentLabel(str.c_str());

			auto pRigidConfig = UTIL_GetRigidConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

			if (pRigidConfig)
			{
				auto str2 = std::format(L"[{0}] {1}: ({2:.2f}, {3:.2f}, {4:.2f}) / {5}: ({6:.2f}, {7:.2f}, {8:.2f})",
					vgui::localize()->Find("#BulletPhysics_Config"),
					vgui::localize()->Find("#BulletPhysics_Origin"), pRigidConfig->origin[0], pRigidConfig->origin[1], pRigidConfig->origin[2],
					vgui::localize()->Find("#BulletPhysics_Angles"), pRigidConfig->angles[0], pRigidConfig->angles[1], pRigidConfig->angles[2]);

				if (pRigidConfig->collisionShape)
				{
					str2 += L" / ";

					switch (pRigidConfig->collisionShape->type)
					{
					case PhysicShape_Box:
					{
						str2 += std::format(L"{0}: ({1:.2f}, {2:.2f}, {3:.2f})", vgui::localize()->Find("#BulletPhysics_Size"), pRigidConfig->collisionShape->size[0], pRigidConfig->collisionShape->size[1], pRigidConfig->collisionShape->size[2]);
						break;
					}
					case PhysicShape_Sphere:
					{
						str2 += std::format(L"{0}: ({1:.2f})", vgui::localize()->Find("#BulletPhysics_Box"), pRigidConfig->collisionShape->size[0]);
						break;
					}
					case PhysicShape_Capsule:
					{
						str2 += std::format(L"{0}: ({1:.2f}, {2:.2f})", vgui::localize()->Find("#BulletPhysics_Capsule"), pRigidConfig->collisionShape->size[0], pRigidConfig->collisionShape->size[1]);
						break;
					}
					case PhysicShape_Cylinder:
					{
						str2 += std::format(L"{0}: ({1:.2f}, {2:.2f})", vgui::localize()->Find("#BulletPhysics_Cylinder"), pRigidConfig->collisionShape->size[0], pRigidConfig->collisionShape->size[1]);
						break;
					}
					case PhysicShape_MultiSphere:
					{
						str2 += vgui::localize()->Find("#BulletPhysics_MultiSphere");
						break;
					}
					case PhysicShape_TriangleMesh:
					{
						str2 += vgui::localize()->Find("#BulletPhysics_TriangleMesh");
						break;
					}
					case PhysicShape_Compound:
					{
						str2 += vgui::localize()->Find("#BulletPhysics_Compound");
						break;
					}
					}
				}

				ShowInspectContentLabel2(str2.c_str());
			}

			return true;
		}
	}

	return false;
}

bool CPhysicDebugGUI::OpenInspectClientEntityMenu(bool bSelected)
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
				return true;
			}
		}
	}
	return false;
}

bool CPhysicDebugGUI::OpenInspectPhysicObjectMenu(bool bSelected)
{
	uint64 physicObjectId = bSelected ? ClientPhysicManager()->GetSelectedPhysicObjectId() : ClientPhysicManager()->GetInspectedPhysicObjectId();

	if (physicObjectId)
	{
		auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

		if (pPhysicObject && (pPhysicObject->GetObjectFlags() & PhysicObjectFlag_FromConfig))
		{
			auto menu = new vgui::Menu(this, "contextmenu");

			menu->SetAutoDelete(true);

			auto kv = new KeyValues("EditPhysicObject");

			kv->SetUint64("physicObjectId", physicObjectId);

			menu->AddMenuItem("#BulletPhysics_EditPhysicObject", kv, this);

			vgui::Menu::PlaceContextMenu(this, menu);
			return true;
		}
	}

	return false;
}


bool CPhysicDebugGUI::OpenInspectPhysicComponentMenu(bool bSelected)
{
	int physicComponentId = bSelected ? ClientPhysicManager()->GetSelectedPhysicComponentId() : ClientPhysicManager()->GetInspectedPhysicComponentId();

	if (physicComponentId > 0)
	{
		auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

		if (pPhysicComponent && pPhysicComponent->IsRigidBody())
		{
			int entindex = pPhysicComponent->GetOwnerEntityIndex();

			auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(entindex);

			if (pPhysicObject && (pPhysicObject->GetObjectFlags() & PhysicObjectFlag_FromConfig))
			{
				auto menu = new vgui::Menu(this, "contextmenu");

				menu->SetAutoDelete(true);

				wchar_t szName[64] = { 0 };
				wchar_t szBuf[256] = { 0 };

				vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), szName, sizeof(szName));

				auto kv = new KeyValues("EditRigidBodyEx");
				kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
				kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
				kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
				kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditRigidBody"), 1, szName);
				menu->AddMenuItem("EditRigidBodyEx", szBuf, kv, this);

				kv = new KeyValues("MoveRigidBodyEx");
				kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
				kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
				kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
				kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_MoveRigidBody"), 1, szName);
				menu->AddMenuItem("MoveRigidBodyEx", szBuf, kv, this);

				kv = new KeyValues("RotateRigidBodyEx");
				kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
				kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
				kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
				kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_RotateRigidBody"), 1, szName);
				menu->AddMenuItem("RotateRigidBodyEx", szBuf, kv, this);

				kv = new KeyValues("ResizeRigidBodyEx");
				kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
				kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
				kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
				kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ResizeRigidBody"), 1, szName);
				menu->AddMenuItem("ResizeRigidBodyEx", szBuf, kv, this);

				kv = new KeyValues("DeleteRigidBodyEx");
				kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
				kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
				kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
				kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteRigidBody"), 1, szName);
				menu->AddMenuItem("DeleteRigidBodyEx", szBuf, kv, this);

				vgui::Menu::PlaceContextMenu(this, menu);
				return true;
			}
		}
	}
	return false;
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
	m_pEditModeLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));

	m_pInspectContentLabel->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectContentLabel2->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectModeLabel->SetFgColor(Color(255, 255, 255, 255));
	m_pEditModeLabel->SetFgColor(Color(255, 255, 255, 255));

	m_pInspectContentLabel->SetPaintBackgroundEnabled(false);
	m_pInspectContentLabel2->SetPaintBackgroundEnabled(false);
	m_pInspectModeLabel->SetPaintBackgroundEnabled(false);
	m_pEditModeLabel->SetPaintBackgroundEnabled(false);
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

void CPhysicDebugGUI::SaveOpenPrompt()
{
	auto box = new vgui::QueryBox("#BulletPhysics_SaveConfirmationTitle", "#BulletPhysics_SaveConfirmationText", this);
	box->SetOKButtonText("#GameUI_OK");
	box->SetOKCommand(new KeyValues("Command", "command", "SaveConfirm"));
	box->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
	box->AddActionSignalTarget(this);
	box->DoModal();
}

void CPhysicDebugGUI::SaveConfirm()
{
	ClientPhysicManager()->SavePhysicObjectConfigs();
}

void CPhysicDebugGUI::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		
	}
	else if (!strcmp(command, "Reload"))
	{
		ClientPhysicManager()->RemoveAllPhysicObjects(PhysicObjectFlag_AnyObject, PhysicObjectFlag_FromBSP);
		ClientPhysicManager()->RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromConfig, 0);
		ClientPhysicManager()->LoadPhysicObjectConfigs();
		return;
	}
	else if (!strcmp(command, "SaveOpenPrompt"))
	{
		SaveOpenPrompt();
		return;
	}
	else if (!strcmp(command, "SaveConfirm"))
	{
		SaveConfirm();
		return;
	}
	else if (!strcmp(command, "SaveAllConfirm"))
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

void CPhysicDebugGUI::OnEditPhysicObject(KeyValues *kv)
{
	auto configId = kv->GetInt("configId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto physicObjectId = kv->GetUint64("physicObjectId");

	OpenEditPhysicObjectDialog(physicObjectId);
}

bool CPhysicDebugGUI::OpenEditPhysicObjectDialog(uint64 physicObjectId)
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (!pPhysicConfig)
		return false;

	if (!(pPhysicConfig->flags & PhysicObjectFlag_FromConfig))
		return false;

	auto dialog = new CPhysicEditorDialog(this, "PhysicEditorDialog", physicObjectId, pPhysicConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal(); 
	
	return true;
}

bool CPhysicDebugGUI::OpenEditRigidBodyDialog(uint64 physicObjectId, int physicObjectConfigId, int rigidBodyConfigId)
{
	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (!pPhysicObjectConfig)
		return false;

	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(rigidBodyConfigId);

	if (!pRigidBodyConfig)
		return false;

	auto dialog = new CRigidBodyEditDialog(this, "RigidBodyEditDialog", physicObjectId, pPhysicObjectConfig, pRigidBodyConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();

	return true;
}

void CPhysicDebugGUI::OnEditRigidBodyEx(KeyValues *kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto rigidBodyConfigId = kv->GetInt("rigidBodyConfigId");

	if (OpenEditRigidBodyDialog(physicObjectId, physicObjectConfigId, rigidBodyConfigId))
	{
		ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	}
}

void CPhysicDebugGUI::OnMoveRigidBodyEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto rigidBodyConfigId = kv->GetInt("rigidBodyConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Move);
}

void CPhysicDebugGUI::OnRotateRigidBodyEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto rigidBodyConfigId = kv->GetInt("rigidBodyConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Rotate);
}

void CPhysicDebugGUI::OnResizeRigidBodyEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto rigidBodyConfigId = kv->GetInt("rigidBodyConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Resize);
}

void CPhysicDebugGUI::OnDeleteRigidBodyEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto rigidBodyConfigId = kv->GetInt("rigidBodyConfigId");

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (pPhysicObjectConfig)
	{
		if (UTIL_RemoveRigidBodyFromPhysicObjectConfig(pPhysicObjectConfig.get(), rigidBodyConfigId))
		{
			ClientPhysicManager()->RebuildPhysicObjectEx(physicObjectId, pPhysicObjectConfig.get());
		}
	}
}

bool CPhysicDebugGUI::UpdateRigidBodyConfigOrigin(int physicComponentId, int axis, float value)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(pPhysicComponent->GetOwnerEntityIndex());

		if (pPhysicObject)
		{
			auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());
			auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

			if (pRigidBodyConfig && pPhysicObjectConfig)
			{
				pRigidBodyConfig->origin[axis] += value;

				return pPhysicObject->Rebuild(pPhysicObjectConfig.get());
			}
		}
	}

	return false;
}

bool CPhysicDebugGUI::UpdateRigidBodyConfigAngles(int physicComponentId, int axis, float value)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(pPhysicComponent->GetOwnerEntityIndex());

		if (pPhysicObject)
		{
			auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());
			auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

			if (pRigidBodyConfig && pPhysicObjectConfig)
			{
				pRigidBodyConfig->angles[axis] += value;

				return pPhysicObject->Rebuild(pPhysicObjectConfig.get());
			}
		}
	}

	return false;
}

bool CPhysicDebugGUI::UpdateRigidBodyConfigSize(int physicComponentId, int axis, float value)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		auto pPhysicObject = ClientPhysicManager()->GetPhysicObject(pPhysicComponent->GetOwnerEntityIndex());

		if (pPhysicObject)
		{
			auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());
			auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

			if (pRigidBodyConfig && pPhysicObjectConfig)
			{
				if (pRigidBodyConfig->collisionShape)
				{
					pRigidBodyConfig->collisionShape->size[axis] += value;
				}

				return pPhysicObject->Rebuild(pPhysicObjectConfig.get());
			}
		}
	}

	return false;
}