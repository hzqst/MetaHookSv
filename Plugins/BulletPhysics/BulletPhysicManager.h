#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "BasePhysicManager.h"

class CBulletPhysicManager : public CBasePhysicManager
{
private:
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btCollisionDispatcher* m_dispatcher;
	btBroadphaseInterface* m_overlappingPairCache;
	btOverlapFilterCallback* m_overlapFilterCallback;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
	btIDebugDraw* m_debugDraw;

public:
	CBulletPhysicManager();

	void Init(void) override;
	void Shutdown() override;
	void NewMap(void) override;
	void DebugDraw(void) override;
	void SetGravity(float velocity) override;
	void StepSimulation(double frametime) override;

	void RemovePhysicObject(int entindex) override;
	void RemoveAllPhysicObjects(int flags) override;
public:
	IStaticObject* CreateStaticObject(cl_entity_t* ent, const CPhysicStaticObjectCreationParameter& CreationParameter) override;

};