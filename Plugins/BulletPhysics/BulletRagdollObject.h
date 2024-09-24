#pragma once

#include "BaseRagdollObject.h"
#include "BulletPhysicManager.h"

class CBulletRagdollObject : public CBaseRagdollObject
{
public:
	CBulletRagdollObject(const CPhysicObjectCreationParameter& CreationParam);

	~CBulletRagdollObject();

	bool SetupBones(studiohdr_t* studiohdr) override;
	void Update(CPhysicObjectUpdateContext* ObjectUpdateContext) override;

protected:

	IPhysicRigidBody* CreateRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) override;
	IPhysicConstraint* CreateConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) override;
	IPhysicAction* CreateAction(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicActionConfig* pActionConfig, int physicComponentId) override;

	void SaveBoneRelativeTransform(const CPhysicObjectCreationParameter& CreationParam) override;

	void CheckConstraintLinearErrors(CPhysicObjectUpdateContext* ctx);

public:

	btTransform m_BoneRelativeTransform[128]{};
};