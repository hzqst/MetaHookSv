#pragma once

#include "BulletPhysicConstraint.h"

class CBulletRagdollConstraint : public CBulletPhysicConstraint
{
public:
	CBulletRagdollConstraint(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		CClientConstraintConfig* pConstraintConfig,
		btTypedConstraint* pInternalConstraint);

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override;
};
