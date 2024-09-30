#pragma once

#include "BulletPhysicComponentAction.h"

class CBulletCameraViewAction : public CBulletPhysicComponentAction
{
public:
	CBulletCameraViewAction(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
		int attachedPhysicComponentId);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	bool IsCameraView() const override;

	void Update(CPhysicComponentUpdateContext* ComponentContext) override;
};