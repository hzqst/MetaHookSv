#include "BulletRagdollObject.h"
#include "BulletRagdollRigidBody.h"
#include "BulletRagdollConstraint.h"
#include "BulletBarnacleDragForceAction.h"
#include "BulletBarnacleChewForceAction.h"
#include "BulletBarnacleConstraintLimitAdjustmentAction.h"
#include "PhysicUTIL.h"
#include "privatehook.h"

CBulletRagdollObject::CBulletRagdollObject(const CPhysicObjectCreationParameter& CreationParam) : CBaseRagdollObject(CreationParam)
{
	auto pRagdollObjectConfig = (CClientRagdollObjectConfig*)CreationParam.m_pPhysicObjectConfig;

	m_AnimControlConfigs = pRagdollObjectConfig->AnimControlConfigs;

	for (const auto& pAnimConfig : m_AnimControlConfigs)
	{
		if (pAnimConfig->activity == StudioAnimActivityType_Idle)
		{
			m_IdleAnimConfig = pAnimConfig;
			break;
		}
	}

	for (const auto& pAnimConfig : m_AnimControlConfigs)
	{
		if (pAnimConfig->activity == StudioAnimActivityType_Debug)
		{
			m_DebugAnimConfig = pAnimConfig;
			break;
		}
	}

	if (CreationParam.m_model->type == mod_studio)
	{
		if (m_IdleAnimConfig)
		{
			ClientPhysicManager()->SetupBonesForRagdollEx(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex, m_IdleAnimConfig.get());
		}
		else
		{
			ClientPhysicManager()->SetupBonesForRagdoll(CreationParam.m_entity, CreationParam.m_entstate, CreationParam.m_model, CreationParam.m_entindex, CreationParam.m_playerindex);
		}
	}

	SaveBoneRelativeTransform(CreationParam);

	DispatchBuildPhysicComponents(
		CreationParam,
		m_RigidBodyConfigs,
		m_ConstraintConfigs,
		m_ActionConfigs,
		std::bind(&CBaseRagdollObject::CreateRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddRigidBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::CreateConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddConstraint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::CreateAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CBaseRagdollObject::AddAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	SetupNonKeyBones(CreationParam);

	InitCameraControl(&pRagdollObjectConfig->ThirdPersonViewCameraControlConfig, m_ThirdPersonViewCameraControl);
	InitCameraControl(&pRagdollObjectConfig->FirstPersonViewCameraControlConfig, m_FirstPersonViewCameraControl);
}

CBulletRagdollObject::~CBulletRagdollObject()
{

}

bool CBulletRagdollObject::SetupBones(studiohdr_t* studiohdr)
{
	if (CBaseRagdollObject::SetupBones(studiohdr))
	{
		mstudiobone_t* pbones = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

		for (size_t index = 0; index < m_nonKeyBones.size(); index++)
		{
			auto i = m_nonKeyBones[index];

			if (i == -1)
				continue;

			auto parentMatrix3x4 = (*pbonetransform)[pbones[i].parent];

			btTransform parentMatrix;

			Matrix3x4ToTransform(parentMatrix3x4, parentMatrix);

			TransformGoldSrcToBullet(parentMatrix);

			btTransform mergedMatrix = parentMatrix * m_BoneRelativeTransform[i];

			TransformBulletToGoldSrc(mergedMatrix);

			TransformToMatrix3x4(mergedMatrix, (*pbonetransform)[i]);
		}

		return true;
	}

	return false;
}

void CBulletRagdollObject::Update(CPhysicObjectUpdateContext* ObjectUpdateContext)
{
	CBaseRagdollObject::Update(ObjectUpdateContext);

	CheckConstraintLinearErrors(ObjectUpdateContext);
}

void CBulletRagdollObject::CheckConstraintLinearErrors(CPhysicObjectUpdateContext* ctx)
{
	if (ctx->m_bRigidbodyPoseChanged)
		return;

	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if(!pPhysicComponent->IsConstraint())
			continue;

		auto pConstraint = (IPhysicConstraint*)pPhysicComponent;

		if (pConstraint->GetFlags() & PhysicConstraintFlag_NonNative)
			continue;

		if (pConstraint->GetFlags() & PhysicConstraintFlag_DontResetPoseOnErrorCorrection)
			continue;

		auto pInternalConstraint = (btTypedConstraint*)pConstraint->GetInternalConstraint();

		if (!pInternalConstraint->isEnabled())
			continue;

		auto& pRigidBodyA = pInternalConstraint->getRigidBodyA();

		auto& pRigidBodyB = pInternalConstraint->getRigidBodyA();

		bool bShouldPerformCheck = false;

		if (pRigidBodyA.isKinematicObject() && !pRigidBodyB.isStaticOrKinematicObject())//A Kinematic, B Dynamic
		{
			bShouldPerformCheck = true;
		}
		else if (pRigidBodyB.isKinematicObject() && !pRigidBodyA.isStaticOrKinematicObject())//B Kinematic, A Dynamic
		{
			bShouldPerformCheck = true;
		}
		else if (!pRigidBodyA.isStaticOrKinematicObject() && !pRigidBodyA.isStaticOrKinematicObject())//A Dynamic, B Dynamic
		{
			bShouldPerformCheck = true;
		}

		if (bShouldPerformCheck)
		{
			auto errorMagnitude = BulletGetConstraintLinearErrorMagnitude(pInternalConstraint);

			float maxTol = pConstraint->GetMaxTolerantLinearError();

			FloatGoldSrcToBullet(&maxTol);

			if (errorMagnitude > maxTol)
			{
				ResetPose(GetClientEntityState());

				ctx->m_bRigidbodyPoseChanged = true;

				return;
			}
		}
	}
}

IPhysicRigidBody* CBulletRagdollObject::CreateRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId)
{
	if (GetRigidBodyByName(pRigidConfig->name))
	{
		gEngfuncs.Con_Printf("CreateRigidBody: cannot create duplicated rigidbody \"%s\".\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	if (pRigidConfig->mass < 0)
	{
		gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because mass < 0.\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	auto pMotionState = BulletCreateMotionState(CreationParam, pRigidConfig, this);

	if (!pMotionState)
	{
		gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because there is no MotionState available.\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	auto pCollisionShape = BulletCreateCollisionShape(pRigidConfig);

	if (!pCollisionShape)
	{
		delete pMotionState;

		gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because there is no CollisionShape available.\n", pRigidConfig->name.c_str());
		return nullptr;
	}

	btVector3 shapeInertia;
	pCollisionShape->calculateLocalInertia(pRigidConfig->mass, shapeInertia);

	btRigidBody::btRigidBodyConstructionInfo cInfo(pRigidConfig->mass, pMotionState, pCollisionShape, shapeInertia);
	cInfo.m_friction = pRigidConfig->linearFriction;
	cInfo.m_rollingFriction = pRigidConfig->rollingFriction;
	cInfo.m_restitution = pRigidConfig->restitution;
	cInfo.m_linearSleepingThreshold = pRigidConfig->linearSleepingThreshold;
	cInfo.m_angularSleepingThreshold = pRigidConfig->angularSleepingThreshold;
	cInfo.m_additionalDamping = true;
	cInfo.m_additionalDampingFactor = 0.5f;
	cInfo.m_additionalLinearDampingThresholdSqr = 1.0f * 1.0f;
	cInfo.m_additionalAngularDampingThresholdSqr = 0.3f * 0.3f;

	int group = btBroadphaseProxy::DefaultFilter | BulletPhysicCollisionFilterGroups::RagdollObjectFilter;

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

	return new CBulletRagdollRigidBody(
		physicComponentId ? physicComponentId : ClientPhysicManager()->AllocatePhysicComponentId(),
		CreationParam.m_entindex,
		this,
		pRigidConfig,
		cInfo,
		group,
		mask);
}

IPhysicConstraint* CBulletRagdollObject::CreateConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId)
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
		return new CBulletRagdollConstraint(
			physicComponentId ? physicComponentId : ClientPhysicManager()->AllocatePhysicComponentId(),
			CreationParam.m_entindex,
			this,
			pConstraintConfig,
			pInternalConstraint);
	}

	return nullptr;
}

IPhysicAction* CBulletRagdollObject::CreateAction(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicActionConfig* pActionConfig, int physicComponentId)
{
	switch (pActionConfig->type)
	{
	case PhysicAction_BarnacleDragForce:
	{
		auto pRigidBodyA = GetRigidBodyByName(pActionConfig->rigidbody);

		if (!pRigidBodyA)
		{
			gEngfuncs.Con_DPrintf("CreateAction: rigidbody \"%s\" not found!\n", pActionConfig->rigidbody.c_str());
			return nullptr;
		}

		return new CBulletBarnacleDragForceAction(
			physicComponentId,
			GetEntityIndex(),
			this,
			pActionConfig,
			pRigidBodyA->GetPhysicComponentId(),
			m_iBarnacleIndex,
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceMagnitude],
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceExtraHeight]);
	}
	case PhysicAction_BarnacleChewForce:
	{
		auto pRigidBodyA = GetRigidBodyByName(pActionConfig->rigidbody);

		if (!pRigidBodyA)
		{
			gEngfuncs.Con_DPrintf("CreateAction: rigidbody \"%s\" not found!\n", pActionConfig->rigidbody.c_str());
			return nullptr;
		}

		return new CBulletBarnacleChewForceAction(
			physicComponentId,
			GetEntityIndex(),
			this,
			pActionConfig,
			pRigidBodyA->GetPhysicComponentId(),
			m_iBarnacleIndex,
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleChewForceMagnitude],
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleChewForceInterval]);
	}
	case PhysicAction_BarnacleConstraintLimitAdjustment:
	{
		auto pConstraint = GetConstraintByName(pActionConfig->constraint);

		if (!pConstraint)
		{
			gEngfuncs.Con_DPrintf("CreateAction: constraint \"%s\" not found!\n", pActionConfig->constraint.c_str());
			return nullptr;
		}

		return new CBulletBarnacleConstraintLimitAdjustmentAction(
			physicComponentId,
			GetEntityIndex(),
			this,
			pActionConfig,
			pConstraint->GetPhysicComponentId(),
			m_iBarnacleIndex,
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleConstraintLimitAdjustmentExtraHeight],
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleConstraintLimitAdjustmentInterval],
			(int)pActionConfig->factors[PhysicActionFactorIdx_BarnacleConstraintLimitAdjustmentAxis]);
	}
	case PhysicAction_SimpleBuoyancy:
	{
		//TODO
		return nullptr;
	}
	}

	return nullptr;
}

void CBulletRagdollObject::SaveBoneRelativeTransform(const CPhysicObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_studiohdr)
		return;

	const auto pbones = (mstudiobone_t*)((byte*)CreationParam.m_studiohdr + CreationParam.m_studiohdr->boneindex);

	//Save bone relative transform

	for (int i = 0; i < CreationParam.m_studiohdr->numbones; ++i)
	{
		int parent = pbones[i].parent;

		if (parent == -1)
		{
			Matrix3x4ToTransform((*pbonetransform)[i], m_BoneRelativeTransform[i]);

			TransformGoldSrcToBullet(m_BoneRelativeTransform[i]);
		}
		else
		{
			btTransform bonematrix;
			Matrix3x4ToTransform((*pbonetransform)[i], bonematrix);

			btTransform parentmatrix;
			Matrix3x4ToTransform((*pbonetransform)[pbones[i].parent], parentmatrix);

			m_BoneRelativeTransform[i] = parentmatrix.inverse() * bonematrix;

			TransformGoldSrcToBullet(m_BoneRelativeTransform[i]);
		}
	}
}