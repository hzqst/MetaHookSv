#pragma once

#include <vector>
#include <unordered_map>

#include "ClientPhysicManager.h"
#include "ClientPhysicConfig.h"

class CPhysicObjectCreationParameter
{
public:
	cl_entity_t* m_entity{};
	int m_entindex{};
	model_t* m_model{};
	studiohdr_t* m_studiohdr{};
	float m_model_scaling{ 1 };
};

class CStaticObjectCreationParameter : public CPhysicObjectCreationParameter
{
public:
	CClientStaticObjectConfig* m_pStaticObjectConfig{};
};

class CDynamicObjectCreationParameter : public CPhysicObjectCreationParameter
{
public:
	CClientDynamicObjectConfig* m_pDynamicObjectConfig{};
};

class CRagdollObjectCreationParameter : public CPhysicObjectCreationParameter
{
public:
	int m_playerindex{};
	bool m_allowNonNativeRigidBody{};
	CClientRagdollObjectConfig* m_pRagdollObjectConfig{};
};

class CBasePhysicAction : public IPhysicAction
{
public:
	CBasePhysicAction(int flags) : m_flags(flags)
	{

	}

	int GetActionFlags() const override
	{
		return m_flags;
	}

	int m_flags;
};

class CPhysicComponentAction : public CBasePhysicAction
{
public:
	CPhysicComponentAction(int id, int flags) : m_physicComponentId(id), CBasePhysicAction(flags)
	{

	}

	int m_physicComponentId{};
};

class CBasePhysicRigidBody : public IPhysicRigidBody
{
public:
	CBasePhysicRigidBody(int id, int entindex, const CClientRigidBodyConfig* pRigidConfig);

	int GetPhysicComponentId() const override
	{
		return m_id;
	}

	int GetOwnerEntityIndex() const override
	{
		return m_entindex;
	}

	const char* GetName() const override
	{
		return m_name.c_str();
	}

	int GetFlags() const override
	{
		return m_flags;
	}

	int GetDebugDrawLevel() const override
	{
		return m_debugDrawLevel;
	}

	void TransferOwnership(int entindex) override
	{
		m_entindex = entindex;
	}

public:
	int m_id{};
	int m_entindex{};
	std::string m_name;
	int m_flags{};
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };

	int m_boneindex{ -1 };
};

class CBasePhysicConstraint : public IPhysicConstraint
{
public:
	CBasePhysicConstraint(
		int id,
		int entindex, 
		const CClientConstraintConfig* pConstraintConfig);

	int GetPhysicComponentId() const override
	{
		return m_id;
	}

	int GetOwnerEntityIndex() const override
	{
		return m_entindex;
	}

	const char* GetName() const override
	{
		return m_name.c_str();
	}

	int GetFlags() const override
	{
		return m_flags;
	}

	int GetDebugDrawLevel() const override
	{
		return m_debugDrawLevel;
	}

	void TransferOwnership(int entindex) override
	{
		m_entindex = entindex;
	}

public:
	int m_id{};
	int m_entindex{};
	std::string m_name;
	int m_flags{};
	int m_debugDrawLevel{ BULLET_DEFAULT_DEBUG_DRAW_LEVEL };
};

class CBasePhysicManager : public IClientPhysicManager
{
protected:
	//CPhysicIndexArray* m_barnacleIndexArray{};
	//CPhysicVertexArray* m_barnacleVertexArray{};
	//CPhysicIndexArray* m_gargantuaIndexArray{};
	//CPhysicVertexArray* m_gargantuaVertexArray{};

	float m_gravity{};
	int m_iAllocatedPhysicComponentId{};

	std::unordered_map<int, IPhysicObject *> m_physicObjects;
	std::unordered_map<int, IPhysicComponent*> m_physicComponents;

	CPhysicVertexArray* m_worldVertexArray{};
	std::vector<CPhysicIndexArray *> m_brushIndexArray;

	//Configs
	CClientPhysicObjectConfigs m_physicConfigs;
public:
	void Destroy(void) override;
	void Init(void) override;
	void Shutdown() override;
	void NewMap(void) override;
	void DebugDraw(void) override;
	void SetGravity(float velocity) override;
	void StepSimulation(double frametime) override;

	std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigForModel(model_t* mod) override;
	std::shared_ptr<CClientPhysicObjectConfig> GetPhysicObjectConfigForModel(model_t* mod) override;
	void LoadPhysicObjectConfigs(void) override; 
	void SavePhysicObjectConfigs(void);

	bool SetupBones(studiohdr_t* studiohdr, int entindex) override;
	bool SetupJiggleBones(studiohdr_t* studiohdr, int entindex) override;
	void MergeBarnacleBones(studiohdr_t* studiohdr, int entindex) override;

	//Physic Object
	IPhysicObject* GetPhysicObject(int entindex) override;
	void AddPhysicObject(int entindex, IPhysicObject* pPhysicObject) override; 
	void FreePhysicObject(IPhysicObject* pPhysicObject) override;
	bool RemovePhysicObject(int entindex) override;
	void RemoveAllPhysicObjects(int withflags, int withoutflags) override;
	bool TransferOwnershipForPhysicObject(int old_entindex, int new_entindex) override;
	void UpdatePhysicObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) override;

	void CreatePhysicObjectForEntity(cl_entity_t* ent, entity_state_t* state, model_t *mod) override;
	void SetupBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) override;
	void SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex, const CClientAnimControlConfig& OverrideAnim) override;
	void UpdateBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex) override;
	
	IPhysicObject* FindBarnacleObjectForPlayer(entity_state_t* state) override;
	IPhysicObject* FindGargantuaObjectForPlayer(entity_state_t* playerState) override;

	//Physic Component
	int AllocatePhysicComponentId() override; 
	IPhysicComponent* GetPhysicComponent(int physicComponentId) override;
	void AddPhysicComponent(int physicComponentId, IPhysicComponent* pPhysicComponent) override;
	void FreePhysicComponent(IPhysicComponent* pPhysicComponent) override;
	bool RemovePhysicComponent(int physicComponentId) override;
public:

	virtual IStaticObject* CreateStaticObject(const CStaticObjectCreationParameter& CreationParam) = 0;
	virtual IDynamicObject* CreateDynamicObject(const CDynamicObjectCreationParameter& CreationParam) = 0;
	virtual IRagdollObject* CreateRagdollObject(const CRagdollObjectCreationParameter& CreationParam) = 0;

private:
	//WorldVertexArray and WorldIndexArray
	void GenerateWorldVertexArray();
	void FreeWorldVertexArray();
	void GenerateBrushIndexArray();
	void FreeAllBrushIndexArray();
	void GenerateIndexArrayForBrushModel(model_t* mod, CPhysicVertexArray* vertexArray, CPhysicIndexArray* indexArray);
	void GenerateIndexArrayRecursiveWorldNode(mnode_t* node, CPhysicVertexArray* vertexArray, CPhysicIndexArray* indexArray);
	void GenerateIndexArrayForSurface(msurface_t* psurf, CPhysicVertexArray* vertexarray, CPhysicIndexArray* indexarray);
	void GenerateIndexArrayForBrushface(CPhysicBrushFace* brushface, CPhysicIndexArray* indexArray);

	CPhysicIndexArray* GetIndexArrayFromBrushModel(model_t* mod);

	//Deprecated: use .obj files now
	//void GenerateBarnacleIndexVertexArray();
	//void FreeBarnacleIndexVertexArray();
	//void GenerateGargantuaIndexVertexArray();
	//void FreeGargantuaIndexVertexArray();

	bool SavePhysicObjectConfigToFile(const std::string& filename, CClientPhysicObjectConfig* pPhysicObjectConfig);
	bool LoadPhysicObjectConfigFromFiles(const std::string& filename, CClientPhysicObjectConfigStorage& Storage);

	void RemoveAllPhysicConfigs();

	void CreatePhysicObjectFromConfig(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex);
	void CreatePhysicObjectForStudioModel(cl_entity_t* ent, entity_state_t* state, model_t* mod);
	void CreatePhysicObjectForBrushModel(cl_entity_t* ent, entity_state_t* state, model_t* mod);

	bool LoadObjToPhysicArrays(const std::string& objFilename, CPhysicVertexArray* vertexArray, CPhysicIndexArray* indexArray);

	void LoadAdditionalResourcesForConfig(CClientPhysicObjectConfig* pPhysicObjectConfig);
};

bool CheckPhysicComponentFilters(IPhysicComponent* pPhysicComponent, const CPhysicComponentFilters& filters);
