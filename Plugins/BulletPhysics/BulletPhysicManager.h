#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <memory>

#include "BasePhysicManager.h"

class CBulletBaseSharedUserData
{
public:
	virtual ~CBulletBaseSharedUserData()
	{

	}
};

class CBulletCollisionShapeSharedUserData : public CBulletBaseSharedUserData
{
public:
	CBulletCollisionShapeSharedUserData(btTriangleIndexVertexArray* pIndexVertexArray) : m_pIndexVertexArray(pIndexVertexArray)
	{

	}

	~CBulletCollisionShapeSharedUserData()
	{
		if (m_pIndexVertexArray)
		{
			delete m_pIndexVertexArray;
			m_pIndexVertexArray = nullptr;
		}
	}

	btTriangleIndexVertexArray* m_pIndexVertexArray{};
};

class CBulletRigidBodySharedUserData : public CBulletBaseSharedUserData
{
public:
	CBulletRigidBodySharedUserData(const std::string & name, int flags, int boneindex, const btRigidBody::btRigidBodyConstructionInfo &info)
	{
		m_name = name;
		m_flags = flags;
		m_mass = info.m_mass;
		m_inertia = info.m_localInertia;
	}

	std::string m_name;
	int m_flags{};

	float m_mass{};
	btVector3 m_inertia{};
	int m_group{};
	int m_mask{};
	int m_boneindex{ -1 };
};

class CBulletConstraintSharedUserData : public CBulletBaseSharedUserData
{
public:
	CBulletConstraintSharedUserData(const CClientConstraintConfig* pConstraintConfig)
	{
		m_disableCollision = pConstraintConfig->disableCollision;
	}

	bool m_disableCollision{};
};

ATTRIBUTE_ALIGNED16(class)
CBulletBaseMotionState : public btMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletBaseMotionState(IPhysicObject* pPhysicObject) : m_pPhysicObject(pPhysicObject)
	{

	}

	IPhysicObject* GetPhysicObject() const
	{
		return m_pPhysicObject;
	}

	virtual bool IsBoneBased() const = 0;

public:
	IPhysicObject* m_pPhysicObject{};
};

ATTRIBUTE_ALIGNED16(class)
CBulletBoneMotionState : public CBulletBaseMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	CBulletBoneMotionState(IPhysicObject* pPhysicObject,const btTransform& bm, const btTransform& om) : CBulletBaseMotionState(pPhysicObject), m_bonematrix(bm), m_offsetmatrix(om), m_initialbonematrix(bm)
	{

	}

	void getWorldTransform(btTransform& worldTrans) const override
	{
		worldTrans.mult(m_bonematrix, m_offsetmatrix);
	}

	void setWorldTransform(const btTransform& worldTrans) override
	{
		m_bonematrix.mult(worldTrans, m_offsetmatrix.inverse());
	}

	bool IsBoneBased() const override
	{
		return true;
	}
public:
	btTransform m_bonematrix;
	btTransform m_offsetmatrix;
	btTransform m_initialbonematrix;
};

class IPhysicObject;

ATTRIBUTE_ALIGNED16(class)
CBulletEntityMotionState : public CBulletBaseMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletEntityMotionState(IPhysicObject*pPhysicObject) : CBulletBaseMotionState(pPhysicObject)
	{

	}

	void getWorldTransform(btTransform& worldTrans) const override;

	void setWorldTransform(const btTransform& worldTrans) override;

	bool IsBoneBased() const override
	{
		return false;
	}
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

	IStaticObject* CreateStaticObject(const CStaticObjectCreationParameter& CreationParam) override;
	IRagdollObject* CreateRagdollObject(const CRagdollObjectCreationParameter& CreationParam) override;

	void AddPhysicObjectToWorld(IPhysicObject* PhysicObject) override;
	void RemovePhysicObjectFromWorld(IPhysicObject* PhysicObject) override;
};