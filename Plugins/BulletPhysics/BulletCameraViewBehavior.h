#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletCameraViewBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletCameraViewBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId, bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	bool IsCameraView() const override;

	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

	virtual bool ShouldSyncCameraView() const;

protected:
	bool m_bActivateOnIdle{};
	bool m_bActivateOnDeath{};
	bool m_bActivateOnCaughtByBarnacle{};
};