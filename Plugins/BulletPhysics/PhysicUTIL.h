#pragma once

#include "ClientPhysicManager.h"

#include <string>

const char* UTIL_GetPhysicObjectTypeLocalizationToken(int type);
const char* UTIL_GetCollisionShapeTypeLocalizationToken(int type);
std::wstring UTIL_GetCollisionShapeTypeLocalizedName(int type);
std::wstring UTIL_GetFormattedRigidBodyFlags(int flags);
const char* UTIL_GetBoneRawName(studiohdr_t* studiohdr, int boneindex);
std::string UTIL_GetFormattedBoneNameEx(studiohdr_t* studiohdr, int boneindex);
std::string UTIL_GetFormattedBoneName(int modelindex, int boneindex);

const char* UTIL_GetPhysicObjectConfigTypeName(int type);
const char* UTIL_GetConstraintTypeName(int type);
const char* UTIL_GetPhysicActionTypeName(int type);
const char* UTIL_GetCollisionShapeTypeName(int type);
int UTIL_GetCollisionTypeFromTypeName(const char* name);
int UTIL_GetConstraintTypeFromTypeName(const char* name);
int UTIL_GetPhysicActionTypeFromTypeName(const char* name);

std::shared_ptr<CClientRigidBodyConfig> UTIL_GetRigidConfigFromConfigId(int configId);
std::shared_ptr<CClientConstraintConfig> UTIL_GetConstraintConfigFromConfigId(int configId);
std::shared_ptr<CClientPhysicObjectConfig> UTIL_GetPhysicObjectConfigFromConfigId(int configId);

bool UTIL_RemoveRigidBodyFromPhysicObjectConfig(CClientPhysicObjectConfig* pPhysicConfig, int rigidBodyConfigId);
std::shared_ptr<CClientCollisionShapeConfig> UTIL_CloneCollisionShapeConfig(const CClientCollisionShapeConfig* pOldShape);
std::shared_ptr<CClientRigidBodyConfig> UTIL_CloneRigidBodyConfig(const CClientRigidBodyConfig* pOldConfig);
std::string UTIL_FormatAbsoluteModelName(model_t* mod);

bool UTIL_IsCollisionShapeConfigModified(const CClientCollisionShapeConfig* pCollisionShapeConfig);
bool UTIL_IsPhysicObjectConfigModified(const CClientPhysicObjectConfig* pPhysicObjectConfig);

IPhysicRigidBody* UTIL_GetPhysicComponentAsRigidBody(int physicComponentId);
IPhysicConstraint* UTIL_GetPhysicComponentAsConstraint(int physicComponentId);

int UTIL_GetRigidBodyIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
int UTIL_GetRigidBodyIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, const CClientRigidBodyConfig* pRigidBodyConfig);
bool UTIL_ShiftUpRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftUpRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientRigidBodyConfig* pRigidBodyConfig);
bool UTIL_ShiftDownRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftDownRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientRigidBodyConfig* pRigidBodyConfig);