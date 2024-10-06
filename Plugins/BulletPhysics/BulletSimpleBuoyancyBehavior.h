#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletSimpleBuoyancyBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletSimpleBuoyancyBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		float flMagnitude,
		float flLinearDamping,
		float flAngularDamping);

	const char* GetTypeString() const override;
	const char* GetTypeLocalizationTokenString() const override;
	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iSourceIndex{ 0 };
	float m_flMagnitude{ 0 };
	float m_flLinearDamping{ 0 };
	float m_flAngularDamping{ 0 };
};