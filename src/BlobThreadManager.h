#pragma once

#include <Windows.h>

void InitBlobThreadManager(void);
void ShutdownBlobThreadManager(void);
void BlobEnterCritSection(void);
void BlobLeaveCritSection(void);
bool BlobFindAliveThread(HANDLE hThread);
bool BlobFindAndRemoveAliveThread(HANDLE hThread);
bool BlobAddAliveThread(HANDLE hThread);
bool BlobFindClosedThread(HANDLE hThread);
bool BlobAddClosedThread(HANDLE hThread);
bool BlobFindAndRemoveSignaledAliveThread(DWORD* pIndex);
bool BlobFindAndRemoveSignaledClosedThread(DWORD* pIndex);
void BlobWaitForAliveThreadsToShutdown(void);
void BlobWaitForClosedThreadsToShutdown(void);

HANDLE WINAPI BlobCreateThread(
	LPSECURITY_ATTRIBUTES   lpThreadAttributes,
	SIZE_T                  dwStackSize,
	LPTHREAD_START_ROUTINE  lpStartAddress,
	LPVOID					lpParameter,
	DWORD                   dwCreationFlags,
	LPDWORD                 lpThreadId
);

BOOL WINAPI BlobCloseHandle(HANDLE hObject);

BOOL WINAPI BlobTerminateThread(
	HANDLE hThread,
	DWORD  dwExitCode
);