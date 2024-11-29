#include "gl_local.h"
#include <algorithm>

#define MAX_ENTITY_COMPONENTS 1024

CEntityComponentContainer gEntityComponentPool[MAX_ENTITY_COMPONENTS];

CEntityComponentContainer*gpEntityComponentActive = NULL;
CEntityComponentContainer*gpEntityComponentFree = NULL;

std::vector<CEntityComponentContainer*> g_ClientEntityRenderComponents;
std::vector<CEntityComponentContainer*> g_TempEntityRenderComponents;

void R_InitEntityComponents(void)
{
	for (int i = 0; i < MAX_ENTITY_COMPONENTS; i++)
	{
		gEntityComponentPool[i].pNext = &gEntityComponentPool[i + 1];
		gEntityComponentPool[i].FollowEnts.clear();
		gEntityComponentPool[i].Decals.clear();
		gEntityComponentPool[i].WaterVBOs.clear();
		gEntityComponentPool[i].ReflectCaches.clear();
		gEntityComponentPool[i].DeferredStudioPasses.clear();
	}
	//???
	gEntityComponentPool[MAX_ENTITY_COMPONENTS - 1].pNext = NULL;
	gpEntityComponentFree = &gEntityComponentPool[0];
	gpEntityComponentActive = NULL;

	g_ClientEntityRenderComponents.clear();
	g_TempEntityRenderComponents.clear();
}

void R_ShutdownEntityComponents(void)
{
	R_InitEntityComponents();
}

CEntityComponentContainer *R_AllocateEntityComponentContainer(void)
{
	if (!gpEntityComponentFree)
	{
		gEngfuncs.Con_DPrintf("Overflow entity component container!\n");
		return NULL;
	}

	auto pTemp = gpEntityComponentFree;
	gpEntityComponentFree = pTemp->pNext;

	pTemp->pNext = gpEntityComponentActive;
	gpEntityComponentActive = pTemp;

	return pTemp;
}

void R_EntityComponents_PreFrame(void)
{
	auto p = gpEntityComponentActive;
	while (p)
	{
		p->FollowEnts.clear();
		p->Decals.clear();
		p->WaterVBOs.clear();
		p->ReflectCaches.clear();
		p->DeferredStudioPasses.clear();

		auto temp = p->pNext;

		p->pNext = gpEntityComponentFree;
		gpEntityComponentFree = p;

		p = temp;
	}
	gpEntityComponentActive = NULL;
	
	g_ClientEntityRenderComponents.clear();
	g_TempEntityRenderComponents.clear();
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
	if (ent >= &EngineGetTempTentsBase()->entity && ent < &EngineGetTempTentByIndex(EngineGetMaxTempEnts())->entity)
	{
		auto tent = STRUCT_FROM_LINK(ent, TEMPENTITY, entity);

		return tent - EngineGetTempTentsBase();
	}

	return -1;
}

CEntityComponentContainer * R_GetEntityComponentContainer(cl_entity_t *ent, bool create_if_not_exists)
{
	CEntityComponentContainer* pContainer = NULL;

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
	else
	{
		index = R_GetTempEntityIndex(ent);

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