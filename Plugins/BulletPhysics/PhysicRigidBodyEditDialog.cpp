#include "PhysicRigidBodyEditDialog.h"
#include "PhysicCollisionShapeEditDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

//RigidBody Editor

CPhysicRigidBodyEditDialog::CPhysicRigidBodyEditDialog(vgui::Panel* parent, const char* name,
	uint64 physicObjectId,
	const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
	const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig) :
	BaseClass(parent, name),
	m_physicObjectId(physicObjectId),
	m_pPhysicObjectConfig(pPhysicObjectConfig),
	m_pRigidBodyConfig(pRigidBodyConfig)
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

	LoadControlSettings("bulletphysics/PhysicRigidBodyEditDialog.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

void CPhysicRigidBodyEditDialog::Activate(void)
{
	BaseClass::Activate();

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicRigidBodyEditDialog::OnResetData()
{
	LoadConfigIntoControls();
}

void CPhysicRigidBodyEditDialog::OnRefreshCollisionShape(int configId)
{
	if (!m_pRigidBodyConfig->collisionShape)
		return;

	if (m_pRigidBodyConfig->collisionShape->configId != configId)
		return;

	LoadShapeIntoControls(m_pRigidBodyConfig->collisionShape);
	PostActionSignal(new KeyValues("RefreshRigidBody", "configId", m_pRigidBodyConfig->configId));
}

void CPhysicRigidBodyEditDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		SaveConfigFromControls();
		//Tell RigidBodyList to refresh
		PostActionSignal(new KeyValues("RefreshRigidBody", "configId", m_pRigidBodyConfig->configId));
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		SaveConfigFromControls();
		//Tell RigidBodyList to refresh
		PostActionSignal(new KeyValues("RefreshRigidBody", "configId", m_pRigidBodyConfig->configId));
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		return;
	}
	else if (!strcmp(command, "EditCollisionShape"))
	{
		OnEditCollisionShape();
		return;
	}

	BaseClass::OnCommand(command);
}

CPhysicRigidBodyEditDialog::~CPhysicRigidBodyEditDialog()
{

}

void CPhysicRigidBodyEditDialog::OnEditCollisionShape()
{
	if (!m_pRigidBodyConfig->collisionShape)
	{
		m_pRigidBodyConfig->collisionShape = std::make_shared<CClientCollisionShapeConfig>();

		m_pRigidBodyConfig->collisionShape->configModified = true;

		m_pRigidBodyConfig->configModified = true;

		ClientPhysicManager()->AddPhysicConfig(m_pRigidBodyConfig->collisionShape->configId, m_pRigidBodyConfig->collisionShape);
	}

	auto dialog = new CPhysicCollisionShapeEditDialog(this, "PhysicCollisionShapeEditDialog", m_physicObjectId, m_pPhysicObjectConfig, m_pRigidBodyConfig, m_pRigidBodyConfig->collisionShape);
	dialog->AddActionSignalTarget(this);
	dialog->DoModal();
}

void CPhysicRigidBodyEditDialog::LoadAvailableBonesIntoControls()
{
	//-1 means invalid bone
	if (1)
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

				auto formattedName = UTIL_GetFormattedBoneNameEx(studiohdr, i);

				m_pBone->AddItem(formattedName.c_str(), kv);

				kv->deleteThis();
			}
		}
	}
}

void CPhysicRigidBodyEditDialog::LoadBoneIntoControls(int boneindex)
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

void CPhysicRigidBodyEditDialog::LoadAvailableShapesIntoControls()
{
	for (int i = 0; i < PhysicShape_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("type", i);

		m_pShape->AddItem(UTIL_GetCollisionShapeTypeLocalizationToken(i), kv);

		kv->deleteThis();
	}
}

void CPhysicRigidBodyEditDialog::LoadShapeIntoControls(const CClientCollisionShapeConfigSharedPtr& collisionShape)
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

void CPhysicRigidBodyEditDialog::LoadConfigIntoControls()
{
	LoadBoneIntoControls(m_pRigidBodyConfig->boneindex);

	LoadShapeIntoControls(m_pRigidBodyConfig->collisionShape);

#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pRigidBodyConfig->from); m_p##to->SetText(str##to.c_str());}

	LOAD_INTO_TEXT_ENTRY(name, Name);
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

void CPhysicRigidBodyEditDialog::SaveConfigFromControls()
{
	m_pRigidBodyConfig->boneindex = GetCurrentSelectedBoneIndex();

	char szText[256];

#define SAVE_FROM_TEXT_ENTRY(to, from, processor) {m_p##from->GetText(szText, sizeof(szText)); m_pRigidBodyConfig->to = processor(szText);}

	SAVE_FROM_TEXT_ENTRY(name, Name, std::string);
	SAVE_FROM_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel, atoi);
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

	//Marked as modified
	m_pRigidBodyConfig->configModified = true;
}

int CPhysicRigidBodyEditDialog::GetCurrentSelectedBoneIndex()
{
	int boneindex = -1;

	auto pBoneKV = m_pBone->GetActiveItemUserData();

	if (pBoneKV)
	{
		boneindex = pBoneKV->GetInt("boneindex", -1);
	}

	return boneindex;
}