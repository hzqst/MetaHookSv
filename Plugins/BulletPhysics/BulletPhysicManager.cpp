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

void OnBeforeDeleteBulletRigidBody(IPhysicObject *pPhysicObjectToDelete, btRigidBody* pRigidBody)
{
	ClientPhysicManager()->OnBroadcastDeleteRigidBody(pPhysicObjectToDelete, pRigidBody);

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

void OnBeforeDeleteBulletConstraint(IPhysicObject *pPhysicObject, btTypedConstraint *pConstraint)
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

#define LOAD_FROM_FACTOR(name) auto name = pConstraintConfig->factors[PhysicConstraintFactorIdx_##name];

#define LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(name, defaultValue) LOAD_FROM_FACTOR(name) if (isnan(name)) name = defaultValue;

btTypedConstraint* BulletCreateConstraint_ConeTwist(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(ConeTwistSwingSpanLimit1, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(ConeTwistSwingSpanLimit2, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(ConeTwistTwistSpanLimit, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(ConeTwistSoftness, BULLET_DEFAULT_SOFTNESS);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(ConeTwistBiasFactor, BULLET_DEFAULT_BIAS_FACTOR);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(ConeTwistRelaxationFactor, BULLET_DEFAULT_RELAXTION_FACTOR);

	auto pConeTwist = new btConeTwistConstraint(*ctx.pRigidBodyA, *ctx.pRigidBodyB, finalLocalTransA, finalLocalTransB);

	pConeTwist->setLimit(ConeTwistSwingSpanLimit1 * M_PI, ConeTwistSwingSpanLimit2 * M_PI, ConeTwistTwistSpanLimit * M_PI, ConeTwistSoftness, ConeTwistBiasFactor, ConeTwistRelaxationFactor);

	LOAD_FROM_FACTOR(LinearERP);
	LOAD_FROM_FACTOR(LinearCFM);
	LOAD_FROM_FACTOR(AngularERP);
	LOAD_FROM_FACTOR(AngularCFM)

	if (!isnan(LinearERP))
	{
		pConeTwist->setParam(BT_CONSTRAINT_ERP, LinearERP, 0);
		pConeTwist->setParam(BT_CONSTRAINT_ERP, LinearERP, 1);
		pConeTwist->setParam(BT_CONSTRAINT_ERP, LinearERP, 2);
	}

	if (!isnan(LinearCFM))
	{
		pConeTwist->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
		pConeTwist->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
		pConeTwist->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);
	}

	if (!isnan(AngularERP))
	{
		pConeTwist->setParam(BT_CONSTRAINT_ERP, AngularERP, 3);
		pConeTwist->setParam(BT_CONSTRAINT_ERP, AngularERP, 4);
	}

	if (!isnan(AngularCFM))
	{
		pConeTwist->setParam(BT_CONSTRAINT_CFM, AngularCFM, 3);
		pConeTwist->setParam(BT_CONSTRAINT_CFM, AngularCFM, 4);
	}

	return pConeTwist;
}

btTypedConstraint* BulletCreateConstraint_Hinge(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(HingeLowLimit, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(HingeHighLimit, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(HingeSoftness, BULLET_DEFAULT_SOFTNESS);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(HingeBiasFactor, BULLET_DEFAULT_BIAS_FACTOR);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(HingeRelaxationFactor, BULLET_DEFAULT_RELAXTION_FACTOR);

	auto pHinge = new btHingeConstraint(*ctx.pRigidBodyA, *ctx.pRigidBodyB, finalLocalTransA, finalLocalTransB);

	pHinge->setLimit(HingeLowLimit * M_PI, HingeHighLimit * M_PI, HingeSoftness, HingeBiasFactor, HingeRelaxationFactor);

	LOAD_FROM_FACTOR(AngularERP);
	LOAD_FROM_FACTOR(AngularCFM);
	LOAD_FROM_FACTOR(AngularStopERP);
	LOAD_FROM_FACTOR(AngularStopCFM);

	if (!isnan(AngularERP))
	{
		pHinge->setParam(BT_CONSTRAINT_ERP, AngularERP, 5);
	}

	if (!isnan(AngularCFM))
	{
		pHinge->setParam(BT_CONSTRAINT_CFM, AngularCFM, 5);
	}

	if (!isnan(AngularStopERP))
	{
		pHinge->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 5);
	}

	if (!isnan(AngularStopCFM))
	{
		pHinge->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 5);
	}

	return pHinge;
}

btTypedConstraint* BulletCreateConstraint_Point(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	auto pPoint2Point = new btPoint2PointConstraint(*ctx.pRigidBodyA, *ctx.pRigidBodyB, finalLocalTransA.getOrigin(), finalLocalTransB.getOrigin());

	LOAD_FROM_FACTOR(AngularERP);
	LOAD_FROM_FACTOR(AngularCFM);

	if (!isnan(AngularERP))
		pPoint2Point->setParam(BT_CONSTRAINT_ERP, AngularERP, -1);

	if (!isnan(AngularCFM))
		pPoint2Point->setParam(BT_CONSTRAINT_CFM, AngularCFM, -1);
	
	return pPoint2Point;
}

btTypedConstraint* BulletCreateConstraint_Slider(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(SliderLowerLinearLimit, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(SliderUpperLinearLimit, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(SliderLowerAngularLimit, -1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(SliderUpperAngularLimit, 1);

	if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit)
	{
		SliderLowerLinearLimit *= ctx.rigidBodyDistance;
		SliderUpperLinearLimit *= ctx.rigidBodyDistance;
	}

	auto pSlider = new btSliderConstraint(*ctx.pRigidBodyA, *ctx.pRigidBodyB, finalLocalTransA, finalLocalTransB, pConstraintConfig->useLinearReferenceFrameA);

	pSlider->setLowerLinLimit(SliderLowerLinearLimit);
	pSlider->setUpperLinLimit(SliderUpperLinearLimit);

	pSlider->setLowerAngLimit(SliderLowerAngularLimit * M_PI);
	pSlider->setUpperAngLimit(SliderUpperAngularLimit * M_PI);

	LOAD_FROM_FACTOR(LinearCFM);
	LOAD_FROM_FACTOR(LinearStopERP);
	LOAD_FROM_FACTOR(LinearStopCFM);

	LOAD_FROM_FACTOR(AngularCFM);
	LOAD_FROM_FACTOR(AngularStopERP);
	LOAD_FROM_FACTOR(AngularStopCFM);

	if (!isnan(LinearCFM))
		pSlider->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);

	if (!isnan(LinearStopERP))
		pSlider->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);

	if (!isnan(LinearStopCFM))
		pSlider->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);

	if (!isnan(AngularCFM))
		pSlider->setParam(BT_CONSTRAINT_CFM, AngularCFM, 3);

	if (!isnan(AngularStopERP))
		pSlider->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 3);

	if (!isnan(AngularStopCFM))
		pSlider->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 3);

	return pSlider;
}

btTypedConstraint* BulletCreateConstraint_Dof6(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerLinearLimitX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerLinearLimitY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerLinearLimitZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperLinearLimitX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperLinearLimitY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperLinearLimitZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerAngularLimitX, -1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerAngularLimitY, -1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerAngularLimitZ, -1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperAngularLimitX, 1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperAngularLimitY, 1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperAngularLimitZ, 1);

	btVector3 vecLowerLinearLimit(Dof6LowerLinearLimitX, Dof6LowerLinearLimitY, Dof6LowerLinearLimitZ);
	btVector3 vecUpperLinearLimit(Dof6UpperLinearLimitX, Dof6UpperLinearLimitY, Dof6UpperLinearLimitZ);
	btVector3 vecLowerAngularLimit(Dof6LowerAngularLimitX, Dof6LowerAngularLimitY, Dof6LowerAngularLimitZ);
	btVector3 vecUpperAngularLimit(Dof6UpperAngularLimitX, Dof6UpperAngularLimitY, Dof6UpperAngularLimitZ);

	if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit)
	{
		vecLowerLinearLimit *= ctx.rigidBodyDistance;
		vecUpperLinearLimit *= ctx.rigidBodyDistance;
	}

	auto pDof6 = new btGeneric6DofConstraint(*ctx.pRigidBodyA, *ctx.pRigidBodyB, finalLocalTransA, finalLocalTransB, pConstraintConfig->useLinearReferenceFrameA);

	pDof6->setLinearLowerLimit(vecLowerLinearLimit);
	pDof6->setLinearUpperLimit(vecUpperLinearLimit);

	vecLowerAngularLimit *= M_PI;
	vecUpperAngularLimit *= M_PI;

	pDof6->setAngularLowerLimit(vecLowerAngularLimit);
	pDof6->setAngularUpperLimit(vecUpperAngularLimit);

	LOAD_FROM_FACTOR(LinearCFM);
	LOAD_FROM_FACTOR(LinearStopERP);
	LOAD_FROM_FACTOR(LinearStopCFM);

	LOAD_FROM_FACTOR(AngularCFM);
	LOAD_FROM_FACTOR(AngularStopERP);
	LOAD_FROM_FACTOR(AngularStopCFM);

	if (!isnan(LinearCFM))
	{
		pDof6->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
		pDof6->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
		pDof6->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);
	}

	if (!isnan(LinearStopERP))
	{
		pDof6->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
		pDof6->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 1);
		pDof6->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 2);
	}

	if (!isnan(LinearStopCFM))
	{
		pDof6->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
		pDof6->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 1);
		pDof6->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 2);
	}

	if (!isnan(AngularCFM))
	{
		pDof6->setParam(BT_CONSTRAINT_CFM, AngularCFM, 3);
		pDof6->setParam(BT_CONSTRAINT_CFM, AngularCFM, 4);
		pDof6->setParam(BT_CONSTRAINT_CFM, AngularCFM, 5);
	}

	if (!isnan(AngularStopERP))
	{
		pDof6->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 3);
		pDof6->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 4);
		pDof6->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 5);
	}

	if (!isnan(AngularStopCFM))
	{
		pDof6->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 3);
		pDof6->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 4);
		pDof6->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 5);
	}

	return pDof6;
}

RotateOrder ConvertRotOrderToBulletRotateOrder(int rotOrder)
{
	switch (rotOrder)
	{
	case PhysicRotOrder_XYZ:
		return RotateOrder::RO_XYZ;
	case PhysicRotOrder_XZY:
		return RotateOrder::RO_XZY;
	case PhysicRotOrder_YXZ:
		return RotateOrder::RO_YXZ;
	case PhysicRotOrder_YZX:
		return RotateOrder::RO_YZX;
	case PhysicRotOrder_ZXY:
		return RotateOrder::RO_ZXY;
	case PhysicRotOrder_ZYX:
		return RotateOrder::RO_ZYX;
	}

	return RotateOrder::RO_XYZ;
}

btTypedConstraint* BulletCreateConstraint_Dof6Spring(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerLinearLimitX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerLinearLimitY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerLinearLimitZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperLinearLimitX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperLinearLimitY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperLinearLimitZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerAngularLimitX, -1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerAngularLimitY, -1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6LowerAngularLimitZ, -1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperAngularLimitX, 1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperAngularLimitY, 1);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6UpperAngularLimitZ, 1);

	btVector3 vecLowerLinearLimit(Dof6LowerLinearLimitX, Dof6LowerLinearLimitY, Dof6LowerLinearLimitZ);
	btVector3 vecUpperLinearLimit(Dof6UpperLinearLimitX, Dof6UpperLinearLimitY, Dof6UpperLinearLimitZ);
	btVector3 vecLowerAngularLimit(Dof6LowerAngularLimitX, Dof6LowerAngularLimitY, Dof6LowerAngularLimitZ);
	btVector3 vecUpperAngularLimit(Dof6UpperAngularLimitX, Dof6UpperAngularLimitY, Dof6UpperAngularLimitZ);

	if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit)
	{
		vecLowerLinearLimit *= ctx.rigidBodyDistance;
		vecUpperLinearLimit *= ctx.rigidBodyDistance;
	}

	auto pDof6Spring = new btGeneric6DofSpring2Constraint(*ctx.pRigidBodyA, *ctx.pRigidBodyB, finalLocalTransA, finalLocalTransB, ConvertRotOrderToBulletRotateOrder(pConstraintConfig->rotOrder));
	
	pDof6Spring->setLinearLowerLimit(vecLowerLinearLimit);
	pDof6Spring->setLinearUpperLimit(vecUpperLinearLimit);

	auto vecLowerAngularLimit_PI = vecLowerAngularLimit * M_PI;
	auto vecUpperAngularLimit_PI = vecUpperAngularLimit * M_PI;

	pDof6Spring->setAngularLowerLimit(vecLowerAngularLimit_PI);
	pDof6Spring->setAngularUpperLimit(vecUpperAngularLimit_PI);

	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringEnableLinearSpringX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringEnableLinearSpringY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringEnableLinearSpringZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringEnableAngularSpringX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringEnableAngularSpringY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringEnableAngularSpringZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringLinearStiffnessX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringLinearStiffnessY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringLinearStiffnessZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringAngularStiffnessX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringAngularStiffnessY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringAngularStiffnessZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringLinearDampingX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringLinearDampingY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringLinearDampingZ, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringAngularDampingX, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringAngularDampingY, 0);
	LOAD_FROM_FACTOR_WITH_DEFAULT_VALUE(Dof6SpringAngularDampingZ, 0);

	if (Dof6SpringEnableLinearSpringX >= 1)
	{
		pDof6Spring->enableSpring(0, true);
		pDof6Spring->setStiffness(0, Dof6SpringLinearStiffnessX);
		pDof6Spring->setDamping(0, Dof6SpringLinearDampingX);
	}

	if (Dof6SpringEnableLinearSpringY >= 1)
	{
		pDof6Spring->enableSpring(1, true);
		pDof6Spring->setStiffness(1, Dof6SpringLinearStiffnessY);
		pDof6Spring->setDamping(1, Dof6SpringLinearDampingY);
	}

	if (Dof6SpringEnableLinearSpringZ >= 1)
	{
		pDof6Spring->enableSpring(2, true);
		pDof6Spring->setStiffness(2, Dof6SpringLinearStiffnessZ);
		pDof6Spring->setDamping(2, Dof6SpringLinearDampingZ);
	}

	if (Dof6SpringEnableAngularSpringX >= 1)
	{
		pDof6Spring->enableSpring(3, true);
		pDof6Spring->setStiffness(3, Dof6SpringAngularStiffnessX);
		pDof6Spring->setDamping(3, Dof6SpringAngularDampingX);
	}

	if (Dof6SpringEnableAngularSpringY >= 1)
	{
		pDof6Spring->enableSpring(4, true);
		pDof6Spring->setStiffness(3, Dof6SpringAngularStiffnessY);
		pDof6Spring->setDamping(3, Dof6SpringAngularDampingY);
	}

	if (Dof6SpringEnableAngularSpringZ >= 1)
	{
		pDof6Spring->enableSpring(5, true);
		pDof6Spring->setStiffness(5, Dof6SpringAngularStiffnessZ);
		pDof6Spring->setDamping(5, Dof6SpringAngularDampingZ);
	}

	LOAD_FROM_FACTOR(LinearCFM);
	LOAD_FROM_FACTOR(LinearStopERP);
	LOAD_FROM_FACTOR(LinearStopCFM);

	LOAD_FROM_FACTOR(AngularCFM);
	LOAD_FROM_FACTOR(AngularStopERP);
	LOAD_FROM_FACTOR(AngularStopCFM);

	if (!isnan(LinearCFM))
	{
		pDof6Spring->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
		pDof6Spring->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
		pDof6Spring->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);
	}

	if (!isnan(LinearStopERP))
	{
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 1);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 2);
	}

	if (!isnan(LinearStopCFM))
	{
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 1);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 2);
	}

	if (!isnan(AngularCFM))
	{
		pDof6Spring->setParam(BT_CONSTRAINT_CFM, AngularCFM, 3);
		pDof6Spring->setParam(BT_CONSTRAINT_CFM, AngularCFM, 4);
		pDof6Spring->setParam(BT_CONSTRAINT_CFM, AngularCFM, 5);
	}

	if (!isnan(AngularStopERP))
	{
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 3);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 4);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 5);
	}

	if (!isnan(AngularStopCFM))
	{
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 3);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 4);
		pDof6Spring->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 5);
	}

	return pDof6Spring;
}

btTypedConstraint* BulletCreateConstraint_Fixed(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	auto pFixed = new btFixedConstraint(*ctx.pRigidBodyA, *ctx.pRigidBodyB, finalLocalTransA, finalLocalTransB);

	auto LinearCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearCFM];
	auto LinearStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopERP];
	auto LinearStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearStopCFM];

	auto AngularCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularCFM];
	auto AngularStopERP = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopERP];
	auto AngularStopCFM = pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopCFM];

	if (!isnan(LinearCFM))
	{
		pFixed->setParam(BT_CONSTRAINT_CFM, LinearCFM, 0);
		pFixed->setParam(BT_CONSTRAINT_CFM, LinearCFM, 1);
		pFixed->setParam(BT_CONSTRAINT_CFM, LinearCFM, 2);
	}

	if (!isnan(LinearStopERP))
	{
		pFixed->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 0);
		pFixed->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 1);
		pFixed->setParam(BT_CONSTRAINT_STOP_ERP, LinearStopERP, 2);
	}

	if (!isnan(LinearStopCFM))
	{
		pFixed->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 0);
		pFixed->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 1);
		pFixed->setParam(BT_CONSTRAINT_STOP_CFM, LinearStopCFM, 2);
	}

	if (!isnan(AngularCFM))
	{
		pFixed->setParam(BT_CONSTRAINT_CFM, AngularCFM, 3);
		pFixed->setParam(BT_CONSTRAINT_CFM, AngularCFM, 4);
		pFixed->setParam(BT_CONSTRAINT_CFM, AngularCFM, 5);
	}

	if (!isnan(AngularStopERP))
	{
		pFixed->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 3);
		pFixed->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 4);
		pFixed->setParam(BT_CONSTRAINT_STOP_ERP, AngularStopERP, 5);
	}

	if (!isnan(AngularStopCFM))
	{
		pFixed->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 3);
		pFixed->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 4);
		pFixed->setParam(BT_CONSTRAINT_STOP_CFM, AngularStopCFM, 5);
	}

	return pFixed;
}

btTypedConstraint* BulletCreateConstraintFromLocalJointTransform(const CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext& ctx, const btTransform& finalLocalTransA, const btTransform& finalLocalTransB)
{
	btTypedConstraint* pConstraint{ };

	switch (pConstraintConfig->type)
	{
	case PhysicConstraint_ConeTwist:
	{
		pConstraint = BulletCreateConstraint_ConeTwist(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
		break;
	}
	case PhysicConstraint_Hinge:
	{
		pConstraint = BulletCreateConstraint_Hinge(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
		break;
	}
	case PhysicConstraint_Point:
	{
		pConstraint = BulletCreateConstraint_Point(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
		break;
	}
	case PhysicConstraint_Slider:
	{
		pConstraint = BulletCreateConstraint_Slider(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
		break;
	}
	case PhysicConstraint_Dof6:
	{
		pConstraint = BulletCreateConstraint_Dof6(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
		break;
	}
	case PhysicConstraint_Dof6Spring:
	{
		pConstraint = BulletCreateConstraint_Dof6Spring(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
		break;
	}
	case PhysicConstraint_Fixed:
	{
		pConstraint = BulletCreateConstraint_Fixed(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
		break;
	}
	}

	if (!pConstraint)
		return nullptr;

	pConstraint->setUserConstraintId(ClientPhysicManager()->AllocatePhysicComponentId());

	pConstraint->setUserConstraintPtr(new CBulletConstraintSharedUserData(pConstraintConfig));

	return pConstraint;
}

btTypedConstraint* BulletCreateConstraintFromGlobalJointTransform(CClientConstraintConfig* pConstraintConfig, const CBulletConstraintCreationContext &ctx, const btTransform& globalJointTransform)
{
	btTransform finalLocalTransA;
	btTransform finalLocalTransB;

	finalLocalTransA.mult(ctx.invWorldTransA, globalJointTransform);
	finalLocalTransB.mult(ctx.invWorldTransB, globalJointTransform);

	if (pConstraintConfig->isLegacyConfig)
	{
		//Convert to new attribs

		const auto& localOriginA = finalLocalTransA.getOrigin();
		btVector3 localAnglesA;
		MatrixEuler(finalLocalTransA.getBasis(), localAnglesA);
		pConstraintConfig->originA[0] = localOriginA.getX();
		pConstraintConfig->originA[1] = localOriginA.getY();
		pConstraintConfig->originA[2] = localOriginA.getZ();
		pConstraintConfig->anglesA[0] = localAnglesA.getX();
		pConstraintConfig->anglesA[1] = localAnglesA.getY();
		pConstraintConfig->anglesA[2] = localAnglesA.getZ();

		const auto& localOriginB = finalLocalTransB.getOrigin();
		btVector3 localAnglesB;
		MatrixEuler(finalLocalTransB.getBasis(), localAnglesB);
		pConstraintConfig->originB[0] = localOriginB.getX();
		pConstraintConfig->originB[1] = localOriginB.getY();
		pConstraintConfig->originB[2] = localOriginB.getZ();
		pConstraintConfig->anglesB[0] = localAnglesB.getX();
		pConstraintConfig->anglesB[1] = localAnglesB.getY();
		pConstraintConfig->anglesB[2] = localAnglesB.getZ();
		//pConstraintConfig->isLegacyConfig = false;
	}

	return BulletCreateConstraintFromLocalJointTransform(pConstraintConfig, ctx, finalLocalTransA, finalLocalTransB);
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

btMotionState* BulletCreateMotionState(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, IPhysicObject* pPhysicObject)
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

		btVector3 vecWorldOrigin = boneorigin + vecDirection * pRigidConfig->pboneoffset;

		btTransform bonematrix2 = bonematrix;

		bonematrix2.setOrigin(vecWorldOrigin);

		btVector3 vecForward(pRigidConfig->forward[0], pRigidConfig->forward[1], pRigidConfig->forward[2]);

		auto rigidWorldTrans = MatrixLookAt(bonematrix2, pboneorigin, vecForward);

		btTransform localTrans;

		localTrans.mult(bonematrix.inverse(), rigidWorldTrans);

		//Convert to new attribs
		const auto& localOrigin = localTrans.getOrigin();
		btVector3 localAngles;
		MatrixEuler(localTrans.getBasis(), localAngles);
		pRigidConfig->origin[0] = localOrigin.getX();
		pRigidConfig->origin[1] = localOrigin.getY();
		pRigidConfig->origin[2] = localOrigin.getZ();
		pRigidConfig->angles[0] = localAngles.getX();
		pRigidConfig->angles[1] = localAngles.getY();
		pRigidConfig->angles[2] = localAngles.getZ();
		//pRigidConfig->isLegacyConfig = false;

		return new CBulletBoneMotionState(pPhysicObject, bonematrix, localTrans);
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

bool BulletCheckPhysicComponentFiltersForRigidBody(btRigidBody* pRigidBody, CBulletRigidBodySharedUserData *pSharedUserData, const CPhysicComponentFilters &filters)
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

	if (filters.m_HasExactMatchRigidBodyComponentId)
	{
		if (pRigidBody->getUserIndex() == filters.m_ExactMatchRigidBodyComponentId)
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

	return BulletCheckPhysicComponentFiltersForRigidBody(pRigidBody, pSharedUserData, filters);
}

bool BulletCheckPhysicComponentFiltersForConstraint(btTypedConstraint* pConstraint, CBulletConstraintSharedUserData* pSharedUserData, const CPhysicComponentFilters& filters)
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

	if (filters.m_HasExactMatchConstraintComponentId)
	{
		if (pConstraint->getUserConstraintId() == filters.m_ExactMatchConstraintComponentId)
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

	return BulletCheckPhysicComponentFiltersForConstraint(pConstraint, pSharedUserData, filters);
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

		if (IsDebugDrawWallHackEnabled())
		{
			glDisable(GL_DEPTH_TEST);

			if (color1.isZero()) {
				gEngfuncs.pTriAPI->Color4fRendermode(1, 1, 0, 1, kRenderTransAlpha);
			}
			else {
				gEngfuncs.pTriAPI->Color4fRendermode(color1.getX(), color1.getY(), color1.getZ(), 1, kRenderTransAlpha);
			}
			gEngfuncs.pTriAPI->Begin(TRI_LINES);

			vec3_t vecFrom = { from1.getX(), from1.getY(), from1.getZ() };
			vec3_t vecTo = { to1.getX(), to1.getY(), to1.getZ() };

			gEngfuncs.pTriAPI->Vertex3fv(vecFrom);
			gEngfuncs.pTriAPI->Vertex3fv(vecTo);
			gEngfuncs.pTriAPI->End();

			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			if (color1.isZero()) {
				gEngfuncs.pTriAPI->Color4fRendermode(1, 1, 0, 1, kRenderTransAlpha);
			}
			else {
				gEngfuncs.pTriAPI->Color4fRendermode(color1.getX(), color1.getY(), color1.getZ(), 1, kRenderTransAlpha);
			}
			gEngfuncs.pTriAPI->Begin(TRI_LINES);

			vec3_t vecFrom = { from1.getX(), from1.getY(), from1.getZ() };
			vec3_t vecTo = { to1.getX(), to1.getY(), to1.getZ() };

			gEngfuncs.pTriAPI->Vertex3fv(vecFrom);
			gEngfuncs.pTriAPI->Vertex3fv(vecTo);
			gEngfuncs.pTriAPI->End();
		}
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

	m_debugDraw = new CBulletPhysicsDebugDraw();
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

	int iRagdollObjectDebugDrawLevel = GetRagdollObjectDebugDrawLevel();
	int iStaticObjectDebugDrawLevel = GetStaticObjectDebugDrawLevel();
	int iConstraintDebugDrawLevel = GetConstraintDebugDrawLevel();

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
				if (iRagdollObjectDebugDrawLevel > 0 && pSharedUserData->m_debugDrawLevel > 0 && iRagdollObjectDebugDrawLevel >= pSharedUserData->m_debugDrawLevel)
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
				if (iStaticObjectDebugDrawLevel > 0 && pSharedUserData->m_debugDrawLevel > 0 && iStaticObjectDebugDrawLevel >= pSharedUserData->m_debugDrawLevel)
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
			if (iConstraintDebugDrawLevel > 0 && pSharedUserData->m_debugDrawLevel > 0 && iConstraintDebugDrawLevel >= pSharedUserData->m_debugDrawLevel)
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

void CBulletPhysicManager::OnBroadcastDeleteRigidBody(IPhysicObject* pPhysicObjectToDelete, void* pRigidBody)
{
	for (auto itor = m_physicObjects.begin(); itor != m_physicObjects.end(); itor++)
	{
		auto pPhysicObject = itor->second;

		if (pPhysicObject != pPhysicObjectToDelete)
		{
			pPhysicObject->OnBroadcastDeleteRigidBody(pPhysicObjectToDelete, m_dynamicsWorld, pRigidBody);
		}
	}
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