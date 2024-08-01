#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "ClientEntityManager.h"
#include "BulletPhysicManager.h"

#include <glew.h>

void EulerMatrix(const btVector3& in_euler, btMatrix3x3& out_matrix) {
	btVector3 angles = in_euler;
	angles *= SIMD_RADS_PER_DEG;

	btScalar c1(btCos(angles[0]));
	btScalar c2(btCos(angles[1]));
	btScalar c3(btCos(angles[2]));
	btScalar s1(btSin(angles[0]));
	btScalar s2(btSin(angles[1]));
	btScalar s3(btSin(angles[2]));

	out_matrix.setValue(c1 * c2, -c3 * s2 - s1 * s3 * c2, s3 * s2 - s1 * c3 * c2,
		c1 * s2, c3 * c2 - s1 * s3 * s2, -s3 * c2 - s1 * c3 * s2,
		s1, c1 * s3, c1 * c3);
}

void MatrixEuler(const btMatrix3x3& in_matrix, btVector3& out_euler) {

	out_euler[0] = btAsin(in_matrix[2][0]);

	if (in_matrix[2][0] >= (1 - 0.002f) && in_matrix[2][0] < 1.002f) {
		out_euler[1] = btAtan2(in_matrix[1][0], in_matrix[0][0]);
		out_euler[2] = btAtan2(in_matrix[2][1], in_matrix[2][2]);
	}
	else if (btFabs(in_matrix[2][0]) < (1 - 0.002f)) {
		out_euler[1] = btAtan2(in_matrix[1][0], in_matrix[0][0]);
		out_euler[2] = btAtan2(in_matrix[2][1], in_matrix[2][2]);
	}
	else {
		out_euler[1] = btAtan2(in_matrix[1][2], in_matrix[1][1]);
		out_euler[2] = 0;
	}

	out_euler[3] = 0;

	out_euler *= SIMD_DEGS_PER_RAD;
}

void Matrix3x4ToTransform(const float matrix3x4[3][4], btTransform& trans)
{
	float matrix4x4[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};
	memcpy(matrix4x4, matrix3x4, sizeof(float[3][4]));

	float matrix4x4_transposed[4][4];
	Matrix4x4_Transpose(matrix4x4_transposed, matrix4x4);

	trans.setFromOpenGLMatrix((float*)matrix4x4_transposed);
}

void TransformToMatrix3x4(const btTransform& trans, float matrix3x4[3][4])
{
	float matrix4x4_transposed[4][4];
	trans.getOpenGLMatrix((float*)matrix4x4_transposed);

	float matrix4x4[4][4];
	Matrix4x4_Transpose(matrix4x4, matrix4x4_transposed);

	memcpy(matrix3x4, matrix4x4, sizeof(float[3][4]));
}

void CBulletEntityMotionState::getWorldTransform(btTransform& worldTrans) const
{
	if (GetPhysicObject()->IsStaticObject())
	{
		auto entindex = GetPhysicObject()->GetEntityIndex();
		auto ent = GetPhysicObject()->GetClientEntity();

		btVector3 vecOrigin(ent->curstate.origin[0], ent->curstate.origin[1], ent->curstate.origin[2]);

		worldTrans = btTransform(btQuaternion(0, 0, 0, 1), vecOrigin);

		btVector3 vecAngles(ent->curstate.angles[0], ent->curstate.angles[1], ent->curstate.angles[2]);

		//TODO: Brush uses reverted pitch ??
		if (ent->curstate.solid == SOLID_BSP)
		{
			//vecAngles.setX(-vecAngles.x());
		}

		EulerMatrix(vecAngles, worldTrans.getBasis());
	}
}

void CBulletEntityMotionState::setWorldTransform(const btTransform& worldTrans)
{

}

CBulletRigidBodySharedUserData* GetSharedUserDataFromRigidBody(btRigidBody *RigidBody)
{
	return (CBulletRigidBodySharedUserData*)RigidBody->getUserPointer();
}

CBulletConstraintSharedUserData* GetSharedUserDataFromConstraint(btTypedConstraint *Constraint)
{
	return (CBulletConstraintSharedUserData*)Constraint->getUserConstraintPtr();
}

CBulletCollisionShapeSharedUserData* GetSharedUserDataFromCollisionShape(btCollisionShape *pCollisionShape)
{
	return (CBulletCollisionShapeSharedUserData*)pCollisionShape->getUserPointer();
}

IPhysicObject* GetPhysicObjectFromRigidBody(btRigidBody *pRigidBody)
{
	auto pMotionState = (CBulletBaseMotionState *)pRigidBody->getMotionState();

	if (pMotionState)
	{
		return pMotionState->GetPhysicObject();
	}

	return nullptr;
}

void OnBeforeDeleteBulletCollisionShape(btCollisionShape* pCollisionShape)
{
	auto pSharedUserData = GetSharedUserDataFromCollisionShape(pCollisionShape);

	if (pSharedUserData)
	{
		delete pSharedUserData;

		pCollisionShape->setUserPointer(nullptr);
	}
}

void OnBeforeDeleteBulletRigidBody(btRigidBody* pRigidBody)
{
	auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

	if (pSharedUserData)
	{
		delete pSharedUserData;

		pRigidBody->setUserPointer(nullptr);
	}

	auto pCollisionShape = pRigidBody->getCollisionShape();

	if (pCollisionShape)
	{
		OnBeforeDeleteBulletCollisionShape(pCollisionShape);

		delete pCollisionShape;

		pRigidBody->setCollisionShape(nullptr);
	}

	auto pMotionState = pRigidBody->getMotionState();

	if (pMotionState)
	{
		delete pMotionState;

		pRigidBody->setMotionState(nullptr);
	}
}

void OnBeforeDeleteBulletConstraint(btTypedConstraint *pConstraint)
{
	auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

	if (pSharedUserData)
	{
		delete pSharedUserData;

		pConstraint->setUserConstraintPtr(nullptr);
	}
}

class CBulletStaticObject : public IStaticObject
{
public:
	CBulletStaticObject(const CStaticObjectCreationParameter& CreationParam)
	{
		m_entindex = CreationParam.m_entindex;
		m_entity = CreationParam.m_entity;
		m_model = CreationParam.m_model;
		m_pVertexArray = CreationParam.m_pVertexArray;
		m_pIndexArray = CreationParam.m_pIndexArray;

		CreateRigidBody(CreationParam);
	}

	~CBulletStaticObject()
	{
		if (m_pRigidBody)
		{
			OnBeforeDeleteBulletRigidBody(m_pRigidBody);

			delete m_pRigidBody;

			m_pRigidBody = nullptr;
		}
	}

	int GetEntityIndex() const override
	{
		return m_entindex;
	}

	cl_entity_t* GetClientEntity() const override
	{
		return m_entity;
	}

	bool GetOrigin(float* v) override
	{
		return false;
	}

	model_t* GetModel() const override
	{
		return m_model;
	}

	int GetPlayerIndex() const override
	{
		return 0;
	}

	int GetFlags() const override
	{
		return PhysicObjectFlag_Static;
	}

	bool Update() override
	{
		return true;
	}

	bool SetupBones(studiohdr_t* studiohdr) override
	{
		return false;
	}

	bool SetupJiggleBones(studiohdr_t* studiohdr) override
	{
		return false;
	}

	void TransformOwnerEntity(int entindex) override
	{
		m_entindex = entindex;
		m_entity = ClientEntityManager()->GetEntityByIndex(entindex);
	}

	void AddToPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		if (m_pRigidBody)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(m_pRigidBody);

			dynamicWorld->addRigidBody(m_pRigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);
		}
	}

	void RemoveFromPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		if (m_pRigidBody)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(m_pRigidBody);

			dynamicWorld->removeRigidBody(m_pRigidBody);
		}
	}

private:

	btCollisionShape* CreateCollisionShape(const CStaticObjectCreationParameter& CreationParam)
	{
		if (m_pVertexArray && m_pIndexArray && m_pIndexArray->vIndexBuffer.size() > 0 && m_pVertexArray->vVertexBuffer.size() > 0)
		{
			auto pIndexVertexArray = new btTriangleIndexVertexArray(
				m_pIndexArray->vIndexBuffer.size() / 3, m_pIndexArray->vIndexBuffer.data(), 3 * sizeof(int),
				m_pVertexArray->vVertexBuffer.size(), (float*)m_pVertexArray->vVertexBuffer.data(), sizeof(CPhysicBrushVertex));

			auto pCollisionShape = new btBvhTriangleMeshShape(pIndexVertexArray, true, true);

			pCollisionShape->setUserPointer(new CBulletCollisionShapeSharedUserData(pIndexVertexArray));

			return pCollisionShape;
		}

		//TODO?
		return nullptr;
	}

	btMotionState* CreateMotionState(const CStaticObjectCreationParameter& CreationParam)
	{
		return new CBulletEntityMotionState(this);
	}

	void CreateRigidBody(const CStaticObjectCreationParameter& CreationParam)
	{
		auto pCollisionShape = CreateCollisionShape(CreationParam);

		if (!pCollisionShape)
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create rigid body because there is no CollisionShape available.\n");
			return;
		}

		auto pMotionState = CreateMotionState(CreationParam);

		btRigidBody::btRigidBodyConstructionInfo cInfo(0, pMotionState, pCollisionShape);
		cInfo.m_friction = 1;
		cInfo.m_rollingFriction = 1;
		cInfo.m_restitution = 1;

		m_pRigidBody = new btRigidBody(cInfo);

		m_pRigidBody->setUserPointer(new CBulletRigidBodySharedUserData(m_model->name, CreationParam.m_bIsKinematic ? PhysicRigidBodyFlag_AlwaysKinematic : PhysicRigidBodyFlag_AlwaysStatic, -1, cInfo));
	}

public:

	int m_entindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	CPhysicVertexArray* m_pVertexArray{};
	CPhysicIndexArray* m_pIndexArray{};
	btRigidBody* m_pRigidBody{};
};

class CBulletRagdollObject : public IRagdollObject
{
public:
	CBulletRagdollObject(const CRagdollObjectCreationParameter& CreationParam)
	{
		m_flLastCreateTime = gEngfuncs.GetClientTime();
		m_entindex = CreationParam.m_entindex;
		m_entity = CreationParam.m_entity;
		m_model = CreationParam.m_model;

		SaveBoneRelativeTransform(CreationParam);
		CreateRigidBodies(CreationParam, CreationParam.m_pPhysConfigs);
		CreateConstraints(CreationParam, CreationParam.m_pPhysConfigs);
		CreateFloaters(CreationParam, CreationParam.m_pPhysConfigs);
		SetupNonKeyBones(CreationParam);
	}

	~CBulletRagdollObject()
	{
		for (auto pConstraint : m_constraints)
		{
			OnBeforeDeleteBulletConstraint(pConstraint);

			delete pConstraint;
		}

		m_constraints.clear();

		for (auto pRigidBody : m_rigidBodies)
		{
			OnBeforeDeleteBulletRigidBody(pRigidBody);

			delete pRigidBody;
		}

		m_rigidBodies.clear();
	}

	int GetEntityIndex() const override
	{
		return m_entindex;
	}

	cl_entity_t *GetClientEntity() const override
	{
		return m_entity;
	}

	bool GetOrigin(float* v) override
	{
		if (m_pelvisRigBody)
		{
			const auto &worldTransform = m_pelvisRigBody->getWorldTransform();

			const auto &worldOrigin = worldTransform.getOrigin();

			v[0] = worldOrigin.x();
			v[1] = worldOrigin.y();
			v[2] = worldOrigin.z();

			return true;
		}

		return false;
	}

	model_t* GetModel() const override
	{
		return m_model;
	}

	int GetPlayerIndex() const override
	{
		return m_playerindex;
	}

	int GetFlags() const override
	{
		return PhysicObjectFlag_Ragdoll;
	}

	void ResetPose(entity_state_t* curstate) override
	{
		//TODO

	}

	void ApplyBarnacle(cl_entity_t* barnacle_entity) override
	{
		//TODO

	}

	void ApplyGargantua(cl_entity_t* gargantua_entity) override
	{
		//TODO

	}

	bool SyncFirstPersonView(cl_entity_t* ent, struct ref_params_s* pparams) override
	{
		//TODO

		return false;
	}

	void ForceSleep() override
	{
		//TODO

	}

	int GetOverrideActivityType(entity_state_t* entstate) override
	{
		//TODO Fetch activity from animConfigs

		return 0;
	}

	int GetActivityType() const override
	{
		return m_iActivityType;
	}

	bool Update() override
	{
		auto playerstate = R_GetPlayerState(m_playerindex);

		int iOldActivityType = GetActivityType();

		int iNewActivityType = StudioGetSequenceActivityType(m_model, playerstate);

		if (iNewActivityType == 0)
		{
			iNewActivityType = GetOverrideActivityType(playerstate);
		}

		//not dead player
		if (m_playerindex == m_entindex)
		{
			if (iNewActivityType == 1)
			{
				ClientEntityManager()->SetPlayerDeathState(m_playerindex, playerstate, m_model);
			}
			else
			{
				ClientEntityManager()->ClearPlayerDeathState(m_playerindex);
			}
		}

		if (UpdateKinematic(iNewActivityType, playerstate))
		{
			//Transform from whatever to barnacle
			if (GetActivityType() == 2)
			{
				auto BarnacleEntity = ClientEntityManager()->FindBarnacleForPlayer(playerstate);

				if (BarnacleEntity)
				{
					ApplyBarnacle(BarnacleEntity);
				}
				else
				{
					auto GargantuaEntity = ClientEntityManager()->FindGargantuaForPlayer(playerstate);

					if (GargantuaEntity)
					{
						ApplyGargantua(GargantuaEntity);
					}
				}
			}

			//Transformed from death or barnacle to idle state.
			else if (iOldActivityType > 0 && GetActivityType() == 0)
			{
				ResetPose(playerstate);
			}
		}

		//Teleported
		else if (iOldActivityType == 0 && GetActivityType() == 0 && VectorDistance((*currententity)->curstate.origin, (*currententity)->latched.prevorigin) > 500)
		{
			ResetPose(playerstate);
		}

		return true;
	}

	bool SetupBones(studiohdr_t* studiohdr) override
	{
		if (GetActivityType() == 0)
			return false;

		mstudiobone_t* pbones = (mstudiobone_t*)((byte*)studiohdr + studiohdr->boneindex);

		for (auto pRigidBody : m_rigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			auto pMotionState = (CBulletBaseMotionState*)pRigidBody->getMotionState();

			if (pMotionState->IsBoneBased())
			{
				auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

				const auto &bonematrix = pBoneMotionState->m_bonematrix;

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

		for (auto pRigidBody : m_rigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);
			auto pMotionState = (CBulletBaseMotionState*)pRigidBody->getMotionState();

			if (pMotionState->IsBoneBased())
			{
				auto pBoneMotionState = (CBulletBoneMotionState*)pMotionState;

				if (pSharedUserData->m_flags & PhysicRigidBodyFlag_AlwaysDynamic)
				{
					const auto &bonematrix = pBoneMotionState->m_bonematrix;

					float bonematrix_3x4[3][4];
					TransformToMatrix3x4(bonematrix, bonematrix_3x4);

					memcpy((*pbonetransform)[pSharedUserData->m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
					memcpy((*plighttransform)[pSharedUserData->m_boneindex], bonematrix_3x4, sizeof(bonematrix_3x4));
				}
				else
				{
					Matrix3x4ToTransform((*pbonetransform)[pSharedUserData->m_boneindex], pBoneMotionState->m_bonematrix);
				}
			}
		}

		return true;
	}

	void TransformOwnerEntity(int entindex) override
	{
		m_entindex = entindex;
		m_entity = ClientEntityManager()->GetEntityByIndex(entindex);
	}

	void AddToPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto RigidBody : m_rigidBodies)
		{
			auto pSharedUserData = GetSharedUserDataFromRigidBody(RigidBody);

			dynamicWorld->addRigidBody(RigidBody, pSharedUserData->m_group, pSharedUserData->m_mask);
		}

		for (auto Constraint : m_constraints)
		{
			auto pSharedUserData = GetSharedUserDataFromConstraint(Constraint);

			dynamicWorld->addConstraint(Constraint, pSharedUserData->m_disableCollision);
		}
	}

	void RemoveFromPhysicWorld(void* world) override
	{
		auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

		for (auto Constraint : m_constraints)
		{
			dynamicWorld->removeConstraint(Constraint);
		}

		for (auto RigidBody : m_rigidBodies)
		{
			dynamicWorld->removeRigidBody(RigidBody);
		}
	}

private:

	bool UpdateKinematic(int iNewActivityType, entity_state_t* curstate)
	{
		if (GetActivityType() == iNewActivityType)
			return false;

		//Start death animation
		if (GetActivityType() == 0 && iNewActivityType > 0)
		{
			auto found = std::find_if(m_AnimControlConfigs.begin(), m_AnimControlConfigs.end(), [curstate](const CClientRagdollAnimControlConfig &Config) {

				if (Config.sequence == curstate->sequence)
					return true;

				return false;

			});

			if (found != m_AnimControlConfigs.end())
			{
				if (curstate->frame < found->frame)
				{
					return false;
				}
			}
		}

		m_iActivityType = iNewActivityType;

		for (auto pRigidBody : m_rigidBodies)
		{
			UpdateRigidBodyKinematic(pRigidBody);
		}

		return true;
	}

	void UpdateRigidBodyKinematic(btRigidBody* pRigidBody)
	{
		auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

		bool bKinematic = false;
		
		do
		{
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
			int iCollisionFlags = btCollisionObject::CF_KINEMATIC_OBJECT;

			if (pSharedUserData->m_flags & PhysicRigidBodyFlag_NoDebugDraw)
			{
				iCollisionFlags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
			}

			pRigidBody->setCollisionFlags(iCollisionFlags);
			pRigidBody->setActivationState(DISABLE_DEACTIVATION);
			pRigidBody->setGravity(btVector3(0, 0, 0));
		}
		else
		{
			int iCollisionFlags = btCollisionObject::CF_DYNAMIC_OBJECT;

			if (pSharedUserData->m_flags & PhysicRigidBodyFlag_NoDebugDraw)
			{
				iCollisionFlags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
			}

			pRigidBody->setCollisionFlags(iCollisionFlags);
			pRigidBody->forceActivationState(ACTIVE_TAG);
			pRigidBody->setMassProps(pSharedUserData->m_mass, pSharedUserData->m_inertia);
		}
	}

	void SaveBoneRelativeTransform(const CRagdollObjectCreationParameter &CreationParam)
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

	void SetupNonKeyBones(const CRagdollObjectCreationParameter &CreationParam)
	{
		for (int i = 0; i < CreationParam.m_studiohdr->numbones; ++i)
		{
			if (std::find(m_keyBones.begin(), m_keyBones.end(), i) == m_keyBones.end())
				m_nonKeyBones.emplace_back(i);
		}
	}

	btMotionState *CreateMotionState(const CRagdollObjectCreationParameter &CreationParam, const CClientRigidBodyConfig* pConfig)
	{
		if (pConfig->boneindex >= 0 && pConfig->boneindex < CreationParam.m_studiohdr->numbones)
		{
			btTransform bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[pConfig->boneindex], bonematrix);

			btVector3 vecOrigin(pConfig->origin[0], pConfig->origin[1], pConfig->origin[2]);

			btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

			btVector3 vecAngles(pConfig->angles[0], pConfig->angles[1], pConfig->angles[2]);

			EulerMatrix(vecAngles, localTrans.getBasis());

			const auto& offsetmatrix = localTrans;

			return new CBulletBoneMotionState(this, bonematrix, offsetmatrix);
		}

		return new CBulletEntityMotionState(this);
	}

	btCollisionShape* CreateCollisionShapeInternal(const CRagdollObjectCreationParameter &CreationParam, const CClientCollisionShapeConfig* pConfig)
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

	btCollisionShape* CreateCollisionShape(const CRagdollObjectCreationParameter &CreationParam, const CClientRigidBodyConfig* pConfig)
	{
		if (pConfig->shapes.size() > 1)
		{
			auto pCompoundShape = new btCompoundShape();

			for (const auto &shapeConfigSharedPtr : pConfig->shapes)
			{
				auto pShapeConfig = shapeConfigSharedPtr.get();

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
				//OnBeforeDeleteCollisionShape(compound);

				delete pCompoundShape;

				return nullptr;
			}

			return pCompoundShape;
		}
		else if (pConfig->shapes.size() == 1)
		{
			auto pShapeConfig = pConfig->shapes[0].get();

			return CreateCollisionShapeInternal(CreationParam, pShapeConfig);
		}

		return nullptr;
	}

	btRigidBody* FindRigidBodyByName(const std::string& name)
	{
		for (auto pRigidBody : m_rigidBodies)
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

	btRigidBody *CreateRigidBody(const CRagdollObjectCreationParameter &CreationParam, const CClientRigidBodyConfig* pRigidConfig)
	{
		if (FindRigidBodyByName(pRigidConfig->name))
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create duplicated one \"%s\".\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		if (pRigidConfig->mass <= 0)
		{
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because mass <= 0.\n", pRigidConfig->name.c_str());
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
			gEngfuncs.Con_Printf("CreateRigidBody: cannot create \"%s\" because there is no CollisionShape available.\n", pRigidConfig->name.c_str());
			return nullptr;
		}

		btVector3 shapeInertia;
		pCollisionShape->calculateLocalInertia(pRigidConfig->mass, shapeInertia);

		btRigidBody::btRigidBodyConstructionInfo cInfo(pRigidConfig->mass, pMotionState, pCollisionShape, shapeInertia);
		cInfo.m_friction = pRigidConfig->linearfriction;
		cInfo.m_rollingFriction = pRigidConfig->rollingfriction;
		cInfo.m_restitution = pRigidConfig->restitution;
		cInfo.m_linearSleepingThreshold = 5.0f;
		cInfo.m_angularSleepingThreshold = 3.0f;
		cInfo.m_additionalDamping = true;
		cInfo.m_additionalDampingFactor = 0.5f;
		cInfo.m_additionalLinearDampingThresholdSqr = 1.0f * 1.0f;
		cInfo.m_additionalAngularDampingThresholdSqr = 0.3f * 0.3f;

		auto pRigidBody = new btRigidBody(cInfo);

		pRigidBody->setUserPointer(new CBulletRigidBodySharedUserData(pRigidConfig->name, pRigidConfig->flags, pRigidConfig->boneindex, cInfo));

		pRigidBody->setCcdSweptSphereRadius(pRigidConfig->ccdradius);
		pRigidBody->setCcdMotionThreshold(pRigidConfig->ccdthreshold);

		UpdateRigidBodyKinematic(pRigidBody);

		return pRigidBody;
	}

	btTypedConstraint* CreateConstraintInternal(const CRagdollObjectCreationParameter &CreationParam, const CClientConstraintConfig* pConstraintConfig, btRigidBody *rbA, btRigidBody* rbB, const btTransform & globalJointTransform)
	{
		btTypedConstraint* pConstraint{  };

		switch (pConstraintConfig->type)
		{
		case PhysicConstraint_ConeTwist:
		{
			auto worldTransA = rbA->getWorldTransform();

			auto worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			pConstraint = new btConeTwistConstraint(*rbA, *rbA, localA, localB);
			break;
		}
		case PhysicConstraint_Hinge:
		{
			auto worldTransA = rbA->getWorldTransform();

			auto worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			pConstraint = new btHingeConstraint(*rbA, *rbA, localA, localB);
			break;
		}
		case PhysicConstraint_Point:
		{
			auto worldTransA = rbA->getWorldTransform();

			auto worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			pConstraint = new btPoint2PointConstraint(*rbA, *rbA, localA.getOrigin(), localB.getOrigin());
			break;
		}
		}

		if (!pConstraint)
			return nullptr;

		pConstraint->setUserConstraintPtr(new CBulletConstraintSharedUserData(pConstraintConfig));

		return pConstraint;
	}

	btTypedConstraint *CreateConstraint(const CRagdollObjectCreationParameter &CreationParam, const CClientConstraintConfig* pConstraintConfig)
	{
		auto pRigidBodyA = FindRigidBodyByName(pConstraintConfig->rigidbodyA);

		if (!pRigidBodyA)
		{
			gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint, rigidbodyA \"%s\" not found\n", pConstraintConfig->rigidbodyA.c_str());
			return nullptr;
		}

		auto pRigidBodyB = FindRigidBodyByName(pConstraintConfig->rigidbodyB);

		if (!pRigidBodyB)
		{
			gEngfuncs.Con_Printf("CreateConstraint: Failed to create constraint, rigidbodyB \"%s\" not found\n", pConstraintConfig->rigidbodyB.c_str());
			return nullptr;
		}

		btTransform bonematrix;

		Matrix3x4ToTransform((*pbonetransform)[pConstraintConfig->boneindex], bonematrix);

		btVector3 vecOrigin(pConstraintConfig->origin[0], pConstraintConfig->origin[1], pConstraintConfig->origin[2]);

		btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

		btVector3 vecAngles(pConstraintConfig->angles[0], pConstraintConfig->angles[1], pConstraintConfig->angles[2]);

		EulerMatrix(vecAngles, localTrans.getBasis());

		btTransform globalJointTransform;

		globalJointTransform.mult(bonematrix, localTrans);
		
		return CreateConstraintInternal(CreationParam, pConstraintConfig, pRigidBodyA, pRigidBodyB, globalJointTransform);
	}

	void CreateFloater(const CRagdollObjectCreationParameter &CreationParam, const CClientFloaterConfig* pConfig)
	{
		//TODO
	}

	void CreateRigidBodies(const CRagdollObjectCreationParameter &CreationParam, const CClientPhysicConfig* pPhysConfigs)
	{
		for (const auto& rigidBodyConfig : pPhysConfigs->rigidBodyConfigs)
		{
			auto pRigidBodyConfig = rigidBodyConfig.get();

			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig);

			if (pRigidBody)
			{
				m_rigidBodies.emplace_back(pRigidBody);

				m_keyBones.emplace_back(pRigidBodyConfig->boneindex);

				if (pRigidBodyConfig->name == "Pelvis")
				{
					m_pelvisRigBody = pRigidBody;
				}
				else if (pRigidBodyConfig->name == "Head")
				{
					m_headRigBody = pRigidBody;
				}
			}
		}
	}

	void CreateConstraints(const CRagdollObjectCreationParameter &CreationParam, const CClientPhysicConfig* pConfigs)
	{
		for (const auto& constraintConfig : pConfigs->constraintConfigs)
		{
			auto pConstraintConfig = constraintConfig.get();

			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig);

			if (pConstraint)
			{
				m_constraints.emplace_back(pConstraint);
			}
		}
	}

	void CreateFloaters(const CRagdollObjectCreationParameter &CreationParam, const CClientPhysicConfig* pPhysConfigs)
	{
		for (const auto& floaterConfig : pPhysConfigs->floaterConfigs)
		{
			auto pFloaterConfig = floaterConfig.get();

			CreateFloater(CreationParam, pFloaterConfig);
		}
	}

public:
	int m_entindex{};
	int m_playerindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	int m_iActivityType{};
	int m_iBarnacleIndex{};
	int m_iGargantuaIndex{};
	float m_flUpdateKinematicTime{};
	float m_bUpdateKinematic{};
	std::vector<btRigidBody *> m_rigidBodies;
	std::vector<btTypedConstraint *> m_constraints;
	std::vector<btRigidBody *> m_barnacleDragBodies;
	std::vector<btRigidBody *> m_barnacleChewBodies;
	std::vector<btRigidBody *> m_gargantuaDragBodies;
	std::vector<btTypedConstraint *> m_barnacleConstraints;
	std::vector<btTypedConstraint *> m_gargantuaConstraints;
	btRigidBody* m_pelvisRigBody{};
	btRigidBody* m_headRigBody{};
	std::vector<int> m_keyBones;
	std::vector<int> m_nonKeyBones;
	std::vector<CClientRagdollAnimControlConfig> m_AnimControlConfigs;
	//TODO
	//std::vector<ragdoll_bar_control_t> m_barcontrol;
	//std::vector<ragdoll_gar_control_t> m_garcontrol;
	vec3_t m_vecFirstPersonAngleOffset{};
	float m_flLastOriginChangeTime{};
	float m_flLastCreateTime{};
	btTransform m_BoneRelativeTransform[128]{};
};

ATTRIBUTE_ALIGNED16(class)
CBulletPhysicsDebugDraw : public btIDebugDraw
{
private:
	int m_debugMode{};
	DefaultColors m_ourColors{};

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CBulletPhysicsDebugDraw() : m_debugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits)
	{

	}

	~CBulletPhysicsDebugDraw()
	{

	}

	DefaultColors getDefaultColors() const override
	{
		return m_ourColors;
	}
	///the default implementation for setDefaultColors has no effect. A derived class can implement it and store the colors.
	void setDefaultColors(const DefaultColors& colors) override
	{
		m_ourColors = colors;
	}

	void drawLine(const btVector3& from1, const btVector3& to1, const btVector3& color1) override
	{
		//TODO: use surface api or Renderer API

		glBindTexture(GL_TEXTURE_2D, 0);
		gEngfuncs.pTriAPI->Color4fRendermode(color1.getX(), color1.getY(), color1.getZ(), 1.0f, kRenderTransAlpha);
		gEngfuncs.pTriAPI->Begin(TRI_LINES);

		vec3_t vecFrom = { from1.getX(), from1.getY(), from1.getZ() };
		vec3_t vecTo = { to1.getX(), to1.getY(), to1.getZ() };

		gEngfuncs.pTriAPI->Vertex3fv(vecFrom);
		gEngfuncs.pTriAPI->Vertex3fv(vecTo);
		gEngfuncs.pTriAPI->End();
	}

	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override
	{
		//TODO: Renderer API?

		drawLine(PointOnB, PointOnB + normalOnB * distance, color);
		btVector3 nColor(0, 0, 0);
		drawLine(PointOnB, PointOnB + normalOnB * 0.01, nColor);
	}

	void reportErrorWarning(const char* warningString) override
	{

	}

	void draw3dText(const btVector3& location, const char* textString) override
	{

	}

	void setDebugMode(int debugMode) override
	{
		m_debugMode = debugMode;
	}

	int getDebugMode() const override
	{
		return m_debugMode;
	}
};

void CBulletPhysicManager::Init(void)
{
	CBasePhysicManager::Init();

	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_overlappingPairCache = new btDbvtBroadphase();
	m_solver = new btSequentialImpulseConstraintSolver;
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

	m_debugDraw = new CBulletPhysicsDebugDraw;
	m_dynamicsWorld->setDebugDrawer(m_debugDraw);

	//m_overlapFilterCallback = new GameFilterCallback();
	m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(m_overlapFilterCallback);

	m_dynamicsWorld->setGravity(btVector3(0, 0, 0));
}

void CBulletPhysicManager::Shutdown()
{
	CBasePhysicManager::Shutdown();

	if (m_debugDraw) {
		delete m_debugDraw;
		m_debugDraw = nullptr;
		m_dynamicsWorld->setDebugDrawer(nullptr);
	}

	if (m_dynamicsWorld) {
		delete m_dynamicsWorld;
		m_dynamicsWorld = nullptr;
	}

	if (m_solver) {
		delete m_solver;
		m_solver = nullptr;
	}

	if (m_overlappingPairCache) {
		delete m_overlappingPairCache;
		m_overlappingPairCache = nullptr;
	}

	if (m_dispatcher) {
		delete m_dispatcher;
		m_dispatcher = nullptr;
	}

	if (m_collisionConfiguration) {
		delete m_collisionConfiguration;
		m_collisionConfiguration = nullptr;
	}
}

void CBulletPhysicManager::NewMap(void)
{
	CBasePhysicManager::NewMap();
}

void CBulletPhysicManager::DebugDraw(void)
{
	CBasePhysicManager::DebugDraw();

	gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);

	m_dynamicsWorld->debugDrawWorld();
}

void CBulletPhysicManager::SetGravity(float velocity)
{
	CBasePhysicManager::SetGravity(velocity);

	m_dynamicsWorld->setGravity(btVector3(0, 0, m_gravity));
}

void CBulletPhysicManager::StepSimulation(double frametime)
{
	CBasePhysicManager::StepSimulation(frametime);

	if (frametime <= 0)
		return;

	m_dynamicsWorld->stepSimulation(frametime, 3, 1.0f / GetSimulationTickRate());
}

void CBulletPhysicManager::AddPhysicObjectToWorld(IPhysicObject *PhysicObject)
{
	PhysicObject->AddToPhysicWorld(m_dynamicsWorld);
}

void CBulletPhysicManager::RemovePhysicObjectFromWorld(IPhysicObject* PhysicObject)
{
	PhysicObject->RemoveFromPhysicWorld(m_dynamicsWorld);
}

IStaticObject* CBulletPhysicManager::CreateStaticObject(const CStaticObjectCreationParameter& CreationParam)
{
	return new CBulletStaticObject(CreationParam);
}

IRagdollObject* CBulletPhysicManager::CreateRagdollObject(const CRagdollObjectCreationParameter& CreationParam)
{
	return new CBulletRagdollObject(CreationParam);
}

IClientPhysicManager* BulletPhysicManager_CreateInstance()
{
	return new CBulletPhysicManager;
}