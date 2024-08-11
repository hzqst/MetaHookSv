#include <metahook.h>
#include <triangleapi.h>
#include "mathlib2.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"

#include "ClientEntityManager.h"
#include "BulletPhysicManager.h"
#include "BulletStaticObject.h"
#include "BulletRagdollObject.h"

#include <vgui_controls/Controls.h>

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

/*
transform: Global transform of current object
at: Global position of the lookat target
forward: The forward vector of current object in local space
*/

btTransform MatrixLookAt(const btTransform& transform, const btVector3& at, const btVector3& forward)
{
	const auto &originVector = forward;
	auto worldToLocalTransform = transform.inverse();

	//transform the target in world position to object's local position
	auto targetVector = worldToLocalTransform * at;

	auto rot = FromToRotaion(originVector, targetVector);
	btTransform rotMatrix = btTransform(rot);

	return transform * rotMatrix;
}

void EulerMatrix(const btVector3& in_euler, btMatrix3x3& out_matrix)
{
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

void MatrixEuler(const btMatrix3x3& in_matrix, btVector3& out_euler)
{

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

CBulletBaseMotionState* GetMotionStateFromRigidBody(btRigidBody* RigidBody)
{
	return (CBulletBaseMotionState*)RigidBody->getMotionState();
}

IPhysicObject* GetPhysicObjectFromRigidBody(btRigidBody *pRigidBody)
{
	auto pMotionState = GetMotionStateFromRigidBody(pRigidBody);

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

btScalar BulletGetConstraintLinearErrorMagnitude(btTypedConstraint *pConstraint)
{
	if (pConstraint->getConstraintType() == CONETWIST_CONSTRAINT_TYPE)
	{
		auto pConeTwist = (btConeTwistConstraint*)pConstraint;

		auto worldPivotA = pConeTwist->getRigidBodyA().getWorldTransform() * pConeTwist->getFrameOffsetA();
		auto worldPivotB = pConeTwist->getRigidBodyB().getWorldTransform() * pConeTwist->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}
	else if (pConstraint->getConstraintType() == HINGE_CONSTRAINT_TYPE)
	{
		auto pHinge = (btHingeConstraint*)pConstraint;

		auto worldPivotA = pHinge->getRigidBodyA().getWorldTransform() * pHinge->getFrameOffsetA();
		auto worldPivotB = pHinge->getRigidBodyB().getWorldTransform() * pHinge->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}
	else if (pConstraint->getConstraintType() == D6_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btGeneric6DofConstraint*)pConstraint;

		auto worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		auto worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}
	else if (pConstraint->getConstraintType() == SLIDER_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btSliderConstraint*)pConstraint;

		auto worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		auto worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return (worldPivotB.getOrigin() - worldPivotA.getOrigin()).length();
	}

	return 0;
}

btTypedConstraint* BulletCreateConstraint_ConeTwist(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& localTransformA, const btTransform& localTransformB)
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

	auto pConeTwist = new btConeTwistConstraint(*rbA, *rbB, localTransformA, localTransformB);

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

	return pConeTwist;
}

btTypedConstraint* BulletCreateConstraint_Hinge(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& localTransformA, const btTransform& localTransformB)
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

	auto pHinge = new btHingeConstraint(*rbA, *rbB, localTransformA, localTransformB);

	pHinge->setLimit(lowLimit * M_PI, highLimit * M_PI, softness, biasFactor, relaxationFactor);

	pHinge->setParam(BT_CONSTRAINT_ERP, AngularERP, 5);
	pHinge->setParam(BT_CONSTRAINT_CFM, AngularCFM, 5);

	pHinge->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 5);
	pHinge->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 5);

	return pHinge;
}

btTypedConstraint* BulletCreateConstraint_Point(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& localTransformA, const btTransform& localTransformB)
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

	auto pPoint2Point = new btPoint2PointConstraint(*rbA, *rbB, localTransformA.getOrigin(), localTransformB.getOrigin());

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

	return pPoint2Point;
}

btTypedConstraint* BulletCreateConstraint_Slider(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& localTransformA, const btTransform& localTransformB)
{
	auto LowerLinearLimit = pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderLowerLinearLimit];

	if (isnan(LowerLinearLimit))
		return nullptr;

	auto UpperLinearLimit = pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderUpperLinearLimit];

	if (isnan(UpperLinearLimit))
		return nullptr;

	auto LowerAngularLimit = pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderLowerAngularLimit];

	if (isnan(LowerAngularLimit))
		LowerAngularLimit = -1;

	auto UpperAngularLimit = pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderUpperAngularLimit];

	if (isnan(UpperAngularLimit))
		UpperAngularLimit = 1;

	auto LinearERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearERP];

	if (isnan(LinearERP))
		LinearERP = BULLET_DEFAULT_LINEAR_ERP;

	auto LinearCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearCFM];

	if (isnan(LinearCFM))
		LinearCFM = BULLET_DEFAULT_LINEAR_CFM;

	auto AngularCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularCFM];

	if (isnan(AngularCFM))
		AngularCFM = BULLET_DEFAULT_ANGULAR_CFM;

	auto LinearStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopERP];

	if (isnan(LinearStopERP))
		LinearStopERP = BULLET_DEFAULT_LINEAR_STOP_ERP;

	auto AngularStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopERP];

	if (isnan(AngularStopERP))
		AngularStopERP = BULLET_DEFAULT_ANGULAR_STOP_ERP;

	auto LinearStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopCFM];

	if (isnan(LinearStopCFM))
		LinearStopCFM = BULLET_DEFAULT_LINEAR_STOP_CFM;

	auto AngularStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopCFM];

	if (isnan(AngularStopCFM))
		AngularStopCFM = BULLET_DEFAULT_ANGULAR_STOP_CFM;

	auto pSlider = new btSliderConstraint(*rbA, *rbB, localTransformA, localTransformB, pConstraintConfig->useLinearReferenceFrameA);

	pSlider->setLowerLinLimit(LowerLinearLimit);
	pSlider->setUpperLinLimit(UpperLinearLimit);

	//pSlider->setLowerAngLimit(LowerAngularLimit * M_PI);
	//pSlider->setUpperAngLimit(UpperAngularLimit * M_PI);

	pSlider->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
	pSlider->setParam(BT_CONSTRAINT_CFM, AngularCFM, 3);

	pSlider->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
	pSlider->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 3);

	pSlider->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
	pSlider->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 3);

	return pSlider;
}

btTypedConstraint* BulletCreateConstraint_Dof6(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& localTransformA, const btTransform& localTransformB)
{
	auto LowerLinearLimitX = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitX];

	if (isnan(LowerLinearLimitX))
		return nullptr;

	auto LowerLinearLimitY = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitY];

	if (isnan(LowerLinearLimitY))
		return nullptr;

	auto LowerLinearLimitZ = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitZ];

	if (isnan(LowerLinearLimitZ))
		return nullptr;

	btVector3 vecLowerLinearLimit(LowerLinearLimitX, LowerLinearLimitY, LowerLinearLimitZ);

	auto UpperLinearLimitX = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitX];

	if (isnan(UpperLinearLimitX))
		return nullptr;

	auto UpperLinearLimitY = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitY];

	if (isnan(UpperLinearLimitY))
		return nullptr;

	auto UpperLinearLimitZ = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitZ];

	if (isnan(UpperLinearLimitZ))
		return nullptr;

	btVector3 vecUpperLinearLimit(UpperLinearLimitX, UpperLinearLimitY, UpperLinearLimitZ);

	auto LowerAngularLimitX = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerAngularLimitX];

	if (isnan(LowerAngularLimitX))
		LowerAngularLimitX = -1;

	auto LowerAngularLimitY = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerAngularLimitY];

	if (isnan(LowerAngularLimitY))
		LowerAngularLimitY = -1;

	auto LowerAngularLimitZ = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerAngularLimitZ];

	if (isnan(LowerAngularLimitZ))
		LowerAngularLimitZ = -1;

	btVector3 vecLowerAngularLimit(LowerAngularLimitX, LowerAngularLimitY, LowerAngularLimitZ);

	auto UpperAngularLimitX = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperAngularLimitX];

	if (isnan(UpperAngularLimitX))
		UpperAngularLimitX = 1;

	auto UpperAngularLimitY = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperAngularLimitY];

	if (isnan(UpperAngularLimitY))
		UpperAngularLimitY = 1;

	auto UpperAngularLimitZ = pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperAngularLimitZ];

	if (isnan(UpperAngularLimitZ))
		UpperAngularLimitZ = 1;

	btVector3 vecUpperAngularLimit(UpperAngularLimitX, UpperAngularLimitY, UpperAngularLimitZ);

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

	auto pDof6 = new btGeneric6DofConstraint(*rbA, *rbB, localTransformA, localTransformB, pConstraintConfig->useLinearReferenceFrameA);

	pDof6->setLinearLowerLimit(vecLowerLinearLimit);
	pDof6->setLinearUpperLimit(vecUpperLinearLimit);

	auto vecLowerAngularLimit_PI = vecLowerAngularLimit * M_PI;
	auto vecUpperAngularLimit_PI = vecUpperAngularLimit * M_PI;

	pDof6->setAngularLowerLimit(vecLowerAngularLimit_PI);
	pDof6->setAngularUpperLimit(vecUpperAngularLimit_PI);

	pDof6->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
	pDof6->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
	pDof6->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);

	pDof6->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
	pDof6->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 1);
	pDof6->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 2);
	pDof6->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
	pDof6->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 1);
	pDof6->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 2);

	return pDof6;
}

btTypedConstraint* BulletCreateConstraint_Fixed(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& localTransformA, const btTransform& localTransformB)
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

	auto pFixed = new btFixedConstraint(*rbA, *rbB, localTransformA, localTransformB);

	pFixed->setParam(BT_CONSTRAINT_ERP, LinearERP, 0);
	pFixed->setParam(BT_CONSTRAINT_ERP, LinearERP, 1);
	pFixed->setParam(BT_CONSTRAINT_ERP, LinearERP, 2);
	pFixed->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
	pFixed->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
	pFixed->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);

	pFixed->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
	pFixed->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 1);
	pFixed->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 2);
	pFixed->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
	pFixed->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 1);
	pFixed->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 2);

	return pFixed;
}

btTypedConstraint* BulletCreateConstraintFromLocalJointTransform(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& localTransformA, const btTransform& localTransformB)
{
	btTypedConstraint* pConstraint{ };

	switch (pConstraintConfig->type)
	{
	case PhysicConstraint_ConeTwist:
	{
		pConstraint = BulletCreateConstraint_ConeTwist(pConstraintConfig, rbA, rbB, localTransformA, localTransformB);
		break;
	}
	case PhysicConstraint_Hinge:
	{
		pConstraint = BulletCreateConstraint_Hinge(pConstraintConfig, rbA, rbB, localTransformA, localTransformB);
		break;
	}
	case PhysicConstraint_Point:
	{
		pConstraint = BulletCreateConstraint_Point(pConstraintConfig, rbA, rbB, localTransformA, localTransformB);
		break;
	}
	case PhysicConstraint_Slider:
	{
		pConstraint = BulletCreateConstraint_Slider(pConstraintConfig, rbA, rbB, localTransformA, localTransformB);
		break;
	}
	case PhysicConstraint_Dof6:
	{
		pConstraint = BulletCreateConstraint_Dof6(pConstraintConfig, rbA, rbB, localTransformA, localTransformB);
		break;
	}
	case PhysicConstraint_Fixed:
	{
		pConstraint = BulletCreateConstraint_Fixed(pConstraintConfig, rbA, rbB, localTransformA, localTransformB);
		break;
	}
	}

	if (!pConstraint)
		return nullptr;

	pConstraint->setUserConstraintPtr(new CBulletConstraintSharedUserData(pConstraintConfig));

	return pConstraint;
}

btTypedConstraint* BulletCreateConstraintFromGlobalJointTransform(const CClientConstraintConfig* pConstraintConfig, btRigidBody* rbA, btRigidBody* rbB, const btTransform& globalJointTransform)
{
	const auto& worldTransA = rbA->getWorldTransform();

	const auto& worldTransB = rbB->getWorldTransform();

	btTransform localTransA;
	localTransA.mult(worldTransA.inverse(), globalJointTransform);

	btTransform localTransB;
	localTransB.mult(worldTransB.inverse(), globalJointTransform);

	return BulletCreateConstraintFromLocalJointTransform(pConstraintConfig, rbA, rbB, localTransA, localTransB);
}

btCollisionShape* BulletCreateCollisionShapeInternal(const CClientCollisionShapeConfig* pConfig)
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
	case PhysicShape_TriangleMesh:
	{
		if (!pConfig->m_pVertexArray)
		{
			gEngfuncs.Con_DPrintf("BulletCreateCollisionShapeInternal: m_pVertexArray cannot be null!\n");
			break;
		}

		if (!pConfig->m_pIndexArray)
		{
			gEngfuncs.Con_DPrintf("BulletCreateCollisionShapeInternal: m_pIndexArray cannot be null!\n");
			break;
		}

		if (!pConfig->m_pIndexArray->vIndexBuffer.size())
		{
			gEngfuncs.Con_DPrintf("BulletCreateCollisionShapeInternal: vIndexBuffer cannot be empty!\n");
			break;
		}

		if (!pConfig->m_pVertexArray->vVertexBuffer.size())
		{
			gEngfuncs.Con_DPrintf("BulletCreateCollisionShapeInternal: vVertexBuffer cannot be empty!\n");
			break;
		}

		auto pIndexVertexArray = new btTriangleIndexVertexArray(
			pConfig->m_pIndexArray->vIndexBuffer.size() / 3, pConfig->m_pIndexArray->vIndexBuffer.data(), 3 * sizeof(int),
			pConfig->m_pVertexArray->vVertexBuffer.size(), (float*)pConfig->m_pVertexArray->vVertexBuffer.data(), sizeof(CPhysicBrushVertex));

		auto pTriMesh = new btBvhTriangleMeshShape(pIndexVertexArray, true, true);

		pTriMesh->setUserPointer(new CBulletCollisionShapeSharedUserData(pIndexVertexArray));

		pShape = pTriMesh;
	}
	}

	return pShape;
}

btCollisionShape* BulletCreateCollisionShape(const CClientRigidBodyConfig* pRigidConfig)
{
	if (pRigidConfig->shapes.size() > 1)
	{
		auto pCompoundShape = new btCompoundShape();

		for (const auto &pShapeConfig : pRigidConfig->shapes)
		{
			auto pCollisionShape = BulletCreateCollisionShapeInternal(pShapeConfig.get());

			if (pCollisionShape)
			{
				btTransform localTrans;

				localTrans.setIdentity();

				EulerMatrix(btVector3(pShapeConfig->angles[0], pShapeConfig->angles[1], pShapeConfig->angles[2]), localTrans.getBasis());

				localTrans.setOrigin(btVector3(pShapeConfig->origin[0], pShapeConfig->origin[1], pShapeConfig->origin[2]));

				pCompoundShape->addChildShape(localTrans, pCollisionShape);
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
	else if (pRigidConfig->shapes.size() == 1)
	{
		auto pShapeConfig = pRigidConfig->shapes[0];

		return BulletCreateCollisionShapeInternal(pShapeConfig.get());
	}
	else
	{
		gEngfuncs.Con_Printf("BulletCreateCollisionShape: no shape available!\n");
	}

	return nullptr;
}

btMotionState* BulletCreateMotionState(const CPhysicObjectCreationParameter& CreationParam, const CClientRigidBodyConfig* pRigidConfig, IPhysicObject* pPhysicObject)
{
	if (pRigidConfig->isLegacyConfig)
	{
		if (!(pRigidConfig->boneindex >= 0 && pRigidConfig->boneindex < CreationParam.m_studiohdr->numbones))
		{
			gEngfuncs.Con_Printf("BulletCreateMotionState: invalid boneindex (%d).\n", pRigidConfig->boneindex);
			return nullptr;
		}

		if (!CreationParam.m_studiohdr)
		{
			gEngfuncs.Con_Printf("BulletCreateMotionState: invalid m_studiohdr.\n");
			return nullptr;
		}

		btTransform bonematrix;

		Matrix3x4ToTransform((*pbonetransform)[pRigidConfig->boneindex], bonematrix);

		if (!(pRigidConfig->pboneindex >= 0 && pRigidConfig->pboneindex < CreationParam.m_studiohdr->numbones))
		{
			gEngfuncs.Con_Printf("BulletCreateMotionState: invalid pboneindex (%d).\n", pRigidConfig->pboneindex);
			return nullptr;
		}

		const auto& boneorigin = bonematrix.getOrigin();

		btVector3 pboneorigin((*pbonetransform)[pRigidConfig->pboneindex][0][3], (*pbonetransform)[pRigidConfig->pboneindex][1][3], (*pbonetransform)[pRigidConfig->pboneindex][2][3]);

		btVector3 vecDirection = pboneorigin - boneorigin;

		vecDirection = vecDirection.normalize();

		btVector3 vecOriginWorldSpace = boneorigin + vecDirection * pRigidConfig->pboneoffset;

		btTransform bonematrix2 = bonematrix;
		bonematrix2.setOrigin(vecOriginWorldSpace);

		btVector3 vecForward(pRigidConfig->forward[0], pRigidConfig->forward[1], pRigidConfig->forward[2]);

		auto rigidTransformWorldSpace = MatrixLookAt(bonematrix2, pboneorigin, vecForward);

		btTransform offsetmatrix;

		offsetmatrix.mult(bonematrix.inverse(), rigidTransformWorldSpace);

		return new CBulletBoneMotionState(pPhysicObject, bonematrix, offsetmatrix);
	}

	btVector3 vecOrigin(pRigidConfig->origin[0], pRigidConfig->origin[1], pRigidConfig->origin[2]);

	btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

	btVector3 vecAngles(pRigidConfig->angles[0], pRigidConfig->angles[1], pRigidConfig->angles[2]);

	EulerMatrix(vecAngles, localTrans.getBasis());

	if (pRigidConfig->boneindex >= 0 && pRigidConfig->boneindex < CreationParam.m_studiohdr->numbones)
	{
		btTransform bonematrix;

		Matrix3x4ToTransform((*pbonetransform)[pRigidConfig->boneindex], bonematrix);

		return new CBulletBoneMotionState(pPhysicObject, bonematrix, localTrans);
	}
	else
	{
		return new CBulletEntityMotionState(pPhysicObject, localTrans);
	}

	return nullptr;
}

bool BulletCheckPhysicComponentFiltersForRigidBody(CBulletRigidBodySharedUserData *pSharedUserData, const CPhysicComponentFilters &filters)
{
	if (filters.m_HasWithoutRigidbodyFlags)
	{
		if (pSharedUserData->m_flags & filters.m_WithoutRigidbodyFlags)
		{
			return false;
		}
	}

	if (filters.m_HasExactMatchRigidbodyFlags)
	{
		if (pSharedUserData->m_flags == filters.m_ExactMatchRigidbodyFlags)
		{
			return true;
		}
	}

	if (filters.m_HasWithRigidbodyFlags)
	{
		if (pSharedUserData->m_flags & filters.m_WithRigidbodyFlags)
		{
			return true;
		}

		return false;
	}

	return true;
}

bool BulletCheckPhysicComponentFiltersForRigidBody(btRigidBody* pRigidBody, const CPhysicComponentFilters& filters)
{
	auto pSharedUserData = GetSharedUserDataFromRigidBody(pRigidBody);

	return BulletCheckPhysicComponentFiltersForRigidBody(pSharedUserData, filters);
}

bool BulletCheckPhysicComponentFiltersForConstraint(CBulletConstraintSharedUserData* pSharedUserData, const CPhysicComponentFilters& filters)
{
	if (filters.m_HasWithoutConstraintFlags)
	{
		if (pSharedUserData->m_flags & filters.m_WithoutConstraintFlags)
		{
			return false;
		}
	}

	if (filters.m_HasExactMatchConstraintFlags)
	{
		if (pSharedUserData->m_flags == filters.m_ExactMatchConstraintFlags)
		{
			return true;
		}
	}

	if (filters.m_HasWithConstraintFlags)
	{
		if (pSharedUserData->m_flags & filters.m_WithConstraintFlags)
		{
			return true;
		}

		return false;
	}

	return true;
}

bool BulletCheckPhysicComponentFiltersForConstraint(btTypedConstraint* pConstraint, const CPhysicComponentFilters& filters)
{
	auto pSharedUserData = GetSharedUserDataFromConstraint(pConstraint);

	return BulletCheckPhysicComponentFiltersForConstraint(pSharedUserData, filters);
}

void CBulletEntityMotionState::getWorldTransform(btTransform& worldTrans) const
{
	if (GetPhysicObject()->IsStaticObject())
	{
		auto entindex = GetPhysicObject()->GetEntityIndex();
		auto ent = GetPhysicObject()->GetClientEntity();

		btVector3 vecOrigin(ent->curstate.origin[0], ent->curstate.origin[1], ent->curstate.origin[2]);

		btTransform entityTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

		btVector3 vecAngles(ent->curstate.angles[0], ent->curstate.angles[1], ent->curstate.angles[2]);

		//Brush uses reverted pitch
		if (ent->curstate.solid == SOLID_BSP)
		{
			vecAngles.setX(-vecAngles.x());
		}

		EulerMatrix(vecAngles, entityTrans.getBasis());

		worldTrans.mult(entityTrans, m_offsetmatrix);
	}
}

void CBulletEntityMotionState::setWorldTransform(const btTransform& worldTrans)
{

}

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
		//The texture must be reset to zero upon drawing.
		vgui::surface()->DrawSetTexture(-1);
		vgui::surface()->DrawSetTexture(0);

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
		//TODO: TriAPI

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

class CBulletOverlapFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const override
	{
		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		if (collides)
		{
			auto pClientObjectA = (btCollisionObject*)proxy0->m_clientObject;
			auto pClientObjectB = (btCollisionObject*)proxy1->m_clientObject;

			auto pRigidBodyA = btRigidBody::upcast(pClientObjectA);
			auto pRigidBodyB = btRigidBody::upcast(pClientObjectB);

			if (pRigidBodyA)
			{
				auto pPhysicObjectA = GetPhysicObjectFromRigidBody(pRigidBodyA);
				if (pPhysicObjectA && pPhysicObjectA->IsClientEntityNonSolid()) {
					return false;
				}
			}

			if (pRigidBodyB)
			{
				auto pPhysicObjectB = GetPhysicObjectFromRigidBody(pRigidBodyB);
				if (pPhysicObjectB && pPhysicObjectB->IsClientEntityNonSolid()) {
					return false;
				}
			}
		}
		return collides;
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

	m_overlapFilterCallback = new CBulletOverlapFilterCallback();
	m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(m_overlapFilterCallback);

	m_dynamicsWorld->setGravity(btVector3(0, 0, 0));
}

void CBulletPhysicManager::Shutdown()
{
	CBasePhysicManager::Shutdown();

	if (m_overlapFilterCallback) {
		delete m_overlapFilterCallback;
		m_overlapFilterCallback = nullptr;
		m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(nullptr);
	}

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
				pConstraint->setDbgDrawSize(3);
			}
			else
			{
				pConstraint->setDbgDrawSize(0);
			}
		}
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

	m_dynamicsWorld->stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate());
}

void CBulletPhysicManager::AddPhysicObjectToWorld(IPhysicObject *PhysicObject, const CPhysicComponentFilters& filters)
{
	PhysicObject->AddToPhysicWorld(m_dynamicsWorld, filters);
}

void CBulletPhysicManager::RemovePhysicObjectFromWorld(IPhysicObject* PhysicObject, const CPhysicComponentFilters& filters)
{
	PhysicObject->RemoveFromPhysicWorld(m_dynamicsWorld, filters);
}

IStaticObject* CBulletPhysicManager::CreateStaticObject(const CStaticObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_pStaticObjectConfig)
	{
		gEngfuncs.Con_Printf("CreateStaticObject: invalid m_pStaticObjectConfig!\n");
		return nullptr;
	}

	return new CBulletStaticObject(CreationParam);
}

IDynamicObject* CBulletPhysicManager::CreateDynamicObject(const CDynamicObjectCreationParameter& CreationParam)
{
	return nullptr;
}

IRagdollObject* CBulletPhysicManager::CreateRagdollObject(const CRagdollObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_studiohdr)
	{
		gEngfuncs.Con_Printf("CreateRagdollObject: invalid m_studiohdr!\n");
		return nullptr;
	}

	if (!CreationParam.m_pRagdollObjectConfig)
	{
		gEngfuncs.Con_Printf("CreateRagdollObject: invalid m_pRagdollObjectConfig!\n");
		return nullptr;
	}

	return new CBulletRagdollObject(CreationParam);
}

IClientPhysicManager* BulletPhysicManager_CreateInstance()
{
	return new CBulletPhysicManager();
}