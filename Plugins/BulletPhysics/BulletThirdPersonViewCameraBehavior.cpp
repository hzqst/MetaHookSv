#include "BulletThirdPersonViewCameraBehavior.h"

CBulletThirdPersonViewCameraBehavior::CBulletThirdPersonViewCameraBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle,
	bool syncViewOrigin, bool syncViewAngles) :
	CBulletCameraViewBehavior(id, entindex, pPhysicObject, pPhysicBehaviorConfig, attachedPhysicComponentId,
		activateOnIdle, activateOnDeath, activateOnCaughtByBarnacle,
		syncViewOrigin, syncViewAngles)
{

}

const char* CBulletThirdPersonViewCameraBehavior::GetTypeString() const
{
	return "ThirdPersonViewCamera";
}

const char* CBulletThirdPersonViewCameraBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_ThirdPersonViewCamera";
}

bool CBulletThirdPersonViewCameraBehavior::ShouldSyncCameraView(bool bIsThirdPersonView, int iSyncViewLevel) const
{
	if (!CBulletCameraViewBehavior::ShouldSyncCameraView(bIsThirdPersonView, iSyncViewLevel))
		return false;

	if (!bIsThirdPersonView)
		return false;

	return true;
}

bool CBulletThirdPersonViewCameraBehavior::SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, int iSyncViewLevel, void(*callback)(struct ref_params_s* pparams))
{
	if (!ShouldSyncCameraView(bIsThirdPersonView, iSyncViewLevel))
		return false;

	auto pRigidBody = GetAttachedRigidBody();

	if (pRigidBody)
	{
		vec3_t vecGoldSrcNewOrigin;

		if (pRigidBody->GetGoldSrcOriginAnglesWithLocalOffset(m_origin, m_angles, vecGoldSrcNewOrigin, nullptr))
		{
			vec3_t vecSavedSimOrgigin;

			VectorCopy(pparams->simorg, vecSavedSimOrgigin);

			VectorCopy(vecGoldSrcNewOrigin, pparams->simorg);

			//int iSavedHealth = pparams->health;

			//pparams->health = 1;

			callback(pparams);

			//pparams->health = iSavedHealth;

			VectorCopy(vecSavedSimOrgigin, pparams->simorg);

			return true;
		}
	}

	return false;
}