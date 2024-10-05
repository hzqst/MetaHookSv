#include "BulletBarnacleChewBehavior.h"

CBulletBarnacleChewBehavior::CBulletBarnacleChewBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iSourceIndex, float flForceMagnitude, float flInterval)
	:
	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iSourceIndex(iSourceIndex),
	m_flForceMagnitude(flForceMagnitude),
	m_flInterval(flInterval)
{

}

const char* CBulletBarnacleChewBehavior::GetTypeString() const
{
	return "BarnacleChew";
}

const char* CBulletBarnacleChewBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_BarnacleChew";
}

void CBulletBarnacleChewBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pSourceObject = ClientPhysicManager()->GetPhysicObject(m_iSourceIndex);

	if (!pSourceObject)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: Invalid SourceObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!pSourceObject->IsRagdollObject())
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: SourceObject must be RagdollObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pSourceObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: SourceObject must have PhysicObjectFlag_Barnacle!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pRigidBody = GetAttachedRigidBody();

	if (!pRigidBody)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: The attached PhysicComponent must be a rigidbody!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pSourceRagdollObject = (IRagdollObject*)pSourceObject;

	if (pSourceRagdollObject->GetActivityType() == StudioAnimActivityType_BarnacleChewing)
	{
		if (gEngfuncs.GetClientTime() > m_flNextChewTime)
		{
			vec3_t vecForce = { 0, 0, m_flForceMagnitude };

			pRigidBody->ApplyCentralImpulse(vecForce);

			m_flNextChewTime = gEngfuncs.GetClientTime() + m_flInterval;
		}
	}
}