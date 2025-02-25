#pragma once

#include <interface.h>
#include <metahook.h>
#include <r_efx.h>
#include <cl_entity.h>
#include <com_model.h>

class IClientEntityManager : public IBaseInterface
{
public:

#if 0 //unused, slow
	virtual bool IsTempEntityPresent(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, TEMPENTITY* tent) = 0;
#endif

#if 0 //unused, ridiculous
	virtual bool IsEntityPresent(cl_entity_t* ent) = 0;
#endif

#if 0 //unused
	virtual bool IsEntityWater(cl_entity_t* ent) = 0;
	virtual bool IsEntityGargantua(cl_entity_t* ent) = 0;
	virtual bool IsEntityBarnacle(cl_entity_t* ent) = 0;
#endif

	/*
		Check if the given ent is rendered as a dead player
	*/
	virtual bool IsEntityDeadPlayer(cl_entity_t* ent) = 0;

	/*
		Check if the given ent is rendered as a corpse (CS / CS:CZ)
	*/
	virtual bool IsEntityClientCorpse(cl_entity_t* ent) = 0;

	/*
		Check if the given ent is a valid player
	*/
	virtual bool IsEntityPlayer(cl_entity_t* ent) = 0;

	/*
		Check if the given entindex is valid for tempentity
	*/
	virtual bool IsEntityIndexTempEntity(int entindex) = 0;

	/*
		Check if the given entindex is valid for network entity
	*/
	virtual bool IsEntityIndexNetworkEntity(int entindex) = 0;

	/*
		Check if the given ent is a temp entity
	*/
	virtual bool IsEntityTempEntity(cl_entity_t* ent) = 0;

	/*
		Check if the given ent is a network entity
	*/
	virtual bool IsEntityNetworkEntity(cl_entity_t* ent) = 0;

	virtual cl_entity_t* GetEntityByIndex(int entindex) = 0;

	/*
		Get index of TEMPENTITY * pointer in the gTempEnts[MAX_TEMP_ENTITIES] array containing the cl_entity_t* pointer, plus ENTINDEX_TEMPENTITY (100000 in this case).

		The input is not validated, and must be a tempentity
	*/
	virtual int GetEntityIndexFromTempEntity(cl_entity_t* ent) = 0;

	/*
		Get index of the given network entity

		The input is not validated, and must be a network entity
	*/
	virtual int GetEntityIndexFromNetworkEntity(cl_entity_t* ent) = 0;

	/*
		Wrapper around GetEntityIndexFromTempEntity and GetEntityIndexFromNetworkEntity

		Returns -1 if the entity is not a tempentity or network entity
	*/
	virtual int GetEntityIndex(cl_entity_t* ent) = 0;

	/*
	
		Returns curstate.scale if ent is rendered as a studiomodel.
	*/
	virtual float GetEntityModelScaling(cl_entity_t* ent) = 0;
	virtual float GetEntityModelScaling(cl_entity_t* ent, model_t *mod) = 0;

	virtual void ClearPlayerDeathState(int entindex) = 0;
	virtual void ClearAllPlayerDeathState() = 0;

	virtual void SetPlayerDeathState(int entindex, entity_state_t* pplayer, model_t* model) = 0;

	/*
		Purpose: Find the player that is playing death animation right at the given origin with give angles, sequence and body. returns entindex if found. otherwise return 0.
	*/
	virtual int FindDyingPlayer(const char* modelname, vec3_t origin, vec3_t angles, int sequence, int body) = 0;

	/*
		An entity marked as emitted means the curstate of this entity is valid in current frame.
	*/
	virtual void SetEntityEmitted(int entindex) = 0;
	virtual void SetEntityEmitted(cl_entity_t* ent) = 0;
	virtual bool IsEntityEmitted(int entindex) = 0;
	virtual bool IsEntityEmitted(cl_entity_t* ent) = 0;
	virtual void ClearEntityEmitStates() = 0;

	/*
		Check if ent is in cl_visedicts array.
	*/
	virtual bool IsEntityInVisibleList(cl_entity_t* ent) = 0;

	/*
		Notify that the ent is rendered with given mod.
	*/
	virtual void NotifyEntityModel(cl_entity_t* ent, model_t* mod) = 0;

	virtual void NewMap(void) = 0;

	virtual void SetInspectedEntityIndex(int entindex) = 0;
	virtual int GetInspectEntityIndex() = 0;
	virtual int GetInspectEntityModelIndex() = 0;
};

IClientEntityManager *ClientEntityManager();