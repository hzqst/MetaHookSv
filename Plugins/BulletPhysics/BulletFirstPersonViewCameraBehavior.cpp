#include "BulletFirstPersonViewCameraBehavior.h"

int ClientGetPlayerFlags();

CBulletFirstPersonViewCameraBehavior::CBulletFirstPersonViewCameraBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle,
	bool syncViewOrigin, bool syncViewAngles,
	bool useSimOrigin,
	float originalViewHeightStand,
	float originalViewHeightDuck,
	float mappedViewHeightStand,
	float mappedViewHeightDuck,
	float newViewHeightDucking)
	:
	CBulletCameraViewBehavior(id, entindex, pPhysicObject, pPhysicBehaviorConfig, attachedPhysicComponentId,
		activateOnIdle, activateOnDeath, activateOnCaughtByBarnacle,
		syncViewOrigin, syncViewAngles,
		useSimOrigin,
		originalViewHeightStand,
		originalViewHeightDuck,
		mappedViewHeightStand,
		mappedViewHeightDuck,
		newViewHeightDucking)
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

	if (m_bUseSimOrigin)
	{
		if (m_bSyncViewOrigin)
		{
			float currentViewHeight = pparams->viewheight[2];
			float newViewHeight = currentViewHeight;

			//currentViewHeight is between m_flOriginalViewHeightStand and m_flOriginalViewHeightDuck
			if (ClientGetPlayerFlags() & FL_DUCKING)
			{
				newViewHeight = m_flNewViewHeightDucking;
			}
			else
			{
				float lerp = 1;

				if (m_flOriginalViewHeightStand > m_flOriginalViewHeightDuck)
				{
					if (currentViewHeight >= m_flOriginalViewHeightDuck)
						lerp = (currentViewHeight - m_flOriginalViewHeightDuck) / (m_flOriginalViewHeightStand - m_flOriginalViewHeightDuck);
					else
						lerp = 0;
				}

				newViewHeight = m_flMappedViewHeightDuck + lerp * (m_flMappedViewHeightStand - m_flMappedViewHeightDuck);
			}

			pparams->viewheight[2] = newViewHeight;

			vec3_t viewangles, forward, right, up;
			VectorCopy(pparams->cl_viewangles, viewangles);
			viewangles[0] = 0;
			AngleVectors(viewangles, forward, right, up);
			VectorMA(pparams->viewheight, m_origin[0], forward, pparams->viewheight);
			VectorMA(pparams->viewheight, m_origin[1], right, pparams->viewheight);
			VectorMA(pparams->viewheight, m_origin[2], up, pparams->viewheight);
			
			//bypass V_CalcViewRoll
			//pparams->health = 1;
		}

		if (m_bSyncViewAngles)
		{
			//bypass V_CalcViewRoll
			pparams->health = 1;
		}

		int iSavedHealth = pparams->health;

		callback(pparams);

		pparams->health = iSavedHealth;

		return true;
	}
	else
	{
		auto pRigidBody = GetAttachedRigidBody();

		if (pRigidBody)
		{
			vec3_t vecGoldSrcNewOrigin, vecGoldSrcNewAngles;

			if (pRigidBody->GetGoldSrcOriginAnglesWithLocalOffset(m_origin, m_angles, vecGoldSrcNewOrigin, vecGoldSrcNewAngles))
			{
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
					//bypass V_CalcViewRoll
					pparams->health = 1;
				}

				callback(pparams);

				pparams->health = iSavedHealth;

				return true;
			}
		}
	}

	return false;
}