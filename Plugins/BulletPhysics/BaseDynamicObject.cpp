#include "BaseDynamicObject.h"

CBaseDynamicObject::CBaseDynamicObject(const CDynamicObjectCreationParameter& CreationParam)
{
	m_entindex = CreationParam.m_entindex;
	m_entity = CreationParam.m_entity;
	m_model = CreationParam.m_model;
	m_model_scaling = CreationParam.m_model_scaling;
	m_playerindex = CreationParam.m_playerindex;
	m_configId = CreationParam.m_pDynamicObjectConfig->configId;
	m_flags = CreationParam.m_pDynamicObjectConfig->flags;
	m_debugDrawLevel = CreationParam.m_pDynamicObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = CreationParam.m_pDynamicObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = CreationParam.m_pDynamicObjectConfig->ConstraintConfigs;
	m_ActionConfigs = CreationParam.m_pDynamicObjectConfig->ActionConfigs;
}

CBaseDynamicObject::~CBaseDynamicObject()
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

int CBaseDynamicObject::GetEntityIndex() const
{
	return m_entindex;
}

cl_entity_t* CBaseDynamicObject::GetClientEntity() const
{
	return m_entity;
}

entity_state_t* CBaseDynamicObject::GetClientEntityState() const
{
	return &m_entity->curstate;
}

bool CBaseDynamicObject::GetGoldSrcOriginAngles(float* origin, float* angles)
{
	if (GetRigidBodyCount() > 0)
	{
		return GetRigidBodyByIndex(0)->GetGoldSrcOriginAngles(origin, angles);
	}

	return false;
}

model_t* CBaseDynamicObject::GetModel() const
{
	return m_model;
}

float CBaseDynamicObject::GetModelScaling() const
{
	return m_model_scaling;
}

uint64 CBaseDynamicObject::GetPhysicObjectId() const
{
	return PACK_PHYSIC_OBJECT_ID(m_entindex, EngineGetModelIndex(m_model));
}

int CBaseDynamicObject::GetPlayerIndex() const
{
	return m_playerindex;
}

int CBaseDynamicObject::GetObjectFlags() const
{
	return m_flags;
}

int CBaseDynamicObject::GetPhysicConfigId() const
{
	return m_configId;
}

bool CBaseDynamicObject::IsClientEntityNonSolid() const
{
	if (GetClientEntity() == r_worldentity)
		return false;

	return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
}

bool CBaseDynamicObject::ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const
{
	if (m_debugDrawLevel > 0 && ctx->m_staticObjectLevel > 0 && ctx->m_staticObjectLevel >= m_debugDrawLevel)
		return true;

	return false;
}

int CBaseDynamicObject::GetRigidBodyCount() const
{
	return (int)m_RigidBodies.size();
}

IPhysicRigidBody* CBaseDynamicObject::GetRigidBodyByIndex(int index) const
{
	return m_RigidBodies.at(index);
}

bool CBaseDynamicObject::EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		if (callback(pRigidBody))
			return true;
	}

	return false;
}

bool CBaseDynamicObject::Rebuild(const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	if (pPhysicObjectConfig->type != PhysicObjectType_DynamicObject)
	{
		gEngfuncs.Con_DPrintf("Rebuild: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pDynamicObjectConfig = (CClientDynamicObjectConfig*)pPhysicObjectConfig;

	CDynamicObjectCreationParameter CreationParam;

	CreationParam.m_entity = GetClientEntity();
	CreationParam.m_entstate = GetClientEntityState();
	CreationParam.m_entindex = GetEntityIndex();
	CreationParam.m_model = GetModel();

	if (CreationParam.m_model->type == mod_studio)
	{
		CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(CreationParam.m_model);
		CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(CreationParam.m_entity, CreationParam.m_model);
	}

	CreationParam.m_playerindex = GetPlayerIndex();

	CreationParam.m_pDynamicObjectConfig = pDynamicObjectConfig;

	CPhysicComponentFilters filters;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

	if (CreationParam.m_model->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
	}

	m_RigidBodyConfigs = pDynamicObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pDynamicObjectConfig->ConstraintConfigs;
	m_ActionConfigs = pDynamicObjectConfig->ActionConfigs;

	RebuildRigidBodies(CreationParam);

	RebuildConstraints(CreationParam);

	return true;
}

void CBaseDynamicObject::Update(CPhysicObjectUpdateContext* ObjectUpdateContext)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		CPhysicComponentUpdateContext ComponentUpdateContext(ObjectUpdateContext);

		pRigidBody->Update(&ComponentUpdateContext);
	}
}

bool CBaseDynamicObject::SetupBones(studiohdr_t* studiohdr)
{
	return false;
}

bool CBaseDynamicObject::SetupJiggleBones(studiohdr_t* studiohdr)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		pRigidBody->SetupJiggleBones(studiohdr);
	}

	return true;
}

bool CBaseDynamicObject::CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams))
{
	return false;
}

void CBaseDynamicObject::AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters& filters)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		if (!pRigidBody->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pRigidBody, filters))
		{
			pRigidBody->AddToPhysicWorld(world);
		}
	}
}

void CBaseDynamicObject::RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		if (pRigidBody->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pRigidBody, filters))
		{
			pRigidBody->RemoveFromPhysicWorld(world);
		}
	}
}

void CBaseDynamicObject::FreePhysicActionsWithFilters(int with_flags, int without_flags)
{

}

void CBaseDynamicObject::TransferOwnership(int entindex)
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

IPhysicComponent* CBaseDynamicObject::GetPhysicComponentByName(const std::string& name)
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

IPhysicComponent* CBaseDynamicObject::GetPhysicComponentByComponentId(int id)
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

IPhysicRigidBody* CBaseDynamicObject::GetRigidBodyByName(const std::string& name)
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

IPhysicRigidBody* CBaseDynamicObject::GetRigidBodyByComponentId(int id)
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

IPhysicConstraint* CBaseDynamicObject::GetConstraintByName(const std::string& name)
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

IPhysicConstraint* CBaseDynamicObject::GetConstraintByComponentId(int id)
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

IPhysicRigidBody* CBaseDynamicObject::FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
{
	return GetRigidBodyByName(name);
}

void CBaseDynamicObject::CreateRigidBodies(const CDynamicObjectCreationParameter& CreationParam)
{
	for (const auto& pRigidBodyConfig : CreationParam.m_pDynamicObjectConfig->RigidBodyConfigs)
	{
		auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), 0);

		if (pRigidBody)
		{
			AddRigidBody(pRigidBody);
		}
	}
}

void CBaseDynamicObject::CreateConstraints(const CDynamicObjectCreationParameter& CreationParam)
{
	for (const auto& pConstraintConfig : CreationParam.m_pDynamicObjectConfig->ConstraintConfigs)
	{
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_NonNative)
			continue;

		auto pConstraint = CreateConstraint(CreationParam, pConstraintConfigPtr, 0);

		if (pConstraint)
		{
			AddConstraint(pConstraint);
		}
	}
}

void CBaseDynamicObject::AddRigidBody(IPhysicRigidBody* pRigidBody)
{
	ClientPhysicManager()->AddPhysicComponent(pRigidBody->GetPhysicComponentId(), pRigidBody);

	CPhysicObjectUpdateContext ObjectUpdateContext;

	CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

	pRigidBody->Update(&ComponentUpdateContext);

	m_RigidBodies.emplace_back(pRigidBody);
}

void CBaseDynamicObject::AddConstraint(IPhysicConstraint* pConstraint)
{
	ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

	CPhysicObjectUpdateContext ObjectUpdateContext;

	CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

	pConstraint->Update(&ComponentUpdateContext);

	m_Constraints.emplace_back(pConstraint);
}

void CBaseDynamicObject::RebuildRigidBodies(const CDynamicObjectCreationParameter& CreationParam)
{
	std::map<int, int> configIdToComponentIdMap;

	for (auto pRigidBody : m_RigidBodies)
	{
		configIdToComponentIdMap[pRigidBody->GetPhysicConfigId()] = pRigidBody->GetPhysicComponentId();

		ClientPhysicManager()->RemovePhysicComponent(pRigidBody->GetPhysicComponentId());
	}

	m_RigidBodies.clear();

	for (const auto& pRigidBodyConfig : m_RigidBodyConfigs)
	{
		const auto pRigidBodyConfigPtr = pRigidBodyConfig.get();

		auto found = configIdToComponentIdMap.find(pRigidBodyConfigPtr->configId);

		if (found != configIdToComponentIdMap.end())
		{
			auto oldPhysicComponentId = found->second;

			auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfigPtr, oldPhysicComponentId);

			if (pNewRigidBody)
			{
				AddRigidBody(pNewRigidBody);
			}
		}
		else
		{
			auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfigPtr, 0);

			if (pNewRigidBody)
			{
				AddRigidBody(pNewRigidBody);
			}
		}
	}
}

void CBaseDynamicObject::RebuildConstraints(const CDynamicObjectCreationParameter& CreationParam)
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
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_NonNative)
			continue;

		auto found = configIdToComponentIdMap.find(pConstraintConfigPtr->configId);

		if (found != configIdToComponentIdMap.end())
		{
			auto oldPhysicComponentId = found->second;

			auto pNewConstraint = CreateConstraint(CreationParam, pConstraintConfigPtr, oldPhysicComponentId);

			if (pNewConstraint)
			{
				AddConstraint(pNewConstraint);
			}
		}
		else
		{
			auto pNewConstraint = CreateConstraint(CreationParam, pConstraintConfigPtr, 0);

			if (pNewConstraint)
			{
				AddConstraint(pNewConstraint);
			}
		}
	}
}