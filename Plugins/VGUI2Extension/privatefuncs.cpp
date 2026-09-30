#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "privatefuncs.h"

double *cl_time = NULL;
double *cl_oldtime = NULL;
double *realtime = NULL;

int* cl_viewentity = NULL;

vec3_t *listener_origin = NULL;

quakeparms_t* host_parms = NULL;

CreateInterfaceFn *g_pClientFactory = NULL;

char m_szCurrentGameLanguage[128] = { 0 };

private_funcs_t gPrivateFuncs = { 0 };

static hook_t* g_phook_LanguageRegistry = nullptr;

HMODULE g_hGameUI = NULL;
HMODULE g_hServerBrowser = NULL;
bool g_bIsServerBrowserHooked = false;

mh_dll_info_t g_GameUIDllInfo = { 0 };
mh_dll_info_t g_ServerBrowserDllInfo = { 0 };

const char* GetCurrentGameLanguage()
{
	return m_szCurrentGameLanguage;
}

void* Sys_GetMainWindow()
{
	return (**pmainwindow);
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

void Engine_FillAddress_PanelInit(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.EngineVGUI2_Panel_Init = (decltype(gPrivateFuncs.EngineVGUI2_Panel_Init))
		GamedataResolvePtr(RealDllInfo.ImageBase, "vgui2::Panel::Init(int, int, int, int)", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_GetClientTime(const mh_dll_info_t& RealDllInfo)
{
	cl_time = (decltype(cl_time))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_time", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_oldtime = (decltype(cl_oldtime))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_oldtime", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_HostParms(const mh_dll_info_t& RealDllInfo)
{
	host_parms = (decltype(host_parms))GamedataResolvePtr(RealDllInfo.ImageBase, "host_parms", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_PatchAddress_VGUIClient001(const mh_dll_info_t&, const mh_dll_info_t& RealDllInfo)
{
	const char* patchName = "VGUIClient001_CreateInterface";
	const auto status = g_pMetaHookAPI->IsGameSymbolAvailable(RealDllInfo.ImageBase, patchName);
	if (status == MH_GAMESYMBOL_SYMBOL_NOT_FOUND)
	{
		// Legacy engines obtain the factory through the zero-argument ClientFactory callback.
		g_pClientFactory = (decltype(g_pClientFactory))GamedataResolvePtr(
			RealDllInfo.ImageBase, "g_pClientFactory", MH_GAMESYMBOL_KIND_GLOBAL);
		return;
	}
	if (status != MH_GAMESYMBOL_OK)
	{
		Sys_Error("Could not query gamedata symbol: %s (%s)\nEngine buildnum: %d",
			patchName, g_pMetaHookAPI->GetGameSymbolStatusString(status), g_dwEngineBuildnum);
		return;
	}

	// The PATCH is the Sys_GetFactory(hClientDLL) CALL, not the interface query.
	auto address = (PUCHAR)GamedataResolvePtr(RealDllInfo.ImageBase, patchName, MH_GAMESYMBOL_KIND_PATCH);
	gPrivateFuncs.VGUIClient001_CreateInterface = (decltype(gPrivateFuncs.VGUIClient001_CreateInterface))GetCallAddress(address);
	g_pMetaHookAPI->InlinePatchRedirectBranch(address, VGUIClient001_CreateInterface, NULL);
}

void Engine_PatchAddress_LanguageStrncpy(const mh_dll_info_t&, const mh_dll_info_t& RealDllInfo)
{
	// Old engines read the registry directly instead of copying an English literal.
	if (gPrivateFuncs.Sys_GetRegKeyValueUnderRoot)
		return;

	// Both filesystem owners copy the default language through the same function.
	const char* patchNames[] = {
		"FileSystem_SetGameDirectory_V_strncpy_callsite_0",
		"FileSystem_AddFallbackGameDir_V_strncpy_callsite_0",
	};
	PVOID patchSites[_countof(patchNames)] = {};
	for (size_t i = 0; i < _countof(patchNames); ++i)
	{
		patchSites[i] = GamedataResolvePtr(RealDllInfo.ImageBase, patchNames[i], MH_GAMESYMBOL_KIND_PATCH);
	}

	for (size_t i = 0; i < _countof(patchNames); ++i)
	{
		// The hook API handles both direct and import-indirect CALLs and saves the original target.
		if (!g_pMetaHookAPI->InlinePatchRedirectBranch(patchSites[i], NewV_strncpy, (void**)&gPrivateFuncs.V_strncpy))
		{
			Sys_Error("Could not redirect gamedata patch: %s\nEngine buildnum: %d", patchNames[i], g_dwEngineBuildnum);
			return;
		}
	}
}

void Engine_FillAddress_StaticEngineSurface(const mh_dll_info_t& RealDllInfo)
{
	staticEngineSurface = (decltype(staticEngineSurface))GamedataResolvePtr(RealDllInfo.ImageBase, "staticEngineSurface", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Sys_GetRegKeyValueUnderRoot(const mh_dll_info_t& RealDllInfo)
{
	// For blob engine only
	gPrivateFuncs.Sys_GetRegKeyValueUnderRoot = (decltype(gPrivateFuncs.Sys_GetRegKeyValueUnderRoot))
		GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "Sys_GetRegKeyValueUnderRoot", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress(const mh_dll_info_t& RealDllInfo)
{
	Engine_FillAddress_PanelInit(RealDllInfo);
	Engine_FillAddress_GetClientTime(RealDllInfo);
	Engine_FillAddress_HostParms(RealDllInfo);
	Engine_FillAddress_StaticEngineSurface(RealDllInfo);
	Engine_FillAddress_Sys_GetRegKeyValueUnderRoot(RealDllInfo);
}

extern bool g_IsNativeClientVGUI2;

void Client_FillAddress_VisibleMouse(const mh_dll_info_t& RealDllInfo)
{
	// Make client dll aware of VGUI2 mouse input capture, which is not natively supported by VGUI1 mods.
	if (!g_IsNativeClientVGUI2)
	{
		g_iVisibleMouse = (decltype(g_iVisibleMouse))GamedataResolvePtrIfAvailable(
			RealDllInfo.ImageBase, "g_iVisibleMouse", MH_GAMESYMBOL_KIND_GLOBAL);
	}
}

void Client_FillAddress(const mh_dll_info_t& RealDllInfo)
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

	Client_FillAddress_VisibleMouse(RealDllInfo);
}

void Engine_InstallHooks(void)
{
	if (gPrivateFuncs.Sys_GetRegKeyValueUnderRoot && !g_phook_LanguageRegistry)
	{
		g_phook_LanguageRegistry = g_pMetaHookAPI->InlineHook(
			(void*)gPrivateFuncs.Sys_GetRegKeyValueUnderRoot, NewEngineSys_GetRegKeyValueUnderRoot,
			(void**)&gPrivateFuncs.Sys_GetRegKeyValueUnderRoot);
		if (!g_phook_LanguageRegistry)
			Sys_Error("Could not install the engine language registry hook.");
	}
}

void Engine_UninstallHooks(void)
{
	if (g_phook_LanguageRegistry)
	{
		g_pMetaHookAPI->UnHook(g_phook_LanguageRegistry);
		g_phook_LanguageRegistry = nullptr;
		gPrivateFuncs.Sys_GetRegKeyValueUnderRoot = nullptr;
	}
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
