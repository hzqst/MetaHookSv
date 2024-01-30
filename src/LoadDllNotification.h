#pragma once

#include <MINT.h>

typedef void* BlobHandle_t;

void InitLoadDllNotification(void);
void ShutdownLoadDllNotification(void);
void MH_DispatchLoadBlobNotificationCallback(BlobHandle_t hBlob, int flags);
void MH_DispatchLoadLdrDllNotificationCallback(PCUNICODE_STRING FullDllName, PCUNICODE_STRING BaseDllName, PVOID ImageBase, ULONG ImageSize, int flags);
void MH_RegisterDllLoaderNotificationCallback(LoadDllNotificationCallback callback);
void MH_UnregisterDllLoaderNotificationCallback(LoadDllNotificationCallback callback);
void MH_ClearDllLoaderNotificationCallback();
bool MH_IsInLdrCriticalRegion();