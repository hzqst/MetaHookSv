#include <Windows.h>
#include "metahook.h"

void MH_SysError(const char* fmt, ...);

static CRITICAL_SECTION g_BlobThreadManagerLock;
static HANDLE g_hBlobAliveThread[MAXIMUM_WAIT_OBJECTS] = { 0 };
static HANDLE g_hBlobClosedThread[MAXIMUM_WAIT_OBJECTS] = { 0 };

static HANDLE(WINAPI* g_pfnCreateThread)(LPSECURITY_ATTRIBUTES   lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE  lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) = NULL;
static BOOL(WINAPI* g_pfnCloseHandle)(HANDLE hObject) = NULL;
static BOOL(WINAPI* g_pfnTerminateThread)(HANDLE hThread, DWORD  dwExitCode) = NULL;

void InitBlobThreadManager(void)
{
	InitializeCriticalSection(&g_BlobThreadManagerLock);

	g_pfnCreateThread = (decltype(g_pfnCreateThread))GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateThread");
	g_pfnTerminateThread = (decltype(g_pfnTerminateThread))GetProcAddress(GetModuleHandleA("kernel32.dll"), "TerminateThread");
	g_pfnCloseHandle = (decltype(g_pfnCloseHandle))GetProcAddress(GetModuleHandleA("kernel32.dll"), "CloseHandle");
}

void ShutdownBlobThreadManager(void)
{
	DeleteCriticalSection(&g_BlobThreadManagerLock);
}

void BlobEnterCritSection(void)
{
	EnterCriticalSection(&g_BlobThreadManagerLock);
}

void BlobLeaveCritSection(void)
{
	LeaveCriticalSection(&g_BlobThreadManagerLock);
}

bool BlobFindAliveThread(HANDLE hThread)
{
	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobAliveThread); ++i)
	{
		if (g_hBlobAliveThread[i] == hThread)
		{
			return true;
		}
	}
	return false;
}

bool BlobFindAndRemoveAliveThread(HANDLE hThread)
{
	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobAliveThread); ++i)
	{
		if (g_hBlobAliveThread[i] == hThread)
		{
			g_hBlobAliveThread[i] = 0;
			return true;
		}
	}
	return false;
}

bool BlobAddAliveThread(HANDLE hThread)
{
	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobAliveThread); ++i)
	{
		if (g_hBlobAliveThread[i] == 0)
		{
			g_hBlobAliveThread[i] = hThread;
			return true;
		}
	}
	return false;
}

bool BlobFindClosedThread(HANDLE hThread)
{
	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobClosedThread); ++i)
	{
		if (g_hBlobClosedThread[i] == hThread)
		{
			return true;
		}
	}
	return false;
}

bool BlobAddClosedThread(HANDLE hThread)
{
	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobClosedThread); ++i)
	{
		if (g_hBlobClosedThread[i] == 0)
		{
			g_hBlobClosedThread[i] = hThread;
			return true;
		}
	}
	return false;
}

bool BlobFindAndRemoveSignaledAliveThread(DWORD* pIndex)
{
	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobAliveThread); ++i)
	{
		if (g_hBlobAliveThread[i])
		{
			if (WAIT_OBJECT_0 == WaitForSingleObject(g_hBlobAliveThread[i], 0))
			{
				g_hBlobAliveThread[i] = 0;
				if (pIndex)
					*pIndex = i;
				return true;
			}
		}
	}

	return false;
}

bool BlobFindAndRemoveSignaledClosedThread(DWORD* pIndex)
{
	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobClosedThread); ++i)
	{
		if (g_hBlobClosedThread[i])
		{
			if (WAIT_OBJECT_0 == WaitForSingleObject(g_hBlobClosedThread[i], 0))
			{
				g_hBlobClosedThread[i] = 0;
				if (pIndex)
					*pIndex = i;
				return true;
			}
		}
	}

	return false;
}

HANDLE WINAPI BlobCreateThread(
	LPSECURITY_ATTRIBUTES   lpThreadAttributes,
	SIZE_T                  dwStackSize,
	LPTHREAD_START_ROUTINE  lpStartAddress,
	LPVOID					lpParameter,
	DWORD                   dwCreationFlags,
	LPDWORD                 lpThreadId
)
{
	DWORD originalCreationFlags = dwCreationFlags;

	auto hThreadHandle = g_pfnCreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);

	if (hThreadHandle)
	{
		BlobEnterCritSection();

		bool bAdded = BlobAddAliveThread(hThreadHandle);

		if (!bAdded)
		{
			DWORD NewIndex = 0;
			if (BlobFindAndRemoveSignaledAliveThread(&NewIndex))
			{
				g_hBlobAliveThread[NewIndex] = hThreadHandle;
				bAdded = true;
			}
		}

		BlobLeaveCritSection();

		if (!bAdded)
		{
			MH_SysError("Failed to insert thread to thread manager!");
		}
	}

	return hThreadHandle;
}

BOOL WINAPI BlobCloseHandle(HANDLE hObject)
{
	BlobEnterCritSection();

	bool bFoundAlive = BlobFindAndRemoveAliveThread(hObject);

	bool bAdded = false;

	if (bFoundAlive)
	{
		bAdded = BlobAddClosedThread(hObject);

		if (!bAdded)
		{
			DWORD NewIndex = 0;

			if (BlobFindAndRemoveSignaledClosedThread(&NewIndex))
			{
				g_hBlobClosedThread[NewIndex] = hObject;
				bAdded = true;
			}
		}
	}

	BlobLeaveCritSection();

	if (bFoundAlive)
	{
		if (!bAdded)
			MH_SysError("Failed to insert thread to thread manager!");

		return TRUE;
	}

	return g_pfnCloseHandle(hObject);
}

BOOL WINAPI BlobTerminateThread(
	HANDLE hThread,
	DWORD  dwExitCode
)
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, INFINITE))
	{
		BlobEnterCritSection();

		bool bFoundAlive = BlobFindAndRemoveAliveThread(hThread);

		CloseHandle(hThread);

		BlobLeaveCritSection();

		return TRUE;
	}

	return g_pfnTerminateThread(hThread, dwExitCode);
}

#if 0
void BlobRunFrame(void)
{
	HANDLE hThreads[MAXIMUM_WAIT_OBJECTS] = { 0 };
	DWORD numThreads = 0;

	EnterCriticalSection(&g_BlobThreadManagerLock);

	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobAliveThread); ++i)
	{
		if (g_hBlobAliveThread[i] != 0)
		{
			hThreads[numThreads] = g_hBlobAliveThread[i];
			numThreads++;
		}
	}

	LeaveCriticalSection(&g_BlobThreadManagerLock);

	auto ret = WaitForMultipleObjects(numThreads, hThreads, TRUE, 0);

	if (ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + numThreads)
	{
		EnterCriticalSection(&g_BlobThreadManagerLock);

		for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobAliveThread); ++i)
		{
			if (g_hBlobAliveThread[i] == hThreads[ret - WAIT_OBJECT_0])
			{
				g_hBlobAliveThread[i] = 0;
				break;
			}
		}

		LeaveCriticalSection(&g_BlobThreadManagerLock);
	}
}
#endif

void BlobWaitForAliveThreadsToShutdown(void)
{
	HANDLE hThreads[MAXIMUM_WAIT_OBJECTS] = { 0 };
	DWORD numThreads = 0;

	BlobEnterCritSection();

	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobAliveThread); ++i)
	{
		if (g_hBlobAliveThread[i] != 0)
		{
			hThreads[numThreads] = g_hBlobAliveThread[i];
			numThreads++;
		}
	}

	BlobLeaveCritSection();

	if (numThreads != 0)
		WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);

	memset(g_hBlobAliveThread, 0, sizeof(g_hBlobAliveThread));

	for (DWORD i = 0; i < numThreads; ++i)
	{
		if (hThreads[i])
		{
			CloseHandle(hThreads[i]);
		}
	}
}

void BlobWaitForClosedThreadsToShutdown(void)
{
	HANDLE hThreads[MAXIMUM_WAIT_OBJECTS] = { 0 };
	DWORD numThreads = 0;

	BlobEnterCritSection();

	for (DWORD i = 0; i < _ARRAYSIZE(g_hBlobClosedThread); ++i)
	{
		if (g_hBlobClosedThread[i] != 0)
		{
			hThreads[numThreads] = g_hBlobClosedThread[i];
			numThreads++;
		}
	}

	BlobLeaveCritSection();

	if(numThreads != 0)
		WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);

	memset(g_hBlobClosedThread, 0, sizeof(g_hBlobClosedThread));

	for (DWORD i = 0; i < numThreads; ++i)
	{
		if (hThreads[i])
		{
			CloseHandle(hThreads[i]);
		}
	}
}

blob_thread_manager_api_t g_BlobThreadManagerAPI = {
	InitBlobThreadManager,
	ShutdownBlobThreadManager,
	BlobEnterCritSection,
	BlobLeaveCritSection,
	BlobFindAliveThread,
	BlobFindAndRemoveAliveThread,
	BlobAddAliveThread,
	BlobFindClosedThread,
	BlobAddClosedThread,
	BlobFindAndRemoveSignaledAliveThread,
	BlobFindAndRemoveSignaledClosedThread,
	&g_pfnCreateThread,
	&g_pfnCloseHandle,
	&g_pfnTerminateThread,
	BlobCreateThread,
	BlobCloseHandle,
	BlobTerminateThread,
};