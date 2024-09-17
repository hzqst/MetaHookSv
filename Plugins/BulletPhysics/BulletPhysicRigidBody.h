#pragma once

#include "BasePhysicRigidBody.h"
#include "BulletPhysicManager.h"

class CBulletPhysicRigidBody : public CBasePhysicRigidBody
{
public:
	CBulletPhysicRigidBody(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group, int mask);

	~CBulletPhysicRigidBody();

	bool AddToPhysicWorld(void* world) override;
	bool RemoveFromPhysicWorld(void* world) override;
	bool IsAddedToPhysicWorld(void* world) const override;
	void ApplyCentralForce(const vec3_t vecForce) override;
	void SetLinearVelocity(const vec3_t vecVelocity) override;
	void SetAngularVelocity(const vec3_t vecVelocity) override;
	bool ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate) override;
	bool SetupBones(studiohdr_t* studiohdr) override;
	bool SetupJiggleBones(studiohdr_t* studiohdr) override;
	void* GetInternalRigidBody() override;
	bool GetGoldSrcOriginAngles(float* origin, float* angles) override;
	bool GetGoldSrcOriginAnglesWithLocalOffset(const vec3_t localoffset_origin, const vec3_t localoffset_angles, float* origin, float* angles) override;
	float GetMass() const override;

public:
	float m_mass{};
	btVector3 m_inertia{};
	float m_density{ BULLET_DEFAULT_DENSENTY };
	int m_group{ btBroadphaseProxy::DefaultFilter };
	int m_mask{ btBroadphaseProxy::AllFilter };

	bool m_addedToPhysicWorld{};

	btRigidBody* m_pInternalRigidBody{};
};