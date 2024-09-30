#pragma once

#include "BasePhysicComponentBehavior.h"
#include "BulletPhysicManager.h"

class CBulletPhysicComponentBehavior : public CBasePhysicComponentBehavior
{
public:
	CBulletPhysicComponentBehavior(int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, int attachedPhysicComponentId);
	~CBulletPhysicComponentBehavior();

	bool AddToPhysicWorld(void* world) override;
	bool RemoveFromPhysicWorld(void* world) override;
	bool IsAddedToPhysicWorld(void* world) const override;

protected:
	btRigidBody* CreateInternalRigidBody(IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, int attachedPhysicComponentId);
	void FreeInternalRigidBody(btRigidBody* pRigidBody);

public:
	bool m_addedToPhysicWorld{};

	btTransform m_offsetmatrix{};

	//For rayTest only
	btRigidBody* m_pInternalRigidBody{};
};