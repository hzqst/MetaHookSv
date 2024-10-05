#include "PhysicConstraintEditDialog.h"
#include "PhysicFactorListPanel.h"

#include "PhysicUTIL.h"

#include <format>

CPhysicConstraintEditDialog::CPhysicConstraintEditDialog(vgui::Panel* parent, const char* name,
    uint64 physicObjectId,
    const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
    const std::shared_ptr<CClientConstraintConfig>& pConstraintConfig) :
    BaseClass(parent, name),
    m_physicObjectId(physicObjectId),
    m_pPhysicObjectConfig(pPhysicObjectConfig),
    m_pConstraintConfig(pConstraintConfig)
{
    SetDeleteSelfOnClose(true);

    SetTitle("#BulletPhysics_ConstraintEditor", false);

    SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(800), vgui::scheme()->GetProportionalScaledValue(560));
    SetSize(vgui::scheme()->GetProportionalScaledValue(800), vgui::scheme()->GetProportionalScaledValue(560));

    // Initialize UI components
    m_pName = new vgui::TextEntry(this, "Name");
    m_pDebugDrawLevel = new vgui::TextEntry(this, "DebugDrawLevel");
    m_pType = new vgui::ComboBox(this, "Type", 0, false);
    m_pRigidBodyA = new vgui::ComboBox(this, "RigidBodyA", 0, true);
    m_pRigidBodyB = new vgui::ComboBox(this, "RigidBodyB", 0, true);
    m_pOriginAX = new vgui::TextEntry(this, "OriginAX");
    m_pOriginAY = new vgui::TextEntry(this, "OriginAY");
    m_pOriginAZ = new vgui::TextEntry(this, "OriginAZ");
    m_pAnglesAX = new vgui::TextEntry(this, "AnglesAX");
    m_pAnglesAY = new vgui::TextEntry(this, "AnglesAY");
    m_pAnglesAZ = new vgui::TextEntry(this, "AnglesAZ");
    m_pOriginBX = new vgui::TextEntry(this, "OriginBX");
    m_pOriginBY = new vgui::TextEntry(this, "OriginBY");
    m_pOriginBZ = new vgui::TextEntry(this, "OriginBZ");
    m_pAnglesBX = new vgui::TextEntry(this, "AnglesBX");
    m_pAnglesBY = new vgui::TextEntry(this, "AnglesBY");
    m_pAnglesBZ = new vgui::TextEntry(this, "AnglesBZ");
    m_pForwardX = new vgui::TextEntry(this, "ForwardX");
    m_pForwardY = new vgui::TextEntry(this, "ForwardY");
    m_pForwardZ = new vgui::TextEntry(this, "ForwardZ");
    m_pDisableCollision = new vgui::CheckButton(this, "DisableCollision", "#BulletPhysics_DisableCollision");
    m_pUseGlobalJointFromA = new vgui::CheckButton(this, "UseGlobalJointFromA", "#BulletPhysics_UseGlobalJointFromA");
    m_pUseLookAtOther = new vgui::CheckButton(this, "UseLookAtOther", "#BulletPhysics_UseLookAtOther");
    m_pUseGlobalJointOriginFromOther = new vgui::CheckButton(this, "UseGlobalJointOriginFromOther", "#BulletPhysics_UseGlobalJointOriginFromOther");
    m_pUseRigidBodyDistanceAsLinearLimit = new vgui::CheckButton(this, "UseRigidBodyDistanceAsLinearLimit", "#BulletPhysics_UseRigidBodyDistanceAsLinearLimit");
    m_pUseLinearReferenceFrameA = new vgui::CheckButton(this, "UseLinearReferenceFrameA", "#BulletPhysics_UseLinearReferenceFrameA");
    m_pRotOrder = new vgui::ComboBox(this, "RotOrder", 0, false);
    m_pMaxTolerantLinearError = new vgui::TextEntry(this, "MaxTolerantLinearError");

#define CREATE_CHECK_BUTTON(name)  m_p##name = new vgui::CheckButton(this, #name, "#BulletPhysics_" #name)

    CREATE_CHECK_BUTTON(Barnacle);
    CREATE_CHECK_BUTTON(Gargantua);
    CREATE_CHECK_BUTTON(DeactiveOnNormalActivity);
    CREATE_CHECK_BUTTON(DeactiveOnDeathActivity);
    CREATE_CHECK_BUTTON(DeactiveOnCaughtByBarnacleActivity);
    CREATE_CHECK_BUTTON(DeactiveOnBarnaclePullingActivity);
    CREATE_CHECK_BUTTON(DeactiveOnBarnacleChewingActivity);
    CREATE_CHECK_BUTTON(DontResetPoseOnErrorCorrection);
    CREATE_CHECK_BUTTON(DeferredCreate);

#undef CREATE_CHECK_BUTTON

    m_pPhysicFactorListPanel = new CPhysicFactorListPanel(this, "PhysicFactorListPanel");

    m_pPhysicFactorListPanel->AddColumnHeader(0, "index", "#BulletPhysics_Index", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
    m_pPhysicFactorListPanel->AddColumnHeader(1, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(160), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
    m_pPhysicFactorListPanel->AddColumnHeader(2, "value", "#BulletPhysics_Value", vgui::scheme()->GetProportionalScaledValue(60), vgui::ListPanel::COLUMN_FIXEDSIZE);
    m_pPhysicFactorListPanel->SetSortColumn(0);
    m_pPhysicFactorListPanel->SetMultiselectEnabled(false);

    vgui::HFont hFallbackFont = vgui::scheme()->GetIScheme(GetScheme())->GetFont("DefaultVerySmallFallBack", false);

    if (vgui::INVALID_FONT != hFallbackFont)
    {
        m_pType->SetUseFallbackFont(true, hFallbackFont);
        m_pRigidBodyA->SetUseFallbackFont(true, hFallbackFont);
        m_pRigidBodyB->SetUseFallbackFont(true, hFallbackFont);
        m_pRotOrder->SetUseFallbackFont(true, hFallbackFont);
    }

    LoadAvailableTypesIntoControl(m_pType);
    LoadAvailableRotOrdersIntoControl(m_pRotOrder);

    // Load control settings
    LoadControlSettings("bulletphysics/PhysicConstraintEditDialog.res", "GAME");

    // Add tick signal
    vgui::ivgui()->AddTickSignal(GetVPanel());
}

CPhysicConstraintEditDialog::~CPhysicConstraintEditDialog()
{
}

void CPhysicConstraintEditDialog::Activate(void)
{
    BaseClass::Activate();
    vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicConstraintEditDialog::OnResetData()
{
    LoadConfigIntoControls();
    UpdateControlStates();
}

void CPhysicConstraintEditDialog::OnModifyFactor(KeyValues* kv)
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

void CPhysicConstraintEditDialog::OnTextChanged(vgui::Panel* panel)
{
    if (panel == m_pType) {
        UpdateControlStates();
    }
}

void CPhysicConstraintEditDialog::OnKeyCodeTyped(vgui::KeyCode code)
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

void CPhysicConstraintEditDialog::OnCommand(const char* command)
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
        PostActionSignal(new KeyValues("RefreshConstraint", "configId", m_pConstraintConfig->configId));
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
        PostActionSignal(new KeyValues("RefreshConstraint", "configId", m_pConstraintConfig->configId));
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

void CPhysicConstraintEditDialog::LoadAvailableTypesIntoControl(vgui::ComboBox* pComboBox)
{
    pComboBox->RemoveAll();

    for (int i = 0; i < PhysicConstraint_Maximum; ++i)
    {
        auto kv = new KeyValues("UserData");

        kv->SetInt("type", i);

        pComboBox->AddItem(UTIL_GetConstraintTypeLocalizationToken(i), kv);

        kv->deleteThis();
    }
}

void CPhysicConstraintEditDialog::LoadAvailableRotOrdersIntoControl(vgui::ComboBox* pComboBox)
{
    pComboBox->RemoveAll();

    for (int i = 0; i < PhysicRotOrder_Maximum; ++i)
    {
        auto kv = new KeyValues("UserData");

        kv->SetInt("rotOrder", i);

        pComboBox->AddItem(UTIL_GetRotOrderTypeLocalizationToken(i), kv);

        kv->deleteThis();
    }
}

void CPhysicConstraintEditDialog::LoadAvailableRigidBodiesIntoControl(vgui::ComboBox* pComboBox)
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

void CPhysicConstraintEditDialog::DeleteFactorListPanelItem(int factorIdx)
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

void CPhysicConstraintEditDialog::LoadFactorAsListPanelItemEx(int factorIdx, const char* name, const char* value, float defaultValue)
{
    auto kv = new KeyValues("Factor");

    kv->SetInt("index", factorIdx);

    kv->SetString("name", name);

    kv->SetString("value", value);

    kv->SetFloat("defaultValue", defaultValue);

    m_pPhysicFactorListPanel->AddItem(kv, factorIdx, false, true);

    kv->deleteThis();
}

void CPhysicConstraintEditDialog::LoadFactorAsListPanelItem(int factorIdx, const char* name, float value, float defaultValue)
{
    auto val = std::format("{0:.4f}", value);

    LoadFactorAsListPanelItemEx(factorIdx, name, val.c_str(), defaultValue);
}

void CPhysicConstraintEditDialog::LoadAvailableFactorsIntoControl(int type)
{
    m_pPhysicFactorListPanel->RemoveAll();

#define LOAD_FACTOR_INTO_LISTPANEL(name) LoadFactorAsListPanelItem(PhysicConstraintFactorIdx_##name, "#BulletPhysics_" #name, m_pConstraintConfig->factors[PhysicConstraintFactorIdx_##name], NAN);
#define LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(name) LoadFactorAsListPanelItem(PhysicConstraintFactorIdx_##name, "#BulletPhysics_" #name, m_pConstraintConfig->factors[PhysicConstraintFactorIdx_##name], PhysicConstraintFactorDefaultValue_##name);

    switch (type)
    {
    case PhysicConstraint_ConeTwist: {

        LOAD_FACTOR_INTO_LISTPANEL(ConeTwistSwingSpanLimit1);
        LOAD_FACTOR_INTO_LISTPANEL(ConeTwistSwingSpanLimit2);
        LOAD_FACTOR_INTO_LISTPANEL(ConeTwistTwistSpanLimit);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(ConeTwistSoftness);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(ConeTwistBiasFactor);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(ConeTwistRelaxationFactor);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularCFM);
        break;
    }
    case PhysicConstraint_Hinge:
    {
        LOAD_FACTOR_INTO_LISTPANEL(HingeLowLimit);
        LOAD_FACTOR_INTO_LISTPANEL(HingeHighLimit);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(HingeSoftness);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(HingeBiasFactor);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(HingeRelaxationFactor);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopCFM);
        break;
    }
    case PhysicConstraint_Point:
    {
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularCFM);
        break;
    }
    case PhysicConstraint_Slider:
    {
        LOAD_FACTOR_INTO_LISTPANEL(SliderLowerLinearLimit);
        LOAD_FACTOR_INTO_LISTPANEL(SliderUpperLinearLimit);
        LOAD_FACTOR_INTO_LISTPANEL(SliderLowerAngularLimit);
        LOAD_FACTOR_INTO_LISTPANEL(SliderUpperAngularLimit);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopCFM);
        break;
    }
    case PhysicConstraint_Dof6:
    {
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerLinearLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerLinearLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerLinearLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperLinearLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperLinearLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperLinearLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerAngularLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerAngularLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerAngularLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperAngularLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperAngularLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperAngularLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopCFM);
        break;
    }
    case PhysicConstraint_Dof6Spring:
    {
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerLinearLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerLinearLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerLinearLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperLinearLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperLinearLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperLinearLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerAngularLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerAngularLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6LowerAngularLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperAngularLimitX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperAngularLimitY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6UpperAngularLimitZ);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(Dof6SpringEnableLinearSpringX);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(Dof6SpringEnableLinearSpringY);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(Dof6SpringEnableLinearSpringZ);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(Dof6SpringEnableAngularSpringX);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(Dof6SpringEnableAngularSpringY);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(Dof6SpringEnableAngularSpringZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringLinearStiffnessX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringLinearStiffnessY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringLinearStiffnessZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringAngularStiffnessX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringAngularStiffnessY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringAngularStiffnessZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringLinearDampingX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringLinearDampingY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringLinearDampingZ);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringAngularDampingX);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringAngularDampingY);
        LOAD_FACTOR_INTO_LISTPANEL(Dof6SpringAngularDampingZ);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopCFM);
        break;
    }
    case PhysicConstraint_Fixed:
    {
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(LinearStopCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularCFM);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopERP);
        LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE(AngularStopCFM);
        break;
    }
    default: {
        break;
    }
    }
#undef LOAD_FACTOR_INTO_LISTPANEL
#undef LOAD_FACTOR_INTO_LISTPANEL_DEFAULT_VALUE
}

void CPhysicConstraintEditDialog::LoadTypeIntoControl(int type)
{
    for (int i = 0; i < m_pType->GetItemCount(); ++i)
    {
        auto kv = m_pType->GetItemUserData(i);

        if (kv && type == kv->GetInt("type"))
        {
            m_pType->ActivateItem(i);
            return;
        }
    }

    m_pType->ActivateItem(0);
}

void CPhysicConstraintEditDialog::LoadRotOrderIntoControl(int rotOrder)
{
    for (int i = 0; i < m_pRotOrder->GetItemCount(); ++i)
    {
        auto kv = m_pRotOrder->GetItemUserData(i);

        if (kv && rotOrder == kv->GetInt("rotOrder"))
        {
            m_pRotOrder->ActivateItem(i);
            return;
        }
    }

    m_pRotOrder->ActivateItem(0);
}

void CPhysicConstraintEditDialog::LoadRigidBodyIntoControl(vgui::ComboBox* pComboBox, const std::string& rigidBodyName)
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

void CPhysicConstraintEditDialog::LoadConfigIntoControls()
{
    LoadAvailableRigidBodiesIntoControl(m_pRigidBodyA);
    LoadAvailableRigidBodiesIntoControl(m_pRigidBodyB);

    LoadTypeIntoControl(m_pConstraintConfig->type);
    LoadRigidBodyIntoControl(m_pRigidBodyA, m_pConstraintConfig->rigidbodyA);
    LoadRigidBodyIntoControl(m_pRigidBodyB, m_pConstraintConfig->rigidbodyB);
    LoadRotOrderIntoControl(m_pConstraintConfig->rotOrder);

#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pConstraintConfig->from); m_p##to->SetText(str##to.c_str());}

    LOAD_INTO_TEXT_ENTRY(name, Name);

    LOAD_INTO_TEXT_ENTRY(originA[0], OriginAX);
    LOAD_INTO_TEXT_ENTRY(originA[1], OriginAY);
    LOAD_INTO_TEXT_ENTRY(originA[2], OriginAZ);

    LOAD_INTO_TEXT_ENTRY(anglesA[0], AnglesAX);
    LOAD_INTO_TEXT_ENTRY(anglesA[1], AnglesAY);
    LOAD_INTO_TEXT_ENTRY(anglesA[2], AnglesAZ);

    LOAD_INTO_TEXT_ENTRY(originB[0], OriginBX);
    LOAD_INTO_TEXT_ENTRY(originB[1], OriginBY);
    LOAD_INTO_TEXT_ENTRY(originB[2], OriginBZ);

    LOAD_INTO_TEXT_ENTRY(anglesB[0], AnglesBX);
    LOAD_INTO_TEXT_ENTRY(anglesB[1], AnglesBY);
    LOAD_INTO_TEXT_ENTRY(anglesB[2], AnglesBZ);

    LOAD_INTO_TEXT_ENTRY(forward[0], ForwardX);
    LOAD_INTO_TEXT_ENTRY(forward[1], ForwardY);
    LOAD_INTO_TEXT_ENTRY(forward[2], ForwardZ);

    LOAD_INTO_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel);
    LOAD_INTO_TEXT_ENTRY(maxTolerantLinearError, MaxTolerantLinearError);

#undef LOAD_INTO_TEXT_ENTRY

#define LOAD_INTO_CHECK_BUTTON(attr, to) m_p##to->SetSelected(m_pConstraintConfig->attr);

    LOAD_INTO_CHECK_BUTTON(disableCollision, DisableCollision);
    LOAD_INTO_CHECK_BUTTON(useGlobalJointFromA, UseGlobalJointFromA);
    LOAD_INTO_CHECK_BUTTON(useLookAtOther, UseLookAtOther);
    LOAD_INTO_CHECK_BUTTON(useGlobalJointOriginFromOther, UseGlobalJointOriginFromOther);
    LOAD_INTO_CHECK_BUTTON(useRigidBodyDistanceAsLinearLimit, UseRigidBodyDistanceAsLinearLimit);
    LOAD_INTO_CHECK_BUTTON(useLinearReferenceFrameA, UseLinearReferenceFrameA);

#undef LOAD_INTO_CHECK_BUTTON

#define LOAD_INTO_CHECK_BUTTON(from, to) m_p##to->SetSelected((m_pConstraintConfig->from & PhysicConstraintFlag_##to) ? true : false);

    LOAD_INTO_CHECK_BUTTON(flags, Barnacle);
    LOAD_INTO_CHECK_BUTTON(flags, Gargantua);
    LOAD_INTO_CHECK_BUTTON(flags, DeactiveOnNormalActivity);
    LOAD_INTO_CHECK_BUTTON(flags, DeactiveOnDeathActivity);
    LOAD_INTO_CHECK_BUTTON(flags, DeactiveOnCaughtByBarnacleActivity);
    LOAD_INTO_CHECK_BUTTON(flags, DeactiveOnBarnaclePullingActivity);
    LOAD_INTO_CHECK_BUTTON(flags, DeactiveOnBarnacleChewingActivity);
    LOAD_INTO_CHECK_BUTTON(flags, DontResetPoseOnErrorCorrection);
    LOAD_INTO_CHECK_BUTTON(flags, DeferredCreate);

#undef LOAD_INTO_CHECK_BUTTON
}

void CPhysicConstraintEditDialog::SaveConfigFromControls()
{
    char szText[256];

    SaveTypeFromControl(m_pType);
    SaveRotOrderFromControl(m_pRotOrder);

    SaveRigidBodyFromControl(m_pRigidBodyA, m_pConstraintConfig->rigidbodyA);
    SaveRigidBodyFromControl(m_pRigidBodyB, m_pConstraintConfig->rigidbodyB);

    // Macro to save values from text entries to the constraint configuration
#define SAVE_FROM_TEXT_ENTRY(to, from, processor) { \
        m_p##from->GetText(szText, sizeof(szText)); \
        m_pConstraintConfig->to = processor(szText); \
    }

    SAVE_FROM_TEXT_ENTRY(name, Name, std::string);

    // Save vector components and other numerical values
    SAVE_FROM_TEXT_ENTRY(originA[0], OriginAX, atof);
    SAVE_FROM_TEXT_ENTRY(originA[1], OriginAY, atof);
    SAVE_FROM_TEXT_ENTRY(originA[2], OriginAZ, atof);

    SAVE_FROM_TEXT_ENTRY(anglesA[0], AnglesAX, atof);
    SAVE_FROM_TEXT_ENTRY(anglesA[1], AnglesAY, atof);
    SAVE_FROM_TEXT_ENTRY(anglesA[2], AnglesAZ, atof);

    SAVE_FROM_TEXT_ENTRY(originB[0], OriginBX, atof);
    SAVE_FROM_TEXT_ENTRY(originB[1], OriginBY, atof);
    SAVE_FROM_TEXT_ENTRY(originB[2], OriginBZ, atof);

    SAVE_FROM_TEXT_ENTRY(anglesB[0], AnglesBX, atof);
    SAVE_FROM_TEXT_ENTRY(anglesB[1], AnglesBY, atof);
    SAVE_FROM_TEXT_ENTRY(anglesB[2], AnglesBZ, atof);

    SAVE_FROM_TEXT_ENTRY(forward[0], ForwardX, atof);
    SAVE_FROM_TEXT_ENTRY(forward[1], ForwardY, atof);
    SAVE_FROM_TEXT_ENTRY(forward[2], ForwardZ, atof);

    SAVE_FROM_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel, atoi);
    SAVE_FROM_TEXT_ENTRY(maxTolerantLinearError, MaxTolerantLinearError, atof);

    // Cleanup macro definition
#undef SAVE_FROM_TEXT_ENTRY

// Macro to save check button states to boolean attributes
#define SAVE_FROM_CHECK_BUTTON(attr, from) m_pConstraintConfig->attr = m_p##from->IsSelected();

    SAVE_FROM_CHECK_BUTTON(disableCollision, DisableCollision);
    SAVE_FROM_CHECK_BUTTON(useGlobalJointFromA, UseGlobalJointFromA);
    SAVE_FROM_CHECK_BUTTON(useLookAtOther, UseLookAtOther);
    SAVE_FROM_CHECK_BUTTON(useGlobalJointOriginFromOther, UseGlobalJointOriginFromOther);
    SAVE_FROM_CHECK_BUTTON(useRigidBodyDistanceAsLinearLimit, UseRigidBodyDistanceAsLinearLimit);
    SAVE_FROM_CHECK_BUTTON(useLinearReferenceFrameA, UseLinearReferenceFrameA);

    // Cleanup macro definition
#undef SAVE_FROM_CHECK_BUTTON

// Save flags from check buttons using bitwise operations
#define SAVE_FLAG_FROM_CHECK_BUTTON(to, from) { \
        if (m_p##from->IsSelected()) \
            m_pConstraintConfig->to |= PhysicConstraintFlag_##from; \
        else \
            m_pConstraintConfig->to &= ~PhysicConstraintFlag_##from; \
    }

    SAVE_FLAG_FROM_CHECK_BUTTON(flags, Barnacle);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, Gargantua);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeactiveOnNormalActivity);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeactiveOnDeathActivity);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeactiveOnCaughtByBarnacleActivity);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeactiveOnBarnaclePullingActivity);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeactiveOnBarnacleChewingActivity);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DontResetPoseOnErrorCorrection);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeferredCreate);

    // Cleanup macro definition
#undef SAVE_FLAG_FROM_CHECK_BUTTON

    SaveFactorsFromControl(m_pPhysicFactorListPanel);

    // Mark the configuration as modified
    m_pConstraintConfig->configModified = true;
}

void CPhysicConstraintEditDialog::SaveTypeFromControl(vgui::ComboBox *pComboBox)
{
    auto kv = pComboBox->GetActiveItemUserData();

    if (kv)
    {
        m_pConstraintConfig->type = kv->GetInt("type");
    }
}

void CPhysicConstraintEditDialog::SaveRotOrderFromControl(vgui::ComboBox* pComboBox)
{
    auto kv = pComboBox->GetActiveItemUserData();

    if (kv)
    {
        m_pConstraintConfig->rotOrder = kv->GetInt("rotOrder");
    }
}

void CPhysicConstraintEditDialog::SaveFactorsFromControl(vgui::ListPanel *pListPanel)
{
    for (int i = 0; i < pListPanel->GetItemCount(); ++i)
    {
        auto item = pListPanel->GetItemData(i);

        if (item && item->kv)
        {
            auto index = item->kv->GetInt("index");
            auto value = item->kv->GetString("value");
            auto defaultValue = item->kv->GetFloat("defaultValue");

            if (index >= 0 && index < _ARRAYSIZE(m_pConstraintConfig->factors))
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

                if (m_pConstraintConfig->factors[index] != changedValue)
                {
                    m_pConstraintConfig->factors[index] = changedValue;
                    m_pConstraintConfig->configModified = true;
                }
            }
        }
    }
}

void CPhysicConstraintEditDialog::SaveRigidBodyFromControl(vgui::ComboBox* pComboBox, std::string& rigidBodyName)
{
    char szText[256] = {0};

    pComboBox->GetText(szText, sizeof(szText));

    rigidBodyName = szText;
}

int CPhysicConstraintEditDialog::GetCurrentSelectedConstraintType()
{
    int type = PhysicConstraint_None;

    auto kv = m_pType->GetActiveItemUserData();

    if (kv)
    {
        type = kv->GetInt("type", PhysicConstraint_None);
    }

    return type;
}

void CPhysicConstraintEditDialog::UpdateControlStates()
{
    int type = GetCurrentSelectedConstraintType();

    LoadAvailableFactorsIntoControl(type);

    switch (type)
    {
    case PhysicConstraint_Dof6:
    case PhysicConstraint_Dof6Spring: {

        m_pRotOrder->SetEnabled(true);
        break;
    }
    default: {
        m_pRotOrder->SetEnabled(false);
        break;
    }
    }
}