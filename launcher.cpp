#include "metahook.h"
#include <IEngine.h>
#include "LoadBlob.h"
#include "ExceptHandle.h"
#include "sys.h"
#include <tlhelp32.h> 

#pragma warning(disable : 4733)
#pragma comment(lib, "ws2_32.lib")

IFileSystem *g_pFileSystem;

#include <dbghelp.h>
#include <shlobj.h>
#pragma comment(lib,"dbghelp.lib")

extern "C"
{
	NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID Base);
	NTSYSAPI NTSTATUS NTAPI NtTerminateProcess(
		HANDLE   ProcessHandle,
		NTSTATUS ExitStatus
	);
}

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

HINTERFACEMODULE LoadFilesystemModule(void)
{
	HINTERFACEMODULE hModule = Sys_LoadModule("filesystem_stdio.dll");

	if (!hModule)
	{
		MessageBox(NULL, "Could not load filesystem dll.\nFileSystem crashed during construction.", "Fatal Error", MB_ICONERROR);
		return NULL;
	}

	return hModule;
}

void SetEngineDLL(const char *&pszEngineDLL)
{
	pszEngineDLL = registry->ReadString("EngineDLL", "hw.dll");

	if (CommandLine()->CheckParm("-soft") || CommandLine()->CheckParm("-software"))
		pszEngineDLL = "sw.dll";
	else if (CommandLine()->CheckParm("-gl") || CommandLine()->CheckParm("-d3d"))
		pszEngineDLL = "hw.dll";

	registry->WriteString("EngineDLL", pszEngineDLL);
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

BlobFootprint_t g_blobfootprintClient;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HANDLE hObject = NULL;

	CommandLine()->CreateCmdLine(GetCommandLine());
	CommandLine()->RemoveParm("-steam");

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

	//SetUnhandledExceptionFilter(MinidumpCallback);

	char szFileName[256];
	Sys_GetExecutableName(szFileName, sizeof(szFileName));
	char *szExeName = strrchr(szFileName, '\\') + 1;

	if(INVALID_FILE_ATTRIBUTES != GetFileAttributesA("svends.exe"))
		CommandLine()->AppendParm("-game", "svencoop");

	if (stricmp(szExeName, "hl.exe") && CommandLine()->CheckParm("-game") == NULL)
	{
		szExeName[strlen(szExeName) - 4] = '\0';
		CommandLine()->AppendParm("-game", szExeName);
	}

	static char szGameName[32];
	const char *_szGameName;
	const char *szGameStr = CommandLine()->CheckParm("-game", &_szGameName);

	if (szGameStr)
		strcpy(szGameName, _szGameName);
	else
		strcpy(szGameName, "valve");

	if (szGameStr && !strnicmp(&szGameStr[6], "czero", 5))
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
			registry->WriteInt("ScreenBPP", 16);
		}
	}

	while (1)
	{
		HINTERFACEMODULE hFileSystem = LoadFilesystemModule();

		if (!hFileSystem)
			break;

		CreateInterfaceFn fsCreateInterface = (CreateInterfaceFn)Sys_GetFactory(hFileSystem);
		g_pFileSystem = (IFileSystem *)fsCreateInterface(FILESYSTEM_INTERFACE_VERSION, NULL);
		g_pFileSystem->Mount();
		g_pFileSystem->AddSearchPath(Sys_GetLongPathName(), "ROOT");

		MH_Init(szGameName);

		static char szNewCommandParams[2048];
		const char *pszEngineDLL;
		int iResult = ENGINE_RESULT_NONE;

		SetActiveProcess();
		SetEngineDLL(pszEngineDLL);

		szNewCommandParams[0] = 0;
		g_blobfootprintClient.m_hDll = NULL;

		IEngine *engineAPI = NULL;
		HINTERFACEMODULE hEngine;
		bool bUseBlobDLL = false;

		if (FIsBlob(pszEngineDLL))
		{
#ifndef _USRDLL
			Sys_CloseDEP();
			SetupExceptHandler3();
#endif
			NLoadBlobFile(pszEngineDLL, &g_blobfootprintClient, (void **)&engineAPI);
			bUseBlobDLL = true;
		}
		else
		{
			hEngine = Sys_LoadModule(pszEngineDLL);

			if (!hEngine)
			{
				static char msg[512];
				wsprintf(msg, "Could not load metahook plugin: %s.", pszEngineDLL);
				MessageBox(NULL, msg, "Fatal Error", MB_ICONERROR);
				ExitProcess(0);
			}

			CreateInterfaceFn engineCreateInterface = (CreateInterfaceFn)Sys_GetFactory(hEngine);
			engineAPI = (IEngine *)engineCreateInterface(VENGINE_LAUNCHER_API_VERSION, NULL);

			if (!engineCreateInterface || !engineAPI)
				Sys_FreeModule(hEngine);
		}

		if (engineAPI)
		{
			MH_LoadEngine(bUseBlobDLL ? NULL : (HMODULE)hEngine);
			iResult = engineAPI->Run(hInstance, Sys_GetLongPathName(), CommandLine()->GetCmdLine(), szNewCommandParams, Sys_GetFactoryThis(), Sys_GetFactory(hFileSystem));
			MH_ExitGame(iResult);
			MH_Shutdown();

			if (bUseBlobDLL)
			{
				FreeBlob(&g_blobfootprintClient);
			}
			else
			{
				Sys_FreeModule(hEngine);
			}
		}

		if (iResult == ENGINE_RESULT_NONE || iResult > ENGINE_RESULT_UNSUPPORTEDVIDEO)
			break;

		bool bContinue;

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

		g_pFileSystem->Unmount();
		Sys_FreeModule(hFileSystem);

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
	MH_Shutdown();
	NtTerminateProcess((HANDLE)-1, 1);
	return 1;
}