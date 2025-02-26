#pragma once

class IFileSystem;
class IFileSystem_HL25;

extern IFileSystem* g_pFileSystem;
extern IFileSystem_HL25* g_pFileSystem_HL25;

extern int g_iEngineType;
extern mh_dll_info_t g_EngineDLLInfo;
extern mh_dll_info_t g_MirroredEngineDLLInfo;
extern mh_dll_info_t g_ClientDLLInfo;
extern DWORD g_dwEngineBuildnum;

#define MHPluginName "Renderer"
#define Sys_Error(msg, ...) g_pMetaHookAPI->SysError("["  MHPluginName   "] " msg, __VA_ARGS__);
#define Sig_NotFound(name) Sys_Error("Could not found: %s\nEngine buildnum: %d", #name, g_dwEngineBuildnum);
#define Sig_VarNotFound(name) if(!name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)
#define Sig_FuncNotFound(name) if(!gPrivateFuncs.name) Sig_NotFound(name)

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig, dllinfo) g_pMetaHookAPI->SearchPattern(dllinfo.TextBase, dllinfo.TextSize, sig, Sig_Length(sig))
#define Search_Pattern_Data(sig, dllinfo) g_pMetaHookAPI->SearchPattern(dllinfo.DataBase, dllinfo.DataSize, sig, Sig_Length(sig))
#define Search_Pattern_Rdata(sig, dllinfo) g_pMetaHookAPI->SearchPattern(dllinfo.RdataBase, dllinfo.RdataSize, sig, Sig_Length(sig))
#define Search_Pattern_From_Size(fn, size, sig) g_pMetaHookAPI->SearchPattern((void *)(fn), size, sig, Sig_Length(sig))
#define Search_Pattern_From(fn, sig, dllinfo) g_pMetaHookAPI->SearchPattern((void *)(fn), ((PUCHAR)dllinfo.TextBase + dllinfo.TextSize) - (PUCHAR)(fn), sig, Sig_Length(sig))

#define Search_Pattern_NoWildCard(sig, dllinfo) g_pMetaHookAPI->SearchPatternNoWildCard(dllinfo.TextBase, dllinfo.TextSize, sig, Sig_Length(sig))
#define Search_Pattern_NoWildCard_Data(sig, dllinfo) g_pMetaHookAPI->SearchPatternNoWildCard(dllinfo.DataBase, dllinfo.DataSize, sig, Sig_Length(sig))
#define Search_Pattern_NoWildCard_Rdata(sig, dllinfo) g_pMetaHookAPI->SearchPatternNoWildCard(dllinfo.RdataBase, dllinfo.RdataSize, sig, Sig_Length(sig))

#define Install_InlineHook(fn) if(!g_phook_##fn) { g_phook_##fn = g_pMetaHookAPI->InlineHook((void *)gPrivateFuncs.fn, fn, (void **)&gPrivateFuncs.fn); }
#define Uninstall_Hook(fn) if(g_phook_##fn){g_pMetaHookAPI->UnHook(g_phook_##fn);g_phook_##fn = NULL;}
#define GetCallAddress(addr) g_pMetaHookAPI->GetNextCallAddr((PUCHAR)addr, 1)