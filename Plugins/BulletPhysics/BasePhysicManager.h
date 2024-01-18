#pragma once

#include <vector>
#include <unordered_map>

#include "PhysicManager.h"

class CPhysicBrushVertex
{
public:
	CPhysicBrushVertex()
	{
		pos[0] = 0;
		pos[1] = 0;
		pos[2] = 0;
	}

	vec3_t	pos;
};

class CPhysicBrushFace
{
public:
	CPhysicBrushFace()
	{
		start_vertex = 0;
		num_vertexes = 0;
	}

	int start_vertex;
	int num_vertexes;
};

class CPhysicVertexArray
{
public:
	CPhysicVertexArray()
	{
		bIsDynamic = false;
	}
	std::vector<CPhysicBrushVertex> vVertexBuffer;
	std::vector<CPhysicBrushFace> vFaceBuffer;
	bool bIsDynamic;
};

class CPhysicIndexArray
{
public:
	CPhysicIndexArray()
	{
		bIsDynamic = false;
	}
	std::vector<int> vIndexBuffer;
	bool bIsDynamic;
};

class CPhysicObjectCreationParameter
{
public:
	CPhysicVertexArray* VertexArray;
	CPhysicIndexArray* IndexArray;
};

class CPhysicStaticObjectCreationParameter : public CPhysicObjectCreationParameter
{
public:
	bool IsKinematic;
};

class CBasePhysicManager : public IPhysicManager
{
protected:
	CPhysicVertexArray* m_worldVertexArray;
	CPhysicIndexArray* m_barnacleIndexArray;
	CPhysicVertexArray* m_barnacleVertexArray;
	CPhysicIndexArray* m_gargantuaIndexArray;
	CPhysicVertexArray* m_gargantuaVertexArray;
	float m_gravity;

	std::unordered_map<int, IPhysicObject *> m_physicObjects;
	std::vector<CRagdollConfig *> m_ragdollConfigs;
	std::vector<CPhysicIndexArray *> m_brushIndexArray;
public:
	CBasePhysicManager();

	void Init(void) override;
	void Shutdown() override;
	void NewMap(void) override;
	void DebugDraw(void) override;
	void SetGravity(float velocity) override;
	void StepSimulation(double framerate) override;
	void ReloadConfig(void)  override;
	bool SetupBones(studiohdr_t* hdr, int entindex)  override;
	bool SetupJiggleBones(studiohdr_t* hdr, int entindex)  override;
	void MergeBarnacleBones(studiohdr_t* hdr, int entindex) override;
	bool ChangeRagdollEntityIndex(int old_entindex, int new_entindex) override;
	IPhysicObject* GetPhysicObject(int entindex) override;
	IRagdollObject* CreateRagdollObject(model_t* mod, int entindex, const CRagdollConfig& config) override;
	void CreateBrushModel(cl_entity_t* ent) override;
	void CreateBarnacle(cl_entity_t* ent) override;
	void CreateGargantua(cl_entity_t* ent) override;
	void RemovePhysicObject(int entindex) override;
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

	CRagdollConfig* GetRagdollConfigFromModel(model_t* mod);
};