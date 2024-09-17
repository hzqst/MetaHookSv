#pragma once

#include "BasePhysicAction.h"

CBasePhysicAction::CBasePhysicAction(int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig):
	m_id(id),
	m_entindex(entindex),
	m_pPhysicObject(pPhysicObject),
	m_name(pActionConfig->name),
	m_flags(pActionConfig->flags),
	m_debugDrawLevel(pActionConfig->debugDrawLevel),
	m_configId(pActionConfig->configId)
{

}

int CBasePhysicAction::GetPhysicConfigId() const
{
	return m_configId;
}

int CBasePhysicAction::GetPhysicComponentId() const
{
	return m_id;
}

IPhysicObject* CBasePhysicAction::GetOwnerPhysicObject() const
{
	return m_pPhysicObject;
}

int CBasePhysicAction::GetOwnerEntityIndex() const
{
	return m_entindex;
}

const char* CBasePhysicAction::GetName() const
{
	return m_name.c_str();
}

int CBasePhysicAction::GetFlags() const
{
	return m_flags;
}

int CBasePhysicAction::GetDebugDrawLevel() const
{
	return m_debugDrawLevel;
}

bool CBasePhysicAction::ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const
{
	if (m_debugDrawLevel > 0 && ctx->m_ragdollObjectLevel > 0 && ctx->m_ragdollObjectLevel >= m_debugDrawLevel)
		return true;

	return false;
}

void CBasePhysicAction::TransferOwnership(int entindex)
{
	m_entindex = entindex;
}
