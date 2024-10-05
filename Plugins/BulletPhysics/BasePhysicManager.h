#pragma once

#include <vector>
#include <map>
#include <unordered_map>

#include "ClientPhysicManager.h"
#include "ClientPhysicConfig.h"

class CBasePhysicManager : public IClientPhysicManager
{
protected:
#if 1
	CPhysicIndexArray* m_barnacleIndexArray{};
	CPhysicVertexArray* m_barnacleVertexArray{};
	CPhysicIndexArray* m_gargantuaIndexArray{};
	CPhysicVertexArray* m_gargantuaVertexArray{};
#endif
	float m_gravity{};

	uint64 m_inspectedPhysicObjectId{};
	int m_inspectedPhysicComponentId{};

	uint64 m_selectedPhysicObjectId{};
	int m_selectedPhysicComponentId{};

	int m_allocatedPhysicConfigId{};
	int m_allocatedPhysicComponentId{};

	std::unordered_map<int, IPhysicObject *> m_physicObjects;
	std::unordered_map<int, IPhysicComponent*> m_physicComponents;
	std::unordered_map<int, std::weak_ptr<CClientBasePhysicConfig>> m_physicConfigs;


#if 0
	//std::shared_ptr<CPhysicVertexArray> m_worldVertexArray;
	//std::vector<std::shared_ptr<CPhysicIndexArray>> m_brushIndexArray;
#endif

	CClientPhysicObjectConfigs m_physicObjectConfigs;

	std::unordered_map<std::string, std::shared_ptr<CPhysicIndexArray>> m_indexArrayResources;
	std::unordered_map<std::string, std::shared_ptr<CPhysicVertexArray>> m_worldVertexResources;

	CPhysicDebugDrawContext m_debugDrawContext;
public:

	void Destroy() override;
	void Init() override;
	void Shutdown() override;
	void NewMap() override;
	void SetGravity(float value) override;
	void StepSimulation(double frametime) override;

	bool SetupBones(CRagdollObjectSetupBoneContext* Context) override;
	bool SetupJiggleBones(CRagdollObjectSetupBoneContext* Context) override;
	bool StudioCheckBBox(studiohdr_t* studiohdr, int entindex, int* nVisible) override;

	//PhysicObjectConfig Management
	bool SavePhysicObjectConfigForModel(model_t* mod) override;
	bool SavePhysicObjectConfigForModelIndex(int modelindex) override;
	std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigForModel(model_t* mod) override;
	std::shared_ptr<CClientPhysicObjectConfig> CreateEmptyPhysicObjectConfigForModel(model_t* mod, int PhysicObjectType) override;
	std::shared_ptr<CClientPhysicObjectConfig> CreateEmptyPhysicObjectConfigForModelIndex(int modelindex, int PhysicObjectType) override;
	std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModel(model_t* mod) override;
	std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModelIndex(int modelindex);
	void LoadPhysicObjectConfigs(void) override; 
	void SavePhysicObjectConfigs(void) override;
	bool SavePhysicObjectConfigToFile(const std::string& modelname, CClientPhysicObjectConfig* pPhysicObjectConfig) override;
	void RemoveAllPhysicObjectConfigs(int withflags, int withoutflags) override;

	//PhysicObject Management
	IPhysicObject* GetPhysicObject(int entindex) override;
	IPhysicObject* GetPhysicObjectEx(uint64 physicObjectId) override;
	void AddPhysicObject(int entindex, IPhysicObject* pPhysicObject) override; 
	void FreePhysicObject(IPhysicObject* pPhysicObject) override;
	bool RemovePhysicObject(int entindex) override;
	bool RemovePhysicObjectEx(uint64 physicObjectId) override;
	void RemoveAllPhysicObjects(int withflags, int withoutflags) override;
	bool TransferOwnershipForPhysicObject(int old_entindex, int new_entindex) override;
	bool RebuildPhysicObject(int entindex, const CClientPhysicObjectConfig* pPhysicObjectConfig) override;
	bool RebuildPhysicObjectEx(uint64 physicObjectId, const CClientPhysicObjectConfig* pPhysicObjectConfig) override;
	bool RebuildPhysicObjectEx2(IPhysicObject* pPhysicObject, const CClientPhysicObjectConfig* pPhysicObjectConfig) override;

	void UpdateAllPhysicObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) override;

	void CreatePhysicObjectForEntity(cl_entity_t* ent, entity_state_t* state, model_t *mod) override;
	
	void SetupBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) override;
	void SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex, const CClientAnimControlConfig* pOverrideAnimControl) override;
	void UpdateBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) override;
	
	IPhysicObject* FindBarnacleObjectForPlayer(entity_state_t* state) override;
	IPhysicObject* FindGargantuaObjectForPlayer(entity_state_t* playerState) override;

	//PhysicComponent Management
	int AllocatePhysicComponentId() override; 
	IPhysicComponent* GetPhysicComponent(int physicComponentId) override;
	void AddPhysicComponent(int physicComponentId, IPhysicComponent* pPhysicComponent) override;
	bool RemovePhysicComponent(int physicComponentId) override;
	void FreePhysicComponent(IPhysicComponent* pPhysicComponent);

	//Inspect / Select System

	void SetInspectedPhysicComponentId(int physicComponentId) override;
	int  GetInspectedPhysicComponentId() const override;

	void SetSelectedPhysicComponentId(int physicComponentId) override;
	int  GetSelectedPhysicComponentId() const override;

	void   SetInspectedPhysicObjectId(uint64 physicObjectId) override;
	uint64 GetInspectedPhysicObjectId() const override;

	void   SetSelectedPhysicObjectId(uint64 physicObjectId) override;
	uint64 GetSelectedPhysicObjectId() const override;

	const CPhysicDebugDrawContext* GetDebugDrawContext() const override;

	//BasePhysicConfig Management
	int AllocatePhysicConfigId() override;
	std::weak_ptr<CClientBasePhysicConfig> GetPhysicConfig(int configId) override;
	void AddPhysicConfig(int configId, const std::shared_ptr<CClientBasePhysicConfig>& pPhysicConfig) override;
	bool RemovePhysicConfig(int configId) override;
	void RemoveAllPhysicConfigs() override;

	//VertexIndexArray Management
	std::shared_ptr<CPhysicIndexArray> LoadIndexArrayFromResource(const std::string& resourcePath) override;
	void FreeAllIndexArrays(int withflags, int withoutflags) override;
	
public:

	virtual IStaticObject* CreateStaticObject(const CPhysicObjectCreationParameter& CreationParam) = 0;
	virtual IDynamicObject* CreateDynamicObject(const CPhysicObjectCreationParameter& CreationParam) = 0;
	virtual IRagdollObject* CreateRagdollObject(const CPhysicObjectCreationParameter& CreationParam) = 0;

private:
	//WorldVertexArray and WorldIndexArray Management
	std::shared_ptr<CPhysicVertexArray> GenerateWorldVertexArray(model_t* mod);
	std::shared_ptr<CPhysicIndexArray> GenerateBrushIndexArray(model_t* mod, const std::shared_ptr<CPhysicVertexArray> & pWorldVertexArray);

	void GenerateIndexArrayForBrushModel(model_t* mod, CPhysicIndexArray* pIndexArray);
	void GenerateIndexArrayRecursiveWorldNode(mnode_t* node, CPhysicIndexArray* pIndexArray);
	void GenerateIndexArrayForSurface(msurface_t* psurf, CPhysicIndexArray* pIndexArray);
	void GenerateIndexArrayForBrushface(CPhysicBrushFace* brushface, CPhysicIndexArray* pIndexArray);

	//Deprecated: use Resource Management now
#if 0
	std::shared_ptr<CPhysicIndexArray> GetIndexArrayFromBrushModel(model_t* mod);
#endif

	//Deprecated: use .obj now
#if 0
	void GenerateBarnacleIndexVertexArray();
	void FreeBarnacleIndexVertexArray();
	void GenerateGargantuaIndexVertexArray();
	void FreeGargantuaIndexVertexArray();
#endif

	void CreatePhysicObjectFromConfig(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex);
	void CreatePhysicObjectForStudioModel(cl_entity_t* ent, entity_state_t* state, model_t* mod);
	void CreatePhysicObjectForBrushModel(cl_entity_t* ent, entity_state_t* state, model_t* mod);

	void LoadAdditionalResourcesForConfig(CClientPhysicObjectConfig* pPhysicObjectConfig);
	void LoadAdditionalResourcesForCollisionShapeConfig(CClientCollisionShapeConfig* pCollisionShapeConfig);

	bool CreateEmptyPhysicObjectConfig(const std::string& filename, CClientPhysicObjectConfigStorage& Storage, int PhysicObjectType);
	void OverwritePhysicObjectConfig(const std::string& filename, CClientPhysicObjectConfigStorage& Storage, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig);

	bool LoadPhysicObjectConfigFromFiles(model_t* mod, CClientPhysicObjectConfigStorage& Storage);
	bool LoadPhysicObjectConfigFromBSP(model_t* mod, CClientPhysicObjectConfigStorage& Storage);

	bool LoadObjToPhysicArrays(const std::string& resourcePath, std::shared_ptr<CPhysicIndexArray>& pIndexArray);

};

bool CheckPhysicComponentFilters(IPhysicComponent* pPhysicComponent, const CPhysicComponentFilters& filters);

bool DispatchPhysicComponentUpdate(IPhysicComponent* PhysicComponent, CPhysicObjectUpdateContext* ObjectUpdateContext, bool bIsAddingPhysicComponent);
void DispatchPhysicComponentsUpdate(std::vector<IPhysicComponent*>& PhysicComponents, CPhysicObjectUpdateContext* ObjectUpdateContext, bool bIsAddingPhysicComponent);
IPhysicComponent* DispatchGetPhysicComponentByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name);
IPhysicComponent* DispatchGetPhysicComponentByComponentId(const std::vector<IPhysicComponent*>& m_PhysicComponents, int id);
IPhysicRigidBody* DispatchGetRigidBodyByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name);
IPhysicRigidBody* DispatchGetRigidBodyByComponentId(const std::vector<IPhysicComponent*>& m_PhysicComponents, int id);
IPhysicConstraint* DispatchGetConstraintByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name);
IPhysicConstraint* DispatchGetConstraintByComponentId(const std::vector<IPhysicComponent*>& m_PhysicComponents, int id); 
IPhysicBehavior* DispatchGetPhysicBehaviorByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name);
IPhysicBehavior* DispatchGetPhysicBehaviorByComponentId(const std::vector<IPhysicComponent*>& m_PhysicComponents, int id);
void DispatchAddPhysicComponent(std::vector<IPhysicComponent*>& PhysicComponents, IPhysicComponent* pPhysicComponent);
void DispatchRemovePhysicComponents(std::vector<IPhysicComponent*>& PhysicComponents);
void DispatchRemovePhysicCompoentsWithFilters(std::vector<IPhysicComponent*>& PhysicComponents, const CPhysicComponentFilters& filters);

void DispatchBuildPhysicComponents(
	const CPhysicObjectCreationParameter& CreationParam,
	const std::vector<std::shared_ptr<CClientRigidBodyConfig>>& RigidBodyConfigs,
	const std::vector<std::shared_ptr<CClientConstraintConfig>>& ConstraintConfigs,
	const std::vector<std::shared_ptr<CClientPhysicBehaviorConfig>>& PhysicBehaviorConfigs,
	const std::function<IPhysicRigidBody* (const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId)>& pfnCreateRigidBody,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, IPhysicRigidBody*)>& pfnAddRigidBody,
	const std::function<IPhysicConstraint* (const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId)>& pfnCreateConstraint,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, IPhysicConstraint*)>& pfnAddConstraint,
	const std::function<IPhysicBehavior* (const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, int physicComponentId)>& pfnCreatePhysicBehavior,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior*)>& pfnAddPhysicBehavior);

void DispatchRebuildPhysicComponents(
	std::vector<IPhysicComponent*>& PhysicComponents,
	const CPhysicObjectCreationParameter& CreationParam,
	const std::vector<std::shared_ptr<CClientRigidBodyConfig>>& RigidBodyConfigs,
	const std::vector<std::shared_ptr<CClientConstraintConfig>>& ConstraintConfigs,
	const std::vector<std::shared_ptr<CClientPhysicBehaviorConfig>>& PhysicBehaviorConfigs,
	const std::function<IPhysicRigidBody* (const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId)>& pfnCreateRigidBody,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, IPhysicRigidBody*)>& pfnAddRigidBody,
	const std::function<IPhysicConstraint* (const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId)>& pfnCreateConstraint,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, IPhysicConstraint*)>& pfnAddConstraint,
	const std::function<IPhysicBehavior* (const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, int physicComponentId)>& pfnCreatePhysicBehavior,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior*)>& pfnAddPhysicBehavior);

bool DispatchStudioCheckBBox(const std::vector<IPhysicComponent*>& PhysicComponents, studiohdr_t* studiohdr, int* nVisible);

void FloatGoldSrcToBullet(float* v);
void FloatBulletToGoldSrc(float* v);
void Vec3GoldSrcToBullet(vec3_t vec);
void Vec3BulletToGoldSrc(vec3_t vec);

bool StudioGetActivityType(model_t* mod, entity_state_t* entstate, StudioAnimActivityType * pStudioAnimActivityType, int* pAnimControlFlags);