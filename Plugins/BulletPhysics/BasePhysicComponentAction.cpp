#include "BasePhysicComponentAction.h"
#include "PhysicUTIL.h"

CBasePhysicComponentAction::CBasePhysicComponentAction(
	int id, 
	int entindex, 
	IPhysicObject* pPhysicObject,
	const CClientPhysicActionConfig* pActionConfig,
	int attachedPhysicComponentId) :

	CBasePhysicAction(
		id,
		entindex,
		pPhysicObject, 
		pActionConfig),

	m_attachedPhysicComponentId(attachedPhysicComponentId)
{
	VectorCopy(pActionConfig->origin, m_origin);
	VectorCopy(pActionConfig->angles, m_angles);
}

IPhysicComponent* CBasePhysicComponentAction::GetAttachedPhysicComponent() const
{
	return ClientPhysicManager()->GetPhysicComponent(m_attachedPhysicComponentId);
}

IPhysicRigidBody* CBasePhysicComponentAction::GetAttachedRigidBody() const
{
	return UTIL_GetPhysicComponentAsRigidBody(m_attachedPhysicComponentId);
}

IPhysicConstraint* CBasePhysicComponentAction::GetAttachedConstraint() const
{
	return UTIL_GetPhysicComponentAsConstraint(m_attachedPhysicComponentId);
}