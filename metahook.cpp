#include "metahook.h"
#include "LoadBlob.h"
#include <detours.h>
#include "interface.h"
#include <capstone.h>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>

extern "C"
{
	NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID Base);
	NTSYSAPI NTSTATUS NTAPI NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);
}

#define MH_HOOK_INLINE 1
#define MH_HOOK_VFTABLE 2
#define MH_HOOK_IAT 3

struct hook_s
{
	int iType;
	qboolean bCommitted;
	void *pOldFuncAddr;
	void *pNewFuncAddr;
	void **pOrginalCall;
	void *pClass;
	int iTableIndex;
	int iFuncIndex;
	HMODULE hModule;
	const char *pszModuleName;
	const char *pszFuncName;
	struct hook_s *pNext;
	void *pInfo;
};

typedef struct cvar_callback_entry_s
{
	cvar_callback_t callback;
	cvar_t *pcvar;
	struct cvar_callback_entry_s *next;
}cvar_callback_entry_t;

cvar_callback_entry_t **cvar_callbacks = NULL;

cvar_callback_entry_t* g_ManagedCvarCallbackList = NULL;

std::vector<cvar_callback_entry_t*> g_ManagedCvarCallbacks;

typedef struct usermsg_s
{
	int index;
	int size;
	char name[16];
	struct usermsg_s *next;
	pfnUserMsgHook function;
}usermsg_t;

usermsg_t **gClientUserMsgs = NULL;

typedef struct cmd_function_s
{
	struct cmd_function_s *next;
	char *name;
	xcommand_t function;
	int flags;
}cmd_function_t;

cmd_function_t *(*Cmd_GetCmdBase)(void) = NULL;

void **g_pVideoMode = NULL;
int (*g_pfnbuild_number)(void) = NULL;

int(*g_original_ClientDLL_Init)(void) = NULL;
int(*g_pfnClientDLL_Init)(void) = NULL;

void(*g_pfnCvar_DirectSet)(cvar_t* var, char* value) = NULL;

CreateInterfaceFn *g_pClientFactory = NULL;

HMODULE *g_phClientModule = NULL;

HMODULE g_hEngineModule = NULL;
PVOID g_dwEngineBase = NULL;
DWORD g_dwEngineSize = NULL;

hook_t *g_pHookBase = NULL;
cl_exportfuncs_t *g_pExportFuncs = NULL;
void *g_ppExportFuncs = NULL;
void *g_ppEngfuncs = NULL;
bool g_bSaveVideo = false;
bool g_bTransactionInlineHook = false;
int g_iEngineType = ENGINE_UNKNOWN;

PVOID MH_GetNextCallAddr(void *pAddress, DWORD dwCount);
hook_t *MH_FindInlineHooked(void *pOldFuncAddr);
hook_t *MH_FindVFTHooked(void *pClass, int iTableIndex, int iFuncIndex);
hook_t *MH_FindIATHooked(HMODULE hModule, const char *pszModuleName, const char *pszFuncName);
BOOL MH_UnHook(hook_t *pHook);
hook_t *MH_InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void **pOriginalCall);
hook_t *MH_VFTHook(void *pClass, int iTableIndex, int iFuncIndex, void *pNewFuncAddr, void **pOriginalCall);
hook_t *MH_IATHook(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void **pOriginalCall);
void *MH_GetClassFuncAddr(...);
HMODULE MH_GetClientModule(void);
PVOID MH_GetModuleBase(PVOID VirtualAddress);
DWORD MH_GetModuleSize(PVOID ModuleBase);
void *MH_SearchPattern(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen);
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
void MH_SysError(const char *fmt, ...);

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

plugin_t *g_pPluginBase;

extern IFileSystem_HL25 *g_pFileSystem_HL25;
extern IFileSystem* g_pFileSystem;

mh_interface_t gInterface = {0};
mh_enginesave_t gMetaSave = {0};

extern metahook_api_t gMetaHookAPI;

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

void MH_SysError(const char *fmt, ...)
{
	char msg[4096] = { 0 };

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(msg, sizeof(msg) - 1, fmt, argptr);
	va_end(argptr);

	msg[sizeof(msg) - 1] = 0;

	if (gMetaSave.pEngineFuncs)
	{
		if(gMetaSave.pEngineFuncs->pfnClientCmd)
			gMetaSave.pEngineFuncs->pfnClientCmd("escape\n");
	}

	MessageBoxA(NULL, msg, "Fatal Error", MB_ICONERROR);
	NtTerminateProcess((HANDLE)(-1), 0);
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

usermsg_t *MH_FindUserMsgHook(const char *szMsgName)
{
	if (!gClientUserMsgs)
		return NULL;

	for (usermsg_t *msg = (*gClientUserMsgs); msg; msg = msg->next)
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
	bool bDuplicate = false;

	for (plugin_t *p = g_pPluginBase; p; p = p->next)
	{
		if (!stricmp(p->filename.c_str(), filename.c_str()))
		{
			bDuplicate = true;
			break;
		}
	}

	if (!bDuplicate && GetModuleHandleA(filename.c_str()))
		bDuplicate = true;

	if (bDuplicate)
	{
		return PLUGIN_LOAD_DUPLICATE;
	}

	HINTERFACEMODULE hModule = Sys_LoadModule(filepath.c_str());

	if (!hModule)
	{
		return PLUGIN_LOAD_ERROR;
	}

	for (plugin_t *p = g_pPluginBase; p; p = p->next)
	{
		if (p->module == hModule)
		{
			bDuplicate = true;
			break;
		}
	}

	if (bDuplicate)
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

	plugin_t *plug = new plugin_t;
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
				((IPluginsV2 *)plug->pPluginAPI)->Init(&gMetaHookAPI, &gInterface, &gMetaSave);
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
					free(plug);
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

void MH_LoadPlugins(const char *gamedir)
{
	std::string aConfigFile = gamedir;
	aConfigFile += "/metahook/configs/plugins.lst";

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

			std::string aPluginPath = gamedir;
			aPluginPath += "/metahook/plugins/";

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
				if (MH_IsDebuggerPresent())
				{
					std::string fileName = aFileName + ".dll";

					int result = MH_LoadPlugin(aPluginPath + ".dll", fileName);

					int win32err = GetLastError();

					if (PLUGIN_LOAD_SUCCEEDED == result)
					{
						break;
					}
					else
					{
						MH_ReportError(fileName, result, win32err);
					}
				}

				if (MH_HasAVX2())
				{
					std::string fileName = aFileName + "_AVX2.dll";

					int result = MH_LoadPlugin(aPluginPath + "_AVX2.dll", fileName);

					int win32err = GetLastError();

					if (PLUGIN_LOAD_SUCCEEDED == result)
					{
						break;
					}
					else if (PLUGIN_LOAD_ERROR == result && (win32err == ERROR_FILE_NOT_FOUND || win32err == ERROR_MOD_NOT_FOUND))
					{

					}
					else
					{
						MH_ReportError(fileName, result, win32err);
					}
				}

				if (MH_HasAVX())
				{
					std::string fileName = aFileName + "_AVX.dll";

					int result = MH_LoadPlugin(aPluginPath + "_AVX.dll", fileName);

					int win32err = GetLastError();

					if (PLUGIN_LOAD_SUCCEEDED == result)
					{
						break;
					}
					else if (PLUGIN_LOAD_ERROR == result && (win32err == ERROR_FILE_NOT_FOUND || win32err == ERROR_MOD_NOT_FOUND))
					{

					}
					else
					{
						MH_ReportError(fileName, result, win32err);
					}
				}

				if (MH_HasSSE2())
				{
					std::string fileName = aFileName + "_SSE2.dll";

					int result = MH_LoadPlugin(aPluginPath + "_SSE2.dll", fileName);

					int win32err = GetLastError();

					if (PLUGIN_LOAD_SUCCEEDED == result)
					{
						break;
					}
					else if (PLUGIN_LOAD_ERROR == result && (win32err == ERROR_FILE_NOT_FOUND || win32err == ERROR_MOD_NOT_FOUND))
					{

					}
					else
					{
						MH_ReportError(fileName, result, win32err);
					}
				}

				if (MH_HasSSE())
				{
					std::string fileName = aFileName + "_SSE.dll";

					int result = MH_LoadPlugin(aPluginPath + "_SSE.dll", fileName);

					int win32err = GetLastError();

					if (PLUGIN_LOAD_SUCCEEDED == result)
					{
						break;
					}
					else if (PLUGIN_LOAD_ERROR == result && (win32err == ERROR_FILE_NOT_FOUND || win32err == ERROR_MOD_NOT_FOUND))
					{

					}
					else
					{
						MH_ReportError(fileName, result, win32err);
					}
				}

				if (1)
				{
					std::string fileName = aFileName + ".dll";

					int result = MH_LoadPlugin(aPluginPath + ".dll", fileName);

					int win32err = GetLastError();

					if (PLUGIN_LOAD_SUCCEEDED == result)
					{
						break;
					}
					else
					{
						MH_ReportError(fileName, result, win32err);
					}
				}
			} while (0);
		}
	}
	infile.close();
}

int(*g_pfnHUD_GetStudioModelInterface)(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio) = NULL;

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	int r = 0;

	g_bTransactionInlineHook = true;

	r = g_pfnHUD_GetStudioModelInterface ? g_pfnHUD_GetStudioModelInterface(version, ppinterface, pstudio) : 0;

	g_bTransactionInlineHook = false;

	for (auto pHook = g_pHookBase; pHook; pHook = pHook->pNext)
	{
		if (pHook->iType == MH_HOOK_INLINE && !pHook->bCommitted)
		{
			DetourTransactionBegin();
			DetourAttach(&(void *&)pHook->pOldFuncAddr, pHook->pNewFuncAddr);
			DetourTransactionCommit();

			*pHook->pOrginalCall = pHook->pOldFuncAddr;
			pHook->bCommitted = true;
		}
	}

	return r;
}

int ClientDLL_Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(gMetaSave.pExportFuncs, g_pExportFuncs, sizeof(cl_exportfuncs_t));
	memcpy(gMetaSave.pEngineFuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	g_bTransactionInlineHook = true;

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
			((IPlugins *)plug->pPluginAPI)->LoadClient(g_pExportFuncs);
			break;
		default:
			((IPluginsV1 *)plug->pPluginAPI)->Init(g_pExportFuncs);
			break;
		}
	}

	g_bTransactionInlineHook = false;

	for (auto pHook = g_pHookBase; pHook; pHook = pHook->pNext)
	{
		if (pHook->iType == MH_HOOK_INLINE && !pHook->bCommitted)
		{
			DetourTransactionBegin();
			DetourAttach(&(void *&)pHook->pOldFuncAddr, pHook->pNewFuncAddr);
			DetourTransactionCommit();

			*pHook->pOrginalCall = pHook->pOldFuncAddr;
			pHook->bCommitted = true;
		}
	}

	g_pfnHUD_GetStudioModelInterface = g_pExportFuncs->HUD_GetStudioModelInterface;
	g_pExportFuncs->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;

	gMetaSave.pEngineFuncs->pfnAddCommand("mh_pluginlist", MH_PrintPluginList);

	return g_pExportFuncs->Initialize(pEnginefuncs, iVersion);
}

void MH_ClientDLL_Init(void)
{
	g_pExportFuncs = *(cl_exportfuncs_t **)g_ppExportFuncs;

	static DWORD dwClientDLL_Initialize[1];
	dwClientDLL_Initialize[0] = (DWORD)&ClientDLL_Initialize;

	MH_WriteDWORD(g_ppExportFuncs, (DWORD)dwClientDLL_Initialize);

	g_original_ClientDLL_Init();

	MH_WriteDWORD(g_ppExportFuncs, (DWORD)g_pExportFuncs);
}

void MH_LoadEngine(HMODULE hModule, const char *szGameName)
{
	Cmd_GetCmdBase = NULL;
	cvar_callbacks = NULL;
	gClientUserMsgs = NULL;
	g_pVideoMode = NULL;
	g_pfnbuild_number = NULL;
	g_pClientFactory = NULL;
	g_pfnClientDLL_Init = NULL;
	g_original_ClientDLL_Init = NULL;
	g_phClientModule = NULL;
	g_ppExportFuncs = NULL;
	g_ppEngfuncs = NULL;

	if(!gMetaSave.pEngineFuncs)
		gMetaSave.pEngineFuncs = new cl_enginefunc_t;

	memset(gMetaSave.pEngineFuncs, 0, sizeof(cl_enginefunc_t));

	if(!gMetaSave.pExportFuncs)
		gMetaSave.pExportFuncs = new cl_exportfuncs_t;

	memset(gMetaSave.pExportFuncs, 0, sizeof(cl_exportfuncs_t));

	g_dwEngineBase = 0;
	g_dwEngineSize = 0;
	g_pHookBase = NULL;
	g_pExportFuncs = NULL;
	g_bSaveVideo = false;

	gInterface.CommandLine = CommandLine();
	gInterface.FileSystem = g_pFileSystem;
	gInterface.Registry = registry;
	gInterface.FileSystem_HL25 = g_pFileSystem_HL25;

	if (hModule)
	{
		g_dwEngineBase = MH_GetModuleBase(hModule);
		g_dwEngineSize = MH_GetModuleSize(hModule);
		g_hEngineModule = hModule;

		g_iEngineType = ENGINE_UNKNOWN;
	}
	else
	{
		g_dwEngineBase = (PVOID)0x1D00000;
		g_dwEngineSize = 0x1000000;
		g_hEngineModule = NULL;

		g_iEngineType = ENGINE_GOLDSRC_BLOB;
	}

	ULONG textSize = 0;
	PVOID textBase = MH_GetSectionByName(g_dwEngineBase, ".text\0\0\0", &textSize);

	if (!textBase)
	{
		textBase = g_dwEngineBase;
		textSize = g_dwEngineSize;
	}

	ULONG dataSize = 0;
	PVOID dataBase = MH_GetSectionByName(g_dwEngineBase, ".data\0\0\0", &dataSize);

	if (!dataBase)
	{
		dataBase = g_dwEngineBase;
		dataSize = g_dwEngineSize;
	}

	ULONG rdataSize = 0;
	PVOID rdataBase = MH_GetSectionByName(g_dwEngineBase, ".rdata\0\0", &rdataSize);

	if (!rdataBase)
	{
		rdataBase = g_dwEngineBase;
		rdataSize = g_dwEngineSize;
	}

#define BUILD_NUMBER_SIG "\xE8\x2A\x2A\x2A\x2A\x50\x68\x2A\x2A\x2A\x2A\x6A\x30\x68"

	auto buildnumber_call = MH_SearchPattern(textBase, textSize, BUILD_NUMBER_SIG, sizeof(BUILD_NUMBER_SIG) - 1);
	
	if (buildnumber_call)
	{
		g_pfnbuild_number = (decltype(g_pfnbuild_number))MH_GetNextCallAddr(buildnumber_call, 1);
	}

	if (!g_pfnbuild_number)
	{
#define EXE_BUILD_STRING_SIG "Exe build: "
		auto ExeBuild_String = MH_SearchPattern((void*)g_dwEngineBase, g_dwEngineSize, EXE_BUILD_STRING_SIG, sizeof(EXE_BUILD_STRING_SIG) - 1);
		if (ExeBuild_String)
		{
			char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x50\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 7) = (DWORD)ExeBuild_String;
			auto ExeBuild_PushString = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
			if (ExeBuild_PushString)
			{
				g_pfnbuild_number = (decltype(g_pfnbuild_number))MH_GetNextCallAddr(ExeBuild_PushString, 1);
			}
		}

	}

	if (!g_pfnbuild_number)
	{
		MH_SysError("MH_LoadEngine: Failed to locate buildnumber");
		return;
	}

	if (buildnumber_call)
	{
		char* pEngineName = *(char**)((PUCHAR)buildnumber_call + sizeof(BUILD_NUMBER_SIG) - 1);

		if (g_iEngineType != ENGINE_GOLDSRC_BLOB)
		{
			if (!strncmp(pEngineName, "Svengine", sizeof("Svengine") - 1))
			{
				g_iEngineType = ENGINE_SVENGINE;
			}
			else if (!strncmp(pEngineName, "Half-Life", sizeof("Half-Life") - 1))
			{
				g_iEngineType = ENGINE_GOLDSRC;
			}
			else if (!strncmp(pEngineName, "version :  ", sizeof("version :  ") - 1))
			{
				g_iEngineType = ENGINE_GOLDSRC_HL25;
			}
			else
			{
				g_iEngineType = ENGINE_UNKNOWN;
			}
		}
	}
	else
	{
		g_iEngineType = ENGINE_UNKNOWN;
	}

	if (1)
	{
#define CLDLL_INIT_STRING_SIG "ScreenShake"
		auto ClientDll_Init_String = MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, CLDLL_INIT_STRING_SIG, sizeof(CLDLL_INIT_STRING_SIG) - 1);
		if (ClientDll_Init_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD *)(pattern + 6) = (DWORD)ClientDll_Init_String;
			auto ClientDll_Init_PushString = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
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
					g_pfnClientDLL_Init = (decltype(g_pfnClientDLL_Init))ClientDll_Init_FunctionBase;

					MH_DisasmRanges(ClientDll_Init_PushString, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
					{
						auto pinst = (cs_insn *)inst;

						if (address[0] == 0x6A && address[1] == 0x07 && address[2] == 0x68)
						{
							g_ppEngfuncs = (decltype(g_ppEngfuncs))(address + 3);
						}
						else if (address[0] == 0xFF && address[1] == 0x15)
						{
							g_ppExportFuncs = (decltype(g_ppExportFuncs))(address + 2);
						}

						if (g_ppExportFuncs && g_ppEngfuncs)
							return TRUE;

						if (address[0] == 0xCC)
							return TRUE;

						return FALSE;
					}, 0, NULL);
				}
			}
		}
	}

	if (!g_pfnClientDLL_Init)
	{
		MH_SysError( "MH_LoadEngine: Failed to locate ClientDLL_Init");
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

	if (1)
	{
#define RIGHTHAND_STRING_SIG "cl_righthand\0"
		auto RightHand_String = MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, RIGHTHAND_STRING_SIG, sizeof(RIGHTHAND_STRING_SIG) - 1);
		if (RightHand_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD *)(pattern + 1) = (DWORD)RightHand_String;
			auto RightHand_PushString = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
			if (RightHand_PushString)
			{
#define HUDINIT_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A"
				auto ClientDLL_HudInit = MH_ReverseSearchPattern(RightHand_PushString, 0x100, HUDINIT_SIG, sizeof(HUDINIT_SIG)-1);
				if (ClientDLL_HudInit)
				{
					PVOID pfnHUDInit = *(PVOID *)((PUCHAR)ClientDLL_HudInit + 1);

					ClientDLL_HudInit = (PUCHAR)ClientDLL_HudInit + sizeof(HUDINIT_SIG) - 1;
					MH_DisasmRanges(ClientDLL_HudInit, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
					{
						auto pinst = (cs_insn *)inst;

						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].type == X86_OP_MEM &&
							pinst->detail->x86.operands[1].mem.base == 0 &&
							(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineBase &&
							(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineBase + g_dwEngineSize)
						{
							PVOID imm = (PVOID)pinst->detail->x86.operands[1].mem.disp;
							if (imm != context)
							{
								g_phClientModule = (decltype(g_phClientModule))imm;
							}
						}

						if (g_phClientModule)
							return TRUE;

						if (address[0] == 0xCC)
							return TRUE;

						return FALSE;
					}, 0, pfnHUDInit);
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

	if (1)
	{
#define VGUICLIENT001_STRING_SIG "VClientVGUI001\0"
		auto VGUIClient001_String = MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, VGUICLIENT001_STRING_SIG, sizeof(VGUICLIENT001_STRING_SIG) - 1);
		if (VGUIClient001_String)
		{
			char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A";
			*(DWORD *)(pattern + 3) = (DWORD)VGUIClient001_String;
			auto VGUIClient001_PushString = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
			if (VGUIClient001_PushString)
			{
#define INITVGUI_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A"
				auto InitVGUI = MH_ReverseSearchPattern(VGUIClient001_PushString, 0x100, INITVGUI_SIG, sizeof(INITVGUI_SIG) - 1);
				if (InitVGUI)
				{
					g_pClientFactory = *(decltype(g_pClientFactory) *)((PUCHAR)InitVGUI + 1);
				}
				else
				{
#define INITVGUI_SIG2 "\x83\x3D\x2A\x2A\x2A\x2A\x00\x74\x2A"
					auto InitVGUI = MH_ReverseSearchPattern(VGUIClient001_PushString, 0x100, INITVGUI_SIG2, sizeof(INITVGUI_SIG2) - 1);
					if (InitVGUI)
					{
						g_pClientFactory = *(decltype(g_pClientFactory) *)((PUCHAR)InitVGUI + 2);
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

	memcpy(gMetaSave.pEngineFuncs, *(void **)g_ppEngfuncs, sizeof(cl_enginefunc_t));

	Cmd_GetCmdBase = *(decltype(Cmd_GetCmdBase) *)(&gMetaSave.pEngineFuncs->GetFirstCmdFunctionHandle);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define FULLSCREEN_STRING_SIG "-fullscreen\0"
		auto FullScreen_String = MH_SearchPattern(g_dwEngineBase, g_dwEngineSize, FULLSCREEN_STRING_SIG, sizeof(FULLSCREEN_STRING_SIG) - 1);
		if (FullScreen_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)FullScreen_String;
			auto FullScreen_PushString = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
			if (FullScreen_PushString)
			{
				FullScreen_PushString = (PUCHAR)FullScreen_PushString + sizeof(pattern) - 1;

				typedef struct
				{
					ULONG_PTR candidate_disp;
					PVOID candidate_addr;
				}FullScreen_PushString_Ctx;

				FullScreen_PushString_Ctx ctx = { 0 };

				MH_DisasmRanges(FullScreen_PushString, 0x400, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn*)inst;
					auto ctx = (FullScreen_PushString_Ctx *)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineBase + g_dwEngineSize &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 0)
					{
						ctx->candidate_disp = (decltype(ctx->candidate_disp))pinst->detail->x86.operands[0].mem.disp;
						ctx->candidate_addr = address;
					}

					if (ctx->candidate_disp &&pinst->id == X86_INS_RET &&
						address > ctx->candidate_addr && address < (PUCHAR)ctx->candidate_addr + 0x30)
					{
						g_pVideoMode = (decltype(g_pVideoMode))ctx->candidate_disp;
					}

					if (g_pVideoMode)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					return FALSE;
				}, 0, &ctx);
			}
		}
	}
	else
	{
#define FULLSCREEN_STRING_SIG "-gl\0"
		auto FullScreen_String = MH_SearchPattern(g_dwEngineBase, g_dwEngineSize, FULLSCREEN_STRING_SIG, sizeof(FULLSCREEN_STRING_SIG) - 1);
		if (FullScreen_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD *)(pattern + 1) = (DWORD)FullScreen_String;
			auto FullScreen_PushString = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
			if (FullScreen_PushString)
			{
				FullScreen_PushString = (PUCHAR)FullScreen_PushString + sizeof(pattern) - 1;

				typedef struct
				{
					ULONG_PTR candidate_disp;
					PVOID candidate_addr;
				}FullScreen_PushString_Ctx;

				FullScreen_PushString_Ctx ctx = { 0 };

				MH_DisasmRanges(FullScreen_PushString, 0x400, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn *)inst;

					auto ctx = (FullScreen_PushString_Ctx *)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineBase + g_dwEngineSize &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 0)
					{
						ctx->candidate_disp = (decltype(ctx->candidate_disp))pinst->detail->x86.operands[0].mem.disp;
						ctx->candidate_addr = address;
					}

					if (ctx->candidate_disp && pinst->id == X86_INS_RET &&
						address > ctx->candidate_addr && address < (PUCHAR)ctx->candidate_addr + 0x30)
					{
						g_pVideoMode = (decltype(g_pVideoMode))ctx->candidate_disp;
					}

					if (g_pVideoMode)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					return FALSE;
				}, 0, &ctx);
			}
		}
	}

	if (!g_pVideoMode)
	{
		MH_SysError("MH_LoadEngine: Failed to locate g_pVideoMode");
		return;
	}

	if (1)
	{
#define HUDTEXT_STRING_SIG "HudText\0"
		auto HudText_String = MH_SearchPattern(g_dwEngineBase, g_dwEngineSize, HUDTEXT_STRING_SIG, sizeof(HUDTEXT_STRING_SIG) - 1);
		if (HudText_String)
		{
			char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
			*(DWORD *)(pattern + 2) = (DWORD)HudText_String;
			auto HudText_PushString = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
			if (HudText_PushString)
			{
				PVOID DispatchDirectUserMsg = (PVOID)MH_GetNextCallAddr((PUCHAR)HudText_PushString + 6, 1);
				MH_DisasmRanges(DispatchDirectUserMsg, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn *)inst;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineBase + g_dwEngineSize)
					{
						gClientUserMsgs = (decltype(gClientUserMsgs))pinst->detail->x86.operands[1].mem.disp;
					}

					if (gClientUserMsgs)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					return FALSE;
				}, 0, NULL);
			}
		}
	}

	if (!gClientUserMsgs)
	{
		MH_SysError("MH_LoadEngine: Failed to locate gClientUserMsgs");
		return;
	}

	if (1)
	{
		const char sigs1[] = "***PROTECTED***";
		auto Cvar_DirectSet_String = MH_SearchPattern(dataBase, dataSize, sigs1, sizeof(sigs1) - 1);
		if (!Cvar_DirectSet_String)
			Cvar_DirectSet_String = MH_SearchPattern(rdataBase, rdataSize, sigs1, sizeof(sigs1) - 1);
		if (Cvar_DirectSet_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 1) = (DWORD)Cvar_DirectSet_String;
			auto Cvar_DirectSet_Call = MH_SearchPattern(textBase, textSize, pattern, sizeof(pattern) - 1);
			if (Cvar_DirectSet_Call)
			{
				g_pfnCvar_DirectSet = (decltype(g_pfnCvar_DirectSet))MH_ReverseSearchFunctionBeginEx(Cvar_DirectSet_Call, 0x500, [](PUCHAR Candidate) {
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

					return FALSE;
				});
			}
		}
	}

	if (!g_pfnCvar_DirectSet)
	{
		MH_SysError("MH_LoadEngine: Failed to locate Cvar_DirectSet");
		return;
	}

	//SvEngine removed cvar callbacks
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "Cvar_Set: variable %s not found\n";
		auto Cvar_DirectSet_String = MH_SearchPattern(dataBase, dataSize, sigs1, sizeof(sigs1) - 1);
		if (!Cvar_DirectSet_String)
			Cvar_DirectSet_String = MH_SearchPattern(rdataBase, rdataSize, sigs1, sizeof(sigs1) - 1);
		if (Cvar_DirectSet_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
			*(DWORD*)(pattern + 1) = (DWORD)Cvar_DirectSet_String;

			*(DWORD*)(pattern + 1) = (DWORD)Cvar_DirectSet_String;

			auto searchBegin = textBase;
			auto searchEnd = (PUCHAR)textBase + textSize;
			while (1)
			{
				auto Cvar_Set_Call = MH_SearchPattern(searchBegin, searchEnd - searchBegin, pattern, sizeof(pattern) - 1);
				if (Cvar_Set_Call)
				{
					searchBegin = (PUCHAR)Cvar_Set_Call + sizeof(pattern) - 1;

					MH_DisasmRanges(searchBegin, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
					{
							auto pinst = (cs_insn*)inst;

							if (address[0] == 0xE8)
							{
								auto CallTarget = address + *(int *)(address + 1) + instLen;

								if ((ULONG_PTR)CallTarget == (ULONG_PTR)g_pfnCvar_DirectSet)
								{
									auto dwNewRVA = (ULONG_PTR)MH_Cvar_DirectSet - (ULONG_PTR)(address + 5);

									MH_WriteDWORD(address + 1, dwNewRVA);
								}
							}

							return FALSE;
						}, 0, NULL);
				}
				else
				{
					break;
				}
			}
		}
	}
	if (g_iEngineType == ENGINE_GOLDSRC || g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		MH_DisasmRanges(gMetaSave.pEngineFuncs->Cvar_Set, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{
				DWORD imm = (DWORD)pinst->detail->x86.operands[1].mem.disp;

				if (!cvar_callbacks)
				{
					cvar_callbacks = (decltype(cvar_callbacks))imm;
				}
			}

			if (cvar_callbacks)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		if (!cvar_callbacks)
		{
			MH_SysError("MH_LoadEngine: Failed to locate cvar_callbacks");
			return;
		}
	}
	else
	{
		cvar_callbacks = &g_ManagedCvarCallbackList;
	}

	MH_InlineHook(g_pfnClientDLL_Init, MH_ClientDLL_Init, (void **)&g_original_ClientDLL_Init);

	MH_LoadPlugins(szGameName);

	g_bTransactionInlineHook = true;

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
			((IPlugins *)plug->pPluginAPI)->LoadEngine();
			break;
		default:
			break;
		}
	}

	g_bTransactionInlineHook = false;

	for (auto pHook = g_pHookBase; pHook; pHook = pHook->pNext)
	{
		if (pHook->iType == MH_HOOK_INLINE && !pHook->bCommitted)
		{
			DetourTransactionBegin();
			DetourAttach(&(void *&)pHook->pOldFuncAddr, pHook->pNewFuncAddr);
			DetourTransactionCommit();

			*pHook->pOrginalCall = pHook->pOldFuncAddr;
			pHook->bCommitted = true;
		}
	}
}

void MH_ExitGame(int iResult)
{
	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next)
	{
		switch (plug->iInterfaceVersion)
		{
		case 4:
			((IPluginsV4 *)plug->pPluginAPI)->ExitGame(iResult);
			break;
		case 3:
			((IPluginsV3 *)plug->pPluginAPI)->ExitGame(iResult);
			break;
		case 2:
			((IPluginsV2 *)plug->pPluginAPI)->ExitGame(iResult);
			break;
		default:
			break;
		}
	}
}

void MH_ShutdownPlugins(void)
{
	plugin_t *plug = g_pPluginBase;

	while (plug)
	{
		plugin_t *pfree = plug;
		plug = plug->next;

		if (pfree->pPluginAPI)
		{
			switch (pfree->iInterfaceVersion)
			{
			case 4:
				((IPluginsV4 *)pfree->pPluginAPI)->Shutdown();
				break;
			case 3:
				((IPluginsV3 *)pfree->pPluginAPI)->Shutdown();
				break;
			case 2:
				((IPluginsV2 *)pfree->pPluginAPI)->Shutdown();
				break;
			default:
				break;
			}
		}

		FreeLibrary((HMODULE)pfree->module);
		delete pfree;
	}

	g_pPluginBase = NULL;
}

void MH_Shutdown(void)
{
	if (g_pHookBase)
		MH_FreeAllHook();

	if (g_pPluginBase)
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

	if (cvar_callbacks)
	{
		(*cvar_callbacks) = NULL;
	}

	Cmd_GetCmdBase = NULL;
	cvar_callbacks = NULL;
	gClientUserMsgs = NULL;
	g_pVideoMode = NULL;
	g_pfnbuild_number = NULL;
	g_pClientFactory = NULL;
	g_pfnClientDLL_Init = NULL;
	g_original_ClientDLL_Init = NULL;
	g_phClientModule = NULL;

	g_hEngineModule = NULL;
	g_dwEngineBase = NULL;
	g_dwEngineSize = NULL;
	g_pHookBase = NULL;
	g_pExportFuncs = NULL;
	g_ppExportFuncs = NULL;
	g_ppEngfuncs = NULL;
	g_bSaveVideo = false;
	g_iEngineType = ENGINE_UNKNOWN;
}

hook_t *MH_NewHook(int iType)
{
	hook_t *h = new hook_t;
	memset(h, 0, sizeof(hook_t));
	h->iType = iType;
	h->pNext = g_pHookBase;
	g_pHookBase = h;
	return h;
}

hook_t *MH_FindInlineHooked(void *pOldFuncAddr)
{
	for (hook_t *h = g_pHookBase; h; h = h->pNext)
	{
		if (h->pOldFuncAddr == pOldFuncAddr)
			return h;
	}

	return NULL;
}

hook_t *MH_FindVFTHooked(void *pClass, int iTableIndex, int iFuncIndex)
{
	for (hook_t *h = g_pHookBase; h; h = h->pNext)
	{
		if (h->pClass == pClass && h->iTableIndex == iTableIndex && h->iFuncIndex == iFuncIndex)
			return h;
	}

	return NULL;
}

hook_t *MH_FindIATHooked(HMODULE hModule, const char *pszModuleName, const char *pszFuncName)
{
	for (hook_t *h = g_pHookBase; h; h = h->pNext)
	{
		if (h->hModule == hModule && h->pszModuleName == pszModuleName && h->pszFuncName == pszFuncName)
			return h;
	}

	return NULL;
}

#pragma pack(push, 1)

struct tagIATDATA
{
	void *pAPIInfoAddr;
};

struct tagCLASS
{
	DWORD *pVMT;
};

struct tagVTABLEDATA
{
	tagCLASS *pInstance;
	void *pVFTInfoAddr;
};

#pragma pack(pop)

void MH_FreeHook(hook_t *pHook)
{
	if (pHook->iType == MH_HOOK_VFTABLE)
	{
		tagVTABLEDATA *info = (tagVTABLEDATA *)pHook->pInfo;
		MH_WriteMemory(info->pVFTInfoAddr, (BYTE *)&pHook->pOldFuncAddr, sizeof(DWORD));
	}
	else if (pHook->iType == MH_HOOK_IAT)
	{
		tagIATDATA *info = (tagIATDATA *)pHook->pInfo;
		MH_WriteMemory(info->pAPIInfoAddr, (BYTE *)&pHook->pOldFuncAddr, sizeof(DWORD));
	}
	else if (pHook->iType == MH_HOOK_INLINE)
	{
		DetourDetach(&(void *&)pHook->pOldFuncAddr, pHook->pNewFuncAddr);
	}

	if (pHook->pInfo)
	{
		delete pHook->pInfo;
		pHook->pInfo = NULL;
	}

	delete pHook;
}

void MH_FreeAllHook(void)
{
	DetourTransactionBegin();

	hook_t *next = NULL;

	for (hook_t *h = g_pHookBase; h; h = next)
	{
		next = h->pNext;
		MH_FreeHook(h);
	}

	g_pHookBase = NULL;

	DetourTransactionCommit();
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

hook_t *MH_InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void **pOrginalCall)
{
	hook_t *h = MH_NewHook(MH_HOOK_INLINE);
	h->pOldFuncAddr = pOldFuncAddr;
	h->pNewFuncAddr = pNewFuncAddr;

	if (!pOrginalCall)
	{
		MessageBox(NULL, "MH_InlineHook: pOrginalCall can not be NULL.", "Fatal Error", MB_ICONERROR);
		NtTerminateProcess((HANDLE)-1, 0);
	}

	if (g_bTransactionInlineHook)
	{
		h->pOrginalCall = pOrginalCall;
		h->bCommitted = false;
	}
	else
	{
		DetourTransactionBegin();
		DetourAttach(&(void *&)h->pOldFuncAddr, pNewFuncAddr);
		DetourTransactionCommit();
		h->pOrginalCall = pOrginalCall;
		*h->pOrginalCall = h->pOldFuncAddr;
		h->bCommitted = true;
	}

	return h;
}

hook_t *MH_VFTHook(void *pClass, int iTableIndex, int iFuncIndex, void *pNewFuncAddr, void **pOrginalCall)
{
	tagVTABLEDATA *info = new tagVTABLEDATA;
	info->pInstance = (tagCLASS *)pClass;

	DWORD *pVMT = ((tagCLASS *)pClass + iTableIndex)->pVMT;
	info->pVFTInfoAddr = pVMT + iFuncIndex;

	hook_t *h = MH_NewHook(MH_HOOK_VFTABLE);
	h->pOldFuncAddr = (void *)pVMT[iFuncIndex];
	h->pNewFuncAddr = pNewFuncAddr;
	h->pInfo = info;
	h->pClass = pClass;
	h->iTableIndex = iTableIndex;
	h->iFuncIndex = iFuncIndex;

	*pOrginalCall = h->pOldFuncAddr;
	h->bCommitted = true;

	MH_WriteMemory(info->pVFTInfoAddr, (BYTE *)&pNewFuncAddr, sizeof(DWORD));
	return h;
}

hook_t *MH_IATHook(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void **pOrginalCall)
{
	IMAGE_NT_HEADERS *pHeader = (IMAGE_NT_HEADERS *)((DWORD)hModule + ((IMAGE_DOS_HEADER *)hModule)->e_lfanew);
	IMAGE_IMPORT_DESCRIPTOR *pImport = (IMAGE_IMPORT_DESCRIPTOR *)((DWORD)hModule + pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImport->Name && stricmp((const char *)((DWORD)hModule + pImport->Name), pszModuleName))
		pImport++;

	DWORD dwFuncAddr = (DWORD)GetProcAddress(GetModuleHandle(pszModuleName), pszFuncName);
	IMAGE_THUNK_DATA *pThunk = (IMAGE_THUNK_DATA *)((DWORD)hModule + pImport->FirstThunk);

	while (pThunk->u1.Function != dwFuncAddr)
		pThunk++;

	tagIATDATA *info = new tagIATDATA;
	info->pAPIInfoAddr = &pThunk->u1.Function;

	hook_t *h = MH_NewHook(MH_HOOK_IAT);
	h->pOldFuncAddr = (void *)pThunk->u1.Function;
	h->pNewFuncAddr = pNewFuncAddr;
	h->pInfo = info;
	h->hModule = hModule;
	h->pszModuleName = pszModuleName;
	h->pszFuncName = pszFuncName;

	*pOrginalCall = h->pOldFuncAddr;
	h->bCommitted = true;

	MH_WriteMemory(info->pAPIInfoAddr, (BYTE *)&pNewFuncAddr, sizeof(DWORD));
	return h;
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

PVOID MH_GetClientBase(void)
{
	return MH_GetClientModule();
}

DWORD MH_GetClientSize(void)
{
	return MH_GetModuleSize(MH_GetClientModule());
}

void *MH_SearchPattern(void *pStartSearch, DWORD dwSearchLen, const char *pPattern, DWORD dwPatternLen)
{
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
	DWORD dwProtect;

	if (VirtualProtect((void *)pAddress, 4, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		*(DWORD *)pAddress = dwValue;
		VirtualProtect((void *)pAddress, 4, dwProtect, &dwProtect);
	}
}

DWORD MH_ReadDWORD(void *pAddress)
{
	DWORD dwProtect;
	DWORD dwValue = 0;

	if (VirtualProtect((void *)pAddress, 4, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		dwValue = *(DWORD *)pAddress;
		VirtualProtect((void *)pAddress, 4, dwProtect, &dwProtect);
	}

	return dwValue;
}

void MH_WriteBYTE(void *pAddress, BYTE ucValue)
{
	DWORD dwProtect;

	if (VirtualProtect((void *)pAddress, 1, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		*(BYTE *)pAddress = ucValue;
		VirtualProtect((void *)pAddress, 1, dwProtect, &dwProtect);
	}
}

BYTE MH_ReadBYTE(void *pAddress)
{
	DWORD dwProtect;
	BYTE ucValue = 0;

	if (VirtualProtect((void *)pAddress, 1, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		ucValue = *(BYTE *)pAddress;
		VirtualProtect((void *)pAddress, 1, dwProtect, &dwProtect);
	}

	return ucValue;
}

void MH_WriteNOP(void *pAddress, DWORD dwCount)
{
	static DWORD dwProtect;

	if (VirtualProtect(pAddress, dwCount, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		for (DWORD i = 0; i < dwCount; i++)
			*(BYTE *)((DWORD)pAddress + i) = 0x90;

		VirtualProtect(pAddress, dwCount, dwProtect, &dwProtect);
	}
}

DWORD MH_WriteMemory(void *pAddress, void *pData, DWORD dwDataSize)
{
	static DWORD dwProtect;

	if (VirtualProtect(pAddress, dwDataSize, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		memcpy(pAddress, pData, dwDataSize);
		VirtualProtect(pAddress, dwDataSize, dwProtect, &dwProtect);
	}

	return dwDataSize;
}

DWORD MH_ReadMemory(void *pAddress, void *pData, DWORD dwDataSize)
{
	static DWORD dwProtect;

	if (VirtualProtect(pAddress, dwDataSize, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		memcpy(pData, pAddress, dwDataSize);
		VirtualProtect(pAddress, dwDataSize, dwProtect, &dwProtect);
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

	if (g_pVideoMode && *g_pVideoMode)
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
	if (g_iEngineType != ENGINE_GOLDSRC_BLOB)
		return (CreateInterfaceFn)GetProcAddress(g_hEngineModule, "CreateInterface");

	static DWORD factoryAddr = 0;

	if (!factoryAddr)
	{
		BlobHeader_t *pHeader = GetBlobHeader();
		DWORD base = pHeader->m_dwExportPoint + 0x8;
		factoryAddr = ((DWORD (*)(void))(base + *(DWORD *)base + 0x4))();
	}

	return (CreateInterfaceFn)factoryAddr;
}

CreateInterfaceFn MH_GetClientFactory(void)
{
	if (MH_GetClientModule())
		return (CreateInterfaceFn)GetProcAddress(MH_GetClientModule(), "CreateInterface");

	if (g_pClientFactory && (*g_pClientFactory))
		return (*g_pClientFactory);

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
		BYTE code = *(BYTE *)pbAddress;

		if (code == 0xFF && *(BYTE *)(pbAddress + 1) == 0x15)
		{
			return *(PVOID *)(pbAddress + 2);
		}

		if (code == 0xE8)
		{
			return (PVOID)(pbAddress + 5 + *(int *)(pbAddress + 1));
		}

		pbAddress++;
	}

	return NULL;
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
	walk_context_s(PVOID a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	PVOID address;
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
	bool bPushRegister;
	bool bSubEspImm;
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

				}, &ctx2);

				if (ctx2.bPushRegister || ctx2.bSubEspImm)
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

					}, &ctx2);

				if (ctx2.bPushRegister || ctx2.bSubEspImm)
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
};