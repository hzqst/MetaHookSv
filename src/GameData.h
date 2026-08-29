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
}

#endif
