#include "PhysicEditorDialog.h"
#include <vgui_controls/Menu.h>

#include <studio.h>
#include <const.h>
#include <r_studioint.h>

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicGUICommon.h"

#include <format>

//RigidBody List

CRigidBodyListPanel::CRigidBodyListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{

}

CRigidBodyPage::CRigidBodyPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicConfig(pPhysicConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pRigidBodyListPanel = new CRigidBodyListPanel(this, "RigidBodyListPanel");

	m_pRigidBodyListPanel->AddColumnHeader(0, "configId", "#BulletPhysics_ConfigId", vgui::scheme()->GetProportionalScaledValue(40), vgui::ListPanel::COLUMN_HIDDEN);
	m_pRigidBodyListPanel->AddColumnHeader(1, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(2, "shape","#BulletPhysics_Shape", vgui::scheme()->GetProportionalScaledValue(60), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(3, "bone", "#BulletPhysics_Bone", vgui::scheme()->GetProportionalScaledValue(180), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(4, "mass", "#BulletPhysics_Mass", vgui::scheme()->GetProportionalScaledValue(60), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(5, "flags", "#BulletPhysics_Flags", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);
	m_pRigidBodyListPanel->SetSortColumn(0);
	m_pRigidBodyListPanel->SetMultiselectEnabled(false);

	m_pCreateNewRigidBody = new vgui::Button(this, "CreateNewRigidBody", L"#BulletPhysics_CreateNewRigidBody", this, "CreateNewRigidBody");

	LoadControlSettings("bulletphysics/RigidBodyPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

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
	else if (!stricmp(command, "CreateNewRigidBody"))
	{
		OnCreateNewRigidBody();
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

	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteRigidBody"), 1, szName);
	menu->AddMenuItem("DeleteRigidBody", szBuf, new KeyValues("DeleteRigidBody", "configId", configId), this);

	vgui::Menu::PlaceContextMenu(this, menu);
}

void CRigidBodyPage::OnOpenRigidBodyEditor(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto dialog = new CRigidBodyEditDialog(this, "RigidBodyEditDialog", m_physicObjectId, pRigidBodyConfig);
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

	for (int i = 0; i < m_pRigidBodyListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pRigidBodyListPanel->GetItemUserData(i);

		if (userData == configId)
		{
			m_pRigidBodyListPanel->RemoveItem(i);
			LoadRigidBodyAsListPanelItem(pRigidBodyConfig.get());
			break;
		}
	}
}

void CRigidBodyPage::OnRefreshRigidBodies()
{
	ReloadAllRigidBodiesIntoListPanelItem();
}

void CRigidBodyPage::LoadRigidBodyAsListPanelItem(const CClientRigidBodyConfig * pRigidBodyConfig)
{
	auto kv = new KeyValues("RigidBody");

	kv->SetInt("configId", pRigidBodyConfig->configId);

	kv->SetString("name", pRigidBodyConfig->name.c_str());

	std::wstring shape = pRigidBodyConfig->collisionShape ? UTIL_GetCollisionShapeTypeLocalizedName(pRigidBodyConfig->collisionShape->type) : L"--";

	kv->SetWString("shape", shape.c_str());

	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(m_physicObjectId);

	auto bonename = UTIL_GetFormattedBoneName(modelindex, pRigidBodyConfig->boneindex);

	kv->SetString("bone", bonename.c_str());

	kv->SetFloat("mass", pRigidBodyConfig->mass);

	auto flags = UTIL_GetFormattedRigidBodyFlags(pRigidBodyConfig->flags);

	kv->SetWString("flags", flags.c_str());

	m_pRigidBodyListPanel->AddItem(kv, pRigidBodyConfig->configId, false, true);

	kv->deleteThis();
}

void CRigidBodyPage::ReloadAllRigidBodiesIntoListPanelItem()
{
	m_pRigidBodyListPanel->RemoveAll();

	for (const auto &pConfig : m_pPhysicConfig->RigidBodyConfigs)
	{
		LoadRigidBodyAsListPanelItem(pConfig.get());
	}
}

void CRigidBodyPage::OnCreateNewRigidBody()
{
	auto pRigidBodyConfig = std::make_shared<CClientRigidBodyConfig>();

	pRigidBodyConfig->name = std::format("Unnamed_{0}", pRigidBodyConfig->configId);

	m_pPhysicConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

	ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

	LoadRigidBodyAsListPanelItem(pRigidBodyConfig.get());

	ClientPhysicManager()->RemovePhysicObjectEx(m_physicObjectId);
}

void CRigidBodyPage::OnDeleteRigidBody(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	for (auto itor = m_pPhysicConfig->RigidBodyConfigs.begin(); itor != m_pPhysicConfig->RigidBodyConfigs.end(); )
	{
		const auto &p = (*itor);

		if (p == pRigidBodyConfig)
		{
			itor = m_pPhysicConfig->RigidBodyConfigs.erase(itor);
			break;
		}

		itor++;
	}

	for (int i = 0; i < m_pRigidBodyListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pRigidBodyListPanel->GetItemUserData(i);

		if (userData == configId)
		{
			m_pRigidBodyListPanel->RemoveItem(i);
			break;
		}
	}

	ClientPhysicManager()->RemovePhysicObjectEx(m_physicObjectId);
}

//CollisionShape Editor

CCollisionShapeEditDialog::CCollisionShapeEditDialog(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientCollisionShapeConfig>& pCollisionShapeConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pCollisionShapeConfig(pCollisionShapeConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_CollisionShapeEditor", false);

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(350), vgui::scheme()->GetProportionalScaledValue(350));
	SetSize(vgui::scheme()->GetProportionalScaledValue(350), vgui::scheme()->GetProportionalScaledValue(350));

	m_pShape = new vgui::ComboBox(this, "Shape", 0, false);
	m_pShape->AddActionSignalTarget(this);

	m_pDirectionLabel = new vgui::Label(this, "DirectionLabel", "#BulletPhysics_Direction");
	m_pDirection = new vgui::ComboBox(this, "Direction", 0, false);

	m_pSizeLabel = new vgui::Label(this, "SizeLabel", "#BulletPhysics_Size");
	m_pSizeX = new vgui::TextEntry(this, "SizeX");
	m_pSizeY = new vgui::TextEntry(this, "SizeY");
	m_pSizeZ = new vgui::TextEntry(this, "SizeZ");

	m_pOriginLabel = new vgui::Label(this, "OriginLabel", "#BulletPhysics_Origin");
	m_pOriginX = new vgui::TextEntry(this, "OriginX");
	m_pOriginY = new vgui::TextEntry(this, "OriginY");
	m_pOriginZ = new vgui::TextEntry(this, "OriginZ");

	m_pAnglesLabel = new vgui::Label(this, "AnglesLabel", "#BulletPhysics_Angles");
	m_pAnglesX = new vgui::TextEntry(this, "AnglesX");
	m_pAnglesY = new vgui::TextEntry(this, "AnglesY");
	m_pAnglesZ = new vgui::TextEntry(this, "AnglesZ");

	m_pObjPathLabel = new vgui::Label(this, "ObjPathLabel", "#BulletPhysics_ObjPath");
	m_pObjPath = new vgui::TextEntry(this, "ObjPath");

	LoadAvailableShapesIntoControls();
	LoadAvailableShapeDirectionsIntoControls();

	LoadControlSettings("bulletphysics/CollisionShapeEditDialog.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

}

CCollisionShapeEditDialog::~CCollisionShapeEditDialog()
{

}

void CCollisionShapeEditDialog::Activate(void)
{
	BaseClass::Activate();

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CCollisionShapeEditDialog::OnResetData()
{
	LoadConfigIntoControls();

	UpdateControlStates();
}

void CCollisionShapeEditDialog::OnTextChanged(vgui::Panel *panel)
{
	if (panel == m_pShape) {
		UpdateControlStates();
	}
}

void CCollisionShapeEditDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		SaveConfigFromControls();
		PostActionSignal(new KeyValues("RefreshCollisionShape"));
		ClientPhysicManager()->RemovePhysicObjectEx(m_physicObjectId);
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		SaveConfigFromControls();
		PostActionSignal(new KeyValues("RefreshCollisionShape"));
		ClientPhysicManager()->RemovePhysicObjectEx(m_physicObjectId);
		return;
	}

	BaseClass::OnCommand(command);
}

void CCollisionShapeEditDialog::LoadAvailableShapesIntoControls()
{
	for (int i = 0; i < PhysicShape_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("type", i);

		m_pShape->AddItem(UTIL_GetCollisionShapeTypeLocalizationToken(i), kv);

		kv->deleteThis();
	}
}

void CCollisionShapeEditDialog::LoadShapeIntoControls()
{
	for (int i = 0; i < m_pShape->GetItemCount(); ++i)
	{
		KeyValues* kv = m_pShape->GetItemUserData(i);

		if (kv && m_pCollisionShapeConfig->type == kv->GetInt("type", PhysicShape_None))
		{
			m_pShape->ActivateItemByRow(i);
			return;
		}
	}

	m_pShape->ActivateItemByRow(0);
}

void CCollisionShapeEditDialog::LoadAvailableShapeDirectionsIntoControls()
{
	for (int i = 0; i < PhysicShapeDirection_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("direction", i);

		const char* XYZ[] = { "X", "Y", "Z" };

		m_pDirection->AddItem(XYZ[i], kv);

		kv->deleteThis();
	}
}

void CCollisionShapeEditDialog::LoadShapeDirectionIntoControls()
{
	for (int i = 0; i < m_pDirection->GetItemCount(); ++i)
	{
		KeyValues* kv = m_pDirection->GetItemUserData(i);

		if (kv && m_pCollisionShapeConfig->direction == kv->GetInt("direction", PhysicShapeDirection_X))
		{
			m_pDirection->ActivateItemByRow(i);
			return;
		}
	}

	m_pDirection->ActivateItemByRow(0);
}

void CCollisionShapeEditDialog::LoadConfigIntoControls()
{
	LoadShapeIntoControls();

	LoadShapeDirectionIntoControls();

	auto sizeX = std::format("{0}", m_pCollisionShapeConfig->size[0]);
	m_pSizeX->SetText(sizeX.c_str());

	auto sizeY = std::format("{0}", m_pCollisionShapeConfig->size[1]);
	m_pSizeY->SetText(sizeY.c_str());

	auto sizeZ = std::format("{0}", m_pCollisionShapeConfig->size[2]);
	m_pSizeZ->SetText(sizeZ.c_str());

	auto originX = std::format("{0}", m_pCollisionShapeConfig->origin[0]);
	m_pOriginX->SetText(originX.c_str());

	auto originY = std::format("{0}", m_pCollisionShapeConfig->origin[1]);
	m_pOriginY->SetText(originY.c_str());

	auto originZ = std::format("{0}", m_pCollisionShapeConfig->origin[2]);
	m_pOriginZ->SetText(originZ.c_str());

	auto anglesX = std::format("{0}", m_pCollisionShapeConfig->angles[0]);
	m_pAnglesX->SetText(anglesX.c_str());

	auto anglesY = std::format("{0}", m_pCollisionShapeConfig->angles[1]);
	m_pAnglesY->SetText(anglesY.c_str());

	auto anglesZ = std::format("{0}", m_pCollisionShapeConfig->angles[2]);
	m_pAnglesZ->SetText(anglesZ.c_str());

	m_pObjPath->SetText(m_pCollisionShapeConfig->objpath.c_str());
}

void CCollisionShapeEditDialog::SaveConfigFromControls()
{
	m_pCollisionShapeConfig->type = GetCurrentSelectedShapeType();
	m_pCollisionShapeConfig->direction = GetCurrentSelectedShapeDirection();

	char szText[256] = { 0 };

	m_pSizeX->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->size[0] = atof(szText);

	m_pSizeY->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->size[1] = atof(szText);

	m_pSizeZ->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->size[2] = atof(szText);

	m_pOriginX->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->origin[0] = atof(szText);

	m_pOriginY->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->origin[1] = atof(szText);

	m_pOriginZ->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->origin[2] = atof(szText);

	m_pAnglesX->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->angles[0] = atof(szText);

	m_pAnglesY->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->angles[1] = atof(szText);

	m_pAnglesZ->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->angles[2] = atof(szText);

	m_pObjPath->GetText(szText, sizeof(szText));
	m_pCollisionShapeConfig->objpath = szText;
}

int CCollisionShapeEditDialog::GetCurrentSelectedShapeType()
{
	int type = PhysicShape_None;

	auto pShapeKV = m_pShape->GetActiveItemUserData();

	if (pShapeKV)
	{
		type = pShapeKV->GetInt("type", PhysicShape_None);
	}

	return type;
}

int CCollisionShapeEditDialog::GetCurrentSelectedShapeDirection()
{
	int direction = PhysicShapeDirection_X;

	auto pDirectionKV = m_pDirection->GetActiveItemUserData();

	if (pDirectionKV)
	{
		direction = pDirectionKV->GetInt("direction", PhysicShapeDirection_X);
	}

	return direction;
}

void CCollisionShapeEditDialog::UpdateControlStates()
{
	switch (GetCurrentSelectedShapeType())
	{
	case PhysicShape_Box: {
		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(true);
		m_pSizeX->SetVisible(true);
		m_pSizeY->SetVisible(true);
		m_pSizeZ->SetVisible(true);

		m_pObjPathLabel->SetVisible(false);
		m_pObjPath->SetVisible(false);
		break;
	}
	case PhysicShape_Sphere: {

		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(true);
		m_pSizeX->SetVisible(true);
		m_pSizeY->SetVisible(false);
		m_pSizeZ->SetVisible(false);

		m_pObjPathLabel->SetVisible(false);
		m_pObjPath->SetVisible(false);
		break;
	}
	case PhysicShape_Capsule:
	case PhysicShape_Cylinder: {

		m_pDirectionLabel->SetVisible(true);
		m_pDirection->SetVisible(true);

		m_pSizeLabel->SetVisible(true);
		m_pSizeX->SetVisible(true);
		m_pSizeY->SetVisible(true);
		m_pSizeZ->SetVisible(false);

		m_pObjPathLabel->SetVisible(false);
		m_pObjPath->SetVisible(false);
		break;
	}
	case PhysicShape_TriangleMesh: {

		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(false);
		m_pSizeX->SetVisible(false);
		m_pSizeY->SetVisible(false);
		m_pSizeZ->SetVisible(false);

		m_pObjPathLabel->SetVisible(true);
		m_pObjPath->SetVisible(true);
		break;
	}
	default:{

		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(false);
		m_pSizeX->SetVisible(false);
		m_pSizeY->SetVisible(false);
		m_pSizeZ->SetVisible(false);

		m_pObjPathLabel->SetVisible(false);
		m_pObjPath->SetVisible(false);
		break;
	}
	}

	if (m_pCollisionShapeConfig->is_child)
	{
		m_pOriginLabel->SetVisible(true);
		m_pOriginX->SetVisible(true);
		m_pOriginY->SetVisible(true);
		m_pOriginZ->SetVisible(true);

		m_pAnglesLabel->SetVisible(true);
		m_pAnglesX->SetVisible(true);
		m_pAnglesY->SetVisible(true);
		m_pAnglesZ->SetVisible(true);
	}
	else
	{
		m_pOriginLabel->SetVisible(false);
		m_pOriginX->SetVisible(false);
		m_pOriginY->SetVisible(false);
		m_pOriginZ->SetVisible(false);

		m_pAnglesLabel->SetVisible(false);
		m_pAnglesX->SetVisible(false);
		m_pAnglesY->SetVisible(false);
		m_pAnglesZ->SetVisible(false);
	}

}

//RigidBody Editor

CRigidBodyEditDialog::CRigidBodyEditDialog(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pRigidBodyConfig(pRigidBodyConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_RigidBodyEditor", false);

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(560), vgui::scheme()->GetProportionalScaledValue(560));
	SetSize(vgui::scheme()->GetProportionalScaledValue(560), vgui::scheme()->GetProportionalScaledValue(560));

	m_pName = new vgui::TextEntry(this, "Name");
	m_pDebugDrawLevel = new vgui::TextEntry(this, "DebugDrawLevel");
	m_pBone = new vgui::ComboBox(this, "Bone", 0, false);
	m_pShape = new vgui::ComboBox(this, "Shape", 0, false);
	m_pOriginX = new vgui::TextEntry(this, "OriginX");
	m_pOriginY = new vgui::TextEntry(this, "OriginY");
	m_pOriginZ = new vgui::TextEntry(this, "OriginZ");
	m_pAnglesX = new vgui::TextEntry(this, "AnglesX");
	m_pAnglesY = new vgui::TextEntry(this, "AnglesY");
	m_pAnglesZ = new vgui::TextEntry(this, "AnglesZ");
	m_pMass = new vgui::TextEntry(this, "Mass");
	m_pDensity = new vgui::TextEntry(this, "Density");
	m_pLinearFriction = new vgui::TextEntry(this, "LinearFriction");
	m_pRollingFriction = new vgui::TextEntry(this, "RollingFriction");
	m_pRestitution = new vgui::TextEntry(this, "Restitution");
	m_pCCDRadius = new vgui::TextEntry(this, "CCDRadius");
	m_pCCDThreshold = new vgui::TextEntry(this, "CCDThreshold");
	m_pLinearSleepingThreshold = new vgui::TextEntry(this, "LinearSleepingThreshold");
	m_pAngularSleepingThreshold = new vgui::TextEntry(this, "AngularSleepingThreshold");
	m_pAlwaysDynamic = new vgui::CheckButton(this, "AlwaysDynamic", "#BulletPhysics_AlwaysDynamic");
	m_pAlwaysKinematic = new vgui::CheckButton(this, "AlwaysKinematic", "#BulletPhysics_AlwaysKinematic");
	m_pAlwaysStatic = new vgui::CheckButton(this, "AlwaysStatic", "#BulletPhysics_AlwaysStatic");
	m_pNoCollisionToWorld = new vgui::CheckButton(this, "NoCollisionToWorld", "#BulletPhysics_NoCollisionToWorld");
	m_pNoCollisionToStaticObject = new vgui::CheckButton(this, "NoCollisionToStaticObject", "#BulletPhysics_NoCollisionToStaticObject");
	m_pNoCollisionToDynamicObject = new vgui::CheckButton(this, "NoCollisionToDynamicObject", "#BulletPhysics_NoCollisionToDynamicObject");
	m_pNoCollisionToRagdollObject = new vgui::CheckButton(this, "NoCollisionToRagdollObject", "#BulletPhysics_NoCollisionToRagdollObject");

	vgui::HFont hFallbackFont = vgui::scheme()->GetIScheme(GetScheme())->GetFont("DefaultVerySmallFallBack", false);

	if (vgui::INVALID_FONT != hFallbackFont)
	{
		m_pBone->SetUseFallbackFont(true, hFallbackFont);
		m_pShape->SetUseFallbackFont(true, hFallbackFont);
	}

	LoadAvailableBonesIntoControls();

	LoadAvailableShapesIntoControls();

	LoadControlSettings("bulletphysics/RigidBodyEditDialog.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

void CRigidBodyEditDialog::Activate(void)
{
	BaseClass::Activate();

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CRigidBodyEditDialog::OnResetData()
{
	LoadConfigIntoControls();
}

void CRigidBodyEditDialog::OnRefreshCollisionShape()
{
	LoadShapeIntoControls(m_pRigidBodyConfig->collisionShape);
}

void CRigidBodyEditDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		SaveConfigFromControls();
		PostActionSignal(new KeyValues("RefreshRigidBody", "configId", m_pRigidBodyConfig->configId));
		ClientPhysicManager()->RemovePhysicObjectEx(m_physicObjectId);
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		SaveConfigFromControls();
		PostActionSignal(new KeyValues("RefreshRigidBody", "configId", m_pRigidBodyConfig->configId));
		ClientPhysicManager()->RemovePhysicObjectEx(m_physicObjectId);
		return;
	}
	else if (!strcmp(command, "EditCollisionShape"))
	{
		OnEditCollisionShape();
		return;
	}

	BaseClass::OnCommand(command);
}

CRigidBodyEditDialog::~CRigidBodyEditDialog()
{

}

void CRigidBodyEditDialog::OnEditCollisionShape()
{
	if (!m_pRigidBodyConfig->collisionShape)
	{
		m_pRigidBodyConfig->collisionShape = std::make_shared<CClientCollisionShapeConfig>();

		ClientPhysicManager()->AddPhysicConfig(m_pRigidBodyConfig->collisionShape->configId, m_pRigidBodyConfig->collisionShape);
	}

	auto dialog = new CCollisionShapeEditDialog(this, "CollisionShapeEditDialog", m_physicObjectId, m_pRigidBodyConfig->collisionShape);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
}

void CRigidBodyEditDialog::LoadAvailableBonesIntoControls()
{
	if(1)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("boneindex", -1);

		m_pBone->AddItem("--", kv);

		kv->deleteThis();
	}

	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(m_physicObjectId);

	auto model = EngineGetModelByIndex(modelindex);

	if (model && model->type == mod_studio)
	{
		auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(model);

		if (studiohdr)
		{
			for (int i = 0; i < studiohdr->numbones; ++i)
			{
				auto kv = new KeyValues("UserData");

				kv->SetString("name", UTIL_GetBoneRawName(studiohdr, i));
				kv->SetInt("boneindex", i);

				auto bone = UTIL_GetFormattedBoneNameEx(studiohdr, i);

				m_pBone->AddItem(bone.c_str(), kv);

				kv->deleteThis();
			}
		}
	}
}

void CRigidBodyEditDialog::LoadBoneIntoControls(int boneindex)
{
	for (int i = 0; i < m_pBone->GetItemCount(); ++i)
	{
		KeyValues* kv = m_pBone->GetItemUserData(i);

		if (kv && m_pRigidBodyConfig->boneindex == kv->GetInt("boneindex", -1))
		{
			m_pBone->ActivateItemByRow(i);
			return;
		}
	}

	m_pBone->ActivateItemByRow(0);
}

void CRigidBodyEditDialog::LoadAvailableShapesIntoControls()
{
	for (int i = 0; i < PhysicShape_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("type", i);

		m_pShape->AddItem(UTIL_GetCollisionShapeTypeLocalizationToken(i), kv);

		kv->deleteThis();
	}
}

void CRigidBodyEditDialog::LoadShapeIntoControls(const CClientCollisionShapeConfigSharedPtr& collisionShape)
{
	if (collisionShape)
	{
		for (int i = 0; i < m_pShape->GetItemCount(); ++i)
		{
			KeyValues* kv = m_pShape->GetItemUserData(i);

			if (kv && collisionShape->type == kv->GetInt("type", PhysicShape_None))
			{
				m_pShape->ActivateItemByRow(i);
				return;
			}
		}
	}
	m_pShape->ActivateItemByRow(0);
}

void CRigidBodyEditDialog::LoadConfigIntoControls()
{
	m_pName->SetText(m_pRigidBodyConfig->name.c_str());

	LoadBoneIntoControls(m_pRigidBodyConfig->boneindex);

	LoadShapeIntoControls(m_pRigidBodyConfig->collisionShape);

#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pRigidBodyConfig->from); m_p##to->SetText(str##to.c_str());}

	LOAD_INTO_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel);
	LOAD_INTO_TEXT_ENTRY(origin[0], OriginX);
	LOAD_INTO_TEXT_ENTRY(origin[1], OriginY);
	LOAD_INTO_TEXT_ENTRY(origin[2], OriginZ);
	LOAD_INTO_TEXT_ENTRY(angles[0], AnglesX);
	LOAD_INTO_TEXT_ENTRY(angles[1], AnglesY);
	LOAD_INTO_TEXT_ENTRY(angles[2], AnglesZ);
	LOAD_INTO_TEXT_ENTRY(mass, Mass);
	LOAD_INTO_TEXT_ENTRY(density, Density);
	LOAD_INTO_TEXT_ENTRY(linearFriction, LinearFriction);
	LOAD_INTO_TEXT_ENTRY(rollingFriction, RollingFriction);
	LOAD_INTO_TEXT_ENTRY(restitution, Restitution);
	LOAD_INTO_TEXT_ENTRY(ccdRadius, CCDRadius);
	LOAD_INTO_TEXT_ENTRY(ccdThreshold, CCDThreshold);
	LOAD_INTO_TEXT_ENTRY(linearSleepingThreshold, LinearSleepingThreshold);
	LOAD_INTO_TEXT_ENTRY(angularSleepingThreshold, AngularSleepingThreshold);
#undef LOAD_INTO_TEXT_ENTRY

#define LOAD_INTO_CHECK_BUTTON(from, to) m_p##to->SetSelected((m_pRigidBodyConfig->from & PhysicRigidBodyFlag_##to) ? true : false);
	LOAD_INTO_CHECK_BUTTON(flags, AlwaysDynamic);
	LOAD_INTO_CHECK_BUTTON(flags, AlwaysKinematic);
	LOAD_INTO_CHECK_BUTTON(flags, AlwaysStatic);
	LOAD_INTO_CHECK_BUTTON(flags, NoCollisionToWorld);
	LOAD_INTO_CHECK_BUTTON(flags, NoCollisionToStaticObject);
	LOAD_INTO_CHECK_BUTTON(flags, NoCollisionToDynamicObject);
	LOAD_INTO_CHECK_BUTTON(flags, NoCollisionToRagdollObject);
#undef LOAD_INTO_CHECK_BUTTON
}

void CRigidBodyEditDialog::SaveConfigFromControls()
{
	char szText[256];

#define SAVE_FROM_TEXT_ENTRY(to, from, processor) {m_p##from->GetText(szText, sizeof(szText)); m_pRigidBodyConfig->to = processor(szText);}

	SAVE_FROM_TEXT_ENTRY(name, Name, std::string);

	SAVE_FROM_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel, atoi);

	m_pRigidBodyConfig->boneindex = GetCurrentSelectedBoneIndex();

	SAVE_FROM_TEXT_ENTRY(origin[0], OriginX, atof);
	SAVE_FROM_TEXT_ENTRY(origin[1], OriginY, atof);
	SAVE_FROM_TEXT_ENTRY(origin[2], OriginZ, atof);
	SAVE_FROM_TEXT_ENTRY(angles[0], AnglesX, atof);
	SAVE_FROM_TEXT_ENTRY(angles[1], AnglesY, atof);
	SAVE_FROM_TEXT_ENTRY(angles[2], AnglesZ, atof);
	SAVE_FROM_TEXT_ENTRY(mass, Mass, atof);
	SAVE_FROM_TEXT_ENTRY(density, Density, atof);
	SAVE_FROM_TEXT_ENTRY(linearFriction, LinearFriction, atof);
	SAVE_FROM_TEXT_ENTRY(rollingFriction, RollingFriction, atof);
	SAVE_FROM_TEXT_ENTRY(restitution, Restitution, atof);
	SAVE_FROM_TEXT_ENTRY(ccdRadius, CCDRadius, atof);
	SAVE_FROM_TEXT_ENTRY(ccdThreshold, CCDThreshold, atof);
	SAVE_FROM_TEXT_ENTRY(linearSleepingThreshold, LinearSleepingThreshold, atof);
	SAVE_FROM_TEXT_ENTRY(angularSleepingThreshold, AngularSleepingThreshold, atof);
#undef SAVE_FROM_TEXT_ENTRY

#define SAVE_FROM_CHECK_BUTTON(to, from) if (m_p##from->IsSelected()) { m_pRigidBodyConfig->to |= PhysicRigidBodyFlag_##from; } else { m_pRigidBodyConfig->to &= ~PhysicRigidBodyFlag_##from; }
	SAVE_FROM_CHECK_BUTTON(flags, AlwaysDynamic);
	SAVE_FROM_CHECK_BUTTON(flags, AlwaysKinematic);
	SAVE_FROM_CHECK_BUTTON(flags, AlwaysStatic);
	SAVE_FROM_CHECK_BUTTON(flags, NoCollisionToWorld);
	SAVE_FROM_CHECK_BUTTON(flags, NoCollisionToStaticObject);
	SAVE_FROM_CHECK_BUTTON(flags, NoCollisionToDynamicObject);
	SAVE_FROM_CHECK_BUTTON(flags, NoCollisionToRagdollObject);
#undef SAVE_FROM_CHECK_BUTTON
}

int CRigidBodyEditDialog::GetCurrentSelectedBoneIndex()
{
	int boneindex = -1;

	auto pBoneKV = m_pBone->GetActiveItemUserData();

	if (pBoneKV)
	{
		boneindex = pBoneKV->GetInt("boneindex", -1);
	}

	return boneindex;
}

//Physic Editor

CPhysicEditorDialog::CPhysicEditorDialog(vgui::Panel* parent, const char *name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicConfig(pPhysicConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_PhysicEditor", false);

	m_pRigidBodyPage = new CRigidBodyPage(this, "RigidBodyPage", m_physicObjectId, pPhysicConfig);
	m_pRigidBodyPage->MakeReadyForUse();

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(640), vgui::scheme()->GetProportionalScaledValue(384));
	SetSize(vgui::scheme()->GetProportionalScaledValue(640), vgui::scheme()->GetProportionalScaledValue(384));

	m_pTabPanel = new vgui::PropertySheet(this, "PhysicTabs");
	m_pTabPanel->SetTabWidth(vgui::scheme()->GetProportionalScaledValue(72));
	m_pTabPanel->AddPage(m_pRigidBodyPage, "#BulletPhysics_RigidBody");
	m_pTabPanel->AddActionSignalTarget(this);

	LoadControlSettings("bulletphysics/PhysicEditorDialog.res", "GAME");

	m_pTabPanel->SetActivePage(m_pRigidBodyPage);

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CPhysicEditorDialog::~CPhysicEditorDialog()
{

}