#include <metahook.h>
#include <triangleapi.h>
#include "mathlib2.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"

#include "ClientEntityManager.h"
#include "BulletPhysicManager.h"
#include "BulletStaticObject.h"
#include "BulletRagdollObject.h"

#include <vgui_controls/Controls.h>

btQuaternion FromToRotaion(btVector3 fromDirection, btVector3 toDirection)
{
	fromDirection = fromDirection.normalize();
	toDirection = toDirection.normalize();

	float cosTheta = fromDirection.dot(toDirection);

	if (cosTheta < -1 + 0.001f) //(Math.Abs(cosTheta)-Math.Abs( -1.0)<1E-6)
	{
		btVector3 up(0.0f, 0.0f, 1.0f);

		auto rotationAxis = up.cross(fromDirection);
		if (rotationAxis.length() < 0.01) // bad luck, they were parallel, try again!
		{
			rotationAxis = up.cross(fromDirection);
		}
		rotationAxis = rotationAxis.normalize();
		return btQuaternion(rotationAxis, (float)M_PI);
	}
	else
	{
		// Implementation from Stan Melax's Game Programming Gems 1 article
		auto rotationAxis = fromDirection.cross(toDirection);

		float s = (float)sqrt((1 + cosTheta) * 2);
		float invs = 1 / s;

		return btQuaternion(
			rotationAxis.x() * invs,
			rotationAxis.y() * invs,
			rotationAxis.z() * invs,
			s * 0.5f
		);
	}
}

btTransform MatrixLookAt(const btTransform& transform, const btVector3& at, const btVector3& forward)
{
	const auto &originVector = forward;
	auto worldToLocalTransform = transform.inverse();

	//transform the target in world position to object's local position
	auto targetVector = worldToLocalTransform * at;

	auto rot = FromToRotaion(originVector, targetVector);
	btTransform rotMatrix = btTransform(rot);

	return transform * rotMatrix;
}

void EulerMatrix(const btVector3& in_euler, btMatrix3x3& out_matrix)
{
	btVector3 angles = in_euler;
	angles *= SIMD_RADS_PER_DEG;

	btScalar c1(btCos(angles[0]));
	btScalar c2(btCos(angles[1]));
	btScalar c3(btCos(angles[2]));
	btScalar s1(btSin(angles[0]));
	btScalar s2(btSin(angles[1]));
	btScalar s3(btSin(angles[2]));

	out_matrix.setValue(c1 * c2, -c3 * s2 - s1 * s3 * c2, s3 * s2 - s1 * c3 * c2,
		c1 * s2, c3 * c2 - s1 * s3 * s2, -s3 * c2 - s1 * c3 * s2,
		s1, c1 * s3, c1 * c3);
}

void MatrixEuler(const btMatrix3x3& in_matrix, btVector3& out_euler)
{

	out_euler[0] = btAsin(in_matrix[2][0]);

	if (in_matrix[2][0] >= (1 - 0.002f) && in_matrix[2][0] < 1.002f) {
		out_euler[1] = btAtan2(in_matrix[1][0], in_matrix[0][0]);
		out_euler[2] = btAtan2(in_matrix[2][1], in_matrix[2][2]);
	}
	else if (btFabs(in_matrix[2][0]) < (1 - 0.002f)) {
		out_euler[1] = btAtan2(in_matrix[1][0], in_matrix[0][0]);
		out_euler[2] = btAtan2(in_matrix[2][1], in_matrix[2][2]);
	}
	else {
		out_euler[1] = btAtan2(in_matrix[1][2], in_matrix[1][1]);
		out_euler[2] = 0;
	}

	out_euler[3] = 0;

	out_euler *= SIMD_DEGS_PER_RAD;
}

void Matrix3x4ToTransform(const float matrix3x4[3][4], btTransform& trans)
{
	float matrix4x4[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};
	memcpy(matrix4x4, matrix3x4, sizeof(float[3][4]));

	float matrix4x4_transposed[4][4];
	Matrix4x4_Transpose(matrix4x4_transposed, matrix4x4);

	trans.setFromOpenGLMatrix((float*)matrix4x4_transposed);
}

void TransformToMatrix3x4(const btTransform& trans, float matrix3x4[3][4])
{
	float matrix4x4_transposed[4][4];
	trans.getOpenGLMatrix((float*)matrix4x4_transposed);

	float matrix4x4[4][4];
	Matrix4x4_Transpose(matrix4x4, matrix4x4_transposed);

	memcpy(matrix3x4, matrix4x4, sizeof(float[3][4]));
}

CBulletRigidBodySharedUserData* GetSharedUserDataFromRigidBody(btRigidBody *RigidBody)
{
	return (CBulletRigidBodySharedUserData*)RigidBody->getUserPointer();
}

CBulletConstraintSharedUserData* GetSharedUserDataFromConstraint(btTypedConstraint *Constraint)
{
	return (CBulletConstraintSharedUserData*)Constraint->getUserConstraintPtr();
}

CBulletCollisionShapeSharedUserData* GetSharedUserDataFromCollisionShape(btCollisionShape *pCollisionShape)
{
	return (CBulletCollisionShapeSharedUserData*)pCollisionShape->getUserPointer();
}

IPhysicObject* GetPhysicObjectFromRigidBody(btRigidBody *pRigidBody)
{
	auto pMotionState = (CBulletBaseMotionState *)pRigidBody->getMotionState();

	if (pMotionState)
	{
		return pMotionState->GetPhysicObject();
	}

	return nullptr;
}

void OnBeforeDeleteBulletCollisionShape(btCollisionShape* pCollisionShape)
{
	auto pSharedUserData = GetSharedUserDataFromCollisionShape(pCollisionShape);

	if (pSharedUserData)
	{
		delete pSharedUserData;

		pCollisionShape->setUserPointer(nullptr);
	}
}

void OnBeforeDeleteBulletRigidBody(btRigidBody* pRigidBody)
{
	auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

	if (pSharedUserData)
	{
		delete pSharedUserData;

		pRigidBody->setUserPointer(nullptr);
	}

	auto pCollisionShape = pRigidBody->getCollisionShape();

	if (pCollisionShape)
	{
		OnBeforeDeleteBulletCollisionShape(pCollisionShape);

		delete pCollisionShape;

		pRigidBody->setCollisionShape(nullptr);
	}

	auto pMotionState = pRigidBody->getMotionState();

	if (pMotionState)
	{
		delete pMotionState;

		pRigidBody->setMotionState(nullptr);
	}
}

void OnBeforeDeleteBulletConstraint(btTypedConstraint *pConstraint)
{
	auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

	if (pSharedUserData)
	{
		delete pSharedUserData;

		pConstraint->setUserConstraintPtr(nullptr);
	}
}

btScalar GetConstraintLinearErrorMagnitude(btTypedConstraint *pConstraint)
{
	if (pConstraint->getConstraintType() == CONETWIST_CONSTRAINT_TYPE)
	{
		auto pConeTwist = (btConeTwistConstraint*)pConstraint;

		auto worldPivotA = pConeTwist->getRigidBodyA().getWorldTransform() * pConeTwist->getFrameOffsetA();
		auto worldPivotB = pConeTwist->getRigidBodyB().getWorldTransform() * pConeTwist->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}
	else if (pConstraint->getConstraintType() == HINGE_CONSTRAINT_TYPE)
	{
		auto pHinge = (btHingeConstraint*)pConstraint;

		auto worldPivotA = pHinge->getRigidBodyA().getWorldTransform() * pHinge->getFrameOffsetA();
		auto worldPivotB = pHinge->getRigidBodyB().getWorldTransform() * pHinge->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}
	else if (pConstraint->getConstraintType() == D6_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btGeneric6DofConstraint*)pConstraint;

		auto worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		auto worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}

	else if (pConstraint->getConstraintType() == SLIDER_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btSliderConstraint*)pConstraint;

		auto worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		auto worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}

	return 0;
}

void CBulletEntityMotionState::getWorldTransform(btTransform& worldTrans) const
{
	if (GetPhysicObject()->IsStaticObject())
	{
		auto entindex = GetPhysicObject()->GetEntityIndex();
		auto ent = GetPhysicObject()->GetClientEntity();

		btVector3 vecOrigin(ent->curstate.origin[0], ent->curstate.origin[1], ent->curstate.origin[2]);

		worldTrans = btTransform(btQuaternion(0, 0, 0, 1), vecOrigin);

		btVector3 vecAngles(ent->curstate.angles[0], ent->curstate.angles[1], ent->curstate.angles[2]);

		//Brush uses reverted pitch ??
		if (ent->curstate.solid == SOLID_BSP)
		{
			vecAngles.setX(-vecAngles.x());
		}

		EulerMatrix(vecAngles, worldTrans.getBasis());
	}
}

void CBulletEntityMotionState::setWorldTransform(const btTransform& worldTrans)
{

}

ATTRIBUTE_ALIGNED16(class)
CBulletPhysicsDebugDraw : public btIDebugDraw
{
private:
	int m_debugMode{};
	DefaultColors m_ourColors{};

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletPhysicsDebugDraw() : m_debugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits)
	{

	}

	~CBulletPhysicsDebugDraw()
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
		//The texture must be reset to zero upon drawing.
		vgui::surface()->DrawSetTexture(-1);
		vgui::surface()->DrawSetTexture(0);

		gEngfuncs.pTriAPI->Color4fRendermode(color1.getX(), color1.getY(), color1.getZ(), 1.0f, kRenderTransAlpha);
		gEngfuncs.pTriAPI->Begin(TRI_LINES);

		vec3_t vecFrom = { from1.getX(), from1.getY(), from1.getZ() };
		vec3_t vecTo = { to1.getX(), to1.getY(), to1.getZ() };

		gEngfuncs.pTriAPI->Vertex3fv(vecFrom);
		gEngfuncs.pTriAPI->Vertex3fv(vecTo);
		gEngfuncs.pTriAPI->End();
	}

	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override
	{
		//TODO: Renderer API?

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

class CBulletOverlapFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const override
	{
		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		if (collides)
		{
			auto pClientObjectA = (btCollisionObject*)proxy0->m_clientObject;
			auto pClientObjectB = (btCollisionObject*)proxy1->m_clientObject;

			auto pRigidBodyA = btRigidBody::upcast(pClientObjectA);
			auto pRigidBodyB = btRigidBody::upcast(pClientObjectB);

			if (pRigidBodyA)
			{
				auto pPhysicObjectA = GetPhysicObjectFromRigidBody(pRigidBodyA);
				if (pPhysicObjectA && pPhysicObjectA->IsClientEntityNonSolid()) {
					return false;
				}
			}

			if (pRigidBodyB)
			{
				auto pPhysicObjectB = GetPhysicObjectFromRigidBody(pRigidBodyB);
				if (pPhysicObjectB && pPhysicObjectB->IsClientEntityNonSolid()) {
					return false;
				}
			}
		}
		return collides;
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

	m_overlapFilterCallback = new CBulletOverlapFilterCallback();
	m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(m_overlapFilterCallback);

	m_dynamicsWorld->setGravity(btVector3(0, 0, 0));
}

void CBulletPhysicManager::Shutdown()
{
	CBasePhysicManager::Shutdown();

	if (m_overlapFilterCallback) {
		delete m_overlapFilterCallback;
		m_overlapFilterCallback = nullptr;
		m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(nullptr);
	}

	if (m_debugDraw) {
		delete m_debugDraw;
		m_debugDraw = nullptr;
		m_dynamicsWorld->setDebugDrawer(nullptr);
	}

	if (m_dynamicsWorld) {
		delete m_dynamicsWorld;
		m_dynamicsWorld = nullptr;
	}

	if (m_solver) {
		delete m_solver;
		m_solver = nullptr;
	}

	if (m_overlappingPairCache) {
		delete m_overlappingPairCache;
		m_overlappingPairCache = nullptr;
	}

	if (m_dispatcher) {
		delete m_dispatcher;
		m_dispatcher = nullptr;
	}

	if (m_collisionConfiguration) {
		delete m_collisionConfiguration;
		m_collisionConfiguration = nullptr;
	}
}

void CBulletPhysicManager::NewMap(void)
{
	CBasePhysicManager::NewMap();
}

void CBulletPhysicManager::DebugDraw(void)
{
	CBasePhysicManager::DebugDraw();

	const auto &objectArray = m_dynamicsWorld->getCollisionObjectArray();
	for (size_t i = 0;i < objectArray.size(); ++i)
	{
		auto pCollisionObject = objectArray[i];

		auto pRigidBody = btRigidBody::upcast(pCollisionObject);
		
		if (pRigidBody)
		{
			auto pPhysicObject = GetPhysicObjectFromRigidBody(pRigidBody);
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (pPhysicObject->IsRagdollObject())
			{
				if (GetRagdollObjectDebugDrawLevel() >= pSharedUserData->m_debugDrawLevel)
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags &= ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pRigidBody->setCollisionFlags(iCollisionFlags);
				}
				else
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pCollisionObject->setCollisionFlags(iCollisionFlags);
				}
			}
			else if (pPhysicObject->IsStaticObject())
			{
				if (GetStaticObjectDebugDrawLevel() >= pSharedUserData->m_debugDrawLevel)
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags &= ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pCollisionObject->setCollisionFlags(iCollisionFlags);
				}
				else
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pCollisionObject->setCollisionFlags(iCollisionFlags);
				}
			}
		}
	}

	auto numConstraint = m_dynamicsWorld->getNumConstraints();

	for (int i = 0; i < numConstraint; ++i)
	{
		auto pConstraint = m_dynamicsWorld->getConstraint(i);

		auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

		if (pSharedUserData)
		{
			if (GetConstraintDebugDrawLevel() >= pSharedUserData->m_debugDrawLevel)
			{
				pConstraint->setDbgDrawSize(3);
			}
			else
			{
				pConstraint->setDbgDrawSize(0);
			}
		}
	}

	if (IsDebugDrawShowCCD())
	{
		int iDebugMode = m_debugDraw->getDebugMode();
		iDebugMode |= btIDebugDraw::DBG_EnableCCD;
		m_debugDraw->setDebugMode(iDebugMode);
	}
	else
	{
		int iDebugMode = m_debugDraw->getDebugMode();
		iDebugMode &= ~btIDebugDraw::DBG_EnableCCD;
		m_debugDraw->setDebugMode(iDebugMode);
	}

	m_dynamicsWorld->debugDrawWorld();
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

	m_dynamicsWorld->stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate());
}

void CBulletPhysicManager::AddPhysicObjectToWorld(IPhysicObject *PhysicObject)
{
	PhysicObject->AddToPhysicWorld(m_dynamicsWorld);
}

void CBulletPhysicManager::RemovePhysicObjectFromWorld(IPhysicObject* PhysicObject)
{
	PhysicObject->RemoveFromPhysicWorld(m_dynamicsWorld);
}

IStaticObject* CBulletPhysicManager::CreateStaticObject(const CStaticObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_pStaticObjectConfig)
	{
		gEngfuncs.Con_Printf("CreateStaticObject: invalid m_pStaticObjectConfig!\n");
		return nullptr;
	}

	return new CBulletStaticObject(CreationParam);
}

IDynamicObject* CBulletPhysicManager::CreateDynamicObject(const CDynamicObjectCreationParameter& CreationParam)
{
	return nullptr;
}

IRagdollObject* CBulletPhysicManager::CreateRagdollObject(const CRagdollObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_studiohdr)
	{
		gEngfuncs.Con_Printf("CreateRagdollObject: invalid m_studiohdr!\n");
		return nullptr;
	}

	if (!CreationParam.m_pRagdollObjectConfig)
	{
		gEngfuncs.Con_Printf("CreateRagdollObject: invalid m_pRagdollObjectConfig!\n");
		return nullptr;
	}

	return new CBulletRagdollObject(CreationParam);
}

IClientPhysicManager* BulletPhysicManager_CreateInstance()
{
	return new CBulletPhysicManager;
}