#include "PhysicRigidBodyPage.h"
#include "PhysicRigidBodyListPanel.h"
#include "PhysicRigidBodyEditDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

CPhysicRigidBodyPage::CPhysicRigidBodyPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pPhysicRigidBodyListPanel = new CPhysicRigidBodyListPanel(this, "PhysicRigidBodyListPanel");

	m_pShiftUpRigidBody = new vgui::Button(this, "ShiftUpRigidBody", L"#BulletPhysics_ShiftUp", this, "ShiftUpRigidBody");
	m_pShiftDownRigidBody = new vgui::Button(this, "ShiftDownRigidBody", L"#BulletPhysics_ShiftDown", this, "ShiftDownRigidBody");
	m_pCreateRigidBody = new vgui::Button(this, "CreateRigidBody", L"#BulletPhysics_CreateRigidBody", this, "CreateRigidBody");

	LoadControlSettings("bulletphysics/PhysicRigidBodyPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicRigidBodyPage::OnResetData()
{
	ReloadAllRigidBodiesIntoListPanelItem();
}

void CPhysicRigidBodyPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pPhysicRigidBodyListPanel->SetFont(m_hFont);
}

void CPhysicRigidBodyPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pPhysicRigidBodyListPanel->GetSelectedItemsCount() > 0)
		{
			auto selectItemId = m_pPhysicRigidBodyListPanel->GetSelectedItem(0);

			auto configId = m_pPhysicRigidBodyListPanel->GetItemUserData(selectItemId);

			OnOpenRigidBodyEditor(configId);

			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicRigidBodyPage::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{

	}
	else if (!stricmp(command, "CreateRigidBody"))
	{
		OnCreateRigidBody();
	}
	else if (!stricmp(command, "DeleteSelectedRigidBody"))
	{
		auto selectItemId = m_pPhysicRigidBodyListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicRigidBodyListPanel->GetItemUserData(selectItemId);
		OnDeleteRigidBody(configId);
	}
	else if (!stricmp(command, "ShiftUpRigidBody"))
	{
		auto selectItemId = m_pPhysicRigidBodyListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicRigidBodyListPanel->GetItemUserData(selectItemId);
		OnShiftUpRigidBody(configId);
	}
	else if (!stricmp(command, "ShiftDownRigidBody"))
	{
		auto selectItemId = m_pPhysicRigidBodyListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicRigidBodyListPanel->GetItemUserData(selectItemId);
		OnShiftDownRigidBody(configId);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicRigidBodyPage::OnOpenContextMenu(int itemId)
{
	if (!m_pPhysicRigidBodyListPanel->GetSelectedItemsCount())
		return;

	auto selectItemId = m_pPhysicRigidBodyListPanel->GetSelectedItem(0);

	auto configId = m_pPhysicRigidBodyListPanel->GetItemUserData(selectItemId);

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

void CPhysicRigidBodyPage::OnOpenRigidBodyEditor(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto dialog = new CPhysicRigidBodyEditDialog(this, "PhysicRigidBodyEditDialog", m_physicObjectId, m_pPhysicObjectConfig, pRigidBodyConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
}

void CPhysicRigidBodyPage::OnEditRigidBody(int configId)
{
	OnOpenRigidBodyEditor(configId);
}

void CPhysicRigidBodyPage::OnRefreshRigidBody(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	DeleteRigidBodyItem(configId);

	LoadRigidBodyAsListPanelItem(pRigidBodyConfig.get());
}

void CPhysicRigidBodyPage::OnRefreshRigidBodies()
{
	ReloadAllRigidBodiesIntoListPanelItem();
}

void CPhysicRigidBodyPage::LoadRigidBodyAsListPanelItem(const CClientRigidBodyConfig* pRigidBodyConfig)
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

	m_pPhysicRigidBodyListPanel->AddItem(kv, pRigidBodyConfig->configId, false, true);

	kv->deleteThis();
}

void CPhysicRigidBodyPage::ReloadAllRigidBodiesIntoListPanelItem()
{
	m_pPhysicRigidBodyListPanel->RemoveAll();

	for (const auto& pConfig : m_pPhysicObjectConfig->RigidBodyConfigs)
	{
		LoadRigidBodyAsListPanelItem(pConfig.get());
	}
}

void CPhysicRigidBodyPage::SelectRigidBodyItem(int configId)
{
	for (int i = 0; i < m_pPhysicRigidBodyListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pPhysicRigidBodyListPanel->GetItemUserData(i);

		if (userData == configId)
		{
			m_pPhysicRigidBodyListPanel->SetSingleSelectedItem(i);
			break;
		}
	}
}

void CPhysicRigidBodyPage::DeleteRigidBodyItem(int configId)
{
	for (int i = 0; i < m_pPhysicRigidBodyListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pPhysicRigidBodyListPanel->GetItemUserData(i);

		if (userData == configId)
		{
			m_pPhysicRigidBodyListPanel->RemoveItem(i);
			break;
		}
	}
}

void CPhysicRigidBodyPage::OnShiftUpRigidBody(int configId)
{
	if (UTIL_ShiftUpRigidBodyIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllRigidBodiesIntoListPanelItem();
		SelectRigidBodyItem(configId);
	}
}

void CPhysicRigidBodyPage::OnShiftDownRigidBody(int configId)
{
	if (UTIL_ShiftDownRigidBodyIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllRigidBodiesIntoListPanelItem();
		SelectRigidBodyItem(configId);
	}
}

void CPhysicRigidBodyPage::OnCreateRigidBody()
{
	auto pRigidBodyConfig = UTIL_CreateEmptyRigidBodyConfig();

	m_pPhysicObjectConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

	m_pPhysicObjectConfig->configModified = true;

	LoadRigidBodyAsListPanelItem(pRigidBodyConfig.get());

	ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
}

void CPhysicRigidBodyPage::OnCloneRigidBody(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto pClonedRigidBodyConfig = UTIL_CloneRigidBodyConfig(pRigidBodyConfig.get());

	pClonedRigidBodyConfig->name = std::format("{0}_Clone ({1})", pRigidBodyConfig->name, pClonedRigidBodyConfig->configId);

	pClonedRigidBodyConfig->configModified = true;

	m_pPhysicObjectConfig->RigidBodyConfigs.emplace_back(pClonedRigidBodyConfig);

	m_pPhysicObjectConfig->configModified = true;

	LoadRigidBodyAsListPanelItem(pClonedRigidBodyConfig.get());

	UTIL_RebuildPhysicObjectWithClonedConfig(m_physicObjectId, m_pPhysicObjectConfig.get(), pClonedRigidBodyConfig->configId);
}

void CPhysicRigidBodyPage::OnDeleteRigidBody(int configId)
{
	//DeleteRigidBodyItem(configId);

	if (UTIL_RemoveRigidBodyFromPhysicObjectConfig(m_pPhysicObjectConfig.get(), configId))
	{
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());

		ReloadAllRigidBodiesIntoListPanelItem();
	}
}