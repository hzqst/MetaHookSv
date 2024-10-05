#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletBarnacleDragOnConstraintBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletBarnacleDragOnConstraintBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		int iSourceIndex,
		float flForceMagnitude, 
		float flTargetVelocity,
		float flExtraHeight,
		int iLimitAxis,
		bool bCalculateLimitFromActualPlayerOrigin,
		bool bUseServoMotor,
		bool bActivatedOnBarnaclePulling,
		bool bActivatedOnBarnacleChewing);

	const char* GetTypeString() const override;
	const char* GetTypeLocalizationTokenString() const override;
	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	float CalculateLimitValueFromActualPlayerOrigin(const btVector3& vStartOrigin);
private:
	int m_iSourceIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flTargetVelocity{ 0 };
	float m_flExtraHeight{ 24 };
	int m_iLimitAxis{ -1 };
	bool m_bCalculateLimitFromActualPlayerOrigin{};
	bool m_bUseServoMotor{};
	bool m_bActivatedOnBarnaclePulling{};
	bool m_bActivatedOnBarnacleChewing{};
};