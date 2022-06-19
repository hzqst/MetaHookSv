#include <metahook.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "corpse.h"
#include "physics.h"
#include "mathlib.h"

int EngineGetModelIndex(model_t *mod);

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
}activity_e;

//TODO: hook Mod_LoadStudioModel?
model_t *g_barnacle_model = NULL;
model_t *g_gargantua_model = NULL;

bool IsEntityEmitted(cl_entity_t *ent)
{
	if(ent->player)
		return gCorpseManager.IsPlayerEmitted(ent->index);

	return true;
}

int GetSequenceActivityType(model_t *mod, entity_state_t* entstate)
{
	if (g_bIsSvenCoop)
	{
		if (entstate->scale != 0 && entstate->scale != 1.0f)
			return 0;
	}

	if (mod->type != mod_studio)
		return 0;

	auto studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return 0;

	int sequence = entstate->sequence;
	if (sequence >= studiohdr->numseq)
		return 0;

	auto pseqdesc = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex) + sequence;

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
		return 1;
	}

	if (
		pseqdesc->activity == ACT_BARNACLE_HIT ||
		pseqdesc->activity == ACT_BARNACLE_PULL ||
		pseqdesc->activity == ACT_BARNACLE_CHOMP ||
		pseqdesc->activity == ACT_BARNACLE_CHEW
		)
	{
		return 2;
	}

	return 0;
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

bool IsEntityGargantua(cl_entity_t* ent)
{
	if (ent && ent->model && ent->model->type == mod_studio)
	{
		if (g_gargantua_model)
		{
			if (g_gargantua_model == ent->model)
			{
				return true;
			}
		}
		else if (!strcmp(ent->model->name, "models/garg.mdl"))
		{
			g_gargantua_model = ent->model;

			return true;
		}
	}

	return false;
}

bool IsEntityWater(cl_entity_t* ent)
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

bool IsEntityPresent(cl_entity_t* ent)
{
	if (!ent->model)
		return false;

	if(!ent->index)
		return false;

	if (ent->curstate.messagenum != (*cl_parsecount))
		return false;

	return true;
}

CorpseManager gCorpseManager;

CorpseManager::CorpseManager(void)
{
	ClearAllPlayerDying();
}

//return entindex of dying player
int CorpseManager::FindDyingPlayer(const char *modelname, vec3_t origin, vec3_t angles, int sequence, int body)
{
	for (int i = 1; i < _ARRAYSIZE(PlayerDying); ++i)
	{
		if (PlayerDying[i].bIsDying &&
			PlayerDying[i].iSequence == sequence &&
			PlayerDying[i].iBody == body &&
			0 == strcmp(PlayerDying[i].szModelName, modelname))
		{
			if (VectorDistance(origin, PlayerDying[i].vecOrigin) < 128 && 
				fabs(angles[0] -  PlayerDying[i].vecAngles[0]) < 10 &&
				fabs(angles[1] - PlayerDying[i].vecAngles[1]) < 10)
			{
				return i;
			}
		}
	}
	return 0;
}

void CorpseManager::ClearAllPlayerDying()
{
	for (int i = 0; i < _ARRAYSIZE(PlayerDying); ++i)
	{
		ClearPlayerDying(i);
	}
}

void CorpseManager::ClearPlayerDying(int entindex)
{
	PlayerDying[entindex].bIsDying = false;
	PlayerDying[entindex].flAnimTime = 0;
	PlayerDying[entindex].iSequence = 0;
	PlayerDying[entindex].iModelIndex = 0;
	memset(PlayerDying[entindex].szModelName, 0, sizeof(PlayerDying[entindex].szModelName));
	VectorClear(PlayerDying[entindex].vecAngles);
	VectorClear(PlayerDying[entindex].vecOrigin);
}

void CorpseManager::SetPlayerDying(int entindex, entity_state_t *pplayer, model_t *model)
{
	PlayerDying[entindex].bIsDying = true;
	PlayerDying[entindex].flClientTime = gEngfuncs.GetClientTime();
	PlayerDying[entindex].flAnimTime = pplayer->animtime;
	PlayerDying[entindex].iSequence = pplayer->sequence;
	PlayerDying[entindex].iBody = pplayer->body;
	
	if (PlayerDying[entindex].iModelIndex != EngineGetModelIndex(model))
	{
		PlayerDying[entindex].iModelIndex = EngineGetModelIndex(model);
		strncpy(PlayerDying[entindex].szModelName, model->name, 64);
		PlayerDying[entindex].szModelName[63] = 0;
	}

	VectorCopy(pplayer->angles, PlayerDying[entindex].vecAngles);
	VectorCopy(pplayer->origin, PlayerDying[entindex].vecOrigin);

	//fix angle?
	if (PlayerDying[entindex].vecAngles[0] > 180)
		PlayerDying[entindex].vecAngles[0] -= 360;

	if (PlayerDying[entindex].vecAngles[0] < -180)
		PlayerDying[entindex].vecAngles[0] += 360;

	if (PlayerDying[entindex].vecAngles[1] > 180)
		PlayerDying[entindex].vecAngles[1] -= 360;

	if (PlayerDying[entindex].vecAngles[1] < -180)
		PlayerDying[entindex].vecAngles[1] += 360;
}

void CorpseManager::SetPlayerEmitted(int entindex)
{
	PlayerEmitted[entindex] = true;
}

bool CorpseManager::IsPlayerEmitted(int entindex)
{
	return PlayerEmitted[entindex];
}

void CorpseManager::ClearAllPlayerEmitState()
{
	for (int i = 0; i < _ARRAYSIZE(PlayerEmitted); ++i)
	{
		PlayerEmitted[i] = false;
	}
}

void CorpseManager::FreePlayerForBarnacle(int entindex)
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

void CorpseManager::NewMap(void)
{
	g_barnacle_model = NULL;

	m_barnacleMap.clear();

	ClearAllPlayerDying();
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

void CorpseManager::AddGargantua(int entindex, int playerindex)
{
	auto itor = m_gargantuaMap.find(entindex);
	if (itor == m_gargantuaMap.end())
	{
		m_gargantuaMap[entindex] = playerindex;
	}
	else if (itor->second == 0 && playerindex != 0)
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
				GetSequenceActivityType(playerEntity->model, &playerEntity->curstate) == 2)
			{
				return playerEntity;
			}
		}
	}

	return NULL;
}

cl_entity_t *CorpseManager::FindPlayerForGargantua(int entindex)
{
	auto itor = m_gargantuaMap.find(entindex);
	if (itor != m_gargantuaMap.end())
	{
		if (itor->second != 0)
		{
			auto playerEntity = gEngfuncs.GetEntityByIndex(itor->second);
			if (playerEntity &&
				playerEntity->player &&
				GetSequenceActivityType(playerEntity->model, &playerEntity->curstate) == 2)
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

cl_entity_t *CorpseManager::FindGargantuaForPlayer(entity_state_t *player)
{
	for (auto itor = m_gargantuaMap.begin(); itor != m_gargantuaMap.end(); itor++)
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