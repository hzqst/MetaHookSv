#pragma once

#include "BaseRagdollObject.h"
#include "BulletPhysicManager.h"

class CBulletRagdollObject : public CBaseRagdollObject
{
public:
	CBulletRagdollObject(const CRagdollObjectCreationParameter& CreationParam);

	~CBulletRagdollObject();

	bool GetGoldSrcOriginAngles(float* origin, float* angles) override;
	bool SetupBones(studiohdr_t* studiohdr) override;
	bool SyncThirdPersonView(struct ref_params_s* pparams, void(*callback)(struct ref_params_s* pparams)) override;
	bool SyncFirstPersonView(struct ref_params_s* pparams, void(*callback)(struct ref_params_s* pparams)) override;
	void FreePhysicActionsWithFilters(int with_flags, int without_flags) override;
	void Update(CPhysicObjectUpdateContext* ObjectUpdateContext) override;

protected:

	IPhysicRigidBody* CreateRigidBody(const CRagdollObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) override;
	IPhysicConstraint* CreateConstraint(const CRagdollObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) override;
	IPhysicAction* CreateAction(const CRagdollObjectCreationParameter& CreationParam, CClientPhysicActionConfig* pActionConfig, int physicComponentId) override;
	void SaveBoneRelativeTransform(const CRagdollObjectCreationParameter& CreationParam) override;

	void CheckConstraintLinearErrors(CPhysicObjectUpdateContext* ctx);

public:

	btTransform m_BoneRelativeTransform[128]{};
};