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
	PVOID (*GetModuleBase)(HMODULE hModule);
	DWORD (*GetModuleSize)(HMODULE hModule);
	HMODULE (*GetEngineModule)(void);
	PVOID (*GetEngineBase)(void);
	DWORD (*GetEngineSize)(void);
	void *(*SearchPattern)(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);
	void (*WriteDWORD)(void *pAddress, DWORD dwValue);
	DWORD (*ReadDWORD)(void *pAddress);
	DWORD (*WriteMemory)(void *pAddress, BYTE *pData, DWORD dwDataSize);
	DWORD (*ReadMemory)(void *pAddress, BYTE *pData, DWORD dwDataSize);
	DWORD (*GetVideoMode)(int *width, int *height, int *bpp, bool *windowed);
	DWORD (*GetEngineBuildnum)(void);
	CreateInterfaceFn (*GetEngineFactory)(void);
	DWORD (*GetNextCallAddr)(void *pAddress, DWORD dwCount);
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

	BOOL (*DisasmRanges)(PVOID DisasmBase, SIZE_T DisasmSize, DisasmCallback callback, int depth, PVOID context);


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