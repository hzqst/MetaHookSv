#include "BaseRagdollObject.h"
#include "BasePhysicManager.h"

#include "PhysicUTIL.h"

#include "privatehook.h"

CBaseRagdollObject::CBaseRagdollObject(const CPhysicObjectCreationParameter& CreationParam)
{
	m_entindex = CreationParam.m_entindex;
	m_entity = CreationParam.m_entity;
	m_model = CreationParam.m_model;
	m_model_scaling = CreationParam.m_model_scaling;
	m_playerindex = CreationParam.m_playerindex;
}

CBaseRagdollObject::~CBaseRagdollObject()
{
	DispatchFreePhysicComponents(m_PhysicComponents);
}

int CBaseRagdollObject::GetEntityIndex() const
{
	return m_entindex;
}

cl_entity_t* CBaseRagdollObject::GetClientEntity() const
{
	return m_entity;
}

entity_state_t* CBaseRagdollObject::GetClientEntityState() const
{
	if (ClientEntityManager()->IsEntityDeadPlayer(m_entity) || ClientEntityManager()->IsEntityPlayer(m_entity))
	{
		if (m_playerindex >= 0)
			return R_GetPlayerState(m_playerindex);
	}

	return &m_entity->curstate;
}

model_t* CBaseRagdollObject::GetModel() const
{
	return m_model;
}

float CBaseRagdollObject::GetModelScaling() const
{
	return m_model_scaling;
}

uint64 CBaseRagdollObject::GetPhysicObjectId() const
{
	return PACK_PHYSIC_OBJECT_ID(m_entindex, EngineGetModelIndex(m_model));
}

int CBaseRagdollObject::GetPlayerIndex() const
{
	return m_playerindex;
}

int CBaseRagdollObject::GetObjectFlags() const
{
	return m_flags;
}

int CBaseRagdollObject::GetPhysicConfigId() const
{
	return m_configId;
}

bool CBaseRagdollObject::IsClientEntityNonSolid() const
{
	if (GetActivityType() > StudioAnimActivityType_Idle)
		return false;

	return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
}

bool CBaseRagdollObject::ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const
{
	if (m_debugDrawLevel > 0 && ctx->m_ragdollObjectLevel > 0 && ctx->m_ragdollObjectLevel >= m_debugDrawLevel)
		return true;

	return false;
}

bool CBaseRagdollObject::EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (callback(pPhysicComponent))
			return true;
	}

	return false;
}

bool CBaseRagdollObject::Build(const CPhysicObjectCreationParameter& CreationParam)
{
	if (CreationParam.m_pPhysicObjectConfig->type != PhysicObjectType_RagdollObject)
	{
		gEngfuncs.Con_DPrintf("CBaseRagdollObject::Build: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pRagdollObjectConfig = (CClientRagdollObjectConfig*)CreationParam.m_pPhysicObjectConfig;

	m_AnimControlConfigs = pRagdollObjectConfig->AnimControlConfigs;

	for (const auto& pAnimConfig : m_AnimControlConfigs)
	{
		if (pAnimConfig->activity == StudioAnimActivityType_Idle)
		{
			m_IdleAnimConfig = pAnimConfig;
			break;
		}
	}

	for (const auto& pAnimConfig : m_AnimControlConfigs)
	{
		if (pAnimConfig->activity == StudioAnimActivityType_Debug)
		{
			m_DebugAnimConfig = pAnimConfig;
			break;
		}
	}

	m_configId = pRagdollObjectConfig->configId;
	m_flags = pRagdollObjectConfig->flags;
	m_debugDrawLevel = pRagdollObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = pRagdollObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pRagdollObjectConfig->ConstraintConfigs;
	m_PhysicBehaviorConfigs = pRagdollObjectConfig->PhysicBehaviorConfigs;

	if (CreationParam.m_model->type == mod_studio)
	{
		if (m_IdleAnimConfig)
		{
			ClientPhysicManager()->SetupBonesForRagdollEx(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex, m_IdleAnimConfig.get());
		}
		else
		{
			ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
		}
	}

	SaveBoneRelativeTransform(CreationParam);

	DispatchBuildPhysicComponents(
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_PhysicBehaviorConfigs,
		std::bind(&CBaseRagdollObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::CreatePhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddPhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	SetupNonKeyBones(CreationParam);

	//InitCameraControl(&pRagdollObjectConfig->ThirdPersonViewCameraControlConfig, m_ThirdPersonViewCameraControl);
	//InitCameraControl(&pRagdollObjectConfig->FirstPersonViewCameraControlConfig, m_FirstPersonViewCameraControl);

	return true;
}

bool CBaseRagdollObject::Rebuild(const CPhysicObjectCreationParameter& CreationParam)
{
	if (CreationParam.m_pPhysicObjectConfig->type != PhysicObjectType_RagdollObject)
	{
		gEngfuncs.Con_DPrintf("CBaseRagdollObject::Rebuild: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pRagdollObjectConfig = (CClientRagdollObjectConfig*)CreationParam.m_pPhysicObjectConfig;

	CPhysicComponentFilters filters;

	filters.m_RigidBodyFilter.m_HasWithFlags = true;
	filters.m_ConstraintFilter.m_HasWithFlags = true;
	filters.m_PhysicBehaviorFilter.m_HasWithFlags = true;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

	m_configId = pRagdollObjectConfig->configId;
	m_flags = pRagdollObjectConfig->flags;
	m_debugDrawLevel = pRagdollObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = pRagdollObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pRagdollObjectConfig->ConstraintConfigs;
	m_PhysicBehaviorConfigs = pRagdollObjectConfig->PhysicBehaviorConfigs;

	m_AnimControlConfigs = pRagdollObjectConfig->AnimControlConfigs;

	for (const auto& pAnimConfig : m_AnimControlConfigs)
	{
		if (pAnimConfig->activity == StudioAnimActivityType_Idle)
		{
			m_IdleAnimConfig = pAnimConfig;
			break;
		}
	}

	for (const auto& pAnimConfig : m_AnimControlConfigs)
	{
		if (pAnimConfig->activity == StudioAnimActivityType_Debug)
		{
			m_DebugAnimConfig = pAnimConfig;
			break;
		}
	}

	m_keyBones.clear();
	m_nonKeyBones.clear();

	if (CreationParam.m_model->type == mod_studio)
	{
		if (m_IdleAnimConfig)
		{
			ClientPhysicManager()->SetupBonesForRagdollEx(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex, m_IdleAnimConfig.get());
		}
		else
		{
			ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
		}
	}

	SaveBoneRelativeTransform(CreationParam);

	DispatchRebuildPhysicComponents(
		m_PhysicComponents,
		CreationParam, 
		m_RigidBodyConfigs, 
		m_ConstraintConfigs, 
		m_PhysicBehaviorConfigs,
		std::bind(&CBaseRagdollObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::CreatePhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddPhysicBehavior, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	SetupNonKeyBones(CreationParam);

	//InitCameraControl(&pRagdollObjectConfig->ThirdPersonViewCameraControlConfig, m_ThirdPersonViewCameraControl);
	//InitCameraControl(&pRagdollObjectConfig->FirstPersonViewCameraControlConfig, m_FirstPersonViewCameraControl);

	return true;
}

void CBaseRagdollObject::Update(CPhysicObjectUpdateContext* ObjectUpdateContext)
{
	auto playerState = GetClientEntityState();

	if (m_bDebugAnimEnabled && m_DebugAnimConfig)
	{
		if (m_DebugAnimConfig->sequence >= 0)
		{
			playerState->sequence = m_DebugAnimConfig->sequence;
			playerState->frame = m_DebugAnimConfig->animframe;
			playerState->framerate = 0;
		}

		if (m_DebugAnimConfig->gaitsequence >= 0)
		{
			playerState->gaitsequence = m_DebugAnimConfig->gaitsequence;
		}

#define COPY_BYTE_ENTSTATE(attr, to, i) if (m_DebugAnimConfig->attr[i] >= 0 && m_DebugAnimConfig->attr[i] <= 255) to->attr[i] = m_DebugAnimConfig->attr[i];
		COPY_BYTE_ENTSTATE(controller, playerState, 0);
		COPY_BYTE_ENTSTATE(controller, playerState, 1);
		COPY_BYTE_ENTSTATE(controller, playerState, 2);
		COPY_BYTE_ENTSTATE(controller, playerState, 3);
		COPY_BYTE_ENTSTATE(blending, playerState, 0);
		COPY_BYTE_ENTSTATE(blending, playerState, 1);
		COPY_BYTE_ENTSTATE(blending, playerState, 2);
		COPY_BYTE_ENTSTATE(blending, playerState, 3);
#undef COPY_BYTE_ENTSTATE
	}

	auto iOldActivityType = GetActivityType();

	auto iNewActivityType = StudioGetSequenceActivityType(m_model, playerState);

	CalculateOverrideActivityType(playerState, iNewActivityType);

	if (m_playerindex == m_entindex)
	{
		if (iNewActivityType == StudioAnimActivityType_Death)
		{
			ClientEntityManager()->SetPlayerDeathState(m_playerindex, playerState, m_model);
		}
		else
		{
			ClientEntityManager()->ClearPlayerDeathState(m_playerindex);
		}
	}

	if (iNewActivityType != StudioAnimActivityType_CaughtByBarnacle)
	{
		ReleaseFromBarnacle();
		ReleaseFromGargantua();
	}

	if (UpdateActivity(iOldActivityType, iNewActivityType, playerState))
	{
		ObjectUpdateContext->m_bActivityChanged = true;

		//Transformed from whatever to barnacle
		if (iNewActivityType == StudioAnimActivityType_CaughtByBarnacle && iOldActivityType != StudioAnimActivityType_CaughtByBarnacle)
		{
			auto pBarnacleObject = ClientPhysicManager()->FindBarnacleObjectForPlayer(playerState);

			if (pBarnacleObject)
			{
				ApplyBarnacle(pBarnacleObject);
			}
			else
			{
				auto pGargantuaEntity = ClientPhysicManager()->FindGargantuaObjectForPlayer(playerState);

				if (pGargantuaEntity)
				{
					ApplyGargantua(pGargantuaEntity);
				}
			}
		}

		//Transformed from death or barnacle to idle state.
		else if (iOldActivityType > 0 && iNewActivityType == 0)
		{
			ObjectUpdateContext->m_bRigidbodyResetPoseRequired = true;
		}
	}

	if (!ClientEntityManager()->IsEntityInVisibleList(GetClientEntity()))
	{
		ObjectUpdateContext->m_bRigidbodyUpdateBonesRequired = true;
	}

	if (!ObjectUpdateContext->m_bRigidbodyUpdateBonesRequired && ShouldForceUpdateBones())
	{
		ObjectUpdateContext->m_bRigidbodyUpdateBonesRequired = true;
	}

	DispatchPhysicComponentsUpdate(m_PhysicComponents, ObjectUpdateContext);

	//Reset all rigidbodies with pose-reset bonematrix, this consumes a large amount of CPU resources so take it carefully
	if (ObjectUpdateContext->m_bRigidbodyResetPoseRequired && !ObjectUpdateContext->m_bRigidbodyPoseChanged)
	{
		ResetPose(playerState);

		ObjectUpdateContext->m_bRigidbodyPoseChanged = true;
	}

	//Sync kinematic rigidbodies with latest bonematrix, this consumes a large amount of CPU resources so take it carefully
	if (ObjectUpdateContext->m_bRigidbodyUpdateBonesRequired && !ObjectUpdateContext->m_bRigidbodyBonesUpdated)
	{
		UpdateBones(playerState);

		ObjectUpdateContext->m_bRigidbodyBonesUpdated = true;
	}
}

bool CBaseRagdollObject::GetGoldSrcOriginAngles(float* origin, float* angles)
{
	float flTotalMass = 0;
	vec3_t vecAddedOrigin{};
	vec3_t vecAddedAngles{};

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			vec3_t vecTempOrigin{};
			vec3_t vecTempAngles{};

			if (pRigidBody->GetGoldSrcOriginAngles(vecTempOrigin, vecTempAngles))
			{
				float mass = pRigidBody->GetMass();

				VectorMA(vecAddedOrigin, mass, vecTempOrigin, vecAddedOrigin);
				VectorMA(vecAddedAngles, mass, vecTempAngles, vecAddedAngles);

				flTotalMass += mass;
			}
		}
	}

	if (flTotalMass > 0)
	{
		float invMass = 1 / flTotalMass;

		if (origin)
		{
			VectorScale(vecAddedOrigin, invMass, origin);
		}

		if (angles)
		{
			VectorScale(vecAddedAngles, invMass, angles);
		}

		return true;
	}

	return false;
}

bool CBaseRagdollObject::CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams))
{
	return SyncCameraView(pparams, bIsThirdPersonView, callback);
}

bool CBaseRagdollObject::SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams))
{
	//Inspecting self ?
	if (!bIsThirdPersonView && g_bIsCounterStrike && GetEntityIndex() == gEngfuncs.GetLocalPlayer()->index)
	{
		if (g_iUser1 && !(*g_iUser1))
			return false;
	}
	
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsCameraView())
		{
			if (pPhysicComponent->SyncCameraView(pparams, bIsThirdPersonView, callback))
			{
				return true;
			}
		}
	}

	return false;
}

void CBaseRagdollObject::UpdateBones(entity_state_t* curstate)
{
	ClientPhysicManager()->UpdateBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());
}

bool CBaseRagdollObject::SetupBones(studiohdr_t* studiohdr, int flags)
{
	if (GetActivityType() == StudioAnimActivityType_Idle)
		return false;

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			pRigidBody->SetupBones(studiohdr, flags);
		}
	}

	return true;
}

bool CBaseRagdollObject::SetupJiggleBones(studiohdr_t* studiohdr, int flags)
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

bool CBaseRagdollObject::StudioCheckBBox(studiohdr_t* studiohdr, int *nVisible)
{
	return DispatchStudioCheckBBox(m_PhysicComponents, studiohdr, nVisible);
}

bool CBaseRagdollObject::ResetPose(entity_state_t* curstate)
{
	auto mod = GetModel();

	if (!mod)
		return false;

	auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return false;

	if (GetModel()->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());
	}

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			pRigidBody->ResetPose(studiohdr, curstate);
		}
	}

	return true;
}

void CBaseRagdollObject::ApplyBarnacle(IPhysicObject* pBarnacleObject)
{
	if (m_iBarnacleIndex)
		return;

	m_iBarnacleIndex = pBarnacleObject->GetEntityIndex();

	CPhysicObjectCreationParameter CreationParam;

	CreationParam.m_entity = GetClientEntity();
	CreationParam.m_entstate = GetClientEntityState();
	CreationParam.m_entindex = GetEntityIndex();
	CreationParam.m_model = GetModel();

	if (GetModel()->type == mod_studio)
	{
		CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(GetModel());
		CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(GetClientEntity(), GetModel());
	}

	CreationParam.m_playerindex = GetPlayerIndex();
	CreationParam.m_allowNonNativeRigidBody = true;

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			vec3_t vecZero = { 0, 0, 0 };

			pRigidBody->SetLinearVelocity(vecZero);
			pRigidBody->SetAngularVelocity(vecZero);
		}
	}

	for (const auto& pConstraintConfig : m_ConstraintConfigs)
	{
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (!(pConstraintConfigPtr->flags & PhysicConstraintFlag_Barnacle))
			continue;

		auto pConstraint = CreateConstraint(CreationParam, pConstraintConfigPtr, 0);

		if (pConstraint)
		{
			AddConstraint(CreationParam, pConstraintConfigPtr, pConstraint);
		}
	}

	for (const auto& pPhysicBehaviorConfig : m_PhysicBehaviorConfigs)
	{
		const auto pPhysicBehaviorConfigPtr = pPhysicBehaviorConfig.get();

		if (!(pPhysicBehaviorConfigPtr->flags & PhysicBehaviorFlag_Barnacle))
			continue;

		auto pPhysicBehavior = CreatePhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, 0);

		if (pPhysicBehavior)
		{
			AddPhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, pPhysicBehavior);
		}
	}
}

void CBaseRagdollObject::ReleaseFromBarnacle()
{
	if (!m_iBarnacleIndex)
		return;

	m_iBarnacleIndex = 0;

	CPhysicComponentFilters filters;

	filters.m_ConstraintFilter.m_HasWithFlags = true;
	filters.m_ConstraintFilter.m_WithFlags = PhysicConstraintFlag_Barnacle;

	filters.m_PhysicBehaviorFilter.m_HasWithFlags = true;
	filters.m_PhysicBehaviorFilter.m_WithFlags = PhysicBehaviorFlag_Barnacle;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

	FreePhysicComponentsWithFilters(filters);
}

void CBaseRagdollObject::ApplyGargantua(IPhysicObject* pGargantuaObject)
{
	//TODO
}

void CBaseRagdollObject::ReleaseFromGargantua()
{
	//TODO
}

StudioAnimActivityType CBaseRagdollObject::GetActivityType() const
{
	return m_iActivityType;
}

void CBaseRagdollObject::CalculateOverrideActivityType(const entity_state_t* entstate, StudioAnimActivityType &ActivityType) const
{
	for (const auto& AnimControlConfig : m_AnimControlConfigs)
	{
		if (AnimControlConfig->gaitsequence == 0)
		{
			if (entstate->sequence == AnimControlConfig->sequence)
			{
				ActivityType = AnimControlConfig->activity;
				return;
			}
		}
		else
		{
			if (entstate->sequence == AnimControlConfig->sequence &&
				entstate->gaitsequence == AnimControlConfig->gaitsequence)
			{
				ActivityType = AnimControlConfig->activity;
				return;
			}
		}
	}
}

int CBaseRagdollObject::GetBarnacleIndex() const
{
	return m_iBarnacleIndex;
}

int CBaseRagdollObject::GetGargantuaIndex() const
{
	return m_iGargantuaIndex;
}

bool CBaseRagdollObject::IsDebugAnimEnabled() const
{
	return m_bDebugAnimEnabled;
}

void CBaseRagdollObject::SetDebugAnimEnabled(bool bEnabled)
{
	m_bDebugAnimEnabled = bEnabled;
}

void CBaseRagdollObject::AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters& filters)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (!pPhysicComponent->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pPhysicComponent, filters))
		{
			pPhysicComponent->AddToPhysicWorld(world);
		}
	}
}

void CBaseRagdollObject::RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters)
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

void CBaseRagdollObject::FreePhysicComponentsWithFilters(const CPhysicComponentFilters& filters)
{
	DispatchFreePhysicCompoentsWithFilters(m_PhysicComponents, filters);
}

void CBaseRagdollObject::TransferOwnership(int entindex)
{
	m_entindex = entindex;
	m_entity = ClientEntityManager()->GetEntityByIndex(entindex);

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		pPhysicComponent->TransferOwnership(entindex);
	}
}

IPhysicComponent* CBaseRagdollObject::GetPhysicComponentByName(const std::string& name)
{
	return DispatchGetPhysicComponentByName(m_PhysicComponents, name);
}

IPhysicComponent* CBaseRagdollObject::GetPhysicComponentByComponentId(int id)
{
	return DispatchGetPhysicComponentByComponentId(m_PhysicComponents, id);
}

IPhysicRigidBody* CBaseRagdollObject::GetRigidBodyByName(const std::string& name)
{
	return DispatchGetRigidBodyByName(m_PhysicComponents, name);
}

IPhysicRigidBody* CBaseRagdollObject::GetRigidBodyByComponentId(int id)
{
	return DispatchGetRigidBodyByComponentId(m_PhysicComponents, id);
}

IPhysicConstraint* CBaseRagdollObject::GetConstraintByName(const std::string& name)
{
	return DispatchGetConstraintByName(m_PhysicComponents, name);
}

IPhysicConstraint* CBaseRagdollObject::GetConstraintByComponentId(int id)
{
	return DispatchGetConstraintByComponentId(m_PhysicComponents, id);
}

IPhysicBehavior* CBaseRagdollObject::GetPhysicBehaviorByName(const std::string& name)
{
	return DispatchGetPhysicBehaviorByName(m_PhysicComponents, name);
}

IPhysicBehavior* CBaseRagdollObject::GetPhysicBehaviorByComponentId(int id)
{
	return DispatchGetPhysicBehaviorByComponentId(m_PhysicComponents, id);
}

IPhysicRigidBody* CBaseRagdollObject::FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
{
	if (allowNonNativeRigidBody)
	{
		if (name.starts_with("@barnacle.") && m_iBarnacleIndex)
		{
			auto findName = name.substr(sizeof("@barnacle.") - 1);

			auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

			if (pBarnacleObject)
			{
				return pBarnacleObject->GetRigidBodyByName(findName);
			}

			return nullptr;
		}
		else if (name.starts_with("@gargantua.") && m_iGargantuaIndex)
		{
			auto findName = name.substr(sizeof("@gargantua.") - 1);

			auto pGargantuaObject = ClientPhysicManager()->GetPhysicObject(m_iGargantuaIndex);

			if (pGargantuaObject)
			{
				return pGargantuaObject->GetRigidBodyByName(findName);
			}

			return nullptr;
		}
	}

	return GetRigidBodyByName(name);
}

void CBaseRagdollObject::SetupNonKeyBones(const CPhysicObjectCreationParameter& CreationParam)
{
	if (CreationParam.m_studiohdr)
	{
		for (int i = 0; i < CreationParam.m_studiohdr->numbones; ++i)
		{
			if (std::find(m_keyBones.begin(), m_keyBones.end(), i) == m_keyBones.end())
				m_nonKeyBones.emplace_back(i);
		}
	}
}
/*
void CBaseRagdollObject::InitCameraControl(const CClientCameraControlConfig* pCameraControlConfig, CPhysicCameraControl& CameraControl)
{
	CameraControl = (*pCameraControlConfig);

	auto pRigidBody = GetRigidBodyByName(pCameraControlConfig->rigidbody);

	if (pRigidBody)
	{
		CameraControl.m_physicComponentId = pRigidBody->GetPhysicComponentId();
	}
}
*/
void CBaseRagdollObject::SaveBoneRelativeTransform(const CPhysicObjectCreationParameter& CreationParam)
{

}

void CBaseRagdollObject::AddRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidBodyConfig, IPhysicRigidBody* pRigidBody)
{
	if (!pRigidBody)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pRigidBody);

	if (CreationParam.m_studiohdr &&
		pRigidBodyConfig->boneindex >= 0 &&
		pRigidBodyConfig->boneindex < CreationParam.m_studiohdr->numbones)
	{
		m_keyBones.emplace_back(pRigidBodyConfig->boneindex);
	}
}

void CBaseRagdollObject::AddConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, IPhysicConstraint* pConstraint)
{
	if (!pConstraint)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pConstraint);
}

void CBaseRagdollObject::AddPhysicBehavior(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior* pPhysicBehavior)
{
	if (!pPhysicBehavior)
		return;

	DispatchAddPhysicComponent(m_PhysicComponents, pPhysicBehavior);
}

bool CBaseRagdollObject::UpdateActivity(StudioAnimActivityType iOldActivityType, StudioAnimActivityType iNewActivityType, entity_state_t* curstate)
{
	if (iOldActivityType == iNewActivityType)
		return false;

	//Start animation ?

	if (iOldActivityType == 0 && iNewActivityType > 0)
	{
		const auto& found = std::find_if(m_AnimControlConfigs.begin(), m_AnimControlConfigs.end(), [curstate](const std::shared_ptr<CClientAnimControlConfig>& pConfig) {

			if (pConfig->sequence == curstate->sequence)
				return true;

			return false;
		});

		if (found != m_AnimControlConfigs.end())
		{
			if (curstate->frame < (*found)->animframe)
			{
				return false;
			}
		}
	}

	m_iActivityType = iNewActivityType;
	return true;
}
