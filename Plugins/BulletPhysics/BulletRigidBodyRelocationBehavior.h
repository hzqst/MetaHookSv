#pragma once

#include "BulletPhysicComponentBehavior.h"

class CBulletRigidBodyRelocationBehavior : public CBulletPhysicComponentBehavior
{
public:
	CBulletRigidBodyRelocationBehavior(
		int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
		int attachedPhysicComponentId,
		int iRigidBodyBComponentId);

	const char* GetTypeString() const override;
	const char* GetTypeLocalizationTokenString() const override;
	void Update(CPhysicComponentUpdateContext* ComponentContext) override;

private:
	int m_iRigidBodyBComponentId{ 0 };
};