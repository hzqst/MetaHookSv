#include "BulletBarnacleChewForceAction.h"

CBulletBarnacleChewForceAction::CBulletBarnacleChewForceAction(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig,
	int attachedPhysicComponentId,
	int iBarnacleIndex, float flForceMagnitude, float flInterval) :

	CBulletPhysicComponentAction(
		id,
		entindex,
		pPhysicObject,
		pActionConfig,
		attachedPhysicComponentId),

	m_iBarnacleIndex(iBarnacleIndex),
	m_flForceMagnitude(flForceMagnitude),
	m_flInterval(flInterval)
{

}

const char* CBulletBarnacleChewForceAction::GetTypeString() const
{
	return "BarnacleChewForce";
}

const char* CBulletBarnacleChewForceAction::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_BarnacleChewForce";
}

void CBulletBarnacleChewForceAction::Update(CPhysicComponentUpdateContext* ComponentContext)
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

			pRigidBody->ApplyCentralForce(vecForce);

			m_flNextChewTime = gEngfuncs.GetClientTime() + m_flInterval;
		}
	}
}