#pragma once

#include "BulletBarnacleDragForceBehavior.h"

CBulletBarnacleDragForceBehavior::CBulletBarnacleDragForceBehavior(
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

const char* CBulletBarnacleDragForceBehavior::GetTypeString() const
{
	return "BarnacleDragForce";
}
const char* CBulletBarnacleDragForceBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_BarnacleDragForce";
}

void CBulletBarnacleDragForceBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

	if (!pBarnacleObject)
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pRigidBody = GetAttachedRigidBody();

	if (!pRigidBody)
	{
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (pBarnacleObject->GetClientEntityState()->sequence == 5)
	{
		vec3_t vecForce = { 0, 0, m_flForceMagnitude };

		pRigidBody->ApplyCentralForce(vecForce);
	}
	else
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
}