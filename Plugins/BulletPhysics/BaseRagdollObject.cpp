#include "BaseRagdollObject.h"
#include "BasePhysicManager.h"
#include "privatehook.h"

CBaseRagdollObject::CBaseRagdollObject(const CRagdollObjectCreationParameter& CreationParam)
{
	m_entindex = CreationParam.m_entindex;
	m_entity = CreationParam.m_entity;
	m_model = CreationParam.m_model;
	m_model_scaling = CreationParam.m_model_scaling;
	m_playerindex = CreationParam.m_playerindex;
	m_configId = CreationParam.m_pRagdollObjectConfig->configId;
	m_flags = CreationParam.m_pRagdollObjectConfig->flags;
	m_debugDrawLevel = CreationParam.m_pRagdollObjectConfig->debugDrawLevel;

	m_RigidBodyConfigs = CreationParam.m_pRagdollObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = CreationParam.m_pRagdollObjectConfig->ConstraintConfigs;
	m_ActionConfigs = CreationParam.m_pRagdollObjectConfig->ActionConfigs;
}

CBaseRagdollObject::~CBaseRagdollObject()
{
	for (auto pAction : m_Actions)
	{
		delete pAction;
	}

	m_Actions.clear();

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

int CBaseRagdollObject::GetRigidBodyCount() const
{
	return (int)m_RigidBodies.size();
}

IPhysicRigidBody* CBaseRagdollObject::GetRigidBodyByIndex(int index) const
{
	return m_RigidBodies.at(index);
}

bool CBaseRagdollObject::EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		if (callback(pRigidBody))
			return true;
	}

	for (auto pConstraint : m_Constraints)
	{
		if (callback(pConstraint))
			return true;
	}

	return false;
}

bool CBaseRagdollObject::Rebuild(const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	if (pPhysicObjectConfig->type != PhysicObjectType_RagdollObject)
	{
		gEngfuncs.Con_DPrintf("Rebuild: pPhysicObjectConfig->type mismatch!\n");
		return false;
	}

	auto pRagdollObjectConfig = (CClientRagdollObjectConfig*)pPhysicObjectConfig;

	CRagdollObjectCreationParameter CreationParam;

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

	CreationParam.m_pRagdollObjectConfig = pRagdollObjectConfig;

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

	m_RigidBodyConfigs = pRagdollObjectConfig->RigidBodyConfigs;
	m_ConstraintConfigs = pRagdollObjectConfig->ConstraintConfigs;
	m_ActionConfigs = pRagdollObjectConfig->ActionConfigs;

	CPhysicComponentFilters filters;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);

	m_keyBones.clear();
	m_nonKeyBones.clear();

	SaveBoneRelativeTransform(CreationParam);

	RebuildRigidBodies(CreationParam);

	RebuildConstraints(CreationParam);

	SetupNonKeyBones(CreationParam);

	return true;
}

void CBaseRagdollObject::Update(CPhysicObjectUpdateContext* ObjectUpdateContext)
{
	auto playerState = GetClientEntityState();

	if (m_bDebugAnimEnabled && m_DebugAnimConfig)
	{
		playerState->sequence = m_DebugAnimConfig->sequence;
		playerState->gaitsequence = m_DebugAnimConfig->gaitsequence;
		playerState->frame = m_DebugAnimConfig->frame;
		playerState->framerate = 0;

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

	if (iNewActivityType == 0)
	{
		iNewActivityType = GetOverrideActivityType(playerState);
	}

	if (m_playerindex == m_entindex)
	{
		if (iNewActivityType == 1)
		{
			ClientEntityManager()->SetPlayerDeathState(m_playerindex, playerState, m_model);
		}
		else
		{
			ClientEntityManager()->ClearPlayerDeathState(m_playerindex);
		}
	}

	if (m_iBarnacleIndex && iNewActivityType != 2)
	{
		ReleaseFromBarnacle();
	}

	if (m_iGargantuaIndex && iNewActivityType != 2)
	{
		ReleaseFromGargantua();
	}

	if (UpdateActivity(iOldActivityType, iNewActivityType, playerState))
	{
		ObjectUpdateContext->m_bActivityChanged = true;

		//Transformed from whatever to barnacle
		if (iNewActivityType == 2 && iOldActivityType != 2)
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
		ObjectUpdateContext->m_bRigidbodyUpdatePoseRequired = true;
	}

	for (auto pRigidBody : m_RigidBodies)
	{
		CPhysicComponentUpdateContext ComponentUpdateContext(ObjectUpdateContext);

		pRigidBody->Update(&ComponentUpdateContext);
	}

	for (auto pConstraint : m_Constraints)
	{
		CPhysicComponentUpdateContext ComponentUpdateContext(ObjectUpdateContext);

		pConstraint->Update(&ComponentUpdateContext);
	}

	for (auto itor = m_Actions.begin(); itor != m_Actions.end();)
	{
		auto pAction = (*itor);

		CPhysicComponentUpdateContext ComponentUpdateContext(ObjectUpdateContext);

		pAction->Update(&ComponentUpdateContext);

		if (ComponentUpdateContext.m_bShouldFree)
		{
			ClientPhysicManager()->FreePhysicComponent(pAction);

			itor = m_Actions.erase(itor);
		}
		else
		{
			itor++;
		}
	}

	if (ObjectUpdateContext->m_bRigidbodyResetPoseRequired && !ObjectUpdateContext->m_bRigidbodyPoseChanged)
	{
		ResetPose(playerState);

		ObjectUpdateContext->m_bRigidbodyPoseChanged = true;
	}

	if (ObjectUpdateContext->m_bRigidbodyUpdatePoseRequired && !ObjectUpdateContext->m_bRigidbodyPoseUpdated)
	{
		UpdatePose(playerState);

		ObjectUpdateContext->m_bRigidbodyPoseUpdated = true;
	}
}

bool CBaseRagdollObject::CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams))
{
	if (GetActivityType() != StudioAnimActivityType_Idle)
	{
		if (bIsThirdPerson)
		{
			return SyncThirdPersonView(pparams, callback);
		}
		else
		{
			if (g_bIsCounterStrike && GetEntityIndex() == gEngfuncs.GetLocalPlayer()->index)
			{
				if (g_iUser1 && !(*g_iUser1))
					return false;
			}

			return SyncFirstPersonView(pparams, callback);
		}
	}

	return false;
}

void CBaseRagdollObject::UpdatePose(entity_state_t* curstate)
{
	//Force bones update
	ClientPhysicManager()->UpdateBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());
}

bool CBaseRagdollObject::SetupBones(studiohdr_t* studiohdr)
{
	if (GetActivityType() == StudioAnimActivityType_Idle)
		return false;

	for (auto pRigidBody : m_RigidBodies)
	{
		pRigidBody->SetupBones(studiohdr);
	}

	return true;
}

bool CBaseRagdollObject::SetupJiggleBones(studiohdr_t* studiohdr)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		pRigidBody->SetupJiggleBones(studiohdr);
	}

	return true;
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
		ClientPhysicManager()->SetupBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());

	for (auto pRigidBody : m_RigidBodies)
	{
		pRigidBody->ResetPose(studiohdr, curstate);
	}

	return true;
}

void CBaseRagdollObject::ApplyBarnacle(IPhysicObject* pBarnacleObject)
{
	if (m_iBarnacleIndex)
		return;

	m_iBarnacleIndex = pBarnacleObject->GetEntityIndex();

	CRagdollObjectCreationParameter CreationParam;

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

	for (auto pRigidBody : m_RigidBodies)
	{
		vec3_t vecZero = { 0, 0, 0 };

		pRigidBody->SetLinearVelocity(vecZero);
		pRigidBody->SetAngularVelocity(vecZero);
	}

	for (const auto& pConstraintConfig : m_ConstraintConfigs)
	{
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (!(pConstraintConfigPtr->flags & PhysicConstraintFlag_Barnacle))
			continue;

		auto pConstraint = CreateConstraint(CreationParam, pConstraintConfigPtr, 0);

		if (pConstraint)
		{
			ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

			CPhysicObjectUpdateContext ObjectUpdateContext;

			CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

			pConstraint->Update(&ComponentUpdateContext);

			m_Constraints.emplace_back(pConstraint);
		}
	}

	for (const auto& pActionConfig : m_ActionConfigs)
	{
		const auto pActionConfigPtr = pActionConfig.get();

		if (!(pActionConfigPtr->flags & PhysicActionFlag_Barnacle))
			continue;

		auto pAction = CreateAction(CreationParam, pActionConfigPtr, 0);

		if (pAction)
		{
			m_Actions.emplace_back(pAction);
		}
	}
}

void CBaseRagdollObject::ReleaseFromBarnacle()
{
	if (!m_iBarnacleIndex)
		return;

	m_iBarnacleIndex = 0;

	FreePhysicActionsWithFilters(PhysicActionFlag_Barnacle, 0);

	CPhysicComponentFilters filters;

	filters.m_HasWithConstraintFlags = true;
	filters.m_WithConstraintFlags = PhysicConstraintFlag_Barnacle;

	ClientPhysicManager()->RemovePhysicComponentsFromWorld(this, filters);
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

StudioAnimActivityType CBaseRagdollObject::GetOverrideActivityType(entity_state_t* entstate) const
{
	for (const auto& AnimControlConfig : m_AnimControlConfigs)
	{
		if (AnimControlConfig->gaitsequence == 0)
		{
			if (entstate->sequence == AnimControlConfig->sequence)
			{
				return AnimControlConfig->activity;
			}
		}
		else
		{
			if (entstate->sequence == AnimControlConfig->sequence &&
				entstate->gaitsequence == AnimControlConfig->gaitsequence)
			{
				return AnimControlConfig->activity;
			}
		}
	}

	return StudioAnimActivityType_Idle;
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
	for (auto pRigidBody : m_RigidBodies)
	{
		if (!pRigidBody->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pRigidBody, filters))
		{
			pRigidBody->AddToPhysicWorld(world);
		}
	}

	for (auto pConstraint : m_Constraints)
	{
		if (!pConstraint->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pConstraint, filters))
		{
			pConstraint->AddToPhysicWorld(world);
		}
	}
}

void CBaseRagdollObject::RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters)
{
	for (auto pConstraint : m_Constraints)
	{
		if (pConstraint->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pConstraint, filters))
		{
			pConstraint->RemoveFromPhysicWorld(world);
		}
	}

	for (auto pRigidBody : m_RigidBodies)
	{
		if (pRigidBody->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pRigidBody, filters))
		{
			pRigidBody->RemoveFromPhysicWorld(world);
		}
	}
}

void CBaseRagdollObject::TransferOwnership(int entindex)
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

	for (auto pAction : m_Actions)
	{
		pAction->TransferOwnership(entindex);
	}
}

IPhysicComponent* CBaseRagdollObject::GetPhysicComponentByName(const std::string& name)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		if (pRigidBody->GetName() == name)
		{
			return pRigidBody;
		}
	}
	for (auto pConstraint : m_Constraints)
	{
		if (pConstraint->GetName() == name)
		{
			return pConstraint;
		}
	}

	return nullptr;
}

IPhysicComponent* CBaseRagdollObject::GetPhysicComponentByComponentId(int id)
{
	for (auto pRigidBody : m_RigidBodies)
	{
		if (pRigidBody->GetPhysicComponentId() == id)
		{
			return pRigidBody;
		}
	}

	for (auto pConstraint : m_Constraints)
	{
		if (pConstraint->GetPhysicComponentId() == id)
		{
			return pConstraint;
		}
	}

	return nullptr;
}

IPhysicRigidBody* CBaseRagdollObject::GetRigidBodyByName(const std::string& name)
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

IPhysicRigidBody* CBaseRagdollObject::GetRigidBodyByComponentId(int id)
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

IPhysicConstraint* CBaseRagdollObject::GetConstraintByName(const std::string& name)
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

IPhysicConstraint* CBaseRagdollObject::GetConstraintByComponentId(int id)
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

void CBaseRagdollObject::CreateRigidBodies(const CRagdollObjectCreationParameter& CreationParam)
{
	for (const auto& pRigidBodyConfig : m_RigidBodyConfigs)
	{
		auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), 0);

		if (pRigidBody)
		{
			AddRigidBody(pRigidBody);

			if (CreationParam.m_studiohdr &&
				pRigidBodyConfig->boneindex >= 0 &&
				pRigidBodyConfig->boneindex < CreationParam.m_studiohdr->numbones)
			{
				m_keyBones.emplace_back(pRigidBodyConfig->boneindex);
			}
		}
	}
}

void CBaseRagdollObject::CreateConstraints(const CRagdollObjectCreationParameter& CreationParam)
{
	for (const auto& pConstraintConfig : m_ConstraintConfigs)
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

void CBaseRagdollObject::CreateActions(const CRagdollObjectCreationParameter& CreationParam)
{
	for (const auto& pActionConfig : m_ActionConfigs)
	{
		const auto pActionConfigPtr = pActionConfig.get();

		if (pActionConfigPtr->flags & PhysicActionFlag_NonNative)
			continue;

		CreateAction(CreationParam, pActionConfigPtr, 0);
	}
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

void CBaseRagdollObject::SetupNonKeyBones(const CRagdollObjectCreationParameter& CreationParam)
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

void CBaseRagdollObject::InitCameraControl(const CClientCameraControlConfig* pCameraControlConfig, CPhysicCameraControl& CameraControl)
{
	CameraControl = (*pCameraControlConfig);

	auto pRigidBody = GetRigidBodyByName(pCameraControlConfig->rigidbody);

	if (pRigidBody)
	{
		CameraControl.m_physicComponentId = pRigidBody->GetPhysicComponentId();
	}
}

void CBaseRagdollObject::SaveBoneRelativeTransform(const CRagdollObjectCreationParameter& CreationParam) {

}

void CBaseRagdollObject::AddRigidBody(IPhysicRigidBody* pRigidBody)
{
	ClientPhysicManager()->AddPhysicComponent(pRigidBody->GetPhysicComponentId(), pRigidBody);

	CPhysicObjectUpdateContext ObjectUpdateContext;

	CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

	pRigidBody->Update(&ComponentUpdateContext);

	m_RigidBodies.emplace_back(pRigidBody);
}

void CBaseRagdollObject::AddConstraint(IPhysicConstraint* pConstraint)
{
	ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

	CPhysicObjectUpdateContext ObjectUpdateContext;

	CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

	pConstraint->Update(&ComponentUpdateContext);

	m_Constraints.emplace_back(pConstraint);
}

void CBaseRagdollObject::RebuildRigidBodies(const CRagdollObjectCreationParameter& CreationParam)
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

				if (CreationParam.m_studiohdr &&
					pRigidBodyConfigPtr->boneindex >= 0 &&
					pRigidBodyConfigPtr->boneindex < CreationParam.m_studiohdr->numbones)
				{
					m_keyBones.emplace_back(pRigidBodyConfigPtr->boneindex);
				}
			}
		}
		else
		{
			auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfigPtr, 0);

			if (pNewRigidBody)
			{
				AddRigidBody(pNewRigidBody);

				if (CreationParam.m_studiohdr &&
					pRigidBodyConfigPtr->boneindex >= 0 &&
					pRigidBodyConfigPtr->boneindex < CreationParam.m_studiohdr->numbones)
				{
					m_keyBones.emplace_back(pRigidBodyConfigPtr->boneindex);
				}
			}
		}
	}
}

void CBaseRagdollObject::RebuildConstraints(const CRagdollObjectCreationParameter& CreationParam)
{
	std::map<int, int> configIdToComponentIdMap;

	for (auto pConstraint : m_Constraints)
	{
		configIdToComponentIdMap[pConstraint->GetPhysicConfigId()] = pConstraint->GetPhysicComponentId();

		ClientPhysicManager()->RemovePhysicComponent(pConstraint->GetPhysicComponentId());
	}

	m_Constraints.clear();

	for (const auto& pConstraintConfig : m_ConstraintConfigs)
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

bool CBaseRagdollObject::UpdateActivity(StudioAnimActivityType iOldActivityType, StudioAnimActivityType iNewActivityType, entity_state_t* curstate)
{
	if (iOldActivityType == iNewActivityType)
		return false;

	//Start death animation
	if (iOldActivityType == 0 && iNewActivityType > 0)
	{
		const auto& found = std::find_if(m_AnimControlConfigs.begin(), m_AnimControlConfigs.end(), [curstate](const std::shared_ptr<CClientAnimControlConfig>& pConfig) {

			if (pConfig->sequence == curstate->sequence)
				return true;

			return false;
			});

		if (found != m_AnimControlConfigs.end())
		{
			if (curstate->frame < (*found)->frame)
			{
				return false;
			}
		}
	}

	m_iActivityType = iNewActivityType;
	return true;
}
