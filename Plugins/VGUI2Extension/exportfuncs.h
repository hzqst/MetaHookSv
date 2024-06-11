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

void *NewClientFactory(void);

const char *GetBaseDirectory();

//int FileSystem_SetGameDirectory(const char *pDefaultDir, const char *pGameDir);

IBaseInterface *NewCreateInterface(const char *pName, int *pReturnCode);

LRESULT WINAPI VID_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void SDL_GetWindowSize(void* window, int* w, int* h);

void InitWin32Stuffs(void);