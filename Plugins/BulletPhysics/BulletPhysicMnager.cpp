#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "BulletPhysicManager.h"

CBulletPhysicManager::CBulletPhysicManager() : CBasePhysicManager()
{
	m_collisionConfiguration = NULL;
	m_dispatcher = NULL;
	m_overlappingPairCache = NULL;
	m_overlapFilterCallback = NULL;
	m_solver = NULL;
	m_dynamicsWorld = NULL;
	m_debugDraw = NULL;
}

void CBulletPhysicManager::Init(void)
{
	CBasePhysicManager::Init();
}

void CBulletPhysicManager::Shutdown()
{
	CBasePhysicManager::Shutdown();

}

void CBulletPhysicManager::NewMap(void)
{
	CBasePhysicManager::NewMap();

}

void CBulletPhysicManager::DebugDraw(void)
{

}
void CBulletPhysicManager::SetGravity(float velocity)
{

}
void CBulletPhysicManager::StepSimulation(double framerate)
{

}
void CBulletPhysicManager::ReloadConfig(void)
{

}

bool CBulletPhysicManager::SetupBones(studiohdr_t* hdr, int entindex)
{
	return false;
}

bool CBulletPhysicManager::SetupJiggleBones(studiohdr_t* hdr, int entindex)
{
	return false;
}

void CBulletPhysicManager::MergeBarnacleBones(studiohdr_t* hdr, int entindex)
{

}

bool CBulletPhysicManager::ChangeRagdollEntityIndex(int old_entindex, int new_entindex)
{
	return false;
}

IRagdollObject* CBulletPhysicManager::FindRagdollObject(int entindex)
{
	return NULL;
}

IRagdollObject* CBulletPhysicManager::CreateRagdollObject(model_t* mod, int entindex, const CRagdollConfig* config)
{
	return NULL;
}

IStaticObject* CBulletPhysicManager::CreateStaticObject(model_t* mod, int entindex)
{
	return NULL;
}

void CBulletPhysicManager::CreateBrushModel(cl_entity_t* ent)
{

}
void CBulletPhysicManager::CreateBarnacle(cl_entity_t* ent)
{

}
void CBulletPhysicManager::CreateGargantua(cl_entity_t* ent)
{

}
void CBulletPhysicManager::RemovePhysicObject(int entindex)
{

}
void CBulletPhysicManager::UpdateTempEntity(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time)
{

}

static CBulletPhysicManager g_BulletPhysicManager;

IPhysicManager* PhysicManager()
{
	return &g_BulletPhysicManager;
}