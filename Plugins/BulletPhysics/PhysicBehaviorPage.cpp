#include "PhysicBehaviorPage.h"
#include "PhysicBehaviorListPanel.h"
#include "PhysicBehaviorEditDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

CPhysicBehaviorPage::CPhysicBehaviorPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pPhysicBehaviorListPanel = new CPhysicBehaviorListPanel(this, "PhysicBehaviorListPanel");

	m_pShiftUpPhysicBehavior = new vgui::Button(this, "ShiftUpPhysicBehavior", L"#BulletPhysics_ShiftUp", this, "ShiftUpPhysicBehavior");
	m_pShiftDownPhysicBehavior = new vgui::Button(this, "ShiftDownPhysicBehavior", L"#BulletPhysics_ShiftDown", this, "ShiftDownPhysicBehavior");
	m_pCreatePhysicBehavior = new vgui::Button(this, "CreatePhysicBehavior", L"#BulletPhysics_CreatePhysicBehavior", this, "CreatePhysicBehavior");

	LoadControlSettings("bulletphysics/PhysicBehaviorPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicBehaviorPage::OnResetData()
{
	ReloadAllPhysicBehaviorsIntoListPanelItem();
}

void CPhysicBehaviorPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pPhysicBehaviorListPanel->SetFont(m_hFont);
}

void CPhysicBehaviorPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pPhysicBehaviorListPanel->HasFocus() && m_pPhysicBehaviorListPanel->GetSelectedItemsCount() > 0)
		{
			auto selectItemId = m_pPhysicBehaviorListPanel->GetSelectedItem(0);
			auto configId = m_pPhysicBehaviorListPanel->GetItemUserData(selectItemId);
			OnOpenPhysicBehaviorEditor(configId);
			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicBehaviorPage::OnCommand(const char* command)
{
	if (!stricmp(command, "CreatePhysicBehavior"))
	{
		OnCreatePhysicBehavior();
	}
	else if (!stricmp(command, "ShiftUpPhysicBehavior"))
	{
		auto selectItemId = m_pPhysicBehaviorListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicBehaviorListPanel->GetItemUserData(selectItemId);
		OnShiftUpPhysicBehavior(configId);
	}
	else if (!stricmp(command, "ShiftDownPhysicBehavior"))
	{
		auto selectItemId = m_pPhysicBehaviorListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicBehaviorListPanel->GetItemUserData(selectItemId);
		OnShiftDownPhysicBehavior(configId);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicBehaviorPage::OnOpenContextMenu(int itemId)
{
	if (!m_pPhysicBehaviorListPanel->GetSelectedItemsCount())
		return;

	auto selectItemId = m_pPhysicBehaviorListPanel->GetSelectedItem(0);
	auto configId = m_pPhysicBehaviorListPanel->GetItemUserData(selectItemId);

	auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(configId);

	if (!pPhysicBehaviorConfig)
		return;

	vgui::Menu* menu = new vgui::Menu(this, "contextmenu");
	menu->SetAutoDelete(true);

	wchar_t szName[64] = { 0 };
	wchar_t szBuf[256] = { 0 };
	vgui::localize()->ConvertANSIToUnicode(pPhysicBehaviorConfig->name.c_str(), szName, sizeof(szName));

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicBehavior"), 1, szName);
	menu->AddMenuItem("EditPhysicBehavior", szBuf, new KeyValues("EditPhysicBehavior", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ClonePhysicBehavior"), 1, szName);
	menu->AddMenuItem("ClonePhysicBehavior", szBuf, new KeyValues("ClonePhysicBehavior", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftUpPhysicBehavior"), 1, szName);
	menu->AddMenuItem("ShiftUpPhysicBehavior", szBuf, new KeyValues("ShiftUpPhysicBehavior", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftDownPhysicBehavior"), 1, szName);
	menu->AddMenuItem("ShiftDownPhysicBehavior", szBuf, new KeyValues("ShiftDownPhysicBehavior", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeletePhysicBehavior"), 1, szName);
	menu->AddMenuItem("DeletePhysicBehavior", szBuf, new KeyValues("DeletePhysicBehavior", "configId", configId), this);

	vgui::Menu::PlaceContextMenu(this, menu);
}

void CPhysicBehaviorPage::OnRefreshPhysicBehavior(int configId)
{
	auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(configId);

	if (!pPhysicBehaviorConfig)
		return;

	DeletePhysicBehaviorItem(configId);

	// Re-add the updated item to the list panel
	LoadPhysicBehaviorAsListPanelItem(pPhysicBehaviorConfig.get());
}

void CPhysicBehaviorPage::OnEditPhysicBehavior(int configId)
{
	OnOpenPhysicBehaviorEditor(configId);
}

void CPhysicBehaviorPage::OnClonePhysicBehavior(int configId)
{
	// Retrieve the original PhysicBehavior configuration using the configId.
	auto originalPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(configId);

	if (!originalPhysicBehaviorConfig)
		return;

	// Create a deep copy of the PhysicBehavior configuration.
	// Assuming you have a utility function to clone PhysicBehavior configurations.
	auto clonedPhysicBehaviorConfig = UTIL_ClonePhysicBehaviorConfig(originalPhysicBehaviorConfig.get());

	if (!clonedPhysicBehaviorConfig)
		return;

	// Modify the name or any other parameters to indicate that this is a cloned version.
	clonedPhysicBehaviorConfig->name += " (Clone)";

	// Add the cloned configuration to the list of PhysicBehaviors in the physic object config.
	m_pPhysicObjectConfig->PhysicBehaviorConfigs.push_back(clonedPhysicBehaviorConfig);

	m_pPhysicObjectConfig->configModified = true;

	// Refresh the UI to include the new PhysicBehavior configuration.
	LoadPhysicBehaviorAsListPanelItem(clonedPhysicBehaviorConfig.get());

	// Optionally, select the newly added item in the list.
	SelectPhysicBehaviorItem(clonedPhysicBehaviorConfig->configId);

	//Update PhysicObject
	UTIL_RebuildPhysicObjectWithClonedConfig(m_physicObjectId, m_pPhysicObjectConfig.get(), clonedPhysicBehaviorConfig->configId);
}

void CPhysicBehaviorPage::LoadPhysicBehaviorAsListPanelItem(const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig)
{
	auto kv = new KeyValues("PhysicBehavior");

	kv->SetInt("index", UTIL_GetPhysicBehaviorIndex(m_pPhysicObjectConfig.get(), pPhysicBehaviorConfig));
	kv->SetInt("configId", pPhysicBehaviorConfig->configId);
	kv->SetString("name", pPhysicBehaviorConfig->name.c_str());
	kv->SetString("type", UTIL_GetPhysicBehaviorTypeLocalizationToken(pPhysicBehaviorConfig->type));
	kv->SetString("rigidbodyA", pPhysicBehaviorConfig->rigidbodyA.c_str());
	kv->SetString("rigidbodyB", pPhysicBehaviorConfig->rigidbodyB.c_str());
	kv->SetString("constraint", pPhysicBehaviorConfig->constraint.c_str());

	std::wstring flags = UTIL_GetFormattedPhysicBehaviorFlags(pPhysicBehaviorConfig->flags);

	kv->SetWString("flags", flags.c_str());

	m_pPhysicBehaviorListPanel->AddItem(kv, pPhysicBehaviorConfig->configId, false, true);

	kv->deleteThis();
}

void CPhysicBehaviorPage::ReloadAllPhysicBehaviorsIntoListPanelItem()
{
	m_pPhysicBehaviorListPanel->RemoveAll();

	for (const auto& pConfig : m_pPhysicObjectConfig->PhysicBehaviorConfigs)
	{
		LoadPhysicBehaviorAsListPanelItem(pConfig.get());
	}
}

void CPhysicBehaviorPage::OnOpenPhysicBehaviorEditor(int configId)
{
	// Retrieve the PhysicBehavior configuration using the configId.
	auto pPhysicBehaviorConfig = UTIL_GetPhysicBehaviorConfigFromConfigId(configId);

	if (!pPhysicBehaviorConfig)
		return;

	auto pEditorDialog = new CPhysicBehaviorEditDialog(this, "PhysicBehaviorEditDialog", m_physicObjectId, m_pPhysicObjectConfig, pPhysicBehaviorConfig);
	pEditorDialog->AddActionSignalTarget(this);
	pEditorDialog->DoModal();
}

void CPhysicBehaviorPage::OnCreatePhysicBehavior()
{
	auto pPhysicBehaviorConfig = UTIL_CreateEmptyPhysicBehaviorConfig();

	m_pPhysicObjectConfig->PhysicBehaviorConfigs.push_back(pPhysicBehaviorConfig);

	m_pPhysicObjectConfig->configModified = true;

	LoadPhysicBehaviorAsListPanelItem(pPhysicBehaviorConfig.get());
}

void CPhysicBehaviorPage::OnShiftUpPhysicBehavior(int configId)
{
	if (UTIL_ShiftUpPhysicBehaviorIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllPhysicBehaviorsIntoListPanelItem();
		SelectPhysicBehaviorItem(configId);
	}
}

void CPhysicBehaviorPage::OnShiftDownPhysicBehavior(int configId)
{
	if (UTIL_ShiftDownPhysicBehaviorIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllPhysicBehaviorsIntoListPanelItem();
		SelectPhysicBehaviorItem(configId);
	}
}

void CPhysicBehaviorPage::SelectPhysicBehaviorItem(int configId)
{
	// Iterate through all items in the PhysicBehavior list panel
	for (int i = 0; i < m_pPhysicBehaviorListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pPhysicBehaviorListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pPhysicBehaviorListPanel->SetSingleSelectedItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CPhysicBehaviorPage::DeletePhysicBehaviorItem(int configId)
{
	// Iterate through all items in the PhysicBehavior list panel
	for (int i = 0; i < m_pPhysicBehaviorListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pPhysicBehaviorListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pPhysicBehaviorListPanel->RemoveItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CPhysicBehaviorPage::OnDeletePhysicBehavior(int configId)
{
	//DeletePhysicBehaviorItem(configId);

	if (UTIL_RemovePhysicBehaviorFromPhysicObjectConfig(m_pPhysicObjectConfig.get(), configId))
	{
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());

		ReloadAllPhysicBehaviorsIntoListPanelItem();
	}
}

void CPhysicBehaviorPage::OnRefreshPhysicBehaviors()
{
	ReloadAllPhysicBehaviorsIntoListPanelItem();
}