#include <metahook.h>
#include "phycorpse.h"
#include "physics.h"
#include "mathlib.h"

#define FTENT_KILLCALLBACK		0x00100000

CorpseManager gCorpseManager;

CorpseManager::CorpseManager(void)
{
	m_corpseIndex = MAX_ENTITIES;
}

bool CorpseManager::IsPlayingDeathAnimation(entity_state_t* entstate)
{
	if (entstate->sequence >= 12 && entstate->sequence <= 18 && entstate->animtime > 5.0f)
		return true;

	return false;
}

void CorpseManager::FreeCorpseForEntity(cl_entity_t* ent)
{
	auto itor = m_corpseMap.find(ent->index);
	if (itor != m_corpseMap.end())
	{
		auto tempent = itor->second;

		tempent->die = 0;

		gPhysicsManager.RemoveRagdoll(tempent->entity.index);

		m_corpseMap.erase(itor);

		return;
	}
}

TEMPENTITY* CorpseManager::FindCorpseForEntity(cl_entity_t* ent)
{
	auto itor = m_corpseMap.find(ent->index);
	if (itor != m_corpseMap.end())
	{
		return itor->second;
	}
	return NULL;
}

TEMPENTITY* CorpseManager::CreateCorpseForEntity(cl_entity_t* ent)
{
	TEMPENTITY* tempent = gEngfuncs.pEfxAPI->CL_TempEntAllocNoModel(ent->curstate.origin);
	tempent->entity.curstate.iuser1 = ent->index;
	tempent->entity.curstate.iuser3 = PhyCorpseFlag1;
	tempent->entity.curstate.iuser4 = PhyCorpseFlag2;
	tempent->entity.curstate.body = ent->curstate.body;
	tempent->entity.curstate.skin = ent->curstate.skin;
	tempent->entity.curstate.colormap = ent->curstate.colormap;
	VectorCopy(ent->angles, tempent->entity.angles);

	tempent->entity.latched = ent->latched;
	VectorCopy(ent->curstate.angles, tempent->entity.curstate.angles);
	tempent->entity.curstate.animtime = ent->curstate.animtime;
	tempent->entity.curstate.sequence = ent->curstate.sequence;
	tempent->entity.curstate.aiment = ent->curstate.aiment;
	tempent->entity.curstate.frame = ent->curstate.frame;
	tempent->entity.curstate.modelindex = ent->curstate.modelindex;
	tempent->flags = 0;
	tempent->callback = NULL;

	tempent->die = gEngfuncs.GetClientTime() + 999999;
	tempent->entity.index = m_corpseIndex++;

	m_corpseMap[ent->index] = tempent;

	return tempent;
}

bool CorpseManager::IsEntityCorpse(cl_entity_t* ent)
{
	if (ent->curstate.iuser3 == PhyCorpseFlag1 && ent->curstate.iuser4 == PhyCorpseFlag2)
	{
		return true;
	}

	return false;
}