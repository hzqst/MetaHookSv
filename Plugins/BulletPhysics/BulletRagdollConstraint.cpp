#include "BulletRagdollConstraint.h"

CBulletRagdollConstraint::CBulletRagdollConstraint(
	int id,
	int entindex,
	IPhysicObject* pPhysicObject,
	CClientConstraintConfig* pConstraintConfig,
	btTypedConstraint* pInternalConstraint) :

	CBulletPhysicConstraint(
		id,
		entindex,
		pPhysicObject,
		pConstraintConfig,
		pInternalConstraint)
{

}

void CBulletRagdollConstraint::Update(CPhysicComponentUpdateContext* ComponentUpdateContext)
{
	if (!m_pInternalConstraint)
		return;

	bool bDeactivate = false;

	bool bConstraintStateChanged = false;

	auto pPhysicObject = GetOwnerPhysicObject();

	if (!pPhysicObject)
		return;

	if (!pPhysicObject->IsRagdollObject())
		return;

	auto pRagdollObject = (IRagdollObject*)pPhysicObject;

	do
	{
		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_Idle && (m_flags & PhysicConstraintFlag_DeactiveOnNormalActivity))
		{
			bDeactivate = true;
			break;
		}

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_Death && (m_flags & PhysicConstraintFlag_DeactiveOnDeathActivity))
		{
			bDeactivate = true;
			break;
		}

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_CaughtByBarnacle && (m_flags & PhysicConstraintFlag_DeactiveOnCaughtByBarnacleActivity))
		{
			bDeactivate = true;
			break;
		}

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_BarnaclePulling && (m_flags & PhysicConstraintFlag_DeactiveOnBarnaclePullingActivity))
		{
			bDeactivate = true;
			break;
		}

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_BarnacleChewing && (m_flags & PhysicConstraintFlag_DeactiveOnBarnacleChewingActivity))
		{
			bDeactivate = true;
			break;
		}

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_GargantuaBite && (m_flags & PhysicConstraintFlag_DeactiveOnGargantuaBiteActivity))
		{
			bDeactivate = true;
			break;
		}

	} while (0);

	if (bDeactivate)
	{
		if (m_pInternalConstraint->isEnabled())
		{
			m_pInternalConstraint->setEnabled(false);

			bConstraintStateChanged = true;
		}
	}
	else
	{
		if (!m_pInternalConstraint->isEnabled())
		{
			m_pInternalConstraint->setEnabled(true);

			bConstraintStateChanged = true;
		}
	}

	if (bConstraintStateChanged)
	{
		ComponentUpdateContext->m_pObjectUpdateContext->m_bConstraintStateChanged = true;
	}
}