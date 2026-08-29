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
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <sys/stat.h>
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
		if (!obj.IsObject())
			return nullptr;
		auto it = obj.FindMember(name);
		return (it != obj.MemberEnd()) ? &it->value : nullptr;
	}

	const rapidjson::Value* FindMember(const rapidjson::Value& obj, const std::string& name)
	{
		if (!obj.IsObject())
			return nullptr;
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
		const rapidjson::Value* gameVersionValue = FindMember(version, "gameVersion");
		const rapidjson::Value* urlValue = FindMember(version, "url");
		const rapidjson::Value* expectedSha256Value = FindMember(version, "sha256");
		const rapidjson::Value* sizeValue = FindMember(version, "size");
		if (!gameVersionValue || !gameVersionValue->IsString() ||
			!urlValue || !urlValue->IsString() ||
			!expectedSha256Value || !expectedSha256Value->IsString() ||
			!sizeValue)
		{
			AddDiagnostic("snapshot index entry is missing gameVersion/url/sha256/size");
			return;
		}

		const char* gameVersion = gameVersionValue->GetString();
		const char* url = urlValue->GetString();
		const char* expectedSha256 = expectedSha256Value->GetString();

		uint64_t expectedSize = 0;
		if (!GetSizeField(*sizeValue, expectedSize))
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
		if (!source || !source->IsObject())
		{
			AddDiagnostic("snapshot '%s': missing source object", gameVersion);
			return;
		}
		const rapidjson::Value* sourceSchema = FindMember(*source, "snapshotSchemaVersion");
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
			if (!it->value.IsObject())
			{
				AddDiagnostic("snapshot '%s': binaries.%s is not an object", gameVersion, moduleName);
				continue;
			}
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
			{
				AddDiagnostic("snapshot '%s': a records entry is not an object", gameVersion);
				continue;
			}

			const rapidjson::Value* platform = FindMember(rec, "platform");
			if (!platform || !platform->IsString())
			{
				AddDiagnostic("snapshot '%s': a records entry is missing platform", gameVersion);
				continue;
			}
			if (std::strcmp(platform->GetString(), "windows") != 0)
				continue;

			const rapidjson::Value* module = FindMember(rec, "module");
			const rapidjson::Value* symbolName = FindMember(rec, "symbolName");
			const rapidjson::Value* kind = FindMember(rec, "kind");
			if (!module || !module->IsString() || !symbolName || !symbolName->IsString() ||
				!kind || !kind->IsString())
			{
				AddDiagnostic("snapshot '%s': a windows records entry is missing module/symbolName/kind", gameVersion);
				continue;
			}

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
					if (error.empty())
						error = "function payload must be an object";
					AddDiagnostic("snapshot '%s': symbol '%s': %s", gameVersion, symbolName->GetString(), error.c_str());
					continue;
				}
			}
			else if (std::strcmp(kindStr, "global") == 0)
			{
				if (!payload || !payload->IsObject() || !NormalizeGlobal(*payload, record, error))
				{
					if (error.empty())
						error = "global payload must be an object";
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
		if (!index.IsObject())
		{
			AddDiagnostic("index.json root is not an object");
			return false;
		}

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
			const rapidjson::Value* sizeValue = FindMember(v, "size");
			if (!sizeValue || !GetSizeField(*sizeValue, size))
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

	// -----------------------------------------------------------------------
	// Module identity + lazy CRC-64/XZ hash cache.
	// -----------------------------------------------------------------------

	enum class ModuleSourceType { None, PeFile, BlobFile };
	enum class ModuleHashState { Uncomputed, Computing, Ready, Failed };

	struct ModuleIdentity
	{
		PVOID moduleBase = nullptr;
		ULONG imageSize = 0;
		ModuleSourceType sourceType = ModuleSourceType::None;
		std::wstring sourcePath;

		std::mutex mutex;
		std::condition_variable cv;
		ModuleHashState state = ModuleHashState::Uncomputed;
		uint64_t crc64 = 0;
		mh_gamesymbol_status_t failureStatus = MH_GAMESYMBOL_OK;
	};

	std::mutex g_modulesMutex;
	std::unordered_map<PVOID, std::shared_ptr<ModuleIdentity>> g_modules;

	std::wstring AnsiToWide(const char* s)
	{
		if (!s || !*s)
			return std::wstring();
		int len = MultiByteToWideChar(CP_ACP, 0, s, -1, nullptr, 0);
		if (len <= 0)
			return std::wstring();
		std::wstring out(len, L'\0');
		MultiByteToWideChar(CP_ACP, 0, s, -1, &out[0], len);
		out.resize(len - 1); // drop the terminating null
		return out;
	}

	bool GetModuleFilePathW(PVOID moduleBase, std::wstring& out)
	{
		std::wstring buf(MAX_PATH, L'\0');
		DWORD len = GetModuleFileNameW((HMODULE)moduleBase, &buf[0], (DWORD)buf.size());
		while (len == buf.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			buf.resize(buf.size() * 2);
			len = GetModuleFileNameW((HMODULE)moduleBase, &buf[0], (DWORD)buf.size());
		}
		if (len == 0)
		{
			out.clear();
			return false;
		}
		buf.resize(len);
		out = buf;
		return true;
	}

	ULONG GetPeImageSize(PVOID moduleBase)
	{
		auto dos = (PIMAGE_DOS_HEADER)moduleBase;
		if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
			return 0;
		auto nt = (PIMAGE_NT_HEADERS)((BYTE*)moduleBase + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE)
			return 0;
		return nt->OptionalHeader.SizeOfImage;
	}

	std::shared_ptr<ModuleIdentity> GetOrCreateModuleIdentity(PVOID moduleBase)
	{
		std::lock_guard<std::mutex> lock(g_modulesMutex);
		auto it = g_modules.find(moduleBase);
		if (it != g_modules.end())
			return it->second;

		auto id = std::make_shared<ModuleIdentity>();
		id->moduleBase = moduleBase;

		// Ordinary PE: discover the on-disk file path and image size.
		if (GetModuleFilePathW(moduleBase, id->sourcePath) && !id->sourcePath.empty())
		{
			id->sourceType = ModuleSourceType::PeFile;
			id->imageSize = GetPeImageSize(moduleBase);
		}

		g_modules[moduleBase] = id;
		return id;
	}

	mh_gamesymbol_status_t ComputeCrc64FromFile(const std::wstring& path, uint64_t& out)
	{
		struct _stat64 before;
		if (_wstat64(path.c_str(), &before) != 0)
			return MH_GAMESYMBOL_MODULE_HASH_FAILED;

		HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return MH_GAMESYMBOL_MODULE_HASH_FAILED;

		Chocobo1::CRC_64_XZ crc;
		std::vector<BYTE> buf(1024 * 1024); // 1 MiB chunks, matching the exporter

		bool readOk = true;
		for (;;)
		{
			DWORD read = 0;
			if (!ReadFile(hFile, buf.data(), (DWORD)buf.size(), &read, NULL))
			{
				readOk = false;
				break;
			}
			if (read == 0)
				break;
			crc.addData(buf.data(), read);
			if (read < buf.size())
				break; // final partial chunk
		}
		CloseHandle(hFile);

		if (!readOk)
			return MH_GAMESYMBOL_MODULE_HASH_FAILED;

		// The file must not have changed while it was being read.
		struct _stat64 after;
		if (_wstat64(path.c_str(), &after) != 0 ||
			after.st_size != before.st_size ||
			after.st_mtime != before.st_mtime)
			return MH_GAMESYMBOL_MODULE_HASH_FAILED;

		crc.finalize();
		out = static_cast<uint64_t>(crc);
		return MH_GAMESYMBOL_OK;
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

		const rapidjson::Value* versions = FindMember(index, "versions");
		if (!versions || !versions->IsArray())
		{
			AddDiagnostic("index.json: 'versions' must be an array");
			return false;
		}
		for (const auto& v : versions->GetArray())
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
		memset(outSymbol, 0, sizeof(mh_gamesymbol_t));
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

	void RegisterModuleFileSource(PVOID moduleBase, const char* filePath, ULONG imageSize)
	{
		if (!moduleBase || !filePath || !*filePath)
			return;

		std::lock_guard<std::mutex> lock(g_modulesMutex);
		auto& id = g_modules[moduleBase];
		if (!id)
		{
			id = std::make_shared<ModuleIdentity>();
			id->moduleBase = moduleBase;
		}
		id->sourceType = ModuleSourceType::BlobFile;
		id->sourcePath = AnsiToWide(filePath);
		if (imageSize)
			id->imageSize = imageSize;
	}

	void RegisterMirrorAlias(PVOID mirrorBase, PVOID realBase)
	{
		if (!mirrorBase || !realBase)
			return;

		auto realId = GetOrCreateModuleIdentity(realBase);
		std::lock_guard<std::mutex> lock(g_modulesMutex);
		g_modules[mirrorBase] = realId;
	}

	mh_gamesymbol_status_t GetModuleCRC64(PVOID moduleBase, uint64_t* outCRC64)
	{
		if (!moduleBase || !outCRC64)
			return MH_GAMESYMBOL_INVALID_ARGUMENT;

		auto id = GetOrCreateModuleIdentity(moduleBase);

		{
			std::unique_lock<std::mutex> lock(id->mutex);
			if (id->state == ModuleHashState::Ready)
			{
				*outCRC64 = id->crc64;
				return MH_GAMESYMBOL_OK;
			}
			if (id->state == ModuleHashState::Failed)
				return id->failureStatus;
			if (id->state == ModuleHashState::Computing)
			{
				id->cv.wait(lock, [&] { return id->state == ModuleHashState::Ready || id->state == ModuleHashState::Failed; });
				if (id->state == ModuleHashState::Ready)
				{
					*outCRC64 = id->crc64;
					return MH_GAMESYMBOL_OK;
				}
				return id->failureStatus;
			}
			// Uncomputed: claim it and compute outside the lock.
			id->state = ModuleHashState::Computing;
		}

		mh_gamesymbol_status_t st;
		uint64_t crc64 = 0;
		if (id->sourceType == ModuleSourceType::None || id->sourcePath.empty())
			st = MH_GAMESYMBOL_MODULE_PATH_UNAVAILABLE;
		else
			st = ComputeCrc64FromFile(id->sourcePath, crc64);

		{
			std::lock_guard<std::mutex> lock(id->mutex);
			if (st == MH_GAMESYMBOL_OK)
			{
				id->crc64 = crc64;
				id->state = ModuleHashState::Ready;
			}
			else
			{
				id->failureStatus = st;
				id->state = ModuleHashState::Failed;
			}
		}
		id->cv.notify_all();

		if (st == MH_GAMESYMBOL_OK)
			*outCRC64 = crc64;
		return st;
	}
}

// ---------------------------------------------------------------------------
// Public game symbol API.
// ---------------------------------------------------------------------------

mh_gamesymbol_status_t MH_GetModuleCRC64(PVOID moduleBase, uint64_t* outCRC64)
{
	return GameData::GetModuleCRC64(moduleBase, outCRC64);
}

mh_gamesymbol_status_t MH_QueryGameSymbolByCRC64(uint64_t moduleCRC64, const char* symbolName, mh_gamesymbol_t* outSymbol)
{
	return GameData::QueryByCRC64(moduleCRC64, symbolName, outSymbol);
}

mh_gamesymbol_status_t MH_QueryGameSymbol(PVOID moduleBase, const char* symbolName, mh_gamesymbol_t* outSymbol)
{
	if (!outSymbol || !symbolName || !*symbolName)
		return MH_GAMESYMBOL_INVALID_ARGUMENT;

	DWORD cbSize = outSymbol->cbSize;
	if (cbSize < sizeof(mh_gamesymbol_t))
		return MH_GAMESYMBOL_OUTPUT_TOO_SMALL;

	// Zero the output (preserving cbSize) so failures leave a clean struct.
	memset(outSymbol, 0, sizeof(mh_gamesymbol_t));
	outSymbol->cbSize = cbSize;

	uint64_t crc64 = 0;
	mh_gamesymbol_status_t st = GameData::GetModuleCRC64(moduleBase, &crc64);
	if (st != MH_GAMESYMBOL_OK)
		return st;

	return GameData::QueryByCRC64(crc64, symbolName, outSymbol);
}

mh_gamesymbol_status_t MH_ResolveGameSymbol(PVOID moduleBase, const char* symbolName, mh_gamesymbol_kind_t expectedKind, PVOID* outAddress)
{
	if (!outAddress)
		return MH_GAMESYMBOL_INVALID_ARGUMENT;
	*outAddress = nullptr;

	if (!moduleBase || !symbolName || !*symbolName)
		return MH_GAMESYMBOL_INVALID_ARGUMENT;

	if (expectedKind != MH_GAMESYMBOL_KIND_FUNCTION && expectedKind != MH_GAMESYMBOL_KIND_GLOBAL)
		return MH_GAMESYMBOL_INVALID_ARGUMENT;

	mh_gamesymbol_t sym;
	sym.cbSize = sizeof(sym);
	mh_gamesymbol_status_t st = MH_QueryGameSymbol(moduleBase, symbolName, &sym);
	if (st != MH_GAMESYMBOL_OK)
		return st;

	if (sym.kind != expectedKind)
		return MH_GAMESYMBOL_KIND_MISMATCH;

	ULONG imageSize = GetOrCreateModuleIdentity(moduleBase)->imageSize;
	if (imageSize != 0)
	{
		if (sym.rva >= imageSize)
			return MH_GAMESYMBOL_RVA_OUT_OF_RANGE;
		if (sym.symbolSize > imageSize - sym.rva)
			return MH_GAMESYMBOL_RVA_OUT_OF_RANGE;
	}
	else if ((uint64_t)sym.rva + sym.symbolSize > UINT32_MAX)
	{
		return MH_GAMESYMBOL_RVA_OUT_OF_RANGE;
	}

	*outAddress = (PVOID)((BYTE*)moduleBase + sym.rva);
	return MH_GAMESYMBOL_OK;
}

PVOID MH_SearchPatternMasked(PVOID searchBase, DWORD searchLength, const BYTE* patternBytes, const BYTE* patternMask, DWORD patternLength)
{
	if (!searchBase || !patternBytes || !patternMask)
		return nullptr;
	if (searchLength == 0 || patternLength == 0 || patternLength > searchLength)
		return nullptr;

	const BYTE* base = (const BYTE*)searchBase;
	for (DWORD i = 0; i <= searchLength - patternLength; ++i)
	{
		bool match = true;
		for (DWORD j = 0; j < patternLength; ++j)
		{
			if (patternMask[j] != 0 && base[i + j] != patternBytes[j])
			{
				match = false;
				break;
			}
		}
		if (match)
			return (PVOID)(base + i);
	}
	return nullptr;
}

const char* MH_GetGameSymbolStatusString(mh_gamesymbol_status_t status)
{
	switch (status)
	{
	case MH_GAMESYMBOL_OK: return "success";
	case MH_GAMESYMBOL_INVALID_ARGUMENT: return "invalid argument";
	case MH_GAMESYMBOL_OUTPUT_TOO_SMALL: return "output buffer is too small";
	case MH_GAMESYMBOL_GAMEDATA_UNAVAILABLE: return "gamedata is not available";
	case MH_GAMESYMBOL_MODULE_PATH_UNAVAILABLE: return "module file path is unavailable";
	case MH_GAMESYMBOL_MODULE_HASH_FAILED: return "failed to hash the module file";
	case MH_GAMESYMBOL_MODULE_NOT_FOUND: return "module was not found in the gamedata";
	case MH_GAMESYMBOL_SYMBOL_NOT_FOUND: return "symbol was not found in the matched gamedata snapshot";
	case MH_GAMESYMBOL_UNSUPPORTED_KIND: return "symbol kind is not supported by this API version";
	case MH_GAMESYMBOL_KIND_MISMATCH: return "symbol kind does not match the expected kind";
	case MH_GAMESYMBOL_RVA_OUT_OF_RANGE: return "symbol rva is out of the module image bounds";
	case MH_GAMESYMBOL_CATALOG_CONFLICT: return "symbol has conflicting duplicate records";
	default: return "unknown game symbol status";
	}
}
