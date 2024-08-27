#include "PhysicEditorDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

//RigidBody List

CRigidBodyListPanel::CRigidBodyListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{

}

//RigidBody Page

CRigidBodyPage::CRigidBodyPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pRigidBodyListPanel = new CRigidBodyListPanel(this, "RigidBodyListPanel");

	m_pRigidBodyListPanel->AddColumnHeader(0, "index", "#BulletPhysics_Index", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	m_pRigidBodyListPanel->AddColumnHeader(1, "configId", "#BulletPhysics_ConfigId", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	m_pRigidBodyListPanel->AddColumnHeader(2, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(3, "shape", "#BulletPhysics_Shape", vgui::scheme()->GetProportionalScaledValue(60), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(4, "bone", "#BulletPhysics_Bone", vgui::scheme()->GetProportionalScaledValue(180), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(5, "mass", "#BulletPhysics_Mass", vgui::scheme()->GetProportionalScaledValue(60), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(6, "flags", "#BulletPhysics_Flags", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	m_pRigidBodyListPanel->SetSortColumn(0);
	m_pRigidBodyListPanel->SetMultiselectEnabled(false);

	m_pShiftUpRigidBody = new vgui::Button(this, "ShiftUpRigidBody", L"#BulletPhysics_ShiftUp", this, "ShiftUpRigidBody");
	m_pShiftDownRigidBody = new vgui::Button(this, "ShiftDownRigidBody", L"#BulletPhysics_ShiftDown", this, "ShiftDownRigidBody");
	m_pCreateRigidBody = new vgui::Button(this, "CreateRigidBody", L"#BulletPhysics_CreateRigidBody", this, "CreateRigidBody");

	LoadControlSettings("bulletphysics/RigidBodyPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CRigidBodyPage::OnResetData()
{
	ReloadAllRigidBodiesIntoListPanelItem();
}

void CRigidBodyPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pRigidBodyListPanel->SetFont(m_hFont);
}

void CRigidBodyPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pRigidBodyListPanel->GetSelectedItemsCount() > 0)
		{
			auto selectItemId = m_pRigidBodyListPanel->GetSelectedItem(0);

			auto configId = m_pRigidBodyListPanel->GetItemUserData(selectItemId);

			OnOpenRigidBodyEditor(configId);

			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CRigidBodyPage::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{

	}
	else if (!stricmp(command, "CreateRigidBody"))
	{
		OnCreateRigidBody();
	}
	else if (!stricmp(command, "ShiftUpRigidBody"))
	{
		auto selectItemId = m_pRigidBodyListPanel->GetSelectedItem(0);
		auto configId = m_pRigidBodyListPanel->GetItemUserData(selectItemId);
		OnShiftUpRigidBody(configId);
	}
	else if (!stricmp(command, "ShiftDownRigidBody"))
	{
		auto selectItemId = m_pRigidBodyListPanel->GetSelectedItem(0);
		auto configId = m_pRigidBodyListPanel->GetItemUserData(selectItemId);
		OnShiftDownRigidBody(configId);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CRigidBodyPage::OnOpenContextMenu(int itemId)
{
	if (!m_pRigidBodyListPanel->GetSelectedItemsCount())
		return;

	auto selectItemId = m_pRigidBodyListPanel->GetSelectedItem(0);

	auto configId = m_pRigidBodyListPanel->GetItemUserData(selectItemId);

	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto menu = new vgui::Menu(this, "contextmenu");

	menu->SetAutoDelete(true);

	wchar_t szName[64] = { 0 };
	wchar_t szBuf[256] = { 0 };
	vgui::localize()->ConvertANSIToUnicode(pRigidBodyConfig->name.c_str(), szName, sizeof(szName));

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditRigidBody"), 1, szName);
	menu->AddMenuItem("EditRigidBody", szBuf, new KeyValues("EditRigidBody", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CloneRigidBody"), 1, szName);
	menu->AddMenuItem("CloneRigidBody", szBuf, new KeyValues("CloneRigidBody", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftUpRigidBody"), 1, szName);
	menu->AddMenuItem("ShiftUpRigidBody", szBuf, new KeyValues("ShiftUpRigidBody", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftDownRigidBody"), 1, szName);
	menu->AddMenuItem("ShiftDownRigidBody", szBuf, new KeyValues("ShiftDownRigidBody", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteRigidBody"), 1, szName);
	menu->AddMenuItem("DeleteRigidBody", szBuf, new KeyValues("DeleteRigidBody", "configId", configId), this);

	vgui::Menu::PlaceContextMenu(this, menu);
}

void CRigidBodyPage::OnOpenRigidBodyEditor(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto dialog = new CRigidBodyEditDialog(this, "RigidBodyEditDialog", m_physicObjectId, m_pPhysicObjectConfig, pRigidBodyConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
}

void CRigidBodyPage::OnEditRigidBody(int configId)
{
	OnOpenRigidBodyEditor(configId);
}

void CRigidBodyPage::OnRefreshRigidBody(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	DeleteRigidBodyItem(configId);

	LoadRigidBodyAsListPanelItem(pRigidBodyConfig.get());
}

void CRigidBodyPage::OnRefreshRigidBodies()
{
	ReloadAllRigidBodiesIntoListPanelItem();
}

void CRigidBodyPage::LoadRigidBodyAsListPanelItem(const CClientRigidBodyConfig* pRigidBodyConfig)
{
	auto kv = new KeyValues("RigidBody");

	kv->SetInt("index", UTIL_GetRigidBodyIndex(m_pPhysicObjectConfig.get(), pRigidBodyConfig));

	kv->SetInt("configId", pRigidBodyConfig->configId);

	kv->SetString("name", pRigidBodyConfig->name.c_str());

	kv->SetString("shape", UTIL_GetCollisionShapeTypeLocalizationToken(pRigidBodyConfig->collisionShape->type));

	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(m_physicObjectId);

	auto bone = UTIL_GetFormattedBoneName(modelindex, pRigidBodyConfig->boneindex);

	kv->SetString("bone", bone.c_str());

	kv->SetFloat("mass", pRigidBodyConfig->mass);

	auto flags = UTIL_GetFormattedRigidBodyFlags(pRigidBodyConfig->flags);

	kv->SetWString("flags", flags.c_str());

	m_pRigidBodyListPanel->AddItem(kv, pRigidBodyConfig->configId, false, true);

	kv->deleteThis();
}

void CRigidBodyPage::ReloadAllRigidBodiesIntoListPanelItem()
{
	m_pRigidBodyListPanel->RemoveAll();

	for (const auto& pConfig : m_pPhysicObjectConfig->RigidBodyConfigs)
	{
		LoadRigidBodyAsListPanelItem(pConfig.get());
	}
}

void CRigidBodyPage::SelectRigidBodyItem(int configId)
{
	for (int i = 0; i < m_pRigidBodyListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pRigidBodyListPanel->GetItemUserData(i);

		if (userData == configId)
		{
			m_pRigidBodyListPanel->SetSingleSelectedItem(i);
			break;
		}
	}
}

void CRigidBodyPage::DeleteRigidBodyItem(int configId)
{
	for (int i = 0; i < m_pRigidBodyListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pRigidBodyListPanel->GetItemUserData(i);

		if (userData == configId)
		{
			m_pRigidBodyListPanel->RemoveItem(i);
			break;
		}
	}
}

void CRigidBodyPage::OnShiftUpRigidBody(int configId)
{
	if (UTIL_ShiftUpRigidBodyIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllRigidBodiesIntoListPanelItem();
		SelectRigidBodyItem(configId);
	}
}

void CRigidBodyPage::OnShiftDownRigidBody(int configId)
{
	if (UTIL_ShiftDownRigidBodyIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllRigidBodiesIntoListPanelItem();
		SelectRigidBodyItem(configId);
	}
}

void CRigidBodyPage::OnCreateRigidBody()
{
	auto pRigidBodyConfig = std::make_shared<CClientRigidBodyConfig>();

	pRigidBodyConfig->name = std::format("UnnamedRigidBody ({0})", pRigidBodyConfig->configId);
	pRigidBodyConfig->configModified = true;

	pRigidBodyConfig->collisionShape = std::make_shared<CClientCollisionShapeConfig>();
	pRigidBodyConfig->collisionShape->type = PhysicShape_Sphere;
	pRigidBodyConfig->collisionShape->size[0] = 3;
	pRigidBodyConfig->collisionShape->configModified = true;

	m_pPhysicObjectConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);
	m_pPhysicObjectConfig->configModified = true;

	ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

	LoadRigidBodyAsListPanelItem(pRigidBodyConfig.get());

	ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
}

void CRigidBodyPage::OnCloneRigidBody(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto pClonedRigidBodyConfig = UTIL_CloneRigidBodyConfig(pRigidBodyConfig.get());

	pClonedRigidBodyConfig->name = std::format("{0}_Clone ({1})", pRigidBodyConfig->name, pClonedRigidBodyConfig->configId);

	pClonedRigidBodyConfig->configModified = true;

	m_pPhysicObjectConfig->RigidBodyConfigs.emplace_back(pClonedRigidBodyConfig);

	m_pPhysicObjectConfig->configModified = true;

	ClientPhysicManager()->AddPhysicConfig(pClonedRigidBodyConfig->configId, pClonedRigidBodyConfig);

	LoadRigidBodyAsListPanelItem(pClonedRigidBodyConfig.get());

	//Update PhysicObject

	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(m_physicObjectId);

	if (pPhysicObject->Rebuild(m_pPhysicObjectConfig.get()))
	{
		int clonedPhysicComponentId = 0;

		pPhysicObject->EnumPhysicComponents([pClonedRigidBodyConfig, &clonedPhysicComponentId](IPhysicComponent* pPhysicCompoent) {

			if (pPhysicCompoent->GetPhysicConfigId() == pClonedRigidBodyConfig->configId)
			{
				clonedPhysicComponentId = pPhysicCompoent->GetPhysicComponentId();
				return true;
			}

			return false;
		});

		if (clonedPhysicComponentId) {
			ClientPhysicManager()->SetSelectedPhysicComponentId(clonedPhysicComponentId);
		}
	}
}

void CRigidBodyPage::OnDeleteRigidBody(int configId)
{
	DeleteRigidBodyItem(configId);

	if (UTIL_RemoveRigidBodyFromPhysicObjectConfig(m_pPhysicObjectConfig.get(), configId))
	{
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
	}
}