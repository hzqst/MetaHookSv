#include "PhysicActionPage.h"
#include "PhysicActionListPanel.h"
#include "PhysicActionEditDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

CPhysicActionPage::CPhysicActionPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pPhysicActionListPanel = new CPhysicActionListPanel(this, "PhysicActionListPanel");

	m_pShiftUpPhysicAction = new vgui::Button(this, "ShiftUpPhysicAction", L"#BulletPhysics_ShiftUp", this, "ShiftUpPhysicAction");
	m_pShiftDownPhysicAction = new vgui::Button(this, "ShiftDownPhysicAction", L"#BulletPhysics_ShiftDown", this, "ShiftDownPhysicAction");
	m_pCreatePhysicAction = new vgui::Button(this, "CreatePhysicAction", L"#BulletPhysics_CreatePhysicAction", this, "CreatePhysicAction");

	LoadControlSettings("bulletphysics/PhysicActionPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicActionPage::OnResetData()
{
	ReloadAllPhysicActionsIntoListPanelItem();
}

void CPhysicActionPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pPhysicActionListPanel->SetFont(m_hFont);
}

void CPhysicActionPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pPhysicActionListPanel->HasFocus() && m_pPhysicActionListPanel->GetSelectedItemsCount() > 0)
		{
			auto selectItemId = m_pPhysicActionListPanel->GetSelectedItem(0);
			auto configId = m_pPhysicActionListPanel->GetItemUserData(selectItemId);
			OnOpenPhysicActionEditor(configId);
			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicActionPage::OnCommand(const char* command)
{
	if (!stricmp(command, "CreatePhysicAction"))
	{
		OnCreatePhysicAction();
	}
	else if (!stricmp(command, "ShiftUpPhysicAction"))
	{
		auto selectItemId = m_pPhysicActionListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicActionListPanel->GetItemUserData(selectItemId);
		OnShiftUpPhysicAction(configId);
	}
	else if (!stricmp(command, "ShiftDownPhysicAction"))
	{
		auto selectItemId = m_pPhysicActionListPanel->GetSelectedItem(0);
		auto configId = m_pPhysicActionListPanel->GetItemUserData(selectItemId);
		OnShiftDownPhysicAction(configId);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicActionPage::OnOpenContextMenu(int itemId)
{
	if (!m_pPhysicActionListPanel->GetSelectedItemsCount())
		return;

	auto selectItemId = m_pPhysicActionListPanel->GetSelectedItem(0);
	auto configId = m_pPhysicActionListPanel->GetItemUserData(selectItemId);

	auto pPhysicActionConfig = UTIL_GetPhysicActionConfigFromConfigId(configId);

	if (!pPhysicActionConfig)
		return;

	vgui::Menu* menu = new vgui::Menu(this, "contextmenu");
	menu->SetAutoDelete(true);

	wchar_t szName[64] = { 0 };
	wchar_t szBuf[256] = { 0 };
	vgui::localize()->ConvertANSIToUnicode(pPhysicActionConfig->name.c_str(), szName, sizeof(szName));

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditPhysicAction"), 1, szName);
	menu->AddMenuItem("EditPhysicAction", szBuf, new KeyValues("EditPhysicAction", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ClonePhysicAction"), 1, szName);
	menu->AddMenuItem("ClonePhysicAction", szBuf, new KeyValues("ClonePhysicAction", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftUpPhysicAction"), 1, szName);
	menu->AddMenuItem("ShiftUpPhysicAction", szBuf, new KeyValues("ShiftUpPhysicAction", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_ShiftDownPhysicAction"), 1, szName);
	menu->AddMenuItem("ShiftDownPhysicAction", szBuf, new KeyValues("ShiftDownPhysicAction", "configId", configId), this);

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeletePhysicAction"), 1, szName);
	menu->AddMenuItem("DeletePhysicAction", szBuf, new KeyValues("DeletePhysicAction", "configId", configId), this);

	vgui::Menu::PlaceContextMenu(this, menu);
}

void CPhysicActionPage::OnRefreshPhysicAction(int configId)
{
	auto pPhysicActionConfig = UTIL_GetPhysicActionConfigFromConfigId(configId);

	if (!pPhysicActionConfig)
		return;

	DeletePhysicActionItem(configId);

	// Re-add the updated item to the list panel
	LoadPhysicActionAsListPanelItem(pPhysicActionConfig.get());
}

void CPhysicActionPage::OnEditPhysicAction(int configId)
{
	OnOpenPhysicActionEditor(configId);
}

void CPhysicActionPage::OnClonePhysicAction(int configId)
{
	// Retrieve the original PhysicAction configuration using the configId.
	auto originalPhysicActionConfig = UTIL_GetPhysicActionConfigFromConfigId(configId);

	if (!originalPhysicActionConfig)
		return;

	// Create a deep copy of the PhysicAction configuration.
	// Assuming you have a utility function to clone PhysicAction configurations.
	auto clonedPhysicActionConfig = UTIL_ClonePhysicActionConfig(originalPhysicActionConfig.get());

	if (!clonedPhysicActionConfig)
		return;

	// Modify the name or any other parameters to indicate that this is a cloned version.
	clonedPhysicActionConfig->name += " (Clone)";

	clonedPhysicActionConfig->configModified = true;

	// Add the cloned configuration to the list of PhysicActions in the physic object config.
	m_pPhysicObjectConfig->ActionConfigs.push_back(clonedPhysicActionConfig);

	m_pPhysicObjectConfig->configModified = true;

	ClientPhysicManager()->AddPhysicConfig(clonedPhysicActionConfig->configId, clonedPhysicActionConfig);

	// Refresh the UI to include the new PhysicAction configuration.
	LoadPhysicActionAsListPanelItem(clonedPhysicActionConfig.get());

	// Optionally, select the newly added item in the list.
	SelectPhysicActionItem(clonedPhysicActionConfig->configId);

	//Update PhysicObject

	auto pPhysicObject = ClientPhysicManager()->GetPhysicObjectEx(m_physicObjectId);

	if (pPhysicObject->Rebuild(m_pPhysicObjectConfig.get()))
	{
		int clonedPhysicComponentId = 0;

		pPhysicObject->EnumPhysicComponents([clonedPhysicActionConfig, &clonedPhysicComponentId](IPhysicComponent* pPhysicCompoent) {

			if (pPhysicCompoent->GetPhysicConfigId() == clonedPhysicActionConfig->configId)
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

void CPhysicActionPage::LoadPhysicActionAsListPanelItem(const CClientPhysicActionConfig* pPhysicActionConfig)
{
	auto kv = new KeyValues("PhysicAction");

	kv->SetInt("index", UTIL_GetPhysicActionIndex(m_pPhysicObjectConfig.get(), pPhysicActionConfig));
	kv->SetInt("configId", pPhysicActionConfig->configId);
	kv->SetString("name", pPhysicActionConfig->name.c_str());
	kv->SetString("type", UTIL_GetPhysicActionTypeLocalizationToken(pPhysicActionConfig->type));
	kv->SetString("rigidbody", pPhysicActionConfig->rigidbody.c_str());

	std::wstring flags;

	flags += UTIL_GetFormattedPhysicActionFlags(pPhysicActionConfig->flags);

	kv->SetWString("flags", flags.c_str());

	m_pPhysicActionListPanel->AddItem(kv, pPhysicActionConfig->configId, false, true);
	kv->deleteThis();
}

void CPhysicActionPage::ReloadAllPhysicActionsIntoListPanelItem()
{
	m_pPhysicActionListPanel->RemoveAll();

	for (const auto& pConfig : m_pPhysicObjectConfig->ActionConfigs)
	{
		LoadPhysicActionAsListPanelItem(pConfig.get());
	}
}

void CPhysicActionPage::OnOpenPhysicActionEditor(int configId)
{
	// Retrieve the PhysicAction configuration using the configId.
	auto pPhysicActionConfig = UTIL_GetPhysicActionConfigFromConfigId(configId);

	if (!pPhysicActionConfig)
		return;

	auto pEditorDialog = new CPhysicActionEditDialog(this, "PhysicActionEditDialog", m_physicObjectId, m_pPhysicObjectConfig, pPhysicActionConfig);
	pEditorDialog->AddActionSignalTarget(this);
	pEditorDialog->DoModal();
}

void CPhysicActionPage::OnCreatePhysicAction()
{
	auto pPhysicActionConfig = std::make_shared<CClientPhysicActionConfig>();
	pPhysicActionConfig->name = std::format("UnnamedAction_{0}", pPhysicActionConfig->configId);
	pPhysicActionConfig->configModified = true;

	m_pPhysicObjectConfig->ActionConfigs.push_back(pPhysicActionConfig);
	m_pPhysicObjectConfig->configModified = true;

	LoadPhysicActionAsListPanelItem(pPhysicActionConfig.get());
}

void CPhysicActionPage::OnShiftUpPhysicAction(int configId)
{
	if (UTIL_ShiftUpPhysicActionIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllPhysicActionsIntoListPanelItem();
		SelectPhysicActionItem(configId);
	}
}

void CPhysicActionPage::OnShiftDownPhysicAction(int configId)
{
	if (UTIL_ShiftDownPhysicActionIndex(m_pPhysicObjectConfig.get(), configId))
	{
		ReloadAllPhysicActionsIntoListPanelItem();
		SelectPhysicActionItem(configId);
	}
}

void CPhysicActionPage::SelectPhysicActionItem(int configId)
{
	// Iterate through all items in the PhysicAction list panel
	for (int i = 0; i < m_pPhysicActionListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pPhysicActionListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pPhysicActionListPanel->SetSingleSelectedItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CPhysicActionPage::DeletePhysicActionItem(int configId)
{
	// Iterate through all items in the PhysicAction list panel
	for (int i = 0; i < m_pPhysicActionListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pPhysicActionListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pPhysicActionListPanel->RemoveItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CPhysicActionPage::OnDeletePhysicAction(int configId)
{
	DeletePhysicActionItem(configId);

	if (UTIL_RemovePhysicActionFromPhysicObjectConfig(m_pPhysicObjectConfig.get(), configId))
	{
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
	}
}

void CPhysicActionPage::OnRefreshPhysicActions()
{
	ReloadAllPhysicActionsIntoListPanelItem();
}