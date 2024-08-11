#pragma once

#include "BaseRagdollObject.h"
#include "BulletPhysicManager.h"

class CBulletRigidBodyBarnacleDragForceAction : public CBulletRigidBodyAction
{
public:
	CBulletRigidBodyBarnacleDragForceAction(btRigidBody* pRigidBody, int flags, int iBarnacleIndex, float flForceMagnitude, float flExtraHeight) : 
		CBulletRigidBodyAction(pRigidBody, flags), 
		m_iBarnacleIndex(iBarnacleIndex), 
		m_flForceMagnitude(flForceMagnitude),
		m_flExtraHeight(flExtraHeight)
	{

	}

	bool Update(CPhysicObjectUpdateContext* ctx) override
	{
		auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

		if (!pBarnacleObject)
			return false;

		if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
			return false;

		vec3_t vecPhysicObjectOrigin{0};

		if (!ctx->m_pPhysicObject->GetOrigin(vecPhysicObjectOrigin))
			return false;

		vec3_t vecClientEntityOrigin{ 0 };

		VectorCopy(ctx->m_pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

		if (vecPhysicObjectOrigin[2] > vecClientEntityOrigin[2] + m_flExtraHeight)
			return true;

		btVector3 vecForce(0, 0, m_flForceMagnitude);

		if (vecPhysicObjectOrigin[2] > vecClientEntityOrigin[2])
		{
			vecForce[2] *= (vecClientEntityOrigin[2] + m_flExtraHeight - vecPhysicObjectOrigin[2]) / m_flExtraHeight;
		}

		m_pRigidBody->applyCentralForce(vecForce);
		return true;
	}

	int m_iBarnacleIndex{ 0 };
	float m_flExtraHeight{ 24 };
	float m_flForceMagnitude{ 0 };
};

class CBulletRagdollObject : public CBaseRagdollObject
{
public:
	CBulletRagdollObject(const CRagdollObjectCreationParameter& CreationParam) : CBaseRagdollObject(CreationParam)
	{
		m_IdleAnimConfig = CreationParam.m_pRagdollObjectConfig->IdleAnimConfig;
		m_AnimControlConfigs = CreationParam.m_pRagdollObjectConfig->AnimControlConfigs;

		m_BarnacleActionConfigs = CreationParam.m_pRagdollObjectConfig->BarnacleActionConfigs;
		m_BarnacleConstraintConfigs = CreationParam.m_pRagdollObjectConfig->BarnacleConstraintConfigs;

		SaveBoneRelativeTransform(CreationParam);
		CreateRigidBodies(CreationParam);
		CreateConstraints(CreationParam);
		CreateFloaters(CreationParam);
		SetupNonKeyBones(CreationParam);
	}

	~CBulletRagdollObject()
	{
		for (auto pAction : m_Actions)
		{
			delete pAction;
		}

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

	void ApplyBarnacle(IPhysicObject *pBarnacleObject) override
	{
		if (m_iBarnacleIndex)
			return;

		m_iBarnacleIndex = pBarnacleObject->GetEntityIndex();

		CRagdollObjectCreationParameter CreationParam;

		CreationParam.m_entity = GetClientEntity();
		CreationParam.m_entindex = GetEntityIndex();
		CreationParam.m_model = GetModel();

		if (GetModel()->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(GetModel());
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(GetClientEntity(), GetModel());
		}

		CreationParam.m_playerindex = GetPlayerIndex();

		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->setLinearVelocity(btVector3(0, 0, 0));
			pRigidBody->setAngularVelocity(btVector3(0, 0, 0));
			//pRigidBody->setInterpolationLinearVelocity(btVector3(0, 0, 0));
			//pRigidBody->setInterpolationAngularVelocity(btVector3(0, 0, 0));
		}

		for (const auto& pActionConfig : m_BarnacleActionConfigs)
		{
			auto pAction = CreateActionFromConfig(pActionConfig.get());

			if (pAction)
			{
				m_Actions.emplace_back(pAction);
			}
		}

		for (const auto& pConstraintConfig : m_BarnacleConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get());

			if (pConstraint)
			{
				m_Constraints.emplace_back(pConstraint);
			}
		}

		CPhysicComponentFilters filters;

		filters.m_HasWithConstraintFlags = true;
		filters.m_WithConstraintFlags = PhysicConstraintFlag_Barnacle;

		ClientPhysicManager()->AddPhysicObjectToWorld(this, filters);
	}

	void ReleaseFromBarnacle() override
	{
		if (!m_iBarnacleIndex)
			return;

		m_iBarnacleIndex = 0;

		FreePhysicActionsWithFilters(PhysicActionFlag_Barnacle, 0);

		CPhysicComponentFilters filters;

		filters.m_HasWithConstraintFlags = true;
		filters.m_WithConstraintFlags = PhysicConstraintFlag_Barnacle;

		ClientPhysicManager()->RemovePhysicObjectFromWorld(this, filters);
	}

	void ApplyGargantua(IPhysicObject* pGargantuaObject) override
	{
		//TODO
	}

	void ReleaseFromGargantua() override
	{
		//TODO
	}

	void Update(CPhysicObjectUpdateContext* ctx) override
	{
		CBaseRagdollObject::Update(ctx);

		CheckConstraintLinearErrors(ctx);

		for (auto pRigidBody : m_RigidBodies)
		{
			if (UpdateRigidBodyKinematic(pRigidBody, false, false))
				ctx->m_bRigidbodyKinematicChanged = true;
		}

		for (auto itor = m_Actions.begin(); itor != m_Actions.end();)
		{
			auto pAction = (*itor);

			if (pAction->Update(ctx))
			{
				itor++;
			}
			else
			{
				delete pAction;
				itor = m_Actions.erase(itor);
			}
		}
	}

	void TransformOwnerEntity(int entindex) override
	{
		m_entindex = entindex;
		m_entity = ClientEntityManager()->GetEntityByIndex(entindex);
	}

	void AddToPhysicWorld(void* world, const CPhysicComponentFilters& filters) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto pRigidBody : m_RigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (!pSharedUserData->m_addedToPhysicWorld && BulletCheckPhysicComponentFiltersForRigidBody(pSharedUserData, filters))
			{
				dynamicWorld->addRigidBody(pRigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);

				pSharedUserData->m_addedToPhysicWorld = true;
			}
		}

		for (auto pConstraint : m_Constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

			if (!pSharedUserData->m_addedToPhysicWorld && BulletCheckPhysicComponentFiltersForConstraint(pSharedUserData, filters))
			{
				dynamicWorld->addConstraint(pConstraint, pSharedUserData->m_disableCollision);

				pSharedUserData->m_addedToPhysicWorld = true;
			}
		}
	}

	void RemoveFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) override
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

			if (pSharedUserData->m_addedToPhysicWorld && BulletCheckPhysicComponentFiltersForRigidBody(pSharedUserData, filters))
			{
				dynamicWorld->removeRigidBody(pRigidBody);

				pSharedUserData->m_addedToPhysicWorld = false;
			}
		}
	}

	void FreePhysicActionsWithFilters(int with_flags, int without_flags) override
	{
		for (auto itor = m_Actions.begin(); itor != m_Actions.end();)
		{
			auto pAction = (*itor);

			if (!(pAction->GetActionFlags() & without_flags))
			{
				itor++;
				continue;
			}

			if (with_flags == 0)
			{
				delete pAction;
				itor = m_Actions.erase(itor);
				continue;
			}
			else
			{
				if ((pAction->GetActionFlags() & with_flags))
				{
					delete pAction;
					itor = m_Actions.erase(itor);
					continue;
				}
			}

			itor++;
		}
	}

	void* GetRigidBodyByName(const std::string& name)
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

	bool IsClientEntityNonSolid() const override
	{
		if (GetActivityType() > 0)
			return false;

		return GetClientEntityState()->solid <= SOLID_TRIGGER ? true : false;
	}

private:

	IPhysicAction *CreateActionFromConfig(CClientPhysicActionConfig* pActionConfig)
	{
		switch (pActionConfig->type)
		{
		case PhysicAction_BarnacleDragForce:
		{
			auto pRigidBodyA = (btRigidBody *)GetRigidBodyByName(pActionConfig->rigidbodyA);

			if (!pRigidBodyA)
				return nullptr;

			return new CBulletRigidBodyBarnacleDragForceAction(
				pRigidBodyA,
				pActionConfig->flags,
				m_iBarnacleIndex,
				pActionConfig->factors[PhysicActionFactor_BarnacleDragForceMagnitude],
				pActionConfig->factors[PhysicActionFactor_BarnacleDragForceExtraHeight]);
		}
		}

		return nullptr;
	}

	void CheckConstraintLinearErrors(CPhysicObjectUpdateContext* ctx)
	{
		if (ctx->m_bRigidbodyPoseChanged)
			return;

		bool bShouldResetPose = false;

		for (auto pConstraint : m_Constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

			if (pSharedUserData->m_flags & PhysicConstraintFlag_Barnacle)
				continue;

			if (pSharedUserData->m_flags & PhysicConstraintFlag_Gargantua)
				continue;

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

				if (errorMagnitude > pSharedUserData->m_maxTolerantLinearErrorMagnitude)
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
		if (name.starts_with("@barnacle.") && m_iBarnacleIndex)
		{
			auto findName = name.substr(sizeof("@barnacle.") - 1);

			auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

			if (pBarnacleObject)
			{
				return (btRigidBody *)pBarnacleObject->GetRigidBodyByName(findName);
			}

			return nullptr;
		}
		else if (name.starts_with("@gargantua.") && m_iGargantuaIndex)
		{
			auto findName = name.substr(sizeof("@gargantua.") - 1);

			auto pGargantuaObject = ClientPhysicManager()->GetPhysicObject(m_iGargantuaIndex);

			if (pGargantuaObject)
			{
				return (btRigidBody*)pGargantuaObject->GetRigidBodyByName(findName);
			}

			return nullptr;
		}

		return (btRigidBody *)GetRigidBodyByName(name);
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

			return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform);
		}

		if (pConstraintConfig->useGlobalJointFromA)
		{
			const auto& worldTransA = pRigidBodyA->getWorldTransform();

			btVector3 vecOriginA(pConstraintConfig->originA[0], pConstraintConfig->originA[1], pConstraintConfig->originA[2]);

			btTransform localTransA(btQuaternion(0, 0, 0, 1), vecOriginA);

			btVector3 vecAnglesA(pConstraintConfig->anglesA[0], pConstraintConfig->anglesA[1], pConstraintConfig->anglesA[2]);

			EulerMatrix(vecAnglesA, localTransA.getBasis());

			btTransform globalJointTransformA;

			globalJointTransformA.mult(worldTransA, localTransA);

			if (pConstraintConfig->useLookAtOther)
			{
				const auto& worldTransB = pRigidBodyB->getWorldTransform();

				btVector3 vecOriginB(pConstraintConfig->originB[0], pConstraintConfig->originB[1], pConstraintConfig->originB[2]);

				btTransform localTransB(btQuaternion(0, 0, 0, 1), vecOriginB);

				btVector3 vecAnglesB(pConstraintConfig->anglesB[0], pConstraintConfig->anglesB[1], pConstraintConfig->anglesB[2]);

				EulerMatrix(vecAnglesB, localTransB.getBasis());

				btTransform globalJointTransformB;

				globalJointTransformB.mult(worldTransB, localTransB);

				btVector3 vecForward(pConstraintConfig->forward[0], pConstraintConfig->forward[1], pConstraintConfig->forward[2]);

				globalJointTransformA = MatrixLookAt(globalJointTransformA, globalJointTransformB.getOrigin(), vecForward);

				if (pConstraintConfig->useGlobalJointOriginFromOther)
				{
					globalJointTransformA.setOrigin(globalJointTransformB.getOrigin());
				}
			}

			return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransformA);
		}
		else
		{
			const auto& worldTransB = pRigidBodyB->getWorldTransform();

			btVector3 vecOriginB(pConstraintConfig->originB[0], pConstraintConfig->originB[1], pConstraintConfig->originB[2]);

			btTransform localTransB(btQuaternion(0, 0, 0, 1), vecOriginB);

			btVector3 vecAnglesB(pConstraintConfig->anglesB[0], pConstraintConfig->anglesB[1], pConstraintConfig->anglesB[2]);

			EulerMatrix(vecAnglesB, localTransB.getBasis());

			btTransform globalJointTransformB;

			globalJointTransformB.mult(worldTransB, localTransB);

			if (pConstraintConfig->useLookAtOther)
			{
				const auto& worldTransA = pRigidBodyA->getWorldTransform();

				btVector3 vecOriginA(pConstraintConfig->originA[0], pConstraintConfig->originA[1], pConstraintConfig->originA[2]);

				btTransform localTransA(btQuaternion(0, 0, 0, 1), vecOriginA);

				btVector3 vecAnglesA(pConstraintConfig->anglesA[0], pConstraintConfig->anglesA[1], pConstraintConfig->anglesA[2]);

				EulerMatrix(vecAnglesA, localTransA.getBasis());

				btTransform globalJointTransformA;

				globalJointTransformA.mult(worldTransA, localTransA);

				btVector3 vecForward(pConstraintConfig->forward[0], pConstraintConfig->forward[1], pConstraintConfig->forward[2]);

				globalJointTransformB = MatrixLookAt(globalJointTransformB, globalJointTransformA.getOrigin(), vecForward);

				if (pConstraintConfig->useGlobalJointOriginFromOther)
				{
					globalJointTransformB.setOrigin(globalJointTransformA.getOrigin());
				}
			}

			return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransformB);
		}

		return nullptr;
	}

	void CreateFloater(const CRagdollObjectCreationParameter& CreationParam, const CClientFloaterConfig* pConfig)
	{
		//TODO
	}

	void CreateRigidBodies(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto &pRigidBodyConfig : CreationParam.m_pRagdollObjectConfig->RigidBodyConfigs)
		{
			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig.get());

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
		for (const auto &pConstraintConfig : CreationParam.m_pRagdollObjectConfig->ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get());

			if (pConstraint)
			{
				m_Constraints.emplace_back(pConstraint);
			}
		}
	}

	void CreateFloaters(const CRagdollObjectCreationParameter& CreationParam)
	{
		for (const auto &pFloaterConfig : CreationParam.m_pRagdollObjectConfig->FloaterConfigs)
		{
			CreateFloater(CreationParam, pFloaterConfig.get());
		}
	}

public:

	std::vector<IPhysicAction*> m_Actions;
	std::vector<btRigidBody*> m_RigidBodies;
	std::vector<btTypedConstraint*> m_Constraints;
	btRigidBody* m_PelvisRigBody{};
	btRigidBody* m_HeadRigBody{};

	btTransform m_BoneRelativeTransform[128]{};

};