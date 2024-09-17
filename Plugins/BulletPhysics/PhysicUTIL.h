#pragma once

#include "ClientPhysicManager.h"

#include <string>

const char* UTIL_GetPhysicObjectTypeLocalizationToken(int type);
const char* UTIL_GetConstraintTypeLocalizationToken(int type);
const char* UTIL_GetPhysicActionTypeLocalizationToken(int type);
const char* UTIL_GetRotOrderTypeLocalizationToken(int type);
const char* UTIL_GetCollisionShapeTypeLocalizationToken(int type);
std::wstring UTIL_GetCollisionShapeTypeLocalizedName(int type);
std::wstring UTIL_GetFormattedRigidBodyFlags(int flags);
std::wstring UTIL_GetFormattedConstraintFlags(int flags);
std::wstring UTIL_GetFormattedConstraintConfigAttributes(const CClientConstraintConfig* pConstraintConfig);
std::wstring UTIL_GetFormattedPhysicActionFlags(int flags);
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
std::shared_ptr<CClientPhysicActionConfig> UTIL_GetPhysicActionConfigFromConfigId(int configId);
std::shared_ptr<CClientPhysicObjectConfig> UTIL_GetPhysicObjectConfigFromConfigId(int configId);

bool UTIL_RemoveRigidBodyFromPhysicObjectConfig(CClientPhysicObjectConfig* pPhysicConfig, int rigidBodyConfigId);
bool UTIL_RemoveConstraintFromPhysicObjectConfig(CClientPhysicObjectConfig* pPhysicObjectConfig, int constraintConfigId);
bool UTIL_RemovePhysicActionFromPhysicObjectConfig(CClientPhysicObjectConfig* pPhysicObjectConfig, int physicActionConfigId);

std::shared_ptr<CClientCollisionShapeConfig> UTIL_CloneCollisionShapeConfig(const CClientCollisionShapeConfig* pOldShape);
std::shared_ptr<CClientRigidBodyConfig> UTIL_CloneRigidBodyConfig(const CClientRigidBodyConfig* pOldConfig);
std::shared_ptr<CClientConstraintConfig> UTIL_CloneConstraintConfig(const CClientConstraintConfig* pOldConfig);
std::shared_ptr<CClientPhysicActionConfig> UTIL_ClonePhysicActionConfig(const CClientPhysicActionConfig* pOldConfig);

std::string UTIL_FormatAbsoluteModelName(model_t* mod);

bool UTIL_IsCollisionShapeConfigModified(const CClientCollisionShapeConfig* pCollisionShapeConfig);
bool UTIL_IsPhysicObjectConfigModified(const CClientPhysicObjectConfig* pPhysicObjectConfig);
void UTIL_SetCollisionShapeConfigUnmodified(CClientCollisionShapeConfig* pCollisionShapeConfig);
void UTIL_SetPhysicObjectConfigUnmodified(CClientPhysicObjectConfig* pPhysicObjectConfig);

IPhysicRigidBody* UTIL_GetPhysicComponentAsRigidBody(int physicComponentId);
IPhysicConstraint* UTIL_GetPhysicComponentAsConstraint(int physicComponentId);

int UTIL_GetRigidBodyIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
int UTIL_GetRigidBodyIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, const CClientRigidBodyConfig* pRigidBodyConfig);
bool UTIL_ShiftUpRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftUpRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientRigidBodyConfig* pRigidBodyConfig);
bool UTIL_ShiftDownRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftDownRigidBodyIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientRigidBodyConfig* pRigidBodyConfig);

int UTIL_GetConstraintIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
int UTIL_GetConstraintIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, const CClientConstraintConfig* pConstraintConfig);
bool UTIL_ShiftUpConstraintIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftUpConstraintIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientConstraintConfig* pConstraintConfig);
bool UTIL_ShiftDownConstraintIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftDownConstraintIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientConstraintConfig* pConstraintConfig);

int UTIL_GetPhysicActionIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
int UTIL_GetPhysicActionIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, const CClientPhysicActionConfig* pPhysicActionConfig);
bool UTIL_ShiftUpPhysicActionIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftUpPhysicActionIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientPhysicActionConfig* pPhysicActionConfig);
bool UTIL_ShiftDownPhysicActionIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftDownPhysicActionIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientPhysicActionConfig* pPhysicActionConfig);