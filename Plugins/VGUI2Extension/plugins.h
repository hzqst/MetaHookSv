#pragma once

#include <metahook.h>

//VGUI2Extension resolves its gamedata-covered game-private symbols (FUNCTION/
//GLOBAL/VIRTUAL_FUNCTION/STRUCT_MEMBER) exclusively through the gamedata catalog.
//Reading a reported vtable slot index (mh_gamesymbol_t::vfuncIndex) requires API 115.
static_assert(METAHOOK_API_VERSION >= 115, "VGUI2Extension consumes mh_gamesymbol_t::vfuncIndex and requires MetaHook API 115");

extern IFileSystem *g_pFileSystem;
extern IFileSystem_HL25 *g_pFileSystem_HL25;
extern mh_interface_t *g_pInterface;
extern metahook_api_t *g_pMetaHookAPI;
extern mh_enginesave_t *g_pMetaSave;

extern int g_iEngineType;
extern DWORD g_dwEngineBuildnum;
extern HMODULE g_hClientModule;

extern mh_dll_info_t g_EngineDLLInfo;
extern mh_dll_info_t g_MirrorEngineDLLInfo;
extern mh_dll_info_t g_ClientDLLInfo;

extern bool g_bIsSvenCoop;
extern bool g_bIsCounterStrike;
extern bool g_bIsCZero;
extern bool g_bIsCZDS;

#define MHPluginName "VGUI2Extension"
#define Sys_Error(msg, ...) g_pMetaHookAPI->SysError("["  MHPluginName   "] " msg, __VA_ARGS__);
#define Sig_NotFound(name) Sys_Error("Could not found: %s\nEngine buildnum: %d", #name, g_dwEngineBuildnum);
#define Sig_VarNotFound(name) if(!name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)
#define Sig_FuncNotFound(name) if(!gPrivateFuncs.name) Sig_NotFound(name)

//Resolve a required gamedata symbol; a missing symbol is fatal, mirroring the
//Sig_FuncNotFound/Sig_VarNotFound policy of the signature-scan locators.
inline PVOID GamedataResolvePtr(PVOID moduleBase, const char* symbolName, mh_gamesymbol_kind_t kind)
{
	PVOID address = nullptr;
	mh_gamesymbol_status_t status = g_pMetaHookAPI->ResolveGameSymbol(moduleBase, symbolName, kind, &address);

	if (status != MH_GAMESYMBOL_OK)
	{
		Sys_Error("Could not resolve gamedata symbol: %s (%s)\nEngine buildnum: %d",
			symbolName, g_pMetaHookAPI->GetGameSymbolStatusString(status), g_dwEngineBuildnum);
	}

	return address;
}

//Native client UI hooks are optional when the binary publishes no entry.
inline PVOID GamedataResolvePtrIfAvailable(PVOID moduleBase, const char* symbolName, mh_gamesymbol_kind_t kind)
{
	if (g_pMetaHookAPI->IsGameSymbolAvailable(moduleBase, symbolName) != MH_GAMESYMBOL_OK)
		return nullptr;

	return GamedataResolvePtr(moduleBase, symbolName, kind);
}

//Resolve a required structMember byte offset; a missing symbol or a kind mismatch
//is fatal, mirroring the Sig_FuncNotFound/Sig_VarNotFound policy of the locators.
inline DWORD GamedataResolveStructMember(PVOID moduleBase, const char* symbolName)
{
	uint32_t offset = 0;
	mh_gamesymbol_status_t status = g_pMetaHookAPI->QueryGameSymbolStructMember(moduleBase, symbolName, &offset);

	if (status != MH_GAMESYMBOL_OK)
	{
		Sys_Error("Could not resolve gamedata structMember: %s (%s)\nEngine buildnum: %d",
			symbolName, g_pMetaHookAPI->GetGameSymbolStatusString(status), g_dwEngineBuildnum);
	}

	return offset;
}

//Resolve a required VIRTUAL_FUNCTION record's owning vtable slot.
inline DWORD GamedataResolveVFuncIndex(PVOID moduleBase, const char* symbolName)
{
	mh_gamesymbol_t symbol = {};
	symbol.cbSize = sizeof(symbol);

	if (g_pMetaHookAPI->QueryGameSymbol(moduleBase, symbolName, &symbol) != MH_GAMESYMBOL_OK ||
		symbol.kind != MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION)
	{
		Sys_Error("Could not resolve gamedata virtualFunction: %s\nEngine buildnum: %d", symbolName, g_dwEngineBuildnum);
	}

	return symbol.vfuncIndex;
}

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig, dllinfo) g_pMetaHookAPI->SearchPattern(dllinfo.TextBase, dllinfo.TextSize, sig, Sig_Length(sig))
#define Search_Pattern_Data(sig, dllinfo) g_pMetaHookAPI->SearchPattern(dllinfo.DataBase, dllinfo.DataSize, sig, Sig_Length(sig))
#define Search_Pattern_Rdata(sig, dllinfo) g_pMetaHookAPI->SearchPattern(dllinfo.RdataBase, dllinfo.RdataSize, sig, Sig_Length(sig))
#define Search_Pattern_From_Size(fn, size, sig) g_pMetaHookAPI->SearchPattern((void *)(fn), size, sig, Sig_Length(sig))
#define Search_Pattern_From(fn, sig, dllinfo) g_pMetaHookAPI->SearchPattern((void *)(fn), ((PUCHAR)dllinfo.TextBase + dllinfo.TextSize) - (PUCHAR)(fn), sig, Sig_Length(sig))

#define Search_Pattern_NoWildCard(sig, dllinfo) g_pMetaHookAPI->SearchPatternNoWildCard(dllinfo.TextBase, dllinfo.TextSize, sig, Sig_Length(sig))
#define Search_Pattern_NoWildCard_Data(sig, dllinfo) g_pMetaHookAPI->SearchPatternNoWildCard(dllinfo.DataBase, dllinfo.DataSize, sig, Sig_Length(sig))
#define Search_Pattern_NoWildCard_Rdata(sig, dllinfo) g_pMetaHookAPI->SearchPatternNoWildCard(dllinfo.RdataBase, dllinfo.RdataSize, sig, Sig_Length(sig))

#define Install_InlineHook(fn) if(!g_phook_##fn) { g_phook_##fn = g_pMetaHookAPI->InlineHook((void *)gPrivateFuncs.fn, fn, (void **)&gPrivateFuncs.fn); }
#define Uninstall_Hook(fn) if(g_phook_##fn){g_pMetaHookAPI->UnHook(g_phook_##fn);g_phook_##fn = NULL;}
#define GetCallAddress(addr) g_pMetaHookAPI->GetNextCallAddr((PUCHAR)addr, 1)

#define RVA_from_VA(name, dllinfo) (ULONG)((ULONG_PTR)name##_VA - (ULONG_PTR)dllinfo.ImageBase)
#define VA_from_RVA(name, dllinfo) ((ULONG_PTR)dllinfo.ImageBase + (ULONG_PTR)name##_RVA)
#define Convert_VA_to_RVA(name, dllinfo) if(name##_VA) name##_RVA = ((ULONG_PTR)name##_VA - (ULONG_PTR)dllinfo.ImageBase)
#define Convert_RVA_to_VA(name, dllinfo) if(name##_RVA) name##_VA = (decltype(name##_VA))VA_from_RVA(name, dllinfo)
