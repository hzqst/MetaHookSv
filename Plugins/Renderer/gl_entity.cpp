#include "gl_local.h"
#include "pm_defs.h"
#include <algorithm>

std::vector<CEntityComponentContainer*> g_ClientEntityRenderComponents;
std::vector<CEntityComponentContainer*> g_TempEntityRenderComponents;
CEntityComponentContainer* g_ViewEntityRenderComponent = NULL;

void R_InitEntityComponents(void)
{
	g_ClientEntityRenderComponents.clear();
	g_TempEntityRenderComponents.clear();
	g_ViewEntityRenderComponent = nullptr;
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

	if (g_ViewEntityRenderComponent)
	{
		delete g_ViewEntityRenderComponent;
		g_ViewEntityRenderComponent = nullptr;
	}
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
	if (g_ViewEntityRenderComponent)
	{
		g_ViewEntityRenderComponent->Reset();
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

static_assert(sizeof(TEMPENTITY) == 3068, "Size Check");

TEMPENTITY *EngineGetTempTentsBase(void)
{
	return gTempEnts;
}

TEMPENTITY *EngineGetTempTentByIndex(int index)
{
	return &gTempEnts[index];
}

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

	if (ent == cl_viewent)
	{
		pContainer = g_ViewEntityRenderComponent;

		if (!pContainer && create_if_not_exists)
		{
			pContainer = R_AllocateEntityComponentContainer();
			g_ViewEntityRenderComponent = pContainer;
		}
	}

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