#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "BasePhysicManager.h"

void CBasePhysicManager::Init(void)
{

}

void CBasePhysicManager::Shutdown()
{

}

void CBasePhysicManager::NewMap(void)
{

}

void CBasePhysicManager::DebugDraw(void)
{

}

void CBasePhysicManager::SetGravity(float velocity)
{

}

void CBasePhysicManager::StepSimulation(double framerate)
{

}

void CBasePhysicManager::ReloadConfig(void)
{

}

bool CBasePhysicManager::SetupBones(studiohdr_t* hdr, int entindex)
{
	return false;
}

bool CBasePhysicManager::SetupJiggleBones(studiohdr_t* hdr, int entindex)
{
	return false;
}

void CBasePhysicManager::MergeBarnacleBones(studiohdr_t* hdr, int entindex)
{

}

bool CBasePhysicManager::ChangeRagdollEntityIndex(int old_entindex, int new_entindex)
{
	return false;
}

IRagdollObject* CBasePhysicManager::FindRagdollObject(int entindex)
{
	return NULL;
}

IRagdollObject* CBasePhysicManager::CreateRagdollObject(model_t* mod, int entindex, const CRagdollConfig* config)
{
	return NULL;
}

IStaticObject* CBasePhysicManager::CreateStaticObject(model_t* mod, int entindex)
{
	return NULL;
}

void CBasePhysicManager::CreateBrushModel(cl_entity_t* ent)
{

}

void CBasePhysicManager::CreateBarnacle(cl_entity_t* ent)
{

}

void CBasePhysicManager::CreateGargantua(cl_entity_t* ent)
{

}

void CBasePhysicManager::RemovePhysicObject(int entindex)
{

}

void CBasePhysicManager::UpdateTempEntity(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time)
{

}