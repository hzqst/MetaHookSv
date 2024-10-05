#pragma once

#include <interface.h>
#include <metahook.h>
#include <cl_entity.h>
#include <com_model.h>
#include <studio.h>
#include <r_studioint.h>
#include <r_efx.h>

#include "ClientPhysicCommon.h"
#include "ClientPhysicConfig.h"

#include <functional>

#define PACK_PHYSIC_OBJECT_ID(entindex, modelindex) ((((uint64)entindex) & 0x00000000FFFFFFFFull) | ((((uint64)modelindex) << 32) & 0xFFFFFFFF00000000ull))
#define UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(physicObjectId) (int)(physicObjectId & 0x00000000FFFFFFFFull)
#define UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId) (int)((physicObjectId >> 32) & 0x00000000FFFFFFFFull)

class IPhysicObject;

class CPhysicDebugDrawContext
{
public:
	int m_staticObjectLevel{};
	int m_dynamicObjectLevel{};
	int m_ragdollObjectLevel{};
	int m_rigidbodyLevel{};
	int m_constraintLevel{};
	int m_behaviorLevel{};
	vec3_t m_constraintColor{};
	vec3_t m_behaviorColor{};
	vec3_t m_inspectedColor{};
	vec3_t m_selectedColor{};
};

class CPhysicObjectCreationParameter
{
public:
	cl_entity_t* m_entity{};
	entity_state_t* m_entstate{};
	int m_entindex{};
	model_t* m_model{};
	studiohdr_t* m_studiohdr{};
	float m_model_scaling{ 1 };
	int m_playerindex{};
	bool m_allowNonNativeRigidBody{};
	const CClientPhysicObjectConfig* m_pPhysicObjectConfig{};
};

const int BoneState_Dynamic = 0x1;
const int BoneState_Kinematic = 0x2;
const int BoneState_BoneMatrixUpdated = 0x1000;
const int BoneState_TransformUpdated = 0x2000;

class CRagdollObjectSetupBoneContext
{
public:
	int m_entindex{};
	studiohdr_t* m_studiohdr{};
	int m_flags{};
	int m_boneStates[MAXSTUDIOBONES]{ };
};

class CPhysicObjectUpdateContext
{
public:
	bool m_bShouldFree{};

	bool m_bRigidbodyKinematicChanged{ };
	bool m_bConstraintStateChanged{ };

	bool m_bActivityChanged{ };

	bool m_bRigidbodyResetPoseRequired{ };
	bool m_bRigidbodyPoseChanged{ };

	bool m_bRigidbodyUpdateBonesRequired{ };
	bool m_bRigidbodyBonesUpdated{ };
};

class CPhysicComponentSubFilters
{
public:
	bool m_HasWithFlags{};
	bool m_HasWithoutFlags{};
	bool m_HasExactMatchFlags{};
	bool m_HasExactMatchComponentId{};
	int m_WithFlags{ -1 };
	int m_WithoutFlags{ 0 };
	int m_ExactMatchFlags{ -1 };
	int m_ExactMatchComponentId{ -1 };
};

class CPhysicComponentFilters
{
public:
	CPhysicComponentSubFilters m_RigidBodyFilter;
	CPhysicComponentSubFilters m_ConstraintFilter;
	CPhysicComponentSubFilters m_PhysicBehaviorFilter;
};

class CPhysicComponentUpdateContext
{
public:
	CPhysicComponentUpdateContext(CPhysicObjectUpdateContext* ObjectUpdateContext) : m_pObjectUpdateContext(ObjectUpdateContext)
	{

	}

	CPhysicComponentUpdateContext() = delete;

	CPhysicObjectUpdateContext* m_pObjectUpdateContext{};
	bool m_bForceDynamic{};
	bool m_bForceKinematic{};
	bool m_bForceStatic{};
	bool m_bShouldFree{};
	bool bIsAddingPhysicComponent{};
};

const int PhysicTraceLineFlag_World = 0x1;
const int PhysicTraceLineFlag_StaticObject = 0x2;
const int PhysicTraceLineFlag_DynamicObject = 0x4;
const int PhysicTraceLineFlag_RagdollObject = 0x8;
const int PhysicTraceLineFlag_RigidBody = 0x10;
const int PhysicTraceLineFlag_Constraint = 0x20;
const int PhysicTraceLineFlag_PhysicBehavior = 0x40;

class CPhysicTraceLineParameters
{
public:
	vec3_t vecStart{0};
	vec3_t vecEnd{0};
	int withflags{ -1 };
	int withoutflags{ 0 };
};

class CPhysicTraceLineHitResult
{
public:
	bool m_bHasHit{};
	float m_flHitFraction{ 1 };
	float m_flDistanceToHitPoint{ 0 };
	vec3_t m_vecHitPoint{ 0 };
	vec3_t m_vecHitPlaneNormal{ 0 };
	int m_iHitEntityIndex{};
	int m_iHitPhysicComponentIndex{};
};

#if 0
class CPhysicCameraControl
{
public:
	CPhysicCameraControl()
	{

	}

	CPhysicCameraControl(const CClientCameraControlConfig& pCameraControlConfig)
	{
		VectorCopy(pCameraControlConfig.origin, m_origin);
		VectorCopy(pCameraControlConfig.origin, m_angles);
	}

	int m_physicComponentId{};
	vec3_t m_origin{ 0 };
	vec3_t m_angles{ 0 };
};
#endif

class IPhysicComponent : public IBaseInterface
{
public:
	virtual void Destroy()
	{
		delete this;
	}
	virtual bool IsRigidBody() const
	{
		return false;
	}
	virtual bool IsConstraint() const
	{
		return false;
	}
	virtual bool IsPhysicBehavior() const
	{
		return false;
	}
	virtual bool IsCameraView() const
	{
		return false;
	}
	virtual bool SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams))
	{
		return false;
	}

	virtual const char* GetTypeString() const = 0;
	virtual const char* GetTypeLocalizationTokenString() const = 0;

	virtual int GetPhysicConfigId() const = 0;
	virtual int GetPhysicComponentId() const = 0;
	virtual int GetOwnerEntityIndex() const = 0;
	virtual IPhysicObject*GetOwnerPhysicObject() const = 0;
	virtual const char* GetName() const = 0;
	virtual int GetFlags() const = 0;
	virtual int GetDebugDrawLevel() const = 0;
	virtual bool ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const = 0;

	virtual bool AddToPhysicWorld(void* world) = 0;
	virtual bool RemoveFromPhysicWorld(void* world) = 0;
	virtual bool IsAddedToPhysicWorld(void* world) const = 0;

	virtual void TransferOwnership(int entindex) = 0;
	virtual void Update(CPhysicComponentUpdateContext *ComponentContext) = 0;
};

class IPhysicRigidBody : public IPhysicComponent
{
public:
	const char* GetTypeString() const override
	{
		return "RigidBody";
	}
	const char* GetTypeLocalizationTokenString() const override
	{
		return "#BulletPhysics_RigidBody";
	}

	bool IsRigidBody() const override
	{
		return true;
	}

	virtual void ApplyCentralForce(const vec3_t vecForce) = 0;
	virtual void ApplyCentralImpulse(const vec3_t vecForce) = 0;
	virtual void SetLinearVelocity(const vec3_t vecVelocity) = 0;
	virtual void SetAngularVelocity(const vec3_t vecVelocity) = 0;
	virtual bool ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate) = 0;
	virtual bool SetupBones(CRagdollObjectSetupBoneContext* Context) = 0;
	virtual bool SetupJiggleBones(CRagdollObjectSetupBoneContext* Context) = 0;
	virtual void* GetInternalRigidBody() = 0;
	virtual bool GetGoldSrcOriginAngles(float* origin, float * angles) = 0;
	virtual bool GetGoldSrcOriginAnglesWithLocalOffset(const vec3_t localoffset_origin, const vec3_t localoffset_angles, float* origin, float * angles) = 0;
	virtual bool SetGoldSrcOriginAngles(const float* origin, const float* angles) = 0;
	virtual float GetMass() const = 0;
	virtual bool GetAABB(vec3_t mins, vec3_t maxs) const = 0;
};

class IPhysicConstraint : public IPhysicComponent
{
public:
	const char* GetTypeString() const override
	{
		return "Constraint";
	}
	const char* GetTypeLocalizationTokenString() const override
	{
		  return "#BulletPhysics_Constraint";
	}
	bool IsConstraint() const override
	{
		return true;
	}

	virtual bool ExtendLinearLimit(int axis, float value) = 0;
	virtual float GetMaxTolerantLinearError() const = 0;
	virtual void* GetInternalConstraint() = 0;
};

class IPhysicBehavior : public IPhysicComponent
{
public:
	const char* GetTypeString() const override
	{
		return "PhysicBehavior";
	}
	const char* GetTypeLocalizationTokenString() const override
	{
		return "#BulletPhysics_PhysicBehavior";
	}

	bool IsPhysicBehavior() const override
	{
		return true;
	}

	virtual IPhysicComponent* GetAttachedPhysicComponent() const
	{
		return nullptr;
	}

	virtual IPhysicRigidBody* GetAttachedRigidBody() const
	{
		return nullptr;
	}

	virtual IPhysicConstraint* GetAttachedConstraint() const
	{
		return nullptr;
	}
};

using fnEnumPhysicComponentCallback = std::function<bool(IPhysicComponent*)>;

class IPhysicObject : public IBaseInterface
{
public:
	virtual void Destroy()
	{
		delete this;
	}
	virtual bool IsCollisionObject() const
	{
		return false;
	}
	virtual bool IsRagdollObject() const
	{
		return false;
	}
	virtual bool IsStaticObject() const
	{
		return false;
	}
	virtual bool IsDynamicObject() const
	{
		return false;
	}
	virtual const char* GetTypeString() const = 0;
	virtual const char* GetTypeLocalizationTokenString() const = 0;

	virtual int GetEntityIndex() const = 0;
	virtual cl_entity_t* GetClientEntity() const = 0;
	virtual entity_state_t* GetClientEntityState() const = 0;
	virtual bool GetGoldSrcOriginAngles(float* origin, float* angles) = 0;
	virtual model_t* GetModel() const = 0;
	virtual float GetModelScaling() const = 0;
	virtual uint64 GetPhysicObjectId() const = 0;
	virtual int GetPlayerIndex() const = 0;
	virtual int GetObjectFlags() const = 0;	
	virtual int GetPhysicConfigId() const = 0;
	virtual bool IsClientEntityNonSolid() const = 0;
	virtual bool ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext *ctx) const = 0;

	virtual bool EnumPhysicComponents(const fnEnumPhysicComponentCallback &callback) = 0;
	virtual bool Build(const CPhysicObjectCreationParameter& CreationParam) = 0;
	virtual bool Rebuild(const CPhysicObjectCreationParameter& CreationParam) = 0;
	virtual void Update(CPhysicObjectUpdateContext* ctx) = 0;
	virtual void TransferOwnership(int entindex) = 0;
	virtual bool SetupBones(CRagdollObjectSetupBoneContext* Context) = 0;
	virtual bool SetupJiggleBones(CRagdollObjectSetupBoneContext* Context) = 0;
	virtual bool StudioCheckBBox(studiohdr_t* studiohdr, int *nVisible) = 0;
	virtual bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams)) = 0;

	virtual void AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters &filters) = 0;
	virtual void RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) = 0;
	virtual void RemovePhysicComponentsWithFilters(const CPhysicComponentFilters& filters) = 0;
	virtual IPhysicComponent* GetPhysicComponentByName(const std::string& name) = 0;
	virtual IPhysicComponent* GetPhysicComponentByComponentId(int id) = 0;
	virtual IPhysicRigidBody* GetRigidBodyByName(const std::string& name) = 0;
	virtual IPhysicRigidBody* GetRigidBodyByComponentId(int id) = 0;
	virtual IPhysicConstraint* GetConstraintByName(const std::string& name) = 0;
	virtual IPhysicConstraint* GetConstraintByComponentId(int id) = 0;
	virtual IPhysicBehavior* GetPhysicBehaviorByName(const std::string& name) = 0;
	virtual IPhysicBehavior* GetPhysicBehaviorByComponentId(int id) = 0;

	//For finding barnacle & garg's rigidbodies
	virtual IPhysicRigidBody* FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody) = 0;
};

class ICollisionPhysicObject : public IPhysicObject
{
public:
	bool IsCollisionObject() const override
	{
		return true;
	}
};

class IStaticObject : public ICollisionPhysicObject
{
public:
	bool IsStaticObject() const override
	{
		return true;
	}
	const char* GetTypeString() const override
	{
		return "StaticObject";
	}
	const char* GetTypeLocalizationTokenString() const override
	{
		return "#BulletPhysics_StaticObject";
	}
};

class IDynamicObject : public ICollisionPhysicObject
{
public:
	bool IsDynamicObject() const override
	{
		return true;
	}
	const char* GetTypeString() const override
	{
		return "DynamicObject";
	}
	const char* GetTypeLocalizationTokenString() const override
	{
		return "#BulletPhysics_DynamicObject";
	}
};

class IRagdollObject : public ICollisionPhysicObject
{
public:
	bool IsRagdollObject() const override
	{
		return true;
	}
	const char* GetTypeString() const override
	{
		return "RagdollObject";
	}
	const char* GetTypeLocalizationTokenString() const override
	{
		return "#BulletPhysics_RagdollObject";
	}

	virtual int GetAnimControlFlags() const = 0;
	virtual StudioAnimActivityType GetActivityType() const = 0;
	virtual void CalculateOverrideActivityType(const entity_state_t* entstate, StudioAnimActivityType* pActivityType, int* pAnimControlFlags) const = 0;

	virtual bool ResetPose(entity_state_t* curstate) = 0;
	virtual void UpdateBones(entity_state_t* curstate) = 0;
	virtual void ApplyBarnacle(IPhysicObject* pBarnacleObject) = 0;
	virtual void ApplyGargantua(IPhysicObject* pGargantuaObject) = 0;
	virtual void ReleaseFromBarnacle() = 0;
	virtual void ReleaseFromGargantua() = 0;
	virtual int GetBarnacleIndex() const = 0;
	virtual int GetGargantuaIndex() const = 0;
	virtual bool IsDebugAnimEnabled() const = 0;
	virtual void SetDebugAnimEnabled(bool bEnabled) = 0;
	virtual bool SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams)) = 0;
};

class IClientPhysicManager : public IBaseInterface
{
public:
	virtual void Destroy() = 0;
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void NewMap() = 0;
	virtual void DebugDraw() = 0;
	virtual void SetGravity(float velocity) = 0;
	virtual void StepSimulation(double framerate) = 0;

	virtual bool SetupBones(CRagdollObjectSetupBoneContext *Context) = 0;
	virtual bool SetupJiggleBones(CRagdollObjectSetupBoneContext* Context) = 0;
	virtual bool StudioCheckBBox(studiohdr_t* studiohdr, int entindex, int *nVisible) = 0;

	virtual void TraceLine(const CPhysicTraceLineParameters& traceParam, CPhysicTraceLineHitResult& hitResult) = 0;

	//PhysicObjectConfig Management

	virtual bool SavePhysicObjectConfigForModel(model_t* mod) = 0;
	virtual bool SavePhysicObjectConfigForModelIndex(int modelindex) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigForModel(model_t* mod) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> CreateEmptyPhysicObjectConfigForModel(model_t* mod, int PhysicObjectType) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> CreateEmptyPhysicObjectConfigForModelIndex(int modelindex, int PhysicObjectType) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModel(model_t* mod) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModelIndex(int modelindex) = 0;
	virtual void LoadPhysicObjectConfigs(void) = 0;
	virtual void SavePhysicObjectConfigs(void) = 0;
	virtual bool SavePhysicObjectConfigToFile(const std::string& filename, CClientPhysicObjectConfig* pPhysicObjectConfig) = 0;
	virtual void RemoveAllPhysicObjectConfigs(int withflags, int withoutflags) = 0;

	virtual IPhysicObject* FindBarnacleObjectForPlayer(entity_state_t* state) = 0;
	virtual IPhysicObject* FindGargantuaObjectForPlayer(entity_state_t* state) = 0;

	//PhysicObject Management

	virtual IPhysicObject* GetPhysicObject(int entindex) = 0;
	virtual IPhysicObject* GetPhysicObjectEx(uint64 physicObjectId) = 0;
	virtual void AddPhysicObject(int entindex, IPhysicObject* pPhysicObject) = 0;
	virtual void FreePhysicObject(IPhysicObject* pPhysicObject) = 0;
	virtual bool RemovePhysicObject(int entindex) = 0;
	virtual bool RemovePhysicObjectEx(uint64 physicObjectId) = 0;
	virtual void RemoveAllPhysicObjects(int withflags, int withoutflags) = 0;
	virtual bool TransferOwnershipForPhysicObject(int old_entindex, int new_entindex) = 0;
	virtual bool RebuildPhysicObject(int entindex, const CClientPhysicObjectConfig* pPhysicObjectConfig) = 0;
	virtual bool RebuildPhysicObjectEx(uint64 physicObjectId, const CClientPhysicObjectConfig* pPhysicObjectConfig) = 0;
	virtual bool RebuildPhysicObjectEx2(IPhysicObject* pPhysicObject, const CClientPhysicObjectConfig* pPhysicObjectConfig) = 0;
	virtual void UpdateAllPhysicObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) = 0;

	virtual void CreatePhysicObjectForEntity(cl_entity_t* ent, entity_state_t* state, model_t* mod) = 0;

	virtual void SetupBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;
	virtual void SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex, const CClientAnimControlConfig* pOverrideAnimControl) = 0;
	virtual void UpdateBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;

	//PhysicWorld

	virtual void AddPhysicComponentsToWorld(IPhysicObject* pPhysicObject, const CPhysicComponentFilters& filters) = 0;
	virtual void RemovePhysicComponentsFromWorld(IPhysicObject* pPhysicObject, const CPhysicComponentFilters& filters) = 0;
	virtual void AddPhysicComponentToWorld(IPhysicComponent* pPhysicComponent) = 0;
	virtual void RemovePhysicComponentFromWorld(IPhysicComponent* pPhysicComponent) = 0;

	//PhysicComponent Management

	virtual int AllocatePhysicComponentId() = 0;
	virtual IPhysicComponent *GetPhysicComponent(int physicComponentId) = 0;
	virtual void AddPhysicComponent(int physicComponentId, IPhysicComponent* pPhysicComponent) = 0;
	virtual bool RemovePhysicComponent(int physicComponentId) = 0;

	//Inspection / Selection System

	virtual void SetInspectedPhysicComponentId(int physicComponentId) = 0;
	virtual int  GetInspectedPhysicComponentId() const = 0;
	virtual void SetSelectedPhysicComponentId(int physicComponentId) = 0;
	virtual int  GetSelectedPhysicComponentId() const = 0;

	virtual void   SetInspectedPhysicObjectId(uint64 physicObjectId) = 0;
	virtual uint64 GetInspectedPhysicObjectId() const = 0;
	virtual void   SetSelectedPhysicObjectId(uint64 physicObjectId) = 0;
	virtual uint64 GetSelectedPhysicObjectId() const = 0;

	virtual const CPhysicDebugDrawContext* GetDebugDrawContext() const = 0;

	//BasePhysicConfig Management

	virtual int AllocatePhysicConfigId() = 0;
	virtual std::weak_ptr<CClientBasePhysicConfig> GetPhysicConfig(int configId) = 0;
	virtual void AddPhysicConfig(int configId, const std::shared_ptr<CClientBasePhysicConfig>& pPhysicConfig) = 0;
	virtual bool RemovePhysicConfig(int configId) = 0;
	virtual void RemoveAllPhysicConfigs() = 0;

	//VertexIndexArray Management
	virtual std::shared_ptr<CPhysicIndexArray> LoadIndexArrayFromResource(const std::string& resourcePath) = 0;
	virtual void FreeAllIndexArrays(int withflags, int withoutflags) = 0;
};

extern IClientPhysicManager* g_pClientPhysicManager;

IClientPhysicManager* ClientPhysicManager();
IClientPhysicManager* BulletPhysicManager_CreateInstance();
IClientPhysicManager* PhysXPhysicManager_CreateInstance();
