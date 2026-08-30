---
title: private_symbols_disasm_workflow
type: note
permalink: metahooksv/private-symbols-disasm-workflow
---

# Locating Unexported Private Functions/Variables Through a Disassembly Engine: General Workflow (MetaHookSv)

Objective:
- Reliably locate private functions and global variables inside engine/client DLLs (including vftables, array base addresses, and structure pointers) when they are **not exported** (no symbols/export table), then write the results to `gPrivateFuncs` or global pointers for subsequent hooks/patches.

Applicable code references (typical implementations in this repository):
- `Plugins/Renderer/gl_hooks.cpp`: `Engine_FillAddress_R_RenderView`, `Engine_FillAddress_RenderSceneVars`
- `Plugins/BulletPhysics/privatehook.cpp`: `Engine_FillAddress_CL_ReallocateDynamicData`, `Engine_FillAddress_VisEdicts`, `Engine_FillAddress_CL_ViewEntityVars`
- `Plugins/VGUI2Extension/privatefuncs.cpp`: `VGUI2_FindMenuVFTable`, `VGUI2_FindKeyValueVFTable`, `Engine_FillAddress_Sys_InitializeGameDLL`

---

## 0) Prerequisite Inputs and Constraints

1) Two module-information structures (**critical**):
- `DllInfo`: Address space for **signature/string** scanning (usually the “scanning/analysis space”).
- `RealDllInfo`: Address space for **runtime calls/hooks** (actual loaded base address).

2) You must be able to distinguish section boundaries:
- `.text`: `TextBase/TextSize`
- `.data`: `DataBase/DataSize`
- `.rdata`: `RdataBase/RdataSize`

3) Account for engine/version differences in advance:
- Typically, branch on `g_iEngineType`: `ENGINE_SVENGINE` / `ENGINE_GOLDSRC` / `ENGINE_GOLDSRC_BLOB` / `ENGINE_GOLDSRC_HL25`.

---

## 1) Choose an Anchor

Priority from highest to lowest (earlier options are more reliable):

### A. Highly Unique Strings (Recommended)
- Find **log/error strings** in `.data/.rdata` (such as `"R_RenderView: NULL worldmodel"`, `"CL_Reallocate cl_entities\n"`, and `"Sys_InitializeGameDLL called twice"`).
- Locate the string address with `Search_Pattern_Data` / `Search_Pattern_Rdata`.

### B. String-Xref Code Patterns
- Construct a small “push string address + call” signature; write the string address into the pattern's immediate-value field, then call `Search_Pattern(pattern, DllInfo)`.
- Examples: `push <str>; call <...>`, or `75 ?? 68 <str>` with a conditional jump.

### C. Pure `.text` Signatures (Partitioned by Engine Type)
- Maintain different signature constants for different engines (SVEngine / HL25 / GoldSrc / Blob), with multiple fallback sets.
- Examples: `Engine_FillAddress_R_RenderView` / `Engine_FillAddress_R_DrawTEntitiesOnList`.

### D. “Structure Write” Anchors (vftable/array writes, etc.)
- First use strings to find the vicinity of construction/initialization, then identify the pattern through disassembly:
  - vftable write: `mov [this], imm`, where `imm` lies in `.rdata` and vftable[0] points to `.text`.
  - Array write: use `disp` in `mov [disp + index*scale], reg` as the array base address.

---

## 2) Recover the Function Entry From the Anchor (Function Begin Recovery)

When the anchor is only an instruction/call site in the middle of a function:
- Search backward for the function prologue with `g_pMetaHookAPI->ReverseSearchFunctionBeginEx(anchor, max_back, predicate)`.

Common predicate patterns (polymorphic across compilers/versions):
- Standard prologue: `55 8B EC`
- Variant: `53 8B DC` (push ebx; mov ebx, esp)
- Variant: `8B 44 24 ?? 83 EC ??` (prologue without `push ebp`)
- Special instructions at the start of specific functions can also be used for detection (for example, beginning with `D9 05 ...`).

---

## 3) Disassembly Scanning (DisasmRanges / DisasmSingleInstruction)

Core approach:
- Call `g_pMetaHookAPI->DisasmRanges(start, len, callback, ..., ctx)` on a **candidate function entry** or **the area around an anchor**.
- Perform semantic matching in the callback using Capstone's `cs_insn` (this project uses `pinst->id` + `pinst->detail->x86.operands[...]`).
- Return `TRUE` as soon as the target is found to stop scanning; also use `RET`, `0xCC`, or an `instCount` limit as termination conditions.

Common scan windows:
- Small window: 0x30~0x100 (locating a single variable/short call chain)
- Medium window: 0x150~0x500 (locating multiple variables or across basic blocks)

---

## 4) Extract the “Target Address” From Instruction Semantics

The following criteria recur across the three plugins and can be reused directly:

### A. Extract Call Targets (Private Function Addresses)
- Identify `E8 rel32` (or Capstone `X86_INS_CALL`) and use:
  - `GetCallAddress(address)`, or
  - `pinst->detail->x86.operands[0].imm` (the project also directly uses imm as the call target).
- Common usage: first find a `call` near the anchor, then treat its call target as a “candidate private function.”

### B. Extract Global Variable Addresses (Absolute disp / imm)
- Identify `mov reg, [disp]`, `cmp [disp], imm`, `push [disp]`, and similar instructions:
  - `operands[*].type == X86_OP_MEM` and `mem.base == 0 && mem.index == 0` (absolute address)
  - Validate that `mem.disp` lies in `DllInfo.DataBase..DataBase+DataSize` (global variables are usually in `.data`)
- Stronger semantic validation sometimes incorporates **following bytes** (to reduce false positives):
  - For example, `memcmp(address + instLen, "\x83\xC4\x04", 3)` checks that the next instruction is `add esp, 4`.

### C. Extract Array Base Addresses (Writes With index*scale)
- Identify patterns such as `mov [disp + ecx*4], eax/edx`:
  - `operands[0].mem.index == X86_REG_ECX && mem.scale == 4`
  - `mem.disp` is the array base address (such as `cl_visedicts`).

### D. Locate vftables (C++ Object Virtual Tables)
- Common pattern: `mov [this + 0], imm` in a constructor.
- Determination order:
  1) `imm` lies within `.rdata` (vtable constants are in rdata)
  2) Treat `imm` as `PVOID* vftable` and require that `vftable[0]` points to `.text` (virtual function entries are in text)
- Examples: `VGUI2_FindMenuVFTable`, `VGUI2_FindKeyValueVFTable`.

### E. Infer Structure Pointers From “push Immediate Arguments”
- Observe the `push imm; call ...` form; `imm` is often the address of a global structure.
- Example: `Engine_FillAddress_RenderSceneVars` extracts the `refdef` pointer from the `push` before a call and maps it to different structure layouts according to engine type.

---

## 5) Address-Space Mapping: `ConvertDllInfoSpace`

Addresses obtained during scanning typically belong to the `DllInfo` space, while actual use/hooks require the `RealDllInfo` space:
- `ConvertDllInfoSpace(addr, DllInfo, RealDllInfo)`:
  - `RVA = RVA_from_VA(addr, SrcDllInfo)`
  - `VA = VA_from_RVA(RVA, TargetDllInfo)`

The reverse direction is also used:
- For example, when `gPrivateFuncs.R_RenderScene` (Real space) is known but scanning must continue in the `DllInfo` space, first call `ConvertDllInfoSpace(real, RealDllInfo, DllInfo)`.

---

## 6) Fallbacks, Caching, and Maintainability

1) Caching:
- Each `Engine_FillAddress_*` entry starts with `if (gPrivateFuncs.xxx) return;` to avoid repeated scans.

2) Recommended fallback order:
- String anchors (stable) -> pure signatures (by engine type) -> broader disassembly semantic matching (costly).

3) False-positive control:
- Range-check every “address extracted from disassembly” (text/data/rdata).
- Use an `instCount` window limit (for example, allow matches only within the first N instructions).
- Use neighboring bytes/immediate-value semantics (such as the immediate value of a cmp) for secondary validation.

4) Scan termination conditions:
- Target hit / `RET` / `0xCC` / `instCount` limit exceeded.

5) Result validation and error reporting:
- This repository conventionally calls `Sig_FuncNotFound(...)` / `Sig_VarNotFound(...)` / `Sig_AddrNotFound(...)` after population for assertions/logging.

---

## 7) Practical Use (Typical Call Chain)

- `Engine_FillAddress(...)` aggregates calls to multiple `Engine_FillAddress_*` functions:
  - First locate private functions/global variables -> convert to runtime space -> write to `gPrivateFuncs` / global pointers.
- The addresses are then used in `*_InstallHooks()` to install inline hooks, vft hooks, and so on (decoupled from the locating workflow in this memory note).
