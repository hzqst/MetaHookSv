#include "BulletPhysicComponentBehavior.h"

CBulletPhysicComponentBehavior::CBulletPhysicComponentBehavior(
	int id,
	int entindex, 
	IPhysicObject* pPhysicObject, 
	const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId) :

	CBasePhysicComponentBehavior(
		id, 
		entindex,
		pPhysicObject, 
		pPhysicBehaviorConfig,
		attachedPhysicComponentId)
{
	btVector3 vecOrigin = GetVector3FromVec3(m_origin);

	btVector3 vecAngles = GetVector3FromVec3(m_angles);

	Vector3GoldSrcToBullet(vecOrigin);

	m_offsetmatrix = btTransform(btQuaternion(0, 0, 0, 1), vecOrigin);

	EulerMatrix(vecAngles, m_offsetmatrix.getBasis());

	m_pInternalRigidBody = CreateInternalRigidBody(pPhysicObject, pPhysicBehaviorConfig, attachedPhysicComponentId);
}

CBulletPhysicComponentBehavior::~CBulletPhysicComponentBehavior()
{
	if (m_pInternalRigidBody)
	{
		FreeInternalRigidBody(m_pInternalRigidBody);
		m_pInternalRigidBody = nullptr;
	}
}

bool CBulletPhysicComponentBehavior::AddToPhysicWorld(void* world)
{
	auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

	if (!m_addedToPhysicWorld)
	{
		auto pAttachedPhysicComponent = ClientPhysicManager()->GetPhysicComponent(m_attachedPhysicComponentId);

		if (!pAttachedPhysicComponent)
		{
			gEngfuncs.Con_DPrintf("CBulletPhysicComponentAction::AddToPhysicWorld: pAttachedPhysicComponent not present !\n");
			return false;
		}

		if (!pAttachedPhysicComponent->IsAddedToPhysicWorld(world))
		{
			gEngfuncs.Con_DPrintf("CBulletPhysicComponentAction::AddToPhysicWorld: pRigidBodyA not added to world !\n");
			return false;
		}

		if (m_pInternalRigidBody)
		{
			dynamicWorld->addRigidBody(m_pInternalRigidBody, BulletPhysicCollisionFilterGroups::PhysicBehaviorFilter, BulletPhysicCollisionFilterGroups::InspectorFilter);
		}

		m_addedToPhysicWorld = true;

		return true;
	}

	gEngfuncs.Con_DPrintf("CBulletPhysicComponentAction::AddToPhysicWorld: already added to world!\n");
	return false;
}

bool CBulletPhysicComponentBehavior::RemoveFromPhysicWorld(void* world)
{
	auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

	if (m_addedToPhysicWorld)
	{
		if (m_pInternalRigidBody)
		{
			dynamicWorld->removeRigidBody(m_pInternalRigidBody);
		}

		m_addedToPhysicWorld = false;

		return true;
	}

	gEngfuncs.Con_DPrintf("CBulletPhysicComponentBehavior::RemoveFromPhysicWorld: already removed from world!\n");
	return false;
}

bool CBulletPhysicComponentBehavior::IsAddedToPhysicWorld(void* world) const
{
	return m_addedToPhysicWorld;
}

btRigidBody* CBulletPhysicComponentBehavior::CreateInternalRigidBody(IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pActionConfig, int attachedPhysicComponentId)
{
	auto pMotionState = new CFollowPhysicComponentMotionState(pPhysicObject, attachedPhysicComponentId, m_offsetmatrix);

	auto size = btVector3(2, 2, 2);

	Vector3GoldSrcToBullet(size);

	auto pCollisionShape = new btBoxShape(size);// new btSphereShape(size.getX());//

	btRigidBody::btRigidBodyConstructionInfo cInfo(0, pMotionState, pCollisionShape);

	auto pRigidBody = new btRigidBody(cInfo);

	pRigidBody->setUserIndex(m_id);
	pRigidBody->setUserPointer(this);
	pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	//pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
	//pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | BulletPhysicCollisionFlags::CF_DISABLE_VISUALIZE_OBJECT_PERMANENT);

	pRigidBody->setActivationState(DISABLE_DEACTIVATION);

	return pRigidBody;
}

void CBulletPhysicComponentBehavior::FreeInternalRigidBody(btRigidBody* pRigidBody)
{
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

	delete pRigidBody;
}