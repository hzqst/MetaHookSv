#pragma once

#include "BulletCameraViewAction.h"

class CBulletFirstPersonViewCameraAction : public CBulletCameraViewAction
{
public:
	CBulletFirstPersonViewCameraAction(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
		int attachedPhysicComponentId);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	bool SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams)) override;
};
