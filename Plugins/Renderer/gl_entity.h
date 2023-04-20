#pragma once

typedef struct water_vbo_s water_vbo_t;

typedef struct entity_component_s
{
	struct entity_component_s *next;
	std::vector <cl_entity_t *> FollowEnts;
	std::vector<decal_t *> Decals;
	std::vector<water_vbo_t *> WaterVBOs;
	std::vector<water_reflect_cache_t *> ReflectCaches;
}entity_component_t;

entity_component_t *R_GetEntityComponent(cl_entity_t *ent, bool create_if_not_exists);
void R_InitEntityComponents(void);
void R_ShutdownEntityComponents(void);
void R_EntityComponents_StartFrame(void);