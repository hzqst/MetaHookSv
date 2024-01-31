#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

void IN_ActivateMouse(void);
void HUD_Init(void);
void HUD_StudioEvent(const struct mstudioevent_s *ev, const struct cl_entity_s *ent);
void HUD_Frame(double time);
void HUD_Shutdown(void);