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
	}
}

PVOID VGUI2_FindPanelInit(PVOID TextBase, ULONG TextSize)
{
	PVOID Panel_Init = NULL;
	if (1)
	{
		const char sigs[] = "\x6A\x18\x6A\x40\x6A\x00\x6A\x00";
		auto Panel_Init_Push = (PUCHAR)Search_Pattern_From_Size(TextBase, TextSize, sigs);
		if (Panel_Init_Push)
		{
			Panel_Init_Push += Sig_Length(sigs);
		}
		else
		{
			//  mov     dword ptr [ebx+24h], 2
			// C7 46 24 02 00 00 00                                mov     dword ptr [esi+24h], 2
			const char sigs2[] = "\x6A\x18\xC6";
			PUCHAR SearchBegin = (PUCHAR)TextBase;
			PUCHAR SearchLimit = (PUCHAR)TextBase + TextSize;
			while (SearchBegin < SearchLimit)
			{
				PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, sigs2);
				if (pFound)
				{
					typedef struct
					{
						int instCount_push40h;
						int instCount_call;
						PVOID pushaddr;
					}VGUI2_FindPanelInit_SearchContext;

					VGUI2_FindPanelInit_SearchContext ctx2;

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
							ctx->pushaddr = address;
						}

						if (address[0] == 0xE8)
						{
							ctx->instCount_call = instCount;
							return TRUE;
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;

					}, 0, &ctx2);

					if (ctx2.instCount_call > ctx2.instCount_push40h && ctx2.instCount_call < ctx2.instCount_push40h + 15)
					{
						Panel_Init_Push = (decltype(Panel_Init_Push))ctx2.pushaddr;
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
		
		if (Panel_Init_Push)
		{
			g_pMetaHookAPI->DisasmRanges(Panel_Init_Push, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto pPanel_Init = (PVOID*)context;

				if (address[0] == 0xE8 && instCount <= 15)
				{
					(*pPanel_Init) = GetCallAddress(address);

					return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

			}, 0, &Panel_Init);
		}
	}

	return Panel_Init;
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

#if 0
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
#endif

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

	gPrivateFuncs.EngineVGUI2_Panel_Init = (decltype(gPrivateFuncs.EngineVGUI2_Panel_Init))VGUI2_FindPanelInit(g_dwEngineTextBase, g_dwEngineTextSize);
	Sig_FuncNotFound(EngineVGUI2_Panel_Init);

	const char sigs1[] = "Sys_InitializeGameDLL called twice";
	auto Sys_InitializeGameDLL_String = Search_Pattern_Data(sigs1);
	if (!Sys_InitializeGameDLL_String)
		Sys_InitializeGameDLL_String = Search_Pattern_Rdata(sigs1);
	Sig_VarNotFound(Sys_InitializeGameDLL_String);
	char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\xC3";
	*(DWORD*)(pattern + 1) = (DWORD)Sys_InitializeGameDLL_String;
	auto Sys_InitializeGameDLL_PushString = Search_Pattern(pattern);
	Sig_VarNotFound(Sys_InitializeGameDLL_PushString);

	g_pMetaHookAPI->DisasmRanges((PUCHAR)Sys_InitializeGameDLL_PushString + sizeof(pattern) - 1, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;

			if (instCount < 5 && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				hostparam_basedir = (decltype(hostparam_basedir))pinst->detail->x86.operands[1].mem.disp;
				return TRUE;
			}

			if (instCount < 5 && pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				hostparam_basedir = (decltype(hostparam_basedir))pinst->detail->x86.operands[0].mem.disp;
				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);
	Sig_VarNotFound(hostparam_basedir);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char pattern[] = "\xB8\x2A\x2A\x2A\x2A\x68\x80\x00\x00\x00\x50";

		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchEnd = SearchBegin + g_dwEngineTextSize;
		while (1)
		{
			auto LanguageStrncpy = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchEnd - SearchBegin, pattern);
			if (LanguageStrncpy)
			{
				typedef struct
				{
					bool bHasPushEax;
					bool bHasPushEnglish;
				}LanguageStrncpySearchContext;

				LanguageStrncpySearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(LanguageStrncpy, 0x30, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
					{
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
							(((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)g_dwEngineRdataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize)
								|| ((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)g_dwEngineDataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
							(((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)g_dwEngineRdataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize)
								|| ((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)g_dwEngineDataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
								gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(address);
								PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
								int rva = pfnNewV_strncpy - (address + 5);
								g_pMetaHookAPI->WriteMemory(address + 1, (BYTE*)&rva, 4);
								return TRUE;
							}
							else if (address[0] == 0xEB)
							{
								char jmprva = *(char*)(address + 1);
								PUCHAR jmptarget = address + 2 + jmprva;

								if (jmptarget[0] == 0xE8)
								{
									gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(jmptarget);
									PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
									int rva = pfnNewV_strncpy - (jmptarget + 5);
									g_pMetaHookAPI->WriteMemory(jmptarget + 1, (BYTE*)&rva, 4);
									return TRUE;
								}
							}
							else if (address[0] == 0xFF && address[1] == 0x15)
							{
								gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy)) * *(ULONG_PTR**)(address + 2);

								PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
								int rva = pfnNewV_strncpy - (address + 5);

								char trampoline[] = "\xE8\x2A\x2A\x2A\x2A\x90";
								*(int*)(trampoline + 1) = rva;

								g_pMetaHookAPI->WriteMemory(address, trampoline, sizeof(trampoline) - 1);
								return TRUE;
							}
						}

						if (instCount > 8)
							return TRUE;

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;
					}, 0, &ctx);

				SearchBegin = LanguageStrncpy + sizeof(pattern) - 1;
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

		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchEnd = SearchBegin + g_dwEngineTextSize;
		while (1)
		{
			auto LanguageStrncpy = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchEnd - SearchBegin, pattern);
			if (LanguageStrncpy)
			{
				typedef struct
				{
					bool bHasPushEax;
					bool bHasPushEnglish;
				}LanguageStrncpySearchContext;

				LanguageStrncpySearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(LanguageStrncpy, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto ctx = (LanguageStrncpySearchContext*)context;
					auto pinst = (cs_insn *)inst;

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
						(((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)g_dwEngineRdataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize)
						|| ((PUCHAR)pinst->detail->x86.operands[0].imm >= (PUCHAR)g_dwEngineDataBase && (PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
						(((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)g_dwEngineRdataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize)
							|| ((PUCHAR)pinst->detail->x86.operands[1].imm >= (PUCHAR)g_dwEngineDataBase && (PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
						else if (address[0] == 0xFF && address[1] == 0x15)
						{
							gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))**(ULONG_PTR **)(address + 2);
							
							PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
							int rva = pfnNewV_strncpy - (address + 5);

							char trampoline[] = "\xE8\x2A\x2A\x2A\x2A\x90";
							*(int*)(trampoline + 1) = rva;

							g_pMetaHookAPI->WriteMemory(address, trampoline, sizeof(trampoline) - 1);
							return TRUE;
						}
					}

					if (instCount > 8)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, &ctx);

				SearchBegin = LanguageStrncpy + sizeof(pattern) - 1;
			}
			else
			{
				break;
			}
		}
	}
}

void Client_FillAddress(void)
{
	ULONG ClientTextSize = 0;
	auto ClientTextBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".text\0\0\0", &ClientTextSize);

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

		if(1)
		{
			char pattern[] = "\x8B\x40\x28\xFF\xD0\x84\xC0\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00";
			auto addr = (PUCHAR)Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);

			Sig_AddrNotFound(g_iVisibleMouse);

			g_iVisibleMouse = *(decltype(g_iVisibleMouse) *)(addr + 11);
		}
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;
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
	}
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
		g_bIsServerBrowserHooked = true;

		ServerBrowser_FillAddress();
		ServerBrowser_InstallHooks();
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

			g_pMetaHookAPI->IATHook(g_hGameUI, "kernel32.dll", "LoadLibraryA", NewLoadLibraryA_GameUI, NULL);
		}
		else if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"ServerBrowser.dll"))
		{
			g_hServerBrowser = ctx->hModule;
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
