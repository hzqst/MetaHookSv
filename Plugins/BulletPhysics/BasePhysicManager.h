#pragma once

#include <vector>
#include <unordered_map>

#include "ClientPhysicManager.h"
#include "ClientPhysicConfig.h"

class CPhysicBrushVertex
{
public:
	vec3_t	pos{ 0 };
};

class CPhysicBrushFace
{
public:
	int start_vertex{};
	int num_vertexes{};
};

class CPhysicVertexArray
{
public:
	std::vector<CPhysicBrushVertex> vVertexBuffer;
	std::vector<CPhysicBrushFace> vFaceBuffer;
	bool bIsDynamic{};
};

class CPhysicIndexArray
{
public:
	std::vector<int> vIndexBuffer;
	bool bIsDynamic{};
};

class CPhysicObjectCreationParameter
{
public:
	cl_entity_t* m_entity{};
	model_t* m_model{};
	int m_entindex{};
	CClientPhysicConfig* m_pPhysConfigs{};
};

class CStaticObjectCreationParameter : public CPhysicObjectCreationParameter
{
public:
	CPhysicVertexArray* m_pVertexArray{};
	CPhysicIndexArray* m_pIndexArray{};
	bool m_bIsKinematic{};
};

class CRagdollObjectCreationParameter : public CPhysicObjectCreationParameter
{
public:
	int m_playerindex{};
	studiohdr_t* m_studiohdr{};
};

class CBasePhysicManager : public IClientPhysicManager
{
protected:
	CPhysicVertexArray* m_worldVertexArray{};
	CPhysicIndexArray* m_barnacleIndexArray{};
	CPhysicVertexArray* m_barnacleVertexArray{};
	CPhysicIndexArray* m_gargantuaIndexArray{};
	CPhysicVertexArray* m_gargantuaVertexArray{};
	float m_gravity{};

	//PhysicObject Manager
	std::unordered_map<int, IPhysicObject *> m_physicObjects;

	std::vector<CPhysicIndexArray *> m_brushIndexArray;

	//Configs
	CClientPhysicConfigs m_physicConfigs;
public:
	void Destroy(void) override;
	void Init(void) override;
	void Shutdown() override;
	void NewMap(void) override;
	void DebugDraw(void) override;
	void SetGravity(float velocity) override;
	void StepSimulation(double frametime) override;
	void LoadPhysicConfigs(void) override;
	bool SetupBones(studiohdr_t* studiohdr, int entindex)  override;
	bool SetupJiggleBones(studiohdr_t* studiohdr, int entindex)  override;
	void MergeBarnacleBones(studiohdr_t* studiohdr, int entindex) override;
	CClientPhysicConfigSharedPtr LoadPhysicConfigForModel(model_t* mod) override;

	IPhysicObject* GetPhysicObject(int entindex) override;
	void AddPhysicObject(int entindex, IPhysicObject* pPhysicObject) override; 
	void FreePhysicObject(IPhysicObject* pPhysicObject) override;
	bool RemovePhysicObject(int entindex) override;
	void RemoveAllPhysicObjects(int flags) override;
	bool TransformOwnerEntityForPhysicObject(int old_entindex, int new_entindex) override;
	void UpdateRagdollObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) override;

	void CreatePhysicObjectForEntity(cl_entity_t* ent) override;
public:

	virtual IStaticObject* CreateStaticObject(const CStaticObjectCreationParameter& CreationParam) = 0;
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

	//Barnacle's VertexArray and IndexArray
	void GenerateBarnacleIndexVertexArray();
	void FreeBarnacleIndexVertexArray();

	//Gargantua's VertexArray and IndexArray
	void GenerateGargantuaIndexVertexArray();
	void FreeGargantuaIndexVertexArray();

	bool LoadPhysicConfigFromFiles(CClientPhysicConfig* Configs, const std::string& filename);

	bool LoadPhysicConfigFromNewFile(CClientPhysicConfig* Configs, const std::string& filename);
	bool LoadPhysicConfigFromNewFileBuffer(CClientPhysicConfig* pConfigs, const char* buf);

	bool LoadPhysicConfigFromLegacyFile(CClientPhysicConfig* Configs, const std::string& filename);
	bool LoadPhysicConfigFromLegacyFileBuffer(CClientPhysicConfig* pConfigs, const char *buf);

	void RemoveAllPhysicConfigs();

	void CreateBarnacle(cl_entity_t* ent);
	void CreateGargantua(cl_entity_t* ent);
	void CreatePhysicObjectForStudioModel(cl_entity_t* ent);
	void CreatePhysicObjectForBrushModel(cl_entity_t* ent);
	void CreatePhysicObjectFromConfig(cl_entity_t* ent, model_t* mod, int entindex, int playerindex);
};