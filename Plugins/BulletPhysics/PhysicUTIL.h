#pragma once

#include "ClientPhysicManager.h"

#include <string>

const char* UTIL_GetPhysicObjectTypeLocalizationToken(int type);
std::wstring UTIL_GetPhysicObjectTypeLocalizedName(int type);

const char* UTIL_GetConstraintTypeLocalizationToken(int type);
std::wstring UTIL_GetConstraintTypeLocalizedName(int type);

const char* UTIL_GetRotOrderTypeLocalizationToken(int type);
std::wstring UTIL_GetRotOrderTypeLocalizedName(int type);

const char* UTIL_GetCollisionShapeTypeLocalizationToken(int type);
std::wstring UTIL_GetCollisionShapeTypeLocalizedName(int type);

const char* UTIL_GetPhysicBehaviorTypeLocalizationToken(int type);
std::wstring UTIL_GetPhysicBehaviorTypeLocalizedName(int type);

const char* UTIL_GetActivityTypeLocalizationToken(StudioAnimActivityType type);
std::wstring UTIL_GetActivityTypeLocalizedName(StudioAnimActivityType type);

std::wstring UTIL_GetFormattedRigidBodyFlags(int flags);
std::wstring UTIL_GetFormattedConstraintFlags(int flags);
std::wstring UTIL_GetFormattedConstraintConfigAttributes(const CClientConstraintConfig* pConstraintConfig);
std::wstring UTIL_GetFormattedPhysicBehaviorFlags(int flags);
std::wstring UTIL_GetFormattedAnimControlFlags(int flags);

const char* UTIL_GetSequenceRawName(studiohdr_t* studiohdr, int sequence);
std::string UTIL_GetFormattedSequenceNameEx(studiohdr_t* studiohdr, int sequence);
std::string UTIL_GetFormattedSequenceName(int modelindex, int sequence);

const char* UTIL_GetBoneRawName(studiohdr_t* studiohdr, int boneindex);
std::string UTIL_GetFormattedBoneNameEx(studiohdr_t* studiohdr, int boneindex);
std::string UTIL_GetFormattedBoneName(int modelindex, int boneindex);
std::string UTIL_GetAbsoluteModelName(model_t* mod);

const char* UTIL_GetPhysicObjectConfigTypeName(int type);
const char* UTIL_GetConstraintTypeName(int type);
const char* UTIL_GetPhysicBehaviorTypeName(int type);
const char* UTIL_GetCollisionShapeTypeName(int type);
int UTIL_GetCollisionTypeFromTypeName(const char* name);
int UTIL_GetConstraintTypeFromTypeName(const char* name);
int UTIL_GetPhysicBehaviorTypeFromTypeName(const char* name);

std::shared_ptr<CClientRigidBodyConfig> UTIL_GetRigidConfigFromConfigId(int configId);
std::shared_ptr<CClientConstraintConfig> UTIL_GetConstraintConfigFromConfigId(int configId);
std::shared_ptr<CClientPhysicBehaviorConfig> UTIL_GetPhysicBehaviorConfigFromConfigId(int configId);
std::shared_ptr<CClientAnimControlConfig> UTIL_GetAnimControlConfigFromConfigId(int configId);
std::shared_ptr<CClientPhysicObjectConfig> UTIL_GetPhysicObjectConfigFromConfigId(int configId);

std::shared_ptr<CClientRagdollObjectConfig> UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(const std::shared_ptr<CClientPhysicObjectConfig>& p);
std::shared_ptr<CClientDynamicObjectConfig> UTIL_ConvertPhysicObjectConfigToDynamicObjectConfig(const std::shared_ptr<CClientPhysicObjectConfig>& p);
std::shared_ptr<CClientStaticObjectConfig> UTIL_ConvertPhysicObjectConfigToStaticObjectConfig(const std::shared_ptr<CClientPhysicObjectConfig>& p);

bool UTIL_RemoveRigidBodyFromPhysicObjectConfig(CClientPhysicObjectConfig* pPhysicConfig, int configId);
bool UTIL_RemoveConstraintFromPhysicObjectConfig(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_RemovePhysicBehaviorFromPhysicObjectConfig(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_RemoveAnimControlFromRagdollObjectConfig(CClientRagdollObjectConfig* pRagdollObjectConfig, int configId);

std::shared_ptr<CClientCollisionShapeConfig> UTIL_CreateEmptyCollisionShapeConfig();
std::shared_ptr<CClientRigidBodyConfig> UTIL_CreateEmptyRigidBodyConfig();
std::shared_ptr<CClientConstraintConfig> UTIL_CreateEmptyConstraintConfig();
std::shared_ptr<CClientPhysicBehaviorConfig> UTIL_CreateEmptyPhysicBehaviorConfig();
std::shared_ptr<CClientAnimControlConfig> UTIL_CreateEmptyAnimControlConfig();

std::shared_ptr<CClientCollisionShapeConfig> UTIL_CloneCollisionShapeConfig(const CClientCollisionShapeConfig* pOldShape);
std::shared_ptr<CClientRigidBodyConfig> UTIL_CloneRigidBodyConfig(const CClientRigidBodyConfig* pOldConfig);
std::shared_ptr<CClientConstraintConfig> UTIL_CloneConstraintConfig(const CClientConstraintConfig* pOldConfig);
std::shared_ptr<CClientPhysicBehaviorConfig> UTIL_ClonePhysicBehaviorConfig(const CClientPhysicBehaviorConfig* pOldConfig);
std::shared_ptr<CClientAnimControlConfig> UTIL_CloneAnimControlConfig(const CClientAnimControlConfig* pOldConfig);

bool UTIL_IsCollisionShapeConfigModified(const CClientCollisionShapeConfig* pCollisionShapeConfig);
bool UTIL_IsPhysicObjectConfigModified(const CClientPhysicObjectConfig* pPhysicObjectConfig);
void UTIL_SetCollisionShapeConfigUnmodified(CClientCollisionShapeConfig* pCollisionShapeConfig);
void UTIL_SetPhysicObjectConfigUnmodified(CClientPhysicObjectConfig* pPhysicObjectConfig);

IPhysicRigidBody* UTIL_GetPhysicComponentAsRigidBody(int physicComponentId);
IPhysicConstraint* UTIL_GetPhysicComponentAsConstraint(int physicComponentId);
IPhysicBehavior* UTIL_GetPhysicComponentAsPhysicBehavior(int physicComponentId);

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

int UTIL_GetPhysicBehaviorIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
int UTIL_GetPhysicBehaviorIndex(const CClientPhysicObjectConfig* pPhysicObjectConfig, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig);
bool UTIL_ShiftUpPhysicBehaviorIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftUpPhysicBehaviorIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig);
bool UTIL_ShiftDownPhysicBehaviorIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);
bool UTIL_ShiftDownPhysicBehaviorIndex(CClientPhysicObjectConfig* pPhysicObjectConfig, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig);

int UTIL_GetAnimControlIndex(const CClientRagdollObjectConfig* pRagdollObjectConfig, int configId);
int UTIL_GetAnimControlIndex(const CClientRagdollObjectConfig* pRagdollObjectConfig, const CClientAnimControlConfig* pAnimControlConfig);
bool UTIL_ShiftUpAnimControlIndex(CClientRagdollObjectConfig* pRagdollObjectConfig, int configId);
bool UTIL_ShiftDownAnimControlIndex(CClientRagdollObjectConfig* pRagdollObjectConfig, int configId);
bool UTIL_ShiftUpAnimControlIndex(CClientRagdollObjectConfig* pRagdollObjectConfig, CClientAnimControlConfig* pAnimControlConfig);
bool UTIL_ShiftDownAnimControlIndex(CClientRagdollObjectConfig* pRagdollObjectConfig, CClientAnimControlConfig* pAnimControlConfig);

bool UTIL_GetCrc32ForBoneChunk(model_t* mod, std::string* output);
bool UTIL_GetCrc32ForModelFile(model_t* mod, std::string* output);

bool UTIL_RebuildPhysicObjectWithClonedConfig(uint64_t physicObjectId, const CClientPhysicObjectConfig* pPhysicObjectConfig, int configId);