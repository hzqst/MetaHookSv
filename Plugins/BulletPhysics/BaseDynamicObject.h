#pragma once

#include "exportfuncs.h"
#include "ClientEntityManager.h"
#include "BasePhysicManager.h"
#include "PhysicUTIL.h"

class CBaseDynamicObject : public IDynamicObject
{
public:
	CBaseDynamicObject(const CDynamicObjectCreationParameter& CreationParam);
	~CBaseDynamicObject();

	int GetEntityIndex() const override;
	cl_entity_t* GetClientEntity() const override;
	entity_state_t* GetClientEntityState() const override;
	bool GetGoldSrcOriginAngles(float* origin, float* angles) override;
	model_t* GetModel() const override;
	float GetModelScaling() const override;
	uint64 GetPhysicObjectId() const override;
	int GetPlayerIndex() const override;
	int GetObjectFlags() const override;
	int GetPhysicConfigId() const override;
	bool IsClientEntityNonSolid() const override;
	bool ShouldDrawOnDebugDraw(const CPhysicDebugDrawContext* ctx) const override;
	int GetRigidBodyCount() const override;
	IPhysicRigidBody* GetRigidBodyByIndex(int index) const override;
	bool EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback) override;
	bool Rebuild(const CClientPhysicObjectConfig* pPhysicObjectConfig) override;
	void Update(CPhysicObjectUpdateContext* ObjectUpdateContext) override;
	bool SetupBones(studiohdr_t* studiohdr) override;
	bool SetupJiggleBones(studiohdr_t* studiohdr) override;
	bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams)) override;
	void AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters& filters) override;
	void RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) override;
	void FreePhysicActionsWithFilters(int with_flags, int without_flags) override;
	void TransferOwnership(int entindex) override;
	IPhysicComponent* GetPhysicComponentByName(const std::string& name) override;
	IPhysicComponent* GetPhysicComponentByComponentId(int id) override;
	IPhysicRigidBody* GetRigidBodyByName(const std::string& name) override;
	IPhysicRigidBody* GetRigidBodyByComponentId(int id) override;
	IPhysicConstraint* GetConstraintByName(const std::string& name) override;
	IPhysicConstraint* GetConstraintByComponentId(int id) override;

public:

	virtual IPhysicRigidBody* FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody);
	virtual void CreateRigidBodies(const CDynamicObjectCreationParameter& CreationParam);
	virtual void CreateConstraints(const CDynamicObjectCreationParameter& CreationParam);

	virtual IPhysicRigidBody* CreateRigidBody(const CDynamicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) = 0;
	virtual IPhysicConstraint* CreateConstraint(const CDynamicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) = 0;

protected:

	void AddRigidBody(IPhysicRigidBody* pRigidBody);
	void AddConstraint(IPhysicConstraint* pConstraint);
	void RebuildRigidBodies(const CDynamicObjectCreationParameter& CreationParam);
	void RebuildConstraints(const CDynamicObjectCreationParameter& CreationParam);

public:
	int m_entindex{};
	int m_playerindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	float m_model_scaling{ 1 };
	int m_flags{ PhysicObjectFlag_DynamicObject };
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	int m_configId{};

	std::vector<std::shared_ptr<CClientRigidBodyConfig>> m_RigidBodyConfigs;
	std::vector<std::shared_ptr<CClientConstraintConfig>> m_ConstraintConfigs;
	std::vector<std::shared_ptr<CClientPhysicActionConfig>> m_ActionConfigs;

	std::vector<IPhysicRigidBody *> m_RigidBodies;
	std::vector<IPhysicConstraint *> m_Constraints;
	std::vector<IPhysicAction *> m_Actions;
};