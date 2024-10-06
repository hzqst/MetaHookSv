#pragma once

#include "BulletSimpleBuoyancyBehavior.h"

CBulletSimpleBuoyancyBehavior::CBulletSimpleBuoyancyBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	float flMagnitude,
	float flLinearDamping,
	float flAngularDamping) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_flMagnitude(flMagnitude),
	m_flLinearDamping(flLinearDamping),
	m_flAngularDamping(flAngularDamping)
{

}

const char* CBulletSimpleBuoyancyBehavior::GetTypeString() const
{
	return "SimpleBuoyancy";
}

const char* CBulletSimpleBuoyancyBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_SimpleBuoyancy";
}

void CBulletSimpleBuoyancyBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pRigidBody = GetAttachedRigidBody();

	if (!pRigidBody)
	{
		gEngfuncs.Con_DPrintf("CBulletSimpleBuoyancyBehavior::Update: The attached PhysicComponent must be a rigidbody!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	vec3_t vecGoldSrcOrigin{};

	if (pRigidBody->GetGoldSrcOriginAngles(vecGoldSrcOrigin, nullptr))
	{
		if (gEngfuncs.PM_PointContents(vecGoldSrcOrigin, NULL) == CONTENT_WATER)
		{
			vec3_t vecGoldSrcForce = { 0, 0, m_flMagnitude * pRigidBody->GetMass() * ComponentContext->m_pObjectUpdateContext->m_flGravity };

			vec3_t vecNewGoldSrcOrigin{}, vecNewGoldSrcAngles{};
			if (pRigidBody->GetGoldSrcOriginAnglesWithLocalOffset(m_origin, m_angles, vecNewGoldSrcOrigin, vecNewGoldSrcAngles))
			{
				pRigidBody->ApplyForceAtOrigin(vecGoldSrcForce, vecNewGoldSrcOrigin);
				pRigidBody->SetDamping(m_flLinearDamping, m_flAngularDamping);
				pRigidBody->KeepWakeUp();
			}
		}
		else
		{
			pRigidBody->SetDamping(0, 0);
		}
	}
}