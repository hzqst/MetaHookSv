#pragma once

void R_Version_f(void);
void HUD_Init(void);
int HUD_VidInit(void);
void V_CalcRefdef(struct ref_params_s *pparams);
void HUD_DrawNormalTriangles(void);
void HUD_DrawTransparentTriangles(void);
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio);
void HUD_Shutdown(void);
int HUD_AddEntity(int type, cl_entity_t *ent, const char *model);
int HUD_Redraw(float time, int intermission);
void HUD_Frame(double time);
void HUD_PlayerMoveInit(struct playermove_s* ppmove);
