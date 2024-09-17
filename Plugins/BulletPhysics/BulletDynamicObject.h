#pragma once

#include "BaseDynamicObject.h"
#include "BulletPhysicManager.h"

class CBulletDynamicObject : public CBaseDynamicObject
{
public:
	CBulletDynamicObject(const CDynamicObjectCreationParameter& CreationParam);

	~CBulletDynamicObject();

	IPhysicRigidBody* CreateRigidBody(const CDynamicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) override;
	IPhysicConstraint* CreateConstraint(const CDynamicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) override;

};
