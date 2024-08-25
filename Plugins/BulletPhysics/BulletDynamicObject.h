#pragma once

#include "BaseDynamicObject.h"
#include "BulletPhysicManager.h"

class CBulletDynamicRigidBody : public CBulletRigidBody
{
public:
	CBulletDynamicRigidBody(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group, int mask)
		:
		CBulletRigidBody(
			id,
			entindex,
			pPhysicObject,
			pRigidConfig,
			constructionInfo,
			group,
			mask)
	{

	}

	bool SetupJiggleBones(studiohdr_t* studiohdr) override
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

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override
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
};

class CBulletDynamicConstraint : public CBulletConstraint
{
public:
	CBulletDynamicConstraint(
		int id,
		int entindex,
		IPhysicObject* pPhysicObject,
		CClientConstraintConfig* pConstraintConfig,
		btTypedConstraint* pInternalConstraint)
		:
		CBulletConstraint(
			id,
			entindex,
			pPhysicObject,
			pConstraintConfig,
			pInternalConstraint)
	{

	}

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override
	{
		//do nothing
	}
};

class CBulletDynamicObject : public CBaseDynamicObject
{
public:
	CBulletDynamicObject(const CDynamicObjectCreationParameter& CreationParam) : CBaseDynamicObject(CreationParam)
	{
		CreateRigidBodies(CreationParam);
		CreateConstraints(CreationParam);
	}

	~CBulletDynamicObject()
	{

	}

	IPhysicRigidBody* CreateRigidBody(const CDynamicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) override
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

			gEngfuncs.Con_DPrintf("CreateRigidBody: cannot create rigid body for DynamicObject because there is no CollisionShape available.\n");
			return nullptr;
		}

		btRigidBody::btRigidBodyConstructionInfo cInfo(pRigidConfig->mass, pMotionState, pCollisionShape);

		cInfo.m_friction = 1;
		cInfo.m_rollingFriction = 1;
		cInfo.m_restitution = 1;

		int group = btBroadphaseProxy::DefaultFilter;

		if (CreationParam.m_entindex == 0)
			group |= BulletPhysicCollisionFilterGroups::WorldFilter;
		else
			group |= BulletPhysicCollisionFilterGroups::DynamicObjectFilter;

		int mask = btBroadphaseProxy::AllFilter;

		mask &= ~(BulletPhysicCollisionFilterGroups::ConstraintFilter | BulletPhysicCollisionFilterGroups::FloaterFilter);

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToWorld)
			mask &= ~BulletPhysicCollisionFilterGroups::WorldFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToStaticObject)
			mask &= ~BulletPhysicCollisionFilterGroups::StaticObjectFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToDynamicObject)
			mask &= ~BulletPhysicCollisionFilterGroups::DynamicObjectFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToRagdollObject)
			mask &= ~BulletPhysicCollisionFilterGroups::RagdollObjectFilter;

		return new CBulletDynamicRigidBody(
			physicComponentId ? physicComponentId : ClientPhysicManager()->AllocatePhysicComponentId(),
			CreationParam.m_entindex,
			this,
			pRigidConfig,
			cInfo,
			group,
			mask);
	}

	IPhysicConstraint* CreateConstraint(const CDynamicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) override
	{
		btTypedConstraint* pInternalConstraint{};

		do
		{
			auto pRigidBodyA = FindRigidBodyByName(pConstraintConfig->rigidbodyA, CreationParam.m_allowNonNativeRigidBody);

			if (!pRigidBodyA)
			{
				gEngfuncs.Con_Printf("CreateConstraint: rigidbodyA \"%s\" not found!\n", pConstraintConfig->rigidbodyA.c_str());
				return nullptr;
			}

			auto pRigidBodyB = FindRigidBodyByName(pConstraintConfig->rigidbodyB, CreationParam.m_allowNonNativeRigidBody);

			if (!pRigidBodyB)
			{
				gEngfuncs.Con_Printf("CreateConstraint: rigidbodyB \"%s\" not found!\n", pConstraintConfig->rigidbodyB.c_str());
				return nullptr;
			}

			if (pRigidBodyA == pRigidBodyB)
			{
				gEngfuncs.Con_Printf("CreateConstraint: rigidbodyA and rigidbodyA must be different!\n");
				return nullptr;
			}

			CBulletConstraintCreationContext ctx;

			ctx.pRigidBodyA = (btRigidBody*)pRigidBodyA->GetInternalRigidBody();
			ctx.pRigidBodyB = (btRigidBody*)pRigidBodyB->GetInternalRigidBody();

			if (pConstraintConfig->isLegacyConfig)
			{
				if (!(pConstraintConfig->boneindexA >= 0 && pConstraintConfig->boneindexA < CreationParam.m_studiohdr->numbones))
				{
					gEngfuncs.Con_Printf("CreateConstraint: invalid boneindexA (%d)!\n", pConstraintConfig->boneindexA);
					return nullptr;
				}
				if (!(pConstraintConfig->boneindexB >= 0 && pConstraintConfig->boneindexB < CreationParam.m_studiohdr->numbones))
				{
					gEngfuncs.Con_Printf("CreateConstraint: invalid boneindexB (%d)!\n", pConstraintConfig->boneindexB);
					return nullptr;
				}

				btTransform bonematrixA;
				Matrix3x4ToTransform((*pbonetransform)[pConstraintConfig->boneindexA], bonematrixA);
				TransformGoldSrcToBullet(bonematrixA);

				btTransform bonematrixB;
				Matrix3x4ToTransform((*pbonetransform)[pConstraintConfig->boneindexB], bonematrixB);
				TransformGoldSrcToBullet(bonematrixB);

				ctx.worldTransA = ctx.pRigidBodyA->getWorldTransform();
				ctx.worldTransB = ctx.pRigidBodyB->getWorldTransform();

				ctx.invWorldTransA = ctx.worldTransA.inverse();
				ctx.invWorldTransB = ctx.worldTransB.inverse();

				btVector3 offsetA(pConstraintConfig->offsetA[0], pConstraintConfig->offsetA[1], pConstraintConfig->offsetA[2]);
				Vector3GoldSrcToBullet(offsetA);

				//This converts bone A's world transform into rigidbody A's local space
				ctx.localTransA.mult(ctx.invWorldTransA, bonematrixA);
				//Uses bone A's direction, but uses offsetA as local origin
				ctx.localTransA.setOrigin(offsetA);

				btVector3 offsetB(pConstraintConfig->offsetB[0], pConstraintConfig->offsetB[1], pConstraintConfig->offsetB[2]);
				Vector3GoldSrcToBullet(offsetB);

				//This converts bone B's world transform into rigidbody B's local space
				ctx.localTransB.mult(ctx.invWorldTransB, bonematrixB);
				//Uses bone B's direction, but uses offsetB as local origin
				ctx.localTransB.setOrigin(offsetB);

				ctx.globalJointA.mult(ctx.worldTransA, ctx.localTransA);
				ctx.globalJointB.mult(ctx.worldTransB, ctx.localTransB);

				if (offsetA.fuzzyZero())
				{
					//Use B as final global joint transform
					pInternalConstraint = BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointB);
					break;
				}
				else
				{
					//Use A as final global joint transform
					pInternalConstraint = BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointA);
					break;
				}

				return nullptr;
			}

			ctx.worldTransA = ctx.pRigidBodyA->getWorldTransform();
			ctx.worldTransB = ctx.pRigidBodyB->getWorldTransform();

			ctx.invWorldTransA = ctx.worldTransA.inverse();
			ctx.invWorldTransB = ctx.worldTransB.inverse();

			btVector3 vecOriginA(pConstraintConfig->originA[0], pConstraintConfig->originA[1], pConstraintConfig->originA[2]);
			btVector3 vecOriginB(pConstraintConfig->originB[0], pConstraintConfig->originB[1], pConstraintConfig->originB[2]);

			Vector3GoldSrcToBullet(vecOriginA);
			Vector3GoldSrcToBullet(vecOriginB);

			ctx.localTransA = btTransform(btQuaternion(0, 0, 0, 1), vecOriginA);
			ctx.localTransB = btTransform(btQuaternion(0, 0, 0, 1), vecOriginB);

			btVector3 vecAnglesA(pConstraintConfig->anglesA[0], pConstraintConfig->anglesA[1], pConstraintConfig->anglesA[2]);
			btVector3 vecAnglesB(pConstraintConfig->anglesB[0], pConstraintConfig->anglesB[1], pConstraintConfig->anglesB[2]);

			EulerMatrix(vecAnglesA, ctx.localTransA.getBasis());
			EulerMatrix(vecAnglesB, ctx.localTransB.getBasis());

			ctx.globalJointA.mult(ctx.worldTransA, ctx.localTransA);
			ctx.globalJointB.mult(ctx.worldTransB, ctx.localTransB);

			if (pConstraintConfig->useGlobalJointFromA)
			{
				if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit)
				{
					ctx.rigidBodyDistance = (ctx.globalJointA.getOrigin() - ctx.globalJointB.getOrigin()).length();

					if (!isnan(pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset]))
					{
						btScalar offset = pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset];

						FloatGoldSrcToBullet(&offset);

						ctx.rigidBodyDistance += offset;
					}
				}

				//A look at B
				if (pConstraintConfig->useLookAtOther)
				{
					btVector3 vecForward(pConstraintConfig->forward[0], pConstraintConfig->forward[1], pConstraintConfig->forward[2]);

					btTransform newGlobalJoint = MatrixLookAt(ctx.globalJointA, ctx.globalJointB.getOrigin(), vecForward);

					if (pConstraintConfig->useGlobalJointOriginFromOther)
					{
						newGlobalJoint.setOrigin(ctx.globalJointB.getOrigin());
					}

					pInternalConstraint = BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, newGlobalJoint);
					break;
				}

				pInternalConstraint = BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointA);
				break;
			}
			else
			{
				if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit)
				{
					ctx.rigidBodyDistance = (ctx.globalJointA.getOrigin() - ctx.globalJointB.getOrigin()).length();

					if (!isnan(pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset]))
					{
						btScalar offset = pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset];

						FloatGoldSrcToBullet(&offset);

						ctx.rigidBodyDistance += offset;
					}
				}

				//B look at A
				if (pConstraintConfig->useLookAtOther)
				{
					btVector3 vecForward(pConstraintConfig->forward[0], pConstraintConfig->forward[1], pConstraintConfig->forward[2]);

					btTransform newGlobalJoint = MatrixLookAt(ctx.globalJointB, ctx.globalJointA.getOrigin(), vecForward);

					if (pConstraintConfig->useGlobalJointOriginFromOther)
					{
						newGlobalJoint.setOrigin(ctx.globalJointA.getOrigin());
					}

					pInternalConstraint = BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, newGlobalJoint);
					break;
				}

				pInternalConstraint = BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointB);
				break;
			}

		} while (0);

		if (pInternalConstraint)
		{
			return new CBulletDynamicConstraint(
				physicComponentId ? physicComponentId : ClientPhysicManager()->AllocatePhysicComponentId(),
				CreationParam.m_entindex,
				this,
				pConstraintConfig,
				pInternalConstraint);
		}

		return nullptr;
	}

protected:


public:
};
