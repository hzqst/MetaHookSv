#pragma once

extern IFileSystem *g_pFileSystem;
extern mh_interface_t *g_pInterface;
extern metahook_api_t *g_pMetaHookAPI;
extern mh_enginesave_t *g_pMetaSave;
extern IFileSystem *g_pFileSystem;
extern BOOL g_IsClientVGUI2;
extern HMODULE g_hClientDll;
extern DWORD g_dwClientSize;
extern int g_iVideoWidth;
extern int g_iVideoHeight;
extern PVOID g_dwEngineBase;
extern DWORD g_dwEngineSize;
extern PVOID g_dwEngineTextBase;
extern DWORD g_dwEngineTextSize;
extern PVOID g_dwEngineDataBase;
extern DWORD g_dwEngineDataSize;
extern PVOID g_dwEngineRdataBase;
extern DWORD g_dwEngineRdataSize;
extern DWORD g_dwEngineBuildnum;
extern int g_iEngineType;

#define GetCallAddress(addr) ((PUCHAR)addr + *(int *)((PUCHAR)addr + 1) + 5)
#define Sig_NotFound(name) Sys_ErrorEx("Could not found: %s\nEngine buildnum£º%d", #name, g_dwEngineBuildnum);
#define Sig_VarNotFound(name) if(!name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)
#define Sig_FuncNotFound(name) if(!gCapFuncs.name) Sig_NotFound(name)

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig) g_pMetaHookAPI->SearchPattern(g_dwEngineTextBase, g_dwEngineTextSize, sig, Sig_Length(sig))
#define Search_Pattern_Data(sig) g_pMetaHookAPI->SearchPattern(g_dwEngineDataBase, g_dwEngineDataSize, sig, Sig_Length(sig))
#define Search_Pattern_Rdata(sig) g_pMetaHookAPI->SearchPattern(g_dwEngineRdataBase, g_dwEngineRdataSize, sig, Sig_Length(sig))
#define Search_Pattern_From(fn, sig) g_pMetaHookAPI->SearchPattern((void *)fn, ((PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize) - (PUCHAR)fn, sig, Sig_Length(sig))
#define Install_InlineHook(fn) g_pMetaHookAPI->InlineHook((void *)gCapFuncs.fn, fn, (void *&)gCapFuncs.fn)