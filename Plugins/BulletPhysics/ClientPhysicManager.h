#pragma once

#include <interface.h>
#include <cl_entity.h>
#include <com_model.h>
#include <studio.h>
#include <r_studioint.h>
#include <r_efx.h>

#include "ClientPhysicCommon.h"
#include "ClientPhysicConfig.h"

class CPhysicObjectUpdateContext
{
public:
	bool bShouldKillMe{};
	bool bRigidbodyKinematicChanged{ };

	//Only available for RagdollObject
	bool bActivityChanged{ };
	bool bRigidbodyPoseChanged{ };
	bool bRigidbodyPoseUpdated{ };
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

	virtual void AddToPhysicWorld(void* world) = 0;
	virtual void RemoveFromPhysicWorld(void* world) = 0;

	virtual bool IsClientEntityNonSolid() const = 0;
};

class IPhysicObjectService : public IBaseInterface
{
public:
	virtual void Destroy()
	{
		delete this;
	}

	virtual IPhysicObject* GetPhysicObject() const = 0;
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
	virtual void ApplyBarnacle(cl_entity_t* pBarnacleEntity) = 0;
	virtual void ApplyGargantua(cl_entity_t* pGargantuaEntity) = 0;
	virtual bool SyncFirstPersonView(cl_entity_t* ent, struct ref_params_s* pparams) = 0;
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
	virtual void LoadPhysicConfigs(void) = 0;
	virtual bool SetupBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual bool SetupJiggleBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual void MergeBarnacleBones(studiohdr_t* studiohdr, int entindex) = 0;
	virtual IPhysicObject* GetPhysicObject(int entindex) = 0;
	virtual CClientPhysicConfig* LoadPhysicConfigForModel(model_t* mod) = 0;

	virtual void CreatePhysicObjectForEntity(cl_entity_t* ent, entity_state_t* state, model_t* mod) = 0;
	virtual void SetupBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;
	virtual void SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex, const CClientRagdollAnimControlConfig& OverrideAnim) = 0;
	virtual void UpdateBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) = 0;

	//PhysicObject Management

	virtual void AddPhysicObject(int entindex, IPhysicObject* pPhysicObject) = 0;
	virtual void FreePhysicObject(IPhysicObject* pPhysicObject) = 0;
	virtual bool RemovePhysicObject(int entindex) = 0;
	virtual void RemoveAllPhysicObjects(int flags) = 0;
	virtual bool TransformOwnerEntityForPhysicObject(int old_entindex, int new_entindex) = 0;
	virtual void UpdateRagdollObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) = 0;

	//PhysicWorld Related
	virtual void AddPhysicObjectToWorld(IPhysicObject* PhysicObject) = 0;
	virtual void RemovePhysicObjectFromWorld(IPhysicObject* PhysicObject) = 0;
};

extern IClientPhysicManager* g_pClientPhysicManager;

IClientPhysicManager* ClientPhysicManager();
IClientPhysicManager* BulletPhysicManager_CreateInstance();
IClientPhysicManager* PhysXPhysicManager_CreateInstance();
