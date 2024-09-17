#pragma once

#include "BasePhysicAction.h"

class CBasePhysicComponentAction : public CBasePhysicAction
{
public:
	CBasePhysicComponentAction(
		int id,
		int entindex, 
		IPhysicObject* pPhysicObject, 
		const CClientPhysicActionConfig* pActionConfig, 
		int attachedPhysicComponentId);

	IPhysicComponent* GetAttachedPhysicComponent() const override;
	IPhysicRigidBody* GetAttachedRigidBody() const override;
	IPhysicConstraint* GetAttachedConstraint() const override;

	int m_attachedPhysicComponentId{};
};