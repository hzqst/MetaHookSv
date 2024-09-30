#pragma once

#include "BaseDynamicObject.h"
#include "BulletPhysicManager.h"

class CBulletDynamicObject : public CBaseDynamicObject
{
public:
	CBulletDynamicObject(const CPhysicObjectCreationParameter& CreationParam);

	~CBulletDynamicObject();

	IPhysicRigidBody* CreateRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) override;
	IPhysicConstraint* CreateConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) override;
	IPhysicBehavior* CreatePhysicBehavior(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, int physicComponentId) override;
};
