#include "BulletBarnacleChewForceBehavior.h"

CBulletBarnacleChewForceBehavior::CBulletBarnacleChewForceBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iBarnacleIndex, float flForceMagnitude, float flInterval) :

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

const char* CBulletBarnacleChewForceBehavior::GetTypeString() const
{
	return "BarnacleChewForce";
}

const char* CBulletBarnacleChewForceBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_BarnacleChewForce";
}

void CBulletBarnacleChewForceBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
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
		if (gEngfuncs.GetClientTime() > m_flNextChewTime)
		{
			vec3_t vecForce = { 0, 0, m_flForceMagnitude };

			pRigidBody->ApplyCentralImpulse(vecForce);

			m_flNextChewTime = gEngfuncs.GetClientTime() + m_flInterval;
		}
	}
}