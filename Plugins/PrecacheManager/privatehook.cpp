#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"

static_assert(METAHOOK_API_VERSION >= 109, "PrecacheManager resolves the engine cl_resourcesonhand global from gamedata and requires MetaHook API 109 (ResolveGameSymbol)");

private_funcs_t gPrivateFuncs = {0};

void Engine_FillAddress(void)
{
	PVOID address = NULL;
	mh_gamesymbol_status_t st = g_pMetaHookAPI->ResolveGameSymbol(g_EngineDLLInfo.ImageBase, "cl_resourcesonhand", MH_GAMESYMBOL_KIND_GLOBAL, &address);

	if (st != MH_GAMESYMBOL_OK)
	{
		uint64_t crc64 = 0;
		mh_gamesymbol_status_t crcSt = g_pMetaHookAPI->GetModuleCRC64(g_EngineDLLInfo.ImageBase, &crc64);

		if (crcSt == MH_GAMESYMBOL_OK)
		{
			Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nCRC64: %016llx\nReason: %s",
				"cl_resourcesonhand", g_dwEngineBuildnum, (unsigned long long)crc64, g_pMetaHookAPI->GetGameSymbolStatusString(st));
		}
		else
		{
			Sys_Error("Failed to resolve \"%s\"\nEngine buildnum: %d\nReason: %s",
				"cl_resourcesonhand", g_dwEngineBuildnum, g_pMetaHookAPI->GetGameSymbolStatusString(st));
		}

		return;
	}

	// The gamedata global is the list sentinel node itself, so the resolved address
	// is exactly what FS_Dump_Precaches walks: start at ->pNext and stop when the
	// walk returns here. No extra dereference.
	cl_resourcesonhand = (decltype(cl_resourcesonhand))address;
}
