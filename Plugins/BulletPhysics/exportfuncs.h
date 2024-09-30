#include <metahook.h>
#include <const.h>
#include <studio.h>
#include <r_studioint.h>
#include <com_model.h>
#include <cl_entity.h>
#include <triangleapi.h>
#include <event_api.h>
#include <pm_defs.h>
#include <cvardef.h>

extern cl_enginefunc_t gEngfuncs;
extern cl_exportfuncs_t gExportfuncs;
extern engine_studio_api_t IEngineStudio;
extern r_studio_interface_t** gpStudioInterface;

extern model_t* r_worldmodel;
extern cl_entity_t* r_worldentity;

extern int* cl_max_edicts;
extern cl_entity_t** cl_entities;

extern int* cl_numvisedicts;
extern cl_entity_t** cl_visedicts;

extern float* g_ChromeOrigin;
extern float* r_origin;

extern bool g_bIsCounterStrike;

extern bool g_bIsUpdatingRefdef;

extern cvar_t* bv_debug_draw_level_ragdoll;
extern cvar_t* bv_debug_draw_level_static;
extern cvar_t* bv_debug_draw_level_dynamic;
extern cvar_t* bv_debug_draw_level_rigidbody;
extern cvar_t* bv_debug_draw_level_constraint;
extern cvar_t* bv_debug_draw_level_behavior;
extern cvar_t* bv_debug_draw_constraint_color;
extern cvar_t* bv_debug_draw_behavior_color;
extern cvar_t* bv_debug_draw_inspected_color;
extern cvar_t* bv_debug_draw_selected_color;

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
void HUD_Frame(double frametime);
void HUD_Shutdown(void);
void HUD_CreateEntities(void);
void V_CalcRefdef(struct ref_params_s* pparams);

msurface_t* GetWorldSurfaceByIndex(int index);
int GetWorldSurfaceIndex(msurface_t* surf);

entity_state_t* R_GetPlayerState(int index);

void V_RenderView(void);

qboolean R_CullBox(vec3_t mins, vec3_t maxs);

bool AllowCheats();

bool IsDebugDrawEnabled();
bool IsDebugDrawWallHackEnabled();
bool ShouldSyncronizeView();
bool ShouldForceUpdateBones();

float GetSimulationTickRate();

bool R_IsRenderingPortals();

float* EngineGetRendererViewOrigin();
int EngineGetNumKnownModel();
int EngineGetMaxKnownModel();
int EngineGetModelIndex(model_t* mod);
model_t* EngineGetModelByIndex(int index);

int EngineGetMaxClientEdicts(void);
cl_entity_t* EngineGetClientEntitiesBase(void);
int EngineGetMaxTempEnts(void);

#define OBS_SVEN_NONE				0
#define OBS_SVEN_CHASE_FREE			1
#define OBS_SVEN_ROAMING			2
#define OBS_SVEN_CHASE_LOCKED		3

void BV_ReloadObjects_f(void);
void BV_ReloadConfigs_f(void);
void BV_ReloadAll_f(void);
void BV_SaveConfigs_f(void);