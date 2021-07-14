#include <metahook.h>
#include "phycorpse.h"
#include "physics.h"
#include "mathlib.h"

#define FTENT_KILLCALLBACK		0x00100000

bool IsEntityCorpse(cl_entity_t* ent)
{
	if (ent->curstate.iuser3 == PhyCorpseFlag1 && ent->curstate.iuser4 == PhyCorpseFlag2)
	{
		return true;
	}

	return false;
}

bool IsPlayerDeathAnimation(entity_state_t* entstate)
{
	if (entstate->sequence >= 12 && entstate->sequence <= 18)
		return true;

	return false;
}

bool IsPlayerBarnacleAnimation(entity_state_t* entstate)
{
	if (entstate->sequence >= 182 && entstate->sequence <= 185)
		return true;

	return false;
}

void GlobalFreeCorpseForEntity(int entindex)
{
	gCorpseManager.FreeCorpseForEntity(entindex);
}

bool IsEntityBarnacle(cl_entity_t* ent)
{
	if (ent && ent->model && ent->model->type == mod_studio &&
		!strcmp(ent->model->name, "models/barnacle.mdl") && ent->curstate.sequence >= 3 && ent->curstate.sequence <= 5)
		return true;

	return false;
}

CorpseManager gCorpseManager;

CorpseManager::CorpseManager(void)
{

}

void CorpseManager::FreeCorpseForEntity(int entindex)
{
	auto itor = m_corpseMap.find(entindex);
	if (itor != m_corpseMap.end())
	{
		auto tempent = itor->second;

		tempent->die = 0;

		gPhysicsManager.RemoveRagdoll(tempent->entity.index);

		m_corpseMap.erase(itor);

		return;
	}
}

TEMPENTITY* CorpseManager::FindCorpseForEntity(int entindex)
{
	auto itor = m_corpseMap.find(entindex);
	if (itor != m_corpseMap.end())
	{
		return itor->second;
	}
	return NULL;
}

bool CorpseManager::HasCorpse(void) const
{
	return m_corpseMap.size();
}

void CorpseManager::AddBarnacle(int entindex)
{
	m_barnacles.emplace(entindex);
}

cl_entity_t *CorpseManager::FindBarnacleForPlayer(entity_state_t *player)
{
	for (auto index : m_barnacles)
	{
		auto ent = gEngfuncs.GetEntityByIndex(index);
		if (IsEntityBarnacle(ent))
		{
			if (fabs(player->origin[0] - ent->origin[0]) < 1 &&
				fabs(player->origin[1] - ent->origin[1]) < 1 && 
				player->origin[2] < ent->origin[2] + 16)
			{
				return ent;
			}
		}
	}

	return NULL;
}

TEMPENTITY* CorpseManager::CreateCorpseForEntity(cl_entity_t *ent, model_t *model)
{
	TEMPENTITY* tempent = gEngfuncs.pEfxAPI->CL_TempEntAlloc(ent->curstate.origin, model);
	if (!tempent)
		return NULL;

	tempent->entity.curstate.iuser1 = ent->index;
	tempent->entity.curstate.iuser2 = 0;
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
	tempent->entity.curstate.gaitsequence = ent->curstate.gaitsequence;
	tempent->entity.curstate.framerate = ent->curstate.framerate;
	tempent->entity.curstate.weaponanim = 0;
	tempent->entity.curstate.weaponmodel = 0;
	tempent->entity.curstate.aiment = 0;
	tempent->entity.curstate.frame = ent->curstate.frame;
	tempent->entity.curstate.modelindex = ent->curstate.modelindex;
	tempent->flags = 0;
	tempent->callback = NULL;

	tempent->die = gEngfuncs.GetClientTime() + 999999;
	tempent->entity.index = ent->index;

	m_corpseMap[ent->index] = tempent;

	return tempent;
}