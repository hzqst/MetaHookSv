#include "PhysicEditorDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

//-----------------------------------------------------------------------------
// Purpose: panel used for inline editing of key bindings
//-----------------------------------------------------------------------------
class CInlineTextEntryPanel : public vgui::TextEntry
{
public:
    CInlineTextEntryPanel() : vgui::TextEntry(NULL, "InlineTextEntryPanel")
    {
        SetEditable(true);
        SetEnabled(true);
    }

    void OnKeyCodeTyped(vgui::KeyCode code) override
    {
        if (code == vgui::KEY_ENTER)
        {
            if (GetParent())
            {
                GetParent()->OnKeyCodeTyped(code);
            }
            return;
        }

        BaseClass::OnKeyCodeTyped(code);
    }

    void ApplySchemeSettings(vgui::IScheme* pScheme) override
    {
        BaseClass::ApplySchemeSettings(pScheme);
        SetBorder(pScheme->GetBorder("DepressedButtonBorder"));

        SetPaintBackgroundEnabled(true);
        auto bgColor = GetBgColor();
        bgColor[3] = 255;
        SetBgColor(bgColor);
        m_hFont = pScheme->GetFont("Default", IsProportional());

        SetFont(m_hFont);
    }

private:
    typedef vgui::TextEntry BaseClass;
    vgui::HFont		m_hFont{};
};

CFactorListPanel::CFactorListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{
    m_pInlineTextEntryPanel = new CInlineTextEntryPanel();
}

CFactorListPanel::~CFactorListPanel()
{
    m_pInlineTextEntryPanel->MarkForDeletion();
}

void CFactorListPanel::StartCaptureMode()
{
    if (GetSelectedItemsCount() == 0)
        return;

	auto selectItemId = GetSelectedItem(0);

	m_bCaptureMode = true;
    m_iCaptureItemId = selectItemId;

	EnterEditMode(selectItemId, 2, m_pInlineTextEntryPanel);
	vgui::input()->SetMouseFocus(m_pInlineTextEntryPanel->GetVPanel());

    auto kv = GetItemData(selectItemId)->kv;

    if (kv)
    {
        m_pInlineTextEntryPanel->SetText(kv->GetString("value"));
        m_iCaptureItemIndex = kv->GetInt("index");
    }
}

void CFactorListPanel::EndCaptureMode()
{
    m_bCaptureMode = false;
    m_iCaptureItemId = -1;
    LeaveEditMode();
    RequestFocus();
    vgui::input()->SetMouseFocus(GetVPanel());

    auto kv = new KeyValues("ModifyFactor");

    kv->SetInt("index", m_iCaptureItemIndex);

    char szText[256];
    m_pInlineTextEntryPanel->GetText(szText, sizeof(szText));

    kv->SetString("newValue", szText);

    PostActionSignal(kv);
}

bool CFactorListPanel::IsCapturing(void) const
{
    return m_bCaptureMode;
}

int CFactorListPanel::GetCapturingItemId(void) const
{
    return m_iCaptureItemId;
}

int CFactorListPanel::GetCapturingItemIndex(void) const
{
    return m_iCaptureItemIndex;
}

void CFactorListPanel::OnMousePressed(vgui::MouseCode code)
{
    BaseClass::OnMousePressed(code);

    if (IsCapturing())
    {
        if (m_VisibleItems.Count() > 0)
        {
            // determine where we were pressed
            int x, y, row, column;
            vgui::input()->GetCursorPos(x, y);
            GetCellAtPos(x, y, row, column);

            if (row < 0 || row >= m_VisibleItems.Count())
            {
                EndCaptureMode();
                return;
            }

            int itemID = m_VisibleItems[row];

            if (itemID != GetCapturingItemId())
            {
                EndCaptureMode();
                return;
            }
        }
    }
}

// Constructor
CConstraintEditDialog::CConstraintEditDialog(vgui::Panel* parent, const char* name,
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

    SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(560), vgui::scheme()->GetProportionalScaledValue(560));
    SetSize(vgui::scheme()->GetProportionalScaledValue(560), vgui::scheme()->GetProportionalScaledValue(560));

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

    // Initialize new CheckButton controls
    m_pBarnacle = new vgui::CheckButton(this, "Barnacle", "#BulletPhysics_Barnacle");
    m_pGargantua = new vgui::CheckButton(this, "Gargantua", "#BulletPhysics_Gargantua");
    m_pDeactiveOnNormalActivity = new vgui::CheckButton(this, "DeactiveOnNormalActivity", "#BulletPhysics_DeactiveOnNormalActivity");
    m_pDeactiveOnDeathActivity = new vgui::CheckButton(this, "DeactiveOnDeathActivity", "#BulletPhysics_DeactiveOnDeathActivity");
    m_pDeactiveOnBarnacleActivity = new vgui::CheckButton(this, "DeactiveOnBarnacleActivity", "#BulletPhysics_DeactiveOnBarnacleActivity");
    m_pDeactiveOnGargantuaActivity = new vgui::CheckButton(this, "DeactiveOnGargantuaActivity", "#BulletPhysics_DeactiveOnGargantuaActivity");
    m_pDontResetPoseOnErrorCorrection = new vgui::CheckButton(this, "DontResetPoseOnErrorCorrection", "#BulletPhysics_DontResetPoseOnErrorCorrection");

    m_pFactorListPanel = new CFactorListPanel(this, "FactorListPanel");

    m_pFactorListPanel->AddColumnHeader(0, "index", "#BulletPhysics_Index", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
    m_pFactorListPanel->AddColumnHeader(1, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_FIXEDSIZE);
    m_pFactorListPanel->AddColumnHeader(2, "value", "#BulletPhysics_Value", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
    m_pFactorListPanel->SetSortColumn(0);
    m_pFactorListPanel->SetMultiselectEnabled(false);

    vgui::HFont hFallbackFont = vgui::scheme()->GetIScheme(GetScheme())->GetFont("DefaultVerySmallFallBack", false);

    if (vgui::INVALID_FONT != hFallbackFont)
    {
        m_pType->SetUseFallbackFont(true, hFallbackFont);
        m_pRigidBodyA->SetUseFallbackFont(true, hFallbackFont);
        m_pRigidBodyB->SetUseFallbackFont(true, hFallbackFont);
        m_pRotOrder->SetUseFallbackFont(true, hFallbackFont);
    }

    LoadAvailableTypesIntoControls();

    LoadAvailableRotOrdersIntoControls();

    // Load control settings
    LoadControlSettings("bulletphysics/ConstraintEditDialog.res", "GAME");

    // Add tick signal
    vgui::ivgui()->AddTickSignal(GetVPanel());
}

void CConstraintEditDialog::Activate(void)
{
    BaseClass::Activate();
    vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CConstraintEditDialog::OnItemSelected()
{

}

void CConstraintEditDialog::OnResetData()
{
    LoadConfigIntoControls();
    UpdateControlStates();
}

void CConstraintEditDialog::OnModifyFactor(KeyValues *kv)
{
    auto index = kv->GetInt("index");
    auto newValue = kv->GetString("newValue");

    for (int i = 0; i < m_pFactorListPanel->GetItemCount(); ++i)
    {
        auto userData = m_pFactorListPanel->GetItemUserData(i);

        if (userData == index)
        {
            auto oldkv = m_pFactorListPanel->GetItemData(i);

            std::string name = oldkv->kv->GetString("name");

            float defaultValue = oldkv->kv->GetFloat("defaultValue");

            m_pFactorListPanel->RemoveItem(i);

            LoadFactorAsListPanelItemEx(index, name.c_str(), newValue, defaultValue);
            break;
        }
    }
}

void CConstraintEditDialog::OnTextChanged(vgui::Panel* panel)
{
    if (panel == m_pType) {
        UpdateControlStates();
    }
}

void CConstraintEditDialog::OnKeyCodeTyped(vgui::KeyCode code)
{
    if (code == vgui::KEY_ENTER)
    {
        if (m_pFactorListPanel->IsCapturing())
        {
            m_pFactorListPanel->EndCaptureMode();
            return;
        }

        if (m_pFactorListPanel->HasFocus())
        {
            if (!m_pFactorListPanel->IsCapturing())
            {
                m_pFactorListPanel->StartCaptureMode();
            }
            return;
        }
    }

    BaseClass::OnKeyCodeTyped(code);
}

void CConstraintEditDialog::OnCommand(const char* command)
{
    if (!stricmp(command, "OK"))
    {
        SaveConfigFromControls();
        ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
        PostActionSignal(new KeyValues("RefreshConstraint", "configId", m_pConstraintConfig->configId));
        PostMessage1(this, new KeyValues("ResetData"));
        Close();
    }
    else if (!stricmp(command, "Apply"))
    {
        SaveConfigFromControls();
        ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
        PostActionSignal(new KeyValues("RefreshConstraint", "configId", m_pConstraintConfig->configId));
        PostMessage1(this, new KeyValues("ResetData"));
    }
    else
    {
        BaseClass::OnCommand(command);
    }
}

// Destructor
CConstraintEditDialog::~CConstraintEditDialog()
{
}

void CConstraintEditDialog::LoadAvailableTypesIntoControls()
{
    m_pType->RemoveAll();

    for (int i = 0; i < PhysicConstraint_Maximum; ++i)
    {
        auto kv = new KeyValues("UserData");

        kv->SetInt("type", i);

        m_pType->AddItem(UTIL_GetConstraintTypeLocalizationToken(i), kv);

        kv->deleteThis();
    }
}

void CConstraintEditDialog::LoadAvailableRotOrdersIntoControls()
{
    m_pRotOrder->RemoveAll();

    for (int i = 0; i < PhysicRotOrder_Maximum; ++i)
    {
        auto kv = new KeyValues("UserData");

        kv->SetInt("rotOrder", i);

        m_pRotOrder->AddItem(UTIL_GetRotOrderTypeLocalizationToken(i), kv);

        kv->deleteThis();
    }
}

void CConstraintEditDialog::LoadAvailableRigidBodiesIntoControls(vgui::ComboBox* pComboBox)
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

void CConstraintEditDialog::DeleteFactorListPanelItem(int factorIdx)
{
    for (int i = 0; i < m_pFactorListPanel->GetItemCount(); ++i)
    {
        auto userData = m_pFactorListPanel->GetItemUserData(i);

        if (userData == factorIdx)
        {
            m_pFactorListPanel->RemoveItem(i);
            break;
        }
    }
}

void CConstraintEditDialog::LoadFactorAsListPanelItemEx(int factorIdx, const char* name, const char *value, float defaultValue)
{
    auto kv = new KeyValues("Factor");

    kv->SetInt("index", factorIdx);

    kv->SetString("name", name);

    kv->SetString("value", value);

    kv->SetFloat("defaultValue", defaultValue);

    m_pFactorListPanel->AddItem(kv, factorIdx, false, true);

    kv->deleteThis();
}

void CConstraintEditDialog::LoadFactorAsListPanelItem(int factorIdx, const char * name, float value, float defaultValue)
{
    auto val = std::format("{0:.4f}", value);
 
    LoadFactorAsListPanelItemEx(factorIdx, name, val.c_str(), defaultValue);
}

void CConstraintEditDialog::LoadAvailableFactorsIntoControls(int type)
{
    m_pFactorListPanel->RemoveAll();

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
}

void CConstraintEditDialog::LoadTypeIntoControl(int type)
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

void CConstraintEditDialog::LoadRotOrderIntoControl(int rotOrder)
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

void CConstraintEditDialog::LoadRigidBodyIntoControl(const std::string &rigidBodyName, vgui::ComboBox *pComboBox)
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

void CConstraintEditDialog::LoadConfigIntoControls()
{
    LoadAvailableRigidBodiesIntoControls(m_pRigidBodyA);
    LoadAvailableRigidBodiesIntoControls(m_pRigidBodyB);

    LoadTypeIntoControl(m_pConstraintConfig->type);
    LoadRigidBodyIntoControl(m_pConstraintConfig->rigidbodyA, m_pRigidBodyA);
    LoadRigidBodyIntoControl(m_pConstraintConfig->rigidbodyB, m_pRigidBodyB);
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
    LOAD_INTO_CHECK_BUTTON(flags, DeactiveOnBarnacleActivity);
    LOAD_INTO_CHECK_BUTTON(flags, DeactiveOnGargantuaActivity);
    LOAD_INTO_CHECK_BUTTON(flags, DontResetPoseOnErrorCorrection);

#undef LOAD_INTO_CHECK_BUTTON
}

void CConstraintEditDialog::SaveTypeFromControl()
{
    auto kv = m_pType->GetActiveItemUserData();

    if (kv)
    {
        m_pConstraintConfig->type = kv->GetInt("type");
    }
}

void CConstraintEditDialog::SaveRotOrderFromControl()
{
    auto kv = m_pRotOrder->GetActiveItemUserData();

    if (kv)
    {
        m_pConstraintConfig->rotOrder = kv->GetInt("rotOrder");
    }
}

void CConstraintEditDialog::SaveFactorFromControls()
{
    for (int i = 0; i < m_pFactorListPanel->GetItemCount(); ++i)
    {
        auto item = m_pFactorListPanel->GetItemData(i);

        if (item && item->kv)
        {
            auto index = item->kv->GetInt("index");
            auto value = item->kv->GetString("value");
            auto defaultValue = item->kv->GetFloat("defaultValue");

            if (index >= 0 && index < _ARRAYSIZE(m_pConstraintConfig->factors))
            {
                if (!value[0])
                {
                    m_pConstraintConfig->factors[index] = defaultValue;
                }
                else if (!stricmp(value, "nan"))
                {
                    m_pConstraintConfig->factors[index] = NAN;
                }
                else
                {
                    m_pConstraintConfig->factors[index] = atof(value);
                }

                m_pConstraintConfig->configModified = true;
            }
        }
    }
}

void CConstraintEditDialog::SaveRigidBodyFromControl(vgui::ComboBox *pComboBox, std::string& rigidBodyName)
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

void CConstraintEditDialog::SaveConfigFromControls()
{
    char szText[256];

    SaveTypeFromControl();
    SaveRotOrderFromControl();

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
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeactiveOnBarnacleActivity);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DeactiveOnGargantuaActivity);
    SAVE_FLAG_FROM_CHECK_BUTTON(flags, DontResetPoseOnErrorCorrection);

    // Cleanup macro definition
#undef SAVE_FLAG_FROM_CHECK_BUTTON

    if (m_pFactorListPanel->IsCapturing())
    {
        m_pFactorListPanel->EndCaptureMode();
    }
    SaveFactorFromControls();

// Mark the configuration as modified
    m_pConstraintConfig->configModified = true;
}

int CConstraintEditDialog::GetCurrentSelectedConstraintType()
{
    int type = PhysicConstraint_None;

    auto kv = m_pType->GetActiveItemUserData();

    if (kv)
    {
        type = kv->GetInt("type", PhysicConstraint_None);
    }

    return type;
}

void CConstraintEditDialog::UpdateControlStates()
{
    int type = GetCurrentSelectedConstraintType();

    LoadAvailableFactorsIntoControls(type);

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