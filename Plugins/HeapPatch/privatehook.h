#pragma once

#include "enginedef.h"

#include <FileSystem.h>

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void * address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	void (*Sys_InitMemory)(void);	
}private_funcs_t;

void Engine_InstallHooks();
void Engine_UninstallHooks();