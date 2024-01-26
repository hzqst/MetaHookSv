#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

void HUD_Frame(double time);
void HUD_Init(void);
void HUD_Shutdown(void);
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio);
void R_StudioChangePlayerModel(void);