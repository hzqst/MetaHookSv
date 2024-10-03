#pragma once

#include "exportfuncs.h"
#include "ClientEntityManager.h"
#include "BasePhysicManager.h"

class CBaseStaticObject : public IStaticObject
{
public:
	CBaseStaticObject(const CPhysicObjectCreationParameter& CreationParam);
	~CBaseStaticObject();
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
	bool EnumPhysicComponents(const fnEnumPhysicComponentCallback& callback) override;
	bool Build(const CPhysicObjectCreationParameter& CreationParam) override;
	bool Rebuild(const CPhysicObjectCreationParameter& CreationParam) override;
	void Update(CPhysicObjectUpdateContext* ObjectUpdateContext) override;
	bool SetupBones(studiohdr_t* studiohdr, int flags) override;
	bool SetupJiggleBones(studiohdr_t* studiohdr, int flags) override;
	bool StudioCheckBBox(studiohdr_t* studiohdr, int* nVisible) override;
	bool CalcRefDef(struct ref_params_s* pparams, bool bIsThirdPerson, void(*callback)(struct ref_params_s* pparams)) override;
	void AddPhysicComponentsToPhysicWorld(void* world, const CPhysicComponentFilters& filters) override;
	void RemovePhysicComponentsFromPhysicWorld(void* world, const CPhysicComponentFilters& filters) override;
	void RemovePhysicComponentsWithFilters(const CPhysicComponentFilters& filters) override;
	void TransferOwnership(int entindex) override;

	IPhysicComponent* GetPhysicComponentByName(const std::string& name) override;
	IPhysicComponent* GetPhysicComponentByComponentId(int id) override;
	IPhysicRigidBody* GetRigidBodyByName(const std::string& name) override;
	IPhysicRigidBody* GetRigidBodyByComponentId(int id) override;
	IPhysicConstraint* GetConstraintByName(const std::string& name) override;
	IPhysicConstraint* GetConstraintByComponentId(int id) override;
	IPhysicBehavior* GetPhysicBehaviorByName(const std::string& name) override;
	IPhysicBehavior* GetPhysicBehaviorByComponentId(int id) override;

	IPhysicRigidBody* FindRigidBodyByName(const std::string& name, bool allowNonNativeRigidBody) override;

	virtual IPhysicRigidBody* CreateRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId) = 0;
	virtual IPhysicConstraint* CreateConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId) = 0;
	virtual IPhysicBehavior* CreatePhysicBehavior(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, int physicComponentId) = 0;

	virtual void AddRigidBody(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidBodyConfig, IPhysicRigidBody* pRigidBody);
	virtual void AddConstraint(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, IPhysicConstraint* pConstraint);
	virtual void AddPhysicBehavior(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior* pPhysicBehavior);

public:
	int m_entindex{};
	int m_playerindex{};
	cl_entity_t* m_entity{};
	model_t* m_model{};
	float m_model_scaling{ 1 };
	int m_flags{ PhysicObjectFlag_StaticObject };
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
	int m_configId{};

	std::vector<std::shared_ptr<CClientRigidBodyConfig>> m_RigidBodyConfigs;
	std::vector<std::shared_ptr<CClientConstraintConfig>> m_ConstraintConfigs;
	std::vector<std::shared_ptr<CClientPhysicBehaviorConfig>> m_PhysicBehaviorConfigs;

	std::vector<IPhysicComponent*> m_PhysicComponents;
};