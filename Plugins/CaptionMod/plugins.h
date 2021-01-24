#pragma once

extern DWORD g_dwEngineBase, g_dwEngineSize;
extern DWORD g_dwEngineBuildnum;
extern DWORD g_iVideoMode;
extern int g_iVideoWidth, g_iVideoHeight, g_iBPP;
extern int g_EngineType;
extern bool g_bWindowed;
extern IFileSystem *g_pFileSystem;
extern HMODULE g_hThisModule, g_hEngineModule;
extern bool g_bIsUseSteam;
extern bool g_bIsRunningSteam;
extern HMODULE g_hClientDll;
extern DWORD g_dwClientSize;
extern BOOL g_IsClientVGUI2;