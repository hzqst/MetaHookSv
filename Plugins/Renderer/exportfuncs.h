#pragma once

void Memory_Init(void);

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion);
void HUD_Init(void);
int HUD_VidInit(void);
void HUD_Reset(void);
void V_CalcRefdef(struct ref_params_s *pparams);
void HUD_DrawNormalTriangles(void);
void HUD_DrawTransparentTriangles(void);
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio);
int HUD_UpdateClientData(client_data_t *pcldata, float flTime);
void HUD_Shutdown(void);
int HUD_AddEntity(int type, cl_entity_t *ent, const char *model);
int HUD_Redraw(float time, int intermission);
void HUD_Frame(double time);

void Sys_ErrorEx(const char *fmt, ...);
char *UTIL_VarArgs(char *format, ...);
void hudGetMousePos(struct tagPOINT *ppt);

void hudGetMousePosition(int *x, int *y);