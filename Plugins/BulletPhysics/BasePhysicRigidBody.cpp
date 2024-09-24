#include "BasePhysicRigidBody.h"

CBasePhysicRigidBody::CBasePhysicRigidBody(
	int id,
	int entindex,
	IPhysicObject* pPhysicObject,
	const CClientRigidBodyConfig* pRigidConfig) :

	m_id(id),
	m_entindex(entindex),
	m_pPhysicObject(pPhysicObject),
	m_name(pRigidConfig->name),
	m_flags(pRigidConfig->flags),
	m_boneindex(pRigidConfig->boneindex),
	m_debugDrawLevel(pRigidConfig->debugDrawLevel),
	m_configId(pRigidConfig->configId)
{

}

int CBasePhysicRigidBody::GetPhysicConfigId() const
{
	return m_configId;
}

int CBasePhysicRigidBody::GetPhysicComponentId() const
{
	return m_id;
}

int CBasePhysicRigidBody::GetOwnerEntityIndex() const
{
	return m_entindex;
}

IPhysicObject* CBasePhysicRigidBody::GetOwnerPhysicObject() const
{
	return m_pPhysicObject;
}

const char* CBasePhysicRigidBody::GetName() const
{
	return m_name.c_str();
}

int CBasePhysicRigidBody::GetFlags() const
{
	return m_flags;
}

int CBasePhysicRigidBody::GetDebugDrawLevel() const
{
	return m_debugDrawLevel;
}

bool CBasePhysicRigidBody::ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const
{
	if (m_debugDrawLevel > 0 && ctx->m_rigidbodyLevel > 0 && ctx->m_rigidbodyLevel >= m_debugDrawLevel)
		return true;

	return false;
}

void CBasePhysicRigidBody::TransferOwnership(int entindex)
{
	m_entindex = entindex;
}
