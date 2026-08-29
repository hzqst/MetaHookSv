#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <metahook.h>

// GameData owns the local gamedata catalog, the signature parser, module
// source/hash cache, and the public game symbol query API implementation.
// The catalog is built once and frozen; all returned string/pattern pointers
// remain valid until process exit.

namespace GameData
{
	// Build (and freeze) the read-only catalog from <gamedataRoot>\index.json.
	// Returns false and leaves the catalog unavailable when index.json is
	// missing, malformed, or has an unsupported schema. Per-snapshot failures
	// (missing file, size/hash mismatch, bad schema, malformed records) are
	// isolated and recorded as internal diagnostics without failing the whole
	// catalog.
	bool Initialize(const char* gamedataRoot);

	// Whether the catalog is available for queries.
	bool IsAvailable();

	// Query normalized symbol metadata by (moduleCRC64, symbolName).
	// The caller must initialize outSymbol->cbSize to sizeof(mh_gamesymbol_t).
	mh_gamesymbol_status_t QueryByCRC64(uint64_t moduleCRC64, const char* symbolName, mh_gamesymbol_t* outSymbol);

	// Register the on-disk source file for a module base. Used by the launcher
	// for blob engines whose in-memory image is not backed by a normal PE file.
	// Must be called before any query that resolves moduleBase.
	void RegisterModuleFileSource(PVOID moduleBase, const char* filePath, ULONG imageSize);

	// Register mirrorBase as an alias of realBase so both resolve to the same
	// module identity (and therefore the same CRC-64/XZ and hash cache).
	void RegisterMirrorAlias(PVOID mirrorBase, PVOID realBase);

	// Lazily compute (and cache) the CRC-64/XZ of the original module file
	// backing moduleBase. The first call performs file I/O; later calls use the
	// cached value or failure status. Concurrent first queries hash once.
	mh_gamesymbol_status_t GetModuleCRC64(PVOID moduleBase, uint64_t* outCRC64);
}

// Public game symbol API entry points (implemented in GameData.cpp).
mh_gamesymbol_status_t MH_GetModuleCRC64(PVOID moduleBase, uint64_t* outCRC64);
mh_gamesymbol_status_t MH_QueryGameSymbol(PVOID moduleBase, const char* symbolName, mh_gamesymbol_t* outSymbol);
mh_gamesymbol_status_t MH_QueryGameSymbolByCRC64(uint64_t moduleCRC64, const char* symbolName, mh_gamesymbol_t* outSymbol);
mh_gamesymbol_status_t MH_ResolveGameSymbol(PVOID moduleBase, const char* symbolName, mh_gamesymbol_kind_t expectedKind, PVOID* outAddress);
PVOID MH_SearchPatternMasked(PVOID searchBase, DWORD searchLength, const BYTE* patternBytes, const BYTE* patternMask, DWORD patternLength);
const char* MH_GetGameSymbolStatusString(mh_gamesymbol_status_t status);

#endif
