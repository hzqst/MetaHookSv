#pragma once

#include "exportfuncs.h"
#include "ClientEntityManager.h"
#include "BasePhysicManager.h"

class CBaseStaticObject : public IStaticObject
{
public:
	CBaseStaticObject(const CStaticObjectCreationParameter& CreationParam)
	{
		m_entindex = CreationParam.m_entindex;
		m_entity = CreationParam.m_entity;
		m_model = CreationParam.m_model;
		m_model_scaling = CreationParam.m_model_scaling;
		m_flags = CreationParam.m_pStaticObjectConfig->flags;
	}

	~CBaseStaticObject()
	{

	}
	int GetEntityIndex() const override
	{
		return m_entindex;
	}

	cl_entity_t* GetClientEntity() const override
	{
		return m_entity;
	}

	entity_state_t* GetClientEntityState() const override
	{
		return &m_entity->curstate;
	}

	bool GetOrigin(float* v) override
	{
		return false;
	}

	model_t* GetModel() const override
	{
		return m_model;
	}

	float GetModelScaling() const override
	{
		return m_model_scaling;
	}

	int GetPlayerIndex() const override
	{
		return 0;
	}

	int GetObjectFlags() const override
	{
		return m_flags;
	}

	void Update(CPhysicObjectUpdateContext* ctx) override
	{
		
	}

	bool SetupBones(studiohdr_t* studiohdr) override
	{
		return false;
	}

	bool SetupJiggleBones(studiohdr_t* studiohdr) override
	{
		return false;
	}

	bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams)) override
	{
		return false;
	}

	void TransformOwnerEntity(int entindex) override
	{
		m_entindex = entindex;
		m_entity = ClientEntityManager()->GetEntityByIndex(entindex);
	}

	bool IsClientEntityNonSolid() const override
	{
		if (GetClientEntity() == r_worldentity)
			return false;

		return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
	}

public:
	int m_entindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	float m_model_scaling{ 1 };
	int m_flags{ PhysicObjectFlag_StaticObject };
};