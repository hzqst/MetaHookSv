#include "BulletBarnacleConstraintLimitAdjustmentBehavior.h"

CBulletBarnacleConstraintLimitAdjustmentBehavior::CBulletBarnacleConstraintLimitAdjustmentBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iBarnacleIndex, float flExtraHeight, float flInterval, int iLimitAxis) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iBarnacleIndex(iBarnacleIndex),
	m_flExtraHeight(flExtraHeight),
	m_flInterval(flInterval),
	m_iLimitAxis(iLimitAxis)
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
		gEngfuncs.Con_DPrintf("CBulletBarnacleConstraintLimitAdjustmentBehavior::Update: Invalid barnacle object!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!pBarnacleObject->IsRagdollObject())
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleConstraintLimitAdjustmentBehavior::Update: Barnacle must be RagdollObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleConstraintLimitAdjustmentBehavior::Update: Barnacle must have PhysicObjectFlag_Barnacle!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pConstraint = GetAttachedConstraint();

	if (!pConstraint)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: The attached PhysicComponent must be a constraint!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pBarnacleRagdollObject = (IRagdollObject*)pBarnacleObject;

	if (pBarnacleRagdollObject->GetActivityType() == StudioAnimActivityType_BarnacleChewing)
	{
		if (gEngfuncs.GetClientTime() > m_flNextAdjustmentTime)
		{
			pConstraint->ExtendLinearLimit(m_iLimitAxis, m_flExtraHeight);

			m_flNextAdjustmentTime = gEngfuncs.GetClientTime() + m_flInterval;
		}
	}
}