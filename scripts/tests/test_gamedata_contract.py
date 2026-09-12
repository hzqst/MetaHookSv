import hashlib
import importlib.util
from pathlib import Path
import unittest


SCRIPTS = Path(__file__).parents[1]


def load_module(name, filename):
    spec = importlib.util.spec_from_file_location(name, SCRIPTS / filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


validate = load_module("validate_gamedata", "validate-gamedata.py")
sync = load_module("sync_gamedata", "sync-gamedata.py")

SHA256_A = "a" * 64
SHA256_CONFIG = "sha256:" + "b" * 64


def make_snapshot(records=None, schema=5, contract=8, analysis=3, is_blob=True,
                  game_version="hl-8684"):
    records = records if records is not None else []
    return {
        "schemaVersion": schema,
        "source": {
            "gameVersion": game_version,
            "snapshotSchemaVersion": contract,
            "configDigestVersion": 2,
            "analysisOutputContractVersion": analysis,
            "configSha256": SHA256_CONFIG,
            "fileCount": len(records),
            "lastPublishTime": "2026-09-12T04:28:50Z",
        },
        "binaries": {
            "engine": {
                "windows": {
                    "crc64": "0011223344556677",
                    "size": 2123776,
                    "sha256": SHA256_A,
                    "isBlob": is_blob,
                }
            }
        },
        "records": records,
    }


def function_record(name="R_NewMap"):
    return {
        "platform": "windows",
        "module": "engine",
        "symbolName": name,
        "kind": "function",
        "payload": {
            "func_rva": "0x1000",
            "func_size": "0x10",
            "func_sig": "55 8B EC",
        },
    }


def scalar_record(name="size_of_frame", value=17080, module="engine",
                  payload_name=None, payload_value=None):
    return {
        "platform": "windows",
        "module": module,
        "symbolName": name,
        "kind": "scalar",
        "payload": {
            "scalar_name": name if payload_name is None else payload_name,
            "scalar_value": value if payload_value is None else payload_value,
        },
    }


class SnapshotContractTests(unittest.TestCase):
    def test_accepts_new_scalar_contract(self):
        doc = make_snapshot([function_record(), scalar_record()])
        errors, module_crc64, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual({"engine": 0x0011223344556677}, module_crc64)
        self.assertEqual({"kind": "scalar", "value": 17080, "module": "engine"},
                         symbols["size_of_frame"])

    def test_rejects_legacy_snapshot_schema(self):
        doc = make_snapshot(schema=4)
        errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
        self.assertTrue(any("schemaVersion must be 5" in e for e in errors), errors)

    def test_rejects_legacy_source_contract(self):
        doc = make_snapshot(contract=7)
        errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
        self.assertTrue(any("snapshotSchemaVersion must be 8" in e for e in errors), errors)

    def test_rejects_legacy_analysis_output_contract(self):
        doc = make_snapshot(analysis=2)
        errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
        self.assertTrue(any("analysisOutputContractVersion must be 3" in e for e in errors), errors)

    def test_requires_is_blob_boolean(self):
        doc = make_snapshot([function_record()], is_blob="false")
        errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
        self.assertTrue(any("isBlob must be a boolean" in e for e in errors), errors)

    def test_rejects_invalid_scalar_value(self):
        for bad in (-1, 0x100000000, "17080", True):
            doc = make_snapshot([scalar_record(payload_value=bad)])
            errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
            self.assertTrue(
                any("invalid uint32 scalar_value" in e for e in errors),
                (bad, errors),
            )

    def test_rejects_missing_scalar_name(self):
        doc = make_snapshot([scalar_record(payload_name="")])
        errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
        self.assertTrue(any("missing/invalid scalar_name" in e for e in errors), errors)


class RequiredScalarGateTests(unittest.TestCase):
    def complete_symbols(self):
        symbols = {}
        for name, kind in validate.COMMON_REQUIRED.items():
            symbols[name] = {"kind": kind}
        for prefix in validate.NUMBERED_PATCH_SETS:
            symbols[f"{prefix}_0"] = {"kind": "patch"}
        symbols["cvar_hooks"] = {"kind": "global"}
        for name in ("NLoadBlob", "FreeBlob"):
            symbols[name] = {"kind": "function"}
        symbols["size_of_frame"] = {"kind": "scalar", "value": 17080, "module": "engine"}
        return symbols

    def test_scalar_gate_passes_when_present(self):
        self.assertEqual(
            [],
            validate.validate_required(self.complete_symbols(), "ENGINE_GOLDSRC", "hl-8684"),
        )

    def test_scalar_gate_flags_missing_scalar(self):
        symbols = self.complete_symbols()
        del symbols["size_of_frame"]
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("missing required scalar 'size_of_frame'" in e for e in errors), errors)

    def test_scalar_gate_flags_wrong_kind(self):
        symbols = self.complete_symbols()
        symbols["size_of_frame"] = {"kind": "global"}
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("must be a scalar record" in e for e in errors), errors)

    def test_scalar_gate_flags_wrong_module(self):
        symbols = self.complete_symbols()
        symbols["size_of_frame"] = {"kind": "scalar", "value": 17080, "module": "client"}
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("must belong to module 'engine'" in e for e in errors), errors)


def make_sync_entry(contents, game_version="hl-8684", contract=8, file_count=0):
    return sync.SnapshotEntry(
        game_version=game_version,
        file_name="{}.{}.json".format(game_version, "c" * 64),
        sha256=hashlib.sha256(contents).hexdigest(),
        size=len(contents),
        snapshot_schema_version=contract,
        file_count=file_count,
        last_publish_time="2026-09-12T04:28:50Z",
    )


def encode_snapshot(schema=5, contract=8, analysis=3):
    import json

    doc = make_snapshot(schema=schema, contract=contract, analysis=analysis)
    return json.dumps(doc).encode("utf-8")


class SyncContractTests(unittest.TestCase):
    def test_contract_constants_match_new_dataset(self):
        self.assertEqual(5, sync.SUPPORTED_SNAPSHOT_SCHEMA_VERSION)
        self.assertEqual(8, sync.SUPPORTED_SNAPSHOT_CONTRACT_VERSION)
        self.assertEqual(3, sync.SUPPORTED_ANALYSIS_OUTPUT_CONTRACT_VERSION)

    def test_accepts_new_snapshot_contract(self):
        contents = encode_snapshot()
        sync.validate_snapshot_contents(contents, make_sync_entry(contents))

    def test_rejects_legacy_snapshot_schema(self):
        contents = encode_snapshot(schema=4)
        with self.assertRaises(sync.UpdateError):
            sync.validate_snapshot_contents(contents, make_sync_entry(contents))

    def test_rejects_legacy_analysis_output_contract(self):
        contents = encode_snapshot(analysis=2)
        with self.assertRaises(sync.UpdateError):
            sync.validate_snapshot_contents(contents, make_sync_entry(contents))

    def test_parse_index_accepts_contract_eight(self):
        import json

        index = {
            "schemaVersion": 4,
            "versions": [{
                "gameVersion": "hl-8684",
                "url": "hl-8684.{}.json".format("d" * 64),
                "sha256": "d" * 64,
                "size": 1024,
                "snapshotSchemaVersion": 8,
                "fileCount": 1,
                "lastPublishTime": "2026-09-12T04:28:50Z",
            }],
        }
        parsed = sync.parse_index(json.dumps(index).encode("utf-8"), "test index")
        self.assertEqual(8, parsed.entries[0].snapshot_schema_version)

    def test_parse_index_rejects_contract_seven(self):
        import json

        index = {
            "schemaVersion": 4,
            "versions": [{
                "gameVersion": "hl-8684",
                "url": "hl-8684.{}.json".format("d" * 64),
                "sha256": "d" * 64,
                "size": 1024,
                "snapshotSchemaVersion": 7,
                "fileCount": 1,
                "lastPublishTime": "2026-09-12T04:28:50Z",
            }],
        }
        with self.assertRaises(sync.UpdateError):
            sync.parse_index(json.dumps(index).encode("utf-8"), "test index")


if __name__ == "__main__":
    unittest.main()
