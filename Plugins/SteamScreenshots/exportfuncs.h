#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

extern cl_enginefunc_t gEngfuncs;
extern cl_exportfuncs_t gExportfuncs;
extern mh_interface_t *g_pInterface;
extern metahook_api_t *g_pMetaHookAPI;
extern mh_enginesave_t *g_pMetaSave;
extern IFileSystem *g_pFileSystem;
extern DWORD g_dwEngineBuildnum;

void IN_ActivateMouse(void);
void HUD_Init(void);
void HUD_StudioEvent(const struct mstudioevent_s *ev, const struct cl_entity_s *ent);
void HUD_Frame(double time);

void Sys_ErrorEx(const char *fmt, ...);

#define GetCallAddress(addr) (addr + (*(int *)((addr)+1)) + 5)
#define Sig_NotFound(name) Sys_ErrorEx("Could not found: %s\nEngine buildnum£º%d", #name, g_dwEngineBuildnum);
#define Sig_FuncNotFound(name) if(!gRefFuncs.name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)
#define SIG_NOT_FOUND(name) Sys_ErrorEx("Could not found: %s\nEngine buildnum£º%d", name, g_dwEngineBuildnum);

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig) g_pMetaHookAPI->SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, sig, Sig_Length(sig));
#define Search_Pattern_From(fn, sig) g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.fn, g_dwEngineSize - (DWORD)gRefFuncs.fn + g_dwEngineBase, sig, Sig_Length(sig));
#define InstallHook(fn) g_pMetaHookAPI->InlineHook((void *)gRefFuncs.fn, fn, (void *&)gRefFuncs.fn);