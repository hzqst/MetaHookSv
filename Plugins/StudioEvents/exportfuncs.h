#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

void HUD_Init(void);
void HUD_StudioEvent(const struct mstudioevent_s *ev, const struct cl_entity_s *ent);
void HUD_Frame(double clientTime);