---
name: cpp-interface-purpose-comments
description: Add a standard Purpose comment above each pure virtual method declaration in C++ interface header files (.h/.hpp). Use when asked to batch annotate interface classes or enforce a consistent comment block above `virtual ... = 0;` declarations.
---

# Cpp Interface Purpose Comments

## Overview
Add comment with following template:

```cpp
/* 
    Purpose : (mandatory, brief or remark or description, merge with existing if there is)
    Notes : (optional, don't emit this line if empty, merge with existing if there is) 
    Args : (optional, don't emit this line if empty, merge with existing if there is) 
    Return : (optional, don't emit this line if empty, merge with existing if there is) 
*/
```

directly above each pure virtual method declaration in C++ interface headers.

 - Prefer reading knowledges about specified functions from serena's memories first.
 - Prefer multi-line comments.
 - Keep everything as **SHORT** as possible.
 - The comments should **ALWAYS** be in English.
 - If there is **ALREADY** Purpose or comment for the interfaces there, use the existing purpose or comment instead of generating our own.

## Workflow
1. Confirm scope: target files or directories, desired comment text, and whether to skip existing comments.
2. Prefer scripted pass using `scripts/add_pure_virtual_purpose_comments.py` with `--dry-run` first.
3. Review changes and handle any missed edge cases with manual edits.
4. Package the skill when stable.

## Scripted workflow (preferred)
Run a dry run:
```
python scripts/add_pure_virtual_purpose_comments.py --dry-run --globs "*.h,*.hpp" <path>
```

Apply edits:
```
python scripts/add_pure_virtual_purpose_comments.py --globs "*.h,*.hpp" <path>
```

### Script behavior
- Detect statements containing `virtual` and `= 0;` with a function-like `(...)` pattern.
- Support multi-line declarations until the first `;`.
- Skip insertion if the nearest non-empty line above is a comment, unless `--force`.
- Preserve existing line endings and indentation.

### Parameters
- `--comment`: override the default comment text.
- `--globs`: comma-separated patterns for recursive directory scans.
- `--dry-run`: report changes without writing files.
- `--force`: insert even if a comment exists immediately above.

## Manual fallback
- Insert the comment directly above the first line of the declaration using the same indentation.
- For macro-heavy or generated code, prefer targeted edits to avoid unintended changes.
