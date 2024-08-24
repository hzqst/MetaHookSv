#pragma once

#include "exportfuncs.h"
#include "ClientEntityManager.h"
#include "BasePhysicManager.h"
#include "PhysicUTIL.h"

class CBaseDynamicObject : public IDynamicObject
{
public:
	CBaseDynamicObject(const CDynamicObjectCreationParameter& CreationParam)
	{
		m_entindex = CreationParam.m_entindex;
		m_entity = CreationParam.m_entity;
		m_model = CreationParam.m_model;
		m_model_scaling = CreationParam.m_model_scaling;
		m_configId = CreationParam.m_pDynamicObjectConfig->configId;
		m_flags = CreationParam.m_pDynamicObjectConfig->flags;
		m_debugDrawLevel = CreationParam.m_pDynamicObjectConfig->debugDrawLevel;
	}

	~CBaseDynamicObject()
	{
		for (auto pConstraint : m_Constraints)
		{
			ClientPhysicManager()->RemovePhysicComponent(pConstraint->GetPhysicComponentId());
		}

		m_Constraints.clear();

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

	bool GetGoldSrcOriginAngles(float* origin, float* angles) override
	{
		if (GetRigidBodyCount() == 1)
		{
			return GetRigidBodyByIndex(0)->GetGoldSrcOriginAngles(origin, angles);
		}

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

	bool ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const override
	{
		if (m_debugDrawLevel > 0 && ctx->m_staticObjectLevel > 0 && ctx->m_staticObjectLevel >= m_debugDrawLevel)
			return true;

		return false;
	}

	int GetRigidBodyCount() const override
	{
		return (int)m_RigidBodies.size();
	}

	IPhysicRigidBody* GetRigidBodyByIndex(int index) const override
	{
		return m_RigidBodies.at(index);
	}

	bool EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (callback(pRigidBody))
				return true;
		}

		return false;
	}

	bool Rebuild(const CClientPhysicObjectConfig* pPhysicObjectConfig) override
	{
		if (pPhysicObjectConfig->type != PhysicObjectType_DynamicObject)
		{
			gEngfuncs.Con_DPrintf("Rebuild: pPhysicObjectConfig->type mismatch!\n");
			return false;
		}

		auto pDynamicObjectConfig = (CClientDynamicObjectConfig*)pPhysicObjectConfig;

		CDynamicObjectCreationParameter CreationParam;

		CreationParam.m_entity = GetClientEntity();
		CreationParam.m_entindex = GetEntityIndex();
		CreationParam.m_model = GetModel();

		if (CreationParam.m_model->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(CreationParam.m_model);
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(CreationParam.m_entity, CreationParam.m_model);
		}

		CreationParam.m_pDynamicObjectConfig = pDynamicObjectConfig;

		if (GetModel()->type == mod_studio)
			ClientPhysicManager()->SetupBonesForRagdoll(GetClientEntity(), GetClientEntityState(), GetModel(), m_entindex, 0);

		CPhysicComponentFilters filters;

		ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

		RebuildRigidBodies(CreationParam);

		RebuildConstraints(CreationParam);

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

	bool MergeBones(studiohdr_t* studiohdr) override
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

		for (auto pConstraint : m_Constraints)
		{
			pConstraint->TransferOwnership(entindex);
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
		for (auto pConstraint : m_Constraints)
		{
			if (pConstraint->GetName() == name)
			{
				return pConstraint;
			}
		}

		return nullptr;
	}

	IPhysicConstraint* GetConstraintByComponentId(int id) override
	{
		for (auto pConstraint : m_Constraints)
		{
			if (pConstraint->GetPhysicComponentId() == id)
			{
				return pConstraint;
			}
		}

		return nullptr;
	}

public:

	IPhysicRigidBody* FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
	{
		return GetRigidBodyByName(name);
	}

	void CreateRigidBodies(const CDynamicObjectCreationParameter& CreationParam)
	{
		for (const auto& pRigidBodyConfig : CreationParam.m_pDynamicObjectConfig->RigidBodyConfigs)
		{
			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), 0);

			if (pRigidBody)
			{
				ClientPhysicManager()->AddPhysicComponent(pRigidBody->GetPhysicComponentId(), pRigidBody);

				CPhysicObjectUpdateContext ObjectUpdateContext;

				CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

				pRigidBody->Update(&ComponentUpdateContext);

				m_RigidBodies.emplace_back(pRigidBody);
			}
		}
	}

	void CreateConstraints(const CDynamicObjectCreationParameter& CreationParam)
	{
		for (const auto& pConstraintConfig : CreationParam.m_pDynamicObjectConfig->ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get(), 0);

			if (pConstraint)
			{
				ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

				m_Constraints.emplace_back(pConstraint);
			}
		}
	}

	virtual IPhysicRigidBody* CreateRigidBody(const CDynamicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) = 0;
	virtual IPhysicConstraint* CreateConstraint(const CDynamicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) = 0;

protected:

	void RebuildRigidBodies(const CDynamicObjectCreationParameter& CreationParam)
	{
		std::map<int, int> configIdToComponentIdMap;

		for (auto pRigidBody : m_RigidBodies)
		{
			configIdToComponentIdMap[pRigidBody->GetPhysicConfigId()] = pRigidBody->GetPhysicComponentId();

			ClientPhysicManager()->RemovePhysicComponent(pRigidBody->GetPhysicComponentId());
		}

		m_RigidBodies.clear();

		for (const auto& pRigidBodyConfig : CreationParam.m_pDynamicObjectConfig->RigidBodyConfigs)
		{
			auto found = configIdToComponentIdMap.find(pRigidBodyConfig->configId);

			if (found != configIdToComponentIdMap.end())
			{
				auto oldPhysicComponentId = found->second;

				auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), oldPhysicComponentId);

				if (pNewRigidBody)
				{
					ClientPhysicManager()->AddPhysicComponent(pNewRigidBody->GetPhysicComponentId(), pNewRigidBody);

					m_RigidBodies.emplace_back(pNewRigidBody);
				}
			}
			else
			{
				auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), 0);

				if (pNewRigidBody)
				{
					ClientPhysicManager()->AddPhysicComponent(pNewRigidBody->GetPhysicComponentId(), pNewRigidBody);

					m_RigidBodies.emplace_back(pNewRigidBody);
				}
			}
		}
	}

	void RebuildConstraints(const CDynamicObjectCreationParameter& CreationParam)
	{
		std::map<int, int> configIdToComponentIdMap;

		for (auto pConstraint : m_Constraints)
		{
			configIdToComponentIdMap[pConstraint->GetPhysicConfigId()] = pConstraint->GetPhysicComponentId();

			ClientPhysicManager()->RemovePhysicComponent(pConstraint->GetPhysicComponentId());
		}
		m_Constraints.clear();

		for (const auto& pConstraintConfig : CreationParam.m_pDynamicObjectConfig->ConstraintConfigs)
		{
			auto found = configIdToComponentIdMap.find(pConstraintConfig->configId);

			if (found != configIdToComponentIdMap.end())
			{
				auto oldPhysicComponentId = found->second;

				auto pNewConstraint = CreateConstraint(CreationParam, pConstraintConfig.get(), oldPhysicComponentId);

				if (pNewConstraint)
				{
					ClientPhysicManager()->AddPhysicComponent(pNewConstraint->GetPhysicComponentId(), pNewConstraint);

					m_Constraints.emplace_back(pNewConstraint);
				}
			}
			else
			{
				auto pNewConstraint = CreateConstraint(CreationParam, pConstraintConfig.get(), 0);

				if (pNewConstraint)
				{
					ClientPhysicManager()->AddPhysicComponent(pNewConstraint->GetPhysicComponentId(), pNewConstraint);

					m_Constraints.emplace_back(pNewConstraint);
				}
			}
		}
	}

public:

	int m_entindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	float m_model_scaling{ 1 };
	int m_flags{ PhysicObjectFlag_DynamicObject };
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	int m_configId{};
	std::vector<IPhysicRigidBody*> m_RigidBodies{};
	std::vector<IPhysicConstraint*> m_Constraints;
};