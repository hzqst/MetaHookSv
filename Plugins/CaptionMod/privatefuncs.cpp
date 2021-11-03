#include <metahook.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "privatefuncs.h"

#define S_FINDNAME_SIG_SVENGINE "\x53\x55\x8B\x6C\x24\x0C\x56\x33\xF6\x57\x85\xED\x75\x2A\x68"
#define S_STARTDYNAMICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x54\x8B\x44\x24\x5C\x55"
#define S_STARTSTATICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x48\x57\x8B\x7C\x24\x5C"
#define S_LOADSOUND_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x8B\x8C\x24\x2A\x2A\x00\x00\x56\x8B\xB4\x24\x2A\x2A\x00\x00\x8A\x06\x3C\x2A"

#define S_INIT_SIG_NEW "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_FINDNAME_SIG_NEW "\x55\x8B\xEC\x53\x56\x8B\x75\x08\x33\xDB\x85\xF6"
#define S_STARTDYNAMICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x85\xC0\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x53\x56\x57\x8B\x7D\x10\x85\xFF\xC7\x45\xFC\x00\x00\x00\x00"
#define S_LOADSOUND_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define S_LOADSOUND_8308_SIG "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08"

#define S_INIT_SIG "\x83\xEC\x08\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_FINDNAME_SIG "\x53\x55\x8B\x6C\x24\x0C\x33\xDB\x56\x57\x85\xED"
#define S_STARTDYNAMICSOUND_SIG "\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x85\xC0\x57\xC7\x44\x24\x10\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG "\x83\xEC\x44\x53\x55\x8B\x6C\x24\x58\x56\x85\xED\x57"
#define S_LOADSOUND_SIG "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"

double *cl_time;
double *cl_oldtime;

char m_szCurrentLanguage[64] = { 0 };

private_funcs_t gPrivateFuncs = {0};

void Engine_FillAddress(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.S_Init = (void(*)(void))Search_Pattern(S_INIT_SIG_NEW);
		Sig_FuncNotFound(S_Init); 
		
		gPrivateFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern(S_FINDNAME_SIG_SVENGINE);
		Sig_FuncNotFound(S_FindName);

		gPrivateFuncs.S_StartDynamicSound = (void(*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern(S_STARTDYNAMICSOUND_SIG_SVENGINE);
		Sig_FuncNotFound(S_StartDynamicSound);

		gPrivateFuncs.S_StartStaticSound = (void(*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern(S_STARTSTATICSOUND_SIG_SVENGINE);
		Sig_FuncNotFound(S_StartStaticSound);

		gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern(S_LOADSOUND_SIG_SVENGINE);
		Sig_FuncNotFound(S_LoadSound);
	}
	else if(g_dwEngineBuildnum >= 5953)
	{
		gPrivateFuncs.S_Init = (void (*)(void))Search_Pattern(S_INIT_SIG_NEW);
		Sig_FuncNotFound(S_Init);

		gPrivateFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern_From(gPrivateFuncs.S_Init, S_FINDNAME_SIG_NEW);
		Sig_FuncNotFound(S_FindName);

		gPrivateFuncs.S_StartDynamicSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(gPrivateFuncs.S_FindName, S_STARTDYNAMICSOUND_SIG_NEW);
		Sig_FuncNotFound(S_StartDynamicSound);

		gPrivateFuncs.S_StartStaticSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(gPrivateFuncs.S_StartDynamicSound, S_STARTSTATICSOUND_SIG_NEW);
		Sig_FuncNotFound(S_StartStaticSound);

		gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern(S_LOADSOUND_SIG_NEW);
		if(!gPrivateFuncs.S_LoadSound)
			gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern( S_LOADSOUND_8308_SIG);
		Sig_FuncNotFound(S_LoadSound);
	}
	else
	{
		gPrivateFuncs.S_Init = (void (*)(void))Search_Pattern(S_INIT_SIG);
		Sig_FuncNotFound(S_Init);

		gPrivateFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern_From(gPrivateFuncs.S_Init, S_FINDNAME_SIG);
		Sig_FuncNotFound(S_FindName);

		gPrivateFuncs.S_StartDynamicSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_FindName, S_STARTDYNAMICSOUND_SIG);
		Sig_FuncNotFound(S_StartDynamicSound);

		gPrivateFuncs.S_StartStaticSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_StartDynamicSound, S_STARTSTATICSOUND_SIG);
		Sig_FuncNotFound(S_StartStaticSound);

		gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern_From(S_StartStaticSound, S_LOADSOUND_SIG);
		Sig_FuncNotFound(S_LoadSound);
	}
}

void Engine_InstallHook(void)
{
	Install_InlineHook(S_FindName);
	Install_InlineHook(S_StartDynamicSound);
	Install_InlineHook(S_StartStaticSound);
}
