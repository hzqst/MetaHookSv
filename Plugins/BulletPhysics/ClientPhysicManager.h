#pragma once

#include <interface.h>
#include <cl_entity.h>
#include <com_model.h>
#include <studio.h>
#include <r_studioint.h>
#include <r_efx.h>

#include "ClientPhysicCommon.h"
#include "ClientPhysicConfig.h"

class IPhysicObject;

class CPhysicObjectUpdateContext
{
public:
	CPhysicObjectUpdateContext(IPhysicObject* pPhysicObject) : m_pPhysicObject(pPhysicObject)
	{

	}

	IPhysicObject* m_pPhysicObject{};

	bool m_bShouldKillMe{};
	bool m_bRigidbodyKinematicChanged{ };

	//Only available for RagdollObject
	bool m_bActivityChanged{ };
	bool m_bRigidbodyPoseChanged{ };
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

	virtual int GetEntityIndex() const = 0;
	virtual cl_entity_t* GetClientEntity() const = 0;
	virtual entity_state_t* GetClientEntityState() const = 0;
	virtual bool GetOrigin(float* v) = 0;
	virtual model_t* GetModel() const = 0;
	virtual float GetModelScaling() const = 0;
	virtual int GetPlayerIndex() const = 0;
	virtual int GetObjectFlags() const = 0;

	virtual void Update(CPhysicObjectUpdateContext* ctx) = 0;
	virtual void TransformOwnerEntity(int entindex) = 0;
	virtual bool SetupBones(studiohdr_t* studiohdr) = 0;
	virtual bool SetupJiggleBones(studiohdr_t* studiohdr) = 0;
	virtual bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson) = 0;

	virtual void AddToPhysicWorld(void* world, const CPhysicComponentFilters &filters) = 0;
	virtual void RemoveFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) = 0;
	virtual void OnBroadcastDeleteRigidBody(IPhysicObject* pPhysicObjectToDelete, void* world, void* rigidbody) = 0;
	virtual void FreePhysicActionsWithFilters(int with_flags, int without_flags) = 0;
	virtual void* GetRigidBodyByName(const std::string& name) = 0;
	virtual void* GetRigidBodyByComponentId(int id) = 0;
	virtual void* GetConstraintByName(const std::string& name) = 0;
	virtual void* GetConstraintByComponentId(int id) = 0;

	virtual bool IsClientEntityNonSolid() const = 0;
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
};

class IDynamicObject : public ICollisionPhysicObject
{
public:
	bool IsDynamicObject() const override
	{
		return true;
	}
};

class IRagdollObject : public ICollisionPhysicObject
{
public:
	bool IsRagdollObject() const override
	{
		return true;
	}

	virtual int GetActivityType() const = 0;
	virtual int GetOverrideActivityType(entity_state_t* entstate) = 0;

	virtual void ResetPose(entity_state_t* curstate) = 0;
	virtual void UpdatePose(entity_state_t* curstate) = 0;
	virtual void ApplyBarnacle(IPhysicObject* pBarnacleObject) = 0;
	virtual void ApplyGargantua(IPhysicObject* pGargantuaObject) = 0;
	virtual void ReleaseFromBarnacle() = 0;
	virtual void ReleaseFromGargantua() = 0;
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

	virtual std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigForModel(model_t* mod) = 0;
	virtual std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModel(model_t* mod) = 0;
	virtual void LoadPhysicObjectConfigs(void) = 0;
	virtual void SavePhysicObjectConfigs(void) = 0;

	virtual bool SetupBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual bool SetupJiggleBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual void MergeBarnacleBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual IPhysicObject* GetPhysicObject(int entindex) = 0;

	virtual void CreatePhysicObjectForEntity(cl_entity_t* ent, entity_state_t* state, model_t* mod) = 0;
	virtual void SetupBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;
	virtual void SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex, const CClientAnimControlConfig& OverrideAnim) = 0;
	virtual void UpdateBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;

	virtual IPhysicObject* FindBarnacleObjectForPlayer(entity_state_t* state) = 0;
	virtual IPhysicObject* FindGargantuaObjectForPlayer(entity_state_t* state) = 0;

	//PhysicObject Management

	virtual void AddPhysicObject(int entindex, IPhysicObject* pPhysicObject) = 0;
	virtual void FreePhysicObject(IPhysicObject* pPhysicObject) = 0;
	virtual bool RemovePhysicObject(int entindex) = 0;
	virtual void RemoveAllPhysicObjects(int withflags, int withoutflags) = 0;
	virtual bool TransformOwnerEntityForPhysicObject(int old_entindex, int new_entindex) = 0;
	virtual void UpdateRagdollObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) = 0;

	//PhysicWorld Related
	virtual void AddPhysicObjectToWorld(IPhysicObject* pPhysicObject, const CPhysicComponentFilters& filters) = 0;
	virtual void RemovePhysicObjectFromWorld(IPhysicObject* pPhysicObject, const CPhysicComponentFilters& filters) = 0;
	virtual void OnBroadcastDeleteRigidBody(IPhysicObject* pPhysicObjectToDelete, void* pRigidBody) = 0;
	virtual int AllocatePhysicComponentId() = 0;
};

extern IClientPhysicManager* g_pClientPhysicManager;

IClientPhysicManager* ClientPhysicManager();
IClientPhysicManager* BulletPhysicManager_CreateInstance();
IClientPhysicManager* PhysXPhysicManager_CreateInstance();
