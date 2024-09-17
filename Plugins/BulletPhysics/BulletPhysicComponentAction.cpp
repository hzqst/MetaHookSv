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
	//TODO create m_pInternalRigidBody
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

		ClientPhysicManager()->OnPhysicComponentAddedIntoPhysicWorld(this);

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

		ClientPhysicManager()->OnPhysicComponentRemovedFromPhysicWorld(this);

		return true;
	}

	gEngfuncs.Con_DPrintf("CBulletPhysicComponentAction::RemoveFromPhysicWorld: already removed from world!\n");
	return false;
}

bool CBulletPhysicComponentAction::IsAddedToPhysicWorld(void* world) const
{
	return m_addedToPhysicWorld;
}