#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "plugins.h"
#include "mathlib2.h"
#include "CounterStrike.h"
#include "BasePhysicManager.h"
#include "ClientEntityManager.h"

IClientPhysicManager* g_pClientPhysicManager{};

IClientPhysicManager* ClientPhysicManager()
{
	return g_pClientPhysicManager;
}

void CBasePhysicManager::Destroy(void)
{
	delete this;
}

void CBasePhysicManager::Init(void)
{

}

void CBasePhysicManager::Shutdown()
{

}

void CBasePhysicManager::NewMap(void)
{
	GenerateWorldVertexArray();
	GenerateBrushIndexArray();
	GenerateBarnacleIndexVertexArray();
	GenerateGargantuaIndexVertexArray();
	CreatePhysicObjectForBrushModel(r_worldentity);
}

void CBasePhysicManager::DebugDraw(void)
{
	
}

void CBasePhysicManager::SetGravity(float velocity)
{
	m_gravity = -velocity;
}

void CBasePhysicManager::StepSimulation(double frametime)
{
	if (GetSimulationTickRate() < 32)
	{
		gEngfuncs.Cvar_SetValue("bv_simrate", 32);
	}
	else if (GetSimulationTickRate() > 128)
	{
		gEngfuncs.Cvar_SetValue("bv_simrate", 128);
	}
}

void CBasePhysicManager::RemoveAllPhysicConfigs()
{
	m_physicConfigs.clear();
}

void CBasePhysicManager::LoadPhysicConfigs(void)
{
	RemoveAllPhysicConfigs();

	int maxNum = EngineGetMaxKnownModel();

	if ((int)m_physicConfigs.size() < maxNum)
		m_physicConfigs.resize(maxNum);

	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);
		if (mod->type == mod_studio && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				auto moddata = IEngineStudio.Mod_Extradata(mod);
				if (moddata)
				{
					LoadPhysicConfigForModel(mod);
				}
			}
		}
	}
}

bool CBasePhysicManager::SetupBones(studiohdr_t* studiohdr, int entindex)
{
	auto pPhysicObject = GetPhysicObject(entindex);

	if (!pPhysicObject)
		return false;

	return pPhysicObject->SetupBones(studiohdr);
}

bool CBasePhysicManager::SetupJiggleBones(studiohdr_t* studiohdr, int entindex)
{
	auto pPhysicObject = GetPhysicObject(entindex);

	if (!pPhysicObject)
		return false;

	return pPhysicObject->SetupJiggleBones(studiohdr);
}

void CBasePhysicManager::MergeBarnacleBones(studiohdr_t* studiohdr, int entindex)
{
	//TODO
}

bool CBasePhysicManager::TransformOwnerEntityForPhysicObject(int old_entindex, int new_entindex)
{
	auto PhysicObject = GetPhysicObject(old_entindex);

	if (PhysicObject)
	{
		m_physicObjects.erase(old_entindex);

		AddPhysicObject(new_entindex, PhysicObject);

		PhysicObject->TransformOwnerEntity(new_entindex);

		return true;
	}

	return false;
}

IPhysicObject* CBasePhysicManager::GetPhysicObject(int entindex)
{
	auto itor = m_physicObjects.find(entindex);

	if (itor == m_physicObjects.end())
	{
		return NULL;
	}

	return itor->second;
}

bool CBasePhysicManager::LoadPhysicConfigFromNewFile(CClientPhysicConfig* pConfigs, const std::string& filename)
{
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return false;

	bool bLoaded = LoadPhysicConfigFromNewFileBuffer(pConfigs, pFileContent);

	gEngfuncs.COM_FreeFile((void*)pFileContent);

	return bLoaded;
}

bool CBasePhysicManager::LoadPhysicConfigFromNewFileBuffer(CClientPhysicConfig* pConfigs, const char* buf)
{
	//TODO
	return false;
}

bool CBasePhysicManager::LoadPhysicConfigFromLegacyFile(CClientPhysicConfig* pConfigs, const std::string& filename)
{
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return false;

	bool bLoaded = LoadPhysicConfigFromLegacyFileBuffer(pConfigs, pFileContent);

	gEngfuncs.COM_FreeFile((void*)pFileContent);

	return bLoaded;
}

bool CBasePhysicManager::LoadPhysicConfigFromLegacyFileBuffer(CClientPhysicConfig* pConfigs, const char *buf)
{
	//TODO
	return false;
}

bool CBasePhysicManager::LoadPhysicConfigFromFiles(CClientPhysicConfig* pConfigs, const std::string& filename)
{
	std::string fullname = filename;

	if (fullname.length() < 4)
	{
		gEngfuncs.Con_DPrintf("LoadPhysicConfigFromFiles: Invalid name %s\n", fullname.c_str());
		return false;
	}

	fullname = fullname.substr(0, fullname.length() - 4);

	auto fullname_phys = fullname;
	fullname_phys += "_physic.txt";

	if (LoadPhysicConfigFromNewFile(pConfigs, fullname_phys))
		return true;

	auto fullname_ragdoll = fullname;
	fullname_ragdoll  += "_ragdoll.txt";

	if (LoadPhysicConfigFromLegacyFile(pConfigs, fullname_ragdoll))
		return true;

	pConfigs->state = PhysicConfigState_LoadedWithError;

	gEngfuncs.Con_DPrintf("LoadPhysicConfigFromFiles: PhysicConfig \"%s\" loaded with error.\n", fullname.c_str());

	return false;
}

CClientPhysicConfigSharedPtr CBasePhysicManager::LoadPhysicConfigForModel(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex == -1)
	{
		g_pMetaHookAPI->SysError("LoadPhysicConfigForModel: Invalid model index\n");
		return nullptr;
	}

	if (m_physicConfigs.size() < EngineGetMaxKnownModel())
		m_physicConfigs.resize(EngineGetMaxKnownModel());

	if (modelindex >= m_physicConfigs.size())
	{
		g_pMetaHookAPI->SysError("LoadPhysicConfig: Invalid model index\n");
		return nullptr;
	}

	auto pConfigs = m_physicConfigs[modelindex];

	if (pConfigs)
		return pConfigs;

	pConfigs = std::make_shared<CClientPhysicConfig>();

	m_physicConfigs[modelindex] = pConfigs;

	LoadPhysicConfigFromFiles(pConfigs.get(), mod->name);

	return pConfigs;
}

void CBasePhysicManager::CreatePhysicObjectForEntity(cl_entity_t* ent)
{
	auto mod = ent->model;

	if (!mod)
		return;

	if (mod->type == mod_studio)
	{
		CreatePhysicObjectForStudioModel(ent);
	}
	else if (mod->type == mod_brush && ent->curstate.solid == SOLID_BSP)
	{
		CreatePhysicObjectForBrushModel(ent);
	}
}

void CBasePhysicManager::CreatePhysicObjectForStudioModel(cl_entity_t* ent)
{
	//TODO Port barnacle and garg to Configs ?
	if (ClientEntityManager()->IsEntityNetworkEntity(ent))
	{
		if (ClientEntityManager()->IsEntityDeadPlayer(ent))
		{
			auto entindex = ent->index;
			auto playerindex = ent->curstate.renderamt;
			auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

			if (g_bIsCounterStrike)
			{
				//Counter-Strike redirects playermodel in a pretty tricky way
				int modelindex = 0;
				model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
			}

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && PhysicObject->GetModel() == model && PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				if (TransformOwnerEntityForPhysicObject(playerindex, entindex))
					return;

				CreatePhysicObjectFromConfig(ent, model, entindex, playerindex);
			}
		}
		else if (ClientEntityManager()->IsEntityPlayer(ent))
		{
			auto entindex = ent->index;
			auto playerindex = ent->index;
			auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

			if (g_bIsCounterStrike)
			{
				//Counter-Strike redirects playermodel in a pretty tricky way
				int modelindex = 0;
				model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
			}

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && PhysicObject->GetModel() == model && PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				CreatePhysicObjectFromConfig(ent, model, entindex, playerindex);
			}
		}
		else
		{
			if (ClientEntityManager()->IsEntityBarnacle(ent))
			{
				ClientEntityManager()->AddBarnacle(ent->index, 0);
			}
			else if (ClientEntityManager()->IsEntityGargantua(ent))
			{
				ClientEntityManager()->AddGargantua(ent->index, 0);
			}

			auto entindex = ent->index;
			auto model = ent->model;

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && PhysicObject->GetModel() == model)
			{

			}
			else
			{
				CreatePhysicObjectFromConfig(ent, model, entindex, 0);
			}
		}
	}
	else if (ClientEntityManager()->IsEntityTempEntity(ent))
	{
		if (ClientEntityManager()->IsEntityClientCorpse(ent))
		{
			auto entindex = ClientEntityManager()->GetEntityIndex(ent);
			auto playerindex = ent->curstate.owner;
			auto model = ent->model;

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && PhysicObject->GetModel() == model && PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				if (TransformOwnerEntityForPhysicObject(playerindex, entindex))
					return;

				CreatePhysicObjectFromConfig(ent, model, entindex, playerindex);
			}
		}
		else
		{
			//TODO other tempents are not supported yet?
		}
	}
}

void CBasePhysicManager::CreatePhysicObjectFromConfig(cl_entity_t* ent, model_t* mod, int entindex, int playerindex)
{
	auto pPhysicConfig = ClientPhysicManager()->LoadPhysicConfigForModel(mod);

	if (!pPhysicConfig)
		return;

	if (pPhysicConfig->state != PhysicConfigState_Loaded)
		return;

	if (pPhysicConfig->type == PhysicConfigType_Ragdoll)
	{
		const auto playerState = R_GetPlayerState(playerindex);

		auto fakePlayerState = *playerState;

		fakePlayerState.number = playerindex;
		fakePlayerState.weaponmodel = 0;
		fakePlayerState.sequence = pPhysicConfig->sequence;
		fakePlayerState.gaitsequence = pPhysicConfig->gaitsequence;
		fakePlayerState.movetype = MOVETYPE_NONE;

		VectorCopy(ent->curstate.angles, fakePlayerState.angles);
		VectorCopy(ent->curstate.origin, fakePlayerState.origin);

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL, &fakePlayerState);

		CRagdollObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_model = mod;
		CreationParam.m_entindex = entindex;
		CreationParam.m_playerindex = playerindex;
		CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
		CreationParam.m_pPhysConfigs = pPhysicConfig.get();

		auto pRagdollObject = CreateRagdollObject(CreationParam);

		if (!pRagdollObject)
			return;

		AddPhysicObject(entindex, pRagdollObject);
	}
	else
	{
		gEngfuncs.Con_DPrintf("CreatePhysicObjectFromConfig: Unsupported type (%d) in PhysicConfig.\n", pPhysicConfig->type);
	}
}

void CBasePhysicManager::CreatePhysicObjectForBrushModel(cl_entity_t* ent)
{
	auto entindex = ClientEntityManager()->GetEntityIndex(ent);
	auto mod = ent->model;

	auto pPhysicObject = GetPhysicObject(entindex);

	if (pPhysicObject)
		return;

	auto pIndexArray = GetIndexArrayFromBrushModel(mod);

	if (!pIndexArray)
		return;

	CStaticObjectCreationParameter CreationParam;
	CreationParam.m_entity = ent;
	CreationParam.m_entindex = entindex;
	CreationParam.m_model = mod;
	CreationParam.m_pVertexArray = m_worldVertexArray;
	CreationParam.m_pIndexArray = pIndexArray;
	CreationParam.m_bIsKinematic = false;

	if ((ent != r_worldentity) && (ent->curstate.movetype == MOVETYPE_PUSH || ent->curstate.movetype == MOVETYPE_PUSHSTEP))
	{
		CreationParam.m_bIsKinematic = true;
	}

	auto pStaticObject = CreateStaticObject(CreationParam);

	if (!pStaticObject)
		return;

	AddPhysicObject(entindex, pStaticObject);
}

void CBasePhysicManager::CreateBarnacle(cl_entity_t* ent)
{
	auto entindex = ent->index;

	auto pPhysicObject = GetPhysicObject(entindex);

	if (pPhysicObject)
		return;

	CStaticObjectCreationParameter CreationParam;
	CreationParam.m_entity = ent;
	CreationParam.m_entindex = entindex;
	CreationParam.m_pVertexArray = m_barnacleVertexArray;
	CreationParam.m_pIndexArray = m_barnacleIndexArray;
	CreationParam.m_bIsKinematic = false;

	auto pStaticObject = CreateStaticObject(CreationParam);

	if (!pStaticObject)
		return;

	AddPhysicObject(entindex, pStaticObject);
}

void CBasePhysicManager::CreateGargantua(cl_entity_t* ent)
{

}

void CBasePhysicManager::AddPhysicObject(int entindex, IPhysicObject* pPhysicObject)
{
	RemovePhysicObject(entindex);

	m_physicObjects[entindex] = pPhysicObject;
}

void CBasePhysicManager::FreePhysicObject(IPhysicObject *pPhysicObject)
{
	RemovePhysicObjectFromWorld(pPhysicObject);

	pPhysicObject->Destroy();
}

bool CBasePhysicManager::RemovePhysicObject(int entindex)
{
	auto itor = m_physicObjects.find(entindex);

	if (itor != m_physicObjects.end())
	{
		auto pPhysicObject = itor->second;

		FreePhysicObject(pPhysicObject);

		m_physicObjects.erase(itor);

		return true;
	}

	return false;
}

void CBasePhysicManager::RemoveAllPhysicObjects(int flags)
{
	for (const auto &itor : m_physicObjects)
	{
		auto pPhysicObject = itor.second;

		//TODO flags check?

		FreePhysicObject(pPhysicObject);
	}

	m_physicObjects.clear();
}

void CBasePhysicManager::UpdateRagdollObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time)
{
	if (frame_time <= 0)
		return;

	for (auto itor = m_physicObjects.begin(); itor != m_physicObjects.end(); ++itor)
	{
		auto entindex = itor->first;
		auto pPhysicObject = itor->second;
		bool bShouldFree = false;

		if (!bShouldFree)
		{
			if (!ClientEntityManager()->IsEntityEmitted(entindex))
			{
				bShouldFree = true;
			}
		}

		if (!bShouldFree)
		{
			if (!pPhysicObject->Update())
			{
				bShouldFree = true;
			}
		}

		if (bShouldFree)
		{
			FreePhysicObject(pPhysicObject);
			itor = m_physicObjects.erase(itor);
			continue;
		}
	}

}

void CBasePhysicManager::GenerateWorldVertexArray()
{
	FreeWorldVertexArray();

	m_worldVertexArray = new CPhysicVertexArray;

	CPhysicBrushVertex Vertexes[3];

	int iNumFaces = 0;
	int iNumVerts = 0;

	m_worldVertexArray->vFaceBuffer.resize(r_worldmodel->numsurfaces);

	for (int i = 0; i < r_worldmodel->numsurfaces; i++)
	{
		auto surf = GetWorldSurfaceByIndex(i);

		if ((surf->flags & (SURF_DRAWTURB | SURF_UNDERWATER | SURF_DRAWSKY)))
			continue;

		auto poly = surf->polys;

		auto brushface = &m_worldVertexArray->vFaceBuffer[i];

		int iStartVert = iNumVerts;

		brushface->start_vertex = iStartVert;

		for (poly = surf->polys; poly; poly = poly->next)
		{
			auto v = poly->verts[0];

			for (int j = 0; j < 3; j++, v += VERTEXSIZE)
			{
				Vertexes[j].pos[0] = v[0];
				Vertexes[j].pos[1] = v[1];
				Vertexes[j].pos[2] = v[2];
			}

			m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[0]);
			m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[1]);
			m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[2]);

			iNumVerts += 3;

			for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
			{
				Vertexes[1] = Vertexes[2];

				Vertexes[2].pos[0] = v[0];
				Vertexes[2].pos[1] = v[1];
				Vertexes[2].pos[2] = v[2];

				m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[0]);
				m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[1]);
				m_worldVertexArray->vVertexBuffer.emplace_back(Vertexes[2]);

				iNumVerts += 3;
			}
		}

		brushface->num_vertexes = iNumVerts - iStartVert;
	}

	//Always shrink to save system memory
	m_worldVertexArray->vVertexBuffer.shrink_to_fit();
}

void CBasePhysicManager::FreeWorldVertexArray()
{
	if (m_worldVertexArray) {
		delete m_worldVertexArray;
		m_worldVertexArray = NULL;
	}
}

/*
	Purpose : Generate IndexArray for world and all brush models
*/

void CBasePhysicManager::GenerateBrushIndexArray()
{
	FreeAllBrushIndexArray();

	int maxNum = EngineGetMaxKnownModel();

	if ((int)m_brushIndexArray.size() < maxNum)
		m_brushIndexArray.resize(maxNum);

	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);

		if (mod->type == mod_brush && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				m_brushIndexArray[i] = new CPhysicIndexArray;
				GenerateIndexArrayForBrushModel(mod, m_worldVertexArray, m_brushIndexArray[i]);
			}
		}
	}
}

void CBasePhysicManager::FreeAllBrushIndexArray()
{
	for (size_t i = 0; i < m_brushIndexArray.size(); ++i)
	{
		if (m_brushIndexArray[i])
		{
			delete m_brushIndexArray[i];
			m_brushIndexArray[i] = NULL;
		}
	}
	m_brushIndexArray.clear();
}

void CBasePhysicManager::GenerateIndexArrayForBrushModel(model_t* mod, CPhysicVertexArray* vertexArray, CPhysicIndexArray* indexArray)
{
	if (mod == r_worldmodel)
	{
		GenerateIndexArrayRecursiveWorldNode(mod->nodes, vertexArray, indexArray);
	}
	else
	{
		for (int i = 0; i < mod->nummodelsurfaces; i++)
		{
			auto surf = GetWorldSurfaceByIndex(mod->firstmodelsurface + i);

			GenerateIndexArrayForSurface(surf, vertexArray, indexArray);
		}
	}

	//Always shrink to save system memory
	indexArray->vIndexBuffer.shrink_to_fit();
}

void CBasePhysicManager::GenerateIndexArrayForSurface(msurface_t* surf, CPhysicVertexArray* vertexarray, CPhysicIndexArray* indexarray)
{
	if (surf->flags & SURF_DRAWTURB)
	{
		return;
	}

	if (surf->flags & SURF_DRAWSKY)
	{
		return;
	}

	if (surf->flags & SURF_UNDERWATER)
	{
		return;
	}

	auto surfIndex = GetWorldSurfaceIndex(surf);

	GenerateIndexArrayForBrushface(&vertexarray->vFaceBuffer[surfIndex], indexarray);
}

void CBasePhysicManager::GenerateIndexArrayRecursiveWorldNode(mnode_t* node, CPhysicVertexArray* vertexArray, CPhysicIndexArray* indexArray)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents < 0)
		return;

	GenerateIndexArrayRecursiveWorldNode(node->children[0], vertexArray, indexArray);

	for (int i = 0; i < node->numsurfaces; ++i)
	{
		auto surf = GetWorldSurfaceByIndex(node->firstsurface + i);

		GenerateIndexArrayForSurface(surf, vertexArray, indexArray);
	}

	GenerateIndexArrayRecursiveWorldNode(node->children[1], vertexArray, indexArray);
}

void CBasePhysicManager::GenerateIndexArrayForBrushface(CPhysicBrushFace* brushface, CPhysicIndexArray* indexArray)
{
	int first = -1;
	int prv0 = -1;
	int prv1 = -1;
	int prv2 = -1;

	for (int i = 0; i < brushface->num_vertexes; i++)
	{
		if (prv0 != -1 && prv1 != -1 && prv2 != -1)
		{
			indexArray->vIndexBuffer.emplace_back(brushface->start_vertex + first);
			indexArray->vIndexBuffer.emplace_back(brushface->start_vertex + prv2);
		}

		indexArray->vIndexBuffer.emplace_back(brushface->start_vertex + i);

		if (first == -1)
			first = i;

		prv0 = prv1;
		prv1 = prv2;
		prv2 = i;
	}
}

void CBasePhysicManager::GenerateBarnacleIndexVertexArray()
{
	const int BARNACLE_SEGMENTS = 12;

	const  float BARNACLE_RADIUS1 = 22;
	const  float BARNACLE_RADIUS2 = 16;
	const  float BARNACLE_RADIUS3 = 10;

	const float BARNACLE_HEIGHT1 = 0;
	const float BARNACLE_HEIGHT2 = -10;
	const float BARNACLE_HEIGHT3 = -36;

	FreeBarnacleIndexVertexArray();

	m_barnacleVertexArray = new CPhysicVertexArray;
	m_barnacleVertexArray->vVertexBuffer.resize(BARNACLE_SEGMENTS * 8);
	m_barnacleVertexArray->vFaceBuffer.resize(BARNACLE_SEGMENTS * 2);

	int iStartVertex = 0;
	int iNumVerts = 0;
	int iNumFace = 0;

	for (int x = 0; x < BARNACLE_SEGMENTS; x++)
	{
		float xSegment = (float)x / (float)BARNACLE_SEGMENTS;
		float xSegment2 = (float)(x + 1) / (float)BARNACLE_SEGMENTS;

		//layer 1

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT1;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS1;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT1;

		iNumVerts++;

		m_barnacleVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_barnacleVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;

		// layer 2

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT3;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS3;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT3;

		iNumVerts++;

		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * BARNACLE_RADIUS2;
		m_barnacleVertexArray->vVertexBuffer[iNumVerts].pos[2] = BARNACLE_HEIGHT2;

		iNumVerts++;

		m_barnacleVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_barnacleVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;
	}

	m_barnacleIndexArray = new CPhysicIndexArray;

	for (int i = 0; i < (int)m_barnacleVertexArray->vFaceBuffer.size(); i++)
	{
		GenerateIndexArrayForBrushface(&m_barnacleVertexArray->vFaceBuffer[i], m_barnacleIndexArray);
	}
}

void CBasePhysicManager::FreeBarnacleIndexVertexArray()
{
	if (m_barnacleVertexArray)
	{
		delete m_barnacleVertexArray;
		m_barnacleVertexArray = NULL;
	}

	if (m_barnacleIndexArray)
	{
		delete m_barnacleIndexArray;
		m_barnacleIndexArray = NULL;
	}
}

void CBasePhysicManager::GenerateGargantuaIndexVertexArray()
{
	const int GARGANTUA_SEGMENTS = 12;

	const float GARGANTUA_RADIUS1 = 16;
	const float GARGANTUA_RADIUS2 = 14;
	const float GARGANTUA_RADIUS3 = 12;

	const float GARGANTUA_HEIGHT1 = 8;
	const float GARGANTUA_HEIGHT2 = -8;
	const float GARGANTUA_HEIGHT3 = -24;

	FreeGargantuaIndexVertexArray();

	m_gargantuaVertexArray = new CPhysicVertexArray;
	m_gargantuaVertexArray->vVertexBuffer.resize(GARGANTUA_SEGMENTS * (4 + 4));// + 3
	m_gargantuaVertexArray->vFaceBuffer.resize(GARGANTUA_SEGMENTS * 2);

	int iStartVertex = 0;
	int iNumVerts = 0;
	int iNumFace = 0;

	for (int x = 0; x < GARGANTUA_SEGMENTS; x++)
	{
		float xSegment = (float)x / (float)GARGANTUA_SEGMENTS;
		float xSegment2 = (float)(x + 1) / (float)GARGANTUA_SEGMENTS;

		//layer 1

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT1;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;
		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS1;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT1;
		iNumVerts++;

		m_gargantuaVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_gargantuaVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;

		// layer 2

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;

		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT3;

		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS3;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT3;

		iNumVerts++;

		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[0] = std::sin(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[1] = std::cos(xSegment2 * 2 * M_PI) * GARGANTUA_RADIUS2;
		m_gargantuaVertexArray->vVertexBuffer[iNumVerts].pos[2] = GARGANTUA_HEIGHT2;

		iNumVerts++;

		m_gargantuaVertexArray->vFaceBuffer[iNumFace].start_vertex = iStartVertex;
		m_gargantuaVertexArray->vFaceBuffer[iNumFace].num_vertexes = 4;
		iNumFace++;

		iStartVertex = iNumVerts;
	}

	m_gargantuaIndexArray = new CPhysicIndexArray;

	for (int i = 0; i < (int)m_gargantuaVertexArray->vFaceBuffer.size(); i++)
	{
		if (i >= 3 * 2 && i < 8 * 2)
			continue;

		GenerateIndexArrayForBrushface(&m_gargantuaVertexArray->vFaceBuffer[i], m_gargantuaIndexArray);
	}
}

void CBasePhysicManager::FreeGargantuaIndexVertexArray()
{
	if (m_gargantuaVertexArray)
	{
		delete m_gargantuaVertexArray;
		m_gargantuaVertexArray = NULL;
	}

	if (m_gargantuaIndexArray)
	{
		delete m_gargantuaIndexArray;
		m_gargantuaIndexArray = NULL;
	}
}

CPhysicIndexArray * CBasePhysicManager::GetIndexArrayFromBrushModel(model_t *mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex < 0 || modelindex > (int)m_brushIndexArray.size())
	{
		return NULL;
	}

	return m_brushIndexArray[modelindex];
}
