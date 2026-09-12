#!/usr/bin/env python3
"""Validate the packaged MetaHook gamedata catalog for release.

Checks (see docs/plans/metahook-gamedata-api-implementation-plan.md, section 14):

  1. index.json schema, path safety, file existence, size and SHA-256.
  2. snapshot schema and Windows binary metadata.
  3. signature token legality.
  4. function / global / patch required field completeness.
  5. (CRC64, symbolName) conflicts.
  6. common required symbols (expected kind) and numbered patch sets for every
     declared engine family.
  7. cvar alternative: cvar_hooks (global) OR Cvar_Set_to_Cvar_DirectSet_callsite_0 (patch).
  8. blob conditional symbols (NLoadBlob + FreeBlob), and the SvEngine pairing rule.
  9. DWORD field ranges and global signatureRva derivation.

Exit code is 0 when the catalog is release-consistent, non-zero otherwise.
Every failure prints gameVersion / module / crc64 / symbol / reason.
"""

import argparse
import hashlib
import json
import os
import sys

# symbol -> expected record kind, required for every declared engine family.
COMMON_REQUIRED = {
    "build_number": "function",
    "Sys_Error": "function",
    "ClientDLL_HudInit": "function",
    "cl_enginefuncs": "global",
    "cl_funcs": "global",
    "g_phClientModule": "global",
    "g_pClientFactory": "global",
    "videomode": "global",
    "gClientUserMsgs": "global",
    "cl_parsefuncs": "global",
    "Cvar_DirectSet": "function",
    # ResourceReplacer (plugin resolves these via ResolveGameSymbol)
    "FS_Open": "function",
    "CL_PrecacheResources": "function",
    # PrecacheManager (plugin resolves this via ResolveGameSymbol)
    "cl_resourcesonhand": "global",
    # SCModelDownloader (plugin resolves these via ResolveGameSymbol)
    "R_StudioDrawPlayer": "function",
    "studioapi_SetupPlayerModel": "function",
    "Host_IsSinglePlayerGame": "function",
    "DM_PlayerState": "global",
    "cl_players_model": "global",
    # ThreadGuard (plugin resolves the engine IEngine* slot via ResolveGameSymbol)
    "engine": "global",
}

# Numbered patch sets required for every declared engine family. Each set is
# numbered contiguously from 0 and must contain at least index 0.
NUMBERED_PATCH_SETS = (
    # HeapPatch
    "Sys_InitMemory_HeapLimitPatches",
    # ResourceReplacer
    "S_LoadSound_to_FS_Open_callsite",
    "Mod_LoadModel_to_FS_Open_callsite",
)

# scalar -> owning module, required for every declared engine family. Scalars
# are plain uint32 values consumed verbatim (no image base, no dereference).
REQUIRED_SCALARS = {
    "size_of_frame": "engine",
}

# BulletPhysics consumer gate. Engine-side private symbols are required for
# every declared engine family; client-side symbols only for game versions
# whose snapshot publishes a client module. The six engine-only builds
# (hl-3248/3266/3329/3647/4554/6153) publish no client module, so the client
# Studio path cannot be gamedata-only for them and is intentionally excluded.
BULLETPHYSICS_ENGINE_FUNCTIONS = (
    "R_NewMap",
    "R_RenderView",
    "V_RenderView",
    "R_CullBox",
    "R_StudioDrawModel",
    "R_StudioDrawPlayer",
    "R_StudioSetupBones",
)
BULLETPHYSICS_ENGINE_GLOBALS = (
    "cl_max_edicts",
    "cl_entities",
    "gTempEnts",
    "cl_viewentity",
    "mod_known",
    "mod_numknown",
    "cl_frames",
    "cl_parsecount",
    "cl_numvisedicts",
    "cl_visedicts",
    "r_worldentity",
    "cl_worldmodel",
    "currententity",
    "pstudiohdr",
    "r_origin",
)
BULLETPHYSICS_SVENGINE_GLOBALS = ("allow_cheats",)

# gameVersion -> snapshot that publishes a client module.
BULLETPHYSICS_CLIENT_GAMES = (
    "svencoop-10257",
    "hl-8684",
    "hl-10210",
    "cof-5936",
    "cstrike-8684",
    "cstrike-10210",
    "czero-8684",
    "czero-10210",
    "czeror-8684",
    "czeror-10210",
)
BULLETPHYSICS_CLIENT_GLOBALS = ("g_iUser1", "g_iUser2")
# Resolved with required=false: clients without the client studio renderer keep the
# engine-side hooks only, so these are validated for kind when present, never for presence.
BULLETPHYSICS_CLIENT_OPTIONAL_VFUNCS = (
    "GameStudioRenderer_StudioDrawModel",
    "GameStudioRenderer_StudioDrawPlayer",
    "GameStudioRenderer_StudioSetupBones",
)
BULLETPHYSICS_SVEN_CLIENT_GLOBALS = (
    "g_bRenderingPortals_SCClient",
    "g_ViewEntityIndex_SCClient",
    "g_pitchdrift",
)
BULLETPHYSICS_CS_CLIENT_GAMES = (
    "cstrike-8684",
    "cstrike-10210",
    "czero-8684",
    "czero-10210",
)
BULLETPHYSICS_CZDS_CLIENT_GAMES = ("czeror-8684", "czeror-10210")

# gameVersion -> engine family. Only these gameVersions are declared supported.
# hl-4554 belongs to ENGINE_GOLDSRC: its hw.dll is a plain PE (isBlob false) with
# build number 4554 <= 9000, so the launcher's rule-based mapping reports
# ENGINE_GOLDSRC for it. Both families require the same blob symbols, so the
# gate result is unchanged.
ENGINE_FAMILIES = {
    "ENGINE_SVENGINE": ["svencoop-10257"],
    "ENGINE_GOLDSRC_HL25": ["hl-10210"],
    "ENGINE_GOLDSRC": ["hl-8684", "hl-6153", "hl-4554"],
    "ENGINE_GOLDSRC_BLOB": ["hl-3248", "hl-3266", "hl-3329", "hl-3647"],
    "ENGINE_GOLDSRC_COF": ["cof-5936"],
}

# Engine families that load a (possibly blob) client and therefore need the
# NLoadBlob / FreeBlob hooks.
BLOB_CLIENT_FAMILIES = ("ENGINE_GOLDSRC", "ENGINE_GOLDSRC_BLOB", "ENGINE_GOLDSRC_HL25", "ENGINE_GOLDSRC_COF")

# gameVersion -> engine family lookup.
GAME_TO_FAMILY = {}
for family, games in ENGINE_FAMILIES.items():
    for game in games:
        GAME_TO_FAMILY[game] = family


def fail(*args):
    print("ERROR:", *args, file=sys.stderr)


def is_lower_hex(s, length):
    return isinstance(s, str) and len(s) == length and all(c in "0123456789abcdef" for c in s)


def is_safe_filename(s):
    if not isinstance(s, str) or not s:
        return False
    if s in (".", ".."):
        return False
    if ".." in s:
        return False
    for c in s:
        if c in "/\\:":
            return False
        if ord(c) < 0x20:
            return False
    return True


def parse_hex_u32(s):
    if not isinstance(s, str) or not s:
        return None
    s = s.strip()
    if s.lower().startswith("0x"):
        s = s[2:]
    if not s:
        return None
    try:
        v = int(s, 16)
    except ValueError:
        return None
    if v > 0xFFFFFFFF:
        return None
    return v


def parse_crc64(s):
    if not isinstance(s, str) or len(s) != 16:
        return None
    try:
        return int(s, 16)
    except ValueError:
        return None


def validate_signature(sig):
    if not isinstance(sig, str) or not sig:
        return False
    tokens = sig.split()
    if not tokens:
        return False
    for t in tokens:
        if t == "??":
            continue
        if len(t) == 2 and all(c in "0123456789abcdefABCDEF" for c in t):
            continue
        return False
    return True


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def validate_index(index):
    errors = []
    if index.get("schemaVersion") != 4:
        errors.append("index schemaVersion must be 4")
        return errors
    versions = index.get("versions")
    if not isinstance(versions, list):
        errors.append("index 'versions' must be an array")
        return errors
    seen = set()
    for v in versions:
        gv = v.get("gameVersion")
        url = v.get("url")
        sha = v.get("sha256")
        size = v.get("size")
        if not isinstance(gv, str) or not gv:
            errors.append("version entry missing gameVersion")
            continue
        if gv in seen:
            errors.append(f"duplicate gameVersion '{gv}'")
        seen.add(gv)
        if not is_safe_filename(url):
            errors.append(f"'{gv}': unsafe url '{url}'")
        if not is_lower_hex(sha, 64):
            errors.append(f"'{gv}': invalid sha256")
        if not isinstance(size, int) or size < 0:
            errors.append(f"'{gv}': invalid size")
        if not isinstance(v.get("snapshotSchemaVersion"), int):
            errors.append(f"'{gv}': missing snapshotSchemaVersion")
    return errors


def validate_snapshot(doc, game_version):
    """Return (errors, module_crc64: dict[str, int], symbol_records: dict)."""
    errors = []
    if doc.get("schemaVersion") != 5:
        errors.append(f"'{game_version}': snapshot schemaVersion must be 5")
        return errors, {}, {}
    source = doc.get("source")
    if not isinstance(source, dict) or source.get("snapshotSchemaVersion") != 8:
        errors.append(f"'{game_version}': source.snapshotSchemaVersion must be 8")
        return errors, {}, {}
    if source.get("analysisOutputContractVersion") != 3:
        errors.append(f"'{game_version}': source.analysisOutputContractVersion must be 3")
        return errors, {}, {}

    binaries = doc.get("binaries")
    if not isinstance(binaries, dict):
        errors.append(f"'{game_version}': missing binaries object")
        return errors, {}, {}

    module_crc64 = {}
    for mod, plats in binaries.items():
        if not isinstance(plats, dict):
            continue
        win = plats.get("windows")
        if not isinstance(win, dict):
            continue
        crc64 = parse_crc64(win.get("crc64"))
        size = win.get("size")
        sha = win.get("sha256")
        if crc64 is None:
            errors.append(f"'{game_version}': module '{mod}': invalid crc64")
        if not isinstance(size, int) or size < 0:
            errors.append(f"'{game_version}': module '{mod}': invalid size")
        if not is_lower_hex(sha, 64):
            errors.append(f"'{game_version}': module '{mod}': invalid sha256")
        if not isinstance(win.get("isBlob"), bool):
            errors.append(f"'{game_version}': module '{mod}': isBlob must be a boolean")
        if crc64 is not None:
            module_crc64[mod] = crc64

    records = doc.get("records")
    if not isinstance(records, list):
        errors.append(f"'{game_version}': missing records array")
        return errors, module_crc64, {}

    symbols = {}  # symbolName -> record (windows only)
    for rec in records:
        if not isinstance(rec, dict):
            continue
        if rec.get("platform") != "windows":
            continue
        mod = rec.get("module")
        name = rec.get("symbolName")
        kind = rec.get("kind")
        payload = rec.get("payload")
        if not isinstance(mod, str) or not isinstance(name, str) or not isinstance(kind, str):
            continue
        if mod not in module_crc64:
            errors.append(f"'{game_version}': record '{name}' references unknown module '{mod}'")
            continue

        key = (module_crc64[mod], name)
        if kind == "function":
            p = payload if isinstance(payload, dict) else {}
            rva = parse_hex_u32(p.get("func_rva"))
            size = parse_hex_u32(p.get("func_size"))
            sig = p.get("func_sig")
            if rva is None or size is None or not isinstance(sig, str):
                errors.append(f"'{game_version}': function '{name}' missing/invalid func_rva/func_size/func_sig")
                continue
            if not validate_signature(sig):
                errors.append(f"'{game_version}': function '{name}' has a malformed signature")
                continue
            rec = {"kind": "function", "rva": rva, "size": size}
            if name in symbols and symbols[name] != rec:
                errors.append(f"'{game_version}': conflicting duplicate symbol '{name}'")
            symbols[name] = rec
        elif kind == "global":
            p = payload if isinstance(payload, dict) else {}
            gv_rva = parse_hex_u32(p.get("gv_rva"))
            gv_va = parse_hex_u32(p.get("gv_va"))
            gv_sig_va = parse_hex_u32(p.get("gv_sig_va"))
            sig = p.get("gv_sig")
            inst_off = parse_hex_u32(p.get("gv_inst_offset"))
            inst_disp = parse_hex_u32(p.get("gv_inst_disp"))
            inst_len = parse_hex_u32(p.get("gv_inst_length"))
            if None in (gv_rva, gv_va, gv_sig_va, inst_off, inst_disp, inst_len) or not isinstance(sig, str):
                errors.append(f"'{game_version}': global '{name}' missing/invalid gv_* fields")
                continue
            if gv_va < gv_rva:
                errors.append(f"'{game_version}': global '{name}': gv_va < gv_rva")
                continue
            image_base = gv_va - gv_rva
            if gv_sig_va < image_base:
                errors.append(f"'{game_version}': global '{name}': gv_sig_va < image base")
                continue
            if not validate_signature(sig):
                errors.append(f"'{game_version}': global '{name}' has a malformed signature")
                continue
            rec = {"kind": "global", "rva": gv_rva, "sig_rva": gv_sig_va - image_base,
                   "inst_off": inst_off, "inst_disp": inst_disp, "inst_len": inst_len}
            if name in symbols and symbols[name] != rec:
                errors.append(f"'{game_version}': conflicting duplicate symbol '{name}'")
            symbols[name] = rec
        elif kind == "patch":
            p = payload if isinstance(payload, dict) else {}
            patch_name = p.get("patch_name")
            patch_rva = parse_hex_u32(p.get("patch_rva"))
            patch_sig = p.get("patch_sig")
            patch_sig_disp = parse_hex_u32(p.get("patch_sig_disp"))
            if (not isinstance(patch_name, str) or patch_rva is None or
                    not isinstance(patch_sig, str) or patch_sig_disp is None):
                errors.append(f"'{game_version}': patch '{name}' missing/invalid patch_name/patch_rva/patch_sig/patch_sig_disp")
                continue
            if not validate_signature(patch_sig):
                errors.append(f"'{game_version}': patch '{name}' has a malformed signature")
                continue
            rec = {"kind": "patch", "rva": patch_rva, "sig_disp": patch_sig_disp}
            if name in symbols and symbols[name] != rec:
                errors.append(f"'{game_version}': conflicting duplicate symbol '{name}'")
            symbols[name] = rec
        elif kind == "scalar":
            p = payload if isinstance(payload, dict) else {}
            scalar_name = p.get("scalar_name")
            scalar_value = p.get("scalar_value")
            if not isinstance(scalar_name, str) or not scalar_name:
                errors.append(f"'{game_version}': scalar '{name}' missing/invalid scalar_name")
                continue
            if (not isinstance(scalar_value, int) or isinstance(scalar_value, bool)
                    or not 0 <= scalar_value <= 0xFFFFFFFF):
                errors.append(f"'{game_version}': scalar '{name}' has an invalid uint32 scalar_value")
                continue
            rec = {"kind": "scalar", "value": scalar_value, "module": mod}
            if name in symbols and symbols[name] != rec:
                errors.append(f"'{game_version}': conflicting duplicate symbol '{name}'")
            symbols[name] = rec
        elif kind == "virtualFunction":
            p = payload if isinstance(payload, dict) else {}
            func_rva = parse_hex_u32(p.get("func_rva"))
            func_size = parse_hex_u32(p.get("func_size"))
            vfunc_sig = p.get("vfunc_sig")
            vfunc_index = p.get("vfunc_index")
            vtable_name = p.get("vtable_name")
            if (func_rva is None or func_size is None or not isinstance(vfunc_sig, str) or
                    not isinstance(vfunc_index, int) or isinstance(vfunc_index, bool) or
                    not isinstance(vtable_name, str) or not vtable_name):
                errors.append(f"'{game_version}': virtualFunction '{name}' missing/invalid func_rva/func_size/vfunc_sig/vfunc_index/vtable_name")
                continue
            if not validate_signature(vfunc_sig):
                errors.append(f"'{game_version}': virtualFunction '{name}' has a malformed signature")
                continue
            rec = {"kind": "virtualFunction", "rva": func_rva, "size": func_size}
            if name in symbols and symbols[name] != rec:
                errors.append(f"'{game_version}': conflicting duplicate symbol '{name}'")
            symbols[name] = rec
        else:
            # unsupported kind is tolerated by the catalog; skip.
            continue

    return errors, module_crc64, symbols


def validate_required(symbols, family, game_version):
    """Return a list of required-symbol failures for a single gameVersion."""
    errors = []
    for sym, kind in COMMON_REQUIRED.items():
        rec = symbols.get(sym)
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}' ({family}): missing common required symbol '{sym}'")
        elif rec.get("kind") != kind:
            errors.append(f"'{game_version}' ({family}): '{sym}' must be a {kind} record")

    for prefix in NUMBERED_PATCH_SETS:
        index = 0
        while True:
            name = f"{prefix}_{index}"
            rec = symbols.get(name)
            if rec is None:
                if index == 0:
                    errors.append(f"'{game_version}' ({family}): missing required patch '{name}'")
                break
            if rec.get("kind") != "patch":
                errors.append(f"'{game_version}' ({family}): '{name}' must be a patch record")
            index += 1

    for sym, module in REQUIRED_SCALARS.items():
        rec = symbols.get(sym)
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}' ({family}): missing required scalar '{sym}'")
        elif rec.get("kind") != "scalar":
            errors.append(f"'{game_version}' ({family}): '{sym}' must be a scalar record")
        elif rec.get("module") != module:
            errors.append(f"'{game_version}' ({family}): '{sym}' must belong to module '{module}'")

    # BulletPhysics engine-side consumer gate.
    for sym in BULLETPHYSICS_ENGINE_FUNCTIONS:
        rec = symbols.get(sym)
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}' ({family}): missing BulletPhysics engine function '{sym}'")
        elif rec.get("kind") != "function":
            errors.append(f"'{game_version}' ({family}): '{sym}' must be a function record")
    for sym in BULLETPHYSICS_ENGINE_GLOBALS:
        rec = symbols.get(sym)
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}' ({family}): missing BulletPhysics engine global '{sym}'")
        elif rec.get("kind") != "global":
            errors.append(f"'{game_version}' ({family}): '{sym}' must be a global record")
    if family == "ENGINE_SVENGINE":
        for sym in BULLETPHYSICS_SVENGINE_GLOBALS:
            rec = symbols.get(sym)
            if not isinstance(rec, dict):
                errors.append(f"'{game_version}' ({family}): missing BulletPhysics engine global '{sym}'")
            elif rec.get("kind") != "global":
                errors.append(f"'{game_version}' ({family}): '{sym}' must be a global record")

    # cvar branch: the engine's native callback list, or at least one managed
    # Cvar_Set -> Cvar_DirectSet call-site redirect.
    has_native = isinstance(symbols.get("cvar_hooks"), dict) and symbols["cvar_hooks"].get("kind") == "global"
    has_managed = (isinstance(symbols.get("Cvar_Set_to_Cvar_DirectSet_callsite_0"), dict) and
                   symbols["Cvar_Set_to_Cvar_DirectSet_callsite_0"].get("kind") == "patch")
    if not (has_native or has_managed):
        errors.append(
            f"'{game_version}' ({family}): missing cvar branch "
            f"(need cvar_hooks global or Cvar_Set_to_Cvar_DirectSet_callsite_0 patch)"
        )

    # blob client hooks
    if family in BLOB_CLIENT_FAMILIES:
        for sym in ("NLoadBlob", "FreeBlob"):
            if sym not in symbols:
                errors.append(f"'{game_version}' ({family}): missing blob client symbol '{sym}'")
    elif family == "ENGINE_SVENGINE":
        # SvEngine may ship without the blob client hooks, but only as a pair.
        if ("NLoadBlob" in symbols) != ("FreeBlob" in symbols):
            errors.append(
                f"'{game_version}' ({family}): NLoadBlob and FreeBlob must both be present or both absent"
            )

    return errors


def validate_bulletphysics_client(symbols, game_version):
    """Return BulletPhysics client-side consumer failures for a game version."""
    errors = []
    for sym in BULLETPHYSICS_CLIENT_GLOBALS:
        rec = symbols.get(sym)
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}': missing BulletPhysics client global '{sym}'")
        elif rec.get("kind") != "global":
            errors.append(f"'{game_version}': '{sym}' must be a global record")
    for sym in BULLETPHYSICS_CLIENT_OPTIONAL_VFUNCS:
        rec = symbols.get(sym)
        if rec is None:
            continue
        if not isinstance(rec, dict) or rec.get("kind") != "virtualFunction":
            errors.append(f"'{game_version}': '{sym}' must be a virtualFunction record")

    if game_version == "svencoop-10257":
        for sym in BULLETPHYSICS_SVEN_CLIENT_GLOBALS:
            rec = symbols.get(sym)
            if not isinstance(rec, dict):
                errors.append(f"'{game_version}': missing Sven Co-op client global '{sym}'")
            elif rec.get("kind") != "global":
                errors.append(f"'{game_version}': '{sym}' must be a global record")
    if game_version in BULLETPHYSICS_CS_CLIENT_GAMES:
        rec = symbols.get("g_PlayerExtraInfo")
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}': missing Counter-Strike client global 'g_PlayerExtraInfo'")
        elif rec.get("kind") != "global":
            errors.append(f"'{game_version}': 'g_PlayerExtraInfo' must be a global record")
        rec = symbols.get("GameStudioRenderer__StudioDrawPlayer")
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}': missing Counter-Strike client virtualFunction 'GameStudioRenderer__StudioDrawPlayer'")
        elif rec.get("kind") != "virtualFunction":
            errors.append(f"'{game_version}': 'GameStudioRenderer__StudioDrawPlayer' must be a virtualFunction record")
    if game_version in BULLETPHYSICS_CZDS_CLIENT_GAMES:
        rec = symbols.get("g_PlayerExtraInfo_CZDS")
        if not isinstance(rec, dict):
            errors.append(f"'{game_version}': missing Condition Zero client global 'g_PlayerExtraInfo_CZDS'")
        elif rec.get("kind") != "global":
            errors.append(f"'{game_version}': 'g_PlayerExtraInfo_CZDS' must be a global record")

    return errors


def main():
    parser = argparse.ArgumentParser(description="Validate packaged MetaHook gamedata")
    parser.add_argument("directory", help="path to the packaged gamedata directory")
    args = parser.parse_args()

    gamedata_dir = args.directory
    index_path = os.path.join(gamedata_dir, "index.json")
    if not os.path.isfile(index_path):
        fail("index.json missing from", gamedata_dir)
        return 1

    try:
        with open(index_path, "r", encoding="utf-8") as f:
            index = json.load(f)
    except (OSError, ValueError) as e:
        fail("failed to parse index.json:", e)
        return 1

    all_errors = validate_index(index)

    # Collect per-gameVersion symbols.
    game_symbols = {}
    declared_files = {"index.json"}
    for v in index.get("versions", []):
        gv = v.get("gameVersion")
        url = v.get("url")
        if not isinstance(gv, str) or not isinstance(url, str):
            continue
        declared_files.add(url)
        snap_path = os.path.join(gamedata_dir, url)
        if not os.path.isfile(snap_path):
            all_errors.append(f"'{gv}': snapshot file missing: {url}")
            continue
        actual_size = os.path.getsize(snap_path)
        if actual_size != v.get("size"):
            all_errors.append(f"'{gv}': size mismatch (expected {v.get('size')}, got {actual_size})")
        if v.get("sha256") is not None and sha256_file(snap_path) != v["sha256"]:
            all_errors.append(f"'{gv}': sha256 mismatch")
        try:
            with open(snap_path, "r", encoding="utf-8") as f:
                doc = json.load(f)
        except (OSError, ValueError) as e:
            all_errors.append(f"'{gv}': failed to parse snapshot: {e}")
            continue
        errs, module_crc64, symbols = validate_snapshot(doc, gv)
        all_errors.extend(errs)
        game_symbols[gv] = (module_crc64, symbols)

    # Undeclared files in the directory.
    for name in os.listdir(gamedata_dir):
        if name not in declared_files:
            all_errors.append(f"undeclared file in gamedata directory: {name}")

    # Required-symbol coverage per engine family.
    for family, games in ENGINE_FAMILIES.items():
        for gv in games:
            if gv not in game_symbols:
                all_errors.append(f"'{gv}' ({family}): snapshot not loaded")
                continue
            symbols = game_symbols[gv][1]
            all_errors.extend(validate_required(symbols, family, gv))

    # BulletPhysics client-side consumer gate (only for client-bearing games).
    for gv in BULLETPHYSICS_CLIENT_GAMES:
        if gv not in game_symbols:
            all_errors.append(f"'{gv}': snapshot not loaded (BulletPhysics client gate)")
            continue
        symbols = game_symbols[gv][1]
        all_errors.extend(validate_bulletphysics_client(symbols, gv))

    if all_errors:
        for e in all_errors:
            fail(e)
        print(f"\n{len(all_errors)} gamedata validation error(s).", file=sys.stderr)
        return 1

    print(f"gamedata validation passed for {gamedata_dir} "
          f"({len(game_symbols)} snapshots, {len(ENGINE_FAMILIES)} engine families).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
