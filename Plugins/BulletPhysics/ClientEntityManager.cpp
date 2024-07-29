#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "mathlib2.h"
#include "ClientEntityManager.h"

#include <unordered_map>
#include <set>

class CClientEntityManager : public IClientEntityManager
{
private:

	std::unordered_map<int, int> m_BarnacleMap;
	std::unordered_map<int, int> m_GargantuaMap;

	CPlayerDeathState m_PlayerDeathState[33];
	bool m_bIsPlayerEmitted[33];

	model_t* m_BarnacleModel;
	model_t* m_GargantuaModel;

public:
	CClientEntityManager(void)
	{
		ClearAllPlayerDeathState();
		ClearAllPlayerEmitState();

		m_BarnacleModel = 0;
		m_GargantuaModel = 0;
	}

	bool IsEntityEmitted(cl_entity_t* ent) override
	{
		if (ent->player)
		{
			return ClientEntityManager()->IsPlayerEmitted(ent->index);
		}

		return true;
	}

	bool IsEntityBarnacle(cl_entity_t* ent) override
	{
		if (ent && ent->model && ent->model->type == mod_studio)
		{
			if (m_BarnacleModel && m_BarnacleModel->needload == NL_PRESENT)
			{
				if (m_BarnacleModel == ent->model)
				{
					return (ent->curstate.sequence >= 3 && ent->curstate.sequence <= 5);
				}
			}
			else if (!strcmp(ent->model->name, "models/barnacle.mdl"))
			{
				m_BarnacleModel = ent->model;

				return (ent->curstate.sequence >= 3 && ent->curstate.sequence <= 5);
			}
		}

		return false;
	}

	bool IsEntityGargantua(cl_entity_t* ent) override
	{
		if (ent && ent->model && ent->model->type == mod_studio)
		{
			if (m_GargantuaModel && m_GargantuaModel->needload == NL_PRESENT)
			{
				if (m_GargantuaModel == ent->model)
				{
					return true;
				}
			}
			else if (!strcmp(ent->model->name, "models/garg.mdl"))
			{
				m_GargantuaModel = ent->model;

				return true;
			}
		}

		return false;
	}

	bool IsEntityWater(cl_entity_t* ent) override
	{
		if (ent && ent->model && ent->model->type == mod_brush)
		{
			if (ent->curstate.skin == -3)
			{
				return true;
			}
		}

		return false;
	}

	bool IsEntityDeadPlayer(cl_entity_t* ent) override
	{
		if (ent && ent->model && ent->model->type == mod_studio)
		{
			if (!ent->player && ent->curstate.renderfx == kRenderFxDeadPlayer &&
				ent->curstate.renderamt >= 1 &&
				ent->curstate.renderamt <= gEngfuncs.GetMaxClients())
			{
				return true;
			}
		}

		return false;
	}

	bool IsTempEntityPresent(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, TEMPENTITY* tent) override
	{
		auto pTEnt = (*ppTempEntActive);

		while (pTEnt)
		{
			if (tent == pTEnt)
			{
				return true;
			}

			pTEnt = pTEnt->next;
		}

		return false;
	}

	bool IsEntityPresent(cl_entity_t* ent) override
	{
		//Bogus index ???
		if (!ent->index)
		{
			return false;
		}

		//Bogus model ???
		if (!ent->model)
		{
			return false;
		}

		if (ent->player)
		{
			//Check playerinfo from hud_player_info because there is a chance that player entity is not transmitted.

			hud_player_info_t HudPlayerInfo = {0};
			gEngfuncs.pfnGetPlayerInfo(ent->index, &HudPlayerInfo);

			//The player is actually disconnected.
			if (!HudPlayerInfo.name)
			{
				return false;
			}

			if (ent->curstate.effects & EF_NODRAW)
			{
				return false;
			}

			return true;
		}

		//The localclient's pvs leafs may delay for 1 frame.
		if ((*cl_parsecount) >= ent->curstate.messagenum + 2)
		{
			// Check PVS state before checking transmission state
			vec3_t mins, maxs;
			mins[0] = ent->curstate.origin[0] + ent->model->mins[0];
			mins[1] = ent->curstate.origin[1] + ent->model->mins[1];
			mins[2] = ent->curstate.origin[2] + ent->model->mins[2];
			maxs[0] = ent->curstate.origin[0] + ent->model->maxs[0];
			maxs[1] = ent->curstate.origin[1] + ent->model->maxs[1];
			maxs[2] = ent->curstate.origin[2] + ent->model->maxs[2];

			if (gEngfuncs.pTriAPI->BoxInPVS(mins, maxs))
			{
				return false;
			}
		}

		if (ent->curstate.effects & EF_NODRAW)
		{
			return false;
		}

		return true;
	}

	void ClearAllPlayerDeathState() override
	{
		for (int i = 0; i < _ARRAYSIZE(m_PlayerDeathState); ++i)
		{
			ClearPlayerDeathState(i);
		}
	}

	void ClearPlayerDeathState(int entindex) override
	{
		m_PlayerDeathState[entindex].bIsDying = false;
		m_PlayerDeathState[entindex].flAnimTime = 0;
		m_PlayerDeathState[entindex].iSequence = 0;
		m_PlayerDeathState[entindex].iModelIndex = 0;
		memset(m_PlayerDeathState[entindex].szModelName, 0, sizeof(m_PlayerDeathState[entindex].szModelName));
		VectorClear(m_PlayerDeathState[entindex].vecAngles);
		VectorClear(m_PlayerDeathState[entindex].vecOrigin);
	}

	/*
		Purpose: Find the player that is playing death animation right at the given origin with give angles, sequence and body. returns entindex if found. otherwise return 0.
	*/
	int FindDyingPlayer(const char* modelname, vec3_t origin, vec3_t angles, int sequence, int body) override
	{
		for (int i = 1; i < _ARRAYSIZE(m_PlayerDeathState); ++i)
		{
			if (m_PlayerDeathState[i].bIsDying &&
				m_PlayerDeathState[i].iSequence == sequence &&
				m_PlayerDeathState[i].iBody == body &&
				0 == strcmp(m_PlayerDeathState[i].szModelName, modelname))
			{
				if (VectorDistance(origin, m_PlayerDeathState[i].vecOrigin) < 128 &&
					fabs(angles[0] - m_PlayerDeathState[i].vecAngles[0]) < 10 &&
					fabs(angles[1] - m_PlayerDeathState[i].vecAngles[1]) < 10)
				{
					return i;
				}
			}
		}
		return 0;
	}

	void SetPlayerDeathState(int entindex, entity_state_t* pplayer, model_t* model) override
	{
		m_PlayerDeathState[entindex].bIsDying = true;
		m_PlayerDeathState[entindex].flClientTime = gEngfuncs.GetClientTime();
		m_PlayerDeathState[entindex].flAnimTime = pplayer->animtime;
		m_PlayerDeathState[entindex].iSequence = pplayer->sequence;
		m_PlayerDeathState[entindex].iBody = pplayer->body;

		if (m_PlayerDeathState[entindex].iModelIndex != EngineGetModelIndex(model))
		{
			m_PlayerDeathState[entindex].iModelIndex = EngineGetModelIndex(model);
			strncpy(m_PlayerDeathState[entindex].szModelName, model->name, sizeof(m_PlayerDeathState[entindex].szModelName));
		}

		VectorCopy(pplayer->angles, m_PlayerDeathState[entindex].vecAngles);
		VectorCopy(pplayer->origin, m_PlayerDeathState[entindex].vecOrigin);

		//fix angle?
		if (m_PlayerDeathState[entindex].vecAngles[0] > 180)
			m_PlayerDeathState[entindex].vecAngles[0] -= 360;

		if (m_PlayerDeathState[entindex].vecAngles[0] < -180)
			m_PlayerDeathState[entindex].vecAngles[0] += 360;

		if (m_PlayerDeathState[entindex].vecAngles[1] > 180)
			m_PlayerDeathState[entindex].vecAngles[1] -= 360;

		if (m_PlayerDeathState[entindex].vecAngles[1] < -180)
			m_PlayerDeathState[entindex].vecAngles[1] += 360;
	}

	void SetPlayerEmitted(int entindex) override
	{
		m_bIsPlayerEmitted[entindex] = true;
	}

	bool IsPlayerEmitted(int entindex) override
	{
		return m_bIsPlayerEmitted[entindex];
	}

	void ClearAllPlayerEmitState() override
	{
		for (int i = 0; i < _ARRAYSIZE(m_bIsPlayerEmitted); ++i)
		{
			m_bIsPlayerEmitted[i] = false;
		}
	}

	void FreePlayerForBarnacle(int entindex) override
	{
		for (auto itor = m_BarnacleMap.begin(); itor != m_BarnacleMap.end(); )
		{
			if (itor->second == entindex)
			{
				itor->second = 0;
				return;
			}

			itor++;
		}
	}

	void NewMap(void) override
	{
		m_BarnacleModel = NULL;
		m_BarnacleMap.clear();

		m_GargantuaModel = NULL;
		m_GargantuaMap.clear();

		ClearAllPlayerDeathState();
	}

	void AddBarnacle(int entindex, int playerindex) override
	{
		auto itor = m_BarnacleMap.find(entindex);
		if (itor == m_BarnacleMap.end())
		{
			m_BarnacleMap[entindex] = playerindex;
		}
		else if (itor->second == 0 && playerindex != 0)
		{
			itor->second = playerindex;
		}
	}

	void AddGargantua(int entindex, int playerindex)
	{
		auto itor = m_GargantuaMap.find(entindex);
		if (itor == m_GargantuaMap.end())
		{
			m_GargantuaMap[entindex] = playerindex;
		}
		else if (itor->second == 0 && playerindex != 0)
		{
			itor->second = playerindex;
		}
	}

	/*
		Purpose: Find the player entity that is caught by the barnacle (which is specified by the given entindex)
	*/
	cl_entity_t* FindPlayerForBarnacle(int entindex)
	{
		auto itor = m_BarnacleMap.find(entindex);
		if (itor != m_BarnacleMap.end())
		{
			if (itor->second != 0)
			{
				auto playerEntity = gEngfuncs.GetEntityByIndex(itor->second);
				if (playerEntity &&
					playerEntity->player &&
					StudioGetSequenceActivityType(playerEntity->model, &playerEntity->curstate) == 2)
				{
					return playerEntity;
				}
			}
		}

		return NULL;
	}

	/*
		Purpose: Find the player entity that is caught by the gargantua (which is specified by the given entindex)
	*/
	cl_entity_t* FindPlayerForGargantua(int entindex)
	{
		auto itor = m_GargantuaMap.find(entindex);
		if (itor != m_GargantuaMap.end())
		{
			if (itor->second != 0)
			{
				auto playerEntity = gEngfuncs.GetEntityByIndex(itor->second);
				if (playerEntity &&
					playerEntity->player &&
					StudioGetSequenceActivityType(playerEntity->model, &playerEntity->curstate) == 2)
				{
					return playerEntity;
				}
			}
		}

		return NULL;
	}

	/*
		Purpose: Find the barnacle entity that is catching the player
	*/
	cl_entity_t* FindBarnacleForPlayer(entity_state_t* player)
	{
		for (auto itor = m_BarnacleMap.begin(); itor != m_BarnacleMap.end(); itor++)
		{
			auto ent = gEngfuncs.GetEntityByIndex(itor->first);
			if (IsEntityBarnacle(ent))
			{
				if (fabs(player->origin[0] - ent->origin[0]) < 1 &&
					fabs(player->origin[1] - ent->origin[1]) < 1 &&
					player->origin[2] < ent->origin[2] + 16)
				{
					itor->second = player->number;
					return ent;
				}
			}
		}

		return NULL;
	}

	/*
		Purpose: Find the gargantua entity that is catching the player
	*/
	cl_entity_t* FindGargantuaForPlayer(entity_state_t* player)
	{
		for (auto itor = m_GargantuaMap.begin(); itor != m_GargantuaMap.end(); itor++)
		{
			auto ent = gEngfuncs.GetEntityByIndex(itor->first);
			if (IsEntityGargantua(ent) && ent->curstate.sequence == 15)
			{
				if (VectorDistance(player->origin, ent->origin) < 128)
				{
					itor->second = player->number;
					return ent;
				}
			}
		}

		return NULL;
	}

};

static CClientEntityManager g_ClientEntityManager;

IClientEntityManager* ClientEntityManager()
{
	return &g_ClientEntityManager;
}