#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <com_model.h>
#include "cl_entity.h"

extern cl_enginefunc_t gEngfuncs;
extern cl_exportfuncs_t gExportfuncs;
extern engine_studio_api_t IEngineStudio;

extern model_t* r_worldmodel;
extern cl_entity_t* r_worldentity;

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model);
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio);
void HUD_TempEntUpdate(
	double frametime,   // Simulation time
	double client_time, // Absolute time on client
	double cl_gravity,  // True gravity on client
	TEMPENTITY **ppTempEntFree,   // List of freed temporary ents
	TEMPENTITY **ppTempEntActive, // List 
	int(*Callback_AddVisibleEntity)(cl_entity_t *pEntity),
	void(*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp));
void HUD_DrawNormalTriangles(void);
void HUD_Init(void);
void V_CalcRefdef(struct ref_params_s *pparams);
void HUD_Frame(double frametime);
void HUD_Shutdown(void);
void HUD_CreateEntities(void);

msurface_t* GetWorldSurfaceByIndex(int index);
int GetWorldSurfaceIndex(msurface_t* surf);