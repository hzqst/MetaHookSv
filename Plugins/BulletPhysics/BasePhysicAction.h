#pragma once

#include "ClientPhysicManager.h"

class CBasePhysicAction : public IPhysicAction
{
public:
	CBasePhysicAction(int id, int entindex, IPhysicObject* pPhysicObject, const CClientPhysicActionConfig* pActionConfig);

	int GetPhysicConfigId() const override;
	int GetPhysicComponentId() const override;
	IPhysicObject* GetOwnerPhysicObject() const override;
	int GetOwnerEntityIndex() const override;
	const char* GetName() const override;
	int GetFlags() const override;
	int GetDebugDrawLevel() const override;
	bool ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const override;
	void TransferOwnership(int entindex) override;

	int m_id{};
	int m_entindex{ };
	IPhysicObject* m_pPhysicObject{};
	std::string m_name;
	int m_flags{};
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	int m_configId{};
};