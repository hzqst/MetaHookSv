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

#include <sstream>
#include <ScopeExit/ScopeExit.h>

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
	RemoveAllPhysicObjects(PhysicObjectFlag_Any);
	GenerateWorldVertexArray();
	GenerateBrushIndexArray();
	GenerateBarnacleIndexVertexArray();
	GenerateGargantuaIndexVertexArray();
	CreatePhysicObjectForBrushModel(r_worldentity);
}

void CBasePhysicManager::DebugDraw(void)
{
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
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

static CClientPhysicConfig* LoadPhysicConfigFromNewFileBuffer(const char* buf)
{
	//TODO
	return nullptr;
}

static CClientPhysicConfig* LoadPhysicConfigFromNewFile(const std::string& filename)
{
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return nullptr;

	SCOPE_EXIT{ gEngfuncs.COM_FreeFile((void*)pFileContent); };

	return LoadPhysicConfigFromNewFileBuffer(pFileContent);
}

static std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return "";
	}
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, last - first + 1);
}

static bool ParseDeathAnimLine(CClientRagdollConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);
	int sequence;
	float frame;
	if (iss >> sequence >> frame) {
		CClientRagdollAnimControlConfig animConfig;
		animConfig.sequence = sequence;
		animConfig.frame = frame;
		animConfig.activity = 1;

		pRagdollConfig->AnimControlConfigs.push_back(animConfig);
		return true;
	}
	return false;
}

static bool ParseRigidBodyLine(CClientRagdollConfig* pRagdollConfig, const std::string& line, int flags) {
	std::istringstream iss(line);
	std::string name, shapeType;
	int boneIndex, pBoneIndex;
	float pBoneOffset, size0, size1, mass;

	if (iss >> name >> boneIndex >> pBoneIndex >> shapeType >> pBoneOffset >> size0 >> size1 >> mass) {
		auto rigidBodyConfig = new CClientRigidBodyConfig();
		rigidBodyConfig->name = name;
		rigidBodyConfig->boneindex = boneIndex;
		rigidBodyConfig->pboneindex = pBoneIndex;
		rigidBodyConfig->pboneoffset = pBoneOffset;
		rigidBodyConfig->mass = mass;
		rigidBodyConfig->flags = flags;
		rigidBodyConfig->isLegacyConfig = true;

		auto shapeConfig = new CClientCollisionShapeConfig();
		if (shapeType == "sphere") {
			shapeConfig->type = PhysicShape_Sphere;
			shapeConfig->size[0] = size0;
		}
		else if (shapeType == "capsule") {
			shapeConfig->type = PhysicShape_Capsule;
			shapeConfig->size[0] = size0;
			shapeConfig->size[1] = size1;
		}
		else {
			delete rigidBodyConfig;
			delete shapeConfig;
			return false; // Unsupported shape type
		}

		rigidBodyConfig->shapes.push_back(shapeConfig);
		pRagdollConfig->RigidBodyConfigs.push_back(rigidBodyConfig);
		return true;
	}
	return false;
}

static bool ParseConstraintLine(CClientRagdollConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);
	std::string rigidbodyA, rigidbodyB, constraintType;
	int boneindexA, boneindexB;
	float offsetAX, offsetAY, offsetAZ;
	float offsetBX, offsetBY, offsetBZ;
	float factor0, factor1, factor2;

	if (iss >> rigidbodyA >> rigidbodyB >> constraintType >> boneindexA >> boneindexB
		>> offsetAX >> offsetAY >> offsetAZ >> offsetBX >> offsetBY >> offsetBZ
		>> factor0 >> factor1 >> factor2) {
		auto constraintConfig = new CClientConstraintConfig();
		constraintConfig->rigidbodyA = rigidbodyA;
		constraintConfig->rigidbodyB = rigidbodyB;
		constraintConfig->boneindexA = boneindexA;
		constraintConfig->boneindexB = boneindexB;
		constraintConfig->offsetA[0] = offsetAX;
		constraintConfig->offsetA[1] = offsetAY;
		constraintConfig->offsetA[2] = offsetAZ;
		constraintConfig->offsetB[0] = offsetBX;
		constraintConfig->offsetB[1] = offsetBY;
		constraintConfig->offsetB[2] = offsetBZ;
		constraintConfig->factors[0] = factor0;
		constraintConfig->factors[1] = factor1;
		constraintConfig->factors[2] = factor2;
		constraintConfig->isLegacyConfig = true;

		if (constraintType == "point" || constraintType == "point_collision") {
			constraintConfig->type = PhysicConstraint_Point;
		}
		else if (constraintType == "conetwist" || constraintType == "conetwist_collision") {
			constraintConfig->type = PhysicConstraint_ConeTwist;
		}
		else if (constraintType == "hinge" || constraintType == "hinge_collision") {
			constraintConfig->type = PhysicConstraint_Hinge;
		}
		else {
			delete constraintConfig;
			return false; // Unsupported constraint type
		}

		if (constraintType.ends_with("_collision")) {
			constraintConfig->disableCollision = false;
		}

		pRagdollConfig->ConstraintConfigs.push_back(constraintConfig);
		return true;
	}
	return false;
}

CClientPhysicConfig *LoadPhysicConfigFromLegacyFileBuffer(const char *buf)
{
	auto pRagdollConfig = new CClientRagdollConfig();

	std::istringstream stream(buf);
	std::string line;
	std::string section;

	while (std::getline(stream, line)) {
		// Trim whitespace
		line = trim(line);

		// Skip empty lines and comments
		if (line.empty() || line[0] == '/' || line[0] == '#') {
			continue;
		}

		// Check for section headers
		if (line[0] == '[') {
			size_t end = line.find(']');
			if (end != std::string::npos) {
				section = line.substr(1, end - 1);
			}
			continue;
		}

		// Parse the content based on the current section
		if (section == "DeathAnim") {
			if (!ParseDeathAnimLine(pRagdollConfig, line)) {
				delete pRagdollConfig;
				return nullptr; // Parsing failed
			}
		}
		else if (section == "RigidBody") {
			if (!ParseRigidBodyLine(pRagdollConfig, line, 0)) {
				delete pRagdollConfig;
				return nullptr; // Parsing failed
			}
		}
		else if (section == "JiggleBone") {
			if (!ParseRigidBodyLine(pRagdollConfig, line, PhysicRigidBodyFlag_Jiggle)) {
				delete pRagdollConfig;
				return nullptr; // Parsing failed
			}
		}
		else if (section == "Constraint") {
			if (!ParseConstraintLine(pRagdollConfig, line)) {
				delete pRagdollConfig;
				return nullptr; // Parsing failed
			}
		}
		else {

		}
	}

	return pRagdollConfig;
}

static CClientPhysicConfig* LoadPhysicConfigFromLegacyFile(const std::string& filename)
{
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return nullptr;

	SCOPE_EXIT{ gEngfuncs.COM_FreeFile((void*)pFileContent); };

	return LoadPhysicConfigFromLegacyFileBuffer(pFileContent);
}

void CBasePhysicManager::LoadPhysicConfigFromFiles(CClientPhysicConfigStorage &Storage, const std::string& filename)
{
	std::string fullname = filename;

	if (fullname.length() < 4)
	{
		gEngfuncs.Con_DPrintf("LoadPhysicConfigFromFiles: Invalid name \"%s\"\n", filename.c_str());
		Storage.state = PhysicConfigState_LoadedWithError;
		return;
	}

	fullname = fullname.substr(0, fullname.length() - 4);

	auto fullname_physic = fullname;
	fullname_physic += "_physic.txt";

	auto pConfig = LoadPhysicConfigFromNewFile(fullname_physic);

	if(pConfig)
	{
		Storage.pConfig = pConfig;
		Storage.state = PhysicConfigState_Loaded;
		gEngfuncs.Con_DPrintf("LoadPhysicConfigFromFiles:\"%s\" has been loaded successfully.\n", fullname.c_str());
		return;
	}

	auto fullname_ragdoll = fullname;
	fullname_ragdoll  += "_ragdoll.txt";

	pConfig = LoadPhysicConfigFromLegacyFile(fullname_ragdoll);

	if (pConfig)
	{
		Storage.pConfig = pConfig;
		Storage.state = PhysicConfigState_Loaded;
		gEngfuncs.Con_DPrintf("LoadPhysicConfigFromFiles:\"%s\" has been loaded successfully.\n", fullname.c_str());
		return;
	}

	gEngfuncs.Con_DPrintf("LoadPhysicConfigFromFiles: Could not load physic configs for \"%s\".\n", fullname.c_str());
	Storage.state = PhysicConfigState_LoadedWithError;
}

CClientPhysicConfig* CBasePhysicManager::LoadPhysicConfigForModel(model_t* mod)
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

	auto& Storage = m_physicConfigs[modelindex];

	if (Storage.state != PhysicConfigState_NotLoaded)
		return Storage.pConfig;

	Storage.pConfig = new CClientPhysicConfig;

	LoadPhysicConfigFromFiles(Storage, mod->name);

	return Storage.pConfig;
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
			auto entindex = ent->index;
			auto model = ent->model;

			if (ClientEntityManager()->IsEntityBarnacle(ent))
			{
				ClientEntityManager()->AddBarnacle(entindex, 0);
			}
			else if (ClientEntityManager()->IsEntityGargantua(ent))
			{
				ClientEntityManager()->AddGargantua(entindex, 0);
			}

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

void CBasePhysicManager::SetupIdleBonesForRagdoll(cl_entity_t* ent, model_t* mod, int entindex, int playerindex, const CClientRagdollAnimControlConfig & idleAnim)
{
	auto pSavedCurrentEntity = (*currententity);
	(*currententity) = ent;

	if (playerindex > 0)
	{
		const auto playerState = R_GetPlayerState(playerindex);

		auto fakePlayerState = *playerState;

		fakePlayerState.number = playerindex;
		fakePlayerState.weaponmodel = 0;
		fakePlayerState.sequence = idleAnim.sequence;
		fakePlayerState.gaitsequence = idleAnim.gaitsequence;
		fakePlayerState.frame = idleAnim.frame;

		VectorCopy(ent->curstate.angles, fakePlayerState.angles);
		VectorCopy(ent->curstate.origin, fakePlayerState.origin);

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL, &fakePlayerState);
	}
	else
	{
		int iWeaponModel = ent->curstate.weaponmodel;
		int iSavedSequence = ent->curstate.sequence;
		int iSavedGaitSequence = ent->curstate.gaitsequence;
		float flSavedFrame = ent->curstate.frame;

		ent->curstate.weaponmodel = 0;
		ent->curstate.sequence = idleAnim.sequence;
		ent->curstate.gaitsequence = idleAnim.gaitsequence;
		ent->curstate.frame = idleAnim.frame;

		(*gpStudioInterface)->StudioDrawModel(STUDIO_RAGDOLL);

		ent->curstate.weaponmodel = 0;
		ent->curstate.sequence = iSavedSequence;
		ent->curstate.gaitsequence = iSavedGaitSequence;
		ent->curstate.frame = flSavedFrame;
	}

	(*currententity) = pSavedCurrentEntity;
}

void CBasePhysicManager::CreatePhysicObjectFromConfig(cl_entity_t* ent, model_t* mod, int entindex, int playerindex)
{
	auto pPhysicConfig = ClientPhysicManager()->LoadPhysicConfigForModel(mod);

	if (!pPhysicConfig)
		return;

	if (pPhysicConfig->type == PhysicConfigType_Ragdoll)
	{
		auto pRagdollConfig = (CClientRagdollConfig*)pPhysicConfig;

		SetupIdleBonesForRagdoll(ent, mod, entindex, playerindex, pRagdollConfig->IdleAnimConfig);

		CRagdollObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_model = mod;
		CreationParam.m_entindex = entindex;
		CreationParam.m_playerindex = playerindex;
		CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
		CreationParam.m_pRagdollConfig = pRagdollConfig;

		auto pRagdollObject = CreateRagdollObject(CreationParam);

		if (!pRagdollObject)
			return;

		AddPhysicObject(entindex, pRagdollObject);
	}
	else if (pPhysicConfig->type == PhysicConfigType_Dynamic)
	{
		//TODO
		//CDynamicObjectCreationParameter CreationParam;
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

	CreationParam.m_debugDrawLevel = (ent == r_worldentity) ? 2 : 1;

	auto pStaticObject = CreateStaticObject(CreationParam);

	if (!pStaticObject)
		return;

	AddPhysicObject(entindex, pStaticObject);
}

void CBasePhysicManager::CreateBarnacle(cl_entity_t* ent)
{
	//TODO... use ragdoll instead?
#if 0
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
#endif
}

void CBasePhysicManager::CreateGargantua(cl_entity_t* ent)
{
	//TODO... use ragdoll instead?
}

void CBasePhysicManager::AddPhysicObject(int entindex, IPhysicObject* pPhysicObject)
{
	RemovePhysicObject(entindex);

	AddPhysicObjectToWorld(pPhysicObject);

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
	for (auto itor = m_physicObjects.begin(); itor != m_physicObjects.end();)
	{
		auto entindex = itor->first;

		auto pPhysicObject = itor->second;

		if (pPhysicObject->GetObjectFlags() & flags)
		{
			FreePhysicObject(pPhysicObject);
			itor = m_physicObjects.erase(itor);
			continue;
		}

		itor++;
	}

	m_physicObjects.clear();
}

void CBasePhysicManager::UpdateRagdollObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time)
{
	if (frame_time <= 0)
		return;

	for (auto itor = m_physicObjects.begin(); itor != m_physicObjects.end();)
	{
		auto entindex = itor->first;
		auto pPhysicObject = itor->second;
		bool bShouldFree = false;

		if (!bShouldFree)
		{
			//world entity is always present
			if (entindex > 0 && !ClientEntityManager()->IsEntityEmitted(entindex))
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

		itor++;
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
