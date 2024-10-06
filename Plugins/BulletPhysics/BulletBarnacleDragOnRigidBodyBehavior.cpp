#pragma once

#include "BulletBarnacleDragOnRigidBodyBehavior.h"

CBulletBarnacleDragOnRigidBodyBehavior::CBulletBarnacleDragOnRigidBodyBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iSourceIndex, float flForceMagnitude, float flExtraHeight) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iSourceIndex(iSourceIndex),
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
	auto pSourceObject = ClientPhysicManager()->GetPhysicObject(m_iSourceIndex);

	if (!pSourceObject)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: Invalid SourceObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!pSourceObject->IsRagdollObject())
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: SourceObject must be RagdollObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pSourceObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: SourceObject must have PhysicObjectFlag_Barnacle!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pRigidBody = GetAttachedRigidBody();

	if (!pRigidBody)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnRigidBodyBehavior::Update: The attached PhysicComponent must be a rigidbody!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pSourceRagdollObject = (IRagdollObject*)pSourceObject;

	if (pSourceRagdollObject->GetActivityType() == StudioAnimActivityType_BarnaclePulling)
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
	else if (pSourceRagdollObject->GetActivityType() == StudioAnimActivityType_BarnacleChewing)
	{
		vec3_t vecForce = { 0, 0, m_flForceMagnitude };

		pRigidBody->ApplyCentralForce(vecForce);
	}
}