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
		m_flags = CreationParam.m_pRagdollObjectConfig->flags;
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

			delete pConstraint;
		}

		m_Constraints.clear();

		for (auto pRigidBody : m_RigidBodies)
		{
			ClientPhysicManager()->RemovePhysicComponent(pRigidBody->GetPhysicComponentId());

			delete pRigidBody;
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

	int GetPlayerIndex() const override
	{
		return m_playerindex;
	}

	int GetObjectFlags() const override
	{
		return m_flags;
	}

	bool IsClientEntityNonSolid() const override
	{
		if (GetActivityType() > 0)
			return false;

		return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
	}

	void UpdatePose(entity_state_t* curstate) override
	{
		//Force bones update
		ClientPhysicManager()->UpdateBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());
	}

	bool SetupBones(studiohdr_t* studiohdr) override
	{
		if (GetActivityType() == 0)
			return false;

		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->SetupBones(studiohdr);
		}

		return true;
	}

	bool ResetPose(entity_state_t* curstate) override
	{
		auto mod = GetModel();

		if (!mod)
			return false;

		auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);

		if (!studiohdr)
			return false;

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
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get());

			if (pConstraint)
			{
				ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

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

	int GetOverrideActivityType(entity_state_t* entstate) override
	{
		for (const auto& AnimControlConfig : m_AnimControlConfigs)
		{
			if (entstate->sequence == AnimControlConfig.sequence)
			{
				return AnimControlConfig.activity;
			}
		}
		return 0;
	}

	int GetActivityType() const override
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

	bool SetupJiggleBones(studiohdr_t* studiohdr) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->SetupJiggleBones(studiohdr);
		}

		return true;
	}

	void Update(CPhysicObjectUpdateContext* ObjectUpdateContext) override
	{
		auto playerState = R_GetPlayerState(m_playerindex);

		int iOldActivityType = GetActivityType();

		int iNewActivityType = StudioGetSequenceActivityType(m_model, playerState);

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
		if (GetActivityType() != 0)
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

	void CreateRigidBodies(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto& pRigidBodyConfig : CreationParam.m_pRagdollObjectConfig->RigidBodyConfigs)
		{
			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get());

			if (pRigidBody)
			{
				ClientPhysicManager()->AddPhysicComponent(pRigidBody->GetPhysicComponentId(), pRigidBody);

				CPhysicObjectUpdateContext ObjectUpdateContext(GetEntityIndex(), this);

				CPhysicComponentUpdateContext ComponentUpdateContext(&ObjectUpdateContext);

				pRigidBody->Update(&ComponentUpdateContext);

				m_RigidBodies.emplace_back(pRigidBody);

				if (pRigidBodyConfig->boneindex >= 0)
				{
					m_keyBones.emplace_back(pRigidBodyConfig->boneindex);
				}
			}
		}
	}

	void CreateConstraints(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto& pConstraintConfig : CreationParam.m_pRagdollObjectConfig->ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get());

			if (pConstraint)
			{
				ClientPhysicManager()->AddPhysicComponent(pConstraint->GetPhysicComponentId(), pConstraint);

				m_Constraints.emplace_back(pConstraint);
			}
		}
	}

	void CreateFloaters(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto& pFloaterConfig : CreationParam.m_pRagdollObjectConfig->FloaterConfigs)
		{
			CreateFloater(CreationParam, pFloaterConfig.get());
		}
	}

	IPhysicRigidBody* FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
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

	virtual IPhysicRigidBody* CreateRigidBody(const CRagdollObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig) = 0;
	virtual IPhysicConstraint* CreateConstraint(const CRagdollObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig) = 0;
	virtual void CreateFloater(const CRagdollObjectCreationParameter& CreationParam, const CClientFloaterConfig* pConfig) = 0;
	virtual IPhysicAction* CreateActionFromConfig(CClientPhysicActionConfig* pActionConfig) = 0;

private:

	bool UpdateActivity(int iOldActivityType, int iNewActivityType, entity_state_t* curstate)
	{
		if (iOldActivityType == iNewActivityType)
			return false;

		//Start death animation
		if (iOldActivityType == 0 && iNewActivityType > 0)
		{
			auto found = std::find_if(m_AnimControlConfigs.begin(), m_AnimControlConfigs.end(), [curstate](const CClientAnimControlConfig& Config) {

				if (Config.sequence == curstate->sequence)
					return true;

				return false;
			});

			if (found != m_AnimControlConfigs.end())
			{
				if (curstate->frame < found->frame)
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

	int m_iActivityType{ 0 };
	int m_iBarnacleIndex{ 0 };
	int m_iGargantuaIndex{ 0 };
	std::vector<int> m_keyBones;
	std::vector<int> m_nonKeyBones;

	CClientAnimControlConfig m_IdleAnimConfig;
	std::vector<CClientAnimControlConfig> m_AnimControlConfigs;

	CClientBarnacleControlConfig m_BarnacleControlConfig;

	std::vector<IPhysicAction *> m_Actions;
	std::vector<IPhysicRigidBody *> m_RigidBodies;
	std::vector<IPhysicConstraint *> m_Constraints;
};