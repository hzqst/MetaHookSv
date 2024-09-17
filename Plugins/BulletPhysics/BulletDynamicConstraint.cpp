#include "BulletDynamicConstraint.h"

CBulletDynamicConstraint::CBulletDynamicConstraint(
	int id,
	int entindex,
	IPhysicObject* pPhysicObject,
	CClientConstraintConfig* pConstraintConfig,
	btTypedConstraint* pInternalConstraint)
	:
	CBulletPhysicConstraint(
		id,
		entindex,
		pPhysicObject,
		pConstraintConfig,
		pInternalConstraint)
{

}

void CBulletDynamicConstraint::Update(CPhysicComponentUpdateContext* ComponentUpdateContext)
{
	//We have nothing to do, unlike ragdoll...
}