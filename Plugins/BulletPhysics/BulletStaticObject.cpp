#include "BulletStaticObject.h"
#include "BulletStaticRigidBody.h"

CBulletStaticObject::CBulletStaticObject(const CPhysicObjectCreationParameter& CreationParam) : CBaseStaticObject(CreationParam)
{

}

CBulletStaticObject::~CBulletStaticObject()
{

}

IPhysicRigidBody* CBulletStaticObject::CreateRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId)
{
	if (GetRigidBodyByName(pRigidConfig->name))
	{
		gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create duplicated rigidbody \"%s\".\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	if (pRigidConfig->mass < 0)
	{
		gEngfuncs.Con_Printf("CreateRigidBody: cannot create rigidbody \"%s\" because mass < 0.\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	auto pMotionState = BulletCreateMotionState(CreationParam, pRigidConfig, this);

	if (!pMotionState)
	{
		gEngfuncs.Con_Printf("CreateRigidBody: cannot create rigidbody \"%s\" because there is no MotionState available.\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	auto pCollisionShape = BulletCreateCollisionShape(pRigidConfig);

	if (!pCollisionShape)
	{
		delete pMotionState;

		gEngfuncs.Con_Printf("CreateRigidBody: cannot create rigidbody \"%s\" because there is no CollisionShape available.\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	if (pRigidConfig->mass > 0 && pCollisionShape->isNonMoving())
	{
		delete pMotionState;
		delete pCollisionShape;

		gEngfuncs.Con_Printf("CreateRigidBody: cannot create rigidbody \"%s\" because mass > 0 is not allowed when using non-moving CollisionShape.\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	btRigidBody::btRigidBodyConstructionInfo cInfo(0, pMotionState, pCollisionShape);

	cInfo.m_friction = 1;
	cInfo.m_rollingFriction = 1;
	cInfo.m_restitution = 1;

	int group = btBroadphaseProxy::DefaultFilter;

	if (CreationParam.m_entindex == 0)
		group |= BulletPhysicCollisionFilterGroups::WorldFilter;
	else
		group |= BulletPhysicCollisionFilterGroups::StaticObjectFilter;

	int mask = btBroadphaseProxy::AllFilter;

	mask &= ~(BulletPhysicCollisionFilterGroups::WorldFilter | BulletPhysicCollisionFilterGroups::StaticObjectFilter);

	mask &= ~(BulletPhysicCollisionFilterGroups::ConstraintFilter | BulletPhysicCollisionFilterGroups::ActionFilter);

	if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToDynamicObject)
		mask &= ~BulletPhysicCollisionFilterGroups::DynamicObjectFilter;

	if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToRagdollObject)
		mask &= ~BulletPhysicCollisionFilterGroups::RagdollObjectFilter;

	return new CBulletStaticRigidBody(
		physicComponentId ? physicComponentId : ClientPhysicManager()->AllocatePhysicComponentId(),
		CreationParam.m_entindex,
		this,
		pRigidConfig,
		cInfo,
		group,
		mask);
}

IPhysicConstraint* CBulletStaticObject::CreateConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId)
{
	return nullptr;
}

IPhysicAction* CBulletStaticObject::CreateAction(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicActionConfig* pActionConfig, int physicComponentId)
{
	return nullptr;
}