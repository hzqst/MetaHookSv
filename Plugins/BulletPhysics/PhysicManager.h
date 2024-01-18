#pragma once

#include <interface.h>
#include <cl_entity.h>
#include <com_model.h>
#include <studio.h>
#include <r_studioint.h>
#include <r_efx.h>

class CRagdollConfig
{
public:
};

class IPhysicObject
{
public:
	virtual ~IPhysicObject() {

	}

	virtual bool IsCollisionObject() const = 0;
	virtual bool IsRagdollObject() const = 0;
	virtual bool IsStaticObject() const = 0;

	virtual int GetEntIndex() const = 0;
	virtual void GetOrigin(float *v) = 0;
};

class ICollisionPhysicObject : public IPhysicObject
{
public:

	bool IsCollisionObject() const override
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

	virtual bool UpdateKinematic(int iActivityType, entity_state_t* curstate) = 0;
	virtual void ResetPose(entity_state_t* curstate) = 0;
	virtual void ApplyBarnacle(cl_entity_t* barnacle_entity) = 0;
	virtual void ApplyGargantua(cl_entity_t* gargantua_entity) = 0;
	virtual int GetSequenceActivityType(entity_state_t* entstate) = 0;
	virtual bool SyncFirstPersonView(cl_entity_t* ent, struct ref_params_s* pparams) = 0;
	virtual void ForceSleep() = 0;
};

class IStaticObject : public ICollisionPhysicObject
{
public:

	bool IsStaticObject() const override
	{
		return true;
	}
};

class IPhysicManager : public IBaseInterface
{
public:
	virtual void Init(void) = 0;
	virtual void Shutdown() = 0;
	virtual void NewMap(void) = 0;
	virtual void DebugDraw(void) = 0;
	virtual void SetGravity(float velocity) = 0;
	virtual void StepSimulation(double framerate) = 0;
	virtual void ReloadConfig(void) = 0;
	virtual bool SetupBones(studiohdr_t* hdr, int entindex) = 0;
	virtual bool SetupJiggleBones(studiohdr_t* hdr, int entindex) = 0;
	virtual void MergeBarnacleBones(studiohdr_t* hdr, int entindex) = 0;
	virtual bool ChangeRagdollEntityIndex(int old_entindex, int new_entindex) = 0;
	virtual IPhysicObject* GetPhysicObject(int entindex) = 0;
	virtual IRagdollObject* CreateRagdollObject(model_t* mod, int entindex, const CRagdollConfig& config) = 0;
	virtual void CreateBrushModel(cl_entity_t* ent) = 0;
	virtual void CreateBarnacle(cl_entity_t* ent) = 0;
	virtual void CreateGargantua(cl_entity_t* ent) = 0;
	virtual void RemovePhysicObject(int entindex) = 0;
	virtual void UpdateTempEntity(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) = 0;
};

IPhysicManager* PhysicManager();