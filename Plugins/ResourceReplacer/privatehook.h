#pragma once

#include <com_model.h>

typedef struct
{
	//FileHandle_t(__fastcall* FileSystem_Open)(const char* pFileName, const char* pOptions, const char* pathID);
	FileHandle_t (*FS_Open)(const char* pFileName, const char* pOptions);
	model_t* (*Mod_LoadModel)(model_t* mod, qboolean crash, qboolean trackCRC);
	qboolean(*CL_PrecacheResources)();
}private_funcs_t;

void FileSystem_InstallHooks();
void FileSystem_UninstallHooks();
void Engine_InstallHooks();
void Engine_UninstallHooks();