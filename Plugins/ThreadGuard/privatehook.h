#pragma once

typedef struct
{
	void(*empty)();
}private_funcs_t;

void Engine_WaitForShutdown(HMODULE hModule, BlobHandle_t hBlobModule);
void Engine_InstallHook();
void Engine_UninstallHook();
void DllLoadNotification(mh_load_dll_notification_context_t* ctx);