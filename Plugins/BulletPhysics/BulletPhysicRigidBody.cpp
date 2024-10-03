#include "BulletPhysicRigidBody.h"
#include "plugins.h"

CBulletPhysicRigidBody::CBulletPhysicRigidBody(
	int id,
	int entindex,
	IPhysicObject* pPhysicObject,
	const CClientRigidBodyConfig* pRigidConfig,
	const btRigidBody::btRigidBodyConstructionInfo& constructionInfo,
	int group, int mask
)
	:
	CBasePhysicRigidBody(id, entindex, pPhysicObject, pRigidConfig),
	m_density(pRigidConfig->density),
	m_mass(constructionInfo.m_mass),
	m_inertia(constructionInfo.m_localInertia),
	m_group(group),
	m_mask(mask)
{
	m_pInternalRigidBody = new btRigidBody(constructionInfo);

	auto pMotionState = (CBulletBaseMotionState*)m_pInternalRigidBody->getMotionState();

	if (pMotionState)
	{
		pMotionState->SetInternalRigidBody(m_pInternalRigidBody);
	}

	m_pInternalRigidBody->setUserIndex(id);
	m_pInternalRigidBody->setUserPointer(this);

	float ccdRadius = pRigidConfig->ccdRadius;
	float ccdThreshold = pRigidConfig->ccdThreshold;

	FloatGoldSrcToBullet(&ccdRadius);
	FloatGoldSrcToBullet(&ccdThreshold);

	m_pInternalRigidBody->setCcdSweptSphereRadius(ccdRadius);
	m_pInternalRigidBody->setCcdMotionThreshold(ccdThreshold);
}

CBulletPhysicRigidBody::~CBulletPhysicRigidBody()
{
	if (m_addedToPhysicWorld)
	{
		Sys_Error("CBulletPhysicRigidBody cannot be deleted before being removed from world!");
		return;
	}

	if (m_pInternalRigidBody)
	{
		int numRefs = m_pInternalRigidBody->getNumConstraintRefs();

		for (int i = 0; i < numRefs; ++i)
		{
			auto pInternalContraint = m_pInternalRigidBody->getConstraintRef(i);

			auto pConstraint = (IPhysicConstraint *)pInternalContraint->getUserConstraintPtr();

			if (pConstraint)
			{
				ClientPhysicManager()->RemovePhysicComponent(pConstraint->GetPhysicComponentId());
			}
		}

		auto pCollisionShape = m_pInternalRigidBody->getCollisionShape();

		if (pCollisionShape)
		{
			OnBeforeDeleteBulletCollisionShape(pCollisionShape);

			delete pCollisionShape;

			m_pInternalRigidBody->setCollisionShape(nullptr);
		}

		auto pMotionState = m_pInternalRigidBody->getMotionState();

		if (pMotionState)
		{
			delete pMotionState;

			m_pInternalRigidBody->setMotionState(nullptr);
		}

		delete m_pInternalRigidBody;

		m_pInternalRigidBody = nullptr;
	}
}

bool CBulletPhysicRigidBody::AddToPhysicWorld(void* world)
{
	auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

	if (!m_pInternalRigidBody)
	{
		gEngfuncs.Con_Printf("CBulletPhysicRigidBody::AddToPhysicWorld: empty m_pInternalRigidBody!\n");
		return false;
	}

	if (!m_addedToPhysicWorld)
	{
		dynamicWorld->addRigidBody(m_pInternalRigidBody, m_group, m_mask);

		m_addedToPhysicWorld = true;

		return true;
	}

	gEngfuncs.Con_Printf("CBulletPhysicRigidBody::AddToPhysicWorld: already added to world!\n");
	return false;
}

bool CBulletPhysicRigidBody::RemoveFromPhysicWorld(void* world)
{
	auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

	if (!m_pInternalRigidBody)
	{
		gEngfuncs.Con_Printf("CBulletPhysicRigidBody::RemoveFromPhysicWorld: empty m_pInternalRigidBody!\n");
		return false;
	}

	if (m_addedToPhysicWorld)
	{
		int numRefs = m_pInternalRigidBody->getNumConstraintRefs();

		for (int i = 0; i < numRefs; ++i)
		{
			auto pInternalContraint = m_pInternalRigidBody->getConstraintRef(i);

			auto pConstraint = (IPhysicConstraint*)pInternalContraint->getUserConstraintPtr();

			if (pConstraint)
			{
				ClientPhysicManager()->RemovePhysicComponentFromWorld(pConstraint);
			}
		}

		dynamicWorld->removeRigidBody(m_pInternalRigidBody);

		m_addedToPhysicWorld = false;

		return true;
	}

	gEngfuncs.Con_DPrintf("CBulletPhysicRigidBody::RemoveFromPhysicWorld: already removed from world!\n");
	return false;
}

bool CBulletPhysicRigidBody::IsAddedToPhysicWorld(void* world) const
{
	return m_addedToPhysicWorld;
}

void CBulletPhysicRigidBody::ApplyCentralForce(const vec3_t vecForce)
{
	if (m_pInternalRigidBody)
	{
		btVector3 vec3BtForce(vecForce[0], vecForce[1], vecForce[2]);

		m_pInternalRigidBody->applyCentralForce(vec3BtForce);
	}
}

void CBulletPhysicRigidBody::ApplyCentralImpulse(const vec3_t vecImpulse)
{
	if (m_pInternalRigidBody)
	{
		btVector3 vec3BtImpulse(vecImpulse[0], vecImpulse[1], vecImpulse[2]);

		m_pInternalRigidBody->applyCentralImpulse(vec3BtImpulse);
	}
}

void CBulletPhysicRigidBody::SetLinearVelocity(const vec3_t vecVelocity)
{
	if (m_pInternalRigidBody)
	{
		btVector3 vec3BtVelocity(vecVelocity[0], vecVelocity[1], vecVelocity[2]);

		m_pInternalRigidBody->setLinearVelocity(vec3BtVelocity);
	}
}

void CBulletPhysicRigidBody::SetAngularVelocity(const vec3_t vecVelocity)
{
	if (m_pInternalRigidBody)
	{
		btVector3 vec3BtVelocity(vecVelocity[0], vecVelocity[1], vecVelocity[2]);

		m_pInternalRigidBody->setAngularVelocity(vec3BtVelocity);
	}
}

bool CBulletPhysicRigidBody::ResetPose(studiohdr_t* studiohdr, entity_state_t* curstate)
{
	return false;
}

bool CBulletPhysicRigidBody::SetupBones(studiohdr_t* studiohdr, int flags)
{
	return false;
}

bool CBulletPhysicRigidBody::SetupJiggleBones(studiohdr_t* studiohdr, int flags)
{
	return false;
}

void* CBulletPhysicRigidBody::GetInternalRigidBody()
{
	return m_pInternalRigidBody;
}

bool CBulletPhysicRigidBody::GetGoldSrcOriginAngles(float* origin, float* angles)
{
	auto ent = GetOwnerPhysicObject()->GetClientEntity();

	const auto& worldTrans = m_pInternalRigidBody->getWorldTransform();

	if (origin)
	{
		const auto& vecOrigin = worldTrans.getOrigin();

		origin[0] = vecOrigin.getX();
		origin[1] = vecOrigin.getY();
		origin[2] = vecOrigin.getZ();

		Vec3BulletToGoldSrc(origin);
	}

	if (angles)
	{
		btVector3 vecAngles;
		MatrixEuler(worldTrans.getBasis(), vecAngles);

		if (ent->curstate.solid == SOLID_BSP)
		{
			vecAngles.setX(-vecAngles.x());
		}

		angles[0] = vecAngles.getX();
		angles[1] = vecAngles.getY();
		angles[2] = vecAngles.getZ();

		//Clamp to -3600~3600
		for (int i = 0; i < 3; i++)
		{
			if (angles[i] < -3600.0f || angles[i] > 3600.0f)
				angles[i] = fmod(angles[i], 3600.0f);
		}
	}

	return true;
}

bool CBulletPhysicRigidBody::GetGoldSrcOriginAnglesWithLocalOffset(const vec3_t localoffset_origin, const vec3_t localoffset_angles, float* origin, float* angles)
{
	auto ent = GetOwnerPhysicObject()->GetClientEntity();

	const auto& worldTrans = m_pInternalRigidBody->getWorldTransform();

	auto localOrigin = GetVector3FromVec3(localoffset_origin);
	auto localAngles = GetVector3FromVec3(localoffset_angles);

	Vector3GoldSrcToBullet(localOrigin);

	btTransform localTrans;
	localTrans.setIdentity();
	localTrans.setOrigin(localOrigin);
	EulerMatrix(localAngles, localTrans.getBasis());

	btTransform worldTransNew;
	worldTransNew.mult(worldTrans, localTrans);

	if (origin)
	{
		const auto& vecNewOrigin = worldTransNew.getOrigin();

		origin[0] = vecNewOrigin.getX();
		origin[1] = vecNewOrigin.getY();
		origin[2] = vecNewOrigin.getZ();

		Vec3BulletToGoldSrc(origin);
	}

	if (angles)
	{
		btVector3 vecNewAngles;
		MatrixEuler(worldTransNew.getBasis(), vecNewAngles);

		if (ent->curstate.solid == SOLID_BSP)
		{
			vecNewAngles.setX(-vecNewAngles.x());
		}

		angles[0] = vecNewAngles.getX();
		angles[1] = vecNewAngles.getY();
		angles[2] = vecNewAngles.getZ();

		//Clamp to -3600~3600
		for (int i = 0; i < 3; i++)
		{
			if (angles[i] < -3600.0f || angles[i] > 3600.0f)
				angles[i] = fmod(angles[i], 3600.0f);
		}
	}
	return true;
}

bool CBulletPhysicRigidBody::SetGoldSrcOriginAngles(const float* origin, const float* angles)
{
	btVector3 vecOrigin = GetVector3FromVec3(origin);

	Vector3GoldSrcToBullet(vecOrigin);

	btTransform worldTrans;

	worldTrans.setIdentity();

	worldTrans.setOrigin(vecOrigin);

	btVector3 vecAngles = GetVector3FromVec3(angles);

	EulerMatrix(vecAngles, worldTrans.getBasis());

	m_pInternalRigidBody->setWorldTransform(worldTrans);
	m_pInternalRigidBody->setInterpolationWorldTransform(worldTrans);
	m_pInternalRigidBody->getMotionState()->setWorldTransform(worldTrans);

	return true;
}

float CBulletPhysicRigidBody::GetMass() const
{
	return m_mass;
}

bool CBulletPhysicRigidBody::GetAABB(vec3_t mins, vec3_t maxs) const
{
	if (m_pInternalRigidBody)
	{
		btVector3 bmins, bmaxs;
		m_pInternalRigidBody->getAabb(bmins, bmaxs);

		Vector3BulletToGoldSrc(bmins);
		Vector3BulletToGoldSrc(bmaxs);

		GetVec3FromVector3(bmins, mins);
		GetVec3FromVector3(bmaxs, maxs);

		return true;
	}

	return false;
}
