#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <memory>

#include "BasePhysicManager.h"

enum BulletPhysicCollisionFilterGroups
{
	RagdollObjectFilter = 0x40,
	DynamicObjectFilter = 0x80,
	StaticObjectFilter = 0x100,
	WorldFilter = 0x200,
};

class CBulletBaseSharedUserData : public IBaseInterface
{
public:
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
#if 0
class CBulletRigidBodySharedUserData : public CBulletBaseSharedUserData
{
public:
	CBulletRigidBodySharedUserData(const btRigidBody::btRigidBodyConstructionInfo& info,
		int group, int mask,
		const std::string & name,
		int flags,
		int boneindex,
		int debugDrawLevel,
		float density)
	{
		m_mass = info.m_mass;
		m_inertia = info.m_localInertia;

		m_group = group;
		m_mask = mask;

		m_name = name;

		m_flags = flags;
		m_boneindex = boneindex;
		m_debugDrawLevel = debugDrawLevel;
		m_density = density;
	}

	float m_mass{};
	btVector3 m_inertia{};

	int m_group{};
	int m_mask{};

	std::string m_name;

	int m_flags{};
	int m_boneindex{ -1 };
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	float m_density{ BULLET_DEFAULT_DENSENTY };
	bool m_addedToPhysicWorld{};
};
class CBulletConstraintSharedUserData : public CBulletBaseSharedUserData
{
public:
	CBulletConstraintSharedUserData(const CClientConstraintConfig* pConstraintConfig)
	{
		m_name = pConstraintConfig->name;
		m_flags = pConstraintConfig->flags;
		m_debugDrawLevel = pConstraintConfig->debugDrawLevel;
		m_maxTolerantLinearError = pConstraintConfig->maxTolerantLinearError;
		m_disableCollision = pConstraintConfig->disableCollision;
	}

	std::string m_name;
	int m_flags;
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	float m_maxTolerantLinearError{ BULLET_MAX_TOLERANT_LINEAR_ERROR };

	bool m_disableCollision{};
	bool m_addedToPhysicWorld{};
};
#endif

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
	CBulletBoneMotionState(IPhysicObject* pPhysicObject,const btTransform& bm, const btTransform& om) : CBulletBaseMotionState(pPhysicObject), m_bonematrix(bm), m_offsetmatrix(om)
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
};

class IPhysicObject;

ATTRIBUTE_ALIGNED16(class)
CBulletEntityMotionState : public CBulletBaseMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletEntityMotionState(IPhysicObject* pPhysicObject) : CBulletBaseMotionState(pPhysicObject)
	{
		m_offsetmatrix.setIdentity();
	}

	CBulletEntityMotionState(IPhysicObject *pPhysicObject, const btTransform &offsetmatrix) : CBulletBaseMotionState(pPhysicObject), m_offsetmatrix(offsetmatrix)
	{

	}

	void getWorldTransform(btTransform& worldTrans) const override;

	void setWorldTransform(const btTransform& worldTrans) override;

	bool IsBoneBased() const override
	{
		return false;
	}

	btTransform m_offsetmatrix;
};

class CBulletConstraintCreationContext
{
public:
	btRigidBody* pRigidBodyA{};
	btRigidBody* pRigidBodyB{};
	btTransform worldTransA{};
	btTransform worldTransB{};
	btTransform invWorldTransA{};
	btTransform invWorldTransB{};
	btTransform localTransA{};
	btTransform localTransB{};
	btTransform globalJointA{};
	btTransform globalJointB{};
	btScalar rigidBodyDistance{};
};

class CBulletRigidBody : public CBasePhysicRigidBody
{
public:
	CBulletRigidBody(
		int id,
		int entindex,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group, int mask);

	~CBulletRigidBody();

	bool AddToPhysicWorld(void* world) override;
	bool RemoveFromPhysicWorld(void* world) override;
	bool IsAddedToPhysicWorld(void* world) const override;

	void ApplyCentralForce(const vec3_t vecForce) override;
	void SetLinearVelocity(const vec3_t vecVelocity) override;
	void SetAngularVelocity(const vec3_t vecVelocity) override;
	bool ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate) override;
	bool SetupBones(studiohdr_t* studiohdr) override;
	bool SetupJiggleBones(studiohdr_t* studiohdr) override;

	void* GetInternalRigidBody() override;

public:
	float m_mass{};
	btVector3 m_inertia{};
	float m_density{ BULLET_DEFAULT_DENSENTY };
	int m_group{ btBroadphaseProxy::DefaultFilter };
	int m_mask{ btBroadphaseProxy::AllFilter  };

	bool m_addedToPhysicWorld{};

	btRigidBody* m_pInternalRigidBody{};
};

class CBulletConstraint : public CBasePhysicConstraint
{
public:
	CBulletConstraint(
		int id,
		int entindex,
		CClientConstraintConfig* pConstraintConfig,
		btTypedConstraint* pInternalConstraint);

	~CBulletConstraint();
	
	const char* GetTypeString() const;
	const char* GetTypeLocalizationTokenString() const;

	bool AddToPhysicWorld(void* world) override;
	bool RemoveFromPhysicWorld(void* world) override;
	bool IsAddedToPhysicWorld(void* world) const override;

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override;

	void ExtendLinearLimit(int axis, float value) override;

	void* GetInternalConstraint() override;

public:

	float m_maxTolerantLinearError{ BULLET_MAX_TOLERANT_LINEAR_ERROR };
	bool m_disableCollision{};

	bool m_addedToPhysicWorld{};

	btTypedConstraint* m_pInternalConstraint{};
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
	IDynamicObject* CreateDynamicObject(const CDynamicObjectCreationParameter& CreationParam) override;
	IRagdollObject* CreateRagdollObject(const CRagdollObjectCreationParameter& CreationParam) override;

	void AddPhysicComponentsToWorld(IPhysicObject* PhysicObject, const CPhysicComponentFilters& filters) override;
	void RemovePhysicComponentsFromWorld(IPhysicObject* PhysicObject, const CPhysicComponentFilters& filters) override;
	void AddPhysicComponentToWorld(IPhysicComponent* pPhysicComponent) override;
	void RemovePhysicComponentFromWorld(IPhysicComponent* pPhysicComponent) override;
	void OnPhysicComponentAddedIntoPhysicWorld(IPhysicComponent* pPhysicComponent) override;
	void OnPhysicComponentRemovedFromPhysicWorld(IPhysicComponent* pPhysicComponent) override;

	void TraceLine(vec3_t vecStart, vec3_t vecEnd, CPhysicTraceLineHitResult &hitResult) override;
};

CBulletCollisionShapeSharedUserData* GetSharedUserDataFromCollisionShape(btCollisionShape* pCollisionShape);
CBulletBaseMotionState* GetMotionStateFromRigidBody(btRigidBody* RigidBody);
IPhysicObject* GetPhysicObjectFromRigidBody(btRigidBody* pRigidBody);

void OnBeforeDeleteBulletCollisionShape(btCollisionShape* pCollisionShape);

btScalar BulletGetConstraintLinearErrorMagnitude(btTypedConstraint* pConstraint);
btTypedConstraint* BulletCreateConstraintFromGlobalJointTransform(CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& globalJointTransform);
btTypedConstraint* BulletCreateConstraintFromLocalJointTransform(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB);
btCollisionShape* BulletCreateCollisionShape(const CClientRigidBodyConfig* pRigidConfig);
btMotionState* BulletCreateMotionState(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, IPhysicObject* pPhysicObject);

void Matrix3x4ToTransform(const float matrix3x4[3][4], btTransform& trans);
void TransformToMatrix3x4(const btTransform& trans, float matrix3x4[3][4]);
void MatrixEuler(const btMatrix3x3& in_matrix, btVector3& out_euler);
void EulerMatrix(const btVector3& in_euler, btMatrix3x3& out_matrix);
btTransform MatrixLookAt(const btTransform& transform, const btVector3& at, const btVector3& forward);
btQuaternion FromToRotaion(btVector3 fromDirection, btVector3 toDirection);