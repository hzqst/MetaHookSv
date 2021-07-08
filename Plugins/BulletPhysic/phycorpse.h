#pragma once

#include <r_efx.h>
#include <cl_entity.h>
#include <com_model.h>
#include <unordered_map>

#define PhyCorpseFlag1 (753951)
#define PhyCorpseFlag2 (152359)

class CorpseManager
{
public:
	CorpseManager(void);

	bool IsPlayerDeathAnimation(entity_state_t* entstate);

	void FreeCorpseForEntity(cl_entity_t* ent);

	TEMPENTITY* FindCorpseForEntity(cl_entity_t* ent);

	TEMPENTITY* CreateCorpseForEntity(cl_entity_t* ent, model_t *model);

private:
	int m_corpseBaseIndex;
	std::unordered_map<int, TEMPENTITY*> m_corpseMap;
};

extern CorpseManager gCorpseManager;