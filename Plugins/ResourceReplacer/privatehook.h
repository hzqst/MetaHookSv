#pragma once

#include "enginedef.h"

typedef struct walk_context_s
{
	walk_context_s(PVOID a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	PVOID address;
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

void Engine_InstallHooks();
void Engine_UninstallHooks();