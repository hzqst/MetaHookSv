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


def struct_member_record(name="CVideoMode_Common.m_ImageID", payload=None):
    if payload is None:
        payload = {
            "struct_name": "CVideoMode_Common",
            "member_name": "m_ImageID",
            "offset": "0x19c",
        }
    return {
        "platform": "windows",
        "module": "engine",
        "symbolName": name,
        "kind": "structMember",
        "payload": payload,
    }


def virtual_function_record(name="GameStudioRenderer_StudioDrawModel", module="engine",
                            payload=None):
    if payload is None:
        payload = {
            "func_rva": "0x1000",
            "func_size": "0x20",
            "vfunc_sig": "55 8B EC",
            "vfunc_index": 2,
            "vtable_name": "GameStudioRenderer",
        }
    return {
        "platform": "windows",
        "module": module,
        "symbolName": name,
        "kind": "virtualFunction",
        "payload": payload,
    }


def vtable_record(name="GameStudioRenderer", module="engine", payload=None):
    if payload is None:
        payload = {
            "vtable_rva": "0x2000",
            "vtable_size": "0x78",
            "vtable_symbol": "GameStudioRenderer",
            "vtable_numvfunc": 30,
        }
    return {
        "platform": "windows",
        "module": module,
        "symbolName": name,
        "kind": "vtable",
        "payload": payload,
    }


class SnapshotContractTests(unittest.TestCase):
    def test_accepts_new_scalar_contract(self):
        doc = make_snapshot([function_record(), scalar_record()])
        errors, module_crc64, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual({"engine": 0x0011223344556677}, module_crc64)
        self.assertEqual({"kind": "scalar", "value": 17080, "module": "engine"},
                         symbols[("engine", "size_of_frame")])

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

    def test_accepts_struct_member_offset(self):
        doc = make_snapshot([struct_member_record()])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "structMember", "offset": 0x19c, "module": "engine"},
            symbols[("engine", "CVideoMode_Common.m_ImageID")],
        )

    def test_rejects_invalid_struct_member_payload(self):
        for payload in (
            {"member_name": "m_ImageID", "offset": "0x19c"},
            {"struct_name": "CVideoMode_Common", "offset": "0x19c"},
            {"struct_name": "CVideoMode_Common", "member_name": "m_ImageID", "offset": "xyz"},
        ):
            doc = make_snapshot([struct_member_record(payload=payload)])
            errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
            self.assertTrue(any("structMember" in e for e in errors), (payload, errors))

    def test_accepts_virtual_function_record(self):
        doc = make_snapshot([virtual_function_record()])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "virtualFunction", "rva": 0x1000, "size": 0x20, "module": "engine"},
            symbols[("engine", "GameStudioRenderer_StudioDrawModel")],
        )

    def test_rejects_virtual_function_missing_slot_identity(self):
        for missing in ("vfunc_index", "vtable_name"):
            payload = {
                "func_rva": "0x1000",
                "func_size": "0x20",
                "vfunc_sig": "55 8B EC",
                "vfunc_index": 2,
                "vtable_name": "GameStudioRenderer",
            }
            del payload[missing]
            doc = make_snapshot([virtual_function_record(payload=payload)])
            errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
            self.assertTrue(
                any("missing/invalid vfunc_index/vtable_name" in e for e in errors),
                (missing, errors),
            )

    def test_rejects_virtual_function_partial_address(self):
        for partial in ({"func_rva": "0x1000"}, {"func_size": "0x20"}):
            payload = {
                "vfunc_sig": "55 8B EC",
                "vfunc_index": 2,
                "vtable_name": "GameStudioRenderer",
            }
            payload.update(partial)
            doc = make_snapshot([virtual_function_record(payload=payload)])
            errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
            self.assertTrue(
                any("missing/invalid func_rva/func_size" in e for e in errors),
                (partial, errors),
            )

    def test_accepts_slot_only_virtual_function_declaration(self):
        # Upstream publishes these for interface methods whose address it does
        # not record. MetaHook cannot resolve them, so they are accepted without
        # being recorded - a consumer gate that pins the name still fails.
        payload = {
            "func_name": "GameStudioRenderer_StudioDrawModel",
            "vfunc_index": 11,
            "vfunc_offset": "0x2c",
            "vtable_name": "IEngineClient",
        }
        doc = make_snapshot([virtual_function_record(payload=payload)])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertNotIn(("engine", "GameStudioRenderer_StudioDrawModel"), symbols)

    def test_accepts_virtual_function_signature_under_func_sig(self):
        payload = {
            "func_rva": "0x1000",
            "func_size": "0x20",
            "func_sig": "55 8B EC",
            "vfunc_index": 2,
            "vtable_name": "GameStudioRenderer",
        }
        doc = make_snapshot([virtual_function_record(payload=payload)])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "virtualFunction", "rva": 0x1000, "size": 0x20, "module": "engine"},
            symbols[("engine", "GameStudioRenderer_StudioDrawModel")],
        )

    def test_accepts_virtual_function_without_signature(self):
        payload = {
            "func_rva": "0x1000",
            "func_size": "0x20",
            "vfunc_index": 39,
            "vtable_name": "vgui2::VPanel",
        }
        doc = make_snapshot([virtual_function_record(payload=payload)])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertIn(("engine", "GameStudioRenderer_StudioDrawModel"), symbols)

    def test_rejects_virtual_function_malformed_or_non_string_signature(self):
        for bad in ("zz", 1234):
            payload = {
                "func_rva": "0x1000",
                "func_size": "0x20",
                "vfunc_sig": bad,
                "vfunc_index": 2,
                "vtable_name": "GameStudioRenderer",
            }
            doc = make_snapshot([virtual_function_record(payload=payload)])
            errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
            self.assertTrue(
                any("signature" in e for e in errors),
                (bad, errors),
            )

    def test_accepts_function_without_signature(self):
        # Upstream omits func_sig when no unique pattern exists (e.g. hl-10210's
        # CTaskbar ctor); the resolver consumes only the rva.
        record = function_record()
        del record["payload"]["func_sig"]
        doc = make_snapshot([record])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "function", "rva": 0x1000, "size": 0x10, "module": "engine"},
            symbols[("engine", "R_NewMap")],
        )

    def test_rejects_function_malformed_or_non_string_signature(self):
        for bad, message in (("zz", "malformed signature"), (1234, "non-string func_sig")):
            record = function_record()
            record["payload"]["func_sig"] = bad
            doc = make_snapshot([record])
            errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
            self.assertTrue(any(message in e for e in errors), (bad, errors))
            self.assertNotIn(("engine", "R_NewMap"), symbols)

    def test_accepts_vtable_record(self):
        doc = make_snapshot([vtable_record()])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "vtable", "rva": 0x2000, "size": 0x78, "module": "engine"},
            symbols[("engine", "GameStudioRenderer")],
        )

    def test_rejects_vtable_missing_fields(self):
        for missing in ("vtable_rva", "vtable_size", "vtable_symbol", "vtable_numvfunc"):
            payload = {
                "vtable_rva": "0x2000",
                "vtable_size": "0x78",
                "vtable_symbol": "GameStudioRenderer",
                "vtable_numvfunc": 30,
            }
            del payload[missing]
            doc = make_snapshot([vtable_record(payload=payload)])
            errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
            self.assertTrue(
                any("missing/invalid vtable_rva/vtable_size/vtable_symbol/vtable_numvfunc" in e
                    for e in errors),
                (missing, errors),
            )

    def test_rejects_vtable_size_numvfunc_mismatch(self):
        payload = {
            "vtable_rva": "0x2000",
            "vtable_size": "0x78",
            "vtable_symbol": "GameStudioRenderer",
            "vtable_numvfunc": 28,
        }
        doc = make_snapshot([vtable_record(payload=payload)])
        errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
        self.assertTrue(
            any("vtable_size does not match vtable_numvfunc" in e for e in errors),
            errors,
        )

    def _shared_name_snapshot(self, name):
        doc = make_snapshot()
        doc["binaries"]["client"] = {
            "windows": {
                "crc64": "8899aabbccddeeff",
                "size": 1024,
                "sha256": SHA256_A,
                "isBlob": True,
            }
        }
        doc["records"] = [function_record(name=name),
                          dict(function_record(name=name), module="client")]
        return doc

    def test_keeps_modules_apart_for_a_shared_symbol_name(self):
        # One symbol name can be published under several modules (client, engine,
        # gameui, serverbrowser). The index must store each module's record
        # instead of reporting the second module as a conflicting duplicate.
        name = "vgui2::Panel::Init(int, int, int, int)"
        errors, _, symbols = validate.validate_snapshot(self._shared_name_snapshot(name), "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "function", "rva": 0x1000, "size": 0x10, "module": "engine"},
            symbols[("engine", name)],
        )
        self.assertEqual(
            {"kind": "function", "rva": 0x1000, "size": 0x10, "module": "client"},
            symbols[("client", name)],
        )

    def test_rejects_divergent_duplicate_within_one_module(self):
        name = "vgui2::Panel::Init(int, int, int, int)"
        doc = self._shared_name_snapshot(name)
        other = function_record(name=name)
        other["payload"] = {"func_rva": "0x2000", "func_size": "0x10", "func_sig": "55 8B EC"}
        doc["records"].append(other)
        errors, _, _ = validate.validate_snapshot(doc, "hl-8684")
        self.assertTrue(any("conflicting duplicate symbol" in e for e in errors), errors)


class RequiredScalarGateTests(unittest.TestCase):
    def complete_symbols(self):
        symbols = {}
        for name, kind in validate.COMMON_REQUIRED.items():
            symbols[("engine", name)] = {"kind": kind}
        for prefix in validate.NUMBERED_PATCH_SETS:
            symbols[("engine", f"{prefix}_0")] = {"kind": "patch"}
        symbols[("engine", "cvar_hooks")] = {"kind": "global"}
        for name in ("NLoadBlob", "FreeBlob"):
            symbols[("engine", name)] = {"kind": "function"}
        symbols[("engine", "size_of_frame")] = {"kind": "scalar", "value": 17080, "module": "engine"}
        for name in validate.BULLETPHYSICS_ENGINE_FUNCTIONS:
            symbols[("engine", name)] = {"kind": "function"}
        for name in validate.BULLETPHYSICS_ENGINE_GLOBALS:
            symbols[("engine", name)] = {"kind": "global"}
        return symbols

    def test_scalar_gate_passes_when_present(self):
        self.assertEqual(
            [],
            validate.validate_required(self.complete_symbols(), "ENGINE_GOLDSRC", "hl-8684"),
        )

    def test_scalar_gate_flags_missing_scalar(self):
        symbols = self.complete_symbols()
        del symbols[("engine", "size_of_frame")]
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("missing required scalar 'size_of_frame'" in e for e in errors), errors)

    def test_scalar_gate_flags_wrong_kind(self):
        symbols = self.complete_symbols()
        symbols[("engine", "size_of_frame")] = {"kind": "global"}
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("must be a scalar record" in e for e in errors), errors)

    def test_scalar_gate_flags_wrong_module(self):
        symbols = self.complete_symbols()
        symbols[("engine", "size_of_frame")] = {"kind": "scalar", "value": 17080, "module": "client"}
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("must belong to module 'engine'" in e for e in errors), errors)

    def test_engine_slot_global_uses_the_catalog_name_eng(self):
        # ThreadGuard resolves the engine module's IEngine* slot. Upstream renamed
        # that global from `engine` to `eng` in its 2026-09-30 release, so the gate
        # must follow the catalog name rather than the old spelling.
        self.assertEqual("global", validate.COMMON_REQUIRED.get("eng"))
        self.assertNotIn("engine", validate.COMMON_REQUIRED)


class BulletPhysicsEngineGateTests(unittest.TestCase):
    def complete_symbols(self):
        symbols = {}
        for name, kind in validate.COMMON_REQUIRED.items():
            symbols[("engine", name)] = {"kind": kind}
        for prefix in validate.NUMBERED_PATCH_SETS:
            symbols[("engine", f"{prefix}_0")] = {"kind": "patch"}
        symbols[("engine", "cvar_hooks")] = {"kind": "global"}
        for name in ("NLoadBlob", "FreeBlob"):
            symbols[("engine", name)] = {"kind": "function"}
        symbols[("engine", "size_of_frame")] = {"kind": "scalar", "value": 17080, "module": "engine"}
        for name in validate.BULLETPHYSICS_ENGINE_FUNCTIONS:
            symbols[("engine", name)] = {"kind": "function"}
        for name in validate.BULLETPHYSICS_ENGINE_GLOBALS:
            symbols[("engine", name)] = {"kind": "global"}
        return symbols

    def test_engine_gate_flags_missing_function(self):
        symbols = self.complete_symbols()
        del symbols[("engine", "R_RenderView")]
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("missing BulletPhysics engine function 'R_RenderView'" in e for e in errors), errors)

    def test_engine_gate_flags_wrong_global_kind(self):
        symbols = self.complete_symbols()
        symbols[("engine", "cl_frames")] = {"kind": "function"}
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("'cl_frames' must be a global record" in e for e in errors), errors)

    def test_svengine_requires_allow_cheats(self):
        symbols = self.complete_symbols()
        errors = validate.validate_required(symbols, "ENGINE_SVENGINE", "svencoop-10257")
        self.assertTrue(any("missing BulletPhysics engine global 'allow_cheats'" in e for e in errors), errors)


class BulletPhysicsClientGateTests(unittest.TestCase):
    def complete_client_symbols(self):
        symbols = {}
        for name in validate.BULLETPHYSICS_CLIENT_GLOBALS:
            symbols[("client", name)] = {"kind": "global"}
        for name in validate.BULLETPHYSICS_CLIENT_OPTIONAL_VFUNCS:
            symbols[("client", name)] = {"kind": "virtualFunction"}
        return symbols

    def test_client_gate_passes_for_hl(self):
        self.assertEqual(
            [],
            validate.validate_bulletphysics_client(self.complete_client_symbols(), "hl-8684"),
        )

    def test_client_gate_tolerates_absent_optional_vfunc(self):
        symbols = self.complete_client_symbols()
        del symbols[("client", "GameStudioRenderer_StudioDrawPlayer")]
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertEqual([], errors)

    def test_client_gate_flags_wrong_kind_optional_vfunc(self):
        symbols = self.complete_client_symbols()
        symbols[("client", "GameStudioRenderer_StudioDrawPlayer")] = {"kind": "global"}
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertTrue(
            any("GameStudioRenderer_StudioDrawPlayer' must be a virtualFunction record" in e for e in errors),
            errors,
        )

    def test_client_gate_flags_missing_global(self):
        symbols = self.complete_client_symbols()
        del symbols[("client", "g_iUser2")]
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertTrue(any("missing BulletPhysics client global" in e for e in errors), errors)

    def test_client_gate_flags_wrong_kind(self):
        symbols = self.complete_client_symbols()
        symbols[("client", "g_iUser1")] = {"kind": "function"}
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertTrue(any("must be a global record" in e for e in errors), errors)

    def test_sven_client_gate_requires_sven_globals(self):
        symbols = self.complete_client_symbols()
        errors = validate.validate_bulletphysics_client(symbols, "svencoop-10257")
        self.assertTrue(any("missing Sven Co-op client global 'g_pitchdrift'" in e for e in errors), errors)
        for name in validate.BULLETPHYSICS_SVEN_CLIENT_GLOBALS:
            symbols[("client", name)] = {"kind": "global"}
        self.assertEqual([], validate.validate_bulletphysics_client(symbols, "svencoop-10257"))

    def test_cs_client_gate_requires_extra_symbols(self):
        symbols = self.complete_client_symbols()
        errors = validate.validate_bulletphysics_client(symbols, "cstrike-8684")
        self.assertTrue(any("g_PlayerExtraInfo" in e for e in errors), errors)
        self.assertTrue(any("GameStudioRenderer__StudioDrawPlayer" in e for e in errors), errors)

    def test_czds_client_gate_requires_czds_array(self):
        symbols = self.complete_client_symbols()
        errors = validate.validate_bulletphysics_client(symbols, "czeror-8684")
        self.assertTrue(any("g_PlayerExtraInfo_CZDS" in e for e in errors), errors)


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


class RendererGateTests(unittest.TestCase):
    def complete_engine_symbols(self, game_version):
        symbols = {}

        def add(names, kind):
            for n in names:
                symbols[("engine", n)] = {"kind": kind, "module": "engine"}

        add(validate.RENDERER_ENGINE_ALL_FUNCTIONS, "function")
        add(validate.RENDERER_ENGINE_ALL_GLOBALS, "global")
        add(validate.RENDERER_ENGINE_ALL_PATCHES, "patch")
        add(validate.RENDERER_ENGINE_STRUCT_MEMBERS, "structMember")
        for prefix in validate.RENDERER_NUMBERED_PATCH_SETS:
            symbols[("engine", f"{prefix}_0")] = {"kind": "patch", "module": "engine"}
            symbols[("engine", f"{prefix}_1")] = {"kind": "patch", "module": "engine"}
        if game_version in validate.RENDERER_NON_SVENGINE_GAMES:
            add(validate.RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS, "function")
            add(validate.RENDERER_ENGINE_NON_SVENGINE_PATCHES, "patch")
            add(validate.RENDERER_ENGINE_NON_SVENGINE_GLOBALS, "global")
        if game_version in validate.RENDERER_MTEX_PROBE_GAMES:
            add(validate.RENDERER_MTEX_PROBE_FUNCTIONS, "function")
        if game_version in validate.RENDERER_INLINED_MTEX_PROBE_GAMES:
            add(validate.RENDERER_INLINED_MTEX_PROBE_FUNCTIONS, "function")
        if game_version in validate.RENDERER_E8_GAMES:
            add(validate.RENDERER_ENGINE_E8_FUNCTIONS, "function")
        if game_version in validate.RENDERER_SVENGINE_GAMES:
            add(validate.RENDERER_ENGINE_SVENGINE_FUNCTIONS, "function")
            add(validate.RENDERER_SVENGINE_GLOBALS, "global")
        if game_version in validate.RENDERER_FBO_GAMES:
            add(validate.RENDERER_FBO_GLOBALS, "global")
        if game_version in validate.RENDERER_HL25_GAMES:
            add(validate.RENDERER_ENGINE_HL25_FUNCTIONS, "function")
        if game_version in validate.RENDERER_SETMODE_GAMES:
            add(validate.RENDERER_SETMODE_FUNCTIONS, "function")
        if game_version in validate.RENDERER_SETMODE_LEGACY_GAMES:
            add(validate.RENDERER_SETMODE_LEGACY_FUNCTIONS, "function")
            add(validate.RENDERER_LEGACY_TEXALLOC_GLOBALS, "global")
            add(validate.RENDERER_LEGACY_TEXALLOC_COMMON_PATCHES, "patch")
        if game_version in validate.RENDERER_LEGACY_TEXALLOC_HL_GAMES:
            add(validate.RENDERER_LEGACY_TEXALLOC_HL_PATCHES, "patch")
        if game_version in validate.RENDERER_SDL_GAMES:
            add(validate.RENDERER_SDL_FUNCTIONS, "function")
        symbols.update(self.complete_client_symbols(game_version))
        return symbols

    def complete_client_symbols(self, game_version):
        symbols = {}

        def add(names, kind):
            for n in names:
                symbols[("client", n)] = {"kind": kind, "module": "client"}

        if game_version in validate.RENDERER_SVENGINE_GAMES:
            add(validate.RENDERER_CLIENT_SVEN_FUNCTIONS, "function")
            add(validate.RENDERER_CLIENT_SVEN_PATCHES, "patch")
            add(validate.RENDERER_CLIENT_SVEN_GLOBALS, "global")
            add(validate.RENDERER_CLIENT_SVEN_STRUCT_MEMBERS, "structMember")
            add(validate.RENDERER_CLIENT_SVEN_SCALARS, "scalar")
            add(validate.RENDERER_CLIENT_10257_SCALARS if game_version in validate.RENDERER_SVEN_10257_GAMES
                else validate.RENDERER_CLIENT_8948_SCALARS, "scalar")
        if game_version in validate.RENDERER_SVEN_10257_GAMES:
            add(validate.RENDERER_CLIENT_10257_GLOBALS, "global")
        if game_version in validate.RENDERER_CLIENT_GAMES:
            add(validate.RENDERER_CLIENT_STUDIO_GLOBALS, "global")
            add(validate.RENDERER_CLIENT_STUDIO_VFUNCS, "virtualFunction")
        if game_version in validate.RENDERER_CS_CLIENT_GAMES:
            add(("g_PlayerExtraInfo",), "global")
        if game_version in validate.RENDERER_CZDS_CLIENT_GAMES:
            add(("g_PlayerExtraInfo_CZDS",), "global")
        return symbols

    def test_gate_passes_for_every_declared_identity(self):
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            symbols.update(self.complete_client_symbols(gv))
            self.assertEqual([], validate.validate_renderer(symbols, gv), gv)
        for gv in validate.RENDERER_CLIENT_GAMES:
            if gv in validate.RENDERER_ALL_GAMES:
                continue
            symbols = self.complete_client_symbols(gv)
            self.assertEqual([], validate.validate_renderer(symbols, gv, include_engine=False), gv)

    def test_sven_core_hooks_are_required_on_both_clients(self):
        for gv in validate.RENDERER_SVENGINE_GAMES:
            for name in ("ClientPortalManager_EnableClipPlane", "ClientPortalManager_InitShader",
                         "CParticleSystem_ParticleDraw", "ClientPortalManager.m_bShadersAvailable"):
                symbols = self.complete_client_symbols(gv)
                del symbols[("client", name)]
                errors = validate.validate_renderer(symbols, gv, include_engine=False)
                self.assertTrue(any(name in e for e in errors), (gv, name, errors))

    def test_sven_shader_flag_requires_struct_member_kind(self):
        for gv in validate.RENDERER_SVENGINE_GAMES:
            symbols = self.complete_client_symbols(gv)
            symbols[("client", "ClientPortalManager.m_bShadersAvailable")]["kind"] = "global"
            errors = validate.validate_renderer(symbols, gv, include_engine=False)
            self.assertTrue(any("must be a structMember record" in e for e in errors), errors)

    def test_portal_redirect_requires_a_client_patch_on_both_sven_clients(self):
        name = "ClientPortalManager_RenderPortals_to_AngleVectors_callsite_0"
        for gv in ("svencoop-8948", "svencoop-10257"):
            for record in (None, {"kind": "function", "module": "client"},
                           {"kind": "patch", "module": "engine"}):
                with self.subTest(gv=gv, record=record):
                    symbols = self.complete_client_symbols(gv)
                    symbols.pop(("client", name), None)
                    if record is not None:
                        symbols[("client", name)] = record
                    errors = validate.validate_renderer(symbols, gv, include_engine=False)
                    self.assertTrue(any(name in e for e in errors), errors)
        self.assertEqual([], validate.validate_renderer({}, "hl-6153", include_engine=False))

    def test_gate_flags_missing_engine_function(self):
        symbols = self.complete_engine_symbols("hl-8684")
        del symbols[("engine", "R_NewMap")]
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("missing Renderer engine function 'R_NewMap'" in e for e in errors), errors)

    def test_gate_flags_kind_mismatch(self):
        symbols = self.complete_engine_symbols("hl-8684")
        symbols[("engine", "R_NewMap")] = {"kind": "global", "module": "engine"}
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("'R_NewMap' must be a function record" in e for e in errors), errors)

    def test_gate_flags_module_mismatch(self):
        symbols = self.complete_engine_symbols("hl-8684")
        symbols[("engine", "R_NewMap")] = {"kind": "function", "module": "client"}
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("'R_NewMap' must belong to module 'engine'" in e for e in errors), errors)

    def test_gate_requires_startup_graphic_member_offsets(self):
        for gv in validate.RENDERER_ALL_GAMES:
            for name in validate.RENDERER_ENGINE_STRUCT_MEMBERS:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (gv, name, errors))

    def test_gate_requires_fbo_aspect_globals_only_when_published(self):
        for gv in validate.RENDERER_FBO_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "s_fXMouseAspectAdjustment")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("s_fXMouseAspectAdjustment" in e for e in errors), (gv, errors))
        symbols = self.complete_engine_symbols("hl-3248")
        self.assertNotIn(("engine", "s_fXMouseAspectAdjustment"), symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "hl-3248"))

    def test_gate_requires_numbered_patch_set(self):
        symbols = self.complete_engine_symbols("hl-8684")
        for key in list(symbols):
            if key[1].startswith("CL_LinkPacketEntities_to_R_ResetLatched_callsite"):
                del symbols[key]
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("missing required Renderer patch "
                            "'CL_LinkPacketEntities_to_R_ResetLatched_callsite_0'" in e for e in errors), errors)

    def test_gate_treats_svengine_variants_as_required(self):
        symbols = self.complete_engine_symbols("svencoop-10257")
        del symbols[("engine", "Draw_SpriteFrameHoles_SvEngine")]
        errors = validate.validate_renderer(symbols, "svencoop-10257")
        self.assertTrue(any("Draw_SpriteFrameHoles_SvEngine" in e for e in errors), errors)

    def test_gate_requires_sven_client_fog_globals(self):
        for gv in validate.RENDERER_SVENGINE_GAMES:
            for name in ("g_iFogColor", "g_iStartDist", "g_iEndDist"):
                symbols = self.complete_engine_symbols(gv)
                del symbols[("client", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (gv, name, errors))

    def test_gate_does_not_require_base_symbol_for_svengine(self):
        symbols = self.complete_engine_symbols("svencoop-10257")
        self.assertNotIn(("engine", "Draw_SpriteFrameHoles"), symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "svencoop-10257"))

    def test_gate_does_not_require_legacy_setmode_for_sdl_build(self):
        symbols = self.complete_engine_symbols("hl-8684")
        self.assertNotIn(("engine", "GL_SetModeLegacy"), symbols)
        self.assertIn(("engine", "GL_SetMode"), symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "hl-8684"))

    def test_gate_requires_legacy_setmode_for_cof(self):
        symbols = self.complete_engine_symbols("cof-5936")
        self.assertIn(("engine", "GL_SetModeLegacy"), symbols)
        self.assertNotIn(("engine", "GL_SetMode"), symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "cof-5936"))
        del symbols[("engine", "GL_SetModeLegacy")]
        errors = validate.validate_renderer(symbols, "cof-5936")
        self.assertTrue(any("GL_SetModeLegacy" in e for e in errors), errors)

    def test_gate_requires_legacy_texture_allocation_patch_sites(self):
        for gv in validate.RENDERER_SETMODE_LEGACY_GAMES:
            names = validate.RENDERER_LEGACY_TEXALLOC_COMMON_PATCHES
            if gv in validate.RENDERER_LEGACY_TEXALLOC_HL_GAMES:
                names += validate.RENDERER_LEGACY_TEXALLOC_HL_PATCHES
            for name in names:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (gv, name, errors))

        symbols = self.complete_engine_symbols("cof-5936")
        for name in validate.RENDERER_LEGACY_TEXALLOC_HL_PATCHES:
            self.assertNotIn(("engine", name), symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "cof-5936"))

    def test_gate_does_not_require_sdl_initgl_without_sdl(self):
        symbols = self.complete_engine_symbols("hl-4554")
        self.assertNotIn(("engine", "SDL_InitGL"), symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "hl-4554"))

    def test_gate_requires_exactly_one_multitexture_init_per_identity(self):
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            if gv in validate.RENDERER_INLINED_MTEX_PROBE_GAMES:
                self.assertIn(("engine", "DT_Initialize"), symbols, gv)
                self.assertNotIn(("engine", "CheckMultiTextureExtensions"), symbols, gv)
            else:
                self.assertIn(("engine", "CheckMultiTextureExtensions"), symbols, gv)
                self.assertNotIn(("engine", "DT_Initialize"), symbols, gv)
            self.assertEqual([], validate.validate_renderer(symbols, gv), gv)

    def test_gate_flags_missing_multitexture_probe(self):
        symbols = self.complete_engine_symbols("hl-8684")
        del symbols[("engine", "CheckMultiTextureExtensions")]
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("CheckMultiTextureExtensions" in e for e in errors), errors)

    def test_gate_flags_missing_dt_initialize_where_probe_is_inlined(self):
        symbols = self.complete_engine_symbols("hl-10210")
        del symbols[("engine", "DT_Initialize")]
        errors = validate.validate_renderer(symbols, "hl-10210")
        self.assertTrue(any("DT_Initialize" in e for e in errors), errors)

    def test_gate_requires_mod_unloadspritetextures_on_every_identity(self):
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "Mod_UnloadSpriteTextures")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("Mod_UnloadSpriteTextures" in e for e in errors), (gv, errors))
        self.assertNotIn("Mod_UnloadSpriteTextures", validate.RENDERER_ENGINE_E8_FUNCTIONS)

    def test_gate_requires_rgba_fill_bodies_on_every_identity(self):
        for name in ("Draw_FillRGBA", "Draw_FillRGBABlend"):
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))
            self.assertNotIn(name, validate.RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS)

    def test_gate_requires_draw_fillrgbabuf_on_svengine_only(self):
        for gv in validate.RENDERER_SVENGINE_GAMES:
            symbols = self.complete_engine_symbols(gv)
            self.assertIn(("engine", "Draw_FillRGBABuf"), symbols)
            del symbols[("engine", "Draw_FillRGBABuf")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("Draw_FillRGBABuf" in e for e in errors), (gv, errors))
        for gv in validate.RENDERER_NON_SVENGINE_GAMES:
            self.assertNotIn("Draw_FillRGBABuf", self.complete_engine_symbols(gv))

    def test_gate_no_longer_references_the_retired_net_drawrect_name(self):
        self.assertNotIn("NET_DrawRect", validate.RENDERER_ENGINE_SVENGINE_FUNCTIONS)
        for gv in validate.RENDERER_ALL_GAMES:
            self.assertNotIn("NET_DrawRect", self.complete_engine_symbols(gv))

    def test_gate_skips_d_fillrect_on_svengine(self):
        for gv in validate.RENDERER_SVENGINE_GAMES:
            symbols = self.complete_engine_symbols(gv)
            self.assertNotIn(("engine", "D_FillRect"), symbols)
            self.assertEqual([], validate.validate_renderer(symbols, gv))
        for gv in validate.RENDERER_NON_SVENGINE_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "D_FillRect")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("D_FillRect" in e for e in errors), (gv, errors))

    def test_gate_requires_user_fog_globals_on_every_identity(self):
        names = ("flFinalFogColor", "flFogDensity", "flFogEnd", "flFogStart",
                 "g_bUserFogOn")
        for name in names:
            self.assertIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))
        self.assertNotIn("R_RenderFinalFog", validate.RENDERER_ENGINE_ALL_FUNCTIONS)
        self.assertNotIn("R_RenderFinalFog", validate.RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS)

    def test_gate_requires_gspritemipmap_on_every_identity(self):
        self.assertIn("gSpriteMipMap", validate.RENDERER_ENGINE_ALL_GLOBALS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "gSpriteMipMap")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("gSpriteMipMap" in e for e in errors), (gv, errors))
        self.assertNotIn("Mod_LoadSpriteFrame", validate.RENDERER_ENGINE_ALL_FUNCTIONS)

    def test_gate_requires_scr_drawloading_on_every_identity(self):
        self.assertIn("scr_drawloading", validate.RENDERER_ENGINE_ALL_GLOBALS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "scr_drawloading")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("scr_drawloading" in e for e in errors), (gv, errors))
        self.assertNotIn("SCR_BeginLoadingPlaque", validate.RENDERER_ENGINE_ALL_FUNCTIONS)

    def test_gate_requires_window_rect_on_every_identity(self):
        self.assertIn("window_rect", validate.RENDERER_ENGINE_ALL_GLOBALS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "window_rect")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("window_rect" in e for e in errors), (gv, errors))
        #VID_UpdateWindowVars was write-only; the catalog FUNCTION record has no
        #consumer and must not become a release dependency.
        self.assertNotIn("VID_UpdateWindowVars", validate.RENDERER_ENGINE_ALL_FUNCTIONS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            symbols[("engine", "VID_UpdateWindowVars")] = {"kind": "function", "module": "engine"}
            self.assertEqual([], validate.validate_renderer(symbols, gv))

    def test_gate_requires_texgammatable_on_every_identity(self):
        self.assertIn("texgammatable", validate.RENDERER_ENGINE_ALL_GLOBALS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "texgammatable")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("texgammatable" in e for e in errors), (gv, errors))
        self.assertIn("BuildGammaTable", validate.RENDERER_ENGINE_ALL_FUNCTIONS)

    def test_gate_requires_particletexture_on_every_identity(self):
        self.assertIn("particletexture", validate.RENDERER_ENGINE_ALL_GLOBALS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols[("engine", "particletexture")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("particletexture" in e for e in errors), (gv, errors))

    def test_gate_requires_enginesurface_vertexbuffer_globals_on_every_identity(self):
        for name in ("g_VertexBuffer", "g_iVertexBufferEntriesUsed"):
            self.assertIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (gv, errors))

    def test_gate_requires_the_engine_surface_scissor_and_window_globals(self):
        #Renderer and VGUI2Extension each located these three by disassembling the
        #mirror engine's EngineSurface::pushMakeCurrent body. Both now resolve them
        #from gamedata and both key the lookup on the loaded engine module's CRC64,
        #so pinning them on the Renderer identities pins their shared dependency.
        for name in ("pmainwindow", "g_bScissor", "g_ScissorRect"):
            self.assertIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (gv, errors))

    def test_gate_requires_direct_resolved_palette_scissor_and_studio_globals(self):
        names = (
            "giScissorTest", "host_basepal", "lightgammatable",
            "r_ambientlight", "r_plightvec", "r_shadelight",
        )
        for name in names:
            self.assertIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))
        self.assertIn("Draw_Frame", validate.RENDERER_ENGINE_ALL_FUNCTIONS)
        self.assertNotIn("R_StudioLighting", validate.RENDERER_ENGINE_ALL_FUNCTIONS)

    def test_gate_requires_direct_resolved_studio_globals_on_every_identity(self):
        names = (
            "g_ForcedFaceFlags", "psubmodel", "r_bottomcolor",
            "r_colormix", "r_topcolor",
        )
        for name in names:
            self.assertIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))
        #pbodypart was written by the studio setup locator but never read; it must
        #not become a release dependency.
        self.assertNotIn("pbodypart", validate.RENDERER_ENGINE_ALL_GLOBALS)

    def test_gate_requires_the_consumed_fallback_texture_for_each_engine_family(self):
        for gv in validate.RENDERER_SVENGINE_GAMES:
            symbols = self.complete_engine_symbols(gv)
            self.assertIn(("engine", "r_missingtexture"), symbols)
            self.assertNotIn(("engine", "r_notexture_mip"), symbols)
            del symbols[("engine", "r_missingtexture")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("r_missingtexture" in e for e in errors), (gv, errors))
        for gv in validate.RENDERER_NON_SVENGINE_GAMES:
            symbols = self.complete_engine_symbols(gv)
            self.assertIn(("engine", "r_notexture_mip"), symbols)
            self.assertNotIn(("engine", "r_missingtexture"), symbols)
            del symbols[("engine", "r_notexture_mip")]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("r_notexture_mip" in e for e in errors), (gv, errors))

        retired = ("r_emptytexture", "r_blightvec", "scissor_x", "scissor_y",
                   "scissor_width", "scissor_height")
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            for name in retired:
                self.assertNotIn(("engine", name), symbols, (gv, name))

    def test_gate_requires_lightmap_and_decal_symbols_on_every_identity(self):
        names = (
            "d_lightstylevalue", "frustum", "gDecalCache", "gDecalPool", "gDecalSurfCount",
            "lightmaps", "rtable",
        )
        for name in names:
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))

    def test_gate_requires_screen_filter_globals_on_every_identity(self):
        names = ("filterBrightness", "filterColorBlue", "filterColorGreen",
                 "filterColorRed", "filterMode")
        for name in names:
            self.assertIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[("engine", name)]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))

    def test_gate_ignores_symbols_the_renderer_no_longer_consumes(self):
        #The multitexture wrappers and their globals lost their last reader when
        #GL_PushDrawState / GL_PopDrawState were deleted, and oldtarget lost its
        #glActiveTexture readers; the plugin resolves none of them, so the gate
        #must not make the release depend on records nobody consumes.
        retired = ("GL_EnableMultitexture", "GL_DisableMultitexture",
                   "gl_mtexable", "mtexenabled", "oldtarget")
        for name in retired:
            self.assertNotIn(name, validate.RENDERER_ENGINE_ALL_FUNCTIONS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_NON_SVENGINE_GLOBALS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_SVENGINE_FUNCTIONS)
            self.assertNotIn(name, validate.RENDERER_SVENGINE_GLOBALS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            for name in retired:
                self.assertNotIn(("engine", name), symbols, (gv, name))
            self.assertEqual([], validate.validate_renderer(symbols, gv), gv)

    def test_gate_ignores_function_resolutions_the_renderer_never_read(self):
        #These five were captured into gPrivateFuncs but had no reader: the plugin
        #draws particles, T-entities and the engine surface through its own bodies
        #instead of chaining to the engine originals, and it obtains
        #R_DrawSequentialPoly by leaving the engine body alone. The plugin no longer
        #resolves any of them, so the gate must not keep the release dependent on
        #records nobody consumes.
        retired = ("GL_Shutdown", "R_DrawParticles", "R_DrawSequentialPoly",
                   "R_DrawTEntitiesOnList", "R_TextureAnimation")
        for name in retired:
            self.assertNotIn(name, validate.RENDERER_ENGINE_ALL_FUNCTIONS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_NON_SVENGINE_GLOBALS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_SVENGINE_FUNCTIONS)
            self.assertNotIn(name, validate.RENDERER_ENGINE_HL25_FUNCTIONS)
            self.assertNotIn(name, validate.RENDERER_SVENGINE_GLOBALS)
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            for name in retired:
                self.assertNotIn(("engine", name), symbols, (gv, name))
            self.assertEqual([], validate.validate_renderer(symbols, gv), gv)

    def test_gate_alias_poly_counter_follows_engine_family(self):
        symbols = self.complete_engine_symbols("hl-8684")
        self.assertIn(("engine", "c_alias_polys"), symbols)
        self.assertNotIn(("engine", "c_model_polys"), symbols)
        symbols = self.complete_engine_symbols("svencoop-10257")
        self.assertIn(("engine", "c_model_polys"), symbols)
        self.assertNotIn(("engine", "c_alias_polys"), symbols)

    def test_gate_requires_client_virtuals_for_client_games(self):
        symbols = self.complete_client_symbols("hl-8684")
        del symbols[("client", "GameStudioRenderer_StudioSetupBones")]
        errors = validate.validate_renderer(symbols, "hl-8684", include_engine=False)
        self.assertTrue(any("GameStudioRenderer_StudioSetupBones" in e for e in errors), errors)

    def test_gate_does_not_require_client_virtuals_without_client_module(self):
        symbols = self.complete_client_symbols("hl-4554")
        self.assertEqual({}, symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "hl-4554", include_engine=False))


class CaptionModGateTests(unittest.TestCase):
    def complete_symbols(self, game_version):
        symbols = {}

        def add(names, kind, module):
            for n in names:
                symbols[(module, n)] = {"kind": kind, "module": module}

        if game_version in validate.RENDERER_ALL_GAMES:
            add(validate.CAPTIONMOD_ENGINE_FUNCTIONS, "function", "engine")
            add(validate.CAPTIONMOD_ENGINE_GLOBALS, "global", "engine")
        if game_version in validate.CAPTIONMOD_CLIENT_GAMES:
            add(validate.CAPTIONMOD_CLIENT_FUNCTIONS, "function", "client")
            add(validate.CAPTIONMOD_CLIENT_GLOBALS, "global", "client")
            add(validate.CAPTIONMOD_CLIENT_STRUCT_MEMBERS, "structMember", "client")
        if game_version in validate.CAPTIONMOD_CS_CLIENT_GAMES:
            add(validate.CAPTIONMOD_CS_CLIENT_FUNCTIONS, "function", "client")
        if game_version in validate.CAPTIONMOD_CS_TEXT_COLOR_GAMES:
            add(("GetTextColor",), "function", "client")
        if game_version in validate.CAPTIONMOD_CS_LOCATION_COLOR_GAMES:
            add(("g_LocationColor",), "global", "client")
        return symbols

    def gate(self, symbols, game_version):
        return validate.validate_captionmod(
            symbols, game_version, include_engine=game_version in validate.RENDERER_ALL_GAMES
        )

    def test_gate_passes_on_every_engine_identity(self):
        for gv in validate.RENDERER_ALL_GAMES:
            self.assertEqual([], self.gate(self.complete_symbols(gv), gv), gv)

    def test_gate_passes_on_every_counter_strike_client(self):
        for gv in validate.CAPTIONMOD_CS_CLIENT_GAMES:
            self.assertEqual([], self.gate(self.complete_symbols(gv), gv), gv)

    def test_gate_reports_missing_engine_symbols(self):
        for name in ("S_LoadSound", "cl_time"):
            with self.subTest(name=name):
                symbols = self.complete_symbols("hl-8684")
                del symbols[("engine", name)]
                errors = self.gate(symbols, "hl-8684")
                self.assertTrue(any(name in e for e in errors), errors)

    def test_gate_reports_wrong_kind(self):
        symbols = self.complete_symbols("hl-8684")
        symbols[("engine", "cl_time")] = {"kind": "function", "module": "engine"}
        errors = self.gate(symbols, "hl-8684")
        self.assertTrue(any("cl_time" in e and "global" in e for e in errors), errors)

    def test_gate_ignores_vox_lookup_string(self):
        # CaptionMod never resolves VOX_LookupString itself, only the sentence
        # counters, so the function record must not be gated.
        self.assertNotIn("VOX_LookupString", validate.CAPTIONMOD_ENGINE_FUNCTIONS)
        self.assertEqual([], self.gate(self.complete_symbols("svencoop-10257"), "svencoop-10257"))

    def test_gate_requires_client_sound_engine_on_svengine(self):
        for gv in validate.CAPTIONMOD_CLIENT_GAMES:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols(gv)
                self.assertEqual([], self.gate(symbols, gv))
                del symbols[("client", "CClient_SoundEngine_LoadSoundList")]
                errors = self.gate(symbols, gv)
                self.assertTrue(any("LoadSoundList" in e for e in errors), errors)

    def test_gate_requires_every_sven_coop_client_global(self):
        for name in validate.CAPTIONMOD_CLIENT_GLOBALS:
            for gv in validate.CAPTIONMOD_CLIENT_GAMES:
                with self.subTest(name=name, gv=gv):
                    symbols = self.complete_symbols(gv)
                    del symbols[("client", name)]
                    errors = self.gate(symbols, gv)
                    self.assertTrue(any(name in e for e in errors), errors)

    def test_gate_requires_every_sven_coop_client_function(self):
        for name in validate.CAPTIONMOD_CLIENT_FUNCTIONS:
            for gv in validate.CAPTIONMOD_CLIENT_GAMES:
                with self.subTest(name=name, gv=gv):
                    symbols = self.complete_symbols(gv)
                    del symbols[("client", name)]
                    errors = self.gate(symbols, gv)
                    self.assertTrue(any(name in e for e in errors), errors)

    def test_gate_does_not_require_client_sound_engine_elsewhere(self):
        for gv in ("hl-8684", "hl-10210", "cof-5936", "hl-4554"):
            with self.subTest(gv=gv):
                # Engine symbols only: no client records, still no failure.
                self.assertEqual([], self.gate(self.complete_symbols(gv), gv))

    def test_gate_reports_missing_sentence_count_member(self):
        for gv in validate.CAPTIONMOD_CLIENT_GAMES:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols(gv)
                del symbols[("client", "CClient_SoundEngine.m_iSentenceCount")]
                errors = self.gate(symbols, gv)
                self.assertTrue(any("m_iSentenceCount" in e for e in errors), errors)

    def test_gate_reports_sentence_count_as_function(self):
        symbols = self.complete_symbols("svencoop-8948")
        symbols[("client", "CClient_SoundEngine.m_iSentenceCount")] = {"kind": "function", "module": "client"}
        errors = self.gate(symbols, "svencoop-8948")
        self.assertTrue(any("m_iSentenceCount" in e and "structMember" in e for e in errors), errors)

    def test_gate_requires_get_client_color_on_every_counter_strike_client(self):
        for gv in validate.CAPTIONMOD_CS_CLIENT_GAMES:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols(gv)
                del symbols[("client", "GetClientColor")]
                errors = self.gate(symbols, gv)
                self.assertTrue(any("GetClientColor" in e for e in errors), errors)

    def test_gate_requires_get_text_color_only_where_published(self):
        for gv in validate.CAPTIONMOD_CS_TEXT_COLOR_GAMES:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols(gv)
                del symbols[("client", "GetTextColor")]
                errors = self.gate(symbols, gv)
                self.assertTrue(any("GetTextColor" in e for e in errors), errors)

    def test_gate_does_not_require_get_text_color_where_unpublished(self):
        # cstrike-10210, czero-10210 and the czeror builds publish no Windows
        # GetTextColor record; CaptionMod falls back to the location colour array
        # on the first two and to a null colour on czeror. These are exactly the
        # builds on which the plugin's own signature locator fails too.
        for gv in ("cstrike-10210", "czero-10210", "czeror-10210", "czeror-8684"):
            with self.subTest(gv=gv):
                self.assertNotIn(gv, validate.CAPTIONMOD_CS_TEXT_COLOR_GAMES)
                self.assertEqual([], self.gate(self.complete_symbols(gv), gv))

    def test_gate_requires_location_color_exactly_where_get_text_color_is_absent(self):
        # The TEXTCOLOR_LOCATION fallback is only resolved when GetTextColor is
        # missing, so its required set is the non-czeror remainder of that group.
        expected = tuple(
            gv for gv in validate.CAPTIONMOD_CS_CLIENT_GAMES
            if gv not in validate.CAPTIONMOD_CS_TEXT_COLOR_GAMES
            and not gv.startswith("czeror-")
        )
        self.assertEqual(expected, validate.CAPTIONMOD_CS_LOCATION_COLOR_GAMES)
        for gv in validate.CAPTIONMOD_CS_LOCATION_COLOR_GAMES:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols(gv)
                del symbols[("client", "g_LocationColor")]
                errors = self.gate(symbols, gv)
                self.assertTrue(any("g_LocationColor" in e for e in errors), errors)

    def test_gate_flags_location_color_as_a_function(self):
        symbols = self.complete_symbols("cstrike-10210")
        symbols[("client", "g_LocationColor")] = {"kind": "function", "module": "client"}
        errors = self.gate(symbols, "cstrike-10210")
        self.assertTrue(any("g_LocationColor" in e and "global" in e for e in errors), errors)

    def test_gate_does_not_require_location_color_on_czeror(self):
        # czeror publishes neither GetTextColor nor g_LocationColor, and the plugin
        # skips it with an explicit game-directory guard, so it must not be gated.
        for gv in ("czeror-10210", "czeror-8684"):
            with self.subTest(gv=gv):
                self.assertNotIn(gv, validate.CAPTIONMOD_CS_LOCATION_COLOR_GAMES)
                self.assertEqual([], self.gate(self.complete_symbols(gv), gv))

    def test_gate_does_not_require_location_color_where_get_text_color_covers_it(self):
        # cstrike-3248/3647/4554/6153/8684 and czero-8684 never reach the fallback
        # branch, so the record is not pinned there even though it is published.
        for gv in validate.CAPTIONMOD_CS_TEXT_COLOR_GAMES:
            with self.subTest(gv=gv):
                self.assertNotIn(gv, validate.CAPTIONMOD_CS_LOCATION_COLOR_GAMES)
                # complete_symbols omits g_LocationColor for these builds; passing the
                # gate without it is what proves the record is not required here.
                self.assertNotIn("g_LocationColor", self.complete_symbols(gv))
                self.assertEqual([], self.gate(self.complete_symbols(gv), gv))


class SCCameraFixGateTests(unittest.TestCase):
    def complete_symbols(self, game_version):
        symbols = {}
        if game_version in validate.SCCAMERAFIX_CLIENT_GAMES:
            for n in validate.SCCAMERAFIX_CLIENT_GLOBALS:
                symbols[("client", n)] = {"kind": "global", "module": "client"}
            for n in validate.SCCAMERAFIX_CLIENT_FUNCTIONS:
                symbols[("client", n)] = {"kind": "function", "module": "client"}
        return symbols

    def gate(self, symbols, game_version):
        return validate.validate_sccamerafix(symbols, game_version)

    def test_gate_passes_on_every_sven_coop_client(self):
        for gv in validate.SCCAMERAFIX_CLIENT_GAMES:
            with self.subTest(gv=gv):
                self.assertEqual([], self.gate(self.complete_symbols(gv), gv))

    def test_camera_symbols_require_the_client_module_and_correct_kind(self):
        records = {"v_origin": "global", "g_vVecViewangles": "global",
                   "V_CalcNormalRefdef": "function"}
        for gv in ("svencoop-8948", "svencoop-10257"):
            for name, kind in records.items():
                for record in (None, {"kind": "patch", "module": "client"},
                               {"kind": kind, "module": "engine"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_symbols(gv)
                        symbols.pop(("client", name), None)
                        if record is not None:
                            symbols[("client", name)] = record
                        self.assertTrue(any(name in e for e in self.gate(symbols, gv)))

    def test_gate_requires_every_client_global(self):
        for name in validate.SCCAMERAFIX_CLIENT_GLOBALS:
            for gv in validate.SCCAMERAFIX_CLIENT_GAMES:
                with self.subTest(name=name, gv=gv):
                    symbols = self.complete_symbols(gv)
                    del symbols[("client", name)]
                    errors = self.gate(symbols, gv)
                    self.assertTrue(any(name in e for e in errors), errors)

    def test_gate_flags_wrong_kind(self):
        symbols = self.complete_symbols("svencoop-10257")
        symbols[("client", "g_iUser1")] = {"kind": "function", "module": "client"}
        errors = self.gate(symbols, "svencoop-10257")
        self.assertTrue(any("g_iUser1" in e and "global" in e for e in errors), errors)

    def test_gate_flags_wrong_module(self):
        symbols = self.complete_symbols("svencoop-8948")
        symbols[("client", "g_vVecViewangles")] = {"kind": "global", "module": "engine"}
        errors = self.gate(symbols, "svencoop-8948")
        self.assertTrue(any("g_vVecViewangles" in e and "client" in e for e in errors), errors)

    def test_gate_requires_engfuncs_for_the_eventapi_slot(self):
        # gEngfuncs is dereferenced to its pEventAPI member to rebuild
        # g_pClientDLLEventAPI, replacing the A1/8B 40 0C/FF E0 pattern block.
        for gv in validate.SCCAMERAFIX_CLIENT_GAMES:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols(gv)
                del symbols[("client", "gEngfuncs")]
                errors = self.gate(symbols, gv)
                self.assertTrue(any("gEngfuncs" in e for e in errors), errors)

    def test_gate_ignores_symbols_the_plugin_no_longer_reads(self):
        # fog/waterlevel/portal/iIsSpectator were read only inside #if 0 blocks
        # and were deleted; gating them would demand records nothing consumes.
        for name in ("g_iFogColor", "g_iStartDist", "g_iEndDist",
                     "iIsSpectator", "g_iWaterLevel", "g_bRenderingPortals_SCClient"):
            with self.subTest(name=name):
                self.assertNotIn(name, validate.SCCAMERAFIX_CLIENT_GLOBALS)

    def test_gate_does_not_require_client_globals_outside_sven_coop(self):
        for gv in ("hl-8684", "hl-10210", "cof-5936", "cstrike-8684", "czero-8684"):
            with self.subTest(gv=gv):
                self.assertNotIn(gv, validate.SCCAMERAFIX_CLIENT_GAMES)
                self.assertEqual([], self.gate(self.complete_symbols(gv), gv))

    def test_gate_skips_the_retired_renderer_studio_globals_entry(self):
        # Renderer deleted its g_iUser1 / g_iUser2 dependency as write-only; the
        # SCCameraFix gate is where they are pinned now.
        self.assertEqual((), validate.RENDERER_CLIENT_STUDIO_GLOBALS)
        self.assertIn("g_iUser1", validate.SCCAMERAFIX_CLIENT_GLOBALS)
        self.assertIn("g_iUser2", validate.SCCAMERAFIX_CLIENT_GLOBALS)


class VGUI2ExtensionGateTests(unittest.TestCase):
    # KeyValues::LoadFromFile is published with and without its vgui2:: namespace
    # (GoldSrc_VibeSignatures issue #316); either alias satisfies the gates.
    keyvalues_namespaced = "vgui2::KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)"
    keyvalues_plain = "KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)"
    keyvalues_namespaced_games = ("hl-3248", "hl-3266", "hl-3329")
    records = {
        "vgui2::Panel::Init(int, int, int, int)": "function",
        "KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)": "virtualFunction",
        "vgui2::Frame::LoadControlSettings(char const*, char const*)": "function",
        "CTeamMenu::LoadMapPage(char const*)": "function",
        "vgui2::RichText::SetText(wchar_t const*)": "function",
        "vgui2::Frame::Activate()": "virtualFunction",
        "CounterStrikeViewport.m_pCSBackGround": "structMember",
        "CounterStrikeViewport::CCSBackGroundPanel::Activate()": "virtualFunction",
        "CounterStrikeViewport::CCSBackGroundPanel.m_offsetX": "structMember",
        "CounterStrikeViewport::CCSBackGroundPanel.m_offsetY": "structMember",
    }
    # The first two are only type-checked when present on non-CS clients.
    optional = ("vgui2::Panel::Init(int, int, int, int)",
                "KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)")
    games = ("cstrike-3248", "cstrike-3647", "cstrike-4554", "cstrike-6153",
             "cstrike-8684", "cstrike-10210", "czero-8684", "czero-10210",
             "czeror-8684", "czeror-10210")
    # Condition Zero Deleted Scenes publishes no background panel: the plugin
    # skips that block for czeror, so its entries are not gated there.
    zds_games = ("czeror-8684", "czeror-10210")
    background_panel_member = "CounterStrikeViewport.m_pCSBackGround"
    background_panel_records = (
        "CounterStrikeViewport.m_pCSBackGround",
        "CounterStrikeViewport::CCSBackGroundPanel.m_offsetX",
        "CounterStrikeViewport::CCSBackGroundPanel.m_offsetY",
        "CounterStrikeViewport::CCSBackGroundPanel::Activate()",
    )
    # The CZDS WorldMap entries, published only by the czeror snapshots.
    czds_records = {
        "CZEROViewPort.m_pWorldMapPanel": "structMember",
        "CWorldMap::PaintBackground()": "virtualFunction",
        "CWorldMapMissionSelect::PaintBackground()": "virtualFunction",
    }
    czds_games = validate.VGUI2EXTENSION_CZDS_CLIENT_GAMES
    # The engine-side globals that replaced the disassembly locators. They live
    # in the engine module, which the Counter-Strike client snapshots do not
    # publish. Older engines use the separately gated registry reader; the
    # remaining engines require both language-copy call-site patches.
    engine_records = {
        "cl_time": "global",
        "cl_oldtime": "global",
        "realtime": "global",
        "cl_viewentity": "global",
        "listener_origin": "global",
        "staticEngineSurface": "global",
        "host_parms": "global",
    }
    # The engine's vgui2 panel init replaced the shared VGUI2_FindPanelInit walk.
    engine_functions = {"vgui2::Panel::Init(int, int, int, int)": "function"}
    # The GameUI.dll / ServerBrowser.dll entries that replaced the string-anchored
    # reverse-search locators. Both modules are published on every engine
    # identity, and the CS/CZ clients ship the hl binaries, so they follow the
    # engine-bearing snapshots.
    gameui_functions = {
        "CGameConsoleDialog::CGameConsoleDialog()": "function",
        "CBasePanel::CBasePanel()": "function",
        "CTaskbar::CTaskbar(vgui2::Panel*, char const*)": "function",
        "CCreateMultiplayerGameDialog::CCreateMultiplayerGameDialog(vgui2::Panel*)": "function",
        "COptionsDialog::COptionsDialog(vgui2::Panel*)": "function",
        "COptionsSubAudio::COptionsSubAudio(vgui2::Panel*)": "function",
        "COptionsSubVideo::COptionsSubVideo(vgui2::Panel*)": "function",
        "COptionsSubMultiplayer::COptionsSubMultiplayer(vgui2::Panel*)": "function",
        "vgui2::Panel::Init(int, int, int, int)": "function",
    }
    # vgui2::RichText::OnThink replaced the ConsoleHistory vftable scan and
    # CTaskbar::OnCommand the taskbar vftable slot read; they are VIRTUAL_FUNCTION
    # records, but published with a func_rva like a function one.
    gameui_virtual_functions = {
        "vgui2::RichText::OnThink()": "virtualFunction",
        "CTaskbar::OnCommand(char const*)": "virtualFunction",
    }
    # The career frames ship in the shared Half-Life GameUI.dll that CZ/CZDS load;
    # Sven Co-op publishes none of them.
    career_functions = {
        "CCareerProfileFrame::CCareerProfileFrame(vgui2::Panel*)": "function",
        "CCareerMapFrame::CCareerMapFrame(vgui2::Panel*)": "function",
        "CCareerBotFrame::CCareerBotFrame(vgui2::Panel*)": "function",
    }
    career_games = validate.VGUI2EXTENSION_GAMEUI_CAREER_GAMES
    # hl-10210 inlined ApplyVidSettings into OnApplyChanges(), so it publishes no
    # standalone function and the plugin resolves it optionally.
    applyvidsettings_games = validate.VGUI2EXTENSION_GAMEUI_APPLYVIDSETTINGS_GAMES
    applyvidsettings = "COptionsSubVideo::ApplyVidSettings(bool)"
    # The plugin hooks the host of the RichText carriage-return filter: InsertChar,
    # or InsertString(wchar_t const*) on hl-10210, where Valve inlined InsertChar.
    insert_char = "vgui2::RichText::InsertChar(wchar_t)"
    insert_string_w = "vgui2::RichText::InsertString(wchar_t const*)"
    insert_string_w_games = ("hl-10210",)
    richtext_patch = "vgui2::RichText carriage-return filter branch"
    # The condump failure-path callees the retired InsertChar walk started from.
    # The catalog publishes both on some identities; neither is gated any more.
    condump_callees = ("CGameConsoleDialog::Print(char const*)",
                       "vgui2::RichText::InsertString(char const*)")
    serverbrowser_functions = {"vgui2::Panel::Init(int, int, int, int)": "function"}
    engine_games = validate.RENDERER_ALL_GAMES
    registry_games = ("hl-3248", "hl-3266", "hl-3329", "hl-3647", "hl-4554")
    registry_reader = "Sys_GetRegKeyValueUnderRoot"
    language_patches = (
        "FileSystem_SetGameDirectory_V_strncpy_callsite_0",
        "FileSystem_AddFallbackGameDir_V_strncpy_callsite_0",
    )
    module_factory_games = ("hl-6153", "hl-8684", "hl-10210", "svencoop-8948", "svencoop-10257")
    factory_patch = "VGUIClient001_CreateInterface"
    # The cursor-visibility global is published only by the Sven Co-op clients;
    # CS/CZ/HL coverage is tracked by GoldSrc_VibeSignatures issue #295.
    visible_mouse_games = ("svencoop-8948", "svencoop-10257")
    visible_mouse_global = "g_iVisibleMouse"

    def complete_symbols(self):
        symbols = {("client", name): {"kind": kind, "module": "client"}
                   for name, kind in self.records.items()}
        symbols.update({("client", name): {"kind": kind, "module": "client"}
                        for name, kind in self.czds_records.items()})
        return symbols

    def complete_engine_symbols(self, game_version=None):
        symbols = {("engine", name): {"kind": kind, "module": "engine"}
                   for name, kind in self.engine_records.items()}
        symbols.update({("engine", name): {"kind": kind, "module": "engine"}
                        for name, kind in self.engine_functions.items()})
        symbols.update({("gameui", name): {"kind": kind, "module": "gameui"}
                        for name, kind in self.gameui_functions.items()})
        symbols.update({("gameui", name): {"kind": kind, "module": "gameui"}
                        for name, kind in self.gameui_virtual_functions.items()})
        if game_version in self.career_games:
            symbols.update({("gameui", name): {"kind": kind, "module": "gameui"}
                            for name, kind in self.career_functions.items()})
        if game_version in self.applyvidsettings_games:
            symbols[("gameui", self.applyvidsettings)] = {"kind": "function", "module": "gameui"}
        richtext_host = self.insert_string_w if game_version in self.insert_string_w_games else self.insert_char
        symbols[("gameui", richtext_host)] = {"kind": "function", "module": "gameui"}
        symbols[("gameui", self.richtext_patch)] = {"kind": "patch", "module": "gameui"}
        keyvalues = (self.keyvalues_namespaced if game_version in self.keyvalues_namespaced_games
                     else self.keyvalues_plain)
        symbols[("gameui", keyvalues)] = {"kind": "virtualFunction", "module": "gameui"}
        symbols.update({("serverbrowser", name): {"kind": kind, "module": "serverbrowser"}
                        for name, kind in self.serverbrowser_functions.items()})
        if game_version in self.registry_games:
            symbols[("engine", self.registry_reader)] = {"kind": "function", "module": "engine"}
        else:
            symbols.update({("engine", name): {"kind": "patch", "module": "engine"}
                            for name in self.language_patches})
        if game_version in self.module_factory_games:
            symbols[("engine", self.factory_patch)] = {"kind": "patch", "module": "engine"}
        else:
            symbols[("engine", "g_pClientFactory")] = {"kind": "global", "module": "engine"}
        if game_version in self.visible_mouse_games:
            symbols[("client", self.visible_mouse_global)] = {"kind": "global", "module": "client"}
        return symbols

    def test_factory_path_requires_the_correct_address_kind_and_module(self):
        for gv in self.engine_games:
            name = self.factory_patch if gv in self.module_factory_games else "g_pClientFactory"
            expected_kind = "patch" if gv in self.module_factory_games else "global"
            for record in (None, {"kind": "function", "module": "engine"},
                           {"kind": expected_kind, "module": "client"}):
                with self.subTest(gv=gv, record=record):
                    symbols = self.complete_engine_symbols(gv)
                    del symbols[("engine", name)]
                    if record is not None:
                        symbols[("engine", name)] = record
                    # A callback slot cannot replace a missing module-factory CALL.
                    if gv in self.module_factory_games:
                        symbols[("engine", "g_pClientFactory")] = {"kind": "global", "module": "engine"}
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(name in error for error in errors), errors)

    def test_factory_patch_is_type_checked_when_present_on_other_engines(self):
        symbols = self.complete_engine_symbols("cof-5936")
        symbols[("engine", self.factory_patch)] = {"kind": "function", "module": "engine"}
        errors = validate.validate_vgui2extension(symbols, "cof-5936")
        self.assertTrue(any(self.factory_patch in error for error in errors), errors)

    def test_both_language_copy_patches_are_required_and_typed(self):
        for gv in set(self.engine_games) - set(self.registry_games):
            for name in self.language_patches:
                for record in (None, {"kind": "function", "module": "engine"},
                               {"kind": "patch", "module": "client"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_engine_symbols(gv)
                        del symbols[("engine", name)]
                        if record is not None:
                            symbols[("engine", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv)
                        self.assertTrue(any(name in error for error in errors), errors)

    def test_registry_language_path_does_not_require_copy_patches(self):
        for gv in self.engine_games:
            with self.subTest(gv=gv):
                symbols = self.complete_engine_symbols(gv)
                for name in self.language_patches:
                    symbols.pop(("engine", name), None)
                symbols[("engine", self.registry_reader)] = {"kind": "function", "module": "engine"}
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv))

    def test_legacy_language_registry_reader_is_required_and_typed(self):
        for gv in self.registry_games:
            self.assertEqual([], validate.validate_vgui2extension(self.complete_engine_symbols(gv), gv))
            for record in (None, {"kind": "patch", "module": "engine"},
                           {"kind": "function", "module": "client"}):
                with self.subTest(gv=gv, record=record):
                    symbols = self.complete_engine_symbols(gv)
                    del symbols[("engine", self.registry_reader)]
                    if record is not None:
                        symbols[("engine", self.registry_reader)] = record
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(self.registry_reader in error for error in errors), errors)

    def test_other_engines_allow_absent_registry_reader_but_reject_wrong_kind(self):
        for gv in set(self.engine_games) - set(self.registry_games):
            with self.subTest(gv=gv):
                symbols = self.complete_engine_symbols(gv)
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv))
                symbols[("engine", self.registry_reader)] = {"kind": "global", "module": "engine"}
                errors = validate.validate_vgui2extension(symbols, gv)
                self.assertTrue(any(self.registry_reader in error for error in errors), errors)

    def test_published_cs_clients_keep_all_required_entries(self):
        for gv in self.games:
            with self.subTest(gv=gv):
                self.assertEqual([], validate.validate_vgui2extension(
                    self.complete_symbols(), gv, include_engine=False))

    def test_visible_mouse_global_is_required_on_the_sven_clients(self):
        for gv in self.visible_mouse_games:
            self.assertEqual([], validate.validate_vgui2extension(self.complete_engine_symbols(gv), gv))
            for record in (None, {"kind": "function", "module": "client"},
                           {"kind": "global", "module": "engine"}):
                with self.subTest(gv=gv, record=record):
                    symbols = self.complete_engine_symbols(gv)
                    del symbols[("client", self.visible_mouse_global)]
                    if record is not None:
                        symbols[("client", self.visible_mouse_global)] = record
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(self.visible_mouse_global in error for error in errors), errors)

    def test_other_clients_do_not_require_the_visible_mouse_global(self):
        for gv in set(self.engine_games) - set(self.visible_mouse_games):
            with self.subTest(gv=gv):
                symbols = self.complete_engine_symbols(gv)
                symbols.pop(("client", self.visible_mouse_global), None)
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv))

    def test_options_sub_page_ctors_are_required_on_every_identity(self):
        for gv in self.engine_games:
            for name in ("COptionsSubAudio::COptionsSubAudio(vgui2::Panel*)",
                         "COptionsSubVideo::COptionsSubVideo(vgui2::Panel*)",
                         "COptionsSubMultiplayer::COptionsSubMultiplayer(vgui2::Panel*)"):
                for record in (None, {"kind": "global", "module": "gameui"},
                               {"kind": "function", "module": "engine"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_engine_symbols(gv)
                        del symbols[("gameui", name)]
                        if record is not None:
                            symbols[("gameui", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv)
                        self.assertTrue(any(name in error for error in errors), errors)

    def test_richtext_onthink_is_required_as_a_virtual_function_on_every_identity(self):
        name = "vgui2::RichText::OnThink()"
        self.assertIn(name, validate.VGUI2EXTENSION_GAMEUI_VIRTUAL_FUNCTIONS)
        for gv in self.engine_games:
            for record in (None, {"kind": "function", "module": "gameui"},
                           {"kind": "virtualFunction", "module": "engine"}):
                with self.subTest(gv=gv, record=record):
                    symbols = self.complete_engine_symbols(gv)
                    del symbols[("gameui", name)]
                    if record is not None:
                        symbols[("gameui", name)] = record
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(name in error for error in errors), errors)

    def test_career_frames_are_required_only_where_the_shared_hl_gameui_is_published(self):
        for gv in self.career_games:
            for name in self.career_functions:
                with self.subTest(gv=gv, name=name):
                    symbols = self.complete_engine_symbols(gv)
                    del symbols[("gameui", name)]
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(name in error for error in errors), errors)
        for gv in set(self.engine_games) - set(self.career_games):
            with self.subTest(gv=gv):
                symbols = self.complete_engine_symbols(gv)
                for name in self.career_functions:
                    symbols.pop(("gameui", name), None)
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv))

    def test_applyvidsettings_is_required_only_where_it_is_standalone(self):
        self.assertNotIn("hl-10210", self.applyvidsettings_games)
        for gv in self.applyvidsettings_games:
            with self.subTest(gv=gv):
                symbols = self.complete_engine_symbols(gv)
                del symbols[("gameui", self.applyvidsettings)]
                errors = validate.validate_vgui2extension(symbols, gv)
                self.assertTrue(any(self.applyvidsettings in error for error in errors), errors)
        symbols = self.complete_engine_symbols("hl-10210")
        symbols.pop(("gameui", self.applyvidsettings), None)
        self.assertEqual([], validate.validate_vgui2extension(symbols, "hl-10210"))

    def test_condump_callees_are_not_gated(self):
        for gv in self.engine_games:
            with self.subTest(gv=gv):
                symbols = self.complete_engine_symbols(gv)
                for name in self.condump_callees:
                    symbols[("gameui", name)] = {"kind": "function", "module": "gameui"}
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv))

    def test_richtext_insertchar_is_required_except_where_it_is_inlined(self):
        self.assertEqual(tuple(gv for gv in self.engine_games if gv not in self.insert_string_w_games),
                         validate.VGUI2EXTENSION_GAMEUI_INSERTCHAR_GAMES)
        for gv in self.engine_games:
            name = self.insert_string_w if gv in self.insert_string_w_games else self.insert_char
            for record in (None, {"kind": "global", "module": "gameui"},
                           {"kind": "function", "module": "engine"}):
                with self.subTest(gv=gv, record=record):
                    symbols = self.complete_engine_symbols(gv)
                    del symbols[("gameui", name)]
                    if record is not None:
                        symbols[("gameui", name)] = record
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(name in error for error in errors), errors)

    def test_richtext_insertstring_w_is_not_required_where_insertchar_is_published(self):
        for gv in self.engine_games:
            with self.subTest(gv=gv):
                symbols = self.complete_engine_symbols(gv)
                symbols.pop(("gameui", self.insert_string_w), None)
                symbols[("gameui", self.insert_char)] = {"kind": "function", "module": "gameui"}
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv))

    def test_richtext_carriage_return_patch_is_required_on_every_identity(self):
        for gv in self.engine_games:
            for record in (None, {"kind": "function", "module": "gameui"},
                           {"kind": "patch", "module": "engine"}):
                with self.subTest(gv=gv, record=record):
                    symbols = self.complete_engine_symbols(gv)
                    del symbols[("gameui", self.richtext_patch)]
                    if record is not None:
                        symbols[("gameui", self.richtext_patch)] = record
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(self.richtext_patch in error for error in errors), errors)

    def test_gameui_keyvalues_accepts_either_alias(self):
        for gv in self.engine_games:
            for name in (self.keyvalues_namespaced, self.keyvalues_plain):
                with self.subTest(gv=gv, name=name):
                    symbols = self.complete_engine_symbols(gv)
                    symbols.pop(("gameui", self.keyvalues_namespaced), None)
                    symbols.pop(("gameui", self.keyvalues_plain), None)
                    symbols[("gameui", name)] = {"kind": "virtualFunction", "module": "gameui"}
                    self.assertEqual([], validate.validate_vgui2extension(symbols, gv))

    def test_gameui_keyvalues_is_required_and_type_checked(self):
        for gv in self.engine_games:
            with self.subTest(gv=gv, case="missing"):
                symbols = self.complete_engine_symbols(gv)
                symbols.pop(("gameui", self.keyvalues_namespaced), None)
                symbols.pop(("gameui", self.keyvalues_plain), None)
                errors = validate.validate_vgui2extension(symbols, gv)
                self.assertTrue(any(self.keyvalues_namespaced in error and self.keyvalues_plain in error
                                    for error in errors), errors)
            for name in (self.keyvalues_namespaced, self.keyvalues_plain):
                with self.subTest(gv=gv, case="mistyped", name=name):
                    symbols = self.complete_engine_symbols(gv)
                    symbols.pop(("gameui", self.keyvalues_namespaced), None)
                    symbols.pop(("gameui", self.keyvalues_plain), None)
                    symbols[("gameui", name)] = {"kind": "function", "module": "gameui"}
                    errors = validate.validate_vgui2extension(symbols, gv)
                    self.assertTrue(any(name in error for error in errors), errors)

    def test_cs_client_keyvalues_accepts_the_namespaced_alias(self):
        for gv in self.games:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols()
                del symbols[("client", self.keyvalues_plain)]
                symbols[("client", self.keyvalues_namespaced)] = {"kind": "virtualFunction", "module": "client"}
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv, include_engine=False))

    def test_missing_or_mistyped_client_entry_is_rejected(self):
        for gv in self.games:
            for name, kind in self.records.items():
                if gv in self.zds_games and name in self.background_panel_records:
                    continue
                for record in (None, {"kind": "global", "module": "client"},
                               {"kind": kind, "module": "engine"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_symbols()
                        del symbols[("client", name)]
                        if record is not None:
                            symbols[("client", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv, include_engine=False)
                        self.assertTrue(any(name in e for e in errors), errors)

    def test_zds_does_not_require_the_background_panel_member(self):
        for gv in self.zds_games:
            with self.subTest(gv=gv):
                symbols = self.complete_symbols()
                for name in self.background_panel_records:
                    del symbols[("client", name)]
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv, include_engine=False))

    def test_background_panel_entries_are_required_where_the_block_runs(self):
        for name in self.background_panel_records:
            self.assertIn(name, set(validate.VGUI2EXTENSION_BACKGROUND_PANEL_STRUCT_MEMBERS)
                          | set(validate.VGUI2EXTENSION_BACKGROUND_PANEL_VIRTUAL_FUNCTIONS))
        for gv in set(self.games) - set(self.zds_games):
            with self.subTest(gv=gv):
                self.assertEqual([], validate.validate_vgui2extension(
                    self.complete_symbols(), gv, include_engine=False))

    def test_background_panel_dimension_offsets_are_both_required(self):
        # CCSBackGroundPanel_Activate zeroes both dimension members; m_offsetY
        # replaced the m_offsetX + 4 arithmetic, so it is gated like m_offsetX.
        for name in ("CounterStrikeViewport::CCSBackGroundPanel.m_offsetX",
                     "CounterStrikeViewport::CCSBackGroundPanel.m_offsetY"):
            self.assertIn(name, validate.VGUI2EXTENSION_BACKGROUND_PANEL_STRUCT_MEMBERS)
        for gv in set(self.games) - set(self.zds_games):
            with self.subTest(gv=gv):
                symbols = self.complete_symbols()
                del symbols[("client", "CounterStrikeViewport::CCSBackGroundPanel.m_offsetY")]
                errors = validate.validate_vgui2extension(symbols, gv, include_engine=False)
                self.assertTrue(any("m_offsetY" in error for error in errors), errors)

    def test_missing_or_mistyped_background_panel_entry_is_rejected(self):
        for gv in set(self.games) - set(self.zds_games):
            for name in self.background_panel_records:
                kind = self.records[name]
                for record in (None, {"kind": "global", "module": "client"},
                               {"kind": kind, "module": "engine"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_symbols()
                        del symbols[("client", name)]
                        if record is not None:
                            symbols[("client", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv, include_engine=False)
                        self.assertTrue(any(name in e for e in errors), errors)

    def test_czds_worldmap_entries_are_required_on_the_deleted_scenes_clients(self):
        for name in self.czds_records:
            self.assertIn(name, set(validate.VGUI2EXTENSION_CZDS_STRUCT_MEMBERS)
                          | set(validate.VGUI2EXTENSION_CZDS_VIRTUAL_FUNCTIONS))
        for gv in self.czds_games:
            with self.subTest(gv=gv):
                self.assertEqual([], validate.validate_vgui2extension(
                    self.complete_symbols(), gv, include_engine=False))

    def test_missing_or_mistyped_czds_entry_is_rejected(self):
        for gv in self.czds_games:
            for name, kind in self.czds_records.items():
                for record in (None, {"kind": "global", "module": "client"},
                               {"kind": kind, "module": "engine"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_symbols()
                        del symbols[("client", name)]
                        if record is not None:
                            symbols[("client", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv, include_engine=False)
                        self.assertTrue(any(name in e for e in errors), errors)

    def test_other_cs_clients_do_not_require_the_czds_entries(self):
        for gv in set(self.games) - set(self.czds_games):
            with self.subTest(gv=gv):
                symbols = self.complete_symbols()
                for name in self.czds_records:
                    del symbols[("client", name)]
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv, include_engine=False))

    def test_other_clients_allow_absent_native_entries(self):
        for gv in ("svencoop-8948", "svencoop-10257", "hl-8684", "hl-10210", "cof-5936"):
            with self.subTest(gv=gv):
                symbols = {}
                if gv in self.visible_mouse_games:
                    symbols[("client", self.visible_mouse_global)] = {"kind": "global", "module": "client"}
                self.assertEqual([], validate.validate_vgui2extension(symbols, gv, include_engine=False))

    def test_optional_entries_are_type_checked_when_present(self):
        for name in self.optional + (self.keyvalues_namespaced,):
            with self.subTest(name=name):
                symbols = {("client", name): {"kind": "global", "module": "client"}}
                errors = validate.validate_vgui2extension(symbols, "svencoop-10257", include_engine=False)
                self.assertTrue(any(name in e for e in errors), errors)

    def test_engine_entries_are_required_on_engine_snapshots(self):
        for name in self.engine_records:
            self.assertIn(name, validate.VGUI2EXTENSION_ENGINE_GLOBALS)
        for name in self.engine_functions:
            self.assertIn(name, validate.VGUI2EXTENSION_ENGINE_FUNCTIONS)
        for gv in self.engine_games:
            with self.subTest(gv=gv):
                self.assertEqual([], validate.validate_vgui2extension(
                    self.complete_engine_symbols(gv), gv))

    def test_missing_or_mistyped_engine_function_is_rejected(self):
        for gv in self.engine_games:
            for name, kind in self.engine_functions.items():
                for record in (None, {"kind": "global", "module": "engine"},
                               {"kind": kind, "module": "client"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_engine_symbols(gv)
                        del symbols[("engine", name)]
                        if record is not None:
                            symbols[("engine", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv)
                        self.assertTrue(any(name in e for e in errors), errors)

    def test_missing_or_mistyped_engine_entry_is_rejected(self):
        for gv in self.engine_games:
            for name, kind in self.engine_records.items():
                wrong_kind = "patch" if kind == "global" else "global"
                for record in (None, {"kind": wrong_kind, "module": "engine"},
                               {"kind": kind, "module": "client"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_engine_symbols(gv)
                        del symbols[("engine", name)]
                        if record is not None:
                            symbols[("engine", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv)
                        self.assertTrue(any(name in e for e in errors), errors)

    def test_gameui_entries_are_required_on_engine_snapshots(self):
        for name in self.gameui_functions:
            self.assertIn(name, validate.VGUI2EXTENSION_GAMEUI_FUNCTIONS)
        for gv in self.engine_games:
            with self.subTest(gv=gv):
                self.assertEqual([], validate.validate_vgui2extension(
                    self.complete_engine_symbols(gv), gv))

    def test_missing_or_mistyped_gameui_entry_is_rejected(self):
        for gv in self.engine_games:
            for name, kind in self.gameui_functions.items():
                for record in (None, {"kind": "global", "module": "gameui"},
                               {"kind": kind, "module": "serverbrowser"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_engine_symbols(gv)
                        del symbols[("gameui", name)]
                        if record is not None:
                            symbols[("gameui", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv)
                        self.assertTrue(any(name in e for e in errors), errors)

    def test_missing_or_mistyped_serverbrowser_entry_is_rejected(self):
        for gv in self.engine_games:
            for name, kind in self.serverbrowser_functions.items():
                for record in (None, {"kind": "global", "module": "serverbrowser"},
                               {"kind": kind, "module": "gameui"}):
                    with self.subTest(gv=gv, name=name, record=record):
                        symbols = self.complete_engine_symbols(gv)
                        del symbols[("serverbrowser", name)]
                        if record is not None:
                            symbols[("serverbrowser", name)] = record
                        errors = validate.validate_vgui2extension(symbols, gv)
                        self.assertTrue(any(name in e for e in errors), errors)

    def test_client_only_snapshots_are_not_gameui_gated(self):
        # cstrike / czero / czeror publish no gameui or serverbrowser module of
        # their own, so the caller's include_engine is False for them.
        for gv in validate.VGUI2EXTENSION_CLIENT_GAMES:
            with self.subTest(gv=gv):
                self.assertEqual([], validate.validate_vgui2extension(
                    self.complete_symbols(), gv, include_engine=False))

    def test_client_only_snapshots_are_not_engine_gated(self):
        # These snapshots publish no engine module, so the caller's include_engine
        # is False for them; the client-side entries they do publish stay required.
        for gv in validate.VGUI2EXTENSION_CLIENT_GAMES:
            with self.subTest(gv=gv):
                self.assertNotIn(gv, self.engine_games)
                symbols = self.complete_symbols()
                if gv in self.zds_games:
                    del symbols[("client", self.background_panel_member)]
                self.assertEqual([], validate.validate_vgui2extension(
                    symbols, gv, include_engine=False))


if __name__ == "__main__":
    unittest.main()
