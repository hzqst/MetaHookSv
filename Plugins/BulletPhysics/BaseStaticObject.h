#pragma once

#include "exportfuncs.h"
#include "ClientEntityManager.h"
#include "BasePhysicManager.h"
#include "PhysicUTIL.h"

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
		m_configId = CreationParam.m_pStaticObjectConfig->configId;
	}

	~CBaseStaticObject()
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			ClientPhysicManager()->RemovePhysicComponent(pRigidBody->GetPhysicComponentId());
		}

		m_RigidBodies.clear();
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

	bool GetGoldSrcOrigin(float* v) override
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

	uint64 GetPhysicObjectId() const override
	{
		return PACK_PHYSIC_OBJECT_ID(m_entindex, EngineGetModelIndex(m_model));
	}

	int GetPlayerIndex() const override
	{
		return 0;
	}

	int GetObjectFlags() const override
	{
		return m_flags;
	}

	int GetPhysicConfigId() const override
	{
		return m_configId;
	}

	bool IsClientEntityNonSolid() const override
	{
		if (GetClientEntity() == r_worldentity)
			return false;

		return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
	}

	bool Rebuild(const CClientPhysicObjectConfig* pPhysicObjectConfig) override
	{
		if (pPhysicObjectConfig->type != PhysicObjectType_StaticObject)
		{
			gEngfuncs.Con_DPrintf("Rebuild: pPhysicObjectConfig->type mismatch!\n");
			return false;
		}

		auto pStaticObjectConfig = (CClientStaticObjectConfig*)pPhysicObjectConfig;

		CStaticObjectCreationParameter CreationParam;

		CreationParam.m_entity = GetClientEntity();
		CreationParam.m_entindex = GetEntityIndex();
		CreationParam.m_model = GetModel();

		if (CreationParam.m_model->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(CreationParam.m_model);
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(CreationParam.m_entity, CreationParam.m_model);
		}

		CreationParam.m_pStaticObjectConfig = pStaticObjectConfig;

		CPhysicComponentFilters filters;

		ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

		RebuildRigidBodies(CreationParam);

		return true;
	}

	void Update(CPhysicObjectUpdateContext* ObjectUpdateContext) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			CPhysicComponentUpdateContext ComponentUpdateContext(ObjectUpdateContext);

			pRigidBody->Update(&ComponentUpdateContext);
		}
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

	void AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters& filters) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (!pRigidBody->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pRigidBody, filters))
			{
				pRigidBody->AddToPhysicWorld(world);
			}
		}
	}

	void RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (pRigidBody->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pRigidBody, filters))
			{
				pRigidBody->RemoveFromPhysicWorld(world);
			}
		}
	}

	void FreePhysicActionsWithFilters(int with_flags, int without_flags) override
	{

	}

	void TransferOwnership(int entindex) override
	{
		m_entindex = entindex;
		m_entity = ClientEntityManager()->GetEntityByIndex(entindex);

		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->TransferOwnership(entindex);
		}
	}

	IPhysicComponent* GetPhysicComponentByName(const std::string& name) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (pRigidBody->GetName() == name)
			{
				return pRigidBody;
			}
		}

		return nullptr;
	}

	IPhysicComponent* GetPhysicComponentByComponentId(int id) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (pRigidBody->GetPhysicComponentId() == id)
			{
				return pRigidBody;
			}
		}

		return nullptr;
	}

	IPhysicRigidBody* GetRigidBodyByName(const std::string& name) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (pRigidBody->GetName() == name)
			{
				return pRigidBody;
			}
		}

		return nullptr;
	}

	IPhysicRigidBody* GetRigidBodyByComponentId(int id) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (pRigidBody->GetPhysicComponentId() == id)
			{
				return pRigidBody;
			}
		}

		return nullptr;
	}

	IPhysicConstraint* GetConstraintByName(const std::string& name) override
	{
		return nullptr;
	}

	IPhysicConstraint* GetConstraintByComponentId(int id) override
	{
		return nullptr;
	}

public:

	void CreateRigidBodies(const CStaticObjectCreationParameter& CreationParam)
	{
		for (const auto& pRigidBodyConfig : CreationParam.m_pStaticObjectConfig->RigidBodyConfigs)
		{
			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), 0);

			if (pRigidBody)
			{
				ClientPhysicManager()->AddPhysicComponent(pRigidBody->GetPhysicComponentId(), pRigidBody);

				CPhysicObjectUpdateContext ObjectUpdateContext(GetEntityIndex(), this);

				CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

				pRigidBody->Update(&ComponentUpdateContext);

				m_RigidBodies.emplace_back(pRigidBody);
			}
		}
	}

	virtual IPhysicRigidBody* CreateRigidBody(const CStaticObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) = 0;

protected:

		void RebuildRigidBodies(const CStaticObjectCreationParameter& CreationParam)
		{
			std::vector<IPhysicRigidBody*> newRigidBodies;

			std::vector<IPhysicRigidBody*> oldRigidBodies = m_RigidBodies;

			m_RigidBodies.clear();

			for (const auto& pRigidBodyConfig : CreationParam.m_pStaticObjectConfig->RigidBodyConfigs)
			{
				auto configId = pRigidBodyConfig->configId;
				auto foundOld = std::find_if(oldRigidBodies.begin(), oldRigidBodies.end(), [configId](IPhysicRigidBody* pRigidBodyOld) {
					return pRigidBodyOld->GetPhysicConfigId() == configId;
					});

				if (foundOld != oldRigidBodies.end())
				{
					auto oldPhysicComponentId = (*foundOld)->GetPhysicComponentId();

					auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), oldPhysicComponentId);

					if (pNewRigidBody)
					{
						newRigidBodies.emplace_back(pNewRigidBody);
					}
				}
				else
				{
					auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), 0);

					if (pNewRigidBody)
					{
						newRigidBodies.emplace_back(pNewRigidBody);
					}
				}
			}

			for (auto pRigidBody : oldRigidBodies)
			{
				ClientPhysicManager()->RemovePhysicComponent(pRigidBody->GetPhysicComponentId());
			}

			for (auto pRigidBody : newRigidBodies)
			{
				ClientPhysicManager()->AddPhysicComponent(pRigidBody->GetPhysicComponentId(), pRigidBody);
			}

			m_RigidBodies = newRigidBodies;
		}

public:

	int m_entindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	float m_model_scaling{ 1 };
	int m_flags{ PhysicObjectFlag_StaticObject };
	int m_configId{};
	std::vector<IPhysicRigidBody*> m_RigidBodies{};
};