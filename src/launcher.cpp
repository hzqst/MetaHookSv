#include "metahook.h"
#include <LoadDllMemoryApi.h>
#include <tlhelp32.h> 

#include "LoadBlob.h"
#include "LoadDllNotification.h"
#include "sys.h"

#include <IEngineAPI.h>

#pragma warning(disable : 4733)
#pragma comment(lib, "ws2_32.lib")

IFileSystem_HL25 *g_pFileSystem_HL25 = nullptr;
IFileSystem* g_pFileSystem = nullptr;

PVOID g_BlobLoaderSectionBase = NULL;
ULONG g_BlobLoaderSectionSize = 0;

PVOID MH_GetEngineBase(void);
DWORD MH_GetEngineSize(void);

void MH_LoadEngine(HMODULE hEngineModule, BlobHandle_t hBlobEngine, const char* szGameName, const char* szFullGamePath);
void MH_ExitGame(int iResult);
void MH_Shutdown(void);

extern "C"
{
	void MH_SysError(const char* fmt, ...);
}

#if 0
#include <dbghelp.h>
#include <shlobj.h>
#pragma comment(lib,"dbghelp.lib")
#endif

extern "C"
{
	NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID Base);
	NTSYSAPI NTSTATUS NTAPI NtTerminateProcess(
		HANDLE   ProcessHandle,
		NTSTATUS ExitStatus
	);
}

#if 0

LONG WINAPI MinidumpCallback(EXCEPTION_POINTERS* pException)
{
	HANDLE hDumpFile = CreateFile("minidump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE) {

		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithFullMemory | MiniDumpWithProcessThreadData | MiniDumpWithThreadInfo), &dumpInfo, NULL, NULL);
		CloseHandle(hDumpFile);
	}

	MessageBoxW(NULL, L"A fatal error occured, sorry but we have to terminate this program.", L"MetaHook Fatal Error", MB_ICONWARNING);
	TerminateProcess((HANDLE)(-1), 0);

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

HINTERFACEMODULE LoadFileSystemModule(void)
{
	HINTERFACEMODULE hModule = Sys_LoadModule("filesystem_stdio.dll");

	if (!hModule)
	{
		MessageBox(NULL, "Could not load filesystem dll.\nFileSystem crashed during construction.", "Fatal Error", MB_ICONERROR);
		return NULL;
	}

	return hModule;
}

void SetEngineDLL(const char * szExeName, const char **pszEngineDLL)
{
	*pszEngineDLL = registry->ReadString("EngineDLL", "hw.dll");

	if (!stricmp(szExeName, "svencoop.exe"))
		*pszEngineDLL = "hw.dll";
	else if (CommandLine()->CheckParm("-soft") || CommandLine()->CheckParm("-software"))
		*pszEngineDLL = "sw.dll";
	else if (CommandLine()->CheckParm("-gl") || CommandLine()->CheckParm("-d3d"))
		*pszEngineDLL = "hw.dll";

	registry->WriteString("EngineDLL", *pszEngineDLL);
}

BOOL FindProcess(DWORD dwProcessID)
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (!hProcessSnap)
		return FALSE;

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);

	if (Process32First(hProcessSnap, &pe32))
	{
		while (1)
		{
			if (pe32.th32ProcessID == dwProcessID)
				return TRUE;

			if (!Process32Next(hProcessSnap, &pe32))
				break;
		}
	}

	return FALSE;
}

BOOL SetActiveProcess(void)
{
	HKEY hKey;
	DWORD dwType;
	DWORD dwSize;
	DWORD dwProcessId;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
	{
		DWORD dwDisposition;

		if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
			return FALSE;
	}

	RegQueryValueEx(hKey, "pid", 0, &dwType, (BYTE *)&dwProcessId, &dwSize);

	if (!FindProcess(dwProcessId))
	{
		dwProcessId = GetCurrentProcessId();
		RegSetValueEx(hKey, "pid", 0, REG_DWORD, (BYTE *)&dwProcessId, dwSize);
	}

	RegCloseKey(hKey);
	return TRUE;
}

class CScopedExitFileSystem {
public:
	CScopedExitFileSystem(HINTERFACEMODULE h)
	{
		hFileSystem = h;

		CreateInterfaceFn fsCreateInterface = (CreateInterfaceFn)Sys_GetFactory(hFileSystem);

		auto pFileSystemInterface = (void*)fsCreateInterface(FILESYSTEM_INTERFACE_VERSION, NULL);

		auto pFileSystemInterface_vftable = *(void***)pFileSystemInterface;

		//compare GetFileTime and GetFileChangeTime
		if (0 == memcmp(pFileSystemInterface_vftable[16], pFileSystemInterface_vftable[17], 16))
		{
			g_pFileSystem_HL25 = (IFileSystem_HL25*)pFileSystemInterface;
		}
		else
		{
			g_pFileSystem = (IFileSystem*)pFileSystemInterface;
		}

		FILESYSTEM_ANY_MOUNT();
		FILESYSTEM_ANY_ADDSEARCHPATH(Sys_GetLongPathName(), "ROOT");
	}

	~CScopedExitFileSystem() {

		FILESYSTEM_ANY_UNMOUNT();

		Sys_FreeModule(hFileSystem);
	}

	HINTERFACEMODULE hFileSystem;
};

char* COM_SkipPath(char* pathname)
{
	char* last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname == '/' || *pathname == '\\')
			last = pathname + 1;
		pathname++;
	}
	return last;
}

void COM_FileBase(const char* in, char* out)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.')		// no '.', copy to end
		end = len - 1;
	else
		end--;					// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (start < 0 || (in[start] != '/' && in[start] != '\\'))
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy(out, &in[start], len);
	out[len] = 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	static char szNewCommandParams[2048];

	HANDLE hObject = NULL;

	CommandLine()->CreateCmdLine(GetCommandLine());

#ifndef _DEBUG
	BOOL (*IsDebuggerPresent)(void) = (BOOL (*)(void))GetProcAddress(GetModuleHandle("kernel32.dll"), "IsDebuggerPresent");

	if (!IsDebuggerPresent() && CommandLine()->CheckParm("-nomutex") == NULL)
	{
		hObject = CreateMutex(NULL, FALSE, "ValveHalfLifeLauncherMutex");

		DWORD dwStatus = WaitForSingleObject(hObject, 0);

		if (dwStatus && dwStatus != WAIT_ABANDONED)
		{
			MessageBox(NULL, "Could not launch game.\nOnly one instance of this game can be run at a time.", "Error", MB_ICONERROR);
			return 0;
		}
	}
#endif

	WSAData WSAData;
	WSAStartup(0x202, &WSAData);

	registry->Init();

	char szFullExePath[MAX_PATH];
	Sys_GetExecutableName(szFullExePath, MAX_PATH);

	char* pszExeFullName = COM_SkipPath(szFullExePath);

	char szFullPath[MAX_PATH];
	strncpy(szFullPath, szFullExePath, pszExeFullName - szFullExePath);
	szFullPath[pszExeFullName - szFullExePath] = 0;

	char szExeBaseName[MAX_PATH];
	COM_FileBase(pszExeFullName, szExeBaseName);

	if (CommandLine()->CheckParm("-game") == NULL)
	{
		if (0 == stricmp(szExeBaseName, "svencoop"))
		{
			CommandLine()->AppendParm("-game", "svencoop");
		}
		if (0 == stricmp(szExeBaseName, "hl"))
		{
			CommandLine()->AppendParm("-game", "valve");
		}
		else
		{
			CommandLine()->AppendParm("-game", szExeBaseName);
		}
	}

	char szGameName[32] = { 0 };
	const char *pszGameName = NULL;
	const char *szGameStr = CommandLine()->CheckParm("-game", &pszGameName);

	strncpy(szGameName, (pszGameName) ? pszGameName : "valve", sizeof(szGameName) - 1);
	szGameName[sizeof(szGameName) - 1] = 0;

	//"czero" or "czeror"
	if (pszGameName && 0 == strnicmp(pszGameName, "czero", sizeof("czero") - 1))
		CommandLine()->AppendParm("-forcevalve", NULL);

	if (registry->ReadInt("CrashInitializingVideoMode", FALSE))
	{
		registry->WriteInt("CrashInitializingVideoMode", FALSE);

		auto hw = registry->ReadString("EngineDLL", "hw.dll");

		if (hw[0] && 0 != strcmp(hw, "hw.dll"))
		{
			if (registry->ReadInt("EngineD3D", FALSE))
			{
				registry->WriteInt("EngineD3D", FALSE);

				if (MessageBox(NULL, "The game has detected that the previous attempt to start in D3D video mode failed.\nThe game will now run attempt to run in openGL mode.", "Video mode change failure", MB_OKCANCEL | MB_ICONWARNING) != IDOK)
					return 0;
			}
			else
			{
				registry->WriteString("EngineDLL", "sw.dll");

				if (MessageBox(NULL, "The game has detected that the previous attempt to start in openGL video mode failed.\nThe game will now run in software mode.", "Video mode change failure", MB_OKCANCEL | MB_ICONWARNING) != IDOK)
					return 0;
			}

			registry->WriteInt("ScreenWidth", 640);
			registry->WriteInt("ScreenHeight", 480);
			registry->WriteInt("ScreenBPP", 32);
		}
	}

	while (1)
	{
		HINTERFACEMODULE hFileSystem = LoadFileSystemModule();

		if (!hFileSystem)
			break;

		CScopedExitFileSystem ScopedExitFileSystem(hFileSystem);

		const char *pszEngineDLL = NULL;
		int iResult = ENGINE_RESULT_NONE;

		SetEngineDLL(pszExeFullName, &pszEngineDLL);

		memset(szNewCommandParams, 0, sizeof(szNewCommandParams));

		IEngineAPI *EngineAPI = NULL;
		HINTERFACEMODULE hEngine = NULL;
		BlobHandle_t hBlobEngine = NULL;
		HMEMORYMODULE hMirroredEngine = NULL;

		if (FIsBlob(pszEngineDLL))
		{
#if defined(METAHOOK_BLOB_SUPPORT) || defined(_DEBUG)
			if (!g_BlobLoaderSectionBase)
			{
				g_BlobLoaderSectionBase = GetBlobLoaderSection((PVOID)hInstance, &g_BlobLoaderSectionSize);

				if (!g_BlobLoaderSectionBase)
				{
					MH_SysError("No available \".blob\" section to load blob engine : %s.", pszEngineDLL);
					return 0;
				}
				else
				{
					DWORD dwOldProtect = 0;
					if (!VirtualProtect(g_BlobLoaderSectionBase, g_BlobLoaderSectionSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
					{
						MH_SysError("Failed to make \".blob\" section executable for blob engine : %s.", pszEngineDLL);
						return 0;
					}
				}
			}
#else
			if (1)
			{
				MH_SysError("This build of metahook does not support blob engine : %s.\nPlease use metahook_blob.exe instead.", pszEngineDLL);
				return 0;
			}
#endif
			hBlobEngine = LoadBlobFile(pszEngineDLL, g_BlobLoaderSectionBase, g_BlobLoaderSectionSize);

			if (!hBlobEngine)
			{
				MH_SysError("Could not load engine : %s.", pszEngineDLL);
				return 0;
			}

			if (hBlobEngine)
			{
				BlobLoaderAddBlob(hBlobEngine);
				RunDllMainForBlob(hBlobEngine, DLL_PROCESS_ATTACH);
				RunExportEntryForBlob(hBlobEngine, (void**)&EngineAPI);

				if (!EngineAPI)
				{
					MH_SysError("Could not get EngineAPI from engine : %s.", pszEngineDLL);
					return 0;
				}
			}
		}
		else
		{
			hEngine = Sys_LoadModule(pszEngineDLL);

			if (!hEngine)
			{
				MH_SysError("Could not load engine : %s.", pszEngineDLL);
				return 0;
			}

			CreateInterfaceFn EngineFactory = (CreateInterfaceFn)Sys_GetFactory(hEngine);

			if (!EngineFactory)
			{
				MH_SysError("Could not get factory from engine : %s.", pszEngineDLL);
				return 0;
			}

			EngineAPI = (IEngineAPI *)EngineFactory(VENGINE_LAUNCHER_API_VERSION, NULL);

			if (!EngineAPI)
			{
				MH_SysError("Could not get EngineAPI from engine : %s.", pszEngineDLL);
				return 0;
			}
		}

		if (EngineAPI)
		{
			MH_LoadEngine((HMODULE)hEngine, hBlobEngine, szGameName, szFullPath);

			iResult = EngineAPI->Run(hInstance, Sys_GetLongPathName(), CommandLine()->GetCmdLine(), szNewCommandParams, Sys_GetFactoryThis(), Sys_GetFactory(hFileSystem));

			MH_ExitGame(iResult);
			
			if (hBlobEngine)
			{
				MH_DispatchLoadBlobNotificationCallback(hBlobEngine, LOAD_DLL_NOTIFICATION_IS_UNLOAD);
				FreeBlobModule(hBlobEngine);
				BlobLoaderRemoveBlob(hBlobEngine);
			}
			else
			{
				MH_DispatchLoadLdrDllNotificationCallback(NULL, NULL, MH_GetEngineBase(), MH_GetEngineSize(), LOAD_DLL_NOTIFICATION_IS_UNLOAD);
				Sys_FreeModule(hEngine);
			}

			MH_Shutdown();
		}

		if (iResult == ENGINE_RESULT_NONE || iResult > ENGINE_RESULT_UNSUPPORTEDVIDEO)
			break;

		bool bContinue = false;

		switch (iResult)
		{
			case ENGINE_RESULT_RESTART:
			{
				bContinue = true;
				break;
			}

			case ENGINE_RESULT_UNSUPPORTEDVIDEO:
			{
				registry->WriteInt("ScreenWidth", 640);
				registry->WriteInt("ScreenHeight", 480);
				registry->WriteInt("ScreenBPP", 16);
				registry->WriteString("EngineDLL", "sw.dll");

				bContinue = MessageBox(NULL, "The specified video mode is not supported.\nThe game will now run in software mode.", "Video mode change failure", MB_OKCANCEL | MB_ICONWARNING) != IDOK;
				break;
			}
		}

		CommandLine()->RemoveParm("-sw");
		CommandLine()->RemoveParm("-startwindowed");
		CommandLine()->RemoveParm("-windowed");
		CommandLine()->RemoveParm("-window");
		CommandLine()->RemoveParm("-full");
		CommandLine()->RemoveParm("-fullscreen");
		CommandLine()->RemoveParm("-soft");
		CommandLine()->RemoveParm("-software");
		CommandLine()->RemoveParm("-gl");
		CommandLine()->RemoveParm("-d3d");
		CommandLine()->RemoveParm("-w");
		CommandLine()->RemoveParm("-width");
		CommandLine()->RemoveParm("-h");
		CommandLine()->RemoveParm("-height");
		CommandLine()->RemoveParm("-novid");

		if (strstr(szNewCommandParams, "-game"))
			CommandLine()->RemoveParm("-game");

		if (strstr(szNewCommandParams, "+load"))
			CommandLine()->RemoveParm("+load");

		CommandLine()->AppendParm(szNewCommandParams, NULL);

		if (!bContinue)
			break;
	}

	registry->Shutdown();

	if (hObject)
	{
		ReleaseMutex(hObject);
		CloseHandle(hObject);
	}

	WSACleanup();

	return 1;
}