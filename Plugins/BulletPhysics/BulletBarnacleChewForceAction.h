#pragma once

#include "BulletPhysicComponentAction.h"

class CBulletBarnacleChewForceAction : public CBulletPhysicComponentAction
{
public:
	CBulletBarnacleChewForceAction(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
		int attachedPhysicComponentId,
		int iBarnacleIndex, float flForceMagnitude, float flInterval);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iBarnacleIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flInterval{ 1 };
	float m_flNextChewTime{ 0 };
};
