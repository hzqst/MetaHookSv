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

typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);

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
	hook_t *(*IATHook)(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void **pOrginalCall);
	void *(*GetClassFuncAddr)(...);
	PVOID (*GetModuleBase)(PVOID VirtualAddress);
	DWORD (*GetModuleSize)(PVOID ModuleBase);
	HMODULE (*GetEngineModule)(void);
	PVOID (*GetEngineBase)(void);
	DWORD (*GetEngineSize)(void);
	void *(*SearchPattern)(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);
	/*
		Search pattern (signature) in given region
	*/

	void (*WriteDWORD)(void *pAddress, DWORD dwValue);
	DWORD (*ReadDWORD)(void *pAddress);
	DWORD (*WriteMemory)(void *pAddress, BYTE *pData, DWORD dwDataSize);
	DWORD (*ReadMemory)(void *pAddress, BYTE *pData, DWORD dwDataSize);
	DWORD (*GetVideoMode)(int *width, int *height, int *bpp, bool *windowed);
	
	DWORD (*GetEngineBuildnum)(void);
	/*
		Get buildnum of loaded engine.
	*/

	CreateInterfaceFn (*GetEngineFactory)(void);
	void *(*GetNextCallAddr)(void *pAddress, DWORD dwCount);
	void (*WriteBYTE)(void *pAddress, BYTE ucValue);
	BYTE (*ReadBYTE)(void *pAddress);
	void (*WriteNOP)(void *pAddress, DWORD dwCount);

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
	pfnUserMsgHook(*HookUserMsg)(const char *szMsgName, pfnUserMsgHook pfn);

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