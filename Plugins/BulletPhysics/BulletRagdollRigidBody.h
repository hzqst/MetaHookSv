#pragma once

#include "BulletPhysicRigidBody.h"

class CBulletRagdollRigidBody : public CBulletPhysicRigidBody
{
public:
	CBulletRagdollRigidBody(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group,
		int mask);

	bool ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate) override;
	bool SetupBones(studiohdr_t* studiohdr) override;
	bool SetupJiggleBones(studiohdr_t* studiohdr) override;
	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override;
};

