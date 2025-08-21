#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "privatefuncs.h"

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

char m_szCurrentGameLanguage[128] = { 0 };

private_funcs_t gPrivateFuncs = { 0 };

HMODULE g_hGameUI = NULL;
HMODULE g_hServerBrowser = NULL;
bool g_bIsServerBrowserHooked = false;

mh_dll_info_t g_GameUIDllInfo = { 0 };
mh_dll_info_t g_ServerBrowserDllInfo = { 0 };

const char* GetCurrentGameLanguage()
{
	return m_szCurrentGameLanguage;
}

bool SCR_IsLoadingVisible(void)
{
	return scr_drawloading && (*scr_drawloading) == 1 ? true : false;
}

void SDL2_FillAddress(void)
{
	auto SDL2 = GetModuleHandleA("sdl2.dll");

	if (SDL2)
	{
		gPrivateFuncs.SDL_GetWindowPosition = (decltype(gPrivateFuncs.SDL_GetWindowPosition))GetProcAddress(SDL2, "SDL_GetWindowPosition");
		gPrivateFuncs.SDL_GetWindowSize = (decltype(gPrivateFuncs.SDL_GetWindowSize))GetProcAddress(SDL2, "SDL_GetWindowSize");
		gPrivateFuncs.SDL_GetDisplayDPI = (decltype(gPrivateFuncs.SDL_GetDisplayDPI))GetProcAddress(SDL2, "SDL_GetDisplayDPI");
		gPrivateFuncs.SDL_GetWindowFromID = (decltype(gPrivateFuncs.SDL_GetWindowFromID))GetProcAddress(SDL2, "SDL_GetWindowFromID");
		gPrivateFuncs.SDL_GetWindowWMInfo = (decltype(gPrivateFuncs.SDL_GetWindowWMInfo))GetProcAddress(SDL2, "SDL_GetWindowWMInfo");
		gPrivateFuncs.SDL_GL_GetCurrentWindow = (decltype(gPrivateFuncs.SDL_GL_GetCurrentWindow))GetProcAddress(SDL2, "SDL_GL_GetCurrentWindow");
	}
}

bool VGUI2_IsPanelInit(PVOID Candidate)
{
	typedef struct
	{
		bool bFoundMov2;//C7 46 24 02 00 00 00                                mov     dword ptr [esi+24h], 2
	}VGUI2_IsPanelInit_SearchContext;

	VGUI2_IsPanelInit_SearchContext ctx = { 0 };

	g_pMetaHookAPI->DisasmRanges(Candidate, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (VGUI2_IsPanelInit_SearchContext*)context;

		if (!ctx->bFoundMov2 &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.disp == 0x24 &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 2)
		{
			ctx->bFoundMov2 = true;
			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	return ctx.bFoundMov2;
}

PVOID VGUI2_FindPanelInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID Panel_Init = NULL;

	if (1)
	{
		const char sigs[] = "\x6A\x18\x6A\x40\x6A\x00\x6A\x00";
		auto Panel_Init_Push = (PUCHAR)Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, sigs);
		if (Panel_Init_Push)
		{
			typedef struct VGUI2_FindPanelInit_SearchContext_s
			{
				PVOID& Panel_Init;
			}VGUI2_FindPanelInit_SearchContext;

			VGUI2_FindPanelInit_SearchContext ctx = { Panel_Init };

			g_pMetaHookAPI->DisasmRanges(Panel_Init_Push + Sig_Length(sigs), 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (VGUI2_FindPanelInit_SearchContext*)context;

				if (address[0] == 0xE8 && instCount <= 15)
				{
					auto Candidate = GetCallAddress(address);

					if (VGUI2_IsPanelInit(Candidate))
					{
						ctx->Panel_Init = Candidate;
					}

					return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

			}, 0, &ctx);
		}
	}

	if (!Panel_Init)
	{
		//  mov     dword ptr [ebx+24h], 2
		/* 8684 engine
.text:01DDB7ED 6A 18                                               push    18h
.text:01DDB7EF C6 47 38 00                                         mov     byte ptr [edi+38h], 0
.text:01DDB7F3 C6 47 39 00                                         mov     byte ptr [edi+39h], 0
.text:01DDB7F7 C6 47 3A 00                                         mov     byte ptr [edi+3Ah], 0
.text:01DDB7FB C6 47 3B 00                                         mov     byte ptr [edi+3Bh], 0
.text:01DDB7FF 6A 40                                               push    40h ; '@'
.text:01DDB801 C6 47 3C 00                                         mov     byte ptr [edi+3Ch], 0
.text:01DDB805 C6 47 3D 00                                         mov     byte ptr [edi+3Dh], 0
.text:01DDB809 C6 47 3E 00                                         mov     byte ptr [edi+3Eh], 0
.text:01DDB80D C6 47 3F 00                                         mov     byte ptr [edi+3Fh], 0
.text:01DDB811 56                                                  push    esi
.text:01DDB812 89 77 50                                            mov     [edi+50h], esi
.text:01DDB815 89 77 54                                            mov     [edi+54h], esi
.text:01DDB818 89 77 58                                            mov     [edi+58h], esi
.text:01DDB81B 56                                                  push    esi
.text:01DDB81C 8B CF                                               mov     ecx, edi
.text:01DDB81E C7 07 FC 0C E2 01                                   mov     dword ptr [edi], offset off_1E20CFC
.text:01DDB824 E8 17 0A 00 00                                      call    sub_1DDC240
		*/
		const char sigs2[] = "\x6A\x18\xC6";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, sigs2);
			if (pFound)
			{
				typedef struct VGUI2_FindPanelInit_SearchContext_s
				{
					PVOID& Panel_Init;
					int instCount_push40h{};
					int reg_pushReg{};
					int instCount_pushReg{};
					int instCount_pushReg2{};
				}VGUI2_FindPanelInit_SearchContext;

				VGUI2_FindPanelInit_SearchContext ctx = { Panel_Init };

				g_pMetaHookAPI->DisasmRanges(pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (VGUI2_FindPanelInit_SearchContext*)context;

					if (!ctx->instCount_push40h &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x40)
					{
						ctx->instCount_push40h = instCount;
					}

					if (!ctx->instCount_pushReg &&
						ctx->instCount_push40h &&
						instCount > ctx->instCount_push40h &&
						instCount < ctx->instCount_push40h + 10 &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						ctx->instCount_pushReg = instCount;
						ctx->reg_pushReg = pinst->detail->x86.operands[0].reg;
					}

					if (!ctx->instCount_pushReg2 &&
						ctx->instCount_pushReg &&
						instCount > ctx->instCount_pushReg &&
						instCount < ctx->instCount_pushReg + 8 &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						ctx->reg_pushReg == pinst->detail->x86.operands[0].reg)
					{
						ctx->instCount_pushReg2 = instCount;
					}

					if (address[0] == 0xE8)
					{
						if (ctx->instCount_pushReg2 &&
							instCount > ctx->instCount_pushReg2 &&
							instCount < ctx->instCount_pushReg2 + 6)
						{
							auto Candidate = GetCallAddress(address);

							if (VGUI2_IsPanelInit(Candidate))
							{
								ctx->Panel_Init = Candidate;
							}
						}
						return TRUE;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

				}, 0, &ctx);

				if (Panel_Init)
				{
					break;
				}

				SearchBegin = pFound + Sig_Length(sigs2);
			}
			else
			{
				break;
			}
		}
	}

	if (!Panel_Init)
	{
		//  mov     dword ptr [ebx+24h], 2
		/* 8684 serverbrowser.dll
.text:100203F1 6A 18                                               push    18h
.text:100203F3 6A 40                                               push    40h ; '@'
.text:100203F5 53                                                  push    ebx
.text:100203F6 53                                                  push    ebx
		*/
		const char sigs3[] = "\x6A\x18\x6A\x40";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, sigs3);
			if (pFound)
			{
				typedef struct VGUI2_FindPanelInit_SearchContext_s
				{
					PVOID& Panel_Init;
					int instCount_push40h{};
					int reg_pushReg{};
					int instCount_pushReg{};
					int instCount_pushReg2{};
				}VGUI2_FindPanelInit_SearchContext;

				VGUI2_FindPanelInit_SearchContext ctx = { Panel_Init };

				g_pMetaHookAPI->DisasmRanges(pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (VGUI2_FindPanelInit_SearchContext*)context;

					if (!ctx->instCount_push40h &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x40)
					{
						ctx->instCount_push40h = instCount;
					}

					if (!ctx->instCount_pushReg &&
						ctx->instCount_push40h &&
						instCount > ctx->instCount_push40h &&
						instCount < ctx->instCount_push40h + 10 &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						ctx->instCount_pushReg = instCount;
						ctx->reg_pushReg = pinst->detail->x86.operands[0].reg;
					}

					if (!ctx->instCount_pushReg2 &&
						ctx->instCount_pushReg &&
						instCount > ctx->instCount_pushReg &&
						instCount < ctx->instCount_pushReg + 8 &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						ctx->reg_pushReg == pinst->detail->x86.operands[0].reg)
					{
						ctx->instCount_pushReg2 = instCount;
					}

					if (address[0] == 0xE8)
					{
						if (ctx->instCount_pushReg2 &&
							instCount > ctx->instCount_pushReg2 &&
							instCount < ctx->instCount_pushReg2 + 6)
						{
							auto Candidate = GetCallAddress(address);

							if (VGUI2_IsPanelInit(Candidate))
							{
								ctx->Panel_Init = Candidate;
							}
						}
						return TRUE;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

				}, 0, &ctx);

				if (Panel_Init)
				{
					break;
				}

				SearchBegin = pFound + Sig_Length(sigs3);
			}
			else
			{
				break;
			}
		}
	}

	return ConvertDllInfoSpace(Panel_Init, DllInfo, RealDllInfo);
}

PVOID *VGUI2_FindMenuVFTable(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "MenuScrollBar";
	auto MenuScrollBar_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs);
	if (!MenuScrollBar_String)
		MenuScrollBar_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs);

	if (!MenuScrollBar_String)
		return NULL;

	char pattern[] = "\x6A\x01\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 3) = (DWORD)MenuScrollBar_String;
	auto MenuScrollBar_PushString = Search_Pattern(pattern, DllInfo);

	if (!MenuScrollBar_PushString)
		return NULL;

	typedef struct Menu_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;

		PVOID Menu_ctor{};
		PVOID* Menu_vftable{};

	}Menu_SearchContext;

	Menu_SearchContext ctx = { DllInfo };

	ctx.Menu_ctor = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(MenuScrollBar_PushString, 0x500, [](PUCHAR Candidate) {

		if (Candidate[0] == 0x55 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xEC)
			return TRUE;

		//.text:10027EC0 53                                                  push    ebx
		//.text : 10027EC1 8B DC                                               mov     ebx, esp
		if (Candidate[0] == 0x53 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xDC)
			return TRUE;

		//.text:1006A220 8B 44 24 08                                         mov     eax, [esp+arg_4]
		//.text:1006A224 83 EC 08                                            sub     esp, 8
		if (Candidate[0] == 0x8B &&
			Candidate[1] == 0x44 &&
			Candidate[2] == 0x24 &&
			Candidate[4] == 0x83 &&
			Candidate[5] == 0xEC)
		{
			return TRUE;
		}

		return FALSE;
	});

	if (!ctx.Menu_ctor)
		return NULL;

	g_pMetaHookAPI->DisasmRanges(ctx.Menu_ctor, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (Menu_SearchContext*)context;

		if (!ctx->Menu_vftable)
		{
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.disp == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize))
			{
				auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;

				if (candidate[0] >= (PUCHAR)ctx->DllInfo.TextBase && candidate[0] < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
				{
					ctx->Menu_vftable = candidate;
				}
			}
		}

		if(ctx->Menu_vftable)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, & ctx);

	return (PVOID*)ConvertDllInfoSpace((PVOID)ctx.Menu_vftable, DllInfo, RealDllInfo);
}

PVOID *VGUI2_FindKeyValueVFTable(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "CursorEnteredMenuButton\0";
	auto CursorEnteredMenuButton_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs);
	if (!CursorEnteredMenuButton_String)
		CursorEnteredMenuButton_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs);

	if (!CursorEnteredMenuButton_String)
		return NULL;

	char pattern[] = "\x74\x2A\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 3) = (DWORD)CursorEnteredMenuButton_String;
	auto CursorEnteredMenuButton_PushString = Search_Pattern(pattern, DllInfo);

	if (!CursorEnteredMenuButton_PushString)
		return NULL;

	typedef struct KeyValues_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;

		PVOID KeyValues_ctor{};
		PVOID* KeyValues_vftable{};

	}KeyValues_SearchContext;

	KeyValues_SearchContext ctx = { DllInfo };

	g_pMetaHookAPI->DisasmRanges(CursorEnteredMenuButton_PushString, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (KeyValues_SearchContext*)context;

		if (address[0] == 0xE8 && instCount <= 5)
		{
			ctx->KeyValues_ctor = (decltype(ctx->KeyValues_ctor))GetCallAddress(address);

			g_pMetaHookAPI->DisasmRanges(ctx->KeyValues_ctor, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (KeyValues_SearchContext*)context;

				if (!ctx->KeyValues_vftable)
				{
					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
							(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize))
					{
						auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;

						if (candidate[0] >= (PUCHAR)ctx->DllInfo.TextBase && candidate[0] < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
						{
							ctx->KeyValues_vftable = candidate;
						}
					}
				}

				if (ctx->KeyValues_vftable)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

				}, 0, ctx);

			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	return (PVOID *)ConvertDllInfoSpace((PVOID)ctx.KeyValues_vftable, DllInfo, RealDllInfo);
}

void Engine_FillAddress_PanelInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.EngineVGUI2_Panel_Init = (decltype(gPrivateFuncs.EngineVGUI2_Panel_Init))VGUI2_FindPanelInit(DllInfo, RealDllInfo);
	Sig_FuncNotFound(EngineVGUI2_Panel_Init);
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

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_viewentity = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
		auto addr = (PUCHAR)Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_SVENGINE);
		Sig_AddrNotFound(cl_viewentity);
		PVOID cl_viewentity_VA = *(PVOID*)(addr + 10);
		cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
		auto addr = (PUCHAR)Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_GOLDSRC);
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

void Engine_FillAddress_Sys_InitializeGameDLL(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "Sys_InitializeGameDLL called twice";
	auto Sys_InitializeGameDLL_String = Search_Pattern_Data(sigs, DllInfo);
	if (!Sys_InitializeGameDLL_String)
		Sys_InitializeGameDLL_String = Search_Pattern_Rdata(sigs, DllInfo);
	Sig_VarNotFound(Sys_InitializeGameDLL_String);
	char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\xC3";
	*(DWORD*)(pattern + 1) = (DWORD)Sys_InitializeGameDLL_String;
	auto Sys_InitializeGameDLL_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(Sys_InitializeGameDLL_PushString);

	typedef struct Sys_InitializeGameDLL_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}Sys_InitializeGameDLL_SearchContext;

	Sys_InitializeGameDLL_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((PUCHAR)Sys_InitializeGameDLL_PushString + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (Sys_InitializeGameDLL_SearchContext*)context;

		if (instCount < 5 && pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			hostparam_basedir = (decltype(hostparam_basedir))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			return TRUE;
		}

		if (instCount < 5 && pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			hostparam_basedir = (decltype(hostparam_basedir))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(hostparam_basedir);
}

void Engine_PatchAddress_VGUIClient001(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "VClientVGUI001";
		auto VClientVGUI001_String = Search_Pattern_Data(sigs, DllInfo);
		if (!VClientVGUI001_String)
			VClientVGUI001_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(VClientVGUI001_String);
		char pattern[] = "\x8B\x2A\x2A\x6A\x00\x68\x2A\x2A\x2A\x2A\x89";
		*(DWORD*)(pattern + 6) = (DWORD)VClientVGUI001_String;
		auto VClientVGUI001_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(VClientVGUI001_PushString);

		const char sigs2[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x85\xC0";
		auto Call_VClientVGUI001_CreateInterface = g_pMetaHookAPI->ReverseSearchPattern(VClientVGUI001_PushString, 0x50, sigs2, sizeof(sigs2) - 1);
		Sig_VarNotFound(Call_VClientVGUI001_CreateInterface);

		PUCHAR address = (PUCHAR)Call_VClientVGUI001_CreateInterface + 15;

		address = (PUCHAR)ConvertDllInfoSpace(address, DllInfo, RealDllInfo);

		gPrivateFuncs.VGUIClient001_CreateInterface = (decltype(gPrivateFuncs.VGUIClient001_CreateInterface))GetCallAddress(address);

		g_pMetaHookAPI->InlinePatchRedirectBranch(address, VGUIClient001_CreateInterface, NULL);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs[] = "VClientVGUI001";
		auto VClientVGUI001_String = Search_Pattern_Data(sigs, DllInfo);
		if (!VClientVGUI001_String)
			VClientVGUI001_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(VClientVGUI001_String);
		char pattern[] = "\x8B\x4B\x1C\x6A\x00\x68\x2A\x2A\x2A\x2A\x89";
		*(DWORD*)(pattern + 6) = (DWORD)VClientVGUI001_String;
		auto VClientVGUI001_PushString = Search_Pattern(pattern, DllInfo);
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

		address = (PUCHAR)ConvertDllInfoSpace(address, DllInfo, RealDllInfo);

		gPrivateFuncs.VGUIClient001_CreateInterface = (decltype(gPrivateFuncs.VGUIClient001_CreateInterface))GetCallAddress(address);

		g_pMetaHookAPI->InlinePatchRedirectBranch(address, VGUIClient001_CreateInterface, NULL);
	}
	else
	{
		const char sigs[] = "VClientVGUI001";
		auto VClientVGUI001_String = Search_Pattern_Data(sigs, DllInfo);
		if (!VClientVGUI001_String)
			VClientVGUI001_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(VClientVGUI001_String);
		char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\x89";
		*(DWORD*)(pattern + 3) = (DWORD)VClientVGUI001_String;
		auto VClientVGUI001_PushString = Search_Pattern(pattern, DllInfo);
		if (!VClientVGUI001_PushString)
		{
			char pattern2[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\xFF";
			*(DWORD*)(pattern2 + 3) = (DWORD)VClientVGUI001_String;
			VClientVGUI001_PushString = Search_Pattern(pattern2, DllInfo);
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

			address = (PUCHAR)ConvertDllInfoSpace(address, DllInfo, RealDllInfo);

			gPrivateFuncs.VGUIClient001_CreateInterface = (decltype(gPrivateFuncs.VGUIClient001_CreateInterface))GetCallAddress(address);

			g_pMetaHookAPI->InlinePatchRedirectBranch(address, VGUIClient001_CreateInterface, NULL);
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
				auto g_pClientFactory_VA = *(PVOID*)(pClientFactoryAddr + 2);
				g_pClientFactory = (decltype(g_pClientFactory))ConvertDllInfoSpace(g_pClientFactory_VA, DllInfo, RealDllInfo);
			}
#endif
		}
	}
}

void Engine_PatchAddress_LanguageStrncpy(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char pattern[] = "\xB8\x2A\x2A\x2A\x2A\x68\x80\x00\x00\x00\x50";

		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchEnd = SearchBegin + DllInfo.TextSize;
		while (1)
		{
			auto LanguageStrncpy = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchEnd - SearchBegin, pattern);
			if (LanguageStrncpy)
			{
				typedef struct LanguageStrncpySearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					bool bHasPushEax;
					bool bHasPushEnglish;
				}LanguageStrncpySearchContext;

				LanguageStrncpySearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges(LanguageStrncpy, 0x30, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto ctx = (LanguageStrncpySearchContext*)context;
					auto pinst = (cs_insn*)inst;

					if (!ctx->bHasPushEax && pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[0].reg == X86_REG_EAX)
					{
						ctx->bHasPushEax = true;
					}

					if (!ctx->bHasPushEnglish && pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						(((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)ctx->DllInfo.RdataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize)
							|| ((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)ctx->DllInfo.DataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
							))
					{
						const char* pPushString = (const char*)pinst->detail->x86.operands[0].imm;

						if (!memcmp(pPushString, "english", sizeof("english")))
						{
							ctx->bHasPushEnglish = true;
						}
					}

					if (!ctx->bHasPushEnglish && pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						(((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)ctx->DllInfo.RdataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize)
							|| ((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)ctx->DllInfo.DataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
							))
					{
						const char* pPushString = (const char*)pinst->detail->x86.operands[1].imm;

						if (!memcmp(pPushString, "english", sizeof("english")))
						{
							ctx->bHasPushEnglish = true;
						}
					}

					if (ctx->bHasPushEax && ctx->bHasPushEnglish)
					{
						if (address[0] == 0xE8)
						{
							auto address_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

							gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(address_RealDllBased);
							g_pMetaHookAPI->InlinePatchRedirectBranch(address_RealDllBased, NewV_strncpy, NULL);
							return TRUE;
						}
						else if (address[0] == 0xEB)
						{
							char jmprva = *(char*)(address + 1);
							PUCHAR jmptarget = address + 2 + jmprva;

							if (jmptarget[0] == 0xE8)
							{
								auto jmptarget_RealDllBased = ConvertDllInfoSpace(jmptarget, ctx->DllInfo, ctx->RealDllInfo);

								gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(jmptarget_RealDllBased);
								g_pMetaHookAPI->InlinePatchRedirectBranch(jmptarget_RealDllBased, NewV_strncpy, NULL);
								return TRUE;
							}
						}
						else if (address[0] == 0xFF && address[1] == 0x15)
						{
							auto address_RealDllBased = (PUCHAR)ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

							gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy)) * *(ULONG_PTR**)(address_RealDllBased + 2);
							g_pMetaHookAPI->InlinePatchRedirectBranch(address_RealDllBased, NewV_strncpy, (void**)&gPrivateFuncs.V_strncpy);
							return TRUE;
						}
					}

					if (instCount > 12)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, &ctx);

				SearchBegin = LanguageStrncpy + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		const char pattern[] = "\x68\x80\x00\x00\x00\x50";

		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchEnd = SearchBegin + DllInfo.TextSize;
		while (1)
		{
			auto LanguageStrncpy = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchEnd - SearchBegin, pattern);
			if (LanguageStrncpy)
			{
				typedef struct LanguageStrncpySearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					bool bHasPushEax;
					bool bHasPushEnglish;
				}LanguageStrncpySearchContext;

				LanguageStrncpySearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges(LanguageStrncpy, 0x30, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto ctx = (LanguageStrncpySearchContext*)context;
					auto pinst = (cs_insn*)inst;

					if (!ctx->bHasPushEax && pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[0].reg == X86_REG_EAX)
					{
						ctx->bHasPushEax = true;
					}

					if (!ctx->bHasPushEnglish && pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						(((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)ctx->DllInfo.RdataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize)
							|| ((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)ctx->DllInfo.DataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
							))
					{
						const char* pPushString = (const char*)pinst->detail->x86.operands[0].imm;

						if (!memcmp(pPushString, "english", sizeof("english")))
						{
							ctx->bHasPushEnglish = true;
						}
					}

					if (!ctx->bHasPushEnglish && pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						(((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)ctx->DllInfo.RdataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize)
							|| ((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)ctx->DllInfo.DataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
							))
					{
						const char* pPushString = (const char*)pinst->detail->x86.operands[1].imm;

						if (!memcmp(pPushString, "english", sizeof("english")))
						{
							ctx->bHasPushEnglish = true;
						}
					}

					if (ctx->bHasPushEax && ctx->bHasPushEnglish)
					{
						if (address[0] == 0xE8)
						{
							auto address_RealDllBased = (PUCHAR)ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

							gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(address_RealDllBased);
							g_pMetaHookAPI->InlinePatchRedirectBranch(address_RealDllBased, NewV_strncpy, NULL);
							return TRUE;
						}
						else if (address[0] == 0xEB)
						{
							char jmprva = *(char*)(address + 1);
							PUCHAR jmptarget = address + 2 + jmprva;

							if (jmptarget[0] == 0xE8)
							{
								auto jmptarget_RealDllBased = (PUCHAR)ConvertDllInfoSpace(jmptarget, ctx->DllInfo, ctx->RealDllInfo);

								gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(jmptarget_RealDllBased);
								g_pMetaHookAPI->InlinePatchRedirectBranch(jmptarget_RealDllBased, NewV_strncpy, NULL);
								return TRUE;
							}
						}
						else if (address[0] == 0xFF && address[1] == 0x15)
						{
							auto address_RealDllBased = (PUCHAR)ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

							g_pMetaHookAPI->InlinePatchRedirectBranch(address_RealDllBased, NewV_strncpy, (void**)&gPrivateFuncs.V_strncpy);
							return TRUE;
						}
					}

					if (instCount > 12)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, &ctx);

				SearchBegin = LanguageStrncpy + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}
	}
}

void Engine_FillAddress_StaticEngineSurface(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*

		void* staticEngineSurface = NULL;
	*/

	if (1)
	{
		/*
			.text:01DCD259 68 F0 00 00 00                                      push    0F0h            ; int
			.text:01DCD25E 68 40 01 00 00                                      push    140h            ; int
			.text:01DCD263 6A 00                                               push    0               ; int
			.text:01DCD265 6A 00                                               push    0               ; int
			.text:01DCD267 8B C8                                               mov     ecx, eax        ; this
			.text:01DCD269 E8 49 85 02 00                                      call    ??0Panel@vgui@@QAE@HHHH@Z ; vgui::Panel::Panel(int,int,int,int)
			.text:01DCD26E 8B C8                                               mov     ecx, eax
		*/

		/*
			if ( v1 )
			  v2 = (struct vgui::Panel *)vgui::Panel::Panel(v1, 0, 0, 320, 240);
			else
			  v2 = 0;
			v3 = *(_DWORD *)v2;
			dword_96F5684 = v2;
			(*(void (__stdcall **)(_DWORD))(v3 + 216))(0);
			(*(void (__thiscall **)(struct vgui::Panel *, _DWORD))(*(_DWORD *)dword_96F5684 + 220))(dword_96F5684, 0);
			(*(void (__thiscall **)(struct vgui::Panel *, _DWORD))(*(_DWORD *)dword_96F5684 + 224))(dword_96F5684, 0);
			v4 = *(void (__thiscall **)(struct vgui::Panel *, int))(*(_DWORD *)dword_96F5684 + 148);
			v5 = vgui::App::getInstance();
			v6 = (*(int (__thiscall **)(struct vgui::App *))(*(_DWORD *)v5 + 84))(v5);
			v7 = (*(int (__thiscall **)(int, int))(*(_DWORD *)v6 + 20))(v6, 1);
			v4(dword_96F5684, v7);
			v8 = (int (__cdecl *)(const char *, _DWORD))sub_1DF42E0();
			if ( j__malloc(0x30u) )
			{
			  v9 = v8("EngineSurface007", 0);
			  v10 = (vgui::SurfaceBase *)sub_1DC6420(dword_96F5684, v9);
		
		*/

		const char pattern[] = "\x68\xF0\x00\x00\x00\x68\x40\x01\x00\x00\x6A\x00\x6A\x00";

		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchEnd = SearchBegin + DllInfo.TextSize;
		while (1)
		{
			auto pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchEnd - SearchBegin, pattern);

			if (pFound)
			{
				auto pStartDisasm = pFound + Sig_Length(pattern);

				typedef struct VGuiWrap_Startup_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					int instCountPush0{};
					int instCountPushEngineSurface007{};
				} VGuiWrap_Startup_SearchContext;

				VGuiWrap_Startup_SearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges(pStartDisasm, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto ctx = (VGuiWrap_Startup_SearchContext*)context;
					auto pinst = (cs_insn*)inst;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0)
					{
						ctx->instCountPush0 = instCount;
					}

					if (!ctx->instCountPushEngineSurface007 && ctx->instCountPush0 && instCount >= ctx->instCountPush0 + 1 && instCount <= ctx->instCountPush0 + 3 &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						(((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)ctx->DllInfo.RdataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize)
							|| ((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)ctx->DllInfo.DataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
							))
					{
						const char* pPushString = (const char*)pinst->detail->x86.operands[0].imm;

						if (!memcmp(pPushString, "EngineSurface007", sizeof("EngineSurface007")))
						{
							ctx->instCountPushEngineSurface007 = instCount;
						}
					}

					if (ctx->instCountPush0 && ctx->instCountPushEngineSurface007 && instCount < ctx->instCountPushEngineSurface007 + 16)
					{
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_REG)
						{
							staticEngineSurface = (decltype(staticEngineSurface))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
							return TRUE;
						}
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

				}, 0, &ctx);

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}

		Sig_VarNotFound(staticEngineSurface);

	}
}

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Engine_FillAddress_PanelInit(DllInfo, RealDllInfo);
	Engine_FillAddress_GetClientTime(DllInfo, RealDllInfo);
	Engine_FillAddress_RealTime(DllInfo, RealDllInfo);
	Engine_FillAddress_CL_ViewEntityVars(DllInfo, RealDllInfo);
	Engine_FillAddress_ListenerOrigin(DllInfo, RealDllInfo);
	Engine_FillAddress_Sys_InitializeGameDLL(DllInfo, RealDllInfo);
	Engine_FillAddress_StaticEngineSurface(DllInfo, RealDllInfo);
}

void Client_FillAddress_SCClient_VisibleMouse(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
.text:1006C550                                     sub_1006C550    proc near               ; CODE XREF: sub_10030310+6↑j
.text:1006C550                                                                             ; .text:100324D2↑p ...
.text:1006C550 56                                                  push    esi
.text:1006C551 8B F1                                               mov     esi, ecx
.text:1006C553 57                                                  push    edi
.text:1006C554 8B 8E E4 15 00 00                                   mov     ecx, [esi+15E4h]
.text:1006C55A 39 8E 1C 14 00 00                                   cmp     [esi+141Ch], ecx
.text:1006C560 75 5A                                               jnz     short loc_1006C5BC
.text:1006C562 85 C9                                               test    ecx, ecx
.text:1006C564 74 56                                               jz      short loc_1006C5BC
.text:1006C566 8B 01                                               mov     eax, [ecx]
.text:1006C568 8B 40 28                                            mov     eax, [eax+28h]
.text:1006C56B FF D0                                               call    eax
.text:1006C56D 84 C0                                               test    al, al
.text:1006C56F 74 4B                                               jz      short loc_1006C5BC
.text:1006C571 C7 05 9C A9 63 10 01 00 00 00                       mov     g_iVisibleMouse, 1
.text:1006C57B 8B 8E E4 15 00 00                                   mov     ecx, [esi+15E4h]
	*/

	char pattern[] = "\x8B\x40\x28\xFF\xD0\x84\xC0\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00";
	auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(g_iVisibleMouse);

	PVOID g_iVisibleMouse_VA = *(PVOID*)(addr + 11);
	g_iVisibleMouse = (decltype(g_iVisibleMouse))ConvertDllInfoSpace(g_iVisibleMouse_VA, DllInfo, RealDllInfo);
}

void Client_FillAddress_VisibleMouse(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iVisibleMouse)
		return;

	PVOID IN_Accumulate = ConvertDllInfoSpace((void*)g_pMetaSave->pExportFuncs->IN_Accumulate, RealDllInfo, DllInfo);

	if (!IN_Accumulate)
	{
		if (g_pMetaHookAPI->GetClientModule())
		{
			IN_Accumulate = ConvertDllInfoSpace(GetProcAddress(g_pMetaHookAPI->GetClientModule(), "IN_Accumulate"), RealDllInfo, DllInfo);
		}
	}

	if (IN_Accumulate)
	{
		typedef struct IN_Accumulate_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			DWORD candidate{};
			int candidate_register{};
		}IN_Accumulate_SearchContext;

		IN_Accumulate_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(IN_Accumulate, 0x30, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto ctx = (IN_Accumulate_SearchContext*)context;
				auto pinst = (cs_insn*)inst;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->candidate = pinst->detail->x86.operands[1].mem.disp;
					ctx->candidate_register = pinst->detail->x86.operands[0].reg;
				}

				auto findVisibleMouseFillZero = [ctx](PVOID address, size_t oprandSize) -> bool{

					typedef struct findVisibleMouseFillZero_SearchContext
					{
						bool FoundCall{};
					}findVisibleMouseFillZero_SearchContext_t;

					findVisibleMouseFillZero_SearchContext_t ctx2{};

					const auto& DllInfo = ctx->DllInfo;
					//C7 05 78 4D 9A 01 00 00 00 00                       mov     g_iVisibleMouse, 0
					if (oprandSize == 4)
					{
						char pattern[] = "\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00";
						*(PVOID*)(pattern + 2) = address;
						auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);

						if (addr)
						{
							g_pMetaHookAPI->DisasmSingleInstruction(addr + Sig_Length(pattern), [](void* inst2, PUCHAR address2, size_t instLen2, PVOID context2) {
								auto ctx2 = (findVisibleMouseFillZero_SearchContext_t*)context2;
								auto pinst2 = (cs_insn*)inst2;

								if (pinst2->id == X86_INS_CALL)
								{
									ctx2->FoundCall = true;
								}

							}, & ctx2);
							return ctx2.FoundCall;
						}
					}
					else if (oprandSize == 1)
					{
						char pattern[] = "\xC6\x05\x2A\x2A\x2A\x2A\x01";
						*(PVOID*)(pattern + 2) = address;
						auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);

						if (addr)
						{
							g_pMetaHookAPI->DisasmSingleInstruction(addr + Sig_Length(pattern), [](void* inst2, PUCHAR address2, size_t instLen2, PVOID context2) {
								auto ctx2 = (findVisibleMouseFillZero_SearchContext_t*)context2;
								auto pinst2 = (cs_insn*)inst2;

								if (pinst2->id == X86_INS_CALL)
								{
									ctx2->FoundCall = true;
								}

							}, &ctx2);
							return ctx2.FoundCall;
						}
					}

					return false;
				};

				if (ctx->candidate_register &&
					pinst->id == X86_INS_TEST &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == ctx->candidate_register &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == ctx->candidate_register)
				{
					if (findVisibleMouseFillZero((PVOID)ctx->candidate, pinst->detail->x86.operands[0].size))
					{
						g_iVisibleMouse = (decltype(g_iVisibleMouse))ConvertDllInfoSpace((PVOID)ctx->candidate, ctx->DllInfo, ctx->RealDllInfo);
					}
				}

				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0)
				{
					if (findVisibleMouseFillZero((PVOID)pinst->detail->x86.operands[0].mem.disp, pinst->detail->x86.operands[0].size))
					{
						g_iVisibleMouse = (decltype(g_iVisibleMouse))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}
				}

				if (g_iVisibleMouse)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);
	}
}

void Client_FillAddress_SCClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory)
	{
		auto SCClient001 = pfnClientFactory("SCClientDLL001", 0);

		if (SCClient001)
		{
			Client_FillAddress_SCClient_VisibleMouse(DllInfo, RealDllInfo);
		}
	}
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCZero = true;
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCZDS = true;
	}

	Client_FillAddress_SCClient(DllInfo, RealDllInfo);

	Client_FillAddress_VisibleMouse(DllInfo, RealDllInfo);
}

void Engine_InstallHooks(void)
{

}

void Engine_UninstallHooks(void)
{

}

void Client_InstallHooks(void)
{

}

void Client_UninstallHooks(void)
{

}

static HMODULE WINAPI NewLoadLibraryA_GameUI(LPCSTR lpLibFileName)
{
	auto result = LoadLibraryA(lpLibFileName);

	if (g_hServerBrowser == result && !g_bIsServerBrowserHooked)
	{
		ServerBrowser_FillAddress();
		ServerBrowser_InstallHooks();

		g_bIsServerBrowserHooked = true;
	}
	
	return result;
}

void DllLoadNotification(mh_load_dll_notification_context_t* ctx)
{
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)
	{
		if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"GameUI.dll"))
		{
			g_hGameUI = ctx->hModule;

			g_GameUIDllInfo.ImageBase = g_pMetaHookAPI->GetModuleBase(g_hGameUI);
			g_GameUIDllInfo.ImageSize = g_pMetaHookAPI->GetModuleSize(g_GameUIDllInfo.ImageBase);
			g_GameUIDllInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_GameUIDllInfo.ImageBase, ".text\0\0\0", &g_GameUIDllInfo.TextSize);
			g_GameUIDllInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_GameUIDllInfo.ImageBase, ".rdata\0\0", &g_GameUIDllInfo.RdataSize);
			g_GameUIDllInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_GameUIDllInfo.ImageBase, ".data\0\0\0", &g_GameUIDllInfo.DataSize);

			g_pMetaHookAPI->IATHook(g_hGameUI, "kernel32.dll", "LoadLibraryA", NewLoadLibraryA_GameUI, NULL);
		}
		else if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"ServerBrowser.dll"))
		{
			g_hServerBrowser = ctx->hModule;

			g_ServerBrowserDllInfo.ImageBase = g_pMetaHookAPI->GetModuleBase(g_hServerBrowser);
			g_ServerBrowserDllInfo.ImageSize = g_pMetaHookAPI->GetModuleSize(g_ServerBrowserDllInfo.ImageBase);
			g_ServerBrowserDllInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_ServerBrowserDllInfo.ImageBase, ".text\0\0\0", &g_ServerBrowserDllInfo.TextSize);
			g_ServerBrowserDllInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_ServerBrowserDllInfo.ImageBase, ".rdata\0\0", &g_ServerBrowserDllInfo.RdataSize);
			g_ServerBrowserDllInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_ServerBrowserDllInfo.ImageBase, ".data\0\0\0", &g_ServerBrowserDllInfo.DataSize);
		}
	}
	else if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)
	{
		if (ctx->hModule == g_hGameUI)
		{
			g_hGameUI = NULL;
		}
		else if (ctx->hModule == g_hServerBrowser)
		{
			ServerBrowser_UninstallHooks();
			g_hServerBrowser = NULL;
			g_bIsServerBrowserHooked = false;
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