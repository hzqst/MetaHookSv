#pragma once

#include "enginedef.h"
#include <FileSystem.h>

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	FileHandle_t (*FS_Open)(const char* pFileName, const char* pOptions);
	qboolean(*CL_PrecacheResources)();
	model_t* (*Mod_LoadModel)(model_t* mod, qboolean crash, qboolean trackCRC);
	sfxcache_t* (*S_LoadSound)(sfx_t* s, channel_t* ch);
}private_funcs_t;

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_InstallHooks();
void Engine_UninstallHooks();
PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);
PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo);