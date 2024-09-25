#include "BulletPhysicComponentAction.h"

CBulletPhysicComponentAction::CBulletPhysicComponentAction(
	int id,
	int entindex, 
	IPhysicObject* pPhysicObject, 
	const CClientPhysicActionConfig* pActionConfig, 
	int attachedPhysicComponentId) :

	CBasePhysicComponentAction(
		id, 
		entindex,
		pPhysicObject, 
		pActionConfig, 
		attachedPhysicComponentId)
{
	m_pInternalRigidBody = CreateInternalRigidBody(pPhysicObject, pActionConfig, attachedPhysicComponentId);
}

CBulletPhysicComponentAction::~CBulletPhysicComponentAction()
{
	if (m_pInternalRigidBody)
	{
		FreeInternalRigidBody(m_pInternalRigidBody);
		m_pInternalRigidBody = nullptr;
	}
}

bool CBulletPhysicComponentAction::AddToPhysicWorld(void* world)
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
			dynamicWorld->addRigidBody(m_pInternalRigidBody, BulletPhysicCollisionFilterGroups::ConstraintFilter, BulletPhysicCollisionFilterGroups::InspectorFilter);
		}

		m_addedToPhysicWorld = true;

		return true;
	}

	gEngfuncs.Con_DPrintf("CBulletPhysicComponentAction::AddToPhysicWorld: already added to world!\n");
	return false;
}

bool CBulletPhysicComponentAction::RemoveFromPhysicWorld(void* world)
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

	gEngfuncs.Con_DPrintf("CBulletPhysicComponentAction::RemoveFromPhysicWorld: already removed from world!\n");
	return false;
}

bool CBulletPhysicComponentAction::IsAddedToPhysicWorld(void* world) const
{
	return m_addedToPhysicWorld;
}

btRigidBody* CBulletPhysicComponentAction::CreateInternalRigidBody(IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig, int attachedPhysicComponentId)
{
	btVector3 vecOrigin(pActionConfig->origin[0], pActionConfig->origin[1], pActionConfig->origin[2]);

	btVector3 vecAngles(pActionConfig->angles[0], pActionConfig->angles[1], pActionConfig->angles[2]);

	Vector3GoldSrcToBullet(vecOrigin);

	btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

	EulerMatrix(vecAngles, localTrans.getBasis());

	auto pMotionState = new CFollowPhysicComponentMotionState(pPhysicObject, attachedPhysicComponentId, localTrans);

	auto size = btVector3(2, 2, 2);

	Vector3GoldSrcToBullet(size);

	auto pCollisionShape = new btBoxShape(size);

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

void CBulletPhysicComponentAction::FreeInternalRigidBody(btRigidBody* pRigidBody)
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