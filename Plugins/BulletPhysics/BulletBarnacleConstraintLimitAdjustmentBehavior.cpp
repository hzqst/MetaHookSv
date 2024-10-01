#include "BulletBarnacleConstraintLimitAdjustmentBehavior.h"

CBulletBarnacleConstraintLimitAdjustmentBehavior::CBulletBarnacleConstraintLimitAdjustmentBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iBarnacleIndex, float flInterval, float flExtraHeight, int iLimitAxis, int iBarnacleSequence) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iBarnacleIndex(iBarnacleIndex),
	m_flInterval(flInterval),
	m_flExtraHeight(flExtraHeight),
	m_iLimitAxis(iLimitAxis),
	m_iBarnacleSequence(iBarnacleSequence)
{

}

const char* CBulletBarnacleConstraintLimitAdjustmentBehavior::GetTypeString() const
{
	return "BarnacleConstraintLimitAdjustment";
}

const char* CBulletBarnacleConstraintLimitAdjustmentBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_BarnacleConstraintLimitAdjustment";
}

void CBulletBarnacleConstraintLimitAdjustmentBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

	if (!pBarnacleObject)
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pConstraint = GetAttachedConstraint();

	if (!pConstraint)
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (pBarnacleObject->GetClientEntityState()->sequence == m_iBarnacleSequence)
	{
		if (gEngfuncs.GetClientTime() > m_flNextAdjustmentTime)
		{
			pConstraint->ExtendLinearLimit(m_iLimitAxis, m_flExtraHeight);

			m_flNextAdjustmentTime = gEngfuncs.GetClientTime() + m_flInterval;
		}
	}
}