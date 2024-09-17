#pragma once

#include "BulletPhysicRigidBody.h"

class CBulletStaticRigidBody : public CBulletPhysicRigidBody
{
public:
	CBulletStaticRigidBody(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group, int mask);

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override;
};