#pragma once

#include "exportfuncs.h"
#include "ClientEntityManager.h"
#include "BasePhysicManager.h"

class CBaseRagdollObject : public IRagdollObject
{
public:
	CBaseRagdollObject(const CRagdollObjectCreationParameter& CreationParam)
	{
		m_entindex = CreationParam.m_entindex;
		m_entity = CreationParam.m_entity;
		m_model = CreationParam.m_model;
		m_model_scaling = CreationParam.m_model_scaling;
		m_playerindex = CreationParam.m_playerindex;
		m_configId = CreationParam.m_pRagdollObjectConfig->configId;
		m_flags = CreationParam.m_pRagdollObjectConfig->flags;
		m_debugDrawLevel = CreationParam.m_pRagdollObjectConfig->debugDrawLevel;
	}

	~CBaseRagdollObject()
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
		if (ClientEntityManager()->IsEntityDeadPlayer(m_entity) || ClientEntityManager()->IsEntityPlayer(m_entity))
		{
			if (m_playerindex >= 0)
				return R_GetPlayerState(m_playerindex);
		}

		return &m_entity->curstate;
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
		return m_playerindex;
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
		if (GetActivityType() > StudioAnimActivityType_Idle)
			return false;

		return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
	}

	bool ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const override
	{
		if (m_debugDrawLevel > 0 && ctx->m_ragdollObjectLevel > 0 && ctx->m_ragdollObjectLevel >= m_debugDrawLevel )
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

		for (auto pConstraint : m_Constraints)
		{
			if (callback(pConstraint))
				return true;
		}

		return false;
	}

	bool Rebuild(const CClientPhysicObjectConfig* pPhysicObjectConfig) override
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

	void Update(CPhysicObjectUpdateContext* ObjectUpdateContext) override
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

			if (pAction->Update(ObjectUpdateContext))
			{
				itor++;
			}
			else
			{
				delete pAction;
				itor = m_Actions.erase(itor);
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

	bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams)) override
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

	void UpdatePose(entity_state_t* curstate) override
	{
		//Force bones update
		ClientPhysicManager()->UpdateBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());
	}

	bool SetupBones(studiohdr_t* studiohdr) override
	{
		if (GetActivityType() == StudioAnimActivityType_Idle)
			return false;

		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->SetupBones(studiohdr);
		}

		return true;
	}

	bool SetupJiggleBones(studiohdr_t* studiohdr) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->SetupJiggleBones(studiohdr);
		}

		return true;
	}

	/*bool MergeBones(studiohdr_t* studiohdr) override
	{
		return false;
	}*/

	bool ResetPose(entity_state_t* curstate) override
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

	void ApplyBarnacle(IPhysicObject* pBarnacleObject) override
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

		for (const auto& pConstraintConfig : m_BarnacleControlConfig.ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get(), 0);

			if (pConstraint)
			{
				ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

				CPhysicObjectUpdateContext ObjectUpdateContext;

				CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

				pConstraint->Update(&ComponentUpdateContext);

				m_Constraints.emplace_back(pConstraint);
			}
		}

		for (const auto& pActionConfig : m_BarnacleControlConfig.ActionConfigs)
		{
			auto pAction = CreateActionFromConfig(pActionConfig.get());

			if (pAction)
			{
				m_Actions.emplace_back(pAction);
			}
		}
	}

	void ReleaseFromBarnacle() override
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

	void ApplyGargantua(IPhysicObject* pGargantuaObject) override
	{
		//TODO
	}

	void ReleaseFromGargantua() override
	{
		//TODO
	}

	StudioAnimActivityType GetOverrideActivityType(entity_state_t* entstate) override
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

	StudioAnimActivityType GetActivityType() const override
	{
		return m_iActivityType;
	}

	int GetBarnacleIndex() const override
	{
		return m_iBarnacleIndex;
	}

	int GetGargantuaIndex() const override
	{
		return m_iGargantuaIndex;
	}

	bool IsDebugAnimEnabled() const override
	{
		return m_bDebugAnimEnabled;
	}

	void SetDebugAnimEnabled(bool bEnabled) override
	{
		m_bDebugAnimEnabled = bEnabled;
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

		for (auto pConstraint : m_Constraints)
		{
			if (!pConstraint->IsAddedToPhysicWorld(world) && CheckPhysicComponentFilters(pConstraint, filters))
			{
				pConstraint->AddToPhysicWorld(world);
			}
		}
	}

	void RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) override
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

		for (auto pAction : m_Actions)
		{
			pAction->TransferOwnership(entindex);
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
		for (auto pConstraint : m_Constraints)
		{
			if (pConstraint->GetName() == name)
			{
				return pConstraint;
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
		
		for (auto pConstraint : m_Constraints)
		{
			if (pConstraint->GetPhysicComponentId() == id)
			{
				return pConstraint;
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

	virtual void CreateRigidBodies(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto& pRigidBodyConfig : CreationParam.m_pRagdollObjectConfig->RigidBodyConfigs)
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

	virtual void CreateConstraints(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto& pConstraintConfig : CreationParam.m_pRagdollObjectConfig->ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get(), 0);

			if (pConstraint)
			{
				AddConstraint(pConstraint);
			}
		}
	}

	virtual void CreateFloaters(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto& pFloaterConfig : CreationParam.m_pRagdollObjectConfig->FloaterConfigs)
		{
			CreateFloater(CreationParam, pFloaterConfig.get());
		}
	}

	virtual IPhysicRigidBody* FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
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

	virtual void SetupNonKeyBones(const CRagdollObjectCreationParameter& CreationParam)
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

	virtual void InitCameraControl(const CClientCameraControlConfig* pCameraControlConfig, CPhysicCameraControl& CameraControl)
	{
		CameraControl = (*pCameraControlConfig);

		auto pRigidBody = GetRigidBodyByName(pCameraControlConfig->rigidbody);

		if (pRigidBody)
		{
			CameraControl.m_physicComponentId = pRigidBody->GetPhysicComponentId();
		}
	}

	virtual void SaveBoneRelativeTransform(const CRagdollObjectCreationParameter& CreationParam) {

	}

	virtual IPhysicRigidBody* CreateRigidBody(const CRagdollObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) = 0;
	virtual IPhysicConstraint* CreateConstraint(const CRagdollObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) = 0;
	virtual void CreateFloater(const CRagdollObjectCreationParameter& CreationParam, const CClientFloaterConfig* pConfig) = 0;
	virtual IPhysicAction* CreateActionFromConfig(CClientPhysicActionConfig* pActionConfig) = 0;

protected:

	void AddRigidBody(IPhysicRigidBody* pRigidBody)
	{
		ClientPhysicManager()->AddPhysicComponent(pRigidBody->GetPhysicComponentId(), pRigidBody);

		CPhysicObjectUpdateContext ObjectUpdateContext;

		CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

		pRigidBody->Update(&ComponentUpdateContext);

		m_RigidBodies.emplace_back(pRigidBody);
	}

	void AddConstraint(IPhysicConstraint* pConstraint)
	{
		ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

		CPhysicObjectUpdateContext ObjectUpdateContext;

		CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

		pConstraint->Update(&ComponentUpdateContext);

		m_Constraints.emplace_back(pConstraint);
	}

	void RebuildRigidBodies(const CRagdollObjectCreationParameter& CreationParam)
	{
		std::map<int, int> configIdToComponentIdMap;

		for (auto pRigidBody : m_RigidBodies)
		{
			configIdToComponentIdMap[pRigidBody->GetPhysicConfigId()] = pRigidBody->GetPhysicComponentId();

			ClientPhysicManager()->RemovePhysicComponent(pRigidBody->GetPhysicComponentId());
		}

		m_RigidBodies.clear();

		for (const auto& pRigidBodyConfig : CreationParam.m_pRagdollObjectConfig->RigidBodyConfigs)
		{
			auto found = configIdToComponentIdMap.find(pRigidBodyConfig->configId);

			if (found != configIdToComponentIdMap.end())
			{
				auto oldPhysicComponentId = found->second;

				auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), oldPhysicComponentId);

				if (pNewRigidBody)
				{
					AddRigidBody(pNewRigidBody);

					if (CreationParam.m_studiohdr &&
						pRigidBodyConfig->boneindex >= 0 &&
						pRigidBodyConfig->boneindex < CreationParam.m_studiohdr->numbones)
					{
						m_keyBones.emplace_back(pRigidBodyConfig->boneindex);
					}
				}
			}
			else
			{
				auto pNewRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get(), 0);

				if (pNewRigidBody)
				{
					AddRigidBody(pNewRigidBody);

					if (CreationParam.m_studiohdr &&
						pRigidBodyConfig->boneindex >= 0 &&
						pRigidBodyConfig->boneindex < CreationParam.m_studiohdr->numbones)
					{
						m_keyBones.emplace_back(pRigidBodyConfig->boneindex);
					}
				}
			}
		}
	}

	void RebuildConstraints(const CRagdollObjectCreationParameter& CreationParam)
	{
		std::map<int, int> configIdToComponentIdMap;

		for (auto pConstraint : m_Constraints)
		{
			configIdToComponentIdMap[pConstraint->GetPhysicConfigId()] = pConstraint->GetPhysicComponentId();

			ClientPhysicManager()->RemovePhysicComponent(pConstraint->GetPhysicComponentId());
		}

		m_Constraints.clear();

		for (const auto& pConstraintConfig : CreationParam.m_pRagdollObjectConfig->ConstraintConfigs)
		{
			auto found = configIdToComponentIdMap.find(pConstraintConfig->configId);

			if (found != configIdToComponentIdMap.end())
			{
				auto oldPhysicComponentId = found->second;

				auto pNewConstraint = CreateConstraint(CreationParam, pConstraintConfig.get(), oldPhysicComponentId);

				if (pNewConstraint)
				{
					AddConstraint(pNewConstraint);
				}
			}
			else
			{
				auto pNewConstraint = CreateConstraint(CreationParam, pConstraintConfig.get(), 0);

				if (pNewConstraint)
				{
					AddConstraint(pNewConstraint);
				}
			}
		}
	}

	bool UpdateActivity(StudioAnimActivityType iOldActivityType, StudioAnimActivityType iNewActivityType, entity_state_t* curstate)
	{
		if (iOldActivityType == iNewActivityType)
			return false;

		//Start death animation
		if (iOldActivityType == 0 && iNewActivityType > 0)
		{
			const auto &found = std::find_if(m_AnimControlConfigs.begin(), m_AnimControlConfigs.end(), [curstate](const std::shared_ptr<CClientAnimControlConfig>& pConfig) {

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

public:
	int m_entindex{};
	int m_playerindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	float m_model_scaling{ 1 };
	int m_flags{ PhysicObjectFlag_RagdollObject };
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	int m_configId{};

	StudioAnimActivityType m_iActivityType{ StudioAnimActivityType_Idle };
	int m_iBarnacleIndex{ 0 };
	int m_iGargantuaIndex{ 0 };

	std::vector<int> m_keyBones;
	std::vector<int> m_nonKeyBones;

	std::shared_ptr<CClientAnimControlConfig> m_IdleAnimConfig;
	std::shared_ptr<CClientAnimControlConfig> m_DebugAnimConfig;
	bool m_bDebugAnimEnabled{};
	std::vector<std::shared_ptr<CClientAnimControlConfig>> m_AnimControlConfigs;

	CClientBarnacleControlConfig m_BarnacleControlConfig;

	std::vector<IPhysicAction *> m_Actions;
	std::vector<IPhysicRigidBody *> m_RigidBodies;
	std::vector<IPhysicConstraint *> m_Constraints;

	CPhysicCameraControl m_FirstPersonViewCameraControl;
	CPhysicCameraControl m_ThirdPersonViewCameraControl;
};