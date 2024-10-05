#pragma once

#include "BulletGargantuaDragOnConstraintBehavior.h"

CBulletGargantuaDragOnConstraintBehavior::CBulletGargantuaDragOnConstraintBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iSourceIndex,
	float flForceMagnitude,
	float flTargetVelocity,
	float flExtraHeight,
	int iLimitAxis,
	bool bUseServoMotor) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iSourceIndex(iSourceIndex),
	m_flForceMagnitude(flForceMagnitude),
	m_flTargetVelocity(flTargetVelocity),
	m_flExtraHeight(flExtraHeight),
	m_iLimitAxis(iLimitAxis),
	m_bUseServoMotor(bUseServoMotor)
{

}

const char* CBulletGargantuaDragOnConstraintBehavior::GetTypeString() const
{
	return "GargantuaDragOnConstraint";
}

const char* CBulletGargantuaDragOnConstraintBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_GargantuaDragOnConstraint";
}

void CBulletGargantuaDragOnConstraintBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pSourceObject = ClientPhysicManager()->GetPhysicObject(m_iSourceIndex);

	if (!pSourceObject)
	{
		gEngfuncs.Con_DPrintf("CBulletGargantuaDragOnConstraintBehavior::Update: Invalid barnacle object!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!pSourceObject->IsRagdollObject())
	{
		gEngfuncs.Con_DPrintf("CBulletGargantuaDragOnConstraintBehavior::Update: SourceObject must be RagdollObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pSourceObject->GetObjectFlags() & PhysicObjectFlag_Gargantua))
	{
		gEngfuncs.Con_DPrintf("CBulletGargantuaDragOnConstraintBehavior::Update: SourceObject must have PhysicObjectFlag_Gargantua!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pConstraint = GetAttachedConstraint();

	if (!pConstraint)
	{
		gEngfuncs.Con_DPrintf("CBulletGargantuaDragOnConstraintBehavior::Update: The attached PhysicComponent must be a constraint!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pSourceRagdollObject = (IRagdollObject*)pSourceObject;

	if (!(m_iLimitAxis >= 0 && m_iLimitAxis < 3))
	{
		gEngfuncs.Con_DPrintf("CBulletGargantuaDragOnConstraintBehavior::Update: The m_iLimitAxis must be ranged from 0~2!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (pSourceRagdollObject->GetActivityType() == StudioAnimActivityType_GargantuaBite)
	{
		auto pInternalConstraint = (btTypedConstraint*)pConstraint->GetInternalConstraint();

		if (pInternalConstraint->getConstraintType() == D6_SPRING_2_CONSTRAINT_TYPE)
		{
			auto pDof6Spring = (btGeneric6DofSpring2Constraint*)pInternalConstraint;

			if (m_iLimitAxis >= 0 && m_iLimitAxis < 3)
			{
				const auto& tr = pDof6Spring->getCalculatedTransformA();
				const btVector3& bbMin = pDof6Spring->getTranslationalLimitMotor()->m_lowerLimit;
				const btVector3& bbMax = pDof6Spring->getTranslationalLimitMotor()->m_upperLimit;

				btVector3 vStartOrigin;
				btScalar flLimitValue{};
				btScalar flLimitSign{};

				if (bbMin[m_iLimitAxis] < -0.01f || bbMax[m_iLimitAxis] > 0.01f)
				{
					flLimitValue = fabs(bbMin[m_iLimitAxis]);
					flLimitSign = bbMin[m_iLimitAxis] < 0 ? -1 : 1;
					vStartOrigin = tr * bbMin;

					pDof6Spring->enableMotor(m_iLimitAxis, true);
					pDof6Spring->setMaxMotorForce(m_iLimitAxis, m_flForceMagnitude);
					pDof6Spring->setTargetVelocity(m_iLimitAxis, m_flTargetVelocity);

					if (m_bUseServoMotor)
					{
						pDof6Spring->setServo(m_iLimitAxis, true);
						pDof6Spring->setServoTarget(m_iLimitAxis, flLimitValue * flLimitSign);
					}
				}
				else if (bbMax[m_iLimitAxis] < -0.01f || bbMax[m_iLimitAxis] > 0.01f)
				{
					flLimitValue = fabs(bbMax[m_iLimitAxis]);
					flLimitSign = bbMax[m_iLimitAxis] < 0 ? -1 : 1;
					vStartOrigin = tr * bbMax;

					pDof6Spring->enableMotor(m_iLimitAxis, true);
					pDof6Spring->setMaxMotorForce(m_iLimitAxis, m_flForceMagnitude);
					pDof6Spring->setTargetVelocity(m_iLimitAxis, m_flTargetVelocity);

					if (m_bUseServoMotor)
					{
						pDof6Spring->setServo(m_iLimitAxis, true);
						pDof6Spring->setServoTarget(m_iLimitAxis, flLimitValue * flLimitSign);
					}
				}
			}
		}
		else
		{
			gEngfuncs.Con_DPrintf("CBulletGargantuaDragOnConstraintBehavior::Update: The attached constraint must be DofSpring!\n");
			ComponentContext->m_bShouldFree = true;
			return;
		}
	}
}