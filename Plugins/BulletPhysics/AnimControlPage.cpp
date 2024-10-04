#include "AnimControlPage.h"
#include "AnimControlListPanel.h"
#include "AnimControlEditDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

CAnimControlPage::CAnimControlPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientRagdollObjectConfig>& pRagdollObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pRagdollObjectConfig(pRagdollObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pAnimControlListPanel = new CAnimControlListPanel(this, "AnimControlListPanel");

	m_pShiftUpAnimControl = new vgui::Button(this, "ShiftUpAnimControl", L"#BulletPhysics_ShiftUp", this, "ShiftUpAnimControl");
	m_pShiftDownAnimControl = new vgui::Button(this, "ShiftDownAnimControl", L"#BulletPhysics_ShiftDown", this, "ShiftDownAnimControl");
	m_pCreateAnimControl = new vgui::Button(this, "CreateAnimControl", L"#BulletPhysics_CreateAnimControl", this, "CreateAnimControl");

	LoadControlSettings("bulletphysics/AnimControlPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CAnimControlPage::OnResetData()
{
	ReloadAllAnimControlsIntoListPanelItem();
}

void CAnimControlPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pAnimControlListPanel->SetFont(m_hFont);
}

void CAnimControlPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pAnimControlListPanel->HasFocus() && m_pAnimControlListPanel->GetSelectedItemsCount() > 0)
		{
			auto selectItemId = m_pAnimControlListPanel->GetSelectedItem(0);
			auto configId = m_pAnimControlListPanel->GetItemUserData(selectItemId);
			OnOpenAnimControlEditor(configId);
			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CAnimControlPage::OnCommand(const char* command)
{
	if (!stricmp(command, "CreateAnimControl"))
	{
		OnCreateAnimControl();
	}
	else if (!stricmp(command, "DeleteSelectedAnimControl"))
	{
		auto selectItemId = m_pAnimControlListPanel->GetSelectedItem(0);
		auto configId = m_pAnimControlListPanel->GetItemUserData(selectItemId);
		OnDeleteAnimControl(configId);
	}
	else if (!stricmp(command, "ShiftUpAnimControl"))
	{
		auto selectItemId = m_pAnimControlListPanel->GetSelectedItem(0);
		auto configId = m_pAnimControlListPanel->GetItemUserData(selectItemId);
		OnShiftUpAnimControl(configId);
	}
	else if (!stricmp(command, "ShiftDownAnimControl"))
	{
		auto selectItemId = m_pAnimControlListPanel->GetSelectedItem(0);
		auto configId = m_pAnimControlListPanel->GetItemUserData(selectItemId);
		OnShiftDownAnimControl(configId);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CAnimControlPage::OnOpenContextMenu(int itemId)
{
	if (!m_pAnimControlListPanel->GetSelectedItemsCount())
		return;

	auto selectItemId = m_pAnimControlListPanel->GetSelectedItem(0);
	auto configId = m_pAnimControlListPanel->GetItemUserData(selectItemId);

	auto pAnimControlConfig = UTIL_GetAnimControlConfigFromConfigId(configId);

	if (!pAnimControlConfig)
		return;

	vgui::Menu* menu = new vgui::Menu(this, "contextmenu");
	menu->SetAutoDelete(true);

	menu->AddMenuItem("EditAnimControl", vgui::localize()->Find("#BulletPhysics_EditAnimControl"), new KeyValues("EditAnimControl", "configId", configId), this);

	menu->AddMenuItem("CloneAnimControl", vgui::localize()->Find("#BulletPhysics_CloneAnimControl"), new KeyValues("CloneAnimControl", "configId", configId), this);

	menu->AddMenuItem("ShiftUpAnimControl", vgui::localize()->Find("#BulletPhysics_ShiftUpAnimControl"), new KeyValues("ShiftUpAnimControl", "configId", configId), this);

	menu->AddMenuItem("ShiftDownAnimControl", vgui::localize()->Find("#BulletPhysics_ShiftDownAnimControl"), new KeyValues("ShiftDownAnimControl", "configId", configId), this);

	menu->AddMenuItem("DeleteAnimControl", vgui::localize()->Find("#BulletPhysics_DeleteAnimControl"), new KeyValues("DeleteAnimControl", "configId", configId), this);

	vgui::Menu::PlaceContextMenu(this, menu);
}

void CAnimControlPage::OnRefreshAnimControl(int configId)
{
	auto pAnimControlConfig = UTIL_GetAnimControlConfigFromConfigId(configId);

	if (!pAnimControlConfig)
		return;

	DeleteAnimControlItem(configId);

	// Re-add the updated item to the list panel
	LoadAnimControlAsListPanelItem(pAnimControlConfig.get());
}

void CAnimControlPage::OnEditAnimControl(int configId)
{
	OnOpenAnimControlEditor(configId);
}

void CAnimControlPage::OnCloneAnimControl(int configId)
{
	// Retrieve the original AnimControl configuration using the configId.
	auto originalAnimControlConfig = UTIL_GetAnimControlConfigFromConfigId(configId);

	if (!originalAnimControlConfig)
		return;

	// Create a deep copy of the AnimControl configuration.
	// Assuming you have a utility function to clone AnimControl configurations.
	auto clonedAnimControlConfig = UTIL_CloneAnimControlConfig(originalAnimControlConfig.get());

	if (!clonedAnimControlConfig)
		return;

	clonedAnimControlConfig->configModified = true;

	// Add the cloned configuration to the list of AnimControls in the physic object config.
	m_pRagdollObjectConfig->AnimControlConfigs.push_back(clonedAnimControlConfig);

	m_pRagdollObjectConfig->configModified = true;

	// Refresh the UI to include the new AnimControl configuration.
	LoadAnimControlAsListPanelItem(clonedAnimControlConfig.get());

	// Optionally, select the newly added item in the list.
	SelectAnimControlItem(clonedAnimControlConfig->configId);

	//Update PhysicObject
	UTIL_RebuildPhysicObjectWithClonedConfig(m_physicObjectId, m_pRagdollObjectConfig.get(), clonedAnimControlConfig->configId);
}

void CAnimControlPage::LoadAnimControlAsListPanelItem(const CClientAnimControlConfig* pAnimControlConfig)
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(m_physicObjectId);

	auto kv = new KeyValues("AnimControl");

	kv->SetInt("index", UTIL_GetAnimControlIndex(m_pRagdollObjectConfig.get(), pAnimControlConfig));
	kv->SetInt("configId", pAnimControlConfig->configId);

	auto animframeString = std::format("{0}", pAnimControlConfig->animframe);
	kv->SetString("animframe", animframeString.c_str());

	auto activityString = UTIL_GetActivityTypeLocalizationToken(pAnimControlConfig->activity);
	kv->SetString("activity", activityString);

	auto sequenceString = UTIL_GetFormattedSequenceName(modelindex, pAnimControlConfig->sequence);
	kv->SetString("sequence", sequenceString.c_str());

	auto gaitsequenceString = UTIL_GetFormattedSequenceName(modelindex, pAnimControlConfig->gaitsequence);
	kv->SetString("gaitsequence", gaitsequenceString.c_str());

	auto controllerString = std::format("{0} {1} {2} {3}", pAnimControlConfig->controller[0], pAnimControlConfig->controller[1], pAnimControlConfig->controller[2], pAnimControlConfig->controller[3]);
	kv->SetString("controller", controllerString.c_str());

	auto blendingString = std::format("{0} {1} {2} {3}", pAnimControlConfig->blending[0], pAnimControlConfig->blending[1], pAnimControlConfig->blending[2], pAnimControlConfig->blending[3]);
	kv->SetString("blending", blendingString.c_str());

	m_pAnimControlListPanel->AddItem(kv, pAnimControlConfig->configId, false, true);

	kv->deleteThis();
}

void CAnimControlPage::ReloadAllAnimControlsIntoListPanelItem()
{
	m_pAnimControlListPanel->RemoveAll();

	for (const auto& pConfig : m_pRagdollObjectConfig->AnimControlConfigs)
	{
		LoadAnimControlAsListPanelItem(pConfig.get());
	}
}

void CAnimControlPage::OnOpenAnimControlEditor(int configId)
{
	// Retrieve the AnimControl configuration using the configId.
	auto pAnimControlConfig = UTIL_GetAnimControlConfigFromConfigId(configId);

	if (!pAnimControlConfig)
		return;

	auto pEditorDialog = new CAnimControlEditDialog(this, "AnimControlEditDialog", m_physicObjectId, m_pRagdollObjectConfig, pAnimControlConfig);
	pEditorDialog->AddActionSignalTarget(this);
	pEditorDialog->DoModal();
}

void CAnimControlPage::OnCreateAnimControl()
{
	auto pAnimControlConfig = UTIL_CreateEmptyAnimControlConfig();

	m_pRagdollObjectConfig->AnimControlConfigs.push_back(pAnimControlConfig);

	m_pRagdollObjectConfig->configModified = true;

	LoadAnimControlAsListPanelItem(pAnimControlConfig.get());
}

void CAnimControlPage::OnShiftUpAnimControl(int configId)
{
	if (UTIL_ShiftUpAnimControlIndex(m_pRagdollObjectConfig.get(), configId))
	{
		ReloadAllAnimControlsIntoListPanelItem();
		SelectAnimControlItem(configId);
	}
}

void CAnimControlPage::OnShiftDownAnimControl(int configId)
{
	if (UTIL_ShiftDownAnimControlIndex(m_pRagdollObjectConfig.get(), configId))
	{
		ReloadAllAnimControlsIntoListPanelItem();
		SelectAnimControlItem(configId);
	}
}

void CAnimControlPage::SelectAnimControlItem(int configId)
{
	// Iterate through all items in the AnimControl list panel
	for (int i = 0; i < m_pAnimControlListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pAnimControlListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pAnimControlListPanel->SetSingleSelectedItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CAnimControlPage::DeleteAnimControlItem(int configId)
{
	// Iterate through all items in the AnimControl list panel
	for (int i = 0; i < m_pAnimControlListPanel->GetItemCount(); ++i)
	{
		// Retrieve the configId stored as user data for the current item
		int itemConfigId = static_cast<int>(m_pAnimControlListPanel->GetItemUserData(i));

		// Check if the current item's configId matches the provided configId
		if (itemConfigId == configId)
		{
			// Select the item in the list panel
			m_pAnimControlListPanel->RemoveItem(i);
			break; // Exit the loop after selecting the item
		}
	}
}

void CAnimControlPage::OnDeleteAnimControl(int configId)
{
	//DeleteAnimControlItem(configId);

	if (UTIL_RemoveAnimControlFromRagdollObjectConfig(m_pRagdollObjectConfig.get(), configId))
	{
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pRagdollObjectConfig.get());

		ReloadAllAnimControlsIntoListPanelItem();
	}
}

void CAnimControlPage::OnRefreshAnimControls()
{
	ReloadAllAnimControlsIntoListPanelItem();
}