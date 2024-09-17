#pragma once

#include "BasePhysicConstraint.h"
#include "BulletPhysicManager.h"

class CBulletPhysicConstraint : public CBasePhysicConstraint
{
public:
	CBulletPhysicConstraint(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		CClientConstraintConfig* pConstraintConfig,
		btTypedConstraint* pInternalConstraint);

	~CBulletPhysicConstraint();

	const char* GetTypeString() const;
	const char* GetTypeLocalizationTokenString() const;

	bool AddToPhysicWorld(void* world) override;
	bool RemoveFromPhysicWorld(void* world) override;
	bool IsAddedToPhysicWorld(void* world) const override;

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override;

	bool ExtendLinearLimit(int axis, float value) override;
	float GetMaxTolerantLinearError() const override;

	void* GetInternalConstraint() override;

private:

	btRigidBody* CreateInternalRigidBody(bool attachToJointB);
	void FreeInternalRigidBody(btRigidBody* pRigidBody);

public:

	float m_maxTolerantLinearError{ BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR };
	bool m_disableCollision{};

	bool m_addedToPhysicWorld{};

	int m_rigidBodyAPhysicComponentId{};
	int m_rigidBodyBPhysicComponentId{};

	btTypedConstraint* m_pInternalConstraint{};

	//For rayTest only
	btRigidBody* m_pInternalRigidBodyA{};
	btRigidBody* m_pInternalRigidBodyB{};
};