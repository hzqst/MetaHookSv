#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "ClientEntityManager.h"
#include "BulletPhysicManager.h"

#include <glew.h>

btQuaternion FromToRotaion(btVector3 fromDirection, btVector3 toDirection)
{
	fromDirection = fromDirection.normalize();
	toDirection = toDirection.normalize();

	float cosTheta = fromDirection.dot(toDirection);

	if (cosTheta < -1 + 0.001f) //(Math.Abs(cosTheta)-Math.Abs( -1.0)<1E-6)
	{
		btVector3 up(0.0f, 0.0f, 1.0f);

		auto rotationAxis = up.cross(fromDirection);
		if (rotationAxis.length() < 0.01) // bad luck, they were parallel, try again!
		{
			rotationAxis = up.cross(fromDirection);
		}
		rotationAxis = rotationAxis.normalize();
		return btQuaternion(rotationAxis, (float)M_PI);
	}
	else
	{
		// Implementation from Stan Melax's Game Programming Gems 1 article
		auto rotationAxis = fromDirection.cross(toDirection);

		float s = (float)sqrt((1 + cosTheta) * 2);
		float invs = 1 / s;

		return btQuaternion(
			rotationAxis.x() * invs,
			rotationAxis.y() * invs,
			rotationAxis.z() * invs,
			s * 0.5f
		);
	}
}

btTransform MatrixLookAt(const btTransform& transform, const btVector3& at, const btVector3& forward)
{
	auto originVector = forward;
	auto worldToLocalTransform = transform.inverse();

	//transform the target in world position to object's local position
	auto targetVector = worldToLocalTransform * at;

	auto rot = FromToRotaion(originVector, targetVector);
	btTransform rotMatrix = btTransform(rot);

	return transform * rotMatrix;
}

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

	int GetObjectFlags() const override
	{
		return PhysicObjectFlag_StaticObject;
	}

	bool Update() override
	{
		if (m_pRigidBody)
		{
			UpdateRigidBodyKinematic(m_pRigidBody);
		}

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

		m_pRigidBody->setUserPointer(new CBulletRigidBodySharedUserData(cInfo, m_model->name, 0, -1, CreationParam.m_debugDrawLevel, 1));

		UpdateRigidBodyKinematic(m_pRigidBody);
	}

	void UpdateRigidBodyKinematic(btRigidBody* pRigidBody)
	{
		auto ent = GetClientEntity();

		auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

		bool bKinematic = false;

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

			if (iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT)
				return;

			iCollisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
			pRigidBody->setActivationState(DISABLE_DEACTIVATION);

			pRigidBody->setCollisionFlags(iCollisionFlags);
		}
		else
		{
			int iCollisionFlags = pRigidBody->getCollisionFlags();

			if (!(iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT))
				return;

			iCollisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
			pRigidBody->setActivationState(ACTIVE_TAG);

			pRigidBody->setCollisionFlags(iCollisionFlags);
		}
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

		m_IdleAnimConfig = CreationParam.m_pRagdollConfig->IdleAnimConfig;
		m_AnimControlConfigs = CreationParam.m_pRagdollConfig->AnimControlConfigs;

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

	int GetObjectFlags() const override
	{
		return PhysicObjectFlag_RagdollObject;
	}

	void ResetPose(entity_state_t* curstate) override
	{
		//TODO

		ClientPhysicManager()->SetupIdleBonesForRagdoll(GetClientEntity(), GetModel(), GetEntityIndex(), GetPlayerIndex(), m_IdleAnimConfig);
		
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
		auto playerState = R_GetPlayerState(m_playerindex);

		int iOldActivityType = GetActivityType();

		int iNewActivityType = StudioGetSequenceActivityType(m_model, playerState);

		if (iNewActivityType == 0)
		{
			iNewActivityType = GetOverrideActivityType(playerState);
		}

		if (m_playerindex == m_entindex)
		{
			if (iNewActivityType == 1)
			{
				ClientEntityManager()->SetPlayerDeathState(m_playerindex, playerState, m_model);
			}
			else
			{
				ClientEntityManager()->ClearPlayerDeathState(m_playerindex);
			}
		}

		if (UpdateKinematic(iNewActivityType, playerState))
		{
			//Transform from whatever to barnacle
			if (GetActivityType() == 2)
			{
				auto BarnacleEntity = ClientEntityManager()->FindBarnacleForPlayer(playerState);

				if (BarnacleEntity)
				{
					ApplyBarnacle(BarnacleEntity);
				}
				else
				{
					auto GargantuaEntity = ClientEntityManager()->FindGargantuaForPlayer(playerState);

					if (GargantuaEntity)
					{
						ApplyGargantua(GargantuaEntity);
					}
				}
			}

			//Transformed from death or barnacle to idle state.
			else if (iOldActivityType > 0 && GetActivityType() == 0)
			{
				ResetPose(playerState);
			}
		}

		//Teleported
		else if (
			iOldActivityType == 0 && GetActivityType() == 0 && 			
			VectorDistance(GetClientEntity()->curstate.origin, GetClientEntity()->latched.prevorigin) > 500)
		{
			ResetPose(playerState);
		}

		return true;
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

		for (auto pRigidBody : m_RigidBodies)
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

		for (auto pRigidBody : m_RigidBodies)
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
			int iCollisionFlags = pRigidBody->getCollisionFlags();

			if (iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT)
				return;

			iCollisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;

			pRigidBody->setCollisionFlags(iCollisionFlags);
			pRigidBody->setActivationState(DISABLE_DEACTIVATION);
			pRigidBody->setGravity(btVector3(0, 0, 0));
		}
		else
		{
			int iCollisionFlags = pRigidBody->getCollisionFlags();

			if (!(iCollisionFlags & btCollisionObject::CF_KINEMATIC_OBJECT))
				return;

			iCollisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;

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
		if (pConfig->isLegacyConfig)
		{
			btTransform bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[pConfig->boneindex], bonematrix);

			if (!(pConfig->pboneindex >= 0 && pConfig->pboneindex < CreationParam.m_studiohdr->numbones))
			{
				gEngfuncs.Con_Printf("CreateMotionState: invalid pConfig->pboneindex (%d).\n", pConfig->pboneindex);
				return nullptr;
			}

			btVector3 boneorigin = bonematrix.getOrigin();

			btVector3 pboneorigin((*pbonetransform)[pConfig->pboneindex][0][3], (*pbonetransform)[pConfig->pboneindex][1][3], (*pbonetransform)[pConfig->pboneindex][2][3]);

			btVector3 vecDirection = pboneorigin - boneorigin;
			vecDirection = vecDirection.normalize();

			btVector3 vecOriginWorldSpace = boneorigin + vecDirection * pConfig->pboneoffset;

			btTransform bonematrix2 = bonematrix;
			bonematrix2.setOrigin(vecOriginWorldSpace);

			btVector3 fwd(0, 1, 0);
			auto rigidTransformWorldSpace = MatrixLookAt(bonematrix2, pboneorigin, fwd);

			btTransform offsetmatrix;

			offsetmatrix.mult(bonematrix.inverse(), rigidTransformWorldSpace);

			return new CBulletBoneMotionState(this, bonematrix, offsetmatrix);
		}

		if (pConfig->boneindex >= 0 && pConfig->boneindex < CreationParam.m_studiohdr->numbones)
		{
			btTransform bonematrix;

			Matrix3x4ToTransform((*pbonetransform)[pConfig->boneindex], bonematrix);

			//Legacy Path
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
				//OnBeforeDeleteCollisionShape(compound);

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
		cInfo.m_friction = pRigidConfig->linearFriction;
		cInfo.m_rollingFriction = pRigidConfig->rollingFriction;
		cInfo.m_restitution = pRigidConfig->restitution;
		cInfo.m_linearSleepingThreshold = 5.0f;
		cInfo.m_angularSleepingThreshold = 3.0f;
		cInfo.m_additionalDamping = true;
		cInfo.m_additionalDampingFactor = 0.5f;
		cInfo.m_additionalLinearDampingThresholdSqr = 1.0f * 1.0f;
		cInfo.m_additionalAngularDampingThresholdSqr = 0.3f * 0.3f;

		auto pRigidBody = new btRigidBody(cInfo);

		pRigidBody->setUserPointer(new CBulletRigidBodySharedUserData(cInfo, pRigidConfig->name, pRigidConfig->flags, pRigidConfig->boneindex, pRigidConfig->debugDrawLevel, pRigidConfig->density));

		pRigidBody->setCcdSweptSphereRadius(pRigidConfig->ccdRadius);
		pRigidBody->setCcdMotionThreshold(pRigidConfig->ccdThreshold);

		UpdateRigidBodyKinematic(pRigidBody);

		return pRigidBody;
	}

	btTypedConstraint* CreateConstraintInternal(const CRagdollObjectCreationParameter &CreationParam, const CClientConstraintConfig* pConstraintConfig, btRigidBody *rbA, btRigidBody* rbB, const btTransform & globalJointTransform)
	{
		btTypedConstraint* pConstraint{ };

		switch (pConstraintConfig->type)
		{
		case PhysicConstraint_ConeTwist:
		{
			const auto& worldTransA = rbA->getWorldTransform();

			const auto& worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			pConstraint = new btConeTwistConstraint(*rbA, *rbB, localA, localB);
			break;
		}
		case PhysicConstraint_Hinge:
		{
			const auto& worldTransA = rbA->getWorldTransform();

			const auto& worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			pConstraint = new btHingeConstraint(*rbA, *rbB, localA, localB);
			break;
		}
		case PhysicConstraint_Point:
		{
			const auto& worldTransA = rbA->getWorldTransform();

			const auto& worldTransB = rbB->getWorldTransform();

			btTransform localA;
			localA.mult(worldTransA.inverse(), globalJointTransform);

			btTransform localB;
			localB.mult(worldTransB.inverse(), globalJointTransform);

			pConstraint = new btPoint2PointConstraint(*rbA, *rbB, localA.getOrigin(), localB.getOrigin());
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

	void CreateFloater(const CRagdollObjectCreationParameter &CreationParam, const CClientFloaterConfig* pConfig)
	{
		//TODO
	}

	void CreateRigidBodies(const CRagdollObjectCreationParameter &CreationParam)
	{
		for (auto pRigidBodyConfig : CreationParam.m_pRagdollConfig->RigidBodyConfigs)
		{
			auto pRigidBody = CreateRigidBody(CreationParam, pRigidBodyConfig);

			if (pRigidBody)
			{
				m_RigidBodies.emplace_back(pRigidBody);

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

	void CreateConstraints(const CRagdollObjectCreationParameter &CreationParam)
	{
		for (auto pConstraintConfig : CreationParam.m_pRagdollConfig->ConstraintConfigs)
		{
			auto pConstraint = CreateConstraint(CreationParam, pConstraintConfig);

			if (pConstraint)
			{
				m_Constraints.emplace_back(pConstraint);
			}
		}
	}

	void CreateFloaters(const CRagdollObjectCreationParameter &CreationParam)
	{
		for (auto pFloaterConfig : CreationParam.m_pRagdollConfig->FloaterConfigs)
		{
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
	//float m_flUpdateKinematicTime{};
	//float m_bUpdateKinematic{};
	std::vector<btRigidBody *> m_RigidBodies;
	std::vector<btTypedConstraint *> m_Constraints;
	std::vector<btRigidBody *> m_BarnacleDragBodies;
	std::vector<btRigidBody *> m_BarnacleChewBodies;
	std::vector<btRigidBody *> m_GargantuaDragBodies;
	std::vector<btTypedConstraint *> m_BarnacleConstraints;
	std::vector<btTypedConstraint *> m_GargantuaConstraints;
	btRigidBody* m_pelvisRigBody{};
	btRigidBody* m_headRigBody{};
	std::vector<int> m_keyBones;
	std::vector<int> m_nonKeyBones;
	CClientRagdollAnimControlConfig m_IdleAnimConfig;
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

	const auto &objectArray = m_dynamicsWorld->getCollisionObjectArray();
	for (size_t i = 0;i < objectArray.size(); ++i)
	{
		auto pCollisionObject = objectArray[i];

		auto pRigidBody = btRigidBody::upcast(pCollisionObject);
		
		if (pRigidBody)
		{
			auto pPhysicObject = GetPhysicObjectFromRigidBody(pRigidBody);
			auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

			if (pPhysicObject->IsRagdollObject())
			{
				if (GetRagdollObjectDebugDrawLevel() >= pSharedUserData->m_debugDrawLevel)
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags &= ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pRigidBody->setCollisionFlags(iCollisionFlags);
				}
				else
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pCollisionObject->setCollisionFlags(iCollisionFlags);
				}
			}
			else if (pPhysicObject->IsStaticObject())
			{
				if (GetStaticObjectDebugDrawLevel() >= pSharedUserData->m_debugDrawLevel)
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags &= ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pCollisionObject->setCollisionFlags(iCollisionFlags);
				}
				else
				{
					int iCollisionFlags = pCollisionObject->getCollisionFlags();
					iCollisionFlags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
					pCollisionObject->setCollisionFlags(iCollisionFlags);
				}
			}
		}
	}

	auto numConstraint = m_dynamicsWorld->getNumConstraints();

	for (int i = 0; i < numConstraint; ++i)
	{
		auto pConstraint = m_dynamicsWorld->getConstraint(i);

		auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

		if (pSharedUserData)
		{
			if (GetConstraintDebugDrawLevel() >= pSharedUserData->m_debugDrawLevel)
			{
				pConstraint->setDbgDrawSize(1);
			}
			else
			{
				pConstraint->setDbgDrawSize(0);
			}
		}
	}

	if (IsDebugDrawShowCCD())
	{
		int iDebugMode = m_debugDraw->getDebugMode();
		iDebugMode |= btIDebugDraw::DBG_EnableCCD;
		m_debugDraw->setDebugMode(iDebugMode);
	}
	else
	{
		int iDebugMode = m_debugDraw->getDebugMode();
		iDebugMode &= ~btIDebugDraw::DBG_EnableCCD;
		m_debugDraw->setDebugMode(iDebugMode);
	}

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