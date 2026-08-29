// Include Chocobo1Hash before metahook.h (which pulls in <windows.h>): the
// Windows min/max macros would otherwise clobber std::min/std::max used inside
// these headers.
#include <crc_64_xz.h>
#include <sha2_256.h>

#include "GameData.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	// -----------------------------------------------------------------------
	// Internal record / module / catalog model (stable heap-owned storage).
	// -----------------------------------------------------------------------

	struct GameSymbolRecord
	{
		mh_gamesymbol_kind_t kind = MH_GAMESYMBOL_KIND_UNKNOWN;
		DWORD flags = 0;
		DWORD rva = 0;
		DWORD symbolSize = 0;
		DWORD signatureRva = 0;

		std::string signatureText;
		std::vector<BYTE> signatureBytes;
		std::vector<BYTE> signatureMask;
		std::vector<char> legacyPattern;   // empty => not generatable (literal 0x2A)

		DWORD instructionOffset = 0;
		DWORD operandOffset = 0;
		DWORD instructionLength = 0;

		bool conflicted = false;   // duplicate key with differing content
		bool unsupportedKind = false; // kind not typed by this API version
	};

	struct ModuleCatalog
	{
		uint64_t crc64 = 0;
		std::string moduleName;
		std::string gameVersion;
		std::string sourceSnapshot;
		std::unordered_map<std::string, std::unique_ptr<GameSymbolRecord>> symbols;
	};

	struct GameDataCatalog
	{
		std::string gamedataRoot;
		bool available = false;
		std::unordered_map<uint64_t, std::unique_ptr<ModuleCatalog>> modules;
		std::vector<std::string> diagnostics;
	};

	GameDataCatalog g_catalog;

	void AddDiagnostic(const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		g_catalog.diagnostics.push_back(buf);
	}

	// -----------------------------------------------------------------------
	// Hex helpers.
	// -----------------------------------------------------------------------

	int HexDigit(char c)
	{
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		return -1;
	}

	bool ParseHexU64(const char* s, uint64_t& out)
	{
		if (!s || !*s) return false;

		const char* p = s;
		if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
			p += 2;
		if (!*p) return false;

		uint64_t v = 0;
		for (; *p; ++p)
		{
			int d = HexDigit(*p);
			if (d < 0) return false;
			if (v > (UINT64_MAX - (uint64_t)d) / 16) return false; // overflow
			v = v * 16 + (uint64_t)d;
		}
		out = v;
		return true;
	}

	bool ParseHexU32(const char* s, DWORD& out)
	{
		uint64_t v;
		if (!ParseHexU64(s, v)) return false;
		if (v > UINT32_MAX) return false;
		out = (DWORD)v;
		return true;
	}

	bool IsLowerHex(const std::string& s, size_t len)
	{
		if (s.size() != len) return false;
		for (char c : s)
		{
			if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
				return false;
		}
		return true;
	}

	bool IsSafeRelativeFilename(const std::string& s)
	{
		if (s.empty()) return false;
		if (s == "." || s == "..") return false;
		for (char c : s)
		{
			if (c == '/' || c == '\\' || c == ':') return false;
			if ((unsigned char)c < 0x20) return false;
		}
		return true;
	}

	std::string JoinPath(const std::string& root, const std::string& name)
	{
		if (root.empty()) return name;
		if (root.back() == '\\' || root.back() == '/')
			return root + name;
		return root + "\\" + name;
	}

	// -----------------------------------------------------------------------
	// File + hash helpers.
	// -----------------------------------------------------------------------

	bool ReadFileBytes(const std::string& path, std::vector<char>& out)
	{
		std::ifstream f(path, std::ios::binary);
		if (!f) return false;

		f.seekg(0, std::ios::end);
		std::streamoff end = f.tellg();
		if (end < 0) return false;
		f.seekg(0, std::ios::beg);

		out.resize((size_t)end);
		if (end > 0)
			f.read(out.data(), end);
		return f.good() || f.eof();
	}

	std::string ComputeSha256Hex(const void* data, size_t size)
	{
		Chocobo1::SHA2_256 hasher;
		hasher.addData(data, size);
		hasher.finalize();
		auto digest = hasher.toArray();

		static const char hexdigits[] = "0123456789abcdef";
		std::string hex;
		hex.reserve(digest.size() * 2);
		for (BYTE b : digest)
		{
			hex.push_back(hexdigits[b >> 4]);
			hex.push_back(hexdigits[b & 0x0F]);
		}
		return hex;
	}

	// -----------------------------------------------------------------------
	// RapidJSON helpers.
	// -----------------------------------------------------------------------

	const rapidjson::Value* FindMember(const rapidjson::Value& obj, const char* name)
	{
		auto it = obj.FindMember(name);
		return (it != obj.MemberEnd()) ? &it->value : nullptr;
	}

	const rapidjson::Value* FindMember(const rapidjson::Value& obj, const std::string& name)
	{
		auto it = obj.FindMember(name.c_str());
		return (it != obj.MemberEnd()) ? &it->value : nullptr;
	}

	bool GetSizeField(const rapidjson::Value& v, uint64_t& out)
	{
		if (v.IsUint64()) { out = v.GetUint64(); return true; }
		if (v.IsUint()) { out = v.GetUint(); return true; }
		if (v.IsInt64() && v.GetInt64() >= 0) { out = (uint64_t)v.GetInt64(); return true; }
		if (v.IsInt() && v.GetInt() >= 0) { out = (uint64_t)v.GetInt(); return true; }
		return false;
	}

	// -----------------------------------------------------------------------
	// Signature parser: whitespace-separated 2-hex bytes and "??" wildcards.
	// -----------------------------------------------------------------------

	bool ParseSignature(const std::string& text,
		std::vector<BYTE>& bytes,
		std::vector<BYTE>& mask,
		std::vector<char>& legacyPattern,
		std::string& error)
	{
		bytes.clear();
		mask.clear();
		legacyPattern.clear();

		if (text.empty())
		{
			error = "signature is empty";
			return false;
		}

		bool hasLiteral2A = false;
		std::vector<char> legacy;

		size_t i = 0;
		const size_t n = text.size();
		while (i < n)
		{
			while (i < n && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n'))
				++i;
			if (i >= n)
				break;

			size_t start = i;
			while (i < n && !(text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n'))
				++i;
			std::string token = text.substr(start, i - start);

			if (token == "??")
			{
				bytes.push_back(0x00);
				mask.push_back(0x00);
				legacy.push_back((char)0x2A);
			}
			else if (token.size() == 2)
			{
				int hi = HexDigit(token[0]);
				int lo = HexDigit(token[1]);
				if (hi < 0 || lo < 0)
				{
					error = "invalid signature token '" + token + "'";
					return false;
				}
				BYTE b = (BYTE)((hi << 4) | lo);
				bytes.push_back(b);
				mask.push_back(0xFF);
				if (b == 0x2A)
					hasLiteral2A = true;
				legacy.push_back((char)b);
			}
			else
			{
				error = "invalid signature token '" + token + "'";
				return false;
			}
		}

		if (bytes.empty())
		{
			error = "signature is empty";
			return false;
		}

		if (!hasLiteral2A)
			legacyPattern = legacy;

		return true;
	}

	// -----------------------------------------------------------------------
	// Payload normalization.
	// -----------------------------------------------------------------------

	bool NormalizeFunction(const rapidjson::Value& payload, GameSymbolRecord& rec, std::string& error)
	{
		const rapidjson::Value* funcRva = FindMember(payload, "func_rva");
		const rapidjson::Value* funcSize = FindMember(payload, "func_size");
		const rapidjson::Value* funcSig = FindMember(payload, "func_sig");
		if (!funcRva || !funcRva->IsString() || !funcSize || !funcSize->IsString() || !funcSig || !funcSig->IsString())
		{
			error = "function payload is missing func_rva/func_size/func_sig";
			return false;
		}

		if (!ParseHexU32(funcRva->GetString(), rec.rva))
		{
			error = "invalid func_rva";
			return false;
		}
		if (!ParseHexU32(funcSize->GetString(), rec.symbolSize))
		{
			error = "invalid func_size";
			return false;
		}
		if (!ParseSignature(funcSig->GetString(), rec.signatureBytes, rec.signatureMask, rec.legacyPattern, error))
			return false;

		rec.kind = MH_GAMESYMBOL_KIND_FUNCTION;
		rec.signatureText = funcSig->GetString();
		rec.signatureRva = rec.rva;
		rec.instructionOffset = 0;
		rec.operandOffset = 0;
		rec.instructionLength = 0;
		rec.flags = 0;

		const rapidjson::Value* allowAcross = FindMember(payload, "func_sig_allow_across_function_boundary");
		if (allowAcross && allowAcross->IsBool() && allowAcross->GetBool())
			rec.flags |= MH_GAMESYMBOL_FLAG_SIGNATURE_ALLOW_ACROSS_FUNCTION_BOUNDARY;

		return true;
	}

	bool NormalizeGlobal(const rapidjson::Value& payload, GameSymbolRecord& rec, std::string& error)
	{
		const rapidjson::Value* gvRva = FindMember(payload, "gv_rva");
		const rapidjson::Value* gvSig = FindMember(payload, "gv_sig");
		const rapidjson::Value* gvSigVa = FindMember(payload, "gv_sig_va");
		const rapidjson::Value* gvVa = FindMember(payload, "gv_va");
		const rapidjson::Value* gvInstOffset = FindMember(payload, "gv_inst_offset");
		const rapidjson::Value* gvInstDisp = FindMember(payload, "gv_inst_disp");
		const rapidjson::Value* gvInstLength = FindMember(payload, "gv_inst_length");

		if (!gvRva || !gvRva->IsString() || !gvSig || !gvSig->IsString() ||
			!gvSigVa || !gvSigVa->IsString() || !gvVa || !gvVa->IsString() ||
			!gvInstOffset || !gvInstOffset->IsString() ||
			!gvInstDisp || !gvInstDisp->IsString() ||
			!gvInstLength || !gvInstLength->IsString())
		{
			error = "global payload is missing a required gv_* field";
			return false;
		}

		DWORD rva, sigVa, va, instOffset, instDisp, instLength;
		if (!ParseHexU32(gvRva->GetString(), rva)) { error = "invalid gv_rva"; return false; }
		if (!ParseHexU32(gvSigVa->GetString(), sigVa)) { error = "invalid gv_sig_va"; return false; }
		if (!ParseHexU32(gvVa->GetString(), va)) { error = "invalid gv_va"; return false; }
		if (!ParseHexU32(gvInstOffset->GetString(), instOffset)) { error = "invalid gv_inst_offset"; return false; }
		if (!ParseHexU32(gvInstDisp->GetString(), instDisp)) { error = "invalid gv_inst_disp"; return false; }
		if (!ParseHexU32(gvInstLength->GetString(), instLength)) { error = "invalid gv_inst_length"; return false; }

		if (va < rva) { error = "gv_va is smaller than gv_rva"; return false; }
		DWORD imageBase = va - rva;
		if (sigVa < imageBase) { error = "gv_sig_va is smaller than derived image base"; return false; }

		if (!ParseSignature(gvSig->GetString(), rec.signatureBytes, rec.signatureMask, rec.legacyPattern, error))
			return false;

		rec.kind = MH_GAMESYMBOL_KIND_GLOBAL;
		rec.rva = rva;
		rec.symbolSize = 0;
		rec.signatureRva = sigVa - imageBase;
		rec.signatureText = gvSig->GetString();
		rec.instructionOffset = instOffset;
		rec.operandOffset = instDisp;
		rec.instructionLength = instLength;
		rec.flags = 0;
		return true;
	}

	// -----------------------------------------------------------------------
	// Catalog assembly.
	// -----------------------------------------------------------------------

	ModuleCatalog& GetOrCreateModule(uint64_t crc64, const std::string& moduleName,
		const std::string& gameVersion, const std::string& snapshot)
	{
		auto& ptr = g_catalog.modules[crc64];
		if (!ptr)
		{
			ptr = std::make_unique<ModuleCatalog>();
			ptr->crc64 = crc64;
			ptr->moduleName = moduleName;
			ptr->gameVersion = gameVersion;
			ptr->sourceSnapshot = snapshot;
		}
		else if (ptr->moduleName != moduleName)
		{
			AddDiagnostic("crc64 %016llx is referenced by both module '%s' and '%s'",
				(unsigned long long)crc64, ptr->moduleName.c_str(), moduleName.c_str());
		}
		return *ptr;
	}

	bool RecordsIdentical(const GameSymbolRecord& a, const GameSymbolRecord& b)
	{
		return a.kind == b.kind &&
			a.flags == b.flags &&
			a.rva == b.rva &&
			a.symbolSize == b.symbolSize &&
			a.signatureRva == b.signatureRva &&
			a.signatureText == b.signatureText &&
			a.signatureBytes == b.signatureBytes &&
			a.signatureMask == b.signatureMask &&
			a.legacyPattern == b.legacyPattern &&
			a.instructionOffset == b.instructionOffset &&
			a.operandOffset == b.operandOffset &&
			a.instructionLength == b.instructionLength &&
			a.unsupportedKind == b.unsupportedKind;
	}

	void AddRecord(uint64_t crc64, const std::string& moduleName, const std::string& gameVersion,
		const std::string& snapshot, const std::string& symbolName, GameSymbolRecord&& rec)
	{
		ModuleCatalog& mc = GetOrCreateModule(crc64, moduleName, gameVersion, snapshot);

		auto it = mc.symbols.find(symbolName);
		if (it != mc.symbols.end())
		{
			if (!RecordsIdentical(*it->second, rec))
			{
				it->second->conflicted = true;
				AddDiagnostic("symbol '%s' (crc64 %016llx, %s) has conflicting duplicate records",
					symbolName.c_str(), (unsigned long long)crc64, gameVersion.c_str());
			}
			// identical duplicate: drop silently.
			return;
		}

		mc.symbols[symbolName] = std::make_unique<GameSymbolRecord>(std::move(rec));
	}

	void LoadSnapshot(const std::string& root, const rapidjson::Value& version)
	{
		const char* gameVersion = version["gameVersion"].GetString();
		const char* url = version["url"].GetString();
		const char* expectedSha256 = version["sha256"].GetString();

		uint64_t expectedSize = 0;
		if (!GetSizeField(version["size"], expectedSize))
		{
			AddDiagnostic("snapshot '%s': invalid size", gameVersion);
			return;
		}

		std::string path = JoinPath(root, url);

		std::vector<char> data;
		if (!ReadFileBytes(path, data))
		{
			AddDiagnostic("snapshot '%s': file missing or unreadable: %s", gameVersion, path.c_str());
			return;
		}

		if (data.size() != expectedSize)
		{
			AddDiagnostic("snapshot '%s': size mismatch (expected %llu, got %zu)",
				gameVersion, (unsigned long long)expectedSize, data.size());
			return;
		}

		std::string sha256 = ComputeSha256Hex(data.data(), data.size());
		if (sha256 != expectedSha256)
		{
			AddDiagnostic("snapshot '%s': sha256 mismatch (expected %s, got %s)",
				gameVersion, expectedSha256, sha256.c_str());
			return;
		}

		rapidjson::Document doc;
		if (doc.Parse(data.data(), data.size()).HasParseError())
		{
			AddDiagnostic("snapshot '%s': json parse error at offset %zu",
				gameVersion, doc.GetErrorOffset());
			return;
		}

		if (!doc.IsObject())
		{
			AddDiagnostic("snapshot '%s': root is not an object", gameVersion);
			return;
		}

		const rapidjson::Value* schemaVersion = FindMember(doc, "schemaVersion");
		if (!schemaVersion || !schemaVersion->IsInt() || schemaVersion->GetInt() != 3)
		{
			AddDiagnostic("snapshot '%s': unsupported schemaVersion", gameVersion);
			return;
		}

		const rapidjson::Value* source = FindMember(doc, "source");
		const rapidjson::Value* sourceSchema = source ? FindMember(*source, "snapshotSchemaVersion") : nullptr;
		if (!sourceSchema || !sourceSchema->IsInt() || sourceSchema->GetInt() != 6)
		{
			AddDiagnostic("snapshot '%s': unsupported source.snapshotSchemaVersion", gameVersion);
			return;
		}

		const rapidjson::Value* binaries = FindMember(doc, "binaries");
		if (!binaries || !binaries->IsObject())
		{
			AddDiagnostic("snapshot '%s': missing binaries object", gameVersion);
			return;
		}

		std::unordered_map<std::string, uint64_t> moduleCrc64;
		for (auto it = binaries->MemberBegin(); it != binaries->MemberEnd(); ++it)
		{
			const char* moduleName = it->name.GetString();
			const rapidjson::Value* win = FindMember(it->value, "windows");
			if (!win || !win->IsObject())
				continue;
			const rapidjson::Value* crc64 = FindMember(*win, "crc64");
			if (!crc64 || !crc64->IsString())
				continue;

			uint64_t v;
			if (!ParseHexU64(crc64->GetString(), v))
			{
				AddDiagnostic("snapshot '%s': invalid crc64 for module '%s'", gameVersion, moduleName);
				continue;
			}
			moduleCrc64[moduleName] = v;
		}

		const rapidjson::Value* records = FindMember(doc, "records");
		if (!records || !records->IsArray())
		{
			AddDiagnostic("snapshot '%s': missing records array", gameVersion);
			return;
		}

		for (const auto& rec : records->GetArray())
		{
			if (!rec.IsObject())
				continue;

			const rapidjson::Value* platform = FindMember(rec, "platform");
			if (!platform || !platform->IsString() || std::strcmp(platform->GetString(), "windows") != 0)
				continue;

			const rapidjson::Value* module = FindMember(rec, "module");
			const rapidjson::Value* symbolName = FindMember(rec, "symbolName");
			const rapidjson::Value* kind = FindMember(rec, "kind");
			if (!module || !module->IsString() || !symbolName || !symbolName->IsString() ||
				!kind || !kind->IsString())
				continue;

			auto crcIt = moduleCrc64.find(module->GetString());
			if (crcIt == moduleCrc64.end())
			{
				AddDiagnostic("snapshot '%s': record '%s' references unknown module '%s'",
					gameVersion, symbolName->GetString(), module->GetString());
				continue;
			}

			const char* kindStr = kind->GetString();
			const rapidjson::Value* payload = FindMember(rec, "payload");

			GameSymbolRecord record;
			std::string error;
			if (std::strcmp(kindStr, "function") == 0)
			{
				if (!payload || !payload->IsObject() || !NormalizeFunction(*payload, record, error))
				{
					AddDiagnostic("snapshot '%s': symbol '%s': %s", gameVersion, symbolName->GetString(), error.c_str());
					continue;
				}
			}
			else if (std::strcmp(kindStr, "global") == 0)
			{
				if (!payload || !payload->IsObject() || !NormalizeGlobal(*payload, record, error))
				{
					AddDiagnostic("snapshot '%s': symbol '%s': %s", gameVersion, symbolName->GetString(), error.c_str());
					continue;
				}
			}
			else
			{
				record.kind = MH_GAMESYMBOL_KIND_UNKNOWN;
				record.unsupportedKind = true;
			}

			AddRecord(crcIt->second, module->GetString(), gameVersion, url, symbolName->GetString(), std::move(record));
		}
	}

	bool ValidateIndex(const rapidjson::Value& index)
	{
		const rapidjson::Value* schemaVersion = FindMember(index, "schemaVersion");
		if (!schemaVersion || !schemaVersion->IsInt() || schemaVersion->GetInt() != 4)
		{
			AddDiagnostic("index.json: unsupported schemaVersion (expected 4)");
			return false;
		}

		const rapidjson::Value* versions = FindMember(index, "versions");
		if (!versions || !versions->IsArray())
		{
			AddDiagnostic("index.json: 'versions' must be an array");
			return false;
		}

		std::unordered_map<std::string, bool> seen;
		for (const auto& v : versions->GetArray())
		{
			if (!v.IsObject())
			{
				AddDiagnostic("index.json: a versions entry is not an object");
				return false;
			}

			const rapidjson::Value* gameVersion = FindMember(v, "gameVersion");
			const rapidjson::Value* url = FindMember(v, "url");
			const rapidjson::Value* sha256 = FindMember(v, "sha256");
			const rapidjson::Value* snapshotSchema = FindMember(v, "snapshotSchemaVersion");

			if (!gameVersion || !gameVersion->IsString() || gameVersion->GetStringLength() == 0 ||
				!url || !url->IsString() ||
				!sha256 || !sha256->IsString() ||
				!snapshotSchema || !snapshotSchema->IsInt())
			{
				AddDiagnostic("index.json: a versions entry is missing gameVersion/url/sha256/snapshotSchemaVersion");
				return false;
			}

			std::string gv = gameVersion->GetString();
			if (seen.count(gv))
			{
				AddDiagnostic("index.json: duplicate gameVersion '%s'", gv.c_str());
				return false;
			}
			seen[gv] = true;

			std::string urlStr = url->GetString();
			if (!IsSafeRelativeFilename(urlStr))
			{
				AddDiagnostic("index.json: unsafe url '%s' for '%s'", urlStr.c_str(), gv.c_str());
				return false;
			}
			if (!IsLowerHex(sha256->GetString(), 64))
			{
				AddDiagnostic("index.json: invalid sha256 for '%s'", gv.c_str());
				return false;
			}

			uint64_t size = 0;
			if (!GetSizeField(v["size"], size))
			{
				AddDiagnostic("index.json: invalid size for '%s'", gv.c_str());
				return false;
			}
		}

		return true;
	}

	void FillSymbol(const GameSymbolRecord& rec, uint64_t crc64, mh_gamesymbol_t* out)
	{
		out->kind = rec.kind;
		out->flags = rec.flags;
		out->moduleCRC64 = crc64;
		out->rva = rec.rva;
		out->symbolSize = rec.symbolSize;
		out->signatureRva = rec.signatureRva;
		out->signature.text = rec.signatureText.c_str();
		out->signature.bytes = rec.signatureBytes.empty() ? nullptr : rec.signatureBytes.data();
		out->signature.mask = rec.signatureMask.empty() ? nullptr : rec.signatureMask.data();
		out->signature.legacyPattern = rec.legacyPattern.empty() ? nullptr : rec.legacyPattern.data();
		out->signature.length = (DWORD)rec.signatureBytes.size();
		out->instructionOffset = rec.instructionOffset;
		out->operandOffset = rec.operandOffset;
		out->instructionLength = rec.instructionLength;
	}
}

namespace GameData
{
	bool Initialize(const char* gamedataRoot)
	{
		g_catalog = GameDataCatalog{};
		g_catalog.gamedataRoot = gamedataRoot ? gamedataRoot : "";

		if (g_catalog.gamedataRoot.empty())
		{
			AddDiagnostic("gamedata root is empty");
			return false;
		}

		std::string indexPath = JoinPath(g_catalog.gamedataRoot, "index.json");

		std::vector<char> data;
		if (!ReadFileBytes(indexPath, data))
		{
			AddDiagnostic("index.json missing or unreadable: %s", indexPath.c_str());
			return false;
		}

		rapidjson::Document index;
		if (index.Parse(data.data(), data.size()).HasParseError())
		{
			AddDiagnostic("index.json parse error at offset %zu", index.GetErrorOffset());
			return false;
		}

		if (!index.IsObject())
		{
			AddDiagnostic("index.json root is not an object");
			return false;
		}

		if (!ValidateIndex(index))
			return false;

		const rapidjson::Value& versions = index["versions"];
		for (const auto& v : versions.GetArray())
			LoadSnapshot(g_catalog.gamedataRoot, v);

		g_catalog.available = true;
		return true;
	}

	bool IsAvailable()
	{
		return g_catalog.available;
	}

	mh_gamesymbol_status_t QueryByCRC64(uint64_t moduleCRC64, const char* symbolName, mh_gamesymbol_t* outSymbol)
	{
		if (!outSymbol || !symbolName || !*symbolName)
			return MH_GAMESYMBOL_INVALID_ARGUMENT;

		DWORD cbSize = outSymbol->cbSize;
		if (cbSize < sizeof(mh_gamesymbol_t))
			return MH_GAMESYMBOL_OUTPUT_TOO_SMALL;

		// Zero the output (preserving cbSize) so any failure leaves a clean struct.
		memset(outSymbol, 0, cbSize);
		outSymbol->cbSize = cbSize;

		if (!g_catalog.available)
			return MH_GAMESYMBOL_GAMEDATA_UNAVAILABLE;

		auto mit = g_catalog.modules.find(moduleCRC64);
		if (mit == g_catalog.modules.end())
			return MH_GAMESYMBOL_MODULE_NOT_FOUND;

		ModuleCatalog& mc = *mit->second;
		auto sit = mc.symbols.find(symbolName);
		if (sit == mc.symbols.end())
			return MH_GAMESYMBOL_SYMBOL_NOT_FOUND;

		GameSymbolRecord& rec = *sit->second;
		if (rec.conflicted)
			return MH_GAMESYMBOL_CATALOG_CONFLICT;
		if (rec.unsupportedKind)
			return MH_GAMESYMBOL_UNSUPPORTED_KIND;

		FillSymbol(rec, moduleCRC64, outSymbol);
		return MH_GAMESYMBOL_OK;
	}
}
