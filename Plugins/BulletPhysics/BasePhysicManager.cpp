#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "plugins.h"
#include "CounterStrike.h"
#include "BasePhysicManager.h"
#include "ClientEntityManager.h"
#include "mathlib2.h"
#include "util.h"

#include <sstream>
#include <ScopeExit/ScopeExit.h>

#include <KeyValues.h>

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
	RemoveAllPhysicObjects(PhysicObjectFlag_AnyObject);
	LoadPhysicObjectConfigs();
	GenerateWorldVertexArray();
	GenerateBrushIndexArray();

	//Deprecated, use .obj instead.
	//GenerateBarnacleIndexVertexArray();
	//GenerateGargantuaIndexVertexArray();

	CreatePhysicObjectForBrushModel(r_worldentity, &r_worldentity->curstate, r_worldmodel);
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
	for (auto& Storage : m_physicConfigs)
	{
		if (Storage.pConfig)
		{
			delete Storage.pConfig;
			Storage.pConfig = nullptr;
		}
		Storage.state = PhysicConfigState_NotLoaded;
	}

	m_physicConfigs.clear();
}

void CBasePhysicManager::LoadPhysicObjectConfigs(void)
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
					LoadPhysicObjectConfigForModel(mod);
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
	auto pPhysicObject = GetPhysicObject(old_entindex);

	if (pPhysicObject)
	{
		m_physicObjects.erase(old_entindex);

		RemovePhysicObject(new_entindex);

		m_physicObjects[new_entindex] = pPhysicObject;

		pPhysicObject->TransformOwnerEntity(new_entindex);

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

static void LoadBaseConfigsFromKeyValues(KeyValues* pKeyValues, CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	if (pKeyValues->GetInt("barnacle") == 1)
	{
		pPhysicObjectConfig->flags |= PhysicObjectFlag_Barnacle;
	}

	if (pKeyValues->GetInt("gargantua") == 1)
	{
		pPhysicObjectConfig->flags |= PhysicObjectFlag_Gargantua;
	}

	auto pRigidBodiesKey = pKeyValues->FindKey("RigidBodies");

	if (pRigidBodiesKey)
	{
		for (auto pRigidBodySubKey = pRigidBodiesKey->GetFirstSubKey(); pRigidBodySubKey; pRigidBodySubKey = pRigidBodySubKey->GetNextKey())
		{
			auto pRigidBodyConfig = new CClientRigidBodyConfig();

			pRigidBodyConfig->name = pRigidBodySubKey->GetName();
			pRigidBodyConfig->flags = pRigidBodySubKey->GetInt("flags");
			pRigidBodyConfig->debugDrawLevel = pRigidBodySubKey->GetInt("debugDrawLevel", 1);
			pRigidBodyConfig->boneindex = pRigidBodySubKey->GetInt("boneindex", -1);

			auto origin = pRigidBodySubKey->GetString("origin");

			if (origin)
			{
				UTIL_ParseStringAsVector3(origin, pRigidBodyConfig->origin);
			}

			auto angles = pRigidBodySubKey->GetString("angles");

			if (angles)
			{
				UTIL_ParseStringAsVector3(angles, pRigidBodyConfig->angles);
			}

			pRigidBodyConfig->mass = pRigidBodySubKey->GetFloat("mass", BULLET_DEFAULT_MASS);
			pRigidBodyConfig->density = pRigidBodySubKey->GetFloat("density", BULLET_DEFAULT_DENSENTY);
			pRigidBodyConfig->linearFriction = pRigidBodySubKey->GetFloat("linearFriction", BULLET_DEFAULT_LINEAR_FIRCTION);
			pRigidBodyConfig->rollingFriction = pRigidBodySubKey->GetFloat("rollingFriction", BULLET_DEFAULT_ANGULAR_FIRCTION);
			pRigidBodyConfig->restitution = pRigidBodySubKey->GetFloat("restitution", BULLET_DEFAULT_RESTITUTION);
			pRigidBodyConfig->ccdRadius = pRigidBodySubKey->GetFloat("ccdRadius", 0);
			pRigidBodyConfig->ccdThreshold = pRigidBodySubKey->GetFloat("ccdThreshold", 0);
			pRigidBodyConfig->linearSleepingThreshold = pRigidBodySubKey->GetFloat("linearSleepingThreshold", BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD);
			pRigidBodyConfig->angularSleepingThreshold = pRigidBodySubKey->GetFloat("angularSleepingThreshold", BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD);

			auto pShapesKey = pRigidBodySubKey->FindKey("shapes");

			if (pShapesKey)
			{
				for (auto pShapeSubKey = pShapesKey->GetFirstSubKey(); pShapeSubKey; pShapeSubKey = pShapeSubKey->GetNextKey())
				{
					auto pShapeConfig = new CClientCollisionShapeConfig();

					pShapeConfig->name = pShapeSubKey->GetName();

					auto type = pShapeSubKey->GetString("type");

					if (type)
					{
						if (!strcmp(type, "Box"))
						{
							pShapeConfig->type = PhysicShape_Box;
						}
						if (!strcmp(type, "Sphere"))
						{
							pShapeConfig->type = PhysicShape_Sphere;
						}
						if (!strcmp(type, "Capsule"))
						{
							pShapeConfig->type = PhysicShape_Capsule;
						}
						if (!strcmp(type, "Cylinder"))
						{
							pShapeConfig->type = PhysicShape_Cylinder;
						}
						if (!strcmp(type, "MultiSphere"))
						{
							pShapeConfig->type = PhysicShape_MultiSphere;
						}
						if (!strcmp(type, "TriangleMesh"))
						{
							pShapeConfig->type = PhysicShape_TriangleMesh;
							pShapeConfig->objpath = pShapeSubKey->GetString("objpath");
						}
					}

					pShapeConfig->direction = pShapeSubKey->GetInt("direction");

					auto origin = pShapeSubKey->GetString("origin");

					if (origin)
					{
						UTIL_ParseStringAsVector3(origin, pShapeConfig->origin);
					}

					auto angles = pShapeSubKey->GetString("angles");

					if (angles)
					{
						UTIL_ParseStringAsVector3(angles, pShapeConfig->angles);
					}

					auto size = pShapeSubKey->GetString("size");

					if (size)
					{
						if (!UTIL_ParseStringAsVector3(size, pShapeConfig->size))
						{
							if (!UTIL_ParseStringAsVector2(size, pShapeConfig->size))
							{
								UTIL_ParseStringAsVector1(size, pShapeConfig->size);
							}
						}
					}

					pRigidBodyConfig->shapes.emplace_back(pShapeConfig);
				}
			}

			pPhysicObjectConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);
		}
	}
}

static CClientPhysicObjectConfig* LoadRagdollObjectConfigFromKeyValues(KeyValues* pKeyValues)
{
	auto pRagdollObjectConfig = new CClientRagdollObjectConfig();

	LoadBaseConfigsFromKeyValues(pKeyValues, pRagdollObjectConfig);

	return pRagdollObjectConfig;
}

static CClientPhysicObjectConfig* LoadStaticObjectConfigFromKeyValues(KeyValues* pKeyValues)
{
	auto pStaticObjectConfig = new CClientStaticObjectConfig();

	LoadBaseConfigsFromKeyValues(pKeyValues, pStaticObjectConfig);

	return pStaticObjectConfig;
}

static CClientPhysicObjectConfig* LoadDynamicObjectConfigFromKeyValues(KeyValues* pKeyValues)
{
	//TODO
	return nullptr;
}

static CClientPhysicObjectConfig* LoadPhysicObjectConfigFromKeyValues(KeyValues *pKeyValues)
{
	auto type = pKeyValues->GetString("type");

	if (type)
	{
		if (!strcmp(type, "RagdollObject"))
		{
			return LoadRagdollObjectConfigFromKeyValues(pKeyValues);
		}
		else if (!strcmp(type, "StaticObject"))
		{
			return LoadStaticObjectConfigFromKeyValues(pKeyValues);
		}
		else if (!strcmp(type, "DynamicObject"))
		{
			return LoadDynamicObjectConfigFromKeyValues(pKeyValues);
		}
	}

	gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromKeyValues: invalid \"type\" from KeyValues!\n");

	return nullptr;
}

static CClientPhysicObjectConfig* LoadPhysicObjectConfigFromNewFile(const std::string& filename)
{
	auto pKeyValues = new KeyValues("PhysicObjectConfig");

	SCOPE_EXIT{ delete pKeyValues; };

	bool bLoaded = false;

	if (g_pFileSystem_HL25)
	{
		bLoaded = pKeyValues->LoadFromFile((IFileSystem*)g_pFileSystem_HL25, filename.c_str());
	}
	else
	{
		bLoaded = pKeyValues->LoadFromFile(g_pFileSystem, filename.c_str());
	}

	if (!bLoaded)
		return nullptr;

	return LoadPhysicObjectConfigFromKeyValues(pKeyValues);

#if 0
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return nullptr;

	SCOPE_EXIT{ gEngfuncs.COM_FreeFile((void*)pFileContent); };

	return LoadPhysicObjectConfigFromNewFileBuffer(pFileContent);
#endif
}

static std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return "";
	}
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, last - first + 1);
}

static bool ParseDeathAnimLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
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

static bool ParseRigidBodyLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line, int flags) {
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
			rigidBodyConfig->ccdRadius = size0 * 0.2f;
		}
		else if (shapeType == "capsule") {
			shapeConfig->type = PhysicShape_Capsule;
			shapeConfig->direction = PhysicShapeDirection_Y;
			shapeConfig->size[0] = size0;
			shapeConfig->size[1] = size1;
			rigidBodyConfig->ccdRadius = max(size0, size1 * 0.5f) * 0.2f;
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

static bool ParseConstraintLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
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

		for (int i = 0; i < _ARRAYSIZE(constraintConfig->factors); ++i)
		{
			constraintConfig->factors[i] = NAN;
		}
		
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

CClientPhysicObjectConfig*LoadPhysicObjectConfigFromLegacyFileBuffer(const char *buf)
{
	auto pRagdollConfig = new CClientRagdollObjectConfig();

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

static CClientPhysicObjectConfig* LoadPhysicObjectConfigFromLegacyFile(const std::string& filename)
{
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return nullptr;

	SCOPE_EXIT{ gEngfuncs.COM_FreeFile((void*)pFileContent); };

	return LoadPhysicObjectConfigFromLegacyFileBuffer(pFileContent);
}

void CBasePhysicManager::LoadPhysicObjectConfigFromFiles(CClientPhysicObjectConfigStorage &Storage, const std::string& filename)
{
	std::string fullname = filename;

	if (fullname.length() < 4)
	{
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles: Invalid name \"%s\"\n", filename.c_str());
		Storage.state = PhysicConfigState_LoadedWithError;
		return;
	}

	fullname = fullname.substr(0, fullname.length() - 4);

	auto fullname_physic = fullname;
	fullname_physic += "_physics.txt";

	auto pConfig = LoadPhysicObjectConfigFromNewFile(fullname_physic);

	if(pConfig)
	{
		Storage.pConfig = pConfig;
		Storage.state = PhysicConfigState_Loaded;
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles:\"%s\" has been loaded successfully.\n", fullname.c_str());
		return;
	}

	auto fullname_ragdoll = fullname;
	fullname_ragdoll  += "_ragdoll.txt";

	pConfig = LoadPhysicObjectConfigFromLegacyFile(fullname_ragdoll);

	if (pConfig)
	{
		Storage.pConfig = pConfig;
		Storage.state = PhysicConfigState_Loaded;
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles:\"%s\" has been loaded successfully.\n", fullname.c_str());
		return;
	}

	gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles: Could not load physic configs for \"%s\".\n", fullname.c_str());
	Storage.state = PhysicConfigState_LoadedWithError;
}

CClientPhysicObjectConfig* CBasePhysicManager::LoadPhysicObjectConfigForModel(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex == -1)
	{
		g_pMetaHookAPI->SysError("LoadPhysicObjectConfigForModel: Invalid model index\n");
		return nullptr;
	}

	if (m_physicConfigs.size() < EngineGetMaxKnownModel())
		m_physicConfigs.resize(EngineGetMaxKnownModel());

	if (modelindex >= m_physicConfigs.size())
	{
		g_pMetaHookAPI->SysError("LoadPhysicObjectConfig: Invalid model index\n");
		return nullptr;
	}

	auto& Storage = m_physicConfigs[modelindex];

	if (Storage.state == PhysicConfigState_NotLoaded)
		LoadPhysicObjectConfigFromFiles(Storage, mod->name);

	return Storage.pConfig;
}

void CBasePhysicManager::CreatePhysicObjectForEntity(cl_entity_t* ent, entity_state_t* state, model_t *mod)
{
	auto saved_currententity = (*currententity);
	(*currententity) = ent;

	if (mod->type == mod_studio)
	{
		CreatePhysicObjectForStudioModel(ent, state, mod);
	}
	else if (mod->type == mod_brush && state->solid == SOLID_BSP)
	{
		CreatePhysicObjectForBrushModel(ent, state, mod);
	}

	(*currententity) = saved_currententity;
}

void CBasePhysicManager::CreatePhysicObjectForStudioModel(cl_entity_t* ent, entity_state_t* state, model_t* mod)
{
	//TODO Port barnacle and garg to Configs ?
	if (ClientEntityManager()->IsEntityNetworkEntity(ent))
	{
		if (ClientEntityManager()->IsEntityDeadPlayer(ent))
		{
			auto entindex = ent->index;
			auto playerindex = state->renderamt;
			auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

			if (!model)
				return;

			if (g_bIsCounterStrike)
			{
				//Counter-Strike redirects playermodel in a pretty tricky way
				int modelindex = 0;
				model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
			}

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && 
				PhysicObject->GetModel() == model &&
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model) &&
				PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				if (TransformOwnerEntityForPhysicObject(playerindex, entindex))
					return;

				CreatePhysicObjectFromConfig(ent, state, model, entindex, playerindex);
			}
		}
		else if (ClientEntityManager()->IsEntityPlayer(ent))
		{
			auto entindex = ent->index;
			auto playerindex = state->number;
			auto model = IEngineStudio.SetupPlayerModel(playerindex - 1);

			if (!model)
				return;

			if (g_bIsCounterStrike)
			{
				//Counter-Strike redirects playermodel in a pretty tricky way
				int modelindex = 0;
				model = CounterStrike_RedirectPlayerModel(model, playerindex, &modelindex);
			}

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && 
				PhysicObject->GetModel() == model && 
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model) &&
				PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				CreatePhysicObjectFromConfig(ent, state, model, entindex, playerindex);
			}
		}
		else
		{
			auto entindex = ent->index;
			auto model = mod;

			if (!model)
				return;

			if (ClientEntityManager()->IsEntityBarnacle(ent))
			{
				//I don't think we need this anymore
				//ClientEntityManager()->AddBarnacle(entindex, 0);
			}
			else if (ClientEntityManager()->IsEntityGargantua(ent))
			{
				//I don't think we need this anymore
				//ClientEntityManager()->AddGargantua(entindex, 0);
			}

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject &&
				PhysicObject->GetModel() == model && 
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model))
			{

			}
			else
			{
				CreatePhysicObjectFromConfig(ent, state, model, entindex, 0);
			}
		}
	}
	else if (ClientEntityManager()->IsEntityTempEntity(ent))
	{
		if (ClientEntityManager()->IsEntityClientCorpse(ent))
		{
			auto entindex = ClientEntityManager()->GetEntityIndex(ent);
			auto playerindex = ent->curstate.owner;
			auto model = mod;

			if (!model)
				return;

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject &&
				PhysicObject->GetModel() == model &&
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model) &&
				PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				if (TransformOwnerEntityForPhysicObject(playerindex, entindex))
					return;

				CreatePhysicObjectFromConfig(ent, state, model, entindex, playerindex);
			}
		}
		else
		{
			//TODO other tempents are not supported yet?
		}
	}
}

void CBasePhysicManager::UpdateBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex)
{
	auto saved_currententity = (*currententity);
	(*currententity) = ent;

	if (playerindex > 0)
	{
		auto fakePlayerState = *state;

		fakePlayerState.number = playerindex;

		vec3_t vecSavedOrigin, vecSavedAngles;
		VectorCopy((*currententity)->origin, vecSavedOrigin);
		VectorCopy((*currententity)->angles, vecSavedAngles);

		auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

		if (pLocalPlayer && pLocalPlayer->index == playerindex)
		{
			VectorCopy(r_params.simorg, (*currententity)->origin);
			VectorCopy((*currententity)->curstate.angles, (*currententity)->angles);
		}

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL_UPDATE_BONES, &fakePlayerState);

		VectorCopy(vecSavedOrigin, (*currententity)->origin);
		VectorCopy(vecSavedAngles, (*currententity)->angles);
	}
	else
	{
		(*gpStudioInterface)->StudioDrawModel(STUDIO_RAGDOLL_UPDATE_BONES);
	}

	(*currententity) = saved_currententity;
}

void CBasePhysicManager::SetupBonesForRagdoll(cl_entity_t* ent, entity_state_t* state, model_t* mod, int entindex, int playerindex)
{
	auto saved_currententity = (*currententity);
	(*currententity) = ent;

	if (playerindex > 0)
	{
		auto fakePlayerState = *state;

		fakePlayerState.number = playerindex;
		fakePlayerState.weaponmodel = 0;

		vec3_t vecSavedOrigin, vecSavedAngles;
		VectorCopy((*currententity)->origin, vecSavedOrigin);
		VectorCopy((*currententity)->angles, vecSavedAngles);

		auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

		if (pLocalPlayer && pLocalPlayer->index == playerindex)
		{
			VectorCopy(r_params.simorg, (*currententity)->origin);
			VectorCopy((*currententity)->curstate.angles, (*currententity)->angles);
		}

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL_SETUP_BONES, &fakePlayerState);

		VectorCopy(vecSavedOrigin, (*currententity)->origin);
		VectorCopy(vecSavedAngles, (*currententity)->angles);
	}
	else
	{
		int iWeaponModel = ent->curstate.weaponmodel;

		ent->curstate.weaponmodel = 0;

		(*gpStudioInterface)->StudioDrawModel(STUDIO_RAGDOLL_SETUP_BONES);

		ent->curstate.weaponmodel = 0;
	}

	(*currententity) = saved_currententity;
}

void CBasePhysicManager::SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t *state, model_t* mod, int entindex, int playerindex, const CClientRagdollAnimControlConfig &OverrideAnim)
{
	auto saved_currententity = (*currententity);
	(*currententity) = ent;

	if (playerindex > 0)
	{
		auto fakePlayerState = *state;

		fakePlayerState.number = playerindex;
		fakePlayerState.weaponmodel = 0;
		fakePlayerState.sequence = OverrideAnim.sequence;
		fakePlayerState.gaitsequence = OverrideAnim.gaitsequence;
		fakePlayerState.frame = OverrideAnim.frame;

		vec3_t vecSavedOrigin, vecSavedAngles;
		VectorCopy((*currententity)->origin, vecSavedOrigin);
		VectorCopy((*currententity)->angles, vecSavedAngles);

		auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

		if (pLocalPlayer && pLocalPlayer->index == playerindex)
		{
			VectorCopy(r_params.simorg, (*currententity)->origin);
			VectorCopy((*currententity)->curstate.angles, (*currententity)->angles);
		}

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL_SETUP_BONES, &fakePlayerState);

		VectorCopy(vecSavedOrigin, (*currententity)->origin);
		VectorCopy(vecSavedAngles, (*currententity)->angles);
	}
	else
	{
		int iWeaponModel = ent->curstate.weaponmodel;
		int iSavedSequence = ent->curstate.sequence;
		int iSavedGaitSequence = ent->curstate.gaitsequence;
		float flSavedFrame = ent->curstate.frame;

		ent->curstate.weaponmodel = 0;
		ent->curstate.sequence = OverrideAnim.sequence;
		ent->curstate.gaitsequence = OverrideAnim.gaitsequence;
		ent->curstate.frame = OverrideAnim.frame;

		(*gpStudioInterface)->StudioDrawModel(STUDIO_RAGDOLL_SETUP_BONES);

		ent->curstate.weaponmodel = 0;
		ent->curstate.sequence = iSavedSequence;
		ent->curstate.gaitsequence = iSavedGaitSequence;
		ent->curstate.frame = flSavedFrame;
	}

	(*currententity) = saved_currententity;
}

void CBasePhysicManager::CreatePhysicObjectFromConfig(cl_entity_t* ent, entity_state_t *state, model_t* mod, int entindex, int playerindex)
{
	auto pPhysicConfig = ClientPhysicManager()->LoadPhysicObjectConfigForModel(mod);

	if (!pPhysicConfig)
		return;

	if (pPhysicConfig->type == PhysicConfigType_RagdollObject)
	{
		auto pRagdollObjectConfig = (CClientRagdollObjectConfig*)pPhysicConfig;

		SetupBonesForRagdollEx(ent, state, mod, entindex, playerindex, pRagdollObjectConfig->IdleAnimConfig);

		CRagdollObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_model = mod;
		CreationParam.m_entindex = entindex;
		CreationParam.m_playerindex = playerindex;
		CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
		CreationParam.m_pRagdollObjectConfig = pRagdollObjectConfig;

		LoadAdditionalResourcesForConfig(pPhysicConfig);

		auto pRagdollObject = CreateRagdollObject(CreationParam);

		if (!pRagdollObject)
			return;

		AddPhysicObject(entindex, pRagdollObject);
	}
	else if (pPhysicConfig->type == PhysicConfigType_DynamicObject)
	{
		//TODO
		//CDynamicObjectCreationParameter CreationParam;
	}
	else if (pPhysicConfig->type == PhysicConfigType_StaticObject)
	{
		auto pStaticObjectConfig = (CClientStaticObjectConfig*)pPhysicConfig;

		CStaticObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_entindex = entindex;
		CreationParam.m_model = mod;
		CreationParam.m_pStaticObjectConfig = pStaticObjectConfig;

		LoadAdditionalResourcesForConfig(pPhysicConfig);

		auto pStaticObject = CreateStaticObject(CreationParam);

		if (!pStaticObject)
			return;

		AddPhysicObject(entindex, pStaticObject);
	}
	else
	{
		gEngfuncs.Con_DPrintf("CreatePhysicObjectFromConfig: Unsupported config type (%d).\n", pPhysicConfig->type);
	}
}

void CBasePhysicManager::CreatePhysicObjectForBrushModel(cl_entity_t* ent, entity_state_t* state, model_t* mod)
{
	auto entindex = ClientEntityManager()->GetEntityIndex(ent);

	auto pPhysicObject = GetPhysicObject(entindex);

	if (pPhysicObject && pPhysicObject->GetModel() == mod)
		return;

	auto pIndexArray = GetIndexArrayFromBrushModel(mod);

	if (!pIndexArray)
		return;

	auto pCollisionShapeConfig = new CClientCollisionShapeConfig;

	pCollisionShapeConfig->type = PhysicShape_TriangleMesh;
	pCollisionShapeConfig->m_pVertexArray = m_worldVertexArray;
	pCollisionShapeConfig->m_pIndexArray = pIndexArray;

	auto pRigidBodyConfig = new CClientRigidBodyConfig;

	pRigidBodyConfig->name = mod->name;
	pRigidBodyConfig->debugDrawLevel = (ent == r_worldentity) ? 2 : 1;
	pRigidBodyConfig->shapes.emplace_back(pCollisionShapeConfig);

	auto pStaticObjectConfig = new CClientStaticObjectConfig;

	pStaticObjectConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

	CStaticObjectCreationParameter CreationParam;
	CreationParam.m_entity = ent;
	CreationParam.m_entindex = entindex;
	CreationParam.m_model = mod;
	CreationParam.m_pStaticObjectConfig = pStaticObjectConfig;

	SCOPE_EXIT{ delete pStaticObjectConfig; };

	auto pStaticObject = CreateStaticObject(CreationParam);

	if (!pStaticObject)
		return;

	AddPhysicObject(entindex, pStaticObject);
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
			CPhysicObjectUpdateContext ctx;

			pPhysicObject->Update(&ctx);

			if(ctx.bShouldKillMe)
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

#if 0

static std::string FormatToObj(const CPhysicVertexArray* vertexArray, const CPhysicIndexArray* indexArray)
{
	std::ostringstream oss;

	for (const auto& vertex : vertexArray->vVertexBuffer) {
		oss << "v " << vertex.pos[0] << " " << vertex.pos[1] << " " << vertex.pos[2] << "\n";
	}

	for (const auto& face : vertexArray->vFaceBuffer) {
		oss << "f";
		for (int i = 0; i < face.num_vertexes; ++i) {
			oss << " " << (face.start_vertex + i + 1);
		}
		oss << "\n";
	}

	return oss.str();
}

static void SaveToObjFile(const std::string& filename, const std::string& objData) {

	auto hFileHandle = FILESYSTEM_ANY_OPEN(filename.c_str(), "wb");

	if (hFileHandle)
	{
		FILESYSTEM_ANY_WRITE(objData.data(), objData.size(), hFileHandle);
		FILESYSTEM_ANY_CLOSE(hFileHandle);
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

	auto objData = FormatToObj(m_barnacleVertexArray, m_barnacleIndexArray);

	SaveToObjFile("models/barnacle.obj", objData);
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


	auto objData = FormatToObj(m_gargantuaVertexArray, m_gargantuaIndexArray);

	SaveToObjFile("models/garg_mouth.obj", objData);
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

#endif

CPhysicIndexArray * CBasePhysicManager::GetIndexArrayFromBrushModel(model_t *mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex < 0 || modelindex > (int)m_brushIndexArray.size())
	{
		return NULL;
	}

	return m_brushIndexArray[modelindex];
}

#include "tiny_obj_loader.h"

#include <istream>
#include <streambuf>
#include <vector>
#include <cstring>

extern IFileSystem* g_pFileSystem;
extern IFileSystem_HL25* g_pFileSystem_HL25;

class CFileStreamBuffer : public std::streambuf {
public:
	CFileStreamBuffer(const std::string& filename) {

		auto hFileHandle = FILESYSTEM_ANY_OPEN(filename.c_str(), "rb");

		if (!hFileHandle) {
			throw std::runtime_error("Failed to open file: " + filename);
		}

		size_t fileSize = FILESYSTEM_ANY_SIZE(hFileHandle);
		buffer_.resize(fileSize);

		FILESYSTEM_ANY_READ(buffer_.data(), fileSize, hFileHandle);
		FILESYSTEM_ANY_CLOSE(hFileHandle);

		setg(buffer_.data(), buffer_.data(), buffer_.data() + buffer_.size());
	}

private:
	std::vector<char> buffer_;
};

class CFileSystemStream : public std::istream {
public:
	CFileSystemStream(const std::string& filename) : std::istream(&fileStreamBuffer_), fileStreamBuffer_(filename) {}

private:
	CFileStreamBuffer fileStreamBuffer_;
};

bool CBasePhysicManager::LoadObjToPhysicArrays(const std::string& objFilename, CPhysicVertexArray* vertexArray, CPhysicIndexArray* indexArray) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	CFileSystemStream fileStream(objFilename);

	bool ret = tinyobj::LoadObj(
		&attrib,
		&shapes,
		&materials,
		&warn,
		&err,
		&fileStream,
		nullptr,
		true,
		false
	);

	if (!warn.empty()) {
		gEngfuncs.Con_Printf("LoadObjToPhysicArrays: (warning) %s", err.c_str());
	}
	if (!err.empty()) {
		gEngfuncs.Con_Printf("LoadObjToPhysicArrays: (error) %s", err.c_str());
	}
	if (!ret) {
		gEngfuncs.Con_Printf("LoadObjToPhysicArrays: Failed to load \"%s\".", objFilename.c_str());
		return false;
	}

	for (size_t i = 0;i < attrib.vertices.size(); i += 3)
	{
		CPhysicBrushVertex vertex;
		vertex.pos[0] = attrib.vertices[0 + i];
		vertex.pos[1] = attrib.vertices[1 + i];
		vertex.pos[2] = attrib.vertices[2 + i];

		vertexArray->vVertexBuffer.push_back(vertex);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			indexArray->vIndexBuffer.push_back(index.vertex_index);
		}

		//I'm not sure if this works or not but this won't affect the trimesh anyway
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int fv = shape.mesh.num_face_vertices[f];
			CPhysicBrushFace face;
			face.start_vertex = index_offset;
			face.num_vertexes = fv;
			vertexArray->vFaceBuffer.push_back(face);

			index_offset += fv;
		}
	}

	return true;
}

void CBasePhysicManager::LoadAdditionalResourcesForConfig(CClientPhysicObjectConfig *pPhysicObjectConfig)
{
	for (auto pRigidBodyConfig : pPhysicObjectConfig->RigidBodyConfigs)
	{
		for (auto pShapeConfig : pRigidBodyConfig->shapes)
		{
			if (pShapeConfig->type == PhysicShape_TriangleMesh && !pShapeConfig->objpath.empty())
			{
				if (!pShapeConfig->m_pVertexArrayStorage)
					pShapeConfig->m_pVertexArrayStorage = new CPhysicVertexArray;

				if (!pShapeConfig->m_pIndexArrayStorage)
					pShapeConfig->m_pIndexArrayStorage = new CPhysicIndexArray;

				if (!pShapeConfig->m_pVertexArray && !pShapeConfig->m_pIndexArray && pShapeConfig->m_pVertexArrayStorage && pShapeConfig->m_pIndexArrayStorage)
				{
					if (LoadObjToPhysicArrays(pShapeConfig->objpath, pShapeConfig->m_pVertexArrayStorage, pShapeConfig->m_pIndexArrayStorage))
					{
						pShapeConfig->m_pVertexArray = pShapeConfig->m_pVertexArrayStorage;
						pShapeConfig->m_pIndexArray = pShapeConfig->m_pIndexArrayStorage;
					}
				}
			}
		}
	}
}
