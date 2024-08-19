#include "PhysicDebugGUI.h"
#include <vgui_controls/TextImage.h>

#include "PhysicEditorDialog.h"

#include "CounterStrike.h"

#include "ClientEntityManager.h"
#include "ClientPhysicManager.h"

#include "exportfuncs.h"

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

	m_pInspectEntityLabel = new vgui::Label(this, "InspectEntityLabel", "");
	m_pInspectEntityLabel->SetVisible(false);

	m_pInspectPhysicComponentLabel = new vgui::Label(this, "InspectPhysicComponentLabel", "");
	m_pInspectPhysicComponentLabel->SetVisible(false);

	m_pClose = new vgui::Button(this, "Close", L"#GameUI_Close", this, "Close");

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	LoadControlSettings("bulletphysics/PhysicDebugGUI.res", "GAME");
}

CPhysicDebugGUI::~CPhysicDebugGUI()
{

}
#if 0
void CPhysicDebugGUI::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ESCAPE)
	{
		Close();
		return;
	}

	BaseClass::OnKeyCodeTyped(code);
}
#endif

void CPhysicDebugGUI::OnThink()
{
	BaseClass::OnThink();

	int entindex = ClientEntityManager()->GetInspectEntityIndex();
	int modelindex = ClientEntityManager()->GetInspectEntityModelIndex();
	int physicComponentId = ClientPhysicManager()->GetInspectPhysicComponentId();

	UpdateInspectEntity(entindex, modelindex);
	UpdateInspectPhysicComponent(physicComponentId);
}

void CPhysicDebugGUI::OnMousePressed(vgui::MouseCode code)
{
	if (code == vgui::MOUSE_RIGHT)
	{
		int entindex = ClientEntityManager()->GetInspectEntityIndex();
		int modelindex = ClientEntityManager()->GetInspectEntityModelIndex();
		int physicComponentId = ClientPhysicManager()->GetInspectPhysicComponentId();

		if (entindex > 0 && modelindex > 0)
		{
			auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

			if (pPhysicConfig && (pPhysicConfig->flags & PhysicObjectFlag_FromConfig))
			{
				auto model = EngineGetModelByIndex(modelindex);

				if (model && model->type == mod_studio)
				{
					auto menu = new vgui::Menu(this, "contextmenu");

					menu->SetAutoDelete(true);

					auto command_0 = std::format("EditPhysicObject|{0}|{1}", entindex, modelindex);

					menu->AddMenuItem("#BulletPhysics_EditPhysicObject", new KeyValues("Command", "command", command_0.c_str()), this);

					if (physicComponentId > 0)
					{
						auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(physicComponentId);

						if (pPhysicComponent && pPhysicComponent->IsRigidBody())
						{
							wchar_t szName[64] = { 0 };
							wchar_t szBuf[256] = { 0 };
							
							vgui::localize()->ConvertANSIToUnicode(pPhysicComponent->GetName(), szName, sizeof(szName));

							auto command_EditRigidBodyEx = std::format("EditRigidBodyEx|{0}|{1}|{2}", pPhysicComponent->GetPhysicConfigId(), entindex, modelindex);
							vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditRigidBody"), 1, szName);
							menu->AddMenuItem("EditRigidBodyEx", szBuf, new KeyValues("Command", "command", command_EditRigidBodyEx.c_str()), this);

							auto command_DeleteRigidBodyEx = std::format("DeleteRigidBodyEx|{0}|{1}|{2}", pPhysicComponent->GetPhysicConfigId(), entindex, modelindex);
							vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteRigidBody"), 1, szName);
							menu->AddMenuItem("DeleteRigidBodyEx", szBuf, new KeyValues("Command", "command", command_DeleteRigidBodyEx.c_str()), this);
						}
					}

					vgui::Menu::PlaceContextMenu(this, menu);
					return;
				}
			}
			else
			{
				auto model = EngineGetModelByIndex(modelindex);

				if (model && model->type == mod_studio)
				{
					auto menu = new vgui::Menu(this, "contextmenu");

					menu->SetAutoDelete(true);

					auto command_0 = std::format("CreateStaticObject|{0}|{1}", entindex, modelindex);
					auto command_1 = std::format("CreateDynamicObject|{0}|{1}", entindex, modelindex);
					auto command_2 = std::format("CreateRagdollObject|{0}|{1}", entindex, modelindex);

					menu->AddMenuItem("#BulletPhysics_CreateStaticObject", new KeyValues("Command", "command", command_0.c_str()), this);
					menu->AddMenuItem("#BulletPhysics_CreateDynamicObject", new KeyValues("Command", "command", command_1.c_str()), this);
					menu->AddMenuItem("#BulletPhysics_CreateRagdollObject", new KeyValues("Command", "command", command_2.c_str()), this);

					vgui::Menu::PlaceContextMenu(this, menu);
					return;
				}
			}
		}
	}

	BaseClass::OnMousePressed(code);
}

void CPhysicDebugGUI::OnMouseDoublePressed(vgui::MouseCode code)
{
	if (code == vgui::MOUSE_LEFT && vgui::input()->IsKeyDown(vgui::KEY_LCONTROL))
	{
		int entindex = ClientEntityManager()->GetInspectEntityIndex();
		int modelindex = ClientEntityManager()->GetInspectEntityModelIndex();

		if (entindex > 0 && modelindex > 0)
		{
			auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

			if (pPhysicConfig && (pPhysicConfig->flags & PhysicObjectFlag_FromConfig))
			{
				auto model = EngineGetModelByIndex(modelindex);

				if (model && model->type == mod_studio)
				{
					OpenEditPhysicObjectDialog(entindex, modelindex);
				}
			}
		}
	}
	else if (code == vgui::MOUSE_LEFT && vgui::input()->IsKeyDown(vgui::KEY_LSHIFT))
	{


	}

	BaseClass::OnMouseDoublePressed(code);
}

void CPhysicDebugGUI::UpdateInspectEntity(int entindex, int modelindex)
{
	if (m_iCachedInspectEntityIndex != entindex || m_iCachedInspectModelIndex != modelindex)
	{
		m_iCachedInspectEntityIndex = entindex;
		m_iCachedInspectModelIndex = modelindex;

		auto model = EngineGetModelByIndex(modelindex);

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
				vgui::localize()->ConstructString(wszBuf, sizeof(wszBuf), vgui::localize()->Find("#BulletPhysics_InspectingPhysicObject"), 3, vgui::localize()->Find(pPhysicObject->GetTypeLocalizationTokenString()), wszIndex, wszModel);
			}
			else
			{
				vgui::localize()->ConstructString(wszBuf, sizeof(wszBuf), vgui::localize()->Find("#BulletPhysics_InspectingEntity"), 2, wszIndex, wszModel);
			}

			SetInspectEntityLabelText(wszBuf);
			ShowInspectEntityLabel(true);
		}
		else
		{
			SetInspectEntityLabelText(L"");
			ShowInspectEntityLabel(false);
		}
	}
}

void CPhysicDebugGUI::UpdateInspectPhysicComponent(int physicComponentId)
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

			SetInspectPhysicComponentLabelText(wszBuf);
			ShowInspectPhysicComponentLabel(true);
		}
		else
		{
			SetInspectPhysicComponentLabelText(L"");
			ShowInspectPhysicComponentLabel(false);
		}
	}
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

void CPhysicDebugGUI::NewMap()
{
	m_iCachedInspectEntityIndex = 0;
	m_iCachedInspectModelIndex = 0;
	m_iCachedInspectPhysicComponentId = 0;
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

	m_pInspectEntityLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));
	m_pInspectPhysicComponentLabel->SetFont(pScheme->GetFont("EngineFont", IsProportional()));

	m_pInspectEntityLabel->SetFgColor(Color(255, 255, 255, 255));
	m_pInspectPhysicComponentLabel->SetFgColor(Color(255, 255, 255, 255));

	m_pInspectEntityLabel->SetPaintBackgroundEnabled(false);
	m_pInspectPhysicComponentLabel->SetPaintBackgroundEnabled(false);
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
	else if (!strncmp(command, "CreateRagdollObject|", sizeof("CreateRagdollObject|") - 1))
	{
		OnCreateRagdollObject(command + sizeof("CreateRagdollObject|") - 1);
	}
	else if (!strncmp(command, "EditPhysicObject|", sizeof("EditPhysicObject|") - 1))
	{
		OnEditPhysicObject(command + sizeof("EditPhysicObject|") - 1);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicDebugGUI::OnCreateRagdollObject(const char* command)
{
	int entindex{};
	int modelindex{};

	if (2 != sscanf_s(command, "%d|%d", &entindex, &modelindex))
		return;

	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (pPhysicConfig)
		return;

	//TODO
}

void CPhysicDebugGUI::OnEditPhysicObject(const char* command)
{
	int entindex{};
	int modelindex{};

	if (2 != sscanf_s(command, "%d|%d", &entindex, &modelindex))
		return;

	OpenEditPhysicObjectDialog(entindex, modelindex);
}

void CPhysicDebugGUI::OpenEditPhysicObjectDialog(int entindex, int modelindex)
{
	auto pPhysicConfig = ClientPhysicManager()->GetPhysicObjectConfigForModelIndex(modelindex);

	if (!pPhysicConfig)
		return;

	if (!(pPhysicConfig->flags & PhysicObjectFlag_FromConfig))
		return;

	auto dialog = new CPhysicEditorDialog(this, "PhysicEditorDialog", entindex, modelindex, pPhysicConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
}