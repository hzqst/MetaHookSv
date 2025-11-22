#pragma once

#include <com_model.h>
#include <r_studioint.h>

typedef struct
{
	int unused;
}private_funcs_t;

extern model_t **r_model;

void EngineStudio_FillAddress(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_InstallHooks(void);
void EngineStudio_FillAddress(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void EngineStudio_InstalHooks();

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);