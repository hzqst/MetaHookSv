#pragma once

#include "BulletPhysicConstraint.h"

class CBulletDynamicConstraint : public CBulletPhysicConstraint
{
public:
	CBulletDynamicConstraint(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		CClientConstraintConfig* pConstraintConfig,
		btTypedConstraint* pInternalConstraint);

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override;
};
