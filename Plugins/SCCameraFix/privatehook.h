#pragma once

#include <ref_params.h>

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
	size_t len;
	int depth;
}walk_context_t;

struct pitchdrift_t
{
	float pitchvel;
	bool nodrift;
	float driftmove;
	double laststop;
};

typedef struct
{
	void (*V_CalcNormalRefdef)(ref_params_t*);
}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

void Engine_InstallHooks();
void Engine_UninstallHooks();

extern vec3_t* v_origin;
extern vec3_t* g_vVecViewangles;

extern int* g_iUser1;
extern int* g_iUser2;

extern float* g_iFogColor_SCClient;
extern float* g_iStartDist_SCClient;
extern float* g_iEndDist_SCClient;

extern int* g_iWaterLevel;
extern int* g_iIsSpectator;
extern bool* g_bRenderingPortals_SCClient;
extern struct event_api_s** g_pClientDLLEventAPI; 

//extern pitchdrift_t* g_pitchdrift; //not used

void V_CalcNormalRefdef(ref_params_t* pparams);