#pragma once

#include "plugins.h"
#include <usercmd.h>
#include <com_model.h>
#include <studio.h>
#include <pm_defs.h>
#include <tier0/basetypes.h>
#include <cvardef.h>
#include "enginedef.h"

#define GetCallAddress(addr) (addr + (*(DWORD *)((addr)+1)) + 5)
#define Sig_NotFound(name) Sys_ErrorEx("Could not found: %s\nEngine buildnum£º%d", #name, g_dwEngineBuildnum);
#define Sig_FuncNotFound(name) if(!gCapFuncs.name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)
#define SIG_NOT_FOUND(name) Sys_ErrorEx("Could not found: %s\nEngine buildnum£º%d", name, g_dwEngineBuildnum);

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig) g_pMetaHookAPI->SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, sig, Sig_Length(sig));
#define Search_Pattern_From(func, sig) g_pMetaHookAPI->SearchPattern((void *)gCapFuncs.func, g_dwEngineSize - (DWORD)gCapFuncs.func + g_dwEngineBase, sig, Sig_Length(sig));
#define InstallHook(func) g_pMetaHookAPI->InlineHook((void *)gCapFuncs.func, func, (void *&)gCapFuncs.func);

typedef struct
{	
	void (*S_Init)(void);
	sfx_t *(*S_FindName)(char *name, int *pfInCache);//hooked
	void (*S_StartDynamicSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
	void (*S_StartStaticSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
	sfxcache_t *(*S_LoadSound)(sfx_t *s, channel_t *ch);
	void *(**pfnClientFactory)(void);
	FARPROC (WINAPI *GetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
	double *pcl_time;
	double *pcl_oldtime;
	char szLanguage[32];
}cap_funcs_t;

#define cl_time (*gCapFuncs.pcl_time)
#define cl_oldtime (*gCapFuncs.pcl_oldtime)

extern cap_funcs_t gCapFuncs;