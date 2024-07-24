#pragma once

#include <vector>
#include <unordered_map>

#include "ClientPhysicManager.h"
#include "ClientRagdollConfig.h"

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
	CPhysicVertexArray* VertexArray{};
	CPhysicIndexArray* IndexArray{};
};

class CPhysicStaticObjectCreationParameter : public CPhysicObjectCreationParameter
{
public:
	bool IsKinematic{};
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
	void ReloadConfig(void)  override;
	bool SetupBones(studiohdr_t* hdr, int entindex)  override;
	bool SetupJiggleBones(studiohdr_t* hdr, int entindex)  override;
	void MergeBarnacleBones(studiohdr_t* hdr, int entindex) override;
	bool ChangeRagdollEntityIndex(int old_entindex, int new_entindex) override;
	IPhysicObject* GetPhysicObject(int entindex) override;
	CClientPhysicConfig* LoadPhysicConfig(model_t* mod) override;
	IRagdollObject* CreateRagdollObject(model_t* mod, int entindex, const CClientPhysicConfig* config) override;
	void CreateBrushModel(cl_entity_t* ent) override;
	void CreateBarnacle(cl_entity_t* ent) override;
	void CreateGargantua(cl_entity_t* ent) override;
	void RemovePhysicObject(int entindex) override;
	void RemoveAllPhysicObjects(int flags) override;
	void UpdateTempEntity(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time) override;

public:

	virtual IStaticObject* CreateStaticObject(cl_entity_t* ent, const CPhysicStaticObjectCreationParameter& CreationParameter) = 0;

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

	void LoadPhysicConfigFromFile(CClientPhysicConfig* Configs, const char* name);
};