#include "BulletFirstPersonViewCameraBehavior.h"

CBulletFirstPersonViewCameraBehavior::CBulletFirstPersonViewCameraBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle,
	bool syncViewOrigin, bool syncViewAngles) :
	CBulletCameraViewBehavior(id, entindex, pPhysicObject, pPhysicBehaviorConfig, attachedPhysicComponentId,
		activateOnIdle, activateOnDeath, activateOnCaughtByBarnacle,
		syncViewOrigin, syncViewAngles)
{

}

const char* CBulletFirstPersonViewCameraBehavior::GetTypeString() const
{
	return "FirstPersonViewCamera";
}

const char* CBulletFirstPersonViewCameraBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_FirstPersonViewCamera";
}

bool CBulletFirstPersonViewCameraBehavior::ShouldSyncCameraView(bool bIsThirdPersonView, int iSyncViewLevel) const
{
	if (!CBulletCameraViewBehavior::ShouldSyncCameraView(bIsThirdPersonView, iSyncViewLevel))
		return false;

	if (bIsThirdPersonView)
		return false;

	return true;
}

bool CBulletFirstPersonViewCameraBehavior::SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, int iSyncViewLevel, void(*callback)(struct ref_params_s* pparams))
{
	if (!ShouldSyncCameraView(bIsThirdPersonView, iSyncViewLevel))
		return false;

	auto pRigidBody = GetAttachedRigidBody();

	if (pRigidBody)
	{
		vec3_t vecGoldSrcNewOrigin, vecGoldSrcNewAngles;

		if (pRigidBody->GetGoldSrcOriginAnglesWithLocalOffset(m_origin, m_angles, vecGoldSrcNewOrigin, vecGoldSrcNewAngles))
		{
			vec3_t vecSavedSimOrgigin;
			vec3_t vecSavedViewHeight;
			vec3_t vecSavedClientViewAngles;
			VectorCopy(pparams->simorg, vecSavedSimOrgigin);
			VectorCopy(pparams->viewheight, vecSavedViewHeight);
			VectorCopy(pparams->cl_viewangles, vecSavedClientViewAngles);

			//VectorClear(pparams->viewheight);
			//VectorCopy(vecGoldSrcNewOrigin, pparams->simorg);

			if (m_bSyncViewOrigin)
			{
				vec3_t vecOffset = { 0 };
				VectorSubtract(vecGoldSrcNewOrigin, pparams->simorg, vecOffset);
				VectorCopy(vecOffset, pparams->viewheight);
			}

			if (m_bSyncViewAngles)
			{
				//The pitch is reverted for player view.
				vecGoldSrcNewAngles[0] *= -1;
				VectorCopy(vecGoldSrcNewAngles, pparams->cl_viewangles);
			}

			int iSavedHealth = pparams->health;

			if (m_bSyncViewAngles)
			{
				pparams->health = 1;
			}

			callback(pparams);

			pparams->health = iSavedHealth;

			VectorCopy(vecSavedSimOrgigin, pparams->simorg);
			VectorCopy(vecSavedViewHeight, pparams->viewheight);
			VectorCopy(vecSavedClientViewAngles, pparams->cl_viewangles);

			return true;
		}
	}

	return false;
}