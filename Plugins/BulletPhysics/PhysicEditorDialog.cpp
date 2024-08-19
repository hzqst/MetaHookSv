#include "PhysicEditorDialog.h"
#include <vgui_controls/Menu.h>

#include <studio.h>
#include <const.h>
#include <r_studioint.h>

#include "exportfuncs.h"

#include "ClientPhysicManager.h"

#include <format>
#include <sstream>

std::wstring UTIL_FormatShapeNameInternal(int type)
{
	const char* tokens[] = { "#BulletPhysics_None", "#BulletPhysics_Box", "#BulletPhysics_Sphere", "#BulletPhysics_Capsule", "#BulletPhysics_Cylinder", "#BulletPhysics_MultiSphere", "#BulletPhysics_TriangleMesh" };

	if (type >= 0 && type < _ARRAYSIZE(tokens))
	{
		return vgui::localize()->Find(tokens[type]);
	}

	return vgui::localize()->Find("#BulletPhysics_Unknown");
}

std::wstring UTIL_FormatShapeName(const CClientRigidBodyConfig *pRigidBodyConfig)
{
	if (pRigidBodyConfig->shapes.size() > 1)
	{
		return vgui::localize()->Find("#BulletPhysics_Compound");
	}
	else if (pRigidBodyConfig->shapes.size() == 1)
	{
		return UTIL_FormatShapeNameInternal(pRigidBodyConfig->shapes[0]->type);
	}

	return L"--";
}

std::wstring UTIL_FormatRigidBodyFlags(int flags)
{
	std::wstringstream ss;

	if (flags & PhysicRigidBodyFlag_AlwaysDynamic) {
		ss << L"(" << vgui::localize()->Find("#BulletPhysics_AlwaysDynamic") << L") ";
	}

	if (flags & PhysicRigidBodyFlag_AlwaysKinematic) {
		ss << L"(" << vgui::localize()->Find("#BulletPhysics_AlwaysKinematic") << L") ";
	}

	if (flags & PhysicRigidBodyFlag_NoCollisionToWorld) {
		ss << L"(" << vgui::localize()->Find("#BulletPhysics_NoCollisionToWorld") << L") ";
	}

	if (flags & PhysicRigidBodyFlag_NoCollisionToStaticObject) {
		ss << L"(" << vgui::localize()->Find("#BulletPhysics_NoCollisionToStaticObject") << L") ";
	}

	if (flags & PhysicRigidBodyFlag_NoCollisionToDynamicObject) {
		ss << L"(" << vgui::localize()->Find("#BulletPhysics_NoCollisionToDynamicObject") << L") ";
	}

	if (flags & PhysicRigidBodyFlag_NoCollisionToRagdollObject) {
		ss << L"(" << vgui::localize()->Find("#BulletPhysics_NoCollisionToRagdollObject") << L") ";
	}

	return ss.str();
}

std::string UTIL_FormatBoneNameEx(studiohdr_t * studiohdr, int boneindex)
{
	if (!(boneindex >= 0 && boneindex < studiohdr->numbones))
	{
		return "--";
	}

	auto pbone = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

	std::string name = pbone[boneindex].name;

	return std::format("#{0} ({1})", boneindex, name);
}

const char *UTIL_GetBoneName(studiohdr_t* studiohdr, int boneindex)
{
	if (!(boneindex >= 0 && boneindex < studiohdr->numbones))
	{
		return "--";
	}

	auto pbone = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

	return pbone[boneindex].name;
}

std::string UTIL_FormatBoneName(int modelindex, int boneindex)
{
	auto model = EngineGetModelByIndex(modelindex);

	if (!model)
	{
		return "--";
	}

	auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(model);

	if (!studiohdr)
	{
		return "--";
	}

	return UTIL_FormatBoneNameEx(studiohdr, boneindex);
}

std::shared_ptr<CClientRigidBodyConfig> UTIL_GetRigidConfigFromConfigId(int configId)
{
	auto pPhysicConfig = ClientPhysicManager()->GetPhysicConfig(configId);

	auto pLockedPhysicConfig = pPhysicConfig.lock();

	if (!pLockedPhysicConfig)
		return nullptr;

	if (pLockedPhysicConfig->configType != PhysicConfigType_RigidBody)
		return nullptr;

	std::shared_ptr<CClientRigidBodyConfig> pRigidBodyConfig = std::static_pointer_cast<CClientRigidBodyConfig>(pLockedPhysicConfig);

	return pRigidBodyConfig;
}

//RigidBody List

CRigidBodyListPanel::CRigidBodyListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{

}


CRigidBodyPage::CRigidBodyPage(vgui::Panel* parent, const char* name, int entindex, int modelindex, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicConfig) :
	BaseClass(parent, name), m_iInspectEntityIndex(entindex), m_iEngineModelIndex(modelindex), m_pPhysicConfig(pPhysicConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pRigidBodyListPanel = new CRigidBodyListPanel(this, "RigidBodyListPanel");

	m_pRigidBodyListPanel->AddColumnHeader(0, "name", "#BulletPhysics_Name", vgui::scheme()->GetProportionalScaledValue(120), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(1, "shape","#BulletPhysics_Shape", vgui::scheme()->GetProportionalScaledValue(60), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(2, "bone", "#BulletPhysics_Bone", vgui::scheme()->GetProportionalScaledValue(180), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(3, "mass", "#BulletPhysics_Mass", vgui::scheme()->GetProportionalScaledValue(60), vgui::ListPanel::COLUMN_FIXEDSIZE);
	m_pRigidBodyListPanel->AddColumnHeader(4, "flags", "#BulletPhysics_Flags", vgui::scheme()->GetProportionalScaledValue(80), vgui::ListPanel::COLUMN_RESIZEWITHWINDOW);

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

			OpenRigidBodyEditor(configId);

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
	else if (!strcmp(command, "CreateNewRigidBody"))
	{
		OnCreateNewRigidBody();
	}
	else if (!strncmp(command, "EditRigidBody|", sizeof("EditRigidBody|") - 1))
	{
		OnEditRigidBody(command + sizeof("EditRigidBody|") - 1);
	}
	else if (!strncmp(command, "DeleteRigidBody|", sizeof("DeleteRigidBody|") - 1))
	{
		OnDeleteRigidBody(command + sizeof("DeleteRigidBody|") - 1);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CRigidBodyPage::OpenRigidBodyEditor(int configId)
{
	auto pRigidBodyConfig = UTIL_GetRigidConfigFromConfigId(configId);

	if (!pRigidBodyConfig)
		return;

	auto dialog = new CRigidBodyEditDialog(this, "RigidBodyEditDialog", m_iInspectEntityIndex, m_iEngineModelIndex, pRigidBodyConfig);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
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

	auto command_EditRigidBody = std::format("EditRigidBody|{0}", configId);
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_EditRigidBody"), 1, szName);
	menu->AddMenuItem("EditRigidBody", szBuf, new KeyValues("Command", "command", command_EditRigidBody.c_str()), this);

	auto command_DeleteRigidBody = std::format("DeleteRigidBody|{0}", configId);
	vgui::localize()->ConstructString(szBuf, sizeof(szBuf), vgui::localize()->Find("#BulletPhysics_DeleteRigidBody"), 1, szName);
	menu->AddMenuItem("DeleteRigidBody", szBuf, new KeyValues("Command", "command", command_DeleteRigidBody.c_str()), this);
	
	vgui::Menu::PlaceContextMenu(this, menu);
}

void CRigidBodyPage::LoadRigidBodyAsListPanelItem(const CClientRigidBodyConfig * pRigidBodyConfig)
{
	auto kv = new KeyValues("RigidBody");

	kv->SetString("name", pRigidBodyConfig->name.c_str());

	auto shape = UTIL_FormatShapeName(pRigidBodyConfig);

	kv->SetWString("shape", shape.c_str());

	auto bone = UTIL_FormatBoneName(m_iEngineModelIndex, pRigidBodyConfig->boneindex);

	kv->SetString("bone", bone.c_str());

	kv->SetFloat("mass", pRigidBodyConfig->mass);

	auto flags = UTIL_FormatRigidBodyFlags(pRigidBodyConfig->flags);

	kv->SetWString("flags", flags.c_str());

	m_pRigidBodyListPanel->AddItem(kv, pRigidBodyConfig->configId, false, false);

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

	pRigidBodyConfig->name = std::format("NewRigidBody_{0}", pRigidBodyConfig->configId);

	m_pPhysicConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

	ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

	LoadRigidBodyAsListPanelItem(pRigidBodyConfig.get());

	ClientPhysicManager()->RemovePhysicObject(m_iInspectEntityIndex);
}

void CRigidBodyPage::OnEditRigidBody(const char* command)
{
	int configId{};

	if (1 != sscanf_s(command, "%d", &configId))
		return;

	OpenRigidBodyEditor(configId);
}

void CRigidBodyPage::OnDeleteRigidBody(const char *command)
{
	int configId{};

	if (1 != sscanf_s(command, "%d", &configId))
		return;

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

	ReloadAllRigidBodiesIntoListPanelItem();

	ClientPhysicManager()->RemovePhysicObject(m_iInspectEntityIndex);
}

//RigidBody Editor

CRigidBodyEditDialog::CRigidBodyEditDialog(vgui::Panel* parent, const char* name, int entindex, int modelindex, const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig) :
	BaseClass(parent, name), m_iInspectEntityIndex(entindex), m_iEngineModelIndex(modelindex), m_pRigidBodyConfig(pRigidBodyConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_RigidBodyEditor", false);

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(440), vgui::scheme()->GetProportionalScaledValue(384));
	SetSize(vgui::scheme()->GetProportionalScaledValue(440), vgui::scheme()->GetProportionalScaledValue(384));

	m_pName = new vgui::TextEntry(this, "Name");
	m_pBone = new vgui::ComboBox(this, "Bone", 0, false);
	m_pOriginX = new vgui::TextEntry(this, "OriginX");
	m_pOriginY = new vgui::TextEntry(this, "OriginY");
	m_pOriginZ = new vgui::TextEntry(this, "OriginZ");
	m_pAnglesX = new vgui::TextEntry(this, "AnglesX");
	m_pAnglesY = new vgui::TextEntry(this, "AnglesY");
	m_pAnglesZ = new vgui::TextEntry(this, "AnglesZ");
	m_pMass = new vgui::TextEntry(this, "Mass");
	m_pDensity = new vgui::TextEntry(this, "Density");

	vgui::HFont hFallbackFont = vgui::scheme()->GetIScheme(GetScheme())->GetFont("DefaultVerySmallFallBack", false);

	if (vgui::INVALID_FONT != hFallbackFont)
		m_pBone->SetUseFallbackFont(true, hFallbackFont);

	LoadBoneIntoControls();

	LoadConfigIntoControls();

	LoadControlSettings("bulletphysics/RigidBodyEditDialog.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

void CRigidBodyEditDialog::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	//m_pBone->GetMenu()->MakeReadyForUse();

	//m_pBone->InvalidateLayout(true);

	//m_pBone->GetMenu()->InvalidateLayout(true);
}

CRigidBodyEditDialog::~CRigidBodyEditDialog()
{

}

void CRigidBodyEditDialog::LoadBoneIntoControls()
{
	if(1)
	{
		auto kv = new KeyValues("UserData");
		kv->SetInt("boneindex", -1);

		m_pBone->AddItem("--", kv);

		kv->deleteThis();
	}

	auto model = EngineGetModelByIndex(m_iEngineModelIndex);

	if (model && model->type == mod_studio)
	{
		auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(model);

		if (studiohdr)
		{
			for (int i = 0; i < studiohdr->numbones; ++i)
			{
				auto kv = new KeyValues("UserData");

				kv->SetString("name", UTIL_GetBoneName(studiohdr, i));
				kv->SetInt("boneindex", i);

				auto bone = UTIL_FormatBoneNameEx(studiohdr, i);

				m_pBone->AddItem(bone.c_str(), kv);

				kv->deleteThis();
			}
		}
	}
}

void CRigidBodyEditDialog::LoadConfigIntoControls()
{
	m_pName->SetText(m_pRigidBodyConfig->name.c_str());

	m_pBone->ActivateItemByRow(0);

	auto model = EngineGetModelByIndex(m_iEngineModelIndex);

	if (model && model->type == mod_studio)
	{
		auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(model);

		if (studiohdr)
		{
			for (int i = 0; i < m_pBone->GetItemCount(); ++i)
			{
				KeyValues* kv = m_pBone->GetItemUserData(i);

				if (kv && m_pRigidBodyConfig->boneindex == kv->GetInt("boneindex", -1))
				{
					m_pBone->ActivateItemByRow(i);
					break;
				}
			}
		}
	}

	auto mass = std::format("{0}", m_pRigidBodyConfig->mass);
	m_pMass->SetText(mass.c_str());

	auto originX = std::format("{0}", m_pRigidBodyConfig->origin[0]);
	m_pOriginX->SetText(originX.c_str());

	auto originY = std::format("{0}", m_pRigidBodyConfig->origin[1]);
	m_pOriginY->SetText(originY.c_str());

	auto originZ = std::format("{0}", m_pRigidBodyConfig->origin[2]);
	m_pOriginZ->SetText(originZ.c_str());

	auto anglesX = std::format("{0}", m_pRigidBodyConfig->angles[0]);
	m_pAnglesX->SetText(anglesX.c_str());

	auto anglesY = std::format("{0}", m_pRigidBodyConfig->angles[1]);
	m_pAnglesY->SetText(anglesY.c_str());

	auto anglesZ = std::format("{0}", m_pRigidBodyConfig->angles[2]);
	m_pAnglesZ->SetText(anglesZ.c_str());
}

//Physic Editor

CPhysicEditorDialog::CPhysicEditorDialog(vgui::Panel* parent, const char *name, int entindex, int modelindex, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicConfig) :
	BaseClass(parent, name), m_iInspectEntityIndex(entindex), m_iEngineModelIndex(modelindex), m_pPhysicConfig(pPhysicConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_PhysicEditor", false);

	m_pRigidBodyPage = new CRigidBodyPage(this, "RigidBodyPage", entindex, modelindex, pPhysicConfig);

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