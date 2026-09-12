#pragma once

extern mh_interface_t *g_pInterface;
extern metahook_api_t *g_pMetaHookAPI;
extern mh_enginesave_t *g_pMetaSave;
extern IFileSystem *g_pFileSystem;
extern IFileSystem_HL25* g_pFileSystem_HL25;

extern int g_iEngineType;
extern DWORD g_dwEngineBuildnum;
extern DWORD g_dwVideoMode;

extern mh_dll_info_t g_EngineDLLInfo;
extern mh_dll_info_t g_ClientDLLInfo;

#define MHPluginName "BulletPhysics"
#define Sys_Error(msg, ...) g_pMetaHookAPI->SysError("["  MHPluginName   "] " msg, __VA_ARGS__);

#define Install_InlineHook(fn) if(!g_phook_##fn) { g_phook_##fn = g_pMetaHookAPI->InlineHook((void *)gPrivateFuncs.fn, fn, (void **)&gPrivateFuncs.fn); }
#define Uninstall_Hook(fn) if(g_phook_##fn){g_pMetaHookAPI->UnHook(g_phook_##fn);g_phook_##fn = NULL;}
