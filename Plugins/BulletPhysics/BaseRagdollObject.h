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

	void UpdatePose(entity_state_t* curstate) override
	{
		//Force bones update
		ClientPhysicManager()->UpdateBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());
	}

	bool SyncFirstPersonView(struct ref_params_s* pparams) override
	{
		//TODO

		return false;
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

	void Update(CPhysicObjectUpdateContext* ctx) override
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
			ctx->m_bActivityChanged = true;

			//Transform from whatever to barnacle
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
				ResetPose(playerState);

				ctx->m_bRigidbodyPoseChanged = true;
			}
		}

		if (!ClientEntityManager()->IsEntityInVisibleList(GetClientEntity()))
		{
			UpdatePose(GetClientEntityState());

			ctx->m_bRigidbodyPoseUpdated = true;
		}
	}

	bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson) override
	{
		if (GetActivityType() != 0)
		{
			if (bIsThirdPerson)
			{
				vec3_t vecSavedSimOrgigin;
				vec3_t vecSavedOrigin;
				vec3_t vecSavedCurStateOrigin;
				vec3_t vecNewOrigin;

				VectorCopy(pparams->simorg, vecSavedSimOrgigin);
				VectorCopy(GetClientEntity()->origin, vecSavedOrigin);
				VectorCopy(GetClientEntity()->curstate.origin, vecSavedCurStateOrigin);

				GetOrigin(vecNewOrigin);

				VectorCopy(vecNewOrigin, GetClientEntity()->origin);
				VectorCopy(vecNewOrigin, GetClientEntity()->curstate.origin);
				VectorCopy(vecNewOrigin, pparams->simorg);

				gExportfuncs.V_CalcRefdef(pparams);

				VectorCopy(vecSavedCurStateOrigin, GetClientEntity()->curstate.origin);
				VectorCopy(vecSavedOrigin, GetClientEntity()->origin);
				VectorCopy(vecSavedSimOrgigin, pparams->simorg);

				return true;
			}
			else
			{
				if (g_bIsCounterStrike && GetEntityIndex() == gEngfuncs.GetLocalPlayer()->index)
				{
					if (g_iUser1 && !(*g_iUser1))
						return false;
				}

				vec3_t vecSavedSimOrgigin;
				vec3_t vecSavedClientViewAngles;
				int iSavedHealth = pparams->health;

				VectorCopy(pparams->simorg, vecSavedSimOrgigin);
				VectorCopy(pparams->cl_viewangles, vecSavedClientViewAngles);

				SyncFirstPersonView(pparams);

				gExportfuncs.V_CalcRefdef(pparams);

				VectorCopy(vecSavedSimOrgigin, pparams->simorg);
				VectorCopy(vecSavedClientViewAngles, pparams->cl_viewangles);
				pparams->health = iSavedHealth;

				return true;
			}
		}

		return false;
	}

private:

	bool UpdateActivity(int iOldActivityType, int iNewActivityType, entity_state_t* curstate)
	{
		if (iOldActivityType == iNewActivityType)
			return false;

		//Start death animation
		if (iOldActivityType == 0 && iNewActivityType > 0)
		{
			auto found = std::find_if(m_AnimControlConfigs.begin(), m_AnimControlConfigs.end(), [curstate](const CClientRagdollAnimControlConfig& Config) {

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
	vec3_t m_vecFirstPersonAngleOffset{};

	CClientRagdollAnimControlConfig m_IdleAnimConfig;
	std::vector<CClientRagdollAnimControlConfig> m_AnimControlConfigs;

	std::vector<std::shared_ptr<CClientConstraintConfig>> m_BarnacleConstraintConfigs;
	std::vector<std::shared_ptr<CClientPhysicActionConfig>> m_BarnacleActionConfigs;
};