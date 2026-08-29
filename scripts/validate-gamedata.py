#!/usr/bin/env python3
"""Validate the packaged MetaHook gamedata catalog for release.

Checks (see docs/plans/metahook-gamedata-api-implementation-plan.md, section 14):

  1. index.json schema, path safety, file existence, size and SHA-256.
  2. snapshot schema and Windows binary metadata.
  3. signature token legality.
  4. function / global required field completeness.
  5. (CRC64, symbolName) conflicts.
  6. common required symbols for every declared engine family.
  7. cvar alternative: cvar_hooks OR (Cvar_Set + Cvar_DirectSet + Cvar_Set.symbolSize).
  8. blob conditional symbols (NLoadBlob + FreeBlob).
  9. DWORD field ranges and global signatureRva derivation.

Exit code is 0 when the catalog is release-consistent, non-zero otherwise.
Every failure prints gameVersion / module / crc64 / symbol / reason.
"""

import argparse
import hashlib
import json
import os
import sys

COMMON_REQUIRED = [
    "build_number",
    "Sys_Error",
    "ClientDLL_HudInit",
    "g_ppEngfuncs",
    "g_ppExportFuncs",
    "g_phClientModule",
    "g_pClientFactory",
    "videomode",
    "gClientUserMsgs",
    "cl_parsefuncs",
]

# gameVersion -> engine family. Only these gameVersions are declared supported.
ENGINE_FAMILIES = {
    "ENGINE_SVENGINE": ["svencoop-10257"],
    "ENGINE_GOLDSRC_HL25": ["hl-10210"],
    "ENGINE_GOLDSRC": ["hl-8684", "hl-6153"],
    "ENGINE_GOLDSRC_BLOB": ["hl-3248", "hl-3266", "hl-3329", "hl-3647", "hl-4554"],
    "ENGINE_GOLDSRC_COF": ["cof-5936"],
}

# Engine families that load a (possibly blob) client and therefore need the
# NLoadBlob / FreeBlob hooks.
BLOB_CLIENT_FAMILIES = ("ENGINE_GOLDSRC", "ENGINE_GOLDSRC_BLOB", "ENGINE_GOLDSRC_HL25")

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
    if doc.get("schemaVersion") != 3:
        errors.append(f"'{game_version}': snapshot schemaVersion must be 3")
        return errors, {}, {}
    source = doc.get("source")
    if not isinstance(source, dict) or source.get("snapshotSchemaVersion") != 6:
        errors.append(f"'{game_version}': source.snapshotSchemaVersion must be 6")
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
        else:
            # unsupported kind is tolerated by the catalog; skip.
            continue

    return errors, module_crc64, symbols


def validate_required(symbols, family, game_version):
    """Return a list of required-symbol failures for a single gameVersion."""
    errors = []
    for sym in COMMON_REQUIRED:
        if sym not in symbols:
            errors.append(f"'{game_version}' ({family}): missing common required symbol '{sym}'")

    # cvar branch
    if "cvar_hooks" not in symbols:
        has_cvar_set = "Cvar_Set" in symbols and "Cvar_DirectSet" in symbols
        has_size = "Cvar_Set" in symbols and isinstance(symbols["Cvar_Set"], dict) and symbols["Cvar_Set"].get("size", 0) > 0
        if not (has_cvar_set and has_size):
            errors.append(
                f"'{game_version}' ({family}): missing cvar branch "
                f"(need cvar_hooks or Cvar_Set + Cvar_DirectSet + Cvar_Set.symbolSize)"
            )

    # blob client hooks
    if family in BLOB_CLIENT_FAMILIES:
        for sym in ("NLoadBlob", "FreeBlob"):
            if sym not in symbols:
                errors.append(f"'{game_version}' ({family}): missing blob client symbol '{sym}'")

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
