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
static hook_t *g_phook_TextMessageParse = NULL;
static hook_t* g_phook_COM_ExplainDisconnection = NULL;
static hook_t *g_phook_WeaponsResource_SelectSlot = NULL;
static hook_t *g_phook_ScClient_SoundEngine_PlayFMODSound = NULL;
static hook_t *g_phook_FMOD_System_playSound = NULL;

static HMODULE g_hFMODEx = NULL;

void FMOD_InstallHooks(HMODULE fmodex)
{
	gPrivateFuncs.FMOD_Sound_getLength = (decltype(gPrivateFuncs.FMOD_Sound_getLength))GetProcAddress(fmodex, "?getLength@Sound@FMOD@@QAG?AW4FMOD_RESULT@@PAII@Z");
	gPrivateFuncs.FMOD_System_playSound = (decltype(gPrivateFuncs.FMOD_System_playSound))GetProcAddress(fmodex, "?playSound@System@FMOD@@QAG?AW4FMOD_RESULT@@W4FMOD_CHANNELINDEX@@PAVSound@2@_NPAPAVChannel@2@@Z");

	if (gPrivateFuncs.FMOD_System_playSound)
	{
		Install_InlineHook(FMOD_System_playSound);
	}
}

void FMOD_UninstallHooks(HMODULE fmodex)
{
	Uninstall_Hook(FMOD_System_playSound);
}

bool SCR_IsLoadingVisible(void)
{
	return scr_drawloading && (*scr_drawloading) == 1 ? true : false;
}

void Engine_FillAddress(void)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		char pattern[] = "\x01\x00\x00\x00\xF2\x0F\x10\x05\x2A\x2A\x2A\x2A\x66\x0F\x5A\xC0\x6A\x60";

		auto addr = (PUCHAR)Search_Pattern(pattern);
		Sig_AddrNotFound("realtime");

		realtime = *(decltype(realtime)*)(addr + 8);
	}
	else
	{
		char pattern[] = "\x01\x00\x00\x00\xDD\x05\x2A\x2A\x2A\x2A\x6A\x60";

		auto addr = (PUCHAR)Search_Pattern(pattern);
		Sig_AddrNotFound("realtime");

		realtime = *(decltype(realtime)*)(addr + 6);
	}

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
		auto Sound_Init_String = Search_Pattern_Data(sigs);
		if (!Sound_Init_String)
			Sound_Init_String = Search_Pattern_Rdata(sigs);
		if (Sound_Init_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 1) = (DWORD)Sound_Init_String;
			auto Sound_Init_PushString = (PUCHAR)Search_Pattern(pattern);
			if (Sound_Init_PushString)
			{
				gPrivateFuncs.S_Init = (decltype(gPrivateFuncs.S_Init))Sound_Init_PushString;
			}
		}
	}

	if (!gPrivateFuncs.S_Init)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.S_Init = (decltype(gPrivateFuncs.S_Init))Search_Pattern(S_INIT_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.S_Init = (decltype(gPrivateFuncs.S_Init))Search_Pattern(S_INIT_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.S_Init = (decltype(gPrivateFuncs.S_Init))Search_Pattern(S_INIT_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.S_Init = (decltype(gPrivateFuncs.S_Init))Search_Pattern(S_INIT_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(S_Init);

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
		auto S_Say_Reliable_String = Search_Pattern_Data(sigs);
		if (!S_Say_Reliable_String)
			S_Say_Reliable_String = Search_Pattern_Rdata(sigs);
		if (S_Say_Reliable_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
			*(DWORD*)(pattern + 1) = (DWORD)S_Say_Reliable_String;
			auto S_Say_Reliable_PushString = (PUCHAR)Search_Pattern(pattern);
			if (S_Say_Reliable_PushString)
			{
				char pattern2[] = "\x6A\x00\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
				auto S_FindName_Call = (PUCHAR)Search_Pattern_From_Size((S_Say_Reliable_PushString - 0x60), 0x60, pattern2);
				if (S_FindName_Call)
				{
					gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))GetCallAddress(S_FindName_Call + 3);
				}
			}
		}
	}

	if (!gPrivateFuncs.S_FindName)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))Search_Pattern(S_FINDNAME_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))Search_Pattern(S_FINDNAME_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))Search_Pattern(S_FINDNAME_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))Search_Pattern(S_FINDNAME_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(S_FindName);

	if (1)
	{
		/*
.text:01D8C299 68 FC 45 E5 01                                      push    offset aSStartdynamics ; "S_StartDynamicSound: %s volume > 255"
.text:01D8C29E E8 ED 09 FA FF                                      call    sub_1D2CC90
.text:01D8C2A3 83 C4 08                                            add     esp, 8
		*/
		const char sigs[] = "Warning: S_StartDynamicSound Ignored";
		auto S_StartDynamicSound_String = Search_Pattern_Data(sigs);
		if (!S_StartDynamicSound_String)
			S_StartDynamicSound_String = Search_Pattern_Rdata(sigs);
		if (S_StartDynamicSound_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)S_StartDynamicSound_String;
			auto S_StartDynamicSound_PushString = (PUCHAR)Search_Pattern(pattern);
			if (S_StartDynamicSound_PushString)
			{
				gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_StartDynamicSound_PushString, 0x300, [](PUCHAR Candidate) {

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

	if (!gPrivateFuncs.S_StartDynamicSound)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))Search_Pattern(S_STARTDYNAMICSOUND_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))Search_Pattern(S_STARTDYNAMICSOUND_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))Search_Pattern(S_STARTDYNAMICSOUND_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))Search_Pattern(S_STARTDYNAMICSOUND_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(S_StartDynamicSound);

	if (1)
	{
		/*
.text:01D96FE6 68 C0 4D ED 01                                      push    offset aWarningSStarts ; "Warning: S_StartStaticSound Ignored, ca"...
.text:01D96FEB E8 50 89 F9 FF                                      call    sub_1D2F940
.text:01D96FF0 83 C4 04                                            add     esp, 4
		*/
		const char sigs[] = "Warning: S_StartStaticSound Ignored";
		auto S_StartStaticSound_String = Search_Pattern_Data(sigs);
		if (!S_StartStaticSound_String)
			S_StartStaticSound_String = Search_Pattern_Rdata(sigs);
		if (S_StartStaticSound_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)S_StartStaticSound_String;
			auto S_StartStaticSound_PushString = (PUCHAR)Search_Pattern(pattern);
			if (S_StartStaticSound_PushString)
			{
				gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_StartStaticSound_PushString, 0x300, [](PUCHAR Candidate) {

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

	if (!gPrivateFuncs.S_StartStaticSound)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))Search_Pattern(S_STARTSTATICSOUND_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))Search_Pattern(S_STARTSTATICSOUND_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))Search_Pattern(S_STARTSTATICSOUND_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))Search_Pattern(S_STARTSTATICSOUND_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(S_StartStaticSound);
	
	if (1)
	{
		/*
.text:01D98912 68 F0 52 ED 01                                      push    offset aSLoadsoundCoul ; "S_LoadSound: Couldn't load %s\n"
.text:01D98917 E8 24 70 F9 FF                                      call    sub_1D2F940
.text:01D9891C 83 C4 08                                            add     esp, 8
		*/
		const char sigs[] = "S_LoadSound: Couldn't load %s";
		auto S_LoadSound_String = Search_Pattern_Data(sigs);
		if (!S_LoadSound_String)
			S_LoadSound_String = Search_Pattern_Rdata(sigs);
		if (S_LoadSound_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)S_LoadSound_String;
			auto S_LoadSound_PushString = (PUCHAR)Search_Pattern(pattern);
			if (S_LoadSound_PushString)
			{
				gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_LoadSound_PushString, 0x500, [](PUCHAR Candidate) {

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

	if (!gPrivateFuncs.S_LoadSound)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_NEW);
			if (!gPrivateFuncs.S_LoadSound)
				gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_8308);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(S_LoadSound);

	if (1)
	{
		/*
.text:10226B8A 68 44 7F 2C 10                                      push    offset aTmessageTextme ; "tmessage::TextMessageParse : messageCou"...
.text:10226B8F E8 9C 8C FF FF                                      call    Sys_Error
.text:10226B94                                     ; ---------------------------------------------------------------------------
.text:10226B94 83 C4 04                                            add     esp, 4
		*/
		const char sigs[] = "tmessage::TextMessageParse";
		auto TextMessageParse_String = Search_Pattern_Data(sigs);
		if (!TextMessageParse_String)
			TextMessageParse_String = Search_Pattern_Rdata(sigs);
		if (TextMessageParse_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)TextMessageParse_String;
			auto TextMessageParse_PushString = (PUCHAR)Search_Pattern(pattern);
			if (TextMessageParse_PushString)
			{
				gPrivateFuncs.TextMessageParse = (decltype(gPrivateFuncs.TextMessageParse))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(TextMessageParse_PushString, 0x500, [](PUCHAR Candidate) {

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
		Sig_FuncNotFound(TextMessageParse);
	}

	if (1)
	{
		const char pattern[] = "\x68\x2A\x2A\x2A\x2A\x6A\x01";
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				const char* pPushString = *(const char**)(pFound + 1);

				if (((ULONG_PTR)pPushString >= (ULONG_PTR)g_dwEngineRdataBase && (ULONG_PTR)pPushString < (ULONG_PTR)g_dwEngineRdataBase + g_dwEngineRdataSize) ||
					((ULONG_PTR)pPushString >= (ULONG_PTR)g_dwEngineDataBase && (ULONG_PTR)pPushString < (ULONG_PTR)g_dwEngineDataBase + g_dwEngineDataSize))
				{
					if (0 == memcmp(pPushString, "#GameUI_DisconnectedFromServerExtended", sizeof("#GameUI_DisconnectedFromServerExtended") - 1))
					{
						typedef struct
						{
							int instCount_push0h;
						}COM_ExplainDisconnectionSearchContext;

						COM_ExplainDisconnectionSearchContext ctx = { 0 };

						g_pMetaHookAPI->DisasmRanges(pFound + Sig_Length(pattern), 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

							auto pinst = (cs_insn*)inst;
							auto ctx = (COM_ExplainDisconnectionSearchContext*)context;

							if (gPrivateFuncs.COM_ExplainDisconnection && gPrivateFuncs.COM_ExtendedExplainDisconnection)
								return TRUE;

							if (!gPrivateFuncs.COM_ExplainDisconnection && address[0] == 0xE8 && instCount < 3)
							{
								gPrivateFuncs.COM_ExplainDisconnection = (decltype(gPrivateFuncs.COM_ExplainDisconnection))GetCallAddress(address);
								return FALSE;
							}

							if (gPrivateFuncs.COM_ExplainDisconnection && !ctx->instCount_push0h && address[0] == 0x6A && address[1] == 0x01)
							{
								ctx->instCount_push0h = instCount;
								return FALSE;
							}

							if (!gPrivateFuncs.COM_ExtendedExplainDisconnection && address[0] == 0xE8 && instCount > ctx->instCount_push0h && instCount < ctx->instCount_push0h + 3)
							{
								gPrivateFuncs.COM_ExtendedExplainDisconnection = (decltype(gPrivateFuncs.COM_ExtendedExplainDisconnection))GetCallAddress(address);
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

	if (1)
	{
		const char pattern[] = "\x50\xFF\x15\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
		auto SequenceGetSentenceByIndex_Call = (PUCHAR)Search_Pattern(pattern);
		if (SequenceGetSentenceByIndex_Call)
		{
			gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))GetCallAddress(SequenceGetSentenceByIndex_Call + 8);
		}
		else
		{
			const char pattern[] = "\x50\xE8\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
			auto SequenceGetSentenceByIndex_Call = (PUCHAR)Search_Pattern(pattern);
			if (SequenceGetSentenceByIndex_Call)
			{
				gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))GetCallAddress(SequenceGetSentenceByIndex_Call + 7);
			}
		}
	}

	if (!gPrivateFuncs.SequenceGetSentenceByIndex)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(SequenceGetSentenceByIndex);

	gPrivateFuncs.SCR_BeginLoadingPlaque = (decltype(gPrivateFuncs.SCR_BeginLoadingPlaque))Search_Pattern(SCR_BEGIN_LOADING_PLAQUE);
	Sig_FuncNotFound(SCR_BeginLoadingPlaque);

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.SCR_BeginLoadingPlaque, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn *)inst;

			if (!scr_drawloading &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{
				//C7 05 60 66 00 08 01 00 00 00                       mov     scr_drawloading, 1
				scr_drawloading = (decltype(scr_drawloading))pinst->detail->x86.operands[0].mem.disp;
			}

			if (scr_drawloading)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(scr_drawloading);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_SVENGINE);
		Sig_AddrNotFound(cl_viewentity);
		cl_viewentity = *(decltype(cl_viewentity) *)(addr + 10);
	}
	else
	{
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_GOLDSRC);
		Sig_AddrNotFound(cl_viewentity);

		typedef struct
		{
			bool found_cmp_200;
		}CL_ViewEntity_ctx;

		CL_ViewEntity_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges((PVOID)addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (CL_ViewEntity_ctx*)context;

				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
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
			}, 0, & ctx);

		if (ctx.found_cmp_200)
		{
			cl_viewentity = *(decltype(cl_viewentity)*)(addr + 1);
		}
		Sig_VarNotFound(cl_viewentity);

#if 0
#define CL_VIEWENTITY_SIG_HL25 "\xE8\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xC3"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_HL25);
		Sig_AddrNotFound(cl_viewentity);
		cl_viewentity = (decltype(cl_viewentity))(addr + 5);
	}
	else
	{
#define CL_VIEWENTITY_SIG_NEW "\x8B\x0D\x2A\x2A\x2A\x2A\x6A\x64\x6A\x00\x68\x00\x00\x80\x3F\x68\x00\x00\x80\x3F\x68\x2A\x2A\x2A\x2A\x50"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_NEW);
		Sig_AddrNotFound(cl_viewentity);
		cl_viewentity = *(decltype(cl_viewentity) *)(addr + 2);
#endif
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define LISTENER_ORIGIN_SIG_SVENGINE "\xD9\x54\x24\x2A\xD9\x1C\x24\x68\x2A\x2A\x2A\x2A\x50\x6A\x00\x2A\xE8"
		DWORD addr = (DWORD)Search_Pattern(LISTENER_ORIGIN_SIG_SVENGINE);
		Sig_AddrNotFound(listener_origin);
		listener_origin = *(decltype(listener_origin) *)(addr + 8);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define LISTENER_ORIGIN_SIG_HL25 "\xF3\x0F\x10\x00\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xF3\x0F\x10\x40\x04"
		DWORD addr = (DWORD)Search_Pattern(LISTENER_ORIGIN_SIG_HL25);
		Sig_AddrNotFound(listener_origin);
		listener_origin = (decltype(listener_origin))(addr + 4);
	}
	else
	{
#define LISTENER_ORIGIN_SIG_NEW "\x50\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B\xC8"
		DWORD addr = (DWORD)Search_Pattern(LISTENER_ORIGIN_SIG_NEW);
		Sig_AddrNotFound(listener_origin);
		listener_origin = *(decltype(listener_origin) *)(addr + 2);
	}

#define VOX_LOOKUPSTRING_SIG "\x80\x2A\x23\x2A\x2A\x8D\x2A\x01\x50\xE8"
#define VOX_LOOKUPSTRING_SIG_HL25 "\x80\x3B\x23\x0F\x85\x90\x00\x00\x00"
	if (1)
	{
		const char sigs[] = "\x40\x68\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xA1";
		void *addr = NULL;

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			addr = Search_Pattern(VOX_LOOKUPSTRING_SIG_HL25); 
		else
			addr = Search_Pattern(VOX_LOOKUPSTRING_SIG);

		if (addr)
		{
			g_pMetaHookAPI->DisasmRanges(addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;

				if (g_iEngineType == ENGINE_SVENGINE)
				{
					if (!cszrawsentences &&
						pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].size == 4)
					{
						//.text:01D99D06 39 35 18 A2 E0 08                                            cmp     cszrawsentences, esi
						cszrawsentences = (decltype(cszrawsentences))pinst->detail->x86.operands[0].mem.disp;
					}


					if (!rgpszrawsentence &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index != 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
						pinst->detail->x86.operands[0].mem.scale == 4)
					{
						//.text:01D99D10 FF 34 B5 18 82 E0 08                                         push    rgpszrawsentence[esi*4]
						rgpszrawsentence = (decltype(rgpszrawsentence))pinst->detail->x86.operands[0].mem.disp;
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
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].size == 4)
					{
						//.text:1020233E 39 35 9C FC 52 10											cmp     cszrawsentences, esi
						cszrawsentences = (decltype(cszrawsentences))pinst->detail->x86.operands[0].mem.disp;
					}


					if (!rgpszrawsentence &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index != 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
						pinst->detail->x86.operands[0].mem.scale == 4)
					{
						//.text:01D99D10 FF 34 B5 18 82 E0 08                                         push    rgpszrawsentence[esi*4]
						rgpszrawsentence = (decltype(rgpszrawsentence))pinst->detail->x86.operands[0].mem.disp;
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
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						//.text:01D90EF9 A1 48 B2 3B 02                                      mov     eax, cszrawsentences
						cszrawsentences = (decltype(cszrawsentences))pinst->detail->x86.operands[1].mem.disp;
					}

					if (!rgpszrawsentence &&
						pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0 &&
						pinst->detail->x86.operands[1].mem.index != 0 &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
						pinst->detail->x86.operands[1].mem.scale == 4 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						//.text:01D90F04 8B 0C B5 00 34 72 02                                mov     ecx, rgpszrawsentence[esi*4]
						rgpszrawsentence = (decltype(rgpszrawsentence))pinst->detail->x86.operands[1].mem.disp;
					}
				}


				if (cszrawsentences && rgpszrawsentence)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				return FALSE;

				}, 0, NULL);
		}

		Sig_VarNotFound(cszrawsentences);
		Sig_VarNotFound(rgpszrawsentence);
	}
}

void Client_FillAddress(void)
{
	if (!g_hClientDll)
		g_hClientDll = g_pMetaHookAPI->GetClientModule();

	if (!g_dwClientBase)
		g_dwClientBase = g_pMetaHookAPI->GetClientBase();

	if (!g_dwClientSize)
		g_dwClientSize = g_pMetaHookAPI->GetClientSize();

	ULONG ClientTextSize = 0;
	PVOID ClientTextBase = ClientTextBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".text\0\0\0", &ClientTextSize);
	
	if (!ClientTextBase)
	{
		Sys_Error("Failed to locate section \".text\" in client.dll!");
		return;
	}

	ULONG ClientDataSize = 0;
	auto ClientDataBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".data\0\0\0", &ClientDataSize);

	ULONG ClientRDataSize = 0;
	auto ClientRDataBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".rdata\0\0", &ClientRDataSize);

	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		g_bIsSvenCoop = true;

		if (1)
		{
			char pattern[] = "\x6A\x00\x50\x6A\xFF\x6A\x08\xE8\x2A\x2A\x2A\x2A\x2A\x2A\xE8";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);
			Sig_VarNotFound("ScClient_SoundEngine_PlayFMODSound");

			gPrivateFuncs.ScClient_SoundEngine_PlayFMODSound = (decltype(gPrivateFuncs.ScClient_SoundEngine_PlayFMODSound))GetCallAddress(addr + Sig_Length(pattern) - 1);
		}

		if (1)
		{
			char pattern[] = "\x8B\x54\x24\x04\x81\xFA\xFF\x0F\x00\x00\x2A\x2A\x83\x3C\x91\x00\x2A\x2A\x0F\xAE\xE8";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);
			Sig_VarNotFound("ScClient_SoundEngine_LookupSoundBySentenceIndex");

			gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySentenceIndex = (decltype(gPrivateFuncs.ScClient_SoundEngine_LookupSoundBySentenceIndex))addr;
		}

		if(1)
		{
			char pattern[] = "\x8B\x4C\x24\x04\x85\xC9\x2A\x2A\x6B\xC1\x58";
			gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);
			Sig_FuncNotFound(GetClientColor);
		}

		if(1)
		{
			char pattern[] = "\x8B\x0D\x2A\x2A\x2A\x2A\x85\xC9\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x84\xC0\x0F";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);

			Sig_AddrNotFound(GameViewport);

			GameViewport = *(decltype(GameViewport) *)(addr + 2);

			gPrivateFuncs.GameViewport_AllowedToPrintText = (decltype(gPrivateFuncs.GameViewport_AllowedToPrintText))GetCallAddress(addr + 10);
		}

		if(1)
		{
			char pattern[] = "\x8B\x01\x8B\x40\x28\xFF\xE0";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);

			Sig_AddrNotFound(GameViewport_IsScoreBoardVisible);

			gPrivateFuncs.GameViewport_IsScoreBoardVisible = (decltype(gPrivateFuncs.GameViewport_AllowedToPrintText))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(addr, 0x50, [](PUCHAR Candidate) {

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

			Sig_FuncNotFound(GameViewport_IsScoreBoardVisible);
		}

		if(1)
		{
			char pattern[] = "common/wpn_hudon.wav";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientRDataBase, ClientRDataSize, pattern);

			Sig_AddrNotFound(wpn_hudon_wav_String);

			char pattern2[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
			*(DWORD *)(pattern2 + 1) = (DWORD)addr;
			auto wpn_hudon_PushString = Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern2);
			Sig_VarNotFound(wpn_hudon_PushString);

			gPrivateFuncs.WeaponsResource_SelectSlot = (decltype(gPrivateFuncs.WeaponsResource_SelectSlot))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(wpn_hudon_PushString, 0x250, [](PUCHAR Candidate) {

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

			Sig_FuncNotFound(WeaponsResource_SelectSlot);
		}

		if(1)
		{
			char pattern[] = "\x8B\x40\x28\xFF\xD0\x84\xC0\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);

			Sig_AddrNotFound(g_iVisibleMouse);

			g_iVisibleMouse = *(decltype(g_iVisibleMouse) *)(addr + 11);
		}

		if(1)
		{
			char pattern[] = "\xF6\x05\x2A\x2A\x2A\x2A\x20\x2A\x2A\xB9\x2A\x2A\x2A\x2A\xE8";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);
			Sig_AddrNotFound(CHud_GetBorderSize);

			gHud = *(decltype(gHud) *)(addr + 10);
			gPrivateFuncs.CHud_GetBorderSize = (decltype(gPrivateFuncs.CHud_GetBorderSize)) GetCallAddress(addr + Sig_Length(pattern) - 1);
		}
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		if (1)
		{
			char pattern[] = "\x8B\x44\x24\x04\x83\xE8\x03\x2A\x2A\x48";
			gPrivateFuncs.GetTextColor = (decltype(gPrivateFuncs.GetTextColor))Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);
			Sig_FuncNotFound(GetTextColor);
		}

#define CS_CZ_GETCLIENTCOLOR_SIG 
#define CS_CZ_GETCLIENTCOLOR_SIG_HL25 
		if (1)
		{
			char pattern[] = "\x0F\xBF\x2A\x2A\x2A\x2A\x2A\x2A\x48\x83\xF8\x03\x77\x2A\xFF\x24";
			char pattern_HL25[] = "\x55\x8B\xEC\x6B\x45\x08\x74\x0F\xBF\x80\x2A\x2A\x2A\x2A\x48\x83\xF8\x03\x77\x23\xFF\x24\x85";
			if (g_iEngineType != ENGINE_GOLDSRC_HL25)
			{
				auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);

				if (addr)
				{
					gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(addr, 0x50, [](PUCHAR Candidate) {

						//8B 44 24 04                                         mov     eax, [esp+arg_0]
						if (Candidate[0] == 0x8B &&
							Candidate[1] == 0x44 &&
							Candidate[2] == 0x24)
						{
							return TRUE;
						}

						return FALSE;
					});

					Sig_FuncNotFound(GetClientColor);
				}
			}
			else
			{
				gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern_HL25);

				Sig_FuncNotFound(GetClientColor);
			}
		}
	}

	if (!g_iVisibleMouse &&
		(PUCHAR)gExportfuncs.IN_Accumulate > (PUCHAR)g_dwClientBase &&
		(PUCHAR)gExportfuncs.IN_Accumulate < (PUCHAR)g_dwClientBase + g_dwClientSize)
	{
		typedef struct
		{
			DWORD candidate;
			int candidate_register;
		}IN_Accumulate_ctx;

		IN_Accumulate_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gExportfuncs.IN_Accumulate, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto ctx = (IN_Accumulate_ctx *)context;
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwClientBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwClientBase + g_dwClientSize)
			{
				ctx->candidate = pinst->detail->x86.operands[1].mem.disp;
				ctx->candidate_register = pinst->detail->x86.operands[0].reg;
			}

			if (ctx->candidate_register &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->candidate_register &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->candidate_register)
			{
				g_iVisibleMouse = (decltype(g_iVisibleMouse))ctx->candidate;
			}

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwClientBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwClientBase + g_dwClientSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				g_iVisibleMouse = (decltype(g_iVisibleMouse))pinst->detail->x86.operands[0].mem.disp;
			}

			if (g_iVisibleMouse)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		//Sig_VarNotFound(g_iVisibleMouse);
	}
}

void Engine_InstallHooks(void)
{
	Install_InlineHook(S_StartDynamicSound);
	Install_InlineHook(S_StartStaticSound);
	Install_InlineHook(pfnTextMessageGet);
	Install_InlineHook(TextMessageParse);
	Install_InlineHook(COM_ExplainDisconnection);
}

void Engine_UninstallHooks(void)
{
	Uninstall_Hook(S_StartDynamicSound);
	Uninstall_Hook(S_StartStaticSound);
	Uninstall_Hook(pfnTextMessageGet);
	Uninstall_Hook(TextMessageParse);
	Uninstall_Hook(COM_ExplainDisconnection);
}

void Client_InstallHooks(void)
{
	if (gPrivateFuncs.ScClient_SoundEngine_PlayFMODSound)
	{
		Install_InlineHook(ScClient_SoundEngine_PlayFMODSound);
	}

	if (gPrivateFuncs.WeaponsResource_SelectSlot)
	{
		Install_InlineHook(WeaponsResource_SelectSlot);
	}
}

void Client_UninstallHooks(void)
{
	Uninstall_Hook(ScClient_SoundEngine_PlayFMODSound);
	Uninstall_Hook(WeaponsResource_SelectSlot);
}

void DllLoadNotification(mh_load_dll_notification_context_t* ctx)
{
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)
	{
		if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_CLIENT)
		{
			g_hClientDll = ctx->hModule;
			g_dwClientBase = ctx->ImageBase;
			g_dwClientSize = ctx->ImageSize;
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
			g_hClientDll = NULL;
			g_dwClientBase = 0;
			g_dwClientSize = 0;
		}
		else if (ctx->hModule == g_hFMODEx)
		{
			FMOD_UninstallHooks(ctx->hModule);
			g_hFMODEx = NULL;
		}
	}
}
