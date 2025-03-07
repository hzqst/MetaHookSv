#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletCameraViewBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletCameraViewBehavior(
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

	bool IsCameraView() const override;

	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

	virtual bool ShouldSyncCameraView(bool bIsThirdPersonView, int iSyncViewLevel) const;

protected:
	bool m_bActivateOnIdle{};
	bool m_bActivateOnDeath{};
	bool m_bActivateOnCaughtByBarnacle{};
	bool m_bSyncViewOrigin{};
	bool m_bSyncViewAngles{};
	bool m_bUseSimOrigin{};
	float m_flOriginalViewHeightStand{};
	float m_flOriginalViewHeightDuck{};
	float m_flMappedViewHeightStand{};
	float m_flMappedViewHeightDuck{};
	float m_flNewViewHeightDucking{};
};