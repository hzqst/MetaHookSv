#pragma once

#include "BaseStaticObject.h"
#include "BulletPhysicManager.h"

class CBulletStaticObject : public CBaseStaticObject
{
public:
	CBulletStaticObject(const CStaticObjectCreationParameter& CreationParam) : CBaseStaticObject(CreationParam)
	{
		CreateRigidBodies(CreationParam);
	}

	~CBulletStaticObject()
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			OnBeforeDeleteBulletRigidBody(pRigidBody);

			delete pRigidBody;
		}

		m_RigidBodies.clear();
	}

	void Update(CPhysicObjectUpdateContext* ctx) override
	{
		CBaseStaticObject::Update(ctx);

		for(auto pRigidBody : m_RigidBodies)
		{
			if (UpdateRigidBodyKinematic(pRigidBody))
				ctx->m_bRigidbodyKinematicChanged = true;
		}
	}

	void AddToPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto pRigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (!pSharedUserData->m_addedToPhysicWorld)
			{
				dynamicWorld->addRigidBody(pRigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);

				pSharedUserData->m_addedToPhysicWorld = true;
			}
		}
	}

	void RemoveFromPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto pRigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (pSharedUserData->m_addedToPhysicWorld)
			{
				dynamicWorld->removeRigidBody(pRigidBody);

				pSharedUserData->m_addedToPhysicWorld = false;
			}
		}
	}

private:

	btRigidBody* FindRigidBodyByName(const std::string& name)
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (pSharedUserData)
			{
				if (pSharedUserData->m_name == name)
					return pRigidBody;
			}
		}

		return nullptr;
	}

	btRigidBody* CreateRigidBody(const CStaticObjectCreationParameter& CreationParam, const CClientRigidBodyConfig* pRigidConfig)
	{
		if (FindRigidBodyByName(pRigidConfig->name))
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create duplicated one \"%s\".\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		auto pMotionState = BulletCreateMotionState(CreationParam, pRigidConfig, this);

		if (!pMotionState)
		{
			gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create rigid body for StaticObject because there is no MotionState available.\n");
			return nullptr;
		}

		auto pCollisionShape = BulletCreateCollisionShape(CreationParam, pRigidConfig);

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

		auto pRigidBody = new btRigidBody(cInfo);

		pRigidBody->setUserPointer(new CBulletRigidBodySharedUserData(
			cInfo,
			btBroadphaseProxy::DefaultFilter | BulletPhysicCollisionFilterGroups::StaticObjectFilter,
			btBroadphaseProxy::AllFilter & ~BulletPhysicCollisionFilterGroups::StaticObjectFilter,
			pRigidConfig->name,
			pRigidConfig->flags & ~PhysicRigidBodyFlag_AlwaysDynamic,
			pRigidConfig->boneindex,
			pRigidConfig->debugDrawLevel,
			1));

		UpdateRigidBodyKinematic(pRigidBody);

		return pRigidBody;
	}

	void CreateRigidBodies(const CStaticObjectCreationParameter& CreationParam)
	{
		for (auto pRigidBodyConfig : CreationParam.m_pStaticObjectConfig->RigidBodyConfigs)
		{
			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig);

			if (pRigidBody)
			{
				m_RigidBodies.emplace_back(pRigidBody);
			}
		}
	}

	bool UpdateRigidBodyKinematic(btRigidBody* pRigidBody)
	{
		auto ent = GetClientEntity();

		auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

		bool bKinematic = false;

		bool bKinematicStateChanged = false;

		do
		{
			if (pSharedUserData->m_flags & PhysicRigidBodyFlag_AlwaysKinematic)
			{
				bKinematic = true;
				break;
			}

			if (pSharedUserData->m_flags & PhysicRigidBodyFlag_AlwaysStatic)
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
			int iCollisionFlags = pRigidBody->getCollisionFlags();

			if (!(iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT))
			{
				iCollisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
				pRigidBody->setActivationState(DISABLE_DEACTIVATION);

				pRigidBody->setCollisionFlags(iCollisionFlags);

				bKinematicStateChanged = true;
			}
		}
		else
		{
			int iCollisionFlags = pRigidBody->getCollisionFlags();

			if (iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT)
			{
				iCollisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
				pRigidBody->setActivationState(ACTIVE_TAG);

				pRigidBody->setCollisionFlags(iCollisionFlags);

				bKinematicStateChanged = true;
			}
		}

		return bKinematicStateChanged;
	}

public:
	std::vector<btRigidBody*> m_RigidBodies{};
};
