#pragma once

#include "BulletCameraViewBehavior.h"

class CBulletFirstPersonViewCameraBehavior : public CBulletCameraViewBehavior
{
public:
	CBulletFirstPersonViewCameraBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	bool SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams)) override;
};
