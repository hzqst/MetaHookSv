#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "privatefuncs.h"

//Legacy signature for the one engine identity whose VOX sentence locator is not
//published (svencoop-10257, the only non-isBlob SvEngine); used only as an
//isolated fallback. In every other identity VOX_LookupString comes from gamedata.
#define VOX_LOOKUPSTRING_SIG "\x80\x2A\x23\x2A\x2A\x8D\x2A\x01\x50\xE8"

double *cl_time = NULL;
double *cl_oldtime = NULL;
double *realtime = NULL;

int* cl_viewentity = NULL;

char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
int *cszrawsentences = NULL;

vec3_t *listener_origin = NULL;

qboolean *scr_drawloading = NULL;

CreateInterfaceFn *g_pClientFactory = NULL;

private_funcs_t gPrivateFuncs = { 0 };

static hook_t *g_phook_S_StartDynamicSound = NULL;
static hook_t *g_phook_S_StartStaticSound = NULL;
static hook_t *g_phook_pfnTextMessageGet = NULL;
static hook_t* g_phook_pfnServerCmdUnreliable = NULL;
static hook_t *g_phook_TextMessageParse = NULL;
static hook_t* g_phook_COM_ExplainDisconnection = NULL;
static hook_t *g_phook_WeaponsResource_SelectSlot = NULL;
static hook_t* g_phook_SCClient_SoundEngine_LoadSoundList = NULL;
static hook_t *g_phook_SCClient_SoundEngine_PlayFMODSound = NULL;
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
	cl_time = (double*)GamedataResolvePtr(RealDllInfo.ImageBase, "cl_time", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_oldtime = (double*)GamedataResolvePtr(RealDllInfo.ImageBase, "cl_oldtime", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_RealTime(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		double *realtime = NULL;
	*/

	//`realtime` has no catalog record on any identity, so this stays a signature scan.
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

void Engine_FillAddress_S_FindName(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_FindName = (decltype(gPrivateFuncs.S_FindName))GamedataResolvePtr(RealDllInfo.ImageBase, "S_FindName", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_S_StartDynamicSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_StartDynamicSound = (decltype(gPrivateFuncs.S_StartDynamicSound))GamedataResolvePtr(RealDllInfo.ImageBase, "S_StartDynamicSound", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_S_StartStaticSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_StartStaticSound = (decltype(gPrivateFuncs.S_StartStaticSound))GamedataResolvePtr(RealDllInfo.ImageBase, "S_StartStaticSound", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_S_LoadSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))GamedataResolvePtr(RealDllInfo.ImageBase, "S_LoadSound", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_TextMessageParse(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.TextMessageParse = (decltype(gPrivateFuncs.TextMessageParse))GamedataResolvePtr(RealDllInfo.ImageBase, "TextMessageParse", MH_GAMESYMBOL_KIND_FUNCTION);
}
// End of Selection


void Engine_FillAddress_COM_ExplainDisconnection(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.COM_ExplainDisconnection = (decltype(gPrivateFuncs.COM_ExplainDisconnection))GamedataResolvePtr(RealDllInfo.ImageBase, "COM_ExplainDisconnection", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.COM_ExtendedExplainDisconnection = (decltype(gPrivateFuncs.COM_ExtendedExplainDisconnection))GamedataResolvePtr(RealDllInfo.ImageBase, "COM_ExtendedExplainDisconnection", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_SequenceGetSentenceByIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.SequenceGetSentenceByIndex = (decltype(gPrivateFuncs.SequenceGetSentenceByIndex))GamedataResolvePtr(RealDllInfo.ImageBase, "SequenceGetSentenceByIndex", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_SCR_BeginLoadingPlaque(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	scr_drawloading = (decltype(scr_drawloading))GamedataResolvePtr(RealDllInfo.ImageBase, "scr_drawloading", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_viewentity = NULL;
	*/

	cl_viewentity = (decltype(cl_viewentity))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_viewentity", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_ListenerOrigin(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	listener_origin = (decltype(listener_origin))GamedataResolvePtr(RealDllInfo.ImageBase, "listener_origin", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_VOX_LookupString(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*

		char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
		int *cszrawsentences = NULL;
	*/
	//The sentence counters are catalog-covered on every engine identity except
	//svencoop-10257 (the only non-isBlob SvEngine). Resolve both from gamedata
	//when published; otherwise fall back to the legacy disassembly locator.
	cszrawsentences = (decltype(cszrawsentences))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "cszrawsentences", MH_GAMESYMBOL_KIND_GLOBAL);
	rgpszrawsentence = (decltype(rgpszrawsentence))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "rgpszrawsentence", MH_GAMESYMBOL_KIND_GLOBAL);

	if (cszrawsentences && rgpszrawsentence)
		return;

	//The legacy locator only decoded the SvEngine code shape; on a GoldSrc /
	//HL25 identity with no published record the symbol is genuinely missing.
	if (g_iEngineType != ENGINE_SVENGINE)
	{
		Sig_VarNotFound(cszrawsentences);
		Sig_VarNotFound(rgpszrawsentence);
	}

	typedef struct VOX_LookupString_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} VOX_LookupString_SearchContext;

	VOX_LookupString_SearchContext ctx = { DllInfo, RealDllInfo };

	PVOID addr = Search_Pattern(VOX_LOOKUPSTRING_SIG, DllInfo);

	if (addr)
	{
		g_pMetaHookAPI->DisasmRanges(addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (VOX_LookupString_SearchContext*)context;

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

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Engine_FillAddress_GetClientTime(DllInfo, RealDllInfo);
	Engine_FillAddress_RealTime(DllInfo, RealDllInfo);
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

		gPrivateFuncs.SCClient_soundengine = (decltype(gPrivateFuncs.SCClient_soundengine))ConvertDllInfoSpace(soundengine_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(SCClient_soundengine);
}

void Client_FillAddress_SCClient_SoundEngine_maxsentences(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	//`CClient_SoundEngine.m_iSentenceCount` is the sentence-handle count at
	//object offset; the old locator re-derived exactly this offset by
	//disassembling the `Sentence length too long` write-back in LoadSoundList.
	gPrivateFuncs.SCClient_soundengine_maxsentences = GamedataQueryStructMember(RealDllInfo.ImageBase, "CClient_SoundEngine.m_iSentenceCount");
}

void Client_FillAddress_SCClient_SoundEngine_LoadSoundList(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.SCClient_SoundEngine_LoadSoundList = (decltype(gPrivateFuncs.SCClient_SoundEngine_LoadSoundList))GamedataResolvePtr(RealDllInfo.ImageBase, "CClient_SoundEngine_LoadSoundList", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_SCClient_SoundEngine_PlayFMODSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (1)
	{
		char pattern[] = "\x6A\x00\x50\x6A\xFF\x6A\x08\xE8";
		auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);

		Sig_AddrNotFound("SCClient_SoundEngine_PlayFMODSound");

		typedef struct SCClient_SoundEngine_PlayFMODSoundContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			bool bFoundPush11B0D4{};
		}SCClient_SoundEngine_PlayFMODSoundContext;

		SCClient_SoundEngine_PlayFMODSoundContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(addr + Sig_Length(pattern) - 1, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (SCClient_SoundEngine_PlayFMODSoundContext*)context;

			if (address[0] == 0xE8)
			{
				auto callTarget = GetCallAddress(address);

				typedef struct SCClient_SoundEngine_PlayFMODSoundContext2_s
				{
					bool bFoundPush11B0D4{};
				}SCClient_SoundEngine_PlayFMODSoundContext2;

				SCClient_SoundEngine_PlayFMODSoundContext2 ctx2 = { 0 };

				g_pMetaHookAPI->DisasmRanges(callTarget, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto ctx2 = (SCClient_SoundEngine_PlayFMODSoundContext*)context;
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

				gPrivateFuncs.SCClient_SoundEngine_PlayFMODSound = (decltype(gPrivateFuncs.SCClient_SoundEngine_PlayFMODSound))ConvertDllInfoSpace(callTarget, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

		Sig_FuncNotFound(SCClient_SoundEngine_PlayFMODSound);
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

		gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySentenceIndex = (decltype(gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySentenceIndex))ConvertDllInfoSpace(LookupSoundBySentenceIndex_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(SCClient_SoundEngine_LookupSoundBySentenceIndex);
	}

	//char pattern[] = "\x8B\x54\x24\x04\x81\xFA\xFF\x0F\x00\x00\x2A\x2A\x83\x3C\x91\x00\x2A\x2A\x0F\xAE\xE8";
	//auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
	//Sig_AddrNotFound("SCClient_SoundEngine_LookupSoundBySentenceIndex");

	//gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySentenceIndex = (decltype(gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySentenceIndex))ConvertDllInfoSpace(addr, DllInfo, RealDllInfo);
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

		gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySample = (decltype(gPrivateFuncs.SCClient_SoundEngine_LookupSoundBySample))ConvertDllInfoSpace(LookupSoundBySample_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(SCClient_SoundEngine_LookupSoundBySample);
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
	if (gPrivateFuncs.SCClient_SoundEngine_PlayFMODSound)
	{
		Install_InlineHook(SCClient_SoundEngine_PlayFMODSound);
	}

	if (gPrivateFuncs.SCClient_SoundEngine_LoadSoundList)
	{
		Install_InlineHook(SCClient_SoundEngine_LoadSoundList);
	}

	if (gPrivateFuncs.WeaponsResource_SelectSlot)
	{
		Install_InlineHook(WeaponsResource_SelectSlot);
	}
}

void Client_UninstallHooks(void)
{
	Uninstall_Hook(SCClient_SoundEngine_PlayFMODSound);
	Uninstall_Hook(SCClient_SoundEngine_LoadSoundList);
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