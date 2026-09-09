#include <metahook.h>
#include <capstone.h>
#include <string>
#include <vector>
#include "plugins.h"
#include "privatehook.h"

static_assert(METAHOOK_API_VERSION >= 110, "HeapPatch consumes gamedata PATCH symbols through IsGameSymbolAvailable and requires MetaHook API 110");

struct HeapLimitPatchSite
{
	std::string symbolName;
	PVOID instructionAddress;
};

static std::vector<HeapLimitPatchSite> g_Sys_InitMemory_HeapLimitPatches;

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

void Engine_FillAddress(void)
{
	// The heap-limit patch set is numbered contiguously from 0; trust the upstream
	// numbering and stop at the first missing index.
	for (int index = 0;; ++index)
	{
		char symbolName[64];
		snprintf(symbolName, sizeof(symbolName), "Sys_InitMemory_HeapLimitPatches_%d", index);

		mh_gamesymbol_status_t st = g_pMetaHookAPI->IsGameSymbolAvailable(g_EngineDLLInfo.ImageBase, symbolName);

		if (st == MH_GAMESYMBOL_SYMBOL_NOT_FOUND)
		{
			// Only the first index may legitimately be absent as an enumeration end;
			// without index 0 the required patch set is missing.
			if (index == 0)
				ReportSymbolFailure(symbolName, st);

			return;
		}

		if (st != MH_GAMESYMBOL_OK)
		{
			ReportSymbolFailure(symbolName, st);
			return;
		}

		PVOID instructionAddress = ResolveGameSymbolOrError(symbolName, MH_GAMESYMBOL_KIND_PATCH);

		if (!instructionAddress)
			return;

		g_Sys_InitMemory_HeapLimitPatches.push_back({ symbolName, instructionAddress });
	}
}

// Decode exactly one instruction at the gamedata-provided address and return the
// address of its DWORD immediate. Nothing is searched or re-located: a decode
// failure or an unexpected layout is fatal.
static PVOID FindHeapLimitImmediate(PVOID instructionAddress, const char* symbolName)
{
	PVOID immediateAddress = NULL;

	g_pMetaHookAPI->DisasmSingleInstruction(instructionAddress, [](void* inst, PUCHAR address, size_t instLen, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto out = (PVOID*)context;
		const auto& x86 = pinst->detail->x86;

		if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_CMP) &&
			x86.op_count == 2 &&
			x86.operands[1].type == X86_OP_IMM &&
			x86.encoding.imm_size == sizeof(DWORD) &&
			(size_t)x86.encoding.imm_offset + sizeof(DWORD) <= instLen)
		{
			*out = address + x86.encoding.imm_offset;
		}

		}, &immediateAddress);

	if (!immediateAddress)
	{
		Sys_Error("\"%s\" at 0x%p is not a MOV/CMP with a DWORD immediate", symbolName, instructionAddress);
	}

	return immediateAddress;
}

void Engine_InstallHooks()
{
	auto HeapLimitOverride = 256;
	DWORD HeapLimitOverrideInBytes = (DWORD)HeapLimitOverride * 1024 * 1024;

	const char* pszHeapLimitOverride = NULL;
	if (gEngfuncs.CheckParm("-heaplimit_override", &pszHeapLimitOverride) &&
		pszHeapLimitOverride &&
		pszHeapLimitOverride[0])
	{
		HeapLimitOverride = atoi(pszHeapLimitOverride);
		HeapLimitOverride = max(min(HeapLimitOverride, 1024), 32);

		HeapLimitOverrideInBytes = (DWORD)HeapLimitOverride * 1024 * 1024;
	}

	for (const auto& patch : g_Sys_InitMemory_HeapLimitPatches)
	{
		PVOID immediateAddress = FindHeapLimitImmediate(patch.instructionAddress, patch.symbolName.c_str());

		if (!immediateAddress)
			return;

		g_pMetaHookAPI->WriteDWORD(immediateAddress, HeapLimitOverrideInBytes);
	}
}

void Engine_UninstallHooks()
{

}
