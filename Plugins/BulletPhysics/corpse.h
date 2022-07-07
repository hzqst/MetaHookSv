#pragma once

#include <r_efx.h>
#include <cl_entity.h>
#include <com_model.h>
#include <unordered_map>
#include <set>

typedef struct
{
	bool bIsDying;
	float flClientTime;
	float flAnimTime;
	int iSequence;
	int iBody;
	int iModelIndex;
	char szModelName[64];
	vec3_t vecOrigin;
	vec3_t vecAngles;
}PlayerDying_t;

class CorpseManager
{
public:
	CorpseManager(void);

	void AddGargantua(int entindex, int playerindex);
	void AddBarnacle(int entindex, int playerindex);
	void NewMap(void);
	cl_entity_t *FindGargantuaForPlayer(entity_state_t *entstate);
	cl_entity_t *FindBarnacleForPlayer(entity_state_t *entstate);
	cl_entity_t *FindPlayerForBarnacle(int entindex);
	cl_entity_t *FindPlayerForGargantua(int entindex);
	void FreePlayerForBarnacle(int entindex);

	void ClearPlayerDying(int entindex);
	void ClearAllPlayerDying();
	void SetPlayerDying(int entindex, entity_state_t *pplayer, model_t *model);
	int FindDyingPlayer(const char *modelname, vec3_t origin, vec3_t angles, int sequence, int body);

	void SetPlayerEmitted(int entindex);
	bool IsPlayerEmitted(int entindex);
	void ClearAllPlayerEmitState();
private:
	std::unordered_map<int, int> m_barnacleMap;
	std::unordered_map<int, int> m_gargantuaMap;

	PlayerDying_t PlayerDying[33];
	bool PlayerEmitted[33];
};

extern CorpseManager gCorpseManager;