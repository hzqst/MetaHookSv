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

ATTRIBUTE_ALIGNED16(class)
CBulletPhysicsDebugDraw : public btIDebugDraw
{
private:
	int m_debugMode;
	DefaultColors m_ourColors;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletPhysicsDebugDraw() : m_debugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits)
	{

	}

	~CBulletPhysicsDebugDraw() override
	{
	}

	DefaultColors getDefaultColors() const override
	{
		return m_ourColors;
	}
	///the default implementation for setDefaultColors has no effect. A derived class can implement it and store the colors.
	void setDefaultColors(const DefaultColors& colors) override
	{
		m_ourColors = colors;
	}

	void drawLine(const btVector3& from1, const btVector3& to1, const btVector3& color1) override
	{

	}

	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override
	{
		drawLine(PointOnB, PointOnB + normalOnB * distance, color);
		btVector3 nColor(0, 0, 0);
		drawLine(PointOnB, PointOnB + normalOnB * 0.01, nColor);
	}

	void reportErrorWarning(const char* warningString) override
	{

	}

	void draw3dText(const btVector3& location, const char* textString) override
	{

	}

	void setDebugMode(int debugMode) override
	{
		m_debugMode = debugMode;
	}

	int getDebugMode() const override
	{
		return m_debugMode;
	}
};


void CBulletPhysicManager::Init(void)
{
	CBasePhysicManager::Init();

	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_overlappingPairCache = new btDbvtBroadphase();
	m_solver = new btSequentialImpulseConstraintSolver;
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

	m_debugDraw = new CBulletPhysicsDebugDraw;
	m_dynamicsWorld->setDebugDrawer(m_debugDraw);

	//m_overlapFilterCallback = new GameFilterCallback();
	m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(m_overlapFilterCallback);

	m_dynamicsWorld->setGravity(btVector3(0, 0, 0));
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
	CBasePhysicManager::SetGravity(velocity);

	m_dynamicsWorld->setGravity(btVector3(0, 0, m_gravity));
}

void CBulletPhysicManager::StepSimulation(double frametime)
{
	CBasePhysicManager::StepSimulation(frametime);

	if (frametime <= 0)
		return;

	m_dynamicsWorld->stepSimulation(frametime, 3, 1.0f / GetSimulationTickRate());
}

void CBulletPhysicManager::RemovePhysicObject(int entindex)
{

}

void CBulletPhysicManager::RemoveAllPhysicObjects(int flags)
{

}

IStaticObject* CBulletPhysicManager::CreateStaticObject(cl_entity_t* ent, const CPhysicStaticObjectCreationParameter& CreationParameter)
{


	return NULL;
}

IClientPhysicManager* BulletPhysicManager_CreateInstance()
{
	return new CBulletPhysicManager;
}