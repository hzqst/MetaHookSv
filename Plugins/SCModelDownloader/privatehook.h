#pragma once

#include <com_model.h>

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
	char		name[260];
	char		modelname[260];
	model_t*	model;
} player_model_t;

typedef struct
{
	void (*R_StudioChangePlayerModel)(void);
}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

extern player_model_t(*DM_PlayerState)[MAX_CLIENTS]; 

void R_StudioChangePlayerModel(void);

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_InstallHook(void);
void Engine_UninstallHook(void);
void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Client_InstallHooks(void);

void DllLoadNotification(mh_load_dll_notification_context_t* ctx);

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);
PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo);