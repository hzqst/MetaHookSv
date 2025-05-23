#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletBarnacleChewBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletBarnacleChewBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		int iSourceIndex, float flForceMagnitude, float flInterval);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iSourceIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flInterval{ 1 };
	float m_flNextChewTime{ 0 };
};
