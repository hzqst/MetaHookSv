#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "privatefuncs.h"

#define S_INIT_SIG_BLOB "\x83\xEC\x08\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_INIT_SIG_NEW "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_INIT_SIG_HL25 S_INIT_SIG_NEW
#define S_INIT_SIG_SVENGINE S_INIT_SIG_NEW

#define S_FINDNAME_SIG_BLOB "\x53\x55\x8B\x6C\x24\x0C\x33\xDB\x56\x57\x85\xED"
#define S_FINDNAME_SIG_NEW "\x55\x8B\xEC\x53\x56\x8B\x75\x08\x33\xDB\x85\xF6"
#define S_FINDNAME_SIG_HL25 "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x33\xF6\x57\x85"
#define S_FINDNAME_SIG_SVENGINE "\x53\x55\x8B\x6C\x24\x0C\x56\x33\xF6\x57\x85\xED\x75\x2A\x68"

#define S_STARTDYNAMICSOUND_SIG_BLOB "\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x85\xC0\x57\xC7\x44\x24\x10\x00\x00\x00\x00"
#define S_STARTDYNAMICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x85\xC0\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTDYNAMICSOUND_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x5C\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x83\x3D\x2A\x2A\x2A\x2A\x2A\x8B\x45\x08"
#define S_STARTDYNAMICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x54\x8B\x44\x24\x5C\x55"

#define S_STARTSTATICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x48\x57\x8B\x7C\x24\x5C"
#define S_STARTSTATICSOUND_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x50\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x57"
#define S_STARTSTATICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x53\x56\x57\x8B\x7D\x10\x85\xFF\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG_BLOB "\x83\xEC\x44\x53\x55\x8B\x6C\x24\x58\x56\x85\xED\x57"

#define S_LOADSOUND_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x8B\x8C\x24\x2A\x2A\x00\x00\x56\x8B\xB4\x24\x2A\x2A\x00\x00\x8A\x06\x3C\x2A"
#define S_LOADSOUND_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x34\x05\x00\x00\xA1"
#define S_LOADSOUND_SIG_8308 "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08"
#define S_LOADSOUND_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define S_LOADSOUND_SIG_BLOB "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"

#define SEQUENCE_GETSENTENCEBYINDEX_SIG_SVENGINE "\x8B\x0D\x2A\x2A\x2A\x2A\x2A\x33\x2A\x85\xC9\x2A\x2A\x8B\x2A\x24\x08\x8B\x41\x04\x2A\x2A\x3B\x2A\x2A\x2A\x8B\x49\x0C"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG_HL25 "\x55\x8B\xEC\x8B\x0D\x2A\x2A\x2A\x2A\x56\x33"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x33\xC9\x85\xC0\x2A\x2A\x2A\x8B\x75\x08\x8B\x50\x04"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x33\xC9\x85\xC0\x56\x2A\x2A\x8B\x74\x24\x08\x8B\x50\x04"

#define SCR_BEGIN_LOADING_PLAQUE "\x6A\x01\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\xC4\x04\x83\xF8\x03"

double *cl_time = NULL;
double *cl_oldtime = NULL;
double *realtime = NULL;

int* cl_viewentity = NULL;

char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
int *cszrawsentences = NULL;

vec3_t *listener_origin = NULL;

char* (*hostparam_basedir) = NULL;

qboolean *scr_drawloading = NULL;

CreateInterfaceFn *g_pClientFactory = NULL;

private_funcs_t gPrivateFuncs = { 0 };

static hook_t *g_phook_S_FindName = NULL;
static hook_t *g_phook_S_StartDynamicSound = NULL;
static hook_t *g_phook_S_StartStaticSound = NULL;
static hook_t *g_phook_pfnTextMessageGet = NULL;
static hook_t* g_phook_pfnServerCmdUnreliable = NULL;
static hook_t *g_phook_TextMessageParse = NULL;
static hook_t* g_phook_COM_ExplainDisconnection = NULL;
static hook_t *g_phook_WeaponsResource_SelectSlot = NULL;
static hook_t* g_phook_ScClient_SoundEngine_LoadSoundList = NULL;
static hook_t *g_phook_ScClient_SoundEngine_PlayFMODSound = NULL;
static hook_t *g_phook_FMOD_System_playSound = NULL;

static HMODULE g_hFMODEx = NULL;

void FMOD_InstallHooks(HMODULE fmodex)
{
	gPrivateFuncs.FMOD_Sound_getLength = (decltype(gPrivateFuncs.FMOD_Sound_getLength))GetProcAddress(fmodex, "?getLength@Sound@FMOD@@QAG?AW4FMOD_RESULT@@PAII@Z");
	gPrivateFuncs.FMOD_System_playSound = (decltype(gPrivateFuncs.FMOD_System_playSound))GetProcAddress(fmodex, "?playSound@System@FMOD@@QAG?AW4FMOD_RESULT@@W4FMOD_CHANNELINDEX@@PAVSound@2@_NPAPAVChannel@2@@Z");

	if (gPrivateFuncs.FMOD_System_playSound)
	{
	//	Install_InlineHook(FMOD_System_playSound);
	}
}

void FMOD_UninstallHooks(HMODULE fmodex)
{
	//Uninstall_Hook(FMOD_System_playSound);
}

bool SCR_IsLoadingVisible(void)
{
	return scr_drawloading && (*scr_drawloading) == 1 ? true : false;
}

void Engine_FillAddress_GetClientTime(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID GetClientTime_VA = ConvertDllInfoSpace(gEngfuncs.GetClientTime, RealDllInfo, DllInfo);

	ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(GetClientTime_VA, 0x20, "\xDD\x05");
	Sig_AddrNotFound("cl_time");

	PVOID cl_time_VA = *(PVOID*)(addr + 2);

	cl_time = (double*)ConvertDllInfoSpace(cl_time_VA, DllInfo, RealDllInfo);
	cl_oldtime = cl_time + 1;
}

void Engine_FillAddress_RealTime(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		double *realtime = NULL;
	*/

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		/*
.text:101A8F2D                                     loc_101A8F2D:                           ; CODE XREF: sub_101A8DF0+131↑j
.text:101A8F2D F2 0F 10 05 98 9E 24 11                             movsd   xmm0, realtime
.text:101A8F35 66 0F 5A C0                                         cvtpd2ps xmm0, xmm0
.text:101A8F39 6A 60                                               push    60h ; '`'       ; Size
.text:101A8F3B 6A 00                                               push    0               ; Val
		*/

		char pattern[] = "\x01\x00\x00\x00\xF2\x0F\x10\x05\x2A\x2A\x2A\x2A\x66\x0F\x5A\xC0\x6A\x60";

		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(realtime);

		PVOID realtime_VA = *(PVOID*)(addr + 8);
		realtime = (decltype(realtime))ConvertDllInfoSpace(realtime_VA, DllInfo, RealDllInfo);
	}
	else
	{
		/*
.text:01D2DBA9                                     loc_1D2DBA9:                            ; CODE XREF: sub_1D2DA60+11D↑j
.text:01D2DBA9 C7 05 24 91 10 02 01 00 00 00                       mov     dword_2109124, 1
.text:01D2DBB3
.text:01D2DBB3                                     loc_1D2DBB3:                            ; CODE XREF: sub_1D2DA60+147↑j
.text:01D2DBB3 DD 05 58 6C 44 08                                   fld     realtime
.text:01D2DBB9 6A 60                                               push    60h ; '`'       ; Size
.text:01D2DBBB 6A 00                                               push    0               ; Val
		*/
		char pattern[] = "\x01\x00\x00\x00\xDD\x05\x2A\x2A\x2A\x2A\x6A\x60";

		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(realtime);

		PVOID realtime_VA = *(PVOID*)(addr + 6);
		realtime = (decltype(realtime))ConvertDllInfoSpace(realtime_VA, DllInfo, RealDllInfo);
	}
}

void Engine_FillAddress_S_Init(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.S_Init)
		return;

	PVOID S_Init_VA = nullptr;

	if (1)
	{
		/*
.text:01D96050                                     S_Init          proc near               ; CODE XREF: sub_1D65260+32B¡üp
.text:01D96050 68 08 CE E6 01                                      push    offset aSoundInitializ ; "Sound Initialization\n"
.text:01D96055 E8 76 DB F6 FF                                      call    sub_1D03BD0
.text:01D9605A E8 E1 3A 00 00                                      call    sub_1D99B40
.text:01D9605F 68 D8 D9 E5 01                                      push    offset aNosound ; "-nosound"
		  */

		const char sigs[] = "Sound Initialization\n";
		auto Sound_Init_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Sound_Init_String)
			Sound_Init_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (Sound_Init_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 1) = (DWORD)Sound_Init_String;
			auto Sound_Init_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (Sound_Init_PushString)
			{
				S_Init_VA = Sound_Init_PushString;
			}
		}
	}

	if (!S_Init_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			S_Init_VA = Search_Pattern(S_INIT_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			S_Init_VA = Search_Pattern(S_INIT_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			S_Init_VA = Search_Pattern(S_INIT_SIG_NEW, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			S_Init_VA = Search_Pattern(S_INIT_SIG_BLOB, DllInfo);
		}
	}

	gPrivateFuncs.S_Init = (decltype(gPrivateFuncs.S_Init))ConvertDllInfoSpace(S_Init_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(S_Init);
}

void Engine_FillAddress_S_FindName(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.S_FindName)
		return;

	PVOID S_FindName_VA = nullptr;

	if (1)
	{
		/*
.text:01D97B9A 6A 00                                               push    0
.text:01D97B9C 50                                                  push    eax/edx
.text:01D97B9D E8 5E F4 FF FF                                      call    S_FindName
.text:01D97BA2 83 C4 08                                            add     esp, 8
.text:01D97BA5 85 C0                                               test    eax, eax
.text:01D97BA7 75 26                                               jnz     short loc_1D97BCF
.text:01D97BA9 8D 04 24                                            lea     eax, [esp+104h+var_104]
.text:01D97BAC 50                                                  push    eax
.text:01D97BAD 68 F0 D1 E6 01                                      push    offset aSSayReliableCa_0 ; "S_Say_Reliable: can't find sentence nam"...
.text:01D97BB2 E8 09 BF F6 FF                                      call    sub_1D03AC0
		  */

		const char sigs[] = "S_Say_Reliable: can't find sentence";
		auto S_Say_Reliable_String = Search_Pattern_Data(sigs, DllInfo);
		if (!S_Say_Reliable_String)
			S_Say_Reliable_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (S_Say_Reliable_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
			*(DWORD*)(pattern + 1) = (DWORD)S_Say_Reliable_String;
			auto S_Say_Reliable_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (S_Say_Reliable_PushString)
			{
				char pattern2[] = "\x6A\x00\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
				auto S_FindName_Call = (PUCHAR)Search_Pattern_From_Size((S_Say_Reliable_PushString - 0x60), 0x60, pattern2);
				if (S_FindName_Call)
				{
					S_FindName_VA = GetCallAddress(S_FindName_Call + 3);
				}
			}
		}
	}

	if (!S_FindName_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_NEW, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_BLOB, DllInfo);
		}
	}

	gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))ConvertDllInfoSpace(S_FindName_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(S_FindName);
}

void Engine_FillAddress_S_StartDynamicSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.S_StartDynamicSound)
		return;

	PVOID S_StartDynamicSound_VA = nullptr;

	if (1)
	{
		/*
.text:01D8C299 68 FC 45 E5 01                                      push    offset aSStartdynamics ; "S_StartDynamicSound: %s volume > 255"
.text:01D8C29E E8 ED 09 FA FF                                      call    sub_1D2CC90
.text:01D8C2A3 83 C4 08                                            add     esp, 8
		*/
		const char sigs[] = "Warning: S_StartDynamicSound Ignored";
		auto S_StartDynamicSound_String = Search_Pattern_Data(sigs, DllInfo);
		if (!S_StartDynamicSound_String)
			S_StartDynamicSound_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (S_StartDynamicSound_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)S_StartDynamicSound_String;
			auto S_StartDynamicSound_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (S_StartDynamicSound_PushString)
			{
				S_StartDynamicSound_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_StartDynamicSound_PushString, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
					{
						return TRUE;
					}

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
					{
						return TRUE;
					}

					return FALSE;
				});
			}
		}
	}

	if (!S_StartDynamicSound_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_NEW, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_BLOB, DllInfo);
		}
	}

	gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))ConvertDllInfoSpace(S_StartDynamicSound_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(S_StartDynamicSound);
}

void Engine_FillAddress_S_StartStaticSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.S_StartStaticSound)
		return;

	PVOID S_StartStaticSound_VA = nullptr;

	if (1)
	{
		/*
.text:01D96FE6 68 C0 4D ED 01                                      push    offset aWarningSStarts ; "Warning: S_StartStaticSound Ignored, ca"...
.text:01D96FEB E8 50 89 F9 FF                                      call    sub_1D2F940
.text:01D96FF0 83 C4 04                                            add     esp, 4
		*/
		const char sigs[] = "Warning: S_StartStaticSound Ignored";
		auto S_StartStaticSound_String = Search_Pattern_Data(sigs, DllInfo);
		if (!S_StartStaticSound_String)
			S_StartStaticSound_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (S_StartStaticSound_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)S_StartStaticSound_String;
			auto S_StartStaticSound_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (S_StartStaticSound_PushString)
			{
				S_StartStaticSound_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_StartStaticSound_PushString, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
					{
						return TRUE;
					}

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
					{
						return TRUE;
					}

					return FALSE;
				});
			}
		}
	}

	if (!S_StartStaticSound_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_NEW, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_BLOB, DllInfo);
		}
	}
	gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))ConvertDllInfoSpace(S_StartStaticSound_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(S_StartStaticSound);
}

void Engine_FillAddress_S_LoadSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.S_LoadSound)
		return;

	PVOID S_LoadSound_VA = nullptr;

	if (1)
	{
		/*
.text:01D98912 68 F0 52 ED 01                                      push    offset aSLoadsoundCoul ; "S_LoadSound: Couldn't load %s\n"
.text:01D98917 E8 24 70 F9 FF                                      call    sub_1D2F940
.text:01D9891C 83 C4 08                                            add     esp, 8
		*/
		const char sigs[] = "S_LoadSound: Couldn't load %s";
		auto S_LoadSound_String = Search_Pattern_Data(sigs, DllInfo);
		if (!S_LoadSound_String)
			S_LoadSound_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (S_LoadSound_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)S_LoadSound_String;
			auto S_LoadSound_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (S_LoadSound_PushString)
			{
				S_LoadSound_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_LoadSound_PushString, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
					{
						return TRUE;
					}

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
					{
						return TRUE;
					}

					//.text:01D98710 81 EC 48 05 00 00                                   sub     esp, 548h
					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00)
					{
						return TRUE;
					}

					return FALSE;
				});
			}
		}
	}

	if (!S_LoadSound_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_NEW, DllInfo);
			if (!S_LoadSound_VA)
				S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_8308, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_BLOB, DllInfo);
		}
	}

	gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))ConvertDllInfoSpace(S_LoadSound_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(S_LoadSound);
}

void Engine_FillAddress_TextMessageParse(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.TextMessageParse)
		return;

	PVOID TextMessageParse_VA = nullptr;

	/*
	.text:10226B8A 68 44 7F 2C 10                                      push    offset aTmessageTextme ; "tmessage::TextMessageParse : messageCou"...
	.text:10226B8F E8 9C 8C FF FF                                      call    Sys_Error
	.text:10226B94                                     ; ---------------------------------------------------------------------------
	.text:10226B94 83 C4 04                                            add     esp, 4
			*/
	const char sigs[] = "tmessage::TextMessageParse";
	auto TextMessageParse_String = Search_Pattern_Data(sigs, DllInfo);
	if (!TextMessageParse_String)
		TextMessageParse_String = Search_Pattern_Rdata(sigs, DllInfo);
	if (TextMessageParse_String)
	{
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
		*(DWORD*)(pattern + 1) = (DWORD)TextMessageParse_String;
		auto TextMessageParse_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
		if (TextMessageParse_PushString)
		{
			TextMessageParse_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(TextMessageParse_PushString, 0x500, [](PUCHAR Candidate) {

				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC)
				{
					return TRUE;
				}

				//.text:01DC1250 B8 F8 F1 00 00                                      mov     eax, 0F1F8h
				//.text : 01DC1255 E8 F6 25 0A 00                                      call    __alloca_probe
				if (Candidate[0] == 0xB8 &&
					Candidate[5] == 0xE8)
				{
					return TRUE;
				}

				return FALSE;
				});
			
		}
	}

	gPrivateFuncs.TextMessageParse = (decltype(gPrivateFuncs.TextMessageParse))ConvertDllInfoSpace(TextMessageParse_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(TextMessageParse);
}
// End of Selection


void Engine_FillAddress_COM_ExplainDisconnection(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x68\x2A\x2A\x2A\x2A\x6A\x01";
	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			const char* pPushString = *(const char**)(pFound + 1);

			if (((ULONG_PTR)pPushString >= (ULONG_PTR)DllInfo.RdataBase && (ULONG_PTR)pPushString < (ULONG_PTR)DllInfo.RdataBase + DllInfo.RdataSize) ||
				((ULONG_PTR)pPushString >= (ULONG_PTR)DllInfo.DataBase && (ULONG_PTR)pPushString < (ULONG_PTR)DllInfo.DataBase + DllInfo.DataSize))
			{
				if (0 == memcmp(pPushString, "#GameUI_DisconnectedFromServerExtended", sizeof("#GameUI_DisconnectedFromServerExtended") - 1))
				{
					typedef struct COM_ExplainDisconnectionSearchContext_s
					{
						const mh_dll_info_t& DllInfo;
						const mh_dll_info_t& RealDllInfo;
						int instCount_push0h{};
					}COM_ExplainDisconnectionSearchContext;

					COM_ExplainDisconnectionSearchContext ctx = { DllInfo, RealDllInfo };

					g_pMetaHookAPI->DisasmRanges(pFound + Sig_Length(pattern), 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx = (COM_ExplainDisconnectionSearchContext*)context;

						if (gPrivateFuncs.COM_ExplainDisconnection && gPrivateFuncs.COM_ExtendedExplainDisconnection)
							return TRUE;

						if (!gPrivateFuncs.COM_ExplainDisconnection && address[0] == 0xE8 && instCount < 3)
						{
							PVOID COM_ExplainDisconnection_VA = GetCallAddress(address);
							gPrivateFuncs.COM_ExplainDisconnection = (decltype(gPrivateFuncs.COM_ExplainDisconnection))ConvertDllInfoSpace(COM_ExplainDisconnection_VA, ctx->DllInfo, ctx->RealDllInfo);
							return FALSE;
						}

						if (gPrivateFuncs.COM_ExplainDisconnection && !ctx->instCount_push0h && address[0] == 0x6A && address[1] == 0x01)
						{
							ctx->instCount_push0h = instCount;
							return FALSE;
						}

						if (!gPrivateFuncs.COM_ExtendedExplainDisconnection && address[0] == 0xE8 && instCount > ctx->instCount_push0h && instCount < ctx->instCount_push0h + 3)
						{
							PVOID COM_ExtendedExplainDisconnection_VA = GetCallAddress(address);
							gPrivateFuncs.COM_ExtendedExplainDisconnection = (decltype(gPrivateFuncs.COM_ExtendedExplainDisconnection))ConvertDllInfoSpace(COM_ExtendedExplainDisconnection_VA, ctx->DllInfo, ctx->RealDllInfo);
							return FALSE;
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;

						}, 0, &ctx);

					break;
				}
			}

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
	Sig_FuncNotFound(COM_ExplainDisconnection);
	Sig_FuncNotFound(COM_ExtendedExplainDisconnection);
}

void Engine_FillAddress_SequenceGetSentenceByIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.SequenceGetSentenceByIndex)
		return;

	PVOID SequenceGetSentenceByIndex_VA = nullptr;

	if (1)
	{
		const char pattern[] = "\x50\xFF\x15\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
		auto SequenceGetSentenceByIndex_Call = (PUCHAR)Search_Pattern(pattern, DllInfo);
		if (SequenceGetSentenceByIndex_Call)
		{
			SequenceGetSentenceByIndex_VA = GetCallAddress(SequenceGetSentenceByIndex_Call + 8);
		}
		else
		{
			const char pattern[] = "\x50\xE8\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
			auto SequenceGetSentenceByIndex_Call = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (SequenceGetSentenceByIndex_Call)
			{
				SequenceGetSentenceByIndex_VA = GetCallAddress(SequenceGetSentenceByIndex_Call + 7);
			}
		}
	}

	if (!SequenceGetSentenceByIndex_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_NEW, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_BLOB, DllInfo);
		}
	}

	gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))ConvertDllInfoSpace(SequenceGetSentenceByIndex_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(SequenceGetSentenceByIndex);
}

void Engine_FillAddress_SCR_BeginLoadingPlaque(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.SCR_BeginLoadingPlaque)
		return;

	PVOID SCR_BeginLoadingPlaque_VA = 0;

	//All engine use the same signature
	SCR_BeginLoadingPlaque_VA = Search_Pattern(SCR_BEGIN_LOADING_PLAQUE, DllInfo);

	gPrivateFuncs.SCR_BeginLoadingPlaque = (decltype(gPrivateFuncs.SCR_BeginLoadingPlaque))ConvertDllInfoSpace(SCR_BeginLoadingPlaque_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(SCR_BeginLoadingPlaque);

	{
		typedef struct SCR_BeginLoadingPlaque_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} SCR_BeginLoadingPlaque_SearchContext;

		SCR_BeginLoadingPlaque_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(SCR_BeginLoadingPlaque_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (SCR_BeginLoadingPlaque_SearchContext*)context;

			if (!scr_drawloading &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{
				//C7 05 60 66 00 08 01 00 00 00                       mov     scr_drawloading, 1
				scr_drawloading = (decltype(scr_drawloading))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (scr_drawloading)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		Sig_VarNotFound(scr_drawloading);
	}
}

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_viewentity = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
		auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_SVENGINE);
		Sig_AddrNotFound(cl_viewentity);
		PVOID cl_viewentity_VA = *(PVOID*)(addr + 10);
		cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
		auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_GOLDSRC);
		Sig_AddrNotFound(cl_viewentity);

		typedef struct CL_ViewEntity_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			bool found_cmp_200{};
		} CL_ViewEntity_SearchContext;

		CL_ViewEntity_SearchContext ctx = { DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_ViewEntity_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x200)
			{
				ctx->found_cmp_200 = true;
			}

			if (ctx->found_cmp_200)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		if (ctx.found_cmp_200)
		{
			PVOID cl_viewentity_VA = *(PVOID*)(addr + 1);
			cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_VarNotFound(cl_viewentity);
}

void Engine_FillAddress_ListenerOrigin(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
.text:01D98363 D9 54 24 04                                         fst     [esp+118h+var_114]
.text:01D98367 D9 1C 24                                            fstp    [esp+118h+var_118]
.text:01D9836A 68 24 85 E3 08                                      push    offset listener_origin
.text:01D9836F 50                                                  push    eax
.text:01D98370 6A 00                                               push    0
.text:01D98372 52                                                  push    edx
		*/
		const char pattern[] = "\xD9\x54\x24\x2A\xD9\x1C\x24\x68\x2A\x2A\x2A\x2A\x50\x6A\x00\x2A\xE8";
		DWORD addr = (DWORD)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(listener_origin);
		PVOID listener_origin_VA = *(PVOID*)(addr + 8);
		listener_origin = (decltype(listener_origin))ConvertDllInfoSpace(listener_origin_VA, DllInfo, RealDllInfo);
		Sig_VarNotFound(listener_origin);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		/*
.text:10195E93 F3 0F 10 00                                         movss   xmm0, dword ptr [eax]
.text:10195E97 F3 0F 11 05 44 2B 28 11                             movss   listener_origin, xmm0
.text:10195E9F F3 0F 10 40 04                                      movss   xmm0, dword ptr [eax+4]
.text:10195EA4 F3 0F 11 05 48 2B 28 11                             movss   dword_11282B48, xmm0
.text:10195EAC F3 0F 10 40 08                                      movss   xmm0, dword ptr [eax+8]
.text:10195EB1 F3 0F 11 05 4C 2B 28 11                             movss   dword_11282B4C, xmm0
.text:10195EB9 5D                                                  pop     ebp
.text:10195EBA C3                                                  retn
.text:10195EBA                                     sub_10195E80    endp
		*/

		const char pattern[] = "\xF3\x0F\x10\x00\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xF3\x0F\x10\x40\x04";
		DWORD addr = (DWORD)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(listener_origin);
		PVOID listener_origin_VA = (PVOID)(addr + 4);
		listener_origin = (decltype(listener_origin))ConvertDllInfoSpace(listener_origin_VA, DllInfo, RealDllInfo);
		Sig_VarNotFound(listener_origin);
	}
	else
	{
		/*
.text:01D8C8D0 50                                                  push    eax
.text:01D8C8D1 68 F0 01 73 02                                      push    offset listener_origin
.text:01D8C8D6 E8 75 34 FB FF                                      call    sub_1D3FD50
.text:01D8C8DB 8B C8                                               mov     ecx, eax
.text:01D8C8DD 83 C4 08                                            add     esp, 8
		*/
		const char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B\xC8";
		DWORD addr = (DWORD)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(listener_origin);
		PVOID listener_origin_VA = *(PVOID*)(addr + 2);
		listener_origin = (decltype(listener_origin))ConvertDllInfoSpace(listener_origin_VA, DllInfo, RealDllInfo);
		Sig_VarNotFound(listener_origin);
	}
}

void Engine_FillAddress_VOX_LookupString(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	
		char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
		int *cszrawsentences = NULL;
	*/
#define VOX_LOOKUPSTRING_SIG "\x80\x2A\x23\x2A\x2A\x8D\x2A\x01\x50\xE8"
#define VOX_LOOKUPSTRING_SIG_HL25 "\x80\x3B\x23\x0F\x85\x90\x00\x00\x00"
	if (1)
	{
		PVOID addr = NULL;

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			addr = Search_Pattern(VOX_LOOKUPSTRING_SIG_HL25, DllInfo);
		else
			addr = Search_Pattern(VOX_LOOKUPSTRING_SIG, DllInfo);

		if (addr)
		{
			typedef struct VOX_LookupString_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
			} VOX_LookupString_SearchContext;

			VOX_LookupString_SearchContext ctx = { DllInfo, RealDllInfo };

			g_pMetaHookAPI->DisasmRanges(addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;
				auto ctx = (VOX_LookupString_SearchContext*)context;

				if (g_iEngineType == ENGINE_SVENGINE)
				{
					if (!cszrawsentences &&
						pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].size == 4)
					{
						//.text:01D99D06 39 35 18 A2 E0 08                                            cmp     cszrawsentences, esi
						cszrawsentences = (decltype(cszrawsentences))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}


					if (!rgpszrawsentence &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index != 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
						pinst->detail->x86.operands[0].mem.scale == 4)
					{
						//.text:01D99D10 FF 34 B5 18 82 E0 08                                         push    rgpszrawsentence[esi*4]
						rgpszrawsentence = (decltype(rgpszrawsentence))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}

				}
				else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
				{
					if (!cszrawsentences &&
						pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].size == 4)
					{
						//.text:1020233E 39 35 9C FC 52 10											cmp     cszrawsentences, esi
						cszrawsentences = (decltype(cszrawsentences))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}


					if (!rgpszrawsentence &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index != 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
						pinst->detail->x86.operands[0].mem.scale == 4)
					{
						//.text:01D99D10 FF 34 B5 18 82 E0 08                                         push    rgpszrawsentence[esi*4]
						rgpszrawsentence = (decltype(rgpszrawsentence))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}

				}
				else
				{
					if (!cszrawsentences &&
						pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0 &&
						pinst->detail->x86.operands[1].mem.index == 0 &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						//.text:01D90EF9 A1 48 B2 3B 02                                      mov     eax, cszrawsentences
						cszrawsentences = (decltype(cszrawsentences))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}

					if (!rgpszrawsentence &&
						pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0 &&
						pinst->detail->x86.operands[1].mem.index != 0 &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
						pinst->detail->x86.operands[1].mem.scale == 4 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						//.text:01D90F04 8B 0C B5 00 34 72 02                                mov     ecx, rgpszrawsentence[esi*4]
						rgpszrawsentence = (decltype(rgpszrawsentence))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}
				}


				if (cszrawsentences && rgpszrawsentence)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				return FALSE;

				}, 0, &ctx);
		}

		Sig_VarNotFound(cszrawsentences);
		Sig_VarNotFound(rgpszrawsentence);
	}
}

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Engine_FillAddress_GetClientTime(DllInfo, RealDllInfo);
	Engine_FillAddress_RealTime(DllInfo, RealDllInfo);
	Engine_FillAddress_S_Init(DllInfo, RealDllInfo);
	Engine_FillAddress_S_FindName(DllInfo, RealDllInfo);
	Engine_FillAddress_S_StartDynamicSound(DllInfo, RealDllInfo);
	Engine_FillAddress_S_StartStaticSound(DllInfo, RealDllInfo);
	Engine_FillAddress_S_LoadSound(DllInfo, RealDllInfo);
	Engine_FillAddress_TextMessageParse(DllInfo, RealDllInfo);
	Engine_FillAddress_COM_ExplainDisconnection(DllInfo, RealDllInfo);
	Engine_FillAddress_SequenceGetSentenceByIndex(DllInfo, RealDllInfo);
	Engine_FillAddress_SCR_BeginLoadingPlaque(DllInfo, RealDllInfo);
	Engine_FillAddress_CL_ViewEntityVars(DllInfo, RealDllInfo);
	Engine_FillAddress_ListenerOrigin(DllInfo, RealDllInfo);
	Engine_FillAddress_VOX_LookupString(DllInfo, RealDllInfo);
}

void Client_FillAddress_SCClient_SoundEngine(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnHUD_PlayerMoveTexture = (PUCHAR)GetProcAddress((HMODULE)RealDllInfo.ImageBase, "HUD_PlayerMoveTexture");

	if (!pfnHUD_PlayerMoveTexture)
		Sig_NotFound(pfnHUD_PlayerMoveTexture);

	auto pfnHUD_PlayerMoveTexture_VA = (PUCHAR)ConvertDllInfoSpace(pfnHUD_PlayerMoveTexture, RealDllInfo, DllInfo);

	if (!pfnHUD_PlayerMoveTexture_VA)
		Sig_NotFound(pfnHUD_PlayerMoveTexture_VA);

	while (1)
	{
		if (pfnHUD_PlayerMoveTexture_VA[0] == 0xE9)
		{
			pfnHUD_PlayerMoveTexture_VA = (PUCHAR)GetCallAddress(pfnHUD_PlayerMoveTexture_VA);
		}
		else
		{
			break;
		}
	}

	if(pfnHUD_PlayerMoveTexture_VA[0] == 0xE8)
	{
		PVOID soundengine_VA = GetCallAddress(pfnHUD_PlayerMoveTexture_VA);

		gPrivateFuncs.ScClient_soundengine = (decltype(gPrivateFuncs.ScClient_soundengine))ConvertDllInfoSpace(soundengine_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(ScClient_soundengine);
}

void Client_FillAddress_SCClient_SoundEngine_maxsentences(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "Sentence length too long! Greater than";

	auto SentenceTooLong_String = (PVOID)nullptr;
	if (!SentenceTooLong_String)
		SentenceTooLong_String = Search_Pattern_Rdata(sigs, DllInfo);
	if (SentenceTooLong_String)
	{
		char pattern[] = "\x68\x80\x01\x00\x00\x68\x2A\x2A\x2A\x2A\x6A\x04";
		*(DWORD*)(pattern + 6) = (DWORD)SentenceTooLong_String;
		auto SentenceTooLong_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(SentenceTooLong_PushString);

		typedef struct SentenceTooLong_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int instCount_incReg{};
		}SentenceTooLong_SearchContext;

		SentenceTooLong_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(SentenceTooLong_PushString, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (SentenceTooLong_SearchContext*)context;

			if (!gPrivateFuncs.ScClient_soundengine_maxsentences &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.disp > 0x100000)
			{
				gPrivateFuncs.ScClient_soundengine_maxsentences = pinst->detail->x86.operands[1].mem.disp;
			}

			if (!gPrivateFuncs.ScClient_soundengine_maxsentences &&
				pinst->id == X86_INS_INC &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.disp > 0x100000)
			{
				gPrivateFuncs.ScClient_soundengine_maxsentences = pinst->detail->x86.operands[1].mem.disp;
			}

			if (gPrivateFuncs.ScClient_soundengine_maxsentences)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);


		Sig_FuncNotFound(ScClient_soundengine_maxsentences);
	}
}

void Client_FillAddress_SCClient_SoundEngine_LoadSoundList(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "Deleting outdated sound list file '%s'. Scanned map name";

	auto DeletingOutdated_String = (PVOID)nullptr;
	if (!DeletingOutdated_String)
		DeletingOutdated_String = Search_Pattern_Rdata(sigs, DllInfo);
	if (DeletingOutdated_String)
	{
		char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\x6A\x01";
		*(DWORD*)(pattern + 2) = (DWORD)DeletingOutdated_String;
		auto DeletingOutdated_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(DeletingOutdated_PushString);

		auto LoadSoundList_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(DeletingOutdated_PushString, 0x350, [](PUCHAR Candidate) {

			//.text:1000ECF0 81 EC A8 08 00 00                                   sub     esp, 8A8h
			//.text:1000ECF6 A1 40 E2 1B 10                                      mov     eax, ___security_cookie
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[6] == 0xA1)
			{
				return TRUE;
			}

			return FALSE;
			});

		gPrivateFuncs.ScClient_SoundEngine_LoadSoundList = (decltype(gPrivateFuncs.ScClient_SoundEngine_LoadSoundList))ConvertDllInfoSpace(LoadSoundList_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(ScClient_SoundEngine_LoadSoundList);
	}
}

void Client_FillAddress_SCClient_SoundEngine_PlayFMODSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		char pattern[] = "\x6A\x00\x50\x6A\xFF\x6A\x08\xE8";
		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);

		Sig_AddrNotFound("ScClient_SoundEngine_PlayFMODSound");

		typedef struct ScClient_SoundEngine_PlayFMODSoundContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			bool bFoundPush11B0D4{};
		}ScClient_SoundEngine_PlayFMODSoundContext;

		ScClient_SoundEngine_PlayFMODSoundContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(addr + Sig_Length(pattern) - 1, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (ScClient_SoundEngine_PlayFMODSoundContext*)context;

			if (address[0] == 0xE8)
			{
				auto callTarget = GetCallAddress(address);

				typedef struct ScClient_SoundEngine_PlayFMODSoundContext2_s
				{
					bool bFoundPush11B0D4{};
				}ScClient_SoundEngine_PlayFMODSoundContext2;

				ScClient_SoundEngine_PlayFMODSoundContext2 ctx2 = { 0 };

				g_pMetaHookAPI->DisasmRanges(callTarget, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto ctx2 = (ScClient_SoundEngine_PlayFMODSoundContext*)context;
					auto pinst = (cs_insn*)inst;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm >= 0x100000)
					{
						ctx2->bFoundPush11B0D4 = true;
						return TRUE;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

				}, 0, &ctx2);

				if (ctx2.bFoundPush11B0D4)
				{
					return FALSE;
				}

				gPrivateFuncs.ScClient_SoundEngine_PlayFMODSound = (decltype(gPrivateFuncs.ScClient_SoundEngine_PlayFMODSound))ConvertDllInfoSpace(callTarget, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

		Sig_FuncNotFound(ScClient_SoundEngine_PlayFMODSound);
	}
}

void Client_FillAddress_SCClient_SoundEngine_LookupSoundBySentenceIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "Tried to look up sound by sentence index out";

	auto TriedToLookUp_String = (PVOID)nullptr;
	if (!TriedToLookUp_String)
		TriedToLookUp_String = Search_Pattern_Rdata(sigs, DllInfo);
	if (TriedToLookUp_String)
	{
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x6A\x04\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)TriedToLookUp_String;
		auto TriedToLookUp_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(TriedToLookUp_PushString);

		auto LookupSoundBySentenceIndex_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(TriedToLookUp_PushString, 0x50, [](PUCHAR Candidate) {

			//.text:1000CFE0 8B 54 24 04                                         mov     edx, [esp + index]
			//.text : 1000CFE4 81 FA FF 0F 00 00                                   cmp     edx, 0FFFh
			if (Candidate[-1] == 0xCC && 
				Candidate[0] == 0x8B &&
				Candidate[2] == 0x24 &&
				Candidate[3] == 0x04)
			{
				return TRUE;
			}

			return FALSE;
		});

		gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySentenceIndex = (decltype(gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySentenceIndex))ConvertDllInfoSpace(LookupSoundBySentenceIndex_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(ScClient_SoundEngine_LookupSoundBySentenceIndex);
	}

	//char pattern[] = "\x8B\x54\x24\x04\x81\xFA\xFF\x0F\x00\x00\x2A\x2A\x83\x3C\x91\x00\x2A\x2A\x0F\xAE\xE8";
	//auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
	//Sig_AddrNotFound("ScClient_SoundEngine_LookupSoundBySentenceIndex");

	//gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySentenceIndex = (decltype(gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySentenceIndex))ConvertDllInfoSpace(addr, DllInfo, RealDllInfo);
}

void Client_FillAddress_SCClient_SoundEngine_LookupSoundBySample(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "Tried to look up sound by sample";

	auto TriedToLookUp_String = (PVOID)nullptr;
	if (!TriedToLookUp_String)
		TriedToLookUp_String = Search_Pattern_Rdata(sigs, DllInfo);
	if (TriedToLookUp_String)
	{
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x6A\x04\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)TriedToLookUp_String;
		auto TriedToLookUp_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(TriedToLookUp_PushString);

		auto LookupSoundBySample_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(TriedToLookUp_PushString, 0x350, [](PUCHAR Candidate) {

	//		.text:1000D030; int __thiscall sub_1000D030(int this, const char* ArgList)
	//			.text:1000D030                                     sub_1000D030    proc near; CODE XREF : sub_1000D9F0 + 190↓p
	//			.text : 1000D030; sub_1000F750 + C1↓p
	//			.text:1000D030
	//			.text : 1000D030                                     var_4 = dword ptr - 4
	//			.text : 1000D030                                     ArgList = dword ptr  4
	//			.text : 1000D030
	//			.text : 1000D030 51                                                  push    ecx
	//			.text : 1000D031 55                                                  push    ebp; ArgList
	//			.text:1000D032 8B 6C 24 0C                                         mov     ebp, [esp + 8 + ArgList]
			if (Candidate[-1] == 0xCC &&
				Candidate[0] >= 0x50 &&
				Candidate[0] <= 0x57 &&
				Candidate[1] >= 0x50 &&
				Candidate[1] <= 0x57 &&
				Candidate[2] == 0x8B)
			{
				return TRUE;
			}

			return FALSE;
			});

		gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySample = (decltype(gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySample))ConvertDllInfoSpace(LookupSoundBySample_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(ScClient_SoundEngine_LookupSoundBySample);
	}
}

void Client_FillAddress_SCClient_GetClientColor(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	char pattern[] = "\x8B\x4C\x24\x04\x85\xC9\x2A\x2A\x6B\xC1";
	auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(GetClientColor);
	
	gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))ConvertDllInfoSpace(addr, DllInfo, RealDllInfo);
	Sig_FuncNotFound(GetClientColor);
}

void Client_FillAddress_SCClient_GameViewport_AllowedToPrintText(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into client dll vars.
		void *GameViewport = NULL;
	*/

	char pattern[] = "\x8B\x0D\x2A\x2A\x2A\x2A\x85\xC9\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x84\xC0\x0F";
	auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);

	Sig_AddrNotFound(GameViewport);

	PVOID GameViewport_VA = *(PVOID*)(addr + 2);
	GameViewport = (decltype(GameViewport))ConvertDllInfoSpace(GameViewport_VA, DllInfo, RealDllInfo);

	Sig_VarNotFound(GameViewport);

	PVOID GameViewport_AllowedToPrintText_VA = GetCallAddress(addr + 10);
	gPrivateFuncs.GameViewport_AllowedToPrintText = (decltype(gPrivateFuncs.GameViewport_AllowedToPrintText))ConvertDllInfoSpace(GameViewport_AllowedToPrintText_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(GameViewport_AllowedToPrintText);
}

void Client_FillAddress_SCClient_GameViewport_IsScoreBoardVisible(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	char pattern[] = "\x8B\x01\x8B\x40\x28\xFF\xE0";
	auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);

	Sig_AddrNotFound(GameViewport_IsScoreBoardVisible);

	typedef struct GameViewport_IsScoreBoardVisible_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} GameViewport_IsScoreBoardVisible_SearchContext;

	GameViewport_IsScoreBoardVisible_SearchContext ctx = { DllInfo, RealDllInfo };

	PVOID GameViewport_IsScoreBoardVisible_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(addr, 0x50, [](PUCHAR Candidate) {

		//8B 89 2C 10 00 00                                   mov     ecx, [ecx+102Ch]
		if (Candidate[0] == 0x8B &&
			Candidate[1] == 0x89 &&
			Candidate[4] == 0x00 &&
			Candidate[5] == 0x00)
		{
			return TRUE;
		}

		return FALSE;
	});

	gPrivateFuncs.GameViewport_IsScoreBoardVisible = (decltype(gPrivateFuncs.GameViewport_IsScoreBoardVisible))ConvertDllInfoSpace(GameViewport_IsScoreBoardVisible_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(GameViewport_IsScoreBoardVisible);
}

void Client_FillAddress_SCClient_WeaponsResource_SelectSlot(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	char pattern[] = "common/wpn_hudon.wav";
	auto addr = (PUCHAR)Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, pattern);

	Sig_AddrNotFound(wpn_hudon_wav_String);

	char pattern2[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
	*(DWORD*)(pattern2 + 1) = (DWORD)addr;
	auto wpn_hudon_PushString = Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, pattern2);
	Sig_VarNotFound(wpn_hudon_PushString);

	typedef struct WeaponsResource_SelectSlot_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} WeaponsResource_SelectSlot_SearchContext;

	WeaponsResource_SelectSlot_SearchContext ctx = { DllInfo, RealDllInfo };

	PVOID WeaponsResource_SelectSlot_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(wpn_hudon_PushString, 0x250, [](PUCHAR Candidate) {

		//.text:10054A80 55                                                  push    ebp
		//.text:10054A81 8B EC                                               mov     ebp, esp
		//.text:10054A83 83 EC 18                                            sub     esp, 18h
		if (Candidate[0] == 0x55 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xEC &&
			Candidate[3] == 0x83 &&
			Candidate[4] == 0xEC)
		{
			return TRUE;
		}

		return FALSE;
		});

	gPrivateFuncs.WeaponsResource_SelectSlot = (decltype(gPrivateFuncs.WeaponsResource_SelectSlot))ConvertDllInfoSpace(WeaponsResource_SelectSlot_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(WeaponsResource_SelectSlot);
}

void Client_FillAddress_SCClient_CHud_GetBorderSize(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	char pattern[] = "\xF6\x05\x2A\x2A\x2A\x2A\x20\x2A\x2A\xB9\x2A\x2A\x2A\x2A\xE8";
	auto addr = (PUCHAR)Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, pattern);
	Sig_AddrNotFound(CHud_GetBorderSize);

	PVOID gHud_VA = *(PVOID*)(addr + 10);
	gHud = (decltype(gHud))ConvertDllInfoSpace(gHud_VA, DllInfo, RealDllInfo);
	
	PVOID CHud_GetBorderSize_VA = GetCallAddress(addr + Sig_Length(pattern) - 1);
	gPrivateFuncs.CHud_GetBorderSize = (decltype(gPrivateFuncs.CHud_GetBorderSize))ConvertDllInfoSpace(CHud_GetBorderSize_VA, DllInfo, RealDllInfo);
}

void Client_FillAddress_SCClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		g_bIsSvenCoop = true;

		Client_FillAddress_SCClient_SoundEngine(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_maxsentences(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_LoadSoundList(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_PlayFMODSound(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_LookupSoundBySentenceIndex(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_SoundEngine_LookupSoundBySample(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_GetClientColor(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_GameViewport_AllowedToPrintText(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_GameViewport_IsScoreBoardVisible(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_WeaponsResource_SelectSlot(DllInfo, RealDllInfo);

		Client_FillAddress_SCClient_CHud_GetBorderSize(DllInfo, RealDllInfo);
	}
}

void Client_FillAddress_CounterStrike_GetTextColor(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		char pattern[] = "\x8B\x44\x24\x04\x83\xE8\x03\x2A\x2A\x48";
		auto addr = Search_Pattern(pattern, DllInfo);
		gPrivateFuncs.GetTextColor = (decltype(gPrivateFuncs.GetTextColor))ConvertDllInfoSpace(addr, DllInfo, RealDllInfo);
	}

	if(1)
	{
		char pattern[] = "\x8D\x41\x01\x50\xE8\x2A\x2A\x2A\x2A\xFF";
		auto addr = Search_Pattern(pattern, DllInfo);
		if (addr)
		{
			auto callTarget = GetCallAddress(addr + 4);
			gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))ConvertDllInfoSpace(callTarget, DllInfo, RealDllInfo);
		}
	}

	if (0 != strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		if (!gPrivateFuncs.GetTextColor)
		{
			if (!gPrivateFuncs.GetClientColor)
			{
				const char sigs1[] = "spec_mode_internal";
				auto SpecModeInternal_String = Search_Pattern_Data(sigs1, DllInfo);
				if (!SpecModeInternal_String)
					SpecModeInternal_String = Search_Pattern_Rdata(sigs1, DllInfo);
				Sig_VarNotFound(SpecModeInternal_String);

				char pattern[] = "\x68\x2A\x2A\x2A\x2A\xFF\x15";
				*(DWORD*)(pattern + 1) = (DWORD)SpecModeInternal_String;
				auto SpecModeInternal_PushString = Search_Pattern(pattern, DllInfo);

				Sig_VarNotFound(SpecModeInternal_PushString);

				typedef struct SpecModeInternal_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					int instCount_incReg{};
				}SpecModeInternal_SearchContext;

				SpecModeInternal_SearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges(SpecModeInternal_PushString, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (SpecModeInternal_SearchContext*)context;

					if (!ctx->instCount_incReg &&
						pinst->id == X86_INS_INC &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						ctx->instCount_incReg = instCount;
					}

					if (!ctx->instCount_incReg &&
						pinst->id == X86_INS_ADD &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 1)
					{
						ctx->instCount_incReg = instCount;
					}

					if (!ctx->instCount_incReg &&
						pinst->id == X86_INS_LEA &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.disp == 1)
					{
						ctx->instCount_incReg = instCount;
					}

					if (address[0] == 0xE8 && instCount > ctx->instCount_incReg && instCount < ctx->instCount_incReg + 3)
					{
						auto callTarget = GetCallAddress(address);

						gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))ConvertDllInfoSpace(callTarget, ctx->DllInfo, ctx->RealDllInfo);

						return TRUE;
					}

					if (gPrivateFuncs.GetClientColor)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx);
			}

			if (!gPrivateFuncs.GetClientColor)
			{
				if (g_iEngineType != ENGINE_GOLDSRC_HL25)
				{
					char pattern[] = "\x0F\xBF\x2A\x2A\x2A\x2A\x2A\x2A\x48\x83\xF8\x03\x77\x2A\xFF\x24";

					auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);

					if (addr)
					{
						auto GetClientColor_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(addr, 0x50, [](PUCHAR Candidate) {

							//8B 44 24 04                                         mov     eax, [esp+arg_0]
							if (Candidate[0] == 0x8B &&
								Candidate[1] == 0x44 &&
								Candidate[2] == 0x24)
							{
								return TRUE;
							}

							return FALSE;
							});

						gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))ConvertDllInfoSpace(GetClientColor_VA, DllInfo, RealDllInfo);
						Sig_FuncNotFound(GetClientColor);
					}
				}
				else
				{
					char pattern_HL25[] = "\x55\x8B\xEC\x6B\x45\x08\x74\x0F\xBF\x80\x2A\x2A\x2A\x2A\x48\x83\xF8\x03\x77\x23\xFF\x24\x85";

					auto addr = Search_Pattern(pattern_HL25, DllInfo);
					gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))ConvertDllInfoSpace(addr, DllInfo, RealDllInfo);

					Sig_FuncNotFound(GetClientColor);
				}
			}

			if (1)
			{
				char pattern[] = "\x33\xC0\xEB\x2A\xB8\x2A\x2A\x2A\x2A\xEB\x2A";
				auto addr = Search_Pattern(pattern, DllInfo);

				Sig_AddrNotFound(BaseTextColor);

				/*
					void* BaseTextColor = NULL;
				*/

				PVOID BaseTextColor_VA = *(PVOID*)((PUCHAR)addr + 5);
				gPrivateFuncs.BaseTextColor = (decltype(gPrivateFuncs.BaseTextColor))ConvertDllInfoSpace(BaseTextColor_VA, DllInfo, RealDllInfo);
			}
		}
	}
}
// End of Selection

void Client_FillAddress_CounterStrike(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		Client_FillAddress_CounterStrike_GetTextColor(DllInfo, RealDllInfo);
	}
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Client_FillAddress_SCClient(DllInfo, RealDllInfo);

	Client_FillAddress_CounterStrike(DllInfo, RealDllInfo);
}

void Engine_InstallHooks(void)
{
	Install_InlineHook(S_StartDynamicSound);
	Install_InlineHook(S_StartStaticSound);
	Install_InlineHook(pfnTextMessageGet);
	Install_InlineHook(pfnServerCmdUnreliable);
	Install_InlineHook(TextMessageParse);
	Install_InlineHook(COM_ExplainDisconnection);
}

void Engine_UninstallHooks(void)
{
	Uninstall_Hook(S_StartDynamicSound);
	Uninstall_Hook(S_StartStaticSound);
	Uninstall_Hook(pfnTextMessageGet);
	Uninstall_Hook(pfnServerCmdUnreliable);
	Uninstall_Hook(TextMessageParse);
	Uninstall_Hook(COM_ExplainDisconnection);
}

void Client_InstallHooks(void)
{
	if (gPrivateFuncs.ScClient_SoundEngine_PlayFMODSound)
	{
		Install_InlineHook(ScClient_SoundEngine_PlayFMODSound);
	}

	if (gPrivateFuncs.ScClient_SoundEngine_LoadSoundList)
	{
		Install_InlineHook(ScClient_SoundEngine_LoadSoundList);
	}

	if (gPrivateFuncs.WeaponsResource_SelectSlot)
	{
		Install_InlineHook(WeaponsResource_SelectSlot);
	}
}

void Client_UninstallHooks(void)
{
	Uninstall_Hook(ScClient_SoundEngine_PlayFMODSound);
	Uninstall_Hook(ScClient_SoundEngine_LoadSoundList);
	Uninstall_Hook(WeaponsResource_SelectSlot);
}

void DllLoadNotification(mh_load_dll_notification_context_t* ctx)
{
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)
	{
		if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_CLIENT)
		{
			
		}
		else if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"fmodex.dll"))
		{
			g_hFMODEx = ctx->hModule;
			FMOD_InstallHooks(ctx->hModule);
		}
	}
	else if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)
	{
		if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_CLIENT)
		{
			
		}
		else if (ctx->hModule == g_hFMODEx)
		{
			FMOD_UninstallHooks(ctx->hModule);
			g_hFMODEx = NULL;
		}
	}
}

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
	if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
	{
		auto addr_VA = (ULONG_PTR)addr;
		auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

		return (PVOID)VA_from_RVA(addr, TargetDllInfo);
	}

	return nullptr;
}

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo)
{
	if ((ULONG_PTR)vftable > (ULONG_PTR)RealDllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)RealDllInfo.ImageBase + RealDllInfo.ImageSize)
	{
		ULONG_PTR vftable_VA = (ULONG_PTR)vftable;
		ULONG vftable_RVA = RVA_from_VA(vftable, RealDllInfo);
		auto vftable_DllInfo = (decltype(vftable))VA_from_RVA(vftable, DllInfo);

		auto vf_VA = (ULONG_PTR)vftable_DllInfo[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}
	else if ((ULONG_PTR)vftable > (ULONG_PTR)DllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)DllInfo.ImageBase + DllInfo.ImageSize)
	{
		auto vf_VA = (ULONG_PTR)vftable[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}

	return vftable[index];
}