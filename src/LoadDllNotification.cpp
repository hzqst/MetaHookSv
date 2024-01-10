#include <MINT.h>
#include <mutex>
#include <vector>
#include <metahook.h>
#include <Detours.h>
#include "LoadBlob.h"

PVOID MH_GetEngineBase(void);
PVOID MH_GetClientBase(void);

typedef VOID(CALLBACK* PLDR_DLL_NOTIFICATION_FUNCTION)(
	_In_     ULONG                       NotificationReason,
	_In_     PLDR_DLL_NOTIFICATION_DATA NotificationData,
	_In_opt_ PVOID                       Context
	);

typedef NTSTATUS(NTAPI* LDR_REGISTER_DLL_NOTIFICATION)(
	_In_     ULONG                          Flags,
	_In_     PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
	_In_opt_ PVOID                          Context,
	_Out_    PVOID* Cookie
	);

static std::mutex g_LoadDllNotificationCallbackLock;
static std::vector<LoadDllNotificationCallback> g_LoadDllNotificationCallbacks;
static PVOID g_LdrNotificationCookie = NULL;

NTSTATUS (NTAPI*g_pfnLdrLoadDll)(PWSTR a1, PULONG a2, PUNICODE_STRING a3, PVOID* a4) = NULL;

void UnicodeToWString(_In_ PCUNICODE_STRING ustr, _Out_ std::wstring& out)
{
	size_t totallen = ustr->Length / sizeof(WCHAR);
	out.assign(ustr->Buffer, totallen);
}

void MH_ClearDllLoaderNotificationCallback()
{
	std::lock_guard<std::mutex> lock(g_LoadDllNotificationCallbackLock);

	g_LoadDllNotificationCallbacks.clear();
}

void MH_RegisterDllLoaderNotificationCallback(LoadDllNotificationCallback callback)
{
	std::lock_guard<std::mutex> lock(g_LoadDllNotificationCallbackLock);

	g_LoadDllNotificationCallbacks.emplace_back(callback);
}

void MH_UnregisterDllLoaderNotificationCallback(LoadDllNotificationCallback callback)
{
	std::lock_guard<std::mutex> lock(g_LoadDllNotificationCallbackLock);

	for (auto itor = g_LoadDllNotificationCallbacks.begin(); itor != g_LoadDllNotificationCallbacks.end();)
	{
		if ((*itor) == callback)
		{
			itor = g_LoadDllNotificationCallbacks.erase(itor);
			return;
		}
		itor++;
	}
}

void MH_DispatchLoadBlobNotificationCallback(BlobHandle_t hBlob, int flags)
{
	std::lock_guard<std::mutex> lock(g_LoadDllNotificationCallbackLock);

	mh_load_dll_notification_context_t ctx;
	ctx.hModule = NULL;
	ctx.hBlob = hBlob;
	ctx.ImageBase = GetBlobModuleImageBase(hBlob);
	ctx.ImageSize = GetBlobModuleImageSize(hBlob);
	ctx.flags = flags;
	ctx.flags |= LOAD_DLL_NOTIFICATION_IS_BLOB;

	if (ctx.ImageBase == MH_GetEngineBase())
		ctx.flags |= LOAD_DLL_NOTIFICATION_IS_ENGINE;
	else if (ctx.ImageBase == MH_GetClientBase())
		ctx.flags |= LOAD_DLL_NOTIFICATION_IS_CLIENT;

	ctx.FullDllName = NULL;
	ctx.BaseDllName = NULL;

	for (auto callback : g_LoadDllNotificationCallbacks)
	{
		callback(&ctx);
	}
}

void MH_DispatchLoadLdrDllNotificationCallback(PCUNICODE_STRING FullDllName, PCUNICODE_STRING BaseDllName, PVOID ImageBase, ULONG ImageSize, int flags)
{
	std::lock_guard<std::mutex> lock(g_LoadDllNotificationCallbackLock);

	mh_load_dll_notification_context_t ctx;
	ctx.hModule = (HMODULE)ImageBase;
	ctx.hBlob = NULL;
	ctx.ImageBase = ImageBase;
	ctx.ImageSize = ImageSize;
	ctx.flags = flags;

	if (ctx.ImageBase == MH_GetEngineBase())
		ctx.flags |= LOAD_DLL_NOTIFICATION_IS_ENGINE;
	else if (ctx.ImageBase == MH_GetClientBase())
		ctx.flags |= LOAD_DLL_NOTIFICATION_IS_CLIENT;

	ctx.FullDllName = NULL;
	ctx.BaseDllName = NULL;

	std::wstring wFullDllName;
	std::wstring wBaseDllName;

	if (FullDllName)
	{
		UnicodeToWString(FullDllName, wFullDllName);
		UnicodeToWString(BaseDllName, wBaseDllName);

		ctx.FullDllName = wFullDllName.c_str();
		ctx.BaseDllName = wBaseDllName.c_str();
	}

	for (auto callback : g_LoadDllNotificationCallbacks)
	{
		callback(&ctx);
	}
}

PLDR_DATA_TABLE_ENTRY GetLdrEntryInfoByDllBase(PVOID DllBase)
{
	auto Peb = NtCurrentTeb()->ProcessEnvironmentBlock;

	auto pEntry = (PLDR_DATA_TABLE_ENTRY)Peb->Ldr->InLoadOrderModuleList.Flink;

	while (pEntry->DllBase)
	{
		if (DllBase == pEntry->DllBase)
		{
			return pEntry;
		}

		pEntry = (PLDR_DATA_TABLE_ENTRY)pEntry->InLoadOrderLinks.Flink;
	}

	return NULL;
}

NTSTATUS NTAPI NewLdrLoadDll(PWSTR a1, PULONG a2, PUNICODE_STRING a3, PVOID* a4)
{
	NTSTATUS r = g_pfnLdrLoadDll(a1, a2, a3, a4);

	if (r == STATUS_SUCCESS && *a4)
	{
		auto pLdrEntryInfo = GetLdrEntryInfoByDllBase(*a4);
		if (pLdrEntryInfo)
		{
			MH_DispatchLoadLdrDllNotificationCallback(&pLdrEntryInfo->FullDllName, &pLdrEntryInfo->BaseDllName, pLdrEntryInfo->DllBase, pLdrEntryInfo->SizeOfImage, LOAD_DLL_NOTIFICATION_IS_LOAD);
		}
	}

	return r;
}

VOID CALLBACK LdrDllNotificationCallback(
	_In_     ULONG                       NotificationReason,
	_In_     PLDR_DLL_NOTIFICATION_DATA NotificationData,
	_In_opt_ PVOID Context)
{
	if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
	{
		auto& args = NotificationData->Loaded;

		MH_DispatchLoadLdrDllNotificationCallback(args.FullDllName, args.BaseDllName, args.DllBase, args.SizeOfImage, LOAD_DLL_NOTIFICATION_IS_LOAD);
	}
	else if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_UNLOADED)
	{
		auto& args = NotificationData->Unloaded;

		MH_DispatchLoadLdrDllNotificationCallback(args.FullDllName, args.BaseDllName, args.DllBase, args.SizeOfImage, LOAD_DLL_NOTIFICATION_IS_UNLOAD);
	}
}

void InitLoadDllNotification(void)
{
	auto ntdll = GetModuleHandle("ntdll.dll");

	if (ntdll)
	{
		auto pfnLdrRegisterDllNotification = (decltype(LdrRegisterDllNotification)*)GetProcAddress(ntdll, "LdrRegisterDllNotification");
		if (pfnLdrRegisterDllNotification)
		{
			if (NT_SUCCESS(pfnLdrRegisterDllNotification(0, LdrDllNotificationCallback, NULL, &g_LdrNotificationCookie)))
			{
				return;
			}
		}
	}

	if (!ntdll)
		return;
	
	//Legacy support for system that don't have LdrRegisterDllNotification
	g_pfnLdrLoadDll = (decltype(g_pfnLdrLoadDll))GetProcAddress(ntdll, "LdrLoadDll");
	if (!g_pfnLdrLoadDll)
		return;

	DetourTransactionBegin();
	DetourAttach(&(void*&)g_pfnLdrLoadDll, NewLdrLoadDll);
	DetourTransactionCommit();
}

void ShutdownLoadDllNotification(void)
{
	if (g_LdrNotificationCookie)
	{
		auto ntdll = GetModuleHandle("ntdll.dll");
		if (ntdll)
		{
			auto pfnLdrUnregisterDllNotification = (decltype(LdrUnregisterDllNotification)*)GetProcAddress(ntdll, "LdrUnregisterDllNotification");
			if (pfnLdrUnregisterDllNotification)
			{
				pfnLdrUnregisterDllNotification(g_LdrNotificationCookie);
				g_LdrNotificationCookie = NULL;
				return;
			}
		}
	}

	if (g_pfnLdrLoadDll)
	{
		DetourTransactionBegin();
		DetourDetach(&(void*&)g_pfnLdrLoadDll, NewLdrLoadDll);
		DetourTransactionCommit();

		g_pfnLdrLoadDll = NULL;
	}
}