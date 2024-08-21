#include "PhysicUTIL.h"
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

const char* UTIL_GetPhysicObjectConfigTypeName(int type)
{
	const char* c_names[] = { "None", "StaticObject", "DynamicObject", "RagdollObject" };

	if (type >= 0 && type < _ARRAYSIZE(c_names))
	{
		return c_names[type];
	}

	return "Unknown";
}

const char* UTIL_GetConstraintTypeName(int type)
{
	const char* c_names[] = { "None", "ConeTwist", "Hinge", "Point", "Slider", "Dof6", "Dof6Spring", "Fixed" };

	if (type >= 0 && type < _ARRAYSIZE(c_names))
	{
		return c_names[type];
	}

	return "Unknown";
}

const char* UTIL_GetPhysicActionTypeName(int type)
{
	const char* c_names[] = { "None", "BarnacleDragForce", "BarnacleChewForce", "BarnacleConstraintLimitAdjustment" };

	if (type >= 0 && type < _ARRAYSIZE(c_names))
	{
		return c_names[type];
	}

	return "Unknown";
}

const char* UTIL_GetCollisionShapeTypeName(int type)
{
	const char* c_names[] = { "None", "Box", "Sphere", "Capsule", "Cylinder", "MultiSphere", "TriangleMesh", "Compound" };

	if (type >= 0 && type < _ARRAYSIZE(c_names))
	{
		return c_names[type];
	}

	return "Unknown";
}

int UTIL_GetCollisionTypeFromTypeName(const char* name)
{
	int type = PhysicShape_None;

	if (!strcmp(name, "Box"))
	{
		type = PhysicShape_Box;
	}
	else if (!strcmp(name, "Sphere"))
	{
		type = PhysicShape_Sphere;
	}
	else if (!strcmp(name, "Capsule"))
	{
		type = PhysicShape_Capsule;
	}
	else if (!strcmp(name, "Cylinder"))
	{
		type = PhysicShape_Cylinder;
	}
	else if (!strcmp(name, "MultiSphere"))
	{
		type = PhysicShape_MultiSphere;
	}
	else if (!strcmp(name, "TriangleMesh"))
	{
		type = PhysicShape_TriangleMesh;
	}
	else if (!strcmp(name, "Compound"))
	{
		type = PhysicShape_Compound;
	}

	return type;
}

int UTIL_GetConstraintTypeFromTypeName(const char* name)
{
	int type = PhysicConstraint_None;

	if (!strcmp(name, "ConeTwist"))
	{
		type = PhysicConstraint_ConeTwist;
	}
	else if (!strcmp(name, "Hinge"))
	{
		type = PhysicConstraint_Hinge;
	}
	else if (!strcmp(name, "Point"))
	{
		type = PhysicConstraint_Point;
	}
	else if (!strcmp(name, "Slider"))
	{
		type = PhysicConstraint_Slider;
	}
	else if (!strcmp(name, "Dof6"))
	{
		type = PhysicConstraint_Dof6;
	}
	else if (!strcmp(name, "Dof6Spring"))
	{
		type = PhysicConstraint_Dof6Spring;
	}
	else if (!strcmp(name, "Fixed"))
	{
		type = PhysicConstraint_Fixed;
	}

	return type;
}

int UTIL_GetPhysicActionTypeFromTypeName(const char* name)
{
	int type = PhysicAction_None;

	if (!strcmp(name, "BarnacleDragForce"))
	{
		type = PhysicAction_BarnacleDragForce;
	}
	else if (!strcmp(name, "BarnacleChewForce"))
	{
		type = PhysicAction_BarnacleChewForce;
	}
	else if (!strcmp(name, "BarnacleConstraintLimitAdjustment"))
	{
		type = PhysicAction_BarnacleConstraintLimitAdjustment;
	}

	return type;
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

bool UTIL_RemoveRigidBodyFromPhysicObjectConfig(CClientPhysicObjectConfig * pPhysicConfig, int rigidBodyConfigId)
{
	for (auto itor = pPhysicConfig->RigidBodyConfigs.begin(); itor != pPhysicConfig->RigidBodyConfigs.end(); )
	{
		const auto& p = (*itor);

		if (p->configId == rigidBodyConfigId)
		{
			itor = pPhysicConfig->RigidBodyConfigs.erase(itor);
			return true;
		}

		itor++;
	}

	return false;
}