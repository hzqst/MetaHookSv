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

    def test_accepts_virtual_function_record(self):
        doc = make_snapshot([virtual_function_record()])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "virtualFunction", "rva": 0x1000, "size": 0x20, "module": "engine"},
            symbols["GameStudioRenderer_StudioDrawModel"],
        )

    def test_rejects_virtual_function_missing_fields(self):
        for missing in ("func_rva", "func_size", "vfunc_sig", "vfunc_index", "vtable_name"):
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
                any("missing/invalid func_rva/func_size/vfunc_sig/vfunc_index/vtable_name" in e
                    for e in errors),
                (missing, errors),
            )

    def test_accepts_vtable_record(self):
        doc = make_snapshot([vtable_record()])
        errors, _, symbols = validate.validate_snapshot(doc, "hl-8684")
        self.assertEqual([], errors, errors)
        self.assertEqual(
            {"kind": "vtable", "rva": 0x2000, "size": 0x78, "module": "engine"},
            symbols["GameStudioRenderer"],
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
        for name in validate.BULLETPHYSICS_ENGINE_FUNCTIONS:
            symbols[name] = {"kind": "function"}
        for name in validate.BULLETPHYSICS_ENGINE_GLOBALS:
            symbols[name] = {"kind": "global"}
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


class BulletPhysicsEngineGateTests(unittest.TestCase):
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
        for name in validate.BULLETPHYSICS_ENGINE_FUNCTIONS:
            symbols[name] = {"kind": "function"}
        for name in validate.BULLETPHYSICS_ENGINE_GLOBALS:
            symbols[name] = {"kind": "global"}
        return symbols

    def test_engine_gate_flags_missing_function(self):
        symbols = self.complete_symbols()
        del symbols["R_RenderView"]
        errors = validate.validate_required(symbols, "ENGINE_GOLDSRC", "hl-8684")
        self.assertTrue(any("missing BulletPhysics engine function 'R_RenderView'" in e for e in errors), errors)

    def test_engine_gate_flags_wrong_global_kind(self):
        symbols = self.complete_symbols()
        symbols["cl_frames"] = {"kind": "function"}
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
            symbols[name] = {"kind": "global"}
        for name in validate.BULLETPHYSICS_CLIENT_OPTIONAL_VFUNCS:
            symbols[name] = {"kind": "virtualFunction"}
        return symbols

    def test_client_gate_passes_for_hl(self):
        self.assertEqual(
            [],
            validate.validate_bulletphysics_client(self.complete_client_symbols(), "hl-8684"),
        )

    def test_client_gate_tolerates_absent_optional_vfunc(self):
        symbols = self.complete_client_symbols()
        del symbols["GameStudioRenderer_StudioDrawPlayer"]
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertEqual([], errors)

    def test_client_gate_flags_wrong_kind_optional_vfunc(self):
        symbols = self.complete_client_symbols()
        symbols["GameStudioRenderer_StudioDrawPlayer"] = {"kind": "global"}
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertTrue(
            any("GameStudioRenderer_StudioDrawPlayer' must be a virtualFunction record" in e for e in errors),
            errors,
        )

    def test_client_gate_flags_missing_global(self):
        symbols = self.complete_client_symbols()
        del symbols["g_iUser2"]
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertTrue(any("missing BulletPhysics client global" in e for e in errors), errors)

    def test_client_gate_flags_wrong_kind(self):
        symbols = self.complete_client_symbols()
        symbols["g_iUser1"] = {"kind": "function"}
        errors = validate.validate_bulletphysics_client(symbols, "hl-8684")
        self.assertTrue(any("must be a global record" in e for e in errors), errors)

    def test_sven_client_gate_requires_sven_globals(self):
        symbols = self.complete_client_symbols()
        errors = validate.validate_bulletphysics_client(symbols, "svencoop-10257")
        self.assertTrue(any("missing Sven Co-op client global 'g_pitchdrift'" in e for e in errors), errors)
        for name in validate.BULLETPHYSICS_SVEN_CLIENT_GLOBALS:
            symbols[name] = {"kind": "global"}
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
                symbols[n] = {"kind": kind, "module": "engine"}

        add(validate.RENDERER_ENGINE_ALL_FUNCTIONS, "function")
        add(validate.RENDERER_ENGINE_ALL_GLOBALS, "global")
        add(validate.RENDERER_ENGINE_ALL_PATCHES, "patch")
        for prefix in validate.RENDERER_NUMBERED_PATCH_SETS:
            symbols[f"{prefix}_0"] = {"kind": "patch", "module": "engine"}
            symbols[f"{prefix}_1"] = {"kind": "patch", "module": "engine"}
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
        if game_version in validate.RENDERER_HL25_GAMES:
            add(validate.RENDERER_ENGINE_HL25_FUNCTIONS, "function")
        if game_version in validate.RENDERER_SETMODE_GAMES:
            add(validate.RENDERER_SETMODE_FUNCTIONS, "function")
        if game_version in validate.RENDERER_SETMODE_LEGACY_GAMES:
            add(validate.RENDERER_SETMODE_LEGACY_FUNCTIONS, "function")
        if game_version in validate.RENDERER_SDL_GAMES:
            add(validate.RENDERER_SDL_FUNCTIONS, "function")
        if game_version in validate.RENDERER_NOT_SVENGINE_10257_GAMES:
            add(validate.RENDERER_NOT_SVENGINE_10257_FUNCTIONS, "function")
        if game_version in validate.RENDERER_NOT_SVENGINE_8948_GAMES:
            add(validate.RENDERER_NOT_SVENGINE_8948_FUNCTIONS, "function")
        symbols.update(self.complete_client_symbols(game_version))
        return symbols

    def complete_client_symbols(self, game_version):
        symbols = {}

        def add(names, kind):
            for n in names:
                symbols[n] = {"kind": kind, "module": "client"}

        if game_version in validate.RENDERER_SVENGINE_GAMES:
            add(validate.RENDERER_CLIENT_SVEN_FUNCTIONS, "function")
            add(validate.RENDERER_CLIENT_SVEN_GLOBALS, "global")
        if game_version in validate.RENDERER_SVEN_10257_GAMES:
            add(validate.RENDERER_CLIENT_10257_FUNCTIONS, "function")
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

    def test_gate_flags_missing_engine_function(self):
        symbols = self.complete_engine_symbols("hl-8684")
        del symbols["R_NewMap"]
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("missing Renderer engine function 'R_NewMap'" in e for e in errors), errors)

    def test_gate_flags_kind_mismatch(self):
        symbols = self.complete_engine_symbols("hl-8684")
        symbols["R_NewMap"] = {"kind": "global", "module": "engine"}
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("'R_NewMap' must be a function record" in e for e in errors), errors)

    def test_gate_flags_module_mismatch(self):
        symbols = self.complete_engine_symbols("hl-8684")
        symbols["R_NewMap"] = {"kind": "function", "module": "client"}
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("'R_NewMap' must belong to module 'engine'" in e for e in errors), errors)

    def test_gate_requires_numbered_patch_set(self):
        symbols = self.complete_engine_symbols("hl-8684")
        for name in list(symbols):
            if name.startswith("CL_LinkPacketEntities_to_R_ResetLatched_callsite"):
                del symbols[name]
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("missing required Renderer patch "
                            "'CL_LinkPacketEntities_to_R_ResetLatched_callsite_0'" in e for e in errors), errors)

    def test_gate_treats_svengine_variants_as_required(self):
        symbols = self.complete_engine_symbols("svencoop-10257")
        del symbols["Draw_SpriteFrameHoles_SvEngine"]
        errors = validate.validate_renderer(symbols, "svencoop-10257")
        self.assertTrue(any("Draw_SpriteFrameHoles_SvEngine" in e for e in errors), errors)

    def test_gate_does_not_require_base_symbol_for_svengine(self):
        symbols = self.complete_engine_symbols("svencoop-10257")
        self.assertNotIn("Draw_SpriteFrameHoles", symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "svencoop-10257"))

    def test_gate_does_not_require_legacy_setmode_for_sdl_build(self):
        symbols = self.complete_engine_symbols("hl-8684")
        self.assertNotIn("GL_SetModeLegacy", symbols)
        self.assertIn("GL_SetMode", symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "hl-8684"))

    def test_gate_requires_legacy_setmode_for_cof(self):
        symbols = self.complete_engine_symbols("cof-5936")
        self.assertIn("GL_SetModeLegacy", symbols)
        self.assertNotIn("GL_SetMode", symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "cof-5936"))
        del symbols["GL_SetModeLegacy"]
        errors = validate.validate_renderer(symbols, "cof-5936")
        self.assertTrue(any("GL_SetModeLegacy" in e for e in errors), errors)

    def test_gate_does_not_require_sdl_initgl_without_sdl(self):
        symbols = self.complete_engine_symbols("hl-4554")
        self.assertNotIn("SDL_InitGL", symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "hl-4554"))

    def test_gate_render_scene_is_not_applicable_for_svengine_10257(self):
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            if gv == "svencoop-10257":
                self.assertNotIn("R_RenderScene", symbols)
            else:
                self.assertIn("R_RenderScene", symbols)
            self.assertEqual([], validate.validate_renderer(symbols, gv), gv)

    def test_gate_requires_exactly_one_multitexture_init_per_identity(self):
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            if gv in validate.RENDERER_INLINED_MTEX_PROBE_GAMES:
                self.assertIn("DT_Initialize", symbols, gv)
                self.assertNotIn("CheckMultiTextureExtensions", symbols, gv)
            else:
                self.assertIn("CheckMultiTextureExtensions", symbols, gv)
                self.assertNotIn("DT_Initialize", symbols, gv)
            self.assertEqual([], validate.validate_renderer(symbols, gv), gv)

    def test_gate_flags_missing_multitexture_probe(self):
        symbols = self.complete_engine_symbols("hl-8684")
        del symbols["CheckMultiTextureExtensions"]
        errors = validate.validate_renderer(symbols, "hl-8684")
        self.assertTrue(any("CheckMultiTextureExtensions" in e for e in errors), errors)

    def test_gate_flags_missing_dt_initialize_where_probe_is_inlined(self):
        symbols = self.complete_engine_symbols("hl-10210")
        del symbols["DT_Initialize"]
        errors = validate.validate_renderer(symbols, "hl-10210")
        self.assertTrue(any("DT_Initialize" in e for e in errors), errors)

    def test_gate_requires_mod_unloadspritetextures_on_every_identity(self):
        for gv in validate.RENDERER_ALL_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols["Mod_UnloadSpriteTextures"]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("Mod_UnloadSpriteTextures" in e for e in errors), (gv, errors))
        self.assertNotIn("Mod_UnloadSpriteTextures", validate.RENDERER_ENGINE_E8_FUNCTIONS)

    def test_gate_requires_rgba_fill_bodies_on_every_identity(self):
        for name in ("Draw_FillRGBA", "Draw_FillRGBABlend"):
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[name]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))
            self.assertNotIn(name, validate.RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS)

    def test_gate_requires_draw_fillrgbabuf_on_svengine_only(self):
        for gv in validate.RENDERER_SVENGINE_GAMES:
            symbols = self.complete_engine_symbols(gv)
            self.assertIn("Draw_FillRGBABuf", symbols)
            del symbols["Draw_FillRGBABuf"]
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
            self.assertNotIn("D_FillRect", symbols)
            self.assertEqual([], validate.validate_renderer(symbols, gv))
        for gv in validate.RENDERER_NON_SVENGINE_GAMES:
            symbols = self.complete_engine_symbols(gv)
            del symbols["D_FillRect"]
            errors = validate.validate_renderer(symbols, gv)
            self.assertTrue(any("D_FillRect" in e for e in errors), (gv, errors))

    def test_gate_requires_lightmap_and_decal_symbols_on_every_identity(self):
        names = (
            "R_TextureAnimation",
            "d_lightstylevalue", "frustum", "gDecalSurfCount",
            "lightmaps", "rtable",
        )
        for name in names:
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[name]
                errors = validate.validate_renderer(symbols, gv)
                self.assertTrue(any(name in e for e in errors), (name, gv, errors))

    def test_gate_requires_screen_filter_globals_on_every_identity(self):
        names = ("filterBrightness", "filterColorBlue", "filterColorGreen",
                 "filterColorRed", "filterMode")
        for name in names:
            self.assertIn(name, validate.RENDERER_ENGINE_ALL_GLOBALS)
            for gv in validate.RENDERER_ALL_GAMES:
                symbols = self.complete_engine_symbols(gv)
                del symbols[name]
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
                self.assertNotIn(name, symbols, (gv, name))
            self.assertEqual([], validate.validate_renderer(symbols, gv), gv)

    def test_gate_alias_poly_counter_follows_engine_family(self):
        symbols = self.complete_engine_symbols("hl-8684")
        self.assertIn("c_alias_polys", symbols)
        self.assertNotIn("c_model_polys", symbols)
        symbols = self.complete_engine_symbols("svencoop-10257")
        self.assertIn("c_model_polys", symbols)
        self.assertNotIn("c_alias_polys", symbols)

    def test_gate_requires_client_virtuals_for_client_games(self):
        symbols = self.complete_client_symbols("hl-8684")
        del symbols["GameStudioRenderer_StudioSetupBones"]
        errors = validate.validate_renderer(symbols, "hl-8684", include_engine=False)
        self.assertTrue(any("GameStudioRenderer_StudioSetupBones" in e for e in errors), errors)

    def test_gate_does_not_require_client_virtuals_without_client_module(self):
        symbols = self.complete_client_symbols("hl-4554")
        self.assertEqual({}, symbols)
        self.assertEqual([], validate.validate_renderer(symbols, "hl-4554", include_engine=False))


if __name__ == "__main__":
    unittest.main()
