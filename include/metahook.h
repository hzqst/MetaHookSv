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

//Fuck Microsoft
#ifdef PropertySheet
#undef PropertySheet
#endif

#ifdef PostMessage
#undef PostMessage
#endif

#ifdef SendMessage
#undef SendMessage
#endif

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

typedef void(*cvar_callback_t)(cvar_t *pcvar);

#ifndef __HLSDK_COMMAND__
#define __HLSDK_COMMAND__

typedef void(*xcommand_t)(void);

typedef struct cmd_function_s
{
	struct cmd_function_s* next;
	char* name;
	xcommand_t function;
	int flags;
}cmd_function_t;

#endif

#ifndef __ENGINE_USER_MSG__
#define __ENGINE_USER_MSG__

typedef struct usermsg_s
{
	int index;
	int size;
	char name[16];
	struct usermsg_s* next;
	pfnUserMsgHook function;
}usermsg_t;

#endif

#ifndef __ENGINR_SVC_FUNCS__
#define __ENGINR_SVC_FUNCS__

typedef void(*fn_parsefunc)(void);

typedef struct svc_func_s
{
	unsigned char opcode;		// Opcode
	unsigned char padding[3];
	const char* pszname;		// Display Name
	fn_parsefunc pfnParse;		// Parse function
} svc_func_t;

#endif

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
#define ENGINE_GOLDSRC_HL25 4

#define PLUGIN_LOAD_SUCCEEDED 0
#define PLUGIN_LOAD_DUPLICATE 1
#define PLUGIN_LOAD_ERROR 2
#define PLUGIN_LOAD_INVALID 3
#define PLUGIN_LOAD_NOMEM 4

#define LOAD_DLL_NOTIFICATION_IS_BLOB		0x1
#define LOAD_DLL_NOTIFICATION_IS_ENGINE		0x2
#define LOAD_DLL_NOTIFICATION_IS_CLIENT		0x4
#define LOAD_DLL_NOTIFICATION_IS_LOAD		0x8
#define LOAD_DLL_NOTIFICATION_IS_UNLOAD		0x10
#define LOAD_DLL_NOTIFICATION_IS_IN_CRIT_REGION		0x20

typedef void* BlobHandle_t;

typedef HMODULE HMEMORYMODULE;

typedef struct mh_load_dll_notification_context_s
{
	HMODULE hModule;
	BlobHandle_t hBlob;
	PVOID ImageBase;
	ULONG ImageSize;
	int flags;
	LPCWSTR FullDllName;
	LPCWSTR BaseDllName;
}mh_load_dll_notification_context_t;

typedef struct mh_dll_info_s
{
	PVOID ImageBase;
	DWORD ImageSize;
	PVOID TextBase;
	DWORD TextSize;
	PVOID DataBase;
	DWORD DataSize;
	PVOID RdataBase;
	DWORD RdataSize;
}mh_dll_info_t;

typedef void (*DisasmSingleCallback)(void *inst, PUCHAR address, size_t instLen, PVOID context);
typedef BOOL (*DisasmCallback)(void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context);
typedef BOOL (*FindAddressCallback)(PUCHAR address);
typedef void (*LoadDllNotificationCallback)(mh_load_dll_notification_context_t*ctx);

typedef struct metahook_api_s
{
	/*
		Purpose : Uninstall the hook
	*/
	BOOL (*UnHook)(hook_t *pHook);

	/*
		Purpose : Install JMP hook at begin of function,
		Warning : This hook operation will be delayed until LoadEngine or LoadClient from all plugins called, if it is requseted from inside LoadEngine or LoadClient.
		otherwise the hook operation will be performed immediately.
	*/
	hook_t *(*InlineHook)(void *pOldFuncAddr, void *pNewFuncAddr, void **pOrginalCall);

	/*
		Purpose : Install VFT hook
		This will modify the content of Virtual Function Table
	*/
	hook_t *(*VFTHook)(void *pClassInstance, int iTableIndex, int iFuncIndex, void *pNewFuncAddr, void **pOrginalCall);

	/*
		Purpose : Install IAT hook
		This will modify the content of Import Address Table
	*/
	hook_t *(*IATHook)(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void **pOrginalCall);

	/*
		Unused
	*/
	void *(*GetClassFuncAddr)(...);

	/*
		Purpose : Query module base of given virtual address
		Return null if it's a BlobEngine
	*/
	PVOID (*GetModuleBase)(PVOID VirtualAddress);

	/*
		Purpose : Query module size of given module base
	*/
	DWORD (*GetModuleSize)(PVOID ModuleBase);

	/*
		Purpose : Query module handle of engine dll
		Return null if it's a BlobEngine
	*/
	HMODULE (*GetEngineModule)(void);

	/*
		Purpose : Query module base of engine
	*/
	PVOID (*GetEngineBase)(void);

	/*
		Purpose : Query module size of engine
	*/
	DWORD (*GetEngineSize)(void);

	/*
		Purpose : Search pattern (signature) in given region, with wildcard 0x2A
	*/
	void *(*SearchPattern)(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);

	/*
		Purpose : Write 4bytes value at given address, ignoring page protection
	*/
	void (*WriteDWORD)(void *pAddress, DWORD dwValue);

	/*
		Purpose : Read 4bytes from given address
	*/
	DWORD (*ReadDWORD)(void *pAddress);

	/*
		Purpose : Write memory at given address, ignoring page protection
	*/
	DWORD (*WriteMemory)(void *pAddress, void *pData, DWORD dwDataSize);

	/*
		Purpose : Read memory from given address
	*/
	DWORD (*ReadMemory)(void *pAddress, void *pData, DWORD dwDataSize);

	/*
		Purpose : Get VideoMode, VideoWidth, VideoHeight, BitPerPixel (16 or 24), WindowedMode
	*/
	DWORD (*GetVideoMode)(int *width, int *height, int *bpp, bool *windowed);

	/*
		Purpose : Get buildnum of loaded engine.
	*/
	DWORD (*GetEngineBuildnum)(void);

	/*
		Purpose : Get factory interface (CreateInterface) of engine dll
	*/
	CreateInterfaceFn (*GetEngineFactory)(void);

	/*
		Purpose : Get the branch target of 0xE8 jmp instruction at given address
	*/
	void *(*GetNextCallAddr)(void *pAddress, DWORD dwCount);

	/*
		Purpose : Write 1 byte value at given address, ignoring page protection
	*/
	void (*WriteBYTE)(void *pAddress, BYTE ucValue);

	/*
		Purpose : Read 1 byte from given address
	*/
	BYTE (*ReadBYTE)(void *pAddress);

	/*
		Purpose : Write 0x90 (x86 nop) at given address, ignoring page protection
	*/
	void (*WriteNOP)(void *pAddress, DWORD dwCount);

	/*
		Purpose : Return one of them :  ENGINE_UNKNOWN, ENGINE_GOLDSRC_BLOB, ENGINE_GOLDSRC, ENGINE_GOLDSRC_HL25, ENGINE_SVENGINE
	*/
	int (*GetEngineType)(void);

	/*
		Purpose : Return one of them : "Unknown", "GoldSrc_Blob", "GoldSrc", "GoldSrc_HL25", "SvEngine"
	*/
	const char *(*GetEngineTypeName)(void);

	/*
		Purpose : Reverse search from given base to lower address, find 90 90 90 99 + ??, or CC CC CC CC + ??
	*/
	PVOID(*ReverseSearchFunctionBegin)(PVOID SearchBegin, DWORD SearchSize);

	/*
		Purpose : Return the section base and section size of given image and section name.
	*/
	PVOID(*GetSectionByName)(PVOID ImageBase, const char *SectionName, ULONG *SectionSize);

	/*
		Purpose : Disassemble a instruction at given address, return result inside callback
	*/
	int (*DisasmSingleInstruction)(PVOID address, DisasmSingleCallback callback, void *context);

	/*
		Purpose : Disassemble instructions at given range of address, return result inside callback

		return TRUE from callback to interrupt the disasm loop.
	*/
	BOOL (*DisasmRanges)(PVOID DisasmBase, SIZE_T DisasmSize, DisasmCallback callback, int depth, PVOID context);

	/*
		Purpose : Search pattern (signature) from pStartSearch in reversed direction
	*/
	void *(*ReverseSearchPattern)(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);

	/*
		Purpose : Get module handle of client dll
		Return null if it's a SecureClient
	*/
	HMODULE	(*GetClientModule)(void);

	/*
		Purpose: Get image base of client
	*/
	PVOID (*GetClientBase)(void);

	/*
		Purpose: Get image size of client
	*/
	DWORD (*GetClientSize)(void);

	/*
		Purpose: Get factory interface (CreateInterface) of client dll
	*/
	CreateInterfaceFn(*GetClientFactory)(void);

	/*
		Purpose: Query information of all loaded plugins.

		Usage:

		mh_plugininfo_t info;
		for(int index = -1; g_pMetaHookAPI->QueryPluginInfo(index, &info); index = info.Index)
		{

		}
	*/
	BOOL (*QueryPluginInfo)(int fromindex, mh_plugininfo_t *info);

	/*
		Purpose: Get information of specified plugin

		Usage:

		mh_plugininfo_t info;
		if(g_pMetaHookAPI->GetPluginInfo("PluginName.dll", &info))//"PluginName.dll" is case-insensitive
		{

		}
	*/
	BOOL (*GetPluginInfo)(const char *name, mh_plugininfo_t *info);

	/*
		Find registered UserMsg, and set it's pfnHook to specified function pointer, return original function pointer if exists.
	*/
	pfnUserMsgHook (*HookUserMsg)(const char *szMsgName, pfnUserMsgHook pfn);

	/*
		Purpose: Find existing cvar, and set it's Cvar-Set callback to specified function pointer, return original callback function pointer if exists.

		Important: Cvar-Set callback only get called when changing cvar value from console.
	*/
	cvar_callback_t (*HookCvarCallback)(const char *cvar_name, cvar_callback_t callback);

	/*
		Purpose: Find existing console command, and set it's command callback to specified function pointer, return original callback function pointer.
	*/
	xcommand_t(*HookCmd)(const char *cmd_name, xcommand_t newfuncs);

	/*
		Purpose: Popup an error message box and terminate game.
	*/
	void (*SysError)(const char *fmt, ...);

	/*
		Purpose: Reverse search from given base to lower address, find 90 90 90 99 + ??, or CC CC CC CC + ??, but with additional check
	*/
	PVOID(*ReverseSearchFunctionBeginEx)(PVOID SearchBegin, DWORD SearchSize, FindAddressCallback callback);

	/*
		Purpose: Check if debugger is attached to current game process.
	*/
	bool(*IsDebuggerPresent)(void);

	/*
		Purpose: Find existing cvar, and register a Cvar-Set callback for it, return original callback function pointer in the pOldCallback if exists.
		Register on existing callback will override the old one (the old one will be in poldcallback)

		Important: Cvar-Set callback only get called when changing cvar value from console.
	*/
	bool(*RegisterCvarCallback)(const char* cvar_name, cvar_callback_t callback, cvar_callback_t *pOldCallback);

	/*
		Purpose: Get handle to blob engine module
	*/
	BlobHandle_t(*GetBlobEngineModule)(void);

	/*
		Purpose: Get handle to blob client module
	*/
	BlobHandle_t(*GetBlobClientModule)(void);

	/*
		Purpose: Get image base of blob module
	*/
	PVOID (*GetBlobModuleImageBase)(BlobHandle_t hBlob);

	/*
		Purpose: Get image size of blob module
	*/
	ULONG (*GetBlobModuleImageSize)(BlobHandle_t hBlob);

	/*
		Purpose: Get section base and section size with the given section name, only ".text\0\0\0" and ".data\0\0\0" supported
	*/
	PVOID(*GetBlobSectionByName)(BlobHandle_t hBlob, const char* SectionName, ULONG* SectionSize);

	/*
		Purpose: Find blob module with specified image base.
	*/
	BlobHandle_t(*BlobLoaderFindBlobByImageBase)(PVOID ImageBase);

	/*
		Purpose: Find blob module with specified virtual address.
	*/
	BlobHandle_t(*BlobLoaderFindBlobByVirtualAddress)(PVOID VirtualAddress);

	/*
		Purpose: Register a load dll notification callback. the callback will be called whenever a dll or blob module is loaded or unloaded.
	*/
	void (*RegisterLoadDllNotificationCallback)(LoadDllNotificationCallback callback);

	/*
		Purpose: Unregister the load dll notification callback
	*/
	void (*UnregisterLoadDllNotificationCallback)(LoadDllNotificationCallback callback);

	/*
		Purpose: Check if the given pszModuleName is imported by hModule
	*/
	bool (*ModuleHasImport)(HMODULE hModule, const char* pszModuleName);

	/*
		Purpose: Check if the given pszModuleName and pszFuncName is imported by hModule
	*/
	bool (*ModuleHasImportEx)(HMODULE hModule, const char* pszModuleName, const char* pszFuncName);

	/*
		Purpose: Check if the given pszModuleName is imported by hBlob
	*/
	bool (*BlobHasImport)(BlobHandle_t hBlob, const char* pszModuleName);

	/*
		Purpose: Check if the given pszModuleName and pszFuncName is imported by hBlob
	*/
	bool (*BlobHasImportEx)(BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName);

	/*
		Purpose: Hook IAT in blob module
	*/
	hook_t* (*BlobIATHook)(BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName, void* pNewFuncAddr, void** pOrginalCall);

	/*
		Purpose: Get current loading game directory, even before it's passed to engine.
	*/

	const char* (*GetGameDirectory)();

	/*
		Purpose: Find command entry by the given cmd_name, return nullptr if not found.
	*/

	cmd_function_t* (*FindCmd)(const char* cmd_name);

	/*
		Purpose : Install VFT hook directly by providing the table address
		This will modify the content of Virtual Function Table
	*/
	hook_t* (*VFTHookEx)(void** pVFTable, int iFuncIndex, void* pNewFuncAddr, void** pOrginalCall);

	/*
		Purpose : Patch E8/E9 call/jmp instruction and redirect the call target
	*/
	hook_t* (*InlinePatchRedirectBranch)(void *pInstructionAddress, void* pNewFuncAddr, void** pOrginalCall);

	/*
		Purpose : Find hook with given parameters.
	*/

	hook_t* (*FindInlineHook)(void* pOldFuncAddr, hook_t*pLastFoundHook);

	hook_t* (*FindVFTHook)(void* pClassInstance, int iTableIndex, int iFuncIndex, hook_t* pLastFoundHook);

	hook_t* (*FindVFTHookEx)(void** pVFTable, int iFuncIndex, hook_t* pLastFoundHook);

	hook_t* (*FindIATHook)(HMODULE hModule, const char* pszModuleName, const char* pszFuncName, hook_t* pLastFoundHook);

	hook_t* (*FindInlinePatchHook)(void* pInstructionAddress, hook_t* pLastFoundHook);

	/*
		Purpose : Get gClientUserMsgs.
	*/
	usermsg_t* (*GetUserMsgBase)();

	/*
		Purpose : Find usermsg_t * by name.
	*/
	usermsg_t* (*FindUserMsgHook)(const char* szMsgName);

	/*
		Purpose : Return the address of cl_parsefuncs tables.
	*/
	svc_func_t* (*GetCLParseFuncBase)();

	/*
		Purpose : Find the parse function by given opcode
	*/
	fn_parsefunc(*FindCLParseFuncByOpcode)(unsigned char opcode);

	/*
		Purpose : Find the parse function by given name
	*/
	fn_parsefunc(*FindCLParseFuncByName)(const char* name);

	/*
		Purpose : Hook the parse function by given opcode, returns the original parse function. returns NULL if the specified entry could not be found.
	*/
	fn_parsefunc(*HookCLParseFuncByOpcode)(unsigned char opcode, fn_parsefunc pfnNewParse);

	/*
		Purpose : Hook the parse function by given name, returns the original parse function. returns NULL if the specified entry could not be found.
	*/
	fn_parsefunc(*HookCLParseFuncByName)(const char* name, fn_parsefunc pfnNewParse);

	/*
		Purpose : Search pattern (signature) in given region, with no wildcard
	*/
	void* (*SearchPatternNoWildCard)(void* pStartSearch, DWORD dwSearchLen, const char* pPattern, DWORD dwPatternLen);

	/*
		Purpose: Return the image base of the mirrored engine dll (with .code relocation fixed), not available on blob engine.
	*/
	PVOID (*GetMirrorEngineBase)(VOID);

	/*
		Purpose: Return the image size of the mirrored engine dll (same as GetEngineSize), not available on blob engine (because blob engine must be loaded at fixed image base).
	*/
	ULONG (*GetMirrorEngineSize)(VOID);

	/*
		Purpose: Return the image base of the mirrored client dll (with .code relocation fixed), not available on blob engine.
	*/
	PVOID(*GetMirrorClientBase)(VOID);

	/*
		Purpose: Return the image size of the mirrored client dll (same as GetClientSize), not available on blob client (because blob client must be loaded at fixed image base).
	*/
	ULONG(*GetMirrorClientSize)(VOID);

	/*
		Purpose: Load mirrored-dll with no execute permission. the dll is opened via fopen.
	*/

	HMEMORYMODULE (*MH_LoadMirrorDLL_Std)(const char* szFileName);

	/*
		Purpose: Load mirrored-dll with no execute permission. the dll is opened via g_pFileSystem.
	*/

	HMEMORYMODULE (*MH_LoadMirrorDLL_FileSystem)(const char* szFileName);

	/*
		Purpose: Free the given mirrored-dll.
	*/

	void (*MH_FreeMirrorDLL)(HMEMORYMODULE hMemoryModule);

	/*
		Purpose: Get ImageBase from the given mirrored-dll.
	*/

	PVOID(*MH_GetMirrorDLLBase)(HMEMORYMODULE hMemoryModule);

	/*
		Purpose: Get ImageSize from the given mirrored-dll.
	*/

	ULONG (*MH_GetMirrorDLLSize)(HMEMORYMODULE hMemoryModule);

	//Always terminate with a NULL
	PVOID Terminator;

}metahook_api_t;

typedef struct mh_enginesave_s
{
	cl_exportfuncs_t *pExportFuncs;
	cl_enginefunc_t *pEngineFuncs;
}mh_enginesave_t;

#include <IFileSystem.h>
#include <ICommandLine.h>
#include <IRegistry.h>

#define METAHOOK_API_VERSION 105

class ICommandLine;
class IFileSystem;
class IRegistry;
class IFileSystem_HL25;

typedef struct mh_interface_s
{
	ICommandLine *CommandLine;
	IFileSystem *FileSystem;
	IRegistry *Registry;
	IFileSystem_HL25* FileSystem_HL25;
	int MetaHookAPIVersion;
}mh_interface_t;

#include <IPlugins.h>

extern mh_interface_t *g_pInterface;
extern cl_enginefunc_t gEngfuncs;
extern metahook_api_t *g_pMetaHookAPI;
extern mh_enginesave_t *g_pMetaSave;

#define FILESYSTEM_ANY_OPEN(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Open(__VA_ARGS__) : g_pFileSystem->Open(__VA_ARGS__))
#define FILESYSTEM_ANY_READ(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Read(__VA_ARGS__) : g_pFileSystem->Read(__VA_ARGS__))
#define FILESYSTEM_ANY_CLOSE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Close(__VA_ARGS__) : g_pFileSystem->Close(__VA_ARGS__))
#define FILESYSTEM_ANY_SIZE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Size(__VA_ARGS__) : g_pFileSystem->Size(__VA_ARGS__))
#define FILESYSTEM_ANY_SEEK(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Seek(__VA_ARGS__) : g_pFileSystem->Seek(__VA_ARGS__))
#define FILESYSTEM_ANY_TELL(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Tell(__VA_ARGS__) : g_pFileSystem->Tell(__VA_ARGS__))
#define FILESYSTEM_ANY_WRITE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Write(__VA_ARGS__) : g_pFileSystem->Write(__VA_ARGS__))
#define FILESYSTEM_ANY_CREATEDIR(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->CreateDirHierarchy(__VA_ARGS__) : g_pFileSystem->CreateDirHierarchy(__VA_ARGS__))
#define FILESYSTEM_ANY_EOF(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->EndOfFile(__VA_ARGS__) : g_pFileSystem->EndOfFile(__VA_ARGS__))
#define FILESYSTEM_ANY_PARSEFILE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->ParseFile(__VA_ARGS__) : g_pFileSystem->ParseFile(__VA_ARGS__))
#define FILESYSTEM_ANY_READLINE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->ReadLine(__VA_ARGS__) : g_pFileSystem->ReadLine(__VA_ARGS__))
#define FILESYSTEM_ANY_FILEEXISTS(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->FileExists(__VA_ARGS__) : g_pFileSystem->FileExists(__VA_ARGS__))
#define FILESYSTEM_ANY_GETLOCALPATH(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->GetLocalPath(__VA_ARGS__) : g_pFileSystem->GetLocalPath(__VA_ARGS__))
#define FILESYSTEM_ANY_GETCURRENTDIRECTORY(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->GetCurrentDirectory(__VA_ARGS__) : g_pFileSystem->GetCurrentDirectory(__VA_ARGS__))
#define FILESYSTEM_ANY_ADDSEARCHPATH(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->AddSearchPath(__VA_ARGS__) : g_pFileSystem->AddSearchPath(__VA_ARGS__))
#define FILESYSTEM_ANY_ADDSEARCHPATHNOWRITE(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->AddSearchPathNoWrite(__VA_ARGS__) : g_pFileSystem->AddSearchPathNoWrite(__VA_ARGS__))
#define FILESYSTEM_ANY_MOUNT(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Mount(__VA_ARGS__) : g_pFileSystem->Mount(__VA_ARGS__))
#define FILESYSTEM_ANY_UNMOUNT(...) (g_pFileSystem_HL25 ? g_pFileSystem_HL25->Unmount(__VA_ARGS__) : g_pFileSystem->Unmount(__VA_ARGS__))

#endif