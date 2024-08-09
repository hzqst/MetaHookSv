#pragma once

#include "exportfuncs.h"
#include "ClientEntityManager.h"
#include "BasePhysicManager.h"

class CBaseRagdollObject : public IRagdollObject
{
public:
	CBaseRagdollObject(const CRagdollObjectCreationParameter& CreationParam)
	{

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

	bool IsBarnacle() const override
	{
		return m_bIsBarnacle;
	}

	int GetPlayerIndex() const override
	{
		return m_playerindex;
	}

	int GetObjectFlags() const override
	{
		return PhysicObjectFlag_RagdollObject;
	}

	void UpdatePose(entity_state_t* curstate) override
	{
		//Force bones update
		ClientPhysicManager()->UpdateBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());
	}

	void ApplyBarnacle(cl_entity_t* barnacle_entity) override
	{
		//TODO

	}

	void ApplyGargantua(cl_entity_t* gargantua_entity) override
	{
		//TODO

	}

	bool SyncFirstPersonView(cl_entity_t* ent, struct ref_params_s* pparams) override
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

		if (UpdateActivity(iOldActivityType, iNewActivityType, playerState))
		{
			ctx->bActivityChanged = true;

			//Transform from whatever to barnacle
			if (iNewActivityType == 2 && iOldActivityType != 2)
			{
				auto BarnacleEntity = ClientEntityManager()->FindBarnacleForPlayer(playerState);

				if (BarnacleEntity)
				{
					ApplyBarnacle(BarnacleEntity);
				}
				else
				{
					auto GargantuaEntity = ClientEntityManager()->FindGargantuaForPlayer(playerState);

					if (GargantuaEntity)
					{
						ApplyGargantua(GargantuaEntity);
					}
				}
			}

			//Transformed from death or barnacle to idle state.
			else if (iOldActivityType > 0 && iNewActivityType == 0)
			{
				ResetPose(playerState);

				ctx->bRigidbodyPoseChanged = true;
			}
		}

		if (!ClientEntityManager()->IsEntityInVisibleList(GetClientEntity()))
		{
			UpdatePose(GetClientEntityState());

			ctx->bRigidbodyPoseUpdated = true;
		}
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
	bool m_bIsBarnacle{};

	int m_iActivityType{};
	int m_iBarnacleIndex{};
	int m_iGargantuaIndex{};
	std::vector<int> m_keyBones;
	std::vector<int> m_nonKeyBones;
	vec3_t m_vecFirstPersonAngleOffset{};
	CClientRagdollAnimControlConfig m_IdleAnimConfig;
	std::vector<CClientRagdollAnimControlConfig> m_AnimControlConfigs;

	bool m_bUpdateKinematic{};
	float m_flNextUpdateKinematicTime{};
};