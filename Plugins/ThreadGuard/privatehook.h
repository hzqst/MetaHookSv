#pragma once

typedef struct
{
	void(*empty)();
}private_funcs_t;

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_WaitForShutdown(HMODULE hModule, BlobHandle_t hBlobModule);
void Engine_InstallHook();
void Engine_UninstallHook();
void DllLoadNotification(mh_load_dll_notification_context_t* ctx);

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);