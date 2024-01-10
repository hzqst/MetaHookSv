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

char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
int *cszrawsentences = NULL;

int *cl_viewentity = NULL;

vec3_t *listener_origin = NULL;

char(*s_pBaseDir)[512] = NULL;

qboolean *scr_drawloading = NULL;

CreateInterfaceFn *g_pClientFactory = NULL;

char m_szCurrentLanguage[128] = { 0 };

private_funcs_t gPrivateFuncs = { 0 };

hook_t *g_phook_S_FindName = NULL;
hook_t *g_phook_S_StartDynamicSound = NULL;
hook_t *g_phook_S_StartStaticSound = NULL;
hook_t *g_phook_ScClient_FindSoundEx = NULL;
hook_t *g_phook_pfnTextMessageGet = NULL;
hook_t *g_phook_CWin32Font_GetCharRGBA = NULL;
hook_t *g_phook_WeaponsResource_SelectSlot = NULL;
hook_t *g_phook_FileSystem_SetGameDirectory = NULL;

PVOID VGUIClient001_CreateInterface(HINTERFACEMODULE hModule)
{
	if (hModule == (HINTERFACEMODULE)g_hClientDll && !g_IsClientVGUI2)
	{
		return NewCreateInterface;
	}

	return Sys_GetFactory(hModule);
}

bool SCR_IsLoadingVisible(void)
{
	return scr_drawloading && (*scr_drawloading) == 1 ? true : false;
}

void Engine_FillAddress(void)
{
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
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
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
		cl_viewentity = *(decltype(cl_viewentity) *)(addr + 10);
		Sig_AddrNotFound(cl_viewentity);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define CL_VIEWENTITY_SIG_HL25 "\xE8\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xC3"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_HL25);
		cl_viewentity = (decltype(cl_viewentity))(addr + 5);
		Sig_AddrNotFound(cl_viewentity);
	}
	else
	{
#define CL_VIEWENTITY_SIG_NEW "\x8B\x0D\x2A\x2A\x2A\x2A\x6A\x64\x6A\x00\x68\x00\x00\x80\x3F\x68\x00\x00\x80\x3F\x68\x2A\x2A\x2A\x2A\x50"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_NEW);
		cl_viewentity = *(decltype(cl_viewentity) *)(addr + 2);
		Sig_AddrNotFound(cl_viewentity);
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

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "User Token 2";
		auto UserToken2_String = Search_Pattern_Data(sigs1);
		if (!UserToken2_String)
			UserToken2_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(UserToken2_String);
		char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8D\x84\x24";
		*(DWORD *)(pattern + 2) = (DWORD)UserToken2_String;
		auto UserToken2_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(UserToken2_PushString);

		gPrivateFuncs.FileSystem_SetGameDirectory = (decltype(gPrivateFuncs.FileSystem_SetGameDirectory))
			g_pMetaHookAPI->ReverseSearchFunctionBeginEx(UserToken2_PushString, 0x100, [](PUCHAR Candidate) {

			//.text : 01D4DA50 81 EC 90 04 00 00                                            sub     esp, 490h
			//.text : 01D4DA56 A1 E8 F0 ED 01                                               mov     eax, ___security_cookie
			//.text : 01D4DA5B 33 C4                                                        xor     eax, esp
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[4] == 0x00 &&
				Candidate[5] == 0x00 &&
				Candidate[6] == 0xA1 &&
				Candidate[11] == 0x33 &&
				Candidate[12] == 0xC4)
				return TRUE;

			return FALSE;
		});
		Sig_FuncNotFound(FileSystem_SetGameDirectory);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs1[] = "User Token 2";
		auto UserToken2_String = Search_Pattern_Data(sigs1);
		if (!UserToken2_String)
			UserToken2_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(UserToken2_String);
		char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 2) = (DWORD)UserToken2_String;
		auto UserToken2_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(UserToken2_PushString);

		gPrivateFuncs.FileSystem_SetGameDirectory = (decltype(gPrivateFuncs.FileSystem_SetGameDirectory))
			g_pMetaHookAPI->ReverseSearchFunctionBeginEx(UserToken2_PushString, 0x100, [](PUCHAR Candidate) {

			//.text	: 101C8B30 55												push    ebp
			//.text : 101C8B31 8B EC											mov     ebp, esp
			//.text : 101C8B33 81 EC 10 04 00 00								sub     esp, 410h xor     eax, ebp
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x81 &&
				Candidate[4] == 0xEC &&
				Candidate[7] == 0x00 &&
				Candidate[8] == 0x00)
				return TRUE;

			return FALSE;
		});
		Sig_FuncNotFound(FileSystem_SetGameDirectory);
	}
	else
	{
		const char sigs1[] = "User Token 2";
		auto UserToken2_String = Search_Pattern_Data(sigs1);
		if (!UserToken2_String)
			UserToken2_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(UserToken2_String);
		char pattern[] = "\x51\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD *)(pattern + 2) = (DWORD)UserToken2_String;
		auto UserToken2_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(UserToken2_PushString);

		gPrivateFuncs.FileSystem_SetGameDirectory = (decltype(gPrivateFuncs.FileSystem_SetGameDirectory))
			g_pMetaHookAPI->ReverseSearchFunctionBeginEx(UserToken2_PushString, 0x100, [](PUCHAR Candidate) {

			//.text : 01D3B150 55                                                  push    ebp
			//.text : 01D3B151 8B EC                                               mov     ebp, esp
			//.text : 01D3B153 81 EC 08 04 00 00                                   sub     esp, 408h
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x81 &&
				Candidate[4] == 0xEC &&
				Candidate[7] == 0x00 &&
				Candidate[8] == 0x00)
				return TRUE;

			//.text:01D3BB30 81 EC 8C 04 00 00                                   sub     esp, 48Ch
			//.text : 01D3BB36 8B 0D C8 0A 08 02                                   mov     ecx, dword_2080AC8
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC)
				return TRUE;


			return FALSE;
		});
		Sig_FuncNotFound(FileSystem_SetGameDirectory);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "VClientVGUI001";
		auto VClientVGUI001_String = Search_Pattern_Data(sigs1);
		if (!VClientVGUI001_String)
			VClientVGUI001_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(VClientVGUI001_String);
		char pattern[] = "\x8B\x2A\x2A\x6A\x00\x68\x2A\x2A\x2A\x2A\x89";
		*(DWORD *)(pattern + 6) = (DWORD)VClientVGUI001_String;
		auto VClientVGUI001_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(VClientVGUI001_PushString);

		const char sigs2[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x85\xC0";
		auto Call_VClientVGUI001_CreateInterface = g_pMetaHookAPI->ReverseSearchPattern(VClientVGUI001_PushString, 0x50, sigs2, sizeof(sigs2) - 1);
		Sig_VarNotFound(Call_VClientVGUI001_CreateInterface);

		PUCHAR address = (PUCHAR)Call_VClientVGUI001_CreateInterface + 15;

		gPrivateFuncs.VGUIClient001_CreateInterface = (decltype(gPrivateFuncs.VGUIClient001_CreateInterface))GetCallAddress(address);

		PUCHAR pfnVGUIClient001_CreateInterface = (PUCHAR)VGUIClient001_CreateInterface;

		int rva = pfnVGUIClient001_CreateInterface - (address + 5);

		g_pMetaHookAPI->WriteMemory(address + 1, (BYTE *)&rva, 4);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs1[] = "VClientVGUI001";
		auto VClientVGUI001_String = Search_Pattern_Data(sigs1);
		if (!VClientVGUI001_String)
			VClientVGUI001_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(VClientVGUI001_String);
		char pattern[] = "\x8B\x4B\x1C\x6A\x00\x68\x2A\x2A\x2A\x2A\x89";
		*(DWORD *)(pattern + 6) = (DWORD)VClientVGUI001_String;
		auto VClientVGUI001_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(VClientVGUI001_PushString);

/*
		if ( !dword_1E67088 )
		__debugbreak();
		if ( ClientFactory )
		{
			factory = VGUIClient001_CreateInterface(hModule);
			if ( factory )
			{
				v4[v4[7]++ + 1] = factory;
				dword_1E66F4C = ((int (__cdecl *)(char *, _DWORD))factory)("VClientVGUI001", 0);
			}
		}
*/

		const char sigs2[] = "\xFF\x35\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x85\xC0\x74\x28";
		auto Call_VClientVGUI001_CreateInterface = g_pMetaHookAPI->ReverseSearchPattern(VClientVGUI001_PushString, 0x50, sigs2, sizeof(sigs2) - 1);
		Sig_VarNotFound(Call_VClientVGUI001_CreateInterface);

		PUCHAR address = (PUCHAR)Call_VClientVGUI001_CreateInterface + 6;

		gPrivateFuncs.VGUIClient001_CreateInterface = (decltype(gPrivateFuncs.VGUIClient001_CreateInterface))GetCallAddress(address);

		PUCHAR pfnVGUIClient001_CreateInterface = (PUCHAR)VGUIClient001_CreateInterface;

		int rva = pfnVGUIClient001_CreateInterface - (address + 5);

		g_pMetaHookAPI->WriteMemory(address + 1, (BYTE *)&rva, 4);
	}
	else
	{
		const char sigs1[] = "VClientVGUI001";
		auto VClientVGUI001_String = Search_Pattern_Data(sigs1);
		if (!VClientVGUI001_String)
			VClientVGUI001_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(VClientVGUI001_String);
		char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\x89";
		*(DWORD *)(pattern + 3) = (DWORD)VClientVGUI001_String;
		auto VClientVGUI001_PushString = Search_Pattern(pattern);
		if (!VClientVGUI001_PushString)
		{
			char pattern2[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\xFF";
			*(DWORD*)(pattern2 + 3) = (DWORD)VClientVGUI001_String;
			VClientVGUI001_PushString = Search_Pattern(pattern2);
		}
		Sig_VarNotFound(VClientVGUI001_PushString);

		/*
		if ( !dword_1E67088 )
		__debugbreak();
		if ( ClientFactory )
		{
			factory = VGUIClient001_CreateInterface(hModule);
			if ( factory )
			{
				v4[v4[7]++ + 1] = factory;
				dword_1E66F4C = ((int (__cdecl *)(char *, _DWORD))factory)("VClientVGUI001", 0);
			}
		}
		*/

		const char sigs2[] = "\xA1\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x85\xC0";
		auto Call_VClientVGUI001_CreateInterface = g_pMetaHookAPI->ReverseSearchPattern(VClientVGUI001_PushString, 0x50, sigs2, sizeof(sigs2) - 1);
		if (Call_VClientVGUI001_CreateInterface)
		{
			PUCHAR address = (PUCHAR)Call_VClientVGUI001_CreateInterface + 6;

			gPrivateFuncs.VGUIClient001_CreateInterface = (decltype(gPrivateFuncs.VGUIClient001_CreateInterface))GetCallAddress(address);

			PUCHAR pfnVGUIClient001_CreateInterface = (PUCHAR)VGUIClient001_CreateInterface;

			int rva = pfnVGUIClient001_CreateInterface - (address + 5);

			g_pMetaHookAPI->WriteMemory(address + 1, (BYTE*)&rva, 4);
		}
		else
		{
			/*
				if ( !dword_1EF4070 )
					__debugbreak();
				if ( ClientFactory )
				{
					v14 = (int (__cdecl *)(const char *, _DWORD))ClientFactory();
					if ( v14 )
					v3[v3[6]++ + 1] = v14;
					g_VClientVGUI = v14("VClientVGUI001", 0);
				}
			*/

			/*
.text:01D011E9 CC                                                  int     3               ; Trap to Debugger
.text:01D011EA
.text:01D011EA                                     loc_1D011EA:                            ; CODE XREF: sub_1D010D0+117↑j
.text:01D011EA A1 48 13 F7 02                                      mov     eax, ClientFactory
.text:01D011EF 85 C0                                               test    eax, eax
.text:01D011F1 74 27                                               jz      short loc_1D0121A
.text:01D011F3 FF D0                                               call    eax ; ClientFactory
.text:01D011F5 85 C0                                               test    eax, eax
			*/
#if 1
			const char sigs3[] = "\xCC\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A\xFF";
			auto pClientFactoryAddr = (PUCHAR)g_pMetaHookAPI->ReverseSearchPattern(VClientVGUI001_PushString, 0x50, sigs3, sizeof(sigs3) - 1);
			if (pClientFactoryAddr)
			{
				g_pClientFactory = *(decltype(g_pClientFactory)*)(pClientFactoryAddr + 2);
			}
#endif
		}
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "\x84\xC0\x75\x2A\x8B\x44\x24\x08\xC7\x05\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x85\xC0";
		auto basedir_pattern = Search_Pattern(sigs1);
		Sig_VarNotFound(basedir_pattern);

		s_pBaseDir = *(decltype(s_pBaseDir) *)((PUCHAR)basedir_pattern + 14);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs1[] = "\x8D\x85\x7C\xFF\xFF\xFF\x50\x68\x2A\x2A\x2A\x2A\x68";
		auto basedir_pattern = Search_Pattern(sigs1);
		Sig_VarNotFound(basedir_pattern);

		s_pBaseDir = *(decltype(s_pBaseDir) *)((PUCHAR)basedir_pattern + 7);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		const char sigs1[] = "\x84\xC9\x75\x2A\x8B\x45\x0C\xC7\x05\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x50\xE8";
		auto basedir_pattern = Search_Pattern(sigs1);
		Sig_VarNotFound(basedir_pattern);

		s_pBaseDir = *(decltype(s_pBaseDir) *)((PUCHAR)basedir_pattern + 13);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		const char sigs1[] = "\xC7\x05\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x84\xC0";
		auto basedir_pattern = Search_Pattern(sigs1);
		Sig_VarNotFound(basedir_pattern);

		s_pBaseDir = *(decltype(s_pBaseDir) *)((PUCHAR)basedir_pattern + 6);
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

	if (1)
	{
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchEnd = SearchBegin + g_dwEngineTextSize;
		while (1)
		{
#define LANGUAGESTRNCPY_SIG "\x68\x80\x00\x00\x00\x50\x8D"
			PUCHAR LanguageStrncpy = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchEnd - SearchBegin, LANGUAGESTRNCPY_SIG);
			if (LanguageStrncpy)
			{
				typedef struct
				{
					bool bHasPushEax;
				}LanguageStrncpy_ctx;

				LanguageStrncpy_ctx ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(LanguageStrncpy, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto ctx = (LanguageStrncpy_ctx *)context;
					auto pinst = (cs_insn *)inst;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[0].reg == X86_REG_EAX)
					{
						ctx->bHasPushEax = true;
					}

					if (ctx->bHasPushEax)
					{
						if (address[0] == 0xE8)
						{
							gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(address);
							PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
							int rva = pfnNewV_strncpy - (address + 5);
							g_pMetaHookAPI->WriteMemory(address + 1, (BYTE *)&rva, 4);
							return TRUE;
						}
						else if (address[0] == 0xEB)
						{
							char jmprva = *(char *)(address + 1);
							PUCHAR jmptarget = address + 2 + jmprva;

							if (jmptarget[0] == 0xE8)
							{
								gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(jmptarget);
								PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
								int rva = pfnNewV_strncpy - (jmptarget + 5);
								g_pMetaHookAPI->WriteMemory(jmptarget + 1, (BYTE *)&rva, 4);
								return TRUE;
							}
						}
					}

					if (instCount > 5)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, &ctx);

				SearchBegin = LanguageStrncpy + sizeof(LANGUAGESTRNCPY_SIG) - 1;
			}
			else
			{
				break;
			}
		}
	}

	//TODO: Remove this? Looks like not used anymore?
#if 0
	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CWIN32FONT_GETCHARABCWIDTHS_SIG_SVENGINE "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x45\x0C\x2A\x2A\x5D\x14"

		gPrivateFuncs.CWin32Font_GetCharRGBA = (decltype(gPrivateFuncs.CWin32Font_GetCharRGBA))Search_Pattern(CWIN32FONT_GETCHARABCWIDTHS_SIG_SVENGINE);
		Sig_FuncNotFound(CWin32Font_GetCharRGBA);
	}
	else
	{
#define CWIN32FONT_GETCHARABCWIDTHS_SIG "\x55\x8B\xEC\x83\xEC\x70\x53\x56\x8B\xF1\x8D\x45\xD0\x57\x8D\x4D\xE4\x50\x8B\x45\x08\x8D\x55\xD4\x51\x52\x50\x8B\xCE"

		gPrivateFuncs.CWin32Font_GetCharRGBA = (decltype(gPrivateFuncs.CWin32Font_GetCharRGBA))Search_Pattern(CWIN32FONT_GETCHARABCWIDTHS_SIG);
		Sig_FuncNotFound(CWin32Font_GetCharRGBA);
	}
#endif


}

void Engine_InstallHooks(void)
{
	Install_InlineHook(S_StartDynamicSound);
	Install_InlineHook(S_StartStaticSound);
	Install_InlineHook(pfnTextMessageGet);
	Install_InlineHook(FileSystem_SetGameDirectory);
}

void Engine_UninstallHooks(void)
{
	Uninstall_Hook(S_StartDynamicSound);
	Uninstall_Hook(S_StartStaticSound);
	Uninstall_Hook(pfnTextMessageGet);
	Uninstall_Hook(FileSystem_SetGameDirectory);
}

void Client_FillAddress(void)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		g_bIsSvenCoop = true;

#define SC_FINDSOUND_SIG "\x51\x55\x8B\x6C\x24\x0C\x89\x4C\x24\x04\x85\xED\x0F\x84\x2A\x2A\x2A\x2A\x80\x7D\x00\x00"
		{
			gPrivateFuncs.ScClient_FindSoundEx = (decltype(gPrivateFuncs.ScClient_FindSoundEx))
				g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_FINDSOUND_SIG, Sig_Length(SC_FINDSOUND_SIG));

			Sig_FuncNotFound(ScClient_FindSoundEx);
		}

#define SC_GETCLIENTCOLOR_SIG "\x8B\x4C\x24\x04\x85\xC9\x2A\x2A\x6B\xC1\x58"
		{
			gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))
				g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_GETCLIENTCOLOR_SIG, Sig_Length(SC_GETCLIENTCOLOR_SIG));

			Sig_FuncNotFound(GetClientColor);
		}

#define SC_VIEWPORT_SIG "\x8B\x0D\x2A\x2A\x2A\x2A\x85\xC9\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x84\xC0\x0F"
		{
			auto addr = (PUCHAR)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_VIEWPORT_SIG, Sig_Length(SC_VIEWPORT_SIG));

			Sig_AddrNotFound(GameViewport);

			GameViewport = *(decltype(GameViewport) *)(addr + 2);

			gPrivateFuncs.GameViewport_AllowedToPrintText = (decltype(gPrivateFuncs.GameViewport_AllowedToPrintText))GetCallAddress(addr + 10);
		}

#define SC_VIEWPORT_ISSCOREBOARDVISIBLE_SIG "\x8B\x01\x8B\x40\x28\xFF\xE0"
		{
			auto addr = (PUCHAR)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_VIEWPORT_ISSCOREBOARDVISIBLE_SIG, Sig_Length(SC_VIEWPORT_ISSCOREBOARDVISIBLE_SIG));

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

#define SELECTSLOT_STRING_SIG "common/wpn_hudon.wav"
		{
			auto addr = (PUCHAR)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SELECTSLOT_STRING_SIG, Sig_Length(SELECTSLOT_STRING_SIG));

			Sig_AddrNotFound(wpn_hudon_wav_String);

			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
			*(DWORD *)(pattern + 1) = (DWORD)addr;
			auto Push_wpn_hudon_Call = g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, pattern, Sig_Length(pattern));
			Sig_VarNotFound(Push_wpn_hudon_Call);

			gPrivateFuncs.WeaponsResource_SelectSlot = (decltype(gPrivateFuncs.WeaponsResource_SelectSlot))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Push_wpn_hudon_Call, 0x250, [](PUCHAR Candidate) {

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

#define SC_UPDATECURSORSTATE_SIG "\x8B\x40\x28\xFF\xD0\x84\xC0\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00"
		{
			auto addr = (PUCHAR)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_UPDATECURSORSTATE_SIG, Sig_Length(SC_UPDATECURSORSTATE_SIG));

			Sig_AddrNotFound(g_iVisibleMouse);

			g_iVisibleMouse = *(decltype(g_iVisibleMouse) *)(addr + 11);
		}

#define SC_GETBORDERSIZE_SIG "\xF6\x05\x2A\x2A\x2A\x2A\x20\x2A\x2A\xB9\x2A\x2A\x2A\x2A\xE8"
		{
			auto addr = (PUCHAR)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_GETBORDERSIZE_SIG, Sig_Length(SC_GETBORDERSIZE_SIG));
			Sig_AddrNotFound(CHud_GetBorderSize);

			gHud = *(decltype(gHud) *)(addr + 10);
			gPrivateFuncs.CHud_GetBorderSize = (decltype(gPrivateFuncs.CHud_GetBorderSize))
				g_pMetaHookAPI->GetNextCallAddr(addr + Sig_Length(SC_GETBORDERSIZE_SIG) - 1, 1);
		}
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		if (1)
		{
#define CS_CZ_GETTEXTCOLOR_SIG "\x8B\x44\x24\x04\x83\xE8\x03\x2A\x2A\x48"
			gPrivateFuncs.GetTextColor = (decltype(gPrivateFuncs.GetTextColor))
				g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, CS_CZ_GETTEXTCOLOR_SIG, Sig_Length(CS_CZ_GETTEXTCOLOR_SIG));
			//Sig_FuncNotFound(GetTextColor);
		}

#define CS_CZ_GETCLIENTCOLOR_SIG "\x0F\xBF\x2A\x2A\x2A\x2A\x2A\x2A\x48\x83\xF8\x03\x77\x2A\xFF\x24"
#define CS_CZ_GETCLIENTCOLOR_SIG_HL25 "\x55\x8B\xEC\x6B\x45\x08\x74\x0F\xBF\x80\x2A\x2A\x2A\x2A\x48\x83\xF8\x03\x77\x23\xFF\x24\x85"
		if (1)
		{
			if (g_iEngineType != ENGINE_GOLDSRC_HL25)
			{
				DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, CS_CZ_GETCLIENTCOLOR_SIG, Sig_Length(CS_CZ_GETCLIENTCOLOR_SIG));

				if (addr)
				{
					gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))g_pMetaHookAPI->ReverseSearchFunctionBeginEx((PVOID)addr, 0x50, [](PUCHAR Candidate) {

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
				gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))g_pMetaHookAPI->SearchPattern(
					g_dwClientBase,
					g_dwClientSize,
					CS_CZ_GETCLIENTCOLOR_SIG_HL25,
					Sig_Length(CS_CZ_GETCLIENTCOLOR_SIG_HL25)
				);

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

void Client_InstallHooks(void)
{
	if (gPrivateFuncs.ScClient_FindSoundEx)
	{
		Install_InlineHook(ScClient_FindSoundEx);
	}

	if (gPrivateFuncs.WeaponsResource_SelectSlot)
	{
		Install_InlineHook(WeaponsResource_SelectSlot);
	}
}

void Client_UninstallHooks(void)
{
	Uninstall_Hook(ScClient_FindSoundEx);
	Uninstall_Hook(WeaponsResource_SelectSlot);
}

cl_entity_t *EngineGetViewEntity(void)
{
	return gEngfuncs.GetEntityByIndex((*cl_viewentity));
}