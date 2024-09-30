#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletBarnacleConstraintLimitAdjustmentBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletBarnacleConstraintLimitAdjustmentBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysixBehaviorConfig,
		int attachedPhysicComponentId,
		int iBarnacleIndex, float flInterval, float flExtraHeight, int iLimitAxis);

	const char* GetTypeString() const override;
	const char* GetTypeLocalizationTokenString() const override;
	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iBarnacleIndex{ 0 };
	float m_flInterval{ 1 };
	float m_flExtraHeight{ 0 };
	int m_iLimitAxis{ -1 };
	float m_flNextAdjustmentTime{ 0 };
};
