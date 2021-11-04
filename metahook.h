#ifndef _METAHOOK_H
#define _METAHOOK_H

#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#include <stdio.h>

typedef float vec_t;
typedef float vec2_t[2];
typedef float vec3_t[3];

#include <wrect.h>
#include <interface.h>

typedef struct cvar_s cvar_t;

typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);

#define HOOK_MESSAGE(x) g_pMetaHookAPI->HookUserMsg(#x, __MsgFunc_##x);

typedef void(*cvar_callback_t)(cvar_t *pcvar);

typedef void(*xcommand_t)(void);

typedef struct mh_plugininfo_s
{
	int Index;
	const char *PluginName;
	const char *PluginPath;
	const char *PluginVersion;
	int InterfaceVersion;
	void *PluginModuleBase;
	size_t PluginModuleSize;
}mh_plugininfo_t;

#include <cdll_export.h>
#include <cdll_int.h>

typedef struct hook_s hook_t;

#define VIDEOMODE_SOFTWARE 0
#define VIDEOMODE_OPENGL 1
#define VIDEOMODE_D3D 2

#define ENGINE_UNKNOWN 0
#define ENGINE_GOLDSRC_BLOB 1
#define ENGINE_GOLDSRC 2
#define ENGINE_SVENGINE 3

typedef void (*DisasmSingleCallback)(void *inst, PUCHAR address, size_t instLen, PVOID context);
typedef BOOL (*DisasmCallback)(void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context);

typedef struct metahook_api_s
{
	BOOL (*UnHook)(hook_t *pHook);
	hook_t *(*InlineHook)(void *pOldFuncAddr, void *pNewFuncAddr, void **pOrginalCall);
	/*
		Install JMP hook at begin of function,
		Warning : this hook operation will be delayed until LoadEngine or LoadClient from all plugins called, if it is requseted from inside LoadEngine or LoadClient.
		otherwise the hook operation is executed immediately.
	*/

	hook_t *(*VFTHook)(void *pClass, int iTableIndex, int iFuncIndex, void *pNewFuncAddr, void **pOrginalCall);
	/*
		Install VFT hook
		This will modify the Virtual Function Table
	*/

	hook_t *(*IATHook)(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void **pOrginalCall);
	/*
		Install IAT hook
		This will modify the Import Address Table
	*/

	void *(*GetClassFuncAddr)(...);
	/*
		Unused
	*/

	PVOID (*GetModuleBase)(PVOID VirtualAddress);
	/*
		Query module base of given virtual address
	*/

	DWORD (*GetModuleSize)(PVOID ModuleBase);
	/*
		Query module size of given module base
	*/

	HMODULE (*GetEngineModule)(void);
	/*
		Query module handle of engine dll
		Could be null if BlobEngine is loaded
	*/

	PVOID (*GetEngineBase)(void);
	/*
		Query module base of engine dll
	*/

	DWORD (*GetEngineSize)(void);
	/*
		Query module size of engine dll
	*/

	void *(*SearchPattern)(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);
	/*
		Search pattern (signature) in given region
	*/

	void (*WriteDWORD)(void *pAddress, DWORD dwValue);
	/*
		Write 4bytes value at given address, ignoring page protection
	*/
	
	DWORD (*ReadDWORD)(void *pAddress);
	/*
		Read 4bytes from given address
	*/

	DWORD (*WriteMemory)(void *pAddress, void *pData, DWORD dwDataSize);
	/*
		Write memory at given address, ignoring page protection
	*/

	DWORD (*ReadMemory)(void *pAddress, void *pData, DWORD dwDataSize);
	/*
		Read memory from given address
	*/

	DWORD (*GetVideoMode)(int *width, int *height, int *bpp, bool *windowed);
	/*
		Get VideoMode, VideoWidth, VideoHeight, BitPerPixel (16 or 24), WindowedMode
	*/

	DWORD (*GetEngineBuildnum)(void);
	/*
		Get buildnum of loaded engine.
	*/

	CreateInterfaceFn (*GetEngineFactory)(void);
	/*
		Get factory interface (CreateInterface) of engine dll
	*/

	void *(*GetNextCallAddr)(void *pAddress, DWORD dwCount);
	/*
		Get the branch target of 0xE8 jmp instruction at given address
	*/

	void (*WriteBYTE)(void *pAddress, BYTE ucValue);
	/*
		Write 1 byte value at given address, ignoring page protection
	*/

	BYTE (*ReadBYTE)(void *pAddress);
	/*
		Read 1 byte from given address
	*/

	void (*WriteNOP)(void *pAddress, DWORD dwCount);
	/*
		Write 0x90 (x86 nop) at given address, ignoring page protection
	*/

	int (*GetEngineType)(void);
	/*
		Return one of them :  ENGINE_UNKNOWN, ENGINE_GOLDSRC_BLOB, ENGINE_GOLDSRC, ENGINE_SVENGINE
	*/

	const char *(*GetEngineTypeName)(void);
	/*
		Return one of them : "Unknown", "GoldSrc_Blob", "GoldSrc", "SvEngine"
	*/

	PVOID(*ReverseSearchFunctionBegin)(PVOID SearchBegin, DWORD SearchSize);
	/*
		Reverse search from given base to lower address, find 90 90 90 99 + ??, or CC CC CC CC + ??
	*/

	PVOID(*GetSectionByName)(PVOID ImageBase, const char *SectionName, ULONG *SectionSize);
	/*
		Return the section base and section size of given image and section name.
	*/

	int (*DisasmSingleInstruction)(PVOID address, DisasmSingleCallback callback, void *context);
	/*
		Disassemble a instruction at given address, return result inside callback
	*/

	BOOL (*DisasmRanges)(PVOID DisasmBase, SIZE_T DisasmSize, DisasmCallback callback, int depth, PVOID context);
	/*
		Disassemble instructions at given range of address, return result inside callback
	*/

	void *(*ReverseSearchPattern)(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);
	/*
		Search pattern (signature) from pStartSearch in reverse direction
	*/

	HMODULE	(*GetClientModule)(void);
	/*
		Get module handle of client dll
		Could be null if SecureClient is loaded
	*/

	PVOID (*GetClientBase)(void);
	/*
		Get module base of client dll
	*/

	DWORD (*GetClientSize)(void);
	/*
		Get module size of client dll
	*/

	CreateInterfaceFn(*GetClientFactory)(void);
	/*
		Get factory interface (CreateInterface) of client dll
	*/

	BOOL (*QueryPluginInfo)(int fromindex, mh_plugininfo_t *info);
	/*
		Query information of all loaded plugins.

		Usage:

		mh_plugininfo_t info;
		for(int index = -1; g_pMetaHookAPI->QueryPluginInfo(index, &info); index = info.Index)
		{

		}
	*/
	BOOL (*GetPluginInfo)(const char *name, mh_plugininfo_t *info);
	/*
		Get information of specified plugin

		Usage:

		mh_plugininfo_t info;
		if(g_pMetaHookAPI->GetPluginInfo("PluginName.dll", &info))//"PluginName.dll" is case-insensitive
		{

		}
	*/
	pfnUserMsgHook (*HookUserMsg)(const char *szMsgName, pfnUserMsgHook pfn);
	/*
		Find registered UserMsg, and set it's pfnHook to specified function pointer, return original function pointer if exists.
	*/

	cvar_callback_t (*HookCvarCallback)(const char *cvar_name, cvar_callback_t callback);
	/*
		Find existing cvar, and set it's Cvar-Set callback to specified function pointer, return original callback function pointer if exists.
	*/

	xcommand_t(*HookCmd)(const char *cmd_name, xcommand_t newfuncs);
	/*
		Find existing console command, and set it's command callback to specified function pointer, return original callback function pointer.
	*/

	void (*SysError)(const char *fmt, ...);
	/*
		Show error msgbox and terminate game process.
	*/
}
metahook_api_t;

typedef struct mh_enginesave_s
{
	cl_exportfuncs_t *pExportFuncs;
	cl_enginefunc_t *pEngineFuncs;
}
mh_enginesave_t;

void MH_FreeAllHook(void);
void MH_LoadPlugins(const char *gamedir);
void MH_LoadEngine(HMODULE hModule, const char *szGameName);
void MH_ExitGame(int iResult);
void MH_Shutdown(void);

#include <IFileSystem.h>
#include <ICommandLine.h>
#include <IRegistry.h>

typedef struct mh_interface_s
{
	ICommandLine *CommandLine;
	IFileSystem *FileSystem;
	IRegistry *Registry;
}
mh_interface_t;

#include <IPlugins.h>

extern mh_interface_t *g_pInterface;
extern cl_enginefunc_t gEngfuncs;
extern metahook_api_t *g_pMetaHookAPI;
extern mh_enginesave_t *g_pMetaSave;

#endif