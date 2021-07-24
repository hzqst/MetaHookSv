#include <metahook.h>
#include "exportfuncs.h"
#include "phycorpse.h"
#include "physics.h"
#include "mathlib.h"

typedef enum
{
	ACT_RESET,
	ACT_IDLE,
	ACT_GUARD,
	ACT_WALK,
	ACT_RUN,
	ACT_FLY,
	ACT_SWIM,
	ACT_HOP,
	ACT_LEAP,
	ACT_FALL,
	ACT_LAND,
	ACT_STRAFE_LEFT,
	ACT_STRAFE_RIGHT,
	ACT_ROLL_LEFT,
	ACT_ROLL_RIGHT,
	ACT_TURN_LEFT,
	ACT_TURN_RIGHT,
	ACT_CROUCH,
	ACT_CROUCHIDLE,
	ACT_STAND,
	ACT_USE,
	ACT_SIGNAL1,
	ACT_SIGNAL2,
	ACT_SIGNAL3,
	ACT_TWITCH,
	ACT_COWER,
	ACT_SMALL_FLINCH,
	ACT_BIG_FLINCH,
	ACT_RANGE_ATTACK1,
	ACT_RANGE_ATTACK2,
	ACT_MELEE_ATTACK1,
	ACT_MELEE_ATTACK2,
	ACT_RELOAD,
	ACT_ARM,
	ACT_DISARM,
	ACT_EAT,
	ACT_DIESIMPLE,
	ACT_DIEBACKWARD,
	ACT_DIEFORWARD,
	ACT_DIEVIOLENT,
	ACT_BARNACLE_HIT,
	ACT_BARNACLE_PULL,
	ACT_BARNACLE_CHOMP,
	ACT_BARNACLE_CHEW,
	ACT_SLEEP,
	ACT_INSPECT_FLOOR,
	ACT_INSPECT_WALL,
	ACT_IDLE_ANGRY,
	ACT_WALK_HURT,
	ACT_RUN_HURT,
	ACT_HOVER,
	ACT_GLIDE,
	ACT_FLY_LEFT,
	ACT_FLY_RIGHT,
	ACT_DETECT_SCENT,
	ACT_SNIFF,
	ACT_BITE,
	ACT_THREAT_DISPLAY,
	ACT_FEAR_DISPLAY,
	ACT_EXCITED,
	ACT_SPECIAL_ATTACK1,
	ACT_SPECIAL_ATTACK2,
	ACT_COMBAT_IDLE,
	ACT_WALK_SCARED,
	ACT_RUN_SCARED,
	ACT_VICTORY_DANCE,
	ACT_DIE_HEADSHOT,
	ACT_DIE_CHESTSHOT,
	ACT_DIE_GUTSHOT,
	ACT_DIE_BACKSHOT,
	ACT_FLINCH_HEAD,
	ACT_FLINCH_CHEST,
	ACT_FLINCH_STOMACH,
	ACT_FLINCH_LEFTARM,
	ACT_FLINCH_RIGHTARM,
	ACT_FLINCH_LEFTLEG,
	ACT_FLINCH_RIGHTLEG,
	ACT_FLINCH_SMALL,
	ACT_FLINCH_LARGE,
	ACT_HOLDBOMB
};

model_t *g_barnacle_model = NULL;
model_t *g_player_model = NULL;
int g_sequence_table[1024] = {0};

bool IsEntityCorpse(cl_entity_t* ent)
{
	if (ent->curstate.iuser3 == PhyCorpseFlag1 && ent->curstate.iuser4 == PhyCorpseFlag2)
	{
		return true;
	}

	return false;
}

void InitializePlayerSequenceTable(model_t *mod)
{
	if (g_player_model)
		return;

	if (mod->type != mod_studio)
		return;

	g_player_model = mod;

	auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return;

	for (int i = 0; i < studiohdr->numseq; ++i)
	{
		auto pseqdesc = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex) + i;
		if (
			pseqdesc->activity == ACT_DIESIMPLE ||
			pseqdesc->activity == ACT_DIEBACKWARD ||
			pseqdesc->activity == ACT_DIEFORWARD ||
			pseqdesc->activity == ACT_DIEVIOLENT ||
			pseqdesc->activity == ACT_DIEVIOLENT ||
			pseqdesc->activity == ACT_DIE_HEADSHOT ||
			pseqdesc->activity == ACT_DIE_CHESTSHOT ||
			pseqdesc->activity == ACT_DIE_GUTSHOT ||
			pseqdesc->activity == ACT_DIE_BACKSHOT
			)
		{
			if (i < 1024)
				g_sequence_table[i] = 1;
		}
		else if (
			pseqdesc->activity == ACT_BARNACLE_HIT ||
			pseqdesc->activity == ACT_BARNACLE_PULL ||
			pseqdesc->activity == ACT_BARNACLE_CHOMP ||
			pseqdesc->activity == ACT_BARNACLE_CHEW 
			)
		{
			if (i < 1024)
				g_sequence_table[i] = 2;
		}
	}
}

bool IsPlayerDeathAnimation(entity_state_t* entstate)
{
	if (entstate->sequence < 1024 && g_sequence_table[entstate->sequence] == 1)
		return true;

	return false;
}

bool IsPlayerBarnacleAnimation(entity_state_t* entstate)
{
	if (entstate->sequence < 1024 && g_sequence_table[entstate->sequence] == 2)
		return true;

	return false;
}

void GlobalFreeCorpseForEntity(int entindex)
{
	gCorpseManager.FreeCorpseForEntity(entindex);
}

bool IsEntityBarnacle(cl_entity_t* ent)
{
	if (ent && ent->model && ent->model->type == mod_studio)
	{
		if (g_barnacle_model)
		{
			if (g_barnacle_model == ent->model)
			{
				return (ent->curstate.sequence >= 3 && ent->curstate.sequence <= 5);
			}
		}
		else if (!strcmp(ent->model->name, "models/barnacle.mdl"))
		{
			g_barnacle_model = ent->model;

			return (ent->curstate.sequence >= 3 && ent->curstate.sequence <= 5);
		}
	}

	return false;
}

CorpseManager gCorpseManager;

CorpseManager::CorpseManager(void)
{

}

void CorpseManager::FreeCorpseForBarnacle(int entindex)
{
	for (auto itor = m_barnacleMap.begin(); itor != m_barnacleMap.end(); )
	{
		if (itor->second == entindex)
		{
			itor->second = 0;
			return;
		}

		itor++;
	}
}

void CorpseManager::FreeCorpseForEntity(int entindex)
{
	auto itor = m_corpseMap.find(entindex);
	if (itor != m_corpseMap.end())
	{
		auto tempent = itor->second;

		tempent->die = 0;

		gPhysicsManager.RemoveRagdoll(tempent->entity.index);
		FreeCorpseForBarnacle(tempent->entity.index);

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

void CorpseManager::NewMap(void)
{
	g_barnacle_model = NULL;

	m_barnacleMap.clear();

	g_player_model = NULL;

	memset(g_sequence_table, 0, sizeof(g_sequence_table));
}

void CorpseManager::AddBarnacle(int entindex, int playerindex)
{
	auto itor = m_barnacleMap.find(entindex);
	if (itor == m_barnacleMap.end())
	{
		m_barnacleMap[entindex] = playerindex;
	}
	else if(itor->second == 0 && playerindex != 0)
	{
		itor->second = playerindex;
	}
}

cl_entity_t *CorpseManager::FindPlayerForBarnacle(int entindex)
{
	auto itor = m_barnacleMap.find(entindex);
	if (itor != m_barnacleMap.end())
	{
		if (itor->second != 0)
		{
			auto playerEntity = gEngfuncs.GetEntityByIndex(itor->second);
			if (playerEntity &&
				playerEntity->player &&
				IsPlayerBarnacleAnimation(&playerEntity->curstate))
			{
				return playerEntity;
			}
		}
	}

	return NULL;
}

cl_entity_t *CorpseManager::FindBarnacleForPlayer(entity_state_t *player)
{
	for (auto itor = m_barnacleMap.begin(); itor != m_barnacleMap.end(); itor++ )
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