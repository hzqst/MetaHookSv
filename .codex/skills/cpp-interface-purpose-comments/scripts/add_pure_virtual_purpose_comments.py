#!/usr/bin/env python3
"""Add a standard Purpose comment above pure virtual method declarations in C++ headers."""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

DEFAULT_COMMENT = "/* Purpose: brief and notes */"
PURE_VIRTUAL_RE = re.compile(r"\bvirtual\b.*?\(.*?\).*?=\s*0\s*;", re.S)


def strip_comments(line: str, in_block: bool) -> tuple[str, bool]:
    code_parts: list[str] = []
    i = 0
    while i < len(line):
        if in_block:
            end = line.find("*/", i)
            if end == -1:
                return "".join(code_parts), True
            i = end + 2
            in_block = False
            continue
        start_block = line.find("/*", i)
        start_line = line.find("//", i)
        if start_line != -1 and (start_block == -1 or start_line < start_block):
            code_parts.append(line[i:start_line])
            return "".join(code_parts), False
        if start_block == -1:
            code_parts.append(line[i:])
            return "".join(code_parts), False
        code_parts.append(line[i:start_block])
        i = start_block + 2
        in_block = True
    return "".join(code_parts), in_block


def has_comment_above(lines: list[str], idx: int) -> bool:
    j = idx - 1
    while j >= 0 and lines[j].strip() == "":
        j -= 1
    if j < 0:
        return False
    stripped = lines[j].lstrip()
    return (
        stripped.startswith("//")
        or stripped.startswith("/*")
        or stripped.startswith("*")
        or stripped.startswith("*/")
    )


def collect_insert_positions(lines: list[str]) -> list[int]:
    positions: list[int] = []
    in_block = False
    in_statement = False
    statement = ""
    start_idx = -1
    for i, line in enumerate(lines):
        code, in_block = strip_comments(line, in_block)
        if not in_statement:
            if "virtual" in code:
                in_statement = True
                start_idx = i
                statement = code
                if ";" in code:
                    if PURE_VIRTUAL_RE.search(statement):
                        positions.append(start_idx)
                    in_statement = False
                    statement = ""
                    start_idx = -1
        else:
            statement += code
            if ";" in code:
                if PURE_VIRTUAL_RE.search(statement):
                    positions.append(start_idx)
                in_statement = False
                statement = ""
                start_idx = -1
    return positions


def iter_target_files(paths: list[Path], globs: list[str]) -> list[Path]:
    files: set[Path] = set()
    for path in paths:
        if path.is_file():
            files.add(path)
            continue
        if path.is_dir():
            for pattern in globs:
                files.update(path.rglob(pattern))
            continue
        print(f"Skip missing path: {path}", file=sys.stderr)
    return sorted(files, key=lambda p: str(p))


def process_file(
    path: Path,
    comment: str,
    dry_run: bool,
    force: bool,
    encoding: str,
) -> int:
    text = path.read_text(encoding=encoding)
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    if not lines:
        return 0

    raw_positions = collect_insert_positions(lines)
    seen: set[int] = set()
    insert_positions: list[int] = []
    for idx in raw_positions:
        if idx in seen:
            continue
        seen.add(idx)
        if not force and has_comment_above(lines, idx):
            continue
        insert_positions.append(idx)

    if not insert_positions:
        return 0

    insert_set = set(insert_positions)
    output: list[str] = []
    for i, line in enumerate(lines):
        if i in insert_set:
            indent = re.match(r"\s*", line).group(0)
            output.append(indent + comment + newline)
        output.append(line)

    if not dry_run:
        with path.open("w", encoding=encoding, newline="") as handle:
            handle.write("".join(output))
    return len(insert_positions)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Add Purpose comments above pure virtual methods in C++ headers.",
    )
    parser.add_argument("paths", nargs="+", help="Files or directories to process")
    parser.add_argument(
        "--globs",
        default="*.h",
        help="Comma-separated glob patterns for recursive scans (default: *.h)",
    )
    parser.add_argument(
        "--comment",
        default=DEFAULT_COMMENT,
        help="Comment text to insert",
    )
    parser.add_argument(
        "--encoding",
        default="utf-8",
        help="File encoding (default: utf-8)",
    )
    parser.add_argument("--dry-run", action="store_true", help="Do not write files")
    parser.add_argument(
        "--force",
        action="store_true",
        help="Insert even if a comment exists immediately above",
    )
    args = parser.parse_args()

    globs = [item.strip() for item in args.globs.split(",") if item.strip()]
    paths = [Path(item) for item in args.paths]
    targets = iter_target_files(paths, globs)
    if not targets:
        print("No files matched.", file=sys.stderr)
        return 1

    total_added = 0
    files_changed = 0
    if args.dry_run:
        print("DRY RUN: no files will be written.")

    for path in targets:
        try:
            added = process_file(
                path=path,
                comment=args.comment,
                dry_run=args.dry_run,
                force=args.force,
                encoding=args.encoding,
            )
        except Exception as exc:  # pragma: no cover - error path reporting
            print(f"Failed: {path} ({exc})", file=sys.stderr)
            continue
        if added:
            files_changed += 1
            total_added += added
            print(f"{path}: +{added}")

    print(f"Done. Files changed: {files_changed}, comments added: {total_added}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
