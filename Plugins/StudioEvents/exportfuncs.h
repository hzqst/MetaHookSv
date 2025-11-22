#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

void HUD_Init(void);
int HUD_VidInit(void);
void HUD_Frame(double clientTime);
void HUD_StudioEvent(const struct mstudioevent_s *ev, const struct cl_entity_s *ent);
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio);