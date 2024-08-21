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

#define PACK_PHYSIC_OBJECT_ID(entindex, modelindex) ((((uint64)entindex) & 0x00000000FFFFFFFFull) | ((((uint64)modelindex) << 32) & 0xFFFFFFFF00000000ull))
#define UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(physicObjectId) (int)(physicObjectId & 0x00000000FFFFFFFFull)
#define UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId) (int)((physicObjectId >> 32) & 0x00000000FFFFFFFFull)

class IPhysicObject;

class CPhysicObjectUpdateContext
{
public:
	CPhysicObjectUpdateContext(int entindex, IPhysicObject* pPhysicObject) : m_entindex(entindex), m_pPhysicObject(pPhysicObject)
	{

	}

	CPhysicObjectUpdateContext() = delete;

	int m_entindex{};
	IPhysicObject* m_pPhysicObject{};

	bool m_bShouldFree{};
	bool m_bRigidbodyKinematicChanged{ };
	bool m_bConstraintStateChanged{ };

	bool m_bActivityChanged{ };

	bool m_bRigidbodyResetPoseRequired{ };
	bool m_bRigidbodyPoseChanged{ };

	bool m_bRigidbodyUpdatePoseRequired{ };
	bool m_bRigidbodyPoseUpdated{ };
};

class CPhysicComponentFilters
{
public:
	bool m_HasWithRigidbodyFlags{ false };
	bool m_HasWithoutRigidbodyFlags{ false };
	bool m_HasExactMatchRigidbodyFlags{ false };
	bool m_HasExactMatchRigidBodyComponentId{};
	int m_WithRigidbodyFlags{ -1 };
	int m_WithoutRigidbodyFlags{ 0 };
	int m_ExactMatchRigidbodyFlags{ -1 };
	int m_ExactMatchRigidBodyComponentId{ -1 };

	bool m_HasWithConstraintFlags{};
	bool m_HasWithoutConstraintFlags{};
	bool m_HasExactMatchConstraintFlags{};
	bool m_HasExactMatchConstraintComponentId{};
	int m_WithConstraintFlags{ -1 };
	int m_WithoutConstraintFlags{ 0 };
	int m_ExactMatchConstraintFlags{ -1 };
	int m_ExactMatchConstraintComponentId{ -1 };
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
	virtual const char* GetTypeString() const
	{
		return "PhysicComponent";
	}
	virtual const char* GetTypeLocalizationTokenString() const
	{
		return "#BulletPhysics_PhysicComponent";
	}

	virtual int GetPhysicConfigId() const = 0;
	virtual int GetPhysicComponentId() const = 0;
	virtual int GetOwnerEntityIndex() const = 0;
	virtual const char* GetName() const = 0;
	virtual int GetFlags() const = 0;
	virtual int GetDebugDrawLevel() const = 0;

	virtual bool AddToPhysicWorld(void* world) = 0;
	virtual bool RemoveFromPhysicWorld(void* world) = 0;
	virtual bool IsAddedToPhysicWorld(void* world) const = 0;

	virtual void TransferOwnership(int entindex) = 0;
	virtual void Update(CPhysicComponentUpdateContext *ComponentContext) = 0;
};

class IPhysicRigidBody : public IPhysicComponent
{
public:
	virtual const char* GetTypeString() const
	{
		return "RigidBody";
	}
	virtual const char* GetTypeLocalizationTokenString() const
	{
		return "#BulletPhysics_RigidBody";
	}

	bool IsRigidBody() const override
	{
		return true;
	}

	virtual float GetMass() const = 0;

	virtual void ApplyCentralForce(const vec3_t vecForce) = 0;
	virtual void SetLinearVelocity(const vec3_t vecVelocity) = 0;
	virtual void SetAngularVelocity(const vec3_t vecVelocity) = 0;
	virtual bool ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate) = 0;
	virtual bool SetupBones(studiohdr_t* studiohdr) = 0;
	virtual bool SetupJiggleBones(studiohdr_t* studiohdr) = 0;
	virtual void* GetInternalRigidBody() = 0;
};

class IPhysicConstraint : public IPhysicComponent
{
public:
	bool IsConstraint() const override
	{
		return true;
	}

	virtual void ExtendLinearLimit(int axis, float value) = 0;
	virtual void* GetInternalConstraint() = 0;
};

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
	virtual const char* GetTypeString() const
	{
		return "PhysicObject";
	}
	virtual const char * GetTypeLocalizationTokenString() const
	{
		return "#BulletPhysics_PhysicObject";
	}

	virtual int GetEntityIndex() const = 0;
	virtual cl_entity_t* GetClientEntity() const = 0;
	virtual entity_state_t* GetClientEntityState() const = 0;
	virtual bool GetGoldSrcOrigin(float* v) = 0;
	virtual model_t* GetModel() const = 0;
	virtual float GetModelScaling() const = 0;
	virtual uint64 GetPhysicObjectId() const = 0;
	virtual int GetPlayerIndex() const = 0;
	virtual int GetObjectFlags() const = 0;	
	virtual int GetPhysicConfigId() const = 0;
	virtual bool IsClientEntityNonSolid() const = 0;

	virtual bool Rebuild(const CClientPhysicObjectConfig *pPhysicObjectConfig) = 0;
	virtual void Update(CPhysicObjectUpdateContext* ctx) = 0;
	virtual void TransferOwnership(int entindex) = 0;
	virtual bool SetupBones(studiohdr_t* studiohdr) = 0;
	virtual bool SetupJiggleBones(studiohdr_t* studiohdr) = 0;
	virtual bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams)) = 0;

	virtual void AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters &filters) = 0;
	virtual void RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) = 0;
	virtual void FreePhysicActionsWithFilters(int with_flags, int without_flags) = 0;
	virtual IPhysicComponent* GetPhysicComponentByName(const std::string& name) = 0;
	virtual IPhysicComponent* GetPhysicComponentByComponentId(int id) = 0;
	virtual IPhysicRigidBody* GetRigidBodyByName(const std::string& name) = 0;
	virtual IPhysicRigidBody* GetRigidBodyByComponentId(int id) = 0;
	virtual IPhysicConstraint* GetConstraintByName(const std::string& name) = 0;
	virtual IPhysicConstraint* GetConstraintByComponentId(int id) = 0;
};

class IPhysicAction : public IBaseInterface
{
public:
	//return false to remove this action
	virtual bool Update(CPhysicObjectUpdateContext* ctx) = 0;
	virtual int GetActionFlags() const = 0;
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

	virtual int GetActivityType() const = 0;
	virtual int GetOverrideActivityType(entity_state_t* entstate) = 0;

	virtual bool ResetPose(entity_state_t* curstate) = 0;
	virtual void UpdatePose(entity_state_t* curstate) = 0;
	virtual void ApplyBarnacle(IPhysicObject* pBarnacleObject) = 0;
	virtual void ApplyGargantua(IPhysicObject* pGargantuaObject) = 0;
	virtual void ReleaseFromBarnacle() = 0;
	virtual void ReleaseFromGargantua() = 0;
	virtual int GetBarnacleIndex() const = 0;
	virtual int GetGargantuaIndex() const = 0;
	virtual bool SyncFirstPersonView(struct ref_params_s* pparams, void(*callback)(struct ref_params_s* pparams)) = 0;
	virtual bool SyncThirdPersonView(struct ref_params_s* pparams, void(*callback)(struct ref_params_s* pparams)) = 0;
};

class IClientPhysicManager : public IBaseInterface
{
public:
	virtual void Destroy(void) = 0;
	virtual void Init(void) = 0;
	virtual void Shutdown() = 0;
	virtual void NewMap(void) = 0;
	virtual void DebugDraw(void) = 0;
	virtual void SetGravity(float velocity) = 0;
	virtual void StepSimulation(double framerate) = 0;

	//PhysicObjectConfig Management
	virtual bool SavePhysicObjectConfigForModel(model_t* mod) = 0;
	virtual bool SavePhysicObjectConfigForModelIndex(int modelindex) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigForModel(model_t* mod) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModel(model_t* mod) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModelIndex(int modelindex) = 0;
	virtual void LoadPhysicObjectConfigs(void) = 0;
	virtual void SavePhysicObjectConfigs(void) = 0;
	virtual bool SavePhysicObjectConfigToFile(const std::string& filename, CClientPhysicObjectConfig* pPhysicObjectConfig) = 0;
	virtual bool LoadPhysicObjectConfigFromFiles(const std::string& filename, CClientPhysicObjectConfigStorage& Storage) = 0;
	virtual bool LoadPhysicObjectConfigFromBSP(model_t *mod, CClientPhysicObjectConfigStorage& Storage) = 0;
	virtual void RemoveAllPhysicObjectConfigs(int withflags, int withoutflags) = 0;

	virtual IPhysicObject* FindBarnacleObjectForPlayer(entity_state_t* state) = 0;
	virtual IPhysicObject* FindGargantuaObjectForPlayer(entity_state_t* state) = 0;
	virtual void TraceLine(vec3_t vecStart, vec3_t vecEnd, CPhysicTraceLineHitResult& hitResult) = 0;

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
	virtual void UpdateAllPhysicObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) = 0;
	
	virtual void CreatePhysicObjectForEntity(cl_entity_t* ent, entity_state_t* state, model_t* mod) = 0;

	virtual bool SetupBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual bool SetupJiggleBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual void MergeBarnacleBones(studiohdr_t* studiohdr, int entindex) = 0;

	virtual void SetupBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;
	virtual void SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex, const CClientAnimControlConfig& OverrideAnim) = 0;
	virtual void UpdateBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;

	//PhysicWorld

	virtual void AddPhysicComponentsToWorld(IPhysicObject* pPhysicObject, const CPhysicComponentFilters& filters) = 0;
	virtual void RemovePhysicComponentsFromWorld(IPhysicObject* pPhysicObject, const CPhysicComponentFilters& filters) = 0;
	virtual void AddPhysicComponentToWorld(IPhysicComponent* pPhysicComponent) = 0;
	virtual void RemovePhysicComponentFromWorld(IPhysicComponent* pPhysicComponent) = 0;
	virtual void OnPhysicComponentAddedIntoPhysicWorld(IPhysicComponent* pPhysicComponent) = 0;
	virtual void OnPhysicComponentRemovedFromPhysicWorld(IPhysicComponent* pPhysicComponent) = 0;

	//PhysicComponent Management

	virtual int AllocatePhysicComponentId() = 0;
	virtual IPhysicComponent *GetPhysicComponent(int physicComponentId) = 0;
	virtual void AddPhysicComponent(int physicComponentId, IPhysicComponent* pPhysicComponent) = 0;
	virtual void FreePhysicComponent(IPhysicComponent* pPhysicComponent) = 0;
	virtual bool RemovePhysicComponent(int physicComponentId) = 0;

	//Inspection / Selection System
	virtual void SetInspectedColor(const vec3_t inspectedColor) = 0;
	virtual void SetSelectedColor(const vec3_t selectedColor) = 0;

	virtual void SetInspectedPhysicComponentId(int physicComponentId) = 0;
	virtual int  GetInspectedPhysicComponentId() const = 0;
	virtual void SetSelectedPhysicComponentId(int physicComponentId) = 0;
	virtual int  GetSelectedPhysicComponentId() const = 0;

	virtual void   SetInspectedPhysicObjectId(uint64 physicObjectId) = 0;
	virtual uint64 GetInspectedPhysicObjectId() const = 0;
	virtual void   SetSelectedPhysicObjectId(uint64 physicObjectId) = 0;
	virtual uint64 GetSelectedPhysicObjectId() const = 0;

	//BasePhysicConfig Management
	virtual int AllocatePhysicConfigId() = 0;
	virtual std::weak_ptr<CClientBasePhysicConfig> GetPhysicConfig(int configId) = 0;
	virtual void AddPhysicConfig(int configId, const std::shared_ptr<CClientBasePhysicConfig>& pPhysicConfig) = 0;
	virtual bool RemovePhysicConfig(int configId) = 0;
	virtual void RemoveAllPhysicConfigs() = 0;
};

extern IClientPhysicManager* g_pClientPhysicManager;

IClientPhysicManager* ClientPhysicManager();
IClientPhysicManager* BulletPhysicManager_CreateInstance();
IClientPhysicManager* PhysXPhysicManager_CreateInstance();
