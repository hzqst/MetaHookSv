#pragma once
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
#include <string>

extern cl_enginefunc_t gEngfuncs;

char * NewV_strncpy(char *a1, const char *a2, size_t a3);

void HUD_Init(void);
int HUD_VidInit(void);
void HUD_Frame(double time);
int HUD_Redraw(float time, int intermission);
void HUD_Shutdown(void);
void IN_MouseEvent(int mstate);
void IN_Accumulate(void);
void CL_CreateMove(float frametime, struct usercmd_s *cmd, int active);

client_textmessage_t *pfnTextMessageGet(const char *pName);
void TextMessageParse(byte* pMemFile, int fileSize);

void COM_ExplainDisconnection(qboolean bPrint, const char* fmt, ...);

void *NewClientFactory(void);

const char *GetBaseDirectory();

//int FileSystem_SetGameDirectory(const char *pDefaultDir, const char *pGameDir);

IBaseInterface *NewCreateInterface(const char *pName, int *pReturnCode);

void S_StartDynamicSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);
void S_StartStaticSound(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);
sfx_t *S_FindName(char *name, int *pfInCache);

void MessageMode_f(void);
void MessageMode2_f(void);
void Cap_Version_f(void);

LRESULT WINAPI VID_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//int __fastcall ScClient_FindSoundEx(void* pthis, int, const char *sound);

void __fastcall ScClient_SoundEngine_PlayFMODSound(void* pSoundEngine, int, int flags, int entindex, float* origin, int channel, const char* name, float fvol, float attenuation, int extraflags, int pitch, int sentenceIndex, float soundLength);
int __stdcall FMOD_System_playSound(void* FMOD_System, int channelid, void* FMOD_Sound, bool paused, void** FMOD_Channel);
void __fastcall WeaponsResource_SelectSlot(void *pthis, int, int iSlot, int fAdvance, int iDirection);

void VGuiWrap2_Paint(void);
void SDL_GetWindowSize(void* window, int* w, int* h);

void InitWin32Stuffs(void);

void RemoveFileExtension(std::string& filePath);

extern cvar_t* cap_debug;
extern cvar_t* cap_enabled;
extern cvar_t* cap_max_distance;
extern cvar_t* cap_min_avol;
extern cvar_t *cap_netmessage;
extern cvar_t *cap_hudmessage;