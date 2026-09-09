#include <metahook.h>
#include <string>
#include <vector>

#include "plugins.h"
#include "privatehook.h"
#include "util.h"

#include "ResourceReplacer.h"

static_assert(METAHOOK_API_VERSION >= 110, "ResourceReplacer consumes gamedata PATCH symbols through IsGameSymbolAvailable and requires MetaHook API 110");

private_funcs_t gPrivateFuncs = {  };

static hook_t* g_phook_CL_PrecacheResources = NULL;

struct CallSite_t
{
	std::string symbolName;
	PVOID address;
};

static std::vector<CallSite_t> g_S_LoadSound_FS_OpenCallSites;
static std::vector<CallSite_t> g_Mod_LoadModel_FS_OpenCallSites;

// On gamedata failure, print diagnostics (symbol / buildnum / CRC64 / status string) and abort via Sys_Error.
static void ReportSymbolFailure(const char* symbolName, mh_gamesymbol_status_t status)
{
	uint64_t crc64 = 0;
	mh_gamesymbol_status_t crcSt = g_pMetaHookAPI->GetModuleCRC64(g_EngineDLLInfo.ImageBase, &crc64);

	if (crcSt == MH_GAMESYMBOL_OK)
	{
		Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nCRC64: %016llx\nReason: %s",
			symbolName, g_dwEngineBuildnum, (unsigned long long)crc64, g_pMetaHookAPI->GetGameSymbolStatusString(status));
	}
	else
	{
		Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nReason: %s",
			symbolName, g_dwEngineBuildnum, g_pMetaHookAPI->GetGameSymbolStatusString(status));
	}
}

// The return value is the real-image VA of the gamedata record.
static PVOID ResolveGameSymbolOrError(const char* symbolName, mh_gamesymbol_kind_t expectedKind)
{
	PVOID va = NULL;
	mh_gamesymbol_status_t st = g_pMetaHookAPI->ResolveGameSymbol(g_EngineDLLInfo.ImageBase, symbolName, expectedKind, &va);

	if (st == MH_GAMESYMBOL_OK)
		return va;

	ReportSymbolFailure(symbolName, st);
	return NULL;
}

// The call-site set is numbered contiguously from 0; trust the upstream numbering
// and stop at the first missing index. Every set is required, so index 0 must exist.
static void CollectFSOpenCallSites(const char* symbolPrefix, std::vector<CallSite_t>& outCallSites)
{
	for (int index = 0;; ++index)
	{
		char symbolName[96];
		snprintf(symbolName, sizeof(symbolName), "%s_%d", symbolPrefix, index);

		mh_gamesymbol_status_t st = g_pMetaHookAPI->IsGameSymbolAvailable(g_EngineDLLInfo.ImageBase, symbolName);

		if (st == MH_GAMESYMBOL_SYMBOL_NOT_FOUND)
		{
			if (index == 0)
				ReportSymbolFailure(symbolName, st);

			return;
		}

		if (st != MH_GAMESYMBOL_OK)
		{
			ReportSymbolFailure(symbolName, st);
			return;
		}

		PVOID callSiteAddress = ResolveGameSymbolOrError(symbolName, MH_GAMESYMBOL_KIND_PATCH);

		if (!callSiteAddress)
			return;

		outCallSites.push_back({ symbolName, callSiteAddress });
	}
}

// Only a five-byte E8 rel32 / E9 rel32 is accepted, so a site that other code
// already rewrote is reported instead of being overwritten. This detects a changed
// opcode, not an existing E8/E9 redirect installed by another plugin.
static bool RedirectCallSite(const CallSite_t& callSite, PVOID newFunc)
{
	PUCHAR opcode = (PUCHAR)callSite.address;

	if (opcode[0] != 0xE8 && opcode[0] != 0xE9)
	{
		Sys_Error("\"%s\" at 0x%p is not a five-byte E8/E9 branch (opcode 0x%02X)",
			callSite.symbolName.c_str(), callSite.address, opcode[0]);
		return false;
	}

	if (!g_pMetaHookAPI->InlinePatchRedirectBranch(callSite.address, newFunc, NULL))
	{
		Sys_Error("Failed to redirect \"%s\" at 0x%p",
			callSite.symbolName.c_str(), callSite.address);
		return false;
	}

	return true;
}

qboolean CL_PrecacheResources()
{
	if (1)
	{
		std::string name = gEngfuncs.pfnGetLevelName();

		RemoveFileExtension(name);

		name += ".gmr";

		ModelReplacer()->LoadMapReplaceList(name.c_str());
	}

	if (1)
	{
		std::string name = gEngfuncs.pfnGetLevelName();

		RemoveFileExtension(name);

		name += ".gsr";

		SoundReplacer()->LoadMapReplaceList(name.c_str());
	}

	return gPrivateFuncs.CL_PrecacheResources();
}

FileHandle_t Mod_LoadModel_FS_Open(const char* pFileName, const char* pOptions)
{
	if (!strcmp(pOptions, "rb"))
	{
		std::string ReplacedFileName;
		if (ModelReplacer()->ReplaceFileName(pFileName, ReplacedFileName))
		{
			return gPrivateFuncs.FS_Open(ReplacedFileName.c_str(), pOptions);
		}
	}
	return gPrivateFuncs.FS_Open(pFileName, pOptions);
}

FileHandle_t S_LoadSound_FS_Open(const char* pFileName, const char* pOptions)
{
	if (!strcmp(pOptions, "rb"))
	{
		std::string ReplacedFileName;
		if (SoundReplacer()->ReplaceFileName(pFileName, ReplacedFileName))
		{
			return gPrivateFuncs.FS_Open(ReplacedFileName.c_str(), pOptions);
		}
	}

	return gPrivateFuncs.FS_Open(pFileName, pOptions);
}

void Engine_FillAddress(void)
{
	gPrivateFuncs.FS_Open = (decltype(gPrivateFuncs.FS_Open))ResolveGameSymbolOrError("FS_Open", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.CL_PrecacheResources = (decltype(gPrivateFuncs.CL_PrecacheResources))ResolveGameSymbolOrError("CL_PrecacheResources", MH_GAMESYMBOL_KIND_FUNCTION);

	CollectFSOpenCallSites("S_LoadSound_to_FS_Open_callsite", g_S_LoadSound_FS_OpenCallSites);
	CollectFSOpenCallSites("Mod_LoadModel_to_FS_Open_callsite", g_Mod_LoadModel_FS_OpenCallSites);
}

void Engine_InstallHooks()
{
	for (const auto& callSite : g_S_LoadSound_FS_OpenCallSites)
	{
		if (!RedirectCallSite(callSite, S_LoadSound_FS_Open))
			return;
	}

	for (const auto& callSite : g_Mod_LoadModel_FS_OpenCallSites)
	{
		if (!RedirectCallSite(callSite, Mod_LoadModel_FS_Open))
			return;
	}

	Install_InlineHook(CL_PrecacheResources);
}

void Engine_UninstallHooks()
{
	Uninstall_Hook(CL_PrecacheResources);
}
