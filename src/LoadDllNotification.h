#pragma once

// Include Veil.h via a relative path rather than adding thirdparty\Musa.Veil_fork
// to the general include path: the fork ships a text file named "VERSION" that
// would otherwise shadow the C++20 <version> header probed by GSL (Chocobo1Hash).
#include "../thirdparty/Musa.Veil_fork/Veil.h"
#include <metahook.h>

typedef void* BlobHandle_t;

void InitLoadDllNotification(void);
void ShutdownLoadDllNotification(void);
void MH_DispatchLoadBlobNotificationCallback(BlobHandle_t hBlob, int flags);
void MH_DispatchLoadLdrDllNotificationCallback(PCUNICODE_STRING FullDllName, PCUNICODE_STRING BaseDllName, PVOID ImageBase, ULONG ImageSize, int flags);
void MH_RegisterDllLoaderNotificationCallback(LoadDllNotificationCallback callback);
void MH_UnregisterDllLoaderNotificationCallback(LoadDllNotificationCallback callback);
void MH_ClearDllLoaderNotificationCallback();
bool MH_IsInLdrCriticalRegion();