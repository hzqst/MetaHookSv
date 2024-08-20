#include "ClientPhysicManager.h"
#include "PhysicGUICommon.h"
#include "exportfuncs.h"
#include "Controls.h"

#include <sstream>
#include <format>

const char* VGUI2Token_CollisionShape[] = { "#BulletPhysics_None", "#BulletPhysics_Box", "#BulletPhysics_Sphere", "#BulletPhysics_Capsule", "#BulletPhysics_Cylinder", "#BulletPhysics_MultiSphere", "#BulletPhysics_TriangleMesh", "#BulletPhysics_Compound" };

const char* UTIL_GetCollisionShapeTypeLocalizationToken(int type)
{
	if (type >= 0 && type < _ARRAYSIZE(VGUI2Token_CollisionShape))
	{
		return VGUI2Token_CollisionShape[type];
	}

	return "#BulletPhysics_None";
}

std::wstring UTIL_GetCollisionShapeTypeLocalizedName(int type)
{
	return vgui::localize()->Find(UTIL_GetCollisionShapeTypeLocalizationToken(type));
}

std::wstring UTIL_GetFormattedRigidBodyFlags(int flags)
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

std::string UTIL_GetFormattedBoneNameEx(studiohdr_t* studiohdr, int boneindex)
{
	if (!(boneindex >= 0 && boneindex < studiohdr->numbones))
	{
		return "--";
	}

	auto pbone = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

	std::string name = pbone[boneindex].name;

	return std::format("#{0} ({1})", boneindex, name);
}

const char* UTIL_GetBoneRawName(studiohdr_t* studiohdr, int boneindex)
{
	if (!(boneindex >= 0 && boneindex < studiohdr->numbones))
	{
		return "--";
	}

	auto pbone = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

	return pbone[boneindex].name;
}

std::string UTIL_GetFormattedBoneName(int modelindex, int boneindex)
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

	return UTIL_GetFormattedBoneNameEx(studiohdr, boneindex);
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

std::shared_ptr<CClientConstraintConfig> UTIL_GetConstraintConfigFromConfigId(int configId)
{
	auto pPhysicConfig = ClientPhysicManager()->GetPhysicConfig(configId);

	auto pLockedPhysicConfig = pPhysicConfig.lock();

	if (!pLockedPhysicConfig)
		return nullptr;

	if (pLockedPhysicConfig->configType != PhysicConfigType_Constraint)
		return nullptr;

	std::shared_ptr<CClientConstraintConfig> pConstraintConfig = std::static_pointer_cast<CClientConstraintConfig>(pLockedPhysicConfig);

	return pConstraintConfig;
}

std::shared_ptr<CClientPhysicObjectConfig> UTIL_GetPhysicObjectConfigFromConfigId(int configId)
{
	auto pPhysicConfig = ClientPhysicManager()->GetPhysicConfig(configId);

	auto pLockedPhysicConfig = pPhysicConfig.lock();

	if (!pLockedPhysicConfig)
		return nullptr;

	if (pLockedPhysicConfig->configType != PhysicConfigType_PhysicObject)
		return nullptr;

	std::shared_ptr<CClientPhysicObjectConfig> pPhysicObjectConfig = std::static_pointer_cast<CClientPhysicObjectConfig>(pLockedPhysicConfig);

	return pPhysicObjectConfig;
}