#pragma once

#include "BaseRagdollObject.h"
#include "BulletPhysicManager.h"

class CBulletBarnacleDragForceAction : public CBulletPhysicComponentAction
{
public:
	CBulletBarnacleDragForceAction(btRigidBody* pRigidBody, int flags, int iBarnacleIndex, float flForceMagnitude, float flExtraHeight) :
		CBulletPhysicComponentAction(pRigidBody->getUserIndex(), flags),
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

		auto pRigidBody = (btRigidBody *)ctx->m_pPhysicObject->GetRigidBodyByComponentId(m_iPhysicComponentId);

		if (!pRigidBody)
			return false;

		if (pBarnacleObject->GetClientEntityState()->sequence == 5)
		{
			btVector3 vecForce(0, 0, m_flForceMagnitude);

			pRigidBody->applyCentralForce(vecForce);
		}
		else
		{
			vec3_t vecPhysicObjectOrigin{ 0 };

			if (ctx->m_pPhysicObject->GetOrigin(vecPhysicObjectOrigin))
			{
				vec3_t vecClientEntityOrigin{ 0 };

				VectorCopy(ctx->m_pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

				if (vecPhysicObjectOrigin[2] < vecClientEntityOrigin[2] + m_flExtraHeight)
				{
					btVector3 vecForce(0, 0, m_flForceMagnitude);

					if (vecPhysicObjectOrigin[2] > vecClientEntityOrigin[2])
					{
						vecForce[2] *= (vecClientEntityOrigin[2] + m_flExtraHeight - vecPhysicObjectOrigin[2]) / m_flExtraHeight;
					}

					pRigidBody->applyCentralForce(vecForce);
				}
			}
		}
		
		return true;
	}

	int m_iBarnacleIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flExtraHeight{ 24 };
};

class CBulletBarnacleChewForceAction : public CBulletPhysicComponentAction
{
public:
	CBulletBarnacleChewForceAction(btRigidBody* pRigidBody, int flags, int iBarnacleIndex, float flForceMagnitude, float flInterval) :
		CBulletPhysicComponentAction(pRigidBody->getUserIndex(), flags),
		m_iBarnacleIndex(iBarnacleIndex),
		m_flForceMagnitude(flForceMagnitude),
		m_flInterval(flInterval)
	{

	}

	bool Update(CPhysicObjectUpdateContext* ctx) override
	{
		auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

		if (!pBarnacleObject)
			return false;

		if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
			return false;

		auto pRigidBody = (btRigidBody*)ctx->m_pPhysicObject->GetRigidBodyByComponentId(m_iPhysicComponentId);

		if (!pRigidBody)
			return false;

		if (pBarnacleObject->GetClientEntityState()->sequence == 5)
		{
			if (gEngfuncs.GetClientTime() > m_flNextChewTime)
			{
				pRigidBody->applyCentralImpulse(btVector3(0, 0, m_flForceMagnitude));

				m_flNextChewTime = gEngfuncs.GetClientTime() + m_flInterval;
			}
		}

		return true;
	}

	int m_iBarnacleIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flInterval{ 1 };
	float m_flNextChewTime{ 0 };
};

class CBulletBarnacleConstraintLimitAdjustmentAction : public CBulletPhysicComponentAction
{
public:
	CBulletBarnacleConstraintLimitAdjustmentAction(btTypedConstraint* pConstraint, int flags, int iBarnacleIndex, float flInterval, float flExtraHeight) :
		CBulletPhysicComponentAction(pConstraint->getUserConstraintId(), flags),
		m_iBarnacleIndex(iBarnacleIndex),
		m_flInterval(flInterval),
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

		auto pConstraint = (btTypedConstraint*)ctx->m_pPhysicObject->GetConstraintByComponentId(m_iPhysicComponentId);

		if (!pConstraint)
			return false;

		if (pBarnacleObject->GetClientEntityState()->sequence == 5)
		{
			if (gEngfuncs.GetClientTime() > m_flNextAdjustmentTime)
			{
				if (pConstraint->getConstraintType() == D6_CONSTRAINT_TYPE)
				{
					auto pDof6 = (btGeneric6DofConstraint*)pConstraint;

					if (1)
					{
						btVector3 currentLimit;
						pDof6->getLinearLowerLimit(currentLimit);

						if (currentLimit.x() < -1) {
							currentLimit.setX(currentLimit.x() - m_flExtraHeight);
						}
						else if (currentLimit.x() > 1) {
							currentLimit.setX(currentLimit.x() + m_flExtraHeight);
						}
						else if (currentLimit.y() < -1) {
							currentLimit.setY(currentLimit.y() - m_flExtraHeight);
						}
						else if (currentLimit.y() > 1) {
							currentLimit.setY(currentLimit.y() + m_flExtraHeight);
						}
						else if (currentLimit.z() < -1) {
							currentLimit.setZ(currentLimit.z() - m_flExtraHeight);
						}
						else if (currentLimit.z() > 1) {
							currentLimit.setZ(currentLimit.z() + m_flExtraHeight);
						}

						pDof6->setLinearLowerLimit(currentLimit);
					}

					if (1)
					{
						btVector3 currentLimit;
						pDof6->getLinearUpperLimit(currentLimit);

						if (currentLimit.x() < -1) {
							currentLimit.setX(currentLimit.x() - m_flExtraHeight);
						}
						else if (currentLimit.x() > 1) {
							currentLimit.setX(currentLimit.x() + m_flExtraHeight);
						}
						else if (currentLimit.y() < -1) {
							currentLimit.setY(currentLimit.y() - m_flExtraHeight);
						}
						else if (currentLimit.y() > 1) {
							currentLimit.setY(currentLimit.y() + m_flExtraHeight);
						}
						else if (currentLimit.z() < -1) {
							currentLimit.setZ(currentLimit.z() - m_flExtraHeight);
						}
						else if (currentLimit.z() > 1) {
							currentLimit.setZ(currentLimit.z() + m_flExtraHeight);
						}

						pDof6->setLinearUpperLimit(currentLimit);
					}
				}
				else if (pConstraint->getConstraintType() == SLIDER_CONSTRAINT_TYPE)
				{
					auto pSlider = (btSliderConstraint*)pConstraint;

					if (1)
					{
						auto currentLimit = pSlider->getLowerLinLimit();

						if (currentLimit < -1) {
							currentLimit = currentLimit - m_flExtraHeight;
						}
						else if (currentLimit > 1) {
							currentLimit = currentLimit + m_flExtraHeight;
						}

						pSlider->setLowerLinLimit(currentLimit);
					}

					if (1)
					{
						auto currentLimit = pSlider->getUpperLinLimit();

						if (currentLimit < -1) {
							currentLimit = currentLimit - m_flExtraHeight;
						}
						else if (currentLimit > 1) {
							currentLimit = currentLimit + m_flExtraHeight;
						}

						pSlider->setUpperLinLimit(currentLimit);
					}
				}
				m_flNextAdjustmentTime = gEngfuncs.GetClientTime() + m_flInterval;
			}
		}

		return true;
	}

	int m_iBarnacleIndex{ 0 };
	float m_flInterval{ 1 };
	float m_flExtraHeight{ 0 };
	float m_flNextAdjustmentTime{ 0 };
};

class CBulletCameraControl
{
public:
	CBulletCameraControl()
	{
		m_pRigidBody = nullptr;
		m_vecOrigin = btVector3(0, 0, 0);
		m_vecAngles = btVector3(0, 0, 0);
	}

	CBulletCameraControl(const CClientCameraControlConfig& pCameraControlConfig)
	{
		m_pRigidBody = nullptr;
		m_vecOrigin = btVector3(pCameraControlConfig.origin[0], pCameraControlConfig.origin[1], pCameraControlConfig.origin[2]);
		m_vecAngles = btVector3(pCameraControlConfig.angles[0], pCameraControlConfig.angles[1], pCameraControlConfig.angles[2]);
	}

	btRigidBody* m_pRigidBody{};
	btVector3 m_vecOrigin{};
	btVector3 m_vecAngles{};
};

class CBulletRagdollObject : public CBaseRagdollObject
{
public:
	CBulletRagdollObject(const CRagdollObjectCreationParameter& CreationParam) : CBaseRagdollObject(CreationParam)
	{
		m_IdleAnimConfig = CreationParam.m_pRagdollObjectConfig->IdleAnimConfig;
		m_AnimControlConfigs = CreationParam.m_pRagdollObjectConfig->AnimControlConfigs;
		m_BarnacleControlConfig = CreationParam.m_pRagdollObjectConfig->BarnacleControlConfig;

		SaveBoneRelativeTransform(CreationParam);
		CreateRigidBodies(CreationParam);
		CreateConstraints(CreationParam);
		CreateFloaters(CreationParam);
		SetupNonKeyBones(CreationParam);

		InitCameraControl(CreationParam);
	}

	~CBulletRagdollObject()
	{
		for (auto pAction : m_Actions)
		{
			OnBeforeDeletePhysicAction(this, pAction);

			delete pAction;
		}

		for (auto pConstraint : m_Constraints)
		{
			OnBeforeDeleteBulletConstraint(this, pConstraint);

			delete pConstraint;
		}

		m_Constraints.clear();

		for (auto pRigidBody : m_RigidBodies)
		{
			OnBeforeDeleteBulletRigidBody(this, pRigidBody);

			delete pRigidBody;
		}

		m_RigidBodies.clear();
	}

	bool GetOrigin(float* v) override
	{
		if (m_ThirdPersionViewCameraControl.m_pRigidBody)
		{
			const auto& worldTransform = m_ThirdPersionViewCameraControl.m_pRigidBody->getWorldTransform();

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
			//TODO ?
			//if (pRigidBody->isStaticOrKinematicObject())
			//	continue;

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
		CreationParam.m_allowNonNativeRigidBody = true;

		for (auto pRigidBody : m_RigidBodies)
		{
			pRigidBody->setLinearVelocity(btVector3(0, 0, 0));
			pRigidBody->setAngularVelocity(btVector3(0, 0, 0));
		}

		for (const auto& pConstraintConfig : m_BarnacleControlConfig.ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig.get());

			if (pConstraint)
			{
				m_Constraints.emplace_back(pConstraint);
			}
		}

		for (const auto& pActionConfig : m_BarnacleControlConfig.ActionConfigs)
		{
			auto pAction = CreateActionFromConfig(pActionConfig.get());

			if (pAction)
			{
				m_Actions.emplace_back(pAction);
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

	bool SyncThirdPersonView(struct ref_params_s* pparams, void(*callback)(struct ref_params_s* pparams)) override
	{
		if (m_ThirdPersionViewCameraControl.m_pRigidBody)
		{
			vec3_t vecSavedSimOrgigin;

			VectorCopy(pparams->simorg, vecSavedSimOrgigin);

			const auto& worldTrans = m_ThirdPersionViewCameraControl.m_pRigidBody->getWorldTransform();

			btTransform localTrans;
			localTrans.setIdentity();
			localTrans.setOrigin(m_FirstPersionViewCameraControl.m_vecOrigin);
			EulerMatrix(m_FirstPersionViewCameraControl.m_vecAngles, localTrans.getBasis());

			btTransform worldTransNew;
			worldTransNew.mult(worldTrans, localTrans);

			const auto& vecOrigin = worldTransNew.getOrigin();

			vec3_t vecGoldSrcOrigin;
			vecGoldSrcOrigin[0] = vecOrigin.getX();
			vecGoldSrcOrigin[1] = vecOrigin.getY();
			vecGoldSrcOrigin[2] = vecOrigin.getZ();

			VectorCopy(vecGoldSrcOrigin, pparams->simorg);

			callback(pparams);

			VectorCopy(vecSavedSimOrgigin, pparams->simorg);

			return true;
		}

		return false;
	}

	bool SyncFirstPersonView(struct ref_params_s* pparams, void(*callback)(struct ref_params_s* pparams)) override
	{
		if (m_FirstPersionViewCameraControl.m_pRigidBody)
		{
			const auto& worldTrans = m_FirstPersionViewCameraControl.m_pRigidBody->getWorldTransform();

			btTransform localTrans;
			localTrans.setIdentity();
			localTrans.setOrigin(m_FirstPersionViewCameraControl.m_vecOrigin);
			EulerMatrix(m_FirstPersionViewCameraControl.m_vecAngles, localTrans.getBasis());

			btTransform worldTransNew;
			worldTransNew.mult(worldTrans, localTrans);

			btVector3 vecAngles;
			MatrixEuler(worldTransNew.getBasis(), vecAngles);

			const auto &vecOrigin = worldTransNew.getOrigin();

			vec3_t vecGoldSrcAngles;
			vecGoldSrcAngles[0] = -vecAngles.getX();
			vecGoldSrcAngles[1] = vecAngles.getY();
			vecGoldSrcAngles[2] = vecAngles.getZ();

			vec3_t vecGoldSrcOrigin;
			vecGoldSrcOrigin[0] = vecOrigin.getX();
			vecGoldSrcOrigin[1] = vecOrigin.getY();
			vecGoldSrcOrigin[2] = vecOrigin.getZ();

			vec3_t vecSavedSimOrgigin;
			vec3_t vecSavedClientViewAngles;
			VectorCopy(pparams->simorg, vecSavedSimOrgigin);
			VectorCopy(pparams->cl_viewangles, vecSavedClientViewAngles);
			int iSavedHealth = pparams->health;

			pparams->viewheight[2] = 0;
			VectorCopy(vecGoldSrcOrigin, pparams->simorg);
			VectorCopy(vecGoldSrcAngles, pparams->cl_viewangles);
			pparams->health = 1;

			callback(pparams);

			pparams->health = iSavedHealth;
			VectorCopy(vecSavedSimOrgigin, pparams->simorg);
			VectorCopy(vecSavedClientViewAngles, pparams->cl_viewangles);

			return true;
		}
		return false;
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

		for (auto pConstraint : m_Constraints)
		{
			if (UpdateConstraintState(pConstraint))
				ctx->m_bConstraintStateChanged = true;
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

			if (!pSharedUserData->m_addedToPhysicWorld && BulletCheckPhysicComponentFiltersForRigidBody(pRigidBody, pSharedUserData, filters))
			{
				dynamicWorld->addRigidBody(pRigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);

				pSharedUserData->m_addedToPhysicWorld = true;
			}
		}

		for (auto pConstraint : m_Constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

			if (!pSharedUserData->m_addedToPhysicWorld && BulletCheckPhysicComponentFiltersForConstraint(pConstraint, pSharedUserData, filters))
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

			if (pSharedUserData->m_addedToPhysicWorld && BulletCheckPhysicComponentFiltersForRigidBody(pRigidBody, pSharedUserData, filters))
			{
				dynamicWorld->removeRigidBody(pRigidBody);

				pSharedUserData->m_addedToPhysicWorld = false;
			}
		}
	}

	void OnBroadcastDeleteRigidBody(IPhysicObject* pPhysicObjectToDelete, void* world, void* rigidbody) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;
		auto pRigidBody = (btRigidBody*)rigidbody;

		for (auto itor = m_Constraints.begin(); itor != m_Constraints.end(); )
		{
			auto pConstraint = (*itor);

			if (pRigidBody == &pConstraint->getRigidBodyA() || pRigidBody == &pConstraint->getRigidBodyB())
			{
				auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

				if (pSharedUserData->m_addedToPhysicWorld)
				{
					dynamicWorld->removeRigidBody(pRigidBody);

					pSharedUserData->m_addedToPhysicWorld = false;
				}

				delete pConstraint;
				itor = m_Constraints.erase(itor);
				continue;
			}

			itor++;
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

	void* GetRigidBodyByName(const std::string& name) override
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

	void* GetRigidBodyByComponentId(int id) override
	{
		for (auto pRigidBody : m_RigidBodies)
		{
			if (pRigidBody->getUserIndex() == id)
			{
				return pRigidBody;
			}
		}

		return nullptr;
	}

	void* GetConstraintByName(const std::string& name) override
	{
		for (auto pConstraint : m_Constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

			if (pSharedUserData)
			{
				if (pSharedUserData->m_name == name)
					return pConstraint;
			}
		}

		return nullptr;
	}

	void* GetConstraintByComponentId(int id) override
	{
		for (auto pConstraint : m_Constraints)
		{
			if (pConstraint->getUserConstraintId() == id)
				return pConstraint;
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

			return new CBulletBarnacleDragForceAction(
				pRigidBodyA,
				pActionConfig->flags,
				m_iBarnacleIndex,
				pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceMagnitude],
				pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceExtraHeight]);
		}
		case PhysicAction_BarnacleChewForce:
		{
			auto pRigidBodyA = (btRigidBody*)GetRigidBodyByName(pActionConfig->rigidbodyA);

			if (!pRigidBodyA)
				return nullptr;

			return new CBulletBarnacleChewForceAction(
				pRigidBodyA,
				pActionConfig->flags,
				m_iBarnacleIndex,
				pActionConfig->factors[PhysicActionFactorIdx_BarnacleChewForceMagnitude],
				pActionConfig->factors[PhysicActionFactorIdx_BarnacleChewForceInterval]);
		}
		case PhysicAction_BarnacleConstraintLimitAdjustment:
		{
			auto pConstraint = (btTypedConstraint*)GetConstraintByName(pActionConfig->constraint);

			if (!pConstraint)
				return nullptr;

			return new CBulletBarnacleConstraintLimitAdjustmentAction(
				pConstraint,
				pActionConfig->flags,
				m_iBarnacleIndex,
				pActionConfig->factors[PhysicActionFactorIdx_BarnacleConstraintLimitAdjustmentExtraHeight],
				pActionConfig->factors[PhysicActionFactorIdx_BarnacleConstraintLimitAdjustmentInterval]);
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
			if (!pConstraint->isEnabled())
				continue;

			auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

			if (pSharedUserData->m_flags & PhysicConstraintFlag_NonNative)
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

				if (errorMagnitude > pSharedUserData->m_maxTolerantLinearError)
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

	bool UpdateConstraintState(btTypedConstraint* pConstraint)
	{
		auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

		bool bDeactivate = false;

		bool bConstraintStateChanged = false;

		do
		{
			if (GetActivityType() == 0 && (pSharedUserData->m_flags & PhysicConstraintFlag_DeactiveOnNormalActivity))
			{
				bDeactivate = true;
				break;
			}

			if (GetActivityType() == 1 && (pSharedUserData->m_flags & PhysicConstraintFlag_DeactiveOnDeathActivity))
			{
				bDeactivate = true;
				break;
			}

			if (GetActivityType() == 2 && m_iBarnacleIndex && (pSharedUserData->m_flags & PhysicConstraintFlag_DeactiveOnBarnacleActivity))
			{
				bDeactivate = true;
				break;
			}

			if (GetActivityType() == 2 && m_iGargantuaIndex && (pSharedUserData->m_flags & PhysicConstraintFlag_DeactiveOnGargantuaActivity))
			{
				bDeactivate = true;
				break;
			}

		} while (0);

		if (bDeactivate)
		{
			if (pConstraint->isEnabled())
			{
				pConstraint->setEnabled(false);

				bConstraintStateChanged = true;
			}
		}
		else
		{
			if (!pConstraint->isEnabled())
			{
				pConstraint->setEnabled(true);

				bConstraintStateChanged = true;
			}
		}

		return bConstraintStateChanged;
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

	btRigidBody* FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody)
	{
		if (allowNonNativeRigidBody)
		{
			if (name.starts_with("@barnacle.") && m_iBarnacleIndex)
			{
				auto findName = name.substr(sizeof("@barnacle.") - 1);

				auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

				if (pBarnacleObject)
				{
					return (btRigidBody*)pBarnacleObject->GetRigidBodyByName(findName);
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
		}

		return (btRigidBody *)GetRigidBodyByName(name);
	}

	btRigidBody* CreateRigidBody(const CRagdollObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig)
	{
		if (GetRigidBodyByName(pRigidConfig->name))
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

		pRigidBody->setUserIndex(ClientPhysicManager()->AllocatePhysicComponentId());

		int group = btBroadphaseProxy::DefaultFilter | BulletPhysicCollisionFilterGroups::RagdollObjectFilter;

		int mask = btBroadphaseProxy::AllFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToWorld)
			mask &= ~BulletPhysicCollisionFilterGroups::WorldFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToStaticObject)
			mask &= ~BulletPhysicCollisionFilterGroups::StaticObjectFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToDynamicObject)
			mask &= ~BulletPhysicCollisionFilterGroups::DynamicObjectFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToRagdollObject)
			mask &= ~BulletPhysicCollisionFilterGroups::RagdollObjectFilter;

		pRigidBody->setUserPointer(new CBulletRigidBodySharedUserData(
			cInfo,
			group,
			mask,
			pRigidConfig->name,
			pRigidConfig->flags,
			pRigidConfig->boneindex,
			pRigidConfig->debugDrawLevel,
			pRigidConfig->density));

		pRigidBody->setCcdSweptSphereRadius(pRigidConfig->ccdRadius);
		pRigidBody->setCcdMotionThreshold(pRigidConfig->ccdThreshold);

		UpdateRigidBodyKinematic(pRigidBody, false, false);

		return pRigidBody;
	}

	btTypedConstraint* CreateConstraint(const CRagdollObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig)
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

		ctx.pRigidBodyA = pRigidBodyA;
		ctx.pRigidBodyB = pRigidBodyB;

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

			ctx.pRigidBodyA = pRigidBodyA;
			ctx.pRigidBodyB = pRigidBodyB;

			ctx.worldTransA = pRigidBodyA->getWorldTransform();
			ctx.worldTransB = pRigidBodyB->getWorldTransform();

			ctx.invWorldTransA = ctx.worldTransA.inverse();
			ctx.invWorldTransB = ctx.worldTransB.inverse();

			btVector3 offsetA(pConstraintConfig->offsetA[0], pConstraintConfig->offsetA[1], pConstraintConfig->offsetA[2]);

			//This converts bone A's world transform into rigidbody A's local space
			ctx.localTransA.mult(ctx.invWorldTransA, bonematrixA);
			//Uses bone A's direction, but uses offsetA as local origin
			ctx.localTransA.setOrigin(offsetA);

			btVector3 offsetB(pConstraintConfig->offsetB[0], pConstraintConfig->offsetB[1], pConstraintConfig->offsetB[2]);

			//This converts bone B's world transform into rigidbody B's local space
			ctx.localTransB.mult(ctx.invWorldTransB, bonematrixB);
			//Uses bone B's direction, but uses offsetB as local origin
			ctx.localTransB.setOrigin(offsetB);

			ctx.globalJointA.mult(ctx.worldTransA, ctx.localTransA);
			ctx.globalJointB.mult(ctx.worldTransB, ctx.localTransB);

			if (offsetA.fuzzyZero())
			{
				//Use B as final global joint transform
				return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointB);
			}
			else
			{
				//Use A as final global joint transform
				return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointA);
			}

			return nullptr;
		}

		ctx.worldTransA = pRigidBodyA->getWorldTransform();
		ctx.worldTransB = pRigidBodyB->getWorldTransform();

		ctx.invWorldTransA = ctx.worldTransA.inverse();
		ctx.invWorldTransB = ctx.worldTransB.inverse();

		btVector3 vecOriginA(pConstraintConfig->originA[0], pConstraintConfig->originA[1], pConstraintConfig->originA[2]);
		btVector3 vecOriginB(pConstraintConfig->originB[0], pConstraintConfig->originB[1], pConstraintConfig->originB[2]);

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
			if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit) {
				ctx.rigidBodyDistance = (ctx.globalJointA.getOrigin() - ctx.globalJointB.getOrigin()).length();
				if (!isnan(pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset]))
					ctx.rigidBodyDistance += pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset];
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

				return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, newGlobalJoint);
			}

			return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointA);
		}
		else
		{
			if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit) {
				ctx.rigidBodyDistance = (ctx.globalJointA.getOrigin() - ctx.globalJointB.getOrigin()).length();
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

				return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, newGlobalJoint);
			}

			return BulletCreateConstraintFromGlobalJointTransform(pConstraintConfig, ctx, ctx.globalJointB);
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

	void InitCameraControl(const CRagdollObjectCreationParameter& CreationParam)
	{
		m_FirstPersionViewCameraControl = CreationParam.m_pRagdollObjectConfig->FirstPersionViewCameraControlConfig;
		m_FirstPersionViewCameraControl.m_pRigidBody = (btRigidBody*)GetRigidBodyByName(CreationParam.m_pRagdollObjectConfig->FirstPersionViewCameraControlConfig.rigidbody);

		m_ThirdPersionViewCameraControl = CreationParam.m_pRagdollObjectConfig->ThirdPersionViewCameraControlConfig;
		m_ThirdPersionViewCameraControl.m_pRigidBody = (btRigidBody*)GetRigidBodyByName(CreationParam.m_pRagdollObjectConfig->ThirdPersionViewCameraControlConfig.rigidbody);
	}

public:
	std::vector<IPhysicAction*> m_Actions;
	std::vector<btRigidBody*> m_RigidBodies;
	std::vector<btTypedConstraint*> m_Constraints;
	CBulletCameraControl m_FirstPersionViewCameraControl;
	CBulletCameraControl m_ThirdPersionViewCameraControl;
	btTransform m_BoneRelativeTransform[128]{};
};