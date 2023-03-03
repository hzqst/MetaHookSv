#include "gl_local.h"
#include <algorithm>

#define MAX_ENTITY_COMPONENTS 1024

entity_component_t gEntityComponentPool[MAX_ENTITY_COMPONENTS];

entity_component_t *gpEntityComponentActive = NULL;
entity_component_t *gpEntityComponentFree = NULL;

std::vector<entity_component_t *> g_ClientEntityRenderComponents;
std::vector<entity_component_t *> g_TempEntityRenderComponents;

void R_InitEntityComponents(void)
{
	for (int i = 0; i < MAX_ENTITY_COMPONENTS; i++)
	{
		gEntityComponentPool[i].next = &gEntityComponentPool[i + 1];
		gEntityComponentPool[i].FollowEnts.clear();
		gEntityComponentPool[i].Decals.clear();
		gEntityComponentPool[i].WaterVBOs.clear();
		gEntityComponentPool[i].ReflectCaches.clear();
	}

	gEntityComponentPool[MAX_TEMP_ENTITIES - 1].next = NULL;
	gpEntityComponentFree = &gEntityComponentPool[0];
	gpEntityComponentActive = NULL;

	g_ClientEntityRenderComponents.clear();
	g_TempEntityRenderComponents.clear();
}

void R_ShutdownEntityComponents(void)
{
	R_InitEntityComponents();
}

entity_component_t *R_AllocateEntityComponent(void)
{
	if (!gpEntityComponentFree)
	{
		gEngfuncs.Con_DPrintf("Overflow entity component!\n");
		return NULL;
	}

	auto pTemp = gpEntityComponentFree;
	gpEntityComponentFree = pTemp->next;

	pTemp->next = gpEntityComponentActive;
	gpEntityComponentActive = pTemp;

	return pTemp;
}

void R_EntityComponents_StartFrame(void)
{
	auto p = gpEntityComponentActive;
	while (p)
	{
		p->FollowEnts.clear();
		p->Decals.clear();
		p->WaterVBOs.clear();
		p->ReflectCaches.clear();
		
		auto temp = p->next;

		p->next = gpEntityComponentFree;
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

int R_GetClientEntityComponentIndex(cl_entity_t *ent)
{
	if (ent >= EngineGetClientEntitiesBase() && ent < EngineGetClientEntitiesBase() + EngineGetMaxClientEdicts())
	{
		return ent - EngineGetClientEntitiesBase();
	}

	return -1;
}

int R_GetTempEntityComponentIndex(cl_entity_t *ent)
{
	if (ent >= &EngineGetTempTentsBase()->entity && ent < &EngineGetTempTentByIndex(EngineGetMaxTempEnts())->entity)
	{
		auto tent = STRUCT_FROM_LINK(ent, TEMPENTITY, entity);

		return tent - EngineGetTempTentsBase();
	}

	return -1;
}

entity_component_t *R_GetEntityComponent(cl_entity_t *ent, bool create_if_not_exists)
{
	entity_component_t * comp = NULL;

	int index = R_GetClientEntityComponentIndex(ent);

	if (index >= 0)
	{
		if ((int)g_ClientEntityRenderComponents.size() < index + 1)
		{
			g_ClientEntityRenderComponents.resize((size_t)index + 1);
		}
		comp = g_ClientEntityRenderComponents[(size_t)index];
		if (!comp && create_if_not_exists)
		{
			comp = R_AllocateEntityComponent();
			g_ClientEntityRenderComponents[(size_t)index] = comp;
		}
	}
	else
	{
		index = R_GetTempEntityComponentIndex(ent);

		if (index >= 0)
		{
			if ((int)g_TempEntityRenderComponents.size() < index + 1)
			{
				g_TempEntityRenderComponents.resize((size_t)index + 1);
			}

			comp = g_TempEntityRenderComponents[(size_t)index];
			if (!comp && create_if_not_exists)
			{
				comp = R_AllocateEntityComponent();
				g_TempEntityRenderComponents[(size_t)index] = comp;
			}
		}
		else
		{
			//g_pMetaHookAPI->SysError("R_AllocateEntityComponent: invalid ent 0x%p !", ent);
			//return R_GetEntityComponent(r_worldentity, create_if_not_exists);
		}
	}

	return comp;
}