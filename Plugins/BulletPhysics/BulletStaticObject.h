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
				ctx->bRigidbodyKinematicChanged = true;
		}
	}

	void AddToPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto RigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(RigidBody);

			dynamicWorld->addRigidBody(RigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);
		}
	}

	void RemoveFromPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto RigidBody : m_RigidBodies)
		{
			dynamicWorld->removeRigidBody(RigidBody);
		}
	}

private:

	btCollisionShape* CreateCollisionShapeInternal(const CStaticObjectCreationParameter& CreationParam, const CClientCollisionShapeConfig* pConfig)
	{
		btCollisionShape* pShape{};

		switch (pConfig->type)
		{
		case PhysicShape_Box:
		{
			btVector3 size(pConfig->size[0], pConfig->size[1], pConfig->size[2]);

			pShape = new btBoxShape(size);

			break;
		}
		case PhysicShape_Sphere:
		{
			auto size = btScalar(pConfig->size[0]);

			pShape = new btSphereShape(size);

			break;
		}
		case PhysicShape_Capsule:
		{
			if (pConfig->direction == PhysicShapeDirection_X)
			{
				pShape = new btCapsuleShapeX(btScalar(pConfig->size[0]), btScalar(pConfig->size[1]));
			}
			else if (pConfig->direction == PhysicShapeDirection_Y)
			{
				pShape = new btCapsuleShape(btScalar(pConfig->size[0]), btScalar(pConfig->size[1]));
			}
			else if (pConfig->direction == PhysicShapeDirection_Z)
			{
				pShape = new btCapsuleShapeZ(btScalar(pConfig->size[0]), btScalar(pConfig->size[1]));
			}

			break;
		}
		case PhysicShape_Cylinder:
		{
			if (pConfig->direction == PhysicShapeDirection_X)
			{
				pShape = new btCylinderShapeX(btVector3(pConfig->size[0], pConfig->size[1], pConfig->size[2]));
			}
			else if (pConfig->direction == PhysicShapeDirection_Y)
			{
				pShape = new btCylinderShape(btVector3(pConfig->size[0], pConfig->size[1], pConfig->size[2]));
			}
			else if (pConfig->direction == PhysicShapeDirection_Z)
			{
				pShape = new btCylinderShapeZ(btVector3(pConfig->size[0], pConfig->size[1], pConfig->size[2]));
			}

			break;
		}
		case PhysicShape_TriangleMesh:
		{
			if (!pConfig->m_pVertexArray)
			{
				gEngfuncs.Con_DPrintf("CreateCollisionShapeInternal: m_pVertexArray cannot be null!\n");
				break;
			}

			if (!pConfig->m_pIndexArray)
			{
				gEngfuncs.Con_DPrintf("CreateCollisionShapeInternal: m_pIndexArray cannot be null!\n");
				break;
			}

			if (!pConfig->m_pIndexArray->vIndexBuffer.size())
			{
				gEngfuncs.Con_DPrintf("CreateCollisionShapeInternal: vIndexBuffer cannot be empty!\n");
				break;
			}

			if (!pConfig->m_pVertexArray->vVertexBuffer.size())
			{
				gEngfuncs.Con_DPrintf("CreateCollisionShapeInternal: vVertexBuffer cannot be empty!\n");
				break;
			}

			auto pIndexVertexArray = new btTriangleIndexVertexArray(
				pConfig->m_pIndexArray->vIndexBuffer.size() / 3, pConfig->m_pIndexArray->vIndexBuffer.data(), 3 * sizeof(int),
				pConfig->m_pVertexArray->vVertexBuffer.size(), (float*)pConfig->m_pVertexArray->vVertexBuffer.data(), sizeof(CPhysicBrushVertex));

			auto pTriMesh = new btBvhTriangleMeshShape(pIndexVertexArray, true, true);

			pTriMesh->setUserPointer(new CBulletCollisionShapeSharedUserData(pIndexVertexArray));

			pShape = pTriMesh;
		}
		}

		return pShape;
	}

	btCollisionShape* CreateCollisionShape(const CStaticObjectCreationParameter& CreationParam, const CClientRigidBodyConfig* pConfig)
	{
		if (pConfig->shapes.size() > 1)
		{
			auto pCompoundShape = new btCompoundShape();

			for (auto pShapeConfig : pConfig->shapes)
			{
				auto shape = CreateCollisionShapeInternal(CreationParam, pShapeConfig);

				if (shape)
				{
					btTransform trans;

					trans.setIdentity();

					EulerMatrix(btVector3(pShapeConfig->angles[0], pShapeConfig->angles[1], pShapeConfig->angles[2]), trans.getBasis());

					trans.setOrigin(btVector3(pShapeConfig->origin[0], pShapeConfig->origin[1], pShapeConfig->origin[2]));

					pCompoundShape->addChildShape(trans, shape);
				}
			}

			if (!pCompoundShape->getNumChildShapes())
			{
				OnBeforeDeleteBulletCollisionShape(pCompoundShape);

				delete pCompoundShape;

				return nullptr;
			}

			return pCompoundShape;
		}
		else if (pConfig->shapes.size() == 1)
		{
			auto pShapeConfig = pConfig->shapes[0];

			return CreateCollisionShapeInternal(CreationParam, pShapeConfig);
		}

		return nullptr;
	}

	btMotionState* CreateMotionState(const CStaticObjectCreationParameter& CreationParam, const CClientRigidBodyConfig* pRigidConfig)
	{
		if (CreationParam.m_studiohdr && pRigidConfig->boneindex >= 0 && pRigidConfig->boneindex < CreationParam.m_studiohdr->numbones)
		{
			btTransform bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[pRigidConfig->boneindex], bonematrix);

			btVector3 vecOrigin(pRigidConfig->origin[0], pRigidConfig->origin[1], pRigidConfig->origin[2]);

			btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

			btVector3 vecAngles(pRigidConfig->angles[0], pRigidConfig->angles[1], pRigidConfig->angles[2]);

			EulerMatrix(vecAngles, localTrans.getBasis());

			const auto& offsetmatrix = localTrans;

			return new CBulletBoneMotionState(this, bonematrix, offsetmatrix);
		}

		return new CBulletEntityMotionState(this);
	}

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

		auto pMotionState = CreateMotionState(CreationParam, pRigidConfig);

		if (!pMotionState)
		{
			gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create rigid body for StaticObject because there is no MotionState available.\n");
			return nullptr;
		}

		auto pCollisionShape = CreateCollisionShape(CreationParam, pRigidConfig);

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
