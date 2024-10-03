#pragma once

#include "BulletRigidBodyRelocationBehavior.h"
#include "PhysicUTIL.h"

CBulletRigidBodyRelocationBehavior::CBulletRigidBodyRelocationBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iRigidBodyBComponentId) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iRigidBodyBComponentId(iRigidBodyBComponentId)
{

}

const char* CBulletRigidBodyRelocationBehavior::GetTypeString() const
{
	return "RigidBodyRelocation";
}

const char* CBulletRigidBodyRelocationBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_RigidBodyRelocation";
}

void CBulletRigidBodyRelocationBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	if (!ComponentContext->bIsAddingPhysicComponent)
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pRigidBodyA = GetAttachedRigidBody();

	if (!pRigidBodyA)
	{
		gEngfuncs.Con_DPrintf("CBulletRigidBodyRelocationBehavior::Update: The attached PhysicComponent must be a rigidbody!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pRigidBodyB = UTIL_GetPhysicComponentAsRigidBody(m_iRigidBodyBComponentId);

	if (!pRigidBodyB)
	{
		gEngfuncs.Con_DPrintf("CBulletRigidBodyRelocationBehavior::Update: Invalid RigidBodyB!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	vec3_t vecGoldSrcOrigin = {0}, vecGoldSrcAngles = { 0 };

	if (pRigidBodyB->GetGoldSrcOriginAnglesWithLocalOffset(m_origin, m_angles, vecGoldSrcOrigin, vecGoldSrcAngles))
	{
		pRigidBodyA->SetGoldSrcOriginAngles(vecGoldSrcOrigin, vecGoldSrcAngles);
	}

	ComponentContext->m_bShouldFree = true;
	return;
}