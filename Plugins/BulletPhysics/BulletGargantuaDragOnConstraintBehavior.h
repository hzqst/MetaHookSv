#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletGargantuaDragOnConstraintBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletGargantuaDragOnConstraintBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		int iSourceIndex,
		float flForceMagnitude,
		float flTargetVelocity,
		float flExtraHeight,
		int iLimitAxis,
		bool bUseServoMotor);

	const char* GetTypeString() const override;
	const char* GetTypeLocalizationTokenString() const override;
	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iSourceIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flTargetVelocity{ 0 };
	float m_flExtraHeight{ 0 };
	int m_iLimitAxis{ -1 };
	bool m_bUseServoMotor{};
};