#pragma once

#include "BasePhysicBehavior.h"

class CBasePhysicComponentBehavior : public CBasePhysicBehavior
{
public:
	CBasePhysicComponentBehavior(
		int id,
		int entindex, 
		IPhysicObject* pPhysicObject, 
		const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, 
		int attachedPhysicComponentId);

	IPhysicComponent* GetAttachedPhysicComponent() const override;
	IPhysicRigidBody* GetAttachedRigidBody() const override;
	IPhysicConstraint* GetAttachedConstraint() const override;

	int m_attachedPhysicComponentId{};
	vec3_t m_origin{};
	vec3_t m_angles{};
};