#include "BulletDynamicRigidBody.h"
#include "privatehook.h"
#include "plugins.h"

CBulletDynamicRigidBody::CBulletDynamicRigidBody(
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

bool CBulletDynamicRigidBody::SetupJiggleBones(studiohdr_t* studiohdr)
{
	if (!m_pInternalRigidBody)
		return false;

	auto pMotionState = (CBulletBaseMotionState*)m_pInternalRigidBody->getMotionState();

	if (pMotionState->IsBoneBased())
	{
		if (!(m_boneindex >= 0 && m_boneindex < studiohdr->numbones))
		{
			Sys_Error("CBulletDynamicRigidBody::SetupJiggleBones invalid m_boneindex!");
			return false;
		}

		auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

		if (!m_pInternalRigidBody->isKinematicObject())
		{
			btTransform bonematrix = pBoneMotionState->m_bonematrix;

			TransformBulletToGoldSrc(bonematrix);

			float bonematrix_3x4[3][4];
			TransformToMatrix3x4(bonematrix, bonematrix_3x4);

			memcpy((*pbonetransform)[m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
			memcpy((*plighttransform)[m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
		}
		else
		{
			auto& bonematrix = pBoneMotionState->m_bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[m_boneindex], bonematrix);

			TransformGoldSrcToBullet(bonematrix);
		}

		return true;
	}

	return false;
}

void CBulletDynamicRigidBody::Update(CPhysicComponentUpdateContext* ComponentUpdateContext)
{
	if (!m_pInternalRigidBody)
		return;

	auto pPhysicObject = GetOwnerPhysicObject();

	if (!pPhysicObject->IsStaticObject())
		return;

	auto ent = pPhysicObject->GetClientEntity();

	bool bKinematic = true;

	bool bKinematicStateChanged = false;

	do
	{
		if (m_flags & PhysicRigidBodyFlag_AlwaysDynamic)
		{
			bKinematic = false;
			break;
		}

		if (m_flags & PhysicRigidBodyFlag_AlwaysKinematic)
		{
			bKinematic = true;
			break;
		}

	} while (0);

	if (bKinematic)
	{
		int iCollisionFlags = m_pInternalRigidBody->getCollisionFlags();

		if (!(iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT))
		{
			iCollisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;

			m_pInternalRigidBody->setCollisionFlags(iCollisionFlags);
			m_pInternalRigidBody->setActivationState(DISABLE_DEACTIVATION);
			m_pInternalRigidBody->setGravity(btVector3(0, 0, 0));

			bKinematicStateChanged = true;
		}
	}
	else
	{
		int iCollisionFlags = m_pInternalRigidBody->getCollisionFlags();

		if (iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT)
		{
			iCollisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;

			m_pInternalRigidBody->setCollisionFlags(iCollisionFlags);
			m_pInternalRigidBody->forceActivationState(ACTIVE_TAG);
			m_pInternalRigidBody->setMassProps(m_mass, m_inertia);

			bKinematicStateChanged = true;
		}
	}

	if (bKinematicStateChanged)
	{
		ComponentUpdateContext->m_pObjectUpdateContext->m_bRigidbodyKinematicChanged = true;
	}
}