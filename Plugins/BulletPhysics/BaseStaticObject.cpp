#include "BaseStaticObject.h"
#include "PhysicUTIL.h"

CBaseStaticObject::CBaseStaticObject(const CPhysicObjectCreationParameter& CreationParam)
{
	m_entindex = CreationParam.m_entindex;
	m_entity = CreationParam.m_entity;
	m_model = CreationParam.m_model;
	m_model_scaling = CreationParam.m_model_scaling;
	m_playerindex = CreationParam.m_playerindex;
}

CBaseStaticObject::~CBaseStaticObject()
{
	DispatchRemovePhysicComponents(m_PhysicComponents);
}

int CBaseStaticObject::GetEntityIndex() const
{
	return m_entindex;
}

cl_entity_t* CBaseStaticObject::GetClientEntity() const
{
	return m_entity;
}

entity_state_t* CBaseStaticObject::GetClientEntityState() const
{
	return &m_entity->curstate;
}

bool CBaseStaticObject::GetGoldSrcOriginAngles(float* origin, float* angles)
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

model_t* CBaseStaticObject::GetModel() const
{
	return m_model;
}

float CBaseStaticObject::GetModelScaling() const
{
	return m_model_scaling;
}

uint64 CBaseStaticObject::GetPhysicObjectId() const
{
	return PACK_PHYSIC_OBJECT_ID(m_entindex, EngineGetModelIndex(m_model));
}

int CBaseStaticObject::GetPlayerIndex() const
{
	return m_playerindex;
}

int CBaseStaticObject::GetObjectFlags() const
{
	return m_flags;
}

int CBaseStaticObject::GetPhysicConfigId() const
{
	return m_configId;
}

bool CBaseStaticObject::IsClientEntityNonSolid() const
{
	if (GetClientEntity() == r_worldentity || GetClientEntity() == gEngfuncs.GetEntityByIndex(0))
		return false;

	return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
}

bool CBaseStaticObject::ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const
{
	if (m_debugDrawLevel > 0 && ctx->m_staticObjectLevel > 0 && ctx->m_staticObjectLevel >= m_debugDrawLevel)
		return true;

	return false;
}

bool CBaseStaticObject::EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (callback(pPhysicComponent))
			return true;
	}

	return false;
}

bool CBaseStaticObject::Build(const CPhysicObjectCreationParameter& CreationParam)
{
	if (CreationParam.m_pPhysicObjectConfig->type != PhysicObjectType_StaticObject)
	{
		gEngfuncs.Con_DPrintf("CBaseStaticObject::Build: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pStaticObjectConfig = (CClientStaticObjectConfig*)CreationParam.m_pPhysicObjectConfig;

	m_configId = pStaticObjectConfig->configId;
	m_flags = pStaticObjectConfig->flags;
	m_debugDrawLevel = pStaticObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = pStaticObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pStaticObjectConfig->ConstraintConfigs;
	m_PhysicBehaviorConfigs = pStaticObjectConfig->PhysicBehaviorConfigs;

	if (CreationParam.m_model->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
	}

	DispatchBuildPhysicComponents(
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_PhysicBehaviorConfigs,
		std::bind(&CBaseStaticObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::CreatePhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddPhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	return true;
}

bool CBaseStaticObject::Rebuild(const CPhysicObjectCreationParameter& CreationParam)
{
	if (CreationParam.m_pPhysicObjectConfig->type != PhysicObjectType_StaticObject)
	{
		gEngfuncs.Con_DPrintf("CBaseStaticObject::Rebuild: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pStaticObjectConfig = (CClientStaticObjectConfig*)CreationParam.m_pPhysicObjectConfig;

	CPhysicComponentFilters filters;

	filters.m_RigidBodyFilter.m_HasWithFlags = true;
	filters.m_ConstraintFilter.m_HasWithFlags = true;
	filters.m_PhysicBehaviorFilter.m_HasWithFlags = true;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

	m_configId = pStaticObjectConfig->configId;
	m_flags = pStaticObjectConfig->flags;
	m_debugDrawLevel = pStaticObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = pStaticObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pStaticObjectConfig->ConstraintConfigs;
	m_PhysicBehaviorConfigs = pStaticObjectConfig->PhysicBehaviorConfigs;

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
		std::bind(&CBaseStaticObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::CreatePhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddPhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);
	return true;
}

void CBaseStaticObject::Update(CPhysicObjectUpdateContext* ObjectUpdateContext)
{
	DispatchPhysicComponentsUpdate(m_PhysicComponents, ObjectUpdateContext, false);
}

bool CBaseStaticObject::SetupBones(CRagdollObjectSetupBoneContext* Context)
{
	return false;
}

bool CBaseStaticObject::SetupJiggleBones(CRagdollObjectSetupBoneContext* Context)
{
	return false;
}

bool CBaseStaticObject::StudioCheckBBox(studiohdr_t* studiohdr, int* nVisible)
{
	if (!(GetObjectFlags() & PhysicObjectFlag_OverrideStudioCheckBBox))
		return false;

	return DispatchStudioCheckBBox(m_PhysicComponents, studiohdr, nVisible);
}

bool CBaseStaticObject::CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPersonView, int iSyncViewLevel, void(*callback)(struct ref_params_s* pparams))
{
	return false;
}

void CBaseStaticObject::AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters& filters)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (!pPhysicComponent->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pPhysicComponent, filters))
		{
			pPhysicComponent->AddToPhysicWorld(world);
		}
	}
}

void CBaseStaticObject::RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters)
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

void CBaseStaticObject::RemovePhysicComponentsWithFilters(const CPhysicComponentFilters& filters)
{
	DispatchRemovePhysicCompoentsWithFilters(m_PhysicComponents, filters);
}

void CBaseStaticObject::TransferOwnership(int entindex)
{
	m_entindex = entindex;
	m_entity = ClientEntityManager()->GetEntityByIndex(entindex);

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		pPhysicComponent->TransferOwnership(entindex);
	}
}

IPhysicComponent* CBaseStaticObject::GetPhysicComponentByName(const std::string& name)
{
	return DispatchGetPhysicComponentByName(m_PhysicComponents, name);
}

IPhysicComponent* CBaseStaticObject::GetPhysicComponentByComponentId(int id)
{
	return DispatchGetPhysicComponentByComponentId(m_PhysicComponents, id);
}

IPhysicRigidBody* CBaseStaticObject::GetRigidBodyByName(const std::string& name)
{
	return DispatchGetRigidBodyByName(m_PhysicComponents, name);
}

IPhysicRigidBody* CBaseStaticObject::GetRigidBodyByComponentId(int id)
{
	return DispatchGetRigidBodyByComponentId(m_PhysicComponents, id);
}

IPhysicConstraint* CBaseStaticObject::GetConstraintByName(const std::string& name)
{
	return DispatchGetConstraintByName(m_PhysicComponents, name);
}

IPhysicConstraint* CBaseStaticObject::GetConstraintByComponentId(int id)
{
	return DispatchGetConstraintByComponentId(m_PhysicComponents, id);
}

IPhysicBehavior* CBaseStaticObject::GetPhysicBehaviorByName(const std::string& name)
{
	return DispatchGetPhysicBehaviorByName(m_PhysicComponents, name);
}

IPhysicBehavior* CBaseStaticObject::GetPhysicBehaviorByComponentId(int id)
{
	return DispatchGetPhysicBehaviorByComponentId(m_PhysicComponents, id);
}

IPhysicRigidBody* CBaseStaticObject::FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
{
	return GetRigidBodyByName(name);
}

void CBaseStaticObject::AddRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidBodyConfig, IPhysicRigidBody* pRigidBody)
{
	if (!pRigidBody)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pRigidBody);
}

void CBaseStaticObject::AddConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, IPhysicConstraint* pConstraint)
{
	if (!pConstraint)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pConstraint);
}

void CBaseStaticObject::AddPhysicBehavior(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior* pPhysicBehavior)
{
	if (!pPhysicBehavior)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pPhysicBehavior);
}
