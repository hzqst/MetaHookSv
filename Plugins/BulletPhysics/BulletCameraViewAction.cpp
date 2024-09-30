#include "BulletCameraViewAction.h"

CBulletCameraViewAction::CBulletCameraViewAction(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
	int attachedPhysicComponentId) :
	CBulletPhysicComponentAction(
		id,
		entindex,
		pPhysicObject,
		pActionConfig,
		attachedPhysicComponentId)
{

}

const char* CBulletCameraViewAction::GetTypeString() const
{
	return "CameraView";
}

const char* CBulletCameraViewAction::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_CameraView";
}

bool CBulletCameraViewAction::IsCameraView() const
{
	return true;
}

void CBulletCameraViewAction::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pRigidBody = GetAttachedRigidBody();

	if (!pRigidBody)
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}
}