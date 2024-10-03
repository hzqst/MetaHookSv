#include "BaseDynamicObject.h"

CBaseDynamicObject::CBaseDynamicObject(const CPhysicObjectCreationParameter& CreationParam)
{
	m_entindex = CreationParam.m_entindex;
	m_entity = CreationParam.m_entity;
	m_model = CreationParam.m_model;
	m_model_scaling = CreationParam.m_model_scaling;
	m_playerindex = CreationParam.m_playerindex;
}

CBaseDynamicObject::~CBaseDynamicObject()
{
	DispatchRemovePhysicComponents(m_PhysicComponents);
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
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			return pRigidBody->GetGoldSrcOriginAngles(origin, angles);
		}
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
	if (m_debugDrawLevel > 0 && ctx->m_dynamicObjectLevel > 0 && ctx->m_dynamicObjectLevel >= m_debugDrawLevel)
		return true;

	return false;
}

bool CBaseDynamicObject::EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (callback(pPhysicComponent))
			return true;
	}

	return false;
}

bool CBaseDynamicObject::Build(const CPhysicObjectCreationParameter& CreationParam)
{
	if (CreationParam.m_pPhysicObjectConfig->type != PhysicObjectType_DynamicObject)
	{
		gEngfuncs.Con_DPrintf("CBaseDynamicObject::Build: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pDynamicObjectConfig = (CClientDynamicObjectConfig*)CreationParam.m_pPhysicObjectConfig;

	m_configId = pDynamicObjectConfig->configId;
	m_flags = pDynamicObjectConfig->flags;
	m_debugDrawLevel = pDynamicObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = pDynamicObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pDynamicObjectConfig->ConstraintConfigs;
	m_PhysicBehaviorConfigs = pDynamicObjectConfig->PhysicBehaviorConfigs;

	if (CreationParam.m_model->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
	}

	DispatchBuildPhysicComponents(
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_PhysicBehaviorConfigs,
		std::bind(&CBaseDynamicObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreatePhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddPhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	return true;
}

bool CBaseDynamicObject::Rebuild(const CPhysicObjectCreationParameter& CreationParam)
{
	if (CreationParam.m_pPhysicObjectConfig->type != PhysicObjectType_DynamicObject)
	{
		gEngfuncs.Con_DPrintf("CBaseDynamicObject::Rebuild: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	CPhysicComponentFilters filters;

	filters.m_RigidBodyFilter.m_HasWithFlags = true;
	filters.m_ConstraintFilter.m_HasWithFlags = true;
	filters.m_PhysicBehaviorFilter.m_HasWithFlags = true;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

	auto pDynamicObjectConfig = (CClientDynamicObjectConfig*)CreationParam.m_pPhysicObjectConfig;

	m_configId = pDynamicObjectConfig->configId;
	m_flags = pDynamicObjectConfig->flags;
	m_debugDrawLevel = pDynamicObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = pDynamicObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pDynamicObjectConfig->ConstraintConfigs;
	m_PhysicBehaviorConfigs = pDynamicObjectConfig->PhysicBehaviorConfigs;

	if (CreationParam.m_model->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
	}

	DispatchRebuildPhysicComponents(
		m_PhysicComponents,
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_PhysicBehaviorConfigs,
		std::bind(&CBaseDynamicObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreatePhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddPhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	return true;
}

void CBaseDynamicObject::Update(CPhysicObjectUpdateContext* ObjectUpdateContext)
{
	DispatchPhysicComponentsUpdate(m_PhysicComponents, ObjectUpdateContext, false);
}

bool CBaseDynamicObject::SetupBones(studiohdr_t* studiohdr, int flags)
{
	return false;
}

bool CBaseDynamicObject::SetupJiggleBones(studiohdr_t* studiohdr, int flags)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			pRigidBody->SetupJiggleBones(studiohdr, flags);
		}
	}

	return true;
}

bool CBaseDynamicObject::StudioCheckBBox(studiohdr_t* studiohdr, int* nVisible)
{
	return DispatchStudioCheckBBox(m_PhysicComponents, studiohdr, nVisible);
}

bool CBaseDynamicObject::CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams))
{
	return false;
}

void CBaseDynamicObject::AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters& filters)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (!pPhysicComponent->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pPhysicComponent, filters))
		{
			pPhysicComponent->AddToPhysicWorld(world);
		}
	}
}

void CBaseDynamicObject::RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters)
{
	for (auto itor = m_PhysicComponents.rbegin(); itor != m_PhysicComponents.rend(); itor++)
	{
		auto pPhysicComponent = (*itor);

		if (pPhysicComponent->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pPhysicComponent, filters))
		{
			pPhysicComponent->RemoveFromPhysicWorld(world);
		}
	}
}

void CBaseDynamicObject::RemovePhysicComponentsWithFilters(const CPhysicComponentFilters& filters)
{
	DispatchRemovePhysicCompoentsWithFilters(m_PhysicComponents, filters);
}

void CBaseDynamicObject::TransferOwnership(int entindex)
{
	m_entindex = entindex;
	m_entity = ClientEntityManager()->GetEntityByIndex(entindex);

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		pPhysicComponent->TransferOwnership(entindex);
	}
}

IPhysicComponent* CBaseDynamicObject::GetPhysicComponentByName(const std::string& name)
{
	return DispatchGetPhysicComponentByName(m_PhysicComponents, name);
}

IPhysicComponent* CBaseDynamicObject::GetPhysicComponentByComponentId(int id)
{
	return DispatchGetPhysicComponentByComponentId(m_PhysicComponents, id);
}

IPhysicRigidBody* CBaseDynamicObject::GetRigidBodyByName(const std::string& name)
{
	return DispatchGetRigidBodyByName(m_PhysicComponents, name);
}

IPhysicRigidBody* CBaseDynamicObject::GetRigidBodyByComponentId(int id)
{
	return DispatchGetRigidBodyByComponentId(m_PhysicComponents, id);
}

IPhysicConstraint* CBaseDynamicObject::GetConstraintByName(const std::string& name)
{
	return DispatchGetConstraintByName(m_PhysicComponents, name);
}

IPhysicConstraint* CBaseDynamicObject::GetConstraintByComponentId(int id)
{
	return DispatchGetConstraintByComponentId(m_PhysicComponents, id);
}

IPhysicBehavior* CBaseDynamicObject::GetPhysicBehaviorByName(const std::string& name)
{
	return DispatchGetPhysicBehaviorByName(m_PhysicComponents, name);
}

IPhysicBehavior* CBaseDynamicObject::GetPhysicBehaviorByComponentId(int id)
{
	return DispatchGetPhysicBehaviorByComponentId(m_PhysicComponents, id);
}

IPhysicRigidBody* CBaseDynamicObject::FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
{
	return GetRigidBodyByName(name);
}

void CBaseDynamicObject::AddRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidBodyConfig, IPhysicRigidBody* pRigidBody)
{
	if (!pRigidBody)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pRigidBody);
}

void CBaseDynamicObject::AddConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, IPhysicConstraint* pConstraint)
{
	if (!pConstraint)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pConstraint);
}

void CBaseDynamicObject::AddPhysicBehavior(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior* pPhysicBehavior)
{
	if (!pPhysicBehavior)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pPhysicBehavior);
}
