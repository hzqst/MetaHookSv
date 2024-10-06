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
	void ApplyCentralImpulse(const vec3_t vecImpulse) override;
	void ApplyForceAtOrigin(const vec3_t vecForce, const vec3_t vecGoldSrcOrigin) override;
	void ApplyImpulseAtOrigin(const vec3_t vecImpulse, const vec3_t vecGoldSrcOrigin) override;
	void SetLinearVelocity(const vec3_t vecVelocity) override;
	void SetAngularVelocity(const vec3_t vecVelocity) override;
	void SetDamping(float flLinearDamping, float flAngularDamping) override;
	void KeepWakeUp() override;
	bool ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate) override;
	bool SetupBones(CRagdollObjectSetupBoneContext* Context) override;
	bool SetupJiggleBones(CRagdollObjectSetupBoneContext* Context) override;
	void* GetInternalRigidBody() override;
	bool GetGoldSrcOriginAngles(float* origin, float* angles) override;
	bool GetGoldSrcOriginAnglesWithLocalOffset(const vec3_t localoffset_origin, const vec3_t localoffset_angles, float* origin, float* angles) override;
	bool SetGoldSrcOriginAngles(const float* origin, const float* angles) override;
	float GetMass() const override;
	bool GetAABB(vec3_t mins, vec3_t maxs) const override;

public:
	float m_mass{};
	btVector3 m_inertia{};
	float m_density{ BULLET_DEFAULT_DENSENTY };
	int m_group{ btBroadphaseProxy::DefaultFilter };
	int m_mask{ btBroadphaseProxy::AllFilter };

	bool m_addedToPhysicWorld{};

	btRigidBody* m_pInternalRigidBody{};
};