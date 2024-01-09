#pragma once

#include <interface.h>
#include <Windows.h>

const int hookflag_CreateThread = 1;
const int hookflag_WaitForSingleObject = 2;
const int hookflag_Sleep = 4;

class IThreadManager : public IBaseInterface
{
public:

public:
	virtual void StartTermination(void) = 0;
	virtual void WaitForAliveThreadsToShutdown(void) = 0;
	//virtual void WaitForClosedThreadsToShutdown(void) = 0;

	virtual void OnCreateThread(HANDLE hThreadHandle) = 0;
	virtual bool OnWaitForSingleObject(HANDLE hObject, DWORD dwMilliseconds) = 0;
	virtual bool OnSleep(DWORD dwMilliseconds) = 0;

	virtual void InstallHook(int flags) = 0;
	virtual void UnistallHook(void) = 0;
	virtual PVOID GetImageBase(void) const = 0;
	virtual ULONG GetImageSize(void) const = 0;
	virtual PVOID GetTextBase(void) const = 0;
	virtual ULONG GetTextSize(void) const = 0;
	virtual HMODULE GetModule(void) const = 0;
	virtual BlobHandle_t GetBlobModule(void) const = 0;
};

void AddThreadManager(IThreadManager* p);
void DeleteThreadManager(IThreadManager* p);
IThreadManager* FindThreadManagerByVirtualAddress(PVOID VirtualAddress);
IThreadManager* CreateThreadManagerForModule(HMODULE hModule);
IThreadManager* CreateThreadManagerForBlob(BlobHandle_t hBlob);