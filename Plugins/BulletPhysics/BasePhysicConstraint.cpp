#include "BasePhysicConstraint.h"

CBasePhysicConstraint::CBasePhysicConstraint(
	int id,
	int entindex,
	IPhysicObject* pPhysicObject,
	const CClientConstraintConfig* pConstraintConfig) :
	m_id(id),
	m_entindex(entindex),
	m_pPhysicObject(pPhysicObject),
	m_name(pConstraintConfig->name),
	m_flags(pConstraintConfig->flags),
	m_debugDrawLevel(pConstraintConfig->debugDrawLevel),
	m_configId(pConstraintConfig->configId)
{

}

int CBasePhysicConstraint::GetPhysicConfigId() const
{
	return m_configId;
}

int CBasePhysicConstraint::GetPhysicComponentId() const
{
	return m_id;
}

int CBasePhysicConstraint::GetOwnerEntityIndex() const
{
	return m_entindex;
}

IPhysicObject* CBasePhysicConstraint::GetOwnerPhysicObject() const
{
	return m_pPhysicObject;
}

const char* CBasePhysicConstraint::GetName() const
{
	return m_name.c_str();
}

int CBasePhysicConstraint::GetFlags() const
{
	return m_flags;
}

int CBasePhysicConstraint::GetDebugDrawLevel() const
{
	return m_debugDrawLevel;
}

bool CBasePhysicConstraint::ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const
{
	if (m_debugDrawLevel > 0 && ctx->m_constraintLevel > 0 && ctx->m_constraintLevel >= m_debugDrawLevel)
		return true;

	return false;
}

void CBasePhysicConstraint::TransferOwnership(int entindex)
{
	m_entindex = entindex;
}