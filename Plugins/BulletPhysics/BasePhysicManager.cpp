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
#include <algorithm>
#include <ScopeExit/ScopeExit.h>

#include <KeyValues.h>

template<class T>
inline T* GetWorldSurfaceByIndex(int index)
{
	return (((T*)(*cl_worldmodel)->surfaces) + index);
}

template<class T>
inline int GetWorldSurfaceIndex(T* surf)
{
	auto surf25 = (T*)surf;
	auto surfbase = (T*)(*cl_worldmodel)->surfaces;

	return surf25 - surfbase;
}

IClientPhysicManager* g_pClientPhysicManager{};

IClientPhysicManager* ClientPhysicManager()
{
	return g_pClientPhysicManager;
}

bool StudioGetActivityType(model_t* mod, entity_state_t* entstate, StudioAnimActivityType *pStudioAnimActivityType, int *pAnimControlFlags)
{
	//if (g_bIsSvenCoop)
	//{
	//	if (entstate->scale != 0 && entstate->scale != 1.0f)
	//		return StudioAnimActivityType_Idle;
	//}

	if (mod->type != mod_studio)
		return false;

	auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);

	if (!studiohdr)
		return false;

	int sequence = entstate->sequence;

	if (sequence < 0 || sequence >= studiohdr->numseq)
		return false;

	auto pseqdesc = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex) + sequence;

	if (
		pseqdesc->activity == ACT_DIESIMPLE ||
		pseqdesc->activity == ACT_DIEBACKWARD ||
		pseqdesc->activity == ACT_DIEFORWARD ||
		pseqdesc->activity == ACT_DIEVIOLENT ||
		pseqdesc->activity == ACT_DIE_HEADSHOT ||
		pseqdesc->activity == ACT_DIE_CHESTSHOT ||
		pseqdesc->activity == ACT_DIE_GUTSHOT ||
		pseqdesc->activity == ACT_DIE_BACKSHOT
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_Death;
		(*pAnimControlFlags) = AnimControlFlag_OverrideAllBones;
		return true;
	}

	if (
		pseqdesc->activity == ACT_BARNACLE_HIT ||
		pseqdesc->activity == ACT_BARNACLE_PULL ||
		pseqdesc->activity == ACT_BARNACLE_CHOMP ||
		pseqdesc->activity == ACT_BARNACLE_CHEW
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_CaughtByBarnacle;
		(*pAnimControlFlags) = AnimControlFlag_OverrideAllBones;
		return true;
	}

	if (
		pseqdesc->activity == ACT_EAT
		&& 0 == stricmp(mod->name, "models/barnacle.mdl")
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_BarnacleChewing;
		(*pAnimControlFlags) = 0;
		return true;
	}

	if (
		pseqdesc->activity == ACT_RANGE_ATTACK2
		&& 0 == stricmp(mod->name, "models/garg.mdl")
		)
	{
		(*pStudioAnimActivityType) = StudioAnimActivityType_GargantuaBite;
		(*pAnimControlFlags) = 0;
		return true;
	}

	(*pStudioAnimActivityType) = StudioAnimActivityType_Idle;
	(*pAnimControlFlags) = 0;
	return true;
}

bool CheckPhysicComponentSubFilters(IPhysicComponent* pPhysicComponent, const CPhysicComponentSubFilters& subfilter)
{
	if (subfilter.m_HasWithoutFlags)
	{
		if (pPhysicComponent->GetFlags() & subfilter.m_WithoutFlags)
		{
			return false;
		}
	}

	if (subfilter.m_HasExactMatchFlags)
	{
		if (pPhysicComponent->GetFlags() == subfilter.m_ExactMatchFlags)
		{
			return true;
		}
	}

	if (subfilter.m_HasExactMatchComponentId)
	{
		if (pPhysicComponent->GetPhysicComponentId() == subfilter.m_ExactMatchComponentId)
		{
			return true;
		}
	}

	if (subfilter.m_HasWithFlags)
	{
		if (pPhysicComponent->GetFlags() & subfilter.m_WithFlags)
		{
			return true;
		}
	}

	return false;
}

bool CheckPhysicComponentFilters(IPhysicComponent* pPhysicComponent, const CPhysicComponentFilters& filters)
{
	if (pPhysicComponent->IsRigidBody())
	{
		return CheckPhysicComponentSubFilters(pPhysicComponent, filters.m_RigidBodyFilter);
	}

	if (pPhysicComponent->IsConstraint())
	{
		return CheckPhysicComponentSubFilters(pPhysicComponent, filters.m_ConstraintFilter);
	}

	if (pPhysicComponent->IsPhysicBehavior())
	{
		return CheckPhysicComponentSubFilters(pPhysicComponent, filters.m_PhysicBehaviorFilter);
	}

	return false;
}

bool DispatchPhysicComponentUpdate(IPhysicComponent* PhysicComponent, CPhysicObjectUpdateContext* ObjectUpdateContext, bool bIsAddingPhysicComponent)
{
	CPhysicComponentUpdateContext ComponentUpdateContext(ObjectUpdateContext);

	ComponentUpdateContext.bIsAddingPhysicComponent = true;

	PhysicComponent->Update(&ComponentUpdateContext);

	return !ComponentUpdateContext.m_bShouldFree;
}

void DispatchPhysicComponentsUpdate(std::vector<IPhysicComponent*>& PhysicComponents, CPhysicObjectUpdateContext* ObjectUpdateContext, bool bIsAddingPhysicComponent)
{
	for (auto itor = PhysicComponents.begin(); itor != PhysicComponents.end();)
	{
		auto pPhysicComponent = (*itor);

		if (DispatchPhysicComponentUpdate(pPhysicComponent, ObjectUpdateContext, bIsAddingPhysicComponent))
		{
			itor++; 
		}
		else
		{
			ClientPhysicManager()->RemovePhysicComponent(pPhysicComponent->GetPhysicComponentId());

			itor = PhysicComponents.erase(itor);
		}
	}
}

IPhysicComponent* DispatchGetPhysicComponentByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->GetName() == name)
		{
			return pPhysicComponent;
		}
	}

	return nullptr;
}

IPhysicComponent* DispatchGetPhysicComponentByComponentId(const std::vector<IPhysicComponent*>& m_PhysicComponents, int id)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->GetPhysicComponentId() == id)
		{
			return pPhysicComponent;
		}
	}

	return nullptr;
}

IPhysicRigidBody* DispatchGetRigidBodyByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			if (pRigidBody->GetName() == name)
			{
				return pRigidBody;
			}
		}
	}

	return nullptr;
}

IPhysicRigidBody* DispatchGetRigidBodyByComponentId(const std::vector<IPhysicComponent*>& m_PhysicComponents, int id)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			if (pRigidBody->GetPhysicComponentId() == id)
			{
				return pRigidBody;
			}
		}
	}

	return nullptr;
}

IPhysicConstraint* DispatchGetConstraintByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsConstraint())
		{
			auto pConstraint = (IPhysicConstraint*)pPhysicComponent;

			if (pConstraint->GetName() == name)
			{
				return (IPhysicConstraint*)pConstraint;
			}
		}
	}

	return nullptr;
}

IPhysicConstraint* DispatchGetConstraintByComponentId(const std::vector<IPhysicComponent*> &m_PhysicComponents, int id)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsConstraint())
		{
			auto pConstraint = (IPhysicConstraint*)pPhysicComponent;

			if (pConstraint->GetPhysicComponentId() == id)
			{
				return (IPhysicConstraint*)pConstraint;
			}
		}
	}

	return nullptr;
}

IPhysicBehavior* DispatchGetPhysicBehaviorByName(const std::vector<IPhysicComponent*>& m_PhysicComponents, const std::string& name)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsPhysicBehavior())
		{
			auto pPhysicBehavior = (IPhysicBehavior*)pPhysicComponent;

			if (pPhysicBehavior->GetName() == name)
			{
				return (IPhysicBehavior*)pPhysicBehavior;
			}
		}
	}

	return nullptr;
}

IPhysicBehavior* DispatchGetPhysicBehaviorByComponentId(const std::vector<IPhysicComponent*>& m_PhysicComponents, int id)
{
	for (auto pPhysicComponent : m_PhysicComponents)
	{
		if (pPhysicComponent->IsPhysicBehavior())
		{
			auto pPhysicBehavior = (IPhysicBehavior*)pPhysicComponent;

			if (pPhysicBehavior->GetPhysicComponentId() == id)
			{
				return (IPhysicBehavior*)pPhysicBehavior;
			}
		}
	}

	return nullptr;
}

void DispatchSortPhysicComponents(std::vector<IPhysicComponent*>& PhysicComponents)
{
	// Comparator for sorting according to the rules
	auto comparator = [](IPhysicComponent* a, IPhysicComponent* b) -> bool
		{
			// Determine the primary priority based on type
			int priorityA = (a->IsRigidBody() ? 0 :
				(a->IsConstraint() ? 1 :
					(a->IsPhysicBehavior() ? 2 : 3)));

			int priorityB = (b->IsRigidBody() ? 0 :
				(b->IsConstraint() ? 1 :
					(b->IsPhysicBehavior() ? 2 : 3)));

			// If they have the same type, prioritize by GetPhysicConfigId
			if (priorityA == priorityB)
			{
				return a->GetPhysicConfigId() < b->GetPhysicConfigId();
			}

			// Otherwise, compare based on type priority
			return priorityA < priorityB;
		};

	// Sort the vector using the comparator
	std::sort(PhysicComponents.begin(), PhysicComponents.end(), comparator);
}

void DispatchAddPhysicComponent(std::vector<IPhysicComponent*> &PhysicComponents, IPhysicComponent* pPhysicComponent)
{
	ClientPhysicManager()->AddPhysicComponent(pPhysicComponent->GetPhysicComponentId(), pPhysicComponent);

	CPhysicObjectUpdateContext ObjectUpdateContext;

	bool bShouldAdd = DispatchPhysicComponentUpdate(pPhysicComponent, &ObjectUpdateContext, true);

	if (bShouldAdd)
	{
		PhysicComponents.emplace_back(pPhysicComponent);

		DispatchSortPhysicComponents(PhysicComponents);
	}
	else
	{
		ClientPhysicManager()->RemovePhysicComponent(pPhysicComponent->GetPhysicComponentId());

	}
}

void DispatchRemovePhysicComponents(std::vector<IPhysicComponent*>& PhysicComponents)
{
	for (auto itor = PhysicComponents.rbegin(); itor != PhysicComponents.rend(); itor ++)
	{
		auto pPhysicComponent = (*itor);

		ClientPhysicManager()->RemovePhysicComponent(pPhysicComponent->GetPhysicComponentId());
	}

	PhysicComponents.clear();
}

void DispatchRemovePhysicCompoentsWithFilters(std::vector<IPhysicComponent*>& PhysicComponents, const CPhysicComponentFilters& filters)
{
	for (auto itor = PhysicComponents.begin(); itor != PhysicComponents.end();)
	{
		auto pPhysicComponent = (*itor);

		if (CheckPhysicComponentFilters(pPhysicComponent, filters))
		{
			ClientPhysicManager()->RemovePhysicComponent(pPhysicComponent->GetPhysicComponentId());

			itor = PhysicComponents.erase(itor);
			continue;
		}

		itor++;
	}
}

void DispatchBuildPhysicComponents(
	const CPhysicObjectCreationParameter& CreationParam,
	const std::vector<std::shared_ptr<CClientRigidBodyConfig>>& RigidBodyConfigs,
	const std::vector<std::shared_ptr<CClientConstraintConfig>>& ConstraintConfigs,
	const std::vector<std::shared_ptr<CClientPhysicBehaviorConfig>>& PhysicBehaviorConfigs,
	const std::function<IPhysicRigidBody *(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, int physicComponentId)>& pfnCreateRigidBody,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientRigidBodyConfig* pRigidConfig, IPhysicRigidBody*)>& pfnAddRigidBody,
	const std::function<IPhysicConstraint *(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, int physicComponentId)>& pfnCreateConstraint,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientConstraintConfig* pConstraintConfig, IPhysicConstraint*)>& pfnAddConstraint,
	const std::function<IPhysicBehavior *(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, int physicComponentId)>& pfnCreatePhysicBehavior,
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior*)>& pfnAddPhysicBehavior)
{
	for (const auto& pRigidBodyConfig : RigidBodyConfigs)
	{
		const auto pRigidBodyConfigPtr = pRigidBodyConfig.get();

		auto pRigidBody = pfnCreateRigidBody(CreationParam, pRigidBodyConfigPtr, 0);

		if (pRigidBody)
		{
			pfnAddRigidBody(CreationParam, pRigidBodyConfigPtr, pRigidBody);
		}
	}

	for (const auto& pConstraintConfig : ConstraintConfigs)
	{
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_NonNative)
			continue;

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_DeferredCreate)
			continue;

		auto pConstraint = pfnCreateConstraint(CreationParam, pConstraintConfigPtr, 0);

		if (pConstraint)
		{
			pfnAddConstraint(CreationParam, pConstraintConfigPtr, pConstraint);
		}
	}

	for (const auto& pPhysicBehaviorConfig : PhysicBehaviorConfigs)
	{
		const auto pPhysicBehaviorConfigPtr = pPhysicBehaviorConfig.get();

		if (pPhysicBehaviorConfigPtr->flags & PhysicBehaviorFlag_NonNative)
			continue;

		auto pPhysicBehavior = pfnCreatePhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, 0);

		if (pPhysicBehavior)
		{
			pfnAddPhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, pPhysicBehavior);
		}
	}

	for (const auto& pConstraintConfig : ConstraintConfigs)
	{
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_NonNative)
			continue;

		if (!(pConstraintConfigPtr->flags & PhysicConstraintFlag_DeferredCreate))
			continue;

		auto pConstraint = pfnCreateConstraint(CreationParam, pConstraintConfigPtr, 0);

		if (pConstraint)
		{
			pfnAddConstraint(CreationParam, pConstraintConfigPtr, pConstraint);
		}
	}
}

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
	const std::function<void(const CPhysicObjectCreationParameter& CreationParam, CClientPhysicBehaviorConfig* pPhysicBehaviorConfig, IPhysicBehavior*)>& pfnAddPhysicBehavior)
{
	std::map<int, int> configIdToComponentIdMap;

	for (auto pPhysicComponent : PhysicComponents)
	{
		configIdToComponentIdMap[pPhysicComponent->GetPhysicConfigId()] = pPhysicComponent->GetPhysicComponentId();

		ClientPhysicManager()->RemovePhysicComponent(pPhysicComponent->GetPhysicComponentId());
	}

	PhysicComponents.clear();

	for (const auto& pRigidBodyConfig : RigidBodyConfigs)
	{
		const auto pRigidBodyConfigPtr = pRigidBodyConfig.get();

		auto found = configIdToComponentIdMap.find(pRigidBodyConfigPtr->configId);

		if (found != configIdToComponentIdMap.end())
		{
			auto oldPhysicComponentId = found->second;

			auto pRigidBody = pfnCreateRigidBody(CreationParam, pRigidBodyConfigPtr, oldPhysicComponentId);

			if (pRigidBody)
			{
				pfnAddRigidBody(CreationParam, pRigidBodyConfigPtr, pRigidBody);
			}
		}
		else
		{
			auto pRigidBody = pfnCreateRigidBody(CreationParam, pRigidBodyConfigPtr, 0);

			if (pRigidBody)
			{
				pfnAddRigidBody(CreationParam, pRigidBodyConfigPtr, pRigidBody);
			}
		}
	}

	for (const auto& pConstraintConfig : ConstraintConfigs)
	{
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_NonNative)
			continue;

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_DeferredCreate)
			continue;

		auto found = configIdToComponentIdMap.find(pConstraintConfigPtr->configId);

		if (found != configIdToComponentIdMap.end())
		{
			auto oldPhysicComponentId = found->second;

			auto pConstraint = pfnCreateConstraint(CreationParam, pConstraintConfigPtr, oldPhysicComponentId);

			if (pConstraint)
			{
				pfnAddConstraint(CreationParam, pConstraintConfigPtr, pConstraint);
			}
		}
		else
		{
			auto pConstraint = pfnCreateConstraint(CreationParam, pConstraintConfigPtr, 0);

			if (pConstraint)
			{
				pfnAddConstraint(CreationParam, pConstraintConfigPtr, pConstraint);
			}
		}
	}

	for (const auto& pPhysicBehaviorConfig : PhysicBehaviorConfigs)
	{
		const auto pPhysicBehaviorConfigPtr = pPhysicBehaviorConfig.get();

		if (pPhysicBehaviorConfigPtr->flags & PhysicBehaviorFlag_NonNative)
			continue;

		auto found = configIdToComponentIdMap.find(pPhysicBehaviorConfigPtr->configId);

		if (found != configIdToComponentIdMap.end())
		{
			auto oldPhysicComponentId = found->second;

			auto pPhysicBehavior = pfnCreatePhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, oldPhysicComponentId);

			if (pPhysicBehavior)
			{
				pfnAddPhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, pPhysicBehavior);
			}
		}
		else
		{
			auto pPhysicBehavior = pfnCreatePhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, 0);

			if (pPhysicBehavior)
			{
				pfnAddPhysicBehavior(CreationParam, pPhysicBehaviorConfigPtr, pPhysicBehavior);
			}
		}
	}

	for (const auto& pConstraintConfig : ConstraintConfigs)
	{
		const auto pConstraintConfigPtr = pConstraintConfig.get();

		if (pConstraintConfigPtr->flags & PhysicConstraintFlag_NonNative)
			continue;

		if (!(pConstraintConfigPtr->flags & PhysicConstraintFlag_DeferredCreate))
			continue;

		auto found = configIdToComponentIdMap.find(pConstraintConfigPtr->configId);

		if (found != configIdToComponentIdMap.end())
		{
			auto oldPhysicComponentId = found->second;

			auto pConstraint = pfnCreateConstraint(CreationParam, pConstraintConfigPtr, oldPhysicComponentId);

			if (pConstraint)
			{
				pfnAddConstraint(CreationParam, pConstraintConfigPtr, pConstraint);
			}
		}
		else
		{
			auto pConstraint = pfnCreateConstraint(CreationParam, pConstraintConfigPtr, 0);

			if (pConstraint)
			{
				pfnAddConstraint(CreationParam, pConstraintConfigPtr, pConstraint);
			}
		}
	}
}

bool DispatchStudioCheckBBox(const std::vector<IPhysicComponent*>& PhysicComponents, studiohdr_t* studiohdr, int* nVisible)
{
	bool bSuccess = false;

	vec3_t aabbmins = { 99999, 99999, 99999 }, aabbmaxs = { -99999, -99999, -99999 };
	for (auto pPhysicComponent : PhysicComponents)
	{
		if (pPhysicComponent->IsRigidBody())
		{
			auto pRigidBody = (IPhysicRigidBody*)pPhysicComponent;

			vec3_t mins, maxs;
			if (pRigidBody->GetAABB(mins, maxs))
			{
				if (mins[0] < aabbmins[0]) aabbmins[0] = mins[0];
				if (mins[1] < aabbmins[1]) aabbmins[1] = mins[1];
				if (mins[2] < aabbmins[2]) aabbmins[2] = mins[2];

				if (maxs[0] > aabbmaxs[0]) aabbmaxs[0] = maxs[0];
				if (maxs[1] > aabbmaxs[1]) aabbmaxs[1] = maxs[1];
				if (maxs[2] > aabbmaxs[2]) aabbmaxs[2] = maxs[2];

				bSuccess = true;
			}
		}
	}

	if (bSuccess)
	{
		(*nVisible) = R_CullBox(aabbmins, aabbmaxs) ? 0 : 1;
	}

	return bSuccess;
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

CClientPhysicBehaviorConfig::CClientPhysicBehaviorConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_PhysicBehavior;

	for (int i = 0; i < _ARRAYSIZE(factors); ++i) {
		factors[i] = NAN;
	}
}

CClientAnimControlConfig::CClientAnimControlConfig() : CClientBasePhysicConfig()
{
	configType = PhysicConfigType_AnimControl;
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
	//FirstPersonViewCameraControlConfig.rigidbody = "Head";
	//ThirdPersonViewCameraControlConfig.rigidbody = "Pelvis";
}

void CBasePhysicManager::Destroy()
{
	delete this;
}

void CBasePhysicManager::Init()
{

}

void CBasePhysicManager::Shutdown()
{

}

void CBasePhysicManager::NewMap()
{
	RemoveAllPhysicObjects(PhysicObjectFlag_AnyObject, 0);
	RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromBSP, 0);

	m_allocatedPhysicComponentId = 0;

	m_inspectedPhysicComponentId = 0;
	m_inspectedPhysicObjectId = 0;

	m_selectedPhysicComponentId = 0;
	m_selectedPhysicObjectId = 0;

	m_worldVertexResources.clear();

	FreeAllIndexArrays(PhysicIndexArrayFlag_FromBSP, 0);

	for (int i = 0; i < EngineGetNumKnownModel(); ++i)
	{
		auto mod = EngineGetModelByIndex(i);

		if (mod->type == mod_brush && mod->name[0])
		{
			if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			{
				std::shared_ptr<CPhysicVertexArray> worldVertexArray = GenerateWorldVertexArray(mod);

				if (worldVertexArray)
				{
					GenerateBrushIndexArray(mod, worldVertexArray);
				}
			}
		}
	}
	
	//Deprecated, use .obj instead.
	//GenerateBarnacleIndexVertexArray();
	//GenerateGargantuaIndexVertexArray();

	LoadPhysicObjectConfigs();

	CreatePhysicObjectForBrushModel(r_worldentity, &r_worldentity->curstate, (*cl_worldmodel));
}

void CBasePhysicManager::SetGravity(float value)
{
	m_gravity = value;
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
	int iSavedCount = 0;
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
						if (SavePhysicObjectConfigToFile(mod->name, pConfig.get()))
						{
							iSavedCount++;
						}
					}
				}
			}
		}
	}
	gEngfuncs.Con_Printf("SavePhysicObjectConfigs: %d config(s) saved !\n", iSavedCount);
}

bool CBasePhysicManager::SetupBones(CRagdollObjectSetupBoneContext* Context)
{
	auto pPhysicObject = GetPhysicObject(Context->m_entindex);

	if (!pPhysicObject)
		return false;

	return pPhysicObject->SetupBones(Context);
}

bool CBasePhysicManager::SetupJiggleBones(CRagdollObjectSetupBoneContext* Context)
{
	auto pPhysicObject = GetPhysicObject(Context->m_entindex);

	if (!pPhysicObject)
		return false;

	return pPhysicObject->SetupJiggleBones(Context);
}

bool CBasePhysicManager::StudioCheckBBox(studiohdr_t* studiohdr, int entindex, int* nVisible)
{
	auto pPhysicObject = GetPhysicObject(entindex);

	if (!pPhysicObject)
		return false;

	return pPhysicObject->StudioCheckBBox(studiohdr, nVisible);
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

bool CBasePhysicManager::RebuildPhysicObjectEx2(IPhysicObject* pPhysicObject, const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	CPhysicObjectCreationParameter CreationParam;

	CreationParam.m_entity = pPhysicObject->GetClientEntity();
	CreationParam.m_entstate = pPhysicObject->GetClientEntityState();
	CreationParam.m_entindex = pPhysicObject->GetEntityIndex();
	CreationParam.m_model = pPhysicObject->GetModel();

	if (CreationParam.m_model->type == mod_studio)
	{
		CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(CreationParam.m_model);
		CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(CreationParam.m_entity, CreationParam.m_model);
	}

	CreationParam.m_playerindex = pPhysicObject->GetPlayerIndex();

	CreationParam.m_pPhysicObjectConfig = pPhysicObjectConfig;

	return pPhysicObject->Rebuild(CreationParam);
}

bool CBasePhysicManager::RebuildPhysicObject(int entindex, const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	auto pPhysicObject = GetPhysicObject(entindex);

	if (pPhysicObject)
	{
		return RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig);
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

	return RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig);
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

static void LoadVerifyStuffsFromKeyValues(KeyValues* pKeyValues, CClientPhysicObjectConfig *pConfig)
{
	if (pKeyValues->GetBool("verifyBoneChunk"))
	{
		pConfig->verifyBoneChunk = true;
	}

	pConfig->crc32BoneChunk = pKeyValues->GetString("crc32BoneChunk");

	if (pKeyValues->GetBool("verifyModelFile"))
	{
		pConfig->verifyModelFile = true;
	}

	pConfig->crc32ModelFile = pKeyValues->GetString("crc32ModelFile");
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

	pShapeConfig->resourcePath = pCollisionShapeKey->GetString("resourcePath");

	auto pCompoundShapesKey = pCollisionShapeKey->FindKey("compoundShapes");

	if (pCompoundShapesKey)
	{
		for (auto pSubShapeKey = pCompoundShapesKey->GetFirstSubKey(); pSubShapeKey; pSubShapeKey = pSubShapeKey->GetNextKey())
		{
			auto pSubShape = LoadCollisionShapeFromKeyValues(pSubShapeKey, true);

			pShapeConfig->compoundShapes.emplace_back(pSubShape);
		}
	}

	ClientPhysicManager()->AddPhysicConfig(pShapeConfig->configId, pShapeConfig);

	return pShapeConfig;
}

static std::shared_ptr<CClientRigidBodyConfig> LoadRigidBodyFromKeyValues(KeyValues* pRigidBodySubKey, int allowedRigidBodyFlags)
{
	auto pRigidBodyConfig = std::make_shared<CClientRigidBodyConfig>();

	pRigidBodyConfig->name = pRigidBodySubKey->GetName();

#define LOAD_CONFIG_FLAGS(str, name) if (pRigidBodySubKey->GetBool(#str)){	pRigidBodyConfig->flags |= PhysicRigidBodyFlag_##name;	}

	LOAD_CONFIG_FLAGS(alwaysDynamic, AlwaysDynamic);
	LOAD_CONFIG_FLAGS(alwaysKinematic, AlwaysKinematic);
	LOAD_CONFIG_FLAGS(alwaysStatic, AlwaysStatic);
	LOAD_CONFIG_FLAGS(invertStateOnIdle, InvertStateOnIdle);
	LOAD_CONFIG_FLAGS(invertStateOnDeath, InvertStateOnDeath);
	LOAD_CONFIG_FLAGS(invertStateOnCaughtByBarnacle, InvertStateOnCaughtByBarnacle);
	LOAD_CONFIG_FLAGS(invertStateOnBarnaclePulling, InvertStateOnBarnaclePulling);
	LOAD_CONFIG_FLAGS(invertStateOnBarnacleChewing, InvertStateOnBarnacleChewing);
	LOAD_CONFIG_FLAGS(invertStateOnGargantuaBite, InvertStateOnGargantuaBite);
	LOAD_CONFIG_FLAGS(noCollisionToWorld, NoCollisionToWorld);
	LOAD_CONFIG_FLAGS(noCollisionToStaticObject, NoCollisionToStaticObject);
	LOAD_CONFIG_FLAGS(noCollisionToDynamicObject, NoCollisionToDynamicObject);
	LOAD_CONFIG_FLAGS(noCollisionToRagdollObject, NoCollisionToRagdollObject);

#undef LOAD_CONFIG_FLAGS

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
	pRigidBodyConfig->linearFriction = pRigidBodySubKey->GetFloat("linearFriction", BULLET_DEFAULT_LINEAR_FRICTION);
	pRigidBodyConfig->rollingFriction = pRigidBodySubKey->GetFloat("rollingFriction", BULLET_DEFAULT_ANGULAR_FRICTION);
	pRigidBodyConfig->restitution = pRigidBodySubKey->GetFloat("restitution", BULLET_DEFAULT_RESTITUTION);
	pRigidBodyConfig->ccdRadius = pRigidBodySubKey->GetFloat("ccdRadius", 0);
	pRigidBodyConfig->ccdThreshold = pRigidBodySubKey->GetFloat("ccdThreshold", BULLET_DEFAULT_CCD_THRESHOLD);
	pRigidBodyConfig->linearSleepingThreshold = pRigidBodySubKey->GetFloat("linearSleepingThreshold", BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD);
	pRigidBodyConfig->angularSleepingThreshold = pRigidBodySubKey->GetFloat("angularSleepingThreshold", BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD);

	auto pCollisionShapeKey = pRigidBodySubKey->FindKey("collisionShape");

	if (pCollisionShapeKey)
	{
		pRigidBodyConfig->collisionShape = LoadCollisionShapeFromKeyValues(pCollisionShapeKey, false);
	}

	ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

	return pRigidBodyConfig;
}

static void LoadRigidBodiesFromKeyValues(KeyValues* pKeyValues, int allowedRigidBodyFlags, std::vector<std::shared_ptr<CClientRigidBodyConfig>> &RigidBodyConfigs)
{
	auto pRigidBodiesKey = pKeyValues->FindKey("rigidBodies");

	if (pRigidBodiesKey)
	{
		for (auto pRigidBodySubKey = pRigidBodiesKey->GetFirstSubKey(); pRigidBodySubKey; pRigidBodySubKey = pRigidBodySubKey->GetNextKey())
		{
			auto pRigidBodyConfig = LoadRigidBodyFromKeyValues(pRigidBodySubKey, allowedRigidBodyFlags);

			RigidBodyConfigs.emplace_back(pRigidBodyConfig);
		}
	}
}

static std::shared_ptr<CClientConstraintConfig> LoadConstraintFromKeyValues(KeyValues* pConstraintSubKey)
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

#define LOAD_CONSTRAINT_FLAG(str, name) { if (pConstraintSubKey->GetBool(#str)) pConstraintConfig->flags |= PhysicConstraintFlag_##name; }

	LOAD_CONSTRAINT_FLAG(barnacle, Barnacle);
	LOAD_CONSTRAINT_FLAG(gargantua, Gargantua);
	LOAD_CONSTRAINT_FLAG(deactiveOnNormalActivity, DeactiveOnNormalActivity);
	LOAD_CONSTRAINT_FLAG(deactiveOnDeathActivity, DeactiveOnDeathActivity);
	LOAD_CONSTRAINT_FLAG(deactiveOnCaughtByBarnacleActivity, DeactiveOnCaughtByBarnacleActivity);
	LOAD_CONSTRAINT_FLAG(deactiveOnBarnaclePullingActivity, DeactiveOnBarnaclePullingActivity);
	LOAD_CONSTRAINT_FLAG(deactiveOnBarnacleChewingActivity, DeactiveOnBarnacleChewingActivity);
	LOAD_CONSTRAINT_FLAG(deactiveOnGargantuaBiteActivity, DeactiveOnGargantuaBiteActivity);
	LOAD_CONSTRAINT_FLAG(dontResetPoseOnErrorCorrection, DontResetPoseOnErrorCorrection);
	LOAD_CONSTRAINT_FLAG(DeferredCreate, DeferredCreate);
#undef LOAD_CONSTRAINT_FLAG

#define LOAD_BOOL_FROM_KEYVALUES(name, defaultValue) pConstraintConfig->name = pConstraintSubKey->GetBool(#name, defaultValue);
	LOAD_BOOL_FROM_KEYVALUES(disableCollision, true);
	LOAD_BOOL_FROM_KEYVALUES(useGlobalJointFromA, true);
	LOAD_BOOL_FROM_KEYVALUES(useLinearReferenceFrameA, true);
	LOAD_BOOL_FROM_KEYVALUES(useLookAtOther, false);
	LOAD_BOOL_FROM_KEYVALUES(useGlobalJointOriginFromOther, false);
	LOAD_BOOL_FROM_KEYVALUES(useRigidBodyDistanceAsLinearLimit, false);
	LOAD_BOOL_FROM_KEYVALUES(useSeperateLocalFrame, false);
#undef LOAD_BOOL_FROM_KEYVALUES

	pConstraintConfig->debugDrawLevel = pConstraintSubKey->GetInt("debugDrawLevel", BULLET_DEFAULT_DEBUG_DRAW_LEVEL);

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

	ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

	return pConstraintConfig;
}

static void LoadConstraintsFromKeyValues(KeyValues* pKeyValues, std::vector<std::shared_ptr<CClientConstraintConfig>> &ConstraintConfigs)
{
	auto pConstraintsKey = pKeyValues->FindKey("constraints");

	if (pConstraintsKey)
	{
		for (auto pConstraintSubKey = pConstraintsKey->GetFirstSubKey(); pConstraintSubKey; pConstraintSubKey = pConstraintSubKey->GetNextKey())
		{
			auto pConstraintConfig = LoadConstraintFromKeyValues(pConstraintSubKey);

			ConstraintConfigs.emplace_back(pConstraintConfig);
		}
	}
}
static std::shared_ptr<CClientAnimControlConfig> LoadAnimControlFromKeyValues(KeyValues* pAnimControlSubKey)
{
	auto pAnimControlConfig = std::make_shared<CClientAnimControlConfig>();

	pAnimControlConfig->sequence = pAnimControlSubKey->GetInt("sequence", -1);
	pAnimControlConfig->gaitsequence = pAnimControlSubKey->GetInt("gaitsequence", -1);
	pAnimControlConfig->animframe = pAnimControlSubKey->GetFloat("animframe", 0);
	pAnimControlConfig->activityType = (decltype(pAnimControlConfig->activityType))pAnimControlSubKey->GetInt("activityType", 0);
	pAnimControlConfig->flags = pAnimControlSubKey->GetInt("flags", 0);
	pAnimControlConfig->controller[0] = pAnimControlSubKey->GetInt("controller_0", -1);
	pAnimControlConfig->controller[1] = pAnimControlSubKey->GetInt("controller_1", -1);
	pAnimControlConfig->controller[2] = pAnimControlSubKey->GetInt("controller_2", -1);
	pAnimControlConfig->controller[3] = pAnimControlSubKey->GetInt("controller_3", -1);
	pAnimControlConfig->blending[0] = pAnimControlSubKey->GetInt("blending_0", -1);
	pAnimControlConfig->blending[1] = pAnimControlSubKey->GetInt("blending_1", -1);
	pAnimControlConfig->blending[2] = pAnimControlSubKey->GetInt("blending_2", -1);
	pAnimControlConfig->blending[3] = pAnimControlSubKey->GetInt("blending_3", -1);

	ClientPhysicManager()->AddPhysicConfig(pAnimControlConfig->configId, pAnimControlConfig);

	return pAnimControlConfig;
}

static void LoadAnimControlsFromKeyValues(KeyValues* pKeyValues, std::vector<std::shared_ptr<CClientAnimControlConfig>>& AnimControlConfigs)
{
	auto pAnimControlsKey = pKeyValues->FindKey("animControls");

	if (pAnimControlsKey)
	{
		for (auto pAnimControlSubKey = pAnimControlsKey->GetFirstSubKey(); pAnimControlSubKey; pAnimControlSubKey = pAnimControlSubKey->GetNextKey())
		{
			auto pAnimControlConfig = LoadAnimControlFromKeyValues(pAnimControlSubKey);

			AnimControlConfigs.emplace_back(pAnimControlConfig);
		}
	}
}

static std::shared_ptr<CClientPhysicBehaviorConfig> LoadPhysicBehaviorFromKeyValues(KeyValues* pPhysicBehaviorSubKey)
{
	auto pPhysicBehaviorConfig = std::make_shared<CClientPhysicBehaviorConfig>();

	pPhysicBehaviorConfig->name = pPhysicBehaviorSubKey->GetName();

	auto type = pPhysicBehaviorSubKey->GetString("type");

	if (type)
	{
		pPhysicBehaviorConfig->type = UTIL_GetPhysicBehaviorTypeFromTypeName(type);
	}

	pPhysicBehaviorConfig->rigidbodyA = pPhysicBehaviorSubKey->GetString("rigidbodyA");
	pPhysicBehaviorConfig->rigidbodyB = pPhysicBehaviorSubKey->GetString("rigidbodyB");
	pPhysicBehaviorConfig->constraint = pPhysicBehaviorSubKey->GetString("constraint");

	if (pPhysicBehaviorSubKey->GetBool("barnacle"))
		pPhysicBehaviorConfig->flags |= PhysicBehaviorFlag_Barnacle;

	if (pPhysicBehaviorSubKey->GetBool("gargantua"))
		pPhysicBehaviorConfig->flags |= PhysicBehaviorFlag_Gargantua;


	auto origin = pPhysicBehaviorSubKey->GetString("origin");

	if (origin)
	{
		UTIL_ParseStringAsVector3(origin, pPhysicBehaviorConfig->origin);
	}

	auto angles = pPhysicBehaviorSubKey->GetString("angles");

	if (angles)
	{
		UTIL_ParseStringAsVector3(angles, pPhysicBehaviorConfig->angles);
	}


#define LOAD_FACTOR_FLOAT(name) pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_##name] = pFactorsKey->GetFloat(#name, NAN);

	auto pFactorsKey = pPhysicBehaviorSubKey->FindKey("factors");

	if (pFactorsKey)
	{
		switch (pPhysicBehaviorConfig->type)
		{
		case PhysicBehavior_BarnacleDragOnRigidBody:
		{
			LOAD_FACTOR_FLOAT(BarnacleDragMagnitude);
			LOAD_FACTOR_FLOAT(BarnacleDragExtraHeight);
			break;
		}
		case PhysicBehavior_BarnacleDragOnConstraint:
		{
			LOAD_FACTOR_FLOAT(BarnacleDragMagnitude);
			LOAD_FACTOR_FLOAT(BarnacleDragVelocity);
			LOAD_FACTOR_FLOAT(BarnacleDragExtraHeight);
			LOAD_FACTOR_FLOAT(BarnacleDragLimitAxis);
			LOAD_FACTOR_FLOAT(BarnacleDragCalculateLimitFromActualPlayerOrigin);
			LOAD_FACTOR_FLOAT(BarnacleDragUseServoMotor);
			LOAD_FACTOR_FLOAT(BarnacleDragActivatedOnBarnaclePulling);
			LOAD_FACTOR_FLOAT(BarnacleDragActivatedOnBarnacleChewing);
			break;
		}
		case PhysicBehavior_BarnacleChew:
		{
			LOAD_FACTOR_FLOAT(BarnacleChewMagnitude);
			LOAD_FACTOR_FLOAT(BarnacleChewInterval);
			break;
		}
		case PhysicBehavior_BarnacleConstraintLimitAdjustment:
		{
			LOAD_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentExtraHeight);
			LOAD_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentInterval);
			LOAD_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentAxis);
			break;
		}
		case PhysicBehavior_GargantuaDragOnConstraint:
		{
			LOAD_FACTOR_FLOAT(BarnacleDragMagnitude);
			LOAD_FACTOR_FLOAT(BarnacleDragVelocity);
			LOAD_FACTOR_FLOAT(BarnacleDragExtraHeight);
			LOAD_FACTOR_FLOAT(BarnacleDragLimitAxis);
			LOAD_FACTOR_FLOAT(BarnacleDragUseServoMotor);
			break;
		}
		case PhysicBehavior_FirstPersonViewCamera:
		case PhysicBehavior_ThirdPersonViewCamera: 
		{
			LOAD_FACTOR_FLOAT(CameraActivateOnIdle);
			LOAD_FACTOR_FLOAT(CameraActivateOnDeath);
			LOAD_FACTOR_FLOAT(CameraActivateOnCaughtByBarnacle);
			LOAD_FACTOR_FLOAT(CameraSyncViewOrigin);
			LOAD_FACTOR_FLOAT(CameraSyncViewAngles);
			LOAD_FACTOR_FLOAT(CameraUseSimOrigin);
			LOAD_FACTOR_FLOAT(CameraOriginalViewHeightStand);
			LOAD_FACTOR_FLOAT(CameraOriginalViewHeightDuck);
			LOAD_FACTOR_FLOAT(CameraMappedViewHeightStand);
			LOAD_FACTOR_FLOAT(CameraMappedViewHeightDuck);
			LOAD_FACTOR_FLOAT(CameraNewViewHeightDucking);
			break;
		}
		case PhysicBehavior_SimpleBuoyancy:
		{
			LOAD_FACTOR_FLOAT(SimpleBuoyancyMagnitude);
			LOAD_FACTOR_FLOAT(SimpleBuoyancyLinearDamping);
			LOAD_FACTOR_FLOAT(SimpleBuoyancyAngularDamping);
			break;
		}
		}
	}

#undef LOAD_FACTOR_FLOAT

	ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfig->configId, pPhysicBehaviorConfig);

	return pPhysicBehaviorConfig;
}

static void LoadPhysicBehaviorsFromKeyValues(KeyValues* pKeyValues, std::vector<std::shared_ptr<CClientPhysicBehaviorConfig>> & PhysicBehaviorConfigs)
{
	auto pPhysicBehaviorsKey = pKeyValues->FindKey("physicBehaviors");

	if (pPhysicBehaviorsKey)
	{
		for (auto pPhysicBehaviorSubKey = pPhysicBehaviorsKey->GetFirstSubKey(); pPhysicBehaviorSubKey; pPhysicBehaviorSubKey = pPhysicBehaviorSubKey->GetNextKey())
		{
			auto pPhysicBehaviorConfig = LoadPhysicBehaviorFromKeyValues(pPhysicBehaviorSubKey);

			PhysicBehaviorConfigs.emplace_back(pPhysicBehaviorConfig);
		}
	}
}
#if 0
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
#endif
static bool VerifyIntegrityForPhysicObjectConfig(model_t* mod, const CClientPhysicObjectConfig *pPhysicObjectConfig)
{
	if (pPhysicObjectConfig->verifyBoneChunk)
	{
		std::string Crc32Value;
		if (UTIL_GetCrc32ForBoneChunk(mod, &Crc32Value) && Crc32Value != pPhysicObjectConfig->crc32BoneChunk)
		{
			gEngfuncs.Con_Printf("VerifyIntegrityForPhysicObjectConfig: \"%s\" failed the crc32-bonechunk integrity check ! expect \"%s\" but got \"%s\" !\n", mod->name, pPhysicObjectConfig->crc32BoneChunk.c_str(), Crc32Value.c_str());
			return false;
		}
	}
	if (pPhysicObjectConfig->verifyModelFile)
	{
		std::string Crc32Value;
		if (UTIL_GetCrc32ForModelFile(mod, &Crc32Value) && Crc32Value != pPhysicObjectConfig->crc32ModelFile)
		{
			gEngfuncs.Con_Printf("VerifyIntegrityForPhysicObjectConfig: \"%s\" failed the crc32-modelfile integrity check ! expect \"%s\" but got \"%s\" !\n", mod->name, pPhysicObjectConfig->crc32ModelFile.c_str(), Crc32Value.c_str());
			return false;
		}
	}
	return true;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadRagdollObjectConfigFromKeyValues(model_t* mod, KeyValues* pKeyValues)
{
	auto pRagdollObjectConfig = std::make_shared<CClientRagdollObjectConfig>();

	LoadVerifyStuffsFromKeyValues(pKeyValues, pRagdollObjectConfig.get());

	if (!VerifyIntegrityForPhysicObjectConfig(mod, pRagdollObjectConfig.get()))
		return nullptr;

	LoadPhysicObjectFlagsFromKeyValues(pKeyValues, pRagdollObjectConfig->flags);
	LoadRigidBodiesFromKeyValues(pKeyValues, PhysicRigidBodyFlag_AllowedOnRagdollObject, pRagdollObjectConfig->RigidBodyConfigs);
	LoadConstraintsFromKeyValues(pKeyValues, pRagdollObjectConfig->ConstraintConfigs);
	LoadPhysicBehaviorsFromKeyValues(pKeyValues, pRagdollObjectConfig->PhysicBehaviorConfigs);
	LoadAnimControlsFromKeyValues(pKeyValues, pRagdollObjectConfig->AnimControlConfigs);
	//LoadCameraControlFromKeyValues(pKeyValues, "firstPersonViewCameraControl", pRagdollObjectConfig->FirstPersonViewCameraControlConfig);
	//LoadCameraControlFromKeyValues(pKeyValues, "thirdPersonViewCameraControl", pRagdollObjectConfig->ThirdPersonViewCameraControlConfig);

	ClientPhysicManager()->AddPhysicConfig(pRagdollObjectConfig->configId, pRagdollObjectConfig);

	return pRagdollObjectConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadStaticObjectConfigFromKeyValues(model_t* mod, KeyValues* pKeyValues)
{
	auto pStaticObjectConfig = std::make_shared<CClientStaticObjectConfig>();

	LoadVerifyStuffsFromKeyValues(pKeyValues, pStaticObjectConfig.get());

	if (!VerifyIntegrityForPhysicObjectConfig(mod, pStaticObjectConfig.get()))
		return nullptr;

	LoadPhysicObjectFlagsFromKeyValues(pKeyValues, pStaticObjectConfig->flags);
	LoadRigidBodiesFromKeyValues(pKeyValues, PhysicRigidBodyFlag_AllowedOnStaticObject, pStaticObjectConfig->RigidBodyConfigs);

	ClientPhysicManager()->AddPhysicConfig(pStaticObjectConfig->configId, pStaticObjectConfig);

	return pStaticObjectConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadDynamicObjectConfigFromKeyValues(model_t* mod, KeyValues* pKeyValues)
{
	auto pDynamicObjectConfig = std::make_shared<CClientDynamicObjectConfig>();

	LoadVerifyStuffsFromKeyValues(pKeyValues, pDynamicObjectConfig.get());

	if (!VerifyIntegrityForPhysicObjectConfig(mod, pDynamicObjectConfig.get()))
		return nullptr;

	LoadPhysicObjectFlagsFromKeyValues(pKeyValues, pDynamicObjectConfig->flags);
	LoadRigidBodiesFromKeyValues(pKeyValues, PhysicRigidBodyFlag_AllowedOnDynamicObject, pDynamicObjectConfig->RigidBodyConfigs);
	LoadConstraintsFromKeyValues(pKeyValues, pDynamicObjectConfig->ConstraintConfigs);

	ClientPhysicManager()->AddPhysicConfig(pDynamicObjectConfig->configId, pDynamicObjectConfig);

	return pDynamicObjectConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromKeyValues(model_t* mod, KeyValues *pKeyValues)
{
	auto type = pKeyValues->GetString("type");

	if (type)
	{
		if (!strcmp(type, "RagdollObject"))
		{
			return LoadRagdollObjectConfigFromKeyValues(mod, pKeyValues);
		}
		else if (!strcmp(type, "StaticObject"))
		{
			return LoadStaticObjectConfigFromKeyValues(mod, pKeyValues);
		}
		else if (!strcmp(type, "DynamicObject"))
		{
			return LoadDynamicObjectConfigFromKeyValues(mod, pKeyValues);
		}

		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromKeyValues: Invalid type \"%s\" from KeyValues!\n", type);
	}
	else
	{
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromKeyValues: \"type\" not found in KeyValues!\n");
	}

	return nullptr;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromNewFile(model_t* mod, const std::string& filename)
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

	return LoadPhysicObjectConfigFromKeyValues(mod, pKeyValues);
}

static void AddVerifyStuffsFromKeyValues(KeyValues* pKeyValues, const CClientPhysicObjectConfig* pConfig)
{
	if (pConfig->verifyBoneChunk)
	{
		pKeyValues->SetBool("verifyBoneChunk", true);
	}

	pKeyValues->SetString("crc32BoneChunk", pConfig->crc32BoneChunk.c_str());

	if (pConfig->verifyModelFile)
	{
		pKeyValues->SetBool("verifyModelFile", true);
	}

	pKeyValues->SetString("crc32ModelFile", pConfig->crc32ModelFile.c_str());
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

	if (pCollisionShapeConfig->resourcePath.size() > 0)
	{
		pCollisionShapeSubKey->SetString("resourcePath", pCollisionShapeConfig->resourcePath.c_str());
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
#define SAVE_CONFIG_FLAGS(str, name) {if (pRigidBodyConfig->flags & PhysicRigidBodyFlag_##name) pRigidBodySubKey->SetBool(#str, true);}
					SAVE_CONFIG_FLAGS(alwaysDynamic,				AlwaysDynamic);
					SAVE_CONFIG_FLAGS(alwaysKinematic,				AlwaysKinematic);
					SAVE_CONFIG_FLAGS(alwaysStatic,					AlwaysStatic);
					SAVE_CONFIG_FLAGS(invertStateOnIdle,					InvertStateOnIdle);
					SAVE_CONFIG_FLAGS(invertStateOnDeath,					InvertStateOnDeath);
					SAVE_CONFIG_FLAGS(invertStateOnCaughtByBarnacle,		InvertStateOnCaughtByBarnacle);
					SAVE_CONFIG_FLAGS(invertStateOnBarnaclePulling,			InvertStateOnBarnaclePulling);
					SAVE_CONFIG_FLAGS(invertStateOnBarnacleChewing,			InvertStateOnBarnacleChewing);
					SAVE_CONFIG_FLAGS(invertStateOnGargantuaBite,			InvertStateOnGargantuaBite);
					SAVE_CONFIG_FLAGS(noCollisionToWorld,			NoCollisionToWorld);
					SAVE_CONFIG_FLAGS(noCollisionToStaticObject,	NoCollisionToStaticObject);
					SAVE_CONFIG_FLAGS(noCollisionToDynamicObject,	NoCollisionToDynamicObject);
					SAVE_CONFIG_FLAGS(noCollisionToRagdollObject,	NoCollisionToRagdollObject);
#undef SAVE_CONFIG_FLAGS

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
#define SAVE_CONSTRAINT_FLAG(str, name) { if (pConstraintConfig->flags & PhysicConstraintFlag_##name) pConstraintSubKey->SetBool(#str, true); }

					SAVE_CONSTRAINT_FLAG(barnacle, Barnacle);
					SAVE_CONSTRAINT_FLAG(gargantua, Gargantua);
					SAVE_CONSTRAINT_FLAG(deactiveOnNormalActivity, DeactiveOnNormalActivity);
					SAVE_CONSTRAINT_FLAG(deactiveOnDeathActivity, DeactiveOnDeathActivity);
					SAVE_CONSTRAINT_FLAG(deactiveOnCaughtByBarnacleActivity, DeactiveOnCaughtByBarnacleActivity);
					SAVE_CONSTRAINT_FLAG(deactiveOnBarnaclePullingActivity, DeactiveOnBarnaclePullingActivity);
					SAVE_CONSTRAINT_FLAG(deactiveOnBarnacleChewingActivity, DeactiveOnBarnacleChewingActivity);
					SAVE_CONSTRAINT_FLAG(deactiveOnGargantuaBiteActivity, DeactiveOnGargantuaBiteActivity);
					SAVE_CONSTRAINT_FLAG(dontResetPoseOnErrorCorrection, DontResetPoseOnErrorCorrection);
					SAVE_CONSTRAINT_FLAG(DeferredCreate, DeferredCreate);

#undef SAVE_CONSTRAINT_FLAG

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

#define SAVE_BOOL_TO_KEYVALUES(name, defaultValue) {if(pConstraintConfig->name != defaultValue) pConstraintSubKey->SetBool(#name, pConstraintConfig->name); }
					SAVE_BOOL_TO_KEYVALUES(disableCollision, true);
					SAVE_BOOL_TO_KEYVALUES(useGlobalJointFromA, true);
					SAVE_BOOL_TO_KEYVALUES(useLinearReferenceFrameA, true);
					SAVE_BOOL_TO_KEYVALUES(useLookAtOther, false);
					SAVE_BOOL_TO_KEYVALUES(useGlobalJointOriginFromOther, false);
					SAVE_BOOL_TO_KEYVALUES(useRigidBodyDistanceAsLinearLimit, false);
					SAVE_BOOL_TO_KEYVALUES(useSeperateLocalFrame, false);
#undef SAVE_BOOL_TO_KEYVALUES

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

static void AddPhysicBehaviorsToKeyValues(KeyValues* pKeyValues, const std::vector<std::shared_ptr<CClientPhysicBehaviorConfig>>& PhysicBehaviorConfigs)
{
	if (PhysicBehaviorConfigs.size() > 0)
	{
		auto pPhysicBehaviorsKey = pKeyValues->FindKey("physicBehaviors", true);

		if (pPhysicBehaviorsKey)
		{
			for (const auto& pPhysicBehaviorConfig : PhysicBehaviorConfigs)
			{
				auto pPhysicBehaviorSubKey = pPhysicBehaviorsKey->FindKey(pPhysicBehaviorConfig->name.c_str(), true);

				if (pPhysicBehaviorSubKey)
				{
					pPhysicBehaviorSubKey->SetString("type", UTIL_GetPhysicBehaviorTypeName(pPhysicBehaviorConfig->type));
					pPhysicBehaviorSubKey->SetString("rigidbodyA", pPhysicBehaviorConfig->rigidbodyA.c_str());
					pPhysicBehaviorSubKey->SetString("rigidbodyB", pPhysicBehaviorConfig->rigidbodyB.c_str());
					pPhysicBehaviorSubKey->SetString("constraint", pPhysicBehaviorConfig->constraint.c_str());

					if (pPhysicBehaviorConfig->flags & PhysicBehaviorFlag_Barnacle)
						pPhysicBehaviorSubKey->SetBool("barnacle", true);

					if (pPhysicBehaviorConfig->flags & PhysicBehaviorFlag_Gargantua)
						pPhysicBehaviorSubKey->SetBool("gargantua", true);

					if (VectorLength(pPhysicBehaviorConfig->origin) > 0)
					{
						pPhysicBehaviorSubKey->SetString("origin", std::format("{0} {1} {2}", pPhysicBehaviorConfig->origin[0], pPhysicBehaviorConfig->origin[1], pPhysicBehaviorConfig->origin[2]).c_str());
					}

					if (VectorLength(pPhysicBehaviorConfig->angles) > 0)
					{
						pPhysicBehaviorSubKey->SetString("angles", std::format("{0} {1} {2}", pPhysicBehaviorConfig->angles[0], pPhysicBehaviorConfig->angles[1], pPhysicBehaviorConfig->angles[2]).c_str());
					}

#define SET_FACTOR_FLOAT(name) if(!isnan(pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_##name])) pFactorsKey->SetFloat(#name, pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_##name]);

					auto pFactorsKey = pPhysicBehaviorSubKey->FindKey("factors", true);
					
					if (pFactorsKey)
					{
						switch (pPhysicBehaviorConfig->type)
						{
						case PhysicBehavior_BarnacleDragOnRigidBody:
						{
							SET_FACTOR_FLOAT(BarnacleDragMagnitude);
							SET_FACTOR_FLOAT(BarnacleDragExtraHeight);
							break;
						}
						case PhysicBehavior_BarnacleDragOnConstraint:
						{
							SET_FACTOR_FLOAT(BarnacleDragMagnitude);
							SET_FACTOR_FLOAT(BarnacleDragVelocity);
							SET_FACTOR_FLOAT(BarnacleDragExtraHeight);
							SET_FACTOR_FLOAT(BarnacleDragLimitAxis);
							SET_FACTOR_FLOAT(BarnacleDragCalculateLimitFromActualPlayerOrigin);
							SET_FACTOR_FLOAT(BarnacleDragUseServoMotor);
							SET_FACTOR_FLOAT(BarnacleDragActivatedOnBarnaclePulling);
							SET_FACTOR_FLOAT(BarnacleDragActivatedOnBarnacleChewing);
							break;
						}
						case PhysicBehavior_BarnacleChew:
						{
							SET_FACTOR_FLOAT(BarnacleChewMagnitude);
							SET_FACTOR_FLOAT(BarnacleChewInterval);
							break;
						}
						case PhysicBehavior_BarnacleConstraintLimitAdjustment:
						{
							SET_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentExtraHeight);
							SET_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentInterval);
							SET_FACTOR_FLOAT(BarnacleConstraintLimitAdjustmentAxis);
							break;
						}
						case PhysicBehavior_GargantuaDragOnConstraint:
						{
							SET_FACTOR_FLOAT(BarnacleDragMagnitude);
							SET_FACTOR_FLOAT(BarnacleDragVelocity);
							SET_FACTOR_FLOAT(BarnacleDragExtraHeight);
							SET_FACTOR_FLOAT(BarnacleDragLimitAxis);
							SET_FACTOR_FLOAT(BarnacleDragUseServoMotor);
							break;
						}
						case PhysicBehavior_FirstPersonViewCamera:
						case PhysicBehavior_ThirdPersonViewCamera: {
							SET_FACTOR_FLOAT(CameraActivateOnIdle);
							SET_FACTOR_FLOAT(CameraActivateOnDeath);
							SET_FACTOR_FLOAT(CameraActivateOnCaughtByBarnacle);
							SET_FACTOR_FLOAT(CameraSyncViewOrigin);
							SET_FACTOR_FLOAT(CameraSyncViewAngles);
							SET_FACTOR_FLOAT(CameraUseSimOrigin);
							SET_FACTOR_FLOAT(CameraOriginalViewHeightStand);
							SET_FACTOR_FLOAT(CameraOriginalViewHeightDuck);
							SET_FACTOR_FLOAT(CameraMappedViewHeightStand);
							SET_FACTOR_FLOAT(CameraMappedViewHeightDuck);
							SET_FACTOR_FLOAT(CameraNewViewHeightDucking);
							break;
						}
						case PhysicBehavior_SimpleBuoyancy:
						{
							SET_FACTOR_FLOAT(SimpleBuoyancyMagnitude);
							SET_FACTOR_FLOAT(SimpleBuoyancyLinearDamping);
							SET_FACTOR_FLOAT(SimpleBuoyancyAngularDamping);
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

static void AddAnimControlToKeyValues(KeyValues* pKeyValues, const std::vector<std::shared_ptr<CClientAnimControlConfig>>& AnimControlConfigs)
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
					pAnimControlSubKey->SetInt("sequence", AnimControl->sequence);
					pAnimControlSubKey->SetInt("gaitsequence", AnimControl->gaitsequence);
					pAnimControlSubKey->SetFloat("animframe", AnimControl->animframe);
					pAnimControlSubKey->SetInt("activityType", (int)AnimControl->activityType);
					pAnimControlSubKey->SetInt("flags", AnimControl->flags);
					pAnimControlSubKey->SetInt("controller_0", AnimControl->controller[0]);
					pAnimControlSubKey->SetInt("controller_1", AnimControl->controller[1]);
					pAnimControlSubKey->SetInt("controller_2", AnimControl->controller[2]);
					pAnimControlSubKey->SetInt("controller_3", AnimControl->controller[3]);
					pAnimControlSubKey->SetInt("blending_0", AnimControl->blending[0]);
					pAnimControlSubKey->SetInt("blending_1", AnimControl->blending[1]);
					pAnimControlSubKey->SetInt("blending_2", AnimControl->blending[2]);
					pAnimControlSubKey->SetInt("blending_3", AnimControl->blending[3]);
				}
			}
		}
	}
}

#if 0
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
#endif

static KeyValues* ConvertStaticObjectConfigToKeyValues(const CClientStaticObjectConfig* StaticObjectConfig)
{
	auto pKeyValues = new KeyValues("PhysicObjectConfig");

	AddVerifyStuffsFromKeyValues(pKeyValues, StaticObjectConfig);
	AddBaseConfigToKeyValues(pKeyValues, StaticObjectConfig);
	AddRigidBodiesToKeyValues(pKeyValues, StaticObjectConfig->RigidBodyConfigs);

	return pKeyValues;
}

static KeyValues* ConvertDynamicObjectConfigToKeyValues(const CClientDynamicObjectConfig* DynamicObjectConfig)
{
	auto pKeyValues = new KeyValues("PhysicObjectConfig");

	AddVerifyStuffsFromKeyValues(pKeyValues, DynamicObjectConfig);
	AddBaseConfigToKeyValues(pKeyValues, DynamicObjectConfig);
	AddRigidBodiesToKeyValues(pKeyValues, DynamicObjectConfig->RigidBodyConfigs);
	AddConstraintsToKeyValues(pKeyValues, DynamicObjectConfig->ConstraintConfigs);

	return pKeyValues;
}

static KeyValues* ConvertRagdollObjectConfigToKeyValues(const CClientRagdollObjectConfig* RagdollObjectConfig)
{
	auto pKeyValues = new KeyValues("PhysicObjectConfig");

	AddVerifyStuffsFromKeyValues(pKeyValues, RagdollObjectConfig);
	AddBaseConfigToKeyValues(pKeyValues, RagdollObjectConfig);
	AddRigidBodiesToKeyValues(pKeyValues, RagdollObjectConfig->RigidBodyConfigs);
	AddConstraintsToKeyValues(pKeyValues, RagdollObjectConfig->ConstraintConfigs);
	AddPhysicBehaviorsToKeyValues(pKeyValues, RagdollObjectConfig->PhysicBehaviorConfigs);
	AddAnimControlToKeyValues(pKeyValues, RagdollObjectConfig->AnimControlConfigs);
	//AddCameraControlToKeyValues(pKeyValues, "firstPersonViewCameraControl", RagdollObjectConfig->FirstPersonViewCameraControlConfig);
	//AddCameraControlToKeyValues(pKeyValues, "thirdPersonViewCameraControl", RagdollObjectConfig->ThirdPersonViewCameraControlConfig);

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

	char filepath[MAX_PATH] = { 0 };
	V_ExtractFilePath(filename.c_str(), filepath, sizeof(filepath));

	bool bSaved = false;

	if (!bSaved)
	{
		FILESYSTEM_ANY_CREATEDIR(filepath, "GAMEDOWNLOAD");

		if (g_pFileSystem_HL25)
			bSaved = pKeyValues->SaveToFile((IFileSystem*)g_pFileSystem_HL25, filename.c_str(), "GAMEDOWNLOAD");
		else
			bSaved = pKeyValues->SaveToFile(g_pFileSystem, filename.c_str(), "GAMEDOWNLOAD");
	}

	if (!bSaved)
	{
		FILESYSTEM_ANY_CREATEDIR(filepath);

		if (g_pFileSystem_HL25)
			bSaved = pKeyValues->SaveToFile((IFileSystem*)g_pFileSystem_HL25, filename.c_str());
		else
			bSaved = pKeyValues->SaveToFile(g_pFileSystem, filename.c_str());
	}

	return bSaved;
}

static bool ParseLegacyDeathAnimLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);
	int sequence = -1;
	float animframe = 0;
	if (iss >> sequence >> animframe) {

		auto pAnimControlConfig = std::make_shared<CClientAnimControlConfig>();

		pAnimControlConfig->sequence = sequence;
		pAnimControlConfig->animframe = animframe;
		pAnimControlConfig->activityType = StudioAnimActivityType_Death;
		pAnimControlConfig->flags = AnimControlFlag_OverrideAllBones;

		ClientPhysicManager()->AddPhysicConfig(pAnimControlConfig->configId, pAnimControlConfig);

		pRagdollConfig->AnimControlConfigs.emplace_back(pAnimControlConfig);

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

		ClientPhysicManager()->AddPhysicConfig(pShapeConfig->configId, pShapeConfig);

		pRigidBodyConfig->name = name;
		pRigidBodyConfig->boneindex = boneIndex;
		pRigidBodyConfig->pboneindex = pBoneIndex;
		pRigidBodyConfig->pboneoffset = pBoneOffset;
		pRigidBodyConfig->mass = mass;
		pRigidBodyConfig->flags = flags;
		pRigidBodyConfig->isLegacyConfig = true;

		pRigidBodyConfig->collisionShape = pShapeConfig;

		ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

		pRagdollConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

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

		ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

		pRagdollConfig->ConstraintConfigs.emplace_back(pConstraintConfig);

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
			pConstraintConfig->name = std::format("BarnacleConstraint|{0}", rigidbody);
			pConstraintConfig->rigidbodyA = "@barnacle.Body";
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

			ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

			pRagdollConfig->ConstraintConfigs.emplace_back(pConstraintConfig);

			auto pPhysicBehaviorConfig = std::make_shared<CClientPhysicBehaviorConfig>();

			pPhysicBehaviorConfig->type = PhysicBehavior_BarnacleDragOnRigidBody;
			pPhysicBehaviorConfig->name = std::format("BarnacleDrag|{}", rigidbody);
			pPhysicBehaviorConfig->flags = PhysicBehaviorFlag_Barnacle;
			pPhysicBehaviorConfig->rigidbodyA = rigidbody;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragMagnitude] = factor0;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragExtraHeight] = 24;

			ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfig->configId, pPhysicBehaviorConfig);

			pRagdollConfig->PhysicBehaviorConfigs.emplace_back(pPhysicBehaviorConfig);

			return true;
		}
		else if (type == "slider")
		{
			auto pConstraintConfig = std::make_shared<CClientConstraintConfig>();
			pConstraintConfig->type = PhysicConstraint_Slider;
			pConstraintConfig->name = std::format("BarnacleConstraint|{0}", rigidbody);
			pConstraintConfig->rigidbodyA = "@barnacle.Body";
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
			pConstraintConfig->useLookAtOther = true;
			pConstraintConfig->useGlobalJointOriginFromOther = true;
			pConstraintConfig->useRigidBodyDistanceAsLinearLimit = true;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset] = -factor2;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderLowerLinearLimit] = -1;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_SliderUpperLinearLimit] = 0;
			pConstraintConfig->debugDrawLevel = 2;

			ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

			pRagdollConfig->ConstraintConfigs.emplace_back(pConstraintConfig);

			auto pPhysicBehaviorConfig = std::make_shared<CClientPhysicBehaviorConfig>();
			pPhysicBehaviorConfig->type = PhysicBehavior_BarnacleDragOnRigidBody;
			pPhysicBehaviorConfig->name = std::format("BarnacleDrag|{}", rigidbody);
			pPhysicBehaviorConfig->flags = PhysicBehaviorFlag_Barnacle;
			pPhysicBehaviorConfig->rigidbodyA = rigidbody;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragMagnitude] = factor0;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragExtraHeight] = 24;

			ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfig->configId, pPhysicBehaviorConfig);

			pRagdollConfig->PhysicBehaviorConfigs.emplace_back(pPhysicBehaviorConfig);

			return true;
		}
		else if (type == "chewforce")
		{
			auto pPhysicBehaviorConfig = std::make_shared<CClientPhysicBehaviorConfig>();

			pPhysicBehaviorConfig->type = PhysicBehavior_BarnacleChew;
			pPhysicBehaviorConfig->name = std::format("BarnacleChew|{}", rigidbody);
			pPhysicBehaviorConfig->flags = PhysicBehaviorFlag_Barnacle;
			pPhysicBehaviorConfig->rigidbodyA = rigidbody;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleChewMagnitude] = factor0;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleChewInterval] = factor1;

			ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfig->configId, pPhysicBehaviorConfig);

			pRagdollConfig->PhysicBehaviorConfigs.emplace_back(pPhysicBehaviorConfig);

			return true;
		}
		else if (type == "chewlimit")
		{
			auto pPhysicBehaviorConfig = std::make_shared<CClientPhysicBehaviorConfig>();

			pPhysicBehaviorConfig->name = std::format("BarnacleConstraintLimitAdjustment|{0}", rigidbody);
			pPhysicBehaviorConfig->type = PhysicBehavior_BarnacleConstraintLimitAdjustment;
			pPhysicBehaviorConfig->flags = PhysicBehaviorFlag_Barnacle;
			pPhysicBehaviorConfig->constraint = std::format("BarnacleConstraint|{0}", rigidbody);
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentExtraHeight] = factor1;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentInterval] = factor2;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentAxis] = -1;

			ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfig->configId, pPhysicBehaviorConfig);

			pRagdollConfig->PhysicBehaviorConfigs.emplace_back(pPhysicBehaviorConfig);

			return true;
		}
	}
	gEngfuncs.Con_DPrintf("ParseLegacyBarnacleLine: failed to parse line \"%s\" !\n", line.c_str());
	return false;
}

static bool ParseLegacyGargantuaLine(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);

	std::string rigidbodyA, rigidbodyB, type;
	float offsetX, offsetY, offsetZ;
	float factor0, factor1, factor2;
	if (iss >> rigidbodyA >> rigidbodyB >> type >> offsetX >> offsetY >> offsetZ >> factor0 >> factor1 >> factor2) {

		if (type == "dof6z")
		{
			auto pConstraintConfig = std::make_shared<CClientConstraintConfig>();
			pConstraintConfig->type = PhysicConstraint_Dof6Spring;
			pConstraintConfig->name = std::format("GargConstraint|{0}", rigidbodyA);
			pConstraintConfig->rigidbodyA = std::format("@gargantua.{0}", rigidbodyB);
			pConstraintConfig->rigidbodyB = rigidbodyA;
			pConstraintConfig->flags = PhysicConstraintFlag_Gargantua;
			pConstraintConfig->originA[0] = 0;
			pConstraintConfig->originA[1] = 0;
			pConstraintConfig->originA[2] = factor1;
			pConstraintConfig->originB[0] = offsetX;
			pConstraintConfig->originB[1] = offsetY;
			pConstraintConfig->originB[2] = offsetZ;
			pConstraintConfig->anglesA[0] = -90;
			pConstraintConfig->anglesA[1] = 0;
			pConstraintConfig->anglesA[2] = 0;
			pConstraintConfig->forward[0] = 1;
			pConstraintConfig->forward[1] = 0;
			pConstraintConfig->forward[2] = 0;
			pConstraintConfig->disableCollision = false;
			pConstraintConfig->useGlobalJointFromA = true;
			pConstraintConfig->useLinearReferenceFrameA = true;
			pConstraintConfig->useSeperateLocalFrame = true;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitX] = -60;//-30-factor2?
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitY] = 0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitZ] = 0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitX] = 30;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitY] = 0;
			pConstraintConfig->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitZ] = 0;
			pConstraintConfig->debugDrawLevel = 2;

			ClientPhysicManager()->AddPhysicConfig(pConstraintConfig->configId, pConstraintConfig);

			pRagdollConfig->ConstraintConfigs.emplace_back(pConstraintConfig);

			auto pPhysicBehaviorConfig = std::make_shared<CClientPhysicBehaviorConfig>();

			pPhysicBehaviorConfig->type = PhysicBehavior_GargantuaDragOnConstraint;
			pPhysicBehaviorConfig->name = std::format("GargantuaDragForce|{}", rigidbodyA);
			pPhysicBehaviorConfig->flags = PhysicBehaviorFlag_Gargantua;
			pPhysicBehaviorConfig->constraint = std::format("GargConstraint|{0}", rigidbodyA);
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragMagnitude] = factor0;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragVelocity] = -60;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragExtraHeight] = 0;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragLimitAxis] = 0;
			pPhysicBehaviorConfig->factors[PhysicBehaviorFactorIdx_BarnacleDragUseServoMotor] = 0;

			ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfig->configId, pPhysicBehaviorConfig);

			pRagdollConfig->PhysicBehaviorConfigs.emplace_back(pPhysicBehaviorConfig);

			return true;
		}
		if (type == "dof6")
		{
			//not implemented
			return true;
		}
	}
	gEngfuncs.Con_DPrintf("ParseLegacyGargantuaLine: failed to parse line \"%s\" !\n", line.c_str());
	return false;
}

static bool ParseLegacyCameraControl(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {

	std::shared_ptr<CClientPhysicBehaviorConfig> pPhysicBehaviorConfigFirstPersonView;
	std::shared_ptr<CClientPhysicBehaviorConfig> pPhysicBehaviorConfigThirdPersonView;

	const auto& HeadItor = std::find_if(pRagdollConfig->RigidBodyConfigs.begin(), pRagdollConfig->RigidBodyConfigs.end(), [](const std::shared_ptr<CClientRigidBodyConfig>& p) {
		return p->name == "Head";
	});

	if (HeadItor != pRagdollConfig->RigidBodyConfigs.end())
	{
		pPhysicBehaviorConfigFirstPersonView = std::make_shared<CClientPhysicBehaviorConfig>();

		pPhysicBehaviorConfigFirstPersonView->name = "HeadCamera";
		pPhysicBehaviorConfigFirstPersonView->type = PhysicBehavior_FirstPersonViewCamera;
		pPhysicBehaviorConfigFirstPersonView->rigidbodyA = "Head";
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraActivateOnIdle] = PhysicBehaviorFactorDefaultValue_CameraActivateOnIdle;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraActivateOnDeath] = PhysicBehaviorFactorDefaultValue_CameraActivateOnDeath;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraActivateOnCaughtByBarnacle] = PhysicBehaviorFactorDefaultValue_CameraActivateOnCaughtByBarnacle;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraSyncViewOrigin] = 1.0f;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraSyncViewAngles] = 1.0f;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraUseSimOrigin] = 0.0f;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraOriginalViewHeightStand] = PhysicBehaviorFactorDefaultValue_CameraOriginalViewHeightStand;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraOriginalViewHeightDuck] = PhysicBehaviorFactorDefaultValue_CameraOriginalViewHeightDuck;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraMappedViewHeightStand] = PhysicBehaviorFactorDefaultValue_CameraMappedViewHeightStand;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraMappedViewHeightDuck] = PhysicBehaviorFactorDefaultValue_CameraMappedViewHeightDuck;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraNewViewHeightDucking] = PhysicBehaviorFactorDefaultValue_CameraNewViewHeightDucking;

		ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfigFirstPersonView->configId, pPhysicBehaviorConfigFirstPersonView);

		pRagdollConfig->PhysicBehaviorConfigs.push_back(pPhysicBehaviorConfigFirstPersonView);
	}

	const auto& PelvisItor = std::find_if(pRagdollConfig->RigidBodyConfigs.begin(), pRagdollConfig->RigidBodyConfigs.end(), [](const std::shared_ptr<CClientRigidBodyConfig>& p) {
		return p->name == "Pelvis";
	});

	if (PelvisItor != pRagdollConfig->RigidBodyConfigs.end())
	{
		pPhysicBehaviorConfigThirdPersonView = std::make_shared<CClientPhysicBehaviorConfig>();

		pPhysicBehaviorConfigThirdPersonView->name = "PelvisCamera";
		pPhysicBehaviorConfigThirdPersonView->type = PhysicBehavior_ThirdPersonViewCamera;
		pPhysicBehaviorConfigThirdPersonView->rigidbodyA = "Pelvis";
		pPhysicBehaviorConfigThirdPersonView->factors[PhysicBehaviorFactorIdx_CameraActivateOnIdle] = 0;
		pPhysicBehaviorConfigThirdPersonView->factors[PhysicBehaviorFactorIdx_CameraActivateOnDeath] = 1;
		pPhysicBehaviorConfigThirdPersonView->factors[PhysicBehaviorFactorIdx_CameraActivateOnCaughtByBarnacle] = 1;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraSyncViewOrigin] = PhysicBehaviorFactorDefaultValue_CameraSyncViewOrigin;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraSyncViewAngles] = 0;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraUseSimOrigin] = 0;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraOriginalViewHeightStand] = PhysicBehaviorFactorDefaultValue_CameraOriginalViewHeightStand;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraOriginalViewHeightDuck] = PhysicBehaviorFactorDefaultValue_CameraOriginalViewHeightDuck;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraMappedViewHeightStand] = PhysicBehaviorFactorDefaultValue_CameraMappedViewHeightStand;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraMappedViewHeightDuck] = PhysicBehaviorFactorDefaultValue_CameraMappedViewHeightDuck;
		pPhysicBehaviorConfigFirstPersonView->factors[PhysicBehaviorFactorIdx_CameraNewViewHeightDucking] = PhysicBehaviorFactorDefaultValue_CameraNewViewHeightDucking;

		ClientPhysicManager()->AddPhysicConfig(pPhysicBehaviorConfigThirdPersonView->configId, pPhysicBehaviorConfigThirdPersonView);

		pRagdollConfig->PhysicBehaviorConfigs.push_back(pPhysicBehaviorConfigThirdPersonView);
	}

	std::istringstream iss(line);

	std::string type;
	float offsetX, offsetY, offsetZ;
	if (iss >> type >> offsetX >> offsetY >> offsetZ) {

		if (type == "FirstPerson_AngleOffset")
		{
			if (pPhysicBehaviorConfigFirstPersonView)
			{
				pPhysicBehaviorConfigFirstPersonView->angles[0] = offsetX;
				pPhysicBehaviorConfigFirstPersonView->angles[1] = offsetY;
				pPhysicBehaviorConfigFirstPersonView->angles[2] = offsetZ;
			}
			return true;
		}
		else if (type == "FirstPerson_OriginOffset")
		{
			if (pPhysicBehaviorConfigFirstPersonView)
			{
				pPhysicBehaviorConfigFirstPersonView->origin[0] = offsetX;
				pPhysicBehaviorConfigFirstPersonView->origin[1] = offsetY;
				pPhysicBehaviorConfigFirstPersonView->origin[2] = offsetZ;
			}
			return true;
		}
		else if (type == "ThirdPerson_AngleOffset")
		{
			if (pPhysicBehaviorConfigThirdPersonView)
			{
				pPhysicBehaviorConfigThirdPersonView->angles[0] = offsetX;
				pPhysicBehaviorConfigThirdPersonView->angles[1] = offsetY;
				pPhysicBehaviorConfigThirdPersonView->angles[2] = offsetZ;
			}
			return true;
		}
		else if (type == "ThirdPerson_OriginOffset")
		{
			if (pPhysicBehaviorConfigThirdPersonView)
			{
				pPhysicBehaviorConfigThirdPersonView->origin[0] = offsetX;
				pPhysicBehaviorConfigThirdPersonView->origin[1] = offsetY;
				pPhysicBehaviorConfigThirdPersonView->origin[2] = offsetZ;
			}
			return true;
		}
	}
	gEngfuncs.Con_DPrintf("ParseLegacyCameraControl: failed to parse line \"%s\" !\n", line.c_str());
	return false;
}

static bool ParseLegacyWaterControl(CClientRagdollObjectConfig* pRagdollConfig, const std::string& line) {
	std::istringstream iss(line);

	std::string rigidbody;
	float offsetX, offsetY, offsetZ;
	float factor0, factor1, factor2;

	if (iss >> rigidbody >> offsetX >> offsetY >> offsetZ >> factor0 >> factor1 >> factor2) {

		auto pSimpleBuoyancyBehavior = std::make_shared<CClientPhysicBehaviorConfig>();

		pSimpleBuoyancyBehavior->name = std::format("SimpleBuoyancy|{}", rigidbody);
		pSimpleBuoyancyBehavior->type = PhysicBehavior_SimpleBuoyancy;
		pSimpleBuoyancyBehavior->rigidbodyA = rigidbody;
		pSimpleBuoyancyBehavior->factors[PhysicBehaviorFactorIdx_SimpleBuoyancyMagnitude] = factor0;
		pSimpleBuoyancyBehavior->factors[PhysicBehaviorFactorIdx_SimpleBuoyancyLinearDamping] = factor1;
		pSimpleBuoyancyBehavior->factors[PhysicBehaviorFactorIdx_SimpleBuoyancyAngularDamping] = factor2;

		ClientPhysicManager()->AddPhysicConfig(pSimpleBuoyancyBehavior->configId, pSimpleBuoyancyBehavior);

		pRagdollConfig->PhysicBehaviorConfigs.push_back(pSimpleBuoyancyBehavior);
		return true;
	}
	return false;
}

std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromLegacyFileBuffer(const char *buf)
{
	auto pRagdollConfig = std::make_shared<CClientRagdollObjectConfig>();

	pRagdollConfig->flags |= PhysicObjectFlag_FromConfig;
	pRagdollConfig->flags |= PhysicObjectFlag_OverrideStudioCheckBBox;

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
			if (!ParseLegacyRigidBodyLine(pRagdollConfig.get(), line, PhysicRigidBodyFlag_InvertStateOnDeath | PhysicRigidBodyFlag_InvertStateOnCaughtByBarnacle)) {
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
		else if (section == "Gargantua") {
			if (!ParseLegacyGargantuaLine(pRagdollConfig.get(), line)) {
				return nullptr; // Parsing failed
			}
		}
		else if (section == "CameraControl") {
			if (!ParseLegacyCameraControl(pRagdollConfig.get(), line)) {
				return nullptr; // Parsing failed
			}
		}
		else if (section == "WaterControl") {
			if (!ParseLegacyWaterControl(pRagdollConfig.get(), line)) {
				return nullptr; // Parsing failed
			}
		}
	}

	ClientPhysicManager()->AddPhysicConfig(pRagdollConfig->configId, pRagdollConfig);

	return pRagdollConfig;
}

static std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromLegacyFile(model_t* mod, const std::string& filename)
{
	auto pFileContent = (const char*)gEngfuncs.COM_LoadFile(filename.c_str(), 5, NULL);

	if (!pFileContent)
		return nullptr;

	SCOPE_EXIT{ gEngfuncs.COM_FreeFile((void*)pFileContent); };

	return LoadPhysicObjectConfigFromLegacyFileBuffer(pFileContent);
}

bool CBasePhysicManager::SavePhysicObjectConfigToFile(const std::string& modelname, CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	if (!(pPhysicObjectConfig->flags & PhysicObjectFlag_FromConfig))
	{
		return false;
	}

	if(!UTIL_IsPhysicObjectConfigModified(pPhysicObjectConfig))
	{
		return false;
	}

	std::string fullname = modelname;

	UTIL_RemoveFileExtension(fullname);

	auto fullname_physic = fullname;
	fullname_physic += "_physics.txt";

	if (SavePhysicObjectConfigToNewFile(fullname_physic, pPhysicObjectConfig))
	{
		UTIL_SetPhysicObjectConfigUnmodified(pPhysicObjectConfig);

		gEngfuncs.Con_Printf("SavePhysicObjectConfigToFile: \"%s\" saved !\n", fullname_physic.c_str());
		return true;
	}

	gEngfuncs.Con_Printf("SavePhysicObjectConfigToFile: Failed to save \"%s\"!\n", fullname_physic.c_str());
	return false;
}

bool CBasePhysicManager::LoadPhysicObjectConfigFromBSP(model_t *mod, CClientPhysicObjectConfigStorage& Storage)
{
	auto resourcePath = UTIL_GetAbsoluteModelName(mod);

	auto pIndexArray = LoadIndexArrayFromResource(resourcePath);

	if (!pIndexArray || !pIndexArray->vIndexBuffer.size() || (pIndexArray->flags & PhysicIndexArrayFlag_LoadFailed))
	{
		Storage.state = PhysicConfigState_LoadedWithError;
		return false;
	}

	auto pCollisionShapeConfig = std::make_shared<CClientCollisionShapeConfig>();

	pCollisionShapeConfig->type = PhysicShape_TriangleMesh;
	pCollisionShapeConfig->resourcePath = resourcePath;

	ClientPhysicManager()->AddPhysicConfig(pCollisionShapeConfig->configId, pCollisionShapeConfig);

	auto pRigidBodyConfig = std::make_shared<CClientRigidBodyConfig>();

	pRigidBodyConfig->name = mod->name;
	pRigidBodyConfig->mass = 0;
	pRigidBodyConfig->flags = 0;
	pRigidBodyConfig->debugDrawLevel = (mod == (*cl_worldmodel)) ? BULLET_WORLD_DEBUG_DRAW_LEVEL : BULLET_DEFAULT_DEBUG_DRAW_LEVEL;
	pRigidBodyConfig->collisionShape = pCollisionShapeConfig;

	ClientPhysicManager()->AddPhysicConfig(pRigidBodyConfig->configId, pRigidBodyConfig);

	auto pStaticObjectConfig = std::make_shared<CClientStaticObjectConfig>();

	pStaticObjectConfig->flags |= PhysicObjectFlag_FromBSP;
	pStaticObjectConfig->debugDrawLevel = (mod == (*cl_worldmodel)) ? BULLET_WORLD_DEBUG_DRAW_LEVEL : BULLET_DEFAULT_DEBUG_DRAW_LEVEL;

	pStaticObjectConfig->RigidBodyConfigs.emplace_back(pRigidBodyConfig);

	ClientPhysicManager()->AddPhysicConfig(pStaticObjectConfig->configId, pStaticObjectConfig);

	OverwritePhysicObjectConfig(resourcePath, Storage, pStaticObjectConfig);

	return true;
}

void CBasePhysicManager::OverwritePhysicObjectConfig(const std::string& modelname, CClientPhysicObjectConfigStorage& Storage, const std::shared_ptr<CClientPhysicObjectConfig> &pPhysicObjectConfig)
{
	Storage.pConfig = pPhysicObjectConfig;
	Storage.modelname = modelname;
	Storage.state = PhysicConfigState_Loaded;

	pPhysicObjectConfig->modelName = modelname;

	char szShortName[64] = {0};
	V_FileBase(modelname.c_str(), szShortName, sizeof(szShortName) - 1);

	pPhysicObjectConfig->shortName = szShortName;
}

bool CBasePhysicManager::CreateEmptyPhysicObjectConfig(const std::string& modelname, CClientPhysicObjectConfigStorage& Storage, int PhysicObjectType)
{
	switch (PhysicObjectType)
	{
	case PhysicObjectType_StaticObject:
	{
		auto pPhysicObjectConfig = std::make_shared<CClientStaticObjectConfig>();
		pPhysicObjectConfig->configModified = true;
		ClientPhysicManager()->AddPhysicConfig(pPhysicObjectConfig->configId, pPhysicObjectConfig);

		OverwritePhysicObjectConfig(modelname, Storage, pPhysicObjectConfig);
		return true;
	}
	case PhysicObjectType_DynamicObject:
	{
		auto pPhysicObjectConfig = std::make_shared<CClientDynamicObjectConfig>();
		pPhysicObjectConfig->configModified = true;
		ClientPhysicManager()->AddPhysicConfig(pPhysicObjectConfig->configId, pPhysicObjectConfig);

		OverwritePhysicObjectConfig(modelname, Storage, pPhysicObjectConfig);
		return true;
	}
	case PhysicObjectType_RagdollObject:
	{
		auto pPhysicObjectConfig = std::make_shared<CClientRagdollObjectConfig>();
		pPhysicObjectConfig->configModified = true;
		ClientPhysicManager()->AddPhysicConfig(pPhysicObjectConfig->configId, pPhysicObjectConfig);

		OverwritePhysicObjectConfig(modelname, Storage, pPhysicObjectConfig);
		return true;
	}
	}

	return false;
}

bool CBasePhysicManager::LoadPhysicObjectConfigFromFiles(model_t *mod, CClientPhysicObjectConfigStorage &Storage)
{
	std::string modelname = mod->name;

	if (modelname.length() < 4)
	{
		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles: Invalid name \"%s\"\n", modelname.c_str());
		Storage.state = PhysicConfigState_LoadedWithError;
		return false;
	}

	UTIL_RemoveFileExtension(modelname);

	auto fullname_physic = modelname;
	fullname_physic += "_physics.txt";

	auto pConfig = LoadPhysicObjectConfigFromNewFile(mod, fullname_physic);

	if(pConfig)
	{
		OverwritePhysicObjectConfig(modelname, Storage, pConfig);

		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles: \"%s\" has been loaded successfully.\n", fullname_physic.c_str());
		return true;
	}

	auto fullname_ragdoll = modelname;
	fullname_ragdoll  += "_ragdoll.txt";

	pConfig = LoadPhysicObjectConfigFromLegacyFile(mod, fullname_ragdoll);

	if (pConfig)
	{
		OverwritePhysicObjectConfig(modelname, Storage, pConfig);

		gEngfuncs.Con_DPrintf("LoadPhysicObjectConfigFromFiles: \"%s\" has been loaded successfully.\n", fullname_ragdoll.c_str());
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
		return SavePhysicObjectConfigToFile(Storage.modelname, Storage.pConfig.get());
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

	if (modelindex >= (int)m_physicObjectConfigs.size())
	{
		g_pMetaHookAPI->SysError("LoadPhysicObjectConfigForModel: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	auto& Storage = m_physicObjectConfigs[modelindex];

	if (Storage.state == PhysicConfigState_NotLoaded)
	{
		if (mod->type == mod_studio)
		{
			LoadPhysicObjectConfigFromFiles(mod, Storage);
		}
		else if (mod->type == mod_brush)
		{
			LoadPhysicObjectConfigFromBSP(mod, Storage);
		}
	}

	return Storage.pConfig;
}

std::shared_ptr<CClientPhysicObjectConfig> CBasePhysicManager::CreateEmptyPhysicObjectConfigForModelIndex(int modelindex, int PhysicObjectType)
{
	auto mod = EngineGetModelByIndex(modelindex);

	if (!mod)
	{
		g_pMetaHookAPI->SysError("CreateEmptyPhysicObjectConfigForModelIndex: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	if (modelindex >= (int)m_physicObjectConfigs.size())
	{
		g_pMetaHookAPI->SysError("CreateEmptyPhysicObjectConfigForModelIndex: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	auto& Storage = m_physicObjectConfigs[modelindex];

	if (!Storage.pConfig)
	{
		if (mod->type == mod_studio)
		{
			CreateEmptyPhysicObjectConfig(mod->name, Storage, PhysicObjectType);
		}
	}

	return Storage.pConfig;
}

std::shared_ptr<CClientPhysicObjectConfig> CBasePhysicManager::CreateEmptyPhysicObjectConfigForModel(model_t* mod, int PhysicObjectType)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex == -1)
	{
		g_pMetaHookAPI->SysError("CreateEmptyPhysicObjectConfigForModelIndex: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	if (modelindex >= (int)m_physicObjectConfigs.size())
	{
		g_pMetaHookAPI->SysError("CreateEmptyPhysicObjectConfigForModelIndex: Invalid model index %d!\n", modelindex);
		return nullptr;
	}

	auto& Storage = m_physicObjectConfigs[modelindex];

	if (Storage.state == PhysicConfigState_NotLoaded)
	{
		if (mod->type == mod_studio)
		{
			CreateEmptyPhysicObjectConfig(mod->name, Storage, PhysicObjectType);
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
	if (modelindex >= (int)m_physicObjectConfigs.size() || modelindex < 0)
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

			ClientEntityManager()->NotifyEntityModel(ent, model);

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

			ClientEntityManager()->NotifyEntityModel(ent, model);

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

			ClientEntityManager()->NotifyEntityModel(ent, model);

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

			ClientEntityManager()->NotifyEntityModel(ent, model);

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

		vec3_t vecSavedOrigin, vecSavedAngles, vecSavedCurStateOrigin, vecSavedCurStateAngles;
		VectorCopy((*currententity)->origin, vecSavedOrigin);
		VectorCopy((*currententity)->angles, vecSavedAngles);
		VectorCopy((*currententity)->curstate.origin, vecSavedCurStateOrigin);
		VectorCopy((*currententity)->curstate.angles, vecSavedCurStateAngles);

		auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

		int iSavedViewEntityIndex = 0;

		if (g_ViewEntityIndex_SCClient)
		{
			iSavedViewEntityIndex = (*g_ViewEntityIndex_SCClient);
			(*g_ViewEntityIndex_SCClient) = 0;
		}

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL_SETUP_BONES, &fakePlayerState);

		if (g_ViewEntityIndex_SCClient)
		{
			*g_ViewEntityIndex_SCClient = iSavedViewEntityIndex;
		}

		VectorCopy(vecSavedOrigin, (*currententity)->origin);
		VectorCopy(vecSavedAngles, (*currententity)->angles);
		VectorCopy(vecSavedCurStateOrigin, (*currententity)->curstate.origin);
		VectorCopy(vecSavedCurStateAngles, (*currententity)->curstate.angles);
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

void CBasePhysicManager::SetupBonesForRagdollEx(cl_entity_t* ent, entity_state_t *state, model_t* mod, int entindex, int playerindex, const CClientAnimControlConfig *pOverrideAnimControl)
{
	auto saved_currententity = (*currententity);
	(*currententity) = ent;

	if (playerindex > 0)
	{
		auto fakePlayerState = *state;

		fakePlayerState.number = playerindex;
		fakePlayerState.weaponmodel = 0;

		if (pOverrideAnimControl && pOverrideAnimControl->sequence >= 0)
		{
			fakePlayerState.sequence = pOverrideAnimControl->sequence;
			fakePlayerState.frame = pOverrideAnimControl->animframe;
			fakePlayerState.animtime = gEngfuncs.GetClientTime();
			fakePlayerState.framerate = 0;
			//fakePlayerState.movetype = MOVETYPE_NONE;
		}

		if (pOverrideAnimControl && pOverrideAnimControl->gaitsequence >= 0)
		{
			fakePlayerState.gaitsequence = pOverrideAnimControl->gaitsequence;
		}

#define COPY_BYTE_ENTSTATE(Attr, attr, to, i) if (pOverrideAnimControl && (pOverrideAnimControl->flags & AnimControlFlag_Override##Attr) && pOverrideAnimControl->attr[i] >= 0 && pOverrideAnimControl->attr[i] <= 255) to.attr[i] = pOverrideAnimControl->attr[i];
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 0);
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 1);
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 2);
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 3);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 0);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 1);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 2);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 3);
#undef COPY_BYTE_ENTSTATE

		vec3_t vecSavedOrigin, vecSavedAngles, vecSavedCurStateOrigin, vecSavedCurStateAngles;
		VectorCopy((*currententity)->origin, vecSavedOrigin);
		VectorCopy((*currententity)->angles, vecSavedAngles);
		VectorCopy((*currententity)->curstate.origin, vecSavedCurStateOrigin);
		VectorCopy((*currententity)->curstate.angles, vecSavedCurStateAngles);

		auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

		int iSavedViewEntityIndex = 0;

		if (g_ViewEntityIndex_SCClient)
		{
			iSavedViewEntityIndex = (*g_ViewEntityIndex_SCClient);
			(*g_ViewEntityIndex_SCClient) = 0;
		}

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL_SETUP_BONES, &fakePlayerState);

		if (g_ViewEntityIndex_SCClient)
		{
			*g_ViewEntityIndex_SCClient = iSavedViewEntityIndex;
		}

		VectorCopy(vecSavedOrigin, (*currententity)->origin);
		VectorCopy(vecSavedAngles, (*currententity)->angles);
		VectorCopy(vecSavedCurStateOrigin, (*currententity)->curstate.origin);
		VectorCopy(vecSavedCurStateAngles, (*currententity)->curstate.angles);
	}
	else
	{
		int iSavedWeaponModel = ent->curstate.weaponmodel;
		int iSavedSequence = ent->curstate.sequence;
		int iSavedGaitSequence = ent->curstate.gaitsequence;
		float flSavedFrame = ent->curstate.frame;
		byte ubSavedController[4];
		byte ubSavedBlending[4];

		memcpy(ubSavedController, ent->curstate.controller, sizeof(ubSavedController));
		memcpy(ubSavedBlending, ent->curstate.blending, sizeof(ubSavedBlending));

		ent->curstate.weaponmodel = 0;

		if (pOverrideAnimControl && pOverrideAnimControl->sequence >= 0)
		{
			ent->curstate.sequence = pOverrideAnimControl->sequence;
			ent->curstate.frame = pOverrideAnimControl->animframe;
			ent->curstate.animtime = gEngfuncs.GetClientTime();
			ent->curstate.framerate = 0;
			//ent->curstate.movetype = MOVETYPE_NONE;
		}

		if (pOverrideAnimControl && pOverrideAnimControl->gaitsequence >= 0)
		{
			ent->curstate.gaitsequence = pOverrideAnimControl->gaitsequence;
		}

#define COPY_BYTE_ENTSTATE(Attr, attr, to, i) if (pOverrideAnimControl && (pOverrideAnimControl->flags & AnimControlFlag_Override##Attr) && pOverrideAnimControl->attr[i] >= 0 && pOverrideAnimControl->attr[i] <= 255) to.attr[i] = pOverrideAnimControl->attr[i];
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 0);
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 1); 
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 2);
		COPY_BYTE_ENTSTATE(Controller, controller, ent->curstate, 3);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 0);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 1);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 2);
		COPY_BYTE_ENTSTATE(Blending, blending, ent->curstate, 3);
#undef COPY_BYTE_ENTSTATE

		(*gpStudioInterface)->StudioDrawModel(STUDIO_RAGDOLL_SETUP_BONES);

		memcpy(ent->curstate.controller, ubSavedController, sizeof(ubSavedController));
		memcpy(ent->curstate.blending, ubSavedBlending, sizeof(ubSavedBlending));

		ent->curstate.weaponmodel = iSavedWeaponModel;
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
		fakePlayerState.weaponmodel = 0;

		vec3_t vecSavedOrigin, vecSavedAngles, vecSavedCurStateOrigin, vecSavedCurStateAngles;
		VectorCopy((*currententity)->origin, vecSavedOrigin);
		VectorCopy((*currententity)->angles, vecSavedAngles);
		VectorCopy((*currententity)->curstate.origin, vecSavedCurStateOrigin);
		VectorCopy((*currententity)->curstate.angles, vecSavedCurStateAngles);

		auto pLocalPlayer = gEngfuncs.GetLocalPlayer();

		int iSavedViewEntityIndex = 0;

		if (g_ViewEntityIndex_SCClient)
		{
			iSavedViewEntityIndex = (*g_ViewEntityIndex_SCClient);
			(*g_ViewEntityIndex_SCClient) = 0;
		}

		(*gpStudioInterface)->StudioDrawPlayer(STUDIO_RAGDOLL_UPDATE_BONES, &fakePlayerState);

		if (g_ViewEntityIndex_SCClient)
		{
			*g_ViewEntityIndex_SCClient = iSavedViewEntityIndex;
		}

		VectorCopy(vecSavedOrigin, (*currententity)->origin);
		VectorCopy(vecSavedAngles, (*currententity)->angles);
		VectorCopy(vecSavedCurStateOrigin, (*currententity)->curstate.origin);
		VectorCopy(vecSavedCurStateAngles, (*currententity)->curstate.angles);
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
			if (pPhysicObject->IsRagdollObject())
			{
				auto pRagdollObject = (IRagdollObject*)pPhysicObject;

				StudioAnimActivityType iNewActivityType{ StudioAnimActivityType_Idle };
				int iNewAnimControlFlags{};

				if (!pRagdollObject->CalculateOverrideActivityType(pRagdollObject->GetClientEntityState(), &iNewActivityType, &iNewAnimControlFlags))
				{
					StudioGetActivityType(pRagdollObject->GetModel(), pRagdollObject->GetClientEntityState(), &iNewActivityType, &iNewAnimControlFlags);
				}

				if (iNewActivityType == StudioAnimActivityType_BarnaclePulling || 
					iNewActivityType == StudioAnimActivityType_BarnacleChewing)
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
			if (pPhysicObject->IsRagdollObject())
			{
				auto pRagdollObject = (IRagdollObject*)pPhysicObject;

				StudioAnimActivityType iNewActivityType{ StudioAnimActivityType_Idle };
				int iNewAnimControlFlags{};

				if (!pRagdollObject->CalculateOverrideActivityType(pRagdollObject->GetClientEntityState(), &iNewActivityType, &iNewAnimControlFlags))
				{
					StudioGetActivityType(pRagdollObject->GetModel(), pRagdollObject->GetClientEntityState(), &iNewActivityType, &iNewAnimControlFlags);
				}

				if (iNewActivityType == StudioAnimActivityType_GargantuaBite)
				{
					vec3_t vecClientEntityOrigin;
					VectorCopy(pPhysicObject->GetClientEntity()->origin, vecClientEntityOrigin);

					if (VectorDistance(playerState->origin, vecClientEntityOrigin) < 128 && pPhysicObject->GetClientEntityState()->sequence == 15)
					{
						return pPhysicObject;
					}
				}
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

const CPhysicDebugDrawContext* CBasePhysicManager::GetDebugDrawContext() const
{
	return &m_debugDrawContext;
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
		
		CPhysicObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_entstate = state;
		CreationParam.m_entindex = entindex;
		CreationParam.m_model = mod;

		if (mod->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(ent, mod);
		}

		CreationParam.m_playerindex = playerindex;

		CreationParam.m_pPhysicObjectConfig = pRagdollObjectConfig;

		LoadAdditionalResourcesForConfig(pRagdollObjectConfig);

		auto pRagdollObject = CreateRagdollObject(CreationParam);

		if (!pRagdollObject)
			return;

		if (pRagdollObject->Build(CreationParam))
		{
			AddPhysicObject(entindex, pRagdollObject);
		}
		else
		{
			pRagdollObject->Destroy();
		}
	}
	else if (pPhysicConfig->type == PhysicObjectType_DynamicObject)
	{
		auto pDynamicObjectConfig = (CClientDynamicObjectConfig*)pPhysicConfig.get();

		CPhysicObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_entstate = state;
		CreationParam.m_entindex = entindex;
		CreationParam.m_model = mod;

		if (mod->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(ent, mod);
		}

		CreationParam.m_playerindex = playerindex;

		CreationParam.m_pPhysicObjectConfig = pDynamicObjectConfig;

		LoadAdditionalResourcesForConfig(pDynamicObjectConfig);

		auto pDynamicObject = CreateDynamicObject(CreationParam);

		if (!pDynamicObject)
			return;

		if (pDynamicObject->Build(CreationParam))
		{
			AddPhysicObject(entindex, pDynamicObject);
		}
		else
		{
			pDynamicObject->Destroy();
		}
	}
	else if (pPhysicConfig->type == PhysicObjectType_StaticObject)
	{
		auto pStaticObjectConfig = (CClientStaticObjectConfig*)pPhysicConfig.get();

		CPhysicObjectCreationParameter CreationParam;

		CreationParam.m_entity = ent;
		CreationParam.m_entstate = state;
		CreationParam.m_entindex = entindex;
		CreationParam.m_model = mod;

		if (mod->type == mod_studio)
		{
			CreationParam.m_studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(mod);
			CreationParam.m_model_scaling = ClientEntityManager()->GetEntityModelScaling(ent, mod);
		}

		CreationParam.m_playerindex = playerindex;

		CreationParam.m_pPhysicObjectConfig = pStaticObjectConfig;

		LoadAdditionalResourcesForConfig(pStaticObjectConfig);

		auto pStaticObject = CreateStaticObject(CreationParam);

		if (!pStaticObject)
			return;

		if (pStaticObject->Build(CreationParam))
		{
			AddPhysicObject(entindex, pStaticObject);
		}
		else
		{
			pStaticObject->Destroy();
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("CreatePhysicObjectFromConfig: Unsupported config type (%d).\n", pPhysicConfig->type);
	}
}

void CBasePhysicManager::CreatePhysicObjectForBrushModel(cl_entity_t* ent, entity_state_t* state, model_t* mod)
{
	ClientEntityManager()->NotifyEntityModel(ent, mod);

	auto entindex = ClientEntityManager()->GetEntityIndex(ent);

	auto pPhysicObject = GetPhysicObject(entindex);

	if (pPhysicObject && pPhysicObject->GetModel() == mod)
		return;

	auto pPhysicConfig = LoadPhysicObjectConfigForModel(mod);

	if (!pPhysicConfig)
		return;

	if (pPhysicConfig->type != PhysicObjectType_StaticObject)
		return;

	CPhysicObjectCreationParameter CreationParam;
	CreationParam.m_entity = ent;
	CreationParam.m_entstate = state;
	CreationParam.m_entindex = entindex;
	CreationParam.m_model = mod;
	CreationParam.m_pPhysicObjectConfig = pPhysicConfig.get();

	auto pStaticObject = CreateStaticObject(CreationParam);

	if (!pStaticObject)
		return;

	if (pStaticObject->Build(CreationParam))
	{
		AddPhysicObject(entindex, pStaticObject);
	}
	else
	{
		pStaticObject->Destroy();
	}
}

void CBasePhysicManager::AddPhysicObject(int entindex, IPhysicObject* pPhysicObject)
{
	RemovePhysicObject(entindex);

	m_physicObjects[entindex] = pPhysicObject;
}

bool CBasePhysicManager::RemovePhysicObject(int entindex)
{
	auto itor = m_physicObjects.find(entindex);

	if (itor != m_physicObjects.end())
	{
		auto pPhysicObject = itor->second;

		pPhysicObject->Destroy();

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
			pPhysicObject->Destroy();

			itor = m_physicObjects.erase(itor);
			continue;
		}

		itor++;
	}
}

void CBasePhysicManager::UpdateAllPhysicObjects(TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, double frame_time, double client_time, double cl_gravity)
{
	if (frame_time <= 0)
		return;

	for (auto itor = m_physicObjects.begin(); itor != m_physicObjects.end();)
	{
		auto entindex = itor->first;
		auto pPhysicObject = itor->second;

		CPhysicObjectUpdateContext ObjectUpdateContext;

		ObjectUpdateContext.m_flGravity = cl_gravity;

		if (!ObjectUpdateContext.m_bShouldFree)
		{
			//world entity is always present
			if (entindex > 0 && !ClientEntityManager()->IsEntityEmitted(entindex))
			{
				ObjectUpdateContext.m_bShouldFree = true;
			}
		}

		if (!ObjectUpdateContext.m_bShouldFree)
		{
			pPhysicObject->Update(&ObjectUpdateContext);
		}

		if (ObjectUpdateContext.m_bShouldFree)
		{
			pPhysicObject->Destroy();

			itor = m_physicObjects.erase(itor);
			continue;
		}

		itor++;
	}

}

template<class T, class T2>
void CBasePhysicManager::BuildSurfaceDisplayList(model_t* mod, T *fa, std::deque<glpoly_t*>& glpolys)
{
#define BLOCK_WIDTH 128
#define BLOCK_HEIGHT 128
	int i, lindex, lnumverts;
	medge_t* pedges, * r_pedge;
	float* vec;
	float s, t;
	glpoly_t* poly;

	pedges = mod->edges;
	lnumverts = fa->numedges;

	int allocSize = (int)sizeof(glpoly_t) + ((lnumverts - 4) * VERTEXSIZE * sizeof(float));

	if (allocSize < 0)
		return;

	poly = (glpoly_t*)malloc(allocSize);

	glpolys.push_front(poly);

	poly->next = NULL;
	poly->flags = fa->flags;
	//fa->polys = poly;
	poly->numverts = lnumverts;
	poly->chain = NULL;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = mod->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = mod->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = mod->vertexes[r_pedge->v[1]].position;
		}

		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		poly->verts[i][0] = vec[0];
		poly->verts[i][1] = vec[1];
		poly->verts[i][2] = vec[2];
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;
#if 0
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16;
#endif
		poly->verts[i][5] = 0;
		poly->verts[i][6] = 0;
	}

	poly->numverts = lnumverts;
}

template<class T, class T2>
std::shared_ptr<CPhysicVertexArray> CBasePhysicManager::GenerateWorldVertexArrayInternal(model_t* mod)
{
	auto worldVertexArray = std::make_shared<CPhysicVertexArray>();

	CPhysicBrushVertex Vertexes[3];

	int iNumFaces = 0;
	int iNumVerts = 0;

	worldVertexArray->vFaceBuffer.resize(mod->numsurfaces);

	for (int i = 0; i < mod->numsurfaces; i++)
	{
		auto surf = GetWorldSurfaceByIndex<T>(i);

		if ((surf->flags & (SURF_DRAWTURB | SURF_UNDERWATER | SURF_DRAWSKY)))
			continue;

		std::deque<glpoly_t*> glpolys;

		BuildSurfaceDisplayList<T, T2>(mod, surf, glpolys);

		if (glpolys.empty())
			continue;

		auto brushface = &worldVertexArray->vFaceBuffer[i];

		int iStartVert = iNumVerts;

		brushface->start_vertex = iStartVert;

		for (const auto& poly : glpolys)
		{
			auto v = poly->verts[0];

			for (int j = 0; j < 3; j++, v += VERTEXSIZE)
			{
				Vertexes[j].pos[0] = v[0];
				Vertexes[j].pos[1] = v[1];
				Vertexes[j].pos[2] = v[2];

				Vec3GoldSrcToBullet(Vertexes[j].pos);
			}

			worldVertexArray->vVertexBuffer.emplace_back(Vertexes[0]);
			worldVertexArray->vVertexBuffer.emplace_back(Vertexes[1]);
			worldVertexArray->vVertexBuffer.emplace_back(Vertexes[2]);

			iNumVerts += 3;

			for (int j = 0; j < (poly->numverts - 3); j++, v += VERTEXSIZE)
			{
				Vertexes[1] = Vertexes[2];

				Vertexes[2].pos[0] = v[0];
				Vertexes[2].pos[1] = v[1];
				Vertexes[2].pos[2] = v[2];

				Vec3GoldSrcToBullet(Vertexes[2].pos);

				worldVertexArray->vVertexBuffer.emplace_back(Vertexes[0]);
				worldVertexArray->vVertexBuffer.emplace_back(Vertexes[1]);
				worldVertexArray->vVertexBuffer.emplace_back(Vertexes[2]);

				iNumVerts += 3;
			}
		}

		for (auto& poly : glpolys)
		{
			free(poly);
		}

		brushface->num_vertexes = iNumVerts - iStartVert;
	}

	//Always shrink to save system memory
	worldVertexArray->vVertexBuffer.shrink_to_fit();

	return worldVertexArray;
}

std::shared_ptr<CPhysicVertexArray> CBasePhysicManager::GenerateWorldVertexArray(model_t *mod)
{
	std::string worldModelName;

	if (mod->name[0] == '*')
	{
		auto worldmodel = EngineFindWorldModelBySubModel(mod);

		if (!worldmodel)
		{
			gEngfuncs.Con_Printf("CBasePhysicManager::GenerateWorldVertexArray: Failed to find worldmodel for submodel \"%s\"!\n", mod->name);
			return nullptr;
		}

		worldModelName = worldmodel->name;
	}
	else
	{
		worldModelName = mod->name;
	}

	auto found = m_worldVertexResources.find(worldModelName);

	if (found != m_worldVertexResources.end())
	{
		return found->second;
	}

	std::shared_ptr<CPhysicVertexArray> worldVertexArray;

	if (g_dwVideoMode == VIDEOMODE_SOFTWARE)
	{
		worldVertexArray = GenerateWorldVertexArrayInternal<msurface_sw_t, mnode_sw_t>(mod);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		worldVertexArray = GenerateWorldVertexArrayInternal<msurface_hl25_t, mnode_t>(mod);
	}
	else
	{
		worldVertexArray = GenerateWorldVertexArrayInternal<msurface_t, mnode_t>(mod);
	}

	m_worldVertexResources[worldModelName] = worldVertexArray;

	return worldVertexArray;
}

/*
	Purpose : Generate IndexArray for world and all brush models
*/

std::shared_ptr<CPhysicIndexArray> CBasePhysicManager::GenerateBrushIndexArray(model_t* mod, const std::shared_ptr<CPhysicVertexArray>& pWorldVertexArray)
{
	auto pIndexArray = std::make_shared<CPhysicIndexArray>();
	pIndexArray->flags |= PhysicIndexArrayFlag_FromBSP;
	pIndexArray->pVertexArray = pWorldVertexArray;

	if (g_dwVideoMode == VIDEOMODE_SOFTWARE)
	{
		GenerateIndexArrayForBrushModel<msurface_sw_t, mnode_sw_t>(mod, pIndexArray.get());
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		GenerateIndexArrayForBrushModel<msurface_hl25_t, mnode_t>(mod, pIndexArray.get());
	}
	else
	{
		GenerateIndexArrayForBrushModel<msurface_t, mnode_t>(mod, pIndexArray.get());
	}

	auto name = UTIL_GetAbsoluteModelName(mod);

	m_indexArrayResources[name] = pIndexArray;

	return pIndexArray;
}

void CBasePhysicManager::FreeAllIndexArrays(int withflags, int withoutflags)
{
	//m_brushIndexArray.clear();
	for (auto itor = m_indexArrayResources.begin(); itor != m_indexArrayResources.end(); )
	{
		const auto& pIndexArray = itor->second;

		if ((pIndexArray->flags & withflags) && !(pIndexArray->flags & withoutflags))
		{
			itor = m_indexArrayResources.erase(itor);
			continue;
		}

		itor++;
	}
}

template<class T, class T2>
void CBasePhysicManager::GenerateIndexArrayForBrushModel(model_t* mod, CPhysicIndexArray* pIndexArray)
{
	if (mod == (*cl_worldmodel))
	{
		GenerateIndexArrayRecursiveWorldNode<T, T2>(mod, (T2 *)mod->nodes, pIndexArray);
	}
	else
	{
		for (int i = 0; i < mod->nummodelsurfaces; i++)
		{
			auto surf = GetWorldSurfaceByIndex<T>(mod->firstmodelsurface + i);

			GenerateIndexArrayForSurface<T>(mod, surf, pIndexArray);
		}
	}

	//Always shrink to save system memory
	pIndexArray->vIndexBuffer.shrink_to_fit();
}

template<class T>
void CBasePhysicManager::GenerateIndexArrayForSurface(model_t* mod, T* surf, CPhysicIndexArray* pIndexArray)
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

	auto surfIndex = GetWorldSurfaceIndex<T>(surf);

	GenerateIndexArrayForBrushface(&pIndexArray->pVertexArray->vFaceBuffer[surfIndex], pIndexArray);
}

template<class T, class T2>
void CBasePhysicManager::GenerateIndexArrayRecursiveWorldNode(model_t* mod, T2* node, CPhysicIndexArray* pIndexArray)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents < 0)
		return;

	GenerateIndexArrayRecursiveWorldNode<T, T2>(mod, node->children[0], pIndexArray);

	for (int i = 0; i < node->numsurfaces; ++i)
	{
		auto surf = GetWorldSurfaceByIndex<T>(node->firstsurface + i);

		GenerateIndexArrayForSurface<T>(mod, surf, pIndexArray);
	}

	GenerateIndexArrayRecursiveWorldNode<T, T2>(mod, node->children[1], pIndexArray);
}

void CBasePhysicManager::GenerateIndexArrayForBrushface(CPhysicBrushFace* brushface, CPhysicIndexArray* pIndexArray)
{
	int first = -1;
	int prv0 = -1;
	int prv1 = -1;
	int prv2 = -1;

	for (int i = 0; i < brushface->num_vertexes; i++)
	{
		if (prv0 != -1 && prv1 != -1 && prv2 != -1)
		{
			pIndexArray->vIndexBuffer.emplace_back(brushface->start_vertex + first);
			pIndexArray->vIndexBuffer.emplace_back(brushface->start_vertex + prv2);
		}

		pIndexArray->vIndexBuffer.emplace_back(brushface->start_vertex + i);

		if (first == -1)
			first = i;

		prv0 = prv1;
		prv1 = prv2;
		prv2 = i;
	}

	pIndexArray->vFaceBuffer.emplace_back(*brushface);
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

	SaveToObjFile("models/gargmouth.obj", objData);
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

#if 0
std::shared_ptr<CPhysicIndexArray> CBasePhysicManager::GetIndexArrayFromBrushModel(model_t* mod)
{
	int modelindex = EngineGetModelIndex(mod);

	if (modelindex < 0 || modelindex >(int)m_brushIndexArray.size())
	{
		return NULL;
	}

	return m_brushIndexArray[modelindex];
}
#endif

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

bool CBasePhysicManager::LoadObjToPhysicArrays(const std::string& resourcePath, std::shared_ptr<CPhysicIndexArray> &pIndexArray)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	CFileSystemStream fileStream(resourcePath);

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
		gEngfuncs.Con_DPrintf("LoadObjToPhysicArrays: Failed to load \"%s\".\n", resourcePath.c_str());
		return false;
	}

	auto &pVertexArray = pIndexArray->pVertexArray;

	for (size_t i = 0;i < attrib.vertices.size(); i += 3)
	{
		CPhysicBrushVertex vertex;

		vertex.pos[0] = attrib.vertices[0 + i];
		vertex.pos[1] = attrib.vertices[1 + i];
		vertex.pos[2] = attrib.vertices[2 + i];

		Vec3GoldSrcToBullet(vertex.pos);

		pVertexArray->vVertexBuffer.push_back(vertex);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			pIndexArray->vIndexBuffer.push_back(index.vertex_index);
		}

		//I'm not sure if this works or not but this won't affect the trimesh anyway
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int fv = shape.mesh.num_face_vertices[f];
			CPhysicBrushFace face;
			face.start_vertex = index_offset;
			face.num_vertexes = fv;
			pVertexArray->vFaceBuffer.push_back(face);

			index_offset += fv;
		}
	}

	return true;
}

std::shared_ptr<CPhysicIndexArray> CBasePhysicManager::LoadIndexArrayFromResource(const std::string& resourcePath)
{
	auto found = m_indexArrayResources.find(resourcePath);

	if (found != m_indexArrayResources.end())
	{
		return found->second;
	}

	const auto extension = V_GetFileExtension(resourcePath.c_str());

	if (0 == stricmp(extension, "obj"))
	{
		auto pVertexArray = std::make_shared<CPhysicVertexArray>();

		auto pIndexArray = std::make_shared<CPhysicIndexArray>();

		pIndexArray->flags |= PhysicIndexArrayFlag_FromOBJ;

		pIndexArray->pVertexArray = pVertexArray;

		if(!LoadObjToPhysicArrays(resourcePath, pIndexArray))
			pIndexArray->flags |= PhysicIndexArrayFlag_LoadFailed;

		m_indexArrayResources[resourcePath] = pIndexArray;

		return pIndexArray;
	}

	gEngfuncs.Con_DPrintf("LoadIndexArrayFromResource: Could not load \"%s\", unsupported file extension!\n", resourcePath.c_str());
	return nullptr;
}

void CBasePhysicManager::LoadAdditionalResourcesForCollisionShapeConfig(CClientCollisionShapeConfig *pCollisionShapeConfig)
{
	if (pCollisionShapeConfig->type == PhysicShape_TriangleMesh && pCollisionShapeConfig->resourcePath.size() > 0)
	{
		LoadIndexArrayFromResource(pCollisionShapeConfig->resourcePath);
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
