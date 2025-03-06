#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletCameraViewBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletCameraViewBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle,
		bool syncViewOrigin, bool syncViewAngles);

	const char* GetTypeString() const override;

	const char* GetTypeLocalizationTokenString() const override;

	bool IsCameraView() const override;

	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

	virtual bool ShouldSyncCameraView(bool bIsThirdPersonView, int iSyncViewLevel) const;

protected:
	bool m_bActivateOnIdle{};
	bool m_bActivateOnDeath{};
	bool m_bActivateOnCaughtByBarnacle{};
	bool m_bSyncViewOrigin{};
	bool m_bSyncViewAngles{};
};