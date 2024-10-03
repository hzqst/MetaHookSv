#pragma once

#include "BulletBarnacleDragOnRigidBodyBehavior.h"

CBulletBarnacleDragOnRigidBodyBehavior::CBulletBarnacleDragOnRigidBodyBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iBarnacleIndex, float flForceMagnitude, float flExtraHeight) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iBarnacleIndex(iBarnacleIndex),
	m_flForceMagnitude(flForceMagnitude),
	m_flExtraHeight(flExtraHeight)
{

}

const char* CBulletBarnacleDragOnRigidBodyBehavior::GetTypeString() const
{
	return "BarnacleDragOnRigidBody";
}

const char* CBulletBarnacleDragOnRigidBodyBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_BarnacleDragOnRigidBody";
}

void CBulletBarnacleDragOnRigidBodyBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

	if (!pBarnacleObject)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: Invalid barnacle object!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!pBarnacleObject->IsRagdollObject())
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: Barnacle must be RagdollObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: Barnacle must have PhysicObjectFlag_Barnacle!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pBarnacleObjectR = (IRagdollObject*)pBarnacleObject;

	auto pRigidBody = GetAttachedRigidBody();

	if (!pRigidBody)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: The attached PhysicComponent must be a rigidbody!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (pBarnacleObjectR->GetActivityType() == StudioAnimActivityType_BarnaclePulling)
	{
		vec3_t vecPhysicObjectOrigin{ 0 };

		if (m_pPhysicObject->GetGoldSrcOriginAngles(vecPhysicObjectOrigin, nullptr))
		{
			vec3_t vecClientEntityOrigin{ 0 };

			VectorCopy(m_pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

			if (vecPhysicObjectOrigin[2] < vecClientEntityOrigin[2] + m_flExtraHeight)
			{
				btVector3 vecForce = { 0, 0, m_flForceMagnitude };

				if (vecPhysicObjectOrigin[2] > vecClientEntityOrigin[2])
				{
					vecForce[2] *= (vecClientEntityOrigin[2] + m_flExtraHeight - vecPhysicObjectOrigin[2]) / m_flExtraHeight;
				}

				pRigidBody->ApplyCentralForce(vecForce);
			}
		}
	}
	else if (pBarnacleObjectR->GetActivityType() == StudioAnimActivityType_BarnacleChewing)
	{
		vec3_t vecForce = { 0, 0, m_flForceMagnitude };

		pRigidBody->ApplyCentralForce(vecForce);
	}
}