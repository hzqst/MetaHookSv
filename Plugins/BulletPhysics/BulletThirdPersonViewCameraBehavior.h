#pragma once

#include "BulletCameraViewBehavior.h"

class CBulletThirdPersonViewCameraBehavior : public CBulletCameraViewBehavior
{
public:
	CBulletThirdPersonViewCameraBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle,
		bool syncViewOrigin, bool syncViewAngles);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	bool ShouldSyncCameraView(bool bIsThirdPersonView, int iSyncViewLevel) const override;

	bool SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, int iSyncViewLevel, void(*callback)(struct ref_params_s* pparams)) override;
};
