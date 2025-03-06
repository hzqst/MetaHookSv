#include "gl_local.h"
#include "pm_defs.h"
#include <vector>
#include <map>
#include <algorithm>

std::vector<CEntityComponentContainer*> g_ClientEntityRenderComponents;
std::vector<CEntityComponentContainer*> g_TempEntityRenderComponents;

//For cl_viewent and client.dll VoiceStatus bakamono.
std::map<void *, CEntityComponentContainer*> g_UnmanagedEntityRenderComponent;

void R_InitEntityComponents(void)
{
	g_ClientEntityRenderComponents.clear();
	g_TempEntityRenderComponents.clear();
	g_UnmanagedEntityRenderComponent.clear();
}

void R_ShutdownEntityComponents(void)
{
	for (auto itor = g_ClientEntityRenderComponents.begin(); itor != g_ClientEntityRenderComponents.end(); itor++)
	{
		auto pContainer = (*itor);

		if (pContainer)
		{
			delete pContainer;
		}
	}
	g_ClientEntityRenderComponents.clear();

	for (auto itor = g_TempEntityRenderComponents.begin(); itor != g_TempEntityRenderComponents.end(); itor++)
	{
		auto pContainer = (*itor);

		if (pContainer)
		{
			delete pContainer;
		}
	}
	g_TempEntityRenderComponents.clear();

	for (auto itor = g_UnmanagedEntityRenderComponent.begin(); itor != g_UnmanagedEntityRenderComponent.end(); itor++)
	{
		auto pContainer = (*itor).second;

		if (pContainer)
		{
			delete pContainer;
		}
	}
	g_UnmanagedEntityRenderComponent.clear();
}

CEntityComponentContainer *R_AllocateEntityComponentContainer(void)
{
	return new CEntityComponentContainer();
}

void R_EntityComponents_StartFrame(void)
{
	for (auto itor = g_ClientEntityRenderComponents.begin(); itor != g_ClientEntityRenderComponents.end(); itor++)
	{
		auto pContainer = (*itor);

		if (pContainer)
		{
			pContainer->Reset();
		}
	}
	for (auto itor = g_TempEntityRenderComponents.begin(); itor != g_TempEntityRenderComponents.end(); itor++)
	{
		auto pContainer = (*itor);

		if (pContainer)
		{
			pContainer->Reset();
		}
	}
	for (auto itor = g_UnmanagedEntityRenderComponent.begin(); itor != g_UnmanagedEntityRenderComponent.end(); itor++)
	{
		auto pContainer = (*itor).second;

		if (pContainer)
		{
			pContainer->Reset();
		}
	}
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

	//TODO: unordered_map ?

	if (!pContainer)
	{
		int index = R_GetClientEntityIndex(ent);

		if (index >= 0)
		{
			if ((int)g_ClientEntityRenderComponents.size() < index + 1)
			{
				g_ClientEntityRenderComponents.resize((size_t)index + 1);
			}

			pContainer = g_ClientEntityRenderComponents[(size_t)index];

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
			if ((int)g_TempEntityRenderComponents.size() < index + 1)
			{
				g_TempEntityRenderComponents.resize((size_t)index + 1);
			}

			pContainer = g_TempEntityRenderComponents[(size_t)index];

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