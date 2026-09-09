#pragma once

void Engine_FillAddress(void);
void Engine_WaitForShutdown(HMODULE hModule, BlobHandle_t hBlobModule);
void DllLoadNotification(mh_load_dll_notification_context_t* ctx);
