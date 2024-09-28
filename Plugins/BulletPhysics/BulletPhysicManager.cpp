#include <metahook.h>
#include <triangleapi.h>
#include "mathlib2.h"
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "plugins.h"
#include "util.h"

#include "ClientEntityManager.h"
#include "BulletPhysicManager.h"
#include "BulletStaticObject.h"
#include "BulletDynamicObject.h"
#include "BulletRagdollObject.h"

#include <vgui_controls/Controls.h>
#include <glew.h>

btVector3 GetVector3FromVec3(const vec3_t src)
{
	return btVector3(src[0], src[1], src[2]);
}

void GetVec3FromVector3(const btVector3 &src, vec3_t dst)
{
	dst[0] = src.x();
	dst[1] = src.y();
	dst[2] = src.z();
}

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

//GoldSrcToBullet Scaling

void FloatGoldSrcToBullet(float* v)
{
	(*v) *= G2BScale;
}

void FloatBulletToGoldSrc(float* v)
{
	(*v) *= B2GScale;
}

void TransformGoldSrcToBullet(btTransform& trans)
{
	auto& org = trans.getOrigin();

	org.m_floats[0] *= G2BScale;
	org.m_floats[1] *= G2BScale;
	org.m_floats[2] *= G2BScale;
}

void Vec3GoldSrcToBullet(vec3_t vec)
{
	vec[0] *= G2BScale;
	vec[1] *= G2BScale;
	vec[2] *= G2BScale;
}

void Vector3GoldSrcToBullet(btVector3& vec)
{
	vec.m_floats[0] *= G2BScale;
	vec.m_floats[1] *= G2BScale;
	vec.m_floats[2] *= G2BScale;
}

//BulletToGoldSrc Scaling

void TransformBulletToGoldSrc(btTransform& trans)
{
	trans.getOrigin().m_floats[0] *= B2GScale;
	trans.getOrigin().m_floats[1] *= B2GScale;
	trans.getOrigin().m_floats[2] *= B2GScale;
}

void Vec3BulletToGoldSrc(vec3_t vec)
{
	vec[0] *= B2GScale;
	vec[1] *= B2GScale;
	vec[2] *= B2GScale;
}

void Vector3BulletToGoldSrc(btVector3& vec)
{
	vec.m_floats[0] *= B2GScale;
	vec.m_floats[1] *= B2GScale;
	vec.m_floats[2] *= B2GScale;
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

bool BulletGetConstraintGlobalPivotTransform(btTypedConstraint* pConstraint, btTransform & worldPivotA, btTransform& worldPivotB)
{
	if (pConstraint->getConstraintType() == CONETWIST_CONSTRAINT_TYPE)
	{
		auto pConeTwist = (btConeTwistConstraint*)pConstraint;

		worldPivotA = pConeTwist->getRigidBodyA().getWorldTransform() * pConeTwist->getFrameOffsetA();
		worldPivotB = pConeTwist->getRigidBodyB().getWorldTransform() * pConeTwist->getFrameOffsetB();

		return true;
	}
	else if (pConstraint->getConstraintType() == HINGE_CONSTRAINT_TYPE)
	{
		auto pHinge = (btHingeConstraint*)pConstraint;

		worldPivotA = pHinge->getRigidBodyA().getWorldTransform() * pHinge->getFrameOffsetA();
		worldPivotB = pHinge->getRigidBodyB().getWorldTransform() * pHinge->getFrameOffsetB();

		return true;
	}
	else if (pConstraint->getConstraintType() == D6_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btGeneric6DofConstraint*)pConstraint;

		worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return true;
	}
	else if (pConstraint->getConstraintType() == D6_SPRING_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btGeneric6DofSpringConstraint*)pConstraint;

		worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return true;
	}
	else if (pConstraint->getConstraintType() == D6_SPRING_2_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btGeneric6DofSpring2Constraint*)pConstraint;

		worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return true;
	}
	else if (pConstraint->getConstraintType() == SLIDER_CONSTRAINT_TYPE)
	{
		auto pDof6 = (btSliderConstraint*)pConstraint;

		worldPivotA = pDof6->getRigidBodyA().getWorldTransform() * pDof6->getFrameOffsetA();
		worldPivotB = pDof6->getRigidBodyB().getWorldTransform() * pDof6->getFrameOffsetB();

		return true;
	}
	return false;
}

btScalar BulletGetConstraintLinearErrorMagnitude(btTypedConstraint *pConstraint)
{
	btTransform worldPivotA;
	btTransform worldPivotB;

	if (BulletGetConstraintGlobalPivotTransform(pConstraint, worldPivotA, worldPivotB))
	{
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

	pDof6Spring->setEquilibriumPoint();

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
		pConstraintConfig->isLegacyConfig = false;
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

		Vector3GoldSrcToBullet(size);

		pShape = new btBoxShape(size);

		break;
	}
	case PhysicShape_Sphere:
	{
		auto size = btScalar(pConfig->size[0]);

		FloatGoldSrcToBullet(&size);

		pShape = new btSphereShape(size);

		break;
	}
	case PhysicShape_Capsule:
	{
		if (pConfig->direction == PhysicShapeDirection_X)
		{
			btScalar sizeX(pConfig->size[0]);
			btScalar sizeY(pConfig->size[1]);

			FloatGoldSrcToBullet(&sizeX);
			FloatGoldSrcToBullet(&sizeY);

			pShape = new btCapsuleShapeX(sizeX, sizeY);
		}
		else if (pConfig->direction == PhysicShapeDirection_Y)
		{
			btScalar sizeX(pConfig->size[0]);
			btScalar sizeY(pConfig->size[1]);

			FloatGoldSrcToBullet(&sizeX);
			FloatGoldSrcToBullet(&sizeY);

			pShape = new btCapsuleShape(sizeX, sizeY);
		}
		else if (pConfig->direction == PhysicShapeDirection_Z)
		{
			btScalar sizeX(pConfig->size[0]);
			btScalar sizeY(pConfig->size[1]);

			FloatGoldSrcToBullet(&sizeX);
			FloatGoldSrcToBullet(&sizeY);

			pShape = new btCapsuleShapeZ(sizeX, sizeY);
		}

		break;
	}
	case PhysicShape_Cylinder:
	{
		if (pConfig->direction == PhysicShapeDirection_X)
		{
			btVector3 size(pConfig->size[0], pConfig->size[1], pConfig->size[2]);

			Vector3GoldSrcToBullet(size);

			pShape = new btCylinderShapeX(size);
		}
		else if (pConfig->direction == PhysicShapeDirection_Y)
		{
			btVector3 size(pConfig->size[0], pConfig->size[1], pConfig->size[2]);

			Vector3GoldSrcToBullet(size);

			pShape = new btCylinderShape(size);
		}
		else if (pConfig->direction == PhysicShapeDirection_Z)
		{
			btVector3 size(pConfig->size[0], pConfig->size[1], pConfig->size[2]);

			Vector3GoldSrcToBullet(size);

			pShape = new btCylinderShapeZ(size);
		}

		break;
	}
	case PhysicShape_TriangleMesh:
	{
		if (pConfig->resourcePath.empty())
		{
			gEngfuncs.Con_Printf("BulletCreateCollisionShapeInternal: resourcePath cannot be empty!\n");
			break;
		}

		auto pIndexArray = ClientPhysicManager()->LoadIndexArrayFromResource(pConfig->resourcePath);

		if (!pIndexArray)
		{
			gEngfuncs.Con_Printf("BulletCreateCollisionShapeInternal: Could not find IndexArray for \"%s\"!\n", pConfig->resourcePath.c_str());
			break;
		}

		auto pTriangleIndexVertexArray = new btTriangleIndexVertexArray(
			pIndexArray->vIndexBuffer.size() / 3, pIndexArray->vIndexBuffer.data(), 3 * sizeof(int),
			pIndexArray->pVertexArray->vVertexBuffer.size(), (float*)pIndexArray->pVertexArray->vVertexBuffer.data(), sizeof(CPhysicBrushVertex));

		auto pSharedData = new CBulletCollisionShapeSharedUserData();

		pSharedData->m_pIndexArray = pIndexArray;

		auto pTriMesh = new btBvhTriangleMeshShape(pTriangleIndexVertexArray, true, true);

		pTriMesh->setUserPointer(pSharedData);

		pShape = pTriMesh;
		break;
	}
	case PhysicShape_Compound:
	{
		auto pCompoundShape = new btCompoundShape();

		for (const auto& pSubShapeConfig : pConfig->compoundShapes)
		{
			auto pCollisionShape = BulletCreateCollisionShapeInternal(pSubShapeConfig.get());

			if (pCollisionShape)
			{
				btTransform localTrans;

				localTrans.setIdentity();

				btVector3 vecAngles(pSubShapeConfig->angles[0], pSubShapeConfig->angles[1], pSubShapeConfig->angles[2]);
				btVector3 vecOrigin(pSubShapeConfig->origin[0], pSubShapeConfig->origin[1], pSubShapeConfig->origin[2]);

				Vector3GoldSrcToBullet(vecOrigin);

				EulerMatrix(vecAngles, localTrans.getBasis());

				localTrans.setOrigin(vecOrigin);

				pCompoundShape->addChildShape(localTrans, pCollisionShape);
			}
		}

		if (!pCompoundShape->getNumChildShapes())
		{
			gEngfuncs.Con_DPrintf("BulletCreateCollisionShapeInternal: childShapes cannot be empty!\n");

			OnBeforeDeleteBulletCollisionShape(pCompoundShape);

			delete pCompoundShape;

			pCompoundShape = nullptr;
		}

		pShape = pCompoundShape;
		break;
	}
	}

	return pShape;
}

btCollisionShape* BulletCreateCollisionShape(const CClientRigidBodyConfig* pRigidConfig)
{
	if (pRigidConfig->collisionShape)
	{
		const auto pShapeConfig = pRigidConfig->collisionShape.get();

		return BulletCreateCollisionShapeInternal(pShapeConfig);
	}
	else
	{
		gEngfuncs.Con_Printf("BulletCreateCollisionShape: no available collisionShape!\n");
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

		TransformGoldSrcToBullet(bonematrix);

		if (!(pRigidConfig->pboneindex >= 0 && pRigidConfig->pboneindex < CreationParam.m_studiohdr->numbones))
		{
			gEngfuncs.Con_Printf("BulletCreateMotionState: invalid pboneindex (%d).\n", pRigidConfig->pboneindex);
			return nullptr;
		}

		const auto& boneorigin = bonematrix.getOrigin();

		btVector3 pboneorigin((*pbonetransform)[pRigidConfig->pboneindex][0][3], (*pbonetransform)[pRigidConfig->pboneindex][1][3], (*pbonetransform)[pRigidConfig->pboneindex][2][3]);

		Vector3GoldSrcToBullet(pboneorigin);

		btVector3 vecDirection = pboneorigin - boneorigin;

		vecDirection = vecDirection.normalize();

		btScalar pboneoffset = pRigidConfig->pboneoffset;

		FloatGoldSrcToBullet(&pboneoffset);

		btVector3 vecWorldOrigin = boneorigin + vecDirection * pboneoffset;

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
		pRigidConfig->isLegacyConfig = false;
		Vec3BulletToGoldSrc(pRigidConfig->origin);

		return new CBulletBoneMotionState(pPhysicObject, bonematrix, localTrans);
	}

	btVector3 vecOrigin(pRigidConfig->origin[0], pRigidConfig->origin[1], pRigidConfig->origin[2]);
	
	Vector3GoldSrcToBullet(vecOrigin);

	btTransform localTrans(btQuaternion(0, 0, 0, 1), vecOrigin);

	btVector3 vecAngles(pRigidConfig->angles[0], pRigidConfig->angles[1], pRigidConfig->angles[2]);

	EulerMatrix(vecAngles, localTrans.getBasis());

	if (pRigidConfig->boneindex >= 0 && pRigidConfig->boneindex < CreationParam.m_studiohdr->numbones)
	{
		btTransform bonematrix;

		Matrix3x4ToTransform((*pbonetransform)[pRigidConfig->boneindex], bonematrix);

		TransformGoldSrcToBullet(bonematrix);

		return new CBulletBoneMotionState(pPhysicObject, bonematrix, localTrans);
	}
	else
	{
		return new CBulletEntityMotionState(pPhysicObject, localTrans);
	}

	return nullptr;
}

/*
	Upload GoldSrc origin and angles to bullet engine
*/
void CBulletEntityMotionState::getWorldTransform(btTransform& worldTrans) const
{
	auto ent = GetPhysicObject()->GetClientEntity();

	if (!GetInternalRigidBody() || GetInternalRigidBody()->isStaticOrKinematicObject())
	{
		btVector3 vecOrigin(ent->curstate.origin[0], ent->curstate.origin[1], ent->curstate.origin[2]);

		Vector3GoldSrcToBullet(vecOrigin);

		btTransform entityTrans;
		entityTrans.setIdentity();

		entityTrans.setOrigin(vecOrigin);

		btVector3 vecAngles(ent->curstate.angles[0], ent->curstate.angles[1], ent->curstate.angles[2]);

		//Brush uses reverted pitch
		if (ent->curstate.solid == SOLID_BSP)
		{
			vecAngles.setX(-vecAngles.x());
		}

		EulerMatrix(vecAngles, entityTrans.getBasis());

		worldTrans.mult(entityTrans, m_offsetmatrix);
	}
	else
	{
		if (m_worldTransformInitialized)
		{
			worldTrans = m_worldTransform;
		}
		else
		{
			btVector3 vecOrigin(ent->curstate.origin[0], ent->curstate.origin[1], ent->curstate.origin[2]);

			Vector3GoldSrcToBullet(vecOrigin);

			btTransform entityTrans;
			entityTrans.setIdentity();

			entityTrans.setOrigin(vecOrigin);

			btVector3 vecAngles(ent->curstate.angles[0], ent->curstate.angles[1], ent->curstate.angles[2]);

			//Brush uses reverted pitch
			if (ent->curstate.solid == SOLID_BSP)
			{
				vecAngles.setX(-vecAngles.x());
			}

			EulerMatrix(vecAngles, worldTrans.getBasis());

			m_worldTransform = worldTrans;
			m_worldTransformInitialized = true;
		}
	}
}

/*
	Download GoldSrc origin and angles from bullet engine
*/
void CBulletEntityMotionState::setWorldTransform(const btTransform& worldTrans)
{
	if (!GetInternalRigidBody() || GetInternalRigidBody()->isStaticOrKinematicObject())
		return;

	auto ent = GetPhysicObject()->GetClientEntity();

	GetPhysicObject()->EnumPhysicComponents([ent](IPhysicComponent* pPhysicComponent) {

		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			vec3_t origin;
			vec3_t angles;

			if (pRigidBody->GetGoldSrcOriginAngles(origin, angles))
			{
				VectorCopy(origin, ent->origin);
				VectorCopy(origin, ent->curstate.origin);

				VectorCopy(angles, ent->angles);
				VectorCopy(angles, ent->curstate.angles);

				return true;
			}
		}
		return false;
	});
}

void CFollowConstraintMotionState::getWorldTransform(btTransform& worldTrans) const
{
	btTransform worldPivotA;
	btTransform worldPivotB;
	if (BulletGetConstraintGlobalPivotTransform(m_pInternalConstraint, worldPivotA, worldPivotB))
	{
		if (m_attachToJointB)
		{
			worldTrans = worldPivotB;
		}
		else
		{
			worldTrans = worldPivotA;
		}
	}
}

void CFollowConstraintMotionState::setWorldTransform(const btTransform& worldTrans)
{

}

void CFollowPhysicComponentMotionState::getWorldTransform(btTransform& worldTrans) const
{
	auto pAttachedPhysicComponent = ClientPhysicManager()->GetPhysicComponent(m_attachedPhysicComponentId);

	if (!pAttachedPhysicComponent)
		return;

	if (pAttachedPhysicComponent->IsRigidBody())
	{
		auto pRigidBody = (IPhysicRigidBody *)pAttachedPhysicComponent;

		auto pInternalRigidBody = (btRigidBody *)pRigidBody->GetInternalRigidBody();

		worldTrans.mult(pInternalRigidBody->getWorldTransform(), m_offsetmatrix);
	}
	else if (pAttachedPhysicComponent->IsConstraint())
	{
		auto pConstraint = (IPhysicConstraint*)pAttachedPhysicComponent;

		auto pInternalConstraint = (btTypedConstraint*)pConstraint->GetInternalConstraint();

		btTransform worldPivotA;
		btTransform worldPivotB;

		if (BulletGetConstraintGlobalPivotTransform(pInternalConstraint, worldPivotA, worldPivotB))
		{
			//attachToJointB?

			worldTrans.mult(worldPivotA, m_offsetmatrix);
		}
	}
}

void CFollowPhysicComponentMotionState::setWorldTransform(const btTransform& worldTrans)
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
		}

		gEngfuncs.pTriAPI->Color4fRendermode(color1.getX(), color1.getY(), color1.getZ(), 1, kRenderTransAlpha);

		gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);

		gEngfuncs.pTriAPI->Begin(TRI_LINES);

		vec3_t vecFromGoldSrc = { from1.getX(), from1.getY(), from1.getZ() };
		vec3_t vecToGoldSrc = { to1.getX(), to1.getY(), to1.getZ() };

		Vec3BulletToGoldSrc(vecFromGoldSrc);
		Vec3BulletToGoldSrc(vecToGoldSrc);

		gEngfuncs.pTriAPI->Vertex3fv(vecFromGoldSrc);
		gEngfuncs.pTriAPI->Vertex3fv(vecToGoldSrc);
		gEngfuncs.pTriAPI->End();

		if (IsDebugDrawWallHackEnabled())
		{
			glEnable(GL_DEPTH_TEST);
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

			auto pInternalRigidBodyA = btRigidBody::upcast(pClientObjectA);
			auto pInternalRigidBodyB = btRigidBody::upcast(pClientObjectB);

			IPhysicComponent* pPhysicComponentA{};
			IPhysicComponent* pPhysicComponentB{};

			IPhysicObject* pPhysicObjectA{};
			IPhysicObject* pPhysicObjectB{};

			if (pInternalRigidBodyA)
			{
				pPhysicComponentA = (IPhysicComponent*)pInternalRigidBodyA->getUserPointer();
				pPhysicObjectA = pPhysicComponentA->GetOwnerPhysicObject();

				if (pPhysicObjectA && pPhysicObjectA->IsClientEntityNonSolid()) {
					return false;
				}
			}

			if (pInternalRigidBodyB)
			{
				pPhysicComponentB = (IPhysicComponent*)pInternalRigidBodyB->getUserPointer();
				pPhysicObjectB = pPhysicComponentB->GetOwnerPhysicObject();

				if (pPhysicObjectB && pPhysicObjectB->IsClientEntityNonSolid()) {
					return false;
				}
			}
		}
		return collides;
	}
};

class CBulletDiscreteDynamicsWorld : public btDiscreteDynamicsWorld
{
public:
	CBulletDiscreteDynamicsWorld(btDispatcher* dispatcher, btBroadphaseInterface* pairCache, btConstraintSolver* constraintSolver, btCollisionConfiguration* collisionConfiguration) :
		btDiscreteDynamicsWorld(dispatcher, pairCache, constraintSolver, collisionConfiguration)
	{

	}

	void debugDrawConstraint(btTypedConstraint* constraint) override
	{
		const auto debugDrawContext = ClientPhysicManager()->GetDebugDrawContext();

		bool drawFrames = (getDebugDrawer()->getDebugMode() & btIDebugDraw::DBG_DrawConstraints) != 0;
		bool drawLimits = (getDebugDrawer()->getDebugMode() & btIDebugDraw::DBG_DrawConstraintLimits) != 0;

		btScalar dbgDrawSize = constraint->getDbgDrawSize();

		if (dbgDrawSize <= btScalar(0.f))
			return;

		auto pPhysicComponent = (IPhysicComponent *)constraint->getUserConstraintPtr();

		if (!pPhysicComponent)
			return;

		auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

		if (!pPhysicObject)
			return;

		if (!pPhysicObject->ShouldDrawOnDebugDraw(debugDrawContext) || !pPhysicComponent->ShouldDrawOnDebugDraw(debugDrawContext))
			return;

		btVector3 color = GetVector3FromVec3(debugDrawContext->m_constraintColor);

		if (ClientPhysicManager()->GetSelectedPhysicComponentId() == pPhysicComponent->GetPhysicComponentId())
		{
			color = GetVector3FromVec3(debugDrawContext->m_selectedColor);
		}
		else if (ClientPhysicManager()->GetInspectedPhysicComponentId() == pPhysicComponent->GetPhysicComponentId())
		{
			color = GetVector3FromVec3(debugDrawContext->m_inspectedColor);
		}

		switch (constraint->getConstraintType())
		{
		case POINT2POINT_CONSTRAINT_TYPE:
		{
			btPoint2PointConstraint* p2pC = (btPoint2PointConstraint*)constraint;
			btTransform tr;
			tr.setIdentity();
			btVector3 pivot = p2pC->getPivotInA();
			pivot = p2pC->getRigidBodyA().getCenterOfMassTransform() * pivot;
			tr.setOrigin(pivot);
			getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			// that ideally should draw the same frame
			pivot = p2pC->getPivotInB();
			pivot = p2pC->getRigidBodyB().getCenterOfMassTransform() * pivot;
			tr.setOrigin(pivot);
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
		}
		break;
		case HINGE_CONSTRAINT_TYPE:
		{
			btHingeConstraint* pHinge = (btHingeConstraint*)constraint;
			btTransform tr = pHinge->getRigidBodyA().getCenterOfMassTransform() * pHinge->getAFrame();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			tr = pHinge->getRigidBodyB().getCenterOfMassTransform() * pHinge->getBFrame();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			btScalar minAng = pHinge->getLowerLimit();
			btScalar maxAng = pHinge->getUpperLimit();
			if (minAng == maxAng)
			{
				break;
			}
			bool drawSect = true;
			if (!pHinge->hasLimit())
			{
				minAng = btScalar(0.f);
				maxAng = SIMD_2_PI;
				drawSect = false;
			}
			if (drawLimits)
			{
				btVector3& center = tr.getOrigin();
				btVector3 normal = tr.getBasis().getColumn(2);
				btVector3 axis = tr.getBasis().getColumn(0);
				getDebugDrawer()->drawArc(center, normal, axis, dbgDrawSize, dbgDrawSize, minAng, maxAng, color, drawSect);
			}
		}
		break;
		case CONETWIST_CONSTRAINT_TYPE:
		{
			btConeTwistConstraint* pCT = (btConeTwistConstraint*)constraint;
			btTransform tr = pCT->getRigidBodyA().getCenterOfMassTransform() * pCT->getAFrame();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			tr = pCT->getRigidBodyB().getCenterOfMassTransform() * pCT->getBFrame();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			if (drawLimits)
			{
				//const btScalar length = btScalar(5);
				const btScalar length = dbgDrawSize;
				static int nSegments = 8 * 4;
				btScalar fAngleInRadians = btScalar(2. * 3.1415926) * (btScalar)(nSegments - 1) / btScalar(nSegments);
				btVector3 pPrev = pCT->GetPointForAngle(fAngleInRadians, length);
				pPrev = tr * pPrev;
				for (int i = 0; i < nSegments; i++)
				{
					fAngleInRadians = btScalar(2. * 3.1415926) * (btScalar)i / btScalar(nSegments);
					btVector3 pCur = pCT->GetPointForAngle(fAngleInRadians, length);
					pCur = tr * pCur;
					getDebugDrawer()->drawLine(pPrev, pCur, color);

					if (i % (nSegments / 8) == 0)
						getDebugDrawer()->drawLine(tr.getOrigin(), pCur, color);

					pPrev = pCur;
				}
				btScalar tws = pCT->getTwistSpan();
				btScalar twa = pCT->getTwistAngle();
				bool useFrameB = (pCT->getRigidBodyB().getInvMass() > btScalar(0.f));
				if (useFrameB)
				{
					tr = pCT->getRigidBodyB().getCenterOfMassTransform() * pCT->getBFrame();
				}
				else
				{
					tr = pCT->getRigidBodyA().getCenterOfMassTransform() * pCT->getAFrame();
				}
				btVector3 pivot = tr.getOrigin();
				btVector3 normal = tr.getBasis().getColumn(0);
				btVector3 axis1 = tr.getBasis().getColumn(1);
				getDebugDrawer()->drawArc(pivot, normal, axis1, dbgDrawSize, dbgDrawSize, -twa - tws, -twa + tws, color, true);
			}
		}
		break;
		case D6_SPRING_CONSTRAINT_TYPE:
		case D6_CONSTRAINT_TYPE:
		{
			btGeneric6DofConstraint* p6DOF = (btGeneric6DofConstraint*)constraint;
			btTransform tr = p6DOF->getCalculatedTransformA();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			tr = p6DOF->getCalculatedTransformB();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			if (drawLimits)
			{
				tr = p6DOF->getCalculatedTransformA();
				const btVector3& center = p6DOF->getCalculatedTransformB().getOrigin();
				btVector3 up = tr.getBasis().getColumn(2);
				btVector3 axis = tr.getBasis().getColumn(0);
				btScalar minTh = p6DOF->getRotationalLimitMotor(1)->m_loLimit;
				btScalar maxTh = p6DOF->getRotationalLimitMotor(1)->m_hiLimit;
				btScalar minPs = p6DOF->getRotationalLimitMotor(2)->m_loLimit;
				btScalar maxPs = p6DOF->getRotationalLimitMotor(2)->m_hiLimit;
				getDebugDrawer()->drawSpherePatch(center, up, axis, dbgDrawSize * btScalar(.9f), minTh, maxTh, minPs, maxPs, color);
				axis = tr.getBasis().getColumn(1);
				btScalar ay = p6DOF->getAngle(1);
				btScalar az = p6DOF->getAngle(2);
				btScalar cy = btCos(ay);
				btScalar sy = btSin(ay);
				btScalar cz = btCos(az);
				btScalar sz = btSin(az);
				btVector3 ref;
				ref[0] = cy * cz * axis[0] + cy * sz * axis[1] - sy * axis[2];
				ref[1] = -sz * axis[0] + cz * axis[1];
				ref[2] = cz * sy * axis[0] + sz * sy * axis[1] + cy * axis[2];
				tr = p6DOF->getCalculatedTransformB();
				btVector3 normal = -tr.getBasis().getColumn(0);
				btScalar minFi = p6DOF->getRotationalLimitMotor(0)->m_loLimit;
				btScalar maxFi = p6DOF->getRotationalLimitMotor(0)->m_hiLimit;
				if (minFi > maxFi)
				{
					getDebugDrawer()->drawArc(center, normal, ref, dbgDrawSize, dbgDrawSize, -SIMD_PI, SIMD_PI, color, false);
				}
				else if (minFi < maxFi)
				{
					getDebugDrawer()->drawArc(center, normal, ref, dbgDrawSize, dbgDrawSize, minFi, maxFi, color, true);
				}
				tr = p6DOF->getCalculatedTransformA();
				btVector3 bbMin = p6DOF->getTranslationalLimitMotor()->m_lowerLimit;
				btVector3 bbMax = p6DOF->getTranslationalLimitMotor()->m_upperLimit;
				getDebugDrawer()->drawBox(bbMin, bbMax, tr, color);
			}
		}
		break;
		///note: the code for D6_SPRING_2_CONSTRAINT_TYPE is identical to D6_CONSTRAINT_TYPE, the D6_CONSTRAINT_TYPE+D6_SPRING_CONSTRAINT_TYPE will likely become obsolete/deprecated at some stage
		case D6_SPRING_2_CONSTRAINT_TYPE:
		{
			{
				btGeneric6DofSpring2Constraint* p6DOF = (btGeneric6DofSpring2Constraint*)constraint;
				btTransform tr = p6DOF->getCalculatedTransformA();
				if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
				tr = p6DOF->getCalculatedTransformB();
				if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
				if (drawLimits)
				{
					tr = p6DOF->getCalculatedTransformA();
					const btVector3& center = p6DOF->getCalculatedTransformB().getOrigin();
					btVector3 up = tr.getBasis().getColumn(2);
					btVector3 axis = tr.getBasis().getColumn(0);
					btScalar minTh = p6DOF->getRotationalLimitMotor(1)->m_loLimit;
					btScalar maxTh = p6DOF->getRotationalLimitMotor(1)->m_hiLimit;
					if (minTh <= maxTh)
					{
						btScalar minPs = p6DOF->getRotationalLimitMotor(2)->m_loLimit;
						btScalar maxPs = p6DOF->getRotationalLimitMotor(2)->m_hiLimit;
						getDebugDrawer()->drawSpherePatch(center, up, axis, dbgDrawSize * btScalar(.9f), minTh, maxTh, minPs, maxPs, color);
					}
					axis = tr.getBasis().getColumn(1);
					btScalar ay = p6DOF->getAngle(1);
					btScalar az = p6DOF->getAngle(2);
					btScalar cy = btCos(ay);
					btScalar sy = btSin(ay);
					btScalar cz = btCos(az);
					btScalar sz = btSin(az);
					btVector3 ref;
					ref[0] = cy * cz * axis[0] + cy * sz * axis[1] - sy * axis[2];
					ref[1] = -sz * axis[0] + cz * axis[1];
					ref[2] = cz * sy * axis[0] + sz * sy * axis[1] + cy * axis[2];
					tr = p6DOF->getCalculatedTransformB();
					btVector3 normal = -tr.getBasis().getColumn(0);
					btScalar minFi = p6DOF->getRotationalLimitMotor(0)->m_loLimit;
					btScalar maxFi = p6DOF->getRotationalLimitMotor(0)->m_hiLimit;
					if (minFi > maxFi)
					{
						getDebugDrawer()->drawArc(center, normal, ref, dbgDrawSize, dbgDrawSize, -SIMD_PI, SIMD_PI, color, false);
					}
					else if (minFi < maxFi)
					{
						getDebugDrawer()->drawArc(center, normal, ref, dbgDrawSize, dbgDrawSize, minFi, maxFi, color, true);
					}
					tr = p6DOF->getCalculatedTransformA();
					btVector3 bbMin = p6DOF->getTranslationalLimitMotor()->m_lowerLimit;
					btVector3 bbMax = p6DOF->getTranslationalLimitMotor()->m_upperLimit;
					getDebugDrawer()->drawBox(bbMin, bbMax, tr, color);
				}
			}
			break;
		}
		case SLIDER_CONSTRAINT_TYPE:
		{
			btSliderConstraint* pSlider = (btSliderConstraint*)constraint;
			btTransform tr = pSlider->getCalculatedTransformA();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			tr = pSlider->getCalculatedTransformB();
			if (drawFrames) getDebugDrawer()->drawTransform(tr, dbgDrawSize);
			if (drawLimits)
			{
				btTransform tr = pSlider->getUseLinearReferenceFrameA() ? pSlider->getCalculatedTransformA() : pSlider->getCalculatedTransformB();
				btVector3 li_min = tr * btVector3(pSlider->getLowerLinLimit(), 0.f, 0.f);
				btVector3 li_max = tr * btVector3(pSlider->getUpperLinLimit(), 0.f, 0.f);
				getDebugDrawer()->drawLine(li_min, li_max, color);
				btVector3 normal = tr.getBasis().getColumn(0);
				btVector3 axis = tr.getBasis().getColumn(1);
				btScalar a_min = pSlider->getLowerAngLimit();
				btScalar a_max = pSlider->getUpperAngLimit();
				const btVector3& center = pSlider->getCalculatedTransformB().getOrigin();
				getDebugDrawer()->drawArc(center, normal, axis, dbgDrawSize, dbgDrawSize, a_min, a_max, color, true);
			}
		}
		break;
		default:
			break;
		}
		return;
	}

};


void CBulletPhysicManager::Init(void)
{
	CBasePhysicManager::Init();

	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_overlappingPairCache = new btDbvtBroadphase();
	m_solver = new btSequentialImpulseConstraintSolver;
	m_dynamicsWorld = new CBulletDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

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
	m_debugDrawContext.m_staticObjectLevel = (int)bv_debug_draw_level_static->value;
	m_debugDrawContext.m_dynamicObjectLevel = (int)bv_debug_draw_level_dynamic->value;
	m_debugDrawContext.m_ragdollObjectLevel = (int)bv_debug_draw_level_ragdoll->value;
	m_debugDrawContext.m_rigidbodyLevel = (int)bv_debug_draw_level_rigidbody->value;
	m_debugDrawContext.m_constraintLevel = (int)bv_debug_draw_level_constraint->value;
	m_debugDrawContext.m_actionLevel = (int)bv_debug_draw_level_action->value;

	if (!UTIL_ParseStringAsColor3(bv_debug_draw_constraint_color->string, m_debugDrawContext.m_constraintColor))
	{
		VectorClear(m_debugDrawContext.m_constraintColor);
	}
	if (!UTIL_ParseStringAsColor3(bv_debug_draw_inspected_color->string, m_debugDrawContext.m_inspectedColor))
	{
		VectorClear(m_debugDrawContext.m_inspectedColor);
	}
	if (!UTIL_ParseStringAsColor3(bv_debug_draw_selected_color->string, m_debugDrawContext.m_selectedColor))
	{
		VectorClear(m_debugDrawContext.m_selectedColor);
	}

	const auto &objectArray = m_dynamicsWorld->getCollisionObjectArray();

	for (int i = 0;i < objectArray.size(); ++i)
	{
		auto pCollisionObject = objectArray[i];

		auto pInternalRigidBody = btRigidBody::upcast(pCollisionObject);
		
		if (pInternalRigidBody && !(pInternalRigidBody->getCollisionFlags() & BulletPhysicCollisionFlags::CF_DISABLE_VISUALIZE_OBJECT_PERMANENT))
		{
			auto physicComponentId = pInternalRigidBody->getUserIndex();
			auto pPhysicComponent = GetPhysicComponent(physicComponentId);

			if (pPhysicComponent)
			{
				auto entindex = pPhysicComponent->GetOwnerEntityIndex();
				auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();
				
				if (pPhysicObject)
				{
					if ((pPhysicObject->ShouldDrawOnDebugDraw(&m_debugDrawContext) && pPhysicComponent->ShouldDrawOnDebugDraw(&m_debugDrawContext)))
					{
						int iCollisionFlags = pInternalRigidBody->getCollisionFlags();
						iCollisionFlags &= ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
						pInternalRigidBody->setCollisionFlags(iCollisionFlags);
					}
					else
					{
						int iCollisionFlags = pInternalRigidBody->getCollisionFlags();
						iCollisionFlags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
						pInternalRigidBody->setCollisionFlags(iCollisionFlags);
					}

					if ((m_selectedPhysicComponentId && m_selectedPhysicComponentId == physicComponentId) || (m_selectedPhysicObjectId && entindex == UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(m_selectedPhysicObjectId)))
					{
						auto customColor = GetVector3FromVec3(m_debugDrawContext.m_selectedColor);

						pInternalRigidBody->setCustomDebugColor(customColor);
					}
					else if ((m_inspectedPhysicComponentId && m_inspectedPhysicComponentId == physicComponentId) || (m_inspectedPhysicObjectId && entindex == UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(m_inspectedPhysicObjectId)))
					{
						auto customColor = GetVector3FromVec3(m_debugDrawContext.m_inspectedColor);

						pInternalRigidBody->setCustomDebugColor(customColor);
					}
					else
					{
						pInternalRigidBody->removeCustomDebugColor();
					}
				}
			}
		}
	}
#if 0
	auto numConstraint = m_dynamicsWorld->getNumConstraints();

	for (int i = 0; i < numConstraint; ++i)
	{
		auto pInternalConstraint = m_dynamicsWorld->getConstraint(i);

		auto physicComponentId = pInternalConstraint->getUserConstraintId();

		auto pPhysicComponent = GetPhysicComponent(physicComponentId);

		if (pPhysicComponent)
		{
			auto entindex = pPhysicComponent->GetOwnerEntityIndex();
			auto pPhysicObject = pPhysicComponent->GetOwnerPhysicObject();

			if (pPhysicObject)
			{
				if (pPhysicObject->ShouldDrawOnDebugDraw(&m_debugDrawContext) && pPhysicComponent->ShouldDrawOnDebugDraw(&m_debugDrawContext))
				{
					float drawSize = 3;

					FloatGoldSrcToBullet(&drawSize);

					pInternalConstraint->setDbgDrawSize(drawSize);
				}
				else
				{
					pInternalConstraint->setDbgDrawSize(0);
				}
			}
		}
	}
#endif
	m_dynamicsWorld->debugDrawWorld();
}

void CBulletPhysicManager::SetGravity(float velocity)
{
	CBasePhysicManager::SetGravity(velocity);

	btVector3 vecGravity(0, 0, -m_gravity);

	Vector3GoldSrcToBullet(vecGravity);

	m_dynamicsWorld->setGravity(vecGravity);
}

void CBulletPhysicManager::StepSimulation(double frametime)
{
	CBasePhysicManager::StepSimulation(frametime);

	if (frametime <= 0)
		return;

	m_dynamicsWorld->stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate());
}

void CBulletPhysicManager::AddPhysicComponentsToWorld(IPhysicObject *PhysicObject, const CPhysicComponentFilters& filters)
{
	PhysicObject->AddPhysicComponentsToPhysicWorld(m_dynamicsWorld, filters);
}

void CBulletPhysicManager::RemovePhysicComponentsFromWorld(IPhysicObject* PhysicObject, const CPhysicComponentFilters& filters)
{
	PhysicObject->RemovePhysicComponentsFromPhysicWorld(m_dynamicsWorld, filters);
}

void CBulletPhysicManager::AddPhysicComponentToWorld(IPhysicComponent* pPhysicComponent)
{
	pPhysicComponent->AddToPhysicWorld(m_dynamicsWorld);
}

void CBulletPhysicManager::RemovePhysicComponentFromWorld(IPhysicComponent* pPhysicComponent)
{
	pPhysicComponent->RemoveFromPhysicWorld(m_dynamicsWorld);
}

class CTraceLineRayTestCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
	btVector3 m_vecStartGoldSrc{  };
	btVector3 m_vecEndGoldSrc{  };

	CTraceLineRayTestCallback(const CPhysicTraceLineParameters& traceParam) : btCollisionWorld::ClosestRayResultCallback(btVector3(), btVector3())
	{
		m_vecStartGoldSrc = GetVector3FromVec3(traceParam.vecStart);
		m_vecEndGoldSrc = GetVector3FromVec3(traceParam.vecEnd);

		m_rayFromWorld = m_vecStartGoldSrc;
		m_rayToWorld = m_vecEndGoldSrc;

		Vector3GoldSrcToBullet(m_rayFromWorld);
		Vector3GoldSrcToBullet(m_rayToWorld);

		m_collisionFilterGroup = BulletPhysicCollisionFilterGroups::InspectorFilter;

		if (traceParam.withflags != -1)
		{
			m_collisionFilterMask = 0;

			if (traceParam.withflags & PhysicTraceLineFlag_RigidBody)
			{
				m_collisionFilterMask |= btBroadphaseProxy::DefaultFilter;
			}
			if (traceParam.withflags & PhysicTraceLineFlag_Constraint)
			{
				m_collisionFilterMask |= BulletPhysicCollisionFilterGroups::ConstraintFilter;
			}
			if (traceParam.withflags & PhysicTraceLineFlag_Action)
			{
				m_collisionFilterMask |= BulletPhysicCollisionFilterGroups::ActionFilter;
			}
			if (traceParam.withflags & PhysicTraceLineFlag_World)
			{
				m_collisionFilterMask |= BulletPhysicCollisionFilterGroups::WorldFilter;
			}
			if (traceParam.withflags & PhysicTraceLineFlag_StaticObject)
			{
				m_collisionFilterMask |= BulletPhysicCollisionFilterGroups::StaticObjectFilter;
			}
			if (traceParam.withflags & PhysicTraceLineFlag_DynamicObject)
			{
				m_collisionFilterMask |= BulletPhysicCollisionFilterGroups::DynamicObjectFilter;
			}
			if (traceParam.withflags & PhysicTraceLineFlag_RagdollObject)
			{
				m_collisionFilterMask |= BulletPhysicCollisionFilterGroups::RagdollObjectFilter;
			}
		}
		if (traceParam.withoutflags != 0)
		{
			if (traceParam.withoutflags & PhysicTraceLineFlag_RigidBody)
			{
				m_collisionFilterMask &= ~btBroadphaseProxy::DefaultFilter;
			}
			if (traceParam.withoutflags & PhysicTraceLineFlag_Constraint)
			{
				m_collisionFilterMask &= ~BulletPhysicCollisionFilterGroups::ConstraintFilter;
			}
			if (traceParam.withoutflags & PhysicTraceLineFlag_Action)
			{
				m_collisionFilterMask &= ~BulletPhysicCollisionFilterGroups::ActionFilter;
			}
			if (traceParam.withoutflags & PhysicTraceLineFlag_World)
			{
				m_collisionFilterMask &= ~BulletPhysicCollisionFilterGroups::WorldFilter;
			}
			if (traceParam.withoutflags & PhysicTraceLineFlag_StaticObject)
			{
				m_collisionFilterMask &= ~BulletPhysicCollisionFilterGroups::StaticObjectFilter;
			}
			if (traceParam.withoutflags & PhysicTraceLineFlag_DynamicObject)
			{
				m_collisionFilterMask &= ~BulletPhysicCollisionFilterGroups::DynamicObjectFilter;
			}
			if (traceParam.withoutflags & PhysicTraceLineFlag_RagdollObject)
			{
				m_collisionFilterMask &= ~BulletPhysicCollisionFilterGroups::RagdollObjectFilter;
			}
		}
	}

	bool needsCollision(btBroadphaseProxy* proxy0) const override
	{
		bool collides = (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
		collides = collides && (m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		if (collides)
		{
			auto pClientObjectA = (btCollisionObject*)proxy0->m_clientObject;

			auto pInternalRigidBodyA = btRigidBody::upcast(pClientObjectA);

			IPhysicComponent* pPhysicComponentA{};

			IPhysicObject* pPhysicObjectA{};

			if (pInternalRigidBodyA)
			{
				pPhysicComponentA = (IPhysicComponent*)pInternalRigidBodyA->getUserPointer();

				if (pPhysicComponentA)
				{
					pPhysicObjectA = pPhysicComponentA->GetOwnerPhysicObject();

					if (!pPhysicObjectA->ShouldDrawOnDebugDraw(ClientPhysicManager()->GetDebugDrawContext()))
						return false;

					if (!pPhysicComponentA->ShouldDrawOnDebugDraw(ClientPhysicManager()->GetDebugDrawContext()))
						return false;
				}
			}
		}

		return collides;
	}
};

void CBulletPhysicManager::TraceLine(const CPhysicTraceLineParameters& traceParam, CPhysicTraceLineHitResult& hitResult)
{
	CTraceLineRayTestCallback rayCallback(traceParam);
	
	m_dynamicsWorld->rayTest(rayCallback.m_rayFromWorld, rayCallback.m_rayToWorld, rayCallback);

	hitResult.m_bHasHit = rayCallback.hasHit();

	if (hitResult.m_bHasHit)
	{
		hitResult.m_flHitFraction = rayCallback.m_closestHitFraction;
		hitResult.m_flDistanceToHitPoint = rayCallback.m_closestHitFraction * (rayCallback.m_vecStartGoldSrc - rayCallback.m_vecEndGoldSrc).length();

		FloatBulletToGoldSrc(&hitResult.m_flDistanceToHitPoint);

		hitResult.m_vecHitPoint[0] = rayCallback.m_hitPointWorld.getX();
		hitResult.m_vecHitPoint[1] = rayCallback.m_hitPointWorld.getY();
		hitResult.m_vecHitPoint[2] = rayCallback.m_hitPointWorld.getZ();

		Vec3BulletToGoldSrc(hitResult.m_vecHitPoint);

		hitResult.m_vecHitPlaneNormal[0] = rayCallback.m_hitNormalWorld.getX();
		hitResult.m_vecHitPlaneNormal[1] = rayCallback.m_hitNormalWorld.getY();
		hitResult.m_vecHitPlaneNormal[2] = rayCallback.m_hitNormalWorld.getZ();

		auto pRigidBody = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);

		if (pRigidBody)
		{
			auto pPhysicComponent = GetPhysicComponent(pRigidBody->getUserIndex());

			if (pPhysicComponent)
			{
				hitResult.m_iHitEntityIndex = pPhysicComponent->GetOwnerEntityIndex();
				hitResult.m_iHitPhysicComponentIndex = pPhysicComponent->GetPhysicComponentId();
			}
		}
	}
}

IStaticObject* CBulletPhysicManager::CreateStaticObject(const CPhysicObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_pPhysicObjectConfig)
	{
		gEngfuncs.Con_Printf("CreateStaticObject: invalid m_pPhysicObjectConfig!\n");
		return nullptr;
	}

	return new CBulletStaticObject(CreationParam);
}

IDynamicObject* CBulletPhysicManager::CreateDynamicObject(const CPhysicObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_pPhysicObjectConfig)
	{
		gEngfuncs.Con_Printf("CreateRagdollObject: invalid m_pPhysicObjectConfig!\n");
		return nullptr;
	}

	return new CBulletDynamicObject(CreationParam);
}

IRagdollObject* CBulletPhysicManager::CreateRagdollObject(const CPhysicObjectCreationParameter& CreationParam)
{
	if (!CreationParam.m_pPhysicObjectConfig)
	{
		gEngfuncs.Con_Printf("CreateRagdollObject: invalid m_pPhysicObjectConfig!\n");
		return nullptr;
	}

	if (!CreationParam.m_studiohdr)
	{
		gEngfuncs.Con_Printf("CreateRagdollObject: invalid m_studiohdr!\n");
		return nullptr;
	}

	return new CBulletRagdollObject(CreationParam);
}

IClientPhysicManager* BulletPhysicManager_CreateInstance()
{
	return new CBulletPhysicManager();
}