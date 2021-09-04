#pragma once

#include <r_efx.h>
#include <cl_entity.h>
#include <com_model.h>
#include <unordered_map>
#include <set>

#define PhyCorpseFlag1 (753951)
#define PhyCorpseFlag2 (152359)
#define PhyCorpseFlag3 (152360)

class CorpseManager
{
public:
	CorpseManager(void);

	void AddBarnacle(int entindex, int playerindex);
	void NewMap(void);
	cl_entity_t *FindBarnacleForPlayer(entity_state_t *entstate);
	cl_entity_t *FindPlayerForBarnacle(int entindex);
	void FreePlayerForBarnacle(int entindex);
private:
	std::unordered_map<int, int> m_barnacleMap;
};

extern CorpseManager gCorpseManager;