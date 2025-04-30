#include <Windows.h>
#include "metahook.h"
#include "interface.h"

#include <detours.h>
#include <capstone.h>
#include <LoadDllMemoryApi.h>

#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <functional>

#include "LoadBlob.h"
#include "LoadDllNotification.h"

extern PVOID g_BlobLoaderSectionBase;
extern ULONG g_BlobLoaderSectionSize;

#define MH_HOOK_INLINE 1
#define MH_HOOK_VFTABLE 2
#define MH_HOOK_IAT 3
#define MH_HOOK_INLINEPATCH 4

struct tagIATDATA
{
	HMODULE hModule;
	BlobHandle_t hBlob;
	PVOID* pImportFuncAddr;
	char szModuleName[64];
	char szFuncName[256];
};

struct tagVTABLEDATA
{
	PVOID pClassInstance;
	int iTableIndex;
	int iFuncIndex;
	PVOID *pVirtualFuncTable;
	PVOID *pVirtualFuncAddr;
};

struct tagINLINEDATA
{
	PVOID pTrampolineCall;
};

struct tagINLINEPATCHDATA
{
	PVOID pInstructionAddress;
	ULONG PatchLength;
	UCHAR OriginalBytes[8];
	UCHAR NewCodeBytes[8];
};

union tagHOOKDATA
{
	tagIATDATA iathook;
	tagVTABLEDATA vfthook;
	tagINLINEDATA inlinehook;
	tagINLINEPATCHDATA inlinepatch;
};

typedef struct hook_s
{
	struct hook_s() {
		iType = 0;
		bCommitted = false;
		pOldFuncAddr = NULL;
		pNewFuncAddr = NULL;
		pOrginalCall = NULL;
		pNext = NULL;

		memset(&hookData, 0, sizeof(hookData));
	}

	int iType;
	bool bCommitted;
	void *pOldFuncAddr;
	void *pNewFuncAddr;
	void **pOrginalCall;
	struct hook_s *pNext;
	tagHOOKDATA hookData;
}hook_t;

typedef struct cvar_callback_entry_s
{
	cvar_callback_t callback;
	cvar_t *pcvar;
	struct cvar_callback_entry_s *next;
}cvar_callback_entry_t;

cvar_callback_entry_t **cvar_callbacks = NULL;
cvar_callback_entry_t* g_ManagedCvarCallbackList = NULL;
std::vector<cvar_callback_entry_t*> g_ManagedCvarCallbacks;
usermsg_t **gClientUserMsgs = NULL;
cmd_function_t *(*Cmd_GetCmdBase)(void) = NULL;
void **g_pVideoMode = NULL;
svc_func_t* cl_parsefuncs = NULL;
int (*g_pfnbuild_number)(void) = NULL;
int(*g_pfnClientDLL_Init)(void) = NULL;
void(*g_pfnCvar_DirectSet)(cvar_t* var, char* value) = NULL;
void(*g_pfnLoadBlobFile)(BYTE* pBuffer, void** pBlobFootprint, void** pv, DWORD dwBufferSize) = NULL;
void(*g_pfnFreeBlob)(void** pBlobFootprint) = NULL;

void *g_StudioInterfaceCall = NULL;
struct engine_studio_api_s* g_pEngineStudioAPI = NULL;
struct r_studio_interface_t** g_pStudioAPI = NULL;

CreateInterfaceFn *g_pClientFactory = NULL;
HMODULE *g_phClientModule = NULL;

BlobHandle_t g_hBlobEngine = NULL;
BlobHandle_t g_hBlobClient = NULL;

HMODULE g_hEngineModule = NULL;
PVOID g_dwEngineBase = NULL;
DWORD g_dwEngineSize = NULL;

hook_t *g_pHookBase = NULL;	

ULONG_PTR g_dwClientDLL_Initialize[1] = {0};
cl_exportfuncs_t *g_pExportFuncs = NULL;
void *g_ppExportFuncs = NULL;
void *g_ppEngfuncs = NULL;

bool g_bSaveVideo = false;
bool g_bTransactionHook = false;
int g_iEngineType = ENGINE_UNKNOWN;

char g_szEnvPath[4096] = { 0 };
char g_szGameDirectory[32] = { 0 };

HMEMORYMODULE g_hMirrorEngine = NULL;
HMEMORYMODULE g_hMirrorClient = NULL;

ThreadPoolHandle_t g_ThreadPool = NULL;

PVOID MH_GetNextCallAddr(void *pAddress, DWORD dwCount);
BOOL MH_UnHook(hook_t *pHook);
hook_t *MH_InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void **pOriginalCall);
hook_t *MH_VFTHook(void *pClassInstance, int iTableIndex, int iFuncIndex, void *pNewFuncAddr, void **pOriginalCall);
hook_t* MH_VFTHookEx(void** pVFTable, int iFuncIndex, void* pNewFuncAddr, void** pOrginalCall);
hook_t *MH_IATHook(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void **pOriginalCall);
void *MH_GetClassFuncAddr(...);
HMODULE MH_GetClientModule(void);
PVOID MH_GetModuleBase(PVOID VirtualAddress);
DWORD MH_GetModuleSize(PVOID ModuleBase);
PVOID MH_GetClientBase(void);
DWORD MH_GetClientSize(void);
void *MH_SearchPattern(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);
hook_t* MH_InlinePatchRedirectBranch(void* pInstructionAddress, void* pNewFuncAddr, void** pOrginalCall);
void MH_WriteDWORD(void *pAddress, DWORD dwValue);
DWORD MH_ReadDWORD(void *pAddress);
void MH_WriteBYTE(void *pAddress, BYTE ucValue);
BYTE MH_ReadBYTE(void *pAddress);
void MH_WriteNOP(void *pAddress, DWORD dwCount);
DWORD MH_WriteMemory(void *pAddress, void *pData, DWORD dwDataSize);
DWORD MH_ReadMemory(void *pAddress, void *pData, DWORD dwDataSize);
DWORD MH_GetVideoMode(int *wide, int *height, int *bpp, bool *windowed);
DWORD MH_GetEngineVersion(void);
int MH_DisasmSingleInstruction(PVOID address, DisasmSingleCallback callback, void *context);
BOOL MH_DisasmRanges(PVOID DisasmBase, SIZE_T DisasmSize, DisasmCallback callback, int depth, PVOID context);
PVOID MH_GetSectionByName(PVOID ImageBase, const char *SectionName, ULONG *SectionSize);
PVOID MH_ReverseSearchFunctionBegin(PVOID SearchBegin, DWORD SearchSize);
PVOID MH_ReverseSearchFunctionBeginEx(PVOID SearchBegin, DWORD SearchSize, FindAddressCallback callback);
void *MH_ReverseSearchPattern(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);
hook_t* MH_BlobIATHook(BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName, void* pNewFuncAddr, void** pOrginalCall);
CreateInterfaceFn MH_GetEngineFactory(void);
HMEMORYMODULE MH_LoadMirrorDLL_Std(const char* szFileName);
HMEMORYMODULE MH_LoadMirrorDLL_FileSystem(const char* szFileName);
void MH_FreeMirrorDLL(HMEMORYMODULE hMemoryModule);
PVOID MH_GetMirrorDLLBase(HMEMORYMODULE hMemoryModule);
ULONG MH_GetMirrorDLLSize(HMEMORYMODULE hMemoryModule);
ThreadPoolHandle_t MH_GetGlobalThreadPool(void);
ThreadPoolHandle_t MH_CreateThreadPool(ULONG minThreads, ULONG maxThreads);
ThreadWorkItemHandle_t MH_CreateWorkItem(ThreadPoolHandle_t hThreadPool, fnThreadWorkItemCallback callback, void* ctx);
void MH_QueueWorkItem(ThreadPoolHandle_t hThreadPool, ThreadWorkItemHandle_t hWorkItem);
void MH_WaitForWorkItemToComplete(ThreadWorkItemHandle_t hWorkItem);
void MH_DeleteThreadPool(ThreadWorkItemHandle_t hThreadPool);
void MH_DeleteWorkItem(ThreadWorkItemHandle_t hWorkItem);

typedef struct plugin_s
{
	std::string filename;
	std::string filepath;
	HINTERFACEMODULE module;
	size_t modulesize;
	IBaseInterface *pPluginAPI;
	int iInterfaceVersion;
	struct plugin_s *next;
}plugin_t;

plugin_t *g_pPluginBase = NULL;

extern IFileSystem_HL25 *g_pFileSystem_HL25;
extern IFileSystem* g_pFileSystem;

mh_interface_t gInterface = {0};
mh_enginesave_t gMetaSave = {0};

extern metahook_api_t gMetaHookAPI_LegacyV2;
extern metahook_api_t gMetaHookAPI;

extern "C"
{
#define MAX_SYS_ERROR_LENGTH 4096

	void(*g_pfnSys_Error)(const char* fmt, ...) = NULL;

	void MH_SysError(const char* fmt, ...)
	{
		char msg[MAX_SYS_ERROR_LENGTH];

		va_list argptr;

		va_start(argptr, fmt);
		_vsnprintf(msg, MAX_SYS_ERROR_LENGTH - 1, fmt, argptr);
		va_end(argptr);

		msg[MAX_SYS_ERROR_LENGTH - 1] = 0;

#if 1
		if (gMetaSave.pEngineFuncs)
		{
			if (gMetaSave.pEngineFuncs->pfnClientCmd)
				gMetaSave.pEngineFuncs->pfnClientCmd("escape\n");
		}
#endif
		MessageBoxA(NULL, msg, "Fatal Error", MB_ICONERROR);
		NtTerminateProcess((HANDLE)(-1), 0);
	}
}

DWORD MH_LoadBlobFile(BYTE* pBuffer, void** pBlobFootPrint, void** pv, DWORD dwBufferSize)
{
#if defined(METAHOOK_BLOB_SUPPORT) || defined(_DEBUG)
	auto hBlob = LoadBlobFromBuffer(pBuffer, dwBufferSize, g_BlobLoaderSectionBase, g_BlobLoaderSectionSize);

	if (hBlob)
	{
		BlobLoaderAddBlob(hBlob);

		bool bIsClientDll = (GetBlobModuleImageBase(hBlob) == (PVOID)BLOB_LOAD_CLIENT_BASE) ? true : false;

		if (bIsClientDll)
		{
			g_hBlobClient = hBlob;
		}

		MH_DispatchLoadBlobNotificationCallback(hBlob, LOAD_DLL_NOTIFICATION_IS_LOAD);

		RunDllMainForBlob(hBlob, DLL_PROCESS_ATTACH);
		RunExportEntryForBlob(hBlob, pv);

		*pBlobFootPrint = hBlob;

		return GetBlobModuleSpecialAddress(hBlob);
	}

#else

	MH_SysError("This build of metahook does not support blob client.\nPlease use metahook_blob.exe instead.");

#endif
	return 0;
}

void MH_FreeBlobProxy(void** pBlobFootPrint)
{
	BlobHandle_t hBlob = (BlobHandle_t)(*pBlobFootPrint);

	MH_DispatchLoadBlobNotificationCallback(hBlob, LOAD_DLL_NOTIFICATION_IS_UNLOAD);
	FreeBlobModule(hBlob);
	BlobLoaderRemoveBlob(hBlob);

	(*pBlobFootPrint) = NULL;
}

void MH_FreeHooksForModule(PVOID ImageBase, ULONG ImageSize)
{
	//TODO
}

void MH_Cvar_DirectSet(cvar_t* var, char* value)
{
	g_pfnCvar_DirectSet(var, value);

	auto v = (*cvar_callbacks);

	if (v)
	{
		while (v->pcvar != var)
		{
			v = v->next;
			if (!v)
				return;
		}
		v->callback(var);
	}
}

bool MH_IsDebuggerPresent()
{
	return IsDebuggerPresent() ? true : false;
}

cvar_callback_t MH_HookCvarCallback(const char *cvar_name, cvar_callback_t callback)
{
	if (!gMetaSave.pEngineFuncs)
		return NULL;

	auto cvar = gMetaSave.pEngineFuncs->pfnGetCvarPointer(cvar_name);

	if (!cvar)
		return NULL;

	if (!cvar_callbacks)
		return NULL;

	auto v = (*cvar_callbacks);
	if (v)
	{
		while (v->pcvar != cvar)
		{
			v = v->next;
			if (!v)
			{
				return NULL;
			}
		}
		auto orig = v->callback;
		v->callback = callback;
		return orig;
	}

	return NULL;
}

bool MH_RegisterCvarCallback(const char* cvar_name, cvar_callback_t callback, cvar_callback_t *poldcallback)
{
	if (!gMetaSave.pEngineFuncs)
		return NULL;

	auto cvar = gMetaSave.pEngineFuncs->pfnGetCvarPointer(cvar_name);

	if (!cvar)
		return NULL;

	if (!cvar_callbacks)
		return NULL;

	auto v = (*cvar_callbacks);
	if (v)
	{
		while (v->pcvar != cvar)
		{
			v = v->next;
			if (!v)
			{
				auto newEntry = new cvar_callback_entry_t;
				newEntry->callback = callback;
				newEntry->pcvar = cvar;
				newEntry->next = (*cvar_callbacks);

				(*cvar_callbacks) = newEntry;

				g_ManagedCvarCallbacks.push_back(newEntry);

				if (poldcallback)
				{
					*poldcallback = NULL;
				}
				return true;
			}
		}

		auto orig = v->callback;
		v->callback = callback;
		if (poldcallback)
		{
			*poldcallback = orig;
		}
		return true;
	}
	else
	{
		auto newEntry = new cvar_callback_entry_t;
		newEntry->callback = callback;
		newEntry->pcvar = cvar;
		newEntry->next = NULL;

		(*cvar_callbacks) = newEntry;

		g_ManagedCvarCallbacks.push_back(newEntry);

		if (poldcallback)
		{
			*poldcallback = NULL;
		}

		return true;
	}

	return false;
}

usermsg_t* MH_GetUserMsgBase()
{
	if (!gClientUserMsgs)
		return NULL;

	return (*gClientUserMsgs);
}

usermsg_t *MH_FindUserMsgHook(const char *szMsgName)
{
	if (!MH_GetUserMsgBase())
		return NULL;

	for (usermsg_t *msg = MH_GetUserMsgBase(); msg; msg = msg->next)
	{
		if (!strcmp(msg->name, szMsgName))
			return msg;
	}

	return NULL;
}

pfnUserMsgHook MH_HookUserMsg(const char *szMsgName, pfnUserMsgHook pfn)
{
	usermsg_t *msg = MH_FindUserMsgHook(szMsgName);

	if (msg)
	{
		pfnUserMsgHook result = msg->function;
		msg->function = pfn;
		return result;
	}

	return NULL;
}

cmd_function_t *MH_FindCmd(const char *cmd_name)
{
	if (!Cmd_GetCmdBase)
		return NULL;

	for (cmd_function_t *cmd = Cmd_GetCmdBase(); cmd; cmd = cmd->next)
	{
		if (!strcmp(cmd->name, cmd_name))
			return cmd;
	}

	return NULL;
}

cmd_function_t *MH_FindCmdPrev(const char *cmd_name)
{
	if (!Cmd_GetCmdBase)
		return NULL;

	cmd_function_t *cmd;

	for (cmd = Cmd_GetCmdBase()->next; cmd->next; cmd = cmd->next)
	{
		if (!strcmp(cmd_name, cmd->next->name))
			return cmd;
	}

	return NULL;
}

xcommand_t MH_HookCmd(const char *cmd_name, xcommand_t newfuncs)
{
	if (!Cmd_GetCmdBase)
		return NULL;

	cmd_function_t *cmd = MH_FindCmd(cmd_name);

	if (!cmd)
		return NULL;

	xcommand_t result = cmd->function;
	cmd->function = newfuncs;
	return result;
}

svc_func_t* MH_GetCLParseFuncBase()
{
	return cl_parsefuncs;
}

fn_parsefunc MH_FindCLParseFuncByOpcode(unsigned char opcode)
{
	for (auto p = MH_GetCLParseFuncBase(); p->opcode != (unsigned char)-1; ++p)
	{
		if (p->opcode == opcode)
		{
			return p->pfnParse;
		}
	}

	return NULL;
}

fn_parsefunc MH_FindCLParseFuncByName(const char* name)
{
	for (auto p = MH_GetCLParseFuncBase(); p->opcode != (unsigned char)-1; ++p)
	{
		if (!strcmp(name, p->pszname))
		{
			return p->pfnParse;
		}
	}

	return NULL;
}

fn_parsefunc MH_HookCLParseFuncByOpcode(unsigned char opcode, fn_parsefunc pfnNewParse)
{
	for (auto p = MH_GetCLParseFuncBase(); p->opcode != (unsigned char)-1; ++p)
	{
		if (p->opcode == opcode)
		{
			auto oldParse = p->pfnParse;
			p->pfnParse = pfnNewParse;

			return oldParse;
		}
	}

	return NULL;
}

fn_parsefunc MH_HookCLParseFuncByName(const char* name, fn_parsefunc pfnNewParse)
{
	for (auto p = MH_GetCLParseFuncBase(); p->opcode != (unsigned char)-1; ++p)
	{
		if (!strcmp(name, p->pszname))
		{
			auto oldParse = p->pfnParse;
			p->pfnParse = pfnNewParse;

			return oldParse;
		}
	}

	return NULL;
}

void MH_PrintPluginList(void)
{
	if (!gMetaSave.pEngineFuncs)
		return;

	gMetaSave.pEngineFuncs->Con_Printf("|%5s|%2s|%24s|%24s|\n", "index", "api", "plugin name", "plugin version");

	int index = 0;
	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next, index++)
	{
		const char *version = "";
		switch (plug->iInterfaceVersion)
		{
		case 4:
			version = ((IPluginsV4 *)plug->pPluginAPI)->GetVersion();
			break;
		default:
			break;
		}
		gMetaSave.pEngineFuncs->Con_Printf("|%5d| v%d|%24s|%24s|\n", index, plug->iInterfaceVersion, plug->filename.c_str(), version);
	}
}

int MH_LoadPlugin(const std::string &filepath, const std::string &filename)
{
	bool bIsDuplicatePlugin = false;

	for (plugin_t *p = g_pPluginBase; p; p = p->next)
	{
		if (!stricmp(p->filename.c_str(), filename.c_str()))
		{
			bIsDuplicatePlugin = true;
			break;
		}
	}

	if (!bIsDuplicatePlugin && GetModuleHandleA(filename.c_str()))
	{
		bIsDuplicatePlugin = true;
	}

	if (bIsDuplicatePlugin)
	{
		return PLUGIN_LOAD_DUPLICATE;
	}

	HINTERFACEMODULE hModule = (HINTERFACEMODULE)Sys_LoadModule(filepath.c_str());

	if (!hModule)
	{
		return PLUGIN_LOAD_ERROR;
	}

	for (auto p = g_pPluginBase; p; p = p->next)
	{
		if (p->module == hModule)
		{
			bIsDuplicatePlugin = true;
			break;
		}
	}

	if (bIsDuplicatePlugin)
	{
		Sys_FreeModule(hModule);
		return PLUGIN_LOAD_DUPLICATE;
	}

	CreateInterfaceFn fnCreateInterface = Sys_GetFactory(hModule);

	if (!fnCreateInterface)
	{
		Sys_FreeModule(hModule);
		return PLUGIN_LOAD_INVALID;
	}

	plugin_t *plug = new (std::nothrow) plugin_t;

	if (!plug)
	{
		Sys_FreeModule(hModule);
		return PLUGIN_LOAD_NOMEM;
	}

	plug->module = hModule;
	plug->modulesize = MH_GetModuleSize(hModule);
	plug->pPluginAPI = fnCreateInterface(METAHOOK_PLUGIN_API_VERSION_V4, NULL);
	if (plug->pPluginAPI)
	{
		plug->iInterfaceVersion = 4;
		((IPluginsV4 *)plug->pPluginAPI)->Init(&gMetaHookAPI, &gInterface, &gMetaSave);
	}
	else
	{
		plug->pPluginAPI = fnCreateInterface(METAHOOK_PLUGIN_API_VERSION_V3, NULL);
		if (plug->pPluginAPI)
		{
			plug->iInterfaceVersion = 3;
			((IPluginsV3 *)plug->pPluginAPI)->Init(&gMetaHookAPI, &gInterface, &gMetaSave);
		}
		else
		{
			plug->pPluginAPI = fnCreateInterface(METAHOOK_PLUGIN_API_VERSION_V2, NULL);

			if (plug->pPluginAPI)
			{
				plug->iInterfaceVersion = 2;

				((IPluginsV2*)plug->pPluginAPI)->Init(&gMetaHookAPI_LegacyV2, &gInterface, &gMetaSave);
			}
			else
			{
				plug->pPluginAPI = fnCreateInterface(METAHOOK_PLUGIN_API_VERSION_V1, NULL);

				if (plug->pPluginAPI)
				{
					plug->iInterfaceVersion = 1;
				}
				else
				{
					delete plug;
					Sys_FreeModule(hModule);

					return PLUGIN_LOAD_INVALID;
				}
			}
		}
	}

	plug->filename = filename;
	plug->filepath = filepath;
	plug->next = g_pPluginBase;
	g_pPluginBase = plug;
	return PLUGIN_LOAD_SUCCEEDED;
}

bool MH_HasSSE()
{
	auto SDL2 = GetModuleHandleA("SDL2.dll");
	if (SDL2)
	{
		bool(__cdecl *SDL_HasSSE)() = (decltype(SDL_HasSSE))GetProcAddress(SDL2, "SDL_HasSSE");
		if (SDL_HasSSE)
			return SDL_HasSSE();
	}

	return false;
}

bool MH_HasSSE2()
{
	auto SDL2 = GetModuleHandleA("SDL2.dll");
	if (SDL2)
	{
		bool(__cdecl *SDL_HasSSE2)() = (decltype(SDL_HasSSE2))GetProcAddress(SDL2, "SDL_HasSSE2");
		if (SDL_HasSSE2)
			return SDL_HasSSE2();
	}

	return false;
}

bool MH_HasAVX()
{
	auto SDL2 = GetModuleHandleA("SDL2.dll");
	if (SDL2)
	{
		bool(__cdecl *SDL_HasAVX)() = (decltype(SDL_HasAVX))GetProcAddress(SDL2, "SDL_HasAVX");
		if (SDL_HasAVX)
			return SDL_HasAVX();
	}

	return false;
}

bool MH_HasAVX2()
{
	auto SDL2 = GetModuleHandleA("SDL2.dll");
	if (SDL2)
	{
		bool(__cdecl *SDL_HasAVX2)() = (decltype(SDL_HasAVX2))GetProcAddress(SDL2, "SDL_HasAVX2");
		if (SDL_HasAVX2)
			return SDL_HasAVX2();
	}

	return false;
}

void MH_ReportError(const std::string &fileName, int result, int win32err)
{
	if (result == PLUGIN_LOAD_DUPLICATE)
	{
		std::stringstream ss;
		ss << "MH_LoadPlugin: Duplicate plugin \"" << fileName << "\" found, ignored.";
		MessageBoxA(NULL, ss.str().c_str(), "Warning", MB_ICONWARNING);
	}
	else if (result == PLUGIN_LOAD_INVALID)
	{
		std::stringstream ss;
		ss << "MH_LoadPlugin: Invalid plugin \"" << fileName << "\" found, ignored.";
		MessageBoxA(NULL, ss.str().c_str(), "Warning", MB_ICONWARNING);
	}
	else if (result == PLUGIN_LOAD_ERROR)
	{
		std::stringstream ss;
		ss << "MH_LoadPlugin: Could not load \"" << fileName << "\", Win32Error = " << win32err << ".\n\n";

		LPVOID lpMsgBuf = NULL;

		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, win32err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		ss << (const char *)lpMsgBuf;

		LocalFree(lpMsgBuf);

		MessageBoxA(NULL, ss.str().c_str(), "Warning", MB_ICONWARNING);
	}
}

void ANSIToUnicode(const std::string& str, std::wstring& out)
{
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), NULL, 0);
	out.resize(len);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), (LPWSTR)out.data(), len);
}

void MH_LoadDllPaths(const char* szGameName, const char* szGameFullPath)
{
	GetEnvironmentVariableA("PATH", g_szEnvPath, _ARRAYSIZE(g_szEnvPath));

	std::string GameFullPath = szGameFullPath;
	std::string GameName = szGameName;

	if (GameFullPath.size() >= 1 && GameFullPath[GameFullPath.size() - 1] != '\\' && GameFullPath[GameFullPath.size() - 1] != '/')
	{
		GameFullPath += "\\";
	}

	std::stringstream NewEnvPathStream;

	NewEnvPathStream << g_szEnvPath;
	NewEnvPathStream << ";";
	NewEnvPathStream << GameFullPath;
	NewEnvPathStream << GameName;
	NewEnvPathStream << "\\metahook\\dlls";
#if 0
	std::string aConfigFile = GameName;
	aConfigFile += "\\metahook\\configs\\dllpaths.lst";

	std::ifstream infile;
	infile.open(aConfigFile);
	if (!infile.is_open())
	{
		return;
	}

	while (!infile.eof())
	{
		std::string stringLine;
		std::getline(infile, stringLine);
		if (stringLine.length() > 1)
		{
			if (stringLine[0] == '\r' || stringLine[0] == '\n')
				continue;
			if (stringLine[0] == '\0')
				continue;
			if (stringLine[0] == ';')
				continue;
			if (stringLine[0] == '/' && stringLine[1] == '/')
				continue;

			if (stringLine.length() > 2 &&
				std::isalpha(stringLine[0]) &&
				stringLine[1] == ':' &&
				(stringLine[2] == '/' || stringLine[2] == '\\'))
			{
				NewEnvPath << ";" << stringLine;
			}
			else
			{
				NewEnvPath << ";" << GameFullPath << GameName << "\\metahook\\dlls\\" << stringLine;
			}

			
		}
	}
	infile.close();
#else

	std::string dllsPath = GameFullPath + GameName + "\\metahook\\dlls";
	std::function<void(const std::string&)> traverseDirectory = [&](const std::string& path) {
		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findData);
		
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
					continue;
				
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					std::string subDirPath = path + "\\" + findData.cFileName;
					NewEnvPathStream << ";" << subDirPath;
					traverseDirectory(subDirPath);
				}
			} while (FindNextFileA(hFind, &findData));
			
			FindClose(hFind);
		}
	};
	
	traverseDirectory(dllsPath);
#endif

	auto newEnvPath = NewEnvPathStream.str();

	SetEnvironmentVariableA("PATH", newEnvPath.c_str());

}

void MH_RemoveDllPaths(void)
{
	SetEnvironmentVariableA("PATH", g_szEnvPath);
}

void MH_LoadPlugins(const char * szGameName, const char* szGameFullPath)
{
	std::string aConfigFile = szGameName;
	aConfigFile += "\\metahook\\configs\\plugins.lst";

	std::ifstream infile;
	infile.open(aConfigFile);
	if (!infile.is_open())
	{
		return;
	}

	while (!infile.eof())
	{
		std::string stringLine;
		std::getline(infile, stringLine);
		if (stringLine.length() > 1)
		{
			if (stringLine[0] == '\r' || stringLine[0] == '\n')
				continue;
			if (stringLine[0] == '\0')
				continue;
			if (stringLine[0] == ';')
				continue;
			if (stringLine[0] == '/' && stringLine[1] == '/')
				continue;

			std::string aPluginPath = szGameFullPath;

			if (aPluginPath.size() >= 1 && aPluginPath[aPluginPath.size() - 1] != '\\' && aPluginPath[aPluginPath.size() - 1] != '/')
			{
				aPluginPath += "\\";
			}

			aPluginPath += szGameName;
			aPluginPath += "\\metahook\\plugins\\";

			std::string aFileName;

			if (stringLine.size() > 4 &&
				tolower(stringLine[stringLine.length() - 1]) == 'l' &&
				tolower(stringLine[stringLine.length() - 2]) == 'l' &&
				tolower(stringLine[stringLine.length() - 3]) == 'd' &&
				tolower(stringLine[stringLine.length() - 4]) == '.')
			{
				aFileName = stringLine.substr(0, stringLine.length() - 4);
				aPluginPath += aFileName;
			}
			else
			{
				aFileName = stringLine;
				aPluginPath += aFileName;
			}

			do
			{
#define MH_LOAD_PLUGIN_TEMPLATE(check, suffix) if (check)\
				{\
					std::string fileName = aFileName + suffix;\
					int result = MH_LoadPlugin(aPluginPath + suffix, fileName);\
					int win32err = GetLastError();\
					if (PLUGIN_LOAD_SUCCEEDED == result)\
					{\
						break;\
					}\
					else\
					{\
						MH_ReportError(fileName, result, win32err);\
					}\
				}

#define MH_LOAD_PLUGIN_TEMPLATE2(check, suffix) if (check) {\
				std::string fileName = aFileName + suffix;\
				int result = MH_LoadPlugin(aPluginPath + suffix, fileName);\
				int win32err = GetLastError();\
				if (PLUGIN_LOAD_SUCCEEDED == result)\
				{\
					break;\
				}\
				else if (PLUGIN_LOAD_ERROR == result && (win32err == ERROR_FILE_NOT_FOUND || win32err == ERROR_MOD_NOT_FOUND))\
				{\
				}\
				else\
				{\
					MH_ReportError(fileName, result, win32err);\
				}\
			}

				MH_LOAD_PLUGIN_TEMPLATE(MH_IsDebuggerPresent(), ".dll");
				MH_LOAD_PLUGIN_TEMPLATE2(MH_HasAVX2(), "_AVX2.dll");
				MH_LOAD_PLUGIN_TEMPLATE2(MH_HasAVX(), "_AVX.dll");
				MH_LOAD_PLUGIN_TEMPLATE2(MH_HasSSE2(), "_SSE2.dll");
				MH_LOAD_PLUGIN_TEMPLATE2(MH_HasSSE(), "_SSE.dll");
				MH_LOAD_PLUGIN_TEMPLATE2(MH_HasSSE(), "_SSE.dll");
				MH_LOAD_PLUGIN_TEMPLATE(1, ".dll");

			} while (0);

#undef MH_LOAD_PLUGIN_TEMPLATE
#undef MH_LOAD_PLUGIN_TEMPLATE2

		}
	}
	infile.close();
}

bool MH_IsTransactionHookEnabled(void)
{
	return g_bTransactionHook;
}

void MH_TransactionHookBegin(void)
{
	g_bTransactionHook = true;
}

void MH_TransactionHookUpdateThreads(void)
{
	int num_threads_frozen;
	auto first_run = true;

	do
	{
		num_threads_frozen = 0;
		HANDLE hThreadHandle = NULL;

		while (1)
		{
			HANDLE hNextThreadHandle = NULL;
			const auto status = NtGetNextThread(GetCurrentProcess(), hThreadHandle,
				THREAD_QUERY_LIMITED_INFORMATION | THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, 0,
				0, &hNextThreadHandle);

			if (hThreadHandle != NULL) {
				CloseHandle(hThreadHandle);
			}

			if (!NT_SUCCESS(status)) {
				break;
			}

			hThreadHandle = hNextThreadHandle;

			const auto thread_id = GetThreadId(hThreadHandle);

			if (thread_id == 0 || thread_id == GetCurrentThreadId()) {
				continue;
			}

			const auto suspend_count = SuspendThread(hThreadHandle);

			if (suspend_count == static_cast<DWORD>(-1)) {
				continue;
			}

			// Check if the thread was already frozen. Only resume if the thread was already frozen, and it wasn't the
			// first run of this freeze loop to account for threads that may have already been frozen for other reasons.
			if (suspend_count != 0 && !first_run) {
				ResumeThread(hThreadHandle);
				continue;
			}

			HANDLE hNewThreadHandle = 0;
			if (DuplicateHandle((HANDLE)(-1), hThreadHandle, (HANDLE)(-1), &hNewThreadHandle, THREAD_ALL_ACCESS, FALSE, DUPLICATE_SAME_ACCESS))
			{
				DetourUpdateThreadEx(hThreadHandle, FALSE, TRUE, TRUE);
			}

			++num_threads_frozen;
		}
		
		first_run = false;

	} while (num_threads_frozen != 0);
}

void MH_TransactionHookCommit(void)
{
	for (auto pHook = g_pHookBase; pHook; pHook = pHook->pNext)
	{
		if (!pHook->bCommitted)
		{
			if (pHook->iType == MH_HOOK_INLINE)
			{
				DetourTransactionBegin();
				DetourAttach(&pHook->hookData.inlinehook.pTrampolineCall, pHook->pNewFuncAddr);
				DetourTransactionCommit();

				if (pHook->pOrginalCall)
					(*pHook->pOrginalCall) = pHook->hookData.inlinehook.pTrampolineCall;
			}
			else if (pHook->iType == MH_HOOK_VFTABLE)
			{
				pHook->pOldFuncAddr = *pHook->hookData.vfthook.pVirtualFuncAddr;

				MH_WriteMemory(pHook->hookData.vfthook.pVirtualFuncAddr, &pHook->pNewFuncAddr, sizeof(PVOID));

				if (pHook->pOrginalCall)
					(*pHook->pOrginalCall) = pHook->pOldFuncAddr;
			}
			else if (pHook->iType == MH_HOOK_IAT)
			{
				pHook->pOldFuncAddr = *pHook->hookData.iathook.pImportFuncAddr;

				MH_WriteMemory(pHook->hookData.iathook.pImportFuncAddr, &pHook->pNewFuncAddr, sizeof(PVOID));

				if (pHook->pOrginalCall)
					(*pHook->pOrginalCall) = pHook->pOldFuncAddr;
			}
			else if (pHook->iType == MH_HOOK_INLINEPATCH)
			{
				pHook->pOldFuncAddr = MH_GetNextCallAddr(pHook->hookData.inlinepatch.pInstructionAddress, 1);

				memcpy(pHook->hookData.inlinepatch.OriginalBytes, (PUCHAR)pHook->hookData.inlinepatch.pInstructionAddress, pHook->hookData.inlinepatch.PatchLength);
				MH_WriteMemory(pHook->hookData.inlinepatch.pInstructionAddress, pHook->hookData.inlinepatch.NewCodeBytes, pHook->hookData.inlinepatch.PatchLength);

				if (pHook->pOrginalCall)
					(*pHook->pOrginalCall) = pHook->pOldFuncAddr;
			}

			pHook->bCommitted = true;
		}
	}

	//MH_TransactionHookUpdateThreads();

	g_bTransactionHook = false;
}

int __fastcall CheckStudioInterfaceTrampoline(int(*pfn)(int version, struct r_studio_interface_t** ppinterface, struct engine_studio_api_s* pstudio), int dummy)
{
	int r = 0;

	MH_TransactionHookBegin();

	r = pfn ? pfn(1, g_pStudioAPI, g_pEngineStudioAPI) : 0;

	MH_TransactionHookCommit();

	return r;
}

int ClientDLL_Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(gMetaSave.pExportFuncs, g_pExportFuncs, sizeof(cl_exportfuncs_t));
	memcpy(gMetaSave.pEngineFuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	if (g_phClientModule && (*g_phClientModule))
	{
		if (g_hMirrorClient)
		{
			MH_FreeMirrorDLL(g_hMirrorClient);
			g_hMirrorClient = NULL;
		}

		g_hMirrorClient = MH_LoadMirrorDLL_FileSystem("cl_dlls\\client.dll");
	}

	MH_TransactionHookBegin();

	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next)
	{
		switch (plug->iInterfaceVersion)
		{
		case 4:
			((IPluginsV4 *)plug->pPluginAPI)->LoadClient(g_pExportFuncs);
			break;
		case 3:
			((IPluginsV3 *)plug->pPluginAPI)->LoadClient(g_pExportFuncs);
			break;
		case 2:
			((IPluginsV2 *)plug->pPluginAPI)->LoadClient(g_pExportFuncs);
			break;
		default:
			((IPluginsV1 *)plug->pPluginAPI)->Init(g_pExportFuncs);
			break;
		}
	}

	MH_TransactionHookCommit();

	gMetaSave.pEngineFuncs->pfnAddCommand("mh_pluginlist", MH_PrintPluginList);

	return g_pExportFuncs->Initialize(pEnginefuncs, iVersion);
}

void MH_ResetAllVars(void)
{
	Cmd_GetCmdBase = NULL;
	cvar_callbacks = NULL;
	gClientUserMsgs = NULL;
	g_pVideoMode = NULL;
	cl_parsefuncs = NULL;
	g_pfnbuild_number = NULL;
	g_pfnSys_Error = NULL;
	g_pClientFactory = NULL;
	g_pfnClientDLL_Init = NULL;
	g_pfnCvar_DirectSet = NULL;
	g_pfnLoadBlobFile = NULL;
	g_pfnFreeBlob = NULL;
	g_StudioInterfaceCall = NULL;
	g_pEngineStudioAPI = NULL;
	g_pStudioAPI = NULL;
	g_phClientModule = NULL;
	g_ppExportFuncs = NULL;
	g_ppEngfuncs = NULL;
	g_hEngineModule = NULL;
	g_hBlobEngine = NULL;
	g_hBlobClient = NULL;
	g_dwEngineBase = NULL;
	g_dwEngineSize = NULL;
	g_pHookBase = NULL;
	g_pExportFuncs = NULL;
	g_bSaveVideo = false;
	g_iEngineType = ENGINE_UNKNOWN;
	g_ThreadPool = NULL;
	memset(g_szGameDirectory, 0, sizeof(g_szGameDirectory));
}

bool MH_GetModuleFilePathA(HMODULE hModule, std::string& filePath)
{
	std::string ModulePath(MAX_PATH, '\0');

	DWORD length = GetModuleFileNameA(hModule, &ModulePath[0], ModulePath.size());

	if (length == 0)
		return false;

	while (length >= ModulePath.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		ModulePath.resize(length + 1024);
		length = GetModuleFileNameA(hModule, &ModulePath[0], ModulePath.size());
	}

	ModulePath.resize(length);
	filePath = ModulePath;
	return true;
}

#define RVA_from_VA(name, dllinfo) (ULONG)((ULONG_PTR)name##_VA - (ULONG_PTR)dllinfo.ImageBase)
#define VA_from_RVA(name, dllinfo) ((ULONG_PTR)dllinfo.ImageBase + (ULONG_PTR)name##_RVA)
#define Convert_VA_to_RVA(name, dllinfo) if(name##_VA) name##_RVA = ((ULONG_PTR)name##_VA - (ULONG_PTR)dllinfo.ImageBase)
#define Convert_RVA_to_VA(name, dllinfo) if(name##_RVA) name##_VA = (decltype(name##_VA))VA_from_RVA(name, dllinfo)

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
	if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
	{
		auto addr_VA = (ULONG_PTR)addr;
		auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

		return (PVOID)VA_from_RVA(addr, TargetDllInfo);
	}

	return nullptr;
}

void MH_LoadEngine_FindBuildNumber(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
#define BUILD_NUMBER_SIG "\xE8\x2A\x2A\x2A\x2A\x50\x68\x2A\x2A\x2A\x2A\x6A\x30\x68"

	auto buildnumber_call = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, BUILD_NUMBER_SIG, sizeof(BUILD_NUMBER_SIG) - 1);

	if (buildnumber_call)
	{
		auto buildnumber_VA = MH_GetNextCallAddr(buildnumber_call, 1);
		g_pfnbuild_number = (decltype(g_pfnbuild_number))ConvertDllInfoSpace(buildnumber_VA, DllInfo, RealDllInfo);
	}

	if (!g_pfnbuild_number)
	{
#define EXE_BUILD_STRING_SIG "Exe build: "
		auto ExeBuild_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, EXE_BUILD_STRING_SIG, sizeof(EXE_BUILD_STRING_SIG) - 1);
		if (!ExeBuild_String)
			ExeBuild_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, EXE_BUILD_STRING_SIG, sizeof(EXE_BUILD_STRING_SIG) - 1);
		if (ExeBuild_String)
		{
			char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x50\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 7) = (DWORD)ExeBuild_String;
			auto ExeBuild_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (ExeBuild_PushString)
			{
				auto buildnumber_VA = MH_GetNextCallAddr(ExeBuild_PushString, 1);
				g_pfnbuild_number = (decltype(g_pfnbuild_number))ConvertDllInfoSpace(buildnumber_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!g_pfnbuild_number)
	{
		MH_SysError("MH_LoadEngine: Failed to locate buildnumber");
	}
}

void MH_LoadEngine_FindEngineType(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//Judge actual engine type
	if (g_iEngineType == ENGINE_UNKNOWN)
	{
		auto factory = MH_GetEngineFactory();

		if (factory("SCEngineClient002", NULL) ||
			factory("SCEngineClient001", NULL))
		{
			g_iEngineType = ENGINE_SVENGINE;
		}
		else
		{
			if (g_pfnbuild_number() > 9000)
			{
				g_iEngineType = ENGINE_GOLDSRC_HL25;
			}
			else
			{
#define HALF_LIFE_STRING_SIG "Half-Life %i/%s (hw build %d)"
				if (MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, HALF_LIFE_STRING_SIG, sizeof(HALF_LIFE_STRING_SIG) - 1))
				{
					g_iEngineType = ENGINE_GOLDSRC;
				}
			}
		}
	}
}

void MH_LoadEngine_FindSysError(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define COULD_NOT_LINK_STRING_SIG_SVENGINE "Couldn't link client library function \"Initialize\"\n"
		auto CouldNotLink_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, COULD_NOT_LINK_STRING_SIG_SVENGINE, sizeof(COULD_NOT_LINK_STRING_SIG_SVENGINE) - 1);
		if (!CouldNotLink_String)
			CouldNotLink_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, COULD_NOT_LINK_STRING_SIG_SVENGINE, sizeof(COULD_NOT_LINK_STRING_SIG_SVENGINE) - 1);
		if (CouldNotLink_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)CouldNotLink_String;
			auto CouldNotLink_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (CouldNotLink_PushString)
			{
				PVOID Sys_Error_VA = MH_GetNextCallAddr((PUCHAR)CouldNotLink_PushString + 5, 1);
				g_pfnSys_Error = (decltype(g_pfnSys_Error))ConvertDllInfoSpace(Sys_Error_VA, DllInfo, RealDllInfo);
			}
		}
	}
	else
	{
#define COULD_NOT_LINK_STRING_SIG_GOLDSRC "could not link client.dll function Initialize\n\0"
		auto CouldNotLink_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, COULD_NOT_LINK_STRING_SIG_GOLDSRC, sizeof(COULD_NOT_LINK_STRING_SIG_GOLDSRC) - 1);
		if (!CouldNotLink_String)
			CouldNotLink_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, COULD_NOT_LINK_STRING_SIG_GOLDSRC, sizeof(COULD_NOT_LINK_STRING_SIG_GOLDSRC) - 1);
		if (CouldNotLink_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)CouldNotLink_String;
			auto CouldNotLink_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (CouldNotLink_PushString)
			{
				PVOID Sys_Error_VA = MH_GetNextCallAddr((PUCHAR)CouldNotLink_PushString + 5, 1);;
				g_pfnSys_Error = (decltype(g_pfnSys_Error))ConvertDllInfoSpace(Sys_Error_VA, DllInfo, RealDllInfo);
			}
		}
	}
}

void MH_LoadEngine_FindClientDLL_Init(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
#define CLDLL_INIT_STRING_SIG "ScreenShake"
		auto ClientDll_Init_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, CLDLL_INIT_STRING_SIG, sizeof(CLDLL_INIT_STRING_SIG) - 1);
		if (!ClientDll_Init_String)
			ClientDll_Init_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, CLDLL_INIT_STRING_SIG, sizeof(CLDLL_INIT_STRING_SIG) - 1);
		if (ClientDll_Init_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 6) = (DWORD)ClientDll_Init_String;
			auto ClientDll_Init_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (ClientDll_Init_PushString)
			{
				auto ClientDll_Init_FunctionBase = MH_ReverseSearchFunctionBeginEx(ClientDll_Init_PushString, 0x200, [](PUCHAR Candidate) {
					//  .text : 01D19E10 81 EC 04 02 00 00                                   sub     esp, 204h
					//	.text : 01D19E16 A1 E8 F0 ED 01                                      mov     eax, ___security_cookie
					//	.text : 01D19E1B 33 C4 xor eax, esp
					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00 &&
						Candidate[6] == 0xA1 &&
						Candidate[11] == 0x33 &&
						Candidate[12] == 0xC4)
						return TRUE;

					//.text:01D0AF60 81 EC 00 04 00 00                                   sub     esp, 400h
					//.text : 01D0AF66 8D 84 24 00 02 00 00                                lea     eax, [esp + 400h + Dest]
					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00 &&
						Candidate[6] == 0x8D &&
						Candidate[8] == 0x24)
						return TRUE;

					//  .text : 01D0B180 55                                                  push    ebp
					//	.text : 01D0B181 8B EC                                               mov     ebp, esp
					//	.text : 01D0B183 81 EC 00 02 00 00                                   sub     esp, 200h
					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC &&
						Candidate[3] == 0x81 &&
						Candidate[4] == 0xEC &&
						Candidate[7] == 0x00 &&
						Candidate[8] == 0x00)
						return TRUE;

					return FALSE;
				});

				if (ClientDll_Init_FunctionBase)
				{
					g_pfnClientDLL_Init = (decltype(g_pfnClientDLL_Init))ConvertDllInfoSpace(ClientDll_Init_FunctionBase, DllInfo, RealDllInfo);

					typedef struct ClientDll_Init_SearchContext_s
					{
						const mh_dll_info_t& DllInfo;
						const mh_dll_info_t& RealDllInfo;
					}ClientDll_Init_SearchContext;

					ClientDll_Init_SearchContext ctx = { DllInfo , RealDllInfo };

					MH_DisasmRanges(ClientDll_Init_PushString, 0x30, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx = (ClientDll_Init_SearchContext*)context;

						if (address[0] == 0x6A && address[1] == 0x07 && address[2] == 0x68)
						{
							g_ppEngfuncs = (decltype(g_ppEngfuncs))ConvertDllInfoSpace(address + 3, ctx->DllInfo, ctx->RealDllInfo);
						}
						else if (address[0] == 0xFF && address[1] == 0x15)
						{
							g_ppExportFuncs = (decltype(g_ppExportFuncs))ConvertDllInfoSpace(address + 2, ctx->DllInfo, ctx->RealDllInfo);
						}

						if (g_ppExportFuncs && g_ppEngfuncs)
							return TRUE;

						if (address[0] == 0xCC)
							return TRUE;

						return FALSE;
					}, 0, &ctx);
				}
			}
		}
	}

	if (!g_pfnClientDLL_Init)
	{
		MH_SysError("MH_LoadEngine: Failed to locate ClientDLL_Init");
		return;
	}

	if (!g_ppEngfuncs)
	{
		MH_SysError("MH_LoadEngine: Failed to locate ppEngfuncs");
		return;
	}

	if (!g_ppExportFuncs)
	{
		MH_SysError("MH_LoadEngine: Failed to locate ppExportFuncs");
		return;
	}

	memcpy(gMetaSave.pEngineFuncs, *(void**)g_ppEngfuncs, sizeof(cl_enginefunc_t));

	Cmd_GetCmdBase = (decltype(Cmd_GetCmdBase))gMetaSave.pEngineFuncs->GetFirstCmdFunctionHandle;
}

void MH_LoadEngine_FindClientDLL_HudInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
#define RIGHTHAND_STRING_SIG "cl_righthand\0"
		auto RightHand_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, RIGHTHAND_STRING_SIG, sizeof(RIGHTHAND_STRING_SIG) - 1);
		if (!RightHand_String)
			RightHand_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, RIGHTHAND_STRING_SIG, sizeof(RIGHTHAND_STRING_SIG) - 1);
		if (RightHand_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 1) = (DWORD)RightHand_String;
			auto RightHand_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (RightHand_PushString)
			{
#define HUDINIT_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A"
				auto ClientDLL_HudInit = MH_ReverseSearchPattern(RightHand_PushString, 0x100, HUDINIT_SIG, sizeof(HUDINIT_SIG) - 1);
				if (ClientDLL_HudInit)
				{
					PVOID pfnHUDInit = *(PVOID*)((PUCHAR)ClientDLL_HudInit + 1);

					ClientDLL_HudInit = (PUCHAR)ClientDLL_HudInit + sizeof(HUDINIT_SIG) - 1;

					typedef struct ClientDLL_HudInit_SearchContext_s
					{
						const mh_dll_info_t& DllInfo;
						const mh_dll_info_t& RealDllInfo;
						PVOID pfnHUDInit{};
					}ClientDLL_HudInit_SearchContext;

					ClientDLL_HudInit_SearchContext ctx = { DllInfo , RealDllInfo, pfnHUDInit };

					MH_DisasmRanges(ClientDLL_HudInit, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
						
						auto pinst = (cs_insn*)inst;
						auto ctx = (ClientDLL_HudInit_SearchContext*)context;

						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].type == X86_OP_MEM &&
							pinst->detail->x86.operands[1].mem.base == 0 &&
							(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
						{
							PVOID target = (PVOID)pinst->detail->x86.operands[1].mem.disp;

							if (target != ctx->pfnHUDInit)
							{
								g_phClientModule = (decltype(g_phClientModule))ConvertDllInfoSpace(target, ctx->DllInfo, ctx->RealDllInfo);
							}
						}

						if (g_phClientModule)
							return TRUE;

						if (address[0] == 0xCC)
							return TRUE;

						return FALSE;

					}, 0, &ctx);
				}
			}
			else
			{
				MH_SysError("MH_LoadEngine: Failed to locate push cl_righthand string");
				return;
			}
		}
		else
		{
			MH_SysError("MH_LoadEngine: Failed to locate cl_righthand");
			return;
		}
	}

	if (!g_phClientModule)
	{
		MH_SysError("MH_LoadEngine: Failed to locate g_hClientModule");
		return;
	}
}

void MH_LoadEngine_FindVClientVGUI(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
#define VGUICLIENT001_STRING_SIG "VClientVGUI001\0"
		auto VGUIClient001_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, VGUICLIENT001_STRING_SIG, sizeof(VGUICLIENT001_STRING_SIG) - 1);
		if (!VGUIClient001_String)
			VGUIClient001_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, VGUICLIENT001_STRING_SIG, sizeof(VGUICLIENT001_STRING_SIG) - 1);
		if (VGUIClient001_String)
		{
			char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)VGUIClient001_String;
			auto VGUIClient001_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (VGUIClient001_PushString)
			{
#define INITVGUI_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A"
				auto InitVGUI = MH_ReverseSearchPattern(VGUIClient001_PushString, 0x100, INITVGUI_SIG, sizeof(INITVGUI_SIG) - 1);
				if (InitVGUI)
				{
					PVOID g_pClientFactory_VA = ((PUCHAR)InitVGUI + 1);

					g_pClientFactory = *(decltype(g_pClientFactory)*)ConvertDllInfoSpace(g_pClientFactory_VA, DllInfo, RealDllInfo) ;
				}
				else
				{
#define INITVGUI_SIG2 "\x83\x3D\x2A\x2A\x2A\x2A\x00\x74\x2A"
					auto InitVGUI = MH_ReverseSearchPattern(VGUIClient001_PushString, 0x100, INITVGUI_SIG2, sizeof(INITVGUI_SIG2) - 1);
					if (InitVGUI)
					{
						PVOID g_pClientFactory_VA = ((PUCHAR)InitVGUI + 2);

						g_pClientFactory = *(decltype(g_pClientFactory)*)ConvertDllInfoSpace(g_pClientFactory_VA, DllInfo, RealDllInfo);
					}
				}
			}
		}
	}

	if (!g_pClientFactory)
	{
		MH_SysError("MH_LoadEngine: Failed to locate ClientFactory");
		return;
	}
}

void MH_LoadEngine_FindVideoMode(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		PVOID VideoMode_SearchBase = NULL;
		if (g_iEngineType == ENGINE_SVENGINE)
		{
#define FULLSCREEN_STRING_SIG_SVENGINE "-fullscreen\0"
			auto FullScreen_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, FULLSCREEN_STRING_SIG_SVENGINE, sizeof(FULLSCREEN_STRING_SIG_SVENGINE) - 1);
			if (!FullScreen_String)
				FullScreen_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, FULLSCREEN_STRING_SIG_SVENGINE, sizeof(FULLSCREEN_STRING_SIG_SVENGINE) - 1);
			if (FullScreen_String)
			{
				char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
				*(DWORD*)(pattern + 1) = (DWORD)FullScreen_String;
				auto FullScreen_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
				if (FullScreen_PushString)
				{
					FullScreen_PushString = (PUCHAR)FullScreen_PushString + sizeof(pattern) - 1;

					VideoMode_SearchBase = FullScreen_PushString;
				}
				else
				{
					MH_SysError("MH_LoadEngine: Failed to locate FullScreen_PushString");
				}
			}
			else
			{
				MH_SysError("MH_LoadEngine: Failed to locate FullScreen_String");
			}
		}
		else
		{
#define GL_STRING_SIG "-gl\0"
			auto FullScreen_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, GL_STRING_SIG, sizeof(GL_STRING_SIG) - 1);
			if (!FullScreen_String)
				FullScreen_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, GL_STRING_SIG, sizeof(GL_STRING_SIG) - 1);

#define FULLSCREEN_STRING_SIG "-fullscreen\0"
			if (!FullScreen_String)
				FullScreen_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, FULLSCREEN_STRING_SIG, sizeof(FULLSCREEN_STRING_SIG) - 1);
			if (!FullScreen_String)
				FullScreen_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, FULLSCREEN_STRING_SIG, sizeof(FULLSCREEN_STRING_SIG) - 1);

			if (FullScreen_String)
			{
				char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
				*(DWORD*)(pattern + 1) = (DWORD)FullScreen_String;
				auto FullScreen_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
				if (FullScreen_PushString)
				{
					FullScreen_PushString = (PUCHAR)FullScreen_PushString + sizeof(pattern) - 1;

					VideoMode_SearchBase = FullScreen_PushString;
				}
				else
				{
					MH_SysError("MH_LoadEngine: Failed to locate FullScreen_PushString");
				}
			}
			else
			{
				MH_SysError("MH_LoadEngine: Failed to locate FullScreen_String");
			}
		}

		if (VideoMode_SearchBase)
		{
			typedef struct VideoMode_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;

				ULONG_PTR candidate_disp{};
				PVOID candidate_addr{};

				int xor_exx_exx_instCount{};
				int xor_exx_exx_reg{};
			}VideoMode_SearchContext;

			VideoMode_SearchContext ctx = { DllInfo, RealDllInfo };

			MH_DisasmRanges(VideoMode_SearchBase, 0x400, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;
				auto ctx = (VideoMode_SearchContext*)context;

				if (pinst->id == X86_INS_XOR &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg)
				{
					ctx->xor_exx_exx_reg = pinst->detail->x86.operands[0].reg;
					ctx->xor_exx_exx_instCount = instCount;
				}

				if ((pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.ImageBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0)
					||
					(pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.ImageBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].reg == X86_REG_EAX)
					||
					(pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.ImageBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].reg == ctx->xor_exx_exx_reg &&
						instCount == ctx->xor_exx_exx_instCount + 1)

					)
				{
					typedef struct FindRet_SearchContext_s
					{
						bool bFindRet{};
					}FindRet_SearchContext;

					FindRet_SearchContext ctx2 = { };

					MH_DisasmRanges(address, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

							auto pinst = (cs_insn*)inst;
							auto ctx = (FindRet_SearchContext*)context;

							if (!ctx->bFindRet && pinst->id == X86_INS_RET)
							{
								ctx->bFindRet = true;
								return TRUE;
							}

							if (address[0] == 0xCC)
								return TRUE;

							if (address[0] == 0x90)
								return TRUE;

							if (instCount > 15)
								return TRUE;

							return FALSE;

					}, 0, &ctx2);

					if (ctx2.bFindRet)
					{
						g_pVideoMode = (decltype(g_pVideoMode))ConvertDllInfoSpace((PVOID) pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}
				}

				if (g_pVideoMode)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				return FALSE;
			}, 0, &ctx);
		}
		else
		{
			MH_SysError("MH_LoadEngine: Failed to locate VideoMode_SearchBase");
			return;
		}
	}

	if (!g_pVideoMode)
	{
		MH_SysError("MH_LoadEngine: Failed to locate g_pVideoMode");
		return;
	}
}

void MH_LoadEngine_FindClientUserMsgs(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
#define HUDTEXT_STRING_SIG "HudText\0"
		auto HudText_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, HUDTEXT_STRING_SIG, sizeof(HUDTEXT_STRING_SIG) - 1);
		if (!HudText_String)
			HudText_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, HUDTEXT_STRING_SIG, sizeof(HUDTEXT_STRING_SIG) - 1);
		if (HudText_String)
		{
			char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
			*(DWORD*)(pattern + 2) = (DWORD)HudText_String;
			auto HudText_PushString = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (HudText_PushString)
			{
				PVOID DispatchDirectUserMsg = (PVOID)MH_GetNextCallAddr((PUCHAR)HudText_PushString + 6, 1);

				typedef struct DispatchDirectUserMsg_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
				}DispatchDirectUserMsg_SearchContext;

				DispatchDirectUserMsg_SearchContext ctx = { DllInfo, RealDllInfo };

				MH_DisasmRanges(DispatchDirectUserMsg, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto pinst = (cs_insn*)inst;
					auto ctx = (DispatchDirectUserMsg_SearchContext*)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.ImageBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize)
					{
						PVOID gClientUserMsgs_VA = (PVOID)pinst->detail->x86.operands[1].mem.disp;

						gClientUserMsgs = (decltype(gClientUserMsgs)) ConvertDllInfoSpace(gClientUserMsgs_VA, ctx->DllInfo, ctx->RealDllInfo);
					}

					if (gClientUserMsgs)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					return FALSE;
				}, 0, &ctx);
			}
		}
	}

	if (!gClientUserMsgs)
	{
		MH_SysError("MH_LoadEngine: Failed to locate gClientUserMsgs");
		return;
	}
}

void MH_LoadEngine_FindParseFuncs(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		char pattern[] = "\x00\x00\x00\x00\x2A\x2A\x2A\x2A\x00\x00\x00\x00\x01\x00\x00\x00\x2A\x2A\x2A\x2A\x00\x00\x00\x00\x02\x00\x00\x00\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x03\x00\x00\x00";
		auto searchBegin = (PUCHAR)DllInfo.DataBase;
		auto searchEnd = (PUCHAR)DllInfo.DataBase + DllInfo.DataSize;
		while (1)
		{
			auto pFound = MH_SearchPattern(searchBegin, searchEnd - searchBegin, pattern, sizeof(pattern) - 1);
			if (pFound)
			{
				auto pString_svc_bad = *(const char**)((PUCHAR)pFound + 4);

				if (((PUCHAR)pString_svc_bad >= (PUCHAR)DllInfo.DataBase && (PUCHAR)pString_svc_bad < (PUCHAR)DllInfo.DataBase + DllInfo.DataSize) ||
					((PUCHAR)pString_svc_bad >= (PUCHAR)DllInfo.RdataBase && (PUCHAR)pString_svc_bad < (PUCHAR)DllInfo.RdataBase + DllInfo.RdataSize))
				{
					if (!memcmp(pString_svc_bad, "svc_bad", sizeof("svc_bad")))
					{
						cl_parsefuncs = (decltype(cl_parsefuncs))ConvertDllInfoSpace(pFound, DllInfo, RealDllInfo);
						break;
					}
				}
				searchBegin = (PUCHAR)pFound + sizeof(pattern) - 1;
			}
			else
			{
				break;
			}
		}
	}

	if (!cl_parsefuncs)
	{
		MH_SysError("MH_LoadEngine: Failed to locate cl_parsefuncs");
		return;
	}
}

void MH_LoadEngine_FindCvarDirectSet(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		const char sigs1[] = "***PROTECTED***";
		auto Cvar_DirectSet_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, sigs1, sizeof(sigs1) - 1);
		if (!Cvar_DirectSet_String)
			Cvar_DirectSet_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, sigs1, sizeof(sigs1) - 1);
		if (Cvar_DirectSet_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 1) = (DWORD)Cvar_DirectSet_String;
			auto Cvar_DirectSet_Call = MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
			if (Cvar_DirectSet_Call)
			{
				PVOID Cvar_DirectSet_VA = MH_ReverseSearchFunctionBeginEx(Cvar_DirectSet_Call, 0x500, [](PUCHAR Candidate) {
					//.text : 01D42120 81 EC 0C 04 00 00                                   sub     esp, 40Ch
					//.text : 01D42126 A1 E8 F0 ED 01                                      mov     eax, ___security_cookie
					//.text : 01D4212B 33 C4
					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00 &&
						Candidate[6] == 0xA1 &&
						Candidate[11] == 0x33 &&
						Candidate[12] == 0xC4)
						return TRUE;

					//.text : 01D2E530 55                                                  push    ebp
					//.text : 01D2E531 8B EC                                               mov     ebp, esp
					//.text : 01D2E533 81 EC 00 04 00 00                                   sub     esp, 400h
					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC &&
						Candidate[3] == 0x81 &&
						Candidate[4] == 0xEC &&
						Candidate[7] == 0x00 &&
						Candidate[8] == 0x00)
						return TRUE;

					//01D311B0 - 8B 4C 24 08           - mov ecx,[esp+08]
					//01D311B4 - 81 EC 00040000        - sub esp,00000400 { 1024 }
					//3248
					if (Candidate[0] == 0x8B &&
						Candidate[1] == 0x4C &&
						Candidate[2] == 0x24 &&
						Candidate[3] == 0x08 &&
						Candidate[4] == 0x81 &&
						Candidate[5] == 0xEC)
						return TRUE;

					//.text:01D2E240 81 EC 00 04 00 00                                   sub     esp, 400h
					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00)
						return TRUE;

					return FALSE;
					});

				g_pfnCvar_DirectSet = (decltype(g_pfnCvar_DirectSet))ConvertDllInfoSpace(Cvar_DirectSet_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!g_pfnCvar_DirectSet)
	{
		MH_SysError("MH_LoadEngine: Failed to locate Cvar_DirectSet");
		return;
	}
}

void MH_LoadEngine_PatchCvarCallbacks(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID Cvar_Set = ConvertDllInfoSpace((void*)gMetaSave.pEngineFuncs->Cvar_Set, RealDllInfo, DllInfo);

	if (Cvar_Set)
	{
		typedef struct Cvar_Set_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}Cvar_Set_SearchContext;

		Cvar_Set_SearchContext ctx = { DllInfo, RealDllInfo };

		MH_DisasmRanges(Cvar_Set, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (Cvar_Set_SearchContext*)context;

			if (!cvar_callbacks)
			{
				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0)
				{
					cvar_callbacks = (decltype(cvar_callbacks))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}
			}

			if (cvar_callbacks)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		if (!cvar_callbacks)
		{
			typedef struct CvarSet_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
				bool bCallManipulated{};
			}CvarSet_SearchContext;

			CvarSet_SearchContext ctx = { DllInfo, RealDllInfo };

			const char sigs1[] = "Cvar_Set: variable %s not found\n";
			auto Cvar_DirectSet_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, sigs1, sizeof(sigs1) - 1);
			if (!Cvar_DirectSet_String)
				Cvar_DirectSet_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, sigs1, sizeof(sigs1) - 1);
			if (Cvar_DirectSet_String)
			{
				char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
				*(DWORD*)(pattern + 1) = (DWORD)Cvar_DirectSet_String;

				auto searchBegin = (PUCHAR)DllInfo.TextBase;
				auto searchEnd = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
				while (1)
				{
					auto Cvar_Set_Call = MH_SearchPattern(searchBegin, searchEnd - searchBegin, pattern, sizeof(pattern) - 1);
					if (Cvar_Set_Call)
					{
						searchBegin = (PUCHAR)Cvar_Set_Call + sizeof(pattern) - 1;

						MH_DisasmRanges(searchBegin, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
							auto pinst = (cs_insn*)inst;
							auto ctx = (CvarSet_SearchContext*)context;

							if (address[0] == 0xE8 || address[0] == 0xE9)
							{
								auto callTarget = (PVOID)pinst->detail->x86.operands[0].imm;
								auto callTarget_RealDllBased = ConvertDllInfoSpace(callTarget, ctx->DllInfo, ctx->RealDllInfo);

								if (callTarget_RealDllBased == g_pfnCvar_DirectSet)
								{
									//auto dwNewRVA = (ULONG_PTR)MH_Cvar_DirectSet - (ULONG_PTR)(address + 5);
									//MH_WriteDWORD(address + 1, dwNewRVA);
									auto address_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

									MH_InlinePatchRedirectBranch(address_RealDllBased, MH_Cvar_DirectSet, NULL);

									ctx->bCallManipulated = true;
								}
							}

							if (address[0] == 0xCC)
								return TRUE;

							if (address[0] == 0x90)
								return TRUE;

							return FALSE;
						}, 0, &ctx);
					}
					else
					{
						break;
					}
				}
			}

			if (!ctx.bCallManipulated) {
				MH_SysError("MH_LoadEngine: Failed to locate call inside Cvar_Set");
				return;
			}

			cvar_callbacks = &g_ManagedCvarCallbackList;
		}
	}
}

void MH_LoadEngine_FindLoadBlobClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_GOLDSRC || g_iEngineType == ENGINE_GOLDSRC_BLOB || g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char pattern[] = "\x85\xBC\x32\x7A\xFF";
		const char pattern2[] = "\x6A\x00\x6A\x01\x6A\x00";

		auto searchBegin = (PUCHAR)DllInfo.TextBase;
		auto searchEnd = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (1)
		{
			auto ExportPoint_Call = MH_SearchPattern(searchBegin, searchEnd - searchBegin, pattern, sizeof(pattern) - 1);
			if (ExportPoint_Call)
			{
				auto ExportPoint_Push = MH_SearchPattern((PUCHAR)ExportPoint_Call - 0x50, 0x50, pattern2, sizeof(pattern2) - 1);
				if (ExportPoint_Push)
				{
					PVOID LoadBlobFile_VA = MH_ReverseSearchFunctionBegin((PUCHAR)ExportPoint_Push, 0x300);
					g_pfnLoadBlobFile = (decltype(g_pfnLoadBlobFile))ConvertDllInfoSpace(LoadBlobFile_VA, DllInfo, RealDllInfo);
						
					break;
				}

				searchBegin = (PUCHAR)ExportPoint_Call + sizeof(pattern) - 1;
			}
			else
			{
				break;
			}
		}

		if (!g_pfnLoadBlobFile) {
			MH_SysError("MH_LoadEngine: Failed to locate LoadBlobFile");
			return;
		}
	}

	if (g_pfnLoadBlobFile)
	{
		const char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x6A\x74";

		auto FreeBlob_Call = (PUCHAR)MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
		if (FreeBlob_Call)
		{
			PVOID FreeBlob_VA = MH_GetNextCallAddr(FreeBlob_Call + 5, 1);
			g_pfnFreeBlob = (decltype(g_pfnFreeBlob))ConvertDllInfoSpace(FreeBlob_VA, DllInfo, RealDllInfo);
		}
		else
		{
			const char pattern2[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A";
			*(ULONG_PTR*)(pattern2 + sizeof(pattern2) - 1 - 4) = (ULONG_PTR)ConvertDllInfoSpace(g_phClientModule, RealDllInfo, DllInfo);

			auto FreeBlob_Call = (PUCHAR)MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern2, sizeof(pattern2) - 1);
			if (FreeBlob_Call)
			{
				PVOID FreeBlob_VA = MH_GetNextCallAddr(FreeBlob_Call + 5, 1);
				g_pfnFreeBlob = (decltype(g_pfnFreeBlob))ConvertDllInfoSpace(FreeBlob_VA, DllInfo, RealDllInfo);
			}
			else
			{
				const char pattern3[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50";
				*(ULONG_PTR*)(pattern3 + sizeof(pattern2) - 1 - 5) = (ULONG_PTR)ConvertDllInfoSpace(g_phClientModule, RealDllInfo, DllInfo);
				auto FreeBlob_Call = (PUCHAR)MH_SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern3, sizeof(pattern3) - 1);
				if (FreeBlob_Call)
				{
					PVOID FreeBlob_VA = MH_GetNextCallAddr(FreeBlob_Call + 5, 1);
					g_pfnFreeBlob = (decltype(g_pfnFreeBlob))ConvertDllInfoSpace(FreeBlob_VA, DllInfo, RealDllInfo);
				}
			}
		}

		if (!g_pfnFreeBlob) {
			MH_SysError("MH_LoadEngine: Failed to locate FreeBlob");
			return;
		}
	}
}

void MH_LoadEngine_FindStudioInterface(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		/*
.text:10196E98 85 C0                                               test    eax, eax
.text:10196E9A 74 27                                               jz      short loc_10196EC3
.text:10196E9C 68 C8 B1 31 10                                      push    offset off_1031B1C8
.text:10196EA1 68 D8 B0 31 10                                      push    offset off_1031B0D8
.text:10196EA6 6A 01                                               push    1
.text:10196EA8 FF D0                                               call    eax ; cl_funcs_pStudioInterface
.text:10196EAA 83 C4 0C                                            add     esp, 0Ch
		*/
		char pattern2[] = "\x85\xC0\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x6A\x01\xFF\x2A\x83\xC4\x0C";
		const char sigs_SvEngine[] = "Couldn't get client library studio model rendering";
		const char sigs_GoldSrc[] = "Couldn't get client .dll studio model rendering";
		const char* sigs = NULL;
		size_t siglen = 0;
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			sigs = sigs_SvEngine;
			siglen = sizeof(sigs_SvEngine) - 1;
		}
		else
		{
			sigs = sigs_GoldSrc;
			siglen = sizeof(sigs_GoldSrc) - 1;
		}

		auto PrintError_String = MH_SearchPattern(DllInfo.DataBase, DllInfo.DataSize, sigs, siglen);
		if (!PrintError_String)
			PrintError_String = MH_SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, sigs, siglen);
		if (PrintError_String)
		{
			*(DWORD*)(pattern + 1) = (DWORD)PrintError_String;

			auto searchBegin = (PUCHAR)DllInfo.TextBase;
			auto searchEnd = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
			while (1)
			{
				auto PrintError_Call = MH_SearchPattern(searchBegin, searchEnd - searchBegin, pattern, sizeof(pattern) - 1);
				if (PrintError_Call)
				{
					auto pStudioInterfaceCall_VA = MH_SearchPattern((PUCHAR)PrintError_Call - 0x50, 0x50, pattern2, sizeof(pattern2) - 1);
					if (pStudioInterfaceCall_VA)
					{
						g_StudioInterfaceCall = ConvertDllInfoSpace(pStudioInterfaceCall_VA, DllInfo, RealDllInfo);

						PVOID g_pEngineStudioAPI_VA = *(PVOID*)((ULONG_PTR)pStudioInterfaceCall_VA + 4 + 1);
						g_pEngineStudioAPI = (decltype(g_pEngineStudioAPI))ConvertDllInfoSpace(g_pEngineStudioAPI_VA, DllInfo, RealDllInfo);

						PVOID g_pStudioAPI_VA = *(PVOID*)((ULONG_PTR)pStudioInterfaceCall_VA + 4 + 5 + 1);
						g_pStudioAPI = (decltype(g_pStudioAPI))ConvertDllInfoSpace(g_pStudioAPI_VA, DllInfo, RealDllInfo);

						break;
					}
				}
				else
				{
					break;
				}
			}
		}

		if (!g_StudioInterfaceCall) {
			MH_SysError("MH_LoadEngine: Failed to locate g_StudioInterfaceCall in ClientDLL_CheckStudioInterface");
			return;
		}
		if (!g_pEngineStudioAPI) {
			MH_SysError("MH_LoadEngine: Failed to locate g_pEngineStudioAPI in ClientDLL_CheckStudioInterface");
			return;
		}
		if (!g_pStudioAPI) {
			MH_SysError("MH_LoadEngine: Failed to locate g_pStudioAPI in ClientDLL_CheckStudioInterface");
			return;
		}
	}
}

void MH_LoadEngine(HMODULE hEngineModule, BlobHandle_t hBlobEngine, const char* szGameName, const char* szFullGamePath)
{
	MH_ResetAllVars();

	if (!gMetaSave.pEngineFuncs)
		gMetaSave.pEngineFuncs = new cl_enginefunc_t;

	memset(gMetaSave.pEngineFuncs, 0, sizeof(cl_enginefunc_t));

	if (!gMetaSave.pExportFuncs)
		gMetaSave.pExportFuncs = new cl_exportfuncs_t;

	memset(gMetaSave.pExportFuncs, 0, sizeof(cl_exportfuncs_t));

	g_dwEngineBase = 0;
	g_dwEngineSize = 0;
	g_pHookBase = NULL;
	g_pExportFuncs = NULL;
	g_bSaveVideo = false;

	strncpy(g_szGameDirectory, szGameName, sizeof(g_szGameDirectory) - 1);
	g_szGameDirectory[sizeof(g_szGameDirectory) - 1] = 0;

	gInterface.CommandLine = CommandLine();
	gInterface.FileSystem = g_pFileSystem;
	gInterface.Registry = registry;
	gInterface.FileSystem_HL25 = g_pFileSystem_HL25;
	gInterface.MetaHookAPIVersion = METAHOOK_API_VERSION;

	g_ThreadPool = MH_CreateThreadPool(1, 8);

	if (hEngineModule)
	{
		g_dwEngineBase = MH_GetModuleBase(hEngineModule);
		g_dwEngineSize = MH_GetModuleSize(hEngineModule);
		g_hEngineModule = hEngineModule;

		std::string EngineDllPath;
		if (MH_GetModuleFilePathA(hEngineModule, EngineDllPath))
		{
			g_hMirrorEngine = MH_LoadMirrorDLL_Std(EngineDllPath.c_str());
		}

		g_iEngineType = ENGINE_UNKNOWN;
	}
	else
	{
		g_dwEngineBase = GetBlobModuleImageBase(hBlobEngine);
		g_dwEngineSize = GetBlobModuleImageSize(hBlobEngine);
		g_hBlobEngine = hBlobEngine;
		g_hMirrorEngine = NULL;

		g_iEngineType = ENGINE_GOLDSRC_BLOB;
	}

	mh_dll_info_t EngineDllInfo = { 0 };

	if (g_dwEngineBase)
	{
		EngineDllInfo.ImageBase = g_dwEngineBase;
		EngineDllInfo.ImageSize = g_dwEngineSize;

		EngineDllInfo.TextBase = MH_GetSectionByName(EngineDllInfo.ImageBase, ".text\0\0\0", &EngineDllInfo.TextSize);
		EngineDllInfo.DataBase = MH_GetSectionByName(EngineDllInfo.ImageBase, ".data\0\0\0", &EngineDllInfo.DataSize);
		EngineDllInfo.RdataBase = MH_GetSectionByName(EngineDllInfo.ImageBase, ".rdata\0\0", &EngineDllInfo.RdataSize);
	}

	mh_dll_info_t MirrorEngineDllInfo = { 0 };

	if (g_hMirrorEngine)
	{
		MirrorEngineDllInfo.ImageBase = MH_GetMirrorDLLBase(g_hMirrorEngine);
		MirrorEngineDllInfo.ImageSize = MH_GetMirrorDLLSize(g_hMirrorEngine);

		MirrorEngineDllInfo.TextBase = MH_GetSectionByName(MirrorEngineDllInfo.ImageBase, ".text\0\0\0", &MirrorEngineDllInfo.TextSize);
		MirrorEngineDllInfo.DataBase = MH_GetSectionByName(MirrorEngineDllInfo.ImageBase, ".data\0\0\0", &MirrorEngineDllInfo.DataSize);
		MirrorEngineDllInfo.RdataBase = MH_GetSectionByName(MirrorEngineDllInfo.ImageBase, ".rdata\0\0", &MirrorEngineDllInfo.RdataSize);
	}

	MH_LoadEngine_FindBuildNumber(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindEngineType(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindSysError(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindClientDLL_Init(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindClientDLL_HudInit(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindVClientVGUI(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindVideoMode(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindClientUserMsgs(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindParseFuncs(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindCvarDirectSet(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_PatchCvarCallbacks(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindLoadBlobClient(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);
	MH_LoadEngine_FindStudioInterface(MirrorEngineDllInfo.ImageBase ? MirrorEngineDllInfo : EngineDllInfo, EngineDllInfo);

	//Hook client dll initialization
	g_pExportFuncs = *(cl_exportfuncs_t**)g_ppExportFuncs;

	g_dwClientDLL_Initialize[0] = (ULONG_PTR)ClientDLL_Initialize;

	MH_WriteDWORD(g_ppExportFuncs, (DWORD)g_dwClientDLL_Initialize);

	//Hook studio interface initialization
	char CheckStudioInterfaceNewCall[] = "\x8B\xC8\xE8\x2A\x2A\x2A\x2A\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";
	*(int*)(CheckStudioInterfaceNewCall + 2 + 1) = ((PUCHAR)CheckStudioInterfaceTrampoline) - ((PUCHAR)g_StudioInterfaceCall + 2 + 5);
	MH_WriteMemory(g_StudioInterfaceCall, CheckStudioInterfaceNewCall, sizeof(CheckStudioInterfaceNewCall) - 1);

	//8B C8          mov     ecx, eax
	//E8 ?? ?? ?? ?? call
	if (g_pfnLoadBlobFile)
	{
		MH_InlineHook(g_pfnLoadBlobFile, MH_LoadBlobFile, NULL);
	}

	/*
.text:01D63B62 68 2C E2 3C 02                                      push    offset g_pBlobFootprint
.text:01D63B67 E8 54 E2 FF FF                                      call    UnloadBlob
.text:01D63B6C 6A 74                                               push    74h ; 't'
	*/
	if (g_pfnFreeBlob)
	{
		MH_InlineHook(g_pfnFreeBlob, MH_FreeBlobProxy, NULL);
	}

	MH_LoadDllPaths(szGameName, szFullGamePath);
	MH_LoadPlugins(szGameName, szFullGamePath);

	MH_TransactionHookBegin();

	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next)
	{
		switch (plug->iInterfaceVersion)
		{
		case 4:
			((IPluginsV4 *)plug->pPluginAPI)->LoadEngine((cl_enginefunc_t *)*(void **)g_ppEngfuncs);
			break;
		case 3:
			((IPluginsV3 *)plug->pPluginAPI)->LoadEngine((cl_enginefunc_t *)*(void **)g_ppEngfuncs);
			break;
		case 2:
			((IPluginsV2 *)plug->pPluginAPI)->LoadEngine();
			break;
		default:
			break;
		}
	}

	if (hBlobEngine)
	{
		MH_DispatchLoadBlobNotificationCallback(hBlobEngine, LOAD_DLL_NOTIFICATION_IS_LOAD);
	}
	else
	{
		MH_DispatchLoadLdrDllNotificationCallback(NULL, NULL, g_dwEngineBase, g_dwEngineSize, LOAD_DLL_NOTIFICATION_IS_LOAD);
	}

	MH_TransactionHookCommit();

	InitLoadDllNotification();
}

hook_t *MH_NewHook(int iType)
{
	auto h = new (std::nothrow) hook_t;

	if (!h)
		return NULL;

	memset(h, 0, sizeof(hook_t));
	h->iType = iType;
	h->pNext = g_pHookBase;

	g_pHookBase = h;

	return h;
}

hook_t *MH_FindInlineHook(void *pOldFuncAddr, hook_t* pLastFoundHook)
{
	auto h = g_pHookBase;

	if (pLastFoundHook)
		h = pLastFoundHook->pNext;

	for (; h; h = h->pNext)
	{
		if (h->iType == MH_HOOK_INLINE)
		{
			if (h->pOldFuncAddr == pOldFuncAddr)
				return h;
		}
	}

	return NULL;
}

hook_t *MH_FindVFTHook(void * pClassInstance, int iTableIndex, int iFuncIndex, hook_t* pLastFoundHook)
{
	auto h = g_pHookBase;

	if (pLastFoundHook)
		h = pLastFoundHook->pNext;

	for (; h; h = h->pNext)
	{
		if (h->iType == MH_HOOK_VFTABLE)
		{
			if (h->hookData.vfthook.pClassInstance == pClassInstance && h->hookData.vfthook.iTableIndex == iTableIndex && h->hookData.vfthook.iFuncIndex == iFuncIndex)
			{
				return h;
			}
		}
	}

	return NULL;
}

hook_t* MH_FindVFTHookEx(void** pVFTable, int iFuncIndex, hook_t* pLastFoundHook)
{
	auto h = g_pHookBase;

	if (pLastFoundHook)
		h = pLastFoundHook->pNext;

	for (; h; h = h->pNext)
	{
		if (h->iType == MH_HOOK_VFTABLE)
		{
			if (h->hookData.vfthook.pVirtualFuncTable == pVFTable && h->hookData.vfthook.iFuncIndex == iFuncIndex)
			{
				return h;
			}
		}
	}

	return NULL;
}

hook_t *MH_FindIATHook(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, hook_t* pLastFoundHook)
{
	auto h = g_pHookBase;

	if (pLastFoundHook)
		h = pLastFoundHook->pNext;

	for (; h; h = h->pNext)
	{
		if (h->iType == MH_HOOK_IAT)
		{
			if (h->hookData.iathook.hModule == hModule && 0 == stricmp(h->hookData.iathook.szModuleName, pszModuleName) && 0 == stricmp(h->hookData.iathook.szFuncName, pszFuncName))
				return h;
		}
	}

	return NULL;
}

hook_t* MH_FindInlinePatchHook(void* pInstructionAddress, hook_t* pLastFoundHook)
{
	auto h = g_pHookBase;

	if (pLastFoundHook)
		h = pLastFoundHook->pNext;

	for (; h; h = h->pNext)
	{
		if (h->iType == MH_HOOK_INLINEPATCH)
		{
			if (h->hookData.inlinepatch.pInstructionAddress == pInstructionAddress)
				return h;
		}
	}

	return NULL;
}

void MH_FreeHook(hook_t *pHook)
{
	if (pHook->bCommitted)
	{
		if (pHook->iType == MH_HOOK_INLINE)
		{
			DetourTransactionBegin();
			DetourDetach(&pHook->hookData.inlinehook.pTrampolineCall, pHook->pNewFuncAddr);
			DetourTransactionCommit();
		}
		else if (pHook->iType == MH_HOOK_VFTABLE)
		{
			MH_WriteMemory(pHook->hookData.vfthook.pVirtualFuncAddr, &pHook->pOldFuncAddr, sizeof(PVOID));
		}
		else if (pHook->iType == MH_HOOK_IAT)
		{
			MH_WriteMemory(pHook->hookData.iathook.pImportFuncAddr, &pHook->pOldFuncAddr, sizeof(PVOID));
		}
		else if (pHook->iType == MH_HOOK_INLINEPATCH)
		{
			MH_WriteMemory(pHook->hookData.inlinepatch.pInstructionAddress, pHook->hookData.inlinepatch.OriginalBytes, pHook->hookData.inlinepatch.PatchLength);
		}

		pHook->bCommitted = false;
	}

	delete pHook;
}

void MH_FreeAllHook(void)
{
	if (!g_pHookBase)
		return;

	hook_t *next = NULL;

	for (auto h = g_pHookBase; h; h = next)
	{
		next = h->pNext;
		MH_FreeHook(h);
	}

	g_pHookBase = NULL;
}

BOOL MH_UnHook(hook_t *pHook)
{
	if (!g_pHookBase)
		return FALSE;

	hook_t *h, **back;
	back = &g_pHookBase;

	while (1)
	{
		h = *back;

		if (!h)
			break;

		if (h == pHook)
		{
			*back = h->pNext;
			MH_FreeHook(h);
			return TRUE;
		}

		back = &h->pNext;
	}

	return FALSE;
}

void MH_ExitGame(int iResult)
{
	for (plugin_t* plug = g_pPluginBase; plug; plug = plug->next)
	{
		switch (plug->iInterfaceVersion)
		{
		case 4:
			((IPluginsV4*)plug->pPluginAPI)->ExitGame(iResult);
			break;
		case 3:
			((IPluginsV3*)plug->pPluginAPI)->ExitGame(iResult);
			break;
		case 2:
			((IPluginsV2*)plug->pPluginAPI)->ExitGame(iResult);
			break;
		default:
			break;
		}
}

	//TODO check if there is any inlinehook left?
	MH_FreeAllHook();

	//Clear all built-in cvar callbacks
	if (cvar_callbacks)
	{
		(*cvar_callbacks) = NULL;
	}

	if (g_hMirrorClient)
	{
		MH_FreeMirrorDLL(g_hMirrorClient);
		g_hMirrorClient = NULL;
	}

	if (g_hMirrorEngine)
	{
		MH_FreeMirrorDLL(g_hMirrorEngine);
		g_hMirrorEngine = NULL;
	}
}

void MH_ShutdownPlugins(void)
{
	if (!g_pPluginBase)
		return;

	auto plug = g_pPluginBase;

	while (plug)
	{
		auto p = plug;
		plug = plug->next;

		if (p->pPluginAPI)
		{
			switch (p->iInterfaceVersion)
			{
			case 4:
				((IPluginsV4*)p->pPluginAPI)->Shutdown();
				break;
			case 3:
				((IPluginsV3*)p->pPluginAPI)->Shutdown();
				break;
			case 2:
				((IPluginsV2*)p->pPluginAPI)->Shutdown();
				break;
			default:
				break;
			}
		}

		Sys_FreeModule(p->module);
		delete p;
	}

	g_pPluginBase = NULL;
}

void MH_Shutdown(void)
{
	ShutdownLoadDllNotification();

	MH_ShutdownPlugins();

	if (gMetaSave.pExportFuncs)
	{
		delete gMetaSave.pExportFuncs;
		gMetaSave.pExportFuncs = NULL;
	}

	if (gMetaSave.pEngineFuncs)
	{
		delete gMetaSave.pEngineFuncs;
		gMetaSave.pEngineFuncs = NULL;
	}

	g_ManagedCvarCallbackList = NULL;

	for (auto p : g_ManagedCvarCallbacks)
	{
		delete p;
	}

	g_ManagedCvarCallbacks.clear();

	MH_ResetAllVars();
	MH_RemoveDllPaths();
	MH_ClearDllLoaderNotificationCallback();

	if (g_ThreadPool) {
		MH_DeleteThreadPool(g_ThreadPool);
		g_ThreadPool = nullptr;
	}
}

hook_t *MH_InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void **pOrginalCall)
{
#if 0
	auto p = (PUCHAR)_ReturnAddress();

	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(p, &mbi, sizeof(mbi));

	if (mbi.Type == MEM_IMAGE)
	{
		char modname[256] = { 0 };
		GetModuleFileNameA((HMODULE)mbi.AllocationBase, modname, sizeof(modname));

		char test[256];
		sprintf(test, "%p called MH_InlineHook, from %p to %p, %s+%X\n", p, pOldFuncAddr, pNewFuncAddr, modname, p - (PUCHAR)mbi.AllocationBase);
		OutputDebugStringA(test);
	}
#endif

	hook_t *h = MH_NewHook(MH_HOOK_INLINE);

	if (!h)
	{
		return NULL;
	}

	h->pOldFuncAddr = pOldFuncAddr;
	h->pNewFuncAddr = pNewFuncAddr;
	h->pOrginalCall = pOrginalCall;

	h->hookData.inlinehook.pTrampolineCall = pOldFuncAddr;

	if (g_bTransactionHook)
	{
		h->bCommitted = false;
	}
	else
	{
		DetourTransactionBegin();
		DetourAttach(&(void *&)h->hookData.inlinehook.pTrampolineCall, pNewFuncAddr);
		DetourTransactionCommit();

		if (h->pOrginalCall)
		{
			(*h->pOrginalCall) = h->hookData.inlinehook.pTrampolineCall;
		}

		h->bCommitted = true;
	}

	return h;
}

bool MH_IsBogusVFTableEntry(PVOID pVirtualFuncAddr, PVOID pOldFuncAddr)
{
	if (1)
	{
		if (pVirtualFuncAddr >= g_dwEngineBase && pVirtualFuncAddr < (PUCHAR)g_dwEngineBase + g_dwEngineSize &&
			pOldFuncAddr >= g_dwEngineBase && pOldFuncAddr < (PUCHAR)g_dwEngineBase + g_dwEngineSize)
		{
			ULONG TextSize = 0;
			PVOID TextBase = MH_GetSectionByName(g_dwEngineBase, ".text\0\0\0", &TextSize);

			if (TextBase)
			{
				if (!(pOldFuncAddr >= TextBase && pOldFuncAddr < (PUCHAR)TextBase + TextSize))
				{
					return true;
				}
			}

			return false;
		}
	}

	if (1)
	{
		auto ClientBase = MH_GetClientBase();
		auto ClientSize = MH_GetClientSize();
		if (pVirtualFuncAddr >= ClientBase && pVirtualFuncAddr < (PUCHAR)ClientBase + ClientSize &&
			pOldFuncAddr >= ClientBase && pOldFuncAddr < (PUCHAR)ClientBase + ClientSize)
		{
			ULONG TextSize = 0;
			PVOID TextBase = MH_GetSectionByName(ClientBase, ".text\0\0\0", &TextSize);

			if (TextBase)
			{
				if (!(pOldFuncAddr >= TextBase && pOldFuncAddr < (PUCHAR)TextBase + TextSize))
				{
					return true;
				}
			}

			return false;
		}
	}

	auto ModuleBase = MH_GetModuleBase(pVirtualFuncAddr);
	if (ModuleBase)
	{
		auto ModuleSize = MH_GetModuleSize(ModuleBase);

		if (ModuleSize > 0)
		{
			if (pVirtualFuncAddr >= ModuleBase && pVirtualFuncAddr < (PUCHAR)ModuleBase + ModuleSize &&
				pOldFuncAddr >= ModuleBase && pOldFuncAddr < (PUCHAR)ModuleBase + ModuleSize)
			{
				ULONG TextSize = 0;
				PVOID TextBase = MH_GetSectionByName(ModuleBase, ".text\0\0\0", &TextSize);

				if (TextBase)
				{
					if (!(pOldFuncAddr >= TextBase && pOldFuncAddr < (PUCHAR)TextBase + TextSize))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

hook_t* MH_InlinePatchRedirectBranch(void* pInstructionAddress, void* pNewFuncAddr, void** pOrginalCall)
{
	typedef struct
	{
		PUCHAR pSourceCode;
		PUCHAR pNewFuncAddr;
		ULONG PatchLength;
		UCHAR NewCodeBytes[8];
	}MH_InlinePatchRedirectBranch_PatchContext;

	MH_InlinePatchRedirectBranch_PatchContext ctx = { 0 };

	ctx.pSourceCode = (PUCHAR)pInstructionAddress;
	ctx.pNewFuncAddr = (PUCHAR)pNewFuncAddr;
	memset(ctx.NewCodeBytes, 0x90, sizeof(ctx.NewCodeBytes));

	if (ctx.pSourceCode[0] == 0xE9 || ctx.pSourceCode[0] == 0xE8)
	{
		ctx.PatchLength = 5;
		ctx.NewCodeBytes[0] = ctx.pSourceCode[0];
		*(int*)(ctx.NewCodeBytes + 1) = ctx.pNewFuncAddr - (ctx.pSourceCode + 5);
	}
	else if (ctx.pSourceCode[0] == 0xFF && ctx.pSourceCode[1] == 0x15)
	{
		//indirect call
		ctx.PatchLength = 6;
		ctx.NewCodeBytes[0] = 0xE8;
		*(int*)(ctx.NewCodeBytes + 1) = ctx.pNewFuncAddr - (ctx.pSourceCode + 5);
	}
	else if (ctx.pSourceCode[0] == 0xFF && ctx.pSourceCode[1] == 0x25)
	{
		//indirect jmp
		ctx.PatchLength = 6;
		ctx.NewCodeBytes[0] = 0xE9;
		*(int*)(ctx.NewCodeBytes + 1) = ctx.pNewFuncAddr - (ctx.pSourceCode + 5);
	}
	else
	{
		MH_DisasmSingleInstruction(pInstructionAddress, [](void* inst, PUCHAR address, size_t instLen, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (MH_InlinePatchRedirectBranch_PatchContext*)context;

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM && instLen >= 5)
			{
				ctx->PatchLength = instLen;
				ctx->NewCodeBytes[0] = 0xE8;
				*(int*)(ctx->NewCodeBytes + 1) = ctx->pNewFuncAddr - (ctx->pSourceCode + 5);
			}

		}, &ctx);
	}

	if (!ctx.PatchLength)
		return NULL;

	hook_t* h = MH_NewHook(MH_HOOK_INLINEPATCH);

	if (!h)
	{
		return NULL;
	}

	h->pOldFuncAddr = MH_GetNextCallAddr(pInstructionAddress, 1);
	h->pNewFuncAddr = pNewFuncAddr;
	h->pOrginalCall = pOrginalCall;
	h->hookData.inlinepatch.pInstructionAddress = pInstructionAddress;
	h->hookData.inlinepatch.PatchLength = ctx.PatchLength;
	memcpy(h->hookData.inlinepatch.NewCodeBytes, ctx.NewCodeBytes, ctx.PatchLength);
	memcpy(h->hookData.inlinepatch.OriginalBytes, (PUCHAR)pInstructionAddress, ctx.PatchLength);

	if (g_bTransactionHook)
	{
		h->bCommitted = false;
	}
	else
	{
		MH_WriteMemory(pInstructionAddress, ctx.NewCodeBytes, ctx.PatchLength);

		if (h->pOrginalCall)
			(*h->pOrginalCall) = h->pOldFuncAddr;

		h->bCommitted = true;
	}

	return h;
}


hook_t* MH_VFTHookEx(void ** pVFTable, int iFuncIndex, void* pNewFuncAddr, void** pOrginalCall)
{
	hook_t* h = MH_NewHook(MH_HOOK_VFTABLE);

	if (!h)
	{
		return NULL;
	}

	PVOID* pVMT = pVFTable;

	h->pOldFuncAddr = pVMT[iFuncIndex];
	h->pNewFuncAddr = pNewFuncAddr;
	h->pOrginalCall = pOrginalCall;
	h->hookData.vfthook.pClassInstance = NULL;
	h->hookData.vfthook.iTableIndex = -1;
	h->hookData.vfthook.iFuncIndex = iFuncIndex;
	h->hookData.vfthook.pVirtualFuncTable = pVMT;
	h->hookData.vfthook.pVirtualFuncAddr = pVMT + iFuncIndex;

	if (CommandLine()->CheckParm("-metahook_check_vfthook"))
	{
		if (MH_IsBogusVFTableEntry(h->hookData.vfthook.pVirtualFuncAddr, h->pOldFuncAddr))
		{
			MH_UnHook(h);

			char msg[256];
			snprintf(msg, sizeof(msg), "MH_VFTHook: Found bogus hook at vftable(%p)[%d] -> %p, hook rejected.", pVFTable, iFuncIndex, pNewFuncAddr);
			MessageBoxA(NULL, msg, "Warning", MB_ICONWARNING);

			return NULL;
		}
	}

	if (g_bTransactionHook)
	{
		h->bCommitted = false;
	}
	else
	{
		MH_WriteMemory(h->hookData.vfthook.pVirtualFuncAddr, &pNewFuncAddr, sizeof(ULONG_PTR));

		if (h->pOrginalCall)
			(*h->pOrginalCall) = h->pOldFuncAddr;

		h->bCommitted = true;
	}

#if 0
	auto p = (PUCHAR)_ReturnAddress();

	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(p, &mbi, sizeof(mbi));

	if (mbi.Type == MEM_IMAGE)
	{
		char modname[256] = { 0 };
		GetModuleFileNameA((HMODULE)mbi.AllocationBase, modname, sizeof(modname));

		char test[256];
		sprintf(test, "%p called MH_VFTHook, from %p[%d] (%p) to %p, %s+%X\n", p, pClassInstance, iFuncIndex, info->pVirtualFuncAddr, pNewFuncAddr, modname, p - (PUCHAR)mbi.AllocationBase);
		OutputDebugStringA(test);
	}
#endif

	return h;
}

hook_t* MH_VFTHook(void* pClassInstance, int iTableIndex, int iFuncIndex, void* pNewFuncAddr, void** pOrginalCall)
{
	hook_t* h = MH_NewHook(MH_HOOK_VFTABLE);

	if (!h)
	{
		return NULL;
	}

	PVOID** pTables = (PVOID**)pClassInstance;
	PVOID* pVMT = pTables[iTableIndex];

	h->pOldFuncAddr = pVMT[iFuncIndex];
	h->pNewFuncAddr = pNewFuncAddr;
	h->pOrginalCall = pOrginalCall;
	h->hookData.vfthook.pClassInstance = pClassInstance;
	h->hookData.vfthook.iTableIndex = iTableIndex;
	h->hookData.vfthook.iFuncIndex = iFuncIndex;
	h->hookData.vfthook.pVirtualFuncTable = pVMT;
	h->hookData.vfthook.pVirtualFuncAddr = pVMT + iFuncIndex;

	if (CommandLine()->CheckParm("-metahook_check_vfthook"))
	{
		if (MH_IsBogusVFTableEntry(h->hookData.vfthook.pVirtualFuncAddr, h->pOldFuncAddr))
		{
			MH_UnHook(h);

			char msg[256];
			snprintf(msg, sizeof(msg), "MH_VFTHook: Found bogus hook at %p_vftable[%d][%d] -> %p, hook rejected.", pClassInstance, iTableIndex, iFuncIndex, pNewFuncAddr);
			MessageBoxA(NULL, msg, "Warning", MB_ICONWARNING);

			return NULL;
		}
	}

	if (g_bTransactionHook)
	{
		h->bCommitted = false;
	}
	else
	{
		MH_WriteMemory(h->hookData.vfthook.pVirtualFuncAddr, &pNewFuncAddr, sizeof(ULONG_PTR));

		if (h->pOrginalCall)
			(*h->pOrginalCall) = h->pOldFuncAddr;

		h->bCommitted = true;
	}

	return h;
}

hook_t* MH_CreateIATHook(HMODULE hModule, BlobHandle_t hBlob, const char* pszModuleName, const char* pszFuncName, void* pNewFuncAddr, void** pOrginalCall, ULONG_PTR *pThunkFunction)
{
	hook_t* h = MH_NewHook(MH_HOOK_IAT);

	if (!h)
	{
		return NULL;
	}

	h->pOldFuncAddr = (void*)(*pThunkFunction);
	h->pNewFuncAddr = pNewFuncAddr;

	h->hookData.iathook.hModule = hModule;
	h->hookData.iathook.hBlob = hBlob;

	h->hookData.iathook.pImportFuncAddr = (PVOID*)pThunkFunction;

	strncpy(h->hookData.iathook.szModuleName, pszModuleName, sizeof(h->hookData.iathook.szModuleName));
	h->hookData.iathook.szModuleName[sizeof(h->hookData.iathook.szModuleName) - 1] = 0;

	strncpy(h->hookData.iathook.szFuncName, pszFuncName, sizeof(h->hookData.iathook.szFuncName));
	h->hookData.iathook.szFuncName[sizeof(h->hookData.iathook.szFuncName) - 1] = 0;

	h->pOrginalCall = pOrginalCall;

	if (g_bTransactionHook)
	{
		h->bCommitted = false;
	}
	else
	{
		MH_WriteMemory(h->hookData.iathook.pImportFuncAddr, &h->pNewFuncAddr, sizeof(PVOID));

		if (h->pOrginalCall)
			(*h->pOrginalCall) = h->pOldFuncAddr;

		h->bCommitted = true;
	}

	return h;
}

hook_t *MH_IATHook(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void **pOrginalCall)
{
	auto pNtHeader = RtlImageNtHeader(hModule);

	if (!pNtHeader)
		return NULL;

	auto pImport = (IMAGE_IMPORT_DESCRIPTOR *)((ULONG_PTR)hModule + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImport->Name && 0 != stricmp((const char *)((ULONG_PTR)hModule + pImport->Name), pszModuleName))
		pImport++;

	auto hProcModule = GetModuleHandleA(pszModuleName);

	if(!hProcModule)
		return NULL;

	ULONG_PTR dwFuncAddr = (ULONG_PTR)GetProcAddress(hProcModule, pszFuncName);

	if (!dwFuncAddr)
		return NULL;

	auto pThunk = (IMAGE_THUNK_DATA *)((ULONG_PTR)hModule + pImport->FirstThunk);

	while (pThunk->u1.Function != dwFuncAddr && pThunk->u1.Function)
	{
		pThunk++;
	}

	if(!pThunk->u1.Function)
		return NULL;

	return MH_CreateIATHook(hModule, NULL, pszModuleName, pszFuncName, pNewFuncAddr, pOrginalCall, &pThunk->u1.Function);
}

bool MH_ModuleHasImport(HMODULE hModule, const char* pszModuleName)
{
	auto pNtHeader = RtlImageNtHeader(hModule);

	if (!pNtHeader)
		return NULL;

	auto pImport = (IMAGE_IMPORT_DESCRIPTOR*)((ULONG_PTR)hModule + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImport->Name)
	{
		if (0 == stricmp((const char*)((ULONG_PTR)hModule + pImport->Name), pszModuleName))
		{
			return true;
		}

		pImport++;
	}

	return false;
}

bool MH_ModuleHasImportEx(HMODULE hModule, const char* pszModuleName, const char* pszFuncName)
{
	auto pNtHeader = RtlImageNtHeader(hModule);

	if (!pNtHeader)
		return NULL;

	auto pImport = (IMAGE_IMPORT_DESCRIPTOR*)((ULONG_PTR)hModule + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImport->Name)
	{
		if (0 == stricmp((const char*)((ULONG_PTR)hModule + pImport->Name), pszModuleName))
		{
			auto hProcModule = GetModuleHandle(pszModuleName);

			if (!hProcModule)
				break;

			ULONG_PTR dwFuncAddr = (ULONG_PTR)GetProcAddress(hProcModule, pszFuncName);

			if (!dwFuncAddr)
				break;

			auto pThunk = (IMAGE_THUNK_DATA*)((ULONG_PTR)hModule + pImport->FirstThunk);

			while (pThunk->u1.Function)
			{
				if (pThunk->u1.Function == dwFuncAddr)
				{
					return true;
				}
				pThunk++;
			}

			break;
		}

		pImport++;
	}

	return false;
}

hook_t* MH_InlineHookTrampoline(void* pOldFuncAddr, void* pNewFuncAddr, void** pOriginalCall)
{
	return NULL;
}

void *MH_GetClassFuncAddr(...)
{
	DWORD address;

	__asm
	{
		lea eax,address
		mov edx, [ebp + 8]
		mov [eax], edx
	}

	return (void *)address;
}

PVOID MH_GetModuleBase(PVOID VirtualAddress)
{
	MEMORY_BASIC_INFORMATION mem;

	if (!VirtualQuery(VirtualAddress, &mem, sizeof(MEMORY_BASIC_INFORMATION)))
		return 0;

	if (mem.Type != MEM_IMAGE)
		return 0;

	return mem.AllocationBase;
}

DWORD MH_GetModuleSize(PVOID ModuleBase)
{
	return ((IMAGE_NT_HEADERS *)((PUCHAR)ModuleBase + ((IMAGE_DOS_HEADER *)ModuleBase)->e_lfanew))->OptionalHeader.SizeOfImage;
}

HMODULE MH_GetEngineModule(void)
{
	return g_hEngineModule;
}

PVOID MH_GetEngineBase_LegacyV2(void)
{
	if (g_hBlobEngine)
	{
		return (PUCHAR)g_dwEngineBase + 0x1000;
	}

	return g_dwEngineBase;
}

PVOID MH_GetEngineBase(void)
{
	return g_dwEngineBase;
}

DWORD MH_GetEngineSize(void)
{
	return g_dwEngineSize;
}

HMODULE MH_GetClientModule(void)
{
	if(g_phClientModule)
		return (*g_phClientModule);

	return NULL;
}

BlobHandle_t MH_GetBlobEngineModule(void)
{
	return g_hBlobEngine;
}

BlobHandle_t MH_GetBlobClientModule(void)
{
	return g_hBlobClient;
}

PVOID MH_GetClientBase(void)
{
	auto hClientModule = MH_GetClientModule();

	if (hClientModule)
		return (PVOID)hClientModule;

	auto hBlobClient = MH_GetBlobClientModule();

	if (hBlobClient)
		return GetBlobModuleImageBase(hBlobClient);

	return NULL;
}

DWORD MH_GetClientSize(void)
{
	auto hClientModule = MH_GetClientModule();

	if (hClientModule)
		return MH_GetModuleSize(hClientModule);

	auto hBlobClient = MH_GetBlobClientModule();

	if (hBlobClient)
		return GetBlobModuleImageSize(hBlobClient);

	return 0;
}

void *MH_SearchPattern(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen)
{
	if (!pStartSearch)
		return NULL;

	char *dwStartAddr = (char *)pStartSearch;
	char *dwEndAddr = dwStartAddr + dwSearchLen - dwPatternLen;

	while (dwStartAddr < dwEndAddr)
	{
		bool found = true;

		for (DWORD i = 0; i < dwPatternLen; i++)
		{
			char code = *(char *)(dwStartAddr + i);

			if (pPattern[i] != 0x2A && pPattern[i] != code)
			{
				found = false;
				break;
			}
		}

		if (found)
			return (void *)dwStartAddr;

		dwStartAddr++;
	}

	return NULL;
}

void* MH_SearchPatternNoWildCard(void* pStartSearch, DWORD dwSearchLen, const char* pPattern, DWORD dwPatternLen)
{
	if (!pStartSearch)
		return NULL;

	char* dwStartAddr = (char*)pStartSearch;
	char* dwEndAddr = dwStartAddr + dwSearchLen - dwPatternLen;

	while (dwStartAddr < dwEndAddr)
	{
		bool found = true;

		for (DWORD i = 0; i < dwPatternLen; i++)
		{
			char code = *(char*)(dwStartAddr + i);

			if (pPattern[i] != code)
			{
				found = false;
				break;
			}
		}

		if (found)
			return (void*)dwStartAddr;

		dwStartAddr++;
	}

	return NULL;
}

void *MH_ReverseSearchPattern(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen)
{
	char * dwStartAddr = (char *)pStartSearch;
	char * dwEndAddr = dwStartAddr - dwSearchLen - dwPatternLen;

	while (dwStartAddr > dwEndAddr)
	{
		bool found = true;

		for (DWORD i = 0; i < dwPatternLen; i++)
		{
			char code = *(char *)(dwStartAddr + i);

			if (pPattern[i] != 0x2A && pPattern[i] != code)
			{
				found = false;
				break;
			}
		}

		if (found)
			return (LPVOID)dwStartAddr;

		dwStartAddr--;
	}

	return 0;
}

void MH_WriteDWORD(void *pAddress, DWORD dwValue)
{
	DWORD dwOldProtect = 0;

	if (VirtualProtect((void *)pAddress, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		*(DWORD *)pAddress = dwValue;
		VirtualProtect((void *)pAddress, 4, dwOldProtect, &dwOldProtect);
	}
}

DWORD MH_ReadDWORD(void *pAddress)
{
	DWORD dwOldProtect = 0;
	DWORD dwValue = 0;

	if (VirtualProtect((void *)pAddress, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		dwValue = *(DWORD *)pAddress;
		VirtualProtect((void *)pAddress, 4, dwOldProtect, &dwOldProtect);
	}

	return dwValue;
}

void MH_WriteBYTE(void *pAddress, BYTE ucValue)
{
	DWORD dwOldProtect = 0;

	if (VirtualProtect((void *)pAddress, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		*(BYTE *)pAddress = ucValue;
		VirtualProtect((void *)pAddress, 1, dwOldProtect, &dwOldProtect);
	}
}

BYTE MH_ReadBYTE(void *pAddress)
{
	DWORD dwOldProtect = 0;
	BYTE ucValue = 0;

	if (VirtualProtect((void *)pAddress, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		ucValue = *(BYTE *)pAddress;
		VirtualProtect((void *)pAddress, 1, dwOldProtect, &dwOldProtect);
	}

	return ucValue;
}

void MH_WriteNOP(void *pAddress, DWORD dwCount)
{
	DWORD dwOldProtect = 0;

	if (VirtualProtect(pAddress, dwCount, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		for (DWORD i = 0; i < dwCount; i++)
			*(BYTE *)((DWORD)pAddress + i) = 0x90;

		VirtualProtect(pAddress, dwCount, dwOldProtect, &dwOldProtect);
	}
}

DWORD MH_WriteMemory(void *pAddress, void *pData, DWORD dwDataSize)
{
	DWORD dwOldProtect = 0;

	if (VirtualProtect(pAddress, dwDataSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		memcpy(pAddress, pData, dwDataSize);
		VirtualProtect(pAddress, dwDataSize, dwOldProtect, &dwOldProtect);
	}

	return dwDataSize;
}

DWORD MH_ReadMemory(void *pAddress, void *pData, DWORD dwDataSize)
{
	DWORD dwOldProtect = 0;

	if (VirtualProtect(pAddress, dwDataSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		memcpy(pData, pAddress, dwDataSize);
		VirtualProtect(pAddress, dwDataSize, dwOldProtect, &dwOldProtect);
	}

	return dwDataSize;
}

typedef struct videomode_s
{
	int width;
	int height;
	int bpp;
}videomode_t;

class IVideoMode
{
public:
	virtual const char *GetName();
	virtual void Init();
	virtual void Shutdown();
	virtual bool AddMode(int width, int height, int bpp);
	virtual videomode_t* GetCurrentMode();
	virtual videomode_t* GetMode(int num);
	virtual int GetModeCount();
	virtual bool IsWindowedMode();
	virtual bool GetInitialized();
	virtual void SetInitialized(bool init);
	virtual void UpdateWindowPosition();
	virtual void FlipScreen();
	virtual void RestoreVideo();
	virtual void ReleaseVideo();
	virtual void dtor();
	virtual int GetBitsPerPixel();
};

class IVideoMode_HL25
{
public:
	virtual const char* GetName();
	virtual void Init();
	virtual void Shutdown();
	virtual void unk();
	virtual bool AddMode(int width, int height, int bpp);
	virtual videomode_t* GetCurrentMode();
	virtual videomode_t* GetMode(int num);
	virtual int GetModeCount();
	virtual bool IsWindowedMode();
	virtual bool GetInitialized();
	virtual void SetInitialized(bool init);
	virtual void UpdateWindowPosition();
	virtual void FlipScreen();
	virtual void RestoreVideo();
	virtual void ReleaseVideo();
	virtual void dtor();
	virtual int GetBitsPerPixel();
};

DWORD MH_GetVideoMode(int *width, int *height, int *bpp, bool *windowed)
{
	static int iSaveMode;
	static int iSaveWidth, iSaveHeight, iSaveBPP;
	static bool bSaveWindowed;

	if (g_pVideoMode && (*g_pVideoMode))
	{
		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			IVideoMode_HL25* pVideoMode = (IVideoMode_HL25*)(*g_pVideoMode);

			auto mode = pVideoMode->GetCurrentMode();

			if (width)
				*width = mode->width;

			if (height)
				*height = mode->height;

			if (bpp)
				*bpp = pVideoMode->GetBitsPerPixel();

			if (windowed)
				*windowed = pVideoMode->IsWindowedMode();

			if (!strcmp(pVideoMode->GetName(), "gl"))
				return VIDEOMODE_OPENGL;

			if (!strcmp(pVideoMode->GetName(), "d3d"))
				return VIDEOMODE_D3D;

			return VIDEOMODE_SOFTWARE;
		}
		else
		{
			IVideoMode* pVideoMode = (IVideoMode*)(*g_pVideoMode);

			auto mode = pVideoMode->GetCurrentMode();

			if (width)
				*width = mode->width;

			if (height)
				*height = mode->height;

			if (bpp)
				*bpp = pVideoMode->GetBitsPerPixel();

			if (windowed)
				*windowed = pVideoMode->IsWindowedMode();

			if (!strcmp(pVideoMode->GetName(), "gl"))
				return VIDEOMODE_OPENGL;

			if (!strcmp(pVideoMode->GetName(), "d3d"))
				return VIDEOMODE_D3D;

			return VIDEOMODE_SOFTWARE;
		}
	}

	if (g_bSaveVideo)
	{
		if (width)
			*width = iSaveWidth;

		if (height)
			*height = iSaveHeight;

		if (bpp)
			*bpp = iSaveBPP;

		if (windowed)
			*windowed = bSaveWindowed;
	}
	else
	{
		const char *pszValues = registry->ReadString("EngineDLL", "hw.dll");
		int iEngineD3D = registry->ReadInt("EngineD3D");

		if (!strcmp(pszValues, "hw.dll"))
		{
			if (CommandLine()->CheckParm("-d3d") || (!CommandLine()->CheckParm("-gl") && iEngineD3D))
				iSaveMode = VIDEOMODE_D3D;
			else
				iSaveMode = VIDEOMODE_OPENGL;
		}
		else
		{
			iSaveMode = VIDEOMODE_SOFTWARE;
		}

		bSaveWindowed = registry->ReadInt("ScreenWindowed") != false;

		if (CommandLine()->CheckParm("-sw") || CommandLine()->CheckParm("-startwindowed") || CommandLine()->CheckParm("-windowed") || CommandLine()->CheckParm("-window"))
			bSaveWindowed = true;
		else if (CommandLine()->CheckParm("-full") || CommandLine()->CheckParm("-fullscreen"))
			bSaveWindowed = false;

		iSaveWidth = registry->ReadInt("ScreenWidth", 640);

		if (CommandLine()->CheckParm("-width", &pszValues))
			iSaveWidth = atoi(pszValues);

		if (CommandLine()->CheckParm("-w", &pszValues))
			iSaveWidth = atoi(pszValues);

		iSaveHeight = registry->ReadInt("ScreenHeight", 480);

		if (CommandLine()->CheckParm("-height", &pszValues))
			iSaveHeight = atoi(pszValues);

		if (CommandLine()->CheckParm("-h", &pszValues))
			iSaveHeight = atoi(pszValues);

		iSaveBPP = registry->ReadInt("ScreenBPP", 32);

		if (CommandLine()->CheckParm("-16bpp"))
			iSaveBPP = 16;
		else if (CommandLine()->CheckParm("-24bpp"))
			iSaveBPP = 24;
		else if (CommandLine()->CheckParm("-32bpp"))
			iSaveBPP = 32;

		if (width)
			*width = iSaveWidth;

		if (height)
			*height = iSaveHeight;

		if (bpp)
			*bpp = iSaveBPP;

		if (windowed)
			*windowed = bSaveWindowed;

		g_bSaveVideo = true;
	}

	return iSaveMode;
}

CreateInterfaceFn MH_GetEngineFactory(void)
{
	if (g_hEngineModule)
	{
		return (CreateInterfaceFn)GetProcAddress(g_hEngineModule, "CreateInterface");
	}

	if (g_hBlobEngine)
	{
		ULONG_PTR base = GetBlobHeaderExportPoint(g_hBlobEngine) + 0x8;
		ULONG_PTR factoryAddr = ((ULONG_PTR(*)(void))(base + *(ULONG_PTR*)base + 0x4))();

		return (CreateInterfaceFn)factoryAddr;
	}

	return NULL;
}

CreateInterfaceFn MH_GetClientFactory(void)
{
	auto hClientModule = MH_GetClientModule();
	if (hClientModule)
		return (CreateInterfaceFn)Sys_GetFactory((HINTERFACEMODULE)hClientModule);

	if (g_pClientFactory && (*g_pClientFactory))
	{
		CreateInterfaceFn(*pfnClientFactory)() = (decltype(pfnClientFactory))(*g_pClientFactory);

		return pfnClientFactory();
	}

	return NULL;
}

PVOID MH_GetNextCallAddr(void *pAddress, DWORD dwCount)
{
	static BYTE *pbAddress = NULL;

	if (pAddress)
		pbAddress = (BYTE *)pAddress;
	else
		pbAddress = pbAddress + 5;

	for (DWORD i = 0; i < dwCount; i++)
	{
		if (pbAddress[0] == 0xFF && pbAddress[1] == 0x15)
		{
			pbAddress = (BYTE*)(**(PVOID **)(pbAddress + 2));
		}
		else if (pbAddress[0] == 0xFF && pbAddress[1] == 0x25)
		{
			pbAddress = (BYTE*)(**(PVOID**)(pbAddress + 2));
		}
		else if (pbAddress[0] == 0xE8)
		{
			pbAddress = (BYTE*)(pbAddress + 5 + *(int *)(pbAddress + 1));
		}
		else if (pbAddress[0] == 0xE9)
		{
			pbAddress = (BYTE*)(pbAddress + 5 + *(int*)(pbAddress + 1));
		}
		else
		{
			return NULL;
		}
	}

	return pbAddress;
}

DWORD MH_GetEngineVersion(void)
{
	if (!g_pfnbuild_number)
		return 0;

	return g_pfnbuild_number();
}

int MH_GetEngineType(void)
{
	return g_iEngineType;
}

const char *engineTypeNames[] = {
	"Unknown",
	"GoldSrc_Blob",
	"GoldSrc",
	"SvEngine",
	"GoldSrc_HL25",
};

const char *MH_GetEngineTypeName(void)
{
	return engineTypeNames[MH_GetEngineType()];
}

PVOID MH_GetSectionByName(PVOID ImageBase, const char *SectionName, ULONG *SectionSize)
{
	auto hBlob = BlobLoaderFindBlobByImageBase(ImageBase);

	if (hBlob)
	{
		return GetBlobSectionByName(hBlob, SectionName, SectionSize);
	}

	PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(ImageBase);
	if (NtHeader)
	{
		PIMAGE_SECTION_HEADER SectionHdr = (PIMAGE_SECTION_HEADER)((PUCHAR)NtHeader + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + NtHeader->FileHeader.SizeOfOptionalHeader);
		for (USHORT i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
		{
			if (0 == memcmp(SectionHdr[i].Name, SectionName, 8))
			{
				if (SectionSize)
					*SectionSize = max(SectionHdr[i].SizeOfRawData, SectionHdr[i].Misc.VirtualSize);

				return (PUCHAR)ImageBase + SectionHdr[i].VirtualAddress;
			}
		}
	}

	return NULL;
}

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	PVOID base;
	size_t max_insts;
	int max_depth;
	std::set<PVOID> code;
	std::set<PVOID> branches;
	std::vector<walk_context_t> walks;

	PVOID DesiredAddress;
	bool bFoundDesiredAddress;
}MH_ReverseSearchFunctionBegin_ctx;

typedef struct
{
	PUCHAR instAddr;
	int instLen;
	bool bPushRegister;
	bool bSubEspImm;
	bool bMovReg1000h;
}MH_ReverseSearchFunctionBegin_ctx2;

PVOID MH_ReverseSearchFunctionBegin(PVOID SearchBegin, DWORD SearchSize)
{
	PUCHAR SearchPtr = (PUCHAR)SearchBegin;
	PUCHAR SearchEnd = (PUCHAR)SearchBegin - SearchSize;

	while (SearchPtr > SearchEnd)
	{
		PVOID Candidate = NULL;
		bool bShouldCheck = false;

		if (SearchPtr[0] == 0xCC || SearchPtr[0] == 0x90 || SearchPtr[0] == 0xC3)
		{
			if (SearchPtr[1] == 0xCC || SearchPtr[1] == 0x90)
			{
				if (SearchPtr[2] != 0x90 &&
					SearchPtr[2] != 0xCC)
				{
					bShouldCheck = true;
					Candidate = SearchPtr + 2;
				}
			}
			else if (
				SearchPtr[1] != 0x90 &&
				SearchPtr[1] != 0xCC)
			{
				MH_ReverseSearchFunctionBegin_ctx2 ctx2 = { 0 };

				MH_DisasmSingleInstruction(SearchPtr + 1, [](void *inst, PUCHAR address, size_t instLen, PVOID context) {
					auto pinst = (cs_insn *)inst;
					auto ctx = (MH_ReverseSearchFunctionBegin_ctx2 *)context;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						ctx->bPushRegister = true;
					}
					else if (pinst->id == X86_INS_SUB &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[0].reg == X86_REG_ESP &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM)
					{
						ctx->bSubEspImm = true;
					}
					else if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm >= 0x1000)
					{
						ctx->bMovReg1000h = true;
					}

					ctx->instAddr = address;
					ctx->instLen = instLen;

				}, &ctx2);

				if (!ctx2.bPushRegister && !ctx2.bSubEspImm && !ctx2.bMovReg1000h)
				{
					MH_DisasmSingleInstruction(ctx2.instAddr + ctx2.instLen, [](void* inst, PUCHAR address, size_t instLen, PVOID context) {
						auto pinst = (cs_insn*)inst;
						auto ctx = (MH_ReverseSearchFunctionBegin_ctx2*)context;

						if (pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG)
						{
							ctx->bPushRegister = true;
						}
						else if (pinst->id == X86_INS_SUB &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG &&
							pinst->detail->x86.operands[0].reg == X86_REG_ESP &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM)
						{
							ctx->bSubEspImm = true;
						}
						else if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[1].imm >= 0x1000)
						{
							ctx->bMovReg1000h = true;
						}

					}, &ctx2);
				}

				if (ctx2.bPushRegister || ctx2.bSubEspImm || ctx2.bMovReg1000h)
				{
					bShouldCheck = true;
					Candidate = SearchPtr + 1;
				}
			}
		}

		if (bShouldCheck)
		{
			MH_ReverseSearchFunctionBegin_ctx ctx = { 0 };

			ctx.bFoundDesiredAddress = false;
			ctx.DesiredAddress = SearchBegin;
			ctx.base = Candidate;
			ctx.max_insts = 1000;
			ctx.max_depth = 16;
			ctx.walks.emplace_back(ctx.base, 0x1000, 0);

			while (ctx.walks.size())
			{
				auto walk = ctx.walks[ctx.walks.size() - 1];
				ctx.walks.pop_back();

				MH_DisasmRanges(walk.address, walk.len, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn *)inst;
					auto ctx = (MH_ReverseSearchFunctionBegin_ctx *)context;

					if (address == ctx->DesiredAddress)
					{
						ctx->bFoundDesiredAddress = true;
						return TRUE;
					}

					if (ctx->code.size() > ctx->max_insts)
						return TRUE;

					if (ctx->code.find(address) != ctx->code.end())
						return TRUE;

					ctx->code.emplace(address);

					if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM)
					{
						PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
						auto foundbranch = ctx->branches.find(imm);
						if (foundbranch == ctx->branches.end())
						{
							ctx->branches.emplace(imm);
							if (depth + 1 < ctx->max_depth)
								ctx->walks.emplace_back(imm, 0x300, depth + 1);
						}

						if (pinst->id == X86_INS_JMP)
							return TRUE;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, walk.depth, &ctx);
			}

			if (ctx.bFoundDesiredAddress)
			{
				return Candidate;
			}
		}

		SearchPtr--;
	}

	return NULL;
}

PVOID MH_ReverseSearchFunctionBeginEx(PVOID SearchBegin, DWORD SearchSize, FindAddressCallback callback)
{
	PUCHAR SearchPtr = (PUCHAR)SearchBegin;
	PUCHAR SearchEnd = (PUCHAR)SearchBegin - SearchSize;

	while (SearchPtr > SearchEnd)
	{
		PVOID Candidate = NULL;
		bool bShouldCheck = false;

		if (SearchPtr[0] == 0xCC || SearchPtr[0] == 0x90 || SearchPtr[0] == 0xC3)
		{
			if (SearchPtr[1] == 0xCC || SearchPtr[1] == 0x90)
			{
				if (SearchPtr[2] != 0x90 &&
					SearchPtr[2] != 0xCC)
				{
					bShouldCheck = true;
					Candidate = SearchPtr + 2;
				}
			}
			else if (
				SearchPtr[1] != 0x90 &&
				SearchPtr[1] != 0xCC)
			{
				MH_ReverseSearchFunctionBegin_ctx2 ctx2 = { 0 };

				MH_DisasmSingleInstruction(SearchPtr + 1, [](void* inst, PUCHAR address, size_t instLen, PVOID context) {
					auto pinst = (cs_insn*)inst;
					auto ctx = (MH_ReverseSearchFunctionBegin_ctx2*)context;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						ctx->bPushRegister = true;
					}
					else if (pinst->id == X86_INS_SUB &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[0].reg == X86_REG_ESP &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM)
					{
						ctx->bSubEspImm = true;
					}
					else if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm >= 0x1000)
					{
						ctx->bMovReg1000h = true;
					}

					ctx->instAddr = address;
					ctx->instLen = instLen;

				}, &ctx2);

				if (!ctx2.bPushRegister && !ctx2.bSubEspImm)
				{
					MH_DisasmSingleInstruction(ctx2.instAddr + ctx2.instLen, [](void* inst, PUCHAR address, size_t instLen, PVOID context) {
						auto pinst = (cs_insn*)inst;
						auto ctx = (MH_ReverseSearchFunctionBegin_ctx2*)context;

						if (pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG)
						{
							ctx->bPushRegister = true;
						}
						else if (pinst->id == X86_INS_SUB &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG &&
							pinst->detail->x86.operands[0].reg == X86_REG_ESP &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM)
						{
							ctx->bSubEspImm = true;
						}
						else if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[1].imm >= 0x1000)
						{
							ctx->bMovReg1000h = true;
						}

					}, &ctx2);
				}

				if (ctx2.bPushRegister || ctx2.bSubEspImm || ctx2.bMovReg1000h)
				{
					bShouldCheck = true;
					Candidate = SearchPtr + 1;
				}
			}
		}

		if (bShouldCheck && callback((PUCHAR)Candidate))
		{
			MH_ReverseSearchFunctionBegin_ctx ctx = { 0 };

			ctx.bFoundDesiredAddress = false;
			ctx.DesiredAddress = SearchBegin;
			ctx.base = Candidate;
			ctx.max_insts = 1000;
			ctx.max_depth = 16;
			ctx.walks.emplace_back(ctx.base, 0x1000, 0);

			while (ctx.walks.size())
			{
				auto walk = ctx.walks[ctx.walks.size() - 1];
				ctx.walks.pop_back();

				MH_DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
					{
						auto pinst = (cs_insn*)inst;
						auto ctx = (MH_ReverseSearchFunctionBegin_ctx*)context;

						if (address == ctx->DesiredAddress)
						{
							ctx->bFoundDesiredAddress = true;
							return TRUE;
						}

						if (ctx->code.size() > ctx->max_insts)
							return TRUE;

						if (ctx->code.find(address) != ctx->code.end())
							return TRUE;

						ctx->code.emplace(address);

						if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_IMM)
						{
							PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
							auto foundbranch = ctx->branches.find(imm);
							if (foundbranch == ctx->branches.end())
							{
								ctx->branches.emplace(imm);
								if (depth + 1 < ctx->max_depth)
									ctx->walks.emplace_back(imm, 0x300, depth + 1);
							}

							if (pinst->id == X86_INS_JMP)
								return TRUE;
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;
					}, walk.depth, &ctx);
			}

			if (ctx.bFoundDesiredAddress)
			{
				return Candidate;
			}
		}

		SearchPtr--;
	}

	return NULL;
}

int MH_DisasmSingleInstruction(PVOID address, DisasmSingleCallback callback, void *context)
{
	int instLen = 0;
	csh handle = 0;
	if (cs_open(CS_ARCH_X86, CS_MODE_32, &handle) == CS_ERR_OK)
	{
		if (cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) == CS_ERR_OK)
		{
			cs_insn *insts = NULL;
			size_t count = 0;

			const uint8_t *addr = (uint8_t *)address;
			uint64_t vaddr = ((uint64_t)address & 0x00000000FFFFFFFFull);
			size_t size = 15;

			bool accessable = !IsBadReadPtr(addr, size);

			if (accessable)
			{
				count = cs_disasm(handle, addr, size, vaddr, 1, &insts);
				if (count)
				{
					callback(insts, (PUCHAR)address, insts->size, context);

					instLen += insts->size;
				}
			}

			if (insts) {
				cs_free(insts, count);
				insts = NULL;
			}
		}
		cs_close(&handle);
	}

	return instLen;
}

BOOL MH_DisasmRanges(PVOID DisasmBase, SIZE_T DisasmSize, DisasmCallback callback, int depth, PVOID context)
{
	BOOL success = FALSE;

	csh handle = 0;
	if (cs_open(CS_ARCH_X86, CS_MODE_32, &handle) == CS_ERR_OK)
	{
		cs_insn *insts = NULL;
		size_t count = 0;
		int instCount = 1;

		if (cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) == CS_ERR_OK)
		{
			PUCHAR pAddress = (PUCHAR)DisasmBase;

			do
			{
				const uint8_t *addr = (uint8_t *)pAddress;
				uint64_t vaddr = ((uint64_t)pAddress & 0x00000000FFFFFFFFull);
				size_t size = 15;

				if (insts) {
					cs_free(insts, count);
					insts = NULL;
				}

				bool accessable = !IsBadReadPtr(addr, size);

				if(!accessable)
					break;

				count = cs_disasm(handle, addr, size, vaddr, 1, &insts);

				if (!count)
					break;

				SIZE_T instLen = insts[0].size;
				if (!instLen)
					break;

				if (callback(&insts[0], pAddress, instLen, instCount, depth, context))
				{
					success = TRUE;
					break;
				}

				pAddress += instLen;
				instCount++;
			} while (pAddress < (PUCHAR)DisasmBase + DisasmSize);
		}

		if (insts) {
			cs_free(insts, count);
			insts = NULL;
		}

		cs_close(&handle);
	}
	return success;
}

BOOL MH_QueryPluginInfo(int fromindex, mh_plugininfo_t *info)
{
	int index = 0;
	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next, index++)
	{
		if (index > fromindex)
		{
			const char *version = "";
			switch (plug->iInterfaceVersion)
			{
			case 4:
				version = ((IPluginsV4 *)plug->pPluginAPI)->GetVersion();
				break;
			default:
				break;
			}

			if (info)
			{
				info->Index = index;
				info->InterfaceVersion = plug->iInterfaceVersion;
				info->PluginModuleBase = plug->module;
				info->PluginModuleSize = plug->modulesize;
				info->PluginName = plug->filename.c_str();
				info->PluginPath = plug->filepath.c_str();
				info->PluginVersion = version;
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL MH_GetPluginInfo(const char *name, mh_plugininfo_t *info)
{
	int index = 0;
	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next, index++)
	{
		if (!stricmp(name, plug->filename.c_str()))
		{
			const char *version = "";
			switch (plug->iInterfaceVersion)
			{
			case 4:
				version = ((IPluginsV4 *)plug->pPluginAPI)->GetVersion();
				break;
			default:
				break;
			}

			if (info)
			{
				info->Index = index;
				info->InterfaceVersion = plug->iInterfaceVersion;
				info->PluginModuleBase = plug->module;
				info->PluginModuleSize = plug->modulesize;
				info->PluginName = plug->filename.c_str();
				info->PluginPath = plug->filepath.c_str();
				info->PluginVersion = version;
			}
			return TRUE;
		}
	}
	return FALSE;
}

const char* MH_GetGameDirectory()
{
	return g_szGameDirectory;
}

PVOID MH_GetMirrorEngineBase()
{
	return MH_GetMirrorDLLBase(g_hMirrorEngine);
}

ULONG MH_GetMirrorEngineSize()
{
	return MH_GetMirrorDLLSize(g_hMirrorEngine);
}

PVOID MH_GetMirrorClientBase()
{
	return MH_GetMirrorDLLBase(g_hMirrorClient);
}

ULONG MH_GetMirrorClientSize()
{
	return MH_GetMirrorDLLSize(g_hMirrorClient);
}

HMEMORYMODULE MH_LoadMirrorDLL_Std(const char* szFileName)
{
	HMEMORYMODULE hMemoryModuleHandle = NULL;

	auto hFileHandle = fopen(szFileName, "rb");
	if (hFileHandle)
	{
		fseek(hFileHandle, 0, FILESYSTEM_SEEK_TAIL);
		auto cbFileSize = ftell(hFileHandle);
		fseek(hFileHandle, 0, FILESYSTEM_SEEK_HEAD);

		auto pFileBuffer = malloc(cbFileSize);

		if (pFileBuffer)
		{
			auto cbReadBytes = fread(pFileBuffer, 1, cbFileSize, hFileHandle);

			if (cbReadBytes == cbFileSize)
			{
				ULONG ImageSize = 0;
				hMemoryModuleHandle = LoadLibraryMemoryExW(pFileBuffer, cbFileSize, &ImageSize, NULL, NULL, LOAD_FLAGS_NOT_MAP_DLL | LOAD_FLAGS_NO_RESOLVE_IMPORTS | LOAD_FLAGS_NOT_HANDLE_TLS | LOAD_FLAGS_NO_EXECUTE | LOAD_FLAGS_READ_ONLY | IMAGE_DLLCHARACTERISTICS_NO_SEH | LOAD_FLAGS_NO_DISCARD_SECTION | LOAD_FLAGS_FORCE_RELOCATION | LOAD_FLAGS_PASS_IMAGE_CHECK);
			}

			free(pFileBuffer);
		}

		fclose(hFileHandle);
	}

	return hMemoryModuleHandle;
}

HMEMORYMODULE MH_LoadMirrorDLL_FileSystem(const char * szFileName)
{
	HMEMORYMODULE hMemoryModuleHandle = NULL;

	auto hFileHandle = FILESYSTEM_ANY_OPEN(szFileName, "rb", "GAME");
	if (hFileHandle)
	{
		FILESYSTEM_ANY_SEEK(hFileHandle, 0, FILESYSTEM_SEEK_TAIL);
		auto cbFileSize = FILESYSTEM_ANY_TELL(hFileHandle);
		FILESYSTEM_ANY_SEEK(hFileHandle, 0, FILESYSTEM_SEEK_HEAD);

		auto pFileBuffer = malloc(cbFileSize);

		if (pFileBuffer)
		{
			auto cbReadBytes = FILESYSTEM_ANY_READ(pFileBuffer, cbFileSize, hFileHandle);

			if (cbReadBytes == cbFileSize)
			{
				ULONG ImageSize = 0;
				hMemoryModuleHandle = LoadLibraryMemoryExW(pFileBuffer, cbFileSize, &ImageSize, NULL, NULL, LOAD_FLAGS_NOT_MAP_DLL | LOAD_FLAGS_NO_RESOLVE_IMPORTS | LOAD_FLAGS_NOT_HANDLE_TLS | LOAD_FLAGS_NO_EXECUTE | LOAD_FLAGS_READ_ONLY | IMAGE_DLLCHARACTERISTICS_NO_SEH | LOAD_FLAGS_NO_DISCARD_SECTION | LOAD_FLAGS_FORCE_RELOCATION | LOAD_FLAGS_PASS_IMAGE_CHECK);
			}

			free(pFileBuffer);
		}

		FILESYSTEM_ANY_CLOSE(hFileHandle);
	}

	return hMemoryModuleHandle;
}

void MH_FreeMirrorDLL(HMEMORYMODULE hMemoryModule)
{
	FreeLibraryMemory(hMemoryModule);
}

PVOID MH_GetMirrorDLLBase(HMEMORYMODULE hMemoryModule)
{
	return (PVOID)(hMemoryModule);
}

ULONG MH_GetMirrorDLLSize(HMEMORYMODULE hMemoryModule)
{
	return GetMemoryModuleSize(hMemoryModule);
}

ThreadPoolHandle_t MH_GetGlobalThreadPool(void)
{
	return g_ThreadPool;
}

typedef struct MH_ThreadPool_s
{
	PTP_POOL m_pTp{};
	PTP_CLEANUP_GROUP m_pCleanupGroup{};
	TP_CALLBACK_ENVIRON m_CallbackEnv{ };
}MH_ThreadPool_t;

typedef struct MH_TpWorkContext_s
{
	PTP_WORK m_pTpWork{};
	fnThreadWorkItemCallback m_UserCallback{};
	void* m_pUserContext{};
	HANDLE m_hEvent{};
}MH_TpWorkContext_t;

ThreadPoolHandle_t MH_CreateThreadPool(ULONG minThreads, ULONG maxThreads)
{
	MH_ThreadPool_t* pThreadPool = new MH_ThreadPool_t;
	if (!pThreadPool)
		return nullptr;

	InitializeThreadpoolEnvironment(&pThreadPool->m_CallbackEnv);

	pThreadPool->m_pTp = CreateThreadpool(NULL);

	SetThreadpoolThreadMinimum(pThreadPool->m_pTp, minThreads);
	SetThreadpoolThreadMaximum(pThreadPool->m_pTp, maxThreads);

	pThreadPool->m_pCleanupGroup = CreateThreadpoolCleanupGroup();

	SetThreadpoolCallbackPool(&pThreadPool->m_CallbackEnv, pThreadPool->m_pTp);
	SetThreadpoolCallbackCleanupGroup(&pThreadPool->m_CallbackEnv, pThreadPool->m_pCleanupGroup, NULL);

	return (ThreadPoolHandle_t)pThreadPool;
}

static VOID NTAPI MH_ThreadPoolWorkItem(
	_Inout_     PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID                 Context,
	_Inout_     PTP_WORK              Work
)
{
	MH_TpWorkContext_t* pTpWorkContext = (MH_TpWorkContext_t*)Context;

	if (pTpWorkContext) {

		auto bDeleteWorkItem = pTpWorkContext->m_UserCallback(pTpWorkContext->m_pUserContext);

		if (pTpWorkContext->m_hEvent) {
			SetEventWhenCallbackReturns(Instance, pTpWorkContext->m_hEvent);
		}

		if (bDeleteWorkItem) {

			if (pTpWorkContext->m_pTpWork) {
				CloseThreadpoolWork(pTpWorkContext->m_pTpWork);
				pTpWorkContext->m_pTpWork = nullptr;
			}

			delete pTpWorkContext;
		}
	}
}

ThreadWorkItemHandle_t MH_CreateWorkItem(ThreadPoolHandle_t hThreadPool, fnThreadWorkItemCallback callback, void* ctx)
{
	MH_TpWorkContext_t *pTpWorkContext = new MH_TpWorkContext_t;
	if (!pTpWorkContext)
		return nullptr;

	MH_ThreadPool_t* pThreadPool = (MH_ThreadPool_t*)hThreadPool;

	pTpWorkContext->m_UserCallback = callback;
	pTpWorkContext->m_pUserContext = ctx;
	pTpWorkContext->m_pTpWork = CreateThreadpoolWork(MH_ThreadPoolWorkItem, pTpWorkContext, &pThreadPool->m_CallbackEnv);
	pTpWorkContext->m_hEvent = NULL;

	return (ThreadWorkItemHandle_t)pTpWorkContext;
}

void MH_QueueWorkItem(ThreadPoolHandle_t hThreadPool, ThreadWorkItemHandle_t hWorkItem)
{
	MH_TpWorkContext_t* pTpWorkContext = (MH_TpWorkContext_t*)hWorkItem;

	SubmitThreadpoolWork(pTpWorkContext->m_pTpWork);
}

void MH_WaitForWorkItemToComplete(ThreadWorkItemHandle_t hWorkItem)
{
	MH_TpWorkContext_t* pTpWorkContext = (MH_TpWorkContext_t*)hWorkItem;

	WaitForThreadpoolWorkCallbacks(pTpWorkContext->m_pTpWork, FALSE);
}

void MH_DeleteThreadPool(ThreadWorkItemHandle_t hThreadPool)
{
	MH_ThreadPool_t* pThreadPool = (MH_ThreadPool_t*)hThreadPool;

	if (pThreadPool->m_pCleanupGroup) {
		CloseThreadpoolCleanupGroupMembers(pThreadPool->m_pCleanupGroup, TRUE, NULL);
		CloseThreadpoolCleanupGroup(pThreadPool->m_pCleanupGroup);
		pThreadPool->m_pCleanupGroup = nullptr;
	}

	DestroyThreadpoolEnvironment(&pThreadPool->m_CallbackEnv);

	if (pThreadPool->m_pTp) {
		CloseThreadpool(pThreadPool->m_pTp);
		pThreadPool->m_pTp = nullptr;
	}

	delete pThreadPool;
}

void MH_DeleteWorkItem(ThreadWorkItemHandle_t hWorkItem)
{
	MH_TpWorkContext_t* pTpWorkContext = (MH_TpWorkContext_t*)hWorkItem;
	if (!pTpWorkContext)
		return;

	if (pTpWorkContext->m_pTpWork) {
		CloseThreadpoolWork(pTpWorkContext->m_pTpWork);
		pTpWorkContext->m_pTpWork = nullptr;
	}

	delete pTpWorkContext;
}

metahook_api_t gMetaHookAPI_LegacyV2 =
{
	MH_UnHook,
	MH_InlineHook,
	MH_VFTHook,
	MH_IATHook,
	MH_GetClassFuncAddr,
	MH_GetModuleBase,
	MH_GetModuleSize,
	MH_GetEngineModule,
	MH_GetEngineBase_LegacyV2,
	MH_GetEngineSize,
	MH_SearchPattern,
	MH_WriteDWORD,
	MH_ReadDWORD,
	MH_WriteMemory,
	MH_ReadMemory,
	MH_GetVideoMode,
	MH_GetEngineVersion,
	MH_GetEngineFactory,
	MH_GetNextCallAddr,
	MH_WriteBYTE,
	MH_ReadBYTE,
	MH_WriteNOP,
	MH_GetEngineType,
	MH_GetEngineTypeName,
	MH_ReverseSearchFunctionBegin,
	MH_GetSectionByName,
	MH_DisasmSingleInstruction,
	MH_DisasmRanges,
	MH_ReverseSearchPattern,
	MH_GetClientModule,
	MH_GetClientBase,
	MH_GetClientSize,
	MH_GetClientFactory,
	MH_QueryPluginInfo,
	MH_GetPluginInfo,
	MH_HookUserMsg,
	MH_HookCvarCallback,
	MH_HookCmd,
	MH_SysError,
	MH_ReverseSearchFunctionBeginEx,
	MH_IsDebuggerPresent,
	MH_RegisterCvarCallback,
	MH_GetBlobEngineModule,
	MH_GetBlobClientModule,
	GetBlobModuleImageBase,
	GetBlobModuleImageSize,
	GetBlobSectionByName,
	BlobLoaderFindBlobByImageBase,
	BlobLoaderFindBlobByVirtualAddress,
	MH_RegisterDllLoaderNotificationCallback,
	MH_UnregisterDllLoaderNotificationCallback,
	MH_ModuleHasImport,
	MH_ModuleHasImportEx,
	MH_BlobHasImport,
	MH_BlobHasImportEx,
	MH_BlobIATHook,
	MH_GetGameDirectory,
	MH_FindCmd,
	MH_VFTHookEx,
	MH_InlinePatchRedirectBranch,
	MH_FindInlineHook,
	MH_FindVFTHook,
	MH_FindVFTHookEx,
	MH_FindIATHook,
	MH_FindInlinePatchHook,
	MH_GetUserMsgBase,
	MH_FindUserMsgHook,
	MH_GetCLParseFuncBase,
	MH_FindCLParseFuncByOpcode,
	MH_FindCLParseFuncByName,
	MH_HookCLParseFuncByOpcode,
	MH_HookCLParseFuncByName,
	MH_SearchPatternNoWildCard,
	MH_GetMirrorEngineBase,
	MH_GetMirrorEngineSize,
	MH_GetMirrorClientBase,
	MH_GetMirrorClientSize,
	MH_LoadMirrorDLL_Std,
	MH_LoadMirrorDLL_FileSystem,
	MH_FreeMirrorDLL,
	MH_GetMirrorDLLBase,
	MH_GetMirrorDLLSize,
	MH_GetGlobalThreadPool,
	MH_CreateThreadPool,
	MH_CreateWorkItem,
	MH_QueueWorkItem,
	MH_WaitForWorkItemToComplete,
	MH_DeleteThreadPool,
	MH_DeleteWorkItem,
	NULL
};

metahook_api_t gMetaHookAPI =
{
	MH_UnHook,
	MH_InlineHook,
	MH_VFTHook,
	MH_IATHook,
	MH_GetClassFuncAddr,
	MH_GetModuleBase,
	MH_GetModuleSize,
	MH_GetEngineModule,
	MH_GetEngineBase,
	MH_GetEngineSize,
	MH_SearchPattern,
	MH_WriteDWORD,
	MH_ReadDWORD,
	MH_WriteMemory,
	MH_ReadMemory,
	MH_GetVideoMode,
	MH_GetEngineVersion,
	MH_GetEngineFactory,
	MH_GetNextCallAddr,
	MH_WriteBYTE,
	MH_ReadBYTE,
	MH_WriteNOP,
	MH_GetEngineType,
	MH_GetEngineTypeName,
	MH_ReverseSearchFunctionBegin,
	MH_GetSectionByName,
	MH_DisasmSingleInstruction,
	MH_DisasmRanges,
	MH_ReverseSearchPattern,
	MH_GetClientModule,
	MH_GetClientBase,
	MH_GetClientSize,
	MH_GetClientFactory,
	MH_QueryPluginInfo,
	MH_GetPluginInfo,
	MH_HookUserMsg,
	MH_HookCvarCallback,
	MH_HookCmd,
	MH_SysError,
	MH_ReverseSearchFunctionBeginEx,
	MH_IsDebuggerPresent,
	MH_RegisterCvarCallback,
	MH_GetBlobEngineModule,
	MH_GetBlobClientModule,
	GetBlobModuleImageBase,
	GetBlobModuleImageSize,
	GetBlobSectionByName,
	BlobLoaderFindBlobByImageBase,
	BlobLoaderFindBlobByVirtualAddress,
	MH_RegisterDllLoaderNotificationCallback,
	MH_UnregisterDllLoaderNotificationCallback,
	MH_ModuleHasImport,
	MH_ModuleHasImportEx,
	MH_BlobHasImport,
	MH_BlobHasImportEx,
	MH_BlobIATHook,
	MH_GetGameDirectory,
	MH_FindCmd,
	MH_VFTHookEx,
	MH_InlinePatchRedirectBranch,
	MH_FindInlineHook,
	MH_FindVFTHook,
	MH_FindVFTHookEx,
	MH_FindIATHook,
	MH_FindInlinePatchHook,
	MH_GetUserMsgBase,
	MH_FindUserMsgHook,
	MH_GetCLParseFuncBase,
	MH_FindCLParseFuncByOpcode,
	MH_FindCLParseFuncByName,
	MH_HookCLParseFuncByOpcode,
	MH_HookCLParseFuncByName,
	MH_SearchPatternNoWildCard,
	MH_GetMirrorEngineBase,
	MH_GetMirrorEngineSize,
	MH_GetMirrorClientBase,
	MH_GetMirrorClientSize,
	MH_LoadMirrorDLL_Std,
	MH_LoadMirrorDLL_FileSystem,
	MH_FreeMirrorDLL,
	MH_GetMirrorDLLBase,
	MH_GetMirrorDLLSize,
	MH_GetGlobalThreadPool,
	MH_CreateThreadPool,
	MH_CreateWorkItem,
	MH_QueueWorkItem,
	MH_WaitForWorkItemToComplete,
	MH_DeleteThreadPool,
	MH_DeleteWorkItem,
	NULL
};