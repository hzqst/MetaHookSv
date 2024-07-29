#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <memory>

#include "BasePhysicManager.h"

class CBulletBaseSharedUserData : public IBaseInterface
{
public:
};

class CBulletCollisionShapeSharedUserData : public CBulletBaseSharedUserData
{
public:
	~CBulletCollisionShapeSharedUserData()
	{
		if (m_vertexArray)
			delete m_vertexArray;
	}

	btTriangleIndexVertexArray* m_vertexArray{};
};

class CBulletRigidBodySharedUserData : public CBulletBaseSharedUserData
{
public:
	CBulletRigidBodySharedUserData(const std::string & name, const btRigidBody::btRigidBodyConstructionInfo &info)
	{
		m_name = name;
		m_mass = info.m_mass;
		m_inertia = info.m_localInertia;
	}

	std::string m_name;
	float m_mass{};
	btVector3 m_inertia{};
	int m_group{};
	int m_mask{};
	int m_boneindex{};
};

class CBulletConstraintSharedUserData : public CBulletBaseSharedUserData
{
public:
	bool m_disableCollision{};
};

ATTRIBUTE_ALIGNED16(class)
CBulletBoneMotionState : public btMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CBulletBoneMotionState(const btTransform& bm, const btTransform& om) : bonematrix(bm), offsetmatrix(om)
	{

	}

	void getWorldTransform(btTransform& worldTrans) const override
	{
		worldTrans.mult(bonematrix, offsetmatrix);
	}

	void setWorldTransform(const btTransform& worldTrans) override
	{
		bonematrix.mult(worldTrans, offsetmatrix.inverse());
	}

	btTransform bonematrix;
	btTransform offsetmatrix;
};

ATTRIBUTE_ALIGNED16(class)
CBulletEntityMotionState : public btMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletEntityMotionState(cl_entity_t* ent) : btMotionState(), m_ent(ent)
	{

	}

	void getWorldTransform(btTransform& worldTrans) const;

	void setWorldTransform(const btTransform& worldTrans);

	cl_entity_t* m_ent;
};

class CBulletPhysicManager : public CBasePhysicManager
{
private:
	btDefaultCollisionConfiguration* m_collisionConfiguration{};
	btCollisionDispatcher* m_dispatcher{};
	btBroadphaseInterface* m_overlappingPairCache{};
	btOverlapFilterCallback* m_overlapFilterCallback{};
	btSequentialImpulseConstraintSolver* m_solver{};
	btDiscreteDynamicsWorld* m_dynamicsWorld{};
	btIDebugDraw* m_debugDraw{};

public:
	void Init(void) override;
	void Shutdown() override;
	void NewMap(void) override;
	void DebugDraw(void) override;
	void SetGravity(float velocity) override;
	void StepSimulation(double frametime) override;

	IStaticObject* CreateStaticObject(cl_entity_t* ent, const CPhysicStaticObjectCreationParameter& CreationParameter) override;
	IRagdollObject* CreateRagdollObject(model_t* mod, int entindex, const CClientPhysicConfig* pConfigs) override;
	void AddPhysicObjectToWorld(IPhysicObject* PhysicObject) override;
	void RemovePhysicObjectFromWorld(IPhysicObject* PhysicObject) override;
public:
	
};