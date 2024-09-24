#include "BulletDynamicObject.h"
#include "BulletDynamicRigidBody.h"
#include "BulletDynamicConstraint.h"
#include "privatehook.h"

CBulletDynamicObject::CBulletDynamicObject(const CPhysicObjectCreationParameter& CreationParam) : CBaseDynamicObject(CreationParam)
{
	if (CreationParam.m_model->type == mod_studio)
	{
		ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
	}

	DispatchBuildPhysicComponents(
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_ActionConfigs,
		std::bind(&CBaseDynamicObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::CreateAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseDynamicObject::AddAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

}

CBulletDynamicObject::~CBulletDynamicObject()
{

}

IPhysicRigidBody* CBulletDynamicObject::CreateRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId)
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

	mask &= ~(BulletPhysicCollisionFilterGroups::ConstraintFilter | BulletPhysicCollisionFilterGroups::ActionFilter);

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

IPhysicConstraint* CBulletDynamicObject::CreateConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId)
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

IPhysicAction* CBulletDynamicObject::CreateAction(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicActionConfig* pActionConfig, int physicComponentId)
{
	switch (pActionConfig->type)
	{
	case PhysicAction_SimpleBuoyancy:
	{
		//TODO
		return nullptr;
	}
	}

	return nullptr;
}
