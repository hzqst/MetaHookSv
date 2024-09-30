#pragma once

#include "BulletPhysicComponentAction.h"

class CBulletBarnacleDragForceAction : public CBulletPhysicComponentAction
{
public:
	CBulletBarnacleDragForceAction(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
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