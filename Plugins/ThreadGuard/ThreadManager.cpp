#include <MINT.h>
#include "metahook.h"
#include "ThreadManager.h"
#include <vector>
#include <mutex>
#include <intrin.h>

static std::mutex g_ThreadManagerLock;
static std::vector<IThreadManager*> g_ThreadManagers;

extern HANDLE g_MainThreadId;

IThreadManager * FindThreadManagerByVirtualAddress(PVOID VirtualAddress)
{
	std::lock_guard<std::mutex> lock(g_ThreadManagerLock);

	for (auto p : g_ThreadManagers)
	{
		if (VirtualAddress >= p->GetTextBase() && VirtualAddress < (PUCHAR)p->GetTextBase() + p->GetTextSize())
		{
			return p;
		}
	}

	return NULL;
}

HANDLE WINAPI NewCreateThread(
	LPSECURITY_ATTRIBUTES   lpThreadAttributes,
	SIZE_T                  dwStackSize,
	LPTHREAD_START_ROUTINE  lpStartAddress,
	LPVOID					lpParameter,
	DWORD                   dwCreationFlags,
	LPDWORD                 lpThreadId
)
{
	DWORD originalCreationFlags = dwCreationFlags;

	auto hThreadHandle = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);

	if (hThreadHandle)
	{
		auto ThreadManager = FindThreadManagerByVirtualAddress(_ReturnAddress());

		if (ThreadManager)
		{
			HANDLE hNewThreadHandle = 0;
			if (DuplicateHandle((HANDLE)(-1), hThreadHandle, (HANDLE)(-1), &hNewThreadHandle, THREAD_ALL_ACCESS, FALSE, DUPLICATE_SAME_ACCESS))
			{
				ThreadManager->OnCreateThread(hNewThreadHandle);
			}
		}
	}

	return hThreadHandle;
}

DWORD WINAPI NewWaitForSingleObject(HANDLE hHandle,  DWORD dwMilliseconds)
{
	if (dwMilliseconds == 0)
	{
		auto ThreadManager = FindThreadManagerByVirtualAddress(_ReturnAddress());

		if (ThreadManager)
		{
			if (ThreadManager->OnWaitForSingleObject(hHandle, dwMilliseconds))
			{
				return WAIT_OBJECT_0;
			}
		}
	}

	return WaitForSingleObject(hHandle, dwMilliseconds);
}

void WINAPI NewSleep(DWORD dwMilliseconds)
{
	if (dwMilliseconds == 1)
	{
		auto ThreadManager = FindThreadManagerByVirtualAddress(_ReturnAddress());

		if (ThreadManager)
		{
			if (ThreadManager->OnSleep(dwMilliseconds))
			{
				ExitThread(0);
				return;
			}
		}
	}

	return Sleep(dwMilliseconds);
}

class CThreadManager : public IThreadManager
{

private:
	std::mutex m_ThreadListLock;

	HANDLE m_hAliveThread[MAXIMUM_WAIT_OBJECTS];
	HANDLE m_hClosedThread[MAXIMUM_WAIT_OBJECTS];

	HMODULE m_hModule;
	BlobHandle_t m_hBlobModule;

	PVOID m_ImageBase;
	ULONG m_ImageSize;

	PVOID m_TextBase;
	ULONG m_TextSize;

	//for __beginthread
	hook_t* m_pHook_CreateThread;

	//for CSocketThread
	hook_t* m_pHook_WaitForSingleObject;

	//for hw.dll
	hook_t* m_pHook_Sleep;

	bool m_bStartTermination;
public:
	CThreadManager(HMODULE hModule, BlobHandle_t hBlob)
	{
		m_hModule = hModule;
		m_hBlobModule = hBlob;

		m_ImageBase = 0;
		m_ImageSize = 0;
		m_TextBase = 0;
		m_TextSize = 0;

		if (m_hModule)
		{
			m_ImageBase = g_pMetaHookAPI->GetModuleBase(m_hModule);
			m_TextBase = g_pMetaHookAPI->GetSectionByName(m_ImageBase, ".text\0\0\0", &m_TextSize);
		}

		if (m_hBlobModule)
		{
			m_ImageBase = g_pMetaHookAPI->GetBlobModuleImageBase(m_hBlobModule);
			m_TextBase = g_pMetaHookAPI->GetSectionByName(m_ImageBase, ".text\0\0\0", &m_TextSize);
		}

		memset(m_hAliveThread, 0, sizeof(m_hAliveThread));
		memset(m_hClosedThread, 0, sizeof(m_hClosedThread));

		m_pHook_CreateThread = NULL;
		m_pHook_WaitForSingleObject = NULL;
		m_pHook_Sleep = NULL;

		m_bStartTermination = false;
	}

	HMODULE GetModule(void) const
	{
		return m_hModule;
	}

	BlobHandle_t GetBlobModule(void) const
	{
		return m_hBlobModule;
	}

	PVOID GetImageBase(void) const
	{
		return m_ImageBase;
	}

	ULONG GetImageSize(void) const
	{
		return m_ImageSize;
	}

	PVOID GetTextBase(void) const
	{
		return m_TextBase;
	}

	ULONG GetTextSize(void) const
	{
		return m_TextSize;
	}

	bool OnWaitForSingleObject(HANDLE hObject, DWORD dwMilliseconds) override
	{
		if (m_bStartTermination)
		{
			return true;
		}
		return false;
	}

	bool OnSleep(DWORD dwMilliseconds) override
	{
		if (m_bStartTermination)
		{
			if(NtCurrentTeb()->ClientId.UniqueThread != g_MainThreadId)
				return true;
		}
		return false;
	}

	void InstallHook(int flags) override
	{
		if (m_hModule)
		{
			if(flags & hookflag_CreateThread)
				m_pHook_CreateThread = g_pMetaHookAPI->IATHook(m_hModule, "kernel32.dll", "CreateThread", NewCreateThread, NULL);

			if (flags & hookflag_WaitForSingleObject)
				m_pHook_WaitForSingleObject = g_pMetaHookAPI->IATHook(m_hModule, "kernel32.dll", "WaitForSingleObject", NewWaitForSingleObject, NULL);

			if (flags & hookflag_Sleep)
				m_pHook_Sleep = g_pMetaHookAPI->IATHook(m_hModule, "kernel32.dll", "Sleep", NewSleep, NULL);
		}
		else if (m_hBlobModule)
		{
			if (flags & hookflag_CreateThread)
				m_pHook_CreateThread = g_pMetaHookAPI->BlobIATHook(m_hBlobModule, "kernel32.dll", "CreateThread", NewCreateThread, NULL);

			if (flags & hookflag_WaitForSingleObject)
				m_pHook_WaitForSingleObject = g_pMetaHookAPI->BlobIATHook(m_hBlobModule, "kernel32.dll", "WaitForSingleObject", NewWaitForSingleObject, NULL);

			if (flags & hookflag_Sleep)
				m_pHook_Sleep = g_pMetaHookAPI->BlobIATHook(m_hBlobModule, "kernel32.dll", "Sleep", NewSleep, NULL);
		}
	}

	void UnistallHook(void) override
	{
#if 0
		if (m_pHook_DllMain)
		{
			g_pMetaHookAPI->UnHook(m_pHook_DllMain);
			m_pHook_DllMain = NULL;
		}
#endif
		if (m_pHook_CreateThread)
		{
			g_pMetaHookAPI->UnHook(m_pHook_CreateThread);
			m_pHook_CreateThread = NULL;
		}
#if 0
		if (m_pHook_CloseHandle)
		{
			g_pMetaHookAPI->UnHook(m_pHook_CloseHandle);
			m_pHook_CloseHandle = NULL;
		}

		if (m_pHook_TerminateThread)
		{
			g_pMetaHookAPI->UnHook(m_pHook_TerminateThread);
			m_pHook_TerminateThread = NULL;
		}
#endif
		if (m_pHook_WaitForSingleObject)
		{
			g_pMetaHookAPI->UnHook(m_pHook_WaitForSingleObject);
			m_pHook_WaitForSingleObject = NULL;
		}

		if (m_pHook_Sleep)
		{
			g_pMetaHookAPI->UnHook(m_pHook_Sleep);
			m_pHook_Sleep = NULL;
		}
	}
private:

	bool FindAliveThread(HANDLE hThread)
	{
		for (DWORD i = 0; i < _ARRAYSIZE(m_hAliveThread); ++i)
		{
			if (m_hAliveThread[i] == hThread)
			{
				return true;
			}
		}
		return false;
	}

	bool FindAndRemoveAliveThread(HANDLE hThread)
	{
		for (DWORD i = 0; i < _ARRAYSIZE(m_hAliveThread); ++i)
		{
			if (m_hAliveThread[i] == hThread)
			{
				m_hAliveThread[i] = 0;
				return true;
			}
		}
		return false;
	}

	bool AddAliveThread(HANDLE hThread)
	{
		for (DWORD i = 0; i < _ARRAYSIZE(m_hAliveThread); ++i)
		{
			if (m_hAliveThread[i] == 0)
			{
				m_hAliveThread[i] = hThread;
				return true;
			}
		}
		return false;
	}

	bool FindClosedThread(HANDLE hThread)
	{
		for (DWORD i = 0; i < _ARRAYSIZE(m_hClosedThread); ++i)
		{
			if (m_hClosedThread[i] == hThread)
			{
				return true;
			}
		}
		return false;
	}

	bool AddClosedThread(HANDLE hThread)
	{
		for (DWORD i = 0; i < _ARRAYSIZE(m_hClosedThread); ++i)
		{
			if (m_hClosedThread[i] == 0)
			{
				m_hClosedThread[i] = hThread;
				return true;
			}
		}
		return false;
	}

	bool FindAndRemoveSignaledAliveThread(DWORD* pIndex)
	{
		for (DWORD i = 0; i < _ARRAYSIZE(m_hAliveThread); ++i)
		{
			if (m_hAliveThread[i])
			{
				if (WAIT_OBJECT_0 == WaitForSingleObject(m_hAliveThread[i], 0))
				{
					m_hAliveThread[i] = 0;
					if (pIndex)
						*pIndex = i;
					return true;
				}
			}
		}

		return false;
	}

	bool FindAndRemoveSignaledClosedThread(DWORD* pIndex)
	{
		for (DWORD i = 0; i < _ARRAYSIZE(m_hClosedThread); ++i)
		{
			if (m_hClosedThread[i])
			{
				if (WAIT_OBJECT_0 == WaitForSingleObject(m_hClosedThread[i], 0))
				{
					m_hClosedThread[i] = 0;
					if (pIndex)
						*pIndex = i;
					return true;
				}
			}
		}

		return false;
	}

	void StartTermination(void) override
	{
		m_bStartTermination = true;
	}

	void WaitForAliveThreadsToShutdown(void) override
	{
		HANDLE hThreads[MAXIMUM_WAIT_OBJECTS] = { 0 };
		DWORD numThreads = 0;

		if (1)
		{
			std::lock_guard<std::mutex> lock(m_ThreadListLock);
			for (DWORD i = 0; i < _ARRAYSIZE(m_hAliveThread); ++i)
			{
				if (m_hAliveThread[i] != 0)
				{
					hThreads[numThreads] = m_hAliveThread[i];
					numThreads++;
				}
			}
		}

		if (numThreads != 0)
			WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);

		memset(m_hAliveThread, 0, sizeof(m_hAliveThread));

#if 0
		if (1)
		{
			std::lock_guard<std::mutex> lock(m_ThreadListLock);
			
			for (DWORD i = 0; i < numThreads; ++i)
			{
				if (hThreads[i] && !FindClosedThread(hThreads[i]))
				{
					AddClosedThread(hThreads[i]);
				}
			}
		}
#endif

		for (DWORD i = 0; i < numThreads; ++i)
		{
			if (hThreads[i])
			{
				CloseHandle(hThreads[i]);
			}
		}
	}

#if 0
	void WaitForClosedThreadsToShutdown(void) override
	{
		HANDLE hThreads[MAXIMUM_WAIT_OBJECTS] = { 0 };
		DWORD numThreads = 0;

		if (1)
		{
			std::lock_guard<std::mutex> lock(m_ThreadListLock);
			for (DWORD i = 0; i < _ARRAYSIZE(m_hClosedThread); ++i)
			{
				if (m_hClosedThread[i] != 0)
				{
					hThreads[numThreads] = m_hClosedThread[i];
					numThreads++;
				}
			}
		}

		if (numThreads != 0)
			WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);

		memset(m_hClosedThread, 0, sizeof(m_hClosedThread));

		for (DWORD i = 0; i < numThreads; ++i)
		{
			if (hThreads[i])
			{
				CloseHandle(hThreads[i]);
			}
		}
	}

#endif

	void OnCreateThread(HANDLE hThreadHandle) override
	{
		std::lock_guard<std::mutex> lock(m_ThreadListLock);

		bool bAdded = AddAliveThread(hThreadHandle);

		if (!bAdded)
		{
			DWORD NewIndex = 0;
			if (FindAndRemoveSignaledAliveThread(&NewIndex))
			{
				m_hAliveThread[NewIndex] = hThreadHandle;
				bAdded = true;
			}
		}

		if (!bAdded)
		{
			g_pMetaHookAPI->SysError("Failed to insert thread to thread manager!");
		}
	}
};

void AddThreadManager(IThreadManager* p)
{
	std::lock_guard<std::mutex> lock(g_ThreadManagerLock);

	g_ThreadManagers.emplace_back(p);
}

void DeleteThreadManager(IThreadManager* p)
{
	std::lock_guard<std::mutex> lock(g_ThreadManagerLock);

	for (auto itor = g_ThreadManagers.begin(); itor != g_ThreadManagers.end();)
	{
		if ((*itor) == p)
		{
			itor = g_ThreadManagers.erase(itor);
			return;
		}
		itor++;
	}
}

IThreadManager* CreateThreadManagerForModule(HMODULE hModule)
{
	auto p = new CThreadManager(hModule, NULL);
	if (p)
	{
		AddThreadManager(p);
		return p;
	}

	return NULL;
}

IThreadManager* CreateThreadManagerForBlob(BlobHandle_t hBlob)
{
	auto p = new CThreadManager(NULL, hBlob);
	if (p)
	{
		AddThreadManager(p);
		return p;
	}

	return NULL;
}