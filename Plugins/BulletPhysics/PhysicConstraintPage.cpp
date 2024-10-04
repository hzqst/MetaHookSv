#include "PhysicConstraintPage.h"
#include "PhysicConstraintListPanel.h"
#include "PhysicConstraintEditDialog.h"

#include "exportfuncs.h"
#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

// Constraint Page

CPhysicConstraintPage::CPhysicConstraintPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pPhysicConstraintListPanel = new CPhysicConstraintListPanel(this, "PhysicConstraintListPanel");

	m_pShiftUpConstraint = new vgui::Button(this, "ShiftUpConstraint", L"#BulletPhysics_ShiftUp", this, "ShiftUpConstraint");
	m_pShiftDownConstraint = new vgui::Button(this, "ShiftDownConstraint", L"#BulletPhysics_ShiftDown", this, "ShiftDownConstraint");
	m_pCreateConstraint = new vgui::Button(this, "CreateConstraint", L"#BulletPhysics_CreateConstraint", this, "CreateConstraint");

	LoadControlSettings("bulletphysics/PhysicConstraintPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicConstraintPage::OnResetData()
{
	ReloadAllConstraintsIntoListPanelItem();
}

void CPhysicConstraintPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pPhysicConstraintListPanel->SetFont(m_hFont);
}

void CPhysicConstraintPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pPhysicConstraintListPanel->HasFocus() && m_pPhysicConstraintListPanel->GetSelectedItemsCount() > 0)
		{
			auto selectItemId = m_pPhysicConstraintListPanel->GetSelectedItem(0);
			auto configId = m_pPhysicConstraintListPanel->GetItemUserData(selectItemId);
			OnOpenConstraintEditor(configId);
			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicConstraintPage::OnCommand(const char* command)
{
	if (!stricmp(command, "CreateConstraint"))
	{
		OnCreateConstraint();
	}
	else if (!stricmp(command, "DeleteSelectedConstraint"))
	{
		auto selectItemId = m_pPhysicConstraintListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicConstraintListPanel->GetItemUserData(selectItemId);
		OnDeleteConstraint(configId);
	}
	else if (!stricmp(command, "ShiftUpConstraint"))
	{
		auto selectItemId = m_pPhysicConstraintListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicConstraintListPanel->GetItemUserData(selectItemId);
		OnShiftUpConstraint(configId);
	}
	else if (!stricmp(command, "ShiftDownConstraint"))
	{
		auto selectItemId = m_pPhysicConstraintListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicConstraintListPanel->GetItemUserData(selectItemId);
		OnShiftDownConstraint(configId);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicConstraintPage::OnOpenContextMenu(int itemId)
{
	if (!m_pPhysicConstraintListPanel->GetSelectedItemsCount())
		return;

	auto selectItemId = m_pPhysicConstraintListPanel->GetSelectedItem(0);
	auto configId = m_pPhysicConstraintListPanel->GetItemUserData(selectItemId);

	auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(configId);

	if (!pConstraintConfig)
		return;

	vgui::Menu* menu = new vgui::Menu(this, "contextmenu");
	menu->SetAutoDelete(true);

	wchar_t szName[64] = { 0 };
	wchar_t szBuf[256] = { 0 };
	vgui::localize()->ConvertANSIToUnicode(pConstraintConfig->name.c_str(), szName, sizeof(szName));

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditConstraint"), 1, szName);
	menu->AddMenuItem("EditConstraint", szBuf, new KeyValues("EditConstraint", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_CloneConstraint"), 1, szName);
	menu->AddMenuItem("CloneConstraint", szBuf, new KeyValues("CloneConstraint", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftUpConstraint"), 1, szName);
	menu->AddMenuItem("ShiftUpConstraint", szBuf, new KeyValues("ShiftUpConstraint", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftDownConstraint"), 1, szName);
	menu->AddMenuItem("ShiftDownConstraint", szBuf, new KeyValues("ShiftDownConstraint", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteConstraint"), 1, szName);
	menu->AddMenuItem("DeleteConstraint", szBuf, new KeyValues("DeleteConstraint", "configId", configId), this);

	vgui::Menu::PlaceContextMenu(this, menu);
}

void CPhysicConstraintPage::OnRefreshConstraint(int configId)
{
	auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(configId);

	if (!pConstraintConfig)
		return;

	DeleteConstraintItem(configId);

	// Re-add the updated item to the list panel
	LoadConstraintAsListPanelItem(pConstraintConfig.get());
}

void CPhysicConstraintPage::OnEditConstraint(int configId)
{
	OnOpenConstraintEditor(configId);
}

void CPhysicConstraintPage::OnCloneConstraint(int configId)
{
	// Retrieve the original constraint configuration using the configId.
	auto originalConstraintConfig = UTIL_GetConstraintConfigFromConfigId(configId);

	if (!originalConstraintConfig)
		return;

	// Create a deep copy of the constraint configuration.
	// Assuming you have a utility function to clone constraint configurations.
	auto clonedConstraintConfig = UTIL_CloneConstraintConfig(originalConstraintConfig.get());

	if (!clonedConstraintConfig)
		return;

	// Modify the name or any other parameters to indicate that this is a cloned version.
	clonedConstraintConfig->name += " (Clone)";

	clonedConstraintConfig->configModified = true;

	// Add the cloned configuration to the list of constraints in the physic object config.
	m_pPhysicObjectConfig->ConstraintConfigs.push_back(clonedConstraintConfig);

	m_pPhysicObjectConfig->configModified = true;

	// Refresh the UI to include the new constraint configuration.
	LoadConstraintAsListPanelItem(clonedConstraintConfig.get());

	// Optionally, select the newly added item in the list.
	SelectConstraintItem(clonedConstraintConfig->configId);

	//Update PhysicObject

	UTIL_RebuildPhysicObjectWithClonedConfig(m_physicObjectId, m_pPhysicObjectConfig.get(), clonedConstraintConfig->configId);
}

void CPhysicConstraintPage::LoadConstraintAsListPanelItem(const CClientConstraintConfig* pConstraintConfig)
{
	auto kv = new KeyValues("Constraint");

	kv->SetInt("index", UTIL_GetConstraintIndex(m_pPhysicObjectConfig.get(), pConstraintConfig));
	kv->SetInt("configId", pConstraintConfig->configId);
	kv->SetString("name", pConstraintConfig->name.c_str());
	kv->SetString("type", UTIL_GetConstraintTypeLocalizationToken(pConstraintConfig->type));
	kv->SetString("rigidbodyA", pConstraintConfig->rigidbodyA.c_str());
	kv->SetString("rigidbodyB", pConstraintConfig->rigidbodyB.c_str());

	std::wstring flags;

	flags += UTIL_GetFormattedConstraintConfigAttributes(pConstraintConfig);
	flags += UTIL_GetFormattedConstraintFlags(pConstraintConfig->flags);

	kv->SetWString("flags", flags.c_str());

	m_pPhysicConstraintListPanel->AddItem(kv, pConstraintConfig->configId, false, true);
	kv->deleteThis();
}

void CPhysicConstraintPage::ReloadAllConstraintsIntoListPanelItem()
{
	m_pPhysicConstraintListPanel->RemoveAll();

	for (const auto& pConfig : m_pPhysicObjectConfig->ConstraintConfigs)
	{
		LoadConstraintAsListPanelItem(pConfig.get());
	}
}

void CPhysicConstraintPage::OnOpenConstraintEditor(int configId)
{
	// Retrieve the constraint configuration using the configId.
	auto pConstraintConfig = UTIL_GetConstraintConfigFromConfigId(configId);

	if (!pConstraintConfig)
		return;

	auto pEditorDialog = new CPhysicConstraintEditDialog(this, "PhysicConstraintEditDialog", m_physicObjectId, m_pPhysicObjectConfig, pConstraintConfig);
	pEditorDialog->AddActionSignalTarget(this);
	pEditorDialog->DoModal();
}

void CPhysicConstraintPage::OnCreateConstraint()
{
	auto pConstraintConfig = UTIL_CreateEmptyConstraintConfig();

	m_pPhysicObjectConfig->ConstraintConfigs.push_back(pConstraintConfig);

	m_pPhysicObjectConfig->configModified = true;

	LoadConstraintAsListPanelItem(pConstraintConfig.get());
}

void CPhysicConstraintPage::OnShiftUpConstraint(int configId)
{
	if (UTIL_ShiftUpConstraintIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllConstraintsIntoListPanelItem();
		SelectConstraintItem(configId);
	}
}

void CPhysicConstraintPage::OnShiftDownConstraint(int configId)
{
	if (UTIL_ShiftDownConstraintIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllConstraintsIntoListPanelItem();
		SelectConstraintItem(configId);
	}
}

void CPhysicConstraintPage::SelectConstraintItem(int configId)
{
	// Iterate through all items in the constraint list panel
	for (int i = 0; i < m_pPhysicConstraintListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pPhysicConstraintListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pPhysicConstraintListPanel->SetSingleSelectedItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CPhysicConstraintPage::DeleteConstraintItem(int configId)
{
	// Iterate through all items in the constraint list panel
	for (int i = 0; i < m_pPhysicConstraintListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pPhysicConstraintListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pPhysicConstraintListPanel->RemoveItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CPhysicConstraintPage::OnDeleteConstraint(int configId)
{
	//DeleteConstraintItem(configId);

	if (UTIL_RemoveConstraintFromPhysicObjectConfig(m_pPhysicObjectConfig.get(), configId))
	{
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
	
		ReloadAllConstraintsIntoListPanelItem();
	}
}

void CPhysicConstraintPage::OnRefreshConstraints()
{
	ReloadAllConstraintsIntoListPanelItem();
}