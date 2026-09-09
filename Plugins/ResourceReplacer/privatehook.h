#pragma once

#include <FileSystem.h>

typedef struct
{
	FileHandle_t (*FS_Open)(const char* pFileName, const char* pOptions);
	qboolean(*CL_PrecacheResources)();
}private_funcs_t;

void Engine_FillAddress(void);
void Engine_InstallHooks();
void Engine_UninstallHooks();
