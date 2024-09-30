#include "BulletThirdPersonViewCameraAction.h"

CBulletThirdPersonViewCameraAction::CBulletThirdPersonViewCameraAction(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
	int attachedPhysicComponentId) : CBulletCameraViewAction(id, entindex, pPhysicObject, pActionConfig, attachedPhysicComponentId)
{

}

const char* CBulletThirdPersonViewCameraAction::GetTypeString() const
{
	return "ThirdPersonViewCamera";
}

const char* CBulletThirdPersonViewCameraAction::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_ThirdPersonViewCamera";
}

bool CBulletThirdPersonViewCameraAction::SyncCameraView(struct ref_params_s* pparams, bool bIsThirdPersonView, void(*callback)(struct ref_params_s* pparams))
{
	if (bIsThirdPersonView)
	{
		auto pRigidBody = GetAttachedRigidBody();

		if (pRigidBody)
		{
			vec3_t vecGoldSrcNewOrigin;

			pRigidBody->GetGoldSrcOriginAnglesWithLocalOffset(m_origin, m_angles, vecGoldSrcNewOrigin, nullptr);

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