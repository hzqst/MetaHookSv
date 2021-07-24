#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

extern cl_enginefunc_t gEngfuncs;
extern cl_exportfuncs_t gExportfuncs;
extern engine_studio_api_t IEngineStudio;

void R_NewMap(void);
int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion);
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
void HUD_DrawTransparentTriangles(void);
void HUD_Init(void);
void V_CalcRefdef(struct ref_params_s *pparams);

void Sys_ErrorEx(const char *fmt, ...);