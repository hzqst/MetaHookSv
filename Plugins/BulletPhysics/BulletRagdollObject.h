#pragma once

#include "BaseRagdollObject.h"
#include "BulletPhysicManager.h"

class CBulletRagdollRigidBody : public CBulletRigidBody
{
public:
	CBulletRagdollRigidBody(
		int id,
		int entindex,
		const CClientRigidBodyConfig* pRigidConfig,
		const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
		int group, 
		int mask) :

		CBulletRigidBody(
			id,
			entindex, 
			pRigidConfig, 
			constructionInfo,
			group, 
			mask)
	{

	}

	bool ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate) override
	{
		if (!m_pInternalRigidBody)
			return false;

		auto pMotionState = (CBulletBaseMotionState*)m_pInternalRigidBody->getMotionState();

		if (pMotionState->IsBoneBased())
		{
			if (!(m_boneindex >= 0 && m_boneindex < studiohdr->numbones))
			{
				Sys_Error("CBulletRagdollRigidBody::SetupJiggleBones invalid m_boneindex!");
				return false;
			}

			auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

			btTransform bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[m_boneindex], bonematrix);

			TransformGoldSrcToBullet(bonematrix);

			auto newWorldTrans = bonematrix * pBoneMotionState->m_offsetmatrix;

			m_pInternalRigidBody->setWorldTransform(newWorldTrans);
			m_pInternalRigidBody->setInterpolationWorldTransform(newWorldTrans);
			m_pInternalRigidBody->getMotionState()->setWorldTransform(newWorldTrans);
		}

		return true;
	}

	bool SetupBones(studiohdr_t* studiohdr) override
	{
		if (!m_pInternalRigidBody)
			return false;

		auto pMotionState = (CBulletBaseMotionState*)m_pInternalRigidBody->getMotionState();

		if (pMotionState->IsBoneBased())
		{
			if (!(m_boneindex >= 0 && m_boneindex < studiohdr->numbones))
			{
				Sys_Error("CBulletRagdollRigidBody::SetupJiggleBones invalid m_boneindex!");
				return false;
			}

			auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

			btTransform bonematrix = pBoneMotionState->m_bonematrix;

			TransformBulletToGoldSrc(bonematrix);

			float bonematrix_3x4[3][4];
			TransformToMatrix3x4(bonematrix, bonematrix_3x4);

			memcpy((*pbonetransform)[m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
			memcpy((*plighttransform)[m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));

			return true;
		}

		return false;
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
				Sys_Error("CBulletRagdollRigidBody::SetupJiggleBones invalid m_boneindex!");
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

		auto pPhysicObject = ComponentUpdateContext->m_pObjectUpdateContext->m_pPhysicObject;

		if (!pPhysicObject)
			return;

		if (!pPhysicObject->IsRagdollObject())
			return;

		auto ent = pPhysicObject->GetClientEntity();

		auto pRagdollObject = (IRagdollObject*)pPhysicObject;

		bool bKinematic = false;

		bool bKinematicStateChanged = false;

		do
		{
			if (ComponentUpdateContext->m_bForceKinematic)
			{
				bKinematic = true;
				break;
			}

			if (ComponentUpdateContext->m_bForceDynamic)
			{
				bKinematic = false;
				break;
			}

			if (m_flags & PhysicRigidBodyFlag_AlwaysKinematic)
			{
				bKinematic = true;
				break;
			}

			if (m_flags & PhysicRigidBodyFlag_AlwaysDynamic)
			{
				bKinematic = false;
				break;
			}

			if (pRagdollObject->GetActivityType() > 0)
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

class CBulletRagdollConstraint : public CBulletConstraint
{
public:
	CBulletRagdollConstraint(
		int id,
		int entindex,
		CClientConstraintConfig* pConstraintConfig,
		btTypedConstraint * pInternalConstraint)
		:
		CBulletConstraint(
			id,
			entindex,
			pConstraintConfig,
			pInternalConstraint)
	{

	}

	void Update(CPhysicComponentUpdateContext* ComponentUpdateContext) override
	{
		if (!m_pInternalConstraint)
			return;

		bool bDeactivate = false;

		bool bConstraintStateChanged = false;

		auto pPhysicObject = ComponentUpdateContext->m_pObjectUpdateContext->m_pPhysicObject;

		if (!pPhysicObject)
			return;

		if (!pPhysicObject->IsRagdollObject())
			return;

		auto pRagdollObject = (IRagdollObject*)pPhysicObject;

		do
		{
			if (pRagdollObject->GetActivityType() == 0 && (m_flags & PhysicConstraintFlag_DeactiveOnNormalActivity))
			{
				bDeactivate = true;
				break;
			}

			if (pRagdollObject->GetActivityType() == 1 && (m_flags & PhysicConstraintFlag_DeactiveOnDeathActivity))
			{
				bDeactivate = true;
				break;
			}

			if (pRagdollObject->GetActivityType() == 2 && pRagdollObject->GetBarnacleIndex() && (m_flags & PhysicConstraintFlag_DeactiveOnBarnacleActivity))
			{
				bDeactivate = true;
				break;
			}

			if (pRagdollObject->GetActivityType() == 2 && pRagdollObject->GetGargantuaIndex() && (m_flags & PhysicConstraintFlag_DeactiveOnGargantuaActivity))
			{
				bDeactivate = true;
				break;
			}

		} while (0);

		if (bDeactivate)
		{
			if (m_pInternalConstraint->isEnabled())
			{
				m_pInternalConstraint->setEnabled(false);

				bConstraintStateChanged = true;
			}
		}
		else
		{
			if (!m_pInternalConstraint->isEnabled())
			{
				m_pInternalConstraint->setEnabled(true);

				bConstraintStateChanged = true;
			}
		}

		if (bConstraintStateChanged)
		{
			ComponentUpdateContext->m_pObjectUpdateContext->m_bConstraintStateChanged = true;
		}
	}

};

class CBulletBarnacleDragForceAction : public CPhysicComponentAction
{
public:
	CBulletBarnacleDragForceAction(btRigidBody* pRigidBody, int flags, int iBarnacleIndex, float flForceMagnitude, float flExtraHeight) :
		CPhysicComponentAction(pRigidBody->getUserIndex(), flags),
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

		auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(m_physicComponentId);

		if (!pPhysicComponent)
			return false;

		if (!pPhysicComponent->IsRigidBody())
			return false;

		auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

		if (pBarnacleObject->GetClientEntityState()->sequence == 5)
		{
			vec3_t vecForce = { 0, 0, m_flForceMagnitude };

			pRigidBody->ApplyCentralForce(vecForce);
		}
		else
		{
			vec3_t vecPhysicObjectOrigin{ 0 };

			if (ctx->m_pPhysicObject->GetGoldSrcOrigin(vecPhysicObjectOrigin))
			{
				vec3_t vecClientEntityOrigin{ 0 };

				VectorCopy(ctx->m_pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

				if (vecPhysicObjectOrigin[2] < vecClientEntityOrigin[2] + m_flExtraHeight)
				{
					btVector3 vecForce = { 0, 0, m_flForceMagnitude };

					if (vecPhysicObjectOrigin[2] > vecClientEntityOrigin[2])
					{
						vecForce[2] *= (vecClientEntityOrigin[2] + m_flExtraHeight - vecPhysicObjectOrigin[2]) / m_flExtraHeight;
					}

					pRigidBody->ApplyCentralForce(vecForce);
				}
			}
		}

		return true;
	}

	int m_iBarnacleIndex{ 0 };
	float m_flForceMagnitude{ 0 };
	float m_flExtraHeight{ 24 };
};

class CBulletBarnacleChewForceAction : public CPhysicComponentAction
{
public:
	CBulletBarnacleChewForceAction(btRigidBody* pRigidBody, int flags, int iBarnacleIndex, float flForceMagnitude, float flInterval) :
		CPhysicComponentAction(pRigidBody->getUserIndex(), flags),
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

		auto pRigidBody = (btRigidBody*)ctx->m_pPhysicObject->GetRigidBodyByComponentId(m_physicComponentId);

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

class CBulletBarnacleConstraintLimitAdjustmentAction : public CPhysicComponentAction
{
public:
	CBulletBarnacleConstraintLimitAdjustmentAction(btTypedConstraint* pConstraint, int flags, int iBarnacleIndex, float flInterval, float flExtraHeight) :
		CPhysicComponentAction(pConstraint->getUserConstraintId(), flags),
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

		auto pPhysicComponent = ClientPhysicManager()->GetPhysicComponent(m_physicComponentId);

		if (!pPhysicComponent)
			return false;

		if (!pPhysicComponent->IsConstraint())
			return false;

		auto pConstraint = (IPhysicConstraint*)pPhysicComponent;

		if (pBarnacleObject->GetClientEntityState()->sequence == 5)
		{
			if (gEngfuncs.GetClientTime() > m_flNextAdjustmentTime)
			{
				pConstraint->ExtendLinearLimit(-1, m_flExtraHeight);
#if 0
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
#endif
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

		Vector3GoldSrcToBullet(m_vecOrigin);
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

	}

	bool GetGoldSrcOrigin(float* v) override
	{
		if (m_ThirdPersionViewCameraControl.m_pRigidBody)
		{
			const auto& worldTransform = m_ThirdPersionViewCameraControl.m_pRigidBody->getWorldTransform();

			const auto& worldOrigin = worldTransform.getOrigin();

			v[0] = worldOrigin.x();
			v[1] = worldOrigin.y();
			v[2] = worldOrigin.z();

			Vec3BulletToGoldSrc(v);

			return true;
		}

		return false;
	}

	bool SetupBones(studiohdr_t* studiohdr) override
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

				btTransform mergedMatrix;

				mergedMatrix = parentMatrix * m_BoneRelativeTransform[i];

				TransformBulletToGoldSrc(mergedMatrix);

				TransformToMatrix3x4(mergedMatrix, (*pbonetransform)[i]);
			}

			return true;
		}

		return false;
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

			Vec3BulletToGoldSrc(vecGoldSrcOrigin);

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

			const auto& vecOrigin = worldTransNew.getOrigin();

			vec3_t vecGoldSrcAngles;
			vecGoldSrcAngles[0] = -vecAngles.getX();
			vecGoldSrcAngles[1] = vecAngles.getY();
			vecGoldSrcAngles[2] = vecAngles.getZ();

			vec3_t vecGoldSrcOrigin;
			vecGoldSrcOrigin[0] = vecOrigin.getX();
			vecGoldSrcOrigin[1] = vecOrigin.getY();
			vecGoldSrcOrigin[2] = vecOrigin.getZ();

			Vec3BulletToGoldSrc(vecGoldSrcOrigin);

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

private:

	IPhysicAction* CreateActionFromConfig(CClientPhysicActionConfig* pActionConfig) override
	{
		switch (pActionConfig->type)
		{
		case PhysicAction_BarnacleDragForce:
		{
			auto pRigidBodyA = (btRigidBody*)GetRigidBodyByName(pActionConfig->rigidbodyA);

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

#if 0//TODO use Update() instead
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

			auto& pRigidBodyA = pConstraint->getRigidBodyA();

			auto& pRigidBodyB = pConstraint->getRigidBodyB();

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

	//TODO use Update instead

#endif

	IPhysicRigidBody* CreateRigidBody(const CRagdollObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig) override
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

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToWorld)
			mask &= ~BulletPhysicCollisionFilterGroups::WorldFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToStaticObject)
			mask &= ~BulletPhysicCollisionFilterGroups::StaticObjectFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToDynamicObject)
			mask &= ~BulletPhysicCollisionFilterGroups::DynamicObjectFilter;

		if (pRigidConfig->flags & PhysicRigidBodyFlag_NoCollisionToRagdollObject)
			mask &= ~BulletPhysicCollisionFilterGroups::RagdollObjectFilter;

		auto physicComponentId = ClientPhysicManager()->AllocatePhysicComponentId();

		return new CBulletRagdollRigidBody(
			physicComponentId,
			CreationParam.m_entindex,
			pRigidConfig,
			cInfo,
			group,
			mask);
	}

	IPhysicConstraint* CreateConstraint(const CRagdollObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig) override
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

		}while (0);

		if (pInternalConstraint)
		{
			auto physicComponentId = ClientPhysicManager()->AllocatePhysicComponentId();

			return new CBulletRagdollConstraint(physicComponentId, CreationParam.m_entindex, pConstraintConfig, pInternalConstraint);
		}

		return nullptr;
	}

	void CreateFloater(const CRagdollObjectCreationParameter& CreationParam, const CClientFloaterConfig* pConfig)
	{

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
				TransformGoldSrcToBullet(m_BoneRelativeTransform[i]);
			}
			else
			{
				btTransform bonematrix;
				Matrix3x4ToTransform((*pbonetransform)[i], bonematrix);
				TransformGoldSrcToBullet(bonematrix);

				btTransform parentmatrix;
				Matrix3x4ToTransform((*pbonetransform)[pbones[i].parent], parentmatrix);
				TransformGoldSrcToBullet(parentmatrix);

				m_BoneRelativeTransform[i] = parentmatrix.inverse() * bonematrix;
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

	CBulletCameraControl m_FirstPersionViewCameraControl;
	CBulletCameraControl m_ThirdPersionViewCameraControl;
	btTransform m_BoneRelativeTransform[128]{};
};