#include "PhysicActionEditDialog.h"
#include "PhysicFactorListPanel.h"

#include "PhysicUTIL.h"

#include <format>

CPhysicActionEditDialog::CPhysicActionEditDialog(vgui::Panel* parent, const char* name,
	uint64 physicObjectId,
	const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
	const std::shared_ptr<CClientPhysicActionConfig>& pPhysicActionConfig) :
	BaseClass(parent, name),
	m_physicObjectId(physicObjectId),
	m_pPhysicObjectConfig(pPhysicObjectConfig),
	m_pPhysicActionConfig(pPhysicActionConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_ActionEditor", false);

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(520), vgui::scheme()->GetProportionalScaledValue(560));
	SetSize(vgui::scheme()->GetProportionalScaledValue(520), vgui::scheme()->GetProportionalScaledValue(560));

	// Initialize UI components
	m_pName = new vgui::TextEntry(this, "Name");
	m_pType = new vgui::ComboBox(this, "Type", 0, false);
	m_pDebugDrawLevel = new vgui::TextEntry(this, "DebugDrawLevel");

	m_pRigidBodyLabel = new vgui::Label(this, "RigidBodyLabel", "#BulletPhysics_RigidBody");
	m_pRigidBody = new vgui::ComboBox(this, "RigidBody", 0, true);

	m_pConstraintLabel = new vgui::Label(this, "ConstraintLabel", "#BulletPhysics_Constraint");
	m_pConstraint = new vgui::ComboBox(this, "Constraint", 0, true);

	m_pOriginX = new vgui::TextEntry(this, "OriginX");
	m_pOriginY = new vgui::TextEntry(this, "OriginY");
	m_pOriginZ = new vgui::TextEntry(this, "OriginZ");
	m_pAnglesX = new vgui::TextEntry(this, "AnglesX");
	m_pAnglesY = new vgui::TextEntry(this, "AnglesY");
	m_pAnglesZ = new vgui::TextEntry(this, "AnglesZ");

	m_pBarnacle = new vgui::CheckButton(this, "Barnacle", "#BulletPhysics_Barnacle");
	m_pGargantua = new vgui::CheckButton(this, "Gargantua", "#BulletPhysics_Gargantua");

	m_pPhysicFactorListPanel = new CPhysicFactorListPanel(this, "PhysicFactorListPanel");

	m_pPhysicFactorListPanel->AddColumnHeader(0, "index", "#BulletPhysics_Index", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	m_pPhysicFactorListPanel->AddColumnHeader(1, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	m_pPhysicFactorListPanel->AddColumnHeader(2, "value", "#BulletPhysics_Value", vgui::scheme()->GetProportionalScaledValue(100), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pPhysicFactorListPanel->SetSortColumn(0);
	m_pPhysicFactorListPanel->SetMultiselectEnabled(false);

	vgui::HFont hFallbackFont = vgui::scheme()->GetIScheme(GetScheme())->GetFont("DefaultVerySmallFallBack", false);

	if (vgui::INVALID_FONT != hFallbackFont)
	{
		m_pType->SetUseFallbackFont(true, hFallbackFont);
		m_pRigidBody->SetUseFallbackFont(true, hFallbackFont);
		m_pConstraint->SetUseFallbackFont(true, hFallbackFont);
	}

	LoadAvailableTypesIntoControl(m_pType);

	// Load control settings
	LoadControlSettings("bulletphysics/PhysicActionEditDialog.res", "GAME");

	// Add tick signal
	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CPhysicActionEditDialog::~CPhysicActionEditDialog()
{

}

void CPhysicActionEditDialog::Activate(void)
{
	BaseClass::Activate();

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicActionEditDialog::OnResetData()
{
	LoadConfigIntoControls();
	UpdateControlStates();
}

void CPhysicActionEditDialog::OnModifyFactor(KeyValues* kv)
{
	auto index = kv->GetInt("index");
	auto newValue = kv->GetString("newValue");

	for (int i = 0; i < m_pPhysicFactorListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pPhysicFactorListPanel->GetItemUserData(i);

		if (userData == index)
		{
			auto oldkv = m_pPhysicFactorListPanel->GetItemData(i);

			if (oldkv)
			{
				std::string name = oldkv->kv->GetString("name");

				float defaultValue = oldkv->kv->GetFloat("defaultValue");

				m_pPhysicFactorListPanel->RemoveItem(i);

				LoadFactorAsListPanelItemEx(index, name.c_str(), newValue, defaultValue);
				break;
			}
		}
	}
}

void CPhysicActionEditDialog::OnTextChanged(vgui::Panel* panel)
{
	if (panel == m_pType) {
		UpdateControlStates();
	}
}

void CPhysicActionEditDialog::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pPhysicFactorListPanel->IsCapturing())
		{
			m_pPhysicFactorListPanel->EndCaptureMode();
			return;
		}

		if (m_pPhysicFactorListPanel->HasFocus())
		{
			if (!m_pPhysicFactorListPanel->IsCapturing())
			{
				m_pPhysicFactorListPanel->StartCaptureMode();
			}
			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicActionEditDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		if (m_pPhysicFactorListPanel->IsCapturing())
		{
			m_pPhysicFactorListPanel->EndCaptureMode();
			PostMessage1(this, new KeyValues("Command", "command", command));
			return;
		}

		SaveConfigFromControls();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		PostActionSignal(new KeyValues("RefreshPhysicAction", "configId", m_pPhysicActionConfig->configId));
		PostMessage1(this, new KeyValues("ResetData"));
		Close();
	}
	else if (!stricmp(command, "Apply"))
	{
		if (m_pPhysicFactorListPanel->IsCapturing())
		{
			m_pPhysicFactorListPanel->EndCaptureMode();
			PostMessage1(this, new KeyValues("Command", "command", command));
			return;
		}

		SaveConfigFromControls();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		PostActionSignal(new KeyValues("RefreshPhysicAction", "configId", m_pPhysicActionConfig->configId));
		PostMessage1(this, new KeyValues("ResetData"));
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicActionEditDialog::LoadConfigIntoControls()
{
	LoadAvailableRigidBodiesIntoControl(m_pRigidBody);

	LoadAvailableConstraintsIntoControl(m_pConstraint);

	LoadTypeIntoControl(m_pType);

	LoadRigidBodyIntoControl(m_pRigidBody, m_pPhysicActionConfig->rigidbody);

	LoadConstraintIntoControl(m_pConstraint, m_pPhysicActionConfig->constraint);

#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pPhysicActionConfig->from); m_p##to->SetText(str##to.c_str());}

	LOAD_INTO_TEXT_ENTRY(name, Name);
	LOAD_INTO_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel);

	LOAD_INTO_TEXT_ENTRY(origin[0], OriginX);
	LOAD_INTO_TEXT_ENTRY(origin[1], OriginY);
	LOAD_INTO_TEXT_ENTRY(origin[2], OriginZ);

	LOAD_INTO_TEXT_ENTRY(angles[0], AnglesX);
	LOAD_INTO_TEXT_ENTRY(angles[1], AnglesY);
	LOAD_INTO_TEXT_ENTRY(angles[2], AnglesZ);

#undef LOAD_INTO_TEXT_ENTRY

#define LOAD_INTO_CHECK_BUTTON(from, to) m_p##to->SetSelected((m_pPhysicActionConfig->from & PhysicActionFlag_##to) ? true : false);

	LOAD_INTO_CHECK_BUTTON(flags, Barnacle);
	LOAD_INTO_CHECK_BUTTON(flags, Gargantua);

#undef LOAD_INTO_CHECK_BUTTON
}

void CPhysicActionEditDialog::LoadAvailableTypesIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < PhysicAction_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("type", i);

		pComboBox->AddItem(UTIL_GetPhysicActionTypeLocalizationToken(i), kv);

		kv->deleteThis();
	}
}

void CPhysicActionEditDialog::LoadAvailableRigidBodiesIntoControl(vgui::ComboBox* pComboBox)
{
	pComboBox->RemoveAll();

	if (1)
	{
		auto kv = new KeyValues("UserData");

		kv->SetString("name", "");

		pComboBox->AddItem("#BulletPhysics_None", kv);

		kv->deleteThis();
	}

	for (const auto& pRigidBody : m_pPhysicObjectConfig->RigidBodyConfigs)
	{
		auto kv = new KeyValues("UserData");

		kv->SetString("name", pRigidBody->name.c_str());

		pComboBox->AddItem(pRigidBody->name.c_str(), kv);

		kv->deleteThis();
	}
}

void CPhysicActionEditDialog::LoadAvailableConstraintsIntoControl(vgui::ComboBox* pComboBox)
{
	pComboBox->RemoveAll();

	if (1)
	{
		auto kv = new KeyValues("UserData");

		kv->SetString("name", "");
		kv->SetInt("type", PhysicConstraint_None);

		pComboBox->AddItem("#BulletPhysics_None", kv);

		kv->deleteThis();
	}

	for (const auto& pConstraint : m_pPhysicObjectConfig->ConstraintConfigs)
	{
		auto kv = new KeyValues("UserData");

		kv->SetString("name", pConstraint->name.c_str());
		kv->SetInt("type", pConstraint->type);

		pComboBox->AddItem(pConstraint->name.c_str(), kv);

		kv->deleteThis();
	}
}

void CPhysicActionEditDialog::LoadTypeIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		KeyValues* kv = pComboBox->GetItemUserData(i);

		if (kv && m_pPhysicActionConfig->type == kv->GetInt("type", PhysicAction_None))
		{
			pComboBox->ActivateItemByRow(i);
			return;
		}
	}

	pComboBox->ActivateItemByRow(0);
}

void CPhysicActionEditDialog::LoadRigidBodyIntoControl(vgui::ComboBox* pComboBox, const std::string& rigidBodyName)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		auto kv = pComboBox->GetItemUserData(i);

		if (kv && rigidBodyName == kv->GetString("name"))
		{
			pComboBox->ActivateItem(i);
			return;
		}
	}

	pComboBox->ActivateItem(0);
}

void CPhysicActionEditDialog::LoadConstraintIntoControl(vgui::ComboBox* pComboBox, const std::string& constraintName)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		auto kv = pComboBox->GetItemUserData(i);

		if (kv && constraintName == kv->GetString("name"))
		{
			pComboBox->ActivateItem(i);
			return;
		}
	}

	pComboBox->ActivateItem(0);
}

void CPhysicActionEditDialog::DeleteFactorListPanelItem(int factorIdx)
{
	for (int i = 0; i < m_pPhysicFactorListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pPhysicFactorListPanel->GetItemUserData(i);

		if (userData == factorIdx)
		{
			m_pPhysicFactorListPanel->RemoveItem(i);
			break;
		}
	}
}

void CPhysicActionEditDialog::LoadFactorAsListPanelItemEx(int factorIdx, const char* name, const char* value, float defaultValue)
{
	auto kv = new KeyValues("Factor");

	kv->SetInt("index", factorIdx);

	kv->SetString("name", name);

	kv->SetString("value", value);

	kv->SetFloat("defaultValue", defaultValue);

	m_pPhysicFactorListPanel->AddItem(kv, factorIdx, false, true);

	kv->deleteThis();
}

void CPhysicActionEditDialog::LoadFactorAsListPanelItem(int factorIdx, const char* name, float value, float defaultValue)
{
	auto val = std::format("{0:.4f}", value);

	LoadFactorAsListPanelItemEx(factorIdx, name, val.c_str(), defaultValue);
}

void CPhysicActionEditDialog::LoadAvailableFactorsIntoControls(int type)
{
	m_pPhysicFactorListPanel->RemoveAll();

#define LOAD_FACTOR_INTO_LISTPANEL(name) LoadFactorAsListPanelItem(PhysicActionFactorIdx_##name, "#BulletPhysics_" #name, m_pPhysicActionConfig->factors[PhysicActionFactorIdx_##name], NAN);
#define LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(name) LoadFactorAsListPanelItem(PhysicActionFactorIdx_##name, "#BulletPhysics_" #name, m_pConstraintConfig->factors[PhysicActionFactorIdx_##name], PhysicActionFactorDefaultValue_##name);

	switch (type)
	{
	case PhysicAction_BarnacleDragForce: {
		LOAD_FACTOR_INTO_LISTPANEL(BarnacleDragForceMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL(BarnacleDragForceExtraHeight);
		break;
	}
	case PhysicAction_BarnacleChewForce: {
		LOAD_FACTOR_INTO_LISTPANEL(BarnacleChewForceMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL(BarnacleChewForceInterval);
		break;
	}
	case PhysicAction_BarnacleConstraintLimitAdjustment: {
		LOAD_FACTOR_INTO_LISTPANEL(BarnacleConstraintLimitAdjustmentExtraHeight);
		LOAD_FACTOR_INTO_LISTPANEL(BarnacleConstraintLimitAdjustmentInterval);
		LOAD_FACTOR_INTO_LISTPANEL(BarnacleConstraintLimitAdjustmentAxis);
		break;
	}
	case PhysicAction_FirstPersonViewCamera:
	case PhysicAction_ThirdPersonViewCamera: {
		//LOAD_FACTOR_INTO_LISTPANEL(CameraActivateOnIdle);
		//LOAD_FACTOR_INTO_LISTPANEL(CameraActivateOnDeath);
		//LOAD_FACTOR_INTO_LISTPANEL(CameraActivateOnCaughtByBarnacle);
		break;
	}
	case PhysicAction_SimpleBuoyancy: {
		LOAD_FACTOR_INTO_LISTPANEL(SimpleBuoyancyMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL(SimpleBuoyancyLinearDrag);
		LOAD_FACTOR_INTO_LISTPANEL(SimpleBuoyancyAngularDrag);
		break;
	}
	default: {
		break;
	}
	}
#undef LOAD_FACTOR_INTO_LISTPANEL
#undef LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE
}

int CPhysicActionEditDialog::GetCurrentSelectedActionTypeIndex()
{
	int type = PhysicAction_None;

	auto kv = m_pType->GetActiveItemUserData();

	if (kv)
	{
		type = kv->GetInt("type", PhysicAction_None);
	}

	return type;
}

void CPhysicActionEditDialog::UpdateControlStates()
{
	int type = GetCurrentSelectedActionTypeIndex();

	LoadAvailableFactorsIntoControls(type);

	switch (type)
	{
	case PhysicAction_BarnacleDragForce:
	case PhysicAction_BarnacleChewForce:
	case PhysicAction_FirstPersonViewCamera: 
	case PhysicAction_ThirdPersonViewCamera:
	case PhysicAction_SimpleBuoyancy:
	{
		m_pRigidBodyLabel->SetVisible(true);
		m_pRigidBody->SetVisible(true);

		m_pConstraintLabel->SetVisible(false);
		m_pConstraint->SetVisible(false);
		break;
	}
	case PhysicAction_BarnacleConstraintLimitAdjustment:
	{
		m_pRigidBodyLabel->SetVisible(false);
		m_pRigidBody->SetVisible(false);

		m_pConstraintLabel->SetVisible(true);
		m_pConstraint->SetVisible(true);

		break;
	}
	default: {
		m_pRigidBodyLabel->SetVisible(false);
		m_pRigidBody->SetVisible(false);

		m_pConstraintLabel->SetVisible(false);
		m_pConstraint->SetVisible(false);
		break;
	}
	}
}

void CPhysicActionEditDialog::SaveConfigFromControls()
{
	char szText[256];

	SaveTypeFromControl(m_pType);

	SaveRigidBodyFromControl(m_pRigidBody, m_pPhysicActionConfig->rigidbody);
	SaveConstraintFromControl(m_pConstraint, m_pPhysicActionConfig->constraint);

#define SAVE_FROM_TEXT_ENTRY(to, from, processor) { \
        m_p##from->GetText(szText, sizeof(szText)); \
        m_pPhysicActionConfig->to = processor(szText); \
    }

	SAVE_FROM_TEXT_ENTRY(name, Name, std::string);

	SAVE_FROM_TEXT_ENTRY(origin[0], OriginX, atof);
	SAVE_FROM_TEXT_ENTRY(origin[1], OriginY, atof);
	SAVE_FROM_TEXT_ENTRY(origin[2], OriginZ, atof);

	SAVE_FROM_TEXT_ENTRY(angles[0], AnglesX, atof);
	SAVE_FROM_TEXT_ENTRY(angles[1], AnglesY, atof);
	SAVE_FROM_TEXT_ENTRY(angles[2], AnglesZ, atof);

	SAVE_FROM_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel, atoi);

#undef SAVE_FROM_TEXT_ENTRY

	// Save flags from check buttons using bitwise operations
#define SAVE_FLAG_FROM_CHECK_BUTTON(to, from) { \
        if (m_p##from->IsSelected()) \
            m_pPhysicActionConfig->to |= PhysicActionFlag_##from; \
        else \
            m_pPhysicActionConfig->to &= ~PhysicActionFlag_##from; \
    }

	SAVE_FLAG_FROM_CHECK_BUTTON(flags, Barnacle);
	SAVE_FLAG_FROM_CHECK_BUTTON(flags, Gargantua);
	//SAVE_FLAG_FROM_CHECK_BUTTON(flags, AffectsRigidBody);
	//SAVE_FLAG_FROM_CHECK_BUTTON(flags, AffectsConstraint);

	// Cleanup macro definition
#undef SAVE_FLAG_FROM_CHECK_BUTTON

	SaveFactorsFromControl(m_pPhysicFactorListPanel);

	// Mark the configuration as modified
	m_pPhysicActionConfig->configModified = true;
}

void CPhysicActionEditDialog::SaveRigidBodyFromControl(vgui::ComboBox* pComboBox, std::string& rigidBodyName)
{
	char szText[256];

	auto iActiveItem = pComboBox->GetActiveItem();
	if (pComboBox->IsItemIDValid(iActiveItem))
	{
		auto kv = pComboBox->GetItemUserData(iActiveItem);
		if (kv)
		{
			rigidBodyName = kv->GetString("name");
		}
		else
		{
			pComboBox->GetItemText(iActiveItem, szText, sizeof(szText));
			rigidBodyName = szText;
		}
	}
}

void CPhysicActionEditDialog::SaveConstraintFromControl(vgui::ComboBox* pComboBox, std::string& constraintName)
{
	char szText[256];

	auto iActiveItem = pComboBox->GetActiveItem();
	if (pComboBox->IsItemIDValid(iActiveItem))
	{
		auto kv = pComboBox->GetItemUserData(iActiveItem);
		if (kv)
		{
			constraintName = kv->GetString("name");
		}
		else
		{
			pComboBox->GetItemText(iActiveItem, szText, sizeof(szText));
			constraintName = szText;
		}
	}
}

void CPhysicActionEditDialog::SaveTypeFromControl(vgui::ComboBox* pComboBox)
{
	auto kv = pComboBox->GetActiveItemUserData();

	if (kv)
	{
		m_pPhysicActionConfig->type = kv->GetInt("type");
	}
}

void CPhysicActionEditDialog::SaveFactorsFromControl(vgui::ListPanel* pListPanel)
{
	for (int i = 0; i < pListPanel->GetItemCount(); ++i)
	{
		auto item = pListPanel->GetItemData(i);

		if (item && item->kv)
		{
			auto index = item->kv->GetInt("index");
			auto value = item->kv->GetString("value");
			auto defaultValue = item->kv->GetFloat("defaultValue");

			if (index >= 0 && index < _ARRAYSIZE(m_pPhysicActionConfig->factors))
			{
				float changedValue;

				if (!value[0])
				{
					changedValue = defaultValue;
				}
				else if (!stricmp(value, "nan"))
				{
					changedValue = NAN;
				}
				else
				{
					//changedValue = atof(value);
					if (1 != sscanf(value, "%f", &changedValue))
					{
						changedValue = NAN;
					}
				}

				if (m_pPhysicActionConfig->factors[index] != changedValue)
				{
					m_pPhysicActionConfig->factors[index] = changedValue;
					m_pPhysicActionConfig->configModified = true;
				}
			}
		}
	}
}