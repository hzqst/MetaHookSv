#include <metahook.h>
#include <triangleapi.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "enginedef.h"
#include "plugins.h"

#include "CounterStrike.h"
#include "BasePhysicManager.h"
#include "ClientEntityManager.h"
#include "PhysicUTIL.h"

#include "mathlib2.h"
#include "util.h"

#include <sstream>
#include <format>
#include <ScopeExit/ScopeExit.h>

#include <KeyValues.h>

IClientPhysicManager* g_pClientPhysicManager{};

IClientPhysicManager* ClientPhysicManager()
{
	return g_pClientPhysicManager;
}

bool CheckPhysicComponentFilters(IPhysicComponent* pPhysicComponent, const CPhysicComponentFilters& filters)
{
	if (pPhysicComponent->IsRigidBody())
	{
		if (filters.m_HasWithoutRigidbodyFlags)
		{
			if (pPhysicComponent->GetFlags() & filters.m_WithoutRigidbodyFlags)
			{
				return false;
			}
		}

		if (filters.m_HasExactMatchRigidbodyFlags)
		{
			if (pPhysicComponent->GetFlags() == filters.m_ExactMatchRigidbodyFlags)
			{
				return true;
			}
		}

		if (filters.m_HasExactMatchRigidBodyComponentId)
		{
			if (pPhysicComponent->GetPhysicComponentId() == filters.m_ExactMatchRigidBodyComponentId)
			{
				return true;
			}
		}

		if (filters.m_HasWithRigidbodyFlags)
		{
			if (pPhysicComponent->GetFlags() & filters.m_WithRigidbodyFlags)
			{
				return true;
			}

			return false;
		}

		return true;
	}

	if (pPhysicComponent->IsConstraint())
	{
		if (filters.m_HasWithoutConstraintFlags)
		{
			if (pPhysicComponent->GetFlags() & filters.m_WithoutConstraintFlags)
			{
				return false;
			}
		}

		if (filters.m_HasExactMatchConstraintFlags)
		{
			if (pPhysicComponent->GetFlags() == filters.m_ExactMatchConstraintFlags)
			{
				return true;
			}
		}

		if (filters.m_HasExactMatchConstraintComponentId)
		{
			if (pPhysicComponent->GetPhysicComponentId() == filters.m_ExactMatchConstraintComponentId)
			{
				return true;
			}
		}

		if (filters.m_HasWithConstraintFlags)
		{
			if (pPhysicComponent->GetFlags() & filters.m_WithConstraintFlags)
			{
				return true;
			}

			return false;
		}

		return true;
	}

	return true;
}

CClientBasePhysicConfig::CClientBasePhysicConfig()
{
	configId = ClientPhysicManager()->AllocatePhysicConfigId();
}

CClientBasePhysicConfig::~CClientBasePhysicConfig()
{
	ClientPhysicManager()->RemovePhysicConfig(configId);
}

CClientCollisionShapeConfig::CClientCollisionShapeConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_CollisionShape;
}

CClientCollisionShapeConfig::~CClientCollisionShapeConfig()
{
	if (m_pVertexArrayStorage)
	{
		delete m_pVertexArrayStorage;
		m_pVertexArrayStorage = nullptr;
	}

	if (m_pIndexArrayStorage)
	{
		delete m_pIndexArrayStorage;
		m_pVertexArrayStorage = nullptr;
	}
}

CClientRigidBodyConfig::CClientRigidBodyConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_RigidBody;
}

CClientConstraintConfig::CClientConstraintConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_Constraint;

	for (int i = 0; i < _ARRAYSIZE(factors); ++i) {
		factors[i] = NAN;
	}
}

CClientPhysicActionConfig::CClientPhysicActionConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_Action;

	for (int i = 0; i < _ARRAYSIZE(factors); ++i) {
		factors[i] = NAN;
	}
}

CClientFloaterConfig::CClientFloaterConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_Floater;
}

CClientPhysicObjectConfig::CClientPhysicObjectConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_PhysicObject;
}

CClientDynamicObjectConfig::CClientDynamicObjectConfig() : CClientPhysicObjectConfig()
{
	type = PhysicObjectType_DynamicObject;
	flags = PhysicObjectFlag_DynamicObject;
}

CClientStaticObjectConfig::CClientStaticObjectConfig() : CClientPhysicObjectConfig()
{
	type = PhysicObjectType_StaticObject;
	flags = PhysicObjectFlag_StaticObject;
}

CClientRagdollObjectConfig::CClientRagdollObjectConfig() : CClientPhysicObjectConfig()
{
	type = PhysicObjectType_RagdollObject;
	flags = PhysicObjectFlag_RagdollObject;
	FirstPersionViewCameraControlConfig.rigidbody = "Head";
	ThirdPersionViewCameraControlConfig.rigidbody = "Pelvis";
}

CBasePhysicRigidBody::CBasePhysicRigidBody(int id, int entindex, const CClientRigidBodyConfig* pRigidConfig) :
	m_id(id),
	m_entindex(entindex),
	m_name(pRigidConfig->name),
	m_flags(pRigidConfig->flags),
	m_boneindex(pRigidConfig->boneindex),
	m_debugDrawLevel(pRigidConfig->debugDrawLevel),
	m_configId(pRigidConfig->configId)
{

}

CBasePhysicConstraint::CBasePhysicConstraint(
	int id,
	int entindex, 
	const CClientConstraintConfig* pConstraintConfig) :
	m_id(id),
	m_entindex(entindex),
	m_name(pConstraintConfig->name),
	m_flags(pConstraintConfig->flags),
	m_debugDrawLevel(pConstraintConfig->debugDrawLevel),
	m_configId(pConstraintConfig->configId)
{

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
	RemoveAllPhysicObjects(PhysicObjectFlag_AnyObject, 0);
	RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromBSP, 0);

	m_allocatedPhysicComponentId = 0;

	m_inspectedPhysicComponentId = 0;
	m_inspectedPhysicObjectId = 0;
	m_inspectedColor[0] = 194.f / 255.0f;
	m_inspectedColor[1] = 230.f / 255.0f;
	m_inspectedColor[2] = 234.f / 255.0f;

	m_selectedPhysicComponentId = 0;
	m_selectedPhysicObjectId = 0;
	m_selectedColor[0] = 1;
	m_selectedColor[1] = 1;
	m_selectedColor[2] = 0;

	GenerateWorldVertexArray();
	GenerateBrushIndexArray();

	//Deprecated, use .obj instead.
	//GenerateBarnacleIndexVertexArray();
	//GenerateGargantuaIndexVertexArray();

	LoadPhysicObjectConfigs();

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

void CBasePhysicManager::RemoveAllPhysicObjectConfigs(int withflags, int withoutflags)
{
	for (auto itor = m_physicObjectConfigs.begin(); itor != m_physicObjectConfigs.end(); itor++)
	{
		auto &Storage = (*itor);

		if (Storage.state == PhysicConfigState_Loaded && 
			(Storage.pConfig->flags & withflags) && 
			(Storage.pConfig->flags & withoutflags) == 0)
		{
			Storage.pConfig.reset();
			Storage.state = PhysicConfigState_NotLoaded;
			continue;
		}
	}
}

void CBasePhysicManager::LoadPhysicObjectConfigs(void)
{
	int maxNum = EngineGetMaxKnownModel();

	if ((int)m_physicObjectConfigs.size() < maxNum)
		m_physicObjectConfigs.resize(maxNum);

	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);

		if (mod->type == mod_studio && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				auto studiohdr = IEngineStudio.Mod_Extradata(mod);

				if (studiohdr)
				{
					LoadPhysicObjectConfigForModel(mod);
				}
			}
		}
		else if (mod->type == mod_brush)
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				LoadPhysicObjectConfigForModel(mod);
			}
		}
	}
}

void CBasePhysicManager::SavePhysicObjectConfigs(void)
{
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
					auto pConfig = GetPhysicObjectConfigForModel(mod);

					if (pConfig)
					{
						SavePhysicObjectConfigToFile(mod->name, pConfig.get());
					}
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

bool CBasePhysicManager::TransferOwnershipForPhysicObject(int old_entindex, int new_entindex)
{
	auto pPhysicObject = GetPhysicObject(old_entindex);

	if (pPhysicObject)
	{
		m_physicObjects.erase(old_entindex);

		RemovePhysicObject(new_entindex);

		m_physicObjects[new_entindex] = pPhysicObject;

		pPhysicObject->TransferOwnership(new_entindex);

		return true;
	}

	return false;
}

bool CBasePhysicManager::RebuildPhysicObject(int entindex, const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	auto pPhysicObject = GetPhysicObject(entindex);

	if (pPhysicObject)
	{
		return pPhysicObject->Rebuild(pPhysicObjectConfig);
	}

	return false;
}

bool CBasePhysicManager::RebuildPhysicObjectEx(uint64 physicObjectId, const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	auto entindex = UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(physicObjectId);
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicObject = GetPhysicObject(entindex);

	if (!pPhysicObject)
		return false;

	if (pPhysicObject->GetModel() != EngineGetModelByIndex(modelindex))
		return false;

	return pPhysicObject->Rebuild(pPhysicObjectConfig);
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

IPhysicObject* CBasePhysicManager::GetPhysicObjectEx(uint64 physicObjectId)
{
	auto entindex = UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(physicObjectId);
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicObject = GetPhysicObject(entindex);

	if (!pPhysicObject)
		return nullptr;

	if (pPhysicObject->GetModel() != EngineGetModelByIndex(modelindex))
		return nullptr;

	return pPhysicObject;
}

static void LoadPhysicObjectFlagsFromKeyValues(KeyValues* pKeyValues, int &flags)
{
	flags |= PhysicObjectFlag_FromConfig;

	if (pKeyValues->GetBool("barnacle"))
	{
		flags |= PhysicObjectFlag_Barnacle;
	}

	if (pKeyValues->GetBool("gargantua"))
	{
		flags |= PhysicObjectFlag_Gargantua;
	}
}

static CClientCollisionShapeConfigSharedPtr LoadCollisionShapeFromKeyValues(KeyValues* pCollisionShapeKey, bool bIsChild)
{
	auto pShapeConfig = std::make_shared<CClientCollisionShapeConfig>();

	auto type = pCollisionShapeKey->GetString("type");

	if (type)
	{
		pShapeConfig->type = UTIL_GetCollisionTypeFromTypeName(type);
	}

	pShapeConfig->is_child = bIsChild;

	pShapeConfig->direction = pCollisionShapeKey->GetInt("direction", PhysicShapeDirection_Y);

	auto origin = pCollisionShapeKey->GetString("origin");

	if (origin)
	{
		UTIL_ParseStringAsVector3(origin, pShapeConfig->origin);
	}

	auto angles = pCollisionShapeKey->GetString("angles");

	if (angles)
	{
		UTIL_ParseStringAsVector3(angles, pShapeConfig->angles);
	}

	auto size = pCollisionShapeKey->GetString("size");

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

	pShapeConfig->objpath = pCollisionShapeKey->GetString("objpath");

	auto pCompoundShapesKey = pCollisionShapeKey->FindKey("compoundShapes");

	if (pCompoundShapesKey)
	{
		for (auto pSubShapeKey = pCompoundShapesKey->GetFirstSubKey(); pSubShapeKey; pSubShapeKey = pSubShapeKey->GetNextKey())
		{
			auto shape = LoadCollisionShapeFromKeyValues(pSubShapeKey, true);

			ClientPhysicManager()->AddPhysicConfig(shape->configId, shape);

			pShapeConfig->compoundShapes.emplace_back(shape);
		}
	}

	return pShapeConfig;
}

static void LoadRigidBodiesFromKeyValues(KeyValues* pKeyValues, int allowedRigidBodyFlags, std::vector<std::shared_ptr<CClientRigidBodyConfig>> &RigidBodyConfigs)
{
	auto pRigidBodiesKey = pKeyValues->FindKey("rigidBodies");

	if (pRigidBodiesKey)
	{
		for (auto pRigidBodySubKey = pRigidBodiesKey->GetFirstSubKey(); pRigidBodySubKey; pRigidBodySubKey = pRigidBodySubKey->GetNextKey())
		{
			auto pRigidBodyConfig = std::make_shared<CClientRigidBodyConfig>();

			pRigidBodyConfig->name = pRigidBodySubKey->GetName();

			if (pRigidBodySubKey->GetBool("alwaysDynamic"))
			{
				pRigidBodyConfig->flags |= PhysicRigidBodyFlag_AlwaysDynamic;
			}

			if (pRigidBodySubKey->GetBool("alwaysKinematic"))
			{
				pRigidBodyConfig->flags |= PhysicRigidBodyFlag_AlwaysKinematic;
			}

			if (pRigidBodySubKey->GetBool("alwaysStatic"))
			{
				pRigidBodyConfig->flags |= PhysicRigidBodyFlag_AlwaysStatic;
			}

			if (pRigidBodySubKey->GetBool("noCollisionToWorld"))
			{
				pRigidBodyConfig->flags |= PhysicRigidBodyFlag_NoCollisionToWorld;
			}

			if (pRigidBodySubKey->GetBool("noCollisionToStaticObject"))
			{
				pRigidBodyConfig->flags |= PhysicRigidBodyFlag_NoCollisionToStaticObject;
			}

			if (pRigidBodySubKey->GetBool("noCollisionToDynamicObject"))
			{
				pRigidBodyConfig->flags |= PhysicRigidBodyFlag_NoCollisionToDynamicObject;
			}

			if (pRigidBodySubKey->GetBool("noCollisionToRagdollObject"))
			{
				pRigidBodyConfig->flags |= PhysicRigidBodyFlag_NoCollisionToRagdollObject;
			}

			pRigidBodyConfig->flags &= allowedRigidBodyFlags;

			pRigidBodyConfig->debugDrawLevel = pRigidBodySubKey->GetInt("debugDrawLevel", BULLET_DEFAULT_DEBUG_DRAW_LEVEL);

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

			auto forward = pRigidBodySubKey->GetString("forward");

			if (forward)
			{
				UTIL_ParseStringAsVector3(forward, pRigidBodyConfig->forward);
			}

			pRigidBodyConfig->isLegacyConfig = pRigidBodySubKey->GetInt("isLegacyConfig", false);
			pRigidBodyConfig->pboneindex = pRigidBodySubKey->GetInt("pboneindex", -1);
			pRigidBodyConfig->pboneoffset = pRigidBodySubKey->GetFloat("pboneoffset", 0);

			pRigidBodyConfig->mass = pRigidBodySubKey->GetFloat("mass", BULLET_DEFAULT_MASS);
			pRigidBodyConfig->density = pRigidBodySubKey->GetFloat("density", BULLET_DEFAULT_DENSENTY);
			pRigidBodyConfig->linearFriction = pRigidBodySubKey->GetFloat("linearFriction", BULLET_DEFAULT_LINEAR_FIRCTION);
			pRigidBodyConfig->rollingFriction = pRigidBodySubKey->GetFloat("rollingFriction", BULLET_DEFAULT_ANGULAR_FIRCTION);
			pRigidBodyConfig->restitution = pRigidBodySubKey->GetFloat("restitution", BULLET_DEFAULT_RESTITUTION);
			pRigidBodyConfig->ccdRadius = pRigidBodySubKey->GetFloat("ccdRadius", 0);
			pRigidBodyConfig->ccdThreshold = pRigidBodySubKey->GetFloat("ccdThreshold", BULLET_DEFAULT_CCD_THRESHOLD);
			pRigidBodyConfig->linearSleepingThreshold = pRigidBodySubKey->GetFloat("linearSleepingThreshold", BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD);
			pRigidBodyConfig->angularSleepingThreshold = pRigidBodySubKey->GetFloat("angularSleepingThreshold", BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD);

			auto pCollisionShapeKey = pRigidBodySubKey->FindKey("collisionShape");

			if (pCollisionShapeKey)
			{
				auto shape = LoadCollisionShapeFromKeyValues(pCollisionShapeKey, false);

				ClientPhysicManager()->AddPhysicConfig(shape->configId, shape);

				pRigidBodyConfig->collisionShape = shape;
			}

			RigidBodyConfigs.emplace_back(pRigidBodyConfig);

			ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);
		}
	}
}

static void LoadConstraintsFromKeyValues(KeyValues* pKeyValues, std::vector<std::shared_ptr<CClientConstraintConfig>> &ConstraintConfigs)
{
	auto pConstraintsKey = pKeyValues->FindKey("constraints");

	if (pConstraintsKey)
	{
		for (auto pConstraintSubKey = pConstraintsKey->GetFirstSubKey(); pConstraintSubKey; pConstraintSubKey = pConstraintSubKey->GetNextKey())
		{
			auto pConstraintConfig = std::make_shared<CClientConstraintConfig>();

			pConstraintConfig->name = pConstraintSubKey->GetName();

			auto type = pConstraintSubKey->GetString("type");

			if (type)
			{
				pConstraintConfig->type = UTIL_GetConstraintTypeFromTypeName(type);
			}

			pConstraintConfig->rigidbodyA = pConstraintSubKey->GetString("rigidbodyA");
			pConstraintConfig->rigidbodyB = pConstraintSubKey->GetString("rigidbodyB");

			auto originA = pConstraintSubKey->GetString("originA");

			if (originA)
			{
				UTIL_ParseStringAsVector3(originA, pConstraintConfig->originA);
			}

			auto anglesA = pConstraintSubKey->GetString("anglesA");

			if (anglesA)
			{
				UTIL_ParseStringAsVector3(anglesA, pConstraintConfig->anglesA);
			}

			auto originB = pConstraintSubKey->GetString("originB");

			if (originB)
			{
				UTIL_ParseStringAsVector3(originB, pConstraintConfig->originB);
			}

			auto anglesB = pConstraintSubKey->GetString("anglesB");

			if (anglesB)
			{
				UTIL_ParseStringAsVector3(anglesB, pConstraintConfig->anglesB);
			}

			auto forward = pConstraintSubKey->GetString("forward");

			if (forward)
			{
				UTIL_ParseStringAsVector3(forward, pConstraintConfig->forward);
			}

			if (pConstraintSubKey->GetBool("barnacle"))
				pConstraintConfig->flags |= PhysicConstraintFlag_Barnacle;

			if (pConstraintSubKey->GetBool("gargantua"))
				pConstraintConfig->flags |= PhysicConstraintFlag_Gargantua;

			if (pConstraintSubKey->GetBool("deactiveOnNormalActivity"))
				pConstraintConfig->flags |= PhysicConstraintFlag_DeactiveOnNormalActivity;

			if (pConstraintSubKey->GetBool("deactiveOnDeathActivity"))
				pConstraintConfig->flags |= PhysicConstraintFlag_DeactiveOnDeathActivity;

			if (pConstraintSubKey->GetBool("deactiveOnBarnacleActivity"))
				pConstraintConfig->flags |= PhysicConstraintFlag_DeactiveOnBarnacleActivity;

			if (pConstraintSubKey->GetBool("deactiveOnGargantuaActivity"))
				pConstraintConfig->flags |= PhysicConstraintFlag_DeactiveOnGargantuaActivity;

			pConstraintConfig->debugDrawLevel = pConstraintSubKey->GetInt("debugDrawLevel", BULLET_DEFAULT_DEBUG_DRAW_LEVEL);
			pConstraintConfig->disableCollision = pConstraintSubKey->GetBool("disableCollision", true);
			pConstraintConfig->useGlobalJointFromA = pConstraintSubKey->GetBool("useGlobalJointFromA", true);
			pConstraintConfig->useLookAtOther = pConstraintSubKey->GetBool("useLookAtOther", false);
			pConstraintConfig->useGlobalJointOriginFromOther = pConstraintSubKey->GetBool("useGlobalJointOriginFromOther", false);
			pConstraintConfig->useRigidBodyDistanceAsLinearLimit = pConstraintSubKey->GetBool("useRigidBodyDistanceAsLinearLimit", false);
			pConstraintConfig->useLinearReferenceFrameA = pConstraintSubKey->GetBool("useLinearReferenceFrameA", true);
			pConstraintConfig->maxTolerantLinearError = pConstraintSubKey->GetFloat("maxTolerantLinearError", BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR);

			pConstraintConfig->isLegacyConfig = pConstraintSubKey->GetBool("isLegacyConfig", false);
			pConstraintConfig->boneindexA = pConstraintSubKey->GetInt("boneindexA", -1);
			pConstraintConfig->boneindexB = pConstraintSubKey->GetInt("boneindexB", -1);

			auto offsetA = pConstraintSubKey->GetString("offsetA");

			if (offsetA)
			{
				UTIL_ParseStringAsVector3(offsetA, pConstraintConfig->offsetA);
			}

			auto offsetB = pConstraintSubKey->GetString("offsetB");

			if (offsetB)
			{
				UTIL_ParseStringAsVector3(offsetB, pConstraintConfig->offsetB);
			}

#define LOAD_FACTOR_FLOAT(name) pConstraintConfig->factors[PhysicConstraintFactorIdx_##name] = pFactorsKey->GetFloat(#name, NAN);

			auto pFactorsKey = pConstraintSubKey->FindKey("factors");

			if (pFactorsKey)
			{
				switch (pConstraintConfig->type)
				{
				case PhysicConstraint_ConeTwist:
				{
					LOAD_FACTOR_FLOAT(ConeTwistSwingSpanLimit1);
					LOAD_FACTOR_FLOAT(ConeTwistSwingSpanLimit2);
					LOAD_FACTOR_FLOAT(ConeTwistTwistSpanLimit);
					LOAD_FACTOR_FLOAT(ConeTwistSoftness);
					LOAD_FACTOR_FLOAT(ConeTwistBiasFactor);
					LOAD_FACTOR_FLOAT(ConeTwistRelaxationFactor);
					LOAD_FACTOR_FLOAT(LinearERP);
					LOAD_FACTOR_FLOAT(LinearCFM);
					LOAD_FACTOR_FLOAT(AngularERP);
					LOAD_FACTOR_FLOAT(AngularCFM);
					break;
				}
				case PhysicConstraint_Hinge:
				{
					LOAD_FACTOR_FLOAT(HingeLowLimit);
					LOAD_FACTOR_FLOAT(HingeHighLimit);
					LOAD_FACTOR_FLOAT(HingeSoftness);
					LOAD_FACTOR_FLOAT(HingeBiasFactor);
					LOAD_FACTOR_FLOAT(HingeRelaxationFactor);
					LOAD_FACTOR_FLOAT(AngularERP);
					LOAD_FACTOR_FLOAT(AngularCFM);
					LOAD_FACTOR_FLOAT(AngularStopERP);
					LOAD_FACTOR_FLOAT(AngularStopCFM);
					break;
				}
				case PhysicConstraint_Point:
				{
					LOAD_FACTOR_FLOAT(AngularERP);
					LOAD_FACTOR_FLOAT(AngularCFM);
					break;
				}
				case PhysicConstraint_Slider:
				{
					LOAD_FACTOR_FLOAT(SliderLowerLinearLimit);
					LOAD_FACTOR_FLOAT(SliderUpperLinearLimit);
					LOAD_FACTOR_FLOAT(SliderLowerAngularLimit);
					LOAD_FACTOR_FLOAT(SliderUpperAngularLimit);
					LOAD_FACTOR_FLOAT(LinearCFM);
					LOAD_FACTOR_FLOAT(LinearStopERP);
					LOAD_FACTOR_FLOAT(LinearStopCFM);
					LOAD_FACTOR_FLOAT(AngularCFM);
					LOAD_FACTOR_FLOAT(AngularStopERP);
					LOAD_FACTOR_FLOAT(AngularStopCFM);
					break;
				}
				case PhysicConstraint_Dof6:
				{
					LOAD_FACTOR_FLOAT(Dof6LowerLinearLimitX);
					LOAD_FACTOR_FLOAT(Dof6LowerLinearLimitY);
					LOAD_FACTOR_FLOAT(Dof6LowerLinearLimitZ);
					LOAD_FACTOR_FLOAT(Dof6UpperLinearLimitX);
					LOAD_FACTOR_FLOAT(Dof6UpperLinearLimitY);
					LOAD_FACTOR_FLOAT(Dof6UpperLinearLimitZ);
					LOAD_FACTOR_FLOAT(Dof6LowerAngularLimitX);
					LOAD_FACTOR_FLOAT(Dof6LowerAngularLimitY);
					LOAD_FACTOR_FLOAT(Dof6LowerAngularLimitZ);
					LOAD_FACTOR_FLOAT(Dof6UpperAngularLimitX);
					LOAD_FACTOR_FLOAT(Dof6UpperAngularLimitY);
					LOAD_FACTOR_FLOAT(Dof6UpperAngularLimitZ);
					LOAD_FACTOR_FLOAT(LinearCFM);
					LOAD_FACTOR_FLOAT(LinearStopERP);
					LOAD_FACTOR_FLOAT(LinearStopCFM);
					LOAD_FACTOR_FLOAT(AngularCFM);
					LOAD_FACTOR_FLOAT(AngularStopERP);
					LOAD_FACTOR_FLOAT(AngularStopCFM);
					break;
				}
				case PhysicConstraint_Dof6Spring:
				{
					LOAD_FACTOR_FLOAT(Dof6LowerLinearLimitX);
					LOAD_FACTOR_FLOAT(Dof6LowerLinearLimitY);
					LOAD_FACTOR_FLOAT(Dof6LowerLinearLimitZ);
					LOAD_FACTOR_FLOAT(Dof6UpperLinearLimitX);
					LOAD_FACTOR_FLOAT(Dof6UpperLinearLimitY);
					LOAD_FACTOR_FLOAT(Dof6UpperLinearLimitZ);
					LOAD_FACTOR_FLOAT(Dof6LowerAngularLimitX);
					LOAD_FACTOR_FLOAT(Dof6LowerAngularLimitY);
					LOAD_FACTOR_FLOAT(Dof6LowerAngularLimitZ);
					LOAD_FACTOR_FLOAT(Dof6UpperAngularLimitX);
					LOAD_FACTOR_FLOAT(Dof6UpperAngularLimitY);
					LOAD_FACTOR_FLOAT(Dof6UpperAngularLimitZ);
					LOAD_FACTOR_FLOAT(Dof6SpringEnableLinearSpringX);
					LOAD_FACTOR_FLOAT(Dof6SpringEnableLinearSpringY);
					LOAD_FACTOR_FLOAT(Dof6SpringEnableLinearSpringZ);
					LOAD_FACTOR_FLOAT(Dof6SpringEnableAngularSpringX);
					LOAD_FACTOR_FLOAT(Dof6SpringEnableAngularSpringY);
					LOAD_FACTOR_FLOAT(Dof6SpringEnableAngularSpringZ);
					LOAD_FACTOR_FLOAT(Dof6SpringLinearStiffnessX);
					LOAD_FACTOR_FLOAT(Dof6SpringLinearStiffnessY);
					LOAD_FACTOR_FLOAT(Dof6SpringLinearStiffnessZ);
					LOAD_FACTOR_FLOAT(Dof6SpringAngularStiffnessX);
					LOAD_FACTOR_FLOAT(Dof6SpringAngularStiffnessY);
					LOAD_FACTOR_FLOAT(Dof6SpringAngularStiffnessZ);
					LOAD_FACTOR_FLOAT(Dof6SpringLinearDampingX);
					LOAD_FACTOR_FLOAT(Dof6SpringLinearDampingY);
					LOAD_FACTOR_FLOAT(Dof6SpringLinearDampingZ);
					LOAD_FACTOR_FLOAT(Dof6SpringAngularDampingX);
					LOAD_FACTOR_FLOAT(Dof6SpringAngularDampingY);
					LOAD_FACTOR_FLOAT(Dof6SpringAngularDampingZ);
					LOAD_FACTOR_FLOAT(LinearCFM);
					LOAD_FACTOR_FLOAT(LinearStopERP);
					LOAD_FACTOR_FLOAT(LinearStopCFM);
					LOAD_FACTOR_FLOAT(AngularCFM);
					LOAD_FACTOR_FLOAT(AngularStopERP);
					LOAD_FACTOR_FLOAT(AngularStopCFM);
					break;
				}
				case PhysicConstraint_Fixed:
				{
					LOAD_FACTOR_FLOAT(LinearCFM);
					LOAD_FACTOR_FLOAT(LinearStopERP);
					LOAD_FACTOR_FLOAT(LinearStopCFM);
					LOAD_FACTOR_FLOAT(AngularCFM);
					LOAD_FACTOR_FLOAT(AngularStopERP);
					LOAD_FACTOR_FLOAT(AngularStopCFM);
					break;
				}
				}
			}

#undef LOAD_FACTOR_FLOAT

			ConstraintConfigs.emplace_back(pConstraintConfig);

			ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);
		}
	}
}

static void LoadAnimControlsFromKeyValues(KeyValues* pKeyValues, std::vector<CClientAnimControlConfig>& AnimControlConfigs)
{
	auto pAnimControlsKey = pKeyValues->FindKey("animControls");

	if (pAnimControlsKey)
	{
		for (auto pAnimControlSubKey = pAnimControlsKey->GetFirstSubKey(); pAnimControlSubKey; pAnimControlSubKey = pAnimControlSubKey->GetNextKey())
		{
			CClientAnimControlConfig AnimControlConfig;

			AnimControlConfig.sequence = pAnimControlSubKey->GetInt("sequence");
			AnimControlConfig.gaitsequence = pAnimControlSubKey->GetInt("gaitsequence");
			AnimControlConfig.frame = pAnimControlSubKey->GetFloat("frame");
			AnimControlConfig.activity = pAnimControlSubKey->GetInt("activity");

			AnimControlConfigs.emplace_back(AnimControlConfig);
		}
	}
}

static void LoadIdleAnimFromKeyValues(KeyValues* pKeyValues, CClientAnimControlConfig& IdleAnimConfigs)
{
	auto pIdleAnimKey = pKeyValues->FindKey("idleAnim");

	if (pIdleAnimKey)
	{
		IdleAnimConfigs.sequence = pIdleAnimKey->GetInt("sequence");
		IdleAnimConfigs.gaitsequence = pIdleAnimKey->GetInt("gaitsequence");
		IdleAnimConfigs.frame = pIdleAnimKey->GetFloat("frame");
		IdleAnimConfigs.activity = pIdleAnimKey->GetInt("activity");
	}
}

static void LoadPhysicActionFromKeyValues(KeyValues* pKeyValues, std::vector<std::shared_ptr<CClientPhysicActionConfig>> &ActionConfigs)
{
	auto pPhysicActionsKey = pKeyValues->FindKey("physicActions");

	if (pPhysicActionsKey)
	{
		for (auto pPhysicActionSubKey = pPhysicActionsKey->GetFirstSubKey(); pPhysicActionSubKey; pPhysicActionSubKey = pPhysicActionSubKey->GetNextKey())
		{
			auto pPhysicActionConfig = std::make_shared<CClientPhysicActionConfig>();

			pPhysicActionConfig->name = pPhysicActionSubKey->GetName();

			auto type = pPhysicActionSubKey->GetString("type");

			if (type)
			{
				pPhysicActionConfig->type = UTIL_GetPhysicActionTypeFromTypeName(type);
			}

			pPhysicActionConfig->rigidbodyA = pPhysicActionSubKey->GetString("rigidbodyA");
			pPhysicActionConfig->rigidbodyB = pPhysicActionSubKey->GetString("rigidbodyB");
			pPhysicActionConfig->constraint = pPhysicActionSubKey->GetString("constraint");

			if(pPhysicActionSubKey->GetBool("barnacle"))
				pPhysicActionConfig->flags |= PhysicActionFlag_Barnacle;

			if (pPhysicActionSubKey->GetBool("gargantua"))
				pPhysicActionConfig->flags |= PhysicActionFlag_Gargantua;

			if (pPhysicActionSubKey->GetBool("affectsRigidBody"))
				pPhysicActionConfig->flags |= PhysicActionFlag_AffectsRigidBody;

			if (pPhysicActionSubKey->GetBool("affectsConstraint"))
				pPhysicActionConfig->flags |= PhysicActionFlag_AffectsConstraint;

#define LOAD_FACTOR_FLOAT(name) pPhysicActionConfig->factors[PhysicActionFactorIdx_##name] = pFactorsKey->GetFloat(#name, NAN);

			auto pFactorsKey = pPhysicActionSubKey->FindKey("factors");

			if (pFactorsKey)
			{
				switch (pPhysicActionConfig->type)
				{
				case PhysicAction_BarnacleDragForce:
				{
					LOAD_FACTOR_FLOAT(BarnacleDragForceMagnitude);
					LOAD_FACTOR_FLOAT(BarnacleDragForceExtraHeight);

					break;
				}
				case PhysicAction_BarnacleChewForce:
				{
					LOAD_FACTOR_FLOAT(BarnacleChewForceMagnitude);
					LOAD_FACTOR_FLOAT(BarnacleChewForceInterval);
					break;
				}
				case PhysicAction_BarnacleConstraintLimitAdjustment:
				{
					LOAD_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentExtraHeight);
					LOAD_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentInterval);
					break;
				}
				}
			}

#undef LOAD_FACTOR_FLOAT

			ActionConfigs.emplace_back(pPhysicActionConfig);

			ClientPhysicManager()->AddPhysicConfig(pPhysicActionConfig->configId, pPhysicActionConfig);
		}
	}
}

static void LoadBarnacleControlFromKeyValues(KeyValues* pKeyValues, CClientBarnacleControlConfig &BarnacleControlConfig)
{
	auto pBarnacleControlKey = pKeyValues->FindKey("barnacleControl");

	if (pBarnacleControlKey)
	{
		LoadConstraintsFromKeyValues(pBarnacleControlKey, BarnacleControlConfig.ConstraintConfigs);
		LoadPhysicActionFromKeyValues(pBarnacleControlKey, BarnacleControlConfig.ActionConfigs);
	}
}

static void LoadCameraControlFromKeyValues(KeyValues* pKeyValues, const char *name, CClientCameraControlConfig& CameraControlConfig)
{
	auto pCameraControlKey = pKeyValues->FindKey(name);

	if (pCameraControlKey)
	{
		CameraControlConfig.rigidbody = pCameraControlKey->GetString("rigidbody");

		auto origin = pCameraControlKey->GetString("origin");

		if (origin)
		{
			UTIL_ParseStringAsVector3(origin, CameraControlConfig.origin);
		}

		auto angles = pCameraControlKey->GetString("angles");

		if (angles)
		{
			UTIL_ParseStringAsVector3(angles, CameraControlConfig.angles);
		}
	}
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadRagdollObjectConfigFromKeyValues(KeyValues* pKeyValues)
{
	auto pRagdollObjectConfig = std::make_shared<CClientRagdollObjectConfig>();

	LoadPhysicObjectFlagsFromKeyValues(pKeyValues, pRagdollObjectConfig->flags);
	LoadRigidBodiesFromKeyValues(pKeyValues, PhysicRigidBodyFlag_AllowedOnRagdollObject, pRagdollObjectConfig->RigidBodyConfigs);
	LoadConstraintsFromKeyValues(pKeyValues, pRagdollObjectConfig->ConstraintConfigs);
	LoadAnimControlsFromKeyValues(pKeyValues, pRagdollObjectConfig->AnimControlConfigs);
	LoadIdleAnimFromKeyValues(pKeyValues, pRagdollObjectConfig->IdleAnimConfig);
	LoadBarnacleControlFromKeyValues(pKeyValues, pRagdollObjectConfig->BarnacleControlConfig);
	LoadCameraControlFromKeyValues(pKeyValues, "firstPersionViewCameraControl", pRagdollObjectConfig->FirstPersionViewCameraControlConfig);
	LoadCameraControlFromKeyValues(pKeyValues, "thirdPersionViewCameraControl", pRagdollObjectConfig->ThirdPersionViewCameraControlConfig);

	return pRagdollObjectConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadStaticObjectConfigFromKeyValues(KeyValues* pKeyValues)
{
	auto pStaticObjectConfig = std::make_shared<CClientStaticObjectConfig>();

	LoadPhysicObjectFlagsFromKeyValues(pKeyValues, pStaticObjectConfig->flags);
	LoadRigidBodiesFromKeyValues(pKeyValues, PhysicRigidBodyFlag_AllowedOnStaticObject, pStaticObjectConfig->RigidBodyConfigs);

	return pStaticObjectConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadDynamicObjectConfigFromKeyValues(KeyValues* pKeyValues)
{
	auto pDynamicObjectConfig = std::make_shared<CClientDynamicObjectConfig>();

	LoadPhysicObjectFlagsFromKeyValues(pKeyValues, pDynamicObjectConfig->flags);
	LoadRigidBodiesFromKeyValues(pKeyValues, PhysicRigidBodyFlag_AllowedOnDynamicObject, pDynamicObjectConfig->RigidBodyConfigs);
	LoadConstraintsFromKeyValues(pKeyValues, pDynamicObjectConfig->ConstraintConfigs);

	ClientPhysicManager()->AddPhysicConfig(pDynamicObjectConfig->configId, pDynamicObjectConfig);

	return pDynamicObjectConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromKeyValues(KeyValues *pKeyValues)
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

		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromKeyValues: invalid type\"%s\" from KeyValues!\n", type);
	}
	else
	{
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromKeyValues: type not found in KeyValues!\n");
	}

	return nullptr;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromNewFile(const std::string& filename)
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
}

static void AddBaseConfigToKeyValues(KeyValues* pKeyValues, const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	pKeyValues->SetString("type", UTIL_GetPhysicObjectConfigTypeName(pPhysicObjectConfig->type));

	if (pPhysicObjectConfig->flags & PhysicObjectFlag_Barnacle)
		pKeyValues->SetInt("barnacle", 1);

	if (pPhysicObjectConfig->flags & PhysicObjectFlag_Gargantua)
		pKeyValues->SetInt("gargantua", 1);
}

static void AddCollisionShapeToKeyValues(KeyValues * pCollisionShapeSubKey, const CClientCollisionShapeConfig *pCollisionShapeConfig)
{
	pCollisionShapeSubKey->SetString("type", UTIL_GetCollisionShapeTypeName(pCollisionShapeConfig->type));

	if (pCollisionShapeConfig->direction != PhysicShapeDirection_Y)
		pCollisionShapeSubKey->SetInt("direction", pCollisionShapeConfig->direction);

	if (VectorLength(pCollisionShapeConfig->origin) > 0)
	{
		pCollisionShapeSubKey->SetString("origin", std::format("{0} {1} {2}", pCollisionShapeConfig->origin[0], pCollisionShapeConfig->origin[1], pCollisionShapeConfig->origin[2]).c_str());
	}

	if (VectorLength(pCollisionShapeConfig->angles) > 0)
	{
		pCollisionShapeSubKey->SetString("angles", std::format("{0} {1} {2}", pCollisionShapeConfig->angles[0], pCollisionShapeConfig->angles[1], pCollisionShapeConfig->angles[2]).c_str());
	}

	if (VectorLength(pCollisionShapeConfig->size) > 0)
	{
		pCollisionShapeSubKey->SetString("size", std::format("{0} {1} {2}", pCollisionShapeConfig->size[0], pCollisionShapeConfig->size[1], pCollisionShapeConfig->size[2]).c_str());
	}

	if (!pCollisionShapeConfig->objpath.empty())
	{
		pCollisionShapeSubKey->SetString("objpath", pCollisionShapeConfig->objpath.c_str());
	}

	if (pCollisionShapeConfig->compoundShapes.size() > 0)
	{
		auto pCompoundShapesKey = pCollisionShapeSubKey->FindKey("compoundShapes", true);

		if (pCompoundShapesKey)
		{
			for (const auto& pCollisionShapeConfig : pCollisionShapeConfig->compoundShapes)
			{
				auto pCollisionShapeSubKey = pCompoundShapesKey->CreateNewKey();

				if (pCollisionShapeSubKey)
				{
					AddCollisionShapeToKeyValues(pCollisionShapeSubKey, pCollisionShapeConfig.get());
				}
			}
		}
	}
}

static void AddRigidBodiesToKeyValues(KeyValues* pKeyValues, const std::vector<std::shared_ptr<CClientRigidBodyConfig>> &RigidBodyConfigs)
{
	if (RigidBodyConfigs.size() > 0)
	{
		auto pRigidBodiesKey = pKeyValues->FindKey("rigidBodies", true);

		if (pRigidBodiesKey)
		{
			for (const auto& pRigidBodyConfig : RigidBodyConfigs)
			{
				auto pRigidBodySubKey = pRigidBodiesKey->FindKey(pRigidBodyConfig->name.c_str(), true);

				if (pRigidBodySubKey)
				{
					if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_AlwaysDynamic)
						pRigidBodySubKey->SetBool("alwaysDynamic", true);

					if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_AlwaysKinematic)
						pRigidBodySubKey->SetBool("alwaysKinematic", true);

					if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_AlwaysStatic)
						pRigidBodySubKey->SetBool("alwaysStatic", true);

					if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_NoCollisionToWorld)
						pRigidBodySubKey->SetBool("noCollisionToWorld", true);

					if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_NoCollisionToStaticObject)
						pRigidBodySubKey->SetBool("noCollisionToStaticObject", true);

					if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_NoCollisionToDynamicObject)
						pRigidBodySubKey->SetBool("noCollisionToDynamicObject", true);

					if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_NoCollisionToRagdollObject)
						pRigidBodySubKey->SetBool("noCollisionToRagdollObject", true);

					pRigidBodySubKey->SetInt("debugDrawLevel", pRigidBodyConfig->debugDrawLevel);

					pRigidBodySubKey->SetInt("boneindex", pRigidBodyConfig->boneindex);

					if (VectorLength(pRigidBodyConfig->origin) > 0)
					{
						pRigidBodySubKey->SetString("origin", std::format("{0} {1} {2}", pRigidBodyConfig->origin[0], pRigidBodyConfig->origin[1], pRigidBodyConfig->origin[2]).c_str());
					}

					if (VectorLength(pRigidBodyConfig->angles) > 0)
					{
						pRigidBodySubKey->SetString("angles", std::format("{0} {1} {2}", pRigidBodyConfig->angles[0], pRigidBodyConfig->angles[1], pRigidBodyConfig->angles[2]).c_str());
					}

					if (VectorLength(pRigidBodyConfig->forward) > 0)
					{
						pRigidBodySubKey->SetString("forward", std::format("{0} {1} {2}", pRigidBodyConfig->forward[0], pRigidBodyConfig->forward[1], pRigidBodyConfig->forward[2]).c_str());
					}

					pRigidBodySubKey->SetBool("isLegacyConfig", pRigidBodyConfig->isLegacyConfig);
					pRigidBodySubKey->SetInt("pboneindex", pRigidBodyConfig->pboneindex);
					pRigidBodySubKey->SetFloat("pboneoffset", pRigidBodyConfig->pboneoffset);

					pRigidBodySubKey->SetFloat("mass", pRigidBodyConfig->mass);
					pRigidBodySubKey->SetFloat("density", pRigidBodyConfig->density);
					pRigidBodySubKey->SetFloat("linearFriction", pRigidBodyConfig->linearFriction);
					pRigidBodySubKey->SetFloat("rollingFriction", pRigidBodyConfig->rollingFriction);
					pRigidBodySubKey->SetFloat("restitution", pRigidBodyConfig->restitution);
					pRigidBodySubKey->SetFloat("ccdRadius", pRigidBodyConfig->ccdRadius);
					pRigidBodySubKey->SetFloat("ccdThreshold", pRigidBodyConfig->ccdThreshold);
					pRigidBodySubKey->SetFloat("linearSleepingThreshold", pRigidBodyConfig->linearSleepingThreshold);
					pRigidBodySubKey->SetFloat("angularSleepingThreshold", pRigidBodyConfig->angularSleepingThreshold);

					if (pRigidBodyConfig->collisionShape)
					{
						auto pCollisionShapeKey = pRigidBodySubKey->FindKey("collisionShape", true);

						if (pCollisionShapeKey)
						{
							AddCollisionShapeToKeyValues(pCollisionShapeKey, pRigidBodyConfig->collisionShape.get());
						}
					}
				}
			}
		}
	}
}

static void AddConstraintsToKeyValues(KeyValues* pKeyValues, const std::vector<std::shared_ptr<CClientConstraintConfig>> &ConstraintConfigs)
{
	if (ConstraintConfigs.size() > 0)
	{
		auto pConstraintsKey = pKeyValues->FindKey("constraints", true);

		if (pConstraintsKey)
		{
			for (const auto& pConstraintConfig : ConstraintConfigs)
			{
				auto pConstraintSubKey = pConstraintsKey->FindKey(pConstraintConfig->name.c_str(), true);

				if (pConstraintSubKey)
				{
					if (pConstraintConfig->flags & PhysicConstraintFlag_Barnacle)
						pConstraintSubKey->SetBool("barnacle", true);

					if (pConstraintConfig->flags & PhysicConstraintFlag_Gargantua)
						pConstraintSubKey->SetBool("gargantua", true);

					if (pConstraintConfig->flags & PhysicConstraintFlag_DeactiveOnNormalActivity)
						pConstraintSubKey->SetBool("deactiveOnNormalActivity", true);

					if (pConstraintConfig->flags & PhysicConstraintFlag_DeactiveOnDeathActivity)
						pConstraintSubKey->SetBool("deactiveOnDeathActivity", true);

					if (pConstraintConfig->flags & PhysicConstraintFlag_DeactiveOnBarnacleActivity)
						pConstraintSubKey->SetBool("deactiveOnBarnacleActivity", true);

					if (pConstraintConfig->flags & PhysicConstraintFlag_DeactiveOnGargantuaActivity)
						pConstraintSubKey->SetBool("deactiveOnGargantuaActivity", true);

					pConstraintSubKey->SetString("type", UTIL_GetConstraintTypeName(pConstraintConfig->type));

					pConstraintSubKey->SetString("rigidbodyA", pConstraintConfig->rigidbodyA.c_str());
					pConstraintSubKey->SetString("rigidbodyB", pConstraintConfig->rigidbodyB.c_str());

					if (VectorLength(pConstraintConfig->originA) > 0)
					{
						pConstraintSubKey->SetString("originA", std::format("{0} {1} {2}", pConstraintConfig->originA[0], pConstraintConfig->originA[1], pConstraintConfig->originA[2]).c_str());
					}

					if (VectorLength(pConstraintConfig->anglesA) > 0)
					{
						pConstraintSubKey->SetString("anglesA", std::format("{0} {1} {2}", pConstraintConfig->anglesA[0], pConstraintConfig->anglesA[1], pConstraintConfig->anglesA[2]).c_str());
					}

					if (VectorLength(pConstraintConfig->originB) > 0)
					{
						pConstraintSubKey->SetString("originB", std::format("{0} {1} {2}", pConstraintConfig->originB[0], pConstraintConfig->originB[1], pConstraintConfig->originB[2]).c_str());
					}

					if (VectorLength(pConstraintConfig->anglesB) > 0)
					{
						pConstraintSubKey->SetString("anglesB", std::format("{0} {1} {2}", pConstraintConfig->anglesB[0], pConstraintConfig->anglesB[1], pConstraintConfig->anglesB[2]).c_str());
					}

					if (VectorLength(pConstraintConfig->forward) > 0)
					{
						pConstraintSubKey->SetString("forward", std::format("{0} {1} {2}", pConstraintConfig->forward[0], pConstraintConfig->forward[1], pConstraintConfig->forward[2]).c_str());
					}

					if(pConstraintConfig->disableCollision != true)
						pConstraintSubKey->SetBool("disableCollision", pConstraintConfig->disableCollision);

					if (pConstraintConfig->useGlobalJointFromA != true)
						pConstraintSubKey->SetBool("useGlobalJointFromA", pConstraintConfig->useGlobalJointFromA);

					if (pConstraintConfig->useLookAtOther != false)
						pConstraintSubKey->SetBool("useLookAtOther", pConstraintConfig->useLookAtOther);

					if (pConstraintConfig->useGlobalJointOriginFromOther != false)
						pConstraintSubKey->SetBool("useGlobalJointOriginFromOther", pConstraintConfig->useGlobalJointOriginFromOther);

					if (pConstraintConfig->useRigidBodyDistanceAsLinearLimit != false)
						pConstraintSubKey->SetBool("useRigidBodyDistanceAsLinearLimit", pConstraintConfig->useRigidBodyDistanceAsLinearLimit);
				
					if (pConstraintConfig->useLinearReferenceFrameA != true)
						pConstraintSubKey->SetBool("useLinearReferenceFrameA", pConstraintConfig->useLinearReferenceFrameA);

					if (pConstraintConfig->debugDrawLevel != BULLET_DEFAULT_DEBUG_DRAW_LEVEL)
						pConstraintSubKey->SetInt("debugDrawLevel", pConstraintConfig->debugDrawLevel); 

					if(pConstraintConfig->maxTolerantLinearError != BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR)
						pConstraintSubKey->SetFloat("maxTolerantLinearError", pConstraintConfig->maxTolerantLinearError);

					if (pConstraintConfig->isLegacyConfig != false)
						pConstraintSubKey->SetBool("isLegacyConfig", pConstraintConfig->isLegacyConfig);

					if(pConstraintConfig->boneindexA != -1)
						pConstraintSubKey->SetInt("boneindexA", pConstraintConfig->boneindexA);

					if (pConstraintConfig->boneindexB != -1)
						pConstraintSubKey->SetInt("boneindexB", pConstraintConfig->boneindexB);

					if (VectorLength(pConstraintConfig->offsetA) > 0)
					{
						pConstraintSubKey->SetString("offsetA", std::format("{0} {1} {2}", pConstraintConfig->offsetA[0], pConstraintConfig->offsetA[1], pConstraintConfig->offsetA[2]).c_str());
					}

					if (VectorLength(pConstraintConfig->offsetB) > 0)
					{
						pConstraintSubKey->SetString("offsetB", std::format("{0} {1} {2}", pConstraintConfig->offsetB[0], pConstraintConfig->offsetB[1], pConstraintConfig->offsetB[2]).c_str());
					}

#define SET_FACTOR_FLOAT(name) if (!isnan(pConstraintConfig->factors[PhysicConstraintFactorIdx_##name])) pFactorsKey->SetFloat(#name, pConstraintConfig->factors[PhysicConstraintFactorIdx_##name]);

					auto pFactorsKey = pConstraintSubKey->FindKey("factors", true);

					if (pFactorsKey)
					{
						switch (pConstraintConfig->type)
						{
						case PhysicConstraint_ConeTwist:
						{
							SET_FACTOR_FLOAT(ConeTwistSwingSpanLimit1);
							SET_FACTOR_FLOAT(ConeTwistSwingSpanLimit2);
							SET_FACTOR_FLOAT(ConeTwistTwistSpanLimit);
							SET_FACTOR_FLOAT(ConeTwistSoftness);
							SET_FACTOR_FLOAT(ConeTwistBiasFactor);
							SET_FACTOR_FLOAT(ConeTwistRelaxationFactor);
							SET_FACTOR_FLOAT(LinearERP);
							SET_FACTOR_FLOAT(LinearCFM);
							SET_FACTOR_FLOAT(AngularERP);
							SET_FACTOR_FLOAT(AngularCFM);
							break;
						}
						case PhysicConstraint_Hinge:
						{
							SET_FACTOR_FLOAT(HingeLowLimit);
							SET_FACTOR_FLOAT(HingeHighLimit);
							SET_FACTOR_FLOAT(HingeSoftness);
							SET_FACTOR_FLOAT(HingeBiasFactor);
							SET_FACTOR_FLOAT(HingeRelaxationFactor);
							SET_FACTOR_FLOAT(AngularERP);
							SET_FACTOR_FLOAT(AngularCFM);
							SET_FACTOR_FLOAT(AngularStopERP);
							SET_FACTOR_FLOAT(AngularStopCFM);
							break;
						}
						case PhysicConstraint_Point:
						{
							SET_FACTOR_FLOAT(AngularERP);
							SET_FACTOR_FLOAT(AngularCFM);
							break;
						}
						case PhysicConstraint_Slider:
						{
							SET_FACTOR_FLOAT(SliderLowerLinearLimit);
							SET_FACTOR_FLOAT(SliderUpperLinearLimit);
							SET_FACTOR_FLOAT(SliderLowerAngularLimit);
							SET_FACTOR_FLOAT(SliderUpperAngularLimit);
							SET_FACTOR_FLOAT(LinearCFM);
							SET_FACTOR_FLOAT(LinearStopERP);
							SET_FACTOR_FLOAT(LinearStopCFM);
							SET_FACTOR_FLOAT(AngularCFM);
							SET_FACTOR_FLOAT(AngularStopERP);
							SET_FACTOR_FLOAT(AngularStopCFM);
							break;
						}
						case PhysicConstraint_Dof6:
						{
							SET_FACTOR_FLOAT(Dof6LowerLinearLimitX);
							SET_FACTOR_FLOAT(Dof6LowerLinearLimitY);
							SET_FACTOR_FLOAT(Dof6LowerLinearLimitZ);
							SET_FACTOR_FLOAT(Dof6UpperLinearLimitX);
							SET_FACTOR_FLOAT(Dof6UpperLinearLimitY);
							SET_FACTOR_FLOAT(Dof6UpperLinearLimitZ);
							SET_FACTOR_FLOAT(Dof6LowerAngularLimitX);
							SET_FACTOR_FLOAT(Dof6LowerAngularLimitY);
							SET_FACTOR_FLOAT(Dof6LowerAngularLimitZ);
							SET_FACTOR_FLOAT(Dof6UpperAngularLimitX);
							SET_FACTOR_FLOAT(Dof6UpperAngularLimitY);
							SET_FACTOR_FLOAT(Dof6UpperAngularLimitZ);
							SET_FACTOR_FLOAT(LinearCFM);
							SET_FACTOR_FLOAT(LinearStopERP);
							SET_FACTOR_FLOAT(LinearStopCFM);
							SET_FACTOR_FLOAT(AngularCFM);
							SET_FACTOR_FLOAT(AngularStopERP);
							SET_FACTOR_FLOAT(AngularStopCFM);
							break;
						}
						case PhysicConstraint_Dof6Spring:
						{
							SET_FACTOR_FLOAT(Dof6LowerLinearLimitX);
							SET_FACTOR_FLOAT(Dof6LowerLinearLimitY);
							SET_FACTOR_FLOAT(Dof6LowerLinearLimitZ);
							SET_FACTOR_FLOAT(Dof6UpperLinearLimitX);
							SET_FACTOR_FLOAT(Dof6UpperLinearLimitY);
							SET_FACTOR_FLOAT(Dof6UpperLinearLimitZ);
							SET_FACTOR_FLOAT(Dof6LowerAngularLimitX);
							SET_FACTOR_FLOAT(Dof6LowerAngularLimitY);
							SET_FACTOR_FLOAT(Dof6LowerAngularLimitZ);
							SET_FACTOR_FLOAT(Dof6UpperAngularLimitX);
							SET_FACTOR_FLOAT(Dof6UpperAngularLimitY);
							SET_FACTOR_FLOAT(Dof6UpperAngularLimitZ);
							SET_FACTOR_FLOAT(Dof6SpringEnableLinearSpringX);
							SET_FACTOR_FLOAT(Dof6SpringEnableLinearSpringY);
							SET_FACTOR_FLOAT(Dof6SpringEnableLinearSpringZ);
							SET_FACTOR_FLOAT(Dof6SpringEnableAngularSpringX);
							SET_FACTOR_FLOAT(Dof6SpringEnableAngularSpringY);
							SET_FACTOR_FLOAT(Dof6SpringEnableAngularSpringZ);
							SET_FACTOR_FLOAT(Dof6SpringLinearStiffnessX);
							SET_FACTOR_FLOAT(Dof6SpringLinearStiffnessY);
							SET_FACTOR_FLOAT(Dof6SpringLinearStiffnessZ);
							SET_FACTOR_FLOAT(Dof6SpringAngularStiffnessX);
							SET_FACTOR_FLOAT(Dof6SpringAngularStiffnessY);
							SET_FACTOR_FLOAT(Dof6SpringAngularStiffnessZ);
							SET_FACTOR_FLOAT(Dof6SpringLinearDampingX);
							SET_FACTOR_FLOAT(Dof6SpringLinearDampingY);
							SET_FACTOR_FLOAT(Dof6SpringLinearDampingZ);
							SET_FACTOR_FLOAT(Dof6SpringAngularDampingX);
							SET_FACTOR_FLOAT(Dof6SpringAngularDampingY);
							SET_FACTOR_FLOAT(Dof6SpringAngularDampingZ);
							SET_FACTOR_FLOAT(LinearCFM);
							SET_FACTOR_FLOAT(LinearStopERP);
							SET_FACTOR_FLOAT(LinearStopCFM);
							SET_FACTOR_FLOAT(AngularCFM);
							SET_FACTOR_FLOAT(AngularStopERP);
							SET_FACTOR_FLOAT(AngularStopCFM);
							break;
						}
						case PhysicConstraint_Fixed:
						{
							SET_FACTOR_FLOAT(LinearCFM);
							SET_FACTOR_FLOAT(LinearStopERP);
							SET_FACTOR_FLOAT(LinearStopCFM);
							SET_FACTOR_FLOAT(AngularCFM);
							SET_FACTOR_FLOAT(AngularStopERP);
							SET_FACTOR_FLOAT(AngularStopCFM);
							break;
						}
						}
					}

#undef SET_FACTOR_FLOAT
				}
			}
		}
	}
}

static void AddPhysicActionsToKeyValues(KeyValues* pKeyValues, const std::vector<std::shared_ptr<CClientPhysicActionConfig>>& PhysicActionConfigs)
{
	if (PhysicActionConfigs.size() > 0)
	{
		auto pPhysicActionsKey = pKeyValues->FindKey("physicActions", true);

		if (pPhysicActionsKey)
		{
			for (const auto& pPhysicActionConfig : PhysicActionConfigs)
			{
				auto pPhysicActionSubKey = pPhysicActionsKey->FindKey(pPhysicActionConfig->name.c_str(), true);

				if (pPhysicActionSubKey)
				{
					pPhysicActionSubKey->SetString("type", UTIL_GetPhysicActionTypeName(pPhysicActionConfig->type));
					pPhysicActionSubKey->SetString("rigidbodyA", pPhysicActionConfig->rigidbodyA.c_str());
					pPhysicActionSubKey->SetString("rigidbodyB", pPhysicActionConfig->rigidbodyB.c_str());
					pPhysicActionSubKey->SetString("constraint", pPhysicActionConfig->constraint.c_str());

					if(pPhysicActionConfig->flags & PhysicActionFlag_Barnacle )
						pPhysicActionSubKey->SetBool("barnacle", true);

					if (pPhysicActionConfig->flags & PhysicActionFlag_Gargantua)
						pPhysicActionSubKey->SetBool("gargantua", true);

					if (pPhysicActionConfig->flags & PhysicActionFlag_AffectsRigidBody)
						pPhysicActionSubKey->SetBool("affectsRigidBody", true);

					if (pPhysicActionConfig->flags & PhysicActionFlag_AffectsConstraint)
						pPhysicActionSubKey->SetBool("affectsConstraint", true);

#define SET_FACTOR_FLOAT(name) if(!isnan(pPhysicActionConfig->factors[PhysicActionFactorIdx_##name])) pFactorsKey->SetFloat(#name, pPhysicActionConfig->factors[PhysicActionFactorIdx_##name]);

					auto pFactorsKey = pPhysicActionSubKey->FindKey("factors", true);
					
					if (pFactorsKey)
					{
						switch (pPhysicActionConfig->type)
						{
						case PhysicAction_BarnacleDragForce:
						{
							SET_FACTOR_FLOAT(BarnacleDragForceMagnitude);
							SET_FACTOR_FLOAT(BarnacleDragForceExtraHeight);

							break;
						}
						case PhysicAction_BarnacleChewForce:
						{
							SET_FACTOR_FLOAT(BarnacleChewForceMagnitude);
							SET_FACTOR_FLOAT(BarnacleChewForceInterval);
							break;
						}
						case PhysicAction_BarnacleConstraintLimitAdjustment:
						{
							SET_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentExtraHeight);
							SET_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentInterval);
							break;
						}
						}
					}

#undef SET_FACTOR_FLOAT
				}
			}
		}
	}
}

static void AddAnimControlToKeyValues(KeyValues* pKeyValues, const std::vector<CClientAnimControlConfig>& AnimControlConfigs)
{
	if (AnimControlConfigs.size() > 0)
	{
		auto pAnimControlsKey = pKeyValues->FindKey("animControls", true);

		if (pAnimControlsKey)
		{
			for (const auto& AnimControl : AnimControlConfigs)
			{
				auto pAnimControlSubKey = pAnimControlsKey->CreateNewKey();

				if (pAnimControlSubKey)
				{
					pAnimControlSubKey->SetInt("sequence", AnimControl.sequence);
					pAnimControlSubKey->SetInt("gaitsequence", AnimControl.gaitsequence);
					pAnimControlSubKey->SetFloat("frame", AnimControl.frame);
					pAnimControlSubKey->SetInt("activity", AnimControl.activity);
				}
			}
		}
	}
}

static void AddIdleAnimToKeyValues(KeyValues* pKeyValues, const CClientAnimControlConfig& IdleAnimConfigs)
{
	auto pIdleAnimKey = pKeyValues->FindKey("idleAnim", true);

	if (pIdleAnimKey)
	{
		pIdleAnimKey->SetInt("sequence", IdleAnimConfigs.sequence);
		pIdleAnimKey->SetInt("gaitsequence", IdleAnimConfigs.gaitsequence);
		pIdleAnimKey->SetFloat("frame", IdleAnimConfigs.frame);
		pIdleAnimKey->SetInt("activity", IdleAnimConfigs.activity);
	}
}

static void AddBarnacleControlToKeyValues(KeyValues* pKeyValues, const CClientBarnacleControlConfig& BarnacleControl)
{
	auto pBarnacleControlKey = pKeyValues->FindKey("barnacleControl", true);

	if (pBarnacleControlKey)
	{
		AddConstraintsToKeyValues(pBarnacleControlKey, BarnacleControl.ConstraintConfigs);
		AddPhysicActionsToKeyValues(pBarnacleControlKey, BarnacleControl.ActionConfigs);
	}
}

static void AddCameraControlToKeyValues(KeyValues* pKeyValues, const char *name, const CClientCameraControlConfig& CameraControl)
{
	auto pCameraControlKey = pKeyValues->FindKey(name, true);

	if (pCameraControlKey)
	{
		pCameraControlKey->SetString("rigidbody", CameraControl.rigidbody.c_str());

		if (VectorLength(CameraControl.origin) > 0)
		{
			pCameraControlKey->SetString("origin", std::format("{0} {1} {2}", CameraControl.origin[0], CameraControl.origin[1], CameraControl.origin[2]).c_str());
		}

		if (VectorLength(CameraControl.angles) > 0)
		{
			pCameraControlKey->SetString("angles", std::format("{0} {1} {2}", CameraControl.angles[0], CameraControl.angles[1], CameraControl.angles[2]).c_str());
		}
	}
}

static KeyValues* ConvertStaticObjectConfigToKeyValues(const CClientStaticObjectConfig* StaticObjectConfig)
{
	auto pKeyValues = new KeyValues("PhysicObjectConfig");

	AddBaseConfigToKeyValues(pKeyValues, StaticObjectConfig);
	AddRigidBodiesToKeyValues(pKeyValues, StaticObjectConfig->RigidBodyConfigs);

	return pKeyValues;
}

static KeyValues* ConvertDynamicObjectConfigToKeyValues(const CClientDynamicObjectConfig* DynamicObjectConfig)
{
	//TODO
	return nullptr;
}

static KeyValues* ConvertRagdollObjectConfigToKeyValues(const CClientRagdollObjectConfig* RagdollObjectConfig)
{
	auto pKeyValues = new KeyValues("PhysicObjectConfig");

	AddBaseConfigToKeyValues(pKeyValues, RagdollObjectConfig);
	AddRigidBodiesToKeyValues(pKeyValues, RagdollObjectConfig->RigidBodyConfigs);
	AddConstraintsToKeyValues(pKeyValues, RagdollObjectConfig->ConstraintConfigs);
	AddAnimControlToKeyValues(pKeyValues, RagdollObjectConfig->AnimControlConfigs);
	AddIdleAnimToKeyValues(pKeyValues, RagdollObjectConfig->IdleAnimConfig);
	AddBarnacleControlToKeyValues(pKeyValues, RagdollObjectConfig->BarnacleControlConfig);
	AddCameraControlToKeyValues(pKeyValues, "firstPersionViewCameraControl", RagdollObjectConfig->FirstPersionViewCameraControlConfig);
	AddCameraControlToKeyValues(pKeyValues, "thirdPersionViewCameraControl", RagdollObjectConfig->ThirdPersionViewCameraControlConfig);

	return pKeyValues;
}

static KeyValues* ConvertPhysicObjectConfigToKeyValues(const CClientPhysicObjectConfig *PhysicObjectConfig)
{
	switch (PhysicObjectConfig->type)
	{
	case PhysicObjectType_StaticObject:
	{
		return ConvertStaticObjectConfigToKeyValues((const CClientStaticObjectConfig*)PhysicObjectConfig);
	}
	case PhysicObjectType_DynamicObject:
	{
		return ConvertDynamicObjectConfigToKeyValues((const CClientDynamicObjectConfig*)PhysicObjectConfig);
	}
	case PhysicObjectType_RagdollObject:
	{
		return ConvertRagdollObjectConfigToKeyValues((const CClientRagdollObjectConfig*)PhysicObjectConfig);
	}
	}

	return nullptr;
}

static bool SavePhysicObjectConfigToNewFile(const std::string& filename, const CClientPhysicObjectConfig* PhysicObjectConfig)
{
	auto pKeyValues = ConvertPhysicObjectConfigToKeyValues(PhysicObjectConfig);

	if (!pKeyValues)
		return false;

	SCOPE_EXIT{ delete pKeyValues; };

	bool bSaved = false;

	if(g_pFileSystem_HL25)
		bSaved = pKeyValues->SaveToFile((IFileSystem *)g_pFileSystem_HL25, filename.c_str(), "GAMEDOWNLOAD");
	else
		bSaved = pKeyValues->SaveToFile(g_pFileSystem, filename.c_str(), "GAMEDOWNLOAD");

	if (!bSaved)
	{
		if (g_pFileSystem_HL25)
			bSaved = pKeyValues->SaveToFile((IFileSystem*)g_pFileSystem_HL25, filename.c_str());
		else
			bSaved = pKeyValues->SaveToFile(g_pFileSystem, filename.c_str());
	}

	return bSaved;
}

static bool ParseLegacyDeathAnimLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);
	int sequence;
	float frame;
	if (iss >> sequence >> frame) {
		CClientAnimControlConfig animConfig;
		animConfig.sequence = sequence;
		animConfig.frame = frame;
		animConfig.activity = 1;

		pRagdollConfig->AnimControlConfigs.push_back(animConfig);
		return true;
	}
	gEngfuncs.Con_DPrintf("ParseLegacyDeathAnimLine: failed to parse line \"%s\"", line.c_str());
	return false;
}

static bool ParseLegacyRigidBodyLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line, int flags) {
	std::istringstream iss(line);
	std::string name, shapeType;
	int boneIndex, pBoneIndex;
	float pBoneOffset, size0, size1, mass;

	if (iss >> name >> boneIndex >> pBoneIndex >> shapeType >> pBoneOffset >> size0 >> size1 >> mass) {

		const auto &ItorRigidBodyConfigWithSameName = std::find_if(pRagdollConfig->RigidBodyConfigs.begin(), pRagdollConfig->RigidBodyConfigs.end(), [&name](const std::shared_ptr<CClientRigidBodyConfig>& p) {
			return p->name == name;
		});

		if (ItorRigidBodyConfigWithSameName != pRagdollConfig->RigidBodyConfigs.end())
		{
			gEngfuncs.Con_DPrintf("ParseLegacyRigidBodyLine: multiple rigidbodies with name \"%s\"", name.c_str());
			return false;
		}

		auto pRigidBodyConfig = std::make_shared<CClientRigidBodyConfig>();
		auto pShapeConfig = std::make_shared<CClientCollisionShapeConfig>();

		if (shapeType == "sphere")
		{
			pShapeConfig->type = PhysicShape_Sphere;
			pShapeConfig->size[0] = size0;
			pRigidBodyConfig->ccdRadius = size0 * 0.2f;
		}
		else if (shapeType == "capsule")
		{
			pShapeConfig->type = PhysicShape_Capsule;
			pShapeConfig->direction = PhysicShapeDirection_Y;
			pShapeConfig->size[0] = size0;
			pShapeConfig->size[1] = size1;
			pRigidBodyConfig->ccdRadius = size0 * 0.2f;
		}
		else
		{
			gEngfuncs.Con_DPrintf("ParseLegacyRigidBodyLine: Unsupported shape type \"%s\"", shapeType.c_str());
			return false;
		}

		pRigidBodyConfig->name = name;
		pRigidBodyConfig->boneindex = boneIndex;
		pRigidBodyConfig->pboneindex = pBoneIndex;
		pRigidBodyConfig->pboneoffset = pBoneOffset;
		pRigidBodyConfig->mass = mass;
		pRigidBodyConfig->flags = flags;
		pRigidBodyConfig->isLegacyConfig = true;

		pRigidBodyConfig->collisionShape = pShapeConfig;

		ClientPhysicManager()->AddPhysicConfig(pShapeConfig->configId, pShapeConfig);

		pRagdollConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

		ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

		return true;
	}
	gEngfuncs.Con_DPrintf("ParseLegacyRigidBodyLine: failed to parse line \"%s\"", line.c_str());
	return false;
}

static bool ParseLegacyConstraintLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);
	std::string rigidbodyA, rigidbodyB, constraintType;
	int boneindexA, boneindexB;
	float offsetAX, offsetAY, offsetAZ;
	float offsetBX, offsetBY, offsetBZ;
	float factor0, factor1, factor2;

	if (iss >> rigidbodyA >> rigidbodyB >> constraintType >> boneindexA >> boneindexB
		>> offsetAX >> offsetAY >> offsetAZ >> offsetBX >> offsetBY >> offsetBZ
		>> factor0 >> factor1 >> factor2) {

		auto pConstraintConfig = std::make_shared<CClientConstraintConfig>();

		if (constraintType == "point" || constraintType == "point_collision") {
			pConstraintConfig->type = PhysicConstraint_Point;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularCFM] = BULLET_DEFAULT_ANGULAR_CFM;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularERP] = BULLET_DEFAULT_ANGULAR_ERP;
		}
		else if (constraintType == "conetwist" || constraintType == "conetwist_collision") {
			pConstraintConfig->type = PhysicConstraint_ConeTwist;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit1] = factor0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit2] = factor1;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_ConeTwistTwistSpanLimit] = factor2;

			pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearCFM] = BULLET_DEFAULT_LINEAR_CFM;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_LinearERP] = BULLET_DEFAULT_LINEAR_ERP;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularCFM] = BULLET_DEFAULT_ANGULAR_CFM;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularERP] = BULLET_DEFAULT_ANGULAR_ERP;
		}
		else if (constraintType == "hinge" || constraintType == "hinge_collision") {
			pConstraintConfig->type = PhysicConstraint_Hinge;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_HingeLowLimit] = factor0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_HingeHighLimit] = factor1;

			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularCFM] = BULLET_DEFAULT_ANGULAR_CFM;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularERP] = BULLET_DEFAULT_ANGULAR_ERP;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopCFM] = BULLET_DEFAULT_ANGULAR_STOP_CFM;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_AngularStopERP] = BULLET_DEFAULT_ANGULAR_STOP_ERP;
		}
		else {
			gEngfuncs.Con_DPrintf("ParseConstraintLine: Unsupported constraint type \"%s\"!", constraintType.c_str());
			return false;
		}

		pConstraintConfig->name = std::format("NativeConstraint|{0}|{1}", rigidbodyA, rigidbodyB);
		pConstraintConfig->rigidbodyA = rigidbodyA;
		pConstraintConfig->rigidbodyB = rigidbodyB;
		pConstraintConfig->boneindexA = boneindexA;
		pConstraintConfig->boneindexB = boneindexB;
		pConstraintConfig->offsetA[0] = offsetAX;
		pConstraintConfig->offsetA[1] = offsetAY;
		pConstraintConfig->offsetA[2] = offsetAZ;
		pConstraintConfig->offsetB[0] = offsetBX;
		pConstraintConfig->offsetB[1] = offsetBY;
		pConstraintConfig->offsetB[2] = offsetBZ;
		pConstraintConfig->isLegacyConfig = true;

		if (constraintType.ends_with("_collision")) {
			pConstraintConfig->disableCollision = false;
		}

		pRagdollConfig->ConstraintConfigs.emplace_back(pConstraintConfig); 
		
		ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

		return true;
	}

	gEngfuncs.Con_DPrintf("ParseLegacyConstraintLine: failed to parse line \"%s\"", line.c_str());
	return false;
}

static bool ParseLegacyBarnacleLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);

	std::string rigidbody, type;
	float offsetX, offsetY, offsetZ;
	float factor0, factor1, factor2;
	if (iss >> rigidbody >> type >> offsetX >> offsetY >> offsetZ >> factor0 >> factor1 >> factor2) {
		
		if (type == "dof6")
		{
			auto pConstraintConfig = std::make_shared<CClientConstraintConfig>();
			pConstraintConfig->type = PhysicConstraint_Dof6;
			pConstraintConfig->name = std::format("BarnacleConstraint|@barnacle.body|{0}", rigidbody);
			pConstraintConfig->rigidbodyA = "@barnacle.body";
			pConstraintConfig->rigidbodyB = rigidbody;
			pConstraintConfig->flags = PhysicConstraintFlag_Barnacle;
			pConstraintConfig->originA[0] = 0;
			pConstraintConfig->originA[1] = 0;
			pConstraintConfig->originA[2] = factor1;
			pConstraintConfig->originB[0] = offsetX;
			pConstraintConfig->originB[1] = offsetY;
			pConstraintConfig->originB[2] = offsetZ;
			pConstraintConfig->forward[0] = 1;
			pConstraintConfig->forward[1] = 0;
			pConstraintConfig->forward[2] = 0;
			pConstraintConfig->disableCollision = false;
			pConstraintConfig->useGlobalJointFromA = true;
			pConstraintConfig->useLinearReferenceFrameA = true;
			pConstraintConfig->useLookAtOther = true;
			pConstraintConfig->useGlobalJointOriginFromOther = true;
			pConstraintConfig->useRigidBodyDistanceAsLinearLimit = true;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset] = -factor2;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitX] = -1;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitY] = 0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitZ] = 0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitX] = 0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitY] = 0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitZ] = 0;
			pConstraintConfig->debugDrawLevel = 2;

			pRagdollConfig->BarnacleControlConfig.ConstraintConfigs.emplace_back(pConstraintConfig);

			ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

			auto pActionConfig = std::make_shared<CClientPhysicActionConfig>();

			pActionConfig->type = PhysicAction_BarnacleDragForce;
			pActionConfig->flags = PhysicActionFlag_Barnacle | PhysicActionFlag_AffectsRigidBody;
			pActionConfig->rigidbodyA = rigidbody;
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceMagnitude] = factor0;
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceExtraHeight] = 24;

			pRagdollConfig->BarnacleControlConfig.ActionConfigs.emplace_back(pActionConfig);

			ClientPhysicManager()->AddPhysicConfig(pActionConfig->configId, pActionConfig);

			return true;
		}
		else if (type == "slider")
		{
			auto pConstraintConfig = std::make_shared<CClientConstraintConfig>();
			pConstraintConfig->type = PhysicConstraint_Slider;
			pConstraintConfig->name = std::format("BarnacleConstraint|@barnacle.body|{}", rigidbody);
			pConstraintConfig->rigidbodyA = "@barnacle.body";
			pConstraintConfig->rigidbodyB = rigidbody;
			pConstraintConfig->flags = PhysicConstraintFlag_Barnacle;
			pConstraintConfig->originA[0] = 0;
			pConstraintConfig->originA[1] = 0;
			pConstraintConfig->originA[2] = factor1;
			pConstraintConfig->originB[0] = offsetX;
			pConstraintConfig->originB[1] = offsetY;
			pConstraintConfig->originB[2] = offsetZ;
			pConstraintConfig->forward[0] = 1;
			pConstraintConfig->forward[1] = 0;
			pConstraintConfig->forward[2] = 0;
			pConstraintConfig->disableCollision = false;
			pConstraintConfig->useGlobalJointFromA = true;
			pConstraintConfig->useLinearReferenceFrameA = true;
			pConstraintConfig->useLookAtOther = true;
			pConstraintConfig->useGlobalJointOriginFromOther = true;
			pConstraintConfig->useRigidBodyDistanceAsLinearLimit = true;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset] = -factor2;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderLowerLinearLimit] = -1;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderUpperLinearLimit] = 0;
			pConstraintConfig->debugDrawLevel = 2;

			pRagdollConfig->BarnacleControlConfig.ConstraintConfigs.emplace_back(pConstraintConfig);

			ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

			auto pActionConfig = std::make_shared<CClientPhysicActionConfig>();
			pActionConfig->type = PhysicAction_BarnacleDragForce;
			pActionConfig->name = std::format("BarnacleDragForce|{}", rigidbody);
			pActionConfig->flags = PhysicActionFlag_Barnacle | PhysicActionFlag_AffectsRigidBody;
			pActionConfig->rigidbodyA = rigidbody;
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceMagnitude] = factor0;
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleDragForceExtraHeight] = 24;

			pRagdollConfig->BarnacleControlConfig.ActionConfigs.emplace_back(pActionConfig);

			ClientPhysicManager()->AddPhysicConfig(pActionConfig->configId, pActionConfig);

			return true;
		}
		else if (type == "chewforce")
		{
			auto pActionConfig = std::make_shared<CClientPhysicActionConfig>();

			pActionConfig->type = PhysicAction_BarnacleChewForce;
			pActionConfig->name = std::format("BarnacleChewForce|{}", rigidbody);
			pActionConfig->flags = PhysicActionFlag_Barnacle | PhysicActionFlag_AffectsRigidBody;
			pActionConfig->rigidbodyA = rigidbody;
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleChewForceMagnitude] = factor0;
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleChewForceInterval] = factor1;

			pRagdollConfig->BarnacleControlConfig.ActionConfigs.emplace_back(pActionConfig);

			ClientPhysicManager()->AddPhysicConfig(pActionConfig->configId, pActionConfig);

			return true;
		}
		else if (type == "chewlimit")
		{
			auto pActionConfig = std::make_shared<CClientPhysicActionConfig>();

			pActionConfig->name = std::format("BarnacleConstraintLimitAdjustment|{0}", rigidbody);
			pActionConfig->type = PhysicAction_BarnacleConstraintLimitAdjustment;
			pActionConfig->flags = PhysicActionFlag_Barnacle | PhysicActionFlag_AffectsConstraint;
			pActionConfig->constraint = std::format("BarnacleConstraint|@barnacle.body|{0}", rigidbody);
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleConstraintLimitAdjustmentExtraHeight] = factor1;
			pActionConfig->factors[PhysicActionFactorIdx_BarnacleConstraintLimitAdjustmentInterval] = factor2;

			pRagdollConfig->BarnacleControlConfig.ActionConfigs.emplace_back(pActionConfig); 
			
			ClientPhysicManager()->AddPhysicConfig(pActionConfig->configId, pActionConfig);
			return true;
		}
	}
	gEngfuncs.Con_DPrintf("ParseLegacyBarnacleLine: failed to parse line \"%s\" !\n", line.c_str());
	return false;
}

static bool ParseLegacyCameraControl(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);

	std::string type;
	float offsetX, offsetY, offsetZ;
	if (iss >> type >> offsetX >> offsetY >> offsetZ) {

		if (type == "FirstPerson_AngleOffset")
		{
			pRagdollConfig->FirstPersionViewCameraControlConfig.angles[0] = offsetX;
			pRagdollConfig->FirstPersionViewCameraControlConfig.angles[1] = offsetY;
			pRagdollConfig->FirstPersionViewCameraControlConfig.angles[2] = offsetZ;
			return true;
		}
		else if (type == "FirstPerson_OriginOffset")
		{
			pRagdollConfig->FirstPersionViewCameraControlConfig.origin[0] = offsetX;
			pRagdollConfig->FirstPersionViewCameraControlConfig.origin[1] = offsetY;
			pRagdollConfig->FirstPersionViewCameraControlConfig.origin[2] = offsetZ;
			return true;
		}
		else if (type == "ThirdPerson_AngleOffset")
		{
			pRagdollConfig->ThirdPersionViewCameraControlConfig.angles[0] = offsetX;
			pRagdollConfig->ThirdPersionViewCameraControlConfig.angles[1] = offsetY;
			pRagdollConfig->ThirdPersionViewCameraControlConfig.angles[2] = offsetZ;
			return true;
		}
		else if (type == "ThirdPerson_OriginOffset")
		{
			pRagdollConfig->ThirdPersionViewCameraControlConfig.origin[0] = offsetX;
			pRagdollConfig->ThirdPersionViewCameraControlConfig.origin[1] = offsetY;
			pRagdollConfig->ThirdPersionViewCameraControlConfig.origin[2] = offsetZ;
			return true;
		}
	}
	gEngfuncs.Con_DPrintf("ParseLegacyCameraControl: failed to parse line \"%s\" !\n", line.c_str());
	return false;
}

std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromLegacyFileBuffer(const char *buf)
{
	auto pRagdollConfig = std::make_shared<CClientRagdollObjectConfig>();

	pRagdollConfig->flags |= PhysicObjectFlag_FromConfig;

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
			if (!ParseLegacyDeathAnimLine(pRagdollConfig.get(), line)) {
				return nullptr; // Parsing failed
			}
		}
		else if (section == "RigidBody") {
			if (!ParseLegacyRigidBodyLine(pRagdollConfig.get(), line, PhysicRigidBodyFlag_None)) {
				return nullptr; // Parsing failed
			}
		}
		else if (section == "JiggleBone") {
			if (!ParseLegacyRigidBodyLine(pRagdollConfig.get(), line, PhysicRigidBodyFlag_AlwaysDynamic)) {

				return nullptr; // Parsing failed
			}
		}
		else if (section == "Constraint") {
			if (!ParseLegacyConstraintLine(pRagdollConfig.get(), line)) {
				return nullptr; // Parsing failed
			}
		}
		else if (section == "Barnacle") {
			if (!ParseLegacyBarnacleLine(pRagdollConfig.get(), line)) {
				return nullptr; // Parsing failed
			}
		}
		else if (section == "CameraControl") {
			if (!ParseLegacyCameraControl(pRagdollConfig.get(), line)) {
				return nullptr; // Parsing failed
			}
		}
	}

	return pRagdollConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromLegacyFile(const std::string& filename)
{
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return nullptr;

	SCOPE_EXIT{ gEngfuncs.COM_FreeFile((void*)pFileContent); };

	return LoadPhysicObjectConfigFromLegacyFileBuffer(pFileContent);
}

bool CBasePhysicManager::SavePhysicObjectConfigToFile(const std::string& filename, CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	std::string fullname = filename;

	UTIL_RemoveFileExtension(fullname);

	auto fullname_physic = fullname;
	fullname_physic += "_physics.txt";

	if (SavePhysicObjectConfigToNewFile(fullname_physic, pPhysicObjectConfig))
	{
		gEngfuncs.Con_Printf("SavePhysicObjectConfigToFile: \"%s\" saved !\n", fullname_physic.c_str());
		return true;
	}

	gEngfuncs.Con_Printf("SavePhysicObjectConfigToFile: Failed to save \"%s\"!\n", fullname_physic.c_str());
	return false;
}

bool CBasePhysicManager::LoadPhysicObjectConfigFromBSP(model_t *mod, CClientPhysicObjectConfigStorage& Storage)
{
	auto pIndexArray = GetIndexArrayFromBrushModel(mod);

	if (!pIndexArray)
	{
		Storage.state = PhysicConfigState_LoadedWithError;
		return false;
	}

	auto pCollisionShapeConfig = std::make_shared<CClientCollisionShapeConfig>();

	pCollisionShapeConfig->type = PhysicShape_TriangleMesh;
	pCollisionShapeConfig->m_pVertexArray = m_worldVertexArray;
	pCollisionShapeConfig->m_pIndexArray = pIndexArray;

	auto pRigidBodyConfig = std::make_shared<CClientRigidBodyConfig>();

	pRigidBodyConfig->name = mod->name;
	pRigidBodyConfig->flags = 0;
	pRigidBodyConfig->debugDrawLevel = (mod == r_worldmodel) ? BULLET_WORLD_DEBUG_DRAW_LEVEL : BULLET_DEFAULT_DEBUG_DRAW_LEVEL;
	pRigidBodyConfig->collisionShape = pCollisionShapeConfig;

	ClientPhysicManager()->AddPhysicConfig(pCollisionShapeConfig->configId, pCollisionShapeConfig);

	auto pStaticObjectConfig = std::make_shared<CClientStaticObjectConfig>();

	pStaticObjectConfig->flags |= PhysicObjectFlag_FromBSP;

	pStaticObjectConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

	ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

	Storage.pConfig = pStaticObjectConfig;
	Storage.state = PhysicConfigState_Loaded;
	return true;
}

bool CBasePhysicManager::LoadPhysicObjectConfigFromFiles(const std::string& filename, CClientPhysicObjectConfigStorage &Storage)
{
	std::string fullname = filename;

	if (fullname.length() < 4)
	{
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles: Invalid name \"%s\"\n", filename.c_str());
		Storage.state = PhysicConfigState_LoadedWithError;
		return false;
	}

	UTIL_RemoveFileExtension(fullname);

	auto fullname_physic = fullname;
	fullname_physic += "_physics.txt";

	auto pConfig = LoadPhysicObjectConfigFromNewFile(fullname_physic);

	if(pConfig)
	{
		ClientPhysicManager()->AddPhysicConfig(pConfig->configId, pConfig);

		Storage.pConfig = pConfig;
		Storage.filename = fullname;
		Storage.state = PhysicConfigState_Loaded;
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles:\"%s\" has been loaded successfully.\n", fullname.c_str());
		return true;
	}

	auto fullname_ragdoll = fullname;
	fullname_ragdoll  += "_ragdoll.txt";

	pConfig = LoadPhysicObjectConfigFromLegacyFile(fullname_ragdoll);

	if (pConfig)
	{
		ClientPhysicManager()->AddPhysicConfig(pConfig->configId, pConfig);

		Storage.pConfig = pConfig;
		Storage.filename = fullname;
		Storage.state = PhysicConfigState_Loaded;
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles:\"%s\" has been loaded successfully.\n", fullname.c_str());
		return true;
	}

	//gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles: Could not load physic configs for \"%s\".\n", fullname.c_str());
	Storage.state = PhysicConfigState_LoadedWithError;
	return false;
}

bool CBasePhysicManager::SavePhysicObjectConfigForModel(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex == -1)
	{
		g_pMetaHookAPI->SysError("SavePhysicObjectConfigForModel: Invalid model index %d!\n", modelindex);
		return false;
	}

	return SavePhysicObjectConfigForModelIndex(modelindex);
}

bool CBasePhysicManager::SavePhysicObjectConfigForModelIndex(int modelindex)
{
	if (modelindex >= 0 && modelindex < EngineGetNumKnownModel())
	{
		g_pMetaHookAPI->SysError("SavePhysicObjectConfigForModelIndex: Invalid model index %d!\n", modelindex);
		return false;
	}

	auto& Storage = m_physicObjectConfigs[modelindex];

	if (Storage.state == PhysicConfigState_Loaded)
	{
		return SavePhysicObjectConfigToFile(Storage.filename, Storage.pConfig.get());
	}

	return false;
}

std::shared_ptr<CClientPhysicObjectConfig> CBasePhysicManager::LoadPhysicObjectConfigForModel(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex == -1)
	{
		g_pMetaHookAPI->SysError("LoadPhysicObjectConfigForModel: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	if (modelindex >= m_physicObjectConfigs.size())
	{
		g_pMetaHookAPI->SysError("LoadPhysicObjectConfigForModel: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	auto& Storage = m_physicObjectConfigs[modelindex];

	if (Storage.state == PhysicConfigState_NotLoaded)
	{
		if (mod->type == mod_studio)
		{
			LoadPhysicObjectConfigFromFiles(mod->name, Storage);
		}
		else if (mod->type == mod_brush)
		{
			LoadPhysicObjectConfigFromBSP(mod, Storage);
		}
	}

	return Storage.pConfig;
}

std::shared_ptr<CClientPhysicObjectConfig> CBasePhysicManager::GetPhysicObjectConfigForModel(model_t* mod)
{
	return GetPhysicObjectConfigForModelIndex(EngineGetModelIndex(mod));
}

std::shared_ptr<CClientPhysicObjectConfig> CBasePhysicManager::GetPhysicObjectConfigForModelIndex(int modelindex)
{
	if (modelindex >= m_physicObjectConfigs.size() || modelindex < 0)
	{
		g_pMetaHookAPI->SysError("GetPhysicObjectConfigForModel: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	auto& Storage = m_physicObjectConfigs[modelindex];

	if (Storage.state == PhysicConfigState_Loaded)
		return Storage.pConfig;

	return nullptr;
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

			ClientEntityManager()->DispatchEntityModel(ent, model);

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && 
				PhysicObject->GetModel() == model &&
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model) &&
				PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				if (TransferOwnershipForPhysicObject(playerindex, entindex))
					return;

				RemovePhysicObject(entindex);
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

			ClientEntityManager()->DispatchEntityModel(ent, model);

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject && 
				PhysicObject->GetModel() == model && 
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model) &&
				PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				RemovePhysicObject(entindex);
				CreatePhysicObjectFromConfig(ent, state, model, entindex, playerindex);
			}
		}
		else
		{
			auto entindex = ent->index;
			auto model = mod;

			if (!model)
				return;

			ClientEntityManager()->DispatchEntityModel(ent, model);

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject &&
				PhysicObject->GetModel() == model && 
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model))
			{

			}
			else
			{
				RemovePhysicObject(entindex);
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

			ClientEntityManager()->DispatchEntityModel(ent, model);

			auto PhysicObject = GetPhysicObject(entindex);

			if (PhysicObject &&
				PhysicObject->GetModel() == model &&
				PhysicObject->GetModelScaling() == ClientEntityManager()->GetEntityModelScaling(ent, model) &&
				PhysicObject->GetPlayerIndex() == playerindex)
			{

			}
			else
			{
				if (TransferOwnershipForPhysicObject(playerindex, entindex))
					return;

				RemovePhysicObject(entindex);
				CreatePhysicObjectFromConfig(ent, state, model, entindex, playerindex);
			}
		}
		else
		{
			//TODO other tempents are not supported yet?
		}
	}
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

void CBasePhysicManager::SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t *state, model_t* mod, int entindex, int playerindex, const CClientAnimControlConfig &OverrideAnim)
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

IPhysicObject* CBasePhysicManager::FindBarnacleObjectForPlayer(entity_state_t* playerState)
{
	for (const auto &itor : m_physicObjects)
	{
		auto pPhysicObject = itor.second;

		if (pPhysicObject->GetObjectFlags() & PhysicObjectFlag_Barnacle)
		{
			vec3_t vecClientEntityOrigin;
			VectorCopy(pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

			if (fabs(playerState->origin[0] - vecClientEntityOrigin[0]) < 1 &&
				fabs(playerState->origin[1] - vecClientEntityOrigin[1]) < 1 &&
				playerState->origin[2] < vecClientEntityOrigin[2] + 16)
			{
				return pPhysicObject;
			}
		}
	}

	return nullptr;
}

IPhysicObject* CBasePhysicManager::FindGargantuaObjectForPlayer(entity_state_t* playerState)
{
	for (const auto& itor : m_physicObjects)
	{
		auto pPhysicObject = itor.second;

		if (pPhysicObject->GetObjectFlags() & PhysicObjectFlag_Gargantua)
		{
			vec3_t vecClientEntityOrigin;
			VectorCopy(pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

			if (VectorDistance(playerState->origin, vecClientEntityOrigin) < 128 && pPhysicObject->GetClientEntityState()->sequence == 15)
			{
				return pPhysicObject;
			}
		}
	}

	return nullptr;
}

int CBasePhysicManager::AllocatePhysicComponentId()
{
	++m_allocatedPhysicComponentId;
	return m_allocatedPhysicComponentId;
}

IPhysicComponent* CBasePhysicManager::GetPhysicComponent(int physicComponentId)
{
	auto itor = m_physicComponents.find(physicComponentId);

	if (itor != m_physicComponents.end())
	{
		return itor->second;
	}

	return nullptr;
}

void CBasePhysicManager::AddPhysicComponent(int physicComponentId, IPhysicComponent* pPhysicComponent)
{
	RemovePhysicComponent(physicComponentId);

	AddPhysicComponentToWorld(pPhysicComponent);

	m_physicComponents[physicComponentId] = pPhysicComponent;
}

void CBasePhysicManager::FreePhysicComponent(IPhysicComponent* pPhysicComponent)
{
	RemovePhysicComponentFromWorld(pPhysicComponent);

	pPhysicComponent->Destroy();
}

bool CBasePhysicManager::RemovePhysicComponent(int physicComponentId)
{
	auto itor = m_physicComponents.find(physicComponentId);

	if (itor != m_physicComponents.end())
	{
		auto pPhysicComponent = itor->second;

		FreePhysicComponent(pPhysicComponent);

		m_physicComponents.erase(itor);

		return true;
	}

	return false;
}

void CBasePhysicManager::SetInspectedColor(const vec3_t inspectedColor)
{
	VectorCopy(inspectedColor, m_inspectedColor);
}

void CBasePhysicManager::SetSelectedColor(const vec3_t selectedColor)
{
	VectorCopy(selectedColor, m_selectedColor);
}

void CBasePhysicManager::SetInspectedPhysicComponentId(int physicComponentId)
{
	m_inspectedPhysicComponentId = physicComponentId;
}

int CBasePhysicManager::GetInspectedPhysicComponentId() const
{
	return m_inspectedPhysicComponentId;
}

void CBasePhysicManager::SetSelectedPhysicComponentId(int physicComponentId)
{
	m_selectedPhysicComponentId = physicComponentId;
}

int CBasePhysicManager::GetSelectedPhysicComponentId() const
{
	return m_selectedPhysicComponentId;
}

void CBasePhysicManager::SetInspectedPhysicObjectId(uint64 physicObjectId)
{
	m_inspectedPhysicObjectId = physicObjectId;
}

uint64 CBasePhysicManager::GetInspectedPhysicObjectId() const
{
	return m_inspectedPhysicObjectId;
}

void CBasePhysicManager::SetSelectedPhysicObjectId(uint64 physicObjectId)
{
	m_selectedPhysicObjectId = physicObjectId;
}

uint64 CBasePhysicManager::GetSelectedPhysicObjectId() const
{
	return m_selectedPhysicObjectId;
}

int CBasePhysicManager::AllocatePhysicConfigId()
{
	++m_allocatedPhysicConfigId;
	return m_allocatedPhysicConfigId;
}

std::weak_ptr<CClientBasePhysicConfig> CBasePhysicManager::GetPhysicConfig(int configId)
{
	auto itor = m_physicConfigs.find(configId);

	if (itor != m_physicConfigs.end())
	{
		return itor->second;
	}

	return std::weak_ptr<CClientBasePhysicConfig>();
}

void CBasePhysicManager::AddPhysicConfig(int configId, const std::shared_ptr<CClientBasePhysicConfig>& pPhysicConfig)
{
	m_physicConfigs[configId] = pPhysicConfig;
}

bool CBasePhysicManager::RemovePhysicConfig(int configId)
{
	auto itor = m_physicConfigs.find(configId);

	if (itor != m_physicConfigs.end())
	{
		m_physicConfigs.erase(itor);
		return true;
	}

	return false;
}

void CBasePhysicManager::RemoveAllPhysicConfigs()
{
	m_physicConfigs.clear();
}

//Private impls

void CBasePhysicManager::CreatePhysicObjectFromConfig(cl_entity_t* ent, entity_state_t *state, model_t* mod, int entindex, int playerindex)
{
	auto pPhysicConfig = LoadPhysicObjectConfigForModel(mod);

	if (!pPhysicConfig)
		return;

	if (pPhysicConfig->type == PhysicObjectType_RagdollObject)
	{
		auto pRagdollObjectConfig = (CClientRagdollObjectConfig*)pPhysicConfig.get();

		SetupBonesForRagdollEx(ent, state, mod, entindex, playerindex, pRagdollObjectConfig->IdleAnimConfig);

		CRagdollObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_entindex = entindex;
		CreationParam.m_model = mod;

		if (mod->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(ent, mod);
		}

		CreationParam.m_playerindex = playerindex;

		CreationParam.m_pRagdollObjectConfig = pRagdollObjectConfig;

		LoadAdditionalResourcesForConfig(pRagdollObjectConfig);

		auto pRagdollObject = CreateRagdollObject(CreationParam);

		if (!pRagdollObject)
			return;

		AddPhysicObject(entindex, pRagdollObject);
	}
	else if (pPhysicConfig->type == PhysicObjectType_DynamicObject)
	{
		//TODO
		//CDynamicObjectCreationParameter CreationParam;
	}
	else if (pPhysicConfig->type == PhysicObjectType_StaticObject)
	{
		auto pStaticObjectConfig = (CClientStaticObjectConfig*)pPhysicConfig.get();

		CStaticObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_entindex = entindex;
		CreationParam.m_model = mod;

		if (mod->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(ent, mod);
		}

		CreationParam.m_pStaticObjectConfig = pStaticObjectConfig;

		LoadAdditionalResourcesForConfig(pStaticObjectConfig);

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
	ClientEntityManager()->DispatchEntityModel(ent, mod);

	auto entindex = ClientEntityManager()->GetEntityIndex(ent);

	auto pPhysicObject = GetPhysicObject(entindex);

	if (pPhysicObject && pPhysicObject->GetModel() == mod)
		return;

	auto pPhysicConfig = LoadPhysicObjectConfigForModel(mod);

	if (!pPhysicConfig)
		return;

	if (pPhysicConfig->type != PhysicObjectType_StaticObject)
		return;

	CStaticObjectCreationParameter CreationParam;
	CreationParam.m_entity = ent;
	CreationParam.m_entindex = entindex;
	CreationParam.m_model = mod;
	CreationParam.m_pStaticObjectConfig = (CClientStaticObjectConfig *)pPhysicConfig.get();

	auto pStaticObject = CreateStaticObject(CreationParam);

	if (!pStaticObject)
		return;

	AddPhysicObject(entindex, pStaticObject);
}

void CBasePhysicManager::AddPhysicObject(int entindex, IPhysicObject* pPhysicObject)
{
	RemovePhysicObject(entindex);

	m_physicObjects[entindex] = pPhysicObject;
}

void CBasePhysicManager::FreePhysicObject(IPhysicObject *pPhysicObject)
{
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

bool CBasePhysicManager::RemovePhysicObjectEx(uint64 physicObjectId)
{
	auto entindex = UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(physicObjectId);
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId);

	auto pPhysicObject = GetPhysicObject(entindex);

	if (!pPhysicObject)
		return false;

	if (pPhysicObject->GetModel() != EngineGetModelByIndex(modelindex))
		return false;

	return RemovePhysicObject(entindex);
}

void CBasePhysicManager::RemoveAllPhysicObjects(int withflags, int withoutflags)
{
	for (auto itor = m_physicObjects.begin(); itor != m_physicObjects.end();)
	{
		auto entindex = itor->first;

		auto pPhysicObject = itor->second;

		if ((pPhysicObject->GetObjectFlags() & withflags) && !(pPhysicObject->GetObjectFlags() & withoutflags))
		{
			FreePhysicObject(pPhysicObject);
			itor = m_physicObjects.erase(itor);
			continue;
		}

		itor++;
	}
}

void CBasePhysicManager::UpdateAllPhysicObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time)
{
	if (frame_time <= 0)
		return;

	for (auto itor = m_physicObjects.begin(); itor != m_physicObjects.end();)
	{
		auto entindex = itor->first;
		auto pPhysicObject = itor->second;

		CPhysicObjectUpdateContext ctx(entindex, pPhysicObject);

		if (!ctx.m_bShouldFree)
		{
			//world entity is always present
			if (entindex > 0 && !ClientEntityManager()->IsEntityEmitted(entindex))
			{
				ctx.m_bShouldFree = true;
			}
		}

		if (!ctx.m_bShouldFree)
		{
			pPhysicObject->Update(&ctx);
		}

		if (ctx.m_bShouldFree)
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

	m_worldVertexArray = new CPhysicVertexArray();

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

				Vec3GoldSrcToBullet(Vertexes[j].pos);
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

				Vec3GoldSrcToBullet(Vertexes[2].pos);

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
				m_brushIndexArray[i] = new CPhysicIndexArray();
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

	m_barnacleVertexArray = new CPhysicVertexArray();
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

	m_barnacleIndexArray = new CPhysicIndexArray();

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

	m_gargantuaVertexArray = new CPhysicVertexArray();
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

	m_gargantuaIndexArray = new CPhysicIndexArray();

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
		gEngfuncs.Con_DPrintf("LoadObjToPhysicArrays: (warning) %s.\n", err.c_str());
	}
	if (!err.empty()) {
		gEngfuncs.Con_DPrintf("LoadObjToPhysicArrays: (error) %s.\n", err.c_str());
	}
	if (!ret) {
		gEngfuncs.Con_DPrintf("LoadObjToPhysicArrays: Failed to load \"%s\".\n", objFilename.c_str());
		return false;
	}

	for (size_t i = 0;i < attrib.vertices.size(); i += 3)
	{
		CPhysicBrushVertex vertex;

		vertex.pos[0] = attrib.vertices[0 + i];
		vertex.pos[1] = attrib.vertices[1 + i];
		vertex.pos[2] = attrib.vertices[2 + i];

		Vec3GoldSrcToBullet(vertex.pos);

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

void CBasePhysicManager::LoadAdditionalResourcesForCollisionShapeConfig(CClientCollisionShapeConfig *pCollisionShapeConfig)
{
	if (pCollisionShapeConfig->type == PhysicShape_TriangleMesh && !pCollisionShapeConfig->objpath.empty())
	{
		if (!pCollisionShapeConfig->m_pVertexArrayStorage)
			pCollisionShapeConfig->m_pVertexArrayStorage = new CPhysicVertexArray();

		if (!pCollisionShapeConfig->m_pIndexArrayStorage)
			pCollisionShapeConfig->m_pIndexArrayStorage = new CPhysicIndexArray();

		if (!pCollisionShapeConfig->m_pVertexArray && !pCollisionShapeConfig->m_pIndexArray && pCollisionShapeConfig->m_pVertexArrayStorage && pCollisionShapeConfig->m_pIndexArrayStorage)
		{
			if (LoadObjToPhysicArrays(pCollisionShapeConfig->objpath, pCollisionShapeConfig->m_pVertexArrayStorage, pCollisionShapeConfig->m_pIndexArrayStorage))
			{
				pCollisionShapeConfig->m_pVertexArray = pCollisionShapeConfig->m_pVertexArrayStorage;
				pCollisionShapeConfig->m_pIndexArray = pCollisionShapeConfig->m_pIndexArrayStorage;
			}
		}
	}
	else if (pCollisionShapeConfig->type == PhysicShape_Compound && pCollisionShapeConfig->compoundShapes.size() > 0)
	{
		for (const auto& pSubShapeConfig : pCollisionShapeConfig->compoundShapes)
		{
			LoadAdditionalResourcesForCollisionShapeConfig(pSubShapeConfig.get());
		}
	}
}

void CBasePhysicManager::LoadAdditionalResourcesForConfig(CClientPhysicObjectConfig *pPhysicObjectConfig)
{
	for (const auto& pRigidBodyConfig : pPhysicObjectConfig->RigidBodyConfigs)
	{
		if (pRigidBodyConfig->collisionShape)
		{
			LoadAdditionalResourcesForCollisionShapeConfig(pRigidBodyConfig->collisionShape.get());
		}
	}
}
