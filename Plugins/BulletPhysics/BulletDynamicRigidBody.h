#pragma once

#include "BulletPhysicRigidBody.h"

class CBulletDynamicRigidBody : public CBulletPhysicRigidBody
{
public:
	CBulletDynamicRigidBody(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group, int mask);

	bool SetupJiggleBones(studiohdr_t* studiohdr, int flags) override;
	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override;
};
