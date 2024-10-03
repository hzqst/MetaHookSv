#include "BulletBarnacleChewBehavior.h"

CBulletBarnacleChewBehavior::CBulletBarnacleChewBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iBarnacleIndex, float flForceMagnitude, float flInterval)
	:
	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iBarnacleIndex(iBarnacleIndex),
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
	auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

	if (!pBarnacleObject)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: Invalid barnacle object!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!pBarnacleObject->IsRagdollObject())
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: Barnacle must be RagdollObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleChewBehavior::Update: Barnacle must have PhysicObjectFlag_Barnacle!\n");
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

	auto pBarnacleRagdollObject = (IRagdollObject*)pBarnacleObject;

	if (pBarnacleRagdollObject->GetActivityType() == StudioAnimActivityType_BarnacleChewing)
	{
		if (gEngfuncs.GetClientTime() > m_flNextChewTime)
		{
			vec3_t vecForce = { 0, 0, m_flForceMagnitude };

			pRigidBody->ApplyCentralImpulse(vecForce);

			m_flNextChewTime = gEngfuncs.GetClientTime() + m_flInterval;
		}
	}
}