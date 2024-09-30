#include "BulletCameraViewBehavior.h"

CBulletCameraViewBehavior::CBulletCameraViewBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId, bool activateOnIdle, bool activateOnDeath, bool activateOnCaughtByBarnacle) :
	m_bActivateOnIdle(activateOnIdle),
	m_bActivateOnDeath(activateOnDeath),
	m_bActivateOnCaughtByBarnacle(activateOnCaughtByBarnacle),
	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId)
{

}

const char* CBulletCameraViewBehavior::GetTypeString() const
{
	return "CameraView";
}

const char* CBulletCameraViewBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_CameraView";
}

bool CBulletCameraViewBehavior::IsCameraView() const
{
	return true;
}

void CBulletCameraViewBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pRigidBody = GetAttachedRigidBody();

	if (!pRigidBody)
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}
}

bool CBulletCameraViewBehavior::ShouldSyncCameraView() const
{
	if (m_pPhysicObject->IsRagdollObject())
	{
		auto pRagdollObject = (IRagdollObject *)m_pPhysicObject;

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_Idle && !m_bActivateOnIdle)
			return false;

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_Death && !m_bActivateOnDeath)
			return false;

		if (pRagdollObject->GetActivityType() == StudioAnimActivityType_CaughtByBarnacle && !m_bActivateOnCaughtByBarnacle)
			return false;
	}

	return true;
}
