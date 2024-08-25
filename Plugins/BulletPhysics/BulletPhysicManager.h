#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <memory>

#include "BasePhysicManager.h"

enum BulletPhysicCollisionFilterGroups
{
	WorldFilter = 0x40,
	StaticObjectFilter = 0x80,
	DynamicObjectFilter = 0x100,
	RagdollObjectFilter = 0x200,
	InspectorFilter = 0x400,
	ConstraintFilter = 0x800,
	FloaterFilter = 0x1000,
};

enum BulletPhysicCollisionFlags
{
	CF_DISABLE_VISUALIZE_OBJECT_PERMANENT = 0x1000,
};

class CBulletBaseSharedUserData : public IBaseInterface
{
public:
};

class CBulletCollisionShapeSharedUserData : public CBulletBaseSharedUserData
{
public:
	CBulletCollisionShapeSharedUserData()
	{

	}

	~CBulletCollisionShapeSharedUserData()
	{
		if (m_pTriangleIndexVertexArray)
		{
			delete m_pTriangleIndexVertexArray;
			m_pTriangleIndexVertexArray = nullptr;
		}
	}

	std::shared_ptr<CPhysicIndexArray> m_pIndexArray;
	btTriangleIndexVertexArray* m_pTriangleIndexVertexArray{};
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

ATTRIBUTE_ALIGNED16(class)
CBulletEntityMotionState : public CBulletBaseMotionState
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletEntityMotionState(IPhysicObject* pPhysicObject) : CBulletBaseMotionState(pPhysicObject)
	{
		m_offsetmatrix.setIdentity();
		m_worldTransform.setIdentity();
		m_worldTransformInitialized = false;
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

	mutable btTransform m_worldTransform;
	mutable bool m_worldTransformInitialized;
};

ATTRIBUTE_ALIGNED16(class)
CFollowConstraintMotionState : public CBulletBaseMotionState
{
public:
	CFollowConstraintMotionState(IPhysicObject * pPhysicObject, btTypedConstraint * constraint, bool attachToJointB) : CBulletBaseMotionState(pPhysicObject), m_pInternalConstraint(constraint), m_attachToJointB(attachToJointB)
	{

	}

	void getWorldTransform(btTransform& worldTrans) const override;

	void setWorldTransform(const btTransform& worldTrans) override;

	bool IsBoneBased() const override
	{
		return false;
	}

	btTypedConstraint * m_pInternalConstraint{};
	bool m_attachToJointB{};
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
		IPhysicObject* pPhysicObject,
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
	//bool MergeBones(studiohdr_t* studiohdr) override;

	void* GetInternalRigidBody() override;

	bool GetGoldSrcOriginAngles(float* origin, float* angles) override;
	bool GetGoldSrcOriginAnglesWithLocalOffset(const vec3_t localoffset_origin, const vec3_t localoffset_angles, float* origin, float* angles) override;

	float GetMass() const override;

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
		IPhysicObject* pPhysicObject,
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
	float GetMaxTolerantLinearError() const override;

	void* GetInternalConstraint() override;

private:

	btRigidBody* CreateInternalRigidBody(bool attachToJointB);
	void FreeInternalRigidBody(btRigidBody* pRigidBody);
public:

	float m_maxTolerantLinearError{ BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR };
	bool m_disableCollision{};

	bool m_addedToPhysicWorld{};

	int m_rigidBodyAPhysicComponentId{};
	int m_rigidBodyBPhysicComponentId{};

	btTypedConstraint* m_pInternalConstraint{};

	//For rayTest only
	btRigidBody* m_pInternalRigidBodyA{};
	btRigidBody* m_pInternalRigidBodyB{};
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

	void TraceLine(const CPhysicTraceLineParameters& traceParam, CPhysicTraceLineHitResult &hitResult) override;
};

CBulletCollisionShapeSharedUserData* GetSharedUserDataFromCollisionShape(btCollisionShape* pCollisionShape);
CBulletBaseMotionState* GetMotionStateFromRigidBody(btRigidBody* RigidBody);
IPhysicObject* GetPhysicObjectFromRigidBody(btRigidBody* pRigidBody);

void OnBeforeDeleteBulletCollisionShape(btCollisionShape* pCollisionShape);

bool BulletGetConstraintGlobalPivotTransform(btTypedConstraint* pConstraint, btTransform& worldPivotA, btTransform& worldPivotB);
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
btVector3 GetVector3FromVec3(const vec3_t src);

void TransformGoldSrcToBullet(btTransform& trans);
void Vector3GoldSrcToBullet(btVector3& vec);
void TransformBulletToGoldSrc(btTransform& trans);
void Vector3BulletToGoldSrc(btVector3& vec);