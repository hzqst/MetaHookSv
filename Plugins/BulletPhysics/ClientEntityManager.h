#pragma once

#include <interface.h>
#include <r_efx.h>
#include <cl_entity.h>
#include <com_model.h>

class CPlayerDeathState
{
public:
	CPlayerDeathState()
	{
		bIsDying = false;
		flClientTime = 0;
		flAnimTime = 0;
		iSequence = 0;
		iBody = 0;
		iModelIndex = 0;
		szModelName[0] = 0;
		vecOrigin[0] = 0;
		vecOrigin[1] = 0;
		vecOrigin[2] = 0;
		vecAngles[0] = 0;
		vecAngles[1] = 0;
		vecAngles[2] = 0;
	}

	bool bIsDying;
	float flClientTime;
	float flAnimTime;
	int iSequence;
	int iBody;
	int iModelIndex;
	char szModelName[64];
	vec3_t vecOrigin;
	vec3_t vecAngles;
};

class IClientEntityManager : public IBaseInterface
{
public:
	virtual bool IsTempEntityPresent(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, TEMPENTITY* tent) = 0;
	virtual bool IsEntityPresent(cl_entity_t* ent) = 0;
	virtual bool IsEntityDeadPlayer(cl_entity_t* ent) = 0;
	virtual bool IsEntityWater(cl_entity_t* ent) = 0;
	virtual bool IsEntityGargantua(cl_entity_t* ent) = 0;
	virtual bool IsEntityBarnacle(cl_entity_t* ent) = 0;
	virtual bool IsEntityEmitted(cl_entity_t* ent) = 0;

	virtual void AddGargantua(int entindex, int playerindex) = 0;
	virtual void AddBarnacle(int entindex, int playerindex) = 0;
	virtual void NewMap(void) = 0;

	virtual cl_entity_t* FindGargantuaForPlayer(entity_state_t* entstate) = 0;
	virtual cl_entity_t* FindBarnacleForPlayer(entity_state_t* entstate) = 0;
	virtual cl_entity_t* FindPlayerForBarnacle(int entindex) = 0;
	virtual cl_entity_t* FindPlayerForGargantua(int entindex) = 0;
	virtual void FreePlayerForBarnacle(int entindex) = 0;

	virtual void ClearPlayerDeathState(int entindex) = 0;
	virtual void ClearAllPlayerDeathState() = 0;
	virtual void SetPlayerDeathState(int entindex, entity_state_t* pplayer, model_t* model) = 0;
	virtual int FindDyingPlayer(const char* modelname, vec3_t origin, vec3_t angles, int sequence, int body) = 0;

	virtual void SetPlayerEmitted(int entindex) = 0;
	virtual bool IsPlayerEmitted(int entindex) = 0;
	virtual void ClearAllPlayerEmitState() = 0;
};

IClientEntityManager *ClientEntityManager();