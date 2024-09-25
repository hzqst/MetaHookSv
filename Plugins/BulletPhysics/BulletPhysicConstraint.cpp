#include "BulletPhysicConstraint.h"
#include "plugins.h"

CBulletPhysicConstraint::CBulletPhysicConstraint(
	int id,
	int entindex,
	IPhysicObject* pPhysicObject,
	CClientConstraintConfig* pConstraintConfig,
	btTypedConstraint* pInternalConstraint) :

	CBasePhysicConstraint(id, entindex, pPhysicObject, pConstraintConfig),
	m_maxTolerantLinearError(pConstraintConfig->maxTolerantLinearError),
	m_disableCollision(pConstraintConfig->disableCollision),
	m_pInternalConstraint(pInternalConstraint)
{
	m_pInternalConstraint = pInternalConstraint;
	m_pInternalConstraint->setUserConstraintId(id);
	m_pInternalConstraint->setUserConstraintPtr(this);

	float drawSize = 3;
	FloatGoldSrcToBullet(&drawSize);
	m_pInternalConstraint->setDbgDrawSize(drawSize);

	m_pInternalRigidBodyA = CreateInternalRigidBody(false);
	m_pInternalRigidBodyB = CreateInternalRigidBody(true);

	m_rigidBodyAPhysicComponentId = pInternalConstraint->getRigidBodyA().getUserIndex();
	m_rigidBodyBPhysicComponentId = pInternalConstraint->getRigidBodyB().getUserIndex();
}

CBulletPhysicConstraint::~CBulletPhysicConstraint()
{
	if (m_addedToPhysicWorld)
	{
		Sys_Error("CBulletConstraint cannot be deleted before being removed from world!");
		return;
	}

	if (m_pInternalConstraint)
	{
		delete m_pInternalConstraint;
		m_pInternalConstraint = nullptr;
	}

	if (m_pInternalRigidBodyA)
	{
		FreeInternalRigidBody(m_pInternalRigidBodyA);
		m_pInternalRigidBodyA = nullptr;
	}

	if (m_pInternalRigidBodyB)
	{
		FreeInternalRigidBody(m_pInternalRigidBodyB);
		m_pInternalRigidBodyB = nullptr;
	}
}

const char* CBulletPhysicConstraint::GetTypeString() const
{
	if (m_pInternalConstraint)
	{
		switch (m_pInternalConstraint->getConstraintType())
		{
		case POINT2POINT_CONSTRAINT_TYPE:
		{
			return "PointConstraint";
		}
		case HINGE_CONSTRAINT_TYPE:
		{
			return "HingeConstraint";
		}
		case CONETWIST_CONSTRAINT_TYPE:
		{
			return "ConeTwistConstraint";
		}
		case D6_CONSTRAINT_TYPE:
		{
			return "Dof6Constraint";
		}
		case SLIDER_CONSTRAINT_TYPE:
		{
			return "SliderConstraint";
		}
		case D6_SPRING_CONSTRAINT_TYPE:
		case D6_SPRING_2_CONSTRAINT_TYPE:
		{
			return "Dof6SpringConstraint";
		}
		case FIXED_CONSTRAINT_TYPE:
		{
			return "FixedConstraint";
		}
		}
	}

	return "Constraint";
}

const char* CBulletPhysicConstraint::GetTypeLocalizationTokenString() const
{
	if (m_pInternalConstraint)
	{
		switch (m_pInternalConstraint->getConstraintType())
		{
		case POINT2POINT_CONSTRAINT_TYPE:
		{
			return "#BulletPhysics_PointConstraint";
		}
		case HINGE_CONSTRAINT_TYPE:
		{
			return "#BulletPhysics_HingeConstraint";
		}
		case CONETWIST_CONSTRAINT_TYPE:
		{
			return "#BulletPhysics_ConeTwistConstraint";
		}
		case D6_CONSTRAINT_TYPE:
		{
			return "#BulletPhysics_Dof6Constraint";
		}
		case SLIDER_CONSTRAINT_TYPE:
		{
			return "#BulletPhysics_SliderConstraint";
		}
		case D6_SPRING_CONSTRAINT_TYPE:
		case D6_SPRING_2_CONSTRAINT_TYPE:
		{
			return "#BulletPhysics_Dof6SpringConstraint";
		}
		case FIXED_CONSTRAINT_TYPE:
		{
			return "#BulletPhysics_FixedConstraint";
		}
		}
	}

	return "#BulletPhysics_Constraint";
}

bool CBulletPhysicConstraint::AddToPhysicWorld(void* world)
{
	auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

	if (!m_pInternalConstraint)
	{
		gEngfuncs.Con_Printf("CBulletPhysicConstraint::AddToPhysicWorld: empty m_pInternalConstraint!\n");
		return false;
	}

	if (!m_addedToPhysicWorld)
	{
		//Check if RigidBodyA and RigidBodyB is in world.

		auto pRigidBodyA = ClientPhysicManager()->GetPhysicComponent(m_rigidBodyAPhysicComponentId);

		if (!pRigidBodyA)
		{
			gEngfuncs.Con_Printf("CBulletPhysicConstraint::AddToPhysicWorld: pRigidBodyA not present !\n");
			return false;
		}

		if (!pRigidBodyA->IsAddedToPhysicWorld(world))
		{
			gEngfuncs.Con_Printf("CBulletPhysicConstraint::AddToPhysicWorld: pRigidBodyA not added to world !\n");
			return false;
		}

		auto pRigidBodyB = ClientPhysicManager()->GetPhysicComponent(m_rigidBodyBPhysicComponentId);

		if (!pRigidBodyB)
		{
			gEngfuncs.Con_Printf("CBulletPhysicConstraint::AddToPhysicWorld: pRigidBodyB not present !\n");
			return false;
		}

		if (!pRigidBodyB->IsAddedToPhysicWorld(world))
		{
			gEngfuncs.Con_Printf("CBulletPhysicConstraint::AddToPhysicWorld: pRigidBodyB not added to world !\n");
			return false;
		}

		dynamicWorld->addConstraint(m_pInternalConstraint, m_disableCollision);

		if (m_pInternalRigidBodyA)
		{
			dynamicWorld->addRigidBody(m_pInternalRigidBodyA, BulletPhysicCollisionFilterGroups::ConstraintFilter, BulletPhysicCollisionFilterGroups::InspectorFilter);
		}

		if (m_pInternalRigidBodyB)
		{
			dynamicWorld->addRigidBody(m_pInternalRigidBodyB, BulletPhysicCollisionFilterGroups::ConstraintFilter, BulletPhysicCollisionFilterGroups::InspectorFilter);
		}

		m_addedToPhysicWorld = true;

		return true;
	}

	gEngfuncs.Con_Printf("CBulletPhysicConstraint::AddToPhysicWorld: already added to world!\n");
	return false;
}

bool CBulletPhysicConstraint::RemoveFromPhysicWorld(void* world)
{
	auto dynamicWorld = (btDiscreteDynamicsWorld*)world;

	if (!m_pInternalConstraint)
	{
		gEngfuncs.Con_Printf("CBulletPhysicConstraint::RemoveFromPhysicWorld: empty m_pInternalConstraint!\n");
		return false;
	}

	if (m_addedToPhysicWorld)
	{
		dynamicWorld->removeConstraint(m_pInternalConstraint);

		if (m_pInternalRigidBodyA)
		{
			dynamicWorld->removeRigidBody(m_pInternalRigidBodyA);
		}

		if (m_pInternalRigidBodyB)
		{
			dynamicWorld->removeRigidBody(m_pInternalRigidBodyB);
		}

		m_addedToPhysicWorld = false;

		return true;
	}

	gEngfuncs.Con_Printf("CBulletPhysicConstraint::RemoveFromPhysicWorld: already removed from world!\n");
	return false;
}

bool CBulletPhysicConstraint::IsAddedToPhysicWorld(void* world) const
{
	return m_addedToPhysicWorld;
}

void CBulletPhysicConstraint::Update(CPhysicComponentUpdateContext* ComponentUpdateContext)
{

}

bool CBulletPhysicConstraint::ExtendLinearLimit(int axis, float value)
{
	if (!m_pInternalConstraint)
		return false;

	FloatGoldSrcToBullet(&value);

	if (m_pInternalConstraint->getConstraintType() == D6_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btGeneric6DofConstraint*)m_pInternalConstraint;

		if (axis == -1)
		{
			btVector3 currentLimit;
			pDof6->getLinearLowerLimit(currentLimit);

			if (value > 0)
			{
				if (currentLimit.x() < -1) {
					currentLimit.setX(currentLimit.x() - value);
				}
				else if (currentLimit.x() > 1) {
					currentLimit.setX(currentLimit.x() + value);
				}
				else if (currentLimit.y() < -1) {
					currentLimit.setY(currentLimit.y() - value);
				}
				else if (currentLimit.y() > 1) {
					currentLimit.setY(currentLimit.y() + value);
				}
				else if (currentLimit.z() < -1) {
					currentLimit.setZ(currentLimit.z() - value);
				}
				else if (currentLimit.z() > 1) {
					currentLimit.setZ(currentLimit.z() + value);
				}
			}
			else if (value < 0)
			{
				if (currentLimit.x() < -1) {
					currentLimit.setX(min(currentLimit.x() - value, 0));
				}
				else if (currentLimit.x() > 1) {
					currentLimit.setX(max(currentLimit.x() + value, 0));
				}
				else if (currentLimit.y() < -1) {
					currentLimit.setY(min(currentLimit.y() - value, 0));
				}
				else if (currentLimit.y() > 1) {
					currentLimit.setY(max(currentLimit.y() + value, 0));
				}
				else if (currentLimit.z() < -1) {
					currentLimit.setZ(min(currentLimit.z() - value, 0));
				}
				else if (currentLimit.z() > 1) {
					currentLimit.setZ(max(currentLimit.z() + value, 0));
				}
			}

			pDof6->setLinearLowerLimit(currentLimit);
		}
		if (axis == -1)
		{
			btVector3 currentLimit;
			pDof6->getLinearUpperLimit(currentLimit);

			if (value > 0)
			{
				if (currentLimit.x() < -1) {
					currentLimit.setX(currentLimit.x() - value);
				}
				else if (currentLimit.x() > 1) {
					currentLimit.setX(currentLimit.x() + value);
				}
				else if (currentLimit.y() < -1) {
					currentLimit.setY(currentLimit.y() - value);
				}
				else if (currentLimit.y() > 1) {
					currentLimit.setY(currentLimit.y() + value);
				}
				else if (currentLimit.z() < -1) {
					currentLimit.setZ(currentLimit.z() - value);
				}
				else if (currentLimit.z() > 1) {
					currentLimit.setZ(currentLimit.z() + value);
				}
			}
			else if (value < 0)
			{
				if (currentLimit.x() < -1) {
					currentLimit.setX(min(currentLimit.x() - value, 0));
				}
				else if (currentLimit.x() > 1) {
					currentLimit.setX(max(currentLimit.x() + value, 0));
				}
				else if (currentLimit.y() < -1) {
					currentLimit.setY(min(currentLimit.y() - value, 0));
				}
				else if (currentLimit.y() > 1) {
					currentLimit.setY(max(currentLimit.y() + value, 0));
				}
				else if (currentLimit.z() < -1) {
					currentLimit.setZ(min(currentLimit.z() - value, 0));
				}
				else if (currentLimit.z() > 1) {
					currentLimit.setZ(max(currentLimit.z() + value, 0));
				}
			}

			pDof6->setLinearUpperLimit(currentLimit);
		}

		if (axis >= 0 && axis <= 2)
		{
			btVector3 currentLimit;
			pDof6->getLinearLowerLimit(currentLimit);
			currentLimit[axis] += value;
			pDof6->setLinearLowerLimit(currentLimit);
		}

		if (axis >= 3 && axis <= 5)
		{
			btVector3 currentLimit;
			pDof6->getLinearUpperLimit(currentLimit);
			currentLimit[axis - 3] += value;
			pDof6->setLinearUpperLimit(currentLimit);
		}

		return true;
	}
	else if (m_pInternalConstraint->getConstraintType() == SLIDER_CONSTRAINT_TYPE)
	{
		auto pSlider = (btSliderConstraint*)m_pInternalConstraint;

		if (axis == -1)
		{
			auto currentLimit = pSlider->getLowerLinLimit();

			if (value > 0)
			{
				if (currentLimit < -1) {
					currentLimit = currentLimit - value;
				}
				else if (currentLimit > 1) {
					currentLimit = currentLimit + value;
				}
			}
			else if (value < 0)
			{
				if (currentLimit < -1) {
					currentLimit = min(currentLimit - value, 0);
				}
				else if (currentLimit > 1) {
					currentLimit = max(currentLimit + value, 0);
				}
			}

			pSlider->setLowerLinLimit(currentLimit);
		}

		if (axis == -1)
		{
			auto currentLimit = pSlider->getUpperLinLimit();

			if (value > 0)
			{
				if (currentLimit < -1) {
					currentLimit = currentLimit - value;
				}
				else if (currentLimit > 1) {
					currentLimit = currentLimit + value;
				}
			}
			else if (value < 0)
			{
				if (currentLimit < -1) {
					currentLimit = min(currentLimit - value, 0);
				}
				else if (currentLimit > 1) {
					currentLimit = max(currentLimit + value, 0);
				}
			}

			pSlider->setUpperLinLimit(currentLimit);
		}

		if (axis == 0)
		{
			btScalar currentLimit = pSlider->getLowerLinLimit();
			currentLimit += value;
			pSlider->setLowerLinLimit(currentLimit);
		}

		if (axis == 3)
		{
			btScalar currentLimit = pSlider->getUpperLinLimit();
			currentLimit += value;
			pSlider->setUpperLinLimit(currentLimit);
		}

		return true;
	}

	return false;
}

float CBulletPhysicConstraint::GetMaxTolerantLinearError() const
{
	return m_maxTolerantLinearError;
}

void* CBulletPhysicConstraint::GetInternalConstraint()
{
	return m_pInternalConstraint;
}

btRigidBody* CBulletPhysicConstraint::CreateInternalRigidBody(bool attachToJointB)
{
	auto pMotionState = new CFollowConstraintMotionState(m_pPhysicObject, m_pInternalConstraint, attachToJointB);

	auto size = btScalar(2);

	FloatGoldSrcToBullet(&size);

	auto pCollisionShape = new btSphereShape(size);

	btRigidBody::btRigidBodyConstructionInfo cInfo(0, pMotionState, pCollisionShape);

	auto pRigidBody = new btRigidBody(cInfo);

	pRigidBody->setUserIndex(m_id);
	pRigidBody->setUserPointer(this);
	pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
	pRigidBody->setCollisionFlags(pRigidBody->getCollisionFlags() | BulletPhysicCollisionFlags::CF_DISABLE_VISUALIZE_OBJECT_PERMANENT);

	pRigidBody->setActivationState(DISABLE_DEACTIVATION);

	return pRigidBody;
}

void CBulletPhysicConstraint::FreeInternalRigidBody(btRigidBody* pRigidBody)
{
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

	delete pRigidBody;
}