#pragma once

#include "BasePhysicBehavior.h"

CBasePhysicBehavior::CBasePhysicBehavior(int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicBehaviorConfig* pPhysicBehaviorConfig):
	m_id(id),
	m_entindex(entindex),
	m_pPhysicObject(pPhysicObject),
	m_name(pPhysicBehaviorConfig->name),
	m_flags(pPhysicBehaviorConfig->flags),
	m_debugDrawLevel(pPhysicBehaviorConfig->debugDrawLevel),
	m_configId(pPhysicBehaviorConfig->configId)
{

}

int CBasePhysicBehavior::GetPhysicConfigId() const
{
	return m_configId;
}

int CBasePhysicBehavior::GetPhysicComponentId() const
{
	return m_id;
}

IPhysicObject* CBasePhysicBehavior::GetOwnerPhysicObject() const
{
	return m_pPhysicObject;
}

int CBasePhysicBehavior::GetOwnerEntityIndex() const
{
	return m_entindex;
}

const char* CBasePhysicBehavior::GetName() const
{
	return m_name.c_str();
}

int CBasePhysicBehavior::GetFlags() const
{
	return m_flags;
}

int CBasePhysicBehavior::GetDebugDrawLevel() const
{
	return m_debugDrawLevel;
}

bool CBasePhysicBehavior::ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const
{
	if (m_debugDrawLevel > 0 && ctx->m_behaviorLevel > 0 && ctx->m_behaviorLevel >= m_debugDrawLevel)
		return true;

	return false;
}

void CBasePhysicBehavior::TransferOwnership(int entindex)
{
	m_entindex = entindex;
}
