#pragma once

#include "BaseRagdollObject.h"
#include "BulletPhysicManager.h"

class CBulletRigidBodyBarnacleDragForceAction : public CBulletRigidBodyTickAction
{
public:
	CBulletRigidBodyBarnacleDragForceAction(btRigidBody* pRigidBody, int iBarnacleIndex, float flForce, float flExtraHeight) : 
		CBulletRigidBodyTickAction(pRigidBody), 
		m_iBarnacleIndex(iBarnacleIndex), 
		m_flForce(flForce),
		m_flExtraHeight(flExtraHeight)
	{

	}

	void Update(CPhysicObjectUpdateContext* ctx) override
	{
		auto pBarnacleEntity = ClientEntityManager()->GetEntityByIndex(m_iBarnacleIndex);

		if (!pBarnacleEntity)
			return;

		if (!ClientEntityManager()->IsEntityBarnacle(pBarnacleEntity))
			return;

		if (pBarnacleEntity->curstate.sequence == 5)
			return;

		vec3_t vecPhysicObjectOrigin{0};

		if (!ctx->m_pPhysicObject->GetOrigin(vecPhysicObjectOrigin))
			return;

		vec3_t vecClientEntityOrigin{ 0 };

		VectorCopy(ctx->m_pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

		if (vecPhysicObjectOrigin[2] > vecClientEntityOrigin[2] + m_flExtraHeight)
			return;

		btVector3 vecForce(0, 0, m_flForce);

		if (vecPhysicObjectOrigin[2] > vecClientEntityOrigin[2])
		{
			vecForce[2] *= (vecClientEntityOrigin[2] + m_flExtraHeight - vecPhysicObjectOrigin[2]) / m_flExtraHeight;
		}

		m_pRigidBody->applyCentralForce(vecForce);
	}

	int m_iBarnacleIndex{ -1 };
	float m_flExtraHeight{ 24 };
	float m_flForce{};
};

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

			auto pMotionState = GetMotionStateFromRigidBody(pRigidBody);

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

			auto pMotionState = GetMotionStateFromRigidBody(pRigidBody);

			if (pMotionState->IsBoneBased())
			{
				auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

				if (!pRigidBody->isKinematicObject())
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

			auto pMotionState = GetMotionStateFromRigidBody(pRigidBody);

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
		}
	}

	void ApplyBarnacle(cl_entity_t* pBarnacleEntity) override
	{
		CBaseRagdollObject::ApplyBarnacle(pBarnacleEntity);

		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->setLinearVelocity(btVector3(0, 0, 0));
			pRigidBody->setAngularVelocity(btVector3(0, 0, 0));


		}
	}

	void Update(CPhysicObjectUpdateContext* ctx) override
	{
		CBaseRagdollObject::Update(ctx);

		CheckConstraintLinearErrors(ctx);

		for (auto pRigidBody : m_RigidBodies)
		{
			if (UpdateRigidBodyKinematic(pRigidBody, false, false))
				ctx->m_bRigidbodyKinematicChanged = true;

			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (pSharedUserData)
			{
				for (auto pAction : pSharedUserData->m_actions)
				{
					pAction->Update(ctx);
				}
			}
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

		for (auto pRigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (!pSharedUserData->m_addedToPhysicWorld)
			{
				dynamicWorld->addRigidBody(pRigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);

				pSharedUserData->m_addedToPhysicWorld = true;
			}
		}

		for (auto pConstraint : m_Constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

			if (!pSharedUserData->m_addedToPhysicWorld)
			{
				dynamicWorld->addConstraint(pConstraint, pSharedUserData->m_disableCollision);

				pSharedUserData->m_addedToPhysicWorld = true;
			}
		}
	}

	void RemoveFromPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto pConstraint : m_Constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

			if (pSharedUserData->m_addedToPhysicWorld)
			{
				dynamicWorld->removeConstraint(pConstraint);

				pSharedUserData->m_addedToPhysicWorld = false;
			}
		}

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

	bool IsClientEntityNonSolid() const override
	{
		if (GetActivityType() > 0)
			return false;

		return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
	}

private:

	void CheckConstraintLinearErrors(CPhysicObjectUpdateContext* ctx)
	{
		if (ctx->m_bRigidbodyPoseChanged)
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
				auto errorMagnitude = BulletGetConstraintLinearErrorMagnitude(pConstraint);

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

			ctx->m_bRigidbodyPoseChanged = true;
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

		auto pMotionState = BulletCreateMotionState(CreationParam, pRigidConfig, this);

		if (!pMotionState)
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because there is no MotionState available.\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		auto pCollisionShape = BulletCreateCollisionShape(CreationParam, pRigidConfig);

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

			const auto& worldTransA = pRigidBodyA->getWorldTransform();
			const auto& worldTransB = pRigidBodyB->getWorldTransform();

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

			btTransform globalJointTransform;

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

			return BulletCreateConstraintFromGlobalJointTransform(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform);
		}

		if (!pConstraintConfig->useSeperateFrame)
		{
			if (pConstraintConfig->useGlobalJointFromA)
			{
				const auto& worldTransA = pRigidBodyA->getWorldTransform();

				btVector3 vecOrigin(pConstraintConfig->originA[0], pConstraintConfig->originA[1], pConstraintConfig->originA[2]);

				btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

				btVector3 vecAngles(pConstraintConfig->anglesA[0], pConstraintConfig->anglesA[1], pConstraintConfig->anglesA[2]);

				EulerMatrix(vecAngles, localTrans.getBasis());

				btTransform globalJointTransform;

				globalJointTransform.mult(worldTransA, localTrans);

				return BulletCreateConstraintFromGlobalJointTransform(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform); 
			}
			else
			{
				const auto& worldTransB = pRigidBodyB->getWorldTransform();

				btVector3 vecOrigin(pConstraintConfig->originB[0], pConstraintConfig->originB[1], pConstraintConfig->originB[2]);

				btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

				btVector3 vecAngles(pConstraintConfig->anglesB[0], pConstraintConfig->anglesB[1], pConstraintConfig->anglesB[2]);

				EulerMatrix(vecAngles, localTrans.getBasis());

				btTransform globalJointTransform;

				globalJointTransform.mult(worldTransB, localTrans);

				return BulletCreateConstraintFromGlobalJointTransform(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform);
			}
		}
		else
		{
			const auto& worldTransA = pRigidBodyA->getWorldTransform();
			const auto& worldTransB = pRigidBodyB->getWorldTransform();

			btVector3 vecOriginA(pConstraintConfig->originA[0], pConstraintConfig->originA[1], pConstraintConfig->originA[2]);
			btVector3 vecOriginB(pConstraintConfig->originB[0], pConstraintConfig->originB[1], pConstraintConfig->originB[2]);

			btTransform localTransA(btQuaternion(0, 0, 0, 1), vecOriginA);
			btTransform localTransB(btQuaternion(0, 0, 0, 1), vecOriginB);

			btVector3 vecAnglesA(pConstraintConfig->anglesA[0], pConstraintConfig->anglesA[1], pConstraintConfig->anglesA[2]);
			btVector3 vecAnglesB(pConstraintConfig->anglesB[0], pConstraintConfig->anglesB[1], pConstraintConfig->anglesB[2]);

			EulerMatrix(vecAnglesA, localTransA.getBasis());
			EulerMatrix(vecAnglesB, localTransB.getBasis());

			if (pConstraintConfig->useGlobalLookAt)
			{
				btTransform globalJointTransformA;

				globalJointTransformA.mult(worldTransA, localTransA);

				globalJointTransformA.getOrigin();

				btTransform globalJointTransformB;

				globalJointTransformB.mult(worldTransB, localTransB);

				globalJointTransformB.getOrigin();

				btVector3 vecForward(pConstraintConfig->forward[0], pConstraintConfig->forward[1], pConstraintConfig->forward[2]);

				if (pConstraintConfig->useLinearReferenceFrameA)
				{
					auto lookAtTransA = MatrixLookAt(globalJointTransformA, globalJointTransformB.getOrigin(), vecForward);

					localTransA.mult(worldTransA.inverse(), lookAtTransA);
				}
				else
				{
					auto lookAtTransB = MatrixLookAt(globalJointTransformB, globalJointTransformA.getOrigin(), vecForward);

					localTransB.mult(worldTransB.inverse(), lookAtTransB);
				}
			}

			return BulletCreateConstraintFromLocalJointTransform(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, localTransA, localTransB);
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
	btRigidBody* m_PelvisRigBody{};
	btRigidBody* m_HeadRigBody{};

	btTransform m_BoneRelativeTransform[128]{};

	std::vector<std::shared_ptr<CClientConstraintConfig>> m_BarnacleConstraintConfigs;
	std::vector<std::shared_ptr<CClientActionConfig>> m_BarnacleActionConfigs;

};