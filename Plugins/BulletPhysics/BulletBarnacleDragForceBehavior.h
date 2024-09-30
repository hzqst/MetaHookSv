#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletBarnacleDragForceBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletBarnacleDragForceBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		int iBarnacleIndex, float flForceMagnitude, float flExtraHeight);

	const char* GetTypeString() const override;
	const char* GetTypeLocalizationTokenString() const override;
	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iBarnacleIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flExtraHeight{ 24 };
};