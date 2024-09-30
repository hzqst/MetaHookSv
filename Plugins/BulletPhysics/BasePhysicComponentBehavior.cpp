#include "BasePhysicComponentBehavior.h"
#include "PhysicUTIL.h"

CBasePhysicComponentBehavior::CBasePhysicComponentBehavior(
	int id, 
	int entindex, 
	IPhysicObject* pPhysicObject,
	const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId) :

	CBasePhysicBehavior(
		id,
		entindex,
		pPhysicObject, 
		pPhysicBehaviorConfig),

	m_attachedPhysicComponentId(attachedPhysicComponentId)
{
	VectorCopy(pPhysicBehaviorConfig->origin, m_origin);
	VectorCopy(pPhysicBehaviorConfig->angles, m_angles);
}

IPhysicComponent* CBasePhysicComponentBehavior::GetAttachedPhysicComponent() const
{
	return ClientPhysicManager()->GetPhysicComponent(m_attachedPhysicComponentId);
}

IPhysicRigidBody* CBasePhysicComponentBehavior::GetAttachedRigidBody() const
{
	return UTIL_GetPhysicComponentAsRigidBody(m_attachedPhysicComponentId);
}

IPhysicConstraint* CBasePhysicComponentBehavior::GetAttachedConstraint() const
{
	return UTIL_GetPhysicComponentAsConstraint(m_attachedPhysicComponentId);
}