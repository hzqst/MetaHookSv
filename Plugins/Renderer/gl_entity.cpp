#include "gl_local.h"
#include "pm_defs.h"
#include <vector>
#include <map>
#include <algorithm>

/*
	The active components in current frame, every active components will stale at frame start.
*/
std::vector<CEntityComponentContainer*> g_ActiveEntityRenderComponents;

/*
	Stale components .
*/
std::vector<CEntityComponentContainer*> g_StaleEntityRenderComponents;

/*
	The entindex -> component mapping.
*/
std::vector<CEntityComponentContainer*> g_ClientEntityRenderComponents;

/*
	The tempent_index -> component mapping.
*/
std::vector<CEntityComponentContainer*> g_TempEntityRenderComponents;

/*
	For cl_viewent and client.dll VoiceStatus fake entity -> component mapping.
*/
std::map<void *, CEntityComponentContainer*> g_UnmanagedEntityRenderComponent;

int EngineGetMaxClientEdicts(void);
int EngineGetMaxTempEnts(void);

void R_InitEntityComponents(void)
{
	g_ActiveEntityRenderComponents.reserve(64);
	g_StaleEntityRenderComponents.reserve(64);

	auto maxClientEdicts = EngineGetMaxClientEdicts();
	if (maxClientEdicts > 0)
	{
		g_ClientEntityRenderComponents.reserve((size_t)maxClientEdicts);
	}

	auto maxTempEnts = EngineGetMaxTempEnts();
	if (maxTempEnts > 0)
	{
		g_TempEntityRenderComponents.reserve((size_t)maxTempEnts);
	}
}

void R_ShutdownEntityComponents(void)
{
	for (auto it : g_ActiveEntityRenderComponents)
	{
		delete it;
	}
	g_ActiveEntityRenderComponents.clear();

	for (auto it : g_StaleEntityRenderComponents)
	{
		delete it;
	}
	g_StaleEntityRenderComponents.clear();

	g_ClientEntityRenderComponents.clear();
	g_TempEntityRenderComponents.clear();
	g_UnmanagedEntityRenderComponent.clear();
}

CEntityComponentContainer *R_AllocateEntityComponentContainer(void)
{
	CEntityComponentContainer* p{};

	if (g_StaleEntityRenderComponents.size() > 0)
	{
		p = g_StaleEntityRenderComponents[g_StaleEntityRenderComponents.size() - 1];

		g_StaleEntityRenderComponents.pop_back();

		p->Reset();

		g_ActiveEntityRenderComponents.emplace_back(p);

		return p;
	}

	p = new CEntityComponentContainer();

	g_ActiveEntityRenderComponents.emplace_back(p);

	return p;
}

void R_EntityComponents_StartFrame(void)
{
	g_StaleEntityRenderComponents.insert(g_StaleEntityRenderComponents.end(), g_ActiveEntityRenderComponents.begin(), g_ActiveEntityRenderComponents.end());
	g_ActiveEntityRenderComponents.clear();

	std::fill(g_ClientEntityRenderComponents.begin(), g_ClientEntityRenderComponents.end(), nullptr);
	std::fill(g_TempEntityRenderComponents.begin(), g_TempEntityRenderComponents.end(), nullptr);
	g_UnmanagedEntityRenderComponent.clear();
}

int EngineGetMaxClientEdicts(void)
{
	return (*cl_max_edicts);
}

cl_entity_t *EngineGetClientEntitiesBase(void)
{
	return (*cl_entities);
}

int EngineGetMaxTempEnts(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
		return MAX_TEMP_ENTITIES_SVENGINE;

	return MAX_TEMP_ENTITIES;
}

int EngineGetMaxVisEdicts(void)
{
	if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
		return MAX_VISEDICTS_10152;

	return MAX_VISEDICTS;
}

TEMPENTITY* EngineGetTempTentsBase(void)
{
	return gTempEnts;
}

TEMPENTITY* EngineGetTempTentByIndex(int index)
{
	return &gTempEnts[index];
}

static_assert(sizeof(TEMPENTITY) == 3068, "Size Check");

int R_GetClientEntityIndex(cl_entity_t *ent)
{
	if (ent >= EngineGetClientEntitiesBase() && ent < EngineGetClientEntitiesBase() + EngineGetMaxClientEdicts())
	{
		return ent - EngineGetClientEntitiesBase();
	}

	return -1;
}

int R_GetTempEntityIndex(cl_entity_t *ent)
{
	auto tempEntBase = EngineGetTempTentsBase();
	auto tempEntEnd = EngineGetTempTentByIndex(EngineGetMaxTempEnts());

	if (ent >= &tempEntBase->entity && ent < &tempEntEnd->entity)
	{
		auto tent = STRUCT_FROM_LINK(ent, TEMPENTITY, entity);

		return tent - tempEntBase;
	}

	return -1;
}

CEntityComponentContainer * R_GetEntityComponentContainer(cl_entity_t *ent, bool create_if_not_exists)
{
	CEntityComponentContainer* pContainer = NULL;

	if (ent == r_worldentity)
	{
		ent = gEngfuncs.GetEntityByIndex(0);
	}

	if (!pContainer)
	{
		int index = R_GetClientEntityIndex(ent);

		if (index > 0)
		{
			if ((size_t)index < g_ClientEntityRenderComponents.size())
			{
				pContainer = g_ClientEntityRenderComponents[(size_t)index];
			}
			else if (create_if_not_exists)
			{
				g_ClientEntityRenderComponents.resize((size_t)index + 1);
			}

			if (!pContainer && create_if_not_exists)
			{
				pContainer = R_AllocateEntityComponentContainer();

				g_ClientEntityRenderComponents[(size_t)index] = pContainer;
			}
		}
	}

	if (!pContainer)
	{
		int index = R_GetTempEntityIndex(ent);

		if (index >= 0)
		{
			if ((size_t)index < g_TempEntityRenderComponents.size())
			{
				pContainer = g_TempEntityRenderComponents[(size_t)index];
			}
			else if (create_if_not_exists)
			{
				g_TempEntityRenderComponents.resize((size_t)index + 1);
			}

			if (!pContainer && create_if_not_exists)
			{
				pContainer = R_AllocateEntityComponentContainer();

				g_TempEntityRenderComponents[(size_t)index] = pContainer;
			}
		}
	}

	if (!pContainer)
	{
		auto itor = g_UnmanagedEntityRenderComponent.find(ent);

		if (itor != g_UnmanagedEntityRenderComponent.end())
		{
			pContainer = (*itor).second;
		}

		if (!pContainer && create_if_not_exists)
		{
			pContainer = R_AllocateEntityComponentContainer();

			g_UnmanagedEntityRenderComponent[ent] = pContainer;
		}
	}

	return pContainer;
}

void R_AllocateEntityComponentsForVisEdicts()
{
	for (int i = 0; i < (*cl_numvisedicts); ++i)
	{
		auto ent = cl_visedicts[i];

		if (ent && ent->model)
		{
			R_GetEntityComponentContainer(ent, true);
		}
	}
}

int EngineFindPhysEntIndexByEntity(cl_entity_t* ent)
{
	if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
	{
		for (int i = 0; i < pmove_10152->numphysent; ++i)
		{
			if (pmove_10152->physents[i].info == ent->index)
				return i;
		}
	}
	else
	{
		for (int i = 0; i < pmove->numphysent; ++i)
		{
			if (pmove->physents[i].info == ent->index)
				return i;
		}
	}

	return -1;
}

bool EngineIsEntityInVisibleList(cl_entity_t* ent)
{
	for (int i = 0; i < (*cl_numvisedicts); ++i)
	{
		if (cl_visedicts[i] == ent)
			return true;
	}

	return false;
}
