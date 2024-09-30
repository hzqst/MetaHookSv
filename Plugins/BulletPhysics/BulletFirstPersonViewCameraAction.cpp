#include "BulletFirstPersonViewCameraAction.h"

CBulletFirstPersonViewCameraAction::CBulletFirstPersonViewCameraAction(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
	int attachedPhysicComponentId) : CBulletCameraViewAction(id, entindex, pPhysicObject, pActionConfig, attachedPhysicComponentId)
{

}

const char* CBulletFirstPersonViewCameraAction::GetTypeString() const
{
	return "FirstPersonViewCamera";
}

const char* CBulletFirstPersonViewCameraAction::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_FirstPersonViewCamera";
}

bool CBulletFirstPersonViewCameraAction::SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams))
{
	if (!bIsThirdPersonView)
	{
		auto pRigidBody = GetAttachedRigidBody();

		if (pRigidBody)
		{
			vec3_t vecGoldSrcNewOrigin, vecGoldSrcNewAngles;

			if (pRigidBody->GetGoldSrcOriginAnglesWithLocalOffset(m_origin, m_angles, vecGoldSrcNewOrigin, vecGoldSrcNewAngles))
			{
				vec3_t vecSavedSimOrgigin;
				vec3_t vecSavedClientViewAngles;
				VectorCopy(pparams->simorg, vecSavedSimOrgigin);
				VectorCopy(pparams->cl_viewangles, vecSavedClientViewAngles);
				int iSavedHealth = pparams->health;

				pparams->viewheight[2] = 0;
				VectorCopy(vecGoldSrcNewOrigin, pparams->simorg);
				VectorCopy(vecGoldSrcNewAngles, pparams->cl_viewangles);
				pparams->health = 1;

				callback(pparams);

				pparams->health = iSavedHealth;
				VectorCopy(vecSavedSimOrgigin, pparams->simorg);
				VectorCopy(vecSavedClientViewAngles, pparams->cl_viewangles);

				return true;
			}
		}
	}

	return false;
}