#pragma once

#include "ClientPhysicManager.h"

#include <string>

enum class PhysicInspectMode
{
	Entity,
	PhysicObject,
	RigidBody,
};

enum class PhysicEditMode
{
	None,
	Move,
	Rotation
};

const char* UTIL_GetCollisionShapeTypeLocalizationToken(int type);
std::wstring UTIL_GetCollisionShapeTypeLocalizedName(int type);
std::wstring UTIL_GetFormattedRigidBodyFlags(int flags);
const char* UTIL_GetBoneRawName(studiohdr_t* studiohdr, int boneindex);
std::string UTIL_GetFormattedBoneNameEx(studiohdr_t* studiohdr, int boneindex);
std::string UTIL_GetFormattedBoneName(int modelindex, int boneindex);
std::shared_ptr<CClientRigidBodyConfig> UTIL_GetRigidConfigFromConfigId(int configId);
std::shared_ptr<CClientConstraintConfig> UTIL_GetConstraintConfigFromConfigId(int configId);
std::shared_ptr<CClientPhysicObjectConfig> UTIL_GetPhysicObjectConfigFromConfigId(int configId);
