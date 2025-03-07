#pragma once

#include "BulletCameraViewBehavior.h"

class CBulletFirstPersonViewCameraBehavior : public CBulletCameraViewBehavior
{
public:
	CBulletFirstPersonViewCameraBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle,
		bool syncViewOrigin, bool syncViewAngles,
		bool useSimOrigin,
		float originalViewHeightStand,
		float originalViewHeightDuck,
		float mappedViewHeightStand,
		float mappedViewHeightDuck,
		float newViewHeightDucking);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	bool ShouldSyncCameraView(bool bIsThirdPersonView, int iSyncViewLevel) const override;

	bool SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, int iSyncViewLevel, void(*callback)(struct ref_params_s* pparams)) override;
};
