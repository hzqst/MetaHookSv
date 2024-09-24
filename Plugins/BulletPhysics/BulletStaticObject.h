#pragma once

#include "BaseStaticObject.h"
#include "BulletPhysicManager.h"

class CBulletStaticObject : public CBaseStaticObject
{
public:
	CBulletStaticObject(const CPhysicObjectCreationParameter& CreationParam);
	~CBulletStaticObject();

	IPhysicRigidBody* CreateRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) override;
	IPhysicConstraint* CreateConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) override;
	IPhysicAction* CreateAction(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicActionConfig* pActionConfig, int physicComponentId) override;
};
