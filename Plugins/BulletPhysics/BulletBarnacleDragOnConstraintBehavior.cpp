#pragma once

#include "BulletBarnacleDragOnConstraintBehavior.h"

CBulletBarnacleDragOnConstraintBehavior::CBulletBarnacleDragOnConstraintBehavior(
	int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig,
	int attachedPhysicComponentId,
	int iBarnacleIndex, 
	float flForceMagnitude,
	float flTargetVelocity, 
	float flExtraHeight,
	int iLimitAxis,
	bool bCalculateLimitFromActualPlayerOrigin,
	bool bUseServoMotor,
	bool bActivatedOnBarnaclePulling,
	bool bActivatedOnBarnacleChewing) :

	CBulletPhysicComponentBehavior(
		id,
		entindex,
		pPhysicObject,
		pPhysicBehaviorConfig,
		attachedPhysicComponentId),

	m_iBarnacleIndex(iBarnacleIndex),
	m_flForceMagnitude(flForceMagnitude),
	m_flTargetVelocity(flTargetVelocity),
	m_flExtraHeight(flExtraHeight),
	m_iLimitAxis(iLimitAxis),
	m_bCalculateLimitFromActualPlayerOrigin(bCalculateLimitFromActualPlayerOrigin),
	m_bUseServoMotor(bUseServoMotor),
	m_bActivatedOnBarnaclePulling( bActivatedOnBarnaclePulling),
	m_bActivatedOnBarnacleChewing(bActivatedOnBarnacleChewing)
{

}

const char* CBulletBarnacleDragOnConstraintBehavior::GetTypeString() const
{
	return "BarnacleDragOnConstraint";
}

const char* CBulletBarnacleDragOnConstraintBehavior::GetTypeLocalizationTokenString() const
{
	return "#BulletPhysics_BarnacleDragOnConstraint";
}

float CBulletBarnacleDragOnConstraintBehavior::CalculateLimitValueFromActualPlayerOrigin(const btVector3 &vStartOrigin)
{
	vec3_t vecClientEntityOrigin{ 0 };

	VectorCopy(m_pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

	btVector3 ClientEntityOrigin = GetVector3FromVec3(vecClientEntityOrigin);

	Vector3GoldSrcToBullet(ClientEntityOrigin);

	return (ClientEntityOrigin - vStartOrigin).length() + m_flExtraHeight;
}

void CBulletBarnacleDragOnConstraintBehavior::Update(CPhysicComponentUpdateContext* ComponentContext)
{
	auto pBarnacleObject = ClientPhysicManager()->GetPhysicObject(m_iBarnacleIndex);

	if (!pBarnacleObject)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnConstraintBehavior::Update: Invalid barnacle object!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!pBarnacleObject->IsRagdollObject())
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnConstraintBehavior::Update: Barnacle must be RagdollObject!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if (!(pBarnacleObject->GetObjectFlags() & PhysicObjectFlag_Barnacle))
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnConstraintBehavior::Update: Barnacle must have PhysicObjectFlag_Barnacle!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pConstraint = GetAttachedConstraint();

	if (!pConstraint)
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnConstraintBehavior::Update: The attached PhysicComponent must be a constraint!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	auto pBarnacleRagdollObject = (IRagdollObject*)pBarnacleObject;

	if (!(m_iLimitAxis >= 0 && m_iLimitAxis < 3))
	{
		gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnConstraintBehavior::Update: The m_iLimitAxis must be ranged from 0~2!\n");
		ComponentContext->m_bShouldFree = true;
		return;
	}

	if ((pBarnacleRagdollObject->GetActivityType() == StudioAnimActivityType_BarnaclePulling && m_bActivatedOnBarnaclePulling) ||
		(pBarnacleRagdollObject->GetActivityType() == StudioAnimActivityType_BarnacleChewing && m_bActivatedOnBarnacleChewing))
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

					if (m_bCalculateLimitFromActualPlayerOrigin && pBarnacleRagdollObject->GetActivityType() == StudioAnimActivityType_BarnaclePulling)
					{
						flLimitValue = CalculateLimitValueFromActualPlayerOrigin(vStartOrigin);
					}

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

					if (m_bCalculateLimitFromActualPlayerOrigin && pBarnacleRagdollObject->GetActivityType() == StudioAnimActivityType_BarnaclePulling)
					{
						flLimitValue = CalculateLimitValueFromActualPlayerOrigin(vStartOrigin);
					}

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
			gEngfuncs.Con_DPrintf("CBulletBarnacleDragOnConstraintBehavior::Update: The attached constraint must be DofSpring!\n");
			ComponentContext->m_bShouldFree = true;
			return;
		}
	}
}