#include "metahook.h"
#include "LoadBlob.h"
#include "Detours\detours.h"
#include "interface.h"

#include <IPluginsV1.h>

struct hook_s
{
	void *pOldFuncAddr;
	void *pNewFuncAddr;
	void *pClass;
	int iTableIndex;
	int iFuncIndex;
	HMODULE hModule;
	const char *pszModuleName;
	const char *pszFuncName;
	struct hook_s *pNext;
	void *pInfo;
};

int (*g_pfnbuild_number)(void);
void *g_pClientDLL_Init;
int (*g_pfnClientDLL_Init)(void);
hook_t *g_phClientDLL_Init;

BOOL g_bEngineIsBlob;
HMODULE g_hEngineModule;
DWORD g_dwEngineBase;
DWORD g_dwEngineSize;
hook_t *g_pHookBase;
cl_exportfuncs_t *g_pExportFuncs;
bool g_bSaveVideo;
char g_szTempFile[MAX_PATH];
bool g_bIsNewEngine = false;
bool g_bIsSvEngine = false;

hook_t *MH_FindInlineHooked(void *pOldFuncAddr);
hook_t *MH_FindVFTHooked(void *pClass, int iTableIndex, int iFuncIndex);
hook_t *MH_FindIATHooked(HMODULE hModule, const char *pszModuleName, const char *pszFuncName);
BOOL MH_UnHook(hook_t *pHook);
hook_t *MH_InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void *&pCallBackFuncAddr);
hook_t *MH_VFTHook(void *pClass, int iTableIndex, int iFuncIndex, void *pNewFuncAddr, void *&pCallBackFuncAddr);
hook_t *MH_IATHook(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void *&pCallBackFuncAddr);
void *MH_GetClassFuncAddr(...);
DWORD MH_GetModuleBase(HMODULE hModule);
DWORD MH_GetModuleSize(HMODULE hModule);
void *MH_SearchPattern(void *pStartSearch, DWORD dwSearchLen, char *pPattern, DWORD dwPatternLen);
void MH_WriteDWORD(void *pAddress, DWORD dwValue);
DWORD MH_ReadDWORD(void *pAddress);
void MH_WriteBYTE(void *pAddress, BYTE ucValue);
BYTE MH_ReadBYTE(void *pAddress);
void MH_WriteNOP(void *pAddress, DWORD dwCount);
DWORD MH_WriteMemory(void *pAddress, BYTE *pData, DWORD dwDataSize);
DWORD MH_ReadMemory(void *pAddress, BYTE *pData, DWORD dwDataSize);
DWORD MH_GetVideoMode(int *wide, int *height, int *bpp, bool *windowed);
DWORD MH_GetEngineVersion(void);

#define BUILD_NUMBER_SIG "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x08\x2A\x33\x2A\x85\xC0"
#define BUILD_NUMBER_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\xA1\x2A\x2A\x2A\x2A\x56\x33\xF6\x85\xC0\x0F\x85\x2A\x2A\x2A\x2A\x53\x33\xDB\x8B\x04\x9D"
#define BUILD_NUMBER_SIG_SVENGINE "\x51\xA1\x2A\x2A\x2A\x2A\x2A\x33\x2A\x85\xC0\x0F\x85\x2A\x2A\x2A\x2A\x2A\x2A\x33\x2A\xEB"
#define CLIENTDLL_INIT_SIG "\x81\xEC\x00\x04\x00\x00\x8D\x44\x24\x00\x68\x2A\x2A\x2A\x2A\x68\x00\x02\x00\x00\x50\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\xC4\x0C\x85\xC0"
#define CLIENTDLL_INIT_SIG_NEW "\x55\x8B\xEC\x81\xEC\x00\x02\x00\x00\x68\x2A\x2A\x2A\x2A\x8D\x85\x00\xFE\xFF\xFF\x68\x00\x02\x00\x00\x50\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\xC4\x0C\x85\xC0\x74\x2A\xE8"
#define CLIENTDLL_INIT_SIG_SVENGINE "\x81\xEC\x2A\x02\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x68\x2A\x2A\x2A\x2A\x8D\x44\x24\x2A\x68\x00\x02\x00\x00\x50\xE8"

typedef struct plugin_s
{
	char *filename;
	HINTERFACEMODULE module;
	IBaseInterface *pPluginAPI;
	int iInterfaceVersion;
	struct plugin_s *next;
}
plugin_t;

plugin_t *g_pPluginBase;

extern IFileSystem *g_pFileSystem;

mh_interface_t gInterface;
mh_enginesave_t gMetaSave;

extern metahook_api_t gMetaHookAPI;

bool HM_LoadPlugins(char *filename, HINTERFACEMODULE hModule)
{
	plugin_t *plug = new plugin_t;
	plug->module = hModule;

	CreateInterfaceFn fnCreateInterface = Sys_GetFactory(plug->module);
	plug->pPluginAPI = fnCreateInterface(METAHOOK_PLUGIN_API_VERSION, NULL);

	if (plug->pPluginAPI)
	{
		((IPlugins *)plug->pPluginAPI)->Init(&gMetaHookAPI, &gInterface, &gMetaSave);
		plug->iInterfaceVersion = 2;
	}
	else
	{
		plug->pPluginAPI = fnCreateInterface(METAHOOK_PLUGIN_API_VERSION_V1, NULL);

		if (plug->pPluginAPI)
			plug->iInterfaceVersion = 1;
		else
			plug->iInterfaceVersion = 0;
	}

	plug->filename = strdup(filename);
	plug->next = g_pPluginBase;
	g_pPluginBase = plug;
	return true;
}

void MH_Init(const char *pszGameName)
{
	g_pfnbuild_number = NULL;
	g_pfnClientDLL_Init = NULL;
	g_phClientDLL_Init = NULL;

	g_dwEngineBase = 0;
	g_dwEngineSize = 0;
	g_pHookBase = NULL;
	g_pExportFuncs = NULL;
	g_bSaveVideo = false;
	g_szTempFile[0] = 0;

	gInterface.CommandLine = CommandLine();
	gInterface.FileSystem = g_pFileSystem;
	gInterface.Registry = registry;

	char metapath[MAX_PATH], filename[MAX_PATH];
	sprintf(metapath, "%s/metahook", pszGameName);
	sprintf(filename, "%s/configs/plugins.lst", metapath);

	FILE *fp = fopen(filename, "rt");

	if (fp)
	{
		static char line[1024];

		while (!feof(fp))
		{
			static char plugins[64];
			fgets(line, sizeof(line), fp);

			if (line[0] == '\0' || line[0] == ';')
				continue;

			if (sscanf(line, "%s", plugins) != 1)
				continue;

			if (!isalnum(plugins[0]))
				continue;

			sprintf(filename, "%s/plugins/%s", metapath, plugins);

			HINTERFACEMODULE hModule = Sys_LoadModule(filename);

			if (!hModule)
			{
				static char msg[512];
				wsprintf(msg, "Could not load %s.\nPlease try again at a later time.", plugins);
				MessageBox(NULL, msg, "Warning", MB_ICONWARNING);
				continue;
			}

			if (!HM_LoadPlugins(plugins, hModule))
				continue;
		}

		fclose(fp);
	}
}

int ClientDLL_Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	gMetaSave.pExportFuncs = new cl_exportfuncs_t;
	gMetaSave.pEngineFuncs = new cl_enginefunc_t;

	memcpy(gMetaSave.pExportFuncs, g_pExportFuncs, sizeof(cl_exportfuncs_t));
	memcpy(gMetaSave.pEngineFuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next)
	{
		if (plug->iInterfaceVersion > 1)
			((IPlugins *)plug->pPluginAPI)->LoadClient(g_pExportFuncs);
		else
			((IPluginsV1 *)plug->pPluginAPI)->Init(g_pExportFuncs);
	}

	return g_pExportFuncs->Initialize(pEnginefuncs, iVersion);
}

void MH_ClientDLL_Init(void)
{
	DWORD dwResult = (DWORD)MH_SearchPattern((void *)((DWORD)g_pClientDLL_Init + 0xB0), 0xFF, "\x6A\x07\x68", 3);

	if (!dwResult)
	{
		return;
	}

	g_pExportFuncs = *(cl_exportfuncs_t **)(dwResult + 0x9);

	static DWORD dwClientDLL_Initialize[1];
	dwClientDLL_Initialize[0] = (DWORD)&ClientDLL_Initialize;
	MH_WriteDWORD((void *)(dwResult + 0x9), (DWORD)dwClientDLL_Initialize);
	g_pfnClientDLL_Init();
}

void MH_LoadEngine(HMODULE hModule)
{
	gInterface.FileSystem = g_pFileSystem;

	if (hModule)
	{
		g_dwEngineBase = MH_GetModuleBase(hModule);
		g_dwEngineSize = MH_GetModuleSize(hModule);
		g_hEngineModule = hModule;
		g_bEngineIsBlob = FALSE;
	}
	else
	{
		g_dwEngineBase = 0x1D01000;
		g_dwEngineSize = 0x1000000;
		g_hEngineModule = GetModuleHandle(NULL);
		g_bEngineIsBlob = TRUE;
	}

	g_bIsNewEngine = false;
	g_pfnbuild_number = (int (*)(void))MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, BUILD_NUMBER_SIG, sizeof(BUILD_NUMBER_SIG) - 1);

	if (!g_pfnbuild_number)
	{
		g_pfnbuild_number = (int (*)(void))MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, BUILD_NUMBER_SIG_NEW, sizeof(BUILD_NUMBER_SIG_NEW) - 1);
		g_bIsNewEngine = true;
	}

	if (!g_pfnbuild_number)
	{
		g_pfnbuild_number = (int(*)(void))MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, BUILD_NUMBER_SIG_SVENGINE, sizeof(BUILD_NUMBER_SIG_SVENGINE) - 1);
		g_bIsSvEngine = true;
	}

	if (!g_pfnbuild_number)
	{
		MessageBox(NULL, "Failed to locate buildnumber.", "Fatal Error", MB_ICONERROR);
		ExitProcess(0);
	}

	if(g_bIsSvEngine)
		g_pClientDLL_Init = MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, CLIENTDLL_INIT_SIG_SVENGINE, sizeof(CLIENTDLL_INIT_SIG_SVENGINE) - 1);
	else if (g_bIsNewEngine)
		g_pClientDLL_Init = MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, CLIENTDLL_INIT_SIG_NEW, sizeof(CLIENTDLL_INIT_SIG_NEW) - 1);
	else
		g_pClientDLL_Init = MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, CLIENTDLL_INIT_SIG, sizeof(CLIENTDLL_INIT_SIG) - 1);

	if (!g_pClientDLL_Init)
	{
		MessageBox(NULL, "Failed to locate ClientDLL_Init.", "Fatal Error", MB_ICONERROR);
		ExitProcess(0);
	}

	g_phClientDLL_Init = MH_InlineHook(g_pClientDLL_Init, MH_ClientDLL_Init, (void *&)g_pfnClientDLL_Init);

	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next)
	{
		if (plug->iInterfaceVersion > 1)
			((IPlugins *)plug->pPluginAPI)->LoadEngine();
	}
}

void MH_ExitGame(int iResult)
{
	for (plugin_t *plug = g_pPluginBase; plug; plug = plug->next)
	{
		if (plug->iInterfaceVersion > 1)
			((IPlugins *)plug->pPluginAPI)->ExitGame(iResult);
	}
}

void MH_FreeAllPlugin(void)
{
	plugin_t *plug = g_pPluginBase;

	while (plug)
	{
		plugin_t *pfree = plug;
		plug = plug->next;

		if (pfree->pPluginAPI)
		{
			if (pfree->iInterfaceVersion > 1)
				((IPlugins *)pfree->pPluginAPI)->Shutdown();
		}

		free(pfree->filename);
		Sys_FreeModule(pfree->module);
		delete pfree;
	}

	g_pPluginBase = NULL;
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
			if (pfree->iInterfaceVersion > 1)
				((IPlugins *)pfree->pPluginAPI)->Shutdown();
		}

		free(pfree->filename);
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
}

hook_t *MH_NewHook(void)
{
	hook_t *h = new hook_t;
	memset(h, 0, sizeof(hook_t));
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
	if (pHook->pClass)
	{
		tagVTABLEDATA *info = (tagVTABLEDATA *)pHook->pInfo;
		MH_WriteMemory(info->pVFTInfoAddr, (BYTE *)pHook->pOldFuncAddr, sizeof(DWORD));
	}
	else if (pHook->hModule)
	{
		tagIATDATA *info = (tagIATDATA *)pHook->pInfo;
		MH_WriteMemory(info->pAPIInfoAddr, (BYTE *)pHook->pOldFuncAddr, sizeof(DWORD));
	}
	else
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(void *&)pHook->pOldFuncAddr, pHook->pNewFuncAddr);
		DetourTransactionCommit();
	}

	if (pHook->pInfo)
		delete pHook->pInfo;

	delete pHook;
}

void MH_FreeAllHook(void)
{
	hook_t *next = NULL;

	for (hook_t *h = g_pHookBase; h; h = next)
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

hook_t *MH_InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void *&pCallBackFuncAddr)
{
	hook_t *h = MH_NewHook();
	h->pOldFuncAddr = pOldFuncAddr;
	h->pNewFuncAddr = pNewFuncAddr;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(void *&)h->pOldFuncAddr, pNewFuncAddr);
	DetourTransactionCommit();

	pCallBackFuncAddr = h->pOldFuncAddr; 
	return h;
}

hook_t *MH_VFTHook(void *pClass, int iTableIndex, int iFuncIndex, void *pNewFuncAddr, void *&pCallBackFuncAddr)
{
	tagVTABLEDATA *info = new tagVTABLEDATA;
	info->pInstance = (tagCLASS *)pClass;

	DWORD *pVMT = ((tagCLASS *)pClass + iTableIndex)->pVMT;
	info->pVFTInfoAddr = pVMT + iFuncIndex;

	hook_t *h = MH_NewHook();
	h->pOldFuncAddr = (void *)pVMT[iFuncIndex];
	h->pNewFuncAddr = pNewFuncAddr;
	h->pInfo = info;
	h->pClass = pClass;
	h->iTableIndex = iTableIndex;
	h->iFuncIndex = iFuncIndex;

	pCallBackFuncAddr = h->pOldFuncAddr;
	MH_WriteMemory(info->pVFTInfoAddr, (BYTE *)&pNewFuncAddr, sizeof(DWORD));
	return h;
}

hook_t *MH_IATHook(HMODULE hModule, const char *pszModuleName, const char *pszFuncName, void *pNewFuncAddr, void *&pCallBackFuncAddr)
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

	hook_t *h = MH_NewHook();
	h->pOldFuncAddr = (void *)pThunk->u1.Function;
	h->pNewFuncAddr = pNewFuncAddr;
	h->pInfo = info;
	h->hModule = hModule;
	h->pszModuleName = pszModuleName;
	h->pszFuncName = pszFuncName;

	pCallBackFuncAddr = h->pOldFuncAddr;
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

DWORD MH_GetModuleBase(HMODULE hModule)
{
	MEMORY_BASIC_INFORMATION mem;

	if (!VirtualQuery(hModule, &mem, sizeof(MEMORY_BASIC_INFORMATION)))
		return 0;

	return (DWORD)mem.AllocationBase;
}

DWORD MH_GetModuleSize(HMODULE hModule)
{
	return ((IMAGE_NT_HEADERS *)((DWORD)hModule + ((IMAGE_DOS_HEADER *)hModule)->e_lfanew))->OptionalHeader.SizeOfImage;
}

HMODULE MH_GetEngineModule(void)
{
	return g_hEngineModule;
}

DWORD MH_GetEngineBase(void)
{
	return g_dwEngineBase;
}

DWORD MH_GetEngineSize(void)
{
	return g_dwEngineSize;
}

void *MH_SearchPattern(void *pStartSearch, DWORD dwSearchLen, char *pPattern, DWORD dwPatternLen)
{
	DWORD dwStartAddr = (DWORD)pStartSearch;
	DWORD dwEndAddr = dwStartAddr + dwSearchLen - dwPatternLen;

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

DWORD MH_WriteMemory(void *pAddress, BYTE *pData, DWORD dwDataSize)
{
	static DWORD dwProtect;

	if (VirtualProtect(pAddress, dwDataSize, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		memcpy(pAddress, pData, dwDataSize);
		VirtualProtect(pAddress, dwDataSize, dwProtect, &dwProtect);
	}

	return dwDataSize;
}

DWORD MH_ReadMemory(void *pAddress, BYTE *pData, DWORD dwDataSize)
{
	static DWORD dwProtect;

	if (VirtualProtect(pAddress, dwDataSize, PAGE_EXECUTE_READWRITE, &dwProtect))
	{
		memcpy(pData, pAddress, dwDataSize);
		VirtualProtect(pAddress, dwDataSize, dwProtect, &dwProtect);
	}

	return dwDataSize;
}

DWORD MH_GetVideoMode(int *width, int *height, int *bpp, bool *windowed)
{
	static int iSaveMode;
	static int iSaveWidth, iSaveHeight, iSaveBPP;
	static bool bSaveWindowed;

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
	if (!g_bEngineIsBlob)
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

DWORD MH_GetNextCallAddr(void *pAddress, DWORD dwCount)
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
			return *(DWORD *)(pbAddress + 2);
		}

		if (code == 0xE8)
		{
			return (DWORD)(*(DWORD *)(pbAddress + 1) + pbAddress + 5);
		}

		pbAddress++;
	}

	return 0;
}

DWORD MH_GetEngineVersion(void)
{
	if (!g_pfnbuild_number)
		return 0;

	return g_pfnbuild_number();
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
};