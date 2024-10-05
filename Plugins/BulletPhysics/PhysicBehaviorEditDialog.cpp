#include "PhysicBehaviorEditDialog.h"
#include "PhysicFactorListPanel.h"

#include "PhysicUTIL.h"

#include <format>

CPhysicBehaviorEditDialog::CPhysicBehaviorEditDialog(vgui::Panel* parent, const char* name,
	uint64 physicObjectId,
	const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
	const std::shared_ptr<CClientPhysicBehaviorConfig>& pPhysicBehaviorConfig) :
	BaseClass(parent, name),
	m_physicObjectId(physicObjectId),
	m_pPhysicObjectConfig(pPhysicObjectConfig),
	m_pPhysicBehaviorConfig(pPhysicBehaviorConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_PhysicBehaviorEditor", false);

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(520), vgui::scheme()->GetProportionalScaledValue(560));
	SetSize(vgui::scheme()->GetProportionalScaledValue(520), vgui::scheme()->GetProportionalScaledValue(560));

	// Initialize UI components
	m_pName = new vgui::TextEntry(this, "Name");
	m_pType = new vgui::ComboBox(this, "Type", 0, false);
	m_pDebugDrawLevel = new vgui::TextEntry(this, "DebugDrawLevel");

	m_pRigidBodyALabel = new vgui::Label(this, "RigidBodyALabel", "#BulletPhysics_RigidBodyA");
	m_pRigidBodyA = new vgui::ComboBox(this, "RigidBodyA", 0, true);

	m_pRigidBodyBLabel = new vgui::Label(this, "RigidBodyBLabel", "#BulletPhysics_RigidBodyB");
	m_pRigidBodyB = new vgui::ComboBox(this, "RigidBodyB", 0, true);

	m_pConstraintLabel = new vgui::Label(this, "ConstraintLabel", "#BulletPhysics_Constraint");
	m_pConstraint = new vgui::ComboBox(this, "Constraint", 0, true);

	m_pOriginX = new vgui::TextEntry(this, "OriginX");
	m_pOriginY = new vgui::TextEntry(this, "OriginY");
	m_pOriginZ = new vgui::TextEntry(this, "OriginZ");
	m_pAnglesX = new vgui::TextEntry(this, "AnglesX");
	m_pAnglesY = new vgui::TextEntry(this, "AnglesY");
	m_pAnglesZ = new vgui::TextEntry(this, "AnglesZ");

#define CREATE_CHECK_BUTTON(name)  m_p##name = new vgui::CheckButton(this, #name, "#BulletPhysics_" #name)

	CREATE_CHECK_BUTTON(Barnacle);
	CREATE_CHECK_BUTTON(Gargantua);

#undef CREATE_CHECK_BUTTON

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
		m_pRigidBodyA->SetUseFallbackFont(true, hFallbackFont);
		m_pRigidBodyB->SetUseFallbackFont(true, hFallbackFont);
		m_pConstraint->SetUseFallbackFont(true, hFallbackFont);
	}

	LoadAvailableTypesIntoControl(m_pType);

	// Load control settings
	LoadControlSettings("bulletphysics/PhysicBehaviorEditDialog.res", "GAME");

	// Add tick signal
	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CPhysicBehaviorEditDialog::~CPhysicBehaviorEditDialog()
{

}

void CPhysicBehaviorEditDialog::Activate(void)
{
	BaseClass::Activate();

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicBehaviorEditDialog::OnResetData()
{
	LoadConfigIntoControls();
}

void CPhysicBehaviorEditDialog::OnModifyFactor(KeyValues* kv)
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

void CPhysicBehaviorEditDialog::OnTextChanged(vgui::Panel* panel)
{
	if (panel == m_pType) {
		int type = GetCurrentSelectedTypeIndex(m_pType);
		UpdateControlStates(type);
	}
}

void CPhysicBehaviorEditDialog::OnKeyCodeTyped(vgui::KeyCode code)
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

void CPhysicBehaviorEditDialog::OnCommand(const char* command)
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
		PostActionSignal(new KeyValues("RefreshPhysicBehavior", "configId", m_pPhysicBehaviorConfig->configId));
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
		PostActionSignal(new KeyValues("RefreshPhysicBehavior", "configId", m_pPhysicBehaviorConfig->configId));
		PostMessage1(this, new KeyValues("ResetData"));
	}
	else if (!stricmp(command, "CloseModalDialogs"))
	{
		for (int i = 0; i < GetChildCount(); i++)
		{
			auto pChild = GetChild(i);
			PostMessage1(pChild, new KeyValues("Command", "command", "CloseModalDialogs"), NULL);
		}
		Close();
		return;
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicBehaviorEditDialog::LoadConfigIntoControls()
{
	LoadAvailableRigidBodiesIntoControl(m_pRigidBodyA);
	LoadAvailableRigidBodiesIntoControl(m_pRigidBodyB);

	LoadAvailableConstraintsIntoControl(m_pConstraint);

	LoadTypeIntoControl(m_pType);

	int type = GetCurrentSelectedTypeIndex(m_pType);

	UpdateControlStates(type);

	if (m_pRigidBodyA->IsVisible())
	{
		LoadRigidBodyIntoControl(m_pRigidBodyA, m_pPhysicBehaviorConfig->rigidbodyA);
	}
	else
	{
		m_pRigidBodyA->SetText("");
	}

	if (m_pRigidBodyB->IsVisible())
	{
		LoadRigidBodyIntoControl(m_pRigidBodyB, m_pPhysicBehaviorConfig->rigidbodyB);
	}
	else
	{
		m_pRigidBodyB->SetText("");
	}

	if (m_pConstraint->IsVisible())
	{
		LoadConstraintIntoControl(m_pConstraint, m_pPhysicBehaviorConfig->constraint);
	}
	else
	{
		m_pConstraint->SetText("");
	}

#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pPhysicBehaviorConfig->from); m_p##to->SetText(str##to.c_str());}

	LOAD_INTO_TEXT_ENTRY(name, Name);
	LOAD_INTO_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel);

	LOAD_INTO_TEXT_ENTRY(origin[0], OriginX);
	LOAD_INTO_TEXT_ENTRY(origin[1], OriginY);
	LOAD_INTO_TEXT_ENTRY(origin[2], OriginZ);

	LOAD_INTO_TEXT_ENTRY(angles[0], AnglesX);
	LOAD_INTO_TEXT_ENTRY(angles[1], AnglesY);
	LOAD_INTO_TEXT_ENTRY(angles[2], AnglesZ);

#undef LOAD_INTO_TEXT_ENTRY

#define LOAD_INTO_CHECK_BUTTON(from, to) m_p##to->SetSelected((m_pPhysicBehaviorConfig->from & PhysicBehaviorFlag_##to) ? true : false);

	LOAD_INTO_CHECK_BUTTON(flags, Barnacle);
	LOAD_INTO_CHECK_BUTTON(flags, Gargantua);

#undef LOAD_INTO_CHECK_BUTTON
}

void CPhysicBehaviorEditDialog::LoadAvailableTypesIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < PhysicBehavior_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("type", i);

		pComboBox->AddItem(UTIL_GetPhysicBehaviorTypeLocalizationToken(i), kv);

		kv->deleteThis();
	}
}

void CPhysicBehaviorEditDialog::LoadAvailableRigidBodiesIntoControl(vgui::ComboBox* pComboBox)
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

void CPhysicBehaviorEditDialog::LoadAvailableConstraintsIntoControl(vgui::ComboBox* pComboBox)
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

void CPhysicBehaviorEditDialog::LoadTypeIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		KeyValues* kv = pComboBox->GetItemUserData(i);

		if (kv && m_pPhysicBehaviorConfig->type == kv->GetInt("type", PhysicBehavior_None))
		{
			pComboBox->ActivateItemByRow(i);
			return;
		}
	}

	pComboBox->ActivateItemByRow(0);
}

void CPhysicBehaviorEditDialog::LoadRigidBodyIntoControl(vgui::ComboBox* pComboBox, const std::string& rigidBodyName)
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

	if (!rigidBodyName.empty())
	{
		pComboBox->SetText(rigidBodyName.c_str());
	}
	else
	{
		pComboBox->ActivateItem(0);
	}
}

void CPhysicBehaviorEditDialog::LoadConstraintIntoControl(vgui::ComboBox* pComboBox, const std::string& constraintName)
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

	if (!constraintName.empty())
	{
		pComboBox->SetText(constraintName.c_str());
	}
	else
	{
		pComboBox->ActivateItem(0);
	}
}

void CPhysicBehaviorEditDialog::DeleteFactorListPanelItem(int factorIdx)
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

void CPhysicBehaviorEditDialog::LoadFactorAsListPanelItemEx(int factorIdx, const char* name, const char* value, float defaultValue)
{
	auto kv = new KeyValues("Factor");

	kv->SetInt("index", factorIdx);

	kv->SetString("name", name);

	kv->SetString("value", value);

	kv->SetFloat("defaultValue", defaultValue);

	m_pPhysicFactorListPanel->AddItem(kv, factorIdx, false, true);

	kv->deleteThis();
}

void CPhysicBehaviorEditDialog::LoadFactorAsListPanelItem(int factorIdx, const char* name, float value, float defaultValue)
{
	auto val = std::format("{0:.4f}", value);

	LoadFactorAsListPanelItemEx(factorIdx, name, val.c_str(), defaultValue);
}

void CPhysicBehaviorEditDialog::LoadAvailableFactorsIntoControls(int type)
{
	m_pPhysicFactorListPanel->RemoveAll();

#define LOAD_FACTOR_INTO_LISTPANEL(name) LoadFactorAsListPanelItem(PhysicBehaviorFactorIdx_##name, "#BulletPhysics_" #name, m_pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_##name], NAN);
#define LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(name) LoadFactorAsListPanelItem(PhysicBehaviorFactorIdx_##name, "#BulletPhysics_" #name, m_pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_##name], PhysicBehaviorFactorDefaultValue_##name);

	switch (type)
	{
	case PhysicBehavior_BarnacleDragOnRigidBody: {
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragExtraHeight);
		break;
	}
	case PhysicBehavior_BarnacleDragOnConstraint: {
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragVelocity);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragExtraHeight);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragLimitAxis);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragCalculateLimitFromActualPlayerOrigin);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragUseServoMotor);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragActivatedOnBarnaclePulling);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragActivatedOnBarnacleChewing);
		break;
	}
	case PhysicBehavior_BarnacleChew: {
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleChewMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleChewInterval);
		break;
	}
	case PhysicBehavior_BarnacleConstraintLimitAdjustment: {
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleConstraintLimitAdjustmentExtraHeight);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleConstraintLimitAdjustmentInterval);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleConstraintLimitAdjustmentAxis);
		break;
	}
	case PhysicBehavior_GargantuaDragOnConstraint: {
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragVelocity);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragExtraHeight);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragLimitAxis);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(BarnacleDragUseServoMotor);
		break;
	}
	case PhysicBehavior_FirstPersonViewCamera:
	case PhysicBehavior_ThirdPersonViewCamera: {
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(CameraActivateOnIdle);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(CameraActivateOnDeath);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(CameraActivateOnCaughtByBarnacle);
		break;
	}
	case PhysicBehavior_SimpleBuoyancy: {
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(SimpleBuoyancyMagnitude);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(SimpleBuoyancyLinearDrag);
		LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(SimpleBuoyancyAngularDrag);
		break;
	}
	default: {
		break;
	}
	}
#undef LOAD_FACTOR_INTO_LISTPANEL
#undef LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE
}

int CPhysicBehaviorEditDialog::GetCurrentSelectedTypeIndex(vgui::ComboBox *pComboBox)
{
	int type = PhysicBehavior_None;

	auto kv = pComboBox->GetActiveItemUserData();

	if (kv)
	{
		type = kv->GetInt("type", PhysicBehavior_None);
	}

	return type;
}

void CPhysicBehaviorEditDialog::UpdateControlStates(int type)
{
	switch (type)
	{
	case PhysicBehavior_BarnacleDragOnRigidBody:
	case PhysicBehavior_BarnacleChew:
	case PhysicBehavior_FirstPersonViewCamera: 
	case PhysicBehavior_ThirdPersonViewCamera:
	case PhysicBehavior_SimpleBuoyancy:
	{
		m_pRigidBodyALabel->SetVisible(true);
		m_pRigidBodyA->SetVisible(true);

		m_pRigidBodyBLabel->SetVisible(false);
		m_pRigidBodyB->SetVisible(false);

		m_pConstraintLabel->SetVisible(false);
		m_pConstraint->SetVisible(false);
		break;
	}
	case PhysicBehavior_RigidBodyRelocation:
	{
		m_pRigidBodyALabel->SetVisible(true);
		m_pRigidBodyA->SetVisible(true);

		m_pRigidBodyBLabel->SetVisible(true);
		m_pRigidBodyB->SetVisible(true);

		m_pConstraintLabel->SetVisible(false);
		m_pConstraint->SetVisible(false);
		break;
	}
	case PhysicBehavior_BarnacleConstraintLimitAdjustment:
	case PhysicBehavior_BarnacleDragOnConstraint:
	case PhysicBehavior_GargantuaDragOnConstraint:
	{
		m_pRigidBodyALabel->SetVisible(false);
		m_pRigidBodyA->SetVisible(false);

		m_pRigidBodyBLabel->SetVisible(false);
		m_pRigidBodyB->SetVisible(false);

		m_pConstraintLabel->SetVisible(true);
		m_pConstraint->SetVisible(true);

		break;
	}
	default: {
		m_pRigidBodyALabel->SetVisible(false);
		m_pRigidBodyA->SetVisible(false);
		m_pRigidBodyA->SetText("");

		m_pRigidBodyBLabel->SetVisible(false);
		m_pRigidBodyB->SetVisible(false);

		m_pConstraintLabel->SetVisible(false);
		m_pConstraint->SetVisible(false);
		break;
	}
	}

	LoadAvailableFactorsIntoControls(type);

}

void CPhysicBehaviorEditDialog::SaveConfigFromControls()
{
	char szText[256];

	SaveTypeFromControl(m_pType);

	if (m_pRigidBodyA->IsVisible())
	{
		SaveRigidBodyFromControl(m_pRigidBodyA, m_pPhysicBehaviorConfig->rigidbodyA);
	}
	else
	{
		m_pPhysicBehaviorConfig->rigidbodyA = "";
	}

	if (m_pRigidBodyB->IsVisible())
	{
		SaveRigidBodyFromControl(m_pRigidBodyB, m_pPhysicBehaviorConfig->rigidbodyB);
	}
	else
	{
		m_pPhysicBehaviorConfig->rigidbodyB = "";
	}

	if (m_pConstraint->IsVisible())
	{
		SaveConstraintFromControl(m_pConstraint, m_pPhysicBehaviorConfig->constraint);
	}
	else
	{
		m_pPhysicBehaviorConfig->constraint = "";
	}

#define SAVE_FROM_TEXT_ENTRY(to, from, processor) { \
        m_p##from->GetText(szText, sizeof(szText)); \
        m_pPhysicBehaviorConfig->to = processor(szText); \
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
            m_pPhysicBehaviorConfig->to |= PhysicBehaviorFlag_##from; \
        else \
            m_pPhysicBehaviorConfig->to &= ~PhysicBehaviorFlag_##from; \
    }

	SAVE_FLAG_FROM_CHECK_BUTTON(flags, Barnacle);
	SAVE_FLAG_FROM_CHECK_BUTTON(flags, Gargantua);

	// Cleanup macro definition
#undef SAVE_FLAG_FROM_CHECK_BUTTON

	SaveFactorsFromControl(m_pPhysicFactorListPanel);

	// Mark the configuration as modified
	m_pPhysicBehaviorConfig->configModified = true;
}

void CPhysicBehaviorEditDialog::SaveRigidBodyFromControl(vgui::ComboBox* pComboBox, std::string& rigidBodyName)
{
	char szText[256];

	pComboBox->GetText(szText, sizeof(szText));
	rigidBodyName = szText;
}

void CPhysicBehaviorEditDialog::SaveConstraintFromControl(vgui::ComboBox* pComboBox, std::string& constraintName)
{
	char szText[256];

	pComboBox->GetText(szText, sizeof(szText));
	constraintName = szText;
}

void CPhysicBehaviorEditDialog::SaveTypeFromControl(vgui::ComboBox* pComboBox)
{
	auto kv = pComboBox->GetActiveItemUserData();

	if (kv)
	{
		m_pPhysicBehaviorConfig->type = kv->GetInt("type");
	}
}

void CPhysicBehaviorEditDialog::SaveFactorsFromControl(vgui::ListPanel* pListPanel)
{
	for (int i = 0; i < pListPanel->GetItemCount(); ++i)
	{
		auto item = pListPanel->GetItemData(i);

		if (item && item->kv)
		{
			auto index = item->kv->GetInt("index");
			auto value = item->kv->GetString("value");
			auto defaultValue = item->kv->GetFloat("defaultValue");

			if (index >= 0 && index < _ARRAYSIZE(m_pPhysicBehaviorConfig->factors))
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

				if (m_pPhysicBehaviorConfig->factors[index] != changedValue)
				{
					m_pPhysicBehaviorConfig->factors[index] = changedValue;
					m_pPhysicBehaviorConfig->configModified = true;
				}
			}
		}
	}
}