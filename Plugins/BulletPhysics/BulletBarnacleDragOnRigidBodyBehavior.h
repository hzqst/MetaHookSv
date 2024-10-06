#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletBarnacleDragOnRigidBodyBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletBarnacleDragOnRigidBodyBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		int iSourceIndex, float flForceMagnitude, float flExtraHeight);

	const char* GetTypeString() const override;
	const char* GetTypeLocalizationTokenString() const override;
	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iSourceIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flExtraHeight{ 24 };
};