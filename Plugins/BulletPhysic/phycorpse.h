#pragma once

#include <r_efx.h>
#include <cl_entity.h>
#include <com_model.h>
#include <unordered_map>

// magic num
#define PhyCorpseFlag1 (753951)
#define PhyCorpseFlag2 (152359)

class CorpseManager
{
public:
	CorpseManager(void);

	// check if the entity is already dead
	bool IsPlayerDeathAnimation(entity_state_t* entstate);

	void FreeCorpseForEntity(cl_entity_t* ent);

	TEMPENTITY* FindCorpseForEntity(cl_entity_t* ent);

	// create ragdoll corpse for specified entity
	TEMPENTITY* CreateCorpseForEntity(cl_entity_t* ent, model_t *model);

	// check if the entity is a ragdoll corpse (temp entity)
	bool IsEntityCorpse(cl_entity_t* ent);

private:
	int m_corpseBaseIndex;
	std::unordered_map<int, TEMPENTITY*> m_corpseMap;
};

extern CorpseManager gCorpseManager;