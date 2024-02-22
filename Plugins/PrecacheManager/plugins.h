#pragma once

#include <metahook.h>

class IFileSystem;

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
extern IFileSystem *g_pFileSystem;
extern PVOID g_dwClientBase;
extern DWORD g_dwClientSize;

#define MHPluginName "PrecacheManager"
#define Sys_Error(msg, ...) g_pMetaHookAPI->SysError("["  MHPluginName   "] " msg, __VA_ARGS__);
#define Sig_NotFound(name) Sys_Error("Could not found: %s\nEngine buildnum: %d", #name, g_dwEngineBuildnum);
#define Sig_VarNotFound(name) if(!name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)
#define Sig_FuncNotFound(name) if(!gPrivateFuncs.name) Sig_NotFound(name)

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig) g_pMetaHookAPI->SearchPattern(g_dwEngineTextBase, g_dwEngineTextSize, sig, Sig_Length(sig))
#define Search_Pattern_Data(sig) g_pMetaHookAPI->SearchPattern(g_dwEngineDataBase, g_dwEngineDataSize, sig, Sig_Length(sig))
#define Search_Pattern_Rdata(sig) g_pMetaHookAPI->SearchPattern(g_dwEngineRdataBase, g_dwEngineRdataSize, sig, Sig_Length(sig))
#define Search_Pattern_From_Size(fn, size, sig) g_pMetaHookAPI->SearchPattern((void *)(fn), size, sig, Sig_Length(sig))
#define Search_Pattern_From(fn, sig) g_pMetaHookAPI->SearchPattern((void *)(fn), ((PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize) - (PUCHAR)(fn), sig, Sig_Length(sig))
#define Install_InlineHook(fn) if(!g_phook_##fn) { g_phook_##fn = g_pMetaHookAPI->InlineHook((void *)gPrivateFuncs.fn, fn, (void **)&gPrivateFuncs.fn); }
#define Uninstall_Hook(fn) if(g_phook_##fn){g_pMetaHookAPI->UnHook(g_phook_##fn);g_phook_##fn = NULL;}
#define GetCallAddress(addr) g_pMetaHookAPI->GetNextCallAddr((PUCHAR)addr, 1)