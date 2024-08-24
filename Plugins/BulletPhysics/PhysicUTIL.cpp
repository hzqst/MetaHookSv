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

bool UTIL_RemoveRigidBodyFromPhysicObjectConfig(CClientPhysicObjectConfig * pPhysicObjectConfig, int rigidBodyConfigId)
{
	for (auto itor = pPhysicObjectConfig->RigidBodyConfigs.begin(); itor != pPhysicObjectConfig->RigidBodyConfigs.end(); )
	{
		const auto& p = (*itor);

		if (p->configId == rigidBodyConfigId)
		{
			itor = pPhysicObjectConfig->RigidBodyConfigs.erase(itor);

			pPhysicObjectConfig->configModified = true;

			return true;
		}

		itor++;
	}

	return false;
}

std::shared_ptr<CClientCollisionShapeConfig> UTIL_CloneCollisionShapeConfig(const CClientCollisionShapeConfig* pOldShape)
{
	auto pCloneShape = std::make_shared<CClientCollisionShapeConfig>();
	pCloneShape->configModified = true;
	pCloneShape->configType = pOldShape->configType;
	pCloneShape->type = pOldShape->type;
	pCloneShape->direction = pOldShape->direction;
	VectorCopy(pOldShape->size, pCloneShape->size);
	pCloneShape->is_child = pOldShape->is_child;
	VectorCopy(pOldShape->origin, pCloneShape->origin);
	VectorCopy(pOldShape->angles, pCloneShape->angles);
	//pCloneShape->multispheres = pOldShape->multispheres;
	pCloneShape->resourcePath = pOldShape->resourcePath;

	for (auto& oldChildShape : pOldShape->compoundShapes) {
		auto pClonedShape = UTIL_CloneCollisionShapeConfig(oldChildShape.get());
		pCloneShape->compoundShapes.push_back(pClonedShape);
	}

	return pCloneShape;
}

std::shared_ptr<CClientRigidBodyConfig> UTIL_CloneRigidBodyConfig(const CClientRigidBodyConfig* pOldConfig)
{
	auto pCloneConfig = std::make_shared<CClientRigidBodyConfig>();
	pCloneConfig->configModified = true;
	pCloneConfig->name = pOldConfig->name;
	pCloneConfig->configType = pOldConfig->configType;
	pCloneConfig->flags = pOldConfig->flags;
	pCloneConfig->debugDrawLevel = pOldConfig->debugDrawLevel;
	pCloneConfig->boneindex = pOldConfig->boneindex;
	VectorCopy(pOldConfig->origin, pCloneConfig->origin);
	VectorCopy(pOldConfig->angles, pCloneConfig->angles);
	pCloneConfig->isLegacyConfig = pOldConfig->isLegacyConfig;
	pCloneConfig->pboneindex = pOldConfig->pboneindex;
	pCloneConfig->pboneoffset = pOldConfig->pboneoffset;
	VectorCopy(pOldConfig->forward, pCloneConfig->forward);
	pCloneConfig->mass = pOldConfig->mass;
	pCloneConfig->density = pOldConfig->density;
	pCloneConfig->linearFriction = pOldConfig->linearFriction;
	pCloneConfig->rollingFriction = pOldConfig->rollingFriction;
	pCloneConfig->restitution = pOldConfig->restitution;
	pCloneConfig->ccdRadius = pOldConfig->ccdRadius;
	pCloneConfig->ccdThreshold = pOldConfig->ccdThreshold;
	pCloneConfig->linearSleepingThreshold = pOldConfig->linearSleepingThreshold;
	pCloneConfig->angularSleepingThreshold = pOldConfig->angularSleepingThreshold;

	if (pOldConfig->collisionShape) {
		pCloneConfig->collisionShape = UTIL_CloneCollisionShapeConfig(pOldConfig->collisionShape.get());
	}

	return pCloneConfig;
}

std::string UTIL_FormatAbsoluteModelName(model_t* mod)
{
	if (mod->type == mod_brush)
	{
		if (mod != r_worldmodel)
		{
			return std::format("{0}/{1}", r_worldmodel->name, mod->name);
		}
		else
		{
			return r_worldmodel->name;
		}
	}

	return mod->name;
}

bool UTIL_IsCollisionShapeConfigModified(const CClientCollisionShapeConfig* pCollisionShapeConfig)
{
	if (pCollisionShapeConfig->configModified)
		return true;

	for (const auto& pSubShapeConfig : pCollisionShapeConfig->compoundShapes)
	{
		if (UTIL_IsCollisionShapeConfigModified(pSubShapeConfig.get()))
		{
			return true;
		}
	}

	return false;
}

bool UTIL_IsPhysicObjectConfigModified(const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	if (pPhysicObjectConfig->configModified)
		return true;

	for (const auto& pRigidBodyConfig : pPhysicObjectConfig->RigidBodyConfigs)
	{
		if (pRigidBodyConfig->configModified)
			return true;

		const auto &pCollisionShapeConfig = pRigidBodyConfig->collisionShape;

		if (pCollisionShapeConfig)
		{
			if(UTIL_IsCollisionShapeConfigModified(pCollisionShapeConfig.get()))
				return true;
		}
	}

	if (pPhysicObjectConfig->type == PhysicObjectType_DynamicObject)
	{
		const auto pDynamicObjectConfig = (const CClientDynamicObjectConfig*)pPhysicObjectConfig;

		for (const auto& pConstraintConfig : pDynamicObjectConfig->ConstraintConfigs)
		{
			if (pConstraintConfig->configModified)
				return true;
		}
	}
	else if (pPhysicObjectConfig->type == PhysicObjectType_RagdollObject)
	{
		const auto pRagdollObjectConfig = (const CClientRagdollObjectConfig*)pPhysicObjectConfig;

		for (const auto& pConstraintConfig : pRagdollObjectConfig->ConstraintConfigs)
		{
			if (pConstraintConfig->configModified)
				return true;
		}

		for (const auto& pFloaterConfig : pRagdollObjectConfig->FloaterConfigs)
		{
			if (pFloaterConfig->configModified)
				return true;
		}

		for (const auto& pActionConfig : pRagdollObjectConfig->BarnacleControlConfig.ActionConfigs)
		{
			if (pActionConfig->configModified)
				return true;
		}

		for (const auto& pConstraintConfig : pRagdollObjectConfig->BarnacleControlConfig.ConstraintConfigs)
		{
			if (pConstraintConfig->configModified)
				return true;
		}
	}

	return false;
}