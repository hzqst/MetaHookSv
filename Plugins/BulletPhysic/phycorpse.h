#pragma once

#include <r_efx.h>
#include <cl_entity.h>
#include <com_model.h>
#include <unordered_map>
#include <set>

#define PhyCorpseFlag1 (753951)
#define PhyCorpseFlag2 (152359)

class CorpseManager
{
public:
	CorpseManager(void);

	void FreeCorpseForEntity(int entindex);
	TEMPENTITY* FindCorpseForEntity(int entindex);
	TEMPENTITY* CreateCorpseForEntity(cl_entity_t* ent, model_t *model);
	void AddBarnacle(int entindex);
	bool HasCorpse(void) const;
	cl_entity_t *FindBarnacleForPlayer(entity_state_t *);
private:
	std::unordered_map<int, TEMPENTITY*> m_corpseMap;
	std::set<int> m_barnacles;
};

extern CorpseManager gCorpseManager;