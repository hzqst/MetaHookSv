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

#define GetCallAddress(addr) (addr + (*(DWORD *)((addr)+1)) + 5)
#define Sig_NotFound(name) Sys_ErrorEx("Could not found: %s\nEngine buildnum£º%d", #name, g_dwEngineBuildnum);
#define Sig_FuncNotFound(name) if(!gRefFuncs.name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)
#define SIG_NOT_FOUND(name) Sys_ErrorEx("Could not found: %s\nEngine buildnum£º%d", name, g_dwEngineBuildnum);

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig) g_pMetaHookAPI->SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, sig, Sig_Length(sig));
#define Search_Pattern_From(fn, sig) g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.fn, g_dwEngineSize - (DWORD)gRefFuncs.fn + g_dwEngineBase, sig, Sig_Length(sig));
#define InstallHook(fn) g_pMetaHookAPI->InlineHook((void *)gRefFuncs.fn, fn, (void *&)gRefFuncs.fn);