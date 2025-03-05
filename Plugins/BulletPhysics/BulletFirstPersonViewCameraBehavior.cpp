#include "BulletFirstPersonViewCameraBehavior.h"

CBulletFirstPersonViewCameraBehavior::CBulletFirstPersonViewCameraBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle) : 
	CBulletCameraViewBehavior(id, entindex, pPhysicObject, pPhysicBehaviorConfig, attachedPhysicComponentId,
		activateOnIdle, activateOnDeath, activateOnCaughtByBarnacle)
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

bool CBulletFirstPersonViewCameraBehavior::SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, int iSyncViewLevel, void(*callback)(struct ref_params_s* pparams))
{
	if (!ShouldSyncCameraView())
		return false;

	if (!bIsThirdPersonView)
	{
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
				
				pparams->viewheight[2] = 0;
				//VectorCopy(vecGoldSrcNewOrigin, pparams->simorg);
				vec3_t vecOffset = { 0 };
				VectorSubtract(vecGoldSrcNewOrigin, pparams->simorg, vecOffset);
				VectorCopy(vecOffset, pparams->viewheight);

				//The pitch is reverted for player view.
				if (iSyncViewLevel >= 2)
				{
					vecGoldSrcNewAngles[0] *= -1;
					VectorCopy(vecGoldSrcNewAngles, pparams->cl_viewangles);
				}

				int iSavedHealth = pparams->health;
				pparams->health = 1;

				callback(pparams);

				pparams->health = iSavedHealth;

				VectorCopy(vecSavedSimOrgigin, pparams->simorg);
				VectorCopy(vecSavedViewHeight, pparams->viewheight);
				VectorCopy(vecSavedClientViewAngles, pparams->cl_viewangles);

				return true;
			}
		}
	}

	return false;
}