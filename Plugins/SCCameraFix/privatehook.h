#pragma once

#include <ref_params.h>

typedef struct
{
	void (*V_CalcNormalRefdef)(ref_params_t*);
}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

extern vec3_t* v_origin;
extern vec3_t* g_vVecViewangles;

extern int* g_iUser1;
extern int* g_iUser2;

extern struct event_api_s** g_pClientDLLEventAPI;

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Client_InstallHooks(void);
void Client_UninstallHooks(void);

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);

void V_CalcNormalRefdef(ref_params_t* pparams);