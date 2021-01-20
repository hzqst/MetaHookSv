#include <windows.h>

typedef LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

#define STATUS_SUCCESS ((NTSTATUS)0x00000000)

#define MEM_EXECUTE_OPTION_DISABLE 0x1 
#define MEM_EXECUTE_OPTION_ENABLE 0x2
#define MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION 0x4
#define MEM_EXECUTE_OPTION_PERMANENT 0x8
#define MEM_EXECUTE_OPTION_EXECUTE_DISPATCH_ENABLE 0x10
#define MEM_EXECUTE_OPTION_IMAGE_DISPATCH_ENABLE 0x20 
#define MEM_EXECUTE_OPTION_VALID_FLAGS 0x3f

typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation,
	ProcessQuotaLimits,
	ProcessIoCounters,
	ProcessVmCounters,
	ProcessTimes,
	ProcessBasePriority,
	ProcessRaisePriority,
	ProcessDebugPort,
	ProcessExceptionPort,
	ProcessAccessToken,
	ProcessLdtInformation,
	ProcessLdtSize,
	ProcessDefaultHardErrorMode,
	ProcessIoPortHandlers,
	ProcessPooledUsageAndLimits,
	ProcessWorkingSetWatch,
	ProcessUserModeIOPL,
	ProcessEnableAlignmentFaultFixup,
	ProcessPriorityClass,
	ProcessWx86Information,
	ProcessHandleCount,
	ProcessAffinityMask,
	ProcessPriorityBoost,
	ProcessDeviceMap,
	ProcessSessionInformation,
	ProcessForegroundInformation,
	ProcessWow64Information,
	ProcessImageFileName,
	ProcessLUIDDeviceMapsEnabled,
	ProcessBreakOnTermination,
	ProcessDebugObjectHandle,
	ProcessDebugFlags,
	ProcessHandleTracing,
	ProcessIoPriority,
	ProcessExecuteFlags,
	ProcessResourceManagement,
	ProcessCookie,
	ProcessImageInformation,
	MaxProcessInfoClass
}
PROCESSINFOCLASS;

BOOL Sys_CloseDEP(void)
{
	static NTSTATUS (WINAPI *pfnNtSetInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength) = (NTSTATUS (WINAPI *)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG))GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetInformationProcess");
	ULONG ExecuteFlags = MEM_EXECUTE_OPTION_ENABLE;

	return (pfnNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &ExecuteFlags, sizeof(ExecuteFlags)) == 0);
}

BOOL Sys_GetExecutableName(char *pszName, int nSize)
{
	return GetModuleFileName(GetModuleHandle(NULL), pszName, nSize) != 0;
}

char *Sys_GetLongPathName(void)
{
	char szShortPath[MAX_PATH];
	static char szLongPath[MAX_PATH];
	char *pszPath;

	szShortPath[0] = 0;
	szLongPath[0] = 0;

	if (GetModuleFileName(NULL, szShortPath, sizeof(szShortPath)))
	{
		GetLongPathName(szShortPath, szLongPath, sizeof(szLongPath));
		pszPath = strrchr(szLongPath, '\\');

		if (pszPath[0])
			pszPath[1] = 0;

		size_t len = strlen(szLongPath);

		if (len > 0)
		{
			if (szLongPath[len - 1] == '\\' || szLongPath[len - 1] == '/')
				szLongPath[len - 1] = 0;
		}
	}

	return szLongPath;
}