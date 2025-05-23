#include "PhysicDebugGUI.h"

#include "PhysicEditorDialog.h"
#include "PhysicRigidBodyEditDialog.h"
#include "PhysicConstraintEditDialog.h"
#include "PhysicBehaviorEditDialog.h"

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
	SetTitleBarVisible(false);

	SetScheme2("BaseUI");
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
	m_pInspectContentLabel3 = new vgui::Label(this, "InspectContentLabel3", "");
	m_pInspectModeLabel = new vgui::Label(this, "InspectModeLabel", "");
	m_pEditModeLabel = new vgui::Label(this, "EditModeLabel", "");

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	LoadControlSettings("bulletphysics/PhysicDebugGUI.res", "GAME");

	LoadAvailableInspectModeIntoControls();
	LoadAvailableEditModeIntoControls();
}

void CPhysicDebugGUI::LoadAvailableInspectModeIntoControls()
{
	const char* VGUI2Tokens_InspectMode[] = { 
		"#BulletPhysics_InspectItem_Entity",
		"#BulletPhysics_InspectItem_PhysicObject", 
		"#BulletPhysics_InspectItem_RigidBody",
		"#BulletPhysics_InspectItem_Constraint",
		"#BulletPhysics_InspectItem_PhysicBehavior" };

	for (int i = 0; i < _ARRAYSIZE(VGUI2Tokens_InspectMode); ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("mode", i);

		m_pInspectMode->AddItem(VGUI2Tokens_InspectMode[i], kv);

		kv->deleteThis();
	}
}

void CPhysicDebugGUI::LoadAvailableEditModeIntoControls()
{
	/*const char* VGUI2Tokens_EditMode[] = {"#BulletPhysics_None", "#BulletPhysics_Move", "#BulletPhysics_Rotate", "#BulletPhysics_Resize"};

	for (int i = 0; i < _ARRAYSIZE(VGUI2Tokens_EditMode); ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("mode", i);

		m_pEditMode->AddItem(VGUI2Tokens_EditMode[i], kv);

		kv->deleteThis();
	}*/
}

CPhysicDebugGUI::~CPhysicDebugGUI()
{

}

static int ConvertKeyCodeToAxis(vgui::KeyCode code)
{
	switch (code)
	{
	case vgui::KEY_LEFT:
	case vgui::KEY_RIGHT:
		return 0;
	case vgui::KEY_UP:
	case vgui::KEY_DOWN:
		return 1;
	case vgui::KEY_PAGEUP:
	case vgui::KEY_PAGEDOWN:
		return 2;
	}

	return 0;
}

static int ConvertKeyCodeToSign(vgui::KeyCode code)
{
	switch (code)
	{
	case vgui::KEY_LEFT:
	case vgui::KEY_UP:
	case vgui::KEY_PAGEUP:
		return -1;
	case vgui::KEY_PAGEDOWN:
	case vgui::KEY_RIGHT:
	case vgui::KEY_DOWN:
		return 1;
	}

	return 1;
}

void CPhysicDebugGUI::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_F1)
	{
		m_pInspectMode->ActivateItem(0);
		return;
	}

	if (code == vgui::KEY_F2)
	{
		m_pInspectMode->ActivateItem(1);
		return;
	}

	if (code == vgui::KEY_F3)
	{
		m_pInspectMode->ActivateItem(2);
		return;
	}

	if (code == vgui::KEY_F4)
	{
		m_pInspectMode->ActivateItem(3);
		return;
	}

	if (code == vgui::KEY_F5)
	{
		m_pInspectMode->ActivateItem(4);
		return;
	}

	if (code == vgui::KEY_S && vgui::input()->IsKeyDown(vgui::KEY_LCONTROL))
	{
		SaveOpenPrompt();
		return;
	}

	if (code == vgui::KEY_R && vgui::input()->IsKeyDown(vgui::KEY_LCONTROL))
	{
		BV_ReloadAll_f();
		return;
	}

	if (code == vgui::KEY_Q && vgui::input()->IsKeyDown(vgui::KEY_LCONTROL))
	{
		Close();
		return;
	}

	if (!vgui::input()->IsKeyDown(vgui::KEY_LCONTROL) &&
		!vgui::input()->IsKeyDown(vgui::KEY_LSHIFT) &&
		!vgui::input()->IsKeyDown(vgui::KEY_LALT))
	{
		auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

		if (physicComponentId)
		{
			if (code == vgui::KEY_E)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent)
				{
					if (pPhysicComponent->IsRigidBody())
					{
						OpenEditRigidBodyDialog(pPhysicComponent->GetOwnerPhysicObject()->GetPhysicObjectId(), pPhysicComponent->GetOwnerPhysicObject()->GetPhysicConfigId(), pPhysicComponent->GetPhysicConfigId());
						return;
					}
					else if (pPhysicComponent->IsConstraint())
					{
						OpenEditConstraintDialog(pPhysicComponent->GetOwnerPhysicObject()->GetPhysicObjectId(), pPhysicComponent->GetOwnerPhysicObject()->GetPhysicConfigId(), pPhysicComponent->GetPhysicConfigId());
						return;
					}
					else if (pPhysicComponent->IsPhysicBehavior())
					{
						OpenEditPhysicBehaviorDialog(pPhysicComponent->GetOwnerPhysicObject()->GetPhysicObjectId(), pPhysicComponent->GetOwnerPhysicObject()->GetPhysicConfigId(), pPhysicComponent->GetPhysicConfigId());
						return;
					}
				}
			}
			else if (code == vgui::KEY_DELETE)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent)
				{
					if (pPhysicComponent->IsRigidBody())
					{
						if (DeleteRigidBodyByComponent(pPhysicComponent))
						{
							ClientPhysicManager()->SetSelectedPhysicComponentId(0);
							return;
						}
					}
					else if (pPhysicComponent->IsConstraint())
					{
						if (DeleteConstraintByComponent(pPhysicComponent))
						{
							ClientPhysicManager()->SetSelectedPhysicComponentId(0);
							return;
						}
					}
					else if (pPhysicComponent->IsPhysicBehavior())
					{
						if (DeleteConstraintByComponent(pPhysicComponent))
						{
							ClientPhysicManager()->SetSelectedPhysicComponentId(0);
							return;
						}
					}
				}
			}
			else if (m_EditMode != PhysicEditMode::Move && code == vgui::KEY_M)
			{
				UpdateEditMode(PhysicEditMode::Move);
				return;
			}
			else if (m_EditMode != PhysicEditMode::Rotate && code == vgui::KEY_R)
			{
				UpdateEditMode(PhysicEditMode::Rotate);
				return;
			}
			else if (m_EditMode != PhysicEditMode::Resize && code == vgui::KEY_S)
			{
				UpdateEditMode(PhysicEditMode::Resize);
				return;
			}
		}
	}

	//Move
	if (code == vgui::KEY_LEFT || code == vgui::KEY_RIGHT || code == vgui::KEY_UP || code == vgui::KEY_DOWN || code == vgui::KEY_PAGEUP || code == vgui::KEY_PAGEDOWN)
	{
		if (m_EditMode == PhysicEditMode::Move)
		{
			auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

			if (physicComponentId)
			{
				float step = 0.1f;

				if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
					step *= 10.0f;
				else if (vgui::input()->IsKeyDown(vgui::KEY_LALT))
					step *= 0.1f;

				if (UpdateConfigOrigin(physicComponentId, ConvertKeyCodeToAxis(code), ConvertKeyCodeToSign(code) * step))
					return;
			}
		}
		else if (m_EditMode == PhysicEditMode::Rotate)
		{
			auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

			if (physicComponentId)
			{
				float step = 0.2f;

				if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
					step *= 10.0f;
				else if (vgui::input()->IsKeyDown(vgui::KEY_LALT))
					step *= 0.1f;

				if (UpdateConfigAngles(physicComponentId, ConvertKeyCodeToAxis(code), ConvertKeyCodeToSign(code) * step))
					return;
			}
		}
		else if (m_EditMode == PhysicEditMode::Resize)
		{
			auto physicComponentId = ClientPhysicManager()->GetSelectedPhysicComponentId();

			if (physicComponentId)
			{
				float step = 0.1f;

				if (vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
					step *= 10.0f;
				else if (vgui::input()->IsKeyDown(vgui::KEY_LALT))
					step *= 0.1f;

				if (UpdateConfigSize(physicComponentId, ConvertKeyCodeToAxis(code), ConvertKeyCodeToSign(code) * step))
					return;
			}
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicDebugGUI::Init()
{
	Reset();
	SetVisible(false);
}

void CPhysicDebugGUI::NewMap()
{
	Reset();
	SetVisible(false);
}

void CPhysicDebugGUI::ConnectToServer(const char* game, int IP, int port)
{
	for (int i = 0; i < GetChildCount(); i++)
	{
		auto pChild = GetChild(i);
		PostMessage1(pChild, new KeyValues("Command", "command", "CloseModalDialogs"), NULL);
	}
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
				CPhysicTraceLineParameters traceParam;
				CPhysicTraceLineHitResult hitResult;
				VectorCopy(EngineGetRendererViewOrigin(), traceParam.vecStart);
				VectorCopy(vecTarget, traceParam.vecEnd);

				traceParam.withoutflags |= PhysicTraceLineFlag_World;

				switch (m_InspectMode)
				{
				case PhysicInspectMode::PhysicObject:
				{
					break;
				}
				case PhysicInspectMode::RigidBody:
				{
					traceParam.withflags = PhysicTraceLineFlag_RigidBody;
					traceParam.withoutflags |= (PhysicTraceLineFlag_Constraint | PhysicTraceLineFlag_PhysicBehavior);
					break;
				}
				case PhysicInspectMode::Constraint:
				{
					traceParam.withflags = PhysicTraceLineFlag_Constraint;
					traceParam.withoutflags |= (PhysicTraceLineFlag_RigidBody | PhysicTraceLineFlag_PhysicBehavior);
					break;
				}
				case PhysicInspectMode::PhysicBehavior:
				{
					traceParam.withflags = PhysicTraceLineFlag_PhysicBehavior;
					traceParam.withoutflags |= (PhysicTraceLineFlag_RigidBody | PhysicTraceLineFlag_Constraint);
					break;
				}
				} 

				ClientPhysicManager()->TraceLine(traceParam, hitResult);

				if (hitResult.m_bHasHit)
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

void CPhysicDebugGUI::OnTextChanged(vgui::Panel* panel)
{
	if (panel == m_pInspectMode)
	{
		UpdateInspectMode( (PhysicInspectMode) m_pInspectMode->GetActiveItem());
	}
}

void CPhysicDebugGUI::Reset()
{
	m_pInspectMode->ActivateItem(0);
	UpdateEditMode(PhysicEditMode::None);
}

void CPhysicDebugGUI::OnThink()
{
	BaseClass::OnThink();

	HideInspectContentLabel();
	HideInspectContentLabel2();
	HideInspectContentLabel3();

	switch (m_InspectMode)
	{
	case PhysicInspectMode::Entity:
	{
		UpdateInspectedClientEntity(false);
		break;
	}
	case PhysicInspectMode::PhysicObject:
	{
		if (false == UpdateInspectedPhysicObject(true))
			UpdateInspectedPhysicObject(false);
		break;
	}
	case PhysicInspectMode::RigidBody:
	{
		if (false == UpdateInspectedRigidBody(true))
			UpdateInspectedRigidBody(false);
		break;
	}
	case PhysicInspectMode::Constraint:
	{
		if (false == UpdateInspectedConstraint(true))
			UpdateInspectedConstraint(false);
		break;
	}
	case PhysicInspectMode::PhysicBehavior:
	{
		if (false == UpdateInspectedPhysicBehavior(true))
			UpdateInspectedPhysicBehavior(false);
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
	else if (m_InspectMode == PhysicInspectMode::Constraint)
	{
		m_pInspectModeLabel->SetText("#BulletPhysics_InspectMode_Constraint");
	}
	else if (m_InspectMode == PhysicInspectMode::PhysicBehavior)
	{
		m_pInspectModeLabel->SetText("#BulletPhysics_InspectMode_PhysicBehavior");
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

			if (physicComponentId)
			{
				auto pRigidBody = UTIL_GetPhysicComponentAsRigidBody(physicComponentId);

				if (pRigidBody)
				{
					if (ClientPhysicManager()->GetSelectedPhysicComponentId() != physicComponentId)
					{
						ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
						//UpdateEditMode(PhysicEditMode::None);
					}
					return;
				}
			}

			ClientPhysicManager()->SetSelectedPhysicComponentId(0);
			UpdateEditMode(PhysicEditMode::None);
			return;
		}
		else if (m_InspectMode == PhysicInspectMode::Constraint)
		{
			int physicComponentId = ClientPhysicManager()->GetInspectedPhysicComponentId();

			if (physicComponentId > 0)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent && pPhysicComponent->IsConstraint())
				{
					if (ClientPhysicManager()->GetSelectedPhysicComponentId() != physicComponentId)
					{
						ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
						//UpdateEditMode(PhysicEditMode::None);
					}
					return;
				}
			}

			ClientPhysicManager()->SetSelectedPhysicComponentId(0);
			UpdateEditMode(PhysicEditMode::None);
			return;
		}
		else if (m_InspectMode == PhysicInspectMode::PhysicBehavior)
		{	
			int physicComponentId = ClientPhysicManager()->GetInspectedPhysicComponentId();

			if (physicComponentId > 0)
			{
				auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

				if (pPhysicComponent && pPhysicComponent->IsPhysicBehavior())
				{
					if (ClientPhysicManager()->GetSelectedPhysicComponentId() != physicComponentId)
					{
						ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
						//UpdateEditMode(PhysicEditMode::None);
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
		else if (
			m_InspectMode == PhysicInspectMode::RigidBody || 
			m_InspectMode == PhysicInspectMode::Constraint || 
			m_InspectMode == PhysicInspectMode::PhysicBehavior)
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
				auto pRigidBody = UTIL_GetPhysicComponentAsRigidBody(physicComponentId);

				if (pRigidBody)
				{
					auto pPhysicObject = pRigidBody->GetOwnerPhysicObject();

					if (pPhysicObject)
					{
						if(OpenEditRigidBodyDialog(pPhysicObject->GetPhysicObjectId(), pPhysicObject->GetPhysicConfigId(), pRigidBody->GetPhysicConfigId()))
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

		wchar_t wszModelName[64] = { 0 };

		auto modelname = UTIL_GetAbsoluteModelName(model);

		vgui::localize()->ConvertANSIToUnicode(modelname.c_str(), wszModelName, sizeof(wszModelName));

		auto str = std::format(L"{0}: {1}", vgui::localize()->Find("#BulletPhysics_Entity"), wszModelName);

		ShowInspectContentLabel(str.c_str());

		auto str2 = std::format(L"{0}: {1}, {2}: {3}, {4}: {5}", 
			vgui::localize()->Find("#BulletPhysics_Sequence"), curstate->sequence, 
			vgui::localize()->Find("#BulletPhysics_GaitSequence"), curstate->gaitsequence,
			vgui::localize()->Find("#BulletPhysics_AnimFrame"), curstate->frame);

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

			auto modelname = UTIL_GetAbsoluteModelName(pPhysicObject->GetModel());

			vgui::localize()->ConvertANSIToUnicode(modelname.c_str(), wszModelName, sizeof(wszModelName));

			auto str = std::format(L"{0}: {1}", vgui::localize()->Find(pPhysicObject->GetTypeLocalizationTokenString()), wszModelName);

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
		auto pRigidBody = UTIL_GetPhysicComponentAsRigidBody(physicComponentId);

		if (pRigidBody)
		{
			wchar_t wszComponentName[64] = { 0 };

			vgui::localize()->ConvertANSIToUnicode(pRigidBody->GetName(), wszComponentName, sizeof(wszComponentName));

			auto str = std::format(L"{0}: {1} / {2}: {3} ", 
				vgui::localize()->Find("#BulletPhysics_RigidBody"), 
				wszComponentName,
				vgui::localize()->Find("#BulletPhysics_Mass"), pRigidBody->GetMass());

			str += UTIL_GetFormattedRigidBodyFlags(pRigidBody->GetFlags());

			ShowInspectContentLabel(str.c_str());

			auto pRigidConfig = UTIL_GetRigidConfigFromConfigId(pRigidBody->GetPhysicConfigId());

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
						wchar_t wszResourcePath[256] = { 0 };
						vgui::localize()->ConvertANSIToUnicode(pRigidConfig->collisionShape->resourcePath.c_str(), wszResourcePath, sizeof(wszResourcePath));

						str2 += std::format(L"{0} ({1})", vgui::localize()->Find("#BulletPhysics_TriangleMesh"), wszResourcePath);
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

bool CPhysicDebugGUI::UpdateInspectedConstraint(bool bSelected)
{
	int physicComponentId = bSelected ? ClientPhysicManager()->GetSelectedPhysicComponentId() : ClientPhysicManager()->GetInspectedPhysicComponentId();

	if (physicComponentId)
	{
		auto pConstraint = UTIL_GetPhysicComponentAsConstraint(physicComponentId);

		if (pConstraint)
		{
			wchar_t wszName[64] = { 0 };

			vgui::localize()->ConvertANSIToUnicode(pConstraint->GetName(), wszName, sizeof(wszName));

			auto str = std::format(L"{0}: {1}", vgui::localize()->Find(pConstraint->GetTypeLocalizationTokenString()), wszName);

			str += UTIL_GetFormattedConstraintFlags(pConstraint->GetFlags());

			ShowInspectContentLabel(str.c_str());

			auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(pConstraint->GetPhysicConfigId());

			if (pConstraintConfig)
			{
				auto str2 = std::format(L"[{0}] ", vgui::localize()->Find("#BulletPhysics_Config"));
				
				str2 += UTIL_GetFormattedConstraintConfigAttributes(pConstraintConfig.get());

				ShowInspectContentLabel2(str2.c_str());

				std::wstring str3;

				bool bUseRigidBodyA = pConstraintConfig->useGlobalJointFromA;
				bool bUseRigidBodyB = !pConstraintConfig->useGlobalJointFromA;

				if (pConstraintConfig->useLookAtOther || pConstraintConfig->useGlobalJointOriginFromOther)
				{
					bUseRigidBodyA = true;
					bUseRigidBodyB = true;
				}

				if (bUseRigidBodyA)
				{
					wchar_t wszRigidBodyName[64] = { 0 };
					vgui::localize()->ConvertANSIToUnicode(pConstraintConfig->rigidbodyA.c_str(), wszRigidBodyName, sizeof(wszRigidBodyName));

					str3 += std::format(L"{0}: {1} / {2}: ({3:.2f}, {4:.2f}, {5:.2f}) / {6}: ({7:.2f}, {8:.2f}, {9:.2f}) ",
						vgui::localize()->Find("#BulletPhysics_RigidBodyA"),
						wszRigidBodyName,
						vgui::localize()->Find("#BulletPhysics_Origin"),
						pConstraintConfig->originA[0],
						pConstraintConfig->originA[1],
						pConstraintConfig->originA[2],
						vgui::localize()->Find("#BulletPhysics_Angles"),
						pConstraintConfig->anglesA[0],
						pConstraintConfig->anglesA[1],
						pConstraintConfig->anglesA[2]);

				}

				if (bUseRigidBodyB)
				{
					wchar_t wszRigidBodyName[64] = { 0 };
					vgui::localize()->ConvertANSIToUnicode(pConstraintConfig->rigidbodyB.c_str(), wszRigidBodyName, sizeof(wszRigidBodyName));

					str3 += std::format(L"{0}: {1} / {2}: ({3:.2f}, {4:.2f}, {5:.2f}) / {6}: ({7:.2f}, {8:.2f}, {9:.2f}) ",
						vgui::localize()->Find("#BulletPhysics_RigidBodyB"),
						wszRigidBodyName,
						vgui::localize()->Find("#BulletPhysics_Origin"),
						pConstraintConfig->originB[0],
						pConstraintConfig->originB[1],
						pConstraintConfig->originB[2],
						vgui::localize()->Find("#BulletPhysics_Angles"),
						pConstraintConfig->anglesB[0],
						pConstraintConfig->anglesB[1],
						pConstraintConfig->anglesB[2]);
				}

				if(str3.size() > 0)
					ShowInspectContentLabel3(str3.c_str());
			}


			return true;
		}
	}

	return false;
}

bool CPhysicDebugGUI::UpdateInspectedPhysicBehavior(bool bSelected)
{
	int physicComponentId = bSelected ? ClientPhysicManager()->GetSelectedPhysicComponentId() : ClientPhysicManager()->GetInspectedPhysicComponentId();

	if (physicComponentId)
	{
		auto pPhysicBehavior = UTIL_GetPhysicComponentAsPhysicBehavior(physicComponentId);

		if (pPhysicBehavior)
		{
			wchar_t wszName[64] = { 0 };

			vgui::localize()->ConvertANSIToUnicode(pPhysicBehavior->GetName(), wszName, sizeof(wszName));

			auto str = std::format(L"{0}: {1}", vgui::localize()->Find(pPhysicBehavior->GetTypeLocalizationTokenString()), wszName);

			str += UTIL_GetFormattedPhysicBehaviorFlags(pPhysicBehavior->GetFlags());

			ShowInspectContentLabel(str.c_str());

			auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(pPhysicBehavior->GetPhysicConfigId());

			if (pPhysicBehaviorConfig)
			{
				auto str2 = std::format(L"[{0}] ", vgui::localize()->Find("#BulletPhysics_Config"));

				if (pPhysicBehaviorConfig->rigidbodyA.size() > 0)
				{
					wchar_t wszRigidBodyName[64] = { 0 };
					vgui::localize()->ConvertANSIToUnicode(pPhysicBehaviorConfig->rigidbodyA.c_str(), wszRigidBodyName, sizeof(wszRigidBodyName));

					str2 += std::format(L"{0}: {1} / {2}: ({3:.2f}, {4:.2f}, {5:.2f}) / {6}: ({7:.2f}, {8:.2f}, {9:.2f}) ",
						vgui::localize()->Find("#BulletPhysics_RigidBody"),
						wszRigidBodyName,
						vgui::localize()->Find("#BulletPhysics_Origin"),
						pPhysicBehaviorConfig->origin[0],
						pPhysicBehaviorConfig->origin[1],
						pPhysicBehaviorConfig->origin[2],
						vgui::localize()->Find("#BulletPhysics_Angles"),
						pPhysicBehaviorConfig->angles[0],
						pPhysicBehaviorConfig->angles[1],
						pPhysicBehaviorConfig->angles[2]);
				}
				else if (pPhysicBehaviorConfig->constraint.size() > 0)
				{
					wchar_t wszConstraintName[64] = { 0 };
					vgui::localize()->ConvertANSIToUnicode(pPhysicBehaviorConfig->constraint.c_str(), wszConstraintName, sizeof(wszConstraintName));

					str2 += std::format(L"{0}: {1} / {2}: ({3:.2f}, {4:.2f}, {5:.2f}) / {6}: ({7:.2f}, {8:.2f}, {9:.2f}) ",
						vgui::localize()->Find("#BulletPhysics_Constraint"),
						wszConstraintName,
						vgui::localize()->Find("#BulletPhysics_Origin"),
						pPhysicBehaviorConfig->origin[0],
						pPhysicBehaviorConfig->origin[1],
						pPhysicBehaviorConfig->origin[2],
						vgui::localize()->Find("#BulletPhysics_Angles"),
						pPhysicBehaviorConfig->angles[0],
						pPhysicBehaviorConfig->angles[1],
						pPhysicBehaviorConfig->angles[2]);
				}

				if (str2.size() > 0)
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
			auto physicObjectId = PACK_PHYSIC_OBJECT_ID(entindex, modelindex);

			auto model = EngineGetModelByIndex(modelindex);

			if (model && model->type == mod_studio)
			{
				auto menu = new vgui::Menu(this, "contextmenu");

				menu->SetAutoDelete(true);

				char szFileName[64] = {0};
				wchar_t wszFileName[64] = { 0 };
				wchar_t szBuf[256] = { 0 };

				V_FileBase(model->name, szFileName, sizeof(szFileName));
				vgui::localize()->ConvertANSIToUnicode(szFileName, wszFileName, sizeof(wszFileName));

				auto kv = new KeyValues("CreateStaticObject");
				kv->SetUint64("physicObjectId", physicObjectId);
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CreateStaticObject"), 1, wszFileName);

				menu->AddMenuItem("CreateStaticObject", szBuf, kv, this);

				kv = new KeyValues("CreateDynamicObject");
				kv->SetUint64("physicObjectId", physicObjectId);
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CreateDynamicObject"), 1, wszFileName);

				menu->AddMenuItem("CreateDynamicObject", szBuf, kv, this);

				kv = new KeyValues("CreateRagdollObject");
				kv->SetUint64("physicObjectId", physicObjectId);
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CreateRagdollObject"), 1, wszFileName);

				menu->AddMenuItem("CreateRagdollObject", szBuf, kv, this);

				vgui::Menu::PlaceContextMenu(this, menu);
				return true;
			}
		}

		if(pPhysicObject && pPhysicConfig)
		{
			if (pPhysicConfig->flags & PhysicObjectFlag_FromConfig)
			{
				auto menu = new vgui::Menu(this, "contextmenu");

				menu->SetAutoDelete(true);

				char szFileName[64] = { 0 };
				wchar_t wszFileName[64] = { 0 };
				wchar_t szBuf[256] = { 0 };

				V_FileBase(pPhysicConfig->shortName.c_str(), szFileName, sizeof(szFileName));
				vgui::localize()->ConvertANSIToUnicode(szFileName, wszFileName, sizeof(wszFileName));

				auto kv = new KeyValues("EditPhysicObject");
				kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
				kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicObject"), 1, wszFileName);

				menu->AddMenuItem("EditPhysicObject", szBuf, kv, this);

				kv = new KeyValues("CreateRigidBody");
				kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
				kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
				vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CreateRigidBodyFor"), 1, wszFileName);

				menu->AddMenuItem("CreateRigidBody", szBuf, kv, this);

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

			char szFileName[64] = { 0 };
			wchar_t wszFileName[64] = { 0 };
			wchar_t szBuf[256] = { 0 };

			V_FileBase(pPhysicObject->GetModel()->name, szFileName, sizeof(szFileName));
			vgui::localize()->ConvertANSIToUnicode(szFileName, wszFileName, sizeof(wszFileName));

			auto kv = new KeyValues("EditPhysicObject");

			kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
			kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
			vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicObject"), 1, wszFileName);

			menu->AddMenuItem("EditPhysicObject", szBuf, kv, this);

			kv = new KeyValues("CreateRigidBody");
			kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
			kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
			vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CreateRigidBodyFor"), 1, wszFileName);

			menu->AddMenuItem("CreateRigidBody", szBuf, kv, this);

			if (pPhysicObject->IsRagdollObject())
			{
				auto pRagdollObject = (IRagdollObject*)pPhysicObject;

				if (!pRagdollObject->IsDebugAnimEnabled())
				{
					kv = new KeyValues("EnableDebugAnim");
					kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
					//kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
					vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EnableDebugAnim"), 1, wszFileName);

					menu->AddMenuItem("EnableDebugAnim", szBuf, kv, this);
				}
				else
				{
					kv = new KeyValues("DisableDebugAnim");
					kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
					//kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
					vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DisableDebugAnim"), 1, wszFileName);

					menu->AddMenuItem("DisableDebugAnim", szBuf, kv, this);
				}
			}

			vgui::Menu::PlaceContextMenu(this, menu);
			return true;
		}
	}

	return false;
}

bool CPhysicDebugGUI::OpenRigidBodyMenu(IPhysicComponent* pPhysicComponent)
{
	auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

	if (pPhysicObject && (pPhysicObject->GetObjectFlags() & PhysicObjectFlag_FromConfig))
	{

	}
	else
	{
		return false;
	}

	auto menu = new vgui::Menu(this, "contextmenu");

	menu->SetAutoDelete(true);

	char szFileName[64] = { 0 };
	wchar_t wszFileName[64] = { 0 };
	wchar_t wszComponentName[64] = { 0 };
	wchar_t szBuf[256] = { 0 };

	V_FileBase(pPhysicObject->GetModel()->name, szFileName, sizeof(szFileName));
	vgui::localize()->ConvertANSIToUnicode(szFileName, wszFileName, sizeof(wszFileName));

	auto kv = new KeyValues("EditPhysicObject");

	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicObject"), 1, wszFileName);

	menu->AddMenuItem("EditPhysicObject", szBuf, kv, this);

	vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), wszComponentName, sizeof(wszComponentName));

	kv = new KeyValues("EditRigidBodyEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditRigidBody"), 1, wszComponentName);
	menu->AddMenuItem("EditRigidBodyEx", szBuf, kv, this);

	kv = new KeyValues("MoveRigidBodyEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_MoveRigidBody"), 1, wszComponentName);
	menu->AddMenuItem("MoveRigidBodyEx", szBuf, kv, this);

	kv = new KeyValues("RotateRigidBodyEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_RotateRigidBody"), 1, wszComponentName);
	menu->AddMenuItem("RotateRigidBodyEx", szBuf, kv, this);

	kv = new KeyValues("ResizeRigidBodyEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ResizeRigidBody"), 1, wszComponentName);
	menu->AddMenuItem("ResizeRigidBodyEx", szBuf, kv, this);

	kv = new KeyValues("CloneRigidBodyEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CloneRigidBody"), 1, wszComponentName);
	menu->AddMenuItem("CloneRigidBodyEx", szBuf, kv, this);

	kv = new KeyValues("DeleteRigidBodyEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("rigidBodyConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteRigidBody"), 1, wszComponentName);
	menu->AddMenuItem("DeleteRigidBodyEx", szBuf, kv, this);

	vgui::Menu::PlaceContextMenu(this, menu);

	return true;
}

bool CPhysicDebugGUI::OpenConstraintMenu(IPhysicComponent* pPhysicComponent)
{
	auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

	if (pPhysicObject && (pPhysicObject->GetObjectFlags() & PhysicObjectFlag_FromConfig))
	{

	}
	else
	{
		return false;
	}

	auto menu = new vgui::Menu(this, "contextmenu");

	menu->SetAutoDelete(true);

	char szFileName[64] = { 0 };
	wchar_t wszFileName[64] = { 0 };
	wchar_t wszComponentName[64] = { 0 };
	wchar_t szBuf[256] = { 0 };

	V_FileBase(pPhysicObject->GetModel()->name, szFileName, sizeof(szFileName));
	vgui::localize()->ConvertANSIToUnicode(szFileName, wszFileName, sizeof(wszFileName));

	auto kv = new KeyValues("EditPhysicObject");

	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicObject"), 1, wszFileName);

	menu->AddMenuItem("EditPhysicObject", szBuf, kv, this);

	vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), wszComponentName, sizeof(wszComponentName));

	kv = new KeyValues("EditConstraintEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("constraintConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditConstraint"), 1, wszComponentName);
	menu->AddMenuItem("EditConstraintEx", szBuf, kv, this);

	kv = new KeyValues("MoveConstraintEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("constraintConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_MoveConstraint"), 1, wszComponentName);
	menu->AddMenuItem("MoveConstraintEx", szBuf, kv, this);

	kv = new KeyValues("RotateConstraintEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("constraintConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_RotateConstraint"), 1, wszComponentName);
	menu->AddMenuItem("RotateConstraintEx", szBuf, kv, this);

	kv = new KeyValues("CloneConstraintEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("constraintConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CloneConstraint"), 1, wszComponentName);
	menu->AddMenuItem("CloneConstraintEx", szBuf, kv, this);

	kv = new KeyValues("DeleteConstraintEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("constraintConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteConstraint"), 1, wszComponentName);
	menu->AddMenuItem("DeleteConstraintEx", szBuf, kv, this);

	vgui::Menu::PlaceContextMenu(this, menu);

	return true;
}

bool CPhysicDebugGUI::OpenPhysicBehaviorMenu(IPhysicComponent* pPhysicComponent)
{
	auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

	if (pPhysicObject && (pPhysicObject->GetObjectFlags() & PhysicObjectFlag_FromConfig))
	{

	}
	else
	{
		return false;
	}

	auto menu = new vgui::Menu(this, "contextmenu");

	menu->SetAutoDelete(true);

	char szFileName[64] = { 0 };
	wchar_t wszFileName[64] = { 0 };
	wchar_t wszComponentName[64] = { 0 };
	wchar_t szBuf[256] = { 0 };

	V_FileBase(pPhysicObject->GetModel()->name, szFileName, sizeof(szFileName));
	vgui::localize()->ConvertANSIToUnicode(szFileName, wszFileName, sizeof(wszFileName));

	auto kv = new KeyValues("EditPhysicObject");

	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicObject"), 1, wszFileName);

	menu->AddMenuItem("EditPhysicObject", szBuf, kv, this);

	vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), wszComponentName, sizeof(wszComponentName));

	kv = new KeyValues("EditPhysicBehaviorEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("physicBehaviorConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicBehavior"), 1, wszComponentName);
	menu->AddMenuItem("EditPhysicBehaviorEx", szBuf, kv, this);

	kv = new KeyValues("MovePhysicBehaviorEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("physicBehaviorConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_MovePhysicBehavior"), 1, wszComponentName);
	menu->AddMenuItem("MovePhysicBehaviorEx", szBuf, kv, this);

	kv = new KeyValues("RotatePhysicBehaviorEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("physicBehaviorConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_RotatePhysicBehavior"), 1, wszComponentName);
	menu->AddMenuItem("RotatePhysicBehaviorEx", szBuf, kv, this);

	kv = new KeyValues("ClonePhysicBehaviorEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("physicBehaviorConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ClonePhysicBehavior"), 1, wszComponentName);
	menu->AddMenuItem("ClonePhysicBehaviorEx", szBuf, kv, this);

	kv = new KeyValues("DeletePhysicBehaviorEx");
	kv->SetUint64("physicObjectId", pPhysicObject->GetPhysicObjectId());
	kv->SetInt("physicObjectConfigId", pPhysicObject->GetPhysicConfigId());
	kv->SetInt("physicComponentId", pPhysicComponent->GetPhysicComponentId());
	kv->SetInt("physicBehaviorConfigId", pPhysicComponent->GetPhysicConfigId());
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeletePhysicBehavior"), 1, wszComponentName);
	menu->AddMenuItem("DeletePhysicBehaviorEx", szBuf, kv, this);

	vgui::Menu::PlaceContextMenu(this, menu);

	return true;
}

bool CPhysicDebugGUI::OpenInspectPhysicComponentMenu(bool bSelected)
{
	int physicComponentId = bSelected ? ClientPhysicManager()->GetSelectedPhysicComponentId() : ClientPhysicManager()->GetInspectedPhysicComponentId();

	if (physicComponentId > 0)
	{
		auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

		if (pPhysicComponent)
		{
			if (pPhysicComponent->IsRigidBody())
			{
				return OpenRigidBodyMenu(pPhysicComponent);
			}
			else if (pPhysicComponent->IsConstraint())
			{
				return OpenConstraintMenu(pPhysicComponent);
			}
			else if (pPhysicComponent->IsPhysicBehavior())
			{
				return OpenPhysicBehaviorMenu(pPhysicComponent);
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

void CPhysicDebugGUI::ShowInspectContentLabel3(const wchar_t* wszText)
{
	m_pInspectContentLabel3->SetText(wszText);
	m_pInspectContentLabel3->SetVisible(true);
}

void CPhysicDebugGUI::HideInspectContentLabel3()
{
	m_pInspectContentLabel3->SetVisible(false);
}

void CPhysicDebugGUI::Activate()
{
	BaseClass::Activate();

	Reset();
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
	if (m_pReload->HasFocus())
	{
		return true;
	}
	if (m_pSave->HasFocus())
	{
		return true;
	}
	if (m_pInspectMode->HasFocus())
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
	m_pInspectContentLabel3->SetFont(pScheme->GetFont("EngineFont", IsProportional()));
	m_pInspectModeLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));
	m_pEditModeLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));

	m_pInspectContentLabel->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectContentLabel2->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectContentLabel3->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectModeLabel->SetFgColor(Color(255, 255, 255, 255));
	m_pEditModeLabel->SetFgColor(Color(255, 255, 255, 255));

	m_pInspectContentLabel->SetPaintBackgroundEnabled(false);
	m_pInspectContentLabel2->SetPaintBackgroundEnabled(false);
	m_pInspectContentLabel3->SetPaintBackgroundEnabled(false);
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
	BV_SaveConfigs_f();
}

void CPhysicDebugGUI::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		
	}
	else if (!strcmp(command, "Reload"))
	{
		BV_ReloadAll_f();
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

void CPhysicDebugGUI::OnCreateStaticObject(uint64 physicObjectId)
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (pPhysicConfig)
		return;

	auto pConfig = ClientPhysicManager()->CreateEmptyPhysicObjectConfigForModelIndex(modelindex, PhysicObjectType_StaticObject);

	pConfig->flags |= PhysicObjectFlag_FromConfig;
}

void CPhysicDebugGUI::OnCreateDynamicObject(uint64 physicObjectId)
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (pPhysicConfig)
		return;

	auto pConfig = ClientPhysicManager()->CreateEmptyPhysicObjectConfigForModelIndex(modelindex, PhysicObjectType_DynamicObject);

	pConfig->flags |= PhysicObjectFlag_FromConfig;
}

void CPhysicDebugGUI::OnCreateRagdollObject(uint64 physicObjectId)
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (pPhysicConfig)
		return;

	auto pConfig = ClientPhysicManager()->CreateEmptyPhysicObjectConfigForModelIndex(modelindex, PhysicObjectType_RagdollObject);

	pConfig->flags |= PhysicObjectFlag_FromConfig;
}

void CPhysicDebugGUI::OnEnableDebugAnim(uint64 physicObjectId)
{
	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

	if (!pPhysicObject)
		return;

	if (!pPhysicObject->IsRagdollObject())
		return;

	auto pRagdollObject = (IRagdollObject*)pPhysicObject;

	pRagdollObject->SetDebugAnimEnabled(true);
}

void CPhysicDebugGUI::OnDisableDebugAnim(uint64 physicObjectId)
{
	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

	if (!pPhysicObject)
		return;

	if (!pPhysicObject->IsRagdollObject())
		return;

	auto pRagdollObject = (IRagdollObject*)pPhysicObject;

	pRagdollObject->SetDebugAnimEnabled(false);
}

void CPhysicDebugGUI::OnCreateRigidBody(uint64 physicObjectId)
{
	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

	if (!pPhysicObject)
		return;

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

	if (!pPhysicObjectConfig)
		return;

	if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto pRigidBodyConfig = UTIL_CreateEmptyRigidBodyConfig();

	pPhysicObjectConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

	pPhysicObjectConfig->configModified = true;

	ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
}

void CPhysicDebugGUI::OnEditPhysicObject(KeyValues *kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");

	OpenEditPhysicObjectDialogEx(physicObjectId, physicObjectConfigId);
}

bool CPhysicDebugGUI::OpenEditPhysicObjectDialog(uint64 physicObjectId)
{
	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

	if (!pPhysicObject)
		return false;

	auto pPhysicConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

	if (!pPhysicConfig)
		return false;

	if (!(pPhysicConfig->flags & PhysicObjectFlag_FromConfig))
		return false;

	auto dialog = new CPhysicEditorDialog(this, "PhysicEditorDialog", physicObjectId, pPhysicConfig);
	AddActionSignalTarget(dialog);
	dialog->DoModal(); 
	
	return true;
}

bool CPhysicDebugGUI::OpenEditPhysicObjectDialogEx(uint64 physicObjectId, int physicObjectConfigId)
{
	auto pPhysicConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (!pPhysicConfig)
		return false;

	if (!(pPhysicConfig->flags & PhysicObjectFlag_FromConfig))
		return false;

	auto dialog = new CPhysicEditorDialog(this, "PhysicEditorDialog", physicObjectId, pPhysicConfig);
	AddActionSignalTarget(dialog);
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

	auto dialog = new CPhysicRigidBodyEditDialog(this, "RigidBodyEditDialog", physicObjectId, pPhysicObjectConfig, pRigidBodyConfig);
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

void CPhysicDebugGUI::OnCloneRigidBodyEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto rigidBodyConfigId = kv->GetInt("rigidBodyConfigId");

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (!pPhysicObjectConfig)
		return;

	if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(rigidBodyConfigId);

	if (!pRigidBodyConfig)
		return;

	auto pClonedRigidBodyConfig = UTIL_CloneRigidBodyConfig(pRigidBodyConfig.get());

	pClonedRigidBodyConfig->name = std::format("{0}_Clone ({1})", pRigidBodyConfig->name, pClonedRigidBodyConfig->configId);

	pClonedRigidBodyConfig->configModified = true;

	pPhysicObjectConfig->RigidBodyConfigs.emplace_back(pClonedRigidBodyConfig);

	pPhysicObjectConfig->configModified = true;

	UTIL_RebuildPhysicObjectWithClonedConfig(physicObjectId, pPhysicObjectConfig.get(), pClonedRigidBodyConfig->configId);
}

bool CPhysicDebugGUI::DeleteRigidBodyByComponent(IPhysicComponent* pPhysicComponent)
{
	if (pPhysicComponent && pPhysicComponent->IsRigidBody())
	{
		auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

		if (pPhysicObject)
		{
			auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

			if (pPhysicObjectConfig)
			{
				if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
					return false;

				if (UTIL_RemoveRigidBodyFromPhysicObjectConfig(pPhysicObjectConfig.get(), pPhysicComponent->GetPhysicConfigId()))
				{
					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
		}
	}

	return false;
}

bool CPhysicDebugGUI::DeleteRigidBodyByComponentId(int physicComponentId)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		return DeleteRigidBodyByComponent(pPhysicComponent);
	}

	return false;
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

bool CPhysicDebugGUI::UpdateConfigOrigin(int physicComponentId, int axis, float value)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

		auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

		if (pPhysicObject)
		{
			if (pPhysicComponent->IsRigidBody())
			{
				auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

				if (pRigidBodyConfig && pPhysicObjectConfig)
				{
					pRigidBodyConfig->origin[axis] += value;
					pRigidBodyConfig->configModified = true;

					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
			else if (pPhysicComponent->IsConstraint())
			{
				auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

				if (pConstraintConfig && pPhysicObjectConfig)
				{
					if(pConstraintConfig->useGlobalJointFromA)
						pConstraintConfig->originA[axis] += value;
					else
						pConstraintConfig->originB[axis] += value;
					pConstraintConfig->configModified = true;

					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
			else if (pPhysicComponent->IsPhysicBehavior())
			{
				auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

				if (pPhysicBehaviorConfig && pPhysicObjectConfig)
				{
					pPhysicBehaviorConfig->origin[axis] += value;
					pPhysicBehaviorConfig->configModified = true;

					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
		}
	}

	return false;
}

bool CPhysicDebugGUI::UpdateConfigAngles(int physicComponentId, int axis, float value)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

		auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

		if (pPhysicObject)
		{
			if (pPhysicComponent->IsRigidBody())
			{
				auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

				if (pRigidBodyConfig && pPhysicObjectConfig)
				{
					pRigidBodyConfig->angles[axis] += value;
					pRigidBodyConfig->configModified = true;

					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
			else if (pPhysicComponent->IsConstraint())
			{
				auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

				if (pConstraintConfig && pPhysicObjectConfig)
				{
					if (pConstraintConfig->useGlobalJointFromA)
						pConstraintConfig->anglesA[axis] += value;
					else
						pConstraintConfig->anglesB[axis] += value;

					pConstraintConfig->configModified = true;

					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
			else if (pPhysicComponent->IsPhysicBehavior())
			{
				auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

				if (pPhysicBehaviorConfig && pPhysicObjectConfig)
				{
					pPhysicBehaviorConfig->angles[axis] += value;
					pPhysicBehaviorConfig->configModified = true;

					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
		}
	}

	return false;
}

bool CPhysicDebugGUI::UpdateConfigSize(int physicComponentId, int axis, float value)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

		if (pPhysicObject)
		{
			auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

			if (pPhysicComponent->IsRigidBody())
			{
				auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(pPhysicComponent->GetPhysicConfigId());

				if (pRigidBodyConfig && pPhysicObjectConfig)
				{
					if (pRigidBodyConfig->collisionShape)
					{
						pRigidBodyConfig->collisionShape->size[axis] += value;
						pRigidBodyConfig->collisionShape->configModified = true;
					}

					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
		}
	}

	return false;
}

//Constraint Event

bool CPhysicDebugGUI::OpenEditConstraintDialog(uint64 physicObjectId, int physicObjectConfigId, int constraintConfigId)
{
	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (!pPhysicObjectConfig)
		return false;

	auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(constraintConfigId);

	if (!pConstraintConfig)
		return false;

	auto dialog = new CPhysicConstraintEditDialog(this, "PhysicConstraintEditDialog", physicObjectId, pPhysicObjectConfig, pConstraintConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();

	return true;
}

void CPhysicDebugGUI::OnCreateConstraint(uint64 physicObjectId)
{
	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

	if (!pPhysicObject)
		return;

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

	if (!pPhysicObjectConfig)
		return;

	if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto pConstraintConfig = UTIL_CreateEmptyConstraintConfig();

	pPhysicObjectConfig->ConstraintConfigs.push_back(pConstraintConfig);

	pPhysicObjectConfig->configModified = true;

	ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
}

void CPhysicDebugGUI::OnEditConstraintEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto constraintConfigId = kv->GetInt("constraintConfigId");

	if (OpenEditConstraintDialog(physicObjectId, physicObjectConfigId, constraintConfigId))
	{
		ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	}
}

void CPhysicDebugGUI::OnMoveConstraintEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto constraintConfigId = kv->GetInt("constraintConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Move);
}

void CPhysicDebugGUI::OnRotateConstraintEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto constraintConfigId = kv->GetInt("constraintConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Rotate);
}

void CPhysicDebugGUI::OnResizeConstraintEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto constraintConfigId = kv->GetInt("constraintConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Resize);
}

void CPhysicDebugGUI::OnCloneConstraintEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto constraintConfigId = kv->GetInt("constraintConfigId");

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (!pPhysicObjectConfig)
		return;

	if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(constraintConfigId);

	if (!pConstraintConfig)
		return;

	auto pClonedConstraintConfig = UTIL_CloneConstraintConfig(pConstraintConfig.get());

	pClonedConstraintConfig->name = std::format("{0}_Clone ({1})", pConstraintConfig->name, pClonedConstraintConfig->configId);

	pClonedConstraintConfig->configModified = true;

	pPhysicObjectConfig->ConstraintConfigs.emplace_back(pClonedConstraintConfig);

	pPhysicObjectConfig->configModified = true;

	//Update PhysicObject

	UTIL_RebuildPhysicObjectWithClonedConfig(physicObjectId, pPhysicObjectConfig.get(), pClonedConstraintConfig->configId);
}

bool CPhysicDebugGUI::DeleteConstraintByComponent(IPhysicComponent *pPhysicComponent)
{
	if (pPhysicComponent && pPhysicComponent->IsConstraint())
	{
		auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

		if (pPhysicObject)
		{
			auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

			if (pPhysicObjectConfig)
			{
				if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
					return false;

				if (UTIL_RemoveConstraintFromPhysicObjectConfig(pPhysicObjectConfig.get(), pPhysicComponent->GetPhysicConfigId()))
				{
					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
		}
	}

	return false;
}

bool CPhysicDebugGUI::DeleteConstraintByComponentId(int physicComponentId)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		return DeleteConstraintByComponent(pPhysicComponent);
	}

	return false;
}

void CPhysicDebugGUI::OnDeleteConstraintEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto constraintConfigId = kv->GetInt("constraintConfigId");

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (pPhysicObjectConfig)
	{
		if (UTIL_RemoveConstraintFromPhysicObjectConfig(pPhysicObjectConfig.get(), constraintConfigId))
		{
			ClientPhysicManager()->RebuildPhysicObjectEx(physicObjectId, pPhysicObjectConfig.get());
		}
	}
}

//PhysicBehavior Event

bool CPhysicDebugGUI::OpenEditPhysicBehaviorDialog(uint64 physicObjectId, int physicObjectConfigId, int physicBehaviorConfigId)
{
	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (!pPhysicObjectConfig)
		return false;

	auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(physicBehaviorConfigId);

	if (!pPhysicBehaviorConfig)
		return false;

	auto dialog = new CPhysicBehaviorEditDialog(this, "PhysicPhysicBehaviorEditDialog", physicObjectId, pPhysicObjectConfig, pPhysicBehaviorConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();

	return true;
}


void CPhysicDebugGUI::OnCreatePhysicBehavior(uint64 physicObjectId)
{
	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(physicObjectId);

	if (!pPhysicObject)
		return;

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

	if (!pPhysicObjectConfig)
		return;

	if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto pPhysicBehaviorConfig = UTIL_CreateEmptyPhysicBehaviorConfig();

	pPhysicObjectConfig->PhysicBehaviorConfigs.push_back(pPhysicBehaviorConfig);

	pPhysicObjectConfig->configModified = true;

	ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
}

void CPhysicDebugGUI::OnEditPhysicBehaviorEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto physicBehaviorConfigId = kv->GetInt("physicBehaviorConfigId");

	if (OpenEditPhysicBehaviorDialog(physicObjectId, physicObjectConfigId, physicBehaviorConfigId))
	{
		ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	}
}

void CPhysicDebugGUI::OnMovePhysicBehaviorEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto physicBehaviorConfigId = kv->GetInt("physicBehaviorConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Move);
}

void CPhysicDebugGUI::OnRotatePhysicBehaviorEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto physicBehaviorConfigId = kv->GetInt("physicBehaviorConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Rotate);
}

void CPhysicDebugGUI::OnResizePhysicBehaviorEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto physicBehaviorConfigId = kv->GetInt("physicBehaviorConfigId");

	ClientPhysicManager()->SetSelectedPhysicComponentId(physicComponentId);
	UpdateEditMode(PhysicEditMode::Resize);
}

void CPhysicDebugGUI::OnClonePhysicBehaviorEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto physicBehaviorConfigId = kv->GetInt("physicBehaviorConfigId");

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (!pPhysicObjectConfig)
		return;

	if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(physicBehaviorConfigId);

	if (!pPhysicBehaviorConfig)
		return;

	auto pClonedPhysicBehaviorConfig = UTIL_ClonePhysicBehaviorConfig(pPhysicBehaviorConfig.get());

	pClonedPhysicBehaviorConfig->name = std::format("{0}_Clone ({1})", pPhysicBehaviorConfig->name, pClonedPhysicBehaviorConfig->configId);

	pClonedPhysicBehaviorConfig->configModified = true;

	pPhysicObjectConfig->PhysicBehaviorConfigs.emplace_back(pClonedPhysicBehaviorConfig);

	pPhysicObjectConfig->configModified = true;

	//Update PhysicObject

	UTIL_RebuildPhysicObjectWithClonedConfig(physicObjectId, pPhysicObjectConfig.get(), pClonedPhysicBehaviorConfig->configId);
}

bool CPhysicDebugGUI::DeletePhysicBehaviorByComponent(IPhysicComponent* pPhysicComponent)
{
	if (pPhysicComponent && pPhysicComponent->IsPhysicBehavior())
	{
		auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

		if (pPhysicObject)
		{
			auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObject->GetPhysicConfigId());

			if (pPhysicObjectConfig)
			{
				if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
					return false;

				if (UTIL_RemovePhysicBehaviorFromPhysicObjectConfig(pPhysicObjectConfig.get(), pPhysicComponent->GetPhysicConfigId()))
				{
					return ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig.get());
				}
			}
		}
	}

	return false;
}

bool CPhysicDebugGUI::DeletePhysicBehaviorByComponentId(int physicComponentId)
{
	auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

	if (pPhysicComponent)
	{
		return DeletePhysicBehaviorByComponent(pPhysicComponent);
	}

	return false;
}

void CPhysicDebugGUI::OnDeletePhysicBehaviorEx(KeyValues* kv)
{
	auto physicObjectId = kv->GetUint64("physicObjectId");
	auto physicObjectConfigId = kv->GetInt("physicObjectConfigId");
	auto physicComponentId = kv->GetInt("physicComponentId");
	auto PhysicBehaviorConfigId = kv->GetInt("PhysicBehaviorConfigId");

	auto pPhysicObjectConfig = UTIL_GetPhysicObjectConfigFromConfigId(physicObjectConfigId);

	if (pPhysicObjectConfig)
	{
		if (UTIL_RemovePhysicBehaviorFromPhysicObjectConfig(pPhysicObjectConfig.get(), PhysicBehaviorConfigId))
		{
			ClientPhysicManager()->RebuildPhysicObjectEx(physicObjectId, pPhysicObjectConfig.get());
		}
	}
}