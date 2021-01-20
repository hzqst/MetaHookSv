class IFileSystem;

extern HINSTANCE g_hInstance, g_hThisModule, g_hEngineModule;
extern DWORD g_dwEngineBase, g_dwEngineSize;
extern DWORD g_dwEngineBuildnum;
extern DWORD g_iVideoMode;
extern bool g_bIsNewEngine;
extern int g_iVideoWidth, g_iVideoHeight, g_iBPP;
extern bool g_bWindowed;
extern bool g_bIsDebuggerPresent;
extern IFileSystem *g_pFileSystem;