#pragma once
#include <gl/gl.h>
#include <const.h>
#include <triangleapi.h>
#include <cl_entity.h>
#include <event_api.h>
#include <ref_params.h>
#include <com_model.h>
#include <cvardef.h>
#include <r_efx.h>
#include <r_studioint.h>
#include <pm_movevars.h>
#include <studio.h>
#include <entity_types.h>
#include <usercmd.h>
#include "enginedef.h"

#define CAPTION_MOD_VERSION "Caption Mod 3.5"

extern cl_enginefunc_t gEngfuncs;

void Steam_Init(void);
void MSG_Init(void);
int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion);
void HUD_Init(void);
int HUD_VidInit(void);
void HUD_Frame(double time);
client_textmessage_t *pfnTextMessageGet(const char *pName);
void Engine_FillAddress(void);
void Engine_InstallHook(void);
void BaseUI_InstallHook(void);
void GameUI_InstallHook(void);
void ClientVGUI_InstallHook(void);
void ClientVGUI_Shutdown(void);
void VGUI1_InstallHook(void);
void Surface_InstallHook(void);
void Scheme_InstallHook(void);
void KeyValuesSystem_InstallHook(void);
void Sys_GetRegKeyValueUnderRoot(const char *pszSubKey, const char *pszElement, char *pszReturnString, int nReturnLength, const char *pszDefaultValue);
FARPROC WINAPI NewGetProcAddress(HMODULE hModule, LPCSTR lpProcName);
void *NewClientFactory(void);
void S_StartDynamicSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);
void S_StartStaticSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);
sfxcache_t *S_LoadSound(sfx_t *s, channel_t *ch);
sfx_t *S_FindName(char *name, int *pfInCache);
int __fastcall SvClient_FindSoundEx(int pthis, int, const char *sound);
void Sys_ErrorEx(const char *fmt, ...);

extern cvar_t *cap_enabled;
extern cvar_t *cap_debug;
extern cvar_t *cap_netmessage;