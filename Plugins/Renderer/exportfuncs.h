#pragma once

void R_Version_f(void);
void HUD_Init(void);
int HUD_VidInit(void);
void V_CalcRefdef(struct ref_params_s *pparams);
void HUD_DrawTransparentTriangles(void);
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio);
void HUD_Shutdown(void);
int HUD_AddEntity(int type, cl_entity_t *ent, const char *model);
int HUD_Redraw(float time, int intermission);
void HUD_Frame(double time);
void HUD_CreateEntities(void);
void HUD_PlayerMoveInit(struct playermove_s* ppmove);
void HUD_OnClientDisconnect(void);

#define DLIGHT_KEY_PLAYER_BRIGHTLIGHT 0x40000
#define DLIGHT_KEY_PLAYER_FLASHLIGHT 0x80000
#define DLIGHT_KEY_LOCAL_PLAYER_FLASHLIGHT 1

enum StudioAnimActivityType
{
	StudioAnimActivityType_Idle,
	StudioAnimActivityType_Death,
	StudioAnimActivityType_CaughtByBarnacle,
	StudioAnimActivityType_BarnaclePulling,
	StudioAnimActivityType_BarnacleChewing,
	StudioAnimActivityType_GargantuaBite,
	StudioAnimActivityType_Debug,
	StudioAnimActivityType_Maximum,
};

const int AnimControlFlag_OverrideAllBones = 0x1;
const int AnimControlFlag_OverrideController = 0x2;
const int AnimControlFlag_OverrideBlending = 0x4;