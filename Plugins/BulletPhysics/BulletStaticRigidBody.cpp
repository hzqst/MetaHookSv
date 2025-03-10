#include "BulletStaticRigidBody.h"
#include "exportfuncs.h"

CBulletStaticRigidBody::CBulletStaticRigidBody(
	int id,
	int entindex,
	IPhysicObject* pPhysicObject,
	const CClientRigidBodyConfig* pRigidConfig,
	const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
	int group, int mask)
	:
	CBulletPhysicRigidBody(
		id,
		entindex,
		pPhysicObject,
		pRigidConfig,
		constructionInfo,
		group,
		mask)
{

}

void CBulletStaticRigidBody::Update(CPhysicComponentUpdateContext* ComponentUpdateContext)
{
	if (!m_pInternalRigidBody)
		return;

	auto pPhysicObject = GetOwnerPhysicObject();

	if (!pPhysicObject->IsStaticObject())
		return;

	auto ent = pPhysicObject->GetClientEntity();

	bool bKinematic = false;

	bool bKinematicStateChanged = false;

	do
	{
		if (m_flags & PhysicRigidBodyFlag_AlwaysKinematic)
		{
			bKinematic = true;
			break;
		}

		if (m_flags & PhysicRigidBodyFlag_AlwaysStatic)
		{
			bKinematic = false;
			break;
		}

		if ((ent != r_worldentity && ent != gEngfuncs.GetEntityByIndex(0)) && (ent->curstate.movetype == MOVETYPE_PUSH || ent->curstate.movetype == MOVETYPE_PUSHSTEP))
		{
			bKinematic = true;
			break;
		}
		else
		{
			bKinematic = false;
			break;
		}

	} while (0);

	if (bKinematic)
	{
		int iCollisionFlags = m_pInternalRigidBody->getCollisionFlags();

		if (!(iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT))
		{
			iCollisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;

			m_pInternalRigidBody->setActivationState(DISABLE_DEACTIVATION);

			m_pInternalRigidBody->setCollisionFlags(iCollisionFlags);

			bKinematicStateChanged = true;
		}
	}
	else
	{
		int iCollisionFlags = m_pInternalRigidBody->getCollisionFlags();

		if (iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT)
		{
			iCollisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;

			m_pInternalRigidBody->setActivationState(ACTIVE_TAG);

			m_pInternalRigidBody->setCollisionFlags(iCollisionFlags);

			bKinematicStateChanged = true;
		}
	}

	if (bKinematicStateChanged)
	{
		ComponentUpdateContext->m_pObjectUpdateContext->m_bRigidbodyKinematicChanged = true;
	}
}