#include "BulletStaticObject.h"
#include "BulletStaticRigidBody.h"

CBulletStaticObject::CBulletStaticObject(const CPhysicObjectCreationParameter& CreationParam) : CBaseStaticObject(CreationParam)
{
	if (CreationParam.m_model->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
	}

	DispatchBuildPhysicComponents(
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_ActionConfigs,
		std::bind(&CBaseStaticObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::CreateAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseStaticObject::AddAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);
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

	auto pMotionState = BulletCreateMotionState(CreationParam, pRigidConfig, this);

	if (!pMotionState)
	{
		gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create rigid body for StaticObject because there is no MotionState available.\n");
		return nullptr;
	}

	auto pCollisionShape = BulletCreateCollisionShape(pRigidConfig);

	if (!pCollisionShape)
	{
		delete pMotionState;

		gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create rigid body for StaticObject because there is no CollisionShape available.\n");
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