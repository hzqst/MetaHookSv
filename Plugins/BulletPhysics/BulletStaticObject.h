#pragma once

#include "BaseStaticObject.h"
#include "BulletPhysicManager.h"

class CBulletStaticRigidBody : public CBulletRigidBody
{
public:
	CBulletStaticRigidBody(
		int id,
		int entindex,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group, int mask)
		:
		CBulletRigidBody(id, entindex, pRigidConfig, constructionInfo, group, mask)
	{

	}

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override
	{
		if (!m_pInternalRigidBody)
			return;

		auto pPhysicObject = ComponentUpdateContext->m_pObjectUpdateContext->m_pPhysicObject;

		if (!pPhysicObject)
			return;

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

			if ((ent != r_worldentity) && (ent->curstate.movetype == MOVETYPE_PUSH || ent->curstate.movetype == MOVETYPE_PUSHSTEP))
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
};

class CBulletStaticObject : public CBaseStaticObject
{
public:
	CBulletStaticObject(const CStaticObjectCreationParameter& CreationParam) : CBaseStaticObject(CreationParam)
	{
		CreateRigidBodies(CreationParam);
	}

	~CBulletStaticObject()
	{

	}

	IPhysicRigidBody* CreateRigidBody(const CStaticObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig) override
	{
		if (GetRigidBodyByName(pRigidConfig->name))
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create duplicated rigidbody \"%s\".\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		auto pMotionState = BulletCreateMotionState(CreationParam, pRigidConfig, this);

		if (!pMotionState)
		{
			gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create rigid body for StaticObject because there is no MotionState available.\n");
			return nullptr;
		}

		auto pCollisionShape = BulletCreateCollisionShape(pRigidConfig);

		if (!pCollisionShape)
		{
			delete pMotionState;

			gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create rigid body for StaticObject because there is no CollisionShape available.\n");
			return nullptr;
		}

		btRigidBody::btRigidBodyConstructionInfo cInfo(0, pMotionState, pCollisionShape);

		cInfo.m_friction = 1;
		cInfo.m_rollingFriction = 1;
		cInfo.m_restitution = 1;

		int group = btBroadphaseProxy::DefaultFilter;

		if (CreationParam.m_entindex == 0)
			group |= BulletPhysicCollisionFilterGroups::WorldFilter;
		else
			group |= BulletPhysicCollisionFilterGroups::StaticObjectFilter;

		int mask = btBroadphaseProxy::AllFilter & ~(BulletPhysicCollisionFilterGroups::WorldFilter | BulletPhysicCollisionFilterGroups::StaticObjectFilter);

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToDynamicObject)
			mask &= ~BulletPhysicCollisionFilterGroups::DynamicObjectFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToRagdollObject)
			mask &= ~BulletPhysicCollisionFilterGroups::RagdollObjectFilter;

		auto physicComponentId = ClientPhysicManager()->AllocatePhysicComponentId();

		return new CBulletStaticRigidBody(
			physicComponentId,
			CreationParam.m_entindex,
			pRigidConfig,
			cInfo,
			group,
			mask);
	}
};
