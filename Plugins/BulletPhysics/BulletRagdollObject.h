#pragma once

#include "BaseRagdollObject.h"
#include "BulletPhysicManager.h"

class CBulletRagdollObject : public CBaseRagdollObject
{
public:
	CBulletRagdollObject(const CRagdollObjectCreationParameter& CreationParam) : CBaseRagdollObject(CreationParam)
	{
		m_entindex = CreationParam.m_entindex;
		m_playerindex = CreationParam.m_playerindex;
		m_entity = CreationParam.m_entity;
		m_model = CreationParam.m_model;

		m_IdleAnimConfig = CreationParam.m_pRagdollObjectConfig->IdleAnimConfig;
		m_AnimControlConfigs = CreationParam.m_pRagdollObjectConfig->AnimControlConfigs;

		SaveBoneRelativeTransform(CreationParam);
		CreateRigidBodies(CreationParam);
		CreateConstraints(CreationParam);
		CreateFloaters(CreationParam);
		SetupNonKeyBones(CreationParam);
	}

	~CBulletRagdollObject()
	{
		for (auto pConstraint : m_Constraints)
		{
			OnBeforeDeleteBulletConstraint(pConstraint);

			delete pConstraint;
		}

		m_Constraints.clear();

		for (auto pRigidBody : m_RigidBodies)
		{
			OnBeforeDeleteBulletRigidBody(pRigidBody);

			delete pRigidBody;
		}

		m_RigidBodies.clear();
	}

	bool GetOrigin(float* v) override
	{
		if (m_PelvisRigBody)
		{
			const auto& worldTransform = m_PelvisRigBody->getWorldTransform();

			const auto& worldOrigin = worldTransform.getOrigin();

			v[0] = worldOrigin.x();
			v[1] = worldOrigin.y();
			v[2] = worldOrigin.z();

			return true;
		}

		return false;
	}

	bool SetupBones(studiohdr_t* studiohdr) override
	{
		if (GetActivityType() == 0)
			return false;

		mstudiobone_t* pbones = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

		for (auto pRigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			auto pMotionState = (CBulletBaseMotionState*)pRigidBody->getMotionState();

			if (pMotionState->IsBoneBased())
			{
				auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

				const auto& bonematrix = pBoneMotionState->m_bonematrix;

				float bonematrix_3x4[3][4];
				TransformToMatrix3x4(bonematrix, bonematrix_3x4);

				memcpy((*pbonetransform)[pSharedUserData->m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
				memcpy((*plighttransform)[pSharedUserData->m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
			}
		}

		for (size_t index = 0; index < m_nonKeyBones.size(); index++)
		{
			auto i = m_nonKeyBones[index];

			if (i == -1)
				continue;

			auto parentMatrix3x4 = (*pbonetransform)[pbones[i].parent];

			btTransform parentMatrix;

			Matrix3x4ToTransform(parentMatrix3x4, parentMatrix);

			btTransform mergedMatrix;

			mergedMatrix = parentMatrix * m_BoneRelativeTransform[i];

			TransformToMatrix3x4(mergedMatrix, (*pbonetransform)[i]);
		}

		return true;
	}

	bool SetupJiggleBones(studiohdr_t* studiohdr) override
	{
		mstudiobone_t* pbones = (mstudiobone_t*)((byte*)(*pstudiohdr) + (*pstudiohdr)->boneindex);

		for (auto pRigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);
			auto pMotionState = (CBulletBaseMotionState*)pRigidBody->getMotionState();

			if (pMotionState->IsBoneBased())
			{
				auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

				if (!pRigidBody->isKinematicObject())//(pSharedUserData->m_flags & PhysicRigidBodyFlag_AlwaysDynamic)
				{
					const auto& bonematrix = pBoneMotionState->m_bonematrix;

					float bonematrix_3x4[3][4];
					TransformToMatrix3x4(bonematrix, bonematrix_3x4);

					memcpy((*pbonetransform)[pSharedUserData->m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
					memcpy((*plighttransform)[pSharedUserData->m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
				}
				else
				{
					auto& bonematrix = pBoneMotionState->m_bonematrix;

					Matrix3x4ToTransform((*pbonetransform)[pSharedUserData->m_boneindex], bonematrix);
				}
			}
		}

		return true;
	}

	void ResetPose(entity_state_t* curstate) override
	{
		ClientPhysicManager()->SetupBonesForRagdoll(GetClientEntity(), curstate, GetModel(), GetEntityIndex(), GetPlayerIndex());

		for (auto pRigidBody : m_RigidBodies)
		{
			if (pRigidBody->isStaticOrKinematicObject())
				continue;

			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			auto pMotionState = (CBulletBaseMotionState*)pRigidBody->getMotionState();

			if (pMotionState->IsBoneBased())
			{
				auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

				btTransform bonematrix;

				Matrix3x4ToTransform((*pbonetransform)[pSharedUserData->m_boneindex], bonematrix);

				auto newWorldTrans = bonematrix * pBoneMotionState->m_offsetmatrix;

				pRigidBody->setWorldTransform(newWorldTrans);
				pRigidBody->setInterpolationWorldTransform(newWorldTrans);
				pRigidBody->getMotionState()->setWorldTransform(newWorldTrans);
			}

			//
		}

		//m_bUpdateKinematic = true;
		//m_flNextUpdateKinematicTime = curstate->msg_time + 0.05f;
	}

	void Update(CPhysicObjectUpdateContext* ctx) override
	{
		CBaseRagdollObject::Update(ctx);

		CheckConstraintLinearErrors(ctx);

		for (auto pRigidBody : m_RigidBodies)
		{
			if (UpdateRigidBodyKinematic(pRigidBody, false, false))
				ctx->bRigidbodyKinematicChanged = true;
		}
	}

	void TransformOwnerEntity(int entindex) override
	{
		m_entindex = entindex;
		m_entity = ClientEntityManager()->GetEntityByIndex(entindex);
	}

	void AddToPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto RigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(RigidBody);

			dynamicWorld->addRigidBody(RigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);
		}

		for (auto Constraint : m_Constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(Constraint);

			dynamicWorld->addConstraint(Constraint, pSharedUserData->m_disableCollision);
		}
	}

	void RemoveFromPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto Constraint : m_Constraints)
		{
			dynamicWorld->removeConstraint(Constraint);
		}

		for (auto RigidBody : m_RigidBodies)
		{
			dynamicWorld->removeRigidBody(RigidBody);
		}
	}

	bool IsClientEntityNonSolid() const override
	{
		if (GetActivityType() > 0)
			return false;

		return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
	}

private:

	void CheckConstraintLinearErrors(CPhysicObjectUpdateContext* ctx)
	{
		if (ctx->bRigidbodyPoseChanged)
			return;

		bool bShouldResetPose = false;

		for (auto pConstraint : m_Constraints)
		{
			auto &pRigidBodyA = pConstraint->getRigidBodyA();
			auto &pRigidBodyB = pConstraint->getRigidBodyB();

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
				auto errorMagnitude = GetConstraintLinearErrorMagnitude(pConstraint);

				if (errorMagnitude > BULLET_MAX_TOLERANT_LINEAR_ERROR)
				{
					bShouldResetPose = true;
					break;
				}
			}
		}

		if (bShouldResetPose)
		{
			ResetPose(GetClientEntityState());

			ctx->bRigidbodyPoseChanged = true;
		}
	}

	bool UpdateRigidBodyKinematic(btRigidBody* pRigidBody, bool bForceKinematic, bool bForceDynamic)
	{
		auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

		bool bKinematic = false;

		bool bKinematicStateChanged = false;

		do
		{
			if (bForceKinematic)
			{
				bKinematic = true;
				break;
			}

			if (bForceDynamic)
			{
				bKinematic = false;
				break;
			}

			if (pSharedUserData->m_flags & PhysicRigidBodyFlag_AlwaysKinematic)
			{
				bKinematic = true;
				break;
			}

			if (pSharedUserData->m_flags & PhysicRigidBodyFlag_AlwaysDynamic)
			{
				bKinematic = false;
				break;
			}

			if (GetActivityType() > 0)
			{
				bKinematic = false;
				break;
			}
			else
			{
				bKinematic = true;
				break;
			}

		} while (0);

		if (bKinematic)
		{
			int iCollisionFlags = pRigidBody->getCollisionFlags();

			if (!(iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT))
			{
				iCollisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;

				pRigidBody->setCollisionFlags(iCollisionFlags);
				pRigidBody->setActivationState(DISABLE_DEACTIVATION);
				pRigidBody->setGravity(btVector3(0, 0, 0));

				bKinematicStateChanged = true;
			}
		}
		else
		{
			int iCollisionFlags = pRigidBody->getCollisionFlags();

			if (iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT)
			{
				iCollisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;

				pRigidBody->setCollisionFlags(iCollisionFlags);
				pRigidBody->forceActivationState(ACTIVE_TAG);
				pRigidBody->setMassProps(pSharedUserData->m_mass, pSharedUserData->m_inertia);

				bKinematicStateChanged = true;
			}
		}

		return bKinematicStateChanged;
	}

	void SaveBoneRelativeTransform(const CRagdollObjectCreationParameter& CreationParam)
	{
		auto pbones = (mstudiobone_t*)((byte*)CreationParam.m_studiohdr + CreationParam.m_studiohdr->boneindex);

		//Save bone relative transform

		for (int i = 0; i < CreationParam.m_studiohdr->numbones; ++i)
		{
			int parent = pbones[i].parent;

			if (parent == -1)
			{
				Matrix3x4ToTransform((*pbonetransform)[i], m_BoneRelativeTransform[i]);
			}
			else
			{
				btTransform matrix;

				Matrix3x4ToTransform((*pbonetransform)[i], matrix);

				btTransform parentmatrix;
				Matrix3x4ToTransform((*pbonetransform)[pbones[i].parent], parentmatrix);

				m_BoneRelativeTransform[i] = parentmatrix.inverse() * matrix;
			}
		}
	}

	void SetupNonKeyBones(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (int i = 0; i < CreationParam.m_studiohdr->numbones; ++i)
		{
			if (std::find(m_keyBones.begin(), m_keyBones.end(), i) == m_keyBones.end())
				m_nonKeyBones.emplace_back(i);
		}
	}

	btMotionState* CreateMotionState(const CRagdollObjectCreationParameter& CreationParam, const CClientRigidBodyConfig* pRigidConfig)
	{
		if (pRigidConfig->isLegacyConfig)
		{
			btTransform bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[pRigidConfig->boneindex], bonematrix);

			if (!(pRigidConfig->pboneindex >= 0 && pRigidConfig->pboneindex < CreationParam.m_studiohdr->numbones))
			{
				gEngfuncs.Con_Printf("CreateMotionState: invalid pConfig->pboneindex (%d).\n", pRigidConfig->pboneindex);
				return nullptr;
			}

			const auto& boneorigin = bonematrix.getOrigin();

			btVector3 pboneorigin((*pbonetransform)[pRigidConfig->pboneindex][0][3], (*pbonetransform)[pRigidConfig->pboneindex][1][3], (*pbonetransform)[pRigidConfig->pboneindex][2][3]);

			btVector3 vecDirection = pboneorigin - boneorigin;

			vecDirection = vecDirection.normalize();

			btVector3 vecOriginWorldSpace = boneorigin + vecDirection * pRigidConfig->pboneoffset;

			btTransform bonematrix2 = bonematrix;
			bonematrix2.setOrigin(vecOriginWorldSpace);

			btVector3 fwd(0, 1, 0);

			auto rigidTransformWorldSpace = MatrixLookAt(bonematrix2, pboneorigin, fwd);

			btTransform offsetmatrix;

			offsetmatrix.mult(bonematrix.inverse(), rigidTransformWorldSpace);

			return new CBulletBoneMotionState(this, bonematrix, offsetmatrix);
		}

		if (pRigidConfig->boneindex >= 0 && pRigidConfig->boneindex < CreationParam.m_studiohdr->numbones)
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

	btCollisionShape* CreateCollisionShapeInternal(const CRagdollObjectCreationParameter& CreationParam, const CClientCollisionShapeConfig* pConfig)
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
		}

		return pShape;
	}

	btCollisionShape* CreateCollisionShape(const CRagdollObjectCreationParameter& CreationParam, const CClientRigidBodyConfig* pConfig)
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

	btRigidBody* CreateRigidBody(const CRagdollObjectCreationParameter& CreationParam, const CClientRigidBodyConfig* pRigidConfig)
	{
		if (FindRigidBodyByName(pRigidConfig->name))
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create duplicated one \"%s\".\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		if (pRigidConfig->mass < 0)
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because mass < 0.\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		auto pMotionState = CreateMotionState(CreationParam, pRigidConfig);

		if (!pMotionState)
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because there is no MotionState available.\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		auto pCollisionShape = CreateCollisionShape(CreationParam, pRigidConfig);

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

		auto pRigidBody = new btRigidBody(cInfo);

		pRigidBody->setUserPointer(new CBulletRigidBodySharedUserData(
			cInfo,
			btBroadphaseProxy::DefaultFilter | BulletPhysicCollisionFilterGroups::RagdollObjectFilter,
			btBroadphaseProxy::AllFilter,
			pRigidConfig->name,
			pRigidConfig->flags & ~PhysicRigidBodyFlag_AlwaysStatic,
			pRigidConfig->boneindex,
			pRigidConfig->debugDrawLevel,
			pRigidConfig->density));

		pRigidBody->setCcdSweptSphereRadius(pRigidConfig->ccdRadius);
		pRigidBody->setCcdMotionThreshold(pRigidConfig->ccdThreshold);

		UpdateRigidBodyKinematic(pRigidBody, false, false);

		return pRigidBody;
	}

	btTypedConstraint* CreateConstraintInternal(const CRagdollObjectCreationParameter& CreationParam, const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& globalJointTransform)
	{
		btTypedConstraint* pConstraint{ };

		switch (pConstraintConfig->type)
		{
		case PhysicConstraint_ConeTwist:
		{
			auto spanLimit1 = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit1];

			if (isnan(spanLimit1))
				return nullptr;

			auto spanLimit2 = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit2];

			if (isnan(spanLimit2))
				return nullptr;

			auto twistLimit = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistTwistSpanLimit];

			if (isnan(twistLimit))
				return nullptr;

			auto softness = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistSoftness];

			if (isnan(softness))
				softness = BULLET_DEFAULT_SOFTNESS;

			auto biasFactor = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistBiasFactor];

			if (isnan(biasFactor))
				biasFactor = BULLET_DEFAULT_BIAS_FACTOR;

			auto relaxationFactor = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistRelaxationFactor];

			if (isnan(relaxationFactor))
				relaxationFactor = BULLET_DEFAULT_RELAXTION_FACTOR;

			auto LinearERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearERP];

			if (isnan(LinearERP))
				LinearERP = BULLET_DEFAULT_LINEAR_ERP;

			auto LinearCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearCFM];

			if (isnan(LinearCFM))
				LinearCFM = BULLET_DEFAULT_LINEAR_CFM;

			auto LinearStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopERP];

			if (isnan(LinearStopERP))
				LinearStopERP = BULLET_DEFAULT_LINEAR_STOP_ERP;

			auto LinearStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopCFM];

			if (isnan(LinearStopCFM))
				LinearStopCFM = BULLET_DEFAULT_LINEAR_STOP_CFM;

			auto AngularERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularERP];

			if (isnan(AngularERP))
				AngularERP = BULLET_DEFAULT_ANGULAR_ERP;

			auto AngularCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularCFM];

			if (isnan(AngularCFM))
				AngularCFM = BULLET_DEFAULT_ANGULAR_CFM;

			auto AngularStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopERP];

			if (isnan(AngularStopERP))
				AngularStopERP = BULLET_DEFAULT_ANGULAR_STOP_ERP;

			auto AngularStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopCFM];

			if (isnan(AngularStopCFM))
				AngularStopCFM = BULLET_DEFAULT_ANGULAR_STOP_CFM;

			const auto& worldTransA = rbA->getWorldTransform();

			const auto& worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			auto pConeTwist = new btConeTwistConstraint(*rbA, *rbB, localA, localB);

			pConeTwist->setLimit(spanLimit1 * M_PI, spanLimit2 * M_PI, twistLimit * M_PI, softness, biasFactor, relaxationFactor);

			pConeTwist->setParam(BT_CONSTRAINT_ERP, LinearERP, 0);
			pConeTwist->setParam(BT_CONSTRAINT_ERP, LinearERP, 1);
			pConeTwist->setParam(BT_CONSTRAINT_ERP, LinearERP, 2);
			pConeTwist->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
			pConeTwist->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
			pConeTwist->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);

			pConeTwist->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 1);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 2);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 1);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 2);

			pConeTwist->setParam(BT_CONSTRAINT_ERP, AngularERP, 3);
			pConeTwist->setParam(BT_CONSTRAINT_ERP, AngularERP, 4);
			pConeTwist->setParam(BT_CONSTRAINT_CFM, AngularCFM, 3);
			pConeTwist->setParam(BT_CONSTRAINT_CFM, AngularCFM, 4);

			pConeTwist->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 3);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 4);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 3);
			pConeTwist->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 4);

			pConstraint = pConeTwist;

			break;
		}
		case PhysicConstraint_Hinge:
		{
			auto lowLimit = pConstraintConfig->factors[PhysicConstraintFactorIdx_HingeLowLimit];

			if (isnan(lowLimit))
				return nullptr;

			auto highLimit = pConstraintConfig->factors[PhysicConstraintFactorIdx_HingeHighLimit];

			if (isnan(highLimit))
				return nullptr;

			auto softness = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistSoftness];

			if (isnan(softness))
				softness = BULLET_DEFAULT_SOFTNESS;

			auto biasFactor = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistBiasFactor];

			if (isnan(biasFactor))
				biasFactor = BULLET_DEFAULT_BIAS_FACTOR;

			auto relaxationFactor = pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistRelaxationFactor];

			if (isnan(relaxationFactor))
				relaxationFactor = BULLET_DEFAULT_RELAXTION_FACTOR;

			auto AngularERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularERP];

			if (isnan(AngularERP))
				AngularERP = BULLET_DEFAULT_ANGULAR_ERP;

			auto AngularCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularCFM];

			if (isnan(AngularCFM))
				AngularCFM = BULLET_DEFAULT_ANGULAR_CFM;

			auto AngularStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopERP];

			if (isnan(AngularStopERP))
				AngularStopERP = BULLET_DEFAULT_ANGULAR_STOP_ERP;

			auto AngularStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopCFM];

			if (isnan(AngularStopCFM))
				AngularStopCFM = BULLET_DEFAULT_ANGULAR_STOP_CFM;

			const auto& worldTransA = rbA->getWorldTransform();

			const auto& worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			auto pHinge = new btHingeConstraint(*rbA, *rbB, localA, localB);

			pHinge->setLimit(lowLimit, highLimit, softness, biasFactor, relaxationFactor);

			pHinge->setParam(BT_CONSTRAINT_ERP, AngularERP, 5);
			pHinge->setParam(BT_CONSTRAINT_CFM, AngularCFM, 5);

			pHinge->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 5);
			pHinge->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 5);

			pConstraint = pHinge;
			break;
		}
		case PhysicConstraint_Point:
		{
			auto LinearERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearERP];

			if (isnan(LinearERP))
				LinearERP = BULLET_DEFAULT_LINEAR_ERP;

			auto LinearCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearCFM];

			if (isnan(LinearCFM))
				LinearCFM = BULLET_DEFAULT_LINEAR_CFM;

			auto LinearStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopERP];

			if (isnan(LinearStopERP))
				LinearStopERP = BULLET_DEFAULT_LINEAR_STOP_ERP;

			auto LinearStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopCFM];

			if (isnan(LinearStopCFM))
				LinearStopCFM = BULLET_DEFAULT_LINEAR_STOP_CFM;

			const auto& worldTransA = rbA->getWorldTransform();

			const auto& worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			auto pPoint2Point = new btPoint2PointConstraint(*rbA, *rbB, localA.getOrigin(), localB.getOrigin());

			pPoint2Point->setParam(BT_CONSTRAINT_ERP, LinearERP, 0);
			pPoint2Point->setParam(BT_CONSTRAINT_ERP, LinearERP, 1);
			pPoint2Point->setParam(BT_CONSTRAINT_ERP, LinearERP, 2);
			pPoint2Point->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
			pPoint2Point->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
			pPoint2Point->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);

			pPoint2Point->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
			pPoint2Point->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 1);
			pPoint2Point->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 2);
			pPoint2Point->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
			pPoint2Point->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 1);
			pPoint2Point->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 2);

			pConstraint = pPoint2Point;

			break;
		}
		}

		if (!pConstraint)
			return nullptr;

		pConstraint->setUserConstraintPtr(new CBulletConstraintSharedUserData(pConstraintConfig));

		return pConstraint;
	}

	btTypedConstraint* CreateConstraint(const CRagdollObjectCreationParameter& CreationParam, const CClientConstraintConfig* pConstraintConfig)
	{
		auto pRigidBodyA = FindRigidBodyByName(pConstraintConfig->rigidbodyA);

		if (!pRigidBodyA)
		{
			gEngfuncs.Con_Printf("CreateConstraint: rigidbodyA \"%s\" not found!\n", pConstraintConfig->rigidbodyA.c_str());
			return nullptr;
		}

		auto pRigidBodyB = FindRigidBodyByName(pConstraintConfig->rigidbodyB);

		if (!pRigidBodyB)
		{
			gEngfuncs.Con_Printf("CreateConstraint: rigidbodyB \"%s\" not found!\n", pConstraintConfig->rigidbodyB.c_str());
			return nullptr;
		}

		if (pRigidBodyA == pRigidBodyB)
		{
			gEngfuncs.Con_Printf("CreateConstraint: rigidbodyA cannot be equal to rigidbodyA!\n");
			return nullptr;
		}

		btTransform globalJointTransform;

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

			btTransform bonematrixB;
			Matrix3x4ToTransform((*pbonetransform)[pConstraintConfig->boneindexB], bonematrixB);

			auto worldTransA = pRigidBodyA->getWorldTransform();
			auto worldTransB = pRigidBodyB->getWorldTransform();

			auto invWorldTransA = worldTransA.inverse();
			auto invWorldTransB = worldTransB.inverse();

			btVector3 offsetA(pConstraintConfig->offsetA[0], pConstraintConfig->offsetA[1], pConstraintConfig->offsetA[2]);

			//This converts bone A's world transform into rigidbody A's local space
			btTransform localTransA;
			localTransA.mult(invWorldTransA, bonematrixA);
			//Uses bone A's direction, but uses offsetA as local origin
			localTransA.setOrigin(offsetA);

			btVector3 offsetB(pConstraintConfig->offsetB[0], pConstraintConfig->offsetB[1], pConstraintConfig->offsetB[2]);

			//This converts bone B's world transform into rigidbody B's local space
			btTransform localTransB;
			localTransB.mult(invWorldTransB, bonematrixB);
			//Uses bone B's direction, but uses offsetB as local origin
			localTransB.setOrigin(offsetB);

			if (offsetA.fuzzyZero())
			{
				//Use B as final global joint
				globalJointTransform.mult(worldTransB, localTransB);
			}
			else
			{
				//Use A as final global joint
				globalJointTransform.mult(worldTransA, localTransA);
			}

			return CreateConstraintInternal(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform);
		}

		if (pConstraintConfig->isFromRigidBodyB)
		{
			auto worldTransB = pRigidBodyB->getWorldTransform();

			btVector3 vecOrigin(pConstraintConfig->origin[0], pConstraintConfig->origin[1], pConstraintConfig->origin[2]);

			btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

			btVector3 vecAngles(pConstraintConfig->angles[0], pConstraintConfig->angles[1], pConstraintConfig->angles[2]);

			EulerMatrix(vecAngles, localTrans.getBasis());

			globalJointTransform.mult(worldTransB, localTrans);

			return CreateConstraintInternal(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform);
		}
		else
		{
			auto worldTransA = pRigidBodyA->getWorldTransform();

			btVector3 vecOrigin(pConstraintConfig->origin[0], pConstraintConfig->origin[1], pConstraintConfig->origin[2]);

			btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

			btVector3 vecAngles(pConstraintConfig->angles[0], pConstraintConfig->angles[1], pConstraintConfig->angles[2]);

			EulerMatrix(vecAngles, localTrans.getBasis());

			globalJointTransform.mult(worldTransA, localTrans);

			return CreateConstraintInternal(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform);
		}

		return nullptr;
	}

	void CreateFloater(const CRagdollObjectCreationParameter& CreationParam, const CClientFloaterConfig* pConfig)
	{
		//TODO
	}

	void CreateRigidBodies(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (auto pRigidBodyConfig : CreationParam.m_pRagdollObjectConfig->RigidBodyConfigs)
		{
			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig);

			if (pRigidBody)
			{
				m_RigidBodies.emplace_back(pRigidBody);

				if (pRigidBodyConfig->boneindex >= 0)
					m_keyBones.emplace_back(pRigidBodyConfig->boneindex);

				if (pRigidBodyConfig->name == "Pelvis")
				{
					m_PelvisRigBody = pRigidBody;
				}
				else if (pRigidBodyConfig->name == "Head")
				{
					m_HeadRigBody = pRigidBody;
				}
			}
		}
	}

	void CreateConstraints(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (auto pConstraintConfig : CreationParam.m_pRagdollObjectConfig->ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig);

			if (pConstraint)
			{
				m_Constraints.emplace_back(pConstraint);
			}
		}
	}

	void CreateFloaters(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (auto pFloaterConfig : CreationParam.m_pRagdollObjectConfig->FloaterConfigs)
		{
			CreateFloater(CreationParam, pFloaterConfig);
		}
	}

public:

	std::vector<btRigidBody*> m_RigidBodies;
	std::vector<btTypedConstraint*> m_Constraints;
	std::vector<btRigidBody*> m_BarnacleDragBodies;
	std::vector<btRigidBody*> m_BarnacleChewBodies;
	std::vector<btRigidBody*> m_GargantuaDragBodies;
	std::vector<btTypedConstraint*> m_BarnacleConstraints;
	std::vector<btTypedConstraint*> m_GargantuaConstraints;
	btRigidBody* m_PelvisRigBody{};
	btRigidBody* m_HeadRigBody{};

	btTransform m_BoneRelativeTransform[128]{};
	//TODO
	//std::vector<ragdoll_bar_control_t> m_barcontrol;
	//std::vector<ragdoll_gar_control_t> m_garcontrol;

};