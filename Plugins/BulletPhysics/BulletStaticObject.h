#pragma once

#include "BaseStaticObject.h"
#include "BulletPhysicManager.h"

class CBulletStaticObject : public CBaseStaticObject
{
public:
	CBulletStaticObject(const CStaticObjectCreationParameter& CreationParam);
	~CBulletStaticObject();

	IPhysicRigidBody* CreateRigidBody(const CStaticObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) override;
};
