#pragma once

#include "BulletPhysicComponentAction.h"

class CBulletBarnacleConstraintLimitAdjustmentAction : public CBulletPhysicComponentAction
{
public:
	CBulletBarnacleConstraintLimitAdjustmentAction(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
		int attachedPhysicComponentId,
		int iBarnacleIndex, float flInterval, float flExtraHeight, int iLimitAxis) :

		CBulletPhysicComponentAction(
			id,
			entindex,
			pPhysicObject,
			pActionConfig,
			attachedPhysicComponentId),

		m_iBarnacleIndex(iBarnacleIndex),
		m_flInterval(flInterval),
		m_flExtraHeight(flExtraHeight),
		m_iLimitAxis(iLimitAxis)
	{

	}

	void Update(CPhysicComponentUpdateContext* ComponentContext) override
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

		if (pBarnacleObject->GetClientEntityState()->sequence == 5)
		{
			if (gEngfuncs.GetClientTime() > m_flNextAdjustmentTime)
			{
				pConstraint->ExtendLinearLimit(m_iLimitAxis, m_flExtraHeight);

				m_flNextAdjustmentTime = gEngfuncs.GetClientTime() + m_flInterval;
			}
		}
	}

	int m_iBarnacleIndex{ 0 };
	float m_flInterval{ 1 };
	float m_flExtraHeight{ 0 };
	int m_iLimitAxis{ -1 };
	float m_flNextAdjustmentTime{ 0 };
};
