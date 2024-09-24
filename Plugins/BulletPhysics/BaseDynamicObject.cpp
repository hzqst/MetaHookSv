#include "BaseDynamicObject.h"

CBaseDynamicObject::CBaseDynamicObject(const CPhysicObjectCreationParameter& CreationParam)
{
	m_entindex = CreationParam.m_entindex;
	m_entity = CreationParam.m_entity;
	m_model = CreationParam.m_model;
	m_model_scaling = CreationParam.m_model_scaling;
	m_playerindex = CreationParam.m_playerindex;
	m_configId = CreationParam.m_pPhysicObjectConfig->configId;
	m_flags = CreationParam.m_pPhysicObjectConfig->flags;
	m_debugDrawLevel = CreationParam.m_pPhysicObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = CreationParam.m_pPhysicObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = CreationParam.m_pPhysicObjectConfig->ConstraintConfigs;
	m_ActionConfigs = CreationParam.m_pPhysicObjectConfig->ActionConfigs;
}

CBaseDynamicObject::~CBaseDynamicObject()
{
	DispatchFreePhysicComponents(m_PhysicComponents);
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

bool CBaseDynamicObject::Rebuild(const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	if (pPhysicObjectConfig->type != PhysicObjectType_DynamicObject)
	{
		gEngfuncs.Con_DPrintf("Rebuild: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pDynamicObjectConfig = (CClientDynamicObjectConfig*)pPhysicObjectConfig;

	CPhysicObjectCreationParameter CreationParam;

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

	CreationParam.m_pPhysicObjectConfig = pDynamicObjectConfig;

	CPhysicComponentFilters filters;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

	if (CreationParam.m_model->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
	}

	m_RigidBodyConfigs = pDynamicObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pDynamicObjectConfig->ConstraintConfigs;
	m_ActionConfigs = pDynamicObjectConfig->ActionConfigs;

	DispatchRebuildPhysicComponents(
		m_PhysicComponents,
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_ActionConfigs,
		std::bind(&CBaseDynamicObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreateAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	return true;
}

void CBaseDynamicObject::Update(CPhysicObjectUpdateContext* ObjectUpdateContext)
{
	DispatchPhysicComponentsUpdate(m_PhysicComponents, ObjectUpdateContext);
}

bool CBaseDynamicObject::SetupBones(studiohdr_t* studiohdr)
{
	return false;
}

bool CBaseDynamicObject::SetupJiggleBones(studiohdr_t* studiohdr)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			pRigidBody->SetupJiggleBones(studiohdr);
		}
	}

	return true;
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

void CBaseDynamicObject::FreePhysicComponentsWithFilters(const CPhysicComponentFilters& filters)
{
	DispatchFreePhysicCompoentsWithFilters(m_PhysicComponents, filters);
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

IPhysicAction* CBaseDynamicObject::GetPhysicActionByName(const std::string& name)
{
	return DispatchGetPhysicActionByName(m_PhysicComponents, name);
}

IPhysicAction* CBaseDynamicObject::GetPhysicActionByComponentId(int id)
{
	return DispatchGetPhysicActionByComponentId(m_PhysicComponents, id);
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

void CBaseDynamicObject::AddAction(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicActionConfig* pPhysicActionConfig, IPhysicAction* pPhysicAction)
{
	if (!pPhysicAction)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pPhysicAction);
}
